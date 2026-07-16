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

explicit_corpus_option = (
    'f"--enable-triton-kernel-compile={'
    "'true' if args.enable_triton_kernel_compile else 'false'}\""
)
assert corpus_script.count(explicit_corpus_option) == 5, (
    "corpus model/oracle/retry commands do not share one Triton option"
)

print("[PASS] bishengir-compile defaults and forwarding stay synchronized")
