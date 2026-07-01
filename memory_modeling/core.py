"""Core data structures for BiShengIR memory modeling.

The Python layer is intentionally a modeling scaffold. Exact local memory
planning should come from the C++ PlanMemory implementation or a dry-run oracle.
"""

from __future__ import annotations

from dataclasses import asdict, dataclass, field
from enum import Enum
from typing import Any


def to_jsonable(value: Any) -> Any:
    if isinstance(value, Enum):
        return value.value
    if isinstance(value, dict):
        return {key: to_jsonable(item) for key, item in value.items()}
    if isinstance(value, (list, tuple)):
        return [to_jsonable(item) for item in value]
    return value


class Precision(str, Enum):
    """How strongly a memory report can be interpreted."""

    EXACT_PLAN = "exact_plan"
    CONCRETE_STRUCTURAL = "concrete_structural"
    SYMBOLIC = "symbolic"
    NOT_APPLICABLE = "not_applicable"


class MemoryScope(str, Enum):
    """Memory scopes relevant to local and workspace planning."""

    UB = "UB"
    L1 = "L1"
    L0C = "L0C"
    GLOBAL_WORKSPACE = "GLOBAL_WORKSPACE"
    UNKNOWN = "UNKNOWN"


class BufferKind(str, Enum):
    """Kinds of buffer-like values tracked by the model."""

    TENSOR_EMPTY = "tensor.empty"
    MEMREF_ALLOC = "memref.alloc"
    MEMREF_ALLOCA = "memref.alloca"
    WORKSPACE_ALLOC = "memref_ext.alloc_workspace"
    EXTRA_BUFFER = "extra_buffer"
    UNKNOWN = "unknown"


@dataclass(frozen=True)
class BufferRecord:
    """A buffer-like value as seen by the model.

    Early text scans may only fill ``type_text`` and ``defining_op``. Later MLIR
    or PlanMemory-backed collectors can fill size, scope, and lineage fields.
    """

    stable_id: str
    kind: BufferKind
    type_text: str | None = None
    defining_op: str | None = None
    scope: MemoryScope = MemoryScope.UNKNOWN
    size_bits: int | None = None
    aligned_size_bits: int | None = None
    multi_buffer_factor: int = 1
    lineage: tuple[str, ...] = ()
    attrs: dict[str, Any] = field(default_factory=dict)


@dataclass(frozen=True)
class PlanPeak:
    """Peak memory returned by PlanMemory or a future dry-run oracle."""

    scope: MemoryScope
    peak_bits: int | None
    peak_bytes: int | None = None
    capacity_bits: int | None = None
    required_bits: int | None = None
    source: str = "unknown"


@dataclass
class MemoryState:
    """Memory-relevant state at a checkpoint."""

    checkpoint_id: str
    ir_level: str
    precision: Precision
    memory_ops: dict[str, int] = field(default_factory=dict)
    buffers: list[BufferRecord] = field(default_factory=list)
    address_spaces: dict[str, int] = field(default_factory=dict)
    functions: list[str] = field(default_factory=list)
    module_attributes: list[str] = field(default_factory=list)
    peaks: list[PlanPeak] = field(default_factory=list)
    blockers: list[str] = field(default_factory=list)
    metadata: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict[str, Any]:
        return to_jsonable(asdict(self))


@dataclass(frozen=True)
class PassEffect:
    """A pass-local memory effect summary.

    Lists store stable ids where available, or human-readable descriptors while
    the model is still text based.
    """

    pass_name: str
    checkpoint_before: str | None = None
    checkpoint_after: str | None = None
    created_buffers: tuple[str, ...] = ()
    deleted_buffers: tuple[str, ...] = ()
    resized_buffers: tuple[str, ...] = ()
    scope_changes: tuple[str, ...] = ()
    layout_changes: tuple[str, ...] = ()
    lifetime_changes: tuple[str, ...] = ()
    copy_insertions: tuple[str, ...] = ()
    extra_buffer_insertions: tuple[str, ...] = ()
    multi_buffer_changes: tuple[str, ...] = ()
    inplace_reuse_candidate_changes: tuple[str, ...] = ()
    workspace_changes: tuple[str, ...] = ()
    precision: Precision = Precision.SYMBOLIC
    blocking_dependencies: tuple[str, ...] = ()
    notes: tuple[str, ...] = ()


@dataclass(frozen=True)
class Checkpoint:
    """A modeling checkpoint and the suffix needed to reach an oracle."""

    checkpoint_id: str
    label: str
    stage: str
    order: float
    starts_after_pass: str | None
    suffix_passes: tuple[str, ...]
    expected_precision: Precision
    goal: str
    notes: tuple[str, ...] = ()

    @property
    def is_planmemory_ready(self) -> bool:
        memory_changing_suffix = tuple(
            suffix
            for suffix in self.suffix_passes
            if suffix != "createDumpIRBeforePlanMemoryPass"
        )
        return self.expected_precision == Precision.EXACT_PLAN and memory_changing_suffix in {
            ("PlanMemory(local)",),
            (),
        }


@dataclass
class ModelingReport:
    """Top-level report emitted by the Python modeling scaffold."""

    input_file: str
    bytes: int
    lines: int
    checkpoint: Checkpoint
    state: MemoryState
    config: dict[str, Any] = field(default_factory=dict)
    effects: list[PassEffect] = field(default_factory=list)

    def to_dict(self) -> dict[str, Any]:
        return to_jsonable(asdict(self))
