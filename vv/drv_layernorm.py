"""Minimal compile-only driver for the layer-norm forward VV kernel.
Two reduction loops (mean, var) + normalize loop -> long-lived accumulators,
the richest VV UB profile in the corpus.
"""
import torch
import triton
import triton.language as tl


@triton.jit
def _layer_norm_fwd_fused(X, Y, W, B, Mean, Rstd, stride, N, eps,
                          BLOCK_SIZE: tl.constexpr):
    row = tl.program_id(0)
    Y += row * stride
    X += row * stride
    mean = 0
    _mean = tl.zeros([BLOCK_SIZE], dtype=tl.float32)
    for off in range(0, N, BLOCK_SIZE):
        cols = off + tl.arange(0, BLOCK_SIZE)
        a = tl.load(X + cols, mask=cols < N, other=0.).to(tl.float32)
        _mean += a
    mean = tl.sum(_mean, axis=0) / N
    _var = tl.zeros([BLOCK_SIZE], dtype=tl.float32)
    for off in range(0, N, BLOCK_SIZE):
        cols = off + tl.arange(0, BLOCK_SIZE)
        x = tl.load(X + cols, mask=cols < N, other=0.).to(tl.float32)
        x = tl.where(cols < N, x - mean, 0.)
        _var += x * x
    var = tl.sum(_var, axis=0) / N
    rstd = 1 / tl.sqrt(var + eps)
    tl.store(Mean + row, mean)
    tl.store(Rstd + row, rstd)
    for off in range(0, N, BLOCK_SIZE):
        cols = off + tl.arange(0, BLOCK_SIZE)
        mask = cols < N
        w = tl.load(W + cols, mask=mask)
        b = tl.load(B + cols, mask=mask)
        x = tl.load(X + cols, mask=mask, other=0.).to(tl.float32)
        x_hat = (x - mean) * rstd
        y = x_hat * w + b
        tl.store(Y + cols, y, mask=mask)


def main():
    M, N = 128, 8192
    BLOCK_SIZE = triton.next_power_of_2(N)   # 8192; 32KB f32 tiles
    x = torch.rand(M, N, dtype=torch.float32)
    w = torch.rand(N, dtype=torch.float32)
    b = torch.rand(N, dtype=torch.float32)
    y = torch.empty_like(x)
    mean = torch.empty(M, dtype=torch.float32)
    rstd = torch.empty(M, dtype=torch.float32)
    grid = (M,)
    _layer_norm_fwd_fused[grid](x, y, w, b, mean, rstd, x.stride(0), N, 1e-5,
                                BLOCK_SIZE=BLOCK_SIZE)
    print("[drv] _layer_norm_fwd_fused launched (compile-only)")


if __name__ == "__main__":
    main()
