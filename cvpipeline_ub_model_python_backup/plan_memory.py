"""PlanMemory-equivalent primitives for the CVPipeline UB model.

Stage B starts from MemoryDisplay JSON emitted by the suffix oracle.  The
offset-replay path is exact with respect to that oracle because it recomputes
the peak from PlanMemory-assigned offsets.  The offset-free planner is kept
separate and marked experimental until it is proven equivalent to PlanMemory.
"""

from __future__ import annotations

from dataclasses import dataclass
import json
import math
from pathlib import Path
from typing import Iterable, Sequence


BITS_PER_BYTE = 8
UB_CAPACITY_BITS = 192 * 1024 * BITS_PER_BYTE
UB_ALIGN_BYTES = 32
UB_ADDRESS_SPACE = "#hivm.address_space<ub>"


def ceil_div(value: int, divisor: int) -> int:
    return (value + divisor - 1) // divisor


def align_up(value: int, align: int) -> int:
    if align <= 0:
        return value
    return ceil_div(value, align) * align


@dataclass(frozen=True)
class BufferRecord:
    """A local-memory buffer as reported by MemoryDisplay."""

    id: str
    buffer: str
    extent_bits: int
    offsets_bytes: tuple[int, ...]
    lifetime: tuple[int, int]
    alloc_time: int | None
    is_tmpbuf: bool
    source_location: str
    memory_space: str

    @property
    def extent_bytes(self) -> int:
        return ceil_div(self.extent_bits, BITS_PER_BYTE)

    @property
    def aligned_extent_bytes(self) -> int:
        if self.memory_space == "ub":
            return align_up(self.extent_bytes, UB_ALIGN_BYTES)
        return self.extent_bytes

    @property
    def start(self) -> int:
        return self.lifetime[0]

    @property
    def end(self) -> int:
        return self.lifetime[1]


@dataclass(frozen=True)
class MemoryInfo:
    """Parsed MemoryDisplay JSON for one kernel/core-side file."""

    path: Path
    kernel_name: str
    records: tuple[BufferRecord, ...]


@dataclass(frozen=True)
class PlannedBuffer:
    record_id: str
    offset_bytes: int
    extent_bytes: int
    start: int
    end: int


@dataclass(frozen=True)
class PlanResult:
    """Peak result for one planning mode."""

    mode: str
    peak_bytes: int
    peak_bits: int
    capacity_bits: int
    overflow: bool
    alloc_count: int
    planned_buffers: tuple[PlannedBuffer, ...]


def _memory_space_from_buffer(buffer: str) -> str:
    if "#hivm.address_space<ub>" in buffer:
        return "ub"
    if "#hivm.address_space<cbuf>" in buffer:
        return "cbuf"
    if "#hivm.address_space<cc>" in buffer:
        return "cc"
    if "#hivm.address_space<gm>" in buffer:
        return "gm"
    return "unknown"


def _record_from_json(item: dict, index: int) -> BufferRecord:
    lifetime = item.get("life_time_in_ir") or [0, 0]
    offsets = item.get("offset") or [0]
    buffer = item.get("buffer", "")
    return BufferRecord(
        id=f"buffer_{index}",
        buffer=buffer,
        extent_bits=int(item.get("extent", 0)),
        offsets_bytes=tuple(int(offset) for offset in offsets),
        lifetime=(int(lifetime[0]), int(lifetime[1])),
        alloc_time=(
            None
            if item.get("alloc_time_in_ir") is None
            else int(item.get("alloc_time_in_ir"))
        ),
        is_tmpbuf=bool(item.get("is_tmpbuf", False)),
        source_location=item.get("source_location", ""),
        memory_space=_memory_space_from_buffer(buffer),
    )


def load_memory_info(path: str | Path, memory_space: str = "ub") -> MemoryInfo:
    """Load MemoryDisplay JSON and keep only the requested memory space."""

    json_path = Path(path)
    with json_path.open() as f:
        data = json.load(f)

    kernel_name = data.get("Header", {}).get("KernelName", "")
    records: list[BufferRecord] = []
    index = 0
    for record in data.get("Record", []):
        for item in record.get("memory_info_array", []):
            parsed = _record_from_json(item, index)
            index += 1
            if parsed.memory_space == memory_space:
                records.append(parsed)

    return MemoryInfo(json_path, kernel_name, tuple(records))


def replay_planmemory_offsets(
    records: Sequence[BufferRecord], capacity_bits: int = UB_CAPACITY_BITS
) -> PlanResult:
    """Recompute peak from PlanMemory-assigned offsets.

    MemoryDisplay writes byte offsets and bit extents.  This replay matches the
    suffix oracle peak by taking the maximum end address over all reported
    offsets.  Multi-buffer entries can have more than one offset.
    """

    planned: list[PlannedBuffer] = []
    peak_bytes = 0
    for record in records:
        for offset in record.offsets_bytes:
            end = offset + record.extent_bytes
            peak_bytes = max(peak_bytes, end)
            planned.append(
                PlannedBuffer(
                    record_id=record.id,
                    offset_bytes=offset,
                    extent_bytes=record.extent_bytes,
                    start=record.start,
                    end=record.end,
                )
            )

    peak_bits = peak_bytes * BITS_PER_BYTE
    return PlanResult(
        mode="offset_replay",
        peak_bytes=peak_bytes,
        peak_bits=peak_bits,
        capacity_bits=capacity_bits,
        overflow=peak_bits > capacity_bits,
        alloc_count=len(records),
        planned_buffers=tuple(planned),
    )


def _intervals_overlap(a: BufferRecord, b: PlannedBuffer) -> bool:
    # PlanMemory lifetimes are reported as inclusive instruction positions.
    return not (a.end < b.start or b.end < a.start)


def first_fit_plan(
    records: Sequence[BufferRecord],
    capacity_bits: int = UB_CAPACITY_BITS,
    align_bytes: int = UB_ALIGN_BYTES,
) -> PlanResult:
    """Experimental offset-free local-memory planner.

    This is intentionally not marked exact.  It is useful for finding the gap
    between a simple interval allocator and PlanMemory.  Exact delivery must
    use `replay_planmemory_offsets` until this algorithm is proven equivalent.
    """

    planned: list[PlannedBuffer] = []
    peak_bytes = 0
    ordered = sorted(records, key=lambda r: (r.start, r.end, -r.aligned_extent_bytes))

    for record in ordered:
        active = [p for p in planned if _intervals_overlap(record, p)]
        active.sort(key=lambda p: p.offset_bytes)
        size = align_up(record.extent_bytes, align_bytes)
        offset = 0
        for other in active:
            gap_end = other.offset_bytes
            if offset + size <= gap_end:
                break
            offset = align_up(other.offset_bytes + other.extent_bytes, align_bytes)

        planned_buffer = PlannedBuffer(
            record_id=record.id,
            offset_bytes=offset,
            extent_bytes=size,
            start=record.start,
            end=record.end,
        )
        planned.append(planned_buffer)
        peak_bytes = max(peak_bytes, offset + size)

    peak_bits = peak_bytes * BITS_PER_BYTE
    return PlanResult(
        mode="first_fit_experimental",
        peak_bytes=peak_bytes,
        peak_bits=peak_bits,
        capacity_bits=capacity_bits,
        overflow=peak_bits > capacity_bits,
        alloc_count=len(records),
        planned_buffers=tuple(planned),
    )


def peak_from_memory_info(path: str | Path, memory_space: str = "ub") -> PlanResult:
    info = load_memory_info(path, memory_space=memory_space)
    return replay_planmemory_offsets(info.records)


def iter_memory_info_files(root: str | Path) -> Iterable[Path]:
    root_path = Path(root)
    if root_path.is_file():
        yield root_path
        return
    yield from sorted(root_path.glob("*/memory_info_aiv.json"))
