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
    "p1_planmemory": 40,
    "p2_cvpipeline": 15,
    "p3_suffix_buffer": 48,
    "p4_layout": 16,
    "p5_cross_priority": 64,
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
        for item in loaded["p1_planmemory"]
    }
    require(p1_pairs == {(str(seed), str(restrict))
                         for seed in range(20) for restrict in (0, 1)},
            "p1_planmemory is not the complete 20x2 matrix")

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

    p2 = loaded["p2_cvpipeline"]
    require(values(p2, "disable_cv_pipelining") == {"0", "1"},
            "p2 missing CV on/off")
    require(values(p2, "enable_preload") == {"0", "1"},
            "p2 missing unroll/skew")
    require(values(p2, "cv_pipeline_depth") ==
            {"-1", "0", "1", "2", "3", "4"},
            "p2 missing depth boundary")
    require(values(p2, "enable_cv_lazy_loading") == {"0", "1"},
            "p2 missing lazy-loading mode")

    p3 = loaded["p3_suffix_buffer"]
    require(values(p3, "enable_auto_multi_buffer") == {"0", "1"},
            "p3 missing auto multi-buffer mode")
    require(values(p3, "local_multi_buffer_strategy") ==
            {"no-l0c", "only-cube", "only-vector", "no-limit"},
            "p3 missing local multi-buffer strategy")
    require(values(p3, "mix_multi_buffer_strategy") ==
            {"no-l0c", "only-cube", "only-vector", "no-limit"},
            "p3 missing mix multi-buffer strategy")
    for field in ("enable_code_motion", "enable_ubuf_saving"):
        require(values(p3, field) == {"0", "1"}, f"p3 missing {field}")
    for field in ("tile_mix_cube_loop", "tile_mix_vector_loop"):
        require(values(p3, field) == {"1", "2", "4"},
                f"p3 missing {field}")

    p4 = loaded["p4_layout"]
    layout_fields = (
        "disable_align_alloc_size", "enable_hivm_auto_storage_align",
        "disable_enable_stride_align", "disable_infer_hivm_data_layout",
    )
    require(len({tuple(item[field] for field in layout_fields) for item in p4})
            == 16, "p4 is not the exhaustive 2^4 layout matrix")

    p5 = loaded["p5_cross_priority"]
    for field in layout_fields + (
        "disable_cv_pipelining", "enable_preload",
        "enable_cv_lazy_loading", "enable_auto_multi_buffer",
        "enable_code_motion", "enable_ubuf_saving",
    ):
        require(values(p5, field) == {"0", "1"},
                f"p5 missing binary domain for {field}")
    require(values(p5, "cv_pipeline_depth") ==
            {"-1", "0", "1", "2", "3", "4"},
            "p5 missing CV depth domain")
    for field in ("local_multi_buffer_strategy",
                  "mix_multi_buffer_strategy"):
        require(values(p5, field) ==
                {"no-l0c", "only-cube", "only-vector", "no-limit"},
                f"p5 missing strategy domain for {field}")
    for field in ("tile_mix_cube_loop", "tile_mix_vector_loop"):
        require(values(p5, field) == {"1", "2", "4"},
                f"p5 missing tile domain for {field}")
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
    print("configs=183 planned_planmemory_runs_per_input_before_hash_dedup=5760")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print(f"[ERROR] {error}", file=sys.stderr)
        raise SystemExit(1)
