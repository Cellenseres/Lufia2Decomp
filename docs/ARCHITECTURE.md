# Architecture

Lufia2Decomp answers one question: what did the original Lufia II SNES program
do? It owns reconstructed semantics, conservative symbols, memory maps, and
function metadata.

The library target `lufia2_decomp` (alias `Lufia2::Decomp`) is portable C. Its
public API must remain independent of SNESRecomp ABI types and desktop/Vita
platform libraries. The consuming Lufia2SNESRecomp repository owns CPU-state
adaptation, generated `hle_func` selection, differential CPU/bus tests,
platform integration, and enhancements.

Only functions whose metadata status is `verified` are eligible for native
selection by a consumer. Consumers independently bind an address to a bridge;
verification without a binding is informational and never activates code.

Partial semantic front-ends may stop at named continuation boundaries when a
whole routine is not reconstructed yet. Function status lives only in
`metadata/functions.toml`; a partial front-end that is not yet a whole
replacement candidate with a representable entry/exit ABI has no entry there
and carries no status anywhere else.

The actor script virtual machines are reconstructed incrementally as explicit
dispatcher prefixes plus handler semantics. Indirect jump-table words remain
ROM-backed reads through the portable memory callback instead of copied ROM
data. This keeps the standalone source descriptive while preserving the exact
original dispatch behavior, including malformed or currently unseen opcodes.

CPU helpers have one direction of reuse: `core/cpu_internal.h` owns scalar
register, flag, arithmetic, transfer and stack primitives; `core/cpu_ops.h`
adapts them to the live M/X widths and explicit operand addresses. New
instruction adapters should reuse these primitives. The older fixed-width
idioms remain for existing modules; do not introduce competing arithmetic or
status implementations. Memory RMW adapters retain their verified byte-write
order rather than being folded into a superficially similar store helper.
`OpRepWidths` and `OpSepWidths` accept only width masks `$10/$20/$30`;
they are not arbitrary REP/SEP status-mask implementations.

These rules describe the legacy CPU-helper dialect. New and refactored code
follows the semantic layer in [CODE_STYLE.md](CODE_STYLE.md): readable C with
named constants and typed accessors, no emulated CPU state in converted
routines unless a caller still depends on it, and behavior identical to the
original game. Hardware-facing code sits behind a small interface so a recomp
can swap individual functions without touching game logic.

Shared implementation children belong in subsystem internal headers. The T14
section readers and party-list search are declared in `field/field_internal.h`
and `party/party_internal.h`; consumer differential tests explicitly include
those private headers. They do not expand the portable consumer API.

The field actor sprite builder exposes an optional `Lufia2FieldActorVisibility`
policy at its collection boundary. The stock entry passes no policy and retains
original memory accesses and CPU behavior. Padding wraps in 16 bits; the pure
world-point filter can only reject otherwise visible actors. Consumers own the
policy and context; this library never reads renderer globals or room metadata.
Sorting, uploads and OAM remain one implementation for both paths.

`Lufia2Memory` also carries an optional execution checkpoint and its context.
Inlined children inherit this observer through the same memory interface. A
checkpoint reports the CPU at an exact original instruction, with that
instruction's program bank; the inlined caller bank is restored afterward.
Observers may change registers, flags, DB/DP and memory before execution resumes.
Zero-initialized or aggregate memory interfaces with no observer retain the
original behavior. Consumers must rebuild against this extended structure and
initialize both optional fields when constructing it manually.

The derived-stat checkpoint is `$81:F4E2`, after the `$F4ED` child and before
register restore. The inventory-received checkpoint is `$81:F099`, after the
inventory-add child has returned and before testing the remainder. Neither
checkpoint changes original arithmetic, stat limits or inventory writes.

## Source layout

Sources are grouped by subsystem (`actor`, `battle`, `cave`, `field`, `item`,
`menu`, `party`, `system`, `text`, `title`, `world`) and a file is split only
along a semantic boundary, never by line count alone:

- The field event script VM is `field_event_script.c` (core loop),
  `field_event_actors.c` (actors, positions, points and the opcode
  dispatcher), `field_event_objects.c` (map-object bits, tiles, pending
  placement and pushing) and `field_event_conditions.c`. Helpers shared
  across these files are declared in `field/event_script_internal.h`.
- Capsule monster stats and skill learning (`$82:C4B3`, `$82:CD1F`)
  live in `party/capsule.c`; `menu/menu_screen.c` keeps the menu screens and
  their drawing helpers. Public declarations did not move.
- `actor/actor_primary.c`, `actor/object_vm.c` and `menu/menu_screen.c` stay
  whole: each is one VM or one family of screens sharing private helpers, and
  splitting them would only turn those helpers into cross-file symbols.

The semantic library remains `lufia2_decomp` (`Lufia2::Decomp`). The pure
resource decoder is a separate target, `Lufia2::ResourceFormat`, reused by
the semantic library and linked directly by the host for resource extraction.
