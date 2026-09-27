# Lufia2Decomp

Portable semantic reconstruction of the game logic of *Lufia II: Rise of the
Sinistrals* (SNES, USA), written as C for use by SNES recompilation and
tooling. Its main consumer is
[Lufia2SNESRecomp](https://github.com/Cellenseres/Lufia2SNESRecomp).

## What this is not

- Not a ROM repository: no ROM, extracted assets or other copyrighted game
  data are distributed here, and none are needed to build the library.
- Not a matching disassembly: the C reproduces observable behavior, not the
  original instruction bytes, and nothing here reassembles the game.
- Not the PC or handheld port: host integration, runtime bridges and
  enhancements belong to the consumer.

## Architecture

```text
original ROM behavior
        |  differential verification (in the consumer, against the owner's ROM)
        v
Lufia2Memory / Lufia2CpuState adapter layer
        |
        v
portable semantic C  (this repository, target Lufia2::Decomp)
        |
        v
consumer integration (Lufia2SNESRecomp bridges and generated code)
```

Each routine is reconstructed against the original program's contract. Where
data bank, direct page, flags, M/X widths or stack frames are observable parts
of that contract, the semantic C keeps them visible through `Lufia2CpuState`
and the byte-level `Lufia2Memory` callbacks instead of hiding them. A routine
may stop at an exact ROM PC (a continuation boundary) and let the consumer
finish in the original code.

Verification and runtime use are separate. A function is `verified` once its
original-ROM differential verification passed; whether it replaces the
original at runtime is a separate consumer decision recorded in the
consumer's `recomp/decomp_bindings.toml`.

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for repository boundaries.

## Status

`metadata/functions.toml` is the only source of function-level state: address,
portable symbol, source file, entry/exit M/X and status. Current totals:

<!-- metadata-counts:begin (scripts/metadata_index.py) -->
175 functions in `metadata/functions.toml`: 175 verified, 0 draft, 0 identified, 0 disabled.
<!-- metadata-counts:end -->

[docs/DECOMP_STATUS.md](docs/DECOMP_STATUS.md) starts with the per-function
index and continues with the research and verification notes.
`python3 scripts/metadata_index.py` validates the metadata and regenerates
both generated blocks; `--check` only reports stale blocks.

## Build

The standalone library needs CMake 3.20+ and a C11 compiler; it has no other
dependencies and does not need the ROM:

```sh
cmake -S . -B build
cmake --build build --config Release
```

This builds the static library `lufia2_decomp` (alias `Lufia2::Decomp`).

| Option | Default | Effect |
| --- | --- | --- |
| `LUFIA2_ENABLE_WARNINGS` | ON standalone, OFF as a subdirectory | `-Wall -Wextra -Wpedantic` (GCC/Clang) or `/W4` (MSVC) on `lufia2_decomp` only |
| `LUFIA2_WARNINGS_AS_ERRORS` | OFF | Adds `-Werror` or `/WX` to those warnings |
| `LUFIA2_ENABLE_SANITIZERS` | OFF | AddressSanitizer and UBSan (GCC/Clang); whatever links the library also links the sanitizer runtimes |

None of the options changes global or consumer flags. CI builds with GCC and
Clang, warnings as errors, and runs `python3 scripts/metadata_index.py
--check`; it never needs a ROM.

There are no tests in this repository; differential verification runs in the
consumer (`decomp-verify` target) against a supported ROM image supplied by
its owner.

Lufia2SNESRecomp consumes this tree through `add_subdirectory`, either from
its pinned `lib/lufia2-decomp` submodule or from a development checkout given
with `-DLUFIA2_DECOMP_ROOT=<path>`.

## Further reading

- [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md): naming and promotion rules.
- [docs/CODE_STYLE.md](docs/CODE_STYLE.md): the CPU helper layers and how
  reconstructed code uses them.
- [docs/MEMORY_MAP.md](docs/MEMORY_MAP.md): address notation.
- [docs/EXTERNAL_RESEARCH.md](docs/EXTERNAL_RESEARCH.md): vetted external
  reverse-engineering references and the rules for using them.
- [docs/CAPTURE_RESEARCH.md](docs/CAPTURE_RESEARCH.md): gameplay-capture
  research notes.
