# Extracted Kernel Stages — FlashAttention Sparse Prefill on Ascend 910B3

Each `.py` file is a complete, executable Triton kernel at a specific stage of optimization.
Extracted from the git history of the `autoresearch/apr15` branch.

## Performance Trajectory

```
5800μs  S0  Original baseline
  │
5392μs  S1  BLOCK_V=512                                     (+7.7%)
  │
4356μs  S2  BLOCK_SBS=256 + multibuffer=False               (+33.3%)  ← biggest win
  │
4337μs  S3  enable_mixed_cv=False                           (+0.5%)
  │
4294μs  S4  workspace_sv bf16                               (+1.0%)
  │
4235μs  S5  enable_hivm_auto_cv_balance=True                (+1.4%)
  │
4075μs  S6  tile_mix_cube_loop=4 tile_mix_vector_loop=1     (+3.9%)
  │
3580μs  S7  shared kv_nope SSA                              (+14.0%)  ← breakthrough
  │
3589μs  S8  hoist Q_nope+Q_rope before SBS loop             (-0.25%)  structural
  │
3573μs  S9  enable_code_motion=True                         (+0.45%)  ← final best
```

**Total: 5800μs → 3573μs = 1.62× speedup (38.4% latency reduction)**

## Stage Details

### S0 — Original Baseline (5800μs)
- **File**: `s0_original_baseline_5800us.py`
- **Commit**: `6d84a1e`
- **Config**: `BLOCK_G=16, BLOCK_SBS=128, BLOCK_K=512, BLOCK_V=256`
- **Flags**: `enable_mixed_cv=True, enable_auto_bind_sub_block=True, multibuffer=True`
- **Architecture**: K nope/rope workspace split, QK = Q@K_nope^T + Q@K_rope^T

### S1 — BLOCK_V=512 (5392μs, +7.7%)
- **File**: `s1_exp3_BLOCK_V_512_5392us.py`
- **Commit**: `f3af797`
- **Change**: `BLOCK_V = 256 → 512` (= D_v). Eliminates inner V-dimension loop.
  - blk_v_loop_time drops from 2 → 1
  - Halves C→V domain crossings and O accumulator HBM round-trips

### S2 — BLOCK_SBS=256 + multibuffer=False (4356μs, +33.3%) ← BIGGEST SINGLE WIN
- **File**: `s2_exp4b_BLOCK_SBS_256_multibuffer_False_4356us.py`
- **Commit**: `c94847b`
- **Change**: `BLOCK_SBS = 128 → 256`. Halves SBS iterations (8→4).
  - Fewer domain crossings (score+SV workspace round-trips: 16→8 per head)
  - `multibuffer=False` required: `multibuffer=True` with BLOCK_SBS=256 overflows cbuf (544KB > 512KB)

### S3 — enable_mixed_cv=False (4337μs, +0.5%)
- **File**: `s3_exp6_mixed_cv_False_4337us.py`
- **Commit**: `3a60809`
- **Change**: `enable_mixed_cv=False`. Disables Cube/Vector pipeline mixing.
  - For this workload, pure CV separation schedules better than mixed

### S4 — workspace_sv bf16 (4294μs, +1.0%)
- **File**: `s4_exp10_workspace_sv_bf16_4294us.py`
- **Commit**: `524b391`
- **Change**: `workspace_sv dtype = float32 → bf16`. Halves C→V domain crossing data size.
  - Score+SV workspace: 2×(float32→bf16) halving per SBS iter

### S5 — enable_hivm_auto_cv_balance=True (4235μs, +1.4%)
- **File**: `s5_exp15_hivm_auto_cv_balance_4235us.py`
- **Commit**: `1c07c05`
- **Change**: `enable_hivm_auto_cv_balance=True`. Auto C/V pipeline workload balancing.
  - Compiler decides optimal distribution between Cube and Vector pipelines

### S6 — tile_mix (4075μs, +3.9%)
- **File**: `s6_exp25_tile_mix_cube4_vec1_4075us.py`
- **Commit**: `54b9a7c`
- **Change**: `tile_mix_cube_loop=4, tile_mix_vector_loop=1`. Optimal cube:vector tile ratio.
  - Reduces vector pipeline stalls by matching cube throughput

### S7 — Shared kv_nope SSA (3580μs, +14.0%) ← BREAKTHROUGH
- **File**: `s7_exp27_shared_kv_nope_SSA_3580us.py`
- **Commit**: `048df68`
- **Major code change**: +54 lines. New `cube1_split_preloaded_k()` helper.
  - Loads K_nope **once** inside SBS loop, shares same SSA value for QK (transposed) and SV (direct)
  - Eliminates 1 redundant `nd2nz` per SBS iteration (nd2nz count: 6→5)
  - Also replaces LSE (log-sum-exp) with max+sum tracking:
    - Saves 2 barriers per iteration (no tl.log)
    - Simple max comparison instead of NaN-gated NaN check

### S8 — Hoist Q Loads (3589μs, -0.25%)
- **File**: `s8_exp28_hoist_Q_loads_3589us.py`
- **Commit**: `e44f55f`
- **Major code change**: +43 lines. New `cube1_split_preloaded_qk()` helper.
  - Pre-loads Q_nope and Q_rope **before** the SBS loop (loop-invariant)
  - Single SSA values across all SBS iterations → compiler avoids re-issuing nd2nz
  - Saves 3×Q_nope + 3×Q_rope nd2nz per T1 head iteration
  - Structural win, but Q traffic << K_nope traffic, so net gain small

### S9 — enable_code_motion=True (3573μs, +0.45%) ← FINAL BEST
- **File**: `s9_exp32_enable_code_motion_3573us_FINAL_BEST.py`
- **Commit**: `790181d`
- **Change**: Single flag addition: `enable_code_motion=True`
  - Compiler hoists invariant computations out of loops for ILP
  - Final 0.45% squeeze from compiler optimization

## Hardware Context

- **Chip**: Ascend 910B3 (no VF unit)
- **Mode**: AIC + AIV mix mode (Cube + Vector co-processors)
- **Benchmark**: batch=1, q_len=2048, k_len=1024, head_num=16, kv_lora_rank=512, qk_rope_head_dim=64
- **Memory hierarchy**: UB=192KB, CBUF=512KB, L0A/L0B/L0C=512KB each
- **Bandwidth ceiling**: ~3010μs theoretical minimum; 3573μs actual = 84% efficiency

## Optimization Categories

| Category | Stage | Mechanism | Gain |
|----------|-------|-----------|------|
| Block size tuning | S1, S2 | Reduce loop iterations & domain crossings | +37.9% |
| Data type reduction | S4 | bf16 workspace halves crossing bandwidth | +1.0% |
| Pipeline config | S3, S5 | Mixed CV off, auto CV balance | +1.9% |
| Tile mix ratio | S6 | Cube:Vector = 4:1 ratio | +3.9% |
| SSA optimization | S7 | Eliminate redundant nd2nz transforms | +14.0% |
| Code hoisting | S8 | Load Q once before loop | -0.25% |
| Compiler pass | S9 | Code motion for ILP | +0.45% |

## How to Use

Each `.py` file is self-contained. Run with:

```bash
python sN_<description>.py
```

The kernel function is `prefill_a5_cvpipe`. Compilation flags are set via
`os.environ` before `triton.compile` is called.
