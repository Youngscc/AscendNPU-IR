#!/usr/bin/env python3
"""Unit tests for the cv2pm/model scenario matrix runner."""

from __future__ import annotations

import importlib.util
import gzip
import hashlib
import json
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import sys
import tempfile
import threading
from types import SimpleNamespace


ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts/run_corpus_matrix.py"
SPEC = importlib.util.spec_from_file_location("run_corpus_matrix", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
MODULE = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = MODULE
SPEC.loader.exec_module(MODULE)

BUILDER_SCRIPT = ROOT / "scripts/build_cv2pm_oracle_cache.py"
BUILDER_SPEC = importlib.util.spec_from_file_location(
    "build_cv2pm_oracle_cache", BUILDER_SCRIPT
)
assert BUILDER_SPEC is not None and BUILDER_SPEC.loader is not None
BUILDER = importlib.util.module_from_spec(BUILDER_SPEC)
sys.modules[BUILDER_SPEC.name] = BUILDER
BUILDER_SPEC.loader.exec_module(BUILDER)

rows = MODULE.load_scenarios(
    ROOT / "config/ub_relevant_parameter_scenarios.tsv", set()
)
assert len(rows) == 27
assert len({row["scenario_id"] for row in rows}) == 27
by_name = {row["scenario_id"]: row for row in rows}
assert by_name["production_default"]["pre_cv_profile"] == "base_default"
assert by_name["auto_mb_workspace_2"]["pre_cv_profile"] == "mb_default_ws2"
assert by_name["cv_workspace_manage_off"]["pre_cv_profile"] == "cv_manage_off"

selected = MODULE.load_scenarios(
    ROOT / "config/ub_relevant_parameter_scenarios.tsv",
    {"production_default", "auto_mb_default"},
)
assert [row["scenario_id"] for row in selected] == [
    "production_default", "auto_mb_default"
]

arguments = MODULE.model_arguments(by_name["storage_align_off"], 13)
assert "--enable-hivm-auto-storage-align=false" in arguments
assert "--plan-memory-seed=13" in arguments
assert "--enable-triton-kernel-compile=true" in arguments
assert "--enable-hivm-cross-core-gss=true" in arguments
assert "--enable-hivm-inject-block-all-sync=false" in arguments
assert "--disable-auto-inject-block-sync=false" in arguments
assert not any("random-seed" in value for value in arguments)
assert not any("disable-cv-pipelining" in value for value in arguments)

workspace_arguments = MODULE.model_arguments(
    by_name["cv_workspace_manage_off"], 0
)
assert "--disable-auto-cv-work-space-manage=true" in workspace_arguments

sync_rows = [row for row in rows if row["scenario_id"].startswith("inject_")]
assert len(sync_rows) == 5
sync_by_name = {row["scenario_id"]: row for row in sync_rows}
normal_arguments = MODULE.model_arguments(
    sync_by_name["inject_block_normal"], 7
)
assert "--enable-hivm-cross-core-gss=false" in normal_arguments
assert "--enable-hivm-inject-block-all-sync=false" in normal_arguments
assert "--disable-auto-inject-block-sync=false" in normal_arguments
block_all_arguments = MODULE.model_arguments(
    sync_by_name["inject_block_all"], 0
)
assert "--enable-hivm-cross-core-gss=true" in block_all_arguments
assert "--enable-hivm-inject-block-all-sync=true" in block_all_arguments
disabled_arguments = MODULE.model_arguments(
    sync_by_name["inject_block_disabled"], 0
)
assert "--disable-auto-inject-block-sync=true" in disabled_arguments
for row in rows:
    assert BUILDER.scenario_arguments(row) == MODULE.oracle_arguments(row)

assert MODULE.selected_seeds("0,2-4,19") == [0, 2, 3, 4, 19]
try:
    MODULE.selected_seeds("20")
except ValueError:
    pass
else:
    raise AssertionError("out-of-range seed was accepted")

assert BUILDER.reached_ub_oracle({
    "stderr": "PLANMEM_LIVENESS_ATTEMPT\tkernel\t0\t0\n"
})
assert BUILDER.reached_ub_oracle({
    "stderr": "PLANMEM_UB_ORACLE_COMPLETE\t0\n"
})
assert not BUILDER.reached_ub_oracle({
    "stderr": "error: failed before PlanMemory\n"
})

model_payload = {
    "result": {
        "precision": "exact",
        "status": "success",
        "ub_peak_bits": 32768,
        "required_bits": 32768,
        "functions": [{
            "function": "kernel",
            "buffers": [{
                "name": "%base_0", "extent_bits": 32768,
                "offsets_bytes": [0], "alloc_time": 10, "free_time": 20,
                "multi_buffer_num": 1,
            }],
            "inplace_pairs": [],
        }],
    }
}

def one_buffer_oracle(offset: int) -> str:
    return (
        "PLANMEM_LIVENESS_ATTEMPT\tkernel\t0\t0\n"
        "PLANMEM_EXACT_BUFFER\t0\t0\t32768\t6\t0\t10\t20\n"
        "PLANMEM_PLAN_ATTEMPT\tkernel\t0\tsuccess\n"
        f"PLANMEM_EXACT_PLANNED_BUFFER\t0\t6\t0\t32768\t{offset}\n"
        "PLANMEM_PEAK\t0\t6\t32768\n"
    )

primary_observation = {"stderr": one_buffer_oracle(4096)}
alternate_observation = {"stderr": one_buffer_oracle(0)}
outcome, observation_index = MODULE.compare_plan_observations(
    model_payload, 0, [primary_observation, alternate_observation], 0
)
assert outcome.matched
assert observation_index == 1
assert (
    MODULE.oracle_contract_signature(primary_observation["stderr"], 0)
    != MODULE.oracle_contract_signature(alternate_observation["stderr"], 0)
)
record_with_variant = {
    "seed_results": {"0": primary_observation},
    "seed_variants": {"0": [alternate_observation]},
}
assert MODULE.oracle_observations(record_with_variant, 0) == [
    primary_observation, alternate_observation
]

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    cache_record = {
        "identity": {"compiler_sha256": "d" * 64},
        "pipeline_failed": False,
        "pipeline": {},
        "seed_results": {"0": primary_observation},
        "seed_variants": {},
    }
    path = root / "record.json.gz"
    with gzip.open(path, "wt", encoding="utf-8") as stream:
        json.dump(cache_record, stream)
    row = {
        "scenario": "auto_mb_default", "adapter": "attention.ttadapter",
        "seed": 0, "kind": "plan", "status": "different",
        "differences": "plan", "evidence": "old", "diagnostic": "old",
        "oracle_variant": "", "_payload": model_payload, "_returncode": 0,
    }
    source = root / "before_cvpipelining.mlirbc"
    source.write_bytes(b"input")
    args = SimpleNamespace(
        oracle_variant_runs=2, pipeline_timeout=10, plan_timeout=5, jobs=1,
        cv2pm=root / "cv2pm", no_progress=True,
    )
    original_observation = MODULE.run_cv2pm_observation

    def fake_observation(*_args, **_kwargs):
        return {
            "returncode": 0, "stdout": "",
            "stderr": alternate_observation["stderr"],
            "timeout": False, "seconds": 0.01,
        }

    MODULE.run_cv2pm_observation = fake_observation
    try:
        added, resolved = MODULE.augment_oracle_variants(
            args, [row], {
                ("auto_mb_default", "attention.ttadapter", 0): (
                    by_name["auto_mb_default"], source, path, cache_record,
                )
            },
        )
    finally:
        MODULE.run_cv2pm_observation = original_observation
    assert (added, resolved) == (1, 1)
    assert row["status"] == "matched" and row["oracle_variant"] == "1"
    with gzip.open(path, "rt", encoding="utf-8") as stream:
        augmented = json.load(stream)
    assert len(augmented["seed_variants"]["0"]) == 1

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    cache_record = {
        "identity": {"compiler_sha256": "d" * 64},
        "pipeline_failed": False,
        "pipeline": {},
        "seed_results": {"0": primary_observation},
        "seed_variants": {},
    }
    path = root / "record.json.gz"
    with gzip.open(path, "wt", encoding="utf-8") as stream:
        json.dump(cache_record, stream)
    original_bytes = path.read_bytes()
    row = {
        "scenario": "auto_mb_default", "adapter": "attention.ttadapter",
        "seed": 0, "kind": "plan", "status": "different",
        "differences": "plan", "evidence": "old", "diagnostic": "old",
        "oracle_variant": "", "_payload": model_payload, "_returncode": 0,
    }
    source = root / "before_cvpipelining.mlirbc"
    source.write_bytes(b"input")
    args = SimpleNamespace(
        oracle_variant_runs=2, pipeline_timeout=10, plan_timeout=5, jobs=1,
        cv2pm=root / "cv2pm", no_progress=True, cache_mode="read-only",
    )
    original_observation = MODULE.run_cv2pm_observation
    MODULE.run_cv2pm_observation = fake_observation
    try:
        added, resolved = MODULE.augment_oracle_variants(
            args, [row], {
                ("auto_mb_default", "attention.ttadapter", 0): (
                    by_name["auto_mb_default"], source, path, cache_record,
                )
            },
        )
    finally:
        MODULE.run_cv2pm_observation = original_observation
    assert (added, resolved) == (1, 1)
    assert row["status"] == "matched" and row["oracle_variant"] == "1"
    assert path.read_bytes() == original_bytes

with tempfile.TemporaryDirectory() as temporary:
    cache = Path(temporary)
    matrix = cache / "matrix.tsv"
    matrix.write_text("scenario_id\nproduction_default\n", encoding="utf-8")
    compiler_digest = "a" * 64
    (cache / "manifest.json").write_text(
        json.dumps(
            {
                "schema": MODULE.CACHE_SCHEMA,
                "execution_mode": MODULE.CACHE_EXECUTION_MODE,
                "compiler_sha256": compiler_digest,
                "matrix_sha256": hashlib.sha256(matrix.read_bytes()).hexdigest(),
            }
        ),
        encoding="utf-8",
    )
    manifest = MODULE.read_cache_manifest(cache, matrix)
    assert manifest["compiler_sha256"] == compiler_digest
    legacy = dict(manifest)
    legacy["schema"] = 1
    (cache / "manifest.json").write_text(
        json.dumps(legacy), encoding="utf-8"
    )
    try:
        MODULE.read_cache_manifest(cache, matrix)
    except ValueError as error:
        assert "unsupported oracle cache manifest schema" in str(error)
    else:
        raise AssertionError("split-process schema 1 cache was accepted")
    legacy["schema"] = MODULE.CACHE_SCHEMA
    (cache / "manifest.json").write_text(
        json.dumps(legacy), encoding="utf-8"
    )
    matrix.write_text("scenario_id\nchanged\n", encoding="utf-8")
    try:
        MODULE.read_cache_manifest(cache, matrix)
    except ValueError as error:
        assert "matrix hash differs" in str(error)
    else:
        raise AssertionError("stale oracle matrix manifest was accepted")

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    source = root / "input.mlirbc"
    source.write_bytes(b"same-input")
    barrier = threading.Barrier(2)
    original_run = MODULE.subprocess.run

    def fake_convert(command, **_kwargs):
        output = Path(command[-1])
        barrier.wait()
        output.write_text("module {}\n", encoding="utf-8")
        return SimpleNamespace(returncode=0, stderr="")

    MODULE.subprocess.run = fake_convert
    try:
        with ThreadPoolExecutor(max_workers=2) as pool:
            futures = [
                pool.submit(
                    MODULE.materialize_model_input,
                    source,
                    root / "converter",
                    root / "cache",
                )
                for _ in range(2)
            ]
            destinations = [future.result() for future in futures]
    finally:
        MODULE.subprocess.run = original_run
    assert destinations[0] == destinations[1]
    assert destinations[0].read_text(encoding="utf-8") == "module {}\n"

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    source = root / "before_cvpipelining.mlirbc"
    source.write_bytes(b"before-cv")
    task = BUILDER.Task(
        by_name["auto_mb_default"], "attention.ttadapter", source
    )
    commands: list[list[str]] = []
    original_run_command = BUILDER.run_command

    def fake_full_cv2pm(command, _timeout, env=None):
        commands.append(command)
        seed = next(
            int(argument.split("=", 1)[1])
            for argument in command
            if argument.startswith("--plan-memory-seed=")
        )
        assert env is not None
        assert env["BISHENGIR_DUMP_PLAN_MEMORY_ATTEMPTS"] == "1"
        return {
            "returncode": 0,
            "stdout": "",
            "stderr": (
                f"PLANMEM_LIVENESS_ATTEMPT\tkernel\t0\t{seed}\n"
                f"PLANMEM_PLAN_ATTEMPT\tkernel\t{seed}\tsuccess\n"
            ),
            "timeout": False,
            "seconds": 0.01,
        }

    BUILDER.run_command = fake_full_cv2pm
    try:
        summary = BUILDER.build_one(
            task, Path("/fake/cv2pm"), "b" * 64, root / "cache",
            10, 5, 4, False,
        )
    finally:
        BUILDER.run_command = original_run_command
    assert not summary["pipeline_failed"]
    assert len(commands) == 20
    assert all(command[1] == str(source) for command in commands)
    assert all("--local-plan-memory-only" not in command for command in commands)
    record = BUILDER.read_gzip_json(BUILDER.record_path(root / "cache", task))
    assert record["identity"]["schema"] == 2
    assert record["identity"]["execution_mode"] == "full_cv2pm_per_seed"
    assert sorted(int(seed) for seed in record["seed_results"]) == list(range(20))
    assert record["seed_variants"] == {}

    commands.clear()

    def fake_pre_plan_failure(command, _timeout, env=None):
        commands.append(command)
        return {
            "returncode": 1,
            "stdout": "",
            "stderr": "error: failed before PlanMemory\n",
            "timeout": False,
            "seconds": 0.01,
        }

    BUILDER.run_command = fake_pre_plan_failure
    try:
        failed = BUILDER.build_one(
            task, Path("/fake/cv2pm"), "c" * 64, root / "failed-cache",
            10, 5, 4, False,
        )
    finally:
        BUILDER.run_command = original_run_command
    assert failed["pipeline_failed"]
    assert len(commands) == 1

print("[PASS] cv2pm scenario/profile/model argument mapping is stable")
