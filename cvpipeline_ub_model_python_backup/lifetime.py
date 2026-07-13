"""Lifetime analysis for PlanMemory-before IR.

This is the B2 model layer.  It intentionally stays lightweight, but follows
the PlanMemory liveness shape: memref.alloc defines a buffer, a later producer
operation generates it, and the free time is the operation where the buffer is
last needed.  View-like memref ops are tracked as aliases of their source
allocs, and simple DPS inplace candidates are unioned to approximate
PlanMemory's storage-entry lifetime.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import re
from typing import Iterable

from .ir_parser import parse_planmemory_before_ir


_FUNC_RE = re.compile(r"func\.func\s+@(?P<name>[\w$.#-]+)")
_CORE_RE = re.compile(r"hivm\.func_core_type\s*=\s*#hivm\.func_core_type<(?P<core>\w+)>")
_RESULT_RE = re.compile(r"^\s*(?P<result>%[\w$.#-]+(?::\d+)?)\s*=")
_SSA_RE = re.compile(r"%[\w$.#-]+")
_OUTS_RE = re.compile(r"outs\((?P<body>.*?)\)", re.DOTALL)
_INS_RE = re.compile(r"ins\((?P<body>.*?)\)", re.DOTALL)
_TEMP_BUFFER_RE = re.compile(r"temp_buffer\((?P<body>.*?)\)", re.DOTALL)
_SCF_FOR_ITER_RE = re.compile(r"iter_args\((?P<body>.*?)\)")
_SCF_YIELD_RE = re.compile(r"^\s*scf\.yield\b(?P<body>.*)")


@dataclass(frozen=True)
class OperationRecord:
    index: int
    line: int
    text: str
    op_name: str


@dataclass(frozen=True)
class LifetimeRecord:
    ssa_name: str
    alloc_time: int
    free_time: int
    direct_alloc_time: int
    direct_free_time: int
    group: tuple[str, ...]

    @property
    def lifetime(self) -> tuple[int, int]:
        return (self.alloc_time, self.free_time)

    @property
    def direct_lifetime(self) -> tuple[int, int]:
        return (self.direct_alloc_time, self.direct_free_time)


@dataclass(frozen=True)
class LifetimeAnalysis:
    path: Path
    records: tuple[LifetimeRecord, ...]
    operations: tuple[OperationRecord, ...]


@dataclass(frozen=True)
class _OpUseInfo:
    operation: OperationRecord
    generated: frozenset[str]
    outs: frozenset[str]
    ins: frozenset[str]
    temps: frozenset[str]
    uses: frozenset[str]


class _UnionFind:
    def __init__(self, values: Iterable[str]):
        self.parent = {value: value for value in values}

    def find(self, value: str) -> str:
        parent = self.parent[value]
        if parent != value:
            self.parent[value] = self.find(parent)
        return self.parent[value]

    def union(self, left: str, right: str) -> None:
        if left not in self.parent or right not in self.parent:
            return
        left_root = self.find(left)
        right_root = self.find(right)
        if left_root == right_root:
            return
        if right_root < left_root:
            left_root, right_root = right_root, left_root
        self.parent[right_root] = left_root

    def groups(self) -> dict[str, list[str]]:
        result: dict[str, list[str]] = {}
        for value in self.parent:
            result.setdefault(self.find(value), []).append(value)
        for values in result.values():
            values.sort(key=_ssa_sort_key)
        return result


def _ssa_sort_key(name: str) -> tuple[int, str]:
    return (len(name), name)


def _brace_delta(line: str) -> int:
    return line.count("{") - line.count("}")


def _extract_op_name(line: str) -> str:
    stripped = line.strip()
    if "=" in stripped:
        stripped = stripped.split("=", 1)[1].strip()
    match = re.match(r"([\w.]+)", stripped)
    return match.group(1) if match else ""


def _is_operation_line(line: str) -> bool:
    stripped = line.strip()
    if not stripped or stripped == "}":
        return False
    return re.match(r"(%[\w$.#:-]+\s*=\s*)?[\w.]+\b", stripped) is not None


def _extract_ssas(fragment: str) -> list[str]:
    return _SSA_RE.findall(fragment)


def _extract_group_ssas(line: str, regex: re.Pattern[str]) -> list[str]:
    match = regex.search(line)
    if not match:
        return []
    return _extract_ssas(match.group("body"))


def _canonical(alias: dict[str, str], value: str) -> str | None:
    seen = set()
    current = value
    while current in alias and current not in seen:
        seen.add(current)
        current = alias[current]
    return current


def _canonical_alloc(alias: dict[str, str], alloc_names: set[str], value: str) -> str | None:
    base = _canonical(alias, value)
    if base in alloc_names:
        return base
    return None


def _parse_function_operations(path: Path, core_type: str) -> tuple[OperationRecord, ...]:
    operations: list[OperationRecord] = []
    current_function = ""
    current_core = ""
    function_depth = 0
    op_index = 0
    loop_stack: list[tuple[int, int]] = []

    for line_no, line in enumerate(path.read_text().splitlines(), start=1):
        func_match = _FUNC_RE.search(line)
        if func_match:
            current_function = func_match.group("name")
            core_match = _CORE_RE.search(line)
            current_core = core_match.group("core") if core_match else ""
            function_depth = max(_brace_delta(line), 1)
            op_index = 0
            loop_stack.clear()
            continue

        if not current_function:
            continue

        if current_core == core_type and _is_operation_line(line):
            operations.append(
                OperationRecord(
                    index=op_index,
                    line=line_no,
                    text=line.strip(),
                    op_name=_extract_op_name(line),
                )
            )
            if line.strip().startswith("scf.for ") or " = scf.for " in line:
                loop_stack.append((function_depth, line_no))
            op_index += 1

        old_depth = function_depth
        function_depth += _brace_delta(line)
        if current_core == core_type and loop_stack:
            while loop_stack and function_depth < loop_stack[-1][0]:
                _, loop_line = loop_stack.pop()
                operations.append(
                    OperationRecord(
                        index=op_index,
                        line=line_no,
                        text=f"<scf.for.end from line {loop_line}>",
                        op_name="scf.for.end",
                    )
                )
                op_index += 1

        if function_depth <= 0:
            current_function = ""
            current_core = ""
            function_depth = 0
            loop_stack.clear()
        elif old_depth <= 0:
            function_depth = 0

    return tuple(operations)


def analyze_lifetimes(
    before_ir: str | Path,
    core_type: str = "AIV",
    memory_space: str = "ub",
) -> LifetimeAnalysis:
    path = Path(before_ir)
    parsed = parse_planmemory_before_ir(path)
    ir_records = parsed.filter(core_type=core_type, memory_space=memory_space)
    alloc_names = {record.ssa_name for record in ir_records}
    alloc_extent = {record.ssa_name: record.extent_bits for record in ir_records}
    temp_names = {record.ssa_name for record in ir_records if record.is_tmpbuf_hint}
    operations = _parse_function_operations(path, core_type=core_type)

    alias: dict[str, str] = {name: name for name in alloc_names}
    direct_start: dict[str, int] = {}
    direct_end: dict[str, int] = {}
    uf = _UnionFind(alloc_names)
    op_use_infos: list[_OpUseInfo] = []

    pending_loop_iter_aliases: list[tuple[str, str]] = []

    for op in operations:
        line = op.text
        if op.op_name == "scf.for.end":
            live_loop_values = {
                base
                for iter_arg, _init in pending_loop_iter_aliases
                if (base := _canonical_alloc(alias, alloc_names, iter_arg))
            }
            for name in live_loop_values:
                direct_end[name] = max(direct_end.get(name, op.index), op.index)
            op_use_infos.append(
                _OpUseInfo(
                    operation=op,
                    generated=frozenset(),
                    outs=frozenset(),
                    ins=frozenset(),
                    temps=frozenset(),
                    uses=frozenset(live_loop_values),
                )
            )
            for iter_arg, init in pending_loop_iter_aliases:
                base = _canonical_alloc(alias, alloc_names, init)
                if base:
                    alias[iter_arg] = base
            pending_loop_iter_aliases.clear()
            continue

        result_match = _RESULT_RE.match(line)
        result_name = result_match.group("result") if result_match else None
        result_base = result_name.split(":", 1)[0] if result_name else None

        if op.op_name.startswith("memref.") and op.op_name not in {"memref.alloc"}:
            operands = [ssa for ssa in _extract_ssas(line) if ssa != result_base]
            for operand in operands:
                base = _canonical_alloc(alias, alloc_names, operand)
                if base and result_base:
                    alias[result_base] = base
                    break
            op_use_infos.append(
                _OpUseInfo(
                    operation=op,
                    generated=frozenset(),
                    outs=frozenset(),
                    ins=frozenset(),
                    temps=frozenset(),
                    uses=frozenset(),
                )
            )
            continue

        if op.op_name == "scf.for":
            iter_match = _SCF_FOR_ITER_RE.search(line)
            if iter_match:
                for iter_arg, init in re.findall(r"(%[\w$.#-]+)\s*=\s*(%[\w$.#-]+)", iter_match.group("body")):
                    base = _canonical_alloc(alias, alloc_names, init)
                    if base:
                        alias[iter_arg] = base
                        pending_loop_iter_aliases.append((iter_arg, init))

        yield_match = _SCF_YIELD_RE.match(line)
        if yield_match:
            yielded = _extract_ssas(yield_match.group("body"))
            for yielded_value, (iter_arg, _init) in zip(yielded, pending_loop_iter_aliases):
                base = _canonical_alloc(alias, alloc_names, yielded_value)
                if base:
                    alias[iter_arg] = base

        outs = {
            base
            for value in _extract_group_ssas(line, _OUTS_RE)
            if (base := _canonical_alloc(alias, alloc_names, value))
        }
        temps = {
            base
            for value in _extract_group_ssas(line, _TEMP_BUFFER_RE)
            if (base := _canonical_alloc(alias, alloc_names, value))
        }
        ins = {
            base
            for value in _extract_group_ssas(line, _INS_RE)
            if (base := _canonical_alloc(alias, alloc_names, value))
        }

        if op.op_name == "memref.store":
            operands = [
                base
                for value in _extract_ssas(line)
                if (base := _canonical_alloc(alias, alloc_names, value))
            ]
            if operands:
                outs.add(operands[-1])
                ins.update(operands[:-1])

        generated = outs | temps
        if op.op_name == "memref.alloc" and result_base in alloc_names:
            # Definition only.  PlanMemory starts the lifetime at the first gen.
            op_use_infos.append(
                _OpUseInfo(
                    operation=op,
                    generated=frozenset(),
                    outs=frozenset(),
                    ins=frozenset(),
                    temps=frozenset(),
                    uses=frozenset(),
                )
            )
            continue
        else:
            for name in generated:
                direct_start.setdefault(name, op.index)
            for name in generated | ins:
                direct_end[name] = max(direct_end.get(name, op.index), op.index)

        # Any other appearance after alias resolution is a conservative use.
        conservative_uses: set[str] = set()
        if op.op_name != "memref.alloc":
            for value in _extract_ssas(line):
                base = _canonical_alloc(alias, alloc_names, value)
                if base and base not in generated:
                    direct_end[base] = max(direct_end.get(base, op.index), op.index)
                    conservative_uses.add(base)

        op_use_infos.append(
            _OpUseInfo(
                operation=op,
                generated=frozenset(generated),
                outs=frozenset(outs),
                ins=frozenset(ins),
                temps=frozenset(temps),
                uses=frozenset(conservative_uses),
            )
        )

    loop_spans: list[tuple[int, int]] = []
    loop_stack: list[int] = []
    for op in operations:
        if op.op_name == "scf.for":
            loop_stack.append(op.index)
        elif op.op_name == "scf.for.end" and loop_stack:
            loop_spans.append((loop_stack.pop(), op.index))

    for loop_begin, loop_end in loop_spans:
        for info in op_use_infos:
            if not (loop_begin < info.operation.index < loop_end):
                continue
            touched = info.generated | info.outs | info.ins | info.temps | info.uses
            for name in touched:
                if direct_start.get(name, loop_end + 1) < loop_begin:
                    direct_end[name] = max(direct_end.get(name, loop_end), loop_end)

    reusable_ops = {
        "hivm.hir.vadd",
        "hivm.hir.vsub",
        "hivm.hir.vmax",
        "hivm.hir.vmin",
        "hivm.hir.vor",
        "hivm.hir.vand",
        "hivm.hir.vmul",
        "hivm.hir.vexp",
        "hivm.hir.vln",
        "hivm.hir.vnot",
        "hivm.hir.vsel",
        "hivm.hir.vcmp",
    }
    for info in op_use_infos:
        if info.operation.op_name not in reusable_ops:
            continue
        killed_here = {
            name
            for name in info.ins | info.uses
            if direct_end.get(name) == info.operation.index and name not in temp_names
        }
        if not killed_here:
            continue
        for out in sorted(info.outs, key=_ssa_sort_key):
            if out in temp_names:
                continue
            for killed in sorted(killed_here, key=_ssa_sort_key):
                if alloc_extent.get(killed, -1) >= alloc_extent.get(out, -2):
                    uf.union(out, killed)
                    break

    records: list[LifetimeRecord] = []
    groups = uf.groups()
    value_to_group = {
        value: tuple(values)
        for values in groups.values()
        for value in values
    }
    for name in sorted(alloc_names, key=_ssa_sort_key):
        start = direct_start.get(name)
        end = direct_end.get(name)
        if start is None:
            start = end if end is not None else -1
        if end is None:
            end = start
        group = value_to_group[name]
        group_starts = [direct_start[v] for v in group if v in direct_start]
        group_ends = [direct_end[v] for v in group if v in direct_end]
        alloc_time = min(group_starts) if group_starts else start
        free_time = max(group_ends) if group_ends else end
        records.append(
            LifetimeRecord(
                ssa_name=name,
                alloc_time=alloc_time,
                free_time=free_time,
                direct_alloc_time=start,
                direct_free_time=end,
                group=group,
            )
        )

    return LifetimeAnalysis(path=path, records=tuple(records), operations=operations)
