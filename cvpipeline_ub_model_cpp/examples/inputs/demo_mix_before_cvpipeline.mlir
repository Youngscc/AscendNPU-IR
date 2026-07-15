// End-to-end MIX fixture for the plan-before-cvpipeline demo.
//
// Input boundary: compiler-emitted generic MLIR immediately before
// createCVPipeliningPass.  The function `kernel` is a real MIX function
// (hivm.func_core_type<MIX>) carrying a Cube part (mmadL1) and a Vector part
// (load + vadd) plus a flat scf.for loop.
//
// The projected AIV function `kernel_mix_aiv` exercises the reproducible
// post-CVPipeline stages: SplitMixKernelAIVProjection (split, drops the Cube
// mmadL1), FoldTensorEmpty / CloneTensorEmpty (tensor-empty ownership),
// LoopInvariantCodeMotion (the flat loop's loop-invariant tensor.empty is
// hoisted), and InlineOTFLoadStore (no concat store -> recognized no-op).
// TileAndBindSubBlock runs the same-address-hazard Exact path: the AIV store's
// destination traces through reinterpret_cast to the same block argument as the
// load source, so limitUniqueSubBlockToStore wraps the store in a
// `limit_sub_block_id0` scf.if guard (Exact).  CanonicalizationAfterSplit
// recognizes the marked guard and leaves it (the runtime get_sub_block_idx
// condition is non-foldable), keeping the limit-store path end-to-end exact.
// TileCubeVectorLoop finds no hivm.loop_core_type candidates and is a recognized
// no-op.  No stage reports Incomplete, so the module reports precision=exact.
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}], function_type = (memref<?xf16>) -> (), sym_name = "kernel"}> ({
  ^bb0(%arg0: memref<?xf16>):
    %c0 = "arith.constant"() <{value = 0 : index}> : () -> index
    // Vector load: source is a reinterpret_cast of %arg0.  The result feeds vadd,
    // so ClassifyLoadFromUsers resolves the load to Vector (retained on AIV).
    %load_view = "memref.reinterpret_cast"(%arg0, %c0) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 16, 1>}> : (memref<?xf16>, index) -> memref<16x16xf16, strided<[16, 1], offset: ?>>
    %load_out = "tensor.empty"() : () -> tensor<16x16xf16>
    %loaded = "hivm.hir.load"(%load_view, %load_out) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<16x16xf16, strided<[16, 1], offset: ?>>, tensor<16x16xf16>) -> tensor<16x16xf16>
    // Cube part: dropped from the AIV projection by SplitMixKernel.  mmadL1 is a
    // single-result DPS op (1 result, 1 dps_init), so the projection replaces its
    // result with its accumulate tensor and erases it cleanly.  fixpipe+workspace
    // are intentionally omitted: a result-less DPS Cube op (fixpipe) hits a
    // separate split-stage gap, and a local workspace alloc hits the workspace
    // invariant stage; both are out of scope for this exact demo.
    %true = "arith.constant"() <{value = true}> : () -> i1
    %c16 = "arith.constant"() <{value = 16 : index}> : () -> index
    %lhs = "tensor.empty"() : () -> tensor<16x16xf16>
    %rhs = "tensor.empty"() : () -> tensor<16x16xf16>
    %acc = "tensor.empty"() : () -> tensor<16x16xf32>
    %cube = "hivm.hir.mmadL1"(%lhs, %rhs, %true, %c16, %c16, %c16, %acc) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> : (tensor<16x16xf16>, tensor<16x16xf16>, i1, index, index, index, tensor<16x16xf32>) -> tensor<16x16xf32>
    // Flat scf.for loop (no hivm.loop_core_type): CVPipelining and
    // TileCubeVectorLoop leave it untouched, and LoopInvariantCodeMotion hoists
    // the loop-invariant tensor.empty out of the body (Exact).  No iter args and
    // no subset slice pair -> LoopInvariantSubsetHoisting is a recognized no-op.
    %lb = "arith.constant"() <{value = 0 : index}> : () -> index
    %ub = "arith.constant"() <{value = 4 : index}> : () -> index
    %step = "arith.constant"() <{value = 1 : index}> : () -> index
    "scf.for"(%lb, %ub, %step) ({
    ^bb0(%iv: index):
      %hoist_me = "tensor.empty"() : () -> tensor<16x16xf16>
      "scf.yield"() : () -> ()
    }) : (index, index, index) -> ()
    // The load result is the UB-relevant Vector artifact (its output tensor
    // materializes one UB buffer).  An AIV store exercises sub-block binding on
    // a non-tiling Exact path: the store destination traces through
    // reinterpret_cast to %arg0, the same block argument as the load source, so
    // TileAndBindSubBlock detects the same-address hazard and applies
    // limitUniqueSubBlockToStore (wrapping the store in a `limit_sub_block_id0`
    // scf.if guard).  CanonicalizationAfterSplit recognizes the marked guard and
    // leaves it in place (the runtime get_sub_block_idx condition is non-foldable,
    // Exact), so the limit-store path is end-to-end exact.
    %store_view = "memref.reinterpret_cast"(%arg0, %c0) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 16, 1>}> : (memref<?xf16>, index) -> memref<16x16xf16, strided<[16, 1], offset: ?>>
    "hivm.hir.store"(%loaded, %store_view) <{operandSegmentSizes = array<i32: 2, 0>}> : (tensor<16x16xf16>, memref<16x16xf16, strided<[16, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = -1 : i64, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()
