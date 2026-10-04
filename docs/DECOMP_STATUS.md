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
461 functions in `metadata/functions.toml`: 454 verified, 7 draft, 0 identified, 0 disabled.
<!-- metadata-counts:end -->

The complete function list is in [FUNCTION_INDEX.md](FUNCTION_INDEX.md).
The routines that are still `draft` are the ones reconstructed after the last
verified checkpoint; [RECOMP_ENTRIES.md](RECOMP_ENTRIES.md) lists, per area,
the address, call kind and modes of each, which is what a consumer needs to
bind them.

Draft reconstructions have preliminary comparisons, with counterexamples and
incomplete paths still under repair. The later feature sections describe their
intended operation and earlier author checks; they do not establish complete
verification. Current status comes from metadata and [FEATURE_REPAIR.md](FEATURE_REPAIR.md).
The
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

The five fixed menu palette copies `$86:90C0` through `$86:910C` are also
verified through RTL, accepting either accumulator width with X16. Consumer
ABI comparisons cover native calls, rejected entries and output/stack overlap.
The world perspective plane `$86:A894` is also verified through RTS.
Its fixed-band child calls are covered by the parent contract; the independent
row entries remain draft. The menu multiply `$82:8000`, world division
`$86:A5A9` and the battle angle lookups `$85:DE2A`/`$85:DE1E` are verified
for any caller widths. The world sprite and slot-flag clears `$86:E650` and
`$86:E640` are verified too, as are the menu item index and position
`$82:88A0`/`$82:88CB` for the `$1F00..$1FFC` caller stack. The copied-member totals `$81:F481` are also verified for DP0 and the same
caller stack, preserving the shared stat event. Scene tracks, view origin and the two battle velocity routines are also
verified. The world object visibility leaf and its bounded original list caller are
also verified. Eighty-one feature additions are verified and 14 remain draft.

The base main checkpoint passed **518 independent jobs**. The normal
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


## Scene tracks, view origin and battle velocities

Four complete return contracts are verified: `$86:94D4` steps the five scene
tracks, `$86:A791` calculates the screen origin, `$85:DD63` calculates signed
angle velocities, and `$81:A598` transfers these velocities to an effect slot.
The first two and the effect routine return through RTS; the angle routine
returns through RTL. The bridges retain the original return-frame sizes.

All four require native mode, the original program bank, DP0 and a caller
stack no higher than `$1FFC`. Tracks accept either accumulator width and X16;
the others require M8/X16. Tracks and view origin additionally require a data
bank that maps the low work-RAM window. The velocity routine requires
`S=$1F00..$1FFC`; its effect caller requires `$1F03..$1FFC`, reserving the
three-byte child frame. Unsupported entries hand off before memory access.
All decimal modes are preserved. Velocity calculation uses the already
verified sine, cosine and multiplication semantics and their original frames.

The matrices exercise all 256 angles, 16 speed edges, timer/script endings,
coordinate edges, banks, widths and decimal modes. Effect slots also overlap
scratch bytes and caller return frames; forward staging and final writes keep
the ROM ordering. CPU, all WRAM, ordered writes and hardware reads are compared
through the actual native bridges. Guarded cases are unchanged entry handoffs,
not original-ROM execution claims.

- scenetracks: 33,856 cases, 33,616 native, 240 unchanged handoffs
- sceneview: 33,856 cases, 12,928 native, 20,928 unchanged handoffs
- velocity: 37,952 cases, 34,064 native, 3,888 unchanged handoffs
- effectvelocity: 33,856 cases, 33,552 native, 304 unchanged handoffs

81 extra guard cases cover three host-return modes. All 35 deliberately broken
bridges are detected: return-frame size, overflow, entry bank, stack bounds,
DP, emulation, width and work-RAM bank checks. Bodies use typed values, named
work fields and arithmetic helpers; CPU-state operations retain observable
flags and call frames. The review found no AI markers in these bodies.

23 of 95 feature routines are verified, with 72 draft. All 518 independent
integrated jobs and the Release build pass with 389 standalone replacements.
Every new binding has a generated dispatch call. Main and the normal build
remain unchanged; this is an isolated local checkpoint.


## World object visibility and its original list caller

`$86:E295` is verified through RTS for native mode, PB86, M0X0, DP0,
S no higher than `$1FFC`, and a data bank mapping low work RAM. The record
start X must be at most `$1FF1`, and list start Y at most `$1FD2`. These limits
keep all record reads and both output fields in RAM. Record, list, camera,
counter and caller-frame aliases preserve their original order and rewritten
RTS destinations. Entries outside this contract retain the original entry.

`$86:E287` is verified through RTS for the original updater's caller:
native PB86/M0X0/DP0, work-RAM DB, X=`$1469`, Y=`$124F`, S=`$1F00..$1FFC`,
and 1..21 objects in DP `$22`. The ROM updater initializes exactly 21 objects;
binary and decimal pointer advances keep these records clear of child frames.
A zero counter wraps on its first DEC; it is not an empty list. Unsupported
counts retain the original code. This contract closes the whole loop, including
its child JSR/RTS frames, rather than using a synthetic iteration cap.

The consumer reads the two counter bytes in explicit low/high order before
materializing a caller frame. Nine count-guard cases check exactly these two
RAM reads, their byte values and the resulting open-bus high byte. All CPU
registers/status and WRAM remain unchanged. The other 57 guard cases require
no memory reads or writes. All 66 cases cover three host-return modes; no
comparison was relaxed. General ROM comparisons still check CPU, all WRAM,
ordered writes and hardware reads. The list matrix includes banks 00/7E/7F/86.

- visibleobject: 66,624 cases, 40,272 native, 26,352 entry handoffs
- visiblelist: 33,856 cases, 32,768 native, 1,088 entry handoffs

All 25 broken bridges are detected: return frame, overflow, PB, stack, DP,
emulation, widths, work-RAM bank, record/list bounds or starts, and count
bounds. The bodies retain readable box calculations and the original forward
loop, with CPU helpers only for observable flags and frames. No AI markers
were found. Parent ProjectObjects and UpdateObjects screens have no hard
mismatches but remain draft, with budgets and explicit child handoffs.

25 of 95 feature routines are verified, with 70 draft. All 518 independent
integrated jobs and the Release build pass with 391 standalone replacements.
Both new bindings have generated dispatch calls. Main and the normal build
remain unchanged; this is an isolated local checkpoint.


## World perspective row builders

The four sign variants at $86:A9B0, AA5B, AB0E and ABC1 are verified
through RTS for their original plane caller: native PB86/M0X0/DP0/DB86,
Y=$0382 or $01C1, and 1..112 rows in DP $26. The consumer accepts stack
pointers up to $1FFC, including the parent's child frame at $1EFC.
Unsupported entries retain original execution before any writes.

Each row preserves reciprocal reads, multiplier accesses, forward output
order, decimal arithmetic, counter RMW order, flags and caller-frame aliases.
The 16-bit read at $4217 includes the joypad byte at $4218; its full value
is retained in X. The source uses shared, readable calculations for the
four quadrants and memory callbacks for observable hardware accesses.

The actual bridges pass 200960 cases: 196608 native ROM comparisons and
4352 entry handoffs. Their matrices cover factors, angles, subtraction steps,
row counts, both bands, decimal mode and overlapping return frames.
132 guard fixtures cover three host-return modes. Count guards check the
exact two low/high RAM reads and resulting open-bus byte; CPU registers,
status and WRAM remain unchanged. Other guards require no accesses.
All 48 deliberately broken bridges are detected. The complete parent plane
matrix still passes all 262144 ROM comparisons after the row contracts.

29 of 95 feature routines are verified, with 66 draft. All 518 independent
integrated jobs and the Release build pass with 395 standalone replacements.
All four bindings have generated dispatch calls. The current row library also
passes 16384 native parent ABI cases and 24 parent guards. Main and the normal
build remain unchanged; this is an isolated local checkpoint.


## Finite product, ripple and packed field attributes

$86:A583 is verified through RTS for M8/X16, native PB86 and S<=1FFC,
with any DP/DB. The body retains both hardware products, scratch aliases,
decimal ADC and the first product in X. $85:A736 is verified through RTS
for M8/X16, native PB85, S<=1FFC and a low-WRAM byte at uint16(DP+$33).
It fills exactly 32 ripple bytes, preserving X, bank restoration, flags,
phase arithmetic and stack aliases, including wrapped stacks.

$80:ED0E is verified through RTL for native PB80, DP0, S=1F00..1FFC,
any entry widths/DB and a nonzero rounded dimension product. It leaves
M8/X16. A readable four-cell group loop replaces the instruction-shaped
body. It retains multiplier accesses, packed-byte and field scratch writes,
pointer carries, attribute merge order, the original decimal rounding and
final registers/flags. Zero count means 65536 groups in the original;
that entry retains original execution before any writes. The count guard
reads layout/width/height RAM only. The bridge checks source flow before
capturing return bytes; supported output cannot reach the 1Fxx caller frame.
Three zero-count guard fixtures check those exact reads, values and open-bus
byte. Other guards require no accesses. CPU registers/status and WRAM stay
unchanged on rejection. The verifier raises only this root's reference budget
to 2097152 instructions to cover valid large maps; no comparison is relaxed.

The three actual bridges pass 107200 cases: 90279 native ROM comparisons,
16921 entry handoffs, 51 extra guard cases and 27 detected bridge/source
mutants. Field matrices cover 4096 dimension/target/width/decimal states,
including 255x255 cells, plus all 256 packed bytes against four old attribute
patterns (1024 cases). An initially ineffective bad-shift control now uses
a populated large map and fails correctly. Native counts are measured by
actual dispatch, not inferred from semantic pass totals.

Low-stack regression found a genuine shared wave defect: at SP0, the PHA
used to select bank85 targets ROM, so PLB may read another bank. The memory
view now follows the bank actually pulled; the wave pattern uses its original
absolute indexed address rather than a fixed long bank85 address. Three
existing wave builders and RippleRow are corrected. The four existing wave
bindings pass 6400 actual-ABI cases (5632 native,768 handoffs), including
2048 low-stack cases, plus 60 guard fixtures. Seven controls restoring either
bad bank assumption fail against the ROM. Normal-stack behavior is preserved.

32 of 95 feature routines are verified, with 63 draft. All 518 independent
integrated jobs and the Release build pass with 398 standalone replacements.
The three new bindings have generated dispatch calls. Main and the normal
build remain unchanged; this is an isolated local checkpoint.


## Battle tile copying and world motion

$81:BCCC is verified through RTL for M8/X16, binary arithmetic, DP0,
S=1F00..1FFC and a DB exposing the hardware multiplier (00..3F or80..BF).
The native bridge also requires native mode and PB81. The low product byte
of the two factors gives the row count; zero means256 rows. The two planes
are copied in their original forward byte order, including overlapping
source and output buffers. No source bytes are cached ahead of writes.

Before execution, six RAM reads inspect position, factors and target base.
Let n be the row count, p the position word and b the target base. The first
target is uint16(b+(((p+(p&F8))&FF)<<6)); the last primary output byte is
first+(n-1)*64+floor((n-1+(p&7))/8)*512+63. The complete native contract
requires first>=2000 and last<=FFFF. This keeps outputs separate from the
work bytes and return frame; the second plane may extend into bank7F RAM.
Unsupported spans retain original execution before writes. Nine guard
fixtures check the exact six reads and final open-bus target high byte;
27 CPU guard fixtures require no accesses. Rejections preserve registers,
status and WRAM. Supported execution captures the return frame after the
body, whose outputs cannot touch that frame.

ROM callers at81:BCA3 and85:EC3A establish the ordinary tile-buffer layout.
The proof also covers zero256-row counts, boundary targets and overlapping
planes with explicitly byte-distinct payloads. An earlier all-zero payload
did not detect a deliberately reordered plane read; the strengthened
payload now detects that control. No comparison was weakened.

$86:A417 and$86:995B are verified through RTS for M8/X16, DP0 and a DB
mapping low work RAM. Their native bridges require native mode and PB86.
StepOffsets accepts S=1E00..1FFC; ScrollAdvance accepts S=1E02..1FFC so its
nested JSR reaches the supported child stack. The original product calls,
quadrant handling, decimal arithmetic, fractional offsets and nested stack
writes/reads remain observable. The minimum stack keeps child frames away
from direct-page scratch and world-map globals.

Actual-library bridge verification passes74944 cases:71602 native ROM
comparisons and3342 original-entry handoffs. The blitter contributes7232
cases; each world routine contributes33856, including all256 angles and
distance, bank, decimal and stack boundaries. All84 additional guard
fixtures and29 deliberately broken source/bridge controls are detected.

35 of95 feature routines are verified, with60 draft. All518 independent
integrated jobs and the Release build pass with401 standalone replacements.
All three new bindings have generated dispatch calls. Main and the normal
build remain unchanged; this is an isolated local checkpoint.


## Six complete field, sprite and circle roots

$80:C195 now uses a readable fixed-range flag loop. It marks slots 5 through
39, preserving the other bits and the accumulator high byte; X and final
comparison flags match the ROM. M8/X16, any DP/DB, RTL. Native PB80 and
S=1F00..1FFC keep the caller frame separate from the output bytes.

$83:F9D0 now expresses cell-pointer arithmetic directly: twice the sum of
column and the hardware row product, then the selected map's table base.
Both original JSR/RTS frames and the saved relative offset remain observable.
M8/X16, any DP/DB, S=1F04..1FFC, RTL; native PB83. Decimal arithmetic,
pointer carry into the next long-address bank, direct-page wrapping and
scratch reads overlapping nested frames are covered. An initial rewrite
omitted N/Z export; the corrected LeaveSum version passes all comparisons.

$86:8E6B now uses a readable 1000-byte clear loop, retaining each individual
bus write. Final X=15C0, Y=0, Z=1, N=0; accumulator, carry and overflow stay
unchanged. M8/X16, any DP/DB, RTL. Native PB86 and S=1F00..1FFC exclude
return-frame overlap with the clear range. The matrices include all 256
banks and four distinct old-byte patterns, including nonzero boundaries.

$85:8F4A is verified through RTS for M8, either index width and any DP/DB.
Native PB85 and S=1F00..1FFC keep the caller frame outside the 88-bit random
register. Every 16-bit direct-page value and each individual random bit,
with zero/all-one patterns, are checked. High-byte-first word writes,
rotation carry, direct-page-derived accumulator high byte and final flags
are retained. The existing readable implementation needed no rewrite.

$85:B26D is verified through RTS for DP0, DB7E, S=1F00..1FFC and any entry
widths/status. It computes the midpoint half-width table and restores the
saved status. $85:B208 is verified through RTS for M8/X16, DP0, any DB and
S=1F08..1FFC; this also supports its nested width child. Both native entries
require PB85. Every radius in binary/decimal modes is covered, with entry
width combinations, high register edges, previous-radius comparisons,
rebuild/no-change paths, saved X/DB and the original table write sequence.
The fixed scratch and stack contract keeps output and saved frames disjoint.

The six actual-library bridges pass 191104 cases: 189946 native ROM
comparisons and 1158 original-entry handoffs. All 108 additional guard
fixtures preserve CPU/WRAM and perform no accesses. Seventy deliberately
broken bridge/source controls are detected, including output byte order,
hidden accumulator bytes, decimal arithmetic, saved X and the circle rebuild
condition. Native dispatch counts are measured separately from semantic passes.

41 of 95 feature routines are verified, with 54 draft. All 518 independent
integrated jobs and the Release build pass with 407 standalone replacements.
All six new bindings have generated dispatch calls. Main and the normal
build remain unchanged; this is an isolated local checkpoint.


## Image rows and shared RAM copy

$86:9009 copies 256 bytes, $86:906A copies 128 bytes and advances both
pointers, and $86:8FF6 copies two rows separated by a $200-byte target
stride. The 128-byte routine previously used binary arithmetic for two
ADC operations even when the entry decimal flag was set. A valid RAM-stub
state (index77, seed1142049) exposes the source-pointer mismatch at WRAM
$08; final CPU flags alone miss it. Both increments now use decimal-aware
arithmetic. Source and target reads are explicitly sequenced, retaining
the original bus order rather than depending on C argument evaluation.

The row contract is X16, DP=0, DB in a first-bank WRAM mirror, prepared
MVN/RTS bytes at $057D/$0580, and S=$1F04..$1FFC. The 256-byte row needs
M8 and target=$2000..$FF00; the 128-byte row accepts either accumulator
width and target=$2000..$FF80. The two-row block accepts either width,
S=$1F08..$1FFC and target=$2000..$FD00. All three retain decimal mode,
saved frames, operand-bank writes, forward overlap behavior and RTS state.
Native bridges additionally require non-emulation mode and PB=$86.
Unsupported entry states hand off before any writes or CPU changes.

$00:057D identifies the RAM MVN/RTS kernel prepared by the original game.
Its complete buffer contract accepts X16, either accumulator width, any
DP/DB, S=$1F00..$1FFC, code in low-RAM banks, destination=$7E/$7F and
Y >= $2000 with Y+A <= $FFFF. A remains a 16-bit byte counter even in M8.
The byte loop retains forward copies and the original bank-operand reads;
it leaves A=$FFFF, advanced X/Y, destination DB and unchanged status flags.
The bounded output keeps live RAM code, scratch and caller frames intact.
Other states keep the original entry. Real field-map loading calls it in
PB=$83 and menu copying calls it in PB=$86.

The consumer binding uses dispatch_addresses for those two execution banks,
with one canonical semantic function and no duplicate alias implementations.
The bridge's fallback and RTS PC inherit PB. Manifest validation preserves
the verified-status gate and rejects conflicting, duplicate, malformed or
undeclared aliases. Eleven portable manifest tests pass; the configured
SNESRecomp emitter separately produces all eight bank/width forwarding shims.

The four actual-library ABI proofs pass 49408 cases: 28240 native ROM
comparisons and 21168 unchanged original-entry handoffs. Profiles cover
decimal flags, register widths, banks, pointer boundaries, overlapping
payloads and counts up to 57344 bytes. All 132 rejection fixtures verify
CPU/WRAM, read sequence, read values and open bus. Twenty-eight deliberately
broken source/bridge controls are caught, including decimal arithmetic,
copy counts, destination bank, strides and return frames.

45 of 95 feature routines are verified, with 50 draft. All 518 independent
integrated jobs and the Release build pass with 411 canonical standalone
replacements. The RAM kernel has actual dispatch in both PB83 and PB86;
all four copy roots have generated calls. Main and the normal build remain
unchanged; this is an isolated local checkpoint.


## Queued uploads, palette components and visible-object ordering

Six complete feature roots are selected: $80:87A7/$87FC, $81:B3F8/$B396,
$82:80A5 and $86:E686. The palette proofs cover every colour word and all
32 component values against all 256 hardware factors, including decimal
fade arithmetic. Tile filling covers every destination offset. Sorting is
stable, descending and unsigned, with the original object references retained;
the native list contract is bounded at 21 entries before any writes.

Queued NMI uploads retain the eight scroll-register writes, the four listed
requests, channel selection, request acknowledgement, tilemap DMA and saved
status. A reverse DMA can replace the listed upload's two-byte return frame.
The old reconstruction discarded those bytes and continued its normal parent.
The original instead transfers to the altered return address with the parent's
saved P still on the stack. The implementation now reads the real frame,
preserves the temporary comparison carry and dispatches that transfer. Original
ROM seed1520000 takes 92 instructions to $80:8638; before repair, C incorrectly
returned at $80:87FB and emitted 27 rather than 24 bus events. No binding of
this feature routine was enabled before this counterexample was repaired.

The actual-library ABI matrices pass 223616 cases:
210208 native comparisons and 13408 unchanged entry fallbacks, with
144 rejection fixtures and 37 broken source/bridge controls caught. A separate
reverse-DMA model writes the child frame at MDMAEN identically for C and ROM:
10240 exact native transfer comparisons cover all four slots, all four request kinds, six channel bits,
five return targets including wraparound, four DB mirrors, four stack bounds,
all entry M/X combinations and both decimal modes. CPU, all WRAM and ordered
bus events remain mandatory; the transfer PC and boundary flow are additionally
required. Five broken return/carry/dispatch controls are caught. The verifier's
default partial label for intentional control transfers is retained; ordinary
routine matrices must still have zero partial results. This models DMA's
external memory effect; it does not claim cycle-accurate PPU/DMA verification.

51 of 95 feature routines are verified, with 44 draft. All 518 independent
integrated jobs and the Release build pass with 417 canonical standalone
replacements. All six roots have actual generated dispatch calls. This is
an isolated local checkpoint; main and the normal build remain unchanged.


## Slide corrections, battle colour tables and world sprites

Nine further roots are selected: menu slide corrections $82:8AD8/$8AE9,
battle colour loaders $85:8AAF/$8AF4/$8B22 and world sprite builders
$86:E555/$E5BB/$E479/$E4E7. The sprite tile and attribute words are now read
in separate statements, so the low/high bus order is fixed by C rather than by
argument evaluation. No other behaviour changed.

The corrections cover every byte error and span pair with both index widths
and safe cursor destinations. The colour loaders cover nonzero payloads, all
data banks, direct pages, status and stacks. The sprite builders cover real
object records, mirrored poses, coordinates, attributes, tiles, decimal mode,
every sprite counter and both high bits. Native contracts: corrections M8,
DP0, hardware DB, Y <= $0800, S $1F00..$1FFC; colours M8/X16, S $1F02..$1FFC,
$8AAF also a hardware DB; sprites M16/X16, DP0, first-bank WRAM DB, counter
at most 126 (pair) or 127 and objects $1000..$1E00. Read-only preflight
checks keep their original read order before any fallback.

The actual-library ABI matrices pass 886336 cases: 877289 native
comparisons and 9047 unchanged entry fallbacks, with 264 rejection
fixtures and 54 broken source/bridge controls caught. The shared production
bridges repeat the same matrices with identical counts, and 20 further
controls against them are caught. CPU, all WRAM and ordered bus events
remain mandatory.

60 of 95 feature routines are verified, with 35 draft. The integrated 518-job
verification and Release build must pass before the staged selection is a
green checkpoint. Main and the normal build remain unchanged.


## Effect stream opcodes and battle tile ids

Four further roots are selected: the effect opcodes $81:A40B (add a stream
word to a slot field) and $81:953F (repeat counter), and the battle tile row
$85:9790 with its grid $85:972E. Their bodies were already exact; no source
change was needed.

The opcodes run from the dispatcher's JSR (abs,X) with M8/X16. Their profile
covers decimal mode, eight direct pages including pages that place the
stream pointer on the scratch word or wrap bank zero, sixteen slot indices
including the scratch word, the caller frame and the bank-7F carry, stream
pointers in WRAM, ROM and the bank-zero mirror, and repeat counts 0, 1, 2
and 255. The tile profile covers counter wrap, row offsets crossing banks
or landing on the caller frame, eight data banks and four stacks. The grid's
stores start at $2816 and cannot reach the bank-zero stack.

The actual-library ABI matrices pass 102656 cases: 101952 native
comparisons and 704 unchanged entry fallbacks. Frame, overflow,
emulation, bank, stack and width bridge mutations and 5 source
mutations are caught (41 controls). The shared production bridges
repeat the matrices with identical counts, and ten further controls against
them are caught. No consumer hook lies inside these routines.

64 of 95 feature routines are verified, with 31 draft. The integrated 518-job
verification and Release build must pass before the staged selection is a
green checkpoint. Main and the normal build remain unchanged.


## Battle actor sprite and world animations

Three further roots are selected: the battle actor sprite $81:8EEA and the
world animation start $86:E0B9 and step $86:E11F. The sprite and start
bodies were already exact. The step inlines the start and the frame-record
lookup with their JSR frames. Its object writes use DP,X addressing; with a
non-zero direct page they can land on that nested frame, and the original
then returns elsewhere. Seed 2299999 (DP $0A86, S $1F00) shows this: the
original runs away while the old C returned normally. The step now requires
DP0 and S $1F00..$1FFC before any access; other entries keep the original.

The sprite profile covers all 64 records, every sprite kind, screen edges on
both axes, list cursors on the stack and wrapping the bank, four direct
pages, data banks and stacks, decimal mode and carry; both the drawn and the
skipped exit occur. The animation profile covers all 22 objects and
out-of-table indices, ROM and RAM tables, every timer class, loop and
follow-up endings, and direct pages and stacks inside and outside the
contract.

The actual-library ABI matrices pass 101568 cases: 80400 native
comparisons and 21168 unchanged entry fallbacks. 29 bridge,
guard and profile-wide source controls are caught, including the nested
frame witness. The shared production bridges repeat the matrices with
identical counts, and nine further controls against them are caught. No
consumer hook lies inside these routines.

67 of 95 feature routines are verified, with 28 draft. The integrated 518-job
verification and Release build must pass before the staged selection is a
green checkpoint. Main and the normal build remain unchanged.


## Battle sprite lists, OAM groups, drift and slot palettes

The six roots $81:8E92, $85:8B4B/$8BC0/$8C27/$894A and $86:911F
have complete native contracts. Unsupported entries resume the original at
the unchanged entry, before any CPU update or memory write. The three OAM
passes require M8/X16, DP0 and S $1F00..$1FFC. Variable output spans must
fit in $7E:2000..$FFFF, protecting DP scratch, return frames and the bank
boundary. Drift requires M8/X16 and binary arithmetic with that stack band.
Slot palettes require X16, DP0 and counts 1..7; the consumer protects the
same stack band. Larger counts can overwrite the RAM move stub.

The actor-list root keeps the actual saved record index and child return
address. Sprite writes can rewrite either. Rewritten child returns are
exported immediately after the original RTS, with its actual CPU, WRAM and
bus effects. All 189 transfer cases match the original landing after RTS;
they are complete abnormal exits, not unimplemented child prefixes. Four
additional saved-index witnesses also match the actual-library ABI. The
longest original run takes 335746 instructions; only those four witnesses
use a 2097152-step cap instead of the default 262144. No comparison is
relaxed. The saved-index mutation is detected by two of those witnesses.

The six actual-library ABI matrices cover 102784 cases: 57421 native
comparisons and 45363 unchanged-entry fallbacks. An additional 141
entry-guard cases cover widths, emulation, program bank, decimal mode,
direct page, both stack bounds, OAM span boundaries and invalid slot counts.
They check CPU and WRAM preservation and exact permitted read-only preflight
accesses. Eleven effective source/bridge controls are caught. Historical
ineffective guard-removal controls and diagnostic crashes are retained in
the receipt but excluded from that count; source controls were repeated with
explicit selection-error diagnostics. Removing one redundant guard alone
correctly leaves the second guard effective. No recomp hook is swallowed.

73 of 95 feature routines are verified, with 22 draft. All 518 independent
verification jobs and the Release build pass with 439 standalone native
replacements. All six roots have actual generated dispatch entries. This is
an isolated local checkpoint; main and the normal build are unchanged.


## Complete world-object parent passes

$86:E430, $E3D2, $E3AB, $E2D2 and $E1B9 now have complete native
contracts. Each accepts its documented fixed-record domain and otherwise
hands off at the unchanged entry without CPU changes or memory writes.
All nested drawing, sorting, projection and slot-user passes finish natively
inside that domain. The per-frame root owns the whole sequence, including
sprite clearing and shared pattern allocation; no wait loop is bypassed.

The five ABI matrices contain 25920 cases (6072 native comparisons and
19848 unchanged-entry fallbacks), with no mismatch, partial completion
or inconclusive original run. The 4096-state families cover coordinate
edges, tilted/plain views, shared and distinct patterns, inactive unknown
kinds, flag combinations, both RAM banks, ROM-bank aliases, stack limits
and invalid counters. CPU, all WRAM and ordered bus effects are compared.
150 explicit guards additionally verify the live host-return states with
zero bus accesses. Twelve source/bridge controls detect altered user counts,
sprite counts, list/projection strides, camera position, wrong root return
frames and damaged last-user guards. Only clean diagnostic exits count.

The $C0-indexed last-user table uses direct-page bank zero, while $1365 is
absolute in DB. A damaged last-user pointer can overwrite a nested return
frame; it is rejected before writes. Update restricts DB to $86/$06 because
its pool tables are ROM absolute reads, while its inactive records need no
kind validation. The header records individual stack and payload contracts.
All executed children stay in bank $86, which has no consumer hook points.

The image parents $86:9022/$8F6F and field parent $80:F821 also preserve
the original accumulator, indexes and flags at child-entry handoffs. Twelve
before/after ROM witnesses detect the old mistakes and match the repairs.
They remain draft: exact partial handoff is not a whole-function proof.

78 of 95 feature additions are verified and 17 remain draft. All 518
independent jobs and the Release build pass with 444 native replacements.
All five roots have actual generated dispatch and wrapper entries. This is
an isolated local checkpoint; main and the normal build are unchanged.


## Complete battle render parents

$85:8A39 (frame setup), $85:8C98 (party sprites) and $85:8D2E (party
tilemap) now have complete native contracts. The shared guard checks the
fixed records' dimensions and output spans. Setup checks all used children
before its first clear, so a rejected entry has no partial effects. Party
roots require M8/X16, binary arithmetic, DP0 and S $1F00..$1FFC; setup
requires S >= $1F10 and a genuine MMIO-bank mirror. Active party dimensions
are 1..16 in each byte; inactive records are ignored. Sprite output fits
$7E:2000..$FFFF; tile output stays $7E:2800..$3FFF, outside all nested frames.
The complete passes keep the original port writes, palette streams, sprite
counting, mirrored strips, tile arithmetic, data-bank saves and CPU flags.

The three ABI matrices cover 53184 cases: 22032 native comparisons and
31152 unchanged-entry fallbacks. They include every 1..16 column/row
combination, six active records, mirrored strips, both render modes, mode
overrides, held grids, unused invalid records, stack boundaries and invalid
widths/DP/decimal/banks/output spans. There is no mismatch, partial completion
or inconclusive original run. Another 81 entry guards check all live
host-return states with zero bus accesses. Eight source/bridge controls
detect wrong mirrors, bottom tile rows, clear extent, root return frames
and weakened dimension bounds. No clean comparison is discarded.

Names now identify the proven block-size word and bound, party enable field,
mode override, tile-grid hold, WRAM port setup and color-byte streams. Shared
preflight belongs to the internal battle header. These paths execute children
only in banks $85/$81's palette and render bodies; none crosses a hook point.
A setup witness takes 5347 original instructions, all replaced by the native
parent and children. This is an instruction count, not a speedup estimate.

81 of 95 feature additions are verified and 14 remain draft. All518
independent jobs and Release pass with447 native replacements. All three
roots have actual generated dispatch and wrapper entries. This is an
isolated local checkpoint; main and the normal build are unchanged.


## Menu tile parents and original frame waits

$82:838F, $82:8069 and $82:80CA implement their complete parent contracts.
The tile work is native; the required child callback executes the original
$82:93C2 frame wait on the already-pushed JSR frame. A returned child restores
the exact parent tail, including the rectangle's REP $20. An unwound child
propagates its live CPU and unwind depth without a stale state restore.
No native loop assumes a completed frame or clears an upload request.

All three require PB $82, X16, DP0, binary arithmetic and S $1F04..$1FFC.
The rectangle additionally requires M16 and each dimension in 1..32.
Missing children and unsupported entries hand off unchanged before writes.
The old partial entry bodies are private drawing helpers; public names and
headers describe the complete callback APIs. The consumer keeps the existing
original wait/yield service, including its generated M1X0 dispatch entry.

19200 full original-ROM comparisons and 19200 preview ABI comparisons cover
all 1..32 rectangle shapes, widths, flags, positions, palette values, counter
wrap, DB aliases and rejected dimensions/stack/DP/decimal states. Another
87 live-state entry guards have zero bus accesses. Each independent ABI run
also checks 15 redirects/unwinds:135 total. Eight deliberate source/bridge
errors are detected cleanly. The symmetric counter increment is a synthetic
environment for differential execution, not a full NMI or gameplay test.

84/95 additions are verified,11 remain draft. All518 independent jobs and
Release pass with450 native replacements. The three roots have actual dispatch
and wrapper entries; their exact production bridges pass against the native
archive. This is a local checkpoint; main and the normal build are unchanged.


## Complete menu screen and image-upload parents

$86:8DD7, $82:8044, $86:9022 and $86:8F6F now implement complete parent
contracts using required original frame-wait children. The screen's wait is
$86:8B48; uploads use $82:93C2. Tile clears, OAM clear, image block moves and
transfer-register/request writes remain native in their original order.
Returned uploads restore the encoded bank and frames before the parent's
RTL. Unwinds preserve the live runtime state. No native loop skips frames,
clears a request or claims completion of hardware work.

Screen/queue entries require M8/X16, DP0, binary arithmetic and S $1F04..$1FFC.
Image parents require X16, DP0, binary arithmetic, S $1F10..$1FFC, a low-WRAM
alias DB or $7E, and the original MVN/RTS stub. The set also requires M8,
1..7 list entries with indices <=85. Missing children or unsupported entries
hand off before writes. Original children remain responsible for timing.

32768 complete-ROM cases and 32768 preview ABI cases include every DB bank,
all flags, counter wrap, 1..7 list lengths, byte indices 0..85, RAM-stub faults,
width/DP/decimal/stack bounds, transfer sizes/addresses and rejected counts.
The ABI matrix has20768 native comparisons and12000 unchanged-entry fallbacks,
120 redirects/unwinds and105 zero-access CPU guards. Nine deliberate errors
are detected cleanly. CPU, all WRAM and ordered writes/MMIO reads are compared.
Counter changes are symmetric fixtures, not a fullNMI timing or gameplay test.

Full-parent/ABI tests found two flaws missed by prefixes: restoring frames
without restoring PB after upload, and treating the grid's RTL as RTS. Both
are repaired and covered by effective source/bridge controls. Public names
now use complete callback signatures; partial bodies are private helpers.

88/95 additions are verified,7 draft. All518 independent jobs and Release
pass with454 native replacements. All four roots have generated dispatch and
wrapper entries. The exact production bridges also pass the native archive.
This is a local checkpoint; main and the normal build remain unchanged.
