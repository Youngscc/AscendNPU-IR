#!/usr/bin/env python3
"""User-facing entry from the CVPipelining-before boundary to a UB plan.

The C++ model already owns the real work.  This wrapper keeps the command line
easy to read by separating CVPipelining knobs from UB-planning knobs, and
it always returns both the UB peak summary and the concrete buffer plan.
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path
from typing import Any


def parse_bool(text: str | bool) -> bool:
    if isinstance(text, bool):
        return text
    value = text.lower()
    if value in {"1", "true", "yes", "on"}:
        return True
    if value in {"0", "false", "no", "off"}:
        return False
    raise argparse.ArgumentTypeError(
        "expected one of: 0, 1, true, false, yes, no, on, off")


def add_optional_bool(
    group: argparse._ArgumentGroup,
    name: str,
    *,
    default: bool,
    help_text: str,
) -> None:
    group.add_argument(
        name,
        nargs="?",
        const=True,
        default=default,
        type=parse_bool,
        metavar="BOOL",
        help=help_text,
    )


def parse_args() -> argparse.Namespace:
    module = Path(__file__).resolve().parent.parent
    repo = module.parent
    parser = argparse.ArgumentParser(
        description=(
            "Run the lightweight CVPipelining-to-PlanMemory model from "
            "generic IR captured before CVPipelining, "
            "and print exact UB peak plus the local-memory plan."))

    input_group = parser.add_argument_group("input")
    input_group.add_argument(
        "--before-cvpipelining-ir",
        dest="before_cvpipelining_ir", type=Path, required=True,
        help="Generic-form MLIR at the createCVPipeliningPass-before boundary.")
    input_group.add_argument(
        "--model", type=Path,
        default=module / "output/bin/bishengir-ub-overflow-model",
        help="Production lightweight C++ model binary.")
    input_group.add_argument(
        "--output", type=Path, default=None,
        help="Optional file to write the report. Stdout is used by default.")
    input_group.add_argument(
        "--format", choices=["text", "json"], default="text",
        help="Report format. JSON includes the parsed plan table.")

    cv_group = parser.add_argument_group("CVPipelining options")
    add_optional_bool(
        cv_group, "--cv-disable-pipelining", default=False,
        help_text="Disable the modeled CVPipelining pass.")
    cv_group.add_argument(
        "--cv-pipeline-depth", type=int, default=-1,
        help="CVPipelining unroll depth; -1 keeps the real default behavior.")
    add_optional_bool(
        cv_group, "--cv-enable-preload", default=False,
        help_text="Enable CVPipelining preload/skew mode.")
    add_optional_bool(
        cv_group, "--cv-enable-lazy-loading", default=False,
        help_text="Enable CVPipelining lazy-loading behavior.")

    plan_group = parser.add_argument_group("UB-affecting planning options")
    add_optional_bool(
        plan_group, "--enable-auto-multi-buffer", default=False,
        help_text="Enable modeled MarkMultiBuffer before local PlanMemory.")
    add_optional_bool(
        plan_group, "--enable-code-motion", default=True,
        help_text="Match the real LICM and subset-hoisting pipeline option.")
    plan_group.add_argument(
        "--tile-mix-cube-loop", type=int, default=2,
        help="Cube loop tiling factor passed to TileCubeVectorLoop.")
    plan_group.add_argument(
        "--tile-mix-vector-loop", type=int, default=2,
        help="Vector loop tiling factor passed to TileCubeVectorLoop.")
    add_optional_bool(
        plan_group, "--enable-ubuf-saving", default=False,
        help_text="Enable SinkOpToConsumerInLoop before OneShotBufferize.")
    add_optional_bool(
        plan_group, "--enable-triton-kernel-compile", default=False,
        help_text=("Run Triton-only DPS insert-slice optimization before "
                   "OneShotBufferize."))
    add_optional_bool(
        plan_group, "--enable-hivm-auto-storage-align", default=True,
        help_text="Match native MarkStrideAlign/EnableStrideAlign behavior.")
    plan_group.add_argument(
        "--local-multi-buffer-strategy", default="no-l0c",
        choices=["no-limit", "only-cube", "only-vector", "no-l0c"],
        help="Strategy passed to --limit-auto-multi-buffer-of-local-buffer.")
    plan_group.add_argument(
        "--mix-multi-buffer-strategy", default="only-cube",
        choices=["no-limit", "only-cube", "only-vector", "no-l0c"],
        help="Strategy passed to --limit-auto-multi-buffer-buffer.")
    plan_group.add_argument(
        "--plan-memory-seed", type=int, default=-1,
        help="Fixed PlanMemory seed. Omit to use PlanMemory retry mode.")
    plan_group.add_argument("--restrict-inplace-as-isa", action="store_true")
    args = parser.parse_args()
    if (args.tile_mix_cube_loop <= 0 or args.tile_mix_vector_loop <= 0):
        parser.error("tile-mix loop factors must be positive")
    return args


def require_file(path: Path, description: str, executable: bool = False) -> Path:
    resolved = path.resolve()
    if not resolved.is_file():
        raise RuntimeError(f"{description} not found: {resolved}")
    if executable and not resolved.stat().st_mode & 0o111:
        raise RuntimeError(f"{description} is not executable: {resolved}")
    return resolved


def model_command(args: argparse.Namespace) -> list[str]:
    command = [
        str(args.model),
        "--before-cvpipelining-ir", str(args.before_cvpipelining_ir),
        "--cv-pipeline-depth", str(args.cv_pipeline_depth),
        "--disable-auto-cv-work-space-manage="
        f"{str(args.cv_disable_pipelining).lower()}",
        f"--enable-preload={str(args.cv_enable_preload).lower()}",
        f"--enable-lazy-loading={str(args.cv_enable_lazy_loading).lower()}",
        f"--enable-auto-multi-buffer={str(args.enable_auto_multi_buffer).lower()}",
        f"--enable-code-motion={str(args.enable_code_motion).lower()}",
        f"--tile-mix-cube-loop={args.tile_mix_cube_loop}",
        f"--tile-mix-vector-loop={args.tile_mix_vector_loop}",
        f"--enable-ubuf-saving={str(args.enable_ubuf_saving).lower()}",
        f"--enable-triton-kernel-compile={str(args.enable_triton_kernel_compile).lower()}",
        "--enable-hivm-auto-storage-align="
        f"{str(args.enable_hivm_auto_storage_align).lower()}",
        "--limit-auto-multi-buffer-of-local-buffer",
        args.local_multi_buffer_strategy,
        "--limit-auto-multi-buffer-buffer",
        args.mix_multi_buffer_strategy,
        f"--format={args.format}",
    ]
    command.append(f"--plan-memory-seed={args.plan_memory_seed}")
    if args.restrict_inplace_as_isa:
        command.append("--restrict-inplace-as-isa")
    return command


def run_model(args: argparse.Namespace) -> subprocess.CompletedProcess[str]:
    return subprocess.run(model_command(args), text=True, capture_output=True)


def parse_model_text(stdout: str) -> dict[str, Any]:
    result: dict[str, Any] = {}
    plan: list[dict[str, Any]] = []
    in_plan = False
    for line in stdout.splitlines():
        if line == "name\textent_bits\toffset_bytes\talloc_time\tfree_time":
            in_plan = True
            continue
        if in_plan:
            columns = line.split("\t")
            if len(columns) == 6:
                function, name, extent_bits, offset_bytes, alloc_time, free_time = columns
            else:
                function = ""
                name, extent_bits, offset_bytes, alloc_time, free_time = columns
            plan.append({
                "function": function,
                "name": name,
                "extent_bits": int(extent_bits),
                "offset_bytes": int(offset_bytes),
                "alloc_time": int(alloc_time),
                "free_time": int(free_time),
            })
            continue
        if "\t" not in line:
            continue
        key, value = line.split("\t", 1)
        if key in {"success", "overflow", "restrict_inplace_as_isa"}:
            result[key] = value == "true"
        elif key in {"selected_seed", "peak_bits", "required_bits", "capacity_bits"}:
            result[key] = None if value == "null" else int(value)
        else:
            result[key] = value
    result["plan"] = plan
    return result


def options_payload(args: argparse.Namespace) -> dict[str, Any]:
    return {
        "cvpipelining": {
            "disable_pipelining": args.cv_disable_pipelining,
            "pipeline_depth": args.cv_pipeline_depth,
            "enable_preload": args.cv_enable_preload,
            "enable_lazy_loading": args.cv_enable_lazy_loading,
        },
        "ub_planning": {
            "enable_auto_multi_buffer": args.enable_auto_multi_buffer,
            "enable_code_motion": args.enable_code_motion,
            "tile_mix_cube_loop": args.tile_mix_cube_loop,
            "tile_mix_vector_loop": args.tile_mix_vector_loop,
            "enable_ubuf_saving": args.enable_ubuf_saving,
            "enable_triton_kernel_compile":
                args.enable_triton_kernel_compile,
            "enable_hivm_auto_storage_align":
                args.enable_hivm_auto_storage_align,
            "local_multi_buffer_strategy": args.local_multi_buffer_strategy,
            "mix_multi_buffer_strategy": args.mix_multi_buffer_strategy,
            "plan_memory_seed": args.plan_memory_seed,
            "restrict_inplace_as_isa": args.restrict_inplace_as_isa,
        },
    }


def normalize_model_json(stdout: str) -> dict[str, Any]:
    """Preserve the product report and add legacy demo projection fields."""
    result = json.loads(stdout)
    result["peak_bits"] = result.get("ub_peak_bits")
    result["success"] = result.get("status") == "success"
    functions = result.get("functions", [])
    result["selected_seed"] = (
        functions[0].get("selected_seed") if len(functions) == 1 else None
    )
    plan: list[dict[str, Any]] = []
    for function in functions:
        for buffer in function.get("buffers", []):
            for offset in buffer.get("offsets_bytes", []):
                plan.append({
                    "function": function.get("function", ""),
                    "name": buffer.get("name", ""),
                    "extent_bits": buffer.get("extent_bits"),
                    "offset_bytes": offset,
                    "alloc_time": buffer.get("alloc_time"),
                    "free_time": buffer.get("free_time"),
                    "multi_buffer_num": buffer.get("multi_buffer_num", 1),
                })
    result["plan"] = plan
    return result


def text_report(args: argparse.Namespace,
                process: subprocess.CompletedProcess[str]) -> str:
    lines = [
        "BEFORE_CVPIPELINING_UB_PLAN",
        f"input.before_cvpipelining_ir\t{args.before_cvpipelining_ir}",
        f"cvpipelining.disable_pipelining\t{str(args.cv_disable_pipelining).lower()}",
        f"cvpipelining.pipeline_depth\t{args.cv_pipeline_depth}",
        f"cvpipelining.enable_preload\t{str(args.cv_enable_preload).lower()}",
        f"cvpipelining.enable_lazy_loading\t{str(args.cv_enable_lazy_loading).lower()}",
        f"ub.enable_auto_multi_buffer\t{str(args.enable_auto_multi_buffer).lower()}",
        f"ub.enable_code_motion\t{str(args.enable_code_motion).lower()}",
        f"ub.tile_mix_cube_loop\t{args.tile_mix_cube_loop}",
        f"ub.tile_mix_vector_loop\t{args.tile_mix_vector_loop}",
        f"ub.enable_ubuf_saving\t{str(args.enable_ubuf_saving).lower()}",
        f"ub.enable_triton_kernel_compile\t{str(args.enable_triton_kernel_compile).lower()}",
        "ub.enable_hivm_auto_storage_align\t"
        f"{str(args.enable_hivm_auto_storage_align).lower()}",
        f"ub.local_multi_buffer_strategy\t{args.local_multi_buffer_strategy}",
        f"ub.mix_multi_buffer_strategy\t{args.mix_multi_buffer_strategy}",
        f"plan.plan_memory_seed\t{args.plan_memory_seed}",
        f"plan.restrict_inplace_as_isa\t{str(args.restrict_inplace_as_isa).lower()}",
        f"model_returncode\t{process.returncode}",
    ]
    if process.stderr:
        lines.append(f"model_stderr\t{process.stderr.strip()}")
    lines.append("MODEL_RESULT")
    lines.append(process.stdout.rstrip("\n"))
    return "\n".join(lines) + "\n"


def json_report(args: argparse.Namespace,
                process: subprocess.CompletedProcess[str]) -> str:
    result = normalize_model_json(process.stdout) if process.stdout else None
    payload = {
        "schema_version": 1,
        "input": {
            "before_cvpipelining_ir": str(args.before_cvpipelining_ir),
        },
        "options": options_payload(args),
        "model_returncode": process.returncode,
        "model_stderr": process.stderr.strip(),
        "result": result,
    }
    return json.dumps(payload, indent=2, sort_keys=True) + "\n"


def main() -> int:
    args = parse_args()
    args.before_cvpipelining_ir = require_file(
        args.before_cvpipelining_ir, "generic IR before CVPipelining")
    args.model = require_file(
        args.model, "bishengir-ub-overflow-model", executable=True)

    process = run_model(args)
    if args.format == "json":
        parsed_result = normalize_model_json(process.stdout) if process.stdout else None
    else:
        parsed_result = parse_model_text(process.stdout) if process.stdout else None
    expected_overflow = (
        process.returncode == 2
        and parsed_result is not None
        and parsed_result.get("overflow") is True
    )
    if process.returncode != 0 and not expected_overflow:
        print(
            f"[ERROR] lightweight UB model failed with exit code "
            f"{process.returncode}",
            file=sys.stderr,
        )
        diagnostic = process.stderr.strip() or process.stdout.strip()
        if diagnostic:
            print(diagnostic, file=sys.stderr)
        else:
            print("[ERROR] model produced no diagnostic output", file=sys.stderr)
    elif expected_overflow:
        print(
            "[INFO] lightweight UB model completed with exact UB overflow",
            file=sys.stderr,
        )
    report = json_report(args, process) if args.format == "json" else text_report(args, process)
    if args.output is None:
        print(report, end="")
    else:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(report)
        print(args.output)
    return process.returncode


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:  # noqa: BLE001 - final CLI error boundary.
        print(f"[ERROR] {exc}", file=sys.stderr)
        raise SystemExit(1)
