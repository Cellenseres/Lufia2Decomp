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
631 functions in `metadata/functions.toml`: 631 verified, 0 draft, 0 identified, 0 disabled.
<!-- metadata-counts:end -->

The complete function list is in [FUNCTION_INDEX.md](FUNCTION_INDEX.md).
The routines that are still `draft` are the ones reconstructed after the last
verified checkpoint; [RECOMP_ENTRIES.md](RECOMP_ENTRIES.md) lists, per area,
the address, call kind and modes of each, which is what a consumer needs to
bind them.

Draft reconstructions have preliminary comparisons, with counterexamples and
incomplete paths still under repair. The later feature sections describe their
intended operation and earlier author checks; they do not establish complete
verification. Current status comes from metadata.
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

## Reconstruction checkpoints

The following sections record the progressive proofs. Earlier statements that
other routines remain draft describe that stage; current status is the generated
metadata count above and the latest complete caller contracts below.

The feature repair verifies `$85:ADE1`, `$85:AE68`, `$85:AEEB` and `$85:AA3D`.
Their complete M1X0 leaf contracts preserve binary and decimal arithmetic,
caller DP/DB, flags, stack residue and ordered writes through the original RTS.
The three wave fills pass every 16-bit phase value with varied caller state;
all four additionally pass consumer return-frame and entry-guard comparisons.
Other feature routines remain draft while their contracts and remaining paths
are reconstructed.

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


## Cursor slide and slide count

$82:8AFA counts the slide steps; every sixteenth step it runs the original
$86:8B55 sprite frame (animation, OAM rebuild and the $86:8B48 wait) through
its JSL frame at $82:8B02. $82:89FA moves the cursor slot along the major
axis with the original error corrections and calls $82:8AFA through its real
JSR sites at $82:8A98 and $82:8AD3, so a bound count runs natively inside the
slide while the frame service stays original.

The slide's entry contract is checked before any write: PB82, M8/X16, DP0,
binary arithmetic, S $1F12..$1FFC, a register/low-WRAM DB mirror, slots below
48 and a major span of at most 127. When a frame will occur, every active
sprite's timer must outlive the slide, and each descriptor must lie in ROM or
$7E:2000+ without wrapping its bank, with at most 128 OAM pieces in total. The
sprite service then cannot rewrite the pending frames or the saved registers.

88192 production-preview ABI cases: 77189 native and 11003
unchanged-entry fallbacks. The selection oracle was written separately from
the ROM services and agrees in every case. The slide's count child runs the
native count in these matrices: about 5.6 million composed count calls in the
whole-ROM cases match. Edge cases cover spans 127/128, the timer threshold,
128/129 pieces, descriptors at $1FFF/$2000 and at the bank end, unused bad
descriptors and slots 47/48. 54 zero-access CPU guards pass. Eleven
deliberate errors (wait target/frame, return frames, count site, span, timer,
piece, descriptor, slot and stack bounds) fail with clear diagnostics. Dropping
the Y refresh after the count is not detectable: both children preserve Y.

Two random-WRAM count seeds (143, 356) stay inconclusive: the original
$86:8B55 child exceeds the trace cap on random sprite data on both sides.
The harness previously left a child interpreter current after such an abort,
which corrupted all later cases; it now restores the outer reference.

90 of 95 feature routines are verified, with 5 draft. All 518 independent
verification jobs and Release pass with 456 native replacements. Both roots
have generated dispatch and wrapper entries. This checkpoint is local; main
and the normal build remain unchanged.


## NMI pad service and battle-effect frame yield

$80:8703 uploads requested OAM/palette buffers and polls the automatic pad read
before updating held/pressed buttons and repeat timers. Native calls require
PB80, M8, DP0, S1F00..1FFC and hardware DB; either index width is restored
with caller P. The consumer refuses LLE driving before any bus access because
that context has no read-side beam progression. The two unchanged core
functions confirm 25350 timer/beam states without new auto-read initiation
(at most 66 reads), plus 76050 states with automatic initiation at the VBlank
edge (at most 109 reads); 128 LLE reads
leave timer and beam unchanged. This is a read-side core witness, not a full
NMI scheduling or gameplay test. Repeat operand reads are sequenced in C.

$81:9169 is now named Lufia2BattleEffectYield: it saves the next stream pointer,
reloads the slot timer, PLX-discards the opcode JSR frame and RTS-returns to the
dispatcher exit at $81:8C59. It does not resume at the next opcode. The bridge
follows the generated non-local return routing: compiled ancestor, interpreter
owner, direct paired bounce, or dispatch. Native entries require PB81,
M8/X16, DP0 and S1F00..1FFA; decimal and DB are preserved as in the ROM.

Actual-library comparisons: 8192 complete effect body/ABI cases and 8192 NMI
cases (4663 native, 3529 unchanged entry fallbacks), no mismatch/partial or
budget-limited NMI case. Forty-eight zero-bus CPU/context guards pass; nine
effective preview errors fail cleanly. The generic NMI fixture still has 32
cases with a static busy bit and no modeled progression; this generic screen
is not the proof for this routine. The explicit per-side polling model and
unchanged-core witness establish the polling contract.

An additional 1536 directed effect body/ABI cases cover saved-register,
opcode/dispatcher-frame and stream-pointer aliases plus long-index bank
crossings at both stack edges. All match the original. The same run adds
1536 NMI cases (870 native,666 entry fallbacks), with no mismatch or cap.

92/95 additions are verified, with 3 draft and 458 native replacements.
Actual production bridges, nine canonical controls, all 518 independent
verification jobs and Release pass. Both generated dispatches are present.
The checkpoint is local; main and the normal build remain unchanged.

## Field region edges and grid-menu cursor

$80:F821 now reconstructs the complete cell-edge walk, unpack, fold and
PLB/PLP/RTL tail. It preserves the original actor mask, byte swaps, scratch
words, nested call frames and write order. A read-only replay selects finite
walks inside a checked WRAM window. The original code handles rejected walks;
no native timeout changes their behavior. PB80, DP0, binary arithmetic,
low-WRAM DB and S1F09..1FFC are required; both caller widths are restored.
Selected layers are 0/2/4/6. Walk and fold tables end below7F:C000 so they
cannot overwrite packed input, metadata or the protected caller stack.
Overlapping fold planes, including odd first-plane pointers, retain live reads.

Actual-library/production-bridge comparison covers 21821 synthetic field
cases:20981 native returns,559 finite fallbacks,281 original budget caps on
rejected paths. Another12267 layer/alias cases give11120 native returns,
382 finite fallbacks and765 rejected original caps. None of the accepted
native cases mismatches or exhausts the original budget. All29 directed
contracts pass. Synthetic fixtures contain no copied ROM map data.

$82:8720 now reconstructs cursor movement, signed wrap/limit checks, button
priority, action codes and original sound children. It requires PB82,
M8/X16, DP0, binary arithmetic, low-WRAM DB, S1F02..1FFC and X<=255.
The sound callback is required; unwinds retain the child's live CPU/frame.
65536 actual-library/production-bridge cases give32768 native and32768
unchanged entry fallbacks, without mismatch, partial or budget-limited cases.
An independently formulated selection predicate agrees in every case;
15 child redirect/unwind probes check the production dispatch ABI.

822 distinct CPU/context guards reject with zero reads/writes in the body
and production bridge (emulation is a bridge-only guard). Seven canonical
semantic errors and two production guard errors are caught. CPU, all WRAM,
ordered writes and hardware reads remain mandatory comparison fields.

$82:8B08 remains draft and unbound. Its repaired four-argument API retains
original frame/text children, but38 synthetic original text-service cases
still exhaust the default budget. Longer diagnostics reach an APU handshake;
this is unresolved fixture/service evidence, not a passing comparison.

94/95 additions have passed their individual proofs;1 remains draft.
The intended460 native replacements require the complete518-job integration
and Release build before this becomes a green checkpoint.

## Complete menu polling caller ($82:8B08)

All95 feature caller contracts are complete; current status is in metadata.
The menu polling caller preserves original sound, string and frame children via
its required child callback, including their stack frames and live CPU on unwind.
Its native entry requires PB82, M8/X16, DP0, binary arithmetic, low-WRAM DB,
S$1F04..$1FFC and a cursor-slot-minus-five byte index. Unsupported states keep
the original entry; changed child/return state transfers at its actual boundary.

Original-ROM/production-ABI matrix:65536 cases with64-instruction original-child
cuts,4778 complete returns,23894 live transfers and36864 entry fallbacks;
zero mismatch, partial or inconclusive. Earlier/later cuts cover4096 cases each:
16 instructions (193 returns,3903 transfers) and4096 instructions (4087 returns,
9 transfers). Without cuts,65536 cases give28634 returns and36864 fallbacks;
38 original children exhaust the synthetic trace budget. Each of those38 is
separately compared at a65536-instruction original-child transfer, preserving
CPU, full WRAM and ordered bus events. A transfer is not a completed child.
This proves the complete polling caller/service contract, not termination or
complete reconstruction of the original text/sprite children. Three semantic
error controls and1230 CPU/context guard cases independently exercise the proof.

## Battle effect video commands

The effect interpreter now has six complete native leaf commands in
`src/battle/battle_effect_video.c`: video-shadow writes, BG3 map selection,
background upload requests, background map release/copy, and window bands.
They preserve the original register, cursor, decimal arithmetic, stack write,
and PPU behavior. The consumer contract is M8/X16, DP0, PB81, and stack
$1F00..$1FFC; unsupported contexts transfer before bus access.

These six video commands retain original screen coordinates; widescreen
presentation belongs to the consumer. The following sections cover graphics,
control commands and their enclosing playback caller.

## Battle effect graphics and control commands

`$81:99D0` and `$81:9A12` read a graphics resource index from the effect
stream, call the original decompressor, reserve a VRAM upload record and
publish its source, destination and byte count. Their shared C implementation
preserves both distinct original command entries and call sites.

The basic effect control commands `$81:915B`, `$81:917F`, `$81:918F`,
`$81:9198` and `$81:91AD` reconstruct termination, frame delay, stream jumps
and the original repeat target. The end command retains byte underflow and
discards the opcode return before the dispatcher's return. No original bug
or frame cleanup behavior is corrected.

The command proof passes 203,264 original-ROM comparisons, including direct
portable entries, the consumer bridge, original interpreted children and
composed native decompressor/VRAM queue children. It compares every CPU
field, complete WRAM, ordered writes, MMIO reads and modeled hardware state.
There are 238 pre-access guard checks and thirteen detected ROM mutations.
Child stops, changed width/DP returns, full queue BRK continuation and the
end command's ancestor/owner/direct-bounce returns are covered separately.
Ordinary RAM reads and instruction fetches are not trace-equivalence claims.

## Effect playback parent and frame dispatcher

`Lufia2BattlePlayEffect` reconstructs the complete `$81:895E` parent through
its original `$81:8C57` RTL, including the `$81:8A76` frame loop. It preserves
the thirty script records, sixty-four actor records, both opcode dispatch
frames, actor motion, upload and background requests, and final cleanup.
Known commands execute their semantic bodies. Unresolved opcode bodies retain
an explicit original pushed-child boundary; they are not declared decompiled.
`$81:8A76` requires the enclosing parent's saved DB and existing frames and
does not receive an independent runtime binding.

The final contract proof passes 4,280 original-ROM comparisons. It covers
all 256 entry DB values, zero through sixty-four active actors, consistent
global active counts, several frames, count transitions through zero, and
cross-record motion targets. The original motion sums target velocity and
target position before storing the current actor's position. Changed child
widths, DP and decimal mode, opcode frame disposal, and nonlocal child returns
are compared at their exact continuation boundaries. Seven zero-access guards
and eleven original-ROM mutations are checked independently.

The final actual production bridge proof passes 4,414 comparisons at forty
distinct child sites. It compiles the final `actor_bridge.c` and links the
updated semantic library. Thirty-two ABI zero-access guards cover the entry
conditions across host-return modes. Ninety-six probes execute the actual
`$85:EC81` bridge up to its owning-interpreter transfer at `$85:EC94`, keeping
the live JSL frame. They cover return codes 5, 7, 9 and 11 and the real
`RECOMP_RETURN_LLE_UNWIND_BASE` and `BASE + 2` values. Native-child results
retain the framework's original per-parent decrement; explicit owning-tail
transfers retain the matching `+1` and `-1` convention. Missing native entries,
rewritten child returns and stack changes also transfer to the owning
interpreter instead of assuming an ordinary return.

A separate composed proof passes 416 whole-parent comparisons with 28,780
actual original-ROM child returns at thirty-six distinct sites. Every reached
ordinary and graphics child body executes in `interp816`. Interrupt progress
and queue consumption are explicit deterministic frame inputs in this phase.
Unknown opcode bodies remain callback boundaries in the contract and ABI
proofs. These results establish CPU, full WRAM, relevant modeled hardware,
ordered writes and MMIO reads; they do not establish PPU pixel equivalence or
the entire interrupt scheduler. Ordinary RAM reads and instruction fetches
remain outside the trace-equivalence claim.

The complete `$81:895E` parent contract is verified. Its unresolved child
boundaries and supported entry modes remain explicit. The seven complete
command entries are separately verified. The semantic package retains the
original effect cleanup behavior; rendering corrections belong to the consumer.

The end command decrements `$1BEC` before final OAM, uploads, sprite rebuilding
and the last frame wait. The caller also retains its duplicate `$15AB` stores
and original map clearing and cleanup waits. The active count can reach zero
before a queued effect-plane clear is uploaded. This ordering remains part of
the original contract, independently of consumer-side presentation policy.

## Parallel integration checkpoint

The consumer full decomp-verify passes all 525 independent jobs, with the
360,448-comparison video-command proof outside that runner. Normal Release,
decomp-disabled Release and the standalone MSVC warnings-as-errors build
pass. These checks do not establish rendered-pixel or current gameplay
interpreter-hit-rate equivalence.

## Field scene actor reconstruction

`$83:A76D` is a complete parent shell from its original PHP/SEP30 through
the PLP and RTS at `$83:A82D`. The C implementation separates the forty
actor slots, thirty-two object slots and shared object-sprite copies.
Eleven original JSR/JSL sites retain their pushed frames and live child
CPU state. Original decimal ADC behavior, indexed stores, TDC/TAX/XBA
ordering, aliases and out-of-range source selectors are preserved.

The semantic API supports all native entry widths; M1X0 metadata is
representative. Unsupported child widths and pathological loops transfer
at the exact original continuation with unchanged live state. The native
consumer requires PB83, DP0, binary arithmetic, DB00/7E/83 and stack
$1F00..$1FFC. Its guards run before memory access. A new opt-in callback
transfers missing native children to the owning interpreter; the legacy
global helper remains unchanged. Native child depth is decremented,
including active LLE sentinels; owning-tail results preserve owner depth.

Final original-ROM verification passes 5,585 direct cases and 5,777 actual
production-ABI cases, plus eighteen return/redirect probes, 704 missing
native owner transfers and 792,624 pre-access guard checks. It compares
CPU, complete WRAM, ordered semantic operand bus events and modeled MMIO
state/writes. Composed cases execute real AC7A, AB4F, AAE5 and FCD1 ROM
bodies; other children remain explicit original-code services. Twelve
semantic and five consumer-ABI fault controls are independently detected.

## Complete field session setup

`$83:ACB7` reconstructs new-session selection and the complete shared map
installation at `$83:B18E..B500`, including both original restart paths.
`$83:AD23` reconstructs the overlapping resume entry and all eleven of its
child calls. Both complete parent contracts are verified. Unknown children
retain explicit original-code services with their original pushed frames.
The permanent field system at `$83:8000` remains an original owning-interpreter
tail; its endless scheduler is not declared decompiled.

The original `$80:8281` JML enters ACB7 in M1X1; the semantic entry accepts
all native widths and performs the original width initialization. The original
`$85:840F` JML enters AD23 in M1X0 after TCS sets S to `$1FFF`. Neither root
returns with RTS or RTL, and neither needs an invented session return frame.
Metadata widths are representative; live child flags and width changes are
preserved. Unexpected child widths and bounded pathological loops transfer
at their exact original continuation instead of assuming completion.

The reconstruction separates display preparation, world-map entry, resource
loading, position installation, actor spawning and session startup. It retains
alias-sensitive record accesses, original word-TDC behavior, reload flags,
temporary stack spills, NMI publication and actions one and seven. All 68
original JSR/JSL sites are covered. The consumer accepts native binary mode,
PB83, DP0, DB00/7E/83 and S `$1F10..$1FFF`; AD23 also requires M1X0. Other
contexts transfer unchanged before any bus access.

The direct original-ROM proof passes 119,800 cases: 4,016 child unwinds and
115,784 exact boundaries, including 114,931 field-system tails. The actual
production bridge passes 144,280 cases, including 2,043 child unwinds and
132,647 exact field-system tails, plus 36 redirect probes and 1,585,209
pre-access guard checks. Comparisons include complete CPU, full WRAM,
ordered operand bus events and modeled MMIO. Twelve independent semantic
fault controls are detected without weakening the original-ROM oracle.

A separate 3,856-case composition executes 30,080 actual original child
returns, including initialization bodies and the `$83:900C` wait. There are
5,200 real native-mode NMI/RTI sequences, with the original common NMI and
published field NMI through its RAM JSL stub. The wait's live JSR frame,
returned old counter byte, word overflows and parent continuation are checked.
SPC handshakes, title selection and remaining unresolved children retain
explicit services. Hardware autojoypad inputs are controlled external inputs.

Twenty-seven tests link the production bridge and unchanged interpreter-owner
implementation. Missing wait children and final field-system tails resume
original execution with their live guest stack and consume the configured
master deadline. Ninety-nine further checks compile the regenerated `$80:8281`
caller, original `$85:83FC` final tail, HLE wrappers and production tail-context
helpers. Both JML paths reach the native setup and discard inherited return
context across all three host-return modes; the resume path resets S to `$1FFF`
before the original wait frame.
The current interpreter treats direct-page WRAM reads as
dynamic; these tests do not claim automatic quiescence detection for the wait.
They also do not establish PPU pixel equivalence, SPC execution or cycle-perfect
interrupt scheduling. Interpreted JML instructions remain under their original
interpreter owner; native bindings select the generated recomp callers.

## Session integration checkpoint

The consumer selects all 478 verified metadata entries, with zero draft
fallbacks and 1,916 actual generated HLE wrappers. The memory map records
338 locations; neutral one-byte records do not imply proven array extents.
These are entry and documentation counts, not whole-ROM completeness or
current interpreter-hit-rate measurements.

The full consumer decomp-verify passes 528 independent jobs, plus 360,448
original-ROM effect-video comparisons outside the runner. Normal Release,
decomp-disabled Release and standalone MSVC W4/WX builds pass. The semantic
sources and production bridges are the same versions used by the proofs.
Original field-system ownership, unresolved child contracts and the stated
hardware/timing limits remain unchanged.

## Counted battle-effect loops

Seven complete commands at $81:94EB, 9500, 9515, 952A, 9553, 9567 and
957B reconstruct four independent counted loops. Each start records its byte
count and next stream position; continuation decrements the original byte
and jumps unless it becomes zero. Zero wraps to 255. The first continuation
at $81:953F was already verified. All eight now execute inside the native
effect parent instead of its unresolved-child callback.

The consumer contract is native PB81, M8/X16, DP0, S=$1F00..$1FFC,
with any DB and decimal mode. Other entries hand off before bus access.
The shared implementation retains stream/slot/stack overlap, long-indexed
address carries, high-first word increments and rewritten RTS destinations.
Count and target remain bus-backed fields rather than cached host records.

The combined leaf and actual production-bridge matrix passes 229,376 original-
ROM comparisons, 238 pre-access guards and fourteen detected mutations.
Every byte count is exercised in both decimal modes. Comparisons cover all
CPU fields, complete WRAM, ordered writes, MMIO reads and modeled hardware;
ordinary RAM reads and instruction fetches are outside the trace claim.

Expanded complete-parent checks cover 1,720 portable and 1,854 production ABI
cases, including all four levels together, byte-zero wrap, original opcode
frames and owner transfers. A further 1,344 compositions execute 393,508 actual
original child returns. These use deterministic interrupt/upload inputs;
they do not claim pixel equivalence or a complete NMI scheduler.

The full consumer regression passes 532 independent jobs. Standalone W4/WX,
Release game and decomp-disabled Release builds pass. The consumer selects
485 verified entries, with 1,944 actual generated bridge invocations.

## Battle-effect spawning and selection

Eight complete entries reconstruct effect allocation and creation: $81:8F91,
8FAB,8FC5,905B,90DF,91F2,920F and929E. Actor/script allocation scans the
original fixed pools. Full pools retain their original endless branches,
including nested JSR and PHY frames. Source/position/attribute operands,
parent parameters, byte versus word active counters and all target records
remain bus-backed. Overlapping activation stores and word decrement order
are retained. The native contract is PB81,M8/X16,DP0,binary arithmetic;
minimum stack is1F00 for lookup,1F02 for creation,1F04 for target lists.

Three complete commands at $81:91B7,94CA and9BA3 reconstruct conditional
field branches, portrait-indexed stream selection and argument skipping.
These accept either decimal mode and any DB. They retain stack and slot
aliases, decimal additions and original width changes. The invalid portrait
path transfers to the original BRK at94EA; it is not repaired in semantic C.

The actual production-source matrix passes262144 allocation/creation and
98304 selection comparisons,414 pre-access guards and22 altered-ROM controls.
All256 target counts and counter overlaps are covered independently. Valid
whole-parent fixtures cover empty,one,four and full-pool target lists. The
combined caller passes802 direct and935 production ABI cases. Another576
compositions execute110986 actual original child returns. All14 integrated
loop/spawn/selection entries are exercised inside the complete effect caller.

Comparisons cover CPU state,full WRAM,ordered writes,MMIO reads and modeled
hardware. Ordinary RAM reads/instruction fetches,pixel equivalence and a
complete NMI scheduler are outside the proof. Unresolved children retain
explicit original frames and interpreter ownership.

The full combined regression passes537 independent jobs. Two unchanged
object-update tests exceeded their120-second limits under concurrent build
load; both pass serial reruns with unchanged inputs and checks. The expanded
caller matrix passes2674 direct and2807 production ABI cases;2304 compositions
execute444108 actual original child returns. Standalone W4/WX and Release
game builds pass. Selection496;1988 real generated bridge invocations.

## Battle action coordinates and effect phases

Seven complete entries reconstruct the sprite table lookups at81:FBC6/FB8E,
target-mask resolution at81:B228, sprite/effect coordinates at81:B7EF/B80A,
and action effect phases at81:B139/B174. Lookup tables are read from the
original ROM. X16 is required; both accumulator widths, decimal modes and
all DP/DB values are accepted by the lookup contracts. The other entries
require PB81,M8/X16,DP0,DB97 and binary arithmetic. Their stack minimums are
1F00,1F03 and1F05 as documented in the verification receipt.

Target resolution retains the original random-table selection and empty-mask
loop. The two coordinate paths retain their separate thirteen/fifteen-byte
position records, packed ROM fields, special target branches and original
hardware multiplication order. Child lookup calls retain their JSL frames.
Action phases preserve all14 original child sites, register-width changes,
stack-backed wait counters and writes to the sprite rebuild marker. Unknown
children remain explicit services with live frames and interpreter ownership.

The combined direct/production matrices pass524288 lookup and98306 target
comparisons,180 pre-access guards and12 detected altered-ROM controls. All
65536 accumulator inputs in both widths are covered for each lookup. Target
requests cover all256 byte values with varied original active records. Two
original-loop continuations compare exactly65536 random picks.

The action-stage matrix passes4236 comparisons,28 child unwinds,112 post-child
mode continuations,82 pre-access guards and16 detected controls. A further
two streaming comparisons verify the exact original state after4096 frames;
each executes8194 child services. Whole CPU/full WRAM and ordered writes/MMIO
reads are compared. Ordinary reads/fetches and pixel/full-NMI scheduling are
outside these contracts; synthetic child internals are not claimed.

The completed checkpoint passes all540 independent decomp-verify jobs,
the standalone W4/WX build and the normal Release consumer build. Runtime
selection contains503 verified entries,0draft and350 WRAM locations;
generated bank bodies contain2016 native bridge invocations. These counts
describe registered contracts and call sites, not whole-game coverage.

## Core battle action presentation

Nine complete entries add action preparation81:AFC4, actor execution81:B057,
presentation81:B08A, no-action81:A8A7, attack81:A8A8, spell81:A968/A977
and item81:AA1B/AA2E. The paired spell/item entries retain their original
shared continuations. Original name and message fields come from the existing
54-byte item/spell record buffer; no second overlapping record is invented.

The contracts require PB81, M8/X16, DP0, DB97, binary arithmetic and entry
stack1F00..1FFC. Unknown children remain explicit services with their exact
live frames. Native preparation retains its original JSR/RTS boundary;
other entries use JSL/RTL. Every post-child mode failure resumes at the
original continuation. Special action results preserve terminal owners
81:8855/88D5, stack-backed waits and word counter saturation.

The preparation/presentation proof passes24876 direct and production-bridge
comparisons,60 child unwinds,4364 owner boundaries,123 pre-access guards
and24 altered-ROM controls. All30 child sites are exercised, including1056
original saturated result paths. Four extra streaming comparisons verify
the exact state after4096 frames.

The core-handler proof passes49392 direct and production-bridge comparisons,
48 child unwinds,192 post-child boundaries,246 guards and24 detected ROM
mutations. All24 child sites are exercised. Four extra streaming comparisons
verify both spell error wait entries after4096 frames. The first item loader
keeps M16; later loading keeps M8. Status rejection, MP subtraction/borrow,
item-effect wrapping and message fallback remain literal ROM behavior.

Whole CPU, full WRAM, ordered writes and modeled MMIO reads are compared.
Ordinary reads/fetches, pixel/full-NMI scheduling and synthetic child-service
internals are outside these contracts. The32-route dispatcher81:A832 remains
original; these entries alone do not constitute that complete dispatcher.

The complete core-action checkpoint passes all542 independent verification
jobs, the standalone W4/WX build and the normal Release consumer build.
Runtime selection contains512 verified entries,0draft,359 WRAM locations
and2052 actual generated native bridge invocations. All nine entries use
their own bindings; the larger action dispatcher81:A832 remains original.

## Complete battle action dispatch

Seventeen complete entries close the original action dispatcher 81:A832,
its remaining action handlers, the repeated effect phase and three record
lookups. All 32 dispatch indices and their 18 distinct handler targets are
implemented. The dispatcher reads the original ROM table; an altered or
unsupported target retains its exact original continuation.

Control entries require PB81, M8/X16, DP0, DB97, binary arithmetic and
stack1F00..1FFC. Record lookups require M16/X16 and retain arbitrary DP/DB.
The IP zero-parameter BRK at 81:AB6A and invalid capsule BRK at 81:AD8C
remain original owners. Followup dispatch retains its original tail into
81:A832. No child is replaced with invented behavior: unknown services keep
their exact JSR/JSL frames, post-child guards and original continuations.

The combined targeted proof passes 633434 direct/production-ABI comparisons,
676 pre-access guards and 86 altered-ROM controls. The dispatcher contributes
132472 comparisons across all 140 reachable child sites. The additional
repeat-effect entry contributes twelve sites outside the dispatcher table.
Three lookup sweeps cover every word input in both semantic and bridge paths.
Twenty-eight streaming comparisons preserve the exact original state after
4096 frames. The action-window route keeps its 672 ordered zero/one WRAM-port
write pairs, live data bank and original word counter saturation.

Whole CPU, full WRAM, ordered writes and modeled MMIO reads are compared.
Unknown child internals, ordinary reads/fetches and pixel/full-NMI scheduling
are outside these proofs. No current runtime hit rate is inferred from the
number of bindings. Complete regression and consumer-build results follow.

The complete action checkpoint passes all547 independent verification jobs,
the standalone W4/WX build and the normal Release consumer build. Runtime
selection contains529 verified entries,0draft,360 WRAM locations and2120
actual generated native bridge invocations. All17 new entries have their
own bindings, with exact guard and original-owner fallback behavior.

## Object coordinates and map addressing - 2026-10-07

Ten complete coordinate, movement and map helpers preserve fixed-point rounding,
fine-coordinate scaling, packed map indices, tile-height bits and the original
RTS/RTL wrappers. All65536 word or coordinate-pair inputs plus4096 additional
bank/index contexts pass direct and production-shaped bridge comparison:
1114112 original-ROM comparisons,312 zero-access guards and12 detected controls.
The multiplier remains a bus-visible hardware operation. Probe conversion keeps
carry rounding, X/Y order, register widths and 16-bit wrapping. Long wrappers
keep their original nested JSR stack bytes. Context is binary M1X0,PB83,DP0,
any data bank andS1F00..1FFC. Outside this contract the original entry is used.
CPU,full WRAM,ordered writes and modeled MMIO reads are compared; ordinary
reads/fetches,pixels and full NMI scheduling are outside this proof.
Shared WRAM fields already have names; no new memory-layout claim is needed.
Two word-entry movement helpers additionally pass278528 ROM/production-shaped
ABI comparisons,78 guards and8 detected controls. Original live-bank divider
and Mode-7 multiplication, word wrapping and sign-bit branches are preserved.
The sharedE650 return is stopped only with the original outer stack depth.
Five complete Object VM routesFF/8C/F0/1E/8B now use these native operations;
655360 full-route comparisons and12 controls pass, plus8192 whole-owner cases
and512 existing entry guards. Changed internal JSR returns retain their exact
original continuation. The VM's remaining unsupported routes keep boundaries.

The complete coordinate batch passes all551 independent verification jobs,
the public production bridge, strict W4/WX and the normal Release build.
Selection contains539 verified functions,0draft,360 WRAM locations and2160
actual generated native bridge invocations. All10 new helpers are bound.
Five further Object VM routes run natively inside their verified owner.
The whole-object fixture now resets modeled hardware together with WRAM
between C and ROM runs and compares multiplier/divider/Mode-7 state.
Three failing seeds came from stale fixture hardware, not semantic drift;
all16384 object cases pass with equal starting hardware. Negative controls
reproduce the mismatch when that reset is disabled. No expectations weakened.

## Field probes and sprite uploads - 2026-10-07

Five complete probe-record wrappers at83:FB2E/FB51/FB61/FB8B/FB9B preserve
original attribute branches, pending-record search, accumulator bytes and
nested JSR/JSL frames. The original unreachable pending fallback is retained.
696320 direct and production-shaped ABI comparisons,195 guards and10 altered-ROM
controls pass. Context:M1X0,PB83,DP0,binary,S1F00..1FFC,anyDB. Other entries fall
back before any bus access. Both biased record bases refer to the existing byte7/byte8 header arrays.
The original address bias of16 bytes is preserved, including synthetic IDs.
No overlapping or invented persistent records are introduced.
Five complete collision/input helpers at83:FBF1/FBFE/FC3C/FC56/FC69 add
696320 direct and production ABI comparisons,195 guards and10 controls.
Original live-bank accesses,40-actor scan,packed map coordinates,occupancy
clear and input transitions are retained. The057C gate remains an unknown
byte in the metadata; no wider field meaning is inferred.
Four complete direction/position helpers at83:FBBD/F49A/F4A7/EC5F add
557056 ROM/ABI comparisons,1416 guards and10 controls. The direction table
is read from the ROM; nested short/long frames,position wrap and rounding
remain literal. Context:M1X0,PB83,DP0,binary,anyDB,S1F04..1FFC. FBBD's
native contract accepts directions0/2/4/6; other values fall back before
bus access. The next-object probe retains unknown table continuations:
504 additional ROM/ABI comparisons verify all252 unsupported direction
bytes at83:FB17 with the original JSL frame intact.
Three complete sprite-resource entries at83:AB4F/ABCC/ABE9 add417792
direct/production ABI comparisons,117 guards and6 controls. Record offsets,
slot-release range and VRAM-address math retain carry and byte wrap. A zero
release count performs the original256 writes. Context:M1X1,PB83,DP0,
binary,anyDB,S1F04..1FFC; ABE9 returns inM0X1. Allocation reset83:AB61
was already verified and is not counted or replaced in this batch.
Object route11 now copies frame graphics natively. 131072 complete-route cases
cover two64-byte or four128-byte rows, wrapping indices, overlap and ordered
WRAM writes, plus7 controls. The original source strides, DB restore and four
operand advances remain. RouteFA queues deferred sound through its existing
native semantic helper:another131072 cases and2 controls pass, including both
branches of the sound gate and original nested JSL bytes.
Another8192 whole-owner cases and512 guards pass
with this transfer and the previously proven coordinate routes in one stream.
CPU/full WRAM/ordered writes/modeled MMIO reads compared. Ordinary reads,
pixels,full NMI scheduling and measured interpreter savings remain unclaimed.

The field-probe and sprite checkpoint passes all557 independent verification
jobs,public production targets,W4/WX and normal Release. Runtime selection is
556 verified,0draft,362 WRAM entries,2228 actual generated bridge invocations.
Seventeen new helpers have bindings; two new routes stay inside the existing VM
owner. Original bugs,transfer order and sound gating are preserved.

## Event map state and sprite allocation - 2026-10-07

Nine complete functions are verified and bound:83:AB7C/B851/B882/F442/F750,
80:D136/D15B/D18C/D227. Sprite allocation retains its original accumulated
free-slot count,byte wrap,carry and failure exits. Unsupported zero counts
fall back before bus access. The frame-free allocation body is shared with
the previous native callers without inserting additional original frames.
Map-region save/restore,origin normalization and48-record flag marking retain
word reads,byte geometry,DB changes and the original horizontal/vertical edge
rules. Five persistent fields are catalogued. Record searches preserve the
ROM sentinel,stride,live pointers,index wrap and carry; zero stride falls back.
Object attribute/tile entries expose existing literal bodies,retaining nested
frames,word writes,layer addressing and flags inside their previous callers.
The complete Object VM F3 route uses these native record/direction probes;
unknown direction branches keep the original continuation at83:FB17.
1384448 helper/route comparisons,8192 whole-owner cases,366 helper guards,
512 owner guards and35 detected altered-ROM controls pass. Direct CPU state,
full WRAM,ordered writes and modeled MMIO reads/state are compared.
Production bridges,W4/WX,normal Release and all563 independent jobs pass.
565 verified,0draft,367 WRAM locations and2264 generated native invocations.
Ordinary-read trace,pixels,full NMI timing and actual hit-rate gains unclaimed.

## Position event callers - 2026-10-07

80:E7FA/E7DF now call the complete native record/event helpers while retaining
original carry,selection,redraw flags and all nested JSL frames. Both complete
entries are verified and bound. Object routes86/87 preserve original height
checks,mode writes,inverted carry and conditional script jumps. Unknown
movement directions keep their original table continuation; event-list limits
retain live child frames. No standalone binding is added for opcode routes.
540672 direct/ABI/route comparisons,8192 whole-owner cases,78 helper guards,
512 owner guards and12 altered-ROM controls pass. CPU,full WRAM,ordered writes
and modeled MMIO state match ROM. W4WX,Release and566 independent jobs pass.
567 verified,0draft,368 WRAM locations,2272 generated native calls. No claim
of measured runtime hit-rate gain,pixel equivalence or NMI timing changes.

## Event object-region lifecycle - 2026-10-07

80:D0AC/D112/D1E1/D19F/D0BA/D077/CE5C/CE7E are complete verified functions with native
bindings. Event routes06/0E/91/A8/1F/8D/92/8E/90/7A call these helpers inside their existing
owner. Destination operands,object headers,region copies,attributes,48-record
removal/restoration and layer redraw retain original state and call frames.
The byte-valued source selector,full byte geometry,overlapping script variables,
DB-relative accesses and original literal renderer at83:8E76 are preserved.
Nested and opcode-call returns validate actual stack bytes; a copied region
may overwrite a return frame,which transfers to the original actual target.
Unknown child work retains its exact pushed frame and continuation. No opcode
branch gets an independent runtime binding. Two persistent fields are named.
1167360 direct/ABI/route comparisons,32768 complete Event timer/ABI owner cases,
16 exact copy-child continuations,312 zero-bus guards and36 detected altered-ROM
controls pass. CPU,full WRAM,ordered writes and modeled hardware state match.
Production bridges,W4WX,Release and all575 independent jobs pass.
Legacy CBAE copy stops also require the exact write ordinal. The bridge
reference permits the same10-million instruction budget as the semantic test.
All16384 semantic and8704 bridge cases pass without changing ROM behavior.
575 verified,0draft,370 WRAM fields and2304 generated native invocations.
No pixel,NMI-cycle or measured interpreter-hit-rate claim is made.

## Actor event starts and refresh - 2026-10-07

83:C0EF/C0FA/F0BC/EF6E/F205 are complete verified helpers with native bindings. Leader
probe coordinates,control-latch acknowledgement and follower draw flags
preserve original state,bus order and loop behavior. Object24/2D/88 start
header events within the existing Object owner. Event8C/B9/BE refresh party
actors or recenter layers within the existing Event owner. Actual return
frames are checked; unknown actor children retain their original call frames.
ObjectEF clears the follower list through the shared verified object removal.
The former duplicate despawn body now uses that same implementation.
The Object script caller validates its actual RTS frame. A targeted random
composition that returns to83:7181 matches the ROM,along with8704 additional
whole-owner/ABI cases. The former assumed return wrongly skipped that target. Matching
positions wake the original pending script,including its word-sized flag reads.
The Event actor-action caller retains the original D350 call frame for
nonzero Direct Page, outside its proven native contract. Original CBAE
case7663 rngCF2EF30DFE84C3CC passes at that exact child boundary.
One persistent control-change byte is named. No partial opcode binding added.
1232896 direct/ABI/route comparisons,32768 complete owner cases,1219 guards
and24 detected altered-ROM controls pass. W4WX,Release,production ABI and
all584 independent jobs pass. 580 verified,0draft,371 fields,2324 native calls.
No pixel,NMI-cycle or measured interpreter-hit-rate claim is made.

## Field object control - 2026-10-07

Twelve complete scene-record, object-collision and sprite-resource helpers
are verified and bound. Event BB and Object 27/F4/21/FE/E0/10/2A use native
helpers within their existing owners. Record search failures, arithmetic
carry, duplicate size reads, leader reactions and exact frames remain
literal. Object 10 handles the four original palette/graphics resources;
other resources retain the exact original handler boundary. Object 2A
retains the original call frame when handing off its push-object child. Five additional
WRAM fields are named. Focused ROM/ABI/owner checks and altered-ROM controls
pass, followed by production ABI, W4WX, Release and 600 independent jobs.
592 verified, 0 draft, 376 fields, 2372 native calls. No measured hit-rate
claim. Private proof records retain the individual test counts.

## Field script transitions - 2026-10-07

Six complete push-object, actor-slot and object-draw helpers are verified
and bound. Object2A now executes its push child natively. Event5A-5D share
that same original implementation. Event1A resolves its original header
and applies the existing area transition through known native children;
unknown audio and special transition children retain exact call frames.
Original flags, failed free-slot selection and map-attribute branches are
preserved. Focused helper, route, owner, ABI, guard and altered-ROM proofs
pass, followed by strict C, Release and 604 independent verification jobs.
598 verified,0 draft,376 fields,2396 generated native calls. These counts
describe bindings and verification, not a measured interpreter hit rate.

## Event sound and object conditions - 2026-10-07

Eight complete sound-command and object-header/condition helpers are
verified and bound. Event0B executes its literal sound chain; Event8F
uses the original condition table and existing object-region operations.
Sound waits and unknown message children retain exact pushed call frames.
Driver mode accepts every native M/X combination and preserves decimal
status. Sound wrappers accept their documented binary-mode contracts;
the immediate wrapper requires M8. The five field helpers require PB83
or PB8E as appropriate, M8/X16,DP0,binary mode,S1F20..1FFC.

Original-ROM helper/caller, composed child, route, whole-owner, production
ABI, altered-ROM control and entry-guard proofs pass. Strict C, Release
and612 independent verification jobs pass.606 verified,0draft,380fields,
2428 generated native calls. These counts are not measured runtime hits.

The earlier event survey overran the table by one entry. The table ends
before80:E722: valid IDs00..BE. BF reads following instruction bytes;
the apparent96AF target overlaps a music-call operand. No artificial
kernel-frame function or binding was added. Unknown routes stay exact.

## Secondary actor position event - 2026-10-07

The complete secondary actor owner now runs its existing verified
position-event child on map attribute9. Preserve the original JSL,
child bank and live continuation frames.32768 forced route comparisons,
32768 whole-owner comparisons and32768 production-ABI comparisons pass.
Two in-memory ROM controls detect wrong dispatch/continuation behavior.
The change shares the full625-job verification batch with sound callers.
Unknown child contexts retain the exact original event-entry boundary.

## Original sound driver callers - 2026-10-07

Twenty-five complete sound-driver request/resource helpers are verified
and bound. Save/restore order, original widths, DB-relative versus
long ports, repeated resource-byte writes and pushed waits remain literal.
The resource-table initializer retains every one of its32 child calls.
Driver-status polling retains original iterations and uses an exact
continuation after65536 polls when the original routine never finishes.
No host sound-wait optimization was added.

Driver callers:122880 semantic and122880 production-ABI comparisons,
16 exact child unwinds,12672 entry guards,23 effective ROM controls.
Table callers:49152 semantic and49152 production-ABI comparisons,
8 exact child unwinds,4224 entry guards,13 effective ROM controls.
Four nonreturning cases compare65536 polling iterations each.
Transfer helpers:49152 semantic and49152 production-ABI comparisons,
3 exact child unwinds,5760 entry guards,10 effective ROM controls.
Strict C,Release and625 independent regression jobs pass.
631 verified entries,0drafts,382 fields,2528 generated native calls.
These are static selection counts, not measured interpreter hits.
