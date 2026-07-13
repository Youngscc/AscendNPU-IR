#!/usr/bin/env python3
"""Migrate legacy stage-first compiler dumps to adapter-first layout."""

from __future__ import annotations

import argparse
import shutil
from pathlib import Path


LEGACY_DATASETS = (
    ("cvpipeline_before_ir_dumps", "01_cvpipeline/compiler_default", "cvpipeline_compiler_default"),
    ("cvpipeline_suffix_compile", "02_suffix/compiler_default", "suffix_compiler_default"),
    ("planmemory_ir_dumps_all", "03_planmemory/all", "planmemory_all"),
    ("planmemory_ir_dumps_ttadapter_168", "03_planmemory/ttadapter_168", "planmemory_ttadapter_168"),
    ("planmemory_ir_dumps_ttadapter_168_recheck", "03_planmemory/ttadapter_168_recheck", "planmemory_ttadapter_168_recheck"),
    ("planmemory_ir_dumps_vv", "03_planmemory/vv", "planmemory_vv"),
)

DATASET_VIEWS = (
    ("cvpipeline_compiler_default", "01_cvpipeline/compiler_default"),
    ("suffix_compiler_default", "02_suffix/compiler_default"),
    ("planmemory_all", "03_planmemory/all"),
    ("planmemory_ttadapter_168", "03_planmemory/ttadapter_168"),
    ("planmemory_ttadapter_168_recheck", "03_planmemory/ttadapter_168_recheck"),
    ("planmemory_vv", "03_planmemory/vv"),
)


def adapter_name(legacy_name: str) -> str:
    if legacy_name.startswith("data_") and legacy_name.endswith(".ttadapter"):
        return legacy_name[len("data_"):]
    return legacy_name


def rewrite_index(source: Path, destination: Path,
                  replacements: dict[str, str]) -> None:
    data = source.read_text(errors="surrogateescape")
    for old, new in sorted(replacements.items(), key=lambda item: -len(item[0])):
        data = data.replace(old, new)
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(data, errors="surrogateescape")
    source.unlink()


def migrate_dataset(output: Path, legacy_name: str, stage: str,
                    index_name: str) -> int:
    legacy = output / legacy_name
    if not legacy.is_dir():
        return 0
    replacements: dict[str, str] = {}
    moved = 0
    for child in sorted(legacy.iterdir()):
        if not child.is_dir():
            continue
        destination = output / "adapters" / adapter_name(child.name) / stage
        if destination.exists():
            raise SystemExit(f"[ERROR] migration destination exists: {destination}")
        destination.parent.mkdir(parents=True, exist_ok=True)
        replacements[str(child.resolve())] = str(destination.resolve())
        shutil.move(str(child), str(destination))
        moved += 1
    index = output / "index" / index_name
    for source in sorted(legacy.iterdir()):
        if source.is_file():
            rewrite_index(source, index / source.name, replacements)
    legacy.rmdir()
    print(f"{legacy_name}: adapters={moved} index={index}")
    return moved


def create_views(output: Path) -> None:
    adapters = output / "adapters"
    for view_name, stage in DATASET_VIEWS:
        view = output / "views" / view_name
        view.mkdir(parents=True, exist_ok=True)
        count = 0
        desired_links: set[str] = set()
        for adapter in sorted(adapters.iterdir()):
            target = adapter / stage
            if not target.is_dir():
                continue
            link_name = (f"data_{adapter.name}"
                         if adapter.name.endswith(".ttadapter") else adapter.name)
            desired_links.add(link_name)
            link = view / link_name
            relative_target = Path("../../adapters") / adapter.name / stage
            if link.is_symlink():
                if link.readlink() != relative_target:
                    link.unlink()
                    link.symlink_to(relative_target)
            elif link.exists():
                raise SystemExit(f"[ERROR] dataset view path is not a symlink: {link}")
            else:
                link.symlink_to(relative_target)
            count += 1
        for link in view.iterdir():
            if (link.is_symlink() and "adapters" in str(link.readlink())
                    and link.name not in desired_links):
                link.unlink()
        index = output / "index" / view_name
        if index.is_dir():
            for source in sorted(index.iterdir()):
                if not source.is_file():
                    continue
                link = view / source.name
                relative_target = Path("../../index") / view_name / source.name
                if link.is_symlink():
                    if link.readlink() != relative_target:
                        raise SystemExit(f"[ERROR] stale index view: {link}")
                elif link.exists():
                    raise SystemExit(f"[ERROR] index view path is not a symlink: {link}")
                else:
                    link.symlink_to(relative_target)
        print(f"view {view_name}: adapters={count}")


def repair_relative_index_paths(output: Path) -> None:
    adapters = output / "adapters"
    replacements: dict[str, str] = {}
    for legacy_name, stage, _ in LEGACY_DATASETS:
        for adapter in adapters.iterdir():
            target = adapter / stage
            if not target.is_dir():
                continue
            old_case = f"Output/{legacy_name}/data_{adapter.name}"
            new_case = f"Output/adapters/{adapter.name}/{stage}"
            replacements[old_case] = new_case
            replacements[str(output / legacy_name / f"data_{adapter.name}")] = str(target)
    index_files = [path for path in (output / "index").rglob("*")
                   if path.is_file()]
    model_output = output.parent / "cvpipeline_ub_model_cpp" / "output"
    if model_output.is_dir():
        index_files.extend(
            path for path in model_output.rglob("*.tsv")
            if path.name in {"summary.tsv", "oracle_summary.tsv"}
        )
    for path in index_files:
        data = path.read_text(errors="surrogateescape")
        updated = data
        for old, new in replacements.items():
            updated = updated.replace(old, new)
        if updated != data:
            path.write_text(updated, errors="surrogateescape")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-root", type=Path,
                        default=Path(__file__).resolve().parent.parent / "Output")
    args = parser.parse_args()
    output = args.output_root.resolve()
    output.mkdir(parents=True, exist_ok=True)
    total = 0
    for legacy, stage, index in LEGACY_DATASETS:
        total += migrate_dataset(output, legacy, stage, index)
    old_experiments = output / "ub_peak_pass_ablation"
    new_experiments = output / "experiments" / "ub_peak_pass_ablation"
    if old_experiments.is_dir():
        if new_experiments.exists():
            raise SystemExit(f"[ERROR] migration destination exists: {new_experiments}")
        new_experiments.parent.mkdir(parents=True, exist_ok=True)
        shutil.move(str(old_experiments), str(new_experiments))
        print(f"ub_peak_pass_ablation: {new_experiments}")
    repair_relative_index_paths(output)
    create_views(output)
    print(f"ADAPTER_OUTPUT_MIGRATION=PASS moved_adapter_stages={total}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
