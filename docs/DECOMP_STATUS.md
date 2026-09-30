# Decompilation status

| Status | Meaning |
| --- | --- |
| `identified` | Boundary/address known; no semantic implementation yet. |
| `draft` | Readable semantic source exists; equivalence is not yet proven. |
| `verified` | The required decomp verification passed; consumers may select it. |
| `disabled` | Retained for reference but deliberately excluded. |

`verified` describes the reconstruction, not runtime use: whether a verified
function replaces the original at runtime is decided by the consumer's
`recomp/decomp_bindings.toml`.

## Function counts

Generated from `metadata/functions.toml` by `scripts/metadata_index.py`; edit
the metadata, not these counts.

<!-- metadata-counts:begin (scripts/metadata_index.py) -->
255 functions in `metadata/functions.toml`: 255 verified, 0 draft, 0 identified, 0 disabled.
<!-- metadata-counts:end -->

The complete function list is in [FUNCTION_INDEX.md](FUNCTION_INDEX.md).

## Current checkpoint

The full Windows Release verifier passes **344 independent jobs**. The normal
application build also passes. The consumer selects 188 functions, including
the complete command-collection and turn-execution callers. The generated CFG
contains 1,403 nodes; no previously eligible AOT function was lost.

Verification uses the original ROM interpreter as the reference. It checks CPU
state, complete WRAM and, where needed, the order of memory and register writes.
Tests also force child calls to unwind and inject deliberate mistakes to check
that the comparisons can detect them. Detailed run logs belong in the consumer's
worklog rather than this status page.

## Battle commands and targets

The battle lifecycle, main loop, turn execution and command-selection callers
are reconstructed. Unknown children remain explicit calls with their original
stack frames. A verified caller does not make those children verified.

The latest work covers command collection, party action choices, spell/item/IP
submenus and target selection. It retains the original polling, scrolling,
backtracking, invalid-target skipping and side changes. Target cancellation can
remove a mark and continue; pair selection waits for exactly two party members.

| Slice | ROM comparison cases | Result |
| --- | ---: | --- |
| Main-loop children (BL6.1) | 8,576 | pass |
| Turn/command control (BL6.2) | 5,120 + 128 control probes | pass |
| Command collection (BL6.3) | 4,159 | pass |
| Party action selection (BL6.4) | 4,151 + 128 stack-read probes | pass |
| Action submenus (BL6.5) | 4,119 | pass |
| Target selection (BL6.6) | 8,207 | pass |
| Target/confirmation helpers (BL6.7) | 13,326 | pass |
| Command/turn runtime bridges (BL6.8) | 15,615 ABI checks | pass |
| Item/spell command lists (BL6.9) | 4,100 | pass |
| Command display helpers (BL6.10) | 6,666 | pass |
| Party names/window uploads (BL6.11) | 8,196 | pass |
| Result windows (BL6.12) | 6,159 | pass |
| Experience/gold/results (BL6.13) | 5,746 | pass |
| Party growth/received loot (BL6.14-15) | 8,711 + 4,098 | pass |
| Turn display (BL6.16) | 10,252 | pass |
| Status support/math entries (BL6.17) | 22,528 | pass |
| Status recovery/expiry (BL6.18) | 5,162 | pass |
| Message display/state (BL6.19, expanded) | 17,415 | pass |
| Hardware product/message length (BL6.20) | 12,288 | pass |
| Status gauges/division bus order (BL6.21) | 24,576 | pass |
| Message renderer/resource bus order (BL6.22) | 5,124 + 5,440 | pass |
| Glyph renderer/font helpers (BL6.23) | 32,771 | pass |

Target coordinates, all four cursor variants, command frame upkeep and the
confirmation prompt are also reconstructed. They retain original coordinate
wrapping, hardware register accesses, title drawing and input waits. Item and
spell lists also preserve availability checks, MP limits and hardware division.
Window setup, cleanup, party names and tilemap queues are reconstructed.
Full queues retain the original BRK handoff. Results cover rewards, party and
capsule progression, stat gains and item reception, including the original
calibration BRK and inventory remainders. Status recovery, timed effects,
gauges and shared message rendering, including aligned and shifted glyphs,
are reconstructed. The resource loader
also preserves the decompressor's original byte-write order. Sound and the
remaining effect children remain explicit calls.

The two new runtime bridges require A8, X/Y16, DB `$97`, DP zero and native binary
arithmetic. Other states use the original interpreter. Their tests cover stack
frames, child unwinds and three host-return modes, detecting ten deliberate
bridge mistakes. The other recent command helpers remain unbound. Declarations
and metadata give each entry's contract.

Some unusual original exits matter:

- Command collection can jump to `$81:8855` with saved frames still on the stack.
- Malformed command selection can reach BRK `$81:C8BC`; its successful branch
  starts inside that instruction's operand at `$C8BD`.
- Party action selection can reach BRK `$81:CF78` or the original `$D12C` self-loop.
- Unreachable submenu blocks at `$D1E0` and `$D3FE` remain unreachable.
- Stack-relative word operands wrap within bank zero, including their high byte.

The reconstruction preserves these behaviors. Rendering readiness, widescreen
policy and optional performance patches are consumer responsibilities.

## Text and menus

The text engine includes character rendering, dictionary expansion, windows,
prompts and conditional expressions. Unsupported script paths hand back to the
original code at explicit boundaries.

Window preparation deliberately stops at `$80:C2A1` before the frame waits,
keeping the outstanding frames intact. Window construction retains both width
interpretations at `$C431`; it does not repair the unusual original path.
Measurement, glyph clearing, row upload and item-possession helpers have their
own ROM comparisons. Conditional expressions retain all eight comparators and
original wraparound behavior.

Menu strings, windows, inventory, equipment and capsule helpers are listed in
the function index. Runtime use is decided by the consumer's bindings file.

## Ancient Cave

The floor generator `$83:9E31` is verified and unbound. It uses A8, X/Y16,
DP zero and a DB that maps low WRAM. Verification covers 16,384 floors, five
helper groups of 4,096 cases each, and APU-return probes.

The APU handshake and map loader remain explicit children. Binding this entry
still needs a consumer bridge and ABI tests; its semantic verification alone
is insufficient. Original generator loops and degenerate-map behavior are
preserved. Host decoding or caching belongs in a separate patch layer.

## Actors, field and shared helpers

Actor routines use slot views and generated memory-map constants while keeping
the original addressing and access order. The field loop, scene NMIs, scrolling,
world-map helpers and shared resource/menu/math routines are covered by the
consumer's differential and bridge suites. The index is the source of truth for
individual functions.

Further battle work closes the remaining child dependencies before considering
new runtime bindings. See [ARCHITECTURE.md](ARCHITECTURE.md) for the boundary
model and [CAPTURE_RESEARCH.md](CAPTURE_RESEARCH.md) for scene evidence.
