#!/usr/bin/env bash
set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "${MODULE_DIR}/.." && pwd)"
SUMMARY="${PLANMEMORY_FIXED_SEED_ORACLE_SUMMARY:-${MODULE_DIR}/output/fixed_seed_oracle/summary.tsv}"
ORACLE_TOOL="${PLANMEMORY_ORACLE_TOOL:-${REPO_ROOT}/build/bin/bishengir-cvpipeline-suffix-compile}"

[[ -f "${SUMMARY}" ]] || {
  echo "[ERROR] oracle summary is missing: ${SUMMARY}" >&2
  exit 2
}
[[ -x "${ORACLE_TOOL}" ]] || {
  echo "[ERROR] oracle tool is missing or not executable: ${ORACLE_TOOL}" >&2
  exit 2
}

header="$(head -n 1 "${SUMMARY}")"
if [[ $'\t'"${header}"$'\t' != *$'\tinput_sha256\t'* ||
      $'\t'"${header}"$'\t' != *$'\toracle_tool_sha256\t'* ]]; then
  echo "[ERROR] oracle has no provenance hashes; regenerate it with dump_fixed_seed_oracles.sh" >&2
  exit 2
fi

WORK_FILE="$(mktemp "${TMPDIR:-/tmp}/cvub-oracle-provenance.XXXXXX")"
trap 'rm -f "${WORK_FILE}"' EXIT
awk -F '\t' -v OFS='\t' '
  NR == 1 {
    for (i = 1; i <= NF; ++i) {
      if ($i == "input") input_column = i
      if ($i == "input_sha256") hash_column = i
      if ($i == "oracle_tool_sha256") tool_hash_column = i
    }
    if (!input_column || !hash_column || !tool_hash_column) exit 2
    next
  }
  { print $input_column, $hash_column, $tool_hash_column }
' "${SUMMARY}" | LC_ALL=C sort -u >"${WORK_FILE}"

hash_file() {
  if command -v shasum >/dev/null 2>&1; then
    shasum -a 256 "$1" | awk '{print $1}'
  else
    sha256sum "$1" | awk '{print $1}'
  fi
}

failures=0
oracle_tool_hash="$(hash_file "${ORACLE_TOOL}")"
while IFS=$'\t' read -r input expected expected_tool_hash; do
  if [[ ! -f "${input}" || "$(hash_file "${input}")" != "${expected}" ]]; then
    echo "[ERROR] stale oracle input: ${input}" >&2
    failures=$((failures + 1))
  fi
  if [[ "${expected_tool_hash}" != "${oracle_tool_hash}" ]]; then
    echo "[ERROR] oracle tool hash does not match ${ORACLE_TOOL}" >&2
    failures=$((failures + 1))
  fi
done <"${WORK_FILE}"

[[ "${failures}" -eq 0 ]] || exit 1
echo "PLANMEMORY_ORACLE_PROVENANCE=PASS"
