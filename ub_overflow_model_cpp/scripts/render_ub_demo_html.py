#!/usr/bin/env python3
"""Create a self-contained visualizer HTML snapshot with embedded UB JSON."""

from __future__ import annotations

import argparse
import json
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--template", type=Path, required=True)
    parser.add_argument("--json", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    html = args.template.read_text(encoding="utf-8")
    payload = json.loads(args.json.read_text(encoding="utf-8"))
    embedded = json.dumps(payload, ensure_ascii=False)
    marker = "    render(sample);"
    replacement = (
        "    const embeddedReport = " + embedded + ";\n"
        "    render(embeddedReport);"
    )
    if marker not in html:
        raise RuntimeError("visualizer template marker not found")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(html.replace(marker, replacement), encoding="utf-8")
    print(args.output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
