#!/usr/bin/env python3
"""Compatibility entry point for cv2pm-to-model corpus validation.

The old suffix-specific implementation has been retired.  Both the single
scenario and matrix workflows now use the same 27-scenario/cache-aware engine;
select one scenario with ``--config NAME`` when a focused run is desired.
"""

from run_corpus_matrix import main


if __name__ == "__main__":
    raise SystemExit(main())
