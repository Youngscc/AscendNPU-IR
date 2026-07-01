"""Text-based MLIR scanner for early memory-modeling checkpoints."""

from __future__ import annotations

import json
import re
from pathlib import Path
from typing import Any

from .checkpoints import S0_INPUT, get_checkpoint
from .core import BufferKind, BufferRecord, MemoryScope, MemoryState, ModelingReport, Precision


MEMORY_OP_PATTERNS: dict[str, str] = {
    "tensor.empty": r"\btensor\.empty\b",
    "bufferization.to_tensor": r"\bbufferization\.to_tensor\b",
    "bufferization.to_memref": r"\bbufferization\.to_memref\b",
    "memref.alloc": r"\bmemref\.alloc\b",
    "memref.alloca": r"\bmemref\.alloca\b",
    "memref.memory_space_cast": r"\bmemref\.memory_space_cast\b",
    "memref_ext.alloc_workspace": r"\bmemref_ext\.alloc_workspace\b",
    "hivm.copy": r"\bhivm\.(?:hir\.)?copy\b",
    "hivm.load": r"\bhivm\.(?:hir\.)?load\b",
    "hivm.store": r"\bhivm\.(?:hir\.)?store\b",
    "hivm.fixpipe": r"\bhivm\.(?:hir\.)?fixpipe\b",
    "hivm.matmul_like": r"\bhivm\.(?:hir\.)?(?:matmul|mmad|batch_matmul|conv)",
}

MEMREF_TYPE_RE = re.compile(r"memref<([^>]+)>")
TENSOR_TYPE_RE = re.compile(r"tensor<([^>]+)>")
FUNC_RE = re.compile(r"\bfunc\.func\s+@([A-Za-z_.$][\w.$]*)")
ATTR_RE = re.compile(r"\b([A-Za-z_][\w.]*)\s*=")
ADDRESS_SPACE_RE = re.compile(r"#hivm\.address_space<([^>]+)>")
ALLOC_LINE_RE = re.compile(
    r"(?P<result>%[\w.$-]+)?[^:\n]*\b(?P<op>tensor\.empty|memref\.alloca|memref\.alloc|memref_ext\.alloc_workspace)\b(?P<tail>[^\n]*)"
)


def strip_line_comments(text: str) -> str:
    return "\n".join(line.split("//", 1)[0] for line in text.splitlines())


def count_patterns(text: str, patterns: dict[str, str] = MEMORY_OP_PATTERNS) -> dict[str, int]:
    return {name: len(re.findall(pattern, text)) for name, pattern in patterns.items()}


def count_values(values: list[str]) -> dict[str, int]:
    result: dict[str, int] = {}
    for value in values:
        result[value] = result.get(value, 0) + 1
    return result


def load_config(path: Path | None) -> dict[str, Any]:
    if path is None:
        return {}
    with path.open("r", encoding="utf-8") as f:
        return json.load(f)


def infer_ir_level(text: str, memory_ops: dict[str, int]) -> str:
    if "tt." in text or "ttir." in text:
        return "TTIR/Triton"
    if "hfusion." in text:
        return "HFusion"
    if "hivm." in text:
        if memory_ops["memref.alloc"] or memory_ops["bufferization.to_tensor"]:
            return "HIVM memref/tensor mixed"
        return "HIVM tensor"
    if "torch." in text:
        return "Torch"
    if memory_ops["memref.alloc"]:
        return "generic memref"
    if "tensor." in text:
        return "generic tensor"
    return "unknown"


def infer_precision(ir_level: str, memory_ops: dict[str, int], checkpoint_id: str) -> Precision:
    if checkpoint_id == "S8.5":
        return Precision.EXACT_PLAN
    if memory_ops["memref.alloc"] or memory_ops["memref_ext.alloc_workspace"]:
        return Precision.CONCRETE_STRUCTURAL
    if ir_level in {"unknown", "Torch", "TTIR/Triton", "HFusion", "HIVM tensor"}:
        return Precision.SYMBOLIC
    return Precision.SYMBOLIC


def infer_blockers(precision: Precision, memory_ops: dict[str, int], checkpoint_id: str) -> list[str]:
    blockers: list[str] = []
    if precision != Precision.EXACT_PLAN:
        blockers.append("not a PlanMemory result")
    if checkpoint_id != "S8.5":
        blockers.append("checkpoint suffix effects are not modeled yet")
    if not memory_ops["memref.alloc"] and not memory_ops["memref_ext.alloc_workspace"]:
        blockers.append("no concrete alloc/workspace found")
    blockers.append("no MLIR typed Operation walk")
    blockers.append("no liveness/inplace/multi-buffer PlanMemory analysis")
    return blockers


def _kind_for_op(op_name: str) -> BufferKind:
    if op_name == "tensor.empty":
        return BufferKind.TENSOR_EMPTY
    if op_name == "memref.alloc":
        return BufferKind.MEMREF_ALLOC
    if op_name == "memref.alloca":
        return BufferKind.MEMREF_ALLOCA
    if op_name == "memref_ext.alloc_workspace":
        return BufferKind.WORKSPACE_ALLOC
    return BufferKind.UNKNOWN


def _scope_from_text(text: str) -> MemoryScope:
    match = ADDRESS_SPACE_RE.search(text)
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


def extract_buffer_records(text: str) -> list[BufferRecord]:
    records: list[BufferRecord] = []
    for index, match in enumerate(ALLOC_LINE_RE.finditer(text)):
        line = match.group(0).strip()
        type_match = MEMREF_TYPE_RE.search(line) or TENSOR_TYPE_RE.search(line)
        result_name = match.group("result")
        stable_id = result_name or f"anon_buffer_{index}"
        records.append(
            BufferRecord(
                stable_id=stable_id,
                kind=_kind_for_op(match.group("op")),
                type_text=type_match.group(0) if type_match else None,
                defining_op=match.group("op"),
                scope=_scope_from_text(line),
            )
        )
    return records


def create_memory_state(text: str, checkpoint_id: str = "S0") -> MemoryState:
    checkpoint = get_checkpoint(checkpoint_id)
    if checkpoint.checkpoint_id == "S8.5":
        from .s8_local_model import create_s8_5_memory_state

        return create_s8_5_memory_state(text)

    scan_text = strip_line_comments(text)
    memory_ops = count_patterns(scan_text)
    ir_level = infer_ir_level(scan_text, memory_ops)
    precision = infer_precision(ir_level, memory_ops, checkpoint.checkpoint_id)
    prefix = scan_text[: scan_text.find("{") if "{" in scan_text else 0]

    return MemoryState(
        checkpoint_id=checkpoint.checkpoint_id,
        ir_level=ir_level,
        precision=precision,
        memory_ops=memory_ops,
        buffers=extract_buffer_records(scan_text),
        address_spaces=count_values(ADDRESS_SPACE_RE.findall(scan_text)),
        functions=FUNC_RE.findall(scan_text),
        module_attributes=sorted(set(ATTR_RE.findall(prefix))),
        blockers=infer_blockers(precision, memory_ops, checkpoint.checkpoint_id),
        metadata={
            "checkpoint_label": checkpoint.label,
            "suffix_passes": list(checkpoint.suffix_passes),
            "scanner": "text",
        },
    )


def create_report(
    input_file: Path,
    config: dict[str, Any] | None = None,
    checkpoint_id: str = S0_INPUT.checkpoint_id,
) -> ModelingReport:
    text = input_file.read_text(encoding="utf-8")
    checkpoint = get_checkpoint(checkpoint_id)
    state = create_memory_state(text, checkpoint.checkpoint_id)
    return ModelingReport(
        input_file=str(input_file),
        bytes=len(text.encode("utf-8")),
        lines=0 if not text else text.count("\n") + (0 if text.endswith("\n") else 1),
        checkpoint=checkpoint,
        state=state,
        config=config or {},
    )
