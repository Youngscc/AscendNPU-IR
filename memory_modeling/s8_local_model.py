"""First-pass S8.5 local memory model.

This module parses the IR dumped after ``createMarkMultiBufferPass`` and builds
a conservative text-level approximation of the local PlanMemory input. It is
not a replacement for the C++ PlanMemory pass; the output is meant to be compared
with that oracle.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from typing import Iterable

from .core import BufferKind, BufferRecord, MemoryScope, MemoryState, PlanPeak, Precision
from .scanner import (
    ADDRESS_SPACE_RE,
    FUNC_RE,
    MEMORY_OP_PATTERNS,
    count_patterns,
    count_values,
    infer_ir_level,
    strip_line_comments,
)


LOCAL_SCOPE_INFO: dict[MemoryScope, tuple[int, int]] = {
    MemoryScope.UB: (32 * 8, 192 * 1024 * 8),
    MemoryScope.L1: (32 * 8, 512 * 1024 * 8),
    MemoryScope.L0C: (512 * 8, 128 * 1024 * 8),
}

ELEMENT_BITS: dict[str, int] = {
    "i1": 1,
    "i8": 8,
    "ui8": 8,
    "si8": 8,
    "i16": 16,
    "ui16": 16,
    "si16": 16,
    "i32": 32,
    "ui32": 32,
    "si32": 32,
    "i64": 64,
    "ui64": 64,
    "si64": 64,
    "f16": 16,
    "bf16": 16,
    "f32": 32,
    "f64": 64,
    "index": 64,
}

ALLOC_RE = re.compile(r"(?P<name>%[\w.$-]+)\s*=\s*memref\.alloc\b")
MARK_MULTIBUFFER_RE = re.compile(
    r"annotation\.mark\s+(?P<name>%[\w.$-]+)\s*\{[^}]*"
    r"hivm\.multi_buffer\s*=\s*(?P<num>\d+)\s*:\s*i\d+"
)
SSA_VALUE_RE = re.compile(r"%[\w.$-]+")
MEMREF_TYPE_RE = re.compile(r"memref<(?P<body>.*)>")


@dataclass(frozen=True)
class MemRefInfo:
    type_text: str
    shape: tuple[int | None, ...]
    element_type: str
    element_bits: int | None
    scope: MemoryScope

    @property
    def is_static(self) -> bool:
        return self.element_bits is not None and all(dim is not None for dim in self.shape)

    @property
    def size_bits(self) -> int | None:
        if not self.is_static or self.element_bits is None:
            return None
        total = 1
        for dim in self.shape:
            assert dim is not None
            total *= dim
        return total * self.element_bits


@dataclass
class ParsedBuffer:
    name: str
    raw_name: str
    func_name: str
    memref: MemRefInfo
    alloc_index: int
    first_use_index: int
    last_use_index: int
    multi_buffer_factor: int = 1
    use_count: int = 0
    blockers: list[str] = field(default_factory=list)

    @property
    def aligned_size_bits(self) -> int | None:
        size = self.memref.size_bits
        if size is None:
            return None
        align = LOCAL_SCOPE_INFO.get(self.memref.scope, (None, None))[0]
        if align is None:
            return None
        return align_up(size, align)

    def to_record(self) -> BufferRecord:
        return BufferRecord(
            stable_id=self.name,
            kind=BufferKind.MEMREF_ALLOC,
            type_text=self.memref.type_text,
            defining_op="memref.alloc",
            scope=self.memref.scope,
            size_bits=self.memref.size_bits,
            aligned_size_bits=self.aligned_size_bits,
            multi_buffer_factor=self.multi_buffer_factor,
            attrs={
                "function": self.func_name,
                "raw_name": self.raw_name,
                "alloc_index": self.alloc_index,
                "first_use_index": self.first_use_index,
                "last_use_index": self.last_use_index,
                "use_count": self.use_count,
                "shape": list(self.memref.shape),
                "element_type": self.memref.element_type,
                "element_bits": self.memref.element_bits,
                "dynamic_or_unknown": not self.memref.is_static,
                "model": "s8.5_text_v1",
            },
        )


@dataclass
class FunctionIR:
    name: str
    lines: list[tuple[int, str]] = field(default_factory=list)


@dataclass(frozen=True)
class IntervalAllocation:
    buffer_name: str
    scope: MemoryScope
    start: int
    end: int
    size_bits: int
    offset_bits: int
    copy_index: int


def align_up(value: int, alignment: int) -> int:
    if alignment <= 0:
        return value
    return ((value + alignment - 1) // alignment) * alignment


def _split_top_level_commas(text: str) -> list[str]:
    parts: list[str] = []
    depth = 0
    start = 0
    for index, char in enumerate(text):
        if char in "<([":
            depth += 1
        elif char in ">)]":
            depth -= 1
        elif char == "," and depth == 0:
            parts.append(text[start:index].strip())
            start = index + 1
    parts.append(text[start:].strip())
    return parts


def _scope_from_memref_body(body: str) -> MemoryScope:
    match = ADDRESS_SPACE_RE.search(body)
    if not match:
        return MemoryScope.UNKNOWN
    raw = match.group(1).upper()
    if "UB" in raw:
        return MemoryScope.UB
    if "L1" in raw or "CBUF" in raw:
        return MemoryScope.L1
    if "L0C" in raw or "CC" in raw:
        return MemoryScope.L0C
    return MemoryScope.UNKNOWN


def parse_memref_type(type_text: str) -> MemRefInfo:
    match = MEMREF_TYPE_RE.fullmatch(type_text.strip())
    if not match:
        return MemRefInfo(type_text, (), "unknown", None, MemoryScope.UNKNOWN)

    body = match.group("body")
    first_part = _split_top_level_commas(body)[0]
    dims_and_element = [part.strip() for part in first_part.split("x")]
    if len(dims_and_element) == 1:
        shape: tuple[int | None, ...] = ()
        element_type = dims_and_element[0]
    else:
        element_type = dims_and_element[-1]
        dims: list[int | None] = []
        for dim_text in dims_and_element[:-1]:
            if dim_text == "?":
                dims.append(None)
            else:
                try:
                    dims.append(int(dim_text))
                except ValueError:
                    dims.append(None)
        shape = tuple(dims)

    return MemRefInfo(
        type_text=type_text.strip(),
        shape=shape,
        element_type=element_type,
        element_bits=ELEMENT_BITS.get(element_type),
        scope=_scope_from_memref_body(body),
    )


def extract_memref_type_after_alloc(line: str) -> str | None:
    type_start = line.find("memref<")
    if type_start < 0:
        return None

    depth = 0
    for index in range(type_start + len("memref<") - 1, len(line)):
        char = line[index]
        if char == "<":
            depth += 1
        elif char == ">":
            depth -= 1
            if depth == 0:
                return line[type_start : index + 1]
    return None


def extract_functions(text: str) -> list[FunctionIR]:
    functions: list[FunctionIR] = []
    current: FunctionIR | None = None
    depth = 0
    seen_body_open = False

    for line_number, raw_line in enumerate(text.splitlines(), start=1):
        line = raw_line.strip()
        if current is None:
            match = FUNC_RE.search(line)
            if not match:
                continue
            current = FunctionIR(match.group(1))
            depth = 0
            seen_body_open = False

        if current is not None:
            current.lines.append((line_number, raw_line))
            if seen_body_open:
                depth += raw_line.count("{") - raw_line.count("}")
            else:
                signature_end = raw_line.rfind(")")
                body_start = raw_line.find("{", signature_end) if signature_end >= 0 else -1
                if body_start >= 0:
                    seen_body_open = True
                    body_fragment = raw_line[body_start:]
                    depth += body_fragment.count("{") - body_fragment.count("}")
            if seen_body_open and depth <= 0:
                functions.append(current)
                current = None
                depth = 0
                seen_body_open = False

    if current is not None:
        functions.append(current)
    return functions


def _collect_multi_buffer_factors(function: FunctionIR) -> dict[str, int]:
    result: dict[str, int] = {}
    for _, line in function.lines:
        match = MARK_MULTIBUFFER_RE.search(line)
        if match:
            result[match.group("name")] = int(match.group("num"))
    return result


def parse_function_buffers(function: FunctionIR) -> list[ParsedBuffer]:
    stripped_lines = [(line_no, line.split("//", 1)[0]) for line_no, line in function.lines]
    multi_buffer = _collect_multi_buffer_factors(function)
    buffers: list[ParsedBuffer] = []
    buffers_by_raw_name: dict[str, list[ParsedBuffer]] = {}
    raw_name_counts: dict[str, int] = {}

    for op_index, (line_no, line) in enumerate(stripped_lines):
        match = ALLOC_RE.search(line)
        if not match:
            continue
        type_text = extract_memref_type_after_alloc(line)
        if type_text is None:
            continue
        raw_name = match.group("name")
        occurrence = raw_name_counts.get(raw_name, 0) + 1
        raw_name_counts[raw_name] = occurrence
        name = raw_name if occurrence == 1 else f"{raw_name}@L{line_no}"
        memref = parse_memref_type(type_text)
        buffer = ParsedBuffer(
            name=name,
            raw_name=raw_name,
            func_name=function.name,
            memref=memref,
            alloc_index=op_index,
            first_use_index=op_index,
            last_use_index=op_index,
            multi_buffer_factor=multi_buffer.get(raw_name, 1),
        )
        if memref.scope not in LOCAL_SCOPE_INFO:
            buffer.blockers.append(f"{name}: unsupported or non-local scope {memref.scope.value}")
        if not memref.is_static:
            buffer.blockers.append(f"{name}: dynamic shape or unknown element width")
        buffers.append(buffer)
        buffers_by_raw_name.setdefault(raw_name, []).append(buffer)

    for op_index, (_, line) in enumerate(stripped_lines):
        values = SSA_VALUE_RE.findall(line)
        if not values:
            continue
        for value in values:
            for buffer in buffers_by_raw_name.get(value, []):
                buffer.use_count += 1
                if op_index < buffer.first_use_index:
                    buffer.first_use_index = op_index
                if op_index > buffer.last_use_index:
                    buffer.last_use_index = op_index

    return buffers


def _interval_conflicts(a: IntervalAllocation, start: int, end: int) -> bool:
    return not (a.end < start or end < a.start)


def allocate_scope_intervals(buffers: Iterable[ParsedBuffer], scope: MemoryScope) -> tuple[int, list[IntervalAllocation]]:
    allocations: list[IntervalAllocation] = []
    peak = 0
    candidates = [
        buffer
        for buffer in buffers
        if buffer.memref.scope == scope and buffer.aligned_size_bits is not None
    ]
    candidates.sort(key=lambda item: (item.first_use_index, item.last_use_index, item.name))

    for buffer in candidates:
        assert buffer.aligned_size_bits is not None
        for copy_index in range(max(buffer.multi_buffer_factor, 1)):
            offset = 0
            while True:
                conflicting = [
                    allocation
                    for allocation in allocations
                    if allocation.scope == scope
                    and _interval_conflicts(
                        allocation, buffer.first_use_index, buffer.last_use_index
                    )
                    and not (
                        allocation.offset_bits + allocation.size_bits <= offset
                        or offset + buffer.aligned_size_bits <= allocation.offset_bits
                    )
                ]
                if not conflicting:
                    break
                offset = min(
                    allocation.offset_bits + allocation.size_bits
                    for allocation in conflicting
                    if allocation.offset_bits + allocation.size_bits > offset
                )
            allocations.append(
                IntervalAllocation(
                    buffer_name=buffer.name,
                    scope=scope,
                    start=buffer.first_use_index,
                    end=buffer.last_use_index,
                    size_bits=buffer.aligned_size_bits,
                    offset_bits=offset,
                    copy_index=copy_index,
                )
            )
            peak = max(peak, offset + buffer.aligned_size_bits)

    return peak, allocations


def create_s8_5_memory_state(text: str) -> MemoryState:
    scan_text = strip_line_comments(text)
    memory_ops = count_patterns(scan_text, MEMORY_OP_PATTERNS)
    ir_level = infer_ir_level(scan_text, memory_ops)
    functions = extract_functions(scan_text)
    parsed_buffers: list[ParsedBuffer] = []
    function_summaries: dict[str, dict[str, object]] = {}
    max_scope_peak: dict[MemoryScope, int] = {}
    blockers: list[str] = [
        "text parser only; verify against C++ PlanMemory oracle",
        "NormalizeLoopIterator is not simulated; use oracle if loop iter/yield conflicts exist",
        "alias/inplace analysis is simplified; interval reuse is conservative",
        "dmaFirstPipelineOpt/spec-level allocation/20 seed retry are not simulated",
    ]

    for function in functions:
        buffers = parse_function_buffers(function)
        parsed_buffers.extend(buffers)
        function_scope_peaks: dict[str, int] = {}
        function_scope_allocations: dict[str, list[dict[str, object]]] = {}
        for scope in (MemoryScope.UB, MemoryScope.L1, MemoryScope.L0C):
            peak, allocations = allocate_scope_intervals(buffers, scope)
            if peak:
                function_scope_peaks[scope.value] = peak
                max_scope_peak[scope] = max(max_scope_peak.get(scope, 0), peak)
                function_scope_allocations[scope.value] = [
                    {
                        "buffer": allocation.buffer_name,
                        "copy_index": allocation.copy_index,
                        "start": allocation.start,
                        "end": allocation.end,
                        "offset_bits": allocation.offset_bits,
                        "size_bits": allocation.size_bits,
                    }
                    for allocation in allocations
                ]
        if buffers:
            function_summaries[function.name] = {
                "buffer_count": len(buffers),
                "scope_peaks_bits": function_scope_peaks,
                "allocations": function_scope_allocations,
            }
        for buffer in buffers:
            blockers.extend(buffer.blockers)

    peaks = [
        PlanPeak(
            scope=scope,
            peak_bits=peak_bits,
            peak_bytes=align_up(peak_bits, 8) // 8,
            capacity_bits=LOCAL_SCOPE_INFO[scope][1],
            required_bits=peak_bits,
            source="s8.5_text_model:max_function",
        )
        for scope, peak_bits in sorted(max_scope_peak.items(), key=lambda item: item[0].value)
    ]

    return MemoryState(
        checkpoint_id="S8.5",
        ir_level=ir_level,
        precision=Precision.CONCRETE_STRUCTURAL,
        memory_ops=memory_ops,
        buffers=[buffer.to_record() for buffer in parsed_buffers],
        address_spaces=count_values(ADDRESS_SPACE_RE.findall(scan_text)),
        functions=[function.name for function in functions],
        peaks=peaks,
        blockers=sorted(set(blockers)),
        metadata={
            "checkpoint_label": "After MarkMultiBuffer",
            "suffix_passes": [
                "createDumpIRBeforePlanMemoryPass",
                "NormalizeLoopIterator",
                "MemLivenessAnalysis",
                "MemPlan",
                "AllocToPointerCast",
                "fixMultibufferEnabledPointerCastOps",
            ],
            "scanner": "text",
            "model": "s8.5_text_v1",
            "scope_info_bits": {
                scope.value: {"align": align, "capacity": capacity}
                for scope, (align, capacity) in LOCAL_SCOPE_INFO.items()
            },
            "functions": function_summaries,
        },
    )
