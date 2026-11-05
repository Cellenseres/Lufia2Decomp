## Unrooted popup and background-clear fragments

`Lufia2BattleBuildLegacyPopup` reconstructs `81:C467..C476`. It sets popup
kind `0F`, then calls the existing popup builder through the original `C473`
JSR frame. The inherited kind behavior is preserved, including the absence
of the digit/glyph write used by other kinds. The algorithm is shared with
`Lufia2BattleBuildPopupRecord`; this is an additional entry variant.
Its scoped contract is PB81/DB7E, M1X0, DP0, binary arithmetic and stack
1F00..1FFC. Unsupported contexts hand off without access.

`Lufia2WorldClearLegacyBackground` reconstructs `86:C5F4..C5F7`: the original
absolute `STZ` at offset `11E1`, followed by the RTS boundary. Both accumulator
and index widths are accepted. The data bank controls the destination, and
M controls its write width. DP, decimal mode, stack, registers and flags are
preserved; no entry frame is invented. Its guards reject a different program
bank or invalid width flags. This is a small original leaf, not a new subsystem.

At one shared checkpoint each MSVC/Clang passes 8,704 original-ROM popup cases
(4,352 existing-builder plus 4,352 new-entry cases), nine no-access guards and
two altered-ROM controls; and 16,384 clear-leaf cases, three guards and two
controls. The clear matrix covers every data bank, both M/X widths and all
sixteen carry/zero/negative/overflow combinations. CPU, complete WRAM and
ordered data accesses match. Popup children execute without stubs.

Neither original incoming caller is established. No canonical function
metadata or runtime binding is added. Scoped fragment evidence does not
establish organic reachability, hardware timing or improved performance.
Tests remain private in the consumer. The combined sources pass strict
Windows/GCC/Clang library, C++ header, actual archive-export and metadata checks.
The CLI binaries remain byte-identical; prior CLI behavior proof is reused.
Full unrelated regression, game builds and consumer admission/cost tests are
deferred because consumer integration did not change.

## Unrooted alternate battle frame

`Lufia2BattleDrawLegacyTileFrame` reconstructs original `81:E40A` through
its `81:E478` RTS boundary. It selects first tile `10F6`, then shares the
existing frame algorithm with `Lufia2BattleTileFrame` at `81:E405` (tile `10F1`).
The original bank save/restore, scratch writes, row loops and child JSR frames
remain intact. This adds an original entry variant, not another frame algorithm.

The scoped alternate entry accepts PB81, M1X0, DP0, binary arithmetic and
stack 1F08..1FFC. DB is preserved. Unsupported contexts hand off at E40A
without access; no incoming call frame is invented. Each MSVC/Clang proof
compares 2,248 original-ROM CPU/full-WRAM/ordered-bus cases across both entries,
eight no-access guards and two altered-ROM controls. Dimensions include every
0..31 pair and the cross-product of 0/1/2/3/31/32/127/128/254/255.
No child is stubbed. Strict Windows/GCC/Clang libraries, C++ header, actual
archive exports and metadata checks pass. The resource CLI binaries are
byte-identical, so their prior behavior proof is reused. Full unrelated tests
and game builds are deferred; no consumer code changed.

The original incoming caller remains unproved. No canonical metadata or
runtime binding is added, and no gameplay or performance improvement is claimed.

## Unrooted slot-offset calculation

`Lufia2FieldLegacyComputeRecordOffset` reconstructs `80:D835` through the
`80:D848` RTS boundary. It doubles the input byte, adds the indexed byte at
`7F:D4E6`, then combines that result with scaled DP slot `A7` through scratch
bytes `54/55`. Original carries participate in the arithmetic; the calculation
must not be replaced by an ordinary multiplication formula. The accumulator's
high byte and the final index width remain observable.

The scoped contract accepts PB80, M1X0 or M1X1, DP0 and binary arithmetic.
DB is unused. It invents no entry frame and returns before the original RTS.
Other contexts hand off at D835 without memory access. Each MSVC/Clang proof
compares 135,168 original-ROM CPU/full-WRAM/ordered-bus cases, six no-access
guards and two altered-ROM controls. The matrices cover all input/source-byte
pairs for both index widths, plus 4,096 wide-index samples.

A bounded search after known terminal instructions identified this region.
Its 20 bytes have no identical second ROM region. No incoming caller or
original entry width is established; this is new scoped calculation coverage,
without canonical function metadata or a consumer binding. It makes no runtime
speed or gameplay-reachability claim. Existing canonical algorithms are reused
when original copies are found; unrelated table bytes are not promoted to code.

## Unrooted battle condition-byte branch

`Lufia2BattleLegacyCompareConditionByte` reconstructs original `85:B56E` to
`85:B45E`. It reads one script byte, compares it with the full 16-bit battle
condition, and either takes the existing script jump or consumes its word.
The existing operand readers and jump tail retain their original stack frames,
flags, pointer wrap and ordered bus accesses. No duplicate reader is added.

The scoped entry requires PB/DB85, M1X0, DP0, binary arithmetic and stack
1F08..1FFC. Unsupported contexts hand off at the original entry without access.
MSVC and Clang each pass 77,824 original-ROM CPU/full-WRAM/ordered-bus cases,
nine no-access guards and three altered-ROM controls. The comparison matrix
covers all byte pairs; additional cases cover equality, cursor/base wrap and
nonzero condition high bytes. This is boundary verification, not a canonical
function promotion or runtime performance proof. No nominal battle opcode or
original incoming call is established, so no consumer binding is invented.

Six other apparent calls belong to IP record headers selected through original
84:8F10 pointers: 84:9084/946B/949D/94CB/94E5/9502. Six isolated native reader
cases reach their header bytes; controlled services stop before action execution.
This data evidence is separate from original-ROM differential verification and
does not prove organic selectors or rule out hypothetical overlapping code.

# Unreferenced ROM fragments

The three scoped boundaries have original-ROM differential evidence. They
have no function metadata or runtime selection: their presence does not prove
that ordinary gameplay reaches them, and the triangle parent entry remains
unresolved. These are reconstructed fragments, not three newly bound functions.

`Lufia2RasterizePreparedTriangle` represents `$83:FD0B..FEC3`, rather than an
invented function entry. It starts with an existing raster header and consumes
the saved data-bank/status pair already on the stack. It does not manufacture
a prologue. The return boundary names the original RTS at `$83:FEC3`.

The fragment prepares row bounds for three coordinate pairs at DP `$16..1B`,
orders the other two vertices, prepares three edges, appends the table tail,
and reconciles crossed adjacent bounds. Original children at `$83:FD9F`,
`$83:FDE7` and `$83:FE44` use the existing `$83:FEC4` edge rasterizer through
their original JSR frames. The asymmetric negative edge counts and inherited
carry in the first row-address calculation are retained.

The current continuation contract requires native mode, PB `$83`, DB `$7F`,
M1X0, DP zero, binary arithmetic, stack `$1F04..1FF8` and a child service.
DP `$F1/$F3/$F4/$F6` holds row storage and prepared extent information. DP zero
initially supplies a header byte and is subsequently reused as the edge
stride. No equality between that header byte and the prepared top row is
assumed. Output addressing retains the original distinction between absolute
parent writes and the child's long indexed writes.

`Lufia2LegacyPreserveRegisters` reconstructs `$83:FF31..FF3D`, including its
stack writes and width restoration. It is not equivalent to an empty RTL.
Its complete 13-byte ROM sequence has no second exact copy in the supported
ROM. No ordinary caller has been established.

`Lufia2LegacyInitializeState` reconstructs `$83:FF3E..FFB9`. The neutral state
fields at `$1360..1385` have no asserted gameplay purpose. The original long
call at `$83:FF7A` targets `$84:B51D`, whose current-ROM path ends in RTS.
The child service must preserve that nonlocal result. The implementation does
not replace it with an assumed long return. A compatible returning service
can also exercise the remaining original state writes; incompatible execution
widths hand off at the exact child continuation. The complete 124-byte region
has no second exact ROM copy.

CPU state and ordered accesses remain explicit because these unreferenced
callers do not supply a proven contract permitting their removal. Structured
phases and shared existing operations replace instruction-address dispatch.
No performance improvement or whole-function contract for the raster parent
is claimed.

## Shared verification

MSVC and Clang each pass 14,256 original-ROM boundary comparisons and 17
no-access admission guards. CPU state, complete WRAM, ordered data/MMIO
accesses, divider state and original child frames agree. Four independent
in-memory ROM mutations are detected by each compiler.

Coverage includes 4,096 register-preservation cases across all entry M/X
modes; 512 executions with the actual legacy child and its nonlocal RTS;
1,024 compatible returning-child cases; 8,192 triangle cases across byte
coordinates and both accepted stack endpoints; 48 child unwinds; and 384
child-context handoffs. Controlled returning services test the original tail
contract, not a claim that the original legacy child returns normally.

The triangle resumes in the child's live program bank after an incompatible
return. Width, DB, DP and decimal changes are checked at each of its three
child boundaries. No guessed caller, clock aggregation, batching hook or
performance improvement is introduced. Existing runtime bindings are unchanged.

Strict standalone GCC and Clang library/CLI builds pass. Both archives contain
all three symbols; the documented CLI `list` command produces identical output.
The public headers also compile as C++17. Two existing missing `<stddef.h>`
includes were corrected in transition setup and menu-string code. No shared
semantic helper or consumer source changed, so unrelated full regression and
runtime performance replays were deferred.

## Equipment cursor and secondary collision branches

`Lufia2MenuContinueLegacyEquipmentCursor` reconstructs `$82:B830..B845`.
It selects style 5 for cursor slot 7, calls the original sprite-frame child,
and selects style 3. The original three JSR/JSL frames and input CPU states
are preserved. Execution hands off at `$82:B846`, before the existing common
menu close sequence. The five known inline command targets do not enter this
fragment; it is not a sixth inferred command or a new menu entry.

`Lufia2ActorContinueLegacyCollisionBranch` reconstructs `$83:D858..D87C`.
It reads the actor-state flags through the original DB/Y address, optionally
calls the existing collision probe, then either advances the script position
by three or reads its branch operand and adds `$8000`. Both paths hand off at
`$83:D5C3`, before the existing redispatch. No script-dispatch body is copied.
Incompatible child contexts hand off before the original continuation branch.

The two branches pass 13,152 original-ROM boundary comparisons and 17 no-access
guards with each of MSVC and Clang. This includes 12,288 regular cases, 128
child unwinds and 736 changed-child contexts. CPU, full WRAM, ordered accesses,
original stack frames and child input CPU states agree. Two altered-ROM
controls per compiler fail as expected. Controlled returning child services
test these parent contracts; they do not reverify child internals or establish
an original caller. Neither fragment adds function metadata or a runtime hook.

The shared standalone GCC/Clang library builds also pass for these two
branches, with both symbols present in each actual archive. C++17 header
compilation and metadata consistency checks pass. Unchanged CLI source and
resource-format dependencies reuse the preceding paired CLI check. No full
regression or runtime performance replay was repeated for unused fragments.

## Remaining input-byte and DMA-call fragments

`Lufia2FieldTakeLegacyHighButtons` reconstructs `$8E:BBA1..BBA7`.
It shares the width-aware held-button mask and pending-press consumption body
with the existing `$8E:BBA8` low-byte entry. The original M width, DP wrapping,
read-modify-write order and conditional write are retained. No byte-only
assumption is imposed on either API. The existing entry guard is unchanged.

`Lufia2BattleConfigureLegacyLayerColorDma` reconstructs the original
`$85:ABDF..ABE3` long call/return wrapper. It calls the existing `$85:AB98`
semantic body through the original pushed JSL frame and consumes its RTL.
The DMA algorithm is shared, not copied; the wrapper is not a new algorithm.
No incoming caller is established for either unreferenced boundary.

MSVC and Clang each pass 155,648 original-ROM comparisons and 18 no-access
guards. Both joypad-byte entries have 73,728 systematic states, including
byte/word accumulator widths, index widths, DP wrapping and varied input
masks. Canonical DMA and its wrapper have 4,096 states each. CPU, complete
WRAM, ordered data/MMIO accesses and stack writes agree. Four altered-ROM
controls per compiler detect changes. No metadata status or binding is added.

The button body expands locally in standard C11 to avoid adding a helper call
in the existing entry. A plain helper and an inline-hinted version introduced
an extra call in MSVC; those compositions were rejected. Clang's final
canonical button code and both compilers' canonical DMA code match the old
function bytes and relocation identities. MSVC uses a different button inline
layout. Seven alternating native-entry cost pairs have matching final CPU/WRAM
and median candidate/reference ratio 0.976. Millisecond timing and variation
prevent a reliable speedup claim. This checks direct native calls, not runtime
admission, organic frequency, APU/PPU cost or game FPS. No hooks, bindings,
wait batches, clocks or consumer dispatch were changed.

Final strict standalone GCC/Clang library builds pass with both new symbols
in each actual archive. Metadata and C++17 header checks pass. Unchanged CLI
resource-format sources reuse the earlier paired CLI result. New fragment
entry contracts remain separate from canonical-function verification status.

## Bounded Windows consumer replay

The current Windows archive links with the existing production host/generated
objects. The link map resolves both canonical button and layer/color DMA
symbols from that archive. Three paired field replays cover 1,149 frames per
version across the accepted puzzle, shrine and castle warm states. Full saved
state, extended APU clocks, nonempty PCM, native APU idle counts, final picture
and recorded raw/continuation counters agree. Only a live-validated eight-byte
host RAM pointer is normalized. Original warm inputs remain unchanged.

These replays check the linked composition in those scenes. They do not prove
that the unused new fragment entries are admitted, measure organic button/DMA
call frequency, or establish a game-wide performance result. Semantic boundary
proofs and runtime replay evidence remain distinct. No hooks or source-selection
rules changed, and the owner's Windows build directory was not used.


## Resource-bank provenance review

The next search examined 2,359,296 bytes in the 72 ROM banks outside the
current canonical code banks. It found three byte patterns resembling long
calls to known functions and two status/data-bank prologue patterns. These
patterns are hypotheses, not function entries or execution evidence.

The existing resource decoder places four hits inside encoded streams:
`AB:B544` belongs to resource 104, `B5:A8B9` to resource 286,
`A9:A545` to resource 71 and `B7:89CE` to resource 313. The actual standalone
resource CLI successfully decodes all 680 resources while measuring their
consumed stream lengths. No decoded data or ROM bytes were exported. Raw
compressed tokens do not establish native instruction boundaries; this does
not prove that every possible decoded payload is non-executable.

The remaining pattern at `87:B22F` lies between the original map-2 and map-3
record bases, `87:AF89` and `87:B3F2`. These pointers come from the eight-byte
scene records at `87:8010`, as read by `Lufia2FieldSelectSceneRecordBase`.
That is a data association, not a complete per-byte script classification or
an original native caller. No new C function or binding follows from it.

Two older tail hypotheses remain unresolved. `81:FD04..FFFF` contains no
literal RTS, RTL or RTI byte, so the preceding call-shaped prefix does not
establish an ordinary returning function. Dynamic execution is not excluded
by that observation. The `8E:E7C0` tail includes known script data beginning
at `8E:EA00`, referenced by the existing text engine. It must not be described
as one padding block or one missing native function. The seven scoped fragment
contracts above remain unchanged; no additional verification status is claimed.

## Unrooted record direction family

Four original entry variants at `85:FCD2`, `85:FCEA`, `85:FD00` and `85:FD16`
share the direction body at `85:FD2A..FE23`. `Lufia2LegacyResolveRecordDirection`
reconstructs record-to-record, point-to-record, record-to-point and the selected
mode variant. This is one additional shared algorithm, not four duplicated
algorithms. Original incoming callers and gameplay purpose remain unresolved;
no canonical metadata entries or runtime bindings are added.

The scoped contract is PB85, DB7F, M1X0, DP zero, binary arithmetic and stack
`1F04..1FF8`. Four entry PCs are accepted. The returned boundary is the original
RTS at `85:FE23`, before executing that return. Saved X/Y, stack writes, the
accumulator high byte, scratch deltas, index output and table reads are retained.
The original direction compares the subtraction's N flag, not a corrected
signed or magnitude comparison. Its destructive absolute-value scratch writes
and mode-specific tie behavior remain literal. This layer supplies no timing.

MSVC and Clang each pass 386,048 original-ROM comparisons: all 65,536 delta
pairs with rotating modes for each entry, plus all 256 mode bytes against an
11-by-11 matrix of extreme deltas. Full CPU, WRAM and ordered data accesses
agree, including saved-register accesses and varied status/high accumulator.
Each compiler also passes 36 no-access guards; two altered-ROM controls detect
branch/output-operand changes. Those tests validate the stated entry contract,
not organic execution, native admission or game-wide performance.

Strict current Windows, GCC and Clang standalone library builds pass; both
Unix archives export the new scoped API once. C++17 header and metadata checks
pass. The CLI binaries remain byte-identical, so the earlier resource CLI
checks are reused. No consumer hooks, admission, clocks or batching changed.
Unrelated full regression was deferred at this shared checkpoint.

## Unrooted record math and probes

Seven neighboring original regions now have scoped C in
`src/system/legacy_record_math.c`. Existing callers and gameplay use remain
unresolved. The entry contract is PB85, DB7F, M1X0, DP zero and binary
arithmetic. Routines that push registers or call a child additionally require
stack `1F04..1FF8`. No canonical function metadata or consumer binding is added.

| Entry | Original return boundary | Operation |
| --- | --- | --- |
| `85:FBE3` | `85:FC3F` | Prepare multiplier operands, call the original child, constrain the returned delta |
| `85:FC40` | `85:FC60` | Constrain a record-derived delta using the original status branch |
| `85:FC61` | `85:FC83` | Initialize or accumulate the record-derived scratch value |
| `85:FC84` | `85:FC97` | Load that scratch value through the shared record reader |
| `85:FC98` | `85:FCD1` | Test record separation, retaining the repeated component read |
| `85:FE28` | `85:FE7E` | Prepare the original child, then conditionally collect nearby-record data |
| `85:FE7F` | `85:FEB0` | Write four record-component differences |

The boundaries identify the original RTS before it executes. The value-reader
pair shares one implementation of the word load, four shifts and indexed table
read. The separation probe deliberately tests `5BEE` twice after its `5BED`
probe; ordered reads, scratch INC and N-flag comparisons remain unchanged.
The nearby probe retains the original carry propagation between its two ADCs.

The two parent bodies retain exact JSL frames at `85:FC1D` and `85:FE35`.
Their child targets are `80:E32B` and `82:E2BD`; neither child is reconstructed
by this batch. In particular, `80:E32B` overlaps an operand in the current
original program and must not be substituted with an assumed divide service.
The callback can hand off or unwind at the real child boundary. A compatible
normal return runs the original parent suffix; incompatible bank/width state
hands off at its exact continuation. These are explicit service contracts,
not proof that the old calls return normally in the released game.

The multiplier-settling NOP is exposed by consecutive `85:FC0E`/`85:FC0F`
checkpoints. Its CPU states and data-access positions match the original
before and after that instruction. This portable layer supplies no cycle
scheduler: future integration must preserve the NOP interval and device timing.
The synchronous synthetic multiplier bus proves operand/access semantics,
not hardware settling time or a native runtime timing contract.

MSVC and Clang each pass 352,256 CPU/full-WRAM/ordered-access cases, including
multiplier state, saved registers, child-entry state and both NOP checkpoints.
Each of the five leaves has 65,536 cases. Parent prefixes and compatible
return/unwind services cover 8,192 and 16,384 states. The original unknown
children are not replaced by a fabricated semantic implementation: controlled
services are explicitly distinguished from original execution. Fifty-seven
no-access guards and seven altered-ROM controls pass per compiler.

A negative-control seed initially missed its mutated branch. Another mutated
child target escaped the intended stop. Corrected control selection and an
unexpected-child boundary check detect both changes. The affected accumulator
and two parent suites were rerun together (90,112 cases/compiler); unchanged
leaf comparisons were reused. Production source did not change during that fix.

Strict Windows/GCC/Clang library builds, actual Unix archive exports, C++17
header and metadata checks pass. Both resource CLI binaries remain byte-identical,
so their previous checks are reused. No hooks, waits, source selection or clocks
changed in the consumer. Full unrelated regression is deferred. No runtime
admission, organic frequency, performance improvement or game-wide completion
is claimed for these unrooted routines.

## Unrooted selection controller

`Lufia2LegacyRunSelection` reconstructs `85:FAB6..FBC0` as selection setup,
record/action branches and an interrupt-counter wait. It accepts four scoped
PCs: the prepared entry hypotheses `85:FAB6`/`85:FAC8`, the common phase at
`85:FB2A`, and the wait continuation at `85:FBB6`. Entry requires PB85, DB7F,
M1X0, DP zero, binary arithmetic and stack `1F04..1FF8`. The original incoming
caller remains unproved. No canonical function metadata or binding is added.

The implementation shares the common refresh, action and finish phases.
Original TSB behavior, byte/word stores, the long-address shared byte, TDC's
full accumulator result, record selection and all ten child frames remain
explicit. The JSR at `85:FB9C` targets `85:81CF`; it does not dispatch through
the neighboring `85:FBC1` table. That table has eleven nominal words, but its
selector bounds and caller are not established. No handler adapters or guessed
table dispatch were introduced. Data-associated child targets are not repaired
into more plausible functions, and the original `82:D037` BRK target remains.

Unknown children are explicit callback boundaries. Nonlocal exits preserve
the real child-input frame and CPU state. Compatible returns run the original
suffix; changed bank or M/X state hands off at the exact return continuation.
Controlled test services verify these parent contracts and do not claim to
reconstruct the original child routines or establish organic execution.

After incrementing DP `5E`, the original controller repeatedly loads it until
zero. The C keeps those reads and exposes the load/branch checkpoints at
`85:FBB6` and `85:FBB8`. After 4,096 unsuccessful reads it hands off at
`85:FBB6` without finishing the operation. Resuming repeats the original wait;
it does not increment the counter again. No wait is removed or fast-forwarded.
These are portable semantics, not guest clock or NMI scheduler integration.

MSVC and Clang each pass 40,517 CPU/full-WRAM/ordered-access cases: 32,768
entry states, 1,920 unwind scenarios, 5,760 changed-context scenarios
and 69 wait-boundary cases. The scenarios force 1,348 actual unwinds and
4,044 actual incompatible returns; others verify an unreached child is untouched. Every original child site has executed, unwound
and rejected an incompatible return context. Original JSR/JSL frame inputs
and actual RTS/RTL returns are compared independently. Wait checkpoints compare
CPU state and data-access position, including explicit parent-counter wrap,
selected release edges from read 1 to 4,097, exhaustion at 4,096 and a second
continuation call. Three added targeted wrap cases leave the existing main
proof unchanged; no full repeat was needed. Forty-three
no-access guards and four altered-ROM controls pass per compiler.

Strict Windows/GCC/Clang library builds, actual Unix archive exports, C++17
header and metadata checks pass. Resource CLI binaries remain byte-identical;
the earlier CLI checks are reused. No consumer hooks, clocks, admission or
batching rules changed. Unrelated full regression is deferred. This adds one
shared control-flow body, not four canonical functions or a runtime speed claim.

## Code-origin and prologue review

A follow-up examines 370 call-shaped origins to owned but noncanonical targets.
These are discovery hypotheses,not370 missing functions. 188 origins associate
with original scene records, 18 have nearby C literals, and 164 need more context.
Source/data association does not by itself exclude deliberately overlapping code.

Six stack/status prefixes produce 22 hypotheses after reconstructed regions
are excluded. 16 correspond to existing C bodies/phases, four have data evidence,
and two at 8E:AAA5/AD0F remain untyped. No new game algorithm is established.

| Apparent prefix | Original association |
| --- | --- |
|80:8A98|Last byte of the 12-word menu-string table 80:8A81..8A98, before handler 8A99|
|86:D3F8|Reciprocal-table word at 86:D3F7, used by world perspective rows|
|8E:E02F|Menu sprite frame 8E:E028..E032, with two OAM pieces|
|8E:EAD1|Dictionary pointer table 8E:EA00..EEFF|

The actual native sprite-table,animation and OAM routines read 8E:E02F for
nominal list 4, animations 6 and 13. Both isolated reader cases pass. A structural
walk follows 9 nominal list roots, 71 animation lists and 107 unique normal frames.
It exports no graphic payload and clamps no original selector. This is data-reader
exploration, not fresh interp816, organic execution, timing or admission proof.
The AA-rich untyped prefix regions are not declared padding or proven noncode.

Five missing catalogue records now identify existing battle-script sections;
the source path for 85:C05F is restored. No API, canonical metadata, binding or
algorithm is added. Original 85:C3AF branches to 85:C332, entering the party loop.
BattleBuildSelectionRows intentionally retains this with `enemy_first = false`.
Changing that statement would change the original program.

Semantic and consumer source bytes remain unchanged. The small reader probe
uses the current Windows library with warnings as errors. Prior proofs are
reused only for unchanged inputs;no full suite, CLI or platform rebuild repeats.
This bounded review does not establish whole-ROM code/data completeness.
