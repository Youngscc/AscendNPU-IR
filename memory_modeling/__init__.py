"""Lightweight memory-modeling helpers for BiShengIR pipeline analysis."""

from .checkpoints import get_checkpoint, list_checkpoints, reverse_local_plan_order
from .core import (
    BufferKind,
    BufferRecord,
    Checkpoint,
    MemoryScope,
    MemoryState,
    ModelingReport,
    PassEffect,
    PlanPeak,
    Precision,
)
from .scanner import create_report

__version__ = "0.1.0"

__all__ = [
    "__version__",
    "BufferKind",
    "BufferRecord",
    "Checkpoint",
    "MemoryScope",
    "MemoryState",
    "ModelingReport",
    "PassEffect",
    "PlanPeak",
    "Precision",
    "create_report",
    "get_checkpoint",
    "list_checkpoints",
    "reverse_local_plan_order",
]
