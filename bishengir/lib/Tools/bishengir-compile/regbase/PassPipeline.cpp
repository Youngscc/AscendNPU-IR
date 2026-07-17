//===- PassPipeline.cpp - BiShengIR regbase pass pipeline ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// RegBase (A5) pass pipeline implementation. Migrated from AscendNPU-IR-Dev.
// Passes that depend on dialects/transforms not yet available in A3 are
// commented out with TODO(regbase-migration) markers for gradual integration.
//
//===----------------------------------------------------------------------===//

#include "bishengir/Tools/bishengir-compile/regbase/PassPipeline.h"

#include "bishengir/Config/bishengir-config.h"
#include "bishengir/Conversion/Passes.h"
// TODO(regbase-migration): HIVMAVEToStandard not yet available in A3.
//   Dependency: bishengir/Conversion/HIVMAVEToStandard (HIVMAVE dialect lowering).
// #include "bishengir/Conversion/HIVMAVEToStandard/HIVMAVEToStandard.h"
// TODO(regbase-migration): FixCallUnknownLoc not yet available in A3.
//   Dependency: bishengir/Conversion/FixCallUnknownLoc.
// #include "bishengir/Conversion/FixCallUnknownLoc/FixCallUnknownLoc.h"
// TODO(regbase-migration): HIVMAVEToAVEIntrin not yet available in A3.
//   Dependency: bishengir/Conversion/HIVMAVEToAVEIntrin.
// #include "bishengir/Conversion/HIVMAVEToAVEIntrin/HIVMAVEToAVEIntrin.h"
#include "bishengir/Conversion/HIVMToStandard/HIVMToStandard.h"
#include "bishengir/Dialect/Annotation/Transforms/Passes.h"
// TODO(regbase-migration): AscendDPX not yet available in A3.
//   Dependency: bishengir/Dialect/AscendDPX (DPX dialect and transforms).
// #include "bishengir/Dialect/AscendDPX/Transforms/Passes.h"
#include "bishengir/Dialect/HACC/IR/HACC.h"
// TODO(regbase-migration): bishengir/Dialect/HACC/Pipelines/Passes.h not in A3.
//   buildLowerHACCToLLVMPipeline is declared in HACCPipelines.cpp but not in a
//   public header in A3. Host-compile path commented out below.
// #include "bishengir/Dialect/HACC/Pipelines/Passes.h"
#include "bishengir/Dialect/HACC/Transforms/Passes.h"
#include "bishengir/Dialect/HACC/Utils/Utils.h"
#include "bishengir/Dialect/HFusion/Pipelines/Passes.h"
#include "bishengir/Dialect/HFusion/Transforms/Passes.h"
#include "bishengir/Dialect/HIVM/Pipelines/ConvertToHIVMPipeline.h"
#include "bishengir/Dialect/HIVM/Pipelines/Passes.h"
#include "bishengir/Dialect/HIVM/Transforms/Passes.h"
// TODO(regbase-migration): HIVMAVE pipelines not yet available in A3.
//   Dependency: bishengir/Dialect/HIVMAVE (HIVMAVE dialect and pipelines).
// #include "bishengir/Dialect/HIVMAVE/Pipelines/Passes.h"
#include "bishengir/Dialect/Scope/Transforms/Passes.h"
// TODO(regbase-migration): Triton pipelines not yet available in A3.
//   Dependency: bishengir/Dialect/Triton (Triton dialect, pipelines, transforms).
// #include "bishengir/Dialect/Triton/Pipelines/Passes.h"
// #include "bishengir/Dialect/Triton/Transforms/Passes.h"
// TODO(regbase-migration): ExecutionEngine not yet available in A3.
//   Dependency: bishengir/ExecutionEngine (execution engine passes).
// #include "bishengir/ExecutionEngine/Passes.h"
#include "bishengir/Tools/bishengir-compile/BiShengIRCompile.h"
// TODO(regbase-migration): InjectIRInstrumentation not yet available in A3.
//   Dependency: bishengir/Transforms/InjectIRInstrumentation.h.
// #include "bishengir/Transforms/InjectIRInstrumentation.h"
#include "bishengir/Transforms/Passes.h"
#include "mlir/Conversion/AffineToStandard/AffineToStandard.h"
#include "mlir/Conversion/ArithToEmitC/ArithToEmitCPass.h"
#include "mlir/Conversion/LLVMCommon/ConversionTarget.h"
#include "mlir/Conversion/Passes.h"
#include "mlir/Dialect/Arith/Transforms/Passes.h"
#include "mlir/Dialect/GPU/Transforms/Passes.h"
#include "mlir/Dialect/Linalg/Passes.h"
#include "mlir/Dialect/MemRef/Transforms/Passes.h"
#include "mlir/Dialect/SCF/Transforms/Passes.h"
#include "mlir/Support/FileUtilities.h"
#include "mlir/Target/LLVMIR/ModuleTranslation.h"
#include "mlir/Transforms/Passes.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/WithColor.h"

#include <memory>
#include <set>

#if BISHENGIR_ENABLE_TORCH_CONVERSIONS
#include "bishengir/Dialect/Torch/Pipelines/Passes.h"
#endif

using namespace mlir;

namespace bishengir {
namespace regbase {

// Helper function to set up HFusionPipelineOptions
static void
setupHFusionPipelineOptions(hfusion::HFusionPipelineOptions &hfusionPipelineOptions,
                            const BiShengIRCompileMainConfig &config) {
  auto &options = hfusionPipelineOptions;
#define GEN_HFUSION_OPTION_SETUP
#include "bishengir/Tools/bishengir-compile/ConfigUtils.cpp.inc"
  // TODO(regbase-migration): insertFFTS and target fields are not available
  //   in A3's HFusionPipelineOptions. These are set via the GEN macro when
  //   the corresponding Options.td entries are migrated.
  // hfusionPipelineOptions.insertFFTS =
  //     !hfusionPipelineOptions.disableFFTS &&
  //     mlir::hacc::utils::isFFTSSupportedArch(config.getTarget());
  // hfusionPipelineOptions.target =
  //     mlir::hacc::stringifyTargetDeviceEnum(config.getTarget());
}

static void
setupHIVMPipelineOptions(hivm::HIVMPipelineOptions &hivmPipelineOptions,
                         const BiShengIRCompileMainConfig &config) {
  auto &options = hivmPipelineOptions;
#define GEN_HIVM_OPTION_SETUP
#include "bishengir/Tools/bishengir-compile/ConfigUtils.cpp.inc"
  // TODO(regbase-migration): target, setCVPipelineMode, and CVPipelineMode
  //   are not available in A3's HIVMPipelineOptions. The CV pipeline mode
  //   guard below will be restored when these fields are migrated.
  // hivmPipelineOptions.target =
  //     mlir::hacc::stringifyTargetDeviceEnum(config.getTarget());

  // TODO(regbase-migration): isUBAwareVfFusion depends on VfFusionMode which is
  // an A5UnmigratedOptionStub in A3. When the VFFusion implementation is
  // migrated, restore this UB-aware VF fusion guard.
  // if (config.isUBAwareVfFusion() && hivmPipelineOptions.enableVfMergeLevel > 0)
  //   hivmPipelineOptions.enableVfMergeLevel = 0;

  // TODO(regbase-migration): CV pipeline mode guards below depend on
  //   setCVPipelineMode and CVPipelineMode which are not in A3's options.
  //   Restore when these options are migrated.
  // if (options.enablePreload) {
  //   if (options.setCVPipelineMode != CVPipelineMode::Skew) {
  //     ...
  //   }
  // }
  // if (options.setCVPipelineMode == CVPipelineMode::Off &&
  //     options.setWorkspaceMultibuffer != 0) {
  //   ...
  // }
}

// TODO(regbase-migration): setupHIVMAVEPipelineOptions not yet available.
//   Dependency: HIVMAVE dialect and HIVMAVEPipelineOptions.
//   Expected location in A5: bishengir/lib/Tools/bishengir-compile/PassPipeline.cpp:133-190
#if 0
static void
setupHIVMAVEPipelineOptions(hivmave::HIVMAVEPipelineOptions &hivmAVEPipelineOptions,
                            const BiShengIRCompileMainConfig &config) {
  hivmAVEPipelineOptions.enableTritonKernelCompile =
      config.getEnableTritonKernelCompile();
  hivmAVEPipelineOptions.enableMixedCV = config.shouldEnableMixedCV();
  hivmAVEPipelineOptions.enableLayoutOptimization =
      config.shouldEnableLayoutOptimization();
  // ... (full implementation in A5)
}
#endif

void buildSIMTPipeline(OpPassManager &pm,
                       const BiShengIRCompileMainConfig &config) {
  pm.addPass(createCSEPass());
  pm.addPass(createSCCPPass());

  // TODO(regbase-migration): TritonRemapPass not yet available in A3.
  //   Dependency: bishengir/Dialect/Triton (Triton dialect).
  // auto tritonGridDim = config.getSimtTritonGrid();
  // bishengir::TritonRemapOptions options;
  // if (!tritonGridDim.empty()) {
  //   options.gridDimX = static_cast<int>(tritonGridDim[0]);
  //   options.useGridFlag = true;
  // }
  // if (tritonGridDim.size() > 1)
  //   options.gridDimY = static_cast<int>(tritonGridDim[1]);
  // if (tritonGridDim.size() > 2)
  //   options.gridDimZ = static_cast<int>(tritonGridDim[2]);
  //
  // // TODO: When DPX covers all remapper features correctly, remove
  // // createTritonRemapPass completely.
  // if (!config.getUseDPX())
  //   pm.addPass(bishengir::triton::createTritonRemapPass(options));

  pm.addPass(createCanonicalizerPass());
  pm.addPass(createCSEPass());
  pm.addPass(createCanonicalizerPass());
  pm.addPass(createConvertSCFToCFPass());
  pm.addPass(createConvertControlFlowToLLVMPass());
  pm.addPass(createArithToLLVMConversionPass());

  buildLowerToLLVMPipeline(pm, config);
}

void buildBiShengHIRAVEToLLVMPipeline(
    OpPassManager &pm, const BiShengIRCompileMainConfig &config) {
  // TODO(regbase-migration): buildLowerHACCToLLVMPipeline not available in A3
  //   (declared in HACCPipelines.cpp but no public header in A3). Host-compile
  //   path disabled until HACC pipeline headers are properly exported.
  // if (config.getCompileHost()) {
  //   hacc::buildLowerHACCToLLVMPipeline(pm, config.getHostOutputFile());
  //   return;
  // }

  // TODO(regbase-migration): HIVMAVE pipeline not yet available in A3.
  //   Dependency: bishengir/Dialect/HIVMAVE (HIVMAVE dialect and pipelines).
  // if (config.getEnableHIVMCompile()) {
  //   hivmave::HIVMAVEPipelineOptions hivmAVEPipelineOptions;
  //   setupHIVMAVEPipelineOptions(hivmAVEPipelineOptions, config);
  //   hivmave::buildLowerAVEPipelines(pm, hivmAVEPipelineOptions);
  // }

  // TODO(regbase-migration): LowerToLLVM pipeline - some passes not available.
  //   buildLowerToLLVMPipeline has been stubbed for A3 compatibility.
  // if (config.getLowerToLLVM()) {
  //   buildLowerToLLVMPipeline(pm, config);
  // }
}

/// Build the pipeline to lower BiShengHIR to LLVM Dialect IR.
/// Many passes in this pipeline depend on A5-specific dialects and conversions
/// that have not yet been migrated to A3. The implementation below preserves
/// the regbase-compatible subset.
void buildLowerToLLVMPipeline(OpPassManager &pm,
                              const BiShengIRCompileMainConfig &config) {

  // TODO(regbase-migration): DebugMemoryPass not yet available in A3.
  //   Dependency: bishengir/Transforms (DebugMemory pass).
  // const bool ascendDebugPrint =
  //     StringRef(getenv("ASCEND_DEBUG_PRINT")) == "ALL";
  // if (ascendDebugPrint) {
  //   DebugMemoryOptions debugMemoryOptions;
  //   pm.addPass(createDebugMemoryPass(debugMemoryOptions));
  // }

  // TODO(regbase-migration): AnnotationLoweringPass not yet available in A3.
  //   Dependency: bishengir/Dialect/Annotation conversion paths.
  // pm.addPass(annotation::createAnnotationLoweringPass());

  // TODO(regbase-migration): AllocToAllocaPass not yet available in A3.
  //   Dependency: bishengir/Dialect/HIVM (hivm::createAllocToAllocaPass).
  // pm.addPass(hivm::createAllocToAllocaPass());

  // TODO(regbase-migration): InsertInitAndFinishForDebugPass not available.
  //   Dependency: bishengir/Dialect/HIVM transforms.
  // pm.nest<func::FuncOp>().addPass(
  //     hivm::createInsertInitAndFinishForDebugPass());

  // HIVM to Standard conversion - core lowering path.
  ConvertHIVMToStandardOptions hivmToStdOptions;
  // TODO(regbase-migration): isOpsAligned and markLibCallNoInline
  //   not available in A3's ConvertHIVMToStandardOptions. Use defaults.

  // TODO(regbase-migration): MarkDisableLoadPass not available.
  // pm.addPass(hivm::createMarkDisableLoadPass());

  // TODO(regbase-migration): addSyncBlockLockFinalizePasses not available.
  // hivm::addSyncBlockLockFinalizePasses(pm);

  pm.addPass(createConvertHIVMToStandardPass(hivmToStdOptions));

  // TODO(regbase-migration): ConvertHIVMAVEToStandardPass not available in A3.
  //   Dependency: bishengir/Conversion/HIVMAVEToStandard.
  // pm.addPass(createConvertHIVMAVEToStandardPass());

  // TODO(regbase-migration): FixCallUnknownLocPass not available in A3.
  //   Dependency: bishengir/Conversion/FixCallUnknownLoc.
  // pm.nest<func::FuncOp>().addPass(createFixCallUnknownLocPass());

  pm.addPass(memref::createExpandStridedMetadataPass());

  // TODO(regbase-migration): ConvertHIVMAVEToAVEIntrinPass not available in A3.
  //   Dependency: bishengir/Conversion/HIVMAVEToAVEIntrin.
  // pm.addPass(createConvertHIVMAVEToAVEIntrinPass());

  // TODO(regbase-migration): HoistVstasPass not available in A3.
  //   Dependency: bishengir/Dialect/HIVMAVE.
  // pm.addPass(hivmave::createHoistVstasPass());

  // TODO(regbase-migration): DPX/SIMT-specific passes not available in A3.
  //   Dependency: AscendDPX dialect, Triton dialect.
  // if (config.getPureSimt() && config.getUseDPX()) {
  //   auto tritonGridDim = config.getSimtTritonGrid();
  //   bishengir::TritonRemapOptions options;
  //   options.isSimdSimtMixCompile = config.getEnableSimdSimtMixCompile();
  //   if (!tritonGridDim.empty()) {
  //     options.gridDimX = static_cast<int>(tritonGridDim[0]);
  //     options.useGridFlag = true;
  //   }
  //   if (tritonGridDim.size() > 1)
  //     options.gridDimY = static_cast<int>(tritonGridDim[1]);
  //   if (tritonGridDim.size() > 2)
  //     options.gridDimZ = static_cast<int>(tritonGridDim[2]);
  //   pm.addPass(bishengir::triton::createAdaptGPUKernelPass(options));
  //   pm.addPass(mlir::ascend_dpx::createHoistCallScalarToCallerPass());
  //   if (config.getEnableSIMTFastDiv())
  //     pm.addPass(mlir::ascend_dpx::createDPXDivOptimizationPass(options));
  // }
  // pm.addPass(createConvertAscendDPXToHIVMRegbaseIntrinPass());
  // pm.addPass(bishengir::triton::createDecomposeFRemPass());

  pm.addPass(createConvertSCFToCFPass());
  pm.addPass(createLowerAffinePass());
  pm.addPass(arith::createArithExpandOpsPass());
}

// TODO(regbase-migration): buildDelayedHFusionRegBaseVectorizePipeline
//   partially not available. ExecutionEngine conversion and
//   createInferFuncCoreTypePass are not in A3.
#if 0
static void buildDelayedHFusionRegBaseVectorizePipeline(
    mlir::OpPassManager &pm, const BiShengIRCompileMainConfig &config,
    bool shouldInferFuncCoreType = true) {
  if (config.getDisableHfusionVectorize()) {
    return;
  }
  // inferMixedCV populates enableMixedCV before this delayed HFusion pipeline
  // is built; adjust only the local HFusion options consumed by flatten.
  BiShengIRCompileMainConfig hfusionConfig = config;
  auto &registeredOptions = llvm::cl::getRegisteredOptions();
  auto enableFlattenOpt = registeredOptions.find("enable-flatten");
  bool hasExplicitEnableFlatten =
      enableFlattenOpt != registeredOptions.end() &&
      enableFlattenOpt->second->getNumOccurrences() != 0;
  // TODO(regbase-migration): shouldEnableMixedCV not available in A3 Config.
  //   Use inline check.
  // if (hfusionConfig.shouldEnableMixedCV() && !hasExplicitEnableFlatten) {
  //   hfusionConfig.setEnableFlatten(false);
  // }

  HIVMAggregatedDecomposeOpOptions decomposeOption;
  decomposeOption.decomposePhase = bishengir::DecomposePhase::NO_CONSTRAINT;
  pm.nest<func::FuncOp>().addPass(
      mlir::hivm::createHIVMAggregatedDecomposeOpPass(decomposeOption));
  hfusion::HFusionPipelineOptions hfusionPipelineOptions;
  setupHFusionPipelineOptions(hfusionPipelineOptions, hfusionConfig);

  // TODO(regbase-migration): ExecutionEngine pass not available.
  // ExecutionEngineHIVMToUpstreamConversionOptions upstreamOptions;
  // upstreamOptions.convertToNamedOp =
  //     hacc::utils::isRegBasedArch(config.getTarget());
  // pm.addPass(
  //     mlir::execution_engine::createConvertHIVMToUpstreamPass(upstreamOptions));

  hfusion::buildHFusionRegBasePipeline(pm, hfusionPipelineOptions);

  // TODO(regbase-migration): InferFuncCoreTypePass not available.
  // if (shouldInferFuncCoreType) {
  //   pm.addPass(mlir::hivm::createInferFuncCoreTypePass());
  // }

  ConvertHFusionToHIVMOptions hfs2hivmOptions;
  hfs2hivmOptions.mmMapMode = config.getEnableTritonKernelCompile()
                                  ? hfusion::MmMapMode::MacroInstr
                                  : hfusion::MmMapMode::CoreOp;
  pm.addPass(createHFusionToHIVMConversionPass(hfs2hivmOptions));
}
#endif

void buildFinalHIVMPipelines(mlir::OpPassManager &pm,
                             const BiShengIRCompileMainConfig &config) {
  if (config.getEnableHIVMCompile()) {
    hivm::HIVMPipelineOptions hivmPipelineOptions;
    setupHIVMPipelineOptions(hivmPipelineOptions, config);

    // TODO(regbase-migration): buildDelayedHFusionRegBaseVectorizePipeline
    //   not available due to missing ExecutionEngine and InferFuncCoreTypePass.
    // if (config.getEnableSimdSimtMixCompile()) {
    //   buildDelayedHFusionRegBaseVectorizePipeline(
    //       pm, config, /*shouldInferFuncCoreType=*/true);
    // }

    // In A3, the HIVM optimization pipeline is named buildOptimizeHIVMPipeline
    // instead of A5's buildLowerHIVMPipelines.
    hivm::buildOptimizeHIVMPipeline(pm, hivmPipelineOptions);
  }
}

// TODO(regbase-migration): buildBiShengTTIRPipeline not yet available in A3.
//   Dependency: whole Triton dialect and lowering pipeline.
//   Expected location in A5: bishengir/lib/Tools/bishengir-compile/PassPipeline.cpp:391-408
#if 0
void buildBiShengTTIRPipeline(OpPassManager &pm,
                              const BiShengIRCompileMainConfig &config) {
  if (config.getEnableSimdSimtMixCompile()) {
    pm.addPass(hivm::createMaterializeSimtVFMemScopePass());
    pm.addPass(createHIVMToTritonGPUConversionPass());
  }

  if (!config.getCompileHost()) {
    pm.addPass(hacc::createAppendDeviceSpecPass(
        hacc::AppendTargetDeviceSpecOptions{config.getTarget()}));
  }
  pm.addPass(createCanonicalizeModulePass());
  triton::LowerTritonPipelineOptions lowerTritonPipelineOptions;
  setupLowerTritonPipelineOptions(lowerTritonPipelineOptions, config);
  bishengir::triton::buildLowerTritonPipeline(pm, lowerTritonPipelineOptions);
}
#endif

void buildBiShengHIRFinishPipeline(mlir::OpPassManager &pm,
                                   const BiShengIRCompileMainConfig &config) {
  // TODO(regbase-migration): createWriteBackSharedPass not available in A3.
  //   Dependency: bishengir/Dialect/HIVM (WriteBackShared pass).
  // pm.addPass(hivm::createWriteBackSharedPass());
}

void buildBiShengHIRPipeline(OpPassManager &pm,
                             const BiShengIRCompileMainConfig &config) {
  if (!config.getCompileHost()) {
    pm.addPass(hacc::createAppendDeviceSpecPass(
        hacc::AppendTargetDeviceSpecOptions{config.getTarget()}));
  }

  pm.addPass(createCanonicalizeModulePass());

#if BISHENGIR_ENABLE_TORCH_CONVERSIONS
  if (config.getEnableTorchCompile()) {
    TorchToNamedOpPipelineOptions torchToNamedOpOptions;
    torchToNamedOpOptions.ensureNoImplicitBroadcast =
        config.getEnsureNoImplicitBroadcast();
    createTorchBackendToNamedOpBackendPipeline(pm, torchToNamedOpOptions);
  }
#endif

  if (config.getEnableHfusionCompile()) {
    auto hfusionPipelineOptions =
        std::make_unique<hfusion::HFusionPipelineOptions>();
    setupHFusionPipelineOptions(*hfusionPipelineOptions, config);

    // TODO(regbase-migration): SIMD/SIMT mix compile not yet available.
    //   When enabled, delay reg-based vectorization until SIMT code is split.
    // if (config.getEnableSimdSimtMixCompile()) {
    //   hfusionPipelineOptions->disableHfusionVectorize = true;
    // }

    hfusion::buildHFusionPipelines(pm, *hfusionPipelineOptions);
  }

  if (config.getEnableHIVMCompile()) {
    // Build convert to HIVM Dialect pipeline.
    auto convertToHIVMOptions =
        std::make_unique<hivm::ConvertToHIVMPipelineOptions>();
    convertToHIVMOptions->enableTritonKernelCompile =
        config.getEnableTritonKernelCompile();
    // TODO(regbase-migration): enableRegBaseHIVMPipe not available in A3's
    //   ConvertToHIVMPipelineOptions. RegBase flag is inferred from target.
    // convertToHIVMOptions->enableRegBaseHIVMPipe =
    //     mlir::hacc::utils::isRegBasedArch(config.getTarget());
    hivm::buildConvertToHIVMPipeline(pm, *convertToHIVMOptions);
    // In A3, the HIVM optimization pipeline (buildOptimizeHIVMPipeline) already
    // includes tensor optimization passes, so the separate
    // buildHIVMTensorOptimizations call from A5 is not needed here.
    // hivm::buildHIVMTensorOptimizations(pm, hivmPipelineOptions);

    // TODO(regbase-migration): Mixed CV decompose + delayed vectorize
    //   not yet available (depends on ExecutionEngine pass and
    //   shouldEnableMixedCV which is not in A3 Config).
    // if (config.shouldEnableMixedCV()) {
    //   ...
    // }

    // TODO(regbase-migration): SIMD/SIMT mix compile scope/split passes
    //   not yet available (depends on getEnableSimdSimtMixCompile which
    //   is not available in A3 Config).
  }
}

/// We define the BiShengIR Compile Pass here not in a tablegen file because
/// there potentially many options that are controlled by cmake options, and
/// it's more flexible to define in cpp.
struct BiShengIRCompilePass
    : public PassWrapper<BiShengIRCompilePass, OperationPass<ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(BiShengIRCompilePass)
  BiShengIRCompilePass() = default;
  BiShengIRCompilePass &operator=(const BiShengIRCompilePass &pass) = delete;
  BiShengIRCompilePass(const BiShengIRCompilePass &pass)
      : PassWrapper<BiShengIRCompilePass, OperationPass<ModuleOp>>(pass) {}
  StringRef getArgument() const override { return "bishengir-compile-regbase"; }
  StringRef getDescription() const override {
    return "Compile BiShengIR module to binary (regbase/A5 pipeline).";
  }

  void runOnOperation() override {
    ModuleOp moduleOp = getOperation();
    BiShengIRCompileMainConfig config;
    config.setOutputFile(outputFile);

    // Collect pass options directly and pass them as HIVMC args.
    // This approach follows A3's BiShengIRCompilePass pattern instead of
    // A5's printAsTextualPipeline-based approach (which depends on
    // MLIR C API utilities not available in A3's MLIR version).
    SmallVector<Pass::Option<bool> *> sharedWithHIVMCompileBool = {
        &enableAutoBindSubBlock,
        &enableAutoBlockifyLoop,
        &enableAutoMultiBuffer,
        &enableBinRelocation,
        &enableCodeMotion,
        &enableDebugInfo,
        &enableHIVMCompile,
        &enableStaticBarePtr,
        &enableTritonKernelCompile,
    };

    SmallVector<Pass::Option<unsigned> *> sharedWithHIVMCompileUnsigned = {
        &setWorkspaceMultibuffer,
        &tileMixVectorLoop,
        &tileMixCubeLoop,
    };

    std::vector<std::string> collectedArgs;
    for (auto &opt : sharedWithHIVMCompileBool) {
      std::string arg =
          opt->getArgStr().str() + "=" + (opt->getValue() ? "true" : "false");
      collectedArgs.push_back(arg);
    }
    for (auto &opt : sharedWithHIVMCompileUnsigned) {
      std::string arg =
          opt->getArgStr().str() + "=" + std::to_string(opt->getValue());
      collectedArgs.push_back(arg);
    }
    // Add target option
    collectedArgs.push_back("target=" +
                            mlir::hacc::stringifyTargetDeviceEnum(target.getValue()).str());
    for (const std::string &arg : this->hivmcArgs) {
      collectedArgs.push_back(arg);
    }

    config.setHIVMCArgs(collectedArgs);

    if (failed(runRegBasePipeline(moduleOp, config))) {
      signalPassFailure();
    }
  }

protected:
#define GEN_ALL_OPTION_REGISTRATION
#include "bishengir/Tools/bishengir-compile/PassOptions.cpp.inc"

  Pass::Option<std::string> outputFile{
      *this, "o", llvm::cl::desc("Specify output bin name"),
      llvm::cl::init("-")};
};

} // namespace regbase
} // namespace bishengir

void bishengir::regbase::registerBiShengIRCompilePass() {
  PassRegistration<bishengir::regbase::BiShengIRCompilePass>();
}
