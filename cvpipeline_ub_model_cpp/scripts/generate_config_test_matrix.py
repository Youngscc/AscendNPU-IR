#!/usr/bin/env python3
"""Generate deterministic, constraint-aware UB configuration test batches."""

from __future__ import annotations

import argparse
import csv
import hashlib
import itertools
import random
from pathlib import Path


RANDOM_SEED = 20260711
STRATEGIES = ("no-l0c", "only-cube", "only-vector", "no-limit")
CONFIG_FIELDS = (
    "disable_cv_pipelining",
    "enable_preload",
    "cv_pipeline_depth",
    "enable_cv_lazy_loading",
    "enable_auto_multi_buffer",
    "local_multi_buffer_strategy",
    "mix_multi_buffer_strategy",
    "enable_code_motion",
    "enable_ubuf_saving",
    "tile_mix_cube_loop",
    "tile_mix_vector_loop",
    "disable_align_alloc_size",
    "enable_hivm_auto_storage_align",
    "disable_enable_stride_align",
    "disable_infer_hivm_data_layout",
)
HEADER = (
    "config_id",
    "batch",
    "origin",
    "plan_memory_seeds",
    "restrict_modes",
    "config_digest",
) + CONFIG_FIELDS


def default_config() -> dict[str, object]:
    return {
        "disable_cv_pipelining": 0,
        "enable_preload": 0,
        "cv_pipeline_depth": -1,
        "enable_cv_lazy_loading": 0,
        "enable_auto_multi_buffer": 0,
        "local_multi_buffer_strategy": "no-l0c",
        "mix_multi_buffer_strategy": "only-cube",
        "enable_code_motion": 1,
        "enable_ubuf_saving": 0,
        "tile_mix_cube_loop": 1,
        "tile_mix_vector_loop": 1,
        "disable_align_alloc_size": 0,
        "enable_hivm_auto_storage_align": 1,
        "disable_enable_stride_align": 0,
        "disable_infer_hivm_data_layout": 0,
    }


def normalize(config: dict[str, object]) -> dict[str, object]:
    result = default_config()
    result.update(config)
    if result["disable_cv_pipelining"]:
        result["enable_preload"] = 0
        result["cv_pipeline_depth"] = -1
        result["enable_cv_lazy_loading"] = 0
    elif result["enable_preload"]:
        result["cv_pipeline_depth"] = -1
    if not result["enable_auto_multi_buffer"]:
        result["local_multi_buffer_strategy"] = "no-l0c"
        result["mix_multi_buffer_strategy"] = "only-cube"
    return result


def key(config: dict[str, object], fields=CONFIG_FIELDS) -> tuple[object, ...]:
    return tuple(config[field] for field in fields)


def digest(config: dict[str, object]) -> str:
    text = "\n".join(f"{field}={config[field]}" for field in CONFIG_FIELDS)
    return hashlib.sha256(text.encode("utf-8")).hexdigest()[:16]


def pair_tokens(config: dict[str, object], fields: tuple[str, ...]) -> set[tuple]:
    return {
        (left, config[left], right, config[right])
        for left, right in itertools.combinations(fields, 2)
    }


def greedy_pairwise(
    candidates: list[dict[str, object]],
    fields: tuple[str, ...],
    limit: int,
    anchors: list[dict[str, object]],
    rng: random.Random,
) -> list[dict[str, object]]:
    unique = {key(normalize(item)): normalize(item) for item in candidates}
    pool = list(unique.values())
    universe = set().union(*(pair_tokens(item, fields) for item in pool))
    selected: list[dict[str, object]] = []
    selected_keys: set[tuple[object, ...]] = set()
    covered: set[tuple] = set()

    def add(item: dict[str, object]) -> None:
        item = normalize(item)
        item_key = key(item)
        if item_key in selected_keys:
            return
        selected.append(item)
        selected_keys.add(item_key)
        covered.update(pair_tokens(item, fields))

    for anchor in anchors:
        add(anchor)
    tie_break = {key(item): rng.random() for item in pool}
    while len(selected) < limit and covered != universe:
        remaining = [item for item in pool if key(item) not in selected_keys]
        if not remaining:
            break
        best = max(
            remaining,
            key=lambda item: (
                len(pair_tokens(item, fields) - covered),
                tie_break[key(item)],
            ),
        )
        add(best)
    while len(selected) < min(limit, len(pool)):
        remaining = [item for item in pool if key(item) not in selected_keys]
        add(rng.choice(remaining))
    return selected


def planmemory_batch() -> list[dict[str, object]]:
    rows = []
    for restrict in (0, 1):
        for seed in range(20):
            rows.append(
                {
                    **default_config(),
                    "plan_memory_seeds": str(seed),
                    "restrict_modes": str(restrict),
                    "origin": "exhaustive",
                }
            )
    return rows


def cvpipeline_batch() -> list[dict[str, object]]:
    rows = [{**default_config(), "disable_cv_pipelining": 1}]
    for lazy in (0, 1):
        rows.append({**default_config(), "enable_preload": 1,
                     "enable_cv_lazy_loading": lazy})
        for depth in (-1, 0, 1, 2, 3, 4):
            rows.append({**default_config(), "cv_pipeline_depth": depth,
                         "enable_cv_lazy_loading": lazy})
    return [{**normalize(row), "origin": "exhaustive"} for row in rows]


def suffix_buffer_batch(rng: random.Random) -> list[dict[str, object]]:
    candidates = []
    for auto, local, mix, code, saving, cube, vector in itertools.product(
        (0, 1), STRATEGIES, STRATEGIES, (0, 1), (0, 1), (1, 2, 4), (1, 2, 4)
    ):
        if not auto and (local != "no-l0c" or mix != "only-cube"):
            continue
        candidates.append(
            normalize(
                {
                    "enable_auto_multi_buffer": auto,
                    "local_multi_buffer_strategy": local,
                    "mix_multi_buffer_strategy": mix,
                    "enable_code_motion": code,
                    "enable_ubuf_saving": saving,
                    "tile_mix_cube_loop": cube,
                    "tile_mix_vector_loop": vector,
                }
            )
        )
    fields = (
        "enable_auto_multi_buffer",
        "local_multi_buffer_strategy",
        "mix_multi_buffer_strategy",
        "enable_code_motion",
        "enable_ubuf_saving",
        "tile_mix_cube_loop",
        "tile_mix_vector_loop",
    )
    anchors = [
        default_config(),
        normalize({"enable_auto_multi_buffer": 1}),
        normalize({"enable_auto_multi_buffer": 1,
                   "local_multi_buffer_strategy": "no-limit",
                   "mix_multi_buffer_strategy": "no-limit"}),
        normalize({"enable_code_motion": 0}),
        normalize({"enable_ubuf_saving": 1}),
        normalize({"tile_mix_cube_loop": 4}),
        normalize({"tile_mix_vector_loop": 4}),
    ]
    return [
        {**item, "origin": "anchor+greedy-pairwise"}
        for item in greedy_pairwise(candidates, fields, 48, anchors, rng)
    ]


def layout_batch() -> list[dict[str, object]]:
    rows = []
    for disable_size, auto_align, disable_stride, disable_layout in (
        itertools.product((0, 1), repeat=4)
    ):
        rows.append(
            normalize(
                {
                    "disable_align_alloc_size": disable_size,
                    "enable_hivm_auto_storage_align": auto_align,
                    "disable_enable_stride_align": disable_stride,
                    "disable_infer_hivm_data_layout": disable_layout,
                }
            )
        )
    return [{**row, "origin": "exhaustive"} for row in rows]


def cross_priority_batch(rng: random.Random) -> list[dict[str, object]]:
    candidates = []
    for _ in range(20000):
        candidates.append(
            normalize(
                {
                    "disable_cv_pipelining": rng.choice((0, 0, 0, 1)),
                    "enable_preload": rng.choice((0, 1)),
                    "cv_pipeline_depth": rng.choice((-1, 0, 1, 2, 3, 4)),
                    "enable_cv_lazy_loading": rng.choice((0, 1)),
                    "enable_auto_multi_buffer": rng.choice((0, 1)),
                    "local_multi_buffer_strategy": rng.choice(STRATEGIES),
                    "mix_multi_buffer_strategy": rng.choice(STRATEGIES),
                    "enable_code_motion": rng.choice((0, 1)),
                    "enable_ubuf_saving": rng.choice((0, 1)),
                    "tile_mix_cube_loop": rng.choice((1, 2, 4)),
                    "tile_mix_vector_loop": rng.choice((1, 2, 4)),
                    "disable_align_alloc_size": rng.choice((0, 1)),
                    "enable_hivm_auto_storage_align": rng.choice((0, 1)),
                    "disable_enable_stride_align": rng.choice((0, 1)),
                    "disable_infer_hivm_data_layout": rng.choice((0, 1)),
                }
            )
        )
    anchors = [default_config()]
    anchors.extend(
        normalize({field: 1 - int(default_config()[field])})
        for field in (
            "disable_cv_pipelining",
            "enable_preload",
            "enable_cv_lazy_loading",
            "enable_auto_multi_buffer",
            "enable_code_motion",
            "enable_ubuf_saving",
            "disable_align_alloc_size",
            "enable_hivm_auto_storage_align",
            "disable_enable_stride_align",
            "disable_infer_hivm_data_layout",
        )
    )
    return [
        {**item, "origin": "fixed-seed-random+greedy-pairwise"}
        for item in greedy_pairwise(
            candidates, CONFIG_FIELDS, 64, anchors, rng
        )
    ]


def finalize(batch: str, rows: list[dict[str, object]]) -> list[dict[str, object]]:
    result = []
    seen = set()
    for index, source in enumerate(rows):
        config = normalize(source)
        seeds = str(source.get("plan_memory_seeds", "0-19"))
        restrict = str(source.get("restrict_modes", "0,1"))
        identity = (key(config), seeds, restrict)
        if identity in seen:
            continue
        seen.add(identity)
        result.append(
            {
                "config_id": f"{batch}_{index:03d}_{digest(config)}",
                "batch": batch,
                "origin": source.get("origin", "generated"),
                "plan_memory_seeds": seeds,
                "restrict_modes": restrict,
                "config_digest": digest(config),
                **config,
            }
        )
    return result


def write_batch(path: Path, rows: list[dict[str, object]]) -> None:
    with path.open("w", newline="", encoding="utf-8") as output:
        writer = csv.DictWriter(output, fieldnames=HEADER, delimiter="\t",
                                lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "testdata/config_matrix",
    )
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    rng = random.Random(RANDOM_SEED)
    batches = {
        "p1_planmemory": planmemory_batch(),
        "p2_cvpipeline": cvpipeline_batch(),
        "p3_suffix_buffer": suffix_buffer_batch(rng),
        "p4_layout": layout_batch(),
        "p5_cross_priority": cross_priority_batch(rng),
    }
    summary = []
    all_rows: list[dict[str, object]] = []
    for name, source_rows in batches.items():
        rows = finalize(name, source_rows)
        write_batch(args.output_dir / f"{name}.tsv", rows)
        all_rows.extend(rows)
        expanded = sum(
            (20 if row["plan_memory_seeds"] == "0-19" else 1)
            * (2 if row["restrict_modes"] == "0,1" else 1)
            for row in rows
        )
        summary.append((name, len(rows), expanded))
    write_batch(args.output_dir / "all_configs.tsv", all_rows)
    with (args.output_dir / "summary.tsv").open(
        "w", newline="", encoding="utf-8"
    ) as output:
        writer = csv.writer(output, delimiter="\t", lineterminator="\n")
        writer.writerow(("batch", "config_rows", "planmemory_runs_per_ir"))
        writer.writerows(summary)
    print(f"RANDOM_SEED={RANDOM_SEED}")
    for name, rows, expanded in summary:
        print(f"{name}: configs={rows} planmemory_runs_per_ir={expanded}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
