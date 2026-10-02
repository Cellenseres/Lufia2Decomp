# Reconstructed code style

This file defines how reconstructed 65816 routines are written in C. It knows
two styles. The semantic layer (see below) is the target for new and refactored
code. The CPU-helper dialect keeps routines that are not converted yet
faithful to the original instruction stream; new or meaningfully changed code in
that dialect uses the layers below instead of adding another private set of
helpers.

## Helper layers

| Header | Owns | Examples |
| --- | --- | --- |
| `src/core/memory_internal.h` | Bus access through `Lufia2Memory` and the 65816 address modes | `Read8`, `Write8`, `DirectAddress`, `AbsoluteIndexedAddress`, `LongIndexedAddress`, `Read16Direct`, `Write16Long` |
| `src/core/cpu_internal.h` | Primitive register, flag, arithmetic, transfer and stack behavior of `Lufia2CpuState`, plus the execution-result helpers | `SetNz8`, `LoadA8`, `LoadX16`, `Compare16`, `Adc8`, `Push8`, `PullDataBank`, `PackStatus`, `ExecutionReturned`, `ExecutionHandoff` |
| `src/core/cpu_ops.h` | Width-aware instruction adapters that follow the current M/X widths, with explicit operand addressing | `OpDp`, `OpAbsX`, `OpLda`, `OpSta`, `OpCmp`, `OpAdc`, `OpStepMem`, `OpLdx`, `OpRepWidths`, `OpSetDataBank`, `OpMoveNext` |
| `src/core/child_call.h` | Pushes the JSR or JSL return frame of a call site and runs the child through the consumer | `CallChildWithFrame` |

The CPU headers are intentional layers, not competing styles:
`cpu_ops.h` builds on `cpu_internal.h` and never re-implements a flag,
arithmetic or stack rule that the lower layer already owns.

## Rules for the CPU-helper dialect

These rules apply to routines that are still written in the dialect.

- Organize routines around proven game operations, with domain names and small
  semantic helpers where useful. Use ordinary structured C when it preserves
  the full observable contract.
- Use the `Op*` adapters from `cpu_ops.h` for required CPU behavior, including
  widths, flags and addressing modes. Do not remove or introduce adapters merely
  to change the abstraction level.
- Use the primitives from `cpu_internal.h` and `memory_internal.h` when the
  exact lower-level behavior is the point, for example a fixed-width access, a
  hand-ordered 16-bit read-modify-write or an explicit stack frame.
- Keep observable CPU state visible. Data bank, direct page, M/X widths,
  flags and stack contents stay in `Lufia2CpuState`, and memory goes through
  `Lufia2Memory`, whenever the original contract can observe them.
- Within the dialect, never introduce a higher-level helper that merges or
  reorders bus accesses, or that drops a register or flag the caller could
  observe. Routines converted to the semantic layer follow the conversion rule
  below instead.
- Name file-local helpers after the proven operation they perform; name CPU-only
  helpers after the instruction or addressing mode they model. Do not redefine a
  shared primitive (`Read8`, `SetNz8`, `LoadA8`, `Compare8`, ...).
- Leave exact continuation points as ROM PCs with `ExecutionHandoff`. Preserve
  useful ROM addresses in boundary comments and child call sites.

Known persistent WRAM fields belong in `metadata/memory_map.toml`. Record field
offsets may be named relative to the generated base. Reused DP scratch belongs
in local enums: the same bytes can mean different things in different routines.
Keep uncertain fields neutral and never cache bus-backed records in host structs.
Replace a raw literal with its catalogued name only where it is that address in
that addressing mode: not for a ROM address, a value or mask, an offset inside a
multi-byte entry, or a direct-page or data-bank context that differs from the
catalog.

## Semantic layer

New and refactored code should read as ordinary, natural C, as if a normal
developer had written the game logic.

- Use named constants and enums instead of raw addresses and magic numbers.
- Use typed accessors instead of raw WRAM offsets. Structs are for records the
  game really stores contiguously; per-slot arrays use slot views (below).
- Write small, well-named functions.
- Return results as real return values or out parameters instead of leaving
  them in emulated registers or flags.
- Prefer structured control flow over `goto` where it is equivalent.
- Keep comments short and about intent (why), not ROM-address banners.

The CPU-helper dialect stays allowed for routines that are not converted yet.
A converted routine does not touch emulated CPU state (carry, zero, negative,
A/X/Y widths, DP/DB) unless a caller still depends on it.

### Conversion rule

Behavior must stay identical to the original game. A function may be converted
when its inputs, outputs and memory side effects are preserved, including the
order of hardware-register, PPU and DMA accesses, and no caller depends on
leftover register or flag state. Otherwise convert the caller and the callee
together.

Convert one routine at a time. There is no mass conversion: each step has to
be checkable against the original on its own, and a bulk rewrite would hide
which change broke which behavior. Renames, constants and comments may be
applied broadly because they leave the object code unchanged.

The contract a converted function keeps is what the differential verification
compares: the exit values of A/X/Y, the flags, S, DB and DP where a caller can
observe them; the stack contents and any frames or return addresses it pushes,
modifies or unwinds; every WRAM and SRAM write; and the order and width of
every memory, PPU, DMA and other hardware-register access, including reads that
have side effects. Child calls keep their call frames, and checkpoints stay at
the original program counters.

Until a caller is converted, keep the original entry point as a thin shell that
sets up and exports the CPU state and delegates to a readable core function
with ordinary parameters and a return value. Only the interior of the core may
be restructured, and only where no access in the list above changes.

### Hardware-facing code

Rendering and other hardware-facing code is reached through a small interface,
so a recomp can replace individual functions (for example for 2D-HD rendering)
without touching game logic.

### Verification

A change is checked by:

- a build with warnings as errors;
- `scripts/metadata_index.py --check`;
- the maintainer's private ROM differential verification.

Commits that only rename, introduce constants or change comments must produce
identical object code.

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

This is the real layout, so it is kept. Byte arrays are indexed by the slot
number, word arrays by twice the slot number and 24-bit script records by three
times the slot number, and the arrays sit in different banks (the `$7E` mirror
reached through the data bank, the `$7F` arrays through long addressing). No
field of a slot is adjacent to another, so a packed `struct` would need a
copy in and out of WRAM, which changes the access order and widths and breaks
swapping a single function. Add accessors and view structs over the names in
`metadata/memory_map.toml`; deriving them from the catalog is the preferred way
to grow the set.

Create the view where the original loads the index register, and keep the
original access order. A view never caches values or copies WRAM into host
memory. `$83:BB93` (`Lufia2UpdateActorSlots`) and the `$83:C7F8`/`$83:D508`
front-ends are written this way; other routines adopt views one at a time, when they are touched for real work.

## REP and SEP

`OpRepWidths` and `OpSepWidths` model only the M and X width bits (`$20` and
`$10`). They are not general REP/SEP implementations: any other status bit in
the original operand, such as carry in `REP #$31`, must be applied
explicitly, and full status transfers use `PackStatus`/`UnpackStatus`.

## Existing code

Most routines predate `cpu_ops.h` and use the `cpu_internal.h` primitives
directly. That is a valid style for them. Legacy routines move to the `Op*`
adapters only when they are touched for real work, and each such change is
verified like any other semantic change. There is no mass conversion of the
legacy dialect; routines move to the semantic layer one at a time under the
conversion rule above.

One routine keeps its own interface on purpose: `$83:BBF3`
(`src/actor/player_update.c`) exposes the original verified bridge ABI
`Lufia2PlayerSlotSpecialMemory`, a 16-bit read-only accessor with a flag
result. It is not a template for new code.

## Comments

A comment is one short line: a useful ROM boundary, a non-obvious CPU quirk or,
in the semantic layer, the intent behind a step. Do not narrate obvious code. Explanations, evidence and
verification history belong in `docs/`, not in the source; prefer clear names
over prose.

## Formatting

`.clang-format` describes the preferred layout for new code. Existing
sources are not reformatted wholesale; see [CONTRIBUTING.md](CONTRIBUTING.md).
