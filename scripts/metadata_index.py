#!/usr/bin/env python3
"""Validate the Decomp metadata and regenerate the derived status views.

metadata/functions.toml is the only source of function-level state (address,
portable symbol, source file, entry/exit M/X, status). This script checks it,
checks that metadata/symbols.toml holds no function or status data, and
rewrites the generated blocks in README.md (counts) and docs/DECOMP_STATUS.md
(index). With --check it writes nothing and fails when a block is stale.

Requires Python 3.11+ (tomllib); no third-party packages.
"""

from __future__ import annotations

import argparse
import re
import sys
import tomllib
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FUNCTIONS = ROOT / "metadata" / "functions.toml"
SYMBOLS = ROOT / "metadata" / "symbols.toml"
README = ROOT / "README.md"
STATUS = ROOT / "docs" / "DECOMP_STATUS.md"

STATUSES = ("verified", "draft", "identified", "disabled")
FIELDS = ("address", "name", "source", "entry_mx", "exit_mx", "status")
ADDRESS = re.compile(r"^[0-9A-F]{2}:[0-9A-F]{4}$")
SYMBOL = re.compile(r"^Lufia2[A-Za-z0-9_]+$")
MODE = re.compile(r"^M[01]X[01]$")


class MetadataError(ValueError):
    pass


def load_functions() -> list[dict]:
    document = tomllib.loads(FUNCTIONS.read_text(encoding="utf-8"))
    if document.get("format") != 1 or document.get("rom") != "lufia2-usa":
        raise MetadataError(f"{FUNCTIONS.name}: expected format 1, rom lufia2-usa")
    functions = document.get("function", [])
    errors = []
    for index, entry in enumerate(functions):
        where = f"{FUNCTIONS.name} function[{index}]"
        for key in FIELDS:
            if not isinstance(entry.get(key), str) or not entry[key]:
                errors.append(f"{where}: missing {key}")
        extra = set(entry) - set(FIELDS)
        if extra:
            errors.append(f"{where}: unknown keys {sorted(extra)}")
        if not ADDRESS.match(entry.get("address", "")):
            errors.append(f"{where}: address must be BB:AAAA (upper case)")
        if not SYMBOL.match(entry.get("name", "")):
            errors.append(f"{where}: name must be a portable Lufia2 symbol")
        if entry.get("status") not in STATUSES:
            errors.append(f"{where}: status must be one of {STATUSES}")
        for key in ("entry_mx", "exit_mx"):
            if not MODE.match(entry.get(key, "")):
                errors.append(f"{where}: {key} must look like M1X0")
        if not (ROOT / entry.get("source", "")).is_file():
            errors.append(f"{where}: source {entry.get('source')!r} not found")
    for key in ("address", "name"):
        for value, count in Counter(e.get(key) for e in functions).items():
            if count > 1:
                errors.append(f"{FUNCTIONS.name}: duplicate {key} {value}")
    if errors:
        raise MetadataError("\n".join(errors))
    return functions


def check_symbols(functions: list[dict]) -> None:
    document = tomllib.loads(SYMBOLS.read_text(encoding="utf-8"))
    addresses = {f["address"] for f in functions}
    names = {f["name"] for f in functions}
    errors = []
    for index, entry in enumerate(document.get("symbol", [])):
        where = f"{SYMBOLS.name} symbol[{index}]"
        if "status" in entry:
            errors.append(f"{where}: status belongs in {FUNCTIONS.name}")
        if entry.get("kind") == "function":
            errors.append(f"{where}: functions belong in {FUNCTIONS.name}")
        if entry.get("address") in addresses or entry.get("name") in names:
            errors.append(f"{where}: duplicates a function of {FUNCTIONS.name}")
    if errors:
        raise MetadataError("\n".join(errors))


def counts_block(functions: list[dict]) -> str:
    counts = Counter(f["status"] for f in functions)
    parts = [f"{counts[s]} {s}" for s in STATUSES]
    return (f"{len(functions)} functions in `metadata/functions.toml`: "
            + ", ".join(parts) + ".")


def index_block(functions: list[dict]) -> str:
    rows = ["| Address | Symbol | Status | Source |",
            "| --- | --- | --- | --- |"]
    for f in sorted(functions, key=lambda f: f["address"]):
        rows.append(f"| `${f['address']}` | `{f['name']}` | {f['status']} "
                    f"| `{f['source']}` |")
    return "\n".join(rows)


def replace_block(text: str, name: str, body: str, path: Path) -> str:
    begin = f"<!-- {name}:begin (scripts/metadata_index.py) -->"
    end = f"<!-- {name}:end -->"
    pattern = re.compile(re.escape(begin) + r".*?" + re.escape(end), re.S)
    if not pattern.search(text):
        raise MetadataError(f"{path.name}: missing {name} markers")
    return pattern.sub(lambda _: f"{begin}\n{body}\n{end}", text, count=1)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true",
                        help="fail instead of rewriting stale blocks")
    args = parser.parse_args()
    try:
        functions = load_functions()
        check_symbols(functions)
        updates = {
            README: replace_block(README.read_text(encoding="utf-8"),
                                  "metadata-counts", counts_block(functions),
                                  README),
            STATUS: replace_block(STATUS.read_text(encoding="utf-8"),
                                  "metadata-index", index_block(functions),
                                  STATUS),
        }
        updates[STATUS] = replace_block(updates[STATUS], "metadata-counts",
                                        counts_block(functions), STATUS)
    except (MetadataError, tomllib.TOMLDecodeError, OSError) as error:
        print(f"metadata_index: {error}", file=sys.stderr)
        return 2
    stale = [p for p, text in updates.items()
             if p.read_text(encoding="utf-8") != text]
    if args.check:
        for path in stale:
            print(f"metadata_index: {path.relative_to(ROOT)} is stale",
                  file=sys.stderr)
        return 1 if stale else 0
    for path in stale:
        path.write_text(updates[path], encoding="utf-8", newline="\n")
        print(f"updated {path.relative_to(ROOT)}")
    print(counts_block(functions))
    return 0


if __name__ == "__main__":
    sys.exit(main())
