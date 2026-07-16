#!/usr/bin/env python3

import collections
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "cvpipeline_ub_model_cpp/scripts"))

from compare_ub_plan_with_suffix_oracle import (  # noqa: E402
    model_multi_and_inplace,
    normalized_lifetimes_from_model,
    parse_oracle_contract,
    plan_multiset_from_model,
)


wrapped_report = {
    "result": {
        "status": "success",
        "functions": [{
            "function": "kernel",
            "buffers": [
                {
                    "name": "%base_0",
                    "extent_bits": 32768,
                    "offsets_bytes": [0],
                    "alloc_time": 10,
                    "free_time": 20,
                    "multi_buffer_num": 1,
                },
                {
                    "name": "%base_1",
                    "extent_bits": 32768,
                    "offsets_bytes": [4096],
                    "alloc_time": 20,
                    "free_time": 30,
                    "multi_buffer_num": 1,
                },
            ],
            "inplace_pairs": [["%base_1", "%base_0"]],
        }],
    },
}

assert plan_multiset_from_model(wrapped_report) == collections.Counter({
    (32768, 0): 1,
    (32768, 4096): 1,
})
assert len(normalized_lifetimes_from_model(wrapped_report)) == 2
model_multi, model_inplace = model_multi_and_inplace(wrapped_report)
assert model_multi == collections.Counter({1: 2})
assert sum(model_inplace.values()) == 1

oracle = """\
PLANMEM_LIVENESS_ATTEMPT\tkernel\t0\t0
PLANMEM_EXACT_BUFFER\t0\t0\t32768\t6\t0\t10\t20
PLANMEM_EXACT_BUFFER\t0\t1\t32768\t6\t0\t20\t30
PLANMEM_PLAN_ATTEMPT\tkernel\t0\tsuccess
PLANMEM_APPLIED_INPLACE_COUNT\t0\t1
PLANMEM_EXACT_APPLIED_INPLACE\t0\t1\t0
PLANMEM_EXACT_PLANNED_BUFFER\t0\t6\t0\t32768\t0
PLANMEM_EXACT_PLANNED_BUFFER\t0\t6\t1\t32768\t4096
PLANMEM_PEAK\t0\t6\t65536
"""
with tempfile.TemporaryDirectory(prefix="cvub-oracle-test-") as directory:
    path = Path(directory) / "oracle.tsv"
    path.write_text(oracle, encoding="utf-8")
    status, required, oracle_multi, oracle_inplace = parse_oracle_contract(
        path, 0, "6"
    )

assert status == "success"
assert required == 65536
assert oracle_multi == model_multi
assert oracle_inplace == model_inplace

print("[PASS] wrapped reports and applied inplace oracle remain comparable")
