# Reconstructed code style

This file defines how reconstructed 65816 routines are written in C. Its main
purpose is to keep one CPU-helper dialect in the tree: new and meaningfully
changed code uses the layers below instead of adding another private set of
helpers.

## Helper layers

| Header | Owns | Examples |
| --- | --- | --- |
| `src/core/memory_internal.h` | Bus access through `Lufia2Memory` and the 65816 address modes | `Read8`, `Write8`, `DirectAddress`, `AbsoluteIndexedAddress`, `LongIndexedAddress`, `Read16Direct`, `Write16Long` |
| `src/core/cpu_internal.h` | Primitive register, flag, arithmetic, transfer and stack behavior of `Lufia2CpuState`, plus the execution-result helpers | `SetNz8`, `LoadA8`, `LoadX16`, `Compare16`, `Adc8`, `Push8`, `PullDataBank`, `PackStatus`, `ExecutionReturned`, `ExecutionHandoff` |
| `src/core/cpu_ops.h` | Width-aware instruction adapters that follow the current M/X widths, with explicit operand addressing | `OpDp`, `OpAbsX`, `OpLda`, `OpSta`, `OpCmp`, `OpAdc`, `OpStepMem`, `OpLdx`, `OpRepWidths`, `OpSetDataBank`, `OpMoveNext` |

The two CPU headers are intentional layers, not competing styles:
`cpu_ops.h` builds on `cpu_internal.h` and never re-implements a flag,
arithmetic or stack rule that the lower layer already owns.

## Rules for new or meaningfully changed code

- Prefer the `Op*` adapters from `cpu_ops.h` when they represent the
  original instruction exactly, including its width and addressing mode.
- Use the primitives from `cpu_internal.h` and `memory_internal.h` when the
  exact lower-level behavior is the point, for example a fixed-width access, a
  hand-ordered 16-bit read-modify-write or an explicit stack frame.
- Keep observable CPU state visible. Data bank, direct page, M/X widths,
  flags and stack contents stay in `Lufia2CpuState`, and memory goes through
  `Lufia2Memory`, whenever the original contract can observe them.
- Never introduce a higher-level helper that merges or reorders bus accesses,
  or that drops a register or flag the caller could observe.
- A small file-local helper is fine when it only composes the shared layers
  and is named after the instruction or addressing mode it models, for example
  `LoadADirect` in `src/actor/actor_frontend.c`. It must not redefine a
  shared primitive (`Read8`, `SetNz8`, `LoadA8`, `Compare8`, ...).
- Leave exact continuation points as ROM PCs with `ExecutionHandoff`, and
  annotate reconstructed instructions with their ROM PC, for example
  `/* $83:C82E */`.

## Slot views

The game stores actor and object fields as parallel per-slot arrays in WRAM.
Reconstructed code never maps them onto packed C structs. Instead,
`src/actor/actor_slot_view.h` provides `Lufia2ActorSlotView`: a handle made
of the memory interface, the CPU (whose data bank the mirrored arrays follow)
and the index register value of the original access. Its accessors are
named after the fields in `metadata/memory_map.toml`
(`Lufia2ActorSlotState`, `Lufia2ActorSlotPrimaryTimer`, ...), and each one is
exactly one bus access. Fields that only have a neutral `unk_` name use the
generic `Lufia2ActorSlotReadMirrored`/`ReadLong` accessors with their
constant.

Create the view where the original loads the index register, and keep the
original access order. A view never caches values or copies WRAM into host
memory. `$83:BB93` (`Lufia2UpdateActorSlots`) and the `$83:C7F8`/`$83:D508`
front-ends are written this way; other routines adopt views when they are
touched for real work.

## REP and SEP

`OpRepWidths` and `OpSepWidths` model only the M and X width bits (`$20` and
`$10`). They are not general REP/SEP implementations: any other status bit in
the original operand, such as carry in `REP #$31`, must be applied
explicitly, and full status transfers use `PackStatus`/`UnpackStatus`.

## Existing code

Most routines predate `cpu_ops.h` and use the `cpu_internal.h` primitives
directly. That is a valid style for them. Legacy routines move to the `Op*`
adapters only when they are touched for real work, and each such change is
verified like any other semantic change. There is no mass conversion.

One routine keeps its own interface on purpose: `$83:BBF3`
(`src/actor/player_update.c`) exposes the original verified bridge ABI
`Lufia2PlayerSlotSpecialMemory`, a 16-bit read-only accessor with a flag
result. It is not a template for new code.

## Comments

A comment is one short line: the ROM address and what the routine does, or
the ROM PC of a reconstructed instruction. Explanations, evidence and
verification history belong in `docs/`, not in the source; prefer clear names
over prose.

## Formatting

`.clang-format` describes the preferred layout for new code. Existing
sources are not reformatted wholesale; see [CONTRIBUTING.md](CONTRIBUTING.md).
