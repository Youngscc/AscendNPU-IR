"""PlanMemory-before IR parser for local UB buffer extraction.

This parser is intentionally narrow: B1 only needs the information that
PlanMemory will later consume for local UB allocation records.  It does not
attempt to build a full MLIR AST.
"""

from __future__ import annotations

from dataclasses import dataclass, replace
from pathlib import Path
import math
import re
from typing import Iterable


_ALLOC_RE = re.compile(
    r"^\s*(?P<ssa>%[\w$.#-]+)\s*=\s*memref\.alloc\(\)\s*"
    r"(?P<attrs>\{.*?\})?\s*:\s*(?P<type>memref<.*>)\s*$"
)
_ALIGN_RE = re.compile(r"alignment\s*=\s*(?P<value>\d+)\s*:\s*i\d+")
_FUNC_RE = re.compile(r"func\.func\s+@(?P<name>[\w$.#-]+)")
_CORE_RE = re.compile(r"hivm\.func_core_type\s*=\s*#hivm\.func_core_type<(?P<core>\w+)>")
_TEMP_BUFFER_RE = re.compile(r"temp_buffer\((?P<ssa>%[\w$.#-]+)\s*:")
_ADDRESS_SPACE_RE = re.compile(r"#hivm\.address_space<(?P<space>\w+)>")


@dataclass(frozen=True)
class IRBufferRecord:
    """A UB-like memref allocation parsed from PlanMemory-before IR."""

    ssa_name: str
    memref_type: str
    shape: tuple[int, ...]
    element_type: str
    element_bits: int
    extent_bits: int
    memory_space: str
    function_name: str
    core_type: str
    line: int
    alignment_bytes: int | None
    is_tmpbuf_hint: bool

    @property
    def extent_bytes(self) -> int:
        return math.ceil(self.extent_bits / 8)


@dataclass(frozen=True)
class ParsedPlanMemoryIR:
    path: Path
    records: tuple[IRBufferRecord, ...]

    def filter(self, core_type: str | None = None, memory_space: str | None = None):
        records: Iterable[IRBufferRecord] = self.records
        if core_type is not None:
            records = (r for r in records if r.core_type == core_type)
        if memory_space is not None:
            records = (r for r in records if r.memory_space == memory_space)
        return tuple(records)


class IRParseError(ValueError):
    pass


def _brace_delta(line: str) -> int:
    return line.count("{") - line.count("}")


def _parse_element_bits(element_type: str) -> int:
    if element_type in {"f16", "bf16"}:
        return 16
    if element_type == "f32":
        return 32
    if element_type == "f64":
        return 64
    match = re.fullmatch(r"i(?P<bits>\d+)", element_type)
    if match:
        return int(match.group("bits"))
    raise IRParseError(f"unsupported element type: {element_type}")


def _split_memref_type(memref_type: str) -> tuple[tuple[int, ...], str, str]:
    if not memref_type.startswith("memref<") or not memref_type.endswith(">"):
        raise IRParseError(f"not a memref type: {memref_type}")

    inner = memref_type[len("memref<") : -1]
    address_space_match = _ADDRESS_SPACE_RE.search(inner)
    if not address_space_match:
        raise IRParseError(f"missing HIVM address space: {memref_type}")
    memory_space = address_space_match.group("space")

    tensor_part = inner.split(",", 1)[0].strip()
    tokens = tensor_part.split("x")
    if not tokens:
        raise IRParseError(f"empty memref type: {memref_type}")

    element_type = tokens[-1]
    shape_tokens = tokens[:-1]
    shape: list[int] = []
    for token in shape_tokens:
        token = token.strip()
        if token == "?":
            raise IRParseError(f"dynamic memref shape is not exact: {memref_type}")
        if not token:
            continue
        shape.append(int(token))

    return tuple(shape), element_type, memory_space


def _extent_bits(shape: tuple[int, ...], element_bits: int) -> int:
    count = 1
    for dim in shape:
        count *= dim
    return count * element_bits


def parse_planmemory_before_ir(path: str | Path) -> ParsedPlanMemoryIR:
    ir_path = Path(path)
    lines = ir_path.read_text().splitlines()

    records: list[IRBufferRecord] = []
    temp_ssas: set[str] = set()
    current_function = ""
    current_core = ""
    function_depth = 0

    for line_no, line in enumerate(lines, start=1):
        func_match = _FUNC_RE.search(line)
        if func_match:
            current_function = func_match.group("name")
            core_match = _CORE_RE.search(line)
            current_core = core_match.group("core") if core_match else ""
            function_depth = max(_brace_delta(line), 1)
        elif current_function:
            core_match = _CORE_RE.search(line)
            if core_match:
                current_core = core_match.group("core")
            function_depth += _brace_delta(line)
            if function_depth <= 0:
                current_function = ""
                current_core = ""
                function_depth = 0
                continue

        for temp_match in _TEMP_BUFFER_RE.finditer(line):
            temp_ssas.add(temp_match.group("ssa"))

        alloc_match = _ALLOC_RE.match(line)
        if not alloc_match:
            continue

        memref_type = alloc_match.group("type")
        shape, element_type, memory_space = _split_memref_type(memref_type)
        element_bits = _parse_element_bits(element_type)
        attrs = alloc_match.group("attrs") or ""
        align_match = _ALIGN_RE.search(attrs)
        alignment_bytes = int(align_match.group("value")) if align_match else None
        records.append(
            IRBufferRecord(
                ssa_name=alloc_match.group("ssa"),
                memref_type=memref_type,
                shape=shape,
                element_type=element_type,
                element_bits=element_bits,
                extent_bits=_extent_bits(shape, element_bits),
                memory_space=memory_space,
                function_name=current_function,
                core_type=current_core,
                line=line_no,
                alignment_bytes=alignment_bytes,
                is_tmpbuf_hint=False,
            )
        )

    records = [
        replace(record, is_tmpbuf_hint=record.ssa_name in temp_ssas)
        for record in records
    ]
    return ParsedPlanMemoryIR(ir_path, tuple(records))


def iter_before_planmemory_files(root: str | Path) -> Iterable[Path]:
    root_path = Path(root)
    if root_path.is_file():
        yield root_path
        return
    yield from sorted(root_path.glob("*/*.before_local_plan_memory.mlir"))
