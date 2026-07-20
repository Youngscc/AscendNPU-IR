#!/usr/bin/env python3
"""Tests for the corpus configuration matrix runner."""

from __future__ import annotations

import argparse
import importlib.util
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts" / "run_corpus_matrix.py"
SPEC = importlib.util.spec_from_file_location("run_corpus_matrix", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
MODULE = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = MODULE
SPEC.loader.exec_module(MODULE)


matrix = MODULE.load_matrix(ROOT / "config" / "corpus_test_matrix.tsv")
assert len(matrix) == 25
assert len({config.name for config in matrix}) == 25
configs = {config.name: config for config in matrix}
assert matrix[0].name == "default"
assert matrix[0].required
assert matrix[1].restrict_inplace_as_isa
assert configs["tile_mix_1x1"].required
assert configs["tile_mix_1x1"].tile_mix_cube_loop == 1
assert configs["tile_mix_1x1"].tile_mix_vector_loop == 1
assert not configs["tile_mix_asymmetric"].required
assert configs["tile_mix_asymmetric"].tile_mix_cube_loop == 1
assert configs["tile_mix_asymmetric"].tile_mix_vector_loop == 2
assert configs["ubuf_saving"].enable_ubuf_saving
assert configs["align_alloc_size_disabled"].disable_align_alloc_size
assert configs["stride_align_disabled"].required
assert configs["stride_align_disabled"].disable_enable_stride_align
assert configs["infer_hivm_data_layout_disabled"].required
assert configs["infer_hivm_data_layout_disabled"].disable_infer_hivm_data_layout
assert not configs["auto_multi_buffer_only_vector"].required
assert configs["depth_2_auto_multi_buffer"].required
assert configs["depth_2_auto_multi_buffer"].cv_pipeline_depth == 2
assert configs["depth_2_auto_multi_buffer"].enable_auto_multi_buffer
assert configs["preload_lazy_auto_multi_buffer"].enable_preload
assert configs["preload_lazy_auto_multi_buffer"].enable_cv_lazy_loading
assert configs["preload_lazy_auto_multi_buffer"].enable_auto_multi_buffer
assert configs["cv_disabled_auto_multi_buffer"].disable_cv_pipelining
assert configs["cv_disabled_auto_multi_buffer"].enable_auto_multi_buffer
assert not configs["auto_multi_buffer_asymmetric_strategy"].required
assert configs["auto_multi_buffer_asymmetric_strategy"].local_multi_buffer_strategy == "no-limit"
assert configs["auto_multi_buffer_asymmetric_strategy"].mix_multi_buffer_strategy == "only-cube"
assert not configs["code_motion_disabled_auto_multi_buffer"].enable_code_motion
assert configs["code_motion_disabled_auto_multi_buffer"].enable_auto_multi_buffer
assert configs["restrict_inplace_auto_multi_buffer"].restrict_inplace_as_isa
assert configs["restrict_inplace_auto_multi_buffer"].enable_auto_multi_buffer
assert matrix[-1].enable_triton_kernel_compile
assert not matrix[-1].enable_auto_multi_buffer

args = argparse.Namespace(
    runner=ROOT / "scripts" / "run_corpus_oracle.py",
    corpus_root=Path("corpus"),
    model=Path("model"),
    compiler=Path("compiler"),
    seed=0,
    max_cases=None,
    pipeline_manifest=None,
    require_all_exact=False,
    quiet=True,
    no_progress=True,
)
command = MODULE.build_command(args, matrix[-1])
assert command.count("--seeds") == 1
assert command[command.index("--seeds") + 1] == "0"
assert "--check-retry" not in command
assert "--enable-triton-kernel-compile" in command
assert "--enable-auto-multi-buffer" not in command
assert "--enable-preload" not in command

stride_command = MODULE.build_command(args, configs["stride_align_disabled"])
assert "--disable-enable-stride-align" in stride_command

ubuf_command = MODULE.build_command(args, configs["ubuf_saving"])
assert "--enable-ubuf-saving" in ubuf_command

tile_command = MODULE.build_command(args, configs["tile_mix_asymmetric"])
assert tile_command[tile_command.index("--tile-mix-cube-loop") + 1] == "1"
assert tile_command[tile_command.index("--tile-mix-vector-loop") + 1] == "2"

with tempfile.TemporaryDirectory() as directory:
    bad_matrix = Path(directory) / "bad.tsv"
    bad_matrix.write_text(
        "\t".join(MODULE.FIELDS) + "\n" +
        "bad\t1\t0\t0\t-1\t2\t0\t1\t2\t2\t0\t0\t0\t0\t0\t0\tno-l0c\tonly-cube\n",
        encoding="utf-8")
    try:
        MODULE.load_matrix(bad_matrix)
    except ValueError as error:
        assert "enable_preload must be 0 or 1" in str(error)
    else:
        raise AssertionError("invalid boolean value was accepted")
