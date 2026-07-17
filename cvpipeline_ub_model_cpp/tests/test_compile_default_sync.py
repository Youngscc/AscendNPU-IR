#!/usr/bin/env python3
"""Keep UB-affecting defaults aligned with bishengir-compile."""

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def read(relative: str) -> str:
    return (ROOT / relative).read_text(encoding="utf-8")


options_td = read("bishengir/include/bishengir/Tools/bishengir-compile/Options.td")
suffix_source = read(
    "bishengir/tools/bishengir-cvpipeline-suffix-compile/"
    "bishengir-cvpipeline-suffix-compile.cpp"
)
model_main = read("cvpipeline_ub_model_cpp/src/main.cpp")
model_pipeline = read(
    "cvpipeline_ub_model_cpp/src/pipeline/cvpipelining_ub_pipeline.hpp"
)
model_wrapper = read(
    "cvpipeline_ub_model_cpp/scripts/plan_before_cvpipelining_ub.py"
)
demo_script = read("cvpipeline_ub_model_cpp/run_demo_ub_plan.sh")
corpus_script = read("cvpipeline_ub_model_cpp/scripts/run_corpus_oracle.py")


def assert_compile_default(option: str, expected: str) -> None:
    assert re.search(
        rf"def {option} .*?\"{re.escape(expected)}\"",
        options_td,
        re.DOTALL,
    ), f"unexpected bishengir-compile default for {option}"


assert_compile_default("TileMixVectorLoop", "2")
assert_compile_default("TileMixCubeLoop", "2")
assert_compile_default("EnableTritonKernelCompile", "false")
assert_compile_default(
    "LimitAutoMultiBufferOfLocalBuffer",
    "MultiBufferStrategy::CUBE_NO_L0C",
)
assert_compile_default(
    "LimitAutoMultiBufferBuffer",
    "MultiBufferStrategy::ONLY_CUBE",
)

for variable in ("tileMixVectorLoop", "tileMixCubeLoop"):
    assert re.search(
        rf'{variable}\(.*?llvm::cl::init\(2\)', suffix_source, re.DOTALL
    ), f"suffix compiler {variable} default differs from bishengir-compile"
    assert re.search(
        rf"unsigned {variable} = 2;", model_pipeline
    ), f"model {variable} default differs from bishengir-compile"

assert re.search(
    r'enableTritonKernelCompile\(.*?"enable-triton-kernel-compile".*?'
    r"llvm::cl::init\(false\)",
    suffix_source,
    re.DOTALL,
), "suffix compiler Triton default differs from bishengir-compile"
assert re.search(
    r"bool enableTritonKernelCompile = false;", model_main
), "model Triton default differs from bishengir-compile"

assert re.search(
    r"localMultiBufferStrategy\s*=\s*\n\s*"
    r"cvub::MultiBufferStrategy::CubeNoL0C;",
    model_main,
), "model local multi-buffer default differs from suffix"
assert re.search(
    r"mixMultiBufferStrategy\s*=\s*\n\s*"
    r"cvub::MultiBufferStrategy::OnlyCube;",
    model_main,
), "model MIX multi-buffer default differs from suffix"
assert re.search(
    r'--suffix-local-multi-buffer-strategy", default="no-l0c"',
    model_wrapper,
), "model wrapper local multi-buffer default differs from suffix"
assert re.search(
    r'--suffix-mix-multi-buffer-strategy", default="only-cube"',
    model_wrapper,
), "model wrapper MIX multi-buffer default differs from suffix"
assert "MULTIBUFFER_LOCAL_STRATEGY=no-l0c" in demo_script
assert "MULTIBUFFER_MIX_STRATEGY=only-cube" in demo_script

assert re.search(
    r'planMemorySeed\(.*?"plan-memory-seed".*?llvm::cl::init\(-1\)',
    suffix_source,
    re.DOTALL,
), "suffix compiler must keep PlanMemory retry mode by default"
assert re.search(
    r"std::optional<uint32_t> randomSeed;", model_main
), "model must leave the PlanMemory seed unspecified by default"
assert re.search(
    r'"--random-seed", type=int, default=None', model_wrapper
), "model wrapper must leave the PlanMemory seed unspecified by default"
assert 'RANDOM_SEED=""' in demo_script, (
    "demo must leave the PlanMemory seed unspecified by default"
)

demo_forward = (
    '--suffix-enable-triton-kernel-compile="${SUFFIX_ENABLE_TRITON_KERNEL_COMPILE}"',
    '--enable-triton-kernel-compile="${SUFFIX_ENABLE_TRITON_KERNEL_COMPILE}"',
)
for argument in demo_forward:
    assert argument in demo_script, f"demo does not forward {argument}"

for option in (
    "--disable-cv-pipelining",
    "--cv-pipeline-depth",
    "--enable-preload",
    "--enable-cv-lazy-loading",
    "--enable-code-motion",
    "--enable-auto-multi-buffer",
    "--enable-triton-kernel-compile",
    "--limit-auto-multi-buffer-of-local-buffer",
    "--limit-auto-multi-buffer-buffer",
):
    assert option in corpus_script, f"corpus does not expose/forward {option}"

assert corpus_script.count("*shared_pipeline_options(args)") == 5, (
    "fixed-seed, blocker and retry model/oracle commands must share options"
)

print("[PASS] bishengir-compile defaults and forwarding stay synchronized")
