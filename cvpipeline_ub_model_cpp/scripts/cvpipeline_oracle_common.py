#!/usr/bin/env python3
"""Shared helpers for CVPipelining-after oracle generation and validation."""

from __future__ import annotations

import hashlib
import json
import re
from pathlib import Path


SCHEMA_VERSION = 1

BOOL_OPTIONS = {
    "disable_cv_pipelining": "disable-cv-pipelining",
    "disable_auto_cv_work_space_manage": "disable-auto-cv-work-space-manage",
    "enable_preload": "enable-preload",
    "enable_cv_lazy_loading": "enable-cv-lazy-loading",
    "enable_auto_multi_buffer": "enable-auto-multi-buffer",
    "enable_triton_kernel_compile": "enable-triton-kernel-compile",
    "enable_code_motion": "enable-code-motion",
    "enable_ubuf_saving": "enable-ubuf-saving",
    "disable_align_alloc_size": "disable-align-alloc-size",
    "enable_hivm_auto_storage_align": "enable-hivm-auto-storage-align",
    "disable_enable_stride_align": "disable-enable-stride-align",
    "disable_infer_hivm_data_layout": "disable-infer-hivm-data-layout",
}

VALUE_OPTIONS = {
    "cv_pipeline_depth": "cv-pipeline-depth",
    "local_multi_buffer_strategy": "limit-auto-multi-buffer-of-local-buffer",
    "mix_multi_buffer_strategy": "limit-auto-multi-buffer-buffer",
    "tile_mix_cube_loop": "tile-mix-cube-loop",
    "tile_mix_vector_loop": "tile-mix-vector-loop",
}


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def canonical_json(value: object) -> str:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def canonical_ir(data: bytes) -> bytes:
    return data.replace(b"\r\n", b"\n").rstrip() + b"\n"


def bool_value(value: str) -> str:
    if value not in {"0", "1"}:
        raise ValueError(f"expected 0 or 1, got {value!r}")
    return "true" if value == "1" else "false"


def config_argv(config: dict[str, str]) -> list[str]:
    argv: list[str] = []
    for field, option in BOOL_OPTIONS.items():
        if field in config and config[field] != "":
            argv.append(f"--{option}={bool_value(config[field])}")
    for field, option in VALUE_OPTIONS.items():
        if field in config and config[field] != "":
            argv.append(f"--{option}={config[field]}")
    return argv


def safe_name(value: str) -> str:
    cleaned = re.sub(r"[^A-Za-z0-9_.-]+", "_", value).strip("._")
    return cleaned or "case"


def discover_inputs(root: Path) -> list[Path]:
    return sorted(
        path.resolve()
        for path in root.rglob("before_cvpipelining_*.mlir")
        if not path.name.endswith(".raw.mlir")
        and ".before_local_plan_memory." not in path.name
        and ".after_local_plan_memory." not in path.name
    )


def adapter_and_snapshot(path: Path, root: Path) -> tuple[str, str]:
    relative = path.resolve().relative_to(root.resolve())
    adapter = next(
        (part for part in relative.parts if part.endswith(".ttadapter")),
        relative.parts[0],
    )
    snapshot = path.stem.removeprefix("before_cvpipelining_")
    return safe_name(adapter), safe_name(snapshot)
