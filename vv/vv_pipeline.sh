#!/bin/bash
# VV corpus pipeline: Triton driver .py -> ttadapter (compile-only dump) ->
# bishengir-compile --enable-memory-display -> memory_info_*.json -> UB peak.
# Usage: vv_pipeline.sh <driver.py> <name>
set -uo pipefail

DRV="$1"; NAME="$2"
VT=/mnt/d/work/git/vTriton
PY=/home/shane/miniconda3/envs/vtriton-verify/bin/python
HW=$VT/configs/ascend_910b.json
TRITONSIM=$VT/build/bin/tritonsim-hivm
export PATH=/home/shane/Ascend/cann-9.0.0-beta.2/x86_64-linux/bin:$PATH
BC=/home/shane/Ascend/cann-9.0.0-beta.2/x86_64-linux/bin/bishengir-compile
ROOT=/tmp/claude-1000/-mnt-d-work-git-AscendNPU-IR/5f0b90d3-2b64-481e-ac0d-76ef25d43c22/scratchpad/vv_corpus/$NAME
mkdir -p "$ROOT"

echo "=== [$NAME] step 1: compile-only dump (arch 9382) ==="
LOG="$ROOT/dump.log"
timeout 300 "$TRITONSIM" --triton-script "$DRV" --python "$PY" --hardware-config "$HW" \
  --triton-ascend-arch Ascend910_9382 --des-graph-file "$ROOT/des.json" --keep-dump-dir > "$LOG" 2>&1
DD=$(grep -oE "Kept dump directory: .*" "$LOG" | tail -1 | sed 's/Kept dump directory: //')
TT=$(find "$DD" -name 'kernel.ttadapter.mlir' 2>/dev/null | head -1)
echo "dump dir: $DD"; echo "ttadapter: $TT"
if [ -z "$TT" ]; then echo "FAIL: no ttadapter"; exit 2; fi
cp "$TT" "$ROOT/$NAME.ttadapter.mlir"
grep -o 'hacc.target = #hacc.target<"[^"]*">' "$TT" | head -1

echo "=== [$NAME] step 2: bishengir-compile + memory-display ==="
env -C "$ROOT" timeout 300 "$BC" --target=Ascend910_9382 \
  --enable-auto-multi-buffer=true --enable-auto-bind-sub-block=true --enable-auto-blockify-loop \
  --enable-hfusion-compile=true --enable-hivm-compile=true --enable-triton-kernel-compile=true \
  --enable-memory-display "$ROOT/$NAME.ttadapter.mlir" -o "$ROOT/$NAME" \
  > "$ROOT/compile.out" 2> "$ROOT/compile.err"
echo "bishengir rc=$?"

echo "=== [$NAME] step 3: UB metrics ==="
for J in "$ROOT"/memory_info_*.json; do
  [ -e "$J" ] || { echo "no memory_info json (see compile.err)"; tail -5 "$ROOT/compile.err"; break; }
  python3 - "$J" <<'PY'
import json,sys,os
d=json.load(open(sys.argv[1]))
kn=d.get("Header",{}).get("KernelName")
for rec in d.get("Record",[]):
    arr=rec.get("memory_info_array",[]); peak=ext=tmp=0
    for it in arr:
        e=int(it.get("extent",0)); ext+=e
        tmp+= 1 if it.get("is_tmpbuf") is True else 0
        offs=it.get("offset",[]); offs=offs if isinstance(offs,list) else [offs]
        for o in offs: peak=max(peak,int(o)*8+e)
    r=f"{peak/ext:.2f}" if ext else "-"
    print(f"  {os.path.basename(sys.argv[1])} kernel={kn} scope={rec.get('scope')} "
          f"status={rec.get('status')} nbuf={len(arr)} tmp={tmp} "
          f"peak={peak} ext={ext} cap=1572864 ratio={r}")
PY
done
