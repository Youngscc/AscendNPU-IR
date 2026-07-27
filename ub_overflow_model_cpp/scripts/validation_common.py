#!/usr/bin/env python3
"""Shared helpers for cv2pm/model validation: failure parity and diff evidence.

Two facilities live here because both the matrix validator and the standalone
diagnosis tools need them:

1. Pre-PlanMemory failure parity.  cv2pm failing before PlanMemory is a
   deterministic, comparable output, not a case to skip.  The comparison is
   done at the highest tier: the canonical failure *reason class* must match,
   not merely the fact that both sides failed.
2. Minimal difference evidence.  Reporting that "plan" differs is not
   actionable; reporting which buffers differ is.
"""

from __future__ import annotations

import collections
from dataclasses import dataclass, field
from pathlib import Path
import re
from typing import Any, Counter, Iterable, Sequence

MODULE = Path(__file__).resolve().parents[1]
DEFAULT_TAXONOMY = MODULE / "config/failure_taxonomy.tsv"

# Everything from these markers onward is an LLVM crash report: a stack dump
# with frame addresses and the exact argv.  None of it is stable across runs or
# machines, and none of it carries failure-reason information the first
# diagnostics do not already carry.
_CRASH_TAIL_MARKERS = (
    "PLEASE submit a bug report",
    "Stack dump:",
)

# The oracle driver's own trailer; it says nothing about why compilation failed.
_DRIVER_TRAILER = re.compile(r"^\[ERROR\] Failed to run \S+ to local PlanMemory")

_LOC_PREFIX = re.compile(r'^loc\((?:"[^"]*"|[^()]|\([^()]*\))*\)\s*:\s*')
_HEX = re.compile(r"\b0x[0-9a-fA-F]+\b")
_NUMBER = re.compile(r"\b\d+\b")
_TEMP_PATH = re.compile(r"/tmp/[^\s\"')]+")
_WHITESPACE = re.compile(r"\s+")

_DIAGNOSTIC_LINE = re.compile(r"\b(error|warning)\s*:\s*(?P<body>.*)$")


def _strip_crash_tail(text: str) -> str:
    cut = len(text)
    for marker in _CRASH_TAIL_MARKERS:
        found = text.find(marker)
        if found != -1:
            cut = min(cut, found)
    return text[:cut]


def normalize_diagnostic_line(line: str) -> str:
    """Reduce one diagnostic to a run-independent form.

    Removes the loc() prefix (temp paths and line numbers vary per dump),
    pointer values, and integer literals.  The crash message
    "out operands and numResults mismatch when replacing results (4214838096
    vs 1)" reads uninitialized memory, so its numbers differ on every run;
    without this normalization such a failure could never compare equal to
    itself.
    """
    text = line.strip()
    text = _LOC_PREFIX.sub("", text)
    match = _DIAGNOSTIC_LINE.search(text)
    if match:
        text = match.group("body")
    text = _TEMP_PATH.sub("<path>", text)
    text = _HEX.sub("<addr>", text)
    text = _NUMBER.sub("<n>", text)
    return _WHITESPACE.sub(" ", text).strip()


def diagnostic_lines(stderr: str) -> list[str]:
    """Extract the normalized, ordered diagnostics from a stderr blob."""
    result: list[str] = []
    for raw in _strip_crash_tail(stderr).splitlines():
        stripped = raw.strip()
        if not stripped or _DRIVER_TRAILER.match(stripped):
            continue
        # Keep MLIR diagnostics and bracketed pass notices; drop everything
        # else (progress chatter, blank separators).
        if not (_DIAGNOSTIC_LINE.search(stripped) or stripped.startswith("[")):
            continue
        normalized = normalize_diagnostic_line(stripped)
        if normalized:
            result.append(normalized)
    return result


@dataclass(frozen=True)
class TaxonomyEntry:
    class_id: str
    side: str
    severity: str
    pattern: re.Pattern[str]


class FailureTaxonomy:
    def __init__(self, entries: Sequence[TaxonomyEntry]) -> None:
        self._entries = list(entries)
        self._not_compared = {
            entry.class_id for entry in entries
            if entry.severity in ("info", "cascade")
        }

    @classmethod
    def load(cls, path: Path = DEFAULT_TAXONOMY) -> "FailureTaxonomy":
        entries: list[TaxonomyEntry] = []
        with path.open(encoding="utf-8") as stream:
            for raw in stream:
                line = raw.rstrip("\n")
                if not line.strip() or line.lstrip().startswith("#"):
                    continue
                fields = line.split("\t")
                if fields[0] == "class_id":
                    continue
                if len(fields) < 4:
                    raise ValueError(f"malformed taxonomy row: {line!r}")
                class_id, side, severity, pattern = fields[:4]
                if side not in ("cv2pm", "model", "both"):
                    raise ValueError(f"unknown taxonomy side {side!r}")
                if severity not in ("fatal", "info", "cascade"):
                    raise ValueError(
                        f"unknown taxonomy severity {severity!r}"
                    )
                entries.append(
                    TaxonomyEntry(class_id, side, severity,
                                  re.compile(pattern))
                )
        return cls(entries)

    def classify_line(self, line: str, side: str) -> str:
        for entry in self._entries:
            if entry.side not in (side, "both"):
                continue
            if entry.pattern.search(line):
                return entry.class_id
        # Deliberately unique: an unknown failure must surface as a mismatch
        # and force a taxonomy update instead of silently joining a bucket.
        return f"unclassified:{line}"

    def is_compared(self, class_id: str) -> bool:
        return class_id not in self._not_compared

    def classify(self, lines: Iterable[str], side: str) -> tuple[str, ...]:
        """Classify diagnostics, keeping only the independently comparable ones.

        Dropped: informational pass notices such as "[hivm-bind-sub-block]
        revert ...: no store/copy op was tiled", which cv2pm alone emits, and
        cascade failures such as the numResults mismatch that follows a failed
        tiling rollback.  Keeping either would make every affected case report a
        spurious failure_sequence difference against a model that cannot
        reproduce them.
        """
        return tuple(
            class_id
            for class_id in (
                self.classify_line(line, side) for line in lines
            )
            if self.is_compared(class_id)
        )


@dataclass(frozen=True)
class FailureSignature:
    """How one side failed before PlanMemory."""

    # "error" (non-zero exit with diagnostics), "abort" (killed by a signal),
    # "timeout", or "none" (did not fail).
    termination: str
    classes: tuple[str, ...] = ()
    raw: tuple[str, ...] = ()

    @property
    def failed(self) -> bool:
        return self.termination != "none"

    @property
    def primary(self) -> str:
        """The first fatal reason.

        For crashes this is the meaningful one: the follow-on message that
        actually trips the assertion is corruption caused by the first failure,
        and a model without that bug cannot reproduce it.
        """
        return self.classes[0] if self.classes else ""

    def describe(self) -> str:
        return f"{self.termination}:{self.primary or '-'}"


def _termination_for(returncode: int, timeout: bool) -> str:
    if timeout:
        return "timeout"
    if returncode == 0:
        return "none"
    return "abort" if returncode < 0 else "error"


def cv2pm_failure_signature(
    pipeline: dict[str, Any], taxonomy: FailureTaxonomy
) -> FailureSignature:
    termination = _termination_for(
        int(pipeline.get("returncode") or 0), bool(pipeline.get("timeout"))
    )
    if termination == "none":
        return FailureSignature("none")
    lines = diagnostic_lines(str(pipeline.get("stderr", "")))
    return FailureSignature(
        termination, taxonomy.classify(lines, "cv2pm"), tuple(lines)
    )


def model_failure_signature(
    payload: dict[str, Any],
    stderr: str,
    returncode: int,
    taxonomy: FailureTaxonomy,
) -> FailureSignature:
    """Classify the model's own refusal to produce an exact plan.

    The model never crashes on purpose, so a cv2pm SIGABRT can only ever be
    matched by a model "error" termination.  Termination kind is therefore
    reported but compared leniently; the reason class is compared strictly.
    """
    result = payload.get("result") if isinstance(payload.get("result"), dict) else payload
    precision = str(result.get("precision", ""))
    if precision == "exact":
        return FailureSignature("none")
    diagnostics = [
        normalize_diagnostic_line(str(entry))
        for entry in result.get("diagnostics", [])
    ]
    diagnostics = [entry for entry in diagnostics if entry]
    if not diagnostics:
        diagnostics = diagnostic_lines(stderr)
    termination = "abort" if returncode < 0 else "error"
    return FailureSignature(
        termination, taxonomy.classify(diagnostics, "model"), tuple(diagnostics)
    )


def compare_failure(
    model: FailureSignature, oracle: FailureSignature
) -> list[str]:
    """Highest-tier failure parity check.

    Returns the list of failed check names.  Three tiers, reported separately
    so a partial match is visible rather than collapsing to one boolean:

      failure_presence  both sides must fail (guards the false-negative case
                        where the model happily plans an input the compiler
                        cannot even compile)
      failure_class     the primary reason class must be identical
      failure_sequence  the full ordered class sequence must be identical
    """
    if not oracle.failed and not model.failed:
        return []
    if oracle.failed != model.failed:
        return ["failure_presence"]
    differences: list[str] = []
    if model.primary != oracle.primary:
        differences.append("failure_class")
    elif model.classes != oracle.classes:
        # Only meaningful once the primary class agrees; otherwise it is noise.
        differences.append("failure_sequence")
    return differences


# --------------------------------------------------------------------------
# Minimal difference evidence
# --------------------------------------------------------------------------

_MAX_EVIDENCE_ITEMS = 4


def _format_item(item: Any) -> str:
    if isinstance(item, tuple):
        return "/".join(str(part) for part in item)
    return str(item)


def counter_evidence(
    name: str, model: Counter[Any], oracle: Counter[Any]
) -> str:
    """Render the symmetric difference of two multisets compactly.

    Without this the report says "plan differs" and every case needs a manual
    re-run to find out which buffer differs.
    """
    only_model = model - oracle
    only_oracle = oracle - model
    parts: list[str] = []
    for label, counter in (("model", only_model), ("cv2pm", only_oracle)):
        items = sorted(counter.items(), key=lambda pair: str(pair[0]))
        if not items:
            continue
        rendered = ", ".join(
            f"{_format_item(key)}x{count}" if count > 1 else _format_item(key)
            for key, count in items[:_MAX_EVIDENCE_ITEMS]
        )
        if len(items) > _MAX_EVIDENCE_ITEMS:
            rendered += f", +{len(items) - _MAX_EVIDENCE_ITEMS} more"
        parts.append(f"{label}_only[{rendered}]")
    return f"{name}: " + "; ".join(parts) if parts else ""


def scalar_evidence(name: str, model: Any, oracle: Any) -> str:
    return f"{name}: model={model} cv2pm={oracle}"


def added_difference_fields(previous: str, current: str) -> tuple[str, ...]:
    """Return comparison dimensions that became newly wrong.

    A validation baseline is a ratchet, not a blanket waiver for a case that
    was already failing.  If a previous ``plan`` mismatch grows into
    ``required,peak,plan``, the two scalar fields are regressions even though
    the (scenario, adapter, seed) key already existed in the baseline.
    """
    old = {field for field in previous.split(",") if field}
    new = {field for field in current.split(",") if field}
    return tuple(sorted(new - old))


@dataclass
class ComparisonOutcome:
    """Result of comparing one (scenario, adapter, seed) against cv2pm."""

    differences: list[str] = field(default_factory=list)
    evidence: list[str] = field(default_factory=list)
    model_exact: bool = False
    oracle_comparable: bool = True

    @property
    def matched(self) -> bool:
        return not self.differences

    def evidence_text(self) -> str:
        return " | ".join(entry for entry in self.evidence if entry)


def aggregate_by_seed(
    rows: Sequence[dict[str, Any]],
    expected_seeds: Iterable[int] = range(20),
) -> dict[str, Any]:
    """Separate proven seed variation from all-seed and incomplete evidence.

    Seeing both a match and a mismatch for one case proves that the final
    divergence varies with PlanMemory's seed.  Seeing a mismatch for *every*
    seed is only a strong clue that the root cause is before PlanMemory; a
    seed-independent PlanMemory bug remains possible and stage evidence is
    still required.  Most importantly, a one-seed development run cannot make
    either all-seed claim and is reported as inconclusive.
    """
    expected = set(expected_seeds)
    if not expected:
        raise ValueError("expected_seeds must not be empty")
    seeds_by_case: dict[tuple[str, str], set[int]] = collections.defaultdict(set)
    failed_by_case: dict[tuple[str, str], set[int]] = collections.defaultdict(set)
    for row in rows:
        key = (str(row["scenario"]), str(row["adapter"]))
        seed = int(row["seed"])
        seeds_by_case[key].add(seed)
        if row["status"] != "matched":
            failed_by_case[key].add(seed)
    all_seed, varying, inconclusive = [], [], []
    for key, failed in sorted(failed_by_case.items()):
        if not failed:
            continue
        observed = seeds_by_case[key]
        entry = {
            "scenario": key[0],
            "adapter": key[1],
            "failed_seeds": len(failed),
            "observed_seeds": len(observed),
            "expected_seeds": len(expected),
            "missing_seeds": sorted(expected - observed),
        }
        if observed - failed:
            varying.append(entry)
        elif observed == expected:
            all_seed.append(entry)
        else:
            inconclusive.append(entry)
    return {
        "all_seed_failures": all_seed,
        "seed_varying": varying,
        "inconclusive": inconclusive,
    }


def pivot_scenario_adapter(
    rows: Sequence[dict[str, Any]],
) -> dict[str, Any]:
    """Classify failures by their scenario x adapter footprint.

    An adapter failing across every scenario points at an input-dependent pass
    modeling bug.  A scenario failing across many adapters points at option
    handling specific to that scenario.  Failures only at intersections point
    at an interaction.
    """
    scenarios = {str(row["scenario"]) for row in rows}
    adapters = {str(row["adapter"]) for row in rows}
    failing_pairs = {
        (str(row["scenario"]), str(row["adapter"]))
        for row in rows
        if row["status"] != "matched"
    }
    scenarios_of: dict[str, set[str]] = collections.defaultdict(set)
    adapters_of: dict[str, set[str]] = collections.defaultdict(set)
    for scenario, adapter in failing_pairs:
        scenarios_of[adapter].add(scenario)
        adapters_of[scenario].add(adapter)
    present_scenarios: dict[str, set[str]] = collections.defaultdict(set)
    for row in rows:
        present_scenarios[str(row["adapter"])].add(str(row["scenario"]))
    input_wide = sorted(
        adapter
        for adapter, hit in scenarios_of.items()
        if hit == present_scenarios[adapter] and len(hit) > 1
    )
    option_wide = sorted(
        (scenario, len(hit))
        for scenario, hit in adapters_of.items()
        if len(hit) > 1
    )
    return {
        "scenarios": len(scenarios),
        "adapters": len(adapters),
        "failing_pairs": len(failing_pairs),
        "input_wide_adapters": input_wide,
        "per_scenario_failing_adapters": dict(sorted(option_wide)),
    }


def cluster_by_difference(
    rows: Sequence[dict[str, Any]],
) -> dict[str, dict[str, Any]]:
    """Group failures by their difference signature, smallest cluster first."""
    clusters: dict[str, list[dict[str, Any]]] = collections.defaultdict(list)
    for row in rows:
        if row["status"] == "matched":
            continue
        clusters[str(row.get("differences", ""))].append(row)
    summary: dict[str, dict[str, Any]] = {}
    for signature, members in clusters.items():
        adapters = collections.Counter(str(row["adapter"]) for row in members)
        scenarios = collections.Counter(str(row["scenario"]) for row in members)
        representative = min(
            members,
            key=lambda row: (str(row["adapter"]), str(row["scenario"]),
                             int(row["seed"])),
        )
        summary[signature] = {
            "count": len(members),
            "adapters": len(adapters),
            "scenarios": len(scenarios),
            "representative": {
                "scenario": representative["scenario"],
                "adapter": representative["adapter"],
                "seed": representative["seed"],
            },
            "top_adapters": adapters.most_common(5),
        }
    return dict(
        sorted(summary.items(), key=lambda item: -item[1]["count"])
    )
