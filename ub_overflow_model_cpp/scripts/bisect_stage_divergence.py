#!/usr/bin/env python3
"""Find the FIRST pipeline stage where the UB model diverges from cv2pm.

The matrix validator compares only the final PlanMemory facts.  When it reports
that "plan" differs there is no way to tell which of ~30 modeled passes is at
fault, so localisation costs hours per bug.  Both sides can already dump
per-stage snapshots (cv2pm via --dump-stage-oracle-dir, the model via --debug
--debug-dir); this tool aligns those snapshots and reports the earliest stage
whose content disagrees, turning the search into a pointed one.

The two snapshot formats differ (cv2pm writes generic MLIR text, the model
writes a hex-encoded TSV of its own IR), so comparison uses derived invariants
rather than bytes.  After SplitMixKernel cv2pm deliberately retains both the
AIC and AIV functions, while the UB model retains only the UB-relevant AIV
projection; the cv2pm snapshot is therefore projected to its AIV function
before deriving invariants.  Comparing the whole cv2pm module here would
manufacture a false first divergence at SplitMixKernel.

  ops    multiset of operation names.  Robust on both sides and sensitive to
         essentially every pass modeling error, since a mis-modeled pass
         creates, erases or replaces operations.
  types  multiset of memref<...>/tensor<...> type literals.  Directly UB
         relevant: it changes when a buffer's shape or element type is wrong.
         Approximate, because type literals nested inside attributes (function
         signatures) are counted too; it is used only as a tie-breaker after
         `ops` agrees, and the report says which invariant fired.

Usage:
    bisect_stage_divergence.py --config preload_auto_mb \\
        --input python_tutorial_06-fused-attention.ttadapter [--seed 0]
"""

from __future__ import annotations

import argparse
import binascii
import collections
import csv
import os
from pathlib import Path
import re
import subprocess
import sys
from typing import Counter

SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from build_cv2pm_oracle_cache import scenario_arguments  # noqa: E402
from run_corpus_matrix import (  # noqa: E402
    DEFAULT_CACHE,
    DEFAULT_CONVERTER,
    DEFAULT_CV2PM,
    DEFAULT_MATRIX,
    DEFAULT_MODEL,
    DEFAULT_PROFILES,
    load_scenarios,
    materialize_model_input,
    model_arguments,
    profile_inputs,
)

MODULE = Path(__file__).resolve().parents[1]
DEFAULT_ALIGNMENT = MODULE / "config/stage_alignment.tsv"

_MLIR_OP = re.compile(r'"([A-Za-z_][\w.]*\.[\w.]+)"\s*\(')
_TYPE_LITERAL = re.compile(r"\b(?:memref|tensor)<[^<>]*(?:<[^<>]*>[^<>]*)*>")
_HEX_FIELD = re.compile(r"^[0-9a-f]+$")
_TOP_LEVEL_FUNC = re.compile(r'^  "func\.func"', re.MULTILINE)
_AIV_FUNCTION_ATTR = "hivm.func_core_type = #hivm.func_core_type<AIV>"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--config", "--scenario", dest="scenario",
                        required=True)
    parser.add_argument("--input", dest="adapter", required=True)
    parser.add_argument("--seed", type=int, default=0)
    parser.add_argument("--matrix", type=Path, default=DEFAULT_MATRIX)
    parser.add_argument("--profiles-root", type=Path, default=DEFAULT_PROFILES)
    parser.add_argument("--cache-dir", type=Path, default=DEFAULT_CACHE)
    parser.add_argument("--model", type=Path, default=DEFAULT_MODEL)
    parser.add_argument("--cv2pm", type=Path, default=DEFAULT_CV2PM)
    parser.add_argument("--converter", type=Path, default=DEFAULT_CONVERTER)
    parser.add_argument("--alignment", type=Path, default=DEFAULT_ALIGNMENT)
    parser.add_argument(
        "--work-dir", type=Path, default=None,
        help="keep snapshots here instead of a fresh temporary directory",
    )
    parser.add_argument("--cv2pm-timeout", type=int, default=900)
    parser.add_argument("--model-timeout", type=int, default=300)
    parser.add_argument(
        "--show", type=int, default=8,
        help="how many differing entries to print per invariant",
    )
    return parser.parse_args()


def load_alignment(path: Path) -> list[tuple[str, str, int, str]]:
    result: list[tuple[str, str, int, str]] = []
    with path.open(encoding="utf-8") as stream:
        for raw in stream:
            line = raw.rstrip("\n")
            if not line.strip() or line.lstrip().startswith("#"):
                continue
            fields = line.split("\t")
            if fields[0] == "cv2pm_stage":
                continue
            if len(fields) < 3:
                raise ValueError(f"malformed alignment row: {line!r}")
            result.append(
                (fields[0], fields[1], int(fields[2]),
                 fields[3] if len(fields) > 3 else "")
            )
    return result


def ub_relevant_mlir_projection(text: str) -> str:
    """Select cv2pm's AIV function when SplitMix retained both projections.

    Generic MLIR prints top-level functions at two-space indentation.  Splitting
    on those boundaries is intentionally simpler and safer than attempting to
    parse nested MLIR delimiters here.  If there is no AIV function (the
    pre-SplitMix stages), the whole module remains the comparable program.
    """
    starts = list(_TOP_LEVEL_FUNC.finditer(text))
    aiv_functions: list[str] = []
    for index, match in enumerate(starts):
        end = starts[index + 1].start() if index + 1 < len(starts) else len(text)
        function = text[match.start():end]
        if _AIV_FUNCTION_ATTR in function:
            aiv_functions.append(function)
    return "\n".join(aiv_functions) if aiv_functions else text


def invariants_from_mlir(text: str) -> tuple[Counter[str], Counter[str]]:
    projection = ub_relevant_mlir_projection(text)
    operations = collections.Counter(_MLIR_OP.findall(projection))
    # The model artifact retains its container row while an AIV-only textual
    # projection does not.  Module existence is not a pass-semantic invariant.
    operations.pop("builtin.module", None)
    return operations, collections.Counter(_TYPE_LITERAL.findall(projection))


def invariants_from_model_artifact(
    text: str,
) -> tuple[Counter[str], Counter[str]]:
    ops: Counter[str] = collections.Counter()
    types: Counter[str] = collections.Counter()
    for line in text.splitlines():
        fields = line.split("\t")
        if not fields or fields[0] != "OP" or len(fields) < 7:
            continue
        name = decode_hex(fields[6])
        if name:
            ops[name] += 1
        for field in fields[7:]:
            for chunk in field.split(", "):
                decoded = decode_hex(chunk)
                if decoded:
                    types.update(_TYPE_LITERAL.findall(decoded))
    ops.pop("builtin.module", None)
    return ops, types


def decode_hex(value: str) -> str:
    value = value.strip()
    if not value or len(value) % 2 or not _HEX_FIELD.match(value):
        return ""
    try:
        return binascii.unhexlify(value).decode("utf-8", errors="replace")
    except binascii.Error:
        return ""


def run_cv2pm_stages(
    args: argparse.Namespace, scenario: dict[str, str], source: Path,
    stage_dir: Path,
) -> None:
    stage_dir.mkdir(parents=True, exist_ok=True)
    environment = os.environ.copy()
    # Stop before PlanMemory: the seeded retry is compared by the matrix
    # validator, and running it here only costs time.
    environment["BISHENGIR_STOP_BEFORE_LOCAL_PLAN_MEMORY"] = "1"
    command = [
        str(args.cv2pm.resolve()), str(source), "--mlir-disable-threading",
        f"--dump-stage-oracle-dir={stage_dir}",
        *scenario_arguments(scenario),
        "-o", os.devnull,
    ]
    completed = subprocess.run(
        command, text=True, capture_output=True,
        timeout=args.cv2pm_timeout, check=False, env=environment,
    )
    produced = list(stage_dir.glob("*.generic.mlir"))
    if not produced:
        raise RuntimeError(
            "cv2pm produced no stage snapshots (rc="
            f"{completed.returncode}): {completed.stderr[-800:]}"
        )


def run_model_stages(
    args: argparse.Namespace, scenario: dict[str, str], model_input: Path,
    debug_dir: Path,
) -> None:
    debug_dir.mkdir(parents=True, exist_ok=True)
    command = [
        str(args.model.resolve()), str(model_input),
        *model_arguments(scenario, args.seed),
        "--debug", f"--debug-dir={debug_dir}",
    ]
    completed = subprocess.run(
        command, text=True, capture_output=True,
        timeout=args.model_timeout, check=False,
    )
    produced = list(debug_dir.glob("*_after_*.tsv"))
    if not produced:
        raise RuntimeError(
            "the model produced no debug artifacts (rc="
            f"{completed.returncode}): {completed.stderr[-800:]}"
        )


def cv2pm_stage_file(stage_dir: Path, stage: str) -> Path | None:
    phase, _, name = stage.partition("-")
    safe = re.sub(r"[^0-9A-Za-z]", "_", name)
    matches = sorted(stage_dir.glob(f"{phase}-*-{safe}.generic.mlir"))
    return matches[0] if matches else None


def model_stage_file(
    debug_dir: Path, stage: str, occurrence: int
) -> Path | None:
    safe = re.sub(r"[^0-9A-Za-z]", "_", stage)
    matches = sorted(debug_dir.glob(f"*_after_{safe}.tsv"))
    if len(matches) < occurrence:
        return None
    return matches[occurrence - 1]


def format_difference(
    label: str, model: Counter[str], oracle: Counter[str], limit: int
) -> list[str]:
    only_model = model - oracle
    only_oracle = oracle - model
    lines: list[str] = []
    for side, counter in (("model only", only_model), ("cv2pm only", only_oracle)):
        if not counter:
            continue
        entries = sorted(counter.items(), key=lambda pair: (-pair[1], pair[0]))
        rendered = ", ".join(
            f"{key} x{count}" for key, count in entries[:limit]
        )
        if len(entries) > limit:
            rendered += f", +{len(entries) - limit} more"
        lines.append(f"    {label} {side}: {rendered}")
    return lines


def main() -> int:
    args = parse_args()
    try:
        scenarios = load_scenarios(args.matrix.resolve(), {args.scenario})
        alignment = load_alignment(args.alignment.resolve())
    except (OSError, ValueError) as error:
        print(f"[ERROR] {error}", file=sys.stderr)
        return 2
    scenario = scenarios[0]
    try:
        inputs = profile_inputs(
            args.profiles_root.resolve(), scenario["pre_cv_profile"],
            {args.adapter}, 0,
        )
    except ValueError as error:
        print(f"[ERROR] {error}", file=sys.stderr)
        return 2
    if not inputs:
        print(f"[ERROR] no input named {args.adapter}", file=sys.stderr)
        return 2
    adapter, source = inputs[0]

    if args.work_dir:
        work = args.work_dir.resolve()
        work.mkdir(parents=True, exist_ok=True)
    else:
        import tempfile
        work = Path(tempfile.mkdtemp(prefix="cvub-bisect-"))
    print(f"work dir (kept): {work}")

    stage_dir = work / "cv2pm_stages"
    debug_dir = work / "model_stages"
    try:
        model_input = materialize_model_input(
            source, args.converter.resolve(),
            args.cache_dir.resolve() / "_model_inputs",
        )
        print(f"running cv2pm stage dump for {scenario['scenario_id']} / {adapter} ...",
              flush=True)
        run_cv2pm_stages(args, scenario, source, stage_dir)
        print("running model stage dump ...", flush=True)
        run_model_stages(args, scenario, model_input, debug_dir)
    except (RuntimeError, subprocess.TimeoutExpired) as error:
        print(f"[ERROR] {error}", file=sys.stderr)
        return 2

    print(f"\n{'stage':44s} {'ops':>18s} {'types':>18s}  verdict")
    first_ops: list[str] = []
    first_types: list[str] = []
    for cv2pm_stage, model_stage, occurrence, _note in alignment:
        oracle_path = cv2pm_stage_file(stage_dir, cv2pm_stage)
        model_path = model_stage_file(debug_dir, model_stage, occurrence)
        if oracle_path is None or model_path is None:
            missing = "cv2pm" if oracle_path is None else "model"
            print(f"{cv2pm_stage:44s} {'-':>18s} {'-':>18s}  "
                  f"skipped ({missing} snapshot absent)")
            continue
        oracle_ops, oracle_types = invariants_from_mlir(
            oracle_path.read_text(encoding="utf-8", errors="replace")
        )
        model_ops, model_types = invariants_from_model_artifact(
            model_path.read_text(encoding="utf-8", errors="replace")
        )
        if not model_ops:
            # After bufferization the model stops emitting its generic-IR
            # schema and dumps purpose-built semantic state instead, which
            # carries no operation list.  Claiming a difference here would be
            # an artifact of the format, not a finding.
            print(f"{cv2pm_stage:44s} {'-':>18s} {'-':>18s}  "
                  f"skipped (model artifact is not generic IR)")
            continue
        ops_match = model_ops == oracle_ops
        types_match = model_types == oracle_types
        verdict = "same" if ops_match and types_match else (
            "OPS DIFFER" if not ops_match else "types differ (advisory)"
        )
        print(
            f"{cv2pm_stage:44s} "
            f"{sum(model_ops.values()):8d}/{sum(oracle_ops.values()):<9d} "
            f"{sum(model_types.values()):8d}/{sum(oracle_types.values()):<9d}  "
            f"{verdict}"
        )
        if not ops_match and not first_ops:
            first_ops = [
                f"\nFIRST OPERATION DIVERGENCE: {cv2pm_stage}",
                *format_difference("ops", model_ops, oracle_ops, args.show),
                f"    cv2pm snapshot: {oracle_path}",
                f"    model snapshot: {model_path}",
            ]
        if not types_match and not first_types:
            first_types = [
                f"\nfirst type divergence (advisory): {cv2pm_stage}",
                *format_difference("types", model_types, oracle_types,
                                   args.show),
            ]
    if first_ops:
        print("\n".join(first_ops))
    if first_types:
        print("\n".join(first_types))
        if not first_ops:
            print(
                "    note: `types` counts type literals nested in attributes "
                "too, so a small difference here can be a reporting artifact. "
                "Confirm against the two snapshots before treating it as the "
                "root cause."
            )
    if not first_ops and not first_types:
        print(
            "\nall aligned stages agree; the divergence is inside PlanMemory "
            "itself or in a stage with no aligned snapshot"
        )
        return 0
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
