"""Checkpoint registry for the reverse memory-modeling workflow."""

from __future__ import annotations

from .core import Checkpoint, Precision


S0_INPUT = Checkpoint(
    checkpoint_id="S0",
    label="Input/config snapshot",
    stage="S0",
    order=0.0,
    starts_after_pass=None,
    suffix_passes=(),
    expected_precision=Precision.SYMBOLIC,
    goal="Capture the input IR level and obvious memory-relevant structure.",
    notes=(
        "Text scan only; not a PlanMemory-ready checkpoint.",
        "Use symbolic or concrete_structural precision depending on visible allocs.",
    ),
)


S8_LOCAL_CHECKPOINTS: tuple[Checkpoint, ...] = (
    Checkpoint(
        checkpoint_id="S8.5",
        label="After MarkMultiBuffer",
        stage="S8",
        order=8.5,
        starts_after_pass="createMarkMultiBufferPass",
        suffix_passes=("createDumpIRBeforePlanMemoryPass", "PlanMemory(local)"),
        expected_precision=Precision.EXACT_PLAN,
        goal=(
            "Validate the model against a PlanMemory-ready IR with the shortest "
            "possible suffix."
        ),
        notes=(
            "First implementation target.",
            "createDumpIRBeforePlanMemoryPass is a debug/no-op IR dump when enabled.",
            "The memory-changing suffix is local PlanMemory.",
        ),
    ),
    Checkpoint(
        checkpoint_id="S8.4",
        label="After InlineLoadCopy",
        stage="S8",
        order=8.4,
        starts_after_pass="createInlineLoadCopyPass",
        suffix_passes=(
            "MarkMultiBuffer",
            "createDumpIRBeforePlanMemoryPass",
            "PlanMemory(local)",
        ),
        expected_precision=Precision.CONCRETE_STRUCTURAL,
        goal="Model MarkMultiBuffer effects before comparing with PlanMemory.",
        notes=("Track MultiBufferAttr and local buffer multiplicity.",),
    ),
    Checkpoint(
        checkpoint_id="S8.3",
        label="After canonicalizationHIVMPipeline",
        stage="S8",
        order=8.3,
        starts_after_pass="canonicalizationHIVMPipeline",
        suffix_passes=(
            "InlineLoadCopy",
            "MarkMultiBuffer",
            "createDumpIRBeforePlanMemoryPass",
            "PlanMemory(local)",
        ),
        expected_precision=Precision.CONCRETE_STRUCTURAL,
        goal="Model InlineLoadCopy changes to copy/load shape, def-use, and lifetime.",
    ),
    Checkpoint(
        checkpoint_id="S8.2",
        label="After InferHIVMMemScope",
        stage="S8",
        order=8.2,
        starts_after_pass="createInferHIVMMemScopePass",
        suffix_passes=(
            "canonicalizationHIVMPipeline",
            "InlineLoadCopy",
            "MarkMultiBuffer",
            "createDumpIRBeforePlanMemoryPass",
            "PlanMemory(local)",
        ),
        expected_precision=Precision.CONCRETE_STRUCTURAL,
        goal=(
            "Model canonicalization/CSE/DSE effects on buffer set and lifetime "
            "after mem scope inference."
        ),
    ),
    Checkpoint(
        checkpoint_id="S8.1",
        label="After AllocExtraBuffer",
        stage="S8",
        order=8.1,
        starts_after_pass="createAllocExtraBufferPass",
        suffix_passes=(
            "InferHIVMMemScope",
            "canonicalizationHIVMPipeline",
            "InlineLoadCopy",
            "MarkMultiBuffer",
            "createDumpIRBeforePlanMemoryPass",
            "PlanMemory(local)",
        ),
        expected_precision=Precision.CONCRETE_STRUCTURAL,
        goal="Model extra buffer creation and subsequent scope inference.",
    ),
    Checkpoint(
        checkpoint_id="S8.0",
        label="After AFTER_LIFT_LOWEST_STRIDE decompose",
        stage="S8",
        order=8.0,
        starts_after_pass="createHIVMAggregatedDecomposeOpPass(AFTER_LIFT_LOWEST_STRIDE)",
        suffix_passes=(
            "AllocExtraBuffer",
            "InferHIVMMemScope",
            "canonicalizationHIVMPipeline",
            "InlineLoadCopy",
            "MarkMultiBuffer",
            "createDumpIRBeforePlanMemoryPass",
            "PlanMemory(local)",
        ),
        expected_precision=Precision.CONCRETE_STRUCTURAL,
        goal="Establish the local memref baseline before AllocExtraBuffer.",
    ),
)


ALL_CHECKPOINTS: tuple[Checkpoint, ...] = (S0_INPUT,) + S8_LOCAL_CHECKPOINTS


def list_checkpoints() -> tuple[Checkpoint, ...]:
    return ALL_CHECKPOINTS


def get_checkpoint(checkpoint_id: str) -> Checkpoint:
    normalized = checkpoint_id.upper()
    for checkpoint in ALL_CHECKPOINTS:
        if checkpoint.checkpoint_id.upper() == normalized:
            return checkpoint
    known = ", ".join(checkpoint.checkpoint_id for checkpoint in ALL_CHECKPOINTS)
    raise KeyError(f"unknown checkpoint {checkpoint_id!r}; known checkpoints: {known}")


def reverse_local_plan_order() -> tuple[Checkpoint, ...]:
    """Return the recommended S8 implementation order."""

    return tuple(sorted(S8_LOCAL_CHECKPOINTS, key=lambda item: item.order, reverse=True))
