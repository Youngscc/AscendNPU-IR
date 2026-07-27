#!/usr/bin/env python3
"""Contract tests for failure parity, diff evidence and triage aggregation."""

from __future__ import annotations

import collections
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

from validation_common import (  # noqa: E402
    FailureSignature,
    FailureTaxonomy,
    added_difference_fields,
    aggregate_by_seed,
    cluster_by_difference,
    compare_failure,
    counter_evidence,
    cv2pm_failure_signature,
    diagnostic_lines,
    model_failure_signature,
    normalize_diagnostic_line,
    pivot_scenario_adapter,
)

TAXONOMY = FailureTaxonomy.load(ROOT / "config/failure_taxonomy.tsv")


# --- normalization -------------------------------------------------------
# The crash diagnostic reads uninitialized memory, so its integer differs on
# every run.  Without normalization the same failure could not even compare
# equal to itself.
first = normalize_diagnostic_line(
    'loc("a"("/tmp/09-persistent-matmul.EZAMx3.py":105:20)): error: out '
    "operands and numResults mismatch when replacing results (4214838096 vs 1)"
)
second = normalize_diagnostic_line(
    'loc("a"("/tmp/09-persistent-matmul.QQQQQQ.py":105:20)): error: out '
    "operands and numResults mismatch when replacing results (1833619152 vs 1)"
)
assert first == second, (first, second)
assert "loc(" not in first
assert "/tmp/" not in first

# The driver's own trailer carries no failure-reason information.
assert diagnostic_lines(
    "[ERROR] Failed to run cv2pm-bishengir-compile to local PlanMemory\n"
) == []

# Stack dumps are machine specific and must not reach the classifier.
crash = diagnostic_lines(
    "loc(\"x\"): error: 'scope.scope' op Failed to collect vector loop "
    "tiling info\n"
    "PLEASE submit a bug report to https://github.com/llvm/llvm-project\n"
    "Stack dump:\n"
    " #0 0x0000000105f3e954 llvm::sys::PrintStackTrace\n"
)
assert crash == ["'scope.scope' op Failed to collect vector loop tiling info"], crash


# --- taxonomy ------------------------------------------------------------
signature = cv2pm_failure_signature(
    {
        "returncode": 1,
        "timeout": False,
        "stderr": (
            "[hivm-bind-sub-block] revert matmul_kernel_mix_aiv: no store/copy "
            "op was tiled\n"
            "loc(\"a\"(\"/tmp/x.py\":105:20)): error: 'hivm.hir.copy' op "
            "Unsupported copy from cbuf to cbuf!\n"
            "[ERROR] Failed to run cv2pm-bishengir-compile to local PlanMemory\n"
        ),
    },
    TAXONOMY,
)
assert signature.termination == "error"
# The bind-sub-block revert notice is informational and must not be taken as
# the primary reason.
assert signature.primary == "copy.cbuf_to_cbuf", signature.classes

aborted = cv2pm_failure_signature(
    {
        "returncode": -6,
        "timeout": False,
        "stderr": (
            "loc(\"a\"): error: 'scope.scope' op Failed to collect vector loop "
            "tiling info\n"
            "loc(\"b\"): error: out operands and numResults mismatch when "
            "replacing results (4214838096 vs 1)\n"
            "Stack dump:\n"
        ),
    },
    TAXONOMY,
)
assert aborted.termination == "abort"
# For a crash the first diagnostic is the meaningful one.  The follow-on
# numResults mismatch is cascade severity: corruption caused by the first
# failure, which a model without that compiler bug cannot reproduce, so it must
# be excluded from the compared classes entirely.
assert aborted.classes == ("tiling.collect_vector_loop_info",), aborted.classes
assert aborted.primary == "tiling.collect_vector_loop_info"
# ... but it is still retained for display.
assert any("numResults mismatch" in entry for entry in aborted.raw), aborted.raw

wrapped = cv2pm_failure_signature(
    {
        "returncode": 1,
        "timeout": False,
        "stderr": (
            'loc("a"): error: Failed to run BiShengHIR pipeline\n'
            'loc("b"): error: \'scope.scope\' op Failed to collect vector '
            'loop tiling info\n'
            '[ERROR] Failed to run BiShengIR pipeline\n'
        ),
    },
    TAXONOMY,
)
assert wrapped.classes == ("tiling.collect_vector_loop_info",), wrapped.classes

# An unknown diagnostic must never be bucketed with a known class.
unknown = cv2pm_failure_signature(
    {"returncode": 1, "timeout": False,
     "stderr": "loc(\"a\"): error: 'x.y' op brand new failure\n"},
    TAXONOMY,
)
assert unknown.primary.startswith("unclassified:"), unknown.primary
assert compare_failure(unknown, aborted) == ["failure_class"]


# --- three-tier comparison ----------------------------------------------
exact_model = model_failure_signature(
    {"precision": "exact", "status": "success"}, "", 0, TAXONOMY
)
assert not exact_model.failed
# The worst error the model can make: an exact plan for an input the real
# compiler cannot compile at all.
assert compare_failure(exact_model, signature) == ["failure_presence"]

# Both sides succeeding is a match, not a comparison to skip.
assert compare_failure(exact_model, FailureSignature("none")) == []

matching = FailureSignature("error", ("copy.cbuf_to_cbuf",))
assert compare_failure(matching, signature) == []

# Same primary reason, different tail: reported as a sequence difference only.
tail = FailureSignature(
    "error", ("copy.cbuf_to_cbuf", "store.rank_mismatch")
)
assert compare_failure(tail, signature) == ["failure_sequence"]

# The model has no crash path, so a cv2pm SIGABRT with the same primary reason
# must still match a model "error": termination kind is reported, not compared.
model_error = FailureSignature("error", ("tiling.collect_vector_loop_info",))
assert compare_failure(model_error, aborted) == []

blocker = model_failure_signature(
    {"precision": "incomplete", "status": "blocker",
     "diagnostics": ["TileCubeVectorLoop[f]: scope.scope: Failed to collect "
                     "vector loop tiling info"]},
    "", 1, TAXONOMY,
)
assert blocker.primary == "tiling.collect_vector_loop_info", blocker.classes
assert compare_failure(blocker, aborted) == []


# --- difference evidence -------------------------------------------------
evidence = counter_evidence(
    "plan",
    collections.Counter({(65536, 12320): 1, (256, 64): 2}),
    collections.Counter({(256, 64): 2, (65536, 20512): 1}),
)
assert "model_only[65536/12320]" in evidence, evidence
assert "cv2pm_only[65536/20512]" in evidence, evidence
# Entries present on both sides are not evidence of anything.
assert "256/64" not in evidence, evidence

assert added_difference_fields("plan,lifetime", "required,plan,lifetime") == (
    "required",
)
assert added_difference_fields("required,plan", "plan") == ()


# --- triage aggregation --------------------------------------------------
rows = [
    # Fails at every seed: divergence is upstream of PlanMemory.
    {"scenario": "s1", "adapter": "a", "seed": 0, "status": "different",
     "differences": "peak"},
    {"scenario": "s1", "adapter": "a", "seed": 1, "status": "different",
     "differences": "peak"},
    # Fails at one seed only: divergence is inside PlanMemory.
    {"scenario": "s1", "adapter": "b", "seed": 0, "status": "different",
     "differences": "plan"},
    {"scenario": "s1", "adapter": "b", "seed": 1, "status": "matched",
     "differences": ""},
    {"scenario": "s2", "adapter": "a", "seed": 0, "status": "different",
     "differences": "peak"},
    {"scenario": "s2", "adapter": "a", "seed": 1, "status": "different",
     "differences": "peak"},
    {"scenario": "s2", "adapter": "b", "seed": 0, "status": "matched",
     "differences": ""},
    {"scenario": "s2", "adapter": "b", "seed": 1, "status": "matched",
     "differences": ""},
]
for row in rows:
    row["kind"] = "plan"
split = aggregate_by_seed(rows, expected_seeds=(0, 1))
assert [entry["adapter"] for entry in split["all_seed_failures"]] == ["a", "a"]
assert [entry["adapter"] for entry in split["seed_varying"]] == ["b"]
assert split["inconclusive"] == []

single_seed = aggregate_by_seed(rows[:1], expected_seeds=range(20))
assert single_seed["all_seed_failures"] == []
assert single_seed["seed_varying"] == []
assert single_seed["inconclusive"][0]["missing_seeds"] == list(range(1, 20))

pivot = pivot_scenario_adapter(rows)
# Adapter "a" fails in every scenario it appears in -> input-dependent bug.
assert pivot["input_wide_adapters"] == ["a"], pivot
assert pivot["per_scenario_failing_adapters"] == {"s1": 2}, pivot

clusters = cluster_by_difference(rows)
assert clusters["peak"]["count"] == 4
assert clusters["peak"]["representative"]["adapter"] == "a"
assert clusters["plan"]["count"] == 1

print("[PASS] failure parity, diff evidence and triage aggregation")
