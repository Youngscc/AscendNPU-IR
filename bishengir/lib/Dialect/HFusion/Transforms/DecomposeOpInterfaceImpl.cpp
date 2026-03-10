//===- BufferizableOpInterfaceImpl.cpp - Impl. of BufferizableOpInterface -===//
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

#include "bishengir/Dialect/HFusion/Transforms/DecomposeOpInterfaceImpl.h"
#include "bishengir/Dialect/HFusion/IR/HFusion.h"
#include "mlir/Dialect/Linalg/IR/Linalg.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/Operation.h"
// 在现有 #include 语句附近添加
#include "mlir/Dialect/Utils/StructuredOpsUtils.h" // 为了使用 utils::IteratorType
// 添加以下两个头文件
#include "mlir/Dialect/Arith/IR/Arith.h"  // 为了使用 arith::ConstantFloatOp, arith::CmpFOp 等
#include "llvm/ADT/APFloat.h"             // 为了完整定义 fltSemantics 结构

using namespace mlir;
using namespace hfusion;

namespace {


struct TransposeDecomposeInterface
    : public bishengir::BiShengIRAggregatedOpInterface::ExternalModel<
          TransposeDecomposeInterface, linalg::TransposeOp> {
  bool needDecompose(ArrayRef<int64_t> arr) const {
    int mismatch = 0;
    for (int i = 0; i < static_cast<int>(arr.size()); ++i) {
      if (arr[i] != i)
        mismatch++;
    }
    return (mismatch > 2);
  }

  void calculateMinSwaps(ArrayRef<int64_t> perm,
                         SmallVector<SmallVector<int64_t, 2>> &swaps) const {
    int64_t N = static_cast<int64_t>(perm.size());
    SmallVector<std::pair<int64_t, int64_t>, 8> permIndexVec(N);
    for (int64_t i = 0; i < N; i++)
      permIndexVec[i] = {perm[i], i};

    std::sort(permIndexVec.begin(), permIndexVec.end());

    for (int64_t i = 0; i < N; i++) {
      if (permIndexVec[i].second == i)
        continue;
      swaps.push_back({i, permIndexVec[i].second});
      std::swap(permIndexVec[i], permIndexVec[permIndexVec[i].second]);
      i--;
    }
  }

  Value decomposeTransposeOp(linalg::TransposeOp op, OpBuilder &rewriter,
                             SmallVector<SmallVector<int64_t, 2>> swaps,
                             int64_t length) const {
    Value inputVal = op.getInput();
    auto inputTensor = dyn_cast<TensorType>(inputVal.getType());
    Type targetElemType = getElementTypeOrSelf(inputTensor);

    for (auto &swap : swaps) {
      auto curInputStaticShape =
          cast<ShapedType>(inputVal.getType()).getShape();
      SmallVector<int64_t> curOutputStaticShape;
      SmallVector<Value> curOutputDynamicShape;

      int64_t idx1 = swap[0], idx2 = swap[1];
      SmallVector<int64_t> tempPerm;
      // populate intermediate permutations and shapes
      for (int64_t i = 0; i < length; ++i) {
        int64_t targetIdx;
        if (i == idx1) {
          targetIdx = idx2;
        } else if (i == idx2) {
          targetIdx = idx1;
        } else {
          targetIdx = i;
        }

        tempPerm.push_back(targetIdx);
        if (curInputStaticShape[targetIdx] == ShapedType::kDynamic) {
          Operation *dynDimOp =
              rewriter.create<tensor::DimOp>(op->getLoc(), inputVal, targetIdx);
          curOutputStaticShape.push_back(ShapedType::kDynamic);
          curOutputDynamicShape.push_back(dynDimOp->getResults()[0]);
        } else {
          curOutputStaticShape.push_back(curInputStaticShape[targetIdx]);
        }
      }
      // create empty tensor holding the intermediate result shape
      Value emptyTensor = rewriter.create<tensor::EmptyOp>(
          op.getLoc(), curOutputStaticShape, targetElemType,
          curOutputDynamicShape);
      auto intermediateTransposeOp = rewriter.create<linalg::TransposeOp>(
          op->getLoc(), inputVal, emptyTensor, tempPerm);
      inputVal = intermediateTransposeOp->getResult(0);
    }

    return inputVal;
  }

  FailureOr<SmallVector<Value>> decomposeOperation(Operation *op,
                                                   OpBuilder &rewriter) const {
    auto transposeOp = llvm::dyn_cast<mlir::linalg::TransposeOp>(op);
    if (!transposeOp)
      return failure();

    auto perm = transposeOp.getPermutation();
    // skip binary transpose
    if (!needDecompose(perm))
      return failure();

    // the order of swaps to be proceeded
    SmallVector<SmallVector<int64_t, 2>> swaps;
    calculateMinSwaps(perm, swaps);

    // Create tensor.empty and decomposed linalg.transpose Ops
    return SmallVector<Value>{
        decomposeTransposeOp(transposeOp, rewriter, swaps, perm.size())};
  }

  bishengir::DecomposePhase getDecomposePhase(Operation *op) const {
    return bishengir::DecomposePhase::AFTER_HFUSION_FLATTEN;
  }
};



// ========== 添加你的 IsInfOp 分解接口实现 ==========
struct IsInfDecomposeInterface
    : public bishengir::BiShengIRAggregatedOpInterface::ExternalModel<
          IsInfDecomposeInterface, hfusion::IsInfOp> {

  //负责将 IsInfOp 分解成更小的操作
  FailureOr<SmallVector<Value>> decomposeOperation(Operation *op,
                                                   OpBuilder &rewriter) const {

    // 添加调试
    llvm::errs() << "=== IsInfDecomposeInterface::decomposeOperation 被调用 ===\n";
    llvm::errs() << "操作名称: " << op->getName() << "\n";
    //llvm::errs() << "操作类型: " << op->getTypeName() << "\n";
    llvm::errs() << "操作类型: " << op->getName().getStringRef() << "\n";


    //获取和验证输入
    // 1. 将 Operation* 转换为具体的 IsInfOp 类型
    auto isInfOp = llvm::dyn_cast<mlir::hfusion::IsInfOp>(op);
    //确认它确实是 IsInfOp，如果不是 IsInfOp，返回 failure()
    if (!isInfOp)
      return failure();

    // 获取位置信息：loc 是操作的位置，用于调试和错误报告
    Location loc = op->getLoc();
    //获取输入值：input 是 IsInfOp 的输入张量
    Value input = isInfOp.getInput();
    
    //检查输入类型：确保输入是 IsInfOp 的定义中要求输入的 RankedTensorType类型（有明确形状信息的张量）
    // 2. 获取输入张量的元素类型和形状
    auto inputType = dyn_cast<RankedTensorType>(input.getType());  // 或 input.getType().dyn_cast<...>()
    if (!inputType)
      return failure();
    
    Type elementType = inputType.getElementType();
    if (!isa<FloatType>(elementType)) {  // 或 !elementType.isa<...>()
      return failure();
    }
    
    // 确保是浮点类型
    auto floatType = cast<FloatType>(elementType);  // 或 elementType.cast<...>()
    
    // 3. 创建正无穷和负无穷常量
    // 直接使用 getFloatSemantics() 的返回值，不存储到变量中
    APFloat posInf = APFloat::getInf(floatType.getFloatSemantics());
    APFloat negInf = APFloat::getInf(floatType.getFloatSemantics(), /*negative*/ true);
    
    // 创建MLIR常量操作
    Value posInfConst = rewriter.create<arith::ConstantFloatOp>(
        loc, posInf, floatType);
    Value negInfConst = rewriter.create<arith::ConstantFloatOp>(
        loc, negInf, floatType);
    
    // 4. 创建输出张量（布尔类型）
    //从 IsInfOp 中获取输出张量的类型
    auto resultType = isInfOp.getOutput().getType().cast<RankedTensorType>();
    Value initTensor = rewriter.create<tensor::EmptyOp>(
        loc, resultType.getShape(), rewriter.getI1Type());
    
// 5. 准备 linalg.generic 的参数
unsigned rank = inputType.getRank();
SmallVector<AffineMap> indexingMaps = {
    rewriter.getMultiDimIdentityMap(rank),  // 输入映射
    rewriter.getMultiDimIdentityMap(rank)   // 输出映射
};

// 使用正确的类型：utils::IteratorType
SmallVector<utils::IteratorType> iteratorTypes(rank, 
    utils::IteratorType::parallel);

// 6. 创建 linalg.generic 操作，对每个元素执行判断
// 注意：添加了 doc 和 library_call 两个必需的参数
auto genericOp = rewriter.create<linalg::GenericOp>(
    loc,                                        // 位置
    resultType,                                 // 输出类型
    ValueRange{input},                          // 输入值
    ValueRange{initTensor},                     // 输出初始化值
    indexingMaps,                               // 索引映射
    iteratorTypes,                              // 迭代器类型
    /*doc=*/"",                                 // ✅ 正确：空字符串
    /*library_call=*/"",                        // ✅ 正确：空字符串
    [&](OpBuilder &b, Location nestedLoc, ValueRange args) {
      Value inputElem = args[0];
      
      // 检查是否为正无穷
      Value isPosInf = b.create<arith::CmpFOp>(
          nestedLoc, arith::CmpFPredicate::OEQ, inputElem, posInfConst);
      
      // 检查是否为负无穷
      Value isNegInf = b.create<arith::CmpFOp>(
          nestedLoc, arith::CmpFPredicate::OEQ, inputElem, negInfConst);
      
      // 逻辑或操作
      Value isInf = b.create<arith::OrIOp>(nestedLoc, isPosInf, isNegInf);
      
      b.create<linalg::YieldOp>(nestedLoc, isInf);
    });
    
    // 7. 返回分解后的结果
    return SmallVector<Value>{genericOp.getResult(0)};
  }

  bishengir::DecomposePhase getDecomposePhase(Operation *op) const {
    // 返回这个操作应该在哪个阶段被分解，参考 transpose 的 AFTER_HFUSION_FLATTEN
    //return bishengir::DecomposePhase::AFTER_HFUSION_FLATTEN;
    // 改为 no-constraint 试试
    //return bishengir::DecomposePhase::no-constraint;
    return bishengir::DecomposePhase::NO_CONSTRAINT;
  }
};



} // namespace

void mlir::hfusion::registerDecomposeInterfaceExternalModels(
    DialectRegistry &registry) {
  registry.addExtension(+[](MLIRContext *ctx, linalg::LinalgDialect *dialect) {
    linalg::TransposeOp::attachInterface<TransposeDecomposeInterface>(*ctx);
  });

  // // ========== 添加下面这行，为 IsInfOp 注册接口 ==========
  // registry.addExtension(+[](MLIRContext *ctx, hfusion::HFusionDialect *dialect) {
  //   hfusion::IsInfOp::attachInterface<IsInfDecomposeInterface>(*ctx);


  registry.addExtension(+[](MLIRContext *ctx, hfusion::HFusionDialect *dialect) {
    hfusion::IsInfOp::attachInterface<IsInfDecomposeInterface>(*ctx);

  });

}
