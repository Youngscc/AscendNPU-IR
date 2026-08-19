//===- SSBufferToLegacyLLVM.h - Lower SSBuffer for legacy hivmc -*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef BISHENGIR_CONVERSION_SSBUFFERTOLEGACYLLVM_SSBUFFERTOLEGACYLLVM_H_
#define BISHENGIR_CONVERSION_SSBUFFERTOLEGACYLLVM_SSBUFFERTOLEGACYLLVM_H_

#include <memory>

#include "mlir/Support/LogicalResult.h"

namespace mlir {
class ModuleOp;
class Pass;

#define GEN_PASS_DECL_CONVERTSSBUFFERTOLEGACYLLVM
#include "bishengir/Conversion/Passes.h.inc"

/// Lower rank-0 SSBuffer memrefs to volatile accesses through !llvm.ptr<11>.
/// This compatibility boundary is intended to run only after all native HIVM
/// optimization and memory-planning passes have completed.
LogicalResult convertSSBufferToLegacyLLVM(ModuleOp module);

std::unique_ptr<Pass> createConvertSSBufferToLegacyLLVMPass();

} // namespace mlir

#endif // BISHENGIR_CONVERSION_SSBUFFERTOLEGACYLLVM_SSBUFFERTOLEGACYLLVM_H_
