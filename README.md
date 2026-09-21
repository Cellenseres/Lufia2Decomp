# Lufia2Decomp

Incremental source-code decompilation of *Lufia II: Rise of the Sinistrals*
for the Super Nintendo Entertainment System.

This repository reconstructs original game semantics as portable source. It is
not itself the Windows, Linux, Steam Deck, or Vita port; host integration and
enhancements belong in
[Lufia2SNESRecomp](https://github.com/Cellenseres/Lufia2SNESRecomp).

No ROM, proprietary extracted assets, or other copyrighted game data are
distributed here. A legally obtained supported game image is required for any
owner-run comparison against the original program.

## Current state

Decompilation is incremental. The first function, `$83:BBF3`, has a readable
semantic implementation, but remains `draft` until its complete CPU, stack,
and bus contract is proven against the original routine.

The portable CMake target is:

```cmake
Lufia2::Decomp
```

## Build

The standalone library builds with standard CMake:

```sh
cmake -S . -B build
cmake --build build --config Release
```

When consumed with `add_subdirectory`, the consumer links the same
`Lufia2::Decomp` target. The companion Lufia2SNESRecomp checkout accepts either
its pinned `lib/lufia2-decomp` submodule or a development checkout selected
with `-DLUFIA2_DECOMP_ROOT=<path>`.

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for repository boundaries and
[docs/DECOMP_STATUS.md](docs/DECOMP_STATUS.md) for status definitions.
