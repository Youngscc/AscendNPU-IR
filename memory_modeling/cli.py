"""Command line entry points for the memory-modeling scaffold."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

from .checkpoints import list_checkpoints, reverse_local_plan_order
from .scanner import create_report, load_config


def _add_common_args(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("input", type=Path, nargs="?", help="Input MLIR file")
    parser.add_argument("--config-json", type=Path, help="Optional compile config JSON")
    parser.add_argument(
        "--checkpoint",
        default="S0",
        help="Checkpoint id to attach to the report, e.g. S0 or S8.5",
    )
    parser.add_argument("--pretty", action="store_true", help="Pretty-print JSON output")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    _add_common_args(parser)
    parser.add_argument(
        "--list-checkpoints",
        action="store_true",
        help="List known checkpoints instead of scanning an input file",
    )
    parser.add_argument(
        "--reverse-plan",
        action="store_true",
        help="List the recommended reverse S8 implementation order",
    )
    return parser.parse_args()


def _checkpoint_rows(reverse_only: bool) -> list[dict[str, object]]:
    checkpoints = reverse_local_plan_order() if reverse_only else list_checkpoints()
    return [
        {
            "id": checkpoint.checkpoint_id,
            "label": checkpoint.label,
            "starts_after_pass": checkpoint.starts_after_pass,
            "suffix_passes": list(checkpoint.suffix_passes),
            "expected_precision": checkpoint.expected_precision.value,
            "goal": checkpoint.goal,
        }
        for checkpoint in checkpoints
    ]


def main() -> int:
    args = parse_args()
    indent = 2 if args.pretty else None

    if args.list_checkpoints or args.reverse_plan:
        print(json.dumps(_checkpoint_rows(args.reverse_plan), indent=indent, sort_keys=True))
        return 0

    if args.input is None:
        raise SystemExit("input file is required unless --list-checkpoints is used")
    if not args.input.is_file():
        raise SystemExit(f"input file not found: {args.input}")

    config = load_config(args.config_json)
    report = create_report(args.input, config=config, checkpoint_id=args.checkpoint)
    print(json.dumps(report.to_dict(), indent=indent, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
