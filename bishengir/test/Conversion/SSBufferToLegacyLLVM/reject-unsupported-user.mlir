// RUN: not bishengir-opt %s --convert-ssbuffer-to-legacy-llvm 2>&1 | FileCheck %s

module {
  func.func private @consume_ssbuffer(
      memref<i32, #hivm.address_space<ssbuf>>)

  func.func @unsupported_ssbuffer_user(%address: i64) {
    %ptr = hivm.hir.pointer_cast(%address)
        : memref<i32, #hivm.address_space<ssbuf>>
    func.call @consume_ssbuffer(%ptr)
        : (memref<i32, #hivm.address_space<ssbuf>>) -> ()
    return
  }
}

// CHECK: error: unsupported user of an SSBuffer memref at the legacy hivmc boundary
