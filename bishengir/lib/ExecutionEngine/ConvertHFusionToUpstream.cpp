//===- ConvertHFusionToUpstream.cpp --------------------------------------===//
//
// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//===----------------------------------------------------------------------===//

#include "bishengir/Dialect/HFusion/IR/HFusion.h"
#include "bishengir/ExecutionEngine/Passes.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Linalg/IR/Linalg.h"
#include "mlir/Dialect/Math/IR/Math.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Transforms/DialectConversion.h"
#include "llvm/ADT/APFloat.h"

namespace mlir {
#define GEN_PASS_DEF_EXECUTIONENGINEHFUSIONTOUPSTREAMCONVERSION
#include "bishengir/ExecutionEngine/Passes.h.inc"
} // namespace mlir

namespace {
using namespace mlir;

struct RewriteHFusionIsFiniteOp : public OpRewritePattern<hfusion::IsFiniteOp> {
  using OpRewritePattern<hfusion::IsFiniteOp>::OpRewritePattern;

  LogicalResult matchAndRewrite(hfusion::IsFiniteOp op,
                                PatternRewriter &rewriter) const final {
    auto inputType = dyn_cast<RankedTensorType>(op.getInput().getType());
    auto resultType = dyn_cast<RankedTensorType>(op.getResult().getType());
    if (!inputType || !resultType)
      return failure();

    auto elemType = dyn_cast<FloatType>(inputType.getElementType());
    if (!elemType)
      return failure();

    const Location loc = op.getLoc();
    const int64_t rank = inputType.getRank();

    SmallVector<Value> dynamicSizes;
    dynamicSizes.reserve(rank);
    for (int64_t i = 0; i < rank; ++i)
      if (inputType.isDynamicDim(i))
        dynamicSizes.push_back(
            rewriter.create<tensor::DimOp>(loc, op.getInput(), i));

    Value initTensor = rewriter.create<tensor::EmptyOp>(
        loc, resultType.getShape(), resultType.getElementType(), dynamicSizes);

    SmallVector<AffineMap> indexingMaps(
        2, rewriter.getMultiDimIdentityMap(rank));
    SmallVector<utils::IteratorType> iteratorTypes(
        rank, utils::IteratorType::parallel);

    auto genericOp = rewriter.create<linalg::GenericOp>(
        loc, resultType, ValueRange{op.getInput()}, ValueRange{initTensor},
        indexingMaps, iteratorTypes,
        [&](OpBuilder &b, Location nestedLoc, ValueRange args) {
          Value val = args[0];
          Value absVal = b.create<math::AbsFOp>(nestedLoc, val);
          llvm::APFloat inf = llvm::APFloat::getInf(elemType.getFloatSemantics());
          Value infConst = b.create<arith::ConstantOp>(
              nestedLoc, b.getFloatAttr(elemType, inf));
          Value isFinite = b.create<arith::CmpFOp>(
              nestedLoc, arith::CmpFPredicate::OLT, absVal, infConst);
          b.create<linalg::YieldOp>(nestedLoc, isFinite);
        });

    rewriter.replaceOp(op, genericOp.getResult(0));
    return success();
  }
};

struct ConvertHFusionToUpstream
    : public impl::ExecutionEngineHFusionToUpstreamConversionBase<
          ConvertHFusionToUpstream> {
  void runOnOperation() final {
    MLIRContext &ctx = getContext();

    RewritePatternSet patterns(&ctx);
    patterns.add<RewriteHFusionIsFiniteOp>(&ctx);

    ConversionTarget target(ctx);
    target.addIllegalOp<hfusion::IsFiniteOp>();
    target.addLegalDialect<arith::ArithDialect, linalg::LinalgDialect,
                           math::MathDialect, tensor::TensorDialect>();
    target.markUnknownOpDynamicallyLegal([](Operation *) { return true; });

    if (failed(applyPartialConversion(getOperation(), target,
                                      std::move(patterns))))
      signalPassFailure();
  }
};
} // namespace

std::unique_ptr<Pass> mlir::execution_engine::createConvertHFusionToUpstreamPass() {
  return std::make_unique<ConvertHFusionToUpstream>();
}

