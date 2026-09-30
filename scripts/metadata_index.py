#!/usr/bin/env python3
"""Validate the Decomp metadata and regenerate the derived views.

metadata/functions.toml is the only source of function-level state (address,
portable symbol, source file, entry/exit M/X, status). This script checks it,
checks that metadata/symbols.toml holds no function or status data, and
rewrites counts in README.md and docs/DECOMP_STATUS.md, and the function list
in docs/FUNCTION_INDEX.md.

metadata/memory_map.toml is the only source of named WRAM locations. The
script validates it, regenerates the constants block in src/system/wram.h and
rejects WRAM_/DP_ constants in other headers that repeat a catalogued address
under an unlisted name.

With --check it writes nothing and fails when a generated block is stale.

Requires Python 3.11+ (tomllib); no third-party packages.
"""

from __future__ import annotations

import argparse
import difflib
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
INDEX = ROOT / "docs" / "FUNCTION_INDEX.md"
MEMORY_MAP = ROOT / "metadata" / "memory_map.toml"
WRAM_HEADER = ROOT / "src" / "system" / "wram.h"

STATUSES = ("verified", "draft", "identified", "disabled")
FIELDS = ("address", "name", "source", "entry_mx", "exit_mx", "status")
ADDRESS = re.compile(r"^[0-9A-F]{2}:[0-9A-F]{4}$")
SYMBOL = re.compile(r"^Lufia2[A-Za-z0-9_]+$")
MODE = re.compile(r"^M[01]X[01]$")
WRAM_ADDRESS = re.compile(r"^7[EF]:[0-9A-F]{4}$")
LOCATION_NAME = re.compile(r"^(?:[a-z][a-z0-9_]*|unk_7[EF][0-9A-F]{4})$")
LOCATION_FIELDS = {"address": str, "name": str, "width": int, "count": int,
                   "stride": int, "owner": str, "notes": str, "aliases": list}
LOCATION_REQUIRED = ("address", "name", "width", "owner", "notes")
CONSTANT = re.compile(
    r"^\s*(?:#define\s+(\w+)\s+|(\w+)\s*=\s*)(0x[0-9A-Fa-f]+)u?\b", re.M)


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
        if str(entry.get("address", "")).startswith(("7E:", "7F:")):
            errors.append(f"{where}: WRAM locations belong in {MEMORY_MAP.name}")
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


def load_memory_map() -> list[dict]:
    document = tomllib.loads(MEMORY_MAP.read_text(encoding="utf-8"))
    if document.get("format") != 1 or document.get("rom") != "lufia2-usa":
        raise MetadataError(f"{MEMORY_MAP.name}: expected format 1, rom lufia2-usa")
    locations = document.get("location", [])
    errors = []
    for index, entry in enumerate(locations):
        where = f"{MEMORY_MAP.name} location[{index}]"
        for key in LOCATION_REQUIRED:
            if key not in entry:
                errors.append(f"{where}: missing {key}")
        for key, value in entry.items():
            kind = LOCATION_FIELDS.get(key)
            if kind is None:
                errors.append(f"{where}: unknown key {key}")
            elif not isinstance(value, kind) or isinstance(value, bool):
                errors.append(f"{where}: {key} must be {kind.__name__}")
        if errors:
            continue
        address, name = entry["address"], entry["name"]
        if not WRAM_ADDRESS.match(address):
            errors.append(f"{where}: address must be 7E:xxxx or 7F:xxxx")
            continue
        if not LOCATION_NAME.match(name):
            errors.append(f"{where}: name must be lower_snake_case")
        if name.startswith("unk_") and name != "unk_" + address.replace(":", ""):
            errors.append(f"{where}: unknown name must be unk_{address.replace(':', '')}")
        width = entry["width"]
        count = entry.get("count", 1)
        stride = entry.get("stride", width)
        if width < 1 or count < 1 or stride < width:
            errors.append(f"{where}: need width >= 1, count >= 1, stride >= width")
        if not entry["owner"] or not entry["notes"]:
            errors.append(f"{where}: owner and notes must not be empty")
        if not all(isinstance(a, str) and a for a in entry.get("aliases", [])):
            errors.append(f"{where}: aliases must be names")
        start = int(address.replace(":", ""), 16)
        entry["_start"] = start
        entry["_end"] = start + stride * (count - 1) + width
        low = start - 0x7E0000
        limit = 0x100 if low < 0x100 else 0x2000 if low < 0x2000 else None
        if limit is not None and low + entry["_end"] - start > limit:
            errors.append(f"{where}: crosses the $7E:{limit:04X} boundary")
        entry["_constant"] = constant_name(entry)
    if errors:
        raise MetadataError("\n".join(errors))
    for key in ("name", "_constant"):
        for value, count in Counter(e[key] for e in locations).items():
            if count > 1:
                errors.append(f"{MEMORY_MAP.name}: duplicate {key.strip('_')} {value}")
    ordered = sorted(locations, key=lambda e: e["_start"])
    for first, second in zip(ordered, ordered[1:]):
        if second["_start"] < first["_end"]:
            errors.append(f"{MEMORY_MAP.name}: {first['name']} overlaps {second['name']}")
    if errors:
        raise MetadataError("\n".join(errors))
    return ordered


def constant_name(entry: dict) -> str:
    low = int(entry["address"].replace(":", ""), 16) - 0x7E0000
    prefix = "DP_" if 0 <= low < 0x100 else "WRAM_"
    return prefix + entry["name"].upper()


def constant_value(entry: dict) -> int:
    low = entry["_start"] - 0x7E0000
    return low if 0 <= low < 0x2000 else entry["_start"]


def wram_block(locations: list[dict]) -> str:
    groups = (
        ("Direct page; D = 0 in the field loop.", lambda v: v < 0x100),
        ("Low WRAM $7E:0100-$7E:1FFF, reached through the bank-$00 mirror.",
         lambda v: 0x100 <= v < 0x2000),
        ("Long WRAM addresses.", lambda v: v >= 0x2000),
    )
    lines = []
    for title, member in groups:
        chosen = [e for e in locations if member(constant_value(e))]
        if not chosen:
            continue
        if lines:
            lines.append("")
        lines.append(f"/* {title} */")
        for e in chosen:
            value = constant_value(e)
            digits = 2 if value < 0x100 else 4 if value < 0x10000 else 6
            lines.append(f"#define {e['_constant']} 0x{value:0{digits}x}u")
            if e.get("count", 1) > 1:
                lines.append(f"#define {e['_constant']}_COUNT {e['count']}u")
    return "\n".join(lines)


def check_header_constants(locations: list[dict]) -> None:
    by_address = {}
    for e in locations:
        by_address[("dp" if constant_value(e) < 0x100 else "wram",
                    constant_value(e))] = e
        by_address[("long", e["_start"])] = e
    errors = []
    for path in sorted((ROOT / "src").rglob("*.h")):
        text = path.read_text(encoding="utf-8")
        if path == WRAM_HEADER:
            text = re.sub(r"/\* memory-map:begin.*?memory-map:end \*/", "",
                          text, flags=re.S)
        for match in CONSTANT.finditer(text):
            name = match.group(1) or match.group(2)
            value = int(match.group(3), 16)
            if name.startswith("DP_") or "_DP_" in name:
                key = ("dp", value)
            elif name.startswith("WRAM_") and value < 0x2000:
                key = ("wram", value)
            elif 0x7E0000 <= value <= 0x7FFFFF:
                key = ("long", value)
            else:
                continue
            entry = by_address.get(key)
            if entry is None or name == entry["_constant"] or name in entry.get("aliases", []):
                continue
            errors.append(f"{path.relative_to(ROOT)}: {name} repeats "
                          f"${entry['address']} ({entry['_constant']}); use it or "
                          f"list {name} in its aliases")
    if errors:
        raise MetadataError("\n".join(errors))


def replace_block(text: str, name: str, body: str, path: Path,
                  html: bool = True) -> str:
    if html:
        begin = f"<!-- {name}:begin (scripts/metadata_index.py) -->"
        end = f"<!-- {name}:end -->"
    else:
        begin = f"/* {name}:begin (scripts/metadata_index.py) */"
        end = f"/* {name}:end */"
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
            INDEX: replace_block(INDEX.read_text(encoding="utf-8"),
                                  "metadata-index", index_block(functions),
                                  INDEX),
        }
        updates[STATUS] = replace_block(STATUS.read_text(encoding="utf-8"), "metadata-counts",
                                        counts_block(functions), STATUS)
        locations = load_memory_map()
        updates[WRAM_HEADER] = replace_block(
            WRAM_HEADER.read_text(encoding="utf-8"), "memory-map",
            wram_block(locations), WRAM_HEADER, html=False)
        check_header_constants(locations)
    except (MetadataError, tomllib.TOMLDecodeError, OSError) as error:
        print(f"metadata_index: {error}", file=sys.stderr)
        return 2
    stale = [p for p, text in updates.items()
             if p.read_text(encoding="utf-8") != text]
    if args.check:
        for path in stale:
            relative = path.relative_to(ROOT)
            print(f"metadata_index: {relative} is stale", file=sys.stderr)
            actual = path.read_text(encoding="utf-8").splitlines()
            expected = updates[path].splitlines()
            diff = list(difflib.unified_diff(
                actual, expected, fromfile=str(relative),
                tofile=f"{relative} (generated)", lineterm=""))
            for line in diff[:80]:
                print(line, file=sys.stderr)
            if len(diff) > 80:
                print(f"... {len(diff) - 80} diff lines omitted", file=sys.stderr)
        return 1 if stale else 0
    for path in stale:
        path.write_text(updates[path], encoding="utf-8", newline="\n")
        print(f"updated {path.relative_to(ROOT)}")
    print(counts_block(functions))
    print(f"{len(locations)} locations in `metadata/memory_map.toml`.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
