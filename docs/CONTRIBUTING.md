# Contributing

Keep changes narrowly tied to original-game behavior. Write reconstructed
routines with the shared CPU helper layers described in
[CODE_STYLE.md](CODE_STYLE.md). Use conservative names until evidence
establishes a structure or purpose, and record original facts in `metadata/`
rather than consumer bridge names.

Every whole-function reconstruction is recorded in `metadata/functions.toml`
(address, portable symbol, source, entry/exit M/X, status), which is the only
place function status lives. A new entry starts as `draft`; promotion to
`verified` requires the documented differential contract against the
supported original program, including registers, flags, stack, memory, and bus
effects. `metadata/symbols.toml` holds non-function symbols only. Named WRAM
locations live in `metadata/memory_map.toml`; new code uses their generated
constants, and an unknown location gets a neutral `unk_` name there before
it gets a meaningful one (see [MEMORY_MAP.md](MEMORY_MAP.md)). After
changing the metadata, run `python3 scripts/metadata_index.py` to validate it
and regenerate the status views. Keep temporary test and debug harnesses
outside committed source.

Before submitting, build with warnings as errors and check the metadata:

```sh
cmake -S . -B build -DLUFIA2_WARNINGS_AS_ERRORS=ON
cmake --build build
python3 scripts/metadata_index.py --check
```

`.clang-format` describes the preferred layout for new code. Most existing
sources predate it and are not reformatted wholesale; format only the lines
you change. `scripts/check_format.sh [base]` reports clang-format differences
on lines changed since `base` (default `HEAD~1`); CI runs it as an advisory
check.

Two helper scripts support these rules; both only read the repository:

- `scripts/objequiv.py BASE NEW` compiles every `src/**/*.c` of two revisions
  with identical flags and compares the disassembly, with symbol names
  canonicalized, so a commit that only renames, introduces constants or edits
  comments can be shown to leave the object code unchanged. It needs `cc` and
  `objdump`; it exits 1 and lists the differing functions otherwise.
- `scripts/lint_raw.py` counts raw hex literals, `goto`, direct flag access,
  `Op*` calls and frame simulation per file in `src/`. `--check` fails when a
  count rises above `scripts/lint_baseline.json`, and `--diff REF` inspects only
  the lines added since `REF`. After a change that lowers counts, refresh the
  baseline with `--write-baseline`. Mark an
  intentional exception with a `lint: allow` comment on that line.

Do not add ROMs, extracted proprietary assets, generated game C, platform
dependencies, or a license chosen without the owner's decision.
