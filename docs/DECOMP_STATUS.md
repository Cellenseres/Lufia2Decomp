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
461 functions in `metadata/functions.toml`: 370 verified, 91 draft, 0 identified, 0 disabled.
<!-- metadata-counts:end -->

The complete function list is in [FUNCTION_INDEX.md](FUNCTION_INDEX.md).
The routines that are still `draft` are the ones reconstructed after the last
verified checkpoint; [RECOMP_ENTRIES.md](RECOMP_ENTRIES.md) lists, per area,
the address, call kind and modes of each, which is what a consumer needs to
bind them.

Most draft routines are checked against the ROM with differential tests. The
world map object, plane, scroll and plane row code, the menu image, input,
cursor, tile map and slide code, the battle background, circle, palette, sprite,
drift, vector, tile copy and effect code, and the scene track code are written
on plain values: typed locals, named fields and the helpers in
`src/core/plain_ops.h` and `src/core/wram_view.h`. They keep only the register,
flag and stack state that the original leaves for its callers. The cell edge walk
in `field_cell_edges.c` was already written that way.

The battle frame setup, the select screen setup, the NMI upload code and the
older large modules still step the CPU-state helpers inside their bodies.
Moving them over is open work and does not change any entry listed above.

## Current checkpoint

The feature repair verifies `$85:ADE1`, `$85:AE68`, `$85:AEEB` and `$85:AA3D`.
Their complete M1X0 leaf contracts preserve binary and decimal arithmetic,
caller DP/DB, flags, stack residue and ordered writes through the original RTS.
The three wave fills pass every 16-bit phase value with varied caller state;
all four additionally pass consumer return-frame and entry-guard comparisons.
Other feature routines remain draft while their contracts and remaining paths
are reconstructed. See [FEATURE_REPAIR.md](FEATURE_REPAIR.md).

The full Windows Release verifier passes **518 independent jobs**. The normal
application build also passes. The consumer selects all 366 verified functions,
including song-load and fade-out after the MSU migration. The generated CFG
contains 1,451 nodes, and every prior native entry remains covered.

Verification uses the original ROM interpreter as the reference. It checks CPU
state, complete WRAM and, where needed, the order of memory and register writes.
Tests also force child calls to unwind and inject deliberate mistakes to check
that the comparisons can detect them. Detailed run logs belong in the consumer's
worklog rather than this status page.

## Scene graphics and palette loading

`$80:EF8E` loads scene graphics through readable phases for record indexing,
object assembly, tileset loading, two DMA transfers and auxiliary resources.
Its nine child-call boundaries retain their original frames and width changes.
`$80:F2F3` configures the scene display. `$80:F338` copies the scene palette
into the CGRAM upload buffer and clears its first color. All three functions
are selected by the consumer.

The loader preserves all four caller accumulator/index width combinations,
data banks, direct pages, fixed-bank DMA writes and zero-counter underflow.
Malformed record, object and metatile loops continue at the exact original
instruction when their respective bounds are reached. Changed child returns
and unsupported child modes likewise preserve the actual continuation.

Direct and runtime comparisons each cover 18,384 loader states, 4,096 display
states and 7,168 palette states. The loader reference executes 56,740,335 body
instructions; ordinary cases take 56 to 5,959 instructions, excluding child
bodies. There are 9,280 calls to the reconstructed display, metatile and palette
children in composed comparisons. Other child bodies use explicit contracts;
these results are not a complete gameplay or wall-clock benchmark.

Loader comparisons cover 4,779 direct and 6,394 runtime boundaries, including
141 bounded loops, plus 3,225 child-unwind cases. Runtime tests also exercise
15 redirects, with no initial fallback on supported entries. All CPU fields,
full WRAM, mutable hardware state and operand bus order are compared. Long
traces compare their first 262,144 events directly and their entire event count
and 64-bit stream hash. There are 4,983 unsupported-entry checks, and all 62
deliberate semantic and bridge faults are detected. The WRAM catalog adds 17
proven locations; indexed bases do not claim unproven array extents.

## Metatile graphics and planar mirroring

`$80:F35B` copies the four planar tiles of a metatile, applying horizontal
and vertical flips from the original tile attributes. Its C separates
raw block copying, mirrored plane words and reverse row traversal. The
byte mirror at `$80:F40A` uses three bit exchanges instead of reproducing
the original shift sequence; it preserves the hidden accumulator byte's
observable carry effect. `$80:F3F1` mirrors both plane bytes through their
actual call frames and direct-page scratch.

Both direct and runtime comparisons pass 65,792 byte-mirror, 65,792
word-mirror and 4,864 metatile states. All initial accumulator words are
covered for the mirror routines. Tests vary data banks, direct pages,
flip combinations and stack aliases, including 324 exact continuations
from changed returns or bounded malformed loops. Runtime execution has
no initial native fallback. Normal metatile cases execute 250 to 5,618
original body instructions per call; this is an instruction-count result,
not a wall-clock benchmark.

The tests compare CPU state, full WRAM, hardware state and operand bus
order. Long malformed traces compare their first 262,144 events directly
and their complete event count and 64-bit stream hash. There are 6,135
unsupported-entry checks, and all 40 deliberate semantic and bridge
faults are detected. All three functions are selected by the consumer.

## Field reload and camera preparation

`$83:85DC` reconstructs the complete field reload through its original
return at `$83:8673`. The earlier setup-only API remains available. All
eleven child calls preserve their actual stack frames, modified returns
and unwind boundaries. The consumer selects the complete routine.

`$83:AB61` clears the 128 sprite-allocation bytes while preserving the
original data-bank and processor-status stack effects. `$8E:B09C` prepares
camera pixel offsets and cell coordinates, then calls the original scroll
child. Its pixel reads are unindexed; its cell reads use the byte actor
selector. This original difference is retained.

3,456 reload, 68,352 camera and 66,048 sprite-reset original-parent states
pass both direct and runtime execution, with no initial native fallback.
They cover all four entry register-width combinations, all accumulator
values for the reset, all pixel-coordinate word values for the camera,
data banks, direct-page variation and small-stack aliases. Tests compare
child-entry and final CPU state, complete WRAM, hardware state and operand
access order. There are 928 exact continuations, 30 dispatcher redirects,
4,599 unsupported-entry checks and 41 detected deliberate faults. The
earlier reload-prefix regression also passes all 8,704 cases. Unknown
child bodies retain explicit contracts; these comparisons do not establish
the complete battle-to-stairs gameplay chain.

## Field battle transition

`$83:83EB` waits for the pending field upload, calls the original fade,
redraw and battle children, and routes the battle result. Cave maps `$F0`
and `$F1` take the original defeat routine; other defeats reset the stack
and hand off to `$83:ACEF`. The normal path retains mosaic and fade writes,
optional actor-touch handling and the field-event call. All six pushed
child frames and changed returns remain observable.

6,800 original-parent states pass in direct and runtime execution without
initial native fallback. They cover all result and map bytes, every data
bank, all high direct-page bytes, small-stack aliases, changed child modes,
modified returns and both 65,536-read wait boundaries. The tests compare
child-entry CPU state, final CPU state, complete WRAM, mutable hardware
state and every operand access. There are 1,885 exact continuations,
15 dispatcher redirects, 2,301 unsupported-entry checks and 31 detected
faults. The fade-completion inputs activate after the original fade-start
write; these schedules do not model the NMI body. Child bodies use identical
explicit contracts, so the parent proof does not establish the gameplay
cause of the Cave stairs issue.

## Area rectangle destinations

`$83:B76E` applies the destination map, coordinates and transition parameters
from an area rectangle. It preserves the current-map case, the original Cave
reload flags, parameter inheritance from `$066A`, and all five child-call
sites. It is a standalone replacement with a PB `$83`, M1/X16, binary-mode
contract. The indexed rectangle reads carry into the next bank; the mixed
short and long destination accesses retain their original bus order.

67,584 original-parent states pass in direct and runtime execution with no
initial native fallback. They include every type/target-map combination,
every 16-bit rectangle index, all 256 high direct-page bytes on the TDC path,
small stacks, changed child modes and banks, and modified returns/unwinds.
The tests compare CPU state at every child entry, final CPU state, complete
WRAM, mutable hardware state and every operand access. There are 448 exact
continuations, 15 dispatcher redirects, 2,301 unsupported-entry checks and
29 detected deliberate faults. The child bodies use identical explicit
contracts on both sides; their gameplay effects are not inferred from these
parent comparisons.

## Ancient Cave party reset and defeat

`$84:8888` clears the original party records and restores the initial items,
party levels and capsule state through explicit child-call contracts.
`$84:8B9C` invokes that reset, saturates the defeat counter at `$FFFF`, and
sets the original field reload flags. Both routines accept PB `$84`, an
8-bit accumulator and 16-bit indexes. They are standalone replacements.

The defeat exposes checkpoints at entry (`$84:8B9C`) and after the actual
party-reset return (`$84:8BA5`). Both support eight subscribers, native
execution and matching interpreter checkpoints without the decomp library.
The reconstruction retains the original inventory loss; consumers can
implement carryout changes through these interfaces.

Verification compares 68,452 original-parent states with direct and runtime
execution, using identical explicit contracts at the three unknown child
sites. All 65,536 defeat-counter values are covered. There are no initial
native fallbacks; 899 exact continuations preserve loop limits, changed
child modes, modified return frames and child unwinds. CPU state, all WRAM,
mutable hardware state and every operand read and write agree. The tests
also cover 30 dispatcher redirects, 4,602 unsupported entries, 1,277
subscriber checks and 36 detected deliberate faults. Direct-page aliases
retain the original TDC fill byte and variable initial-item byte count.

## Ancient Cave exit and carry items

`$84:890B` restores the pre-Cave state, preserves the original time arithmetic,
and returns the original carry items. `$84:8AF4` checks the 41 blue-equipment
IDs and appends eligible item words through WMDATA. Both are standalone
replacements with an explicit PB `$84` contract; unsupported banks and CPU
modes enter the interpreter at the actual original address.

The exit exposes checkpoints before its first PHP (`$84:890B`) and after
restoring the backup (`$84:8A38`). The consumer event layer supports eight
subscribers and the same interpreter checkpoints without a decomp build.
The standalone code contains the original behavior; carryout fixes belong
to consumers of these interfaces.

Verification covers 65,792 blue-item states and 10,540 exit states against the
original parent instructions, with identical explicit contracts at six unknown
child-call sites. The runtime bridge enters both routines without an initial
fallback in these cases. The 171 exact continuations cover bounded loops,
changed live returns, child unwinds and changed child CPU modes. The tests
compare CPU state, all WRAM and every operand read and write, including the
1,931 real backup bytes read through WMDATA. There are also 4,602 unsupported
entry checks, 40 detected deliberate faults, 1,151 subscriber checks and 768
WRAM-port bank/wrap checks.

## Menu member status read order

`$82:950E` now follows the original conditional and repeated status/HP reads,
rather than caching their values before selecting a colour. The difference
is observable when a synthetic member pointer reaches the WMDATA read port.
All 16,384 original-ROM and runtime cases pass; reinserting the old cached
sequence reproduces the three detected failures.

## Scene script operand reader

`$80:C0B7` reads a scene operand at DB:Y and advances the original script
pointer. It preserves the original accumulator width, bank-byte rollover,
saved status and stack operations. A restored 8-bit index status resumes the
interpreter before `$80:C0CC`, where the operand length changes.

131,680 original-ROM and native cases pass without positive native fallback,
including every 16-bit Y value with both accumulator widths and every bank
byte. CPU state, full WRAM, all operand accesses and mutable hardware state
agree; opcode fetches are counted separately so operands overlapping ROM code
remain checked. All 1,152 unsupported entries and 22 injected faults pass;
32 saved-status width continuations retain their exact boundary. The complete
reader is selected as a standalone replacement.

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

## Updating a placed object

`$83:8B0E` combines the original object-record lookup, tile copying, layer
rendering, tile-bit clear and attribute refresh. It retains the byte coordinate
and height adjustments, restores the caller's bank, and accepts either caller
accumulator width with 16-bit indexes. Each child follows its live return frame
and passes through its exact continuation when it cannot finish.

8,328 ROM and native cases pass without a positive entry fallback. CPU state,
full WRAM, all data accesses and mutable hardware state agree. The checks cover
44 independently counted child limits and 288 changed returns, including 24
symmetric frame rewrites. Eight additional symmetric state changes exercise
region-renderer limits after tile copying. Original calls and returns are
tracked independently by the reference runner. All 1,152 unsupported entries
and 30 injected faults are detected. The complete update is selected as a
standalone replacement.

## Object rendering and upload queues

`$83:89CE` clears the original tile bit below a pending object when its flags
permit it. `$83:8A0A` renders the selected object layers and appends their source,
destination and row count to the upload queues. Both retain the original height
thresholds, ROM-backed tables, saved bank, live child frames and byte queue count.
The coordinate helper at `$83:F9D9` also follows its live child RTS frame.

82,000 original-ROM and native cases pass without a positive entry fallback.
CPU state, full WRAM, every data access and mutable hardware state agree. The
checks include eight independently counted loop limits, nineteen naturally
changed returns and sixteen symmetric bus-observer frame rewrites. All 3,072
unsupported entries and 44 injected faults are detected. Both complete object
routines are selected as standalone replacements.

## Object map attributes

`$83:8C8A` updates the attribute grid under the pending object. It keeps the
original metatile catalog lookup, classification bytes, preserved attribute
bits, row strides and byte counters. A zero dimension still processes 256
cells along that axis. The caller's saved bank and M/X widths are restored.

32,952 ROM and native states pass with no positive entry fallback. The checks
cover all four caller widths, bank crossings, overwritten saved state, twelve
read-only child returns, 32 symmetric bus-observer return rewrites and eight
exact 262,144-cell continuations. CPU state, full WRAM, all data accesses and
mutable hardware state agree. All 768 unsupported entries and 30 injected
faults are detected. The complete routine is selected as a standalone replacement.

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

## World map ground plane

`$86:A894` builds the two Mode 7 matrix HDMA tables of the world map's
perspective view: the sine and cosine terms of the view angle, the row step
from the tilt, and 2 x 112 scanlines scaled by a reciprocal table through the
hardware multiplier. It is the most expensive routine the interpreter still
ran in play (about 11,000 instructions per call, once per frame). The angle
terms, the 16-bit multiply, the 32-bit restoring divider and the four sign
patterns are internal. The routine is `draft`: it was compared against the
original ROM code in the interpreter on random work-RAM states (CPU state,
complete WRAM, ordered writes, hardware multiplier accesses and the stack
frames it leaves behind) and awaits the maintainer's verifier.

## NMI uploads

Three children of the main NMI are `draft` reconstructions: `$80:8703`
(OAM and palette DMA when requested, then the pad words with their key repeat
after the automatic read), `$80:87A7` (the scroll registers from `$0594`, the
four listed DMA requests, the tilemap uploads and the HDMA channel mask) and
`$80:87FC` (the three VRAM tilemap blocks). They are the NMI's largest
remaining interpreted cost (about 150 instructions per frame). Each was
compared against the original ROM code in the interpreter on random work-RAM
states with the pad registers varied to reach the repeat paths: CPU state,
complete WRAM, ordered writes, the order of every hardware register access
and the stack frames they leave behind. They await the maintainer's verifier.

Entry contract of the three shells: `$80:8703` and `$80:87A7` are declared
`M1X1` (the main NMI enters with M8 and either index width; both save P, set
the widths they need and restore P, so exit widths equal entry widths) and
`$80:87FC` is `M1X0` (it runs on the caller's widths, without saving P). A
shell does not reinterpret any other entry state: `$80:8703` and `$80:87FC`
return `ExecutionHandoff` to the original code when M or X differ from the
declared widths, which keeps the original instruction semantics for the
callers that do not match.

## Battle actor sprites

`$81:8E92` (all 64 actor records into the three OAM lists, then the
published totals) and its per-actor child `$81:8EEA` are `draft`
reconstructions. They are the interpreter's largest battle-frame cost outside
the main loop. Both were compared against the original ROM code in the
interpreter on random actor tables (CPU state, complete WRAM, ordered writes
and the stack frames they leave behind) and await the maintainer's verifier.

## Battle palette brightness

`$81:B396` (darken or lighten the 15 palette entries of a set into the upload
buffer) and its colour helper `$81:B3F8` (per-component scaling through the
hardware multiplier) are `draft` reconstructions, compared against the
original ROM code like the sections above, including the order of the
read-modify-write stores and of the multiplier accesses.

## World map objects

`$86:E287` / `$86:E295` (the on-screen test that builds the visible object
list), `$86:E640` (object slot flags) and `$86:E650` (hide every hardware
sprite, clear the OAM high table) are `draft` reconstructions of the world map
per-frame object update. They are compared against the original ROM code on
random object tables, camera positions and data banks, including the
screen-edge boundaries, for CPU state, work RAM, ordered writes and the
frames they leave behind. They await the maintainer's verifier.

`$86:E0B9` (start an animation on an object: first frame record, step,
timer and tile base) and `$86:E11F` (the per-frame timer step of the 22
objects at `$1469`, with the frame advance and the loop or restart at the
end of an animation) are `draft` reconstructions in the same file. Both
need M1X0 and the world map data bank for the animation tables; any other
entry state is handed back. They are compared against the original ROM code
on random object tables, including wrapped frame counters, flagged
animation ends and data bank mirrors.

`$86:E555` (the pair of hardware sprites of an object, mirrored for the
poses that face the other way) and its helper `$86:E5BB` (two x bits into
the OAM high table, advancing the sprite counter) are `draft`
reconstructions in the same file, entered with M0X0 only. They are compared
on random objects, counters near the byte boundary of the high table and
several data banks, including the stack frames of the helper's indirect
call.

The rest of the world map object pass is `draft` as well, so the whole
per-frame pass `$86:E1B9` (hide the sprites, clear the slot flags, build the
visible list, sort it, draw it and then draw the later users of each shared
tile block pattern) runs natively: `$86:E686` (insertion sort of the visible
list), `$86:E3AB` with its dispatch `$86:E3D2` (the objects in sorted order,
then the player object), `$86:E430` (find or add the sprite pattern slot of an
object), `$86:E479` and `$86:E4E7` (the single-sprite variants of the pair
writer), `$86:E2D2` (the visibility test of the tilted map view) and
`$86:A5A9` (the 32-bit by 16-bit division it uses). They need M0X0, except
`$86:E1B9` which needs M1X0, and the division, which keeps the caller's
widths; other entry states are handed back. An object kind beyond the three
known ones hands the original indirect dispatch back at its call, with the
stack frames of the routines above it already in place, so the original code
finishes the pass. They are compared against the original ROM code for CPU
state, work RAM, ordered writes and stack frames, on random object tables, slot
pools near their limits, camera positions at the screen edge and the horizon
limit of the tilted view.

## Battle background wave

`$85:AEEB`, `$85:AE68` and `$85:ADE1` rebuild the 176-word table of scroll
offsets for the wave effect from the ripple pattern in ROM and the base offset
(from phase 0, from the stored phase advancing it, and from the stored phase
stepping it back) and are `draft` reconstructions. They need M1X0 (else handed
back) and switch to their own data bank the way the original does, so the
stack bytes of the bank save and restore are reproduced. They are compared
against the original ROM code for CPU state, work RAM, ordered writes and
stack frames, with phases at both ends of the pattern.

## Battle circle window

`$85:B208` (the scanline edge table of the circular window, rebuilt when the
radius changes, with open rows beyond it) and its helper `$85:B26D` (the
half-width table of the circle of the radius by the midpoint rule) are `draft`
reconstructions. `$85:B208` needs M1X0 (else handed back) and switches to its
own data bank like the original; the helper keeps the caller's widths. They
are compared against the original ROM code for CPU state, work RAM, ordered
writes and stack frames, with radii at both ends of the range and unchanged
radii.

## Menu tile maps and cursor placement

`$82:838F` (clear both menu layers), `$82:8069` (the 8 x 8 grid of numbered
tile blocks), its block filler `$82:80A5`, the rectangle recolor `$82:80CA`,
the 16-bit shift-and-add multiply `$82:8000` and the two cursor placement
routines `$82:88A0` (list index) and `$82:88CB` (pixel position) are `draft`
reconstructions. The four that end in a redraw request stop at the frame wait
`$82:93C2` with their own return pushed, so the wait and the rest of the
caller run unchanged. Widths that the original does not take are handed back
at the entry. The multiply keeps X and the status flags and leaves the rotated
multiplier behind, as the original does. They are compared against the
original ROM code for CPU state, work RAM, ordered writes and stack frames, at
every entry width.

## Menu cursor slide

`$82:89FA` slides a menu cursor sprite between the positions of two slots in
straight steps (an error-accumulating line), with the corrections `$82:8AD8`
and `$82:8AE9` and the step counter `$82:8AFA` that asks for a frame every
sixteenth step. They are `draft` reconstructions that need M1 (else handed
back). The counter hands off at its sprite frame call, and the slide passes
that hand off up with the frames of both routines pushed, so the rest of the
slide continues in the original code. They are compared against the original
ROM code for CPU state, work RAM, ordered writes and stack frames, on random
slot pairs, equal slots, short slides that finish before the first frame and
long ones that hand off.

## Menu cursor input

`$82:8720` moves the cursor of a menu item from the buttons just pressed
(rows and columns with wrap, the edge flags, the item limit through the list
index, and the action codes 2 to 9) and `$82:8B08` is one pass of the menu
input loop around it (button edges, the window request, the cursor blink).
They are `draft` reconstructions that need M1 (else handed back). The moves
that play a button sound and the sprite frame at the end of the loop hand off
at that call, with the state exactly as the original has it there. They are
compared against the original ROM code for CPU state, work RAM, ordered writes
and stack frames, on random item tables, every button, rows at both edges,
items with and without a limit and flagged items.

## Select screen setup

`$86:8DD7` sets up the select screen video registers and the four empty tile
map layers, clears the sprite table and stops at the frame wait of `$86:8B48`
with its return pushed. `$86:8E6B` switches all sprite slot flags off. Both
are `draft` reconstructions that need M1X0 (else handed back). They are
compared against the original ROM code for CPU state, work RAM, ordered
writes (including every video register write) and stack frames, at every
entry width and with the data bank both inside and outside work RAM.

## Battle frame setup

The battle sprite pass already existed as private helpers reachable through the `$85:8A2F` entry. This group adds the JSL entry points the game calls directly, so the sprite records (`$85:8B4B`), the single sprite (`$85:8BC0`), the marker sprites (`$85:8C27`), the party sprites (`$85:8C98`) and the party tilemap (`$85:8D2E`) now run natively. The tile grid (`$85:972E`) calls the row writer `$85:9790` fifteen times, which is now native too.

`$85:8A39` is the full variant: it forces mode 1 when `$11DE` is set, clears `$2A0` word pairs at `$7E:2800` through the work RAM port, then runs the color loaders (`$85:8AAF`, `$85:8AF4`, `$85:8B22`) and the sprite routines in the order of the selected mode. Mode 1 ends with the party tilemap or the tile grid, mode 2 with the party sprites, and any other value returns at once.

Entry widths are M8/X16 for the sprite and color routines (anything else is handed back at the entry), M16/X16 for `$85:9790`, and any M with X16 for `$85:972E`. Each JSL made by `$85:8A39` pushes its return frame, so a child that hands back leaves the frame in place for the interpreter.

## Battle party drift

The battle party records at `$13DA` (six records of 15 bytes) carry two offset words that make the party sprites drift. `$85:894A` walks the records whose active bit in `$13DB` is set and, by the state bits in `$13DA`, either reads the shake table at `$85:9E37` with a countdown from `$152A+i`, flips the sign of the first offset word, or draws both offsets from the random bit source. Nothing happens unless `$129A` is negative.

`$85:8F4A` shifts the 88-bit register at `$122F`–`$1239` left by one through the carry. It returns the new low bit in `$122F`. Both routines are native: the first is a JSL entry that needs M8/X16 and hands back at the entry otherwise, the second is a JSR routine that needs M8.

## Scene value tracks

`$86:94D4` steps five value tracks that are driven by small scripts: the cells at `$1206` hold the current script position, the timers at `$121A` count the frames of a step, and the values at `$1210` grow by the step delta each frame. When a timer runs out the next step is fetched (position moved by four), and a zero length ends the script with carry set. After a full pass the values are copied to the scene registers (`$11FC`, `$11FE`, `$1247`, `$1249`, `$1200`) and carry is cleared. It accepts any accumulator width and needs 16-bit index registers, and it leaves the accumulator 8-bit.

`$86:A791` derives the screen origin from the view position: `$11F8` and `$11FA` get the position masked to 12 bits, and `$0594` and `$0596` the scroll values relative to the screen centre. It needs M8/X16.

## Attribute unpack and angle vectors

`$80:ED0E` unpacks the cell fields stored two bits each at `$7F:C000`: every source byte feeds four cells, and each field replaces bits 4 and 5 of the attribute byte at `$7F:0001,X` before X moves on by two. The number of source bytes is `(a * b + 3) / 4`, with the two factors taken from the table at `$7F:D010` and `$7F:D018` for the map selected in `$05AA` and multiplied through the hardware unit. It forces M8/X16 whatever the entry width.

`$85:DE2A` and `$85:DE1E` look up the quarter-wave table at `$97:B226` for the angle in `$54` (the second adds a quarter turn, which gives the cosine) and leave the word in A and in `$63`/`$64`; the second half-turn carries the sign in bit 15. They save and restore the status and accept any width. `$85:DD63` turns an angle and a speed (`$5A`) into the two velocity words `$56` and `$58`, either by the four axis cases or by multiplying the speed with both table words through `$80:834C`. It needs M8/X16 and a zero direct page, like the multiply routine.

## World plane rows

The world map plane rotation builds its per-scanline scale tables four rows at a time in the four routines at `$86:A9B0`, `$86:AA5B`, `$86:AB0E` and `$86:ABC1`, picked through the jump table at `$86:A9A8` by the quadrant of the rotation angle. Each fills the 112 rows (`$26` counts them down, `Y` walks the tables at `$1718`/`$171A` and their mirrors at `$1A9B`/`$1A9D` backwards) with the entry of the table at `$D3B7` selected by `$24`, scaled by the two factors in `$58` and `$5A` through the multiply unit, or copied when a factor has no fractional part (low byte of `$58` zero). The quadrants differ only in which of the four stores are negated and in the order of the last two stores. After each row the angle in `$22`/`$24` is reduced by the step in `$00`/`$02`.

All four need M16/X16 and reach the multiply registers through the data bank, which is why the test also runs them with a data bank that maps work RAM over the register addresses.

## Battle background wave tables

`$85:A736` (`Lufia2BattleRippleRow`) and `$85:AA3D` (`Lufia2BattleRippleWords`) belong to the battle background wave effect and live in `battle_background_wave.c`. The first derives one row of horizontal offsets from the frame counters and the second fills the 84-word table at `$7E:40DE` from a phase taken from the random byte at `$1B22`. Both are checked against the ROM with randomised state and both match on the full write log; mutations of constants, widths, carry setup and loop bounds are detected.

## Battle effect interpreter operations

`$81:A40B`, `$81:953F`, `$81:9169` and `$81:A598` are small operations of the battle effect interpreter and live in `battle_effect_ops.c`. They work on a slot addressed by Y in bank `$7E` and read their operands through the stream pointer at `$C3`: adding a stream word to a slot field, counting down the repeat byte and looping the stream back, remembering the loop start, and deriving the slot velocity from its angle and speed through `$85:DD63`. `$81:9169` ends with a jump to `$81:8C58`, which is left to the original code. All four match the ROM on the full write log with randomised slots and streams in ROM and WRAM.

## Menu image loaders

`$00:057D` is the block move kept in work RAM: an MVN whose bank operands the callers fill in at `$057E` and `$057F`, followed by an RTS. `Lufia2RamBlockMove` in `block_move.c` performs the copy when the stub holds the expected bytes and otherwise leaves the call to the original code.

The bank `$86` menu image loaders in `menu_image_load.c` use it to build the image buffer at `$7E:6000`: `$86:9009` and `$86:906A` copy a 256-byte and a 128-byte row, `$86:8FF6` copies two rows, `$86:9022` and `$86:8F6F` load a whole image grid or image set and queue the buffer for upload, and `$86:90C0`, `$90D3`, `$90E6`, `$90F9`, `$910C` and `$911F` copy the palette blocks. `$82:8044` sets up the video transfer for the queued buffer; both it and the loaders that call it stop at the frame wait of `$82:93C2` with the returns of their callers pushed, as the other menu routines do. `$80:C195` in `field_object_flags.c` sets bit 0 of the object slot flags 5 to `$27`. The reference for the block move is a separate model in the test, because a routine that always handed off would otherwise pass the ROM comparison.

## World scroll step, tile blit and totals copy

The world scroll step is rebuilt as three routines: the 24-bit product of a word and a byte through the hardware multiplier, the four-quadrant step offsets taken from the sine table in bank $97, and the advance of the 24-bit scroll offsets with the 12-bit wrap of the positions. The battle tile copy moves rows of 64 bytes with a second plane $40 bytes further on into two buffers $200 bytes apart. The member totals wrapper works on a copy of the record at $7E:3800 and copies only the 14 bytes of totals back. All five are compared against the ROM with random inputs, and the multiplier is modelled by the test bus.

## Field cell edge walk

The cell edge routine walks along the border of a region of the field cell map, starting from the party cell, and marks the cells it passes. The walk keeps a direction and a step budget, turns at corners, clears the marks left behind on side runs, and finally folds the marks into the attribute bytes of the cell table. It is split into the entry shell, the two ways of starting the walk, one function per direction, and the final fold, and each of these sections is compared against the ROM from its own entry address with its own set of random maps. The cell pointer routine that selects the start cell is reconstructed with it.

Further work reconstructs the remaining field and battle dependencies.
See [ARCHITECTURE.md](ARCHITECTURE.md) for the boundary
model and [CAPTURE_RESEARCH.md](CAPTURE_RESEARCH.md) for scene evidence.
