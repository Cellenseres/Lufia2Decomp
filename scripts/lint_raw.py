#!/usr/bin/env python3
"""Ratchet lint against new assembly-style constructs in src/ (never writes to the repo).

Counted per file (comments/strings stripped; #define and enum-definition lines are exempt because that is
where named addresses are supposed to live):
  raw_addr  hex literals >= 0x100 on a code line            (should become WRAM_*/DP_*/ROM_* names)
  goto      goto statements
  flag      cpu->carry|zero|negative|overflow|decimal|irq_disable
  op        Op<Upper>... ( emulation-style op calls
  frame     Simulate{Jsl,Jsr,Rtl,Rts}Frame
Modes:
  --write-baseline [FILE]  snapshot the counts of the working tree
  --check [FILE]           exit 1 if any per-file count rose; a new file must be clean
  --diff REF               inspect only lines added since REF (`git diff REF -- src`); any hit exits 1
  --report                 totals and the worst files
The default baseline is scripts/lint_baseline.json. A line carrying `lint: allow` is skipped.
"""
import argparse, json, os, re, subprocess, sys, collections

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_BASE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "lint_baseline.json")
PATS = {
    "raw_addr": re.compile(r"\b0[xX](?:[0-9a-fA-F]{3,})[uUlL]*\b"),
    "goto":     re.compile(r"\bgoto\b"),
    "flag":     re.compile(r"->\s*(?:carry|zero|negative|overflow|decimal|irq_disable)\b"),
    "op":       re.compile(r"\bOp[A-Z]\w*\s*\("),
    "frame":    re.compile(r"\bSimulate(?:Jsl|Jsr|Rtl|Rts)Frame\b"),
}
EXEMPT = re.compile(r"^\s*#\s*define\b|^\s*[A-Z][A-Z0-9_]*\s*=\s*(?:0[xX][0-9a-fA-F]+|[0-9]+)[uUlL]*\s*,?\s*(?:/\*.*)?$")
ALLOW_MARK = re.compile(r"lint:\s*allow")

def clean(line):
    line = re.sub(r"/\*.*?\*/", "", line)
    line = re.sub(r"//.*", "", line)
    return re.sub(r'"(?:\\.|[^"\\])*"', '""', line)

def count_lines(lines):
    c = collections.Counter(); in_block = False
    for ln in lines:
        if in_block:
            if "*/" in ln: ln = ln.split("*/", 1)[1]; in_block = False
            else: continue
        if "/*" in ln and "*/" not in ln:
            ln = ln.split("/*", 1)[0]; in_block = True
        if ALLOW_MARK.search(ln): continue
        s = clean(ln)
        if EXEMPT.match(s): continue
        for k, p in PATS.items():
            c[k] += len(p.findall(s))
    return c

def scan_tree(repo):
    out = {}
    for d, _, fs in os.walk(os.path.join(repo, "src")):
        for f in fs:
            if f.endswith((".c", ".h")) and not f.endswith(("cpu_ops.h", "cpu_internal.h", "memory_internal.h", "child_call.h")):
                p = os.path.join(d, f)
                out[os.path.relpath(p, repo)] = dict(count_lines(open(p, errors="replace").read().splitlines()))
    return out

def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--repo", default=REPO)
    g = ap.add_mutually_exclusive_group(required=True)
    g.add_argument("--write-baseline", nargs="?", const=DEFAULT_BASE)
    g.add_argument("--check", nargs="?", const=DEFAULT_BASE)
    g.add_argument("--diff"); g.add_argument("--report", action="store_true")
    a = ap.parse_args()
    if a.write_baseline:
        data = scan_tree(a.repo); json.dump(data, open(a.write_baseline, "w"), indent=1, sort_keys=True)
        tot = collections.Counter(); [tot.update(v) for v in data.values()]
        print("baseline written:", a.write_baseline, dict(tot)); return 0
    if a.report:
        data = scan_tree(a.repo); tot = collections.Counter(); [tot.update(v) for v in data.values()]
        print("totals:", dict(tot))
        for k in PATS:
            worst = sorted(data.items(), key=lambda kv: -kv[1].get(k, 0))[:5]
            print(k, [(f, v.get(k, 0)) for f, v in worst])
        return 0
    if a.check:
        base = json.load(open(a.check)); cur = scan_tree(a.repo); bad = []
        for f, v in cur.items():
            b = base.get(f, {})
            for k in PATS:
                if v.get(k, 0) > b.get(k, 0):
                    bad.append(f"{f}: {k} {b.get(k, 0)} -> {v.get(k, 0)}" + ("  (new file must be clean)" if f not in base else ""))
        print("\n".join(bad) if bad else "lint_raw: no increase vs baseline"); return 1 if bad else 0
    diff = subprocess.run(["git", "-C", a.repo, "diff", "-U0", a.diff, "--", "src"], capture_output=True, text=True).stdout
    hits, cur = [], None
    for ln in diff.splitlines():
        if ln.startswith("+++ b/"): cur = ln[6:]
        elif ln.startswith("+") and not ln.startswith("+++") and cur and cur.endswith((".c", ".h")):
            c = count_lines([ln[1:]])
            for k, n in c.items():
                if n: hits.append(f"{cur}: +{k} x{n}: {ln[1:].strip()[:100]}")
    print("\n".join(hits) if hits else "lint_raw: added lines are clean"); return 1 if hits else 0

if __name__ == "__main__":
    sys.exit(main())
