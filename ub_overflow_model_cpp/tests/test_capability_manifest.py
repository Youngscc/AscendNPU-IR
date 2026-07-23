#!/usr/bin/env python3
import csv
import re
from pathlib import Path


manifest = Path("ub_overflow_model_cpp/tests/capability_parity.tsv")
with manifest.open(encoding="utf-8", newline="") as stream:
    rows = list(csv.DictReader(stream, delimiter="\t"))

if len(rows) != 166:
    raise SystemExit(f"expected 166 legacy capability rows, found {len(rows)}")
names = [row["legacy_test"] for row in rows]
if len(names) != len(set(names)):
    raise SystemExit("duplicate legacy capability mapping")
allowed = {
    "ported",
    "updated",
    "yy-retained",
    "yy-retained+evidence",
    "yy-retained+tested",
}
unknown = sorted({row["migration_status"] for row in rows} - allowed)
if unknown:
    raise SystemExit(f"unknown capability migration status: {unknown}")
if any(not row["evidence"].strip() for row in rows):
    raise SystemExit("every legacy capability must name its evidence or audit path")

print("[PASS] all 166 legacy capability checks are mapped")

oracle_source = Path(
    "bishengir/tools/bishengir-cvpipeline-suffix-compile/"
    "bishengir-cvpipeline-suffix-compile.cpp"
).read_text(encoding="utf-8")
oracle_post_stages = re.findall(
    r'addOracleStageSnapshot\(\s*pm,\s*"post",\s*"([^"]+)"\s*\)',
    oracle_source,
)
post_manifest = Path(
    "ub_overflow_model_cpp/config/post_cvpipeline_manifest.tsv"
)
with post_manifest.open(encoding="utf-8", newline="") as stream:
    modeled_post_stages = [
        row["stage"] for row in csv.DictReader(stream, delimiter="\t")
    ]
if set(oracle_post_stages) != set(modeled_post_stages):
    raise SystemExit(
        "post-CVPipeline manifest stage set differs from oracle source:\n"
        f"oracle={oracle_post_stages}\nmodel={modeled_post_stages}"
    )

builder = oracle_source[oracle_source.index("static void buildSuffixPipeline") :]
ordered_calls = [
    "createTileCubeVectorLoopPass",
    "addInferAndSetBufferSizePipeline",
    "addCrossCoreSyncPipeline",
    "createSplitMixKernelPass",
    "createInlineScopePass",
    "createTileAndBindSubBlockPass",
    "createFoldTensorEmptyPass",
    "addCanonicalizationHIVMPipeline",
    "createLoopInvariantCodeMotionPass",
    "createLoopInvariantSubsetHoistingPass",
    "createCloneTensorEmptyPass",
    "createHIVMInlineOTFLoadStorePass",
]
positions = [builder.index(call) for call in ordered_calls]
if positions != sorted(positions):
    raise SystemExit("oracle post-CVPipeline pass calls are out of order")
print("[PASS] 14 post-CVPipeline stages match the oracle source")
