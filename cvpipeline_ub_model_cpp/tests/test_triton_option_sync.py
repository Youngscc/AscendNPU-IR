#!/usr/bin/env python3

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
demo_script = read("cvpipeline_ub_model_cpp/run_demo_ub_plan.sh")
corpus_script = read("cvpipeline_ub_model_cpp/scripts/run_corpus_oracle.py")

assert re.search(
    r"def EnableTritonKernelCompile.*?\"false\"", options_td, re.DOTALL
), "bishengir-compile Triton default is not false"
assert re.search(
    r'enableTritonKernelCompile\(.*?"enable-triton-kernel-compile".*?'
    r"llvm::cl::init\(false\)",
    suffix_source,
    re.DOTALL,
), "suffix compiler Triton default differs from bishengir-compile"
assert re.search(
    r"bool enableTritonKernelCompile = false;", model_main
), "model Triton default differs from bishengir-compile"

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

print("[PASS] Triton option defaults and forwarding stay synchronized")
