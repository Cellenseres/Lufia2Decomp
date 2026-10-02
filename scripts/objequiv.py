#!/usr/bin/env python3
"""Object-code equivalence check for rename-only / refactor-only commits.

Usage: scripts/objequiv.py [--repo DIR] [--opt -O2] BASE_REV NEW_REV
       scripts/objequiv.py --worktree-a DIR_A --worktree-b DIR_B      (two already extracted trees)

Both revisions are exported with `git archive` into temp dirs (the repository itself is never written
to), every src/**/*.c is compiled with identical flags (default -O2 -g0 -fno-ident, include/ + src/ on
the include path) and the disassembly of each TU is compared:
  * instruction text (objdump -dr --no-show-raw-insn), addresses stripped
  * symbol names (functions, callees in relocations, data) are canonicalised per TU by order of first
    appearance, so pure renames of functions/locals/constants/enumerators compare equal; use
    --strict-symbols to require identical names too
  * .rodata/.data/.bss sizes and bytes
Exit status 0 = every TU identical, 1 = differences (functions listed), 2 = compile failure.
It cannot prove behavior for changes that legitimately alter the compiled code; those need a differential test.
Requires cc and objdump (binutils).
"""
import argparse, concurrent.futures as cf, os, re, subprocess, sys, tempfile, shutil

def export(repo, rev, dest):
    os.makedirs(dest, exist_ok=True)
    p1 = subprocess.Popen(["git", "-C", repo, "archive", rev, "src", "include"], stdout=subprocess.PIPE)
    subprocess.check_call(["tar", "-x", "-C", dest], stdin=p1.stdout)
    if p1.wait():
        raise SystemExit(f"git archive failed for {rev}")

def sources(root):
    out = []
    for d, _, fs in os.walk(os.path.join(root, "src")):
        out += [os.path.relpath(os.path.join(d, f), root) for f in fs if f.endswith(".c")]
    return sorted(out)

def compile_obj(root, rel, opt, outdir):
    obj = os.path.join(outdir, rel.replace("/", "__") + ".o")
    cmd = ["cc", "-std=c11", opt, "-g0", "-fno-ident", "-fno-asynchronous-unwind-tables", "-w",
           f"-I{root}/include", f"-I{root}/src", "-c", os.path.join(root, rel), "-o", obj]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode:
        return rel, None, r.stderr
    return rel, obj, ""

HEX = re.compile(r"\b[0-9a-f]{3,}\b")
def disasm(obj):
    txt = subprocess.run(["objdump", "-dr", "--no-show-raw-insn", obj], capture_output=True, text=True).stdout
    funcs, cur, name = {}, None, None
    for line in txt.splitlines():
        m = re.match(r"^[0-9a-f]+ <(.+)>:$", line)
        if m:
            name = m.group(1); cur = funcs.setdefault(name, []); continue
        if cur is None or not line.strip() or line.startswith("Disassembly"):
            continue
        m = re.match(r"^\s*[0-9a-f]+:\s+R_\S+\s+(\S+)$", line)      # relocation line
        if m:
            cur.append("RELOC " + m.group(1)); continue
        m = re.match(r"^\s*[0-9a-f]+:\s+(.*)$", line)
        if m:
            cur.append(m.group(1))
    return funcs

def data_sections(obj):
    txt = subprocess.run(["objdump", "-s", "-j", ".rodata", "-j", ".data", "-j", ".bss", obj],
                         capture_output=True, text=True).stdout
    txt = "\n".join(l for l in txt.splitlines() if "file format" not in l)
    return re.sub(r"^\s*[0-9a-f]+ ", "", txt, flags=re.M)

def canon(funcs, strict):
    """Rename symbols by first appearance; also strip jump-target addresses inside the function bodies."""
    table = {}
    def sym(n):
        if strict: return n
        base = re.sub(r"[+-]0x[0-9a-f]+$", "", n)
        suffix = n[len(base):]
        if base.startswith("."):             # section-relative (.rodata+0x..) - keep
            return n
        table.setdefault(base, f"S{len(table)}")
        return table[base] + suffix
    out = []
    for fname, body in funcs.items():
        lines = []
        for ins in body:
            if ins.startswith("RELOC "):
                lines.append("RELOC " + sym(ins[6:])); continue
            ins = re.sub(r"<([^>]+)>", lambda m: "<" + sym(m.group(1)) + ">", ins)
            ins = re.sub(r"\b[0-9a-f]+ (?=<)", "", ins)               # absolute jump targets
            lines.append(ins)
        out.append((sym(fname), lines))
    return out

def build_tree(root, opt, jobs, workdir):
    os.makedirs(workdir, exist_ok=True)
    res, errs = {}, []
    with cf.ThreadPoolExecutor(jobs) as ex:
        for rel, obj, err in ex.map(lambda r: compile_obj(root, r, opt, workdir), sources(root)):
            if obj is None: errs.append((rel, err))
            else: res[rel] = obj
    return res, errs

def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("base", nargs="?"); ap.add_argument("new", nargs="?")
    ap.add_argument("--repo", default=os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    ap.add_argument("--opt", default="-O2"); ap.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
    ap.add_argument("--strict-symbols", action="store_true")
    ap.add_argument("--worktree-a"); ap.add_argument("--worktree-b")
    ap.add_argument("--keep", action="store_true")
    a = ap.parse_args()
    tmp = tempfile.mkdtemp(prefix="objequiv_")
    try:
        if a.worktree_a:
            ra, rb = a.worktree_a, a.worktree_b
        else:
            ra, rb = os.path.join(tmp, "a"), os.path.join(tmp, "b")
            export(a.repo, a.base, ra); export(a.repo, a.new, rb)
        oa, ea = build_tree(ra, a.opt, a.jobs, os.path.join(tmp, "oa"))
        ob, eb = build_tree(rb, a.opt, a.jobs, os.path.join(tmp, "ob"))
        for tag, errs in (("A", ea), ("B", eb)):
            for rel, e in errs: print(f"COMPILE FAIL [{tag}] {rel}: {e.strip().splitlines()[:1]}")
        if ea or eb: return 2
        only_a, only_b = sorted(set(oa) - set(ob)), sorted(set(ob) - set(oa))
        diff_tus, same = [], 0
        for rel in sorted(set(oa) & set(ob)):
            fa, fb = canon(disasm(oa[rel]), a.strict_symbols), canon(disasm(ob[rel]), a.strict_symbols)
            bad = []
            if len(fa) != len(fb): bad.append(f"function count {len(fa)} vs {len(fb)}")
            for (na, ba), (nb, bb) in zip(fa, fb):
                if ba != bb or (a.strict_symbols and na != nb):
                    bad.append(f"{na}/{nb}: {len(ba)} vs {len(bb)} insns")
            if data_sections(oa[rel]) != data_sections(ob[rel]): bad.append("data/rodata differ")
            if bad: diff_tus.append((rel, bad))
            else: same += 1
        print(f"TUs identical: {same}; different: {len(diff_tus)}; only in A: {len(only_a)}; only in B: {len(only_b)}")
        for rel, bad in diff_tus:
            print(" DIFF", rel); [print("    ", b) for b in bad[:8]]
        for r in only_a: print(" only in A:", r)
        for r in only_b: print(" only in B:", r)
        return 1 if (diff_tus or only_a or only_b) else 0
    finally:
        if not a.keep: shutil.rmtree(tmp, ignore_errors=True)

if __name__ == "__main__":
    sys.exit(main())
