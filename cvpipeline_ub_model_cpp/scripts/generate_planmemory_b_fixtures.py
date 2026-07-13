#!/usr/bin/env python3
"""Generate named PlanMemory-before fixtures for B-stage exact validation."""

from __future__ import annotations

import csv
import re
from pathlib import Path


AIV_ATTR = "hivm.func_core_type = #hivm.func_core_type<AIV>"
SOURCE_CASES = {
    "b_control_while_scalar_live": "test_scfwhileop",
    "b_control_while_ub_loop_carried": "test_scfwhileop_yield_inplace",
    "b_control_while_do_while_ub": "test_dowhile_yield_inplace",
    "b_control_cf_diamond_single_arg": "test_branchop",
    "b_control_cf_diamond_multi_arg": "test_branchop_inplace",
    "b_control_scope_nested_iter_copy": "test_scope_copy_arg_used_after_yield_init",
    "b_control_preload_expand_lifetime": "test_preload_buffer_expand_lifetime",
    "b_alias_select_conditional": "test_select_cond_alias_not_inplace",
    "b_alias_multi_if_separate_results": "test_multi_if_yield_not_alias",
    "b_alias_if_three_results": "test_if_multi_yield_not_alias",
    "b_liveness_nested_if_in_for": "test_dead_after_op_nested_if_in_for",
    "b_alias_subview_offset_not_inplace": "test_vabs_offset_unequal_not_inplace",
    "b_planner_multi_buffer_no_inplace": "test_multi_buffer_loadstore_not_inplace",
    "b_planner_multi_buffer_level0_inplace": "test_multi_buffer_loadstore_level0_inplace",
    "b_planner_basic_reuse_chain": "test_mem_allocate_basic",
    "b_planner_zero_temp_buffer": "test_mem_noreuse_max",
    "b_planner_specalloc_overlap": "test_mem_specalloc_max",
    "b_control_if_yield_single_result": "test_infer_plan_memory_if_yield",
    "b_control_if_else_no_result": "test_if_else",
    "b_control_for_iter_if_use": "test_iter_arg_use_in_if_yield_outside",
    "b_control_for_result_alias": "test_plan_memory_for_result",
    "b_alias_subview_in_loop_multi": "test_plan_memory_subview_in_loop",
    "b_planner_multi_buffer_count4": "test_mem_plan_multi_address",
    "b_planner_multi_buffer_single_to_four": "test_inplace_single_and_mb",
    "b_planner_multi_buffer_four_inplace": "test_mem_inplace_mb_and_mb",
}


def strip_test_comments(module: str) -> str:
    lines = []
    for line in module.splitlines():
        if re.match(r"^\s*//", line):
            continue
        lines.append(line.rstrip())
    return "\n".join(lines).strip() + "\n"


def add_aiv_attribute(module: str, function: str) -> str:
    position = module.find(f"func.func @{function}")
    if position < 0:
        raise RuntimeError(f"function not found while adding AIV attr: {function}")
    if AIV_ATTR in module[position:position + 2000]:
        return module
    open_paren = module.find("(", position)
    depth = 0
    close_paren = -1
    for index in range(open_paren, len(module)):
        if module[index] == "(":
            depth += 1
        elif module[index] == ")":
            depth -= 1
            if depth == 0:
                close_paren = index
                break
    if close_paren < 0:
        raise RuntimeError(f"unterminated signature: {function}")
    body_brace = module.find("{", close_paren)
    if body_brace < 0:
        raise RuntimeError(f"function body not found: {function}")
    between = module[close_paren + 1:body_brace]
    if "attributes" in between:
        raise RuntimeError(f"unexpected existing attributes without AIV: {function}")
    return (module[:body_brace] + f"attributes {{{AIV_ATTR}}} " +
            module[body_brace:])


def extract_source_cases(source: Path) -> dict[str, str]:
    text = source.read_text(encoding="utf-8")
    modules = re.split(r"^\s*// -----\s*$", text, flags=re.MULTILINE)
    result = {}
    for output_name, function in SOURCE_CASES.items():
        matches = [module for module in modules
                   if re.search(rf"func\.func\s+@{re.escape(function)}\s*\(",
                                module)]
        if len(matches) != 1:
            raise RuntimeError(f"expected one source module for {function}, got {len(matches)}")
        module = strip_test_comments(matches[0])
        module = add_aiv_attribute(module, function)
        result[output_name] = module
    return result


def module(function: str, arguments: str, body: str) -> str:
    return (
        "module {\n"
        f"  func.func @{function}({arguments}) attributes {{{AIV_ATTR}}} {{\n"
        f"{body.rstrip()}\n"
        "    return\n"
        "  }\n"
        "}\n"
    )


def custom_cases() -> dict[str, tuple[str, str]]:
    gm_i8 = "#hivm.address_space<gm>"
    ub_i8 = "#hivm.address_space<ub>"
    return {
        "b_boundary_align_31_32_33_bytes": (
            "exact",
            module(
                "b_boundary_align_31_32_33_bytes",
                f"%src31: memref<31xi8, {gm_i8}>, %dst31: memref<31xi8, {gm_i8}>, "
                f"%src32: memref<32xi8, {gm_i8}>, %dst32: memref<32xi8, {gm_i8}>, "
                f"%src33: memref<33xi8, {gm_i8}>, %dst33: memref<33xi8, {gm_i8}>",
                f"""    %a31 = memref.alloc() : memref<31xi8, {ub_i8}>
    %a32 = memref.alloc() : memref<32xi8, {ub_i8}>
    %a33 = memref.alloc() : memref<33xi8, {ub_i8}>
    hivm.hir.load ins(%src31 : memref<31xi8, {gm_i8}>) outs(%a31 : memref<31xi8, {ub_i8}>)
    hivm.hir.load ins(%src32 : memref<32xi8, {gm_i8}>) outs(%a32 : memref<32xi8, {ub_i8}>)
    hivm.hir.load ins(%src33 : memref<33xi8, {gm_i8}>) outs(%a33 : memref<33xi8, {ub_i8}>)
    hivm.hir.store ins(%a31 : memref<31xi8, {ub_i8}>) outs(%dst31 : memref<31xi8, {gm_i8}>)
    hivm.hir.store ins(%a32 : memref<32xi8, {ub_i8}>) outs(%dst32 : memref<32xi8, {gm_i8}>)
    hivm.hir.store ins(%a33 : memref<33xi8, {ub_i8}>) outs(%dst33 : memref<33xi8, {gm_i8}>)""",
            ),
        ),
        "b_boundary_capacity_exact_192kib": (
            "exact",
            module(
                "b_boundary_capacity_exact_192kib",
                f"%src: memref<196608xi8, {gm_i8}>, %dst: memref<196608xi8, {gm_i8}>",
                f"""    %alloc = memref.alloc() : memref<196608xi8, {ub_i8}>
    hivm.hir.load ins(%src : memref<196608xi8, {gm_i8}>) outs(%alloc : memref<196608xi8, {ub_i8}>)
    hivm.hir.store ins(%alloc : memref<196608xi8, {ub_i8}>) outs(%dst : memref<196608xi8, {gm_i8}>)""",
            ),
        ),
        "b_boundary_capacity_over_by_one_byte": (
            "exact",
            module(
                "b_boundary_capacity_over_by_one_byte",
                f"%src: memref<196609xi8, {gm_i8}>, %dst: memref<196609xi8, {gm_i8}>",
                f"""    %alloc = memref.alloc() : memref<196609xi8, {ub_i8}>
    hivm.hir.load ins(%src : memref<196609xi8, {gm_i8}>) outs(%alloc : memref<196609xi8, {ub_i8}>)
    hivm.hir.store ins(%alloc : memref<196609xi8, {ub_i8}>) outs(%dst : memref<196609xi8, {gm_i8}>)""",
            ),
        ),
        "b_type_rank0_i32": (
            "exact",
            module(
                "b_type_rank0_i32",
                "%src: memref<i32, #hivm.address_space<gm>>, "
                "%dst: memref<i32, #hivm.address_space<gm>>",
                """    %alloc = memref.alloc() : memref<i32, #hivm.address_space<ub>>
    hivm.hir.load ins(%src : memref<i32, #hivm.address_space<gm>>) outs(%alloc : memref<i32, #hivm.address_space<ub>>)
    hivm.hir.store ins(%alloc : memref<i32, #hivm.address_space<ub>>) outs(%dst : memref<i32, #hivm.address_space<gm>>)""",
            ),
        ),
        "b_liveness_five_live_buffers": (
            "exact",
            module(
                "b_liveness_five_live_buffers",
                "%src: memref<32xf32, #hivm.address_space<gm>>, "
                "%dst: memref<32xf32, #hivm.address_space<gm>>",
                """    %a0 = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
    %a1 = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
    %a2 = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
    %a3 = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
    %a4 = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
    hivm.hir.load ins(%src : memref<32xf32, #hivm.address_space<gm>>) outs(%a0 : memref<32xf32, #hivm.address_space<ub>>)
    hivm.hir.load ins(%src : memref<32xf32, #hivm.address_space<gm>>) outs(%a1 : memref<32xf32, #hivm.address_space<ub>>)
    hivm.hir.load ins(%src : memref<32xf32, #hivm.address_space<gm>>) outs(%a2 : memref<32xf32, #hivm.address_space<ub>>)
    hivm.hir.load ins(%src : memref<32xf32, #hivm.address_space<gm>>) outs(%a3 : memref<32xf32, #hivm.address_space<ub>>)
    hivm.hir.load ins(%src : memref<32xf32, #hivm.address_space<gm>>) outs(%a4 : memref<32xf32, #hivm.address_space<ub>>)
    %r0 = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
    %r1 = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
    %r2 = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
    %r3 = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
    hivm.hir.vadd ins(%a0, %a1 : memref<32xf32, #hivm.address_space<ub>>, memref<32xf32, #hivm.address_space<ub>>) outs(%r0 : memref<32xf32, #hivm.address_space<ub>>)
    hivm.hir.vadd ins(%r0, %a2 : memref<32xf32, #hivm.address_space<ub>>, memref<32xf32, #hivm.address_space<ub>>) outs(%r1 : memref<32xf32, #hivm.address_space<ub>>)
    hivm.hir.vadd ins(%r1, %a3 : memref<32xf32, #hivm.address_space<ub>>, memref<32xf32, #hivm.address_space<ub>>) outs(%r2 : memref<32xf32, #hivm.address_space<ub>>)
    hivm.hir.vadd ins(%r2, %a4 : memref<32xf32, #hivm.address_space<ub>>, memref<32xf32, #hivm.address_space<ub>>) outs(%r3 : memref<32xf32, #hivm.address_space<ub>>)
    hivm.hir.store ins(%r3 : memref<32xf32, #hivm.address_space<ub>>) outs(%dst : memref<32xf32, #hivm.address_space<gm>>)""",
            ),
        ),
        "b_control_cf_backedge_loop": (
            "exact",
            module(
                "b_control_cf_backedge_loop",
                "%src: memref<32xf32, #hivm.address_space<gm>>, "
                "%dst: memref<32xf32, #hivm.address_space<gm>>, %cond: i1",
                """    %alloc = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
    hivm.hir.load ins(%src : memref<32xf32, #hivm.address_space<gm>>) outs(%alloc : memref<32xf32, #hivm.address_space<ub>>)
    cf.br ^loop(%alloc : memref<32xf32, #hivm.address_space<ub>>)
  ^loop(%iter: memref<32xf32, #hivm.address_space<ub>>):
    cf.cond_br %cond, ^loop(%iter : memref<32xf32, #hivm.address_space<ub>>), ^exit(%iter : memref<32xf32, #hivm.address_space<ub>>)
  ^exit(%result: memref<32xf32, #hivm.address_space<ub>>):
    hivm.hir.store ins(%result : memref<32xf32, #hivm.address_space<ub>>) outs(%dst : memref<32xf32, #hivm.address_space<gm>>)""",
            ),
        ),
        "b_control_while_two_ub_results": (
            "exact",
            module(
                "b_control_while_two_ub_results",
                "%src0: memref<32xf32, #hivm.address_space<gm>>, "
                "%src1: memref<32xf32, #hivm.address_space<gm>>, "
                "%dst0: memref<32xf32, #hivm.address_space<gm>>, "
                "%dst1: memref<32xf32, #hivm.address_space<gm>>, %cond: i1",
                """    %a = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
    %b = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
    hivm.hir.load ins(%src0 : memref<32xf32, #hivm.address_space<gm>>) outs(%a : memref<32xf32, #hivm.address_space<ub>>)
    hivm.hir.load ins(%src1 : memref<32xf32, #hivm.address_space<gm>>) outs(%b : memref<32xf32, #hivm.address_space<ub>>)
    %result:2 = scf.while (%iter0 = %a, %iter1 = %b) : (memref<32xf32, #hivm.address_space<ub>>, memref<32xf32, #hivm.address_space<ub>>) -> (memref<32xf32, #hivm.address_space<ub>>, memref<32xf32, #hivm.address_space<ub>>) {
      scf.condition(%cond) %iter0, %iter1 : memref<32xf32, #hivm.address_space<ub>>, memref<32xf32, #hivm.address_space<ub>>
    } do {
    ^bb0(%after0: memref<32xf32, #hivm.address_space<ub>>, %after1: memref<32xf32, #hivm.address_space<ub>>):
      scf.yield %after1, %after0 : memref<32xf32, #hivm.address_space<ub>>, memref<32xf32, #hivm.address_space<ub>>
    }
    hivm.hir.store ins(%result#0 : memref<32xf32, #hivm.address_space<ub>>) outs(%dst0 : memref<32xf32, #hivm.address_space<gm>>)
    hivm.hir.store ins(%result#1 : memref<32xf32, #hivm.address_space<ub>>) outs(%dst1 : memref<32xf32, #hivm.address_space<gm>>)""",
            ),
        ),
        "b_control_scope_two_ub_results": (
            "exact",
            module(
                "b_control_scope_two_ub_results",
                "%src0: memref<32xf32, #hivm.address_space<gm>>, "
                "%src1: memref<32xf32, #hivm.address_space<gm>>, "
                "%dst0: memref<32xf32, #hivm.address_space<gm>>, "
                "%dst1: memref<32xf32, #hivm.address_space<gm>>",
                """    %result:2 = scope.scope : () -> (memref<32xf32, #hivm.address_space<ub>>, memref<32xf32, #hivm.address_space<ub>>) {
      %a = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
      %b = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
      hivm.hir.load ins(%src0 : memref<32xf32, #hivm.address_space<gm>>) outs(%a : memref<32xf32, #hivm.address_space<ub>>)
      hivm.hir.load ins(%src1 : memref<32xf32, #hivm.address_space<gm>>) outs(%b : memref<32xf32, #hivm.address_space<ub>>)
      scope.return %a, %b : memref<32xf32, #hivm.address_space<ub>>, memref<32xf32, #hivm.address_space<ub>>
    } {no_inline}
    hivm.hir.store ins(%result#0 : memref<32xf32, #hivm.address_space<ub>>) outs(%dst0 : memref<32xf32, #hivm.address_space<gm>>)
    hivm.hir.store ins(%result#1 : memref<32xf32, #hivm.address_space<ub>>) outs(%dst1 : memref<32xf32, #hivm.address_space<gm>>)""",
            ),
        ),
        "b_control_scope_two_preload_buffers": (
            "exact",
            module(
                "b_control_scope_two_preload_buffers",
                "%src0: memref<32xf32, #hivm.address_space<gm>>, "
                "%src1: memref<32xf32, #hivm.address_space<gm>>, "
                "%dst0: memref<32xf32, #hivm.address_space<gm>>, "
                "%dst1: memref<32xf32, #hivm.address_space<gm>>",
                """    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index
    %c4 = arith.constant 4 : index
    scf.for %iv = %c0 to %c4 step %c1 {
      %result:2 = scope.scope : () -> (memref<32xf32, #hivm.address_space<ub>>, memref<32xf32, #hivm.address_space<ub>>) {
        %a = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
        annotation.mark %a {hivm.multi_buffer = 2 : i32, hivm.preload_local_buffer = 1 : i32} : memref<32xf32, #hivm.address_space<ub>>
        %b = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
        annotation.mark %b {hivm.multi_buffer = 4 : i32, hivm.preload_local_buffer = 1 : i32} : memref<32xf32, #hivm.address_space<ub>>
        hivm.hir.load ins(%src0 : memref<32xf32, #hivm.address_space<gm>>) outs(%a : memref<32xf32, #hivm.address_space<ub>>)
        hivm.hir.load ins(%src1 : memref<32xf32, #hivm.address_space<gm>>) outs(%b : memref<32xf32, #hivm.address_space<ub>>)
        scope.return %a, %b : memref<32xf32, #hivm.address_space<ub>>, memref<32xf32, #hivm.address_space<ub>>
      } {hivm.loop_core_type = #hivm.tcore_type<VECTOR>, hivm.preload_num = 2 : i32, no_inline}
      hivm.hir.store ins(%result#0 : memref<32xf32, #hivm.address_space<ub>>) outs(%dst0 : memref<32xf32, #hivm.address_space<gm>>)
      hivm.hir.store ins(%result#1 : memref<32xf32, #hivm.address_space<ub>>) outs(%dst1 : memref<32xf32, #hivm.address_space<gm>>)
    }""",
            ),
        ),
        "b_control_cf_branches_with_scf_for": (
            "exact",
            module(
                "b_control_cf_branches_with_scf_for",
                "%src: memref<32xf32, #hivm.address_space<gm>>, "
                "%dst0: memref<32xf32, #hivm.address_space<gm>>, "
                "%dst1: memref<32xf32, #hivm.address_space<gm>>, %cond: i1",
                """    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index
    %c2 = arith.constant 2 : index
    %alloc = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
    hivm.hir.load ins(%src : memref<32xf32, #hivm.address_space<gm>>) outs(%alloc : memref<32xf32, #hivm.address_space<ub>>)
    cf.cond_br %cond, ^bb1(%alloc : memref<32xf32, #hivm.address_space<ub>>), ^bb2(%alloc : memref<32xf32, #hivm.address_space<ub>>)
  ^bb1(%left: memref<32xf32, #hivm.address_space<ub>>):
    scf.for %iv = %c0 to %c2 step %c1 {
      %tmp = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
      hivm.hir.copy ins(%left : memref<32xf32, #hivm.address_space<ub>>) outs(%tmp : memref<32xf32, #hivm.address_space<ub>>)
      hivm.hir.store ins(%tmp : memref<32xf32, #hivm.address_space<ub>>) outs(%dst0 : memref<32xf32, #hivm.address_space<gm>>)
    }
    cf.br ^exit(%left : memref<32xf32, #hivm.address_space<ub>>)
  ^bb2(%right: memref<32xf32, #hivm.address_space<ub>>):
    scf.for %iv = %c0 to %c2 step %c1 {
      %tmp = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
      hivm.hir.copy ins(%right : memref<32xf32, #hivm.address_space<ub>>) outs(%tmp : memref<32xf32, #hivm.address_space<ub>>)
      hivm.hir.store ins(%tmp : memref<32xf32, #hivm.address_space<ub>>) outs(%dst0 : memref<32xf32, #hivm.address_space<gm>>)
    }
    cf.br ^exit(%right : memref<32xf32, #hivm.address_space<ub>>)
  ^exit(%joined: memref<32xf32, #hivm.address_space<ub>>):
    hivm.hir.store ins(%joined : memref<32xf32, #hivm.address_space<ub>>) outs(%dst1 : memref<32xf32, #hivm.address_space<gm>>)""",
            ),
        ),
    }


def write_fixture(path: Path, text: str) -> None:
    path.write_text(text, encoding="utf-8")


def main() -> int:
    module_dir = Path(__file__).resolve().parents[1]
    repo_root = module_dir.parent
    output_root = module_dir / "testdata/planmemory_b"
    exact_dir = output_root / "exact"
    exact_dir.mkdir(parents=True, exist_ok=True)

    source = repo_root / "bishengir/test/Dialect/HIVM/plan-memory.mlir"
    generated = extract_source_cases(source)
    manifest = []
    for name, text in generated.items():
        path = exact_dir / f"{name}.mlir"
        write_fixture(path, text)
        manifest.append((name, "exact", "source-derived", SOURCE_CASES[name], path))
    for name, (mode, text) in custom_cases().items():
        if mode != "exact":
            raise RuntimeError(f"B-stage fixture must be exact: {name}")
        path = exact_dir / f"{name}.mlir"
        write_fixture(path, text)
        manifest.append((name, mode, "boundary-designed", "", path))

    manifest.sort()
    with (output_root / "manifest.tsv").open("w", encoding="utf-8", newline="") as output:
        writer = csv.writer(output, delimiter="\t", lineterminator="\n")
        writer.writerow(("case", "mode", "origin", "source_function", "path"))
        for name, mode, origin, function, path in manifest:
            writer.writerow((name, mode, origin, function,
                             path.relative_to(repo_root)))
    print(f"fixtures={len(manifest)} exact={sum(row[1] == 'exact' for row in manifest)} blocker={sum(row[1] == 'blocker' for row in manifest)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
