//===- SSBufferToLegacyLLVM.cpp - Lower SSBuffer for legacy hivmc --------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "bishengir/Conversion/SSBufferToLegacyLLVM/SSBufferToLegacyLLVM.h"

#include "bishengir/Dialect/Annotation/IR/Annotation.h"
#include "bishengir/Dialect/HIVM/IR/HIVM.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Visitors.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/SmallVector.h"

namespace mlir {
#define GEN_PASS_DEF_CONVERTSSBUFFERTOLEGACYLLVM
#include "bishengir/Conversion/Passes.h.inc"
} // namespace mlir

using namespace mlir;

namespace {
constexpr unsigned kLegacySSBufferAddressSpace = 11;
constexpr llvm::StringLiteral kMemrefExtVolatile = "memref_ext.volatile";

bool isSSBufferMemRef(Type type) {
  auto memrefType = dyn_cast<MemRefType>(type);
  if (!memrefType)
    return false;
  auto addressSpace =
      dyn_cast_or_null<hivm::AddressSpaceAttr>(memrefType.getMemorySpace());
  return addressSpace &&
         addressSpace.getAddressSpace() == hivm::AddressSpace::SSBUF;
}

LogicalResult validateSSBufferUse(hivm::PointerCastOp pointerCast) {
  auto memrefType = dyn_cast<MemRefType>(pointerCast.getResult().getType());
  if (!memrefType || memrefType.getRank() != 0)
    return pointerCast.emitOpError(
        "legacy hivmc compatibility lowering requires a rank-0 SSBuffer memref");
  if (pointerCast.getAddrs().size() != 1 ||
      !pointerCast.getDynamicSizes().empty())
    return pointerCast.emitOpError(
        "legacy hivmc compatibility lowering requires one address and no dynamic sizes");

  for (Operation *user : pointerCast.getResult().getUsers()) {
    if (auto load = dyn_cast<memref::LoadOp>(user)) {
      if (load.getMemRef() == pointerCast.getResult() &&
          load.getIndices().empty())
        continue;
    }
    if (auto store = dyn_cast<memref::StoreOp>(user)) {
      if (store.getMemRef() == pointerCast.getResult() &&
          store.getIndices().empty())
        continue;
    }
    return user->emitError(
        "unsupported user of an SSBuffer memref at the legacy hivmc boundary");
  }
  return success();
}

void consumeVolatileMarks(Value oldLoadResult, Value newLoadResult) {
  SmallVector<annotation::MarkOp> volatileMarks;
  for (Operation *user : oldLoadResult.getUsers()) {
    auto mark = dyn_cast<annotation::MarkOp>(user);
    if (mark && mark->hasAttr(kMemrefExtVolatile))
      volatileMarks.push_back(mark);
  }

  oldLoadResult.replaceAllUsesWith(newLoadResult);
  for (annotation::MarkOp mark : volatileMarks) {
    mark->removeAttr(kMemrefExtVolatile);
    if (mark.isAttrEmpty())
      mark.erase();
  }
}

struct ConvertSSBufferToLegacyLLVM
    : public impl::ConvertSSBufferToLegacyLLVMBase<
          ConvertSSBufferToLegacyLLVM> {
  using Base::Base;

  void runOnOperation() override {
    if (failed(convertSSBufferToLegacyLLVM(getOperation())))
      signalPassFailure();
  }
};
} // namespace

LogicalResult mlir::convertSSBufferToLegacyLLVM(ModuleOp module) {
  SmallVector<hivm::PointerCastOp> pointerCasts;
  WalkResult validation = module.walk([&](hivm::PointerCastOp pointerCast) {
    if (!isSSBufferMemRef(pointerCast.getResult().getType()))
      return WalkResult::advance();
    if (failed(validateSSBufferUse(pointerCast)))
      return WalkResult::interrupt();
    pointerCasts.push_back(pointerCast);
    return WalkResult::advance();
  });
  if (validation.wasInterrupted())
    return failure();

  auto legacyPointerType =
      LLVM::LLVMPointerType::get(module.getContext(),
                                 kLegacySSBufferAddressSpace);
  for (hivm::PointerCastOp pointerCast : pointerCasts) {
    OpBuilder builder(pointerCast);
    auto legacyPointer = builder.create<LLVM::IntToPtrOp>(
        pointerCast.getLoc(), legacyPointerType, pointerCast.getSingleAddr());

    SmallVector<Operation *> users(pointerCast.getResult().getUsers());
    for (Operation *user : users) {
      builder.setInsertionPoint(user);
      if (auto load = dyn_cast<memref::LoadOp>(user)) {
        auto legacyLoad = builder.create<LLVM::LoadOp>(
            load.getLoc(), load.getType(), legacyPointer.getResult(),
            /*alignment=*/0, /*isVolatile=*/true);
        consumeVolatileMarks(load.getResult(), legacyLoad.getResult());
        load.erase();
        continue;
      }

      auto store = cast<memref::StoreOp>(user);
      builder.create<LLVM::StoreOp>(
          store.getLoc(), store.getValue(), legacyPointer.getResult(),
          /*alignment=*/0, /*isVolatile=*/true);
      store.erase();
    }
    pointerCast.erase();
  }

  return success();
}

std::unique_ptr<Pass> mlir::createConvertSSBufferToLegacyLLVMPass() {
  return std::make_unique<ConvertSSBufferToLegacyLLVM>();
}
