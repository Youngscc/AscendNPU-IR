"""Minimal compile-only driver for the vector-add VV kernel.
Defines just the @triton.jit kernel and triggers one JIT compile with CPU
tensors, so the tritonsim-hivm dump launcher captures ttadapter/npuir without
running the tutorial's version-drifted __main__ benchmark harness.
"""
import torch
import triton
import triton.language as tl


@triton.jit
def add_kernel(x_ptr, y_ptr, output_ptr, n_elements,
               BLOCK_SIZE: tl.constexpr):
    pid = tl.program_id(axis=0)
    block_start = pid * BLOCK_SIZE
    offsets = block_start + tl.arange(0, BLOCK_SIZE)
    mask = offsets < n_elements
    x = tl.load(x_ptr + offsets, mask=mask)
    y = tl.load(y_ptr + offsets, mask=mask)
    output = x + y
    tl.store(output_ptr + offsets, output, mask=mask)


def main():
    n_elements = 98304          # 96K elements
    BLOCK_SIZE = 8192           # UB footprint driver (elements per program)
    x = torch.rand(n_elements, dtype=torch.float32)
    y = torch.rand(n_elements, dtype=torch.float32)
    out = torch.empty_like(x)
    grid = (triton.cdiv(n_elements, BLOCK_SIZE),)
    add_kernel[grid](x, y, out, n_elements, BLOCK_SIZE=BLOCK_SIZE)
    print("[drv] add_kernel launched (compile-only)")


if __name__ == "__main__":
    main()
