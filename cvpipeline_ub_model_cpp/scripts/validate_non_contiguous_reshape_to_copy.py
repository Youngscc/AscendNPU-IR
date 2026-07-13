#!/usr/bin/env python3
"""Validate existing no-op cases and one real triggered reshape-to-copy case."""

from __future__ import annotations

import re
import subprocess
import tempfile
from pathlib import Path


COLLAPSE = re.compile(
    r'^(\s*)(%[^ ]+) = "memref\.collapse_shape".* -> (memref<.*>)$')


def run_model(tool: Path, source: Path, output: Path, after: Path | None = None) -> int:
    action = ("dump-non-contiguous-reshape-to-copy-oracle" if after
              else "model-non-contiguous-reshape-to-copy")
    command = [str(tool), f"--action={action}", "--root", str(source),
               "--output", str(output)]
    if after:
        command.extend(("--after-ir", str(after)))
    return subprocess.run(command, stdout=subprocess.DEVNULL,
                          stderr=subprocess.DEVNULL).returncode


def main() -> int:
    module = Path(__file__).resolve().parent.parent
    repo = module.parent
    objects = repo / "Output/experiments/c2_c3_pass_oracles/objects"
    model_tool = module / "output/bin/cvpipeline_ub_model_dev_validate"
    compiler = repo / "build/bin/bishengir-opt"
    failures: list[str] = []
    module_pairs = []
    focused_source: Path | None = None
    for before in sorted(objects.glob(
            "*/pass_tree/builtin_module_no-symbol-name/4_convert-"
            "non-contiguous-reshape-to-copy.mlir")):
        after = before.with_name("5_convert-non-contiguous-reshape-to-copy.mlir")
        module_pairs.append((before, after))
        text = before.read_text()
        if (focused_source is None and "memref.collapse_shape" in text and
                "#hivm.func_core_type<AIV>" in text):
            focused_source = before

    with tempfile.TemporaryDirectory(prefix="cvub-non-contiguous-") as temporary:
        temp = Path(temporary)
        for before, after in module_pairs:
            if not after.is_file() or before.read_text().splitlines()[1:] != \
                    after.read_text().splitlines()[1:]:
                failures.append(f"unexpected real pass mutation: {before}")

        if focused_source is None:
            failures.append("no AIV collapse_shape fixture source")
        else:
            lines = focused_source.read_text().splitlines()
            injected = False
            output_lines = []
            for line in lines:
                output_lines.append(line)
                match = COLLAPSE.match(line)
                if match and not injected:
                    indent, value, type_text = match.groups()
                    output_lines.append(
                        f'{indent}"annotation.mark"({value}) '
                        f'{{maybeUnCollapsibleReshape}} : ({type_text}) -> ()')
                    injected = True
            if not injected:
                failures.append("failed to inject focused annotation")
            else:
                focused = temp / "focused.mlir"
                actual = temp / "actual.mlir"
                model = temp / "model.tsv"
                oracle = temp / "oracle.tsv"
                focused.write_text("\n".join(output_lines) + "\n")
                compiled = subprocess.run([
                    str(compiler), str(focused),
                    "--convert-non-contiguous-reshape-to-copy",
                    "--mlir-print-op-generic", "--mlir-disable-threading",
                    "-o", str(actual)], stdout=subprocess.DEVNULL,
                    stderr=subprocess.DEVNULL)
                if compiled.returncode:
                    failures.append("focused real pass failed")
                elif run_model(model_tool, focused, model) or \
                        run_model(model_tool, focused, oracle, actual) or \
                        model.read_bytes() != oracle.read_bytes():
                    failures.append("focused model/oracle mismatch")

    if failures:
        print(f"NON_CONTIGUOUS_RESHAPE_TO_COPY=FAIL "
              f"objects={len(module_pairs)} failures={len(failures)}")
        print("\n".join(failures[:30]))
        return 1
    print(f"NON_CONTIGUOUS_RESHAPE_TO_COPY=PASS "
          f"objects={len(module_pairs)} focused=1")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
