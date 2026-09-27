# Contributing

Keep changes narrowly tied to original-game behavior. Use conservative names
until evidence establishes a structure or purpose, and record original facts
in `metadata/` rather than consumer bridge names.

Every whole-function reconstruction is recorded in `metadata/functions.toml`
(address, portable symbol, source, entry/exit M/X, status), which is the only
place function status lives. A new entry starts as `draft`; promotion to
`verified` requires the documented differential contract against the
supported original program, including registers, flags, stack, memory, and bus
effects. `metadata/symbols.toml` holds non-function symbols only. After
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

Do not add ROMs, extracted proprietary assets, generated game C, platform
dependencies, or a license chosen without the owner's decision.
