"""Minimal compile-only driver for the fused-softmax VV kernel."""
import torch
import triton
import triton.language as tl


@triton.jit
def softmax_kernel(output_ptr, input_ptr, input_row_stride, output_row_stride,
                   n_rows, n_cols, BLOCK_SIZE: tl.constexpr,
                   num_stages: tl.constexpr):
    row_start = tl.program_id(0)
    row_step = tl.num_programs(0)
    for row_idx in tl.range(row_start, n_rows, row_step, num_stages=num_stages):
        row_start_ptr = input_ptr + row_idx * input_row_stride
        col_offsets = tl.arange(0, BLOCK_SIZE)
        input_ptrs = row_start_ptr + col_offsets
        mask = col_offsets < n_cols
        row = tl.load(input_ptrs, mask=mask, other=-float('inf'))
        row_minus_max = row - tl.max(row, axis=0)
        numerator = tl.exp(row_minus_max)
        denominator = tl.sum(numerator, axis=0)
        softmax_output = numerator / denominator
        output_row_start_ptr = output_ptr + row_idx * output_row_stride
        output_ptrs = output_row_start_ptr + col_offsets
        tl.store(output_ptrs, softmax_output, mask=mask)


def main():
    n_rows, n_cols = 128, 8192
    BLOCK_SIZE = triton.next_power_of_2(n_cols)   # 8192; 32KB f32 UB tiles
    x = torch.rand(n_rows, n_cols, dtype=torch.float32)
    y = torch.empty_like(x)
    grid = (64,)
    softmax_kernel[grid](y, x, x.stride(0), y.stride(0), n_rows, n_cols,
                         BLOCK_SIZE=BLOCK_SIZE, num_stages=2)
    print("[drv] softmax_kernel launched (compile-only)")


if __name__ == "__main__":
    main()
