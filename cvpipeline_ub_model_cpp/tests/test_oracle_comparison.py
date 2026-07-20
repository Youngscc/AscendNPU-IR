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
from run_corpus_oracle import is_ub_overflow, overflow_address_space  # noqa: E402


assert overflow_address_space(
    'loc("input.mlir":2:3): error: cbuf overflow, requires 4456448 bits') == "cbuf"
assert overflow_address_space(
    'input.mlir:2:3: error: ubuf overflow, requires 1710080 bits') == "ubuf"
assert overflow_address_space("[ERROR] unrelated compiler failure") is None
assert is_ub_overflow("ub")
assert is_ub_overflow("ubuf")
assert not is_ub_overflow("cbuf")

with tempfile.TemporaryDirectory() as directory:
    empty_ub_oracle = Path(directory) / "empty-ub.tsv"
    empty_ub_oracle.write_text(
        "PLANMEM_UB_ORACLE_COMPLETE\t0\nPLANMEM_RUN_RESULT\tsuccess\n",
        encoding="utf-8",
    )
    status, required, multi, inplace = parse_oracle_contract(
        empty_ub_oracle, 0, "6")
    assert status == "success"
    assert required == 0
    assert not multi
    assert not inplace


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
PLANMEM_EXACT_BUFFER\t0\t0\t32768\t6\t0\t100\t200
PLANMEM_EXACT_BUFFER\t0\t1\t32768\t6\t0\t200\t300
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

# Split MIX functions reuse local semantic buffer IDs.  An AIC/L1 multi mark
# must not be joined to an AIV/UB buffer with the same ID.
split_oracle = """\
PLANMEM_LIVENESS_ATTEMPT\tkernel_mix_aic\t0\t0
PLANMEM_EXACT_BUFFER\t0\t0\t32768\t2\t0\t1\t2
PLANMEM_EXACT_MULTI\t0\t0\t2
PLANMEM_PLAN_ATTEMPT\tkernel_mix_aic\t0\tsuccess
PLANMEM_LIVENESS_ATTEMPT\tkernel_mix_aiv\t0\t0
PLANMEM_EXACT_BUFFER\t0\t0\t32768\t6\t0\t10\t20
PLANMEM_PLAN_ATTEMPT\tkernel_mix_aiv\t0\tsuccess
PLANMEM_EXACT_PLANNED_BUFFER\t0\t6\t0\t32768\t0
PLANMEM_PEAK\t0\t6\t32768
"""
with tempfile.TemporaryDirectory(prefix="cvub-split-oracle-test-") as directory:
    path = Path(directory) / "oracle.tsv"
    path.write_text(split_oracle, encoding="utf-8")
    _, _, split_multi, _ = parse_oracle_contract(path, 0, "6")

assert split_multi == collections.Counter({1: 1})

print("[PASS] wrapped reports and applied inplace oracle remain comparable")
