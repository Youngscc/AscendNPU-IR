"""Backward-compatible S0 snapshot entry point.

The implementation now lives in :mod:`memory_modeling.scanner` and the shared
CLI in :mod:`memory_modeling.cli`.
"""

from __future__ import annotations

from pathlib import Path
from typing import Any

from .cli import main
from .scanner import create_report


def create_snapshot(input_file: Path, config: dict[str, Any] | None = None):
    """Create an S0 modeling report for existing callers."""

    return create_report(input_file, config=config, checkpoint_id="S0")


if __name__ == "__main__":
    raise SystemExit(main())
