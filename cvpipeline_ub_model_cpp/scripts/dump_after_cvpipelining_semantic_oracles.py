#!/usr/bin/env python3
"""Generate after-CVPipelining SemanticIR oracles."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import subprocess
from pathlib import Path


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def parse_args() -> argparse.Namespace:
    module = Path(__file__).resolve().parent.parent
    repo = module.parent
    parser = argparse.ArgumentParser()
    parser.add_argument("--cvpipelining-summary", type=Path,
                        default=repo / "Output/index/cvpipelining/summary.tsv")
    parser.add_argument("--tool", type=Path,
                        default=repo / "build/bin/bishengir-cvpipeline-suffix-compile")
    parser.add_argument("--model", type=Path,
                        default=module / "output/bin/cvpipeline_ub_model_dev_validate")
    parser.add_argument("--output-root", type=Path,
                        default=repo / "Output/index/after_cvpipelining_semantic_ir")
    parser.add_argument("--max-objects", type=int, default=0)
    parser.add_argument("--no-resume", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    summary_path = args.cvpipelining_summary.resolve()
    tool = args.tool.resolve()
    model = args.model.resolve()
    output_root = args.output_root.resolve()
    if not summary_path.is_file() or not tool.is_file() or not model.is_file():
        raise SystemExit(
            "[ERROR] missing CVPipelining summary, suffix tool, or "
            "after-CVPipelining model")
    with summary_path.open(newline="") as stream:
        rows = list(csv.DictReader(stream, delimiter="\t"))
    rows = [row for row in rows if row["status"] == "complete"]
    objects: dict[str, Path] = {}
    for row in rows:
        objects.setdefault(row["after_cvpipelining_sha256"],
                           Path(row["after_cvpipelining_ir"]))
    if args.max_objects:
        selected = dict(list(sorted(objects.items()))[:args.max_objects])
    else:
        selected = dict(sorted(objects.items()))

    object_root = output_root / "objects"
    object_root.mkdir(parents=True, exist_ok=True)
    tool_hash = sha256(tool)
    model_hash = sha256(model)
    object_status: dict[str, dict[str, str]] = {}
    for input_hash, input_path in selected.items():
        directory = object_root / input_hash
        directory.mkdir(parents=True, exist_ok=True)
        generic = directory / "generic.mlir"
        oracle = directory / "oracle.semantic.tsv"
        modeled = directory / "model.semantic.tsv"
        suffix_output = directory / "boundary.mlir"
        metadata = directory / "provenance.json"
        fingerprint = {
            "schema_version": 1,
            "input_sha256": input_hash,
            "oracle_tool_sha256": tool_hash,
        }
        reusable = False
        if not args.no_resume and metadata.is_file() and generic.is_file() and oracle.is_file():
            try:
                saved = json.loads(metadata.read_text())
                reusable = all(saved.get(key) == value for key, value in fingerprint.items())
            except (OSError, json.JSONDecodeError):
                reusable = False
        compiler_status = 0
        if not reusable:
            command = [
                str(tool), str(input_path), "--disable-cv-pipelining",
                "--stop-after-cvpipelining", "--mlir-disable-threading",
                f"--dump-after-cvpipelining-semantic-oracle={oracle}",
                f"--dump-after-cvpipelining-generic-ir={generic}", "-o", str(suffix_output),
            ]
            result = subprocess.run(command, stdout=subprocess.DEVNULL,
                                    stderr=(directory / "oracle.stderr.log").open("wb"))
            compiler_status = result.returncode
            if compiler_status == 0 and generic.is_file() and oracle.is_file():
                metadata.write_text(json.dumps(fingerprint, sort_keys=True) + "\n")
        model_result = subprocess.run([
            str(model), "--action=dump-after-cvpipelining-semantic-ir", f"--root={generic}",
            f"--output={modeled}",
        ], stdout=subprocess.DEVNULL, stderr=(directory / "model.stderr.log").open("wb"))
        complete = (compiler_status == 0 and model_result.returncode == 0 and
                    generic.is_file() and oracle.is_file() and modeled.is_file())
        object_status[input_hash] = {
            "status": "complete" if complete else "incomplete",
            "compiler_status": str(compiler_status),
            "model_status": str(model_result.returncode),
            "generic_ir": str(generic),
            "oracle_semantic": str(oracle),
            "model_semantic": str(modeled),
            "generic_sha256": sha256(generic) if generic.is_file() else "",
            "oracle_sha256": sha256(oracle) if oracle.is_file() else "",
            "model_sha256": sha256(modeled) if modeled.is_file() else "",
        }

    fields = [
        "adapter", "snapshot", "config_id", "status", "compiler_status",
        "model_status", "after_cvpipelining_sha256", "after_cvpipelining_ir",
        "generic_ir", "oracle_semantic", "model_semantic", "generic_sha256",
        "oracle_sha256", "model_sha256", "oracle_tool_sha256", "model_tool_sha256",
    ]
    output_rows = []
    for row in rows:
        status = object_status.get(row["after_cvpipelining_sha256"])
        if status is None:
            continue
        output_rows.append({
            "adapter": row["adapter"], "snapshot": row["snapshot"],
            "config_id": row["config_id"],
            "after_cvpipelining_sha256": row["after_cvpipelining_sha256"],
            "after_cvpipelining_ir": row["after_cvpipelining_ir"],
            "oracle_tool_sha256": tool_hash, "model_tool_sha256": model_hash,
            **status,
        })
    output_root.mkdir(parents=True, exist_ok=True)
    with (output_root / "summary.tsv").open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, delimiter="\t")
        writer.writeheader()
        writer.writerows(output_rows)
    manifest = {
        "schema_version": 1,
        "cvpipelining_summary": str(summary_path),
        "cvpipelining_summary_sha256": sha256(summary_path),
        "cvpipelining_tuple_count": len(rows),
        "mapped_tuple_count": len(output_rows),
        "unique_object_count": len(selected),
        "complete_object_count": sum(
            item["status"] == "complete" for item in object_status.values()),
        "oracle_tool_sha256": tool_hash,
        "model_tool_sha256": model_hash,
    }
    (output_root / "dataset.json").write_text(
        json.dumps(manifest, sort_keys=True) + "\n")
    print(f"tuples={len(output_rows)} unique_objects={len(selected)} "
          f"complete_objects={manifest['complete_object_count']}")
    return 0 if manifest["complete_object_count"] == len(selected) else 1


if __name__ == "__main__":
    raise SystemExit(main())
