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
1251 functions in `metadata/functions.toml`: 1251 verified, 0 draft, 0 identified, 0 disabled.
<!-- metadata-counts:end -->

[docs/DECOMP_STATUS.md](docs/DECOMP_STATUS.md) starts with the per-function
index and continues with the research and verification notes.
`python3 scripts/metadata_index.py` validates the metadata, including the
WRAM catalog `metadata/memory_map.toml`, and regenerates the generated
blocks here, in `docs/DECOMP_STATUS.md` and in `src/system/wram.h`; `--check`
only reports stale blocks.

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
| `LUFIA2_BUILD_TOOLS` | ON standalone, OFF as a subdirectory | Command-line tools in `tools/` |

None of the options changes global or consumer flags. CI builds with GCC and
Clang, warnings as errors, and runs
`python3 scripts/metadata_index.py --check`; it never needs a ROM.

Original-ROM differential verification is performed privately by the project
maintainers against a supported ROM image supplied by its owner.

## Tools

`lufia2-resource` inspects and decodes the compressed resources of a
supported ROM that you supply; it contains no game data, and nothing it
extracts belongs in this repository:

```sh
lufia2-resource check <rom>                  # decode all 680 resources
lufia2-resource list <rom>                   # table of addresses and sizes
lufia2-resource extract <rom> <id> <output>  # one decoded resource
lufia2-resource decode <stream> <output>     # a raw stream file
```

It uses the pure format decoder `include/lufia2/resource_format.h`, which
also serves consumers that need the format without the CPU adapter
`$80:8E9D`. There is no compressor: the original encoder is not
reconstructed, and any encoder added later only has to satisfy
`decode(stream) == data`, not reproduce the original bytes.

The decoder has its own target, `Lufia2::ResourceFormat`. `Lufia2::Decomp`
reuses that target, while hosts can link the smaller format target directly.

Lufia2SNESRecomp consumes this tree through `add_subdirectory`, either from
its pinned `lib/lufia2-decomp` submodule or from a development checkout given
with `-DLUFIA2_DECOMP_ROOT=<path>`.

## Further reading

- [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md): naming and promotion rules.
- [docs/CODE_STYLE.md](docs/CODE_STYLE.md): the CPU helper layers and how
  reconstructed code uses them.
- [docs/RECOMP_ENTRIES.md](docs/RECOMP_ENTRIES.md): the entries a consumer
  provides for the routines that are still `draft`.
- [docs/MEMORY_MAP.md](docs/MEMORY_MAP.md): address notation, the WRAM
  catalog `metadata/memory_map.toml` and its generated constants.
- [docs/EXTERNAL_RESEARCH.md](docs/EXTERNAL_RESEARCH.md): vetted external
  reverse-engineering references and the rules for using them.
- [docs/CAPTURE_RESEARCH.md](docs/CAPTURE_RESEARCH.md): gameplay-capture
  research notes.

Save-menu constructors preserve all four original descriptors, stored selection,
cursor setup andnested call frames throughverified native contracts.

Save-menu alternate selection and display services retain original list widths,
member retries, packed windows and live child frames in verified native contracts.

Verified name-entry and menu-record services preserve original glyphs,
live buffers, equipment records and child frames through native contracts.

Menu confirmation and price callers preserve original retries and returns.
Original item-name stack reads are explicitly ordered; menu M0 item-byte
reads use the native bridge within its verified context.

Selection gold and equipment updates retain original arithmetic, live
pointers and child returns without interpreting the reconstructed callers.

Menu inventory additions use shared possession lookup and original packed
slot updates without interpreting the reconstructed parent/child chain.

Menu selection and session services retain original input returns, cursor
frames, Capsule records, gold arithmetic and field-buffer ordering.

Capsule abilities and party spell registration retain original ROM tables,
form masks, saved records and the36-slot learning rules.

Original save-selection callers retain callback, mode and display contracts
while the remaining inline-table dispatcher stays an explicit child.

Capsule and received-item selection callers retain their original saved IDs,
stat transfers, inventory decisions and shared display returns.

Name-entry and item-selection loops retain original input repeats, glyph
records, cursor edges, closure conditions and scenario handoff.

Save/erase confirmation retains original SRAM operations and child frames.
Menu window and string drawing preserve original ordered data accesses.

Menu selection caches preserve the original six cursor records and scroll words.

Initial field object conditions and saved control regions retain original child calls.

Save-menu dispatch and character summaries retain original child services.

Party status callers preserve original drawing and upload services.

The shared party gauge has an independent verified entry.

Window edge patterns have four independent verified entries.

Field diagnostic flag/value drawing has nine verified original entries.

Five independent field-object entries reuse their verified shared cores.

Three shared actor, battle-message and field-reset entries are verified.

Original object-parameter, sprite-grid and prefixed-name entries are verified.

Nine shared field-OAM size and layout entries are verified.

Two original field palette children are verified;the field caller retains shared native cycles.

Eight original world-map motion services preserve shared arrival and fade behavior.

Six original world-view services preserve resource pointers and shared view state.

Six original world-frame callers preserve shared children and display transitions.

Six finite field-camera services preserve scene bytes, input and encounter state.

Battle HDMA dispatch and eighteen original command handlers retain real child frames.

Eight original HDMA pattern builders retain phase boundaries and ordered table writes.

Four original transition-HDMA routines retain carry-chained patterns and paired descriptors.

The primary-wave caller and existing NMI share one original table builder.

Field requests and the Event VM share the original animation-slot queue.
