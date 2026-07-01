"""Optimized sparse flash attention prefill kernel for Ascend910D (A5).

Optimizations applied:
  1. sync_solver=True, inject_barrier_all=False: Uses GraphSyncSolver for
     minimal dependency-based sync instead of barrier-before-every-op.

  2. K nope/rope workspace split: The gathered K workspace is split into
     K_nope[T1, N2, K, D_v=512] and K_rope[T1, N2, K, D_rope=64].
     - QK matmul = Q_nope @ K_nope.T + Q_rope @ K_rope.T (two smaller matmuls)
     - SV matmul uses K_nope directly (same pointer as QK_nope)
     Benefits:
     - K_nope loaded for QK can potentially be reused in L1 for SV
       (no Vector ops touch L1 between QK and SV)
     - 512-stride workspace is power-of-2 aligned (vs 576) for better
       ND2NZ DMA efficiency
     - Separate K_rope buffer (16KB) can be evicted from L1 immediately
       after QK_rope, freeing space for SV loads
     - Total workspace memory unchanged: (512 + 64) = 576 per entry

  3. BLOCK_V=512 (= D_v): Eliminates the inner V-dimension loop.
     - blk_v_loop_time = D_v/BLOCK_V = 1 (was 2 with BLOCK_V=256)
     - Halves C→V domain crossings in SV computation per SBS block iteration
     - Halves O accumulator HBM round-trips (load/store O_ptr per SBS block)
"""
import torch
import triton
import triton.language as tl
import random
import triton.runtime.driver as driver

from einops import rearrange
import os

import numpy as np
import random
def set_seed(seed=42):
    torch.manual_seed(seed)
    if hasattr(torch, 'npu') and torch.npu.is_available():
        torch.npu.manual_seed(seed)
        torch.npu.manual_seed_all(seed)
    np.random.seed(seed)
    random.seed(seed)

set_seed(1234)


@triton.jit
def cube1_split(
    Q_ptr,
    Wksp_KNope_ptr,
    Wksp_KRope_ptr,
    # Strides for Q
    stride_qt1, stride_qn2, stride_qn1, stride_qd,
    # Strides for K nope workspace
    stride_wknt1, stride_wknn2, stride_wknsbs, stride_wknd,
    # Strides for K rope workspace
    stride_wkrt1, stride_wkrn2, stride_wkrsbs, stride_wkrd,
    # Dimensions
    D_v: tl.constexpr,
    D_qk: tl.constexpr,
    BLOCK_G: tl.constexpr,
    BLOCK_SBS: tl.constexpr,
    # Offsets
    offs_G,
    offs_sbs,
    q_base,
    wk_nope_base,
    wk_rope_base,
    # Block sizes
    BLOCK_K: tl.constexpr,
):
    """QK matmul split into nope and rope parts.

    QK = Q_nope @ K_nope.T + Q_rope @ K_rope.T

    Q_nope = Q[:, :D_v],  Q_rope = Q[:, D_v:D_qk]
    K_nope from Wksp_KNope_ptr, K_rope from Wksp_KRope_ptr.
    """
    qk_acc = tl.zeros([BLOCK_G, BLOCK_SBS], dtype=tl.float32)

    # --- Nope part: Q[:, :D_v] @ K_nope.T ---
    nope_loop_times: tl.constexpr = (D_v + BLOCK_K - 1) // BLOCK_K
    for idx_D in range(0, nope_loop_times):
        offs_D = idx_D * BLOCK_K + tl.arange(0, BLOCK_K)
        mask_D = offs_D < D_v

        l1_q = tl.load(
            Q_ptr + (q_base + offs_G[:, None] * stride_qn1 + offs_D[None, :] * stride_qd),
            mask=mask_D[None, :])
        l1_k = tl.load(
            Wksp_KNope_ptr + (wk_nope_base + offs_sbs[:, None] * stride_wknsbs + offs_D[None, :] * stride_wknd),
            mask=mask_D[None, :])

        qk_acc += tl.dot(l1_q, l1_k.trans())

    # --- Rope part: Q[:, D_v:D_qk] @ K_rope.T ---
    D_rope: tl.constexpr = D_qk - D_v
    BLOCK_K_ROPE: tl.constexpr = D_rope
    rope_loop_times: tl.constexpr = (D_rope + BLOCK_K_ROPE - 1) // BLOCK_K_ROPE
    for idx_D in range(0, rope_loop_times):
        offs_D_rope = idx_D * BLOCK_K_ROPE + tl.arange(0, BLOCK_K_ROPE)
        mask_D_rope = offs_D_rope < D_rope

        # Q rope columns start at D_v
        l1_q_rope = tl.load(
            Q_ptr + (q_base + offs_G[:, None] * stride_qn1 + (D_v + offs_D_rope)[None, :] * stride_qd),
            mask=mask_D_rope[None, :])
        l1_k_rope = tl.load(
            Wksp_KRope_ptr + (wk_rope_base + offs_sbs[:, None] * stride_wkrsbs + offs_D_rope[None, :] * stride_wkrd),
            mask=mask_D_rope[None, :])

        qk_acc += tl.dot(l1_q_rope, l1_k_rope.trans())

    return qk_acc


@triton.jit
def _gather(
    dst_ptr,
    src_ptr,
    Indices_ptr,
    # Strides for indices
    stride_it1, stride_in2, stride_ik,
    # Strides for dst
    stride_dt1, stride_dn2, stride_dsbs, stride_dd,
    # Strides for src
    stride_st2, stride_sn2, stride_s1, stride_sd,
    # Params
    D: tl.constexpr,
    K: tl.constexpr,
    BLOCK_D_VEC: tl.constexpr,
    # Meta
    actual_sel_blk,
    dst_base,
    src_base,
    topk_base,
    cur_s2,
):
    for idx_topk in range(0, actual_sel_blk):
        sparse_id = tl.load(Indices_ptr + topk_base + idx_topk * stride_ik)
        if sparse_id >= 0:
            src_t2 = src_base + sparse_id * stride_st2
            dst_sbs = dst_base + idx_topk * stride_dsbs

            D_loop_times: tl.constexpr = (D + BLOCK_D_VEC - 1) // BLOCK_D_VEC
            for idx_sub_Dv in range(0, D_loop_times):
                offs_sub_Dv = idx_sub_Dv * BLOCK_D_VEC + tl.arange(0, BLOCK_D_VEC)
                mask_sub_Dv = offs_sub_Dv < D

                src_D = offs_sub_Dv * stride_sd
                gather_tensor = tl.load(src_ptr + (src_t2 + src_D), mask=mask_sub_Dv)

                dst_D = offs_sub_Dv * stride_dd
                tl.store(dst_ptr + (dst_sbs + dst_D), gather_tensor, mask=mask_sub_Dv)


@triton.jit(do_not_specialize=["T1", "T2", "S1", "S2"])
def sparse_flash_attention_prefill_kernel_cvpipe(
    # Input pointers
    Q_ptr,             # [B, S, H, D_qk] - queries (combined nope + rope)
    K_ptr,             # [B, T, H, D_qk] - keys (combined nope + rope)
    V_ptr,             # [B, T, H, D_v] - values
    Indices_ptr,       # [B, S, K] - topk indices
    O_ptr,
    # Workspace pointers
    Wksp_KNope_ptr,    # [T1, N2, K, D_v] - gathered K nope part
    Wksp_KRope_ptr,    # [T1, N2, K, D_rope] - gathered K rope part
    Wksp_qk_ptr,
    Wksp_score_ptr,
    Wksp_sv_ptr,
    # Strides for Q
    stride_qt1, stride_qn2, stride_qn1, stride_qd,
    # Strides for K
    stride_kt2, stride_kn2, stride_k1, stride_kd,
    # Strides for indices
    stride_it1, stride_in2, stride_ik,
    # Strides for O
    stride_ot1, stride_on1, stride_wod,
    # Strides for K nope workspace
    stride_wknt1, stride_wknn2, stride_wknsbs, stride_wknd,
    # Strides for K rope workspace
    stride_wkrt1, stride_wkrn2, stride_wkrsbs, stride_wkrd,
    # Strides for aux qk
    stride_wqkt1, stride_wqkn1, stride_wqks2,
    # Strides for aux score
    stride_wscoret1, stride_wscoren1, stride_wscores2,
    # Strides for aux sv
    stride_wsvt1, stride_wsvn1, stride_wsvd,
    # Param
    scale,
    ## Basic
    T1,
    N1: tl.constexpr,
    T2,
    G: tl.constexpr,
    D_qk: tl.constexpr,
    D_v: tl.constexpr,
    K: tl.constexpr,
    ## Extend
    B: tl.constexpr,
    S1,
    S2,
    # Proc per core
    bs1_per_core,
    # Block sizes
    BLOCK_G: tl.constexpr,
    BLOCK_SBS: tl.constexpr,
    BLOCK_K: tl.constexpr,
    BLOCK_V: tl.constexpr,
):
    pid = tl.program_id(0)
    t1_start = pid * bs1_per_core
    t1_end = tl.minimum(T1, (pid + 1) * bs1_per_core)

    for idx_t1 in range(t1_start, t1_end):
        idx_b = idx_t1 // S1
        cur_s2 = S2
        beg_s2 = idx_b * S2

        topk_base = idx_t1 * stride_it1
        q_base = idx_t1 * stride_qt1

        wk_nope_base = idx_t1 * stride_wknt1
        wk_rope_base = idx_t1 * stride_wkrt1
        wdq_base = pid * stride_wqkt1
        wdqt_base = pid * stride_wscoret1
        k_base = beg_s2 * stride_kt2

        wsv_base = pid * stride_wsvt1
        wsacc_base = idx_t1 * stride_ot1
        HEAD_LOOP_TIMES: tl.constexpr = triton.cdiv(G, BLOCK_G)
        sbs_loop_times = tl.cdiv(K, BLOCK_SBS)
        for idx_n1_blk in range(0, HEAD_LOOP_TIMES):
            offs_G = idx_n1_blk * BLOCK_G + tl.arange(0, BLOCK_G)
            lse = tl.zeros([BLOCK_G], dtype=tl.float32) - float("inf")

            for idx_sub_sbs in range(0, sbs_loop_times):
                offs_sbs = idx_sub_sbs * BLOCK_SBS + tl.arange(0, BLOCK_SBS)

                # QK matmul (split nope/rope)
                qk_l1 = cube1_split(
                    Q_ptr,
                    Wksp_KNope_ptr,
                    Wksp_KRope_ptr,
                    stride_qt1, stride_qn2, stride_qn1, stride_qd,
                    stride_wknt1, stride_wknn2, stride_wknsbs, stride_wknd,
                    stride_wkrt1, stride_wkrn2, stride_wkrsbs, stride_wkrd,
                    D_v,
                    D_qk,
                    BLOCK_G,
                    BLOCK_SBS,
                    offs_G,
                    offs_sbs,
                    q_base,
                    wk_nope_base,
                    wk_rope_base,
                    BLOCK_K,
                )

                # Softmax + score
                qk = qk_l1
                qk *= scale     #[BLOCK_G, BLOCK_SBS]

                mask = offs_sbs < K
                qk = tl.where(mask[None, :], qk, -float('inf'))
                local_max = tl.max(qk, 1)     #[BLOCK_G]
                local_exp = tl.exp(qk - local_max[:, None])     #[BLOCK_G, BLOCK_SBS]
                local_sum = tl.sum(local_exp, 1)     #[BLOCK_G]
                local_lse = local_max + tl.log(local_sum)     #[BLOCK_G]
                mean_lse = (lse + local_lse) / 2     #[BLOCK_G]
                new_lse = mean_lse + tl.log(tl.exp(lse - mean_lse) + tl.exp(local_lse - mean_lse))     #[BLOCK_G]
                new_lse = tl.where(new_lse != new_lse, local_lse, new_lse)     #[BLOCK_G]
                current_weight = tl.exp(local_lse - new_lse)     #[BLOCK_G]
                score = (local_exp / local_sum[:, None])     #[BLOCK_G, BLOCK_SBS]
                # Score store/load: V->C domain crossing (MayImplicitTransposeWithLastAxis)
                tl.store(Wksp_score_ptr + wdqt_base + tl.arange(0, BLOCK_G)[:, None] * stride_wscoren1 + tl.arange(0, BLOCK_SBS)[None, :] * stride_wscores2, score.to(Q_ptr.dtype.element_ty))

                blk_v_loop_time: tl.constexpr = triton.cdiv(D_v, BLOCK_V)

                score_l1 = tl.load(Wksp_score_ptr + wdqt_base + tl.arange(0, BLOCK_G)[:, None] * stride_wscoren1 + tl.arange(0, BLOCK_SBS)[None, :] * stride_wscores2)
                for idx_blk_v in range(0, blk_v_loop_time):
                    offset_V = idx_blk_v * BLOCK_V + tl.arange(0, BLOCK_V)
                    # SV matmul uses K_nope workspace (same data loaded for QK nope)
                    kv_cache = tl.load(Wksp_KNope_ptr + wk_nope_base + offs_sbs[:, None] * stride_wknsbs + offset_V[None, :] * stride_wknd)
                    sv = tl.dot(score_l1, kv_cache)
                    # SV store/load: C->V domain crossing (MayImplicitTransposeWithLastAxis)
                    tl.store(Wksp_sv_ptr + wsv_base + offs_G[:, None] * stride_wsvn1 + offset_V[None, :] * stride_wsvd, sv)

                    acc = tl.load(O_ptr + wsacc_base + tl.arange(0, BLOCK_G)[:, None] * stride_on1 + offset_V[None, :] * stride_wod)
                    sv_sub = tl.load(Wksp_sv_ptr + wsv_base + tl.arange(0, BLOCK_G)[:, None] * stride_wsvn1 + offset_V[None, :] * stride_wsvd)
                    acc = acc * tl.exp(lse - new_lse)[:, None]

                    acc += sv_sub * current_weight[:, None]

                    tl.store(O_ptr + wsacc_base + tl.arange(0, BLOCK_G)[:, None] * stride_on1 + offset_V[None, :] * stride_wod, acc)

                lse = new_lse


def triton_sparse_flash_attention_prefill(
    q: torch.Tensor,           # [B, S1, N1, D_qk]
    k: torch.Tensor,           # [B, S2, N2, D_qk]
    v: torch.Tensor,           # [B, S2, N2, D_v]
    topk_indices: torch.Tensor, # [B, S1, N2, K]
    query_rope: torch.Tensor,  # [B, S1, N1, D_r]
    key_rope: torch.Tensor,    # [B, S2, N2, D_r]
    scale: float = 1.0,
):
    # BSND -> TND
    B, S1, _, _ = q.shape
    _, S2, _, _  = v.shape

    q, k, v = \
        map(lambda x: rearrange(x, 'b s ... -> (b s) ... '),
            (q, k, v))
    query_rope, key_rope = \
        map(lambda x: rearrange(x, 'b s ... -> (b s) ... '),
            (query_rope, key_rope))

    # Preprocess shape TND (Asc like design)
    _, _, _, K = topk_indices.shape
    T1, N1, D_qk = q.shape
    T2, N2, D_v = v.shape

    assert (D_qk == 512 or D_qk == 128) and q.shape[-1] == k.shape[-1] and D_qk == D_v,\
        "D for query, key, value has to be the same, and value can only be 512 or 128."
    assert N1 <= 128 and N1 in (1, 2, 4, 8, 16, 32, 64, 128),\
        f"N1 should be power of 2, and value should be less or equal 128, but get: {N1}."
    assert N2 == 1, "N2 can only be 1."

    G = N1 // N2

    q = q.reshape(T1, N2, G, D_qk)
    k = k.reshape(T2, N2, 1, D_qk)
    v = v.reshape(T2, N2, 1, D_v)

    # Concat rope if possible
    D_qrope = query_rope.shape[-1]
    D_krope = key_rope.shape[-1]
    assert D_qrope == 64 and D_qrope == D_krope,\
        "D for qrope and vrope has to be the same, and value can only be 64."
    query_rope = query_rope.reshape(T1, N2, G, D_qrope)
    key_rope = key_rope.reshape(T2, N2, 1, D_krope)
    q = torch.cat([q, query_rope], dim=-1)
    k = torch.cat([k, key_rope], dim=-1)
    D_qk += query_rope.shape[-1]
    D_rope = D_qrope  # = 64

    BLOCK_G = 16
    BLOCK_SBS = 128
    BLOCK_K = D_v  # = 512
    BLOCK_V = 512  # = D_v: eliminates inner V-loop, halves C→V crossings

    if os.getenv("TRITON_COMPILE_ONLY") == '1' or os.getenv("TRITON_DISABLE_LINE_INFO") == '0':
        core_num = 20  # default for compile-only mode
    else:
        core_num = driver.active.utils.get_device_properties("npu")["num_aicore"]
    workspace_qk = q.new_zeros(size=(core_num, BLOCK_G, BLOCK_SBS), dtype=torch.float32)
    workspace_score = q.new_zeros(size=(core_num, BLOCK_G, BLOCK_SBS))
    workspace_sv = q.new_zeros(size=(core_num, N1, BLOCK_V), dtype=torch.float32)
    o = q.new_zeros(size=(T1, N1, D_v), dtype=torch.float32)

    if T1 <= core_num:
        prog_num = T1
        t1_per_prog = 1
    else:
        prog_num = core_num
        t1_per_prog = triton.cdiv(T1, core_num)

    if os.getenv("TRITON_COMPILE_ONLY") == '1' or os.getenv("TRITON_DISABLE_LINE_INFO") == '0':
        grid = (1,)
    else:
        grid = (prog_num,)

    # Gather K and split into nope/rope workspaces
    _compile_only = os.getenv("TRITON_COMPILE_ONLY") == '1' or os.getenv("TRITON_DISABLE_LINE_INFO") == '0'
    if _compile_only:
        idx_expand = topk_indices.cpu().reshape(B, S1, K, 1).to(torch.int64)
        k_host = k.cpu().reshape(B, 1, S2, D_qk).expand(-1, S1, -1, -1)
        key_full = k_host.gather(2, idx_expand.expand(-1, -1, -1, D_qk)).reshape(T1, N2, K, D_qk).contiguous()
    else:
        topk_long = topk_indices.squeeze(2).to(torch.int64)
        batch_idx = torch.arange(B, device=k.device, dtype=torch.int64)[:, None, None]
        k_view = k.reshape(B, S2, D_qk)
        key_full = k_view[batch_idx, topk_long].contiguous().reshape(T1, N2, K, D_qk)

    key_nope = key_full[..., :D_v].contiguous()       # [T1, N2, K, D_v=512]
    key_rope_ws = key_full[..., D_v:].contiguous()    # [T1, N2, K, D_rope=64]

    sparse_flash_attention_prefill_kernel_cvpipe[grid](
        # Input pointers
        q, k, v, topk_indices,
        o,
        # Workspace: split K workspaces
        key_nope,       # Wksp_KNope_ptr
        key_rope_ws,    # Wksp_KRope_ptr
        workspace_qk,
        workspace_score,
        workspace_sv,
        # Q strides
        q.stride(0), q.stride(1), q.stride(2), q.stride(3),
        # K strides
        k.stride(0), k.stride(1), k.stride(2), k.stride(3),
        # Indices strides
        topk_indices.stride(0), topk_indices.stride(1), topk_indices.stride(2),
        # O strides
        o.stride(0), o.stride(1), o.stride(2),
        # K nope workspace strides
        key_nope.stride(0), key_nope.stride(1), key_nope.stride(2), key_nope.stride(3),
        # K rope workspace strides
        key_rope_ws.stride(0), key_rope_ws.stride(1), key_rope_ws.stride(2), key_rope_ws.stride(3),
        # Aux QK strides
        workspace_qk.stride(0), workspace_qk.stride(1), workspace_qk.stride(2),
        # Aux Score strides
        workspace_score.stride(0), workspace_score.stride(1), workspace_score.stride(2),
        # Aux SV strides
        workspace_sv.stride(0), workspace_sv.stride(1), workspace_sv.stride(2),
        # Params
        scale,
        # Dimensions
        ## Basic
        T1,
        N1,
        T2,
        G,
        D_qk,
        D_v,
        K,
        ## Extend
        B,
        S1,
        S2,
        # Proc per prog
        t1_per_prog,
        # Block sizes
        BLOCK_G=BLOCK_G,
        BLOCK_SBS=BLOCK_SBS,
        BLOCK_K=BLOCK_K,
        BLOCK_V=BLOCK_V,
        enable_mixed_cv=True,  # EXP2_2: Re-enable mixed CV for better pipeline overlap
        enable_auto_bind_sub_block=False,  # EXP2_2: Disable for manual control
        multibuffer=True,  # EXP2_2: Re-enable double buffering
        inject_barrier_all=False,  # EXP2_2: Use sync solver (best from Phase 1)
    )

    return o.to(q.dtype).reshape(B, S1, N1, D_v)


def pytorch_dsa_prefill(
    query: torch.Tensor,
    key: torch.Tensor,
    value: torch.Tensor,
    topk_indices: torch.Tensor,
    query_rope: torch.Tensor,
    key_rope: torch.Tensor,
    scale
) -> torch.Tensor:
    bsz, seqlen_q, n_heads, kv_lora_rank = query.shape
    _, seqlen_k, _, qk_rope_head_dim = key_rope.shape
    q = torch.cat([query, query_rope], -1)
    k = torch.cat([key, key_rope], -1)
    v = key.clone()
    scores = torch.einsum("bshc,btc->bsht", q.to(torch.float32), k.to(torch.float32).squeeze(2)) * scale
    index_mask = torch.full((bsz, seqlen_q, seqlen_k), float("-inf"), device=query.device).scatter_(-1, topk_indices.to(torch.int64).squeeze(2), 0).unsqueeze(2)
    scores += index_mask
    scores = scores.softmax(dim=-1).to(q.dtype)
    o = torch.einsum("bsht,btc->bshc", scores, v.to(q.dtype).squeeze(2)).to(q.dtype)
    return o


def test_dsa_prefill(batch, q_seq_len, k_seq_len, head_num, kv_lora_rank, qk_rope_head_dim, dtype):
    compile_only = os.getenv("TRITON_COMPILE_ONLY") == '1' or os.getenv("TRITON_DISABLE_LINE_INFO") == '0'
    if compile_only:
        device = "cpu"
    else:
        device = "npu"
    sparse_block_count = 1024
    scale_value = 0.041666666666666664
    query = torch.randn(
        batch,
        q_seq_len,
        head_num,
        kv_lora_rank,
        device=device,
        dtype=dtype,
    )
    query_rope = torch.randn(
        batch,
        q_seq_len,
        head_num,
        qk_rope_head_dim,
        device=device,
        dtype=dtype,
    )
    key = torch.randn(
        batch,
        k_seq_len,
        1,
        kv_lora_rank,
        device=device,
        dtype=dtype,
    )
    key_rope = torch.randn(
        batch,
        k_seq_len,
        1,
        qk_rope_head_dim,
        device=device,
        dtype=dtype,
    )
    value = key
    idxs = random.sample(range(k_seq_len), sparse_block_count)
    topk_indices = torch.tensor([idxs for _ in range(batch * q_seq_len * 1)], device=device).reshape(batch, q_seq_len, 1, sparse_block_count). \
        to(torch.int32)

    if compile_only:
        triton_out = triton_sparse_flash_attention_prefill(query, key, value, topk_indices, query_rope, key_rope, scale_value)
    else:
        triton_out = triton_sparse_flash_attention_prefill(query.npu(), key.npu(), value.npu(), topk_indices.npu(), query_rope.npu(), key_rope.npu(), scale_value)
    if not compile_only:
        pytorch_out = pytorch_dsa_prefill(query, key, value, topk_indices, query_rope, key_rope, scale_value)
        golden = pytorch_dsa_prefill(
            query.to(torch.float32).cpu(),
            key.to(torch.float32).cpu(),
            value.to(torch.float32).cpu(),
            topk_indices.cpu(),
            query_rope.to(torch.float32).cpu(),
            key_rope.to(torch.float32).cpu(),
            scale_value
        )
        print("pytorch_out -------------->", pytorch_out)
        print("triton_out -------------->", triton_out)

        print(torch.allclose(triton_out.to(torch.float32), pytorch_out.npu().to(torch.float32), atol=1e-2, rtol=1e-2))
        assert torch.allclose(triton_out.to(torch.float32), pytorch_out.npu().to(torch.float32), atol=1e-2, rtol=1e-2)


if __name__ == "__main__":
    test_dsa_prefill(1, 2048, 1024, 16, 512, 64, torch.bfloat16)
