#!/usr/bin/env python3
"""Validate determinism, constraints, domains, and batch cardinality."""

from __future__ import annotations

import csv
import filecmp
import subprocess
import sys
import tempfile
from pathlib import Path


EXPECTED_COUNTS = {
    "plan_memory": 40,
    "cvpipelining": 15,
    "suffix_ub_passes": 48,
    "align_storage_infer_data_layout": 16,
    "cvpipelining_suffix_plan_memory_cross": 64,
}


def rows(path: Path) -> list[dict[str, str]]:
    with path.open(encoding="utf-8", newline="") as source:
        return list(csv.DictReader(source, delimiter="\t"))


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def values(items: list[dict[str, str]], field: str) -> set[str]:
    return {item[field] for item in items}


def main() -> int:
    module = Path(__file__).resolve().parents[1]
    matrix = module / "testdata/config_matrix"
    generator = module / "scripts/generate_config_test_matrix.py"
    loaded: dict[str, list[dict[str, str]]] = {}
    for batch, expected in EXPECTED_COUNTS.items():
        loaded[batch] = rows(matrix / f"{batch}.tsv")
        require(len(loaded[batch]) == expected,
                f"{batch}: expected {expected} rows")
        require(len({item["config_id"] for item in loaded[batch]}) == expected,
                f"{batch}: duplicate config_id")

    p1_pairs = {
        (item["plan_memory_seeds"], item["restrict_modes"])
        for item in loaded["plan_memory"]
    }
    require(p1_pairs == {(str(seed), str(restrict))
                         for seed in range(20) for restrict in (0, 1)},
            "plan_memory is not the complete 20x2 matrix")

    for batch, items in loaded.items():
        for item in items:
            if item["disable_cv_pipelining"] == "1":
                require(item["enable_preload"] == "0" and
                        item["cv_pipeline_depth"] == "-1" and
                        item["enable_cv_lazy_loading"] == "0",
                        f"{item['config_id']}: disabled CV has active CV options")
            if item["enable_preload"] == "1":
                require(item["cv_pipeline_depth"] == "-1",
                        f"{item['config_id']}: skew mode has unroll depth")
            if item["enable_auto_multi_buffer"] == "0":
                require(item["local_multi_buffer_strategy"] == "no-l0c" and
                        item["mix_multi_buffer_strategy"] == "only-cube",
                        f"{item['config_id']}: inactive multi-buffer strategy")

    cvpipelining = loaded["cvpipelining"]
    require(values(cvpipelining, "disable_cv_pipelining") == {"0", "1"},
            "cvpipelining missing on/off")
    require(values(cvpipelining, "enable_preload") == {"0", "1"},
            "cvpipelining missing unroll/skew")
    require(values(cvpipelining, "cv_pipeline_depth") ==
            {"-1", "0", "1", "2", "3", "4"},
            "cvpipelining missing depth boundary")
    require(values(cvpipelining, "enable_cv_lazy_loading") == {"0", "1"},
            "cvpipelining missing lazy-loading mode")

    suffix_ub_passes = loaded["suffix_ub_passes"]
    require(values(suffix_ub_passes, "enable_auto_multi_buffer") == {"0", "1"},
            "suffix_ub_passes missing auto multi-buffer mode")
    require(values(suffix_ub_passes, "local_multi_buffer_strategy") ==
            {"no-l0c", "only-cube", "only-vector", "no-limit"},
            "suffix_ub_passes missing local multi-buffer strategy")
    require(values(suffix_ub_passes, "mix_multi_buffer_strategy") ==
            {"no-l0c", "only-cube", "only-vector", "no-limit"},
            "suffix_ub_passes missing mix multi-buffer strategy")
    for field in ("enable_code_motion", "enable_ubuf_saving"):
        require(values(suffix_ub_passes, field) == {"0", "1"},
                f"suffix_ub_passes missing {field}")
    for field in ("tile_mix_cube_loop", "tile_mix_vector_loop"):
        require(values(suffix_ub_passes, field) == {"1", "2", "4"},
                f"suffix_ub_passes missing {field}")

    align_storage = loaded["align_storage_infer_data_layout"]
    layout_fields = (
        "disable_align_alloc_size", "enable_hivm_auto_storage_align",
        "disable_enable_stride_align", "disable_infer_hivm_data_layout",
    )
    require(len({tuple(item[field] for field in layout_fields)
                 for item in align_storage}) == 16,
            "align_storage_infer_data_layout is not the exhaustive 2^4 matrix")

    cross_pass = loaded["cvpipelining_suffix_plan_memory_cross"]
    for field in layout_fields + (
        "disable_cv_pipelining", "enable_preload",
        "enable_cv_lazy_loading", "enable_auto_multi_buffer",
        "enable_code_motion", "enable_ubuf_saving",
    ):
        require(values(cross_pass, field) == {"0", "1"},
                f"cross-pass matrix missing binary domain for {field}")
    require(values(cross_pass, "cv_pipeline_depth") ==
            {"-1", "0", "1", "2", "3", "4"},
            "cross-pass matrix missing CVPipelining depth domain")
    for field in ("local_multi_buffer_strategy",
                  "mix_multi_buffer_strategy"):
        require(values(cross_pass, field) ==
                {"no-l0c", "only-cube", "only-vector", "no-limit"},
                f"cross-pass matrix missing strategy domain for {field}")
    for field in ("tile_mix_cube_loop", "tile_mix_vector_loop"):
        require(values(cross_pass, field) == {"1", "2", "4"},
                f"cross-pass matrix missing tile domain for {field}")
    require(len(rows(matrix / "all_configs.tsv")) == 183,
            "all_configs must contain all 183 rows")

    with tempfile.TemporaryDirectory(prefix="cvub-config-matrix-") as temp:
        regenerated = Path(temp)
        subprocess.run([sys.executable, str(generator),
                        "--output-dir", str(regenerated)], check=True,
                       stdout=subprocess.DEVNULL)
        for name in (*EXPECTED_COUNTS, "all_configs", "summary"):
            require(filecmp.cmp(matrix / f"{name}.tsv",
                                regenerated / f"{name}.tsv", shallow=False),
                    f"{name}: checked-in data is not reproducible")

    print("CONFIG_TEST_MATRIX_VALIDATION=PASS")
    print("configs=183 planned_plan_memory_runs_per_input_before_hash_dedup=5760")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print(f"[ERROR] {error}", file=sys.stderr)
        raise SystemExit(1)
