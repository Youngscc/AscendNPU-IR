#!/usr/bin/env bash

needs_rebuild() {
  local output="$1"
  local module_dir="$2"

  if [[ "${FORCE_REBUILD:-0}" == "1" ]]; then
    return 0
  fi
  if [[ ! -x "${output}" ]]; then
    return 0
  fi

  while IFS= read -r source_file; do
    if [[ "${source_file}" -nt "${output}" ]]; then
      return 0
    fi
  done < <(find "${module_dir}/src" -type f \
      \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \))

  return 1
}

compile_if_needed() {
  local source="$1"
  local output="$2"
  local module_dir="$3"
  local compiler="${CXX:-c++}"

  mkdir -p "$(dirname "${output}")"
  if needs_rebuild "${output}" "${module_dir}"; then
    "${compiler}" -std=c++17 -O3 -Wall -Wextra -Wpedantic -Wconversion \
      -Wshadow -Werror \
      "${source}" -o "${output}"
  fi
  echo "${output}"
}
