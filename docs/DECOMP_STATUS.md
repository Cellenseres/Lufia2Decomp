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
347 functions in `metadata/functions.toml`: 347 verified, 0 draft, 0 identified, 0 disabled.
<!-- metadata-counts:end -->

The complete function list is in [FUNCTION_INDEX.md](FUNCTION_INDEX.md).

## Current checkpoint

The full Windows Release verifier passes **474 independent jobs**. The normal
application build also passes. The consumer selects all 347 verified functions,
including song-load and fade-out after the MSU migration. The generated CFG
contains 1,451 nodes, and every prior native entry remains covered.

Verification uses the original ROM interpreter as the reference. It checks CPU
state, complete WRAM and, where needed, the order of memory and register writes.
Tests also force child calls to unwind and inject deliberate mistakes to check
that the comparisons can detect them. Detailed run logs belong in the consumer's
worklog rather than this status page.

## Shared party stat totals

The checkpoint at `$81:F576` reports the completed original totals before RTS,
with X pointing to the stat block. It covers direct and far entry, equipment
preview, level-up and wrapped derived stats. The earlier `$81:F4E2` checkpoint
remains available. The reconstruction preserves the original uncapped values;
consumer adjustments belong to subscribers compiled for both native and
interpreter execution.

35,840 original-ROM states and 27,648 native entries pass, including actual
equipment and level-up JSR frames. There are no positive native fallbacks.
All 1,025 subscriber checks and 11 injected faults pass.

## Rendering a pair of layers

`$83:8E76` renders the pending rectangle in layers zero and one through the
verified region renderer. It retains the live child RTL frame, the original
layer increments and exact continuations after 4,096 child calls or 262,144
cells. The region renderer also follows the live RTS frames of its coordinate
and map-offset helpers. Native continuations export the actual program bank.

8,257 ROM and native cases pass without a positive entry fallback. CPU state,
complete WRAM, every data read and write, and mutable hardware state agree.
The checks include 17 changed child returns, eight additional symmetric
bus-observer frame rewrites and eight independently counted loop limits.
All 1,536 unsupported native entries and 17 injected faults are detected.
The pair is selected as a standalone replacement.

## Layer scroll and coordinate scaling

`$83:8D42` sets the four layer scrolls, including centered coordinates, zero
scroll and signed scaling. Its six helpers at `$83:8DDA`, `$83:8DF0`,
`$83:8E01`, `$83:8E2B`, `$83:8E09` and `$83:8E1A` retain ROM-backed shift
lookup, register-width changes and original arithmetic. Disabled layers still
update the coarse cache; its X and Y words overlap in the original memory.

2,067,760 original-ROM and native cases pass without a positive entry fallback.
The checks compare CPU state, full WRAM, every data read and write, and mutable
hardware state. They include all caller M/X combinations, 3,396 exact shift
continuations, 64 changed child returns and overwritten saved status and bank.
All 9,600 unsupported entries and 52 injected faults are detected. The complete
parent and six helpers are selected together, retaining compiled caller coverage.

## Object state bits

`$83:8AF7` computes an object's byte index and ROM-backed mask; `$83:8AC9`,
`$83:8AD5` and `$83:8AE5` test, set and clear the state bit. The reconstruction
retains the hidden direct-page byte in the index, the original stack frames,
and the different flag results of testing versus writing. If scratch overwrites
a child's return frame, the parent follows its live return address through an
exact interpreter continuation.

532,480 original-ROM and native cases pass without a positive entry fallback.
They cover every direct-page and accumulator word and 8,192 deliberate stack
aliases. All data reads and writes are compared; 2,301 changed child returns
and 6,144 unsupported native entries are proven. All 24 injected faults are
caught, including duplicate reads in both semantic and native tests. The four
roots are selected as standalone replacements. Event-adapter sharing remains
a separate composed verification task.

## Clipped region rendering

`$83:8E85` clips an object rectangle against the layer viewport and writes
each visible metatile as four tiles into the original tilemap ring. It retains
the signed-byte clipping branches, source-cell attributes, carry between row
additions, ring wrapping, child stack frames, and every word-decrement write.
Long loops can resume in the interpreter at `$83:8F7F` after exactly 262,144
cells, with the original CPU and memory state.

8,736 original-ROM and native cases pass without a positive entry fallback.
They cover four layers, bank crossings, all sixteen scroll pixel phases,
signed-coordinate edges, and overwritten saved frames or counters. The native
entry passes 1,536 unsupported-state guards; all 23 injected faults are caught.
The existing event-call adapter separately passes 8,720 cases with complete
outer call and return frames. The renderer is selected as a standalone
replacement.

## Region coordinate conversion

`$83:9000` adds fifteen before the shared `$83:9004` conversion. The latter
uses its entry Negative flag to select the direct page, then shifts four times.
The reconstruction retains wrapping arithmetic, every flag and either index
width. The existing region renderer shares these bodies and their original
child frames. Neither root includes the following frame-wait loop.

393,216 original-ROM and native cases pass with no positive fallback: every
accumulator word, both index widths, all direct-page words and the independent
Negative flag are covered. Both native entries pass 2,304 unsupported-state
guards together, and all 12 injected faults are caught. Both roots are selected
as standalone replacements.

## Clearing tile IDs in object rectangles

`$83:8A6F` clears the tile IDs in an object's rectangle while retaining cell
attributes. It preserves the separate data-bank width read, live width reload
on every row, wrapping word counters, and every original memory write. After
262,144 cells it can hand control to the interpreter at `$83:8AAC`, with the
exact original state, so long and self-modifying loops remain executable.

8,224 complete ROM and native cases pass without a positive entry fallback.
Cases include bank crossings, zero width and height, a rectangle overwriting
its own size fields, and wrapped direct pages. The native guard passes 1,536
unsupported entries; all 18 injected faults are detected. The root is selected
as a standalone replacement. Event-script sharing remains a separate task.

## Pending object registration and tile replacement

`$83:F86B` registers a pending object's coordinates, marks its attributes,
preserves tiles from an object already covering the cell, and saves and replaces
the affected map tiles. The original early exit, two separate row decrements,
data bank and status restore, and all nine pushed child calls are retained.

10,250 complete ROM and native cases pass with no positive fallback. Tests
exercise every direct child unwind, an unwind inside attribute marking, all
three tile-copy paths, and composition with the actual children. CPU state,
full WRAM and ordered writes agree. The bridge passes 1,536 unsupported-entry
guards and all 24 fault probes. The complete parent is selected as a standalone
replacement; existing event-script composition is a separate integration task.

## Pending-object tile and coordinate helpers

`$83:F9F7` converts the packed row/column in A/B to a word-cell offset with
the original hardware multiplier and index-width truncation. `$83:F9D9`
adds the selected layer's cell-data base, retaining its child and stack
frames. `$83:F91F` copies the tile bits of cell X to cell Y while keeping
the destination attributes. Actor and event callers share these bodies.

352,256 complete ROM cases and 352,256 native calls pass, with no positive
fallback. Tests exhaust the multiplier inputs and tile-word values and cover
overlapping source/destination/scratch addresses, word bank crossings and
MMIO order. The three roots pass 4,224 unsupported-state guards and all 15
fault probes. All three are selected as standalone replacements.

## Inherited checkpoints and map completion

Optional memory-interface checkpoints now reach derived party stats at
`$81:F4E2` and item receipt at `$81:F099`, including inlined callers. The
original uncapped stats and inventory remainder writes remain unchanged.
Capsule growth retains each original memory shift and its high-first word
writes. The consumer rejects unsupported X8 entry at `$82:C261`.

81,154 original-ROM cases cover the two checkpoints, cross-bank callers,
both inventory child unwinds and exhaustive byte/word memory shifts.
15,362 native calls pass without positive fallback; 6,144 unsupported-state
guards and all 24 injected faults pass. The always-built consumer event layer
supports eight observers per event and passes 946 checks without the decomp
library, including subscriber mutations and native-to-interpreter map completion.
Spell-shop setup is now selected after its consumer hook migration.

## Spell-shop prices and windows

`$82:D905` loads the selected spell, adjusts its price, stores the two price
words and sets up all original text, windows and HDMA children. The original
third purchase-price byte remains unchanged. `$82:9918` halves price X with
rounding up, retaining the original party/item condition, entry-width Y and
status. Existing shop rows use this independently verified shared helper.

81,930 original-ROM runs include all 65,536 half-price inputs, ten child
unwinds, six data-bank/Direct-Page combinations and both entry widths.
90,129 native calls pass with no positive fallback, 15 redirects and 2,304
unsupported-state guards. All 30 semantic/event/bridge fault variants are
caught. The optional checkpoint at `$82:D922` follows both word stores;
the always-built event layer also passes 47 native/interpreter pairs without
the decomp library, including CPU steering and subscriber isolation.

## Song loading and music commands

`$80:941A` loads a song resource, validates its signature, selects the sample
upload mode and processes all 32 sample slots. Both the direct-upload and
lookup/cache paths preserve original memory access order, scratch bytes and
flags. `$80:93FE` plays a successfully loaded song; `$80:9692` and `$80:9601`
send the original fade-out and music-volume commands. APU handshakes remain
explicit interpreter children, including the waits at `$80:9A0A`.

40,985 original-ROM cases cover all four entry width combinations, resource
rejection, both sample paths, 24 original child sites, a requested volume call
and 8,192 composed player/loader cases. The bridges pass 49,177 native calls
with no positive fallback, 75 redirects and 3,072 guards. All 39 injected
semantic, event and bridge errors are detected. The always-built song event
can request a guest call that returns to the same checkpoint, allowing the
consumer to restore the song ID before the original store. Native/interpreter
events also match across 30 pairs without the decomp library.

## Save files and random seeding

`$80:9099` loads a game file and preserves the original success/error carry;
`$80:90C9` saves it using the original packing children. `$80:914B` decrypts
the complete 2 KiB slot, and `$80:9184` encrypts and writes it with the original
random stream, seed byte and word checksum. The checksum and slot-address
helpers at `$80:90FC` and `$80:91D3`, plus random seeding at `$80:82E7`, retain
all original flags, widths, scratch bytes, wrap behavior and child frames.
Packing and unpacking children remain explicit dispatch boundaries.

34,447 original-ROM runs compare CPU state, all WRAM, the 8 KiB SRAM backing,
ordered writes and SRAM reads. The bridges pass 34,895 native calls with no
positive fallback, 75 redirects and 8,064 unsupported-state guards. All 37
semantic/event fault variants are caught. The always-built file-entry event
distinguishes a nested header read during loading from a menu preview using
the actual guest return frame. It also passes native/interpreter comparisons
without the decomp library, including bank aliases and both stack wraps.

## Regular map installation

`$83:B53B` reconstructs regular map resource loading, hardware multiplication,
attribute-grid filling and final setup. The ascending MVN keeps its original
source overlap and wrapping, including zero-sized products. The generated-map
skip shares the RTL but emits no load-commit event. The resource loader at
`$80:EAE7` resets all four section headers, selects the optional second resource
and palette, and restores the original registers, widths and data bank. All
14 child call sites retain their original frames and explicit dispatch.

18,478 original-ROM cases cover both index widths, Direct-Page wraps, child
unwinds, large fills and the composed loader. The production bridges pass
19,502 native calls with no positive fallback, 30 redirects and 2,304 guards.
All 23 semantic/event fault variants are detected. Native/interpreter events
also match across 12 callback pairs without linking the decomp library; skip,
stale load, wrong return stack and repeated commits are checked separately.

## Status menus and main NMI

The complete status/equipment dispatcher at `$82:A318` preserves the inline
handler table, party loops and all original child frames. Optional execution
checkpoints precede the equipment-list draws at `$82:A3B4` and `$82:A3F0`.
Invalid selectors and changing loop counts retain exact ROM continuations.

The main NMI body at `$80:8638` preserves screen blanking, live RAM callbacks,
the original uncapped byte clock, frame counters, reset condition and interrupt
restore. It continues at the original JML or RTI boundary, keeping the interrupt
frame intact. A checkpoint follows the clock tick at `$80:8699`, including the
stopped-clock path. Menu string rendering can also report each number operation
at `$80:8922`, before PHY, including recursive strings. Default callbacks leave
ROM behavior unchanged; time caps and expanded displays belong to consumers.

These additions pass 82,463 original-ROM and native ABI cases, 30 redirected
child probes and 2,304 unsupported-state guards. All 35 deliberate fault variants
are detected. The consumer event layer also passes eight native/interpreter
callback pairs without linking the decomp library, with safe unsubscription.

## Battle commands and targets

The battle lifecycle, main loop, turn execution and command-selection callers
are reconstructed. Unknown children remain explicit calls with their original
stack frames. A verified caller does not make those children verified.

The latest work covers command collection, party action choices, spell/item/IP
submenus and target selection. It retains the original polling, scrolling,
backtracking, invalid-target skipping and side changes. Target cancellation can
remove a mark and continue; pair selection waits for exactly two party members.

Battle code uses named action, turn-queue, target, status-icon and result-window
records. Helpers separate priority calculation, target drawing, command publication
and sprite overlays while preserving CPU state and memory access order.

Recent ROM comparison suites are summarized below. Earlier slices and exact
run history are recorded in the consumer's worklog.

| Suite | Cases | Result |
| --- | ---: | --- |
| Turn/command control, including composed records | 6,144 + 128 probes | pass |
| Newly bound runtime bridges | 717,659 native ABI cases | pass |
| Results and growth | 5,746 + 8,711 | pass |
| Status recovery and support | 5,162 + 22,528 | pass |
| Turn display and gauges | 10,252 + 24,576 | pass |
| Message display and glyph rendering | 17,415 + 32,771 | pass |
| Resource decompressor write order | 5,440 | pass |
| Message effects and status icons | 12,288 + 12,289 | pass |
| Frame/input/pause handling | 5,122 | pass |
| Word TSB/TRB write order | 34,816 | pass |
| Sprite builder and record append | 16,395 | pass |
| Action working records | 32,774 | pass |
| Action script callers and active-mask write order | 12,294 + 8,192 | pass |

Target coordinates, all four cursor variants, command frame upkeep and the
confirmation prompt are also reconstructed. They retain original coordinate
wrapping, hardware register accesses, title drawing and input waits. Item and
spell lists also preserve availability checks, MP limits and hardware division.
Window setup, cleanup, party names and tilemap queues are reconstructed.
Full queues retain the original BRK handoff. Results cover rewards, party and
capsule progression, stat gains and item reception, including the original
calibration BRK and inventory remainders. Status recovery, timed effects, gauges
and shared messages include aligned and shifted glyphs. Frame and pause handling
keep the original polling loops. Resource loading preserves the decompressor's
byte-write order. Sprite building preserves group packing, byte-counter wrap
and the original overlay loops. Action records retain their descending copies,
register saves and zero-mask shifts. Configured, battler and item script callers
retain their default scripts; the script VM remains an explicit child.

Runtime bridges check each entry's CPU mode, DB and DP contract. Other states
use the original interpreter. Tests cover stack frames, child unwinds and three
host-return modes; all 18 deliberate bridge mistakes were detected. Frame upkeep
yields at `$85:EC94` before frame and pause waits so interrupts can run.
Declarations and metadata give each entry's contract.

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

The floor generator `$83:9E31` is verified and selected. It uses A8, X/Y16,
DP zero and a DB that maps low WRAM. Verification covers 16,384 floors, five
helper groups of 4,096 cases each, and APU-return probes.

The APU handshake remains an explicit child. The verified map loader has its
own runtime binding and keeps the RAM copy routine and event tick as explicit
children. Original generator
loops and degenerate-map behavior are preserved. Host decoding or caching
belongs in a separate patch layer.

## Actors, field and shared helpers

Actor routines use slot views and generated memory-map constants while keeping
the original addressing and access order. The field loop, scene NMIs, scrolling,
world-map helpers and shared resource/menu/math routines are covered by the
consumer's differential and bridge suites. The index is the source of truth for
individual functions.

Map edge streaming and full layer redraws now have six independently verified
entries. Their 21,504 ROM cases compare CPU state, complete WRAM, hardware
arithmetic and every ordered write. Five word decrements retain the original
high-byte-first writes. Runtime bridges pass 49,152 native cases and 3,072
unsupported-state checks; all 13 deliberate rendering faults were detected.

Map-header loading `$83:B5D3`, map-event initialization `$80:E844`, event
startup `$80:E722`, Ancient Cave header construction `$8E:B847` and room
coordinates `$83:9B44` are independently verified. Their synthetic ROM cases
compare CPU state, all WRAM and every ordered write, including script-bank
crossing, zero room counts, index wrapping and a header without a terminator.
The latter hands off at `$83:B635` after 65,536 records with the original state.
The map loader preserves all five pushed child-call contracts. The RAM MVN
routine and event tick remain explicit children; the three other children are
also exercised through their reconstructed implementations. Runtime ABI tests
cover 51,209 native calls, 15 redirected children and 4,032 unsupported states.
All 13 deliberately faulty variants are detected. Ten proven WRAM fields were
added to the canonical catalog.

Twelve field-object graphics dependencies are independently verified: layer
selection, actor position probes, pending origins and tile offsets, tile-number
replacement, claimed-actor release, object palettes, actor sprite setup,
object-event startup and the fixed graphics DMA loader. Their 98,304 ROM cases
compare CPU state, complete WRAM and every ordered write. This includes word
TSB order, overlapping palette copies, index wrapping, all four entry widths
for the DMA loader and the unusual TDC behavior with nonzero direct page.
The same cases pass through the production bridges with 98,304 native calls;
8,256 unsupported-state checks and all 15 deliberate fault variants pass.
Eight proven object/actor WRAM fields were added to the canonical catalog.

The legacy event VM palette path also preserves the original high-byte-first
word TSB. Its 8,192 direct original-call comparisons cover flags, saved DB,
palette copy overlap, wrapping indices and every ordered write.

Six placed-object transitions are complete and independently verified: the
object-record loader, actor initialization and refresh, object placement and
rebuild, and claiming a placed object. Their explicit child callbacks preserve
the original pushed frames and every unwind boundary. Known children compose
with the existing decomp; remaining children retain original dispatch.
49,152 ROM cases compare CPU state, complete WRAM and every ordered write;
all 34 child unwind sites pass. Production bridges pass 90,173 native calls,
90 redirected-child contracts and 4,608 unsupported-state guards. All 16
deliberate fault variants are detected. The record loader retains the child’s
returned X; rebuild preserves the original caller-X lookup; claim retains the
indexed map-tile read and its conditional saved data bank.

Three object lookup dependencies are independently verified: header-record
search, pending-object search and the attribute-cell multiplier. Their 24,576
ROM cases cover both search exits, wrapped record pointers and carry-sensitive
strides. Ten malformed header cycles yield at the original loop PC after
65,536 records with the saved data bank intact. All 24,586 production native
calls and 2,304 unsupported-state checks pass; all 13 fault variants are caught.
The multiplier keeps its original register writes and result read.

Four object tile operations are complete and independently verified:
attribute masks, saved-tile restoration, redraw queue requests and tile-bit
rectangle copying. Their 32,768 normal ROM cases, six child unwinds and 24
carry/zero-counter/header-overlap cases compare CPU, complete WRAM and every
ordered write. The copier retains all three original bit-mask paths and the
carry shared by its row advances; large or self-mutating loops yield at the
original tile body after 262,144 transfers with the saved data bank intact.
Production bridges pass 32,798 native cases, 45 redirected-child contracts
and 3,072 unsupported-state checks. All 22 fault variants are caught. Six
canonical fields identify the attribute grid, layer section words and the
column-upload queue consumed by NMI.

Further work reconstructs the remaining field and battle dependencies.
See [ARCHITECTURE.md](ARCHITECTURE.md) for the boundary
model and [CAPTURE_RESEARCH.md](CAPTURE_RESEARCH.md) for scene evidence.
