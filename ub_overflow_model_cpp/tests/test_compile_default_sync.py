#!/usr/bin/env python3
"""Keep the lightweight model's supported semantic CLI aligned with cv2pm."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
model = (ROOT / "ub_overflow_model_cpp/src/main.cpp").read_text()
pipeline = (
    ROOT / "ub_overflow_model_cpp/src/pipeline/cvpipelining_ub_pipeline.hpp"
).read_text()
cv2pm = (
    ROOT / "bishengir/tools/bishengir-cvpipeline-suffix-compile/"
    "bishengir-cvpipeline-suffix-compile.cpp"
).read_text()
runner = (
    ROOT / "ub_overflow_model_cpp/scripts/run_corpus_matrix.py"
).read_text()
retry_manager = (
    ROOT
    / "bishengir/lib/Tools/RetriablePassManager/RetriablePassManager.cpp"
).read_text()
production_pipeline = (
    ROOT / "bishengir/lib/Dialect/HIVM/Pipelines/HIVMPipelines.cpp"
).read_text()
prediction_pass = (
    ROOT / "bishengir/lib/Dialect/HIVM/Pipelines/UBOverflowPrediction.cpp"
).read_text()
compile_options = (
    ROOT / "bishengir/include/bishengir/Tools/bishengir-compile/Options.td"
).read_text()

supported = (
    "--disable-auto-cv-work-space-manage",
    "--enable-preload",
    "--enable-code-motion",
    "--enable-auto-bind-sub-block",
    "--enable-hivm-auto-storage-align",
    "--enable-hivm-cross-core-gss",
    "--enable-hivm-inject-block-all-sync",
    "--disable-auto-inject-block-sync",
    "--enable-ubuf-saving",
    "--tile-mix-cube-loop",
    "--tile-mix-vector-loop",
    "--enable-auto-multi-buffer",
    "--limit-auto-multi-buffer-of-local-buffer",
    "--limit-auto-multi-buffer-buffer",
    "--enable-triton-kernel-compile",
    "--plan-memory-seed",
)
for option in supported:
    assert option in model, f"model does not accept {option}"
    assert option.removeprefix("--") in cv2pm, f"cv2pm does not accept {option}"
    assert option in runner, f"22-scenario runner does not forward {option}"

for retired in (
    "--random-seed",
    "--disable-cv-pipelining",
    "--enable-cv-lazy-loading",
    "--disable-align-alloc-size",
    "--disable-enable-stride-align",
    "--disable-infer-hivm-data-layout",
):
    assert retired not in model, f"retired model-only option remains: {retired}"

assert "disableAutoCVWorkSpaceManage" in pipeline
assert "if (!options.disableAutoCVWorkSpaceManage)" in pipeline
assert "options.enableHIVMCrossCoreGSS &&" in pipeline
assert "!options.enableHIVMInjectBlockAllSync &&" in pipeline
assert "!options.disableAutoInjectBlockSync" in pipeline
assert "RunInjectBlockSync" in pipeline
assert "bool enableHIVMAutoStorageAlign = true;" in model
assert "int planMemorySeed = -1;" in model

# The model adapter and the real pass must share one resolved
# CVPipeliningOptions object.  Reconstructing CV defaults independently at the
# prediction boundary makes option drift invisible to unit tests.
assert "const CVPipeliningOptions &cvPipeliningOptions" in production_pipeline
assert (
    "predictionConfig(hivmPipelineOptions, pipelineOptions,\n"
    "                         traceAttempt)"
    in production_pipeline
)
assert "createCVPipeliningPass(pipelineOptions)" in production_pipeline
assert production_pipeline.index(
    "predictionConfig(hivmPipelineOptions, pipelineOptions,"
) < production_pipeline.index("createCVPipeliningPass(pipelineOptions)")
assert "cvPipeliningOptions.setDepthInUnrollMode" in production_pipeline
assert "cvPipeliningOptions.enableLazyLoading" in production_pipeline
assert "cvPipeliningOptions.enableSkewMode" in production_pipeline
for field in (
    "disableAutoCVWorkSpaceManage",
    "cvPipelineDepth",
    "enableCVLazyLoading",
    "enablePreload",
    "enableCodeMotion",
    "enableAutoBindSubBlock",
    "enableUbufSaving",
    "enableAutoMultiBuffer",
    "enableHIVMAutoStorageAlign",
    "enableHIVMCrossCoreGSS",
    "enableHIVMInjectBlockAllSync",
    "disableAutoInjectBlockSync",
    "tileMixVectorLoop",
    "tileMixCubeLoop",
    "localMultiBufferStrategy",
    "mixMultiBufferStrategy",
):
    assert f"config.modelOptions.{field}" in production_pipeline, (
        f"prediction adapter omits {field}"
    )

# This stderr record is the stable subprocess result consumed by autotune.
for field in (
    "contract_version=",
    "status=",
    "precision=",
    "overflow=",
    "ub_peak_bits=",
    "required_bits=",
    "capacity_bits=",
    "selected_seed=",
    "decision_path=",
    "conservative_upper_bound_bits=",
    "serialize_ns=",
    "model_ns=",
    "input_digest=",
    "options_digest=",
    "diagnostic_category=",
):
    assert field in prediction_pass, f"prediction result omits {field}"

# Both BiSheng's retry manager and Triton UBTuner identify the production
# PlanMemory failure with the literal phrase "ub overflow".  Keep the stable
# category as well, but do not bypass those existing recovery paths.
assert "predicted_ub_overflow: ub overflow, requires" in prediction_pass
assert "BISHENGIR_UB_MODEL_EMIT_RESULT" in prediction_pass
for option, next_option in (
    ("EnableUBOverflowPrediction", "PrunePredictedUBOverflow"),
    ("PrunePredictedUBOverflow", "EnableHIVMCrossCoreGSS"),
):
    option_definition = compile_options.split(f"def {option} :", 1)[1].split(
        f"def {next_option} :", 1
    )[0]
    assert '"bool",\n  "true"' in option_definition, (
        f"{option} must remain enabled by default"
    )

# The opt-in manual trace is deliberately separate from the production result
# contract.  Its second marker is a module pass sequenced after the nested real
# CVPipelining pass, so its presence proves that CVPipelining completed.
assert "[UB-FLOW][ATTEMPT " in prediction_pass
assert "[LIGHTWEIGHT_MODEL][RESULT]" in prediction_pass
assert "[LIGHTWEIGHT_MODEL][OPTIONS]" in prediction_pass
assert "[LIGHTWEIGHT_MODEL][DECISION]" in prediction_pass
assert "[UB-FLOW][ATTEMPT " in production_pipeline
assert "[BISHENG_CVPIPELINE][DONE]" in production_pipeline
assert production_pipeline.index("createCVPipeliningPass(pipelineOptions)") < (
    production_pipeline.rindex("createTraceAfterCVPipeliningPass(traceAttempt)")
)
assert "[BISHENG][FALLBACK][RETRY]" in retry_manager
assert "[BISHENG][FALLBACK][SUMMARY]" in retry_manager

print("[PASS] model/cv2pm options and prediction contract stay aligned")
