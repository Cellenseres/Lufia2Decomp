# Native shop purchase and sale preparation — 2026-10-20

Seven complete original functions now prepare item purchases (82:D7BC),
item sales (D970), equipped-item counts and adjusted quotes (DA13), equipment
preview windows (D8A5), spell purchases (D8D4), and original spell/item list
selection (E6FC/E709). Existing D905 spell-shop setup, DA5C list-window
selection and F703 equipment-slot selection are reused rather than duplicated.
The byte quantity subtraction retains both original SEC operations. A word
stock-limit write deliberately crosses154B/154C into purchase mode. Selection
from the byte spell list still performs the original word read. Equipped-item
counting rereads the member count, selected item and live indirect pointer.

One shared checkpoint passes7,205 direct ROM states/36,530 child calls and
3,621 production-ABI states/18,354 calls, all37 encoded sites and forced
unwinds,630 redirects,5,376 ABI guards and zero positive fallback. A private
ABI fixture initially retained the previous batch's M-width guard indices;
correcting that fixture makes the complete ABI run pass without source edits.
492 additional boundaries/65,812 calls cover long-pointer bank wrap, pointer
and stack aliases, raw list indices, original JSR/RTS sites, and zero/255
party counts. The zero-count case preserves all65,536 original iterations.
320 compositions execute1,215 actual nested native children among3,347 calls,
including the new selection/counting functions and existing item records,
price adjustment, equipment selection, tile clearing and spell-shop setup.
917,537 no-access guards and seven finite ROM mutations are detected. The
item-list mutation uses the high mask byte because the initial fixture's
values all had bit0 clear; unchanged first-six mutation results are reused.

CPU, full WRAM, MMIO and ordered data agree. Own-body traces omit bank82
ROM instruction reads. Compositions additionally omit81:F194..F1AA and
F1C5..F2E6 code while retaining actual bank96 record and bank9E name data;
the initial name-instruction trace noise was corrected only in the driver.
Unknown slot-type scan, input, text and rendering children remain controlled
peers. No live whole-shop, timing or speedup claim. Full legacy regression
and unchanged startup replay are deferred under the owner's batch cadence.
Proofs remain private; both planned commits share the final-source checkpoint.

Strict MSVC, standalone GCC/Clang and isolated Windows Release pass at this
shared checkpoint. Unchanged CLI binaries and private fixture retain their
prior accepted CLI contract by hash. All seven production bindings are generated.
Owner work and build/windows remain untouched.

# Native shop exchange and equipment purchases — 2026-10-20

Ten complete original routines cover secondary-balance credit, spending and
comparison (82:9867/989D/98B5), exchange purchases and unit lookup
(E0A9/E0E2), equipment preview and purchase (DFC4/E0F8), item receipts
(E140), equipment replacement and swapping (F97D/F9B6). The currency's
identity remains unclaimed. Original 24-bit wrap before the credit cap,
raw table indices, incoming multiply carry, byte-index truncation, live
indirect-pointer rereads and word quantity reads remain literal.

A shared checkpoint passes17,952 direct original-ROM executions and5,152
production-ABI invocations with zero positive fallback,555 redirects and
7,680 ABI guards. All32 encoded child sites and forced unwinds pass.
The ABI fixture separately varies possession, capacity and inventory count;
its initial30-site coverage is not claimed as a complete32-site result.
Another1,432 edge states cover currency overflow/cap boundaries, raw unit
indices, bank crossing, pointer/stack aliases and the real F952 JSR/RTS.
192 compositions execute581 actual native children among1,261 calls,
including the preceding shop-purchase parent and existing inventory, gold,
record, multiply and tile helpers.1,310,765 no-access guards and ten finite
ROM faults pass. The composed driver recognizes the parent's E0A8 return;
no semantic-source correction was needed after the first direct proof.

CPU, full WRAM, MMIO and ordered data agree. Own-body trace filtering
excludes bank82 instructions while retaining the E0F0..E0F7 unit table;
compositions also omit only81:F194..F1AA/F291..F2A8 instruction reads,
retaining actual bank96 records. Unknown setup/input/text/graphics children
remain explicit controlled peers. No live whole-shop, timing or speedup
claim. Full legacy regression and unchanged startup replay are deferred
under the owner's batching cadence. Proofs remain private. Both planned
commits share the combined final-source checkpoint.

Strict MSVC, standalone GCC/Clang and isolated Windows Release pass at this
shared checkpoint. Unchanged CLI binaries and private fixture retain their
prior accepted CLI contract by hash. All ten production bindings are generated.
Owner work and build/windows remain untouched.

# Native shop transactions, quantities and spell learning — 2026-10-20

Nine complete original routines now cover selling (82:DEA4), purchase
quantity selection (DF45), spell learning and insertion (E167/E1DE), quantity
price calculation and adjustment (E20C/E22C), messages and completion
(DE53/DE75), and item purchase (E02C). Their original child-call boundaries,
signed byte-limit comparisons, live message-byte rereads, uncleared multiply
carry, adjacent-byte quantity word reads, and preview PHP/PLP are retained.
Spell insertion scans exactly36 slots and keeps original indirect bank carry
and stack-alias behavior. No new instruction-PC adapters are introduced.

One shared checkpoint covers17,988 direct ROM executions: the initial16,187
states cover59 sites; only the purchase fixture is extended for1,801 states
after separating two flags to reach the remaining rectangle-clear site.
Production ABI passes4,668 invocations without positive fallback,1,035
redirects and6,912 guards; all60 encoded sites and forced unwinds pass.
Another1,234 edge states cover signed quantity limits, full spell lists,
indirect bank crossing and stack aliasing. Only the114 spell-edge cases are
rerun after clearing setup-only MMIO traces; the1,120 quantity cases remain
unchanged.288 compositions execute464 real native children among1,763 calls,
including multiply, packed counts, inventory quantities, gold, item records
and tile clearing.1,179,693 no-access guards and nine finite ROM faults pass.

CPU, full WRAM, MMIO and ordered data agree. Own-body traces exclude bank82
ROM reads and separate ABI bookkeeping. Compositions also exclude only the
item-record instruction ranges81:F194..F1AA and81:F291..F2A8 while retaining
the actual bank96 record tables. Nested original returns are accepted only
below the current owner's entry stack; premature owner returns still fail.
Unknown input/setup/transaction-dispatch/text/graphics children remain
controlled peers. No full live-shop, cycle-timing or speedup claim.
Full legacy regression and unchanged startup replay are deferred under the
owner's batch cadence. Tests remain private; both planned commits share
the combined final-source checkpoint.

Strict MSVC, standalone GCC/Clang and isolated Windows Release pass at this
shared checkpoint. The CLI executable and private fixture hashes are unchanged;
their prior accepted behavior is reused. All nine native production bindings
appear in generated dispatch. Owner work and build/windows are preserved.

# Native shop interaction and list construction — 2026-10-20

Six complete original routines now cover the shop interaction loop (82:DA8F),
item selection (DB15), list-mode selection (E012), list-panel and visible-row
drawing callers (DC69/DC8A), and selected-list construction (E327). DB15
includes its remote action branches through DC46. Inline dispatch tables
remain explicit child boundaries; their data are not decoded as instructions.
E327 retains the word flag scan, carry rotation, byte/word transitions,
96-entry reserve mode, source-pointer bank and transformed-list writes.
Zero flag words retain the original nonreturning scan; no safety clamp is
introduced into the semantic implementation.

The shared checkpoint covers 9,219 direct ROM states: the unchanged first
five functions reuse their accepted 8,960-state portion; the affected list
function passes 259 new states after correcting its reserve geometry. A
synthetic owned-list seed was corrected to fixed bank7E rather than DB; the
unterminated fixture run was stopped and is not acceptance evidence.
Actual production ABI passes 2,672 invocations without positive fallback,
795 redirects and 4,608 guards. All 48 encoded child sites and their forced
unwinds pass. Another 96 raw flag/row cases add 9,763 calls. Forty-eight
compositions add 6,312 calls, including 1,344 real native children across
the shop session, selection, list, panel and existing tile-clear code.
786,461 no-access guards and six finite original-ROM mutations pass.

CPU, full WRAM, MMIO and ordered operands match. Bank82 ROM reads and separate
ABI bookkeeping reads are excluded; long source-list data remain compared.
Unknown transaction, input, text and graphics children are controlled peers;
no whole-shop live-scene, cycle-timing or speedup claim. Tests stay private.
Full legacy regression and unchanged startup replay are deferred under the
owner's batch cadence. Both planned commits share this final-source checkpoint.

Strict MSVC, standalone GCC/Clang and isolated Windows Release pass at the
shared checkpoint. CLI executable and private fixture hashes are unchanged;
their accepted CLI behavior is reused. All six production bindings appear in
generated dispatch. Unrelated owner work and build/windows are preserved.

# Shop session and Capsule menu assets - 2026-10-20

Six previously missing original functions are verified:82:CF72 loads the
form palette;D248 places the selected sprite;D69C owns shop entry/cleanup;
D702 initializes selection;DA5C selects the list window;DAD8 refreshes it.
The shop root preserves both its RTL and terminal JML84:A689. Its DA8F
interaction loop remains an explicit original child, not a completed native
shop engine. Palette copying preserves stack aliasing and restored DB;
sprite placement retains the selected-ID word read and byte coordinates.

One related checkpoint passes10,779 ROM body/boundary states and45,188
child calls, covering all27 original sites and forced unwinds.512 caller
compositions add2,816 calls and512 actual native children. Production ABI
passes3,099 positive invocations with zero fallback,465 redirect probes
and4,608 guards.786,460 no-access guards and six finite ROM mutations pass.
CPU,full WRAM,MMIO and ordered data accesses match. Bank82 ROM reads and
separate ABI bookkeeping reads are excluded;palette and coordinate ROM
data remain compared. The ABI oracle distinguishes the pre-dispatch tail
boundary from the post-JML bank; target,stack and dispatch state are checked.
Unknown child internals are controlled seams. No live shop-scene,cycle-timing
or host-speedup claim. Tests and ROM remain private.

Eleven catalogue labels already belonged to verified menu-string or Capsule
screen owners. Their internal ownership is documented without duplicate C,
new standalone proofs or extra function counts. Full legacy regression and
unchanged startup replay are deferred under the owner's batched cadence;
the new/affected ROM and actual ABI union is checked at this checkpoint.

Strict MSVC, standalone GCC/Clang and isolated Windows Release pass at
this combined checkpoint. CLI executables and private fixture hashes are
unchanged; their prior CLI acceptance is reused. The six bindings are present
in generated production dispatch. Owner builds and unrelated WIP are preserved.
Both commits share this final-source checkpoint, not independent test runs.

# Capsule form-effect lifecycle and placement - 2026-10-20

Five complete original routines now run the form-effect owner, step angle and
lifetime, place each effect and apply the signed scaled product. D68C retains
M0X0; the other four retain M1X0. The original22-frame spawn cadence, four
slots, age-word240 comparison, byte radius, exact180-step angle reset and
unaligned product word1575 remain literal. The sign byte is rotated before
a word shift at1583; the adjacent byte participates. Existing sine, cosine
and multiply bodies/bridges are reused without new copies or changes.

One combined checkpoint retains16,384 unchanged non-owner ROM states and
their seven child unwinds. The owner is checked separately in134 states with
149,078 calls and all six own sites/unwinds after increasing only its synthetic
peer lifetime beyond the original spawn interval. The first shorter fixture
covered only12/13 sites; no complete proof is claimed for that initial run.
The resulting union covers16,525 body/boundary states and all13 sites.
Another264 composed states execute82,204 actual native children among84,488
calls, including real sine/cosine/multiply and food-meter code. Two complete
form-change callers execute27,204 real children among28,916 calls. Seven
real-lifecycle/late-unwind states add73,919 calls and71,578 native children.
A complete selected-ID0/meter32 lifecycle has331 waits,1,056 angle/lifetime/
placement updates and66 meter checks.320 boundary states add16 calls.

Actual production ABI passes2,077 invocations with zero positive fallback,
240 redirects and3,840 guards.655,383 no-access guards and five finite ROM
faults pass. CPU/full WRAM/MMIO and ordered operands match. Bank82 ROM reads
are excluded except coefficient dataC9D0..CA17; composed checks also exclude
trig instruction bytes808450..84BE while keeping the actual84BF table reads.
ABI bookkeeping reads are excluded separately. Unknown animation and frame
wait children remain controlled services; no hardware scene or timing proof
is claimed. Tests and ROM data remain private.

Strict MSVC, standalone GCC/Clang and isolated Windows Release pass at the
shared integration checkpoint. Unchanged CLI executables and private fixture
reuse their accepted contract. All3,000 normal-input CPU/WRAM,PPU/picture,
APU/SPC,DSP and PCM rows equal the preceding accepted revision. The five new
functions have zero hits in that replay; the separate original-ROM lifecycle
and composition proofs cover their semantics. No live VFX or timing coverage
is implied. Counts are1,862 verified,zero drafts,1,829 bindings and795 WRAM
fields. Full regression is deferred. Both atomic commits share this combined
final state; no independent intermediate checkpoint is claimed. Tests remain
private. Parallel renderer changes are outside this batch's ownership.

# Capsule graphics, food display and palette - 2026-10-20

Thirteen complete original routines now load/arrange form graphics, publish
both original DMA requests, draw the food meter, rotate/advance/fade its
palette and begin/end/load the form-effect presentation. D026/D397 retain
M0X0 contracts; the others retain M1X0. Palette components still increment
on inequality even above their target. Meter values remain unclamped; saved
words, byte wraps, index-width changes, MVN order and DB-relative I/O remain
literal. The eight effect records atA920 follow the3072-byte mask sequence.

One checkpoint passes13,840 body states with10,036 child calls, all16 encoded
sites and forced unwinds. The768 composed states execute194 actual new
children among2,562 calls. Eight complete affected feeding/form-scene/tile
callers execute23,602 real nested native children among25,794 calls, including
the64-frame owner. Two late-slot unwinds add15 calls. Actual production ABI
passes3,472 invocations with zero positive fallback,330 redirect probes and
9,984 guards. Another1,703,994 semantic no-access guards and seven finite
ROM faults pass. The DMA fault is caught by ordered MMIO comparison.
CPU/full WRAM/MMIO and ordered operands match. The observer excludes
bank82 ROM reads except C9D0..CA17 coefficient data; ABI bookkeeping reads
are excluded separately. Unknown decompression, wait and
other child bodies remain explicit services. No complete hardware scene or
cycle-timing proof is claimed. Tests and ROM data stay private.

The shared integration checkpoint passes strict MSVC, standalone GCC/Clang
and isolated Windows Release. The unchanged CLI executables and private
fixture reuse their accepted contract. All3,000 normal-input CPU/WRAM,
PPU/picture,APU/SPC,DSP and PCM rows equal the preceding accepted revision.
These thirteen routines have zero hits in that replay; their actual semantic
coverage is the separate original-ROM, child and production-ABI checkpoint.
Counts are1,857 verified,zero drafts,1,824 bindings and794 WRAM fields. Full
regression is deferred. Both commits share this combined final source state;
no separately tested intermediate revision is implied. Tests remain private.

# Capsule form-transition masks - 2026-10-20

Four complete original routines now generate cumulative form-transition masks,
choose an unused bit with the original random retries, merge each frame and
apply its mask to both tile planes. CA18 enters/exits M0X0; CA62, CAB1 and
CB00 enter/exit M1X0. Short indirect tile accesses retain caller DB, while
mask-generation pointers retain their original long banks. The 48-byte mask
at9CF0 and3072-byte sequence at9D20 reuse snapshot/save-preview storage in this
scene; existing WRAM ranges remain unchanged. Bit masks remain ROM-backed.

The combined checkpoint passes 3,140 body states with39,951 child calls and
all four original encoded sites/forced unwinds. Eight full64-frame composed
states execute3,584 actual new native children among9,744 calls. Four late
unwind states add926 calls, including the384th column and64th merge plus
random collision retries. Another788 production-ABI calls have zero positive
fallbacks;90 redirects,3,072 ABI guards,524,306 no-access guards and four
finite ROM faults pass. CPU/full WRAM/MMIO and ordered operands agree.
Bank82 instruction fetches and ABI bookkeeping reads are excluded. Random
generation remains an explicit original-child service; no whole-animation
timing proof is claimed. Tests and ROM data stay private. Integration results
are recorded separately; the four bodies share one final checkpoint.

The affected complete64-frame C958 caller also passes four original-ROM
states with10,312 child calls, including9,216 real native CA18 transforms.
The shared integration checkpoint passes strict MSVC, standalone GCC/Clang
and isolated Windows Release. CLI binaries/fixture are unchanged; their prior
contract is reused. All3,000 normal-input CPU/WRAM,PPU/picture,APU/SPC,DSP and
PCM frame rows equal the preceding accepted state. These four routines have
zero hits in that replay, so no live form-animation coverage is claimed.
Counts are1,844 verified,zero drafts,1,811 bindings and793 WRAM fields. Full
regression is deferred; both commits share this combined final checkpoint.

# Capsule feeding callers and form animation - 2026-10-20

Seventeen complete original routines now handle feeding, food-meter lookup,
form selection, panel/tile transfers and food particles. They are original
function bodies, not per-instruction adapters. Explicit child services keep
unknown redraw, wait and tile-transform internals outside these contracts.
The original 82:CA18 transform kernel remains a separate missing function.
ROM tables stay bus-backed. Eight-bit particle-count overflow, exact meter
comparison, live DP counters, saved frame counts and MVN order are preserved.

One shared checkpoint passes 11,869 own-body states with 119,715 child calls,
all 89 original encoded child sites and their forced unwinds. Seven late-loop
unwinds add 8,560 calls, including the 256-iteration wrapped particle count
and the final tile/frame iterations. Another 85 composed states execute 965
real new native children among 18,965 child calls. CPU, full WRAM, MMIO and
ordered data accesses agree. Bank82 instruction reads are excluded, while
actual coefficient data at C9D0..CA17 is included. Child services are controlled
seams; no whole-animation timing or live-scene coverage is claimed.

Current production wrappers also pass the reduced ROM matrix without positive
fallbacks, 1,500 redirect probes, 13,056 actual ABI guard states and 2,228,303
semantic no-access guards. All 89 sites receive fifteen targeted redirect
probes. ABI dispatch/return bookkeeping reads are separately excluded. Seven
finite in-memory ROM faults are detected. No test or ROM data is committed.
The related compiler, consumer and normal-input replay checkpoint is recorded
separately; no full-regression or intermediate-commit result is implied.

The shared integration checkpoint passes strict MSVC, standalone GCC/Clang
and the isolated Windows Release build. The CLI binaries and private fixture
are unchanged, so their accepted contract is reused. All 3,000 normal-input
frame rows equal the prior accepted state for CPU/WRAM, PPU/picture, APU/SPC,
DSP and PCM. These seventeen functions have zero hits in that replay; no live
feeding or animation-timing coverage is claimed. Full regression is deferred.
There are 1,840 verified entries, zero drafts, 1,807 selected bindings and 793
WRAM fields. Both commits share this combined checkpoint; tests stay private.

# Capsule food and special-form operations - 2026-10-20

Four complete original routines now classify special food, draw its message,
select food values and apply or reject special-form changes. The food tables
remain ROM-backed; selected-ID word reads, live DP scratch, original form-bit
updates, physical child frames and return flags are preserved. The complete
feeding caller and its animation/redraw children remain separate work.

One related checkpoint passes 275,986 original-body comparisons, 9,938
finite-child composition cases and 34,450 production-ABI invocations with no
positive fallbacks. All eighteen child sites are reached and forced to unwind;
270 targeted redirect probes plus 45 initial probes preserve host-return modes
and nonlocal returns. There are 524,307 semantic no-access guards, 3,072 actual
bridge guards and four detected original-ROM mutations. CPU state, full WRAM,
MMIO and ordered data accesses agree. The bus observer excludes bank82 ROM
reads; ABI bookkeeping reads are separately excluded. Unimplemented children
remain explicit services; no complete animation or cycle-timing proof is claimed.

The semantic ROM proof is reused unchanged for the integration checkpoint.
Full regression is deferred; tests stay private. Strict MSVC, GCC, Clang and
the isolated consumer Release build pass. All 3,000 normal-input frame rows
match the prior accepted state for CPU/WRAM, picture and audio. These four
routines have zero hits in that replay; their semantic and ABI evidence is
the separate ROM checkpoint. Both repositories share the combined checkpoint.

# Semantic enemy ordering and target remapping - 2026-10-20

The existing complete enemy-reordering owner now builds column order and
remaps enemy target bits through ordinary C operations. Column sorting keeps
the original swaps for equal keys, repeated word reads, byte-ordered stores
and live pass counter. Target remapping keeps six separate scratch-byte
read/shift/write operations and reads each selected slot mapping in order.
The CPU shell retains accumulator high-byte residue, flags, widths, original
JSR frames and return boundaries. Scratch and target tables remain bus-backed;
no host snapshot replaces the original arrays. No public API, instruction
adapter, canonical entry or binding is added.

One combined checkpoint passes 16,384 original-ROM sort/remap region cases,
4,096 complete original-ROM owner cases and 4,096 current production ABI
cases with zero fallbacks and 4,096 real RTL returns. The owner matrix covers
all 720 enemy orders, all 64 six-slot occupancy masks and all 256 observed
target-byte inputs. Separate region cases include duplicate/missing columns,
all data banks, flags and arbitrary target-map bytes. CPU state, complete
WRAM and ordered data-bus streams agree; ABI bookkeeping reads are excluded.
Three in-memory ROM mutations detect the equality branch, mapping source and
six-slot loop bound. Strict MSVC, isolated Windows Release and incremental
GCC/Clang builds pass. Unchanged CLI/fixture hashes reuse prior CLI evidence.

All 3,000 production CPU/WRAM, PPU/picture, APU/SPC, DSP and PCM rows match
the preceding accepted run. Both new cores have zero hits in that startup/
menu scenario; decoder count stays 9,040. No runtime speedup is claimed.
Full regression is deferred; related ROM/ABI/build/runtime checks are green.
Tests remain private. Counts stay 1,819 verified, zero drafts, 1,786 bindings,
791 WRAM fields and 500 source units.

# Semantic sound-resource queue - 2026-10-20

The complete sound queue owner now selects its next occupied resource slot
through ordinary C, returning a slot and resource identifier. Transfer source
publication is a separate operation with explicit source/length inputs and
an original-order bank read after both word stores. Both use catalogued WRAM
names. The CPU shell retains original flags, physical stack writes, child
calls and unwinds. Unexpected child-return widths retain the old publication
path. No instruction-per-PC adapter or additional public API is introduced.

The combined checkpoint passes 16,384 original-ROM region cases, 8,192 exact
accepted-parent streams, 1,172 complete original-ROM parent executions with
explicit controlled peer seams, and 9,364 current production ABI calls with
zero positive fallbacks. Coverage includes all resource byte values in each
slot, empty queues, source-bank aliases, physical stack aliases, saved widths,
child-return width changes and early child unwinds. Ordered data accesses,
CPU state and full WRAM agree. Three in-memory ROM faults are detected.
ABI comparisons exclude dispatch/return bookkeeping reads; the separate ROM
and accepted-parent checks compare the complete ordered data streams.

Strict MSVC, isolated Windows Release and incremental GCC/Clang builds pass.
Unchanged CLI executable/fixture hashes reuse prior CLI evidence. All 3,000
production frame rows match the preceding accepted run for CPU/WRAM,
PPU/picture, APU/SPC, DSP and PCM. These queue cores have zero calls in that
scenario; decoder count remains 9,040. No speedup or new coverage is claimed.
No scheduler/timing adapter changes. Full regression is deferred. Tests remain
private. Counts remain 1,819 verified, zero drafts, 1,786 bindings and 791
WRAM fields. No new source unit or binding is added.

# Semantic sound-transfer progress - 2026-10-20

Header source/bank/length progression and payload sequence/length progression
now use ordinary C calculations shared with their complete upload owners.
Header advancement preserves 16-bit source wrap, the separate bank-byte RMW
and the original 8000 reload. Payload advancement retains two separate byte
RMWs and the original continuation test against the incoming length: zero or
one wraps the stored remainder but still stops. The CPU shell exports the
original indexes and flags. Stepped checkpoints and unexpected-width paths
retain their previous behavior.

A finite Recomp header operation runs at 80:98FC before the existing sound
instruction adapter. It preserves original opcode/data order, branch cycles
and wrapped-bank behavior. Admission keeps explicit Stop/Yield, pending events,
armed timer IRQs, unsafe wrap targets and insufficient budgets on their old
paths. Enabled NMI requires a deadline. No SPC-port access or handshake is
batched. No new instruction-per-PC switch is added.

The combined checkpoint passes 205,248 original-ROM API/region cases, including
all 65,536 incoming word lengths at each of three original regions, plus
45,360 exact consumer fetch/data/device/cycle cases. The consumer admits
22,608 and declines 22,752 cases. Another 4,096 rejection cases access no bus.
Three in-memory ROM faults detect source reload, the second sequence RMW and
remaining-length decrement. Complete parent streams match 576 accepted-source
cases; the current production ABI passes another 576 native invocations with
zero positive fallbacks. That fixture uses controlled peer responses and
excludes ABI bookkeeping reads; exact operation bus order is checked against
the ROM. It does not claim SPC hardware timing or new redirect coverage.

All 72 actual SDK Stop/Yield/deadline/refresh/FastROM/timer controls match,
with positive header admission and zero admissions in explicit/timer contexts.
All 3,000 production CPU/WRAM, PPU/picture, APU/SPC, DSP and PCM rows match
both the prior baseline and a kernel with only this header operation disabled.
The scenario replaces 2,304 old sound-adapter calls with 576 complete header
operations: old calls fall from 6,912 to 4,608, a net 1,728 fewer native
adapter dispatches. Decoder count remains 9,040. The header C core executes
576 times; the payload C core has zero hits in this scenario. No wall-clock
speedup or new canonical coverage is claimed.

Strict MSVC, isolated Windows Release, incremental GCC/Clang, C++17 linkage
and seven compatibility gates pass. Unchanged CLI executable/fixture hashes
reuse prior evidence. Full regression is deferred. Tests remain private; ROM
and owner SRAM are unchanged. Two operation APIs are added. Counts remain
1,819 verified, zero drafts, 1,786 ordinary bindings, 791 WRAM fields and
500 C units.

# Semantic field-frame decisions and transition state - 2026-10-20

The existing field loop and frame resume now use three ordinary C operations:
choose the next frame action, exchange current/destination maps and clear
post-transition state. The decision reports wait, transition, map change,
scene change or ordinary update. The CPU shell exports the original value,
Z/N/V behavior and second-controller BIT status. Flag consumption retains
its original read/write order and short-circuit behavior.

Map exchange deliberately preserves the original guest-stack writes and
reloads. An overlapping stack can change the destination or saved map word;
the C operation retains that behavior rather than using a host-only swap.
Transition-state clearing keeps original store order. Unexpected child-return
widths retain the prior width-dependent path. No instruction-PC switch or
runtime adapter is added. Both 83:8000 and 83:8007 were already verified and
bound; historical notes describing an undecompiled/unbound field loop are
obsolete and do not describe this checkpoint.

The shared checkpoint passes 134,912 original-ROM CPU/full-WRAM/exact ordered
data-bus/device cases across all banks, decimal/status/index-width variants,
DP/source aliases, stack wrap and stack/map overlap. All five actions have
coverage. Three independent in-memory ROM faults detect action masks, map
sources and clear destinations. Complete owner/resume verification through
the current production bridge passes 4,096 cases each, with zero fallbacks.
Nineteen controlled child sites retain their frames; the fixtures exercise
233,092 controlled child returns and 32,096 actual ROM wait/NMI events on
each reference/native side. Those services are not newly decompiled here.

An instrumented production replay executes action selection 1,816 times;
map exchange and clear have no hits in this scenario. All 3,000 CPU/WRAM,
PPU/picture, APU/SPC, DSP and PCM rows match the preceding accepted baseline.
Decoder count remains 9,040. This is semantic reconstruction within already
native owners, not new coverage or a measured runtime speedup.

Strict MSVC, isolated Windows Release, incremental GCC/Clang and C++17 public
interface linkage pass. Unchanged CLI executable/fixture hashes reuse prior
evidence. Full regression is deferred. Tests stay private; ROM and owner SRAM
are unchanged. Three operation APIs are added; counts remain 1,819 verified,
zero drafts, 1,786 ordinary bindings, 791 WRAM fields and 500 C units.

# Finite sound source-pair restoration - 2026-10-20

The sound-upload owners now share an ordinary C operation that reads two
source bytes into the original direct-page staging fields. It reads each
byte after the preceding store, preserving source/destination aliases. The
CPU shell exports the final high byte and source index with original flags.
Pairs that cross the source-bank boundary remain with the original caller,
including its JSR frames, bank update and queued-source shadow stores.
Unexpected widths also retain the original path.

Recomp uses this shared operation for four complete finite regions at
80:9953, 997D, 9815 and 9848. Admission requires a nonwrapping ROM/WRAM source,
original widths, no pending event and enough refresh-aware budget. It keeps
explicit Stop/Yield boundaries, armed H/V timer IRQs and unbounded enabled-NMI
contexts on the old path. Source-pair opcode/data order and page/DP cycle
costs are preserved. SPC-port writes and handshakes are not grouped.
No instruction-per-PC switch is added.

The combined checkpoint passes 25,088 original-ROM API/helper cases, including
real bank-helper returns, and 110,592 consumer CPU/full-WRAM/exact fetch/data/
device/cycle cases. The consumer admits 14,248 and declines 96,344 cases.
Another 18,944 rejection cases make no access. Coverage includes all banks,
source/direct-page aliases, page edges, decimal/status flags and wrapped
pairs through the original helper. Two ROM operand faults detect source and
staging-field addresses. The initial cycle check detected an overcharge for
non-page-crossing reads; the corrected shared matrix passes.

Complete upload parents match 576 accepted-baseline CPU/full-WRAM/ordered-bus/
child/checkpoint streams. Another 576 current production-ABI invocations pass
with zero positive fallbacks across three return modes. That ABI fixture uses
a write-driven synthetic APU peer; bookkeeping reads are excluded, with exact
data-bus behavior checked independently against the ROM. It does not claim
new redirect/continuation coverage or SPC hardware timing.

All 288 actual SDK Stop/Yield/deadline/refresh/FastROM/timer controls match.
Each of the four operations has observed SDK admission; timer and explicit
stop contexts have none. All 3,000 production CPU/WRAM, PPU/picture, APU/SPC,
DSP and PCM rows match both the prior baseline and a kernel with only these
operations disabled. This scenario executes the initial payload reader 75
times; other phases have zero hits here. Actual decoder calls fall from
9,640 to 9,040. No measured wall-clock gain is claimed.

Strict MSVC, isolated Windows Release, incremental GCC/Clang, C++17 linkage
and ten compatibility gates pass. Unchanged CLI executable/fixture hashes
reuse previous evidence. Full regression is deferred. Tests stay private;
ROM and owner SRAM remain unchanged. One operation API is added; canonical
entries, bindings and C units are unchanged: 1,819 verified, zero drafts,
1,786 ordinary bindings and 500 C units.

# Finite music sample-cache operations - 2026-10-20

The song loader and Recomp now share ordinary C sample-cache clearing and
searching operations. The canonical owner retains its CPU shell and original
width-dependent path for unexpected child-return modes. The consumer runs
whole finite loops at 80:94B0 and 80:94DE for the original 32 cache slots.
It emits opcode/data accesses in their original order and returns exact path
cycles, registers and flags. It introduces no instruction-per-PC switch.

Admission requires enough refresh-aware deadline budget, no pending events,
and the original widths/value contract. Explicit Stop/Yield boundaries keep
the original path. Armed H/V timer IRQs also retain instruction boundaries;
NMI-enabled contexts require a deadline. Unsupported or tight-budget contexts
continue through the existing native/decoder paths without side effects.

One combined checkpoint passes 32,790 original-ROM API/region/owner-helper
cases and 57,344 consumer CPU/full-WRAM/exact fetch/data/device/cycle cases.
The consumer admits 24,512 and declines 32,832 cases. Coverage includes every
sample byte, all 32 slots, found/missing values, all banks, decimal/status
flags, wrapping indexes and DP/stack edges. Another 7,936 rejection cases
make no bus access. Two in-memory ROM faults detect clear/search addresses.

Complete music callers pass 40,960 original-ROM cases, 25 pushed-child unwind
sites and requested volume-call seams. The current production bridge records
49,177 native invocations, zero positive fallbacks and 75 event redirects.
Actual SDK Stop/Yield/deadline/refresh/FastROM and timer controls pass 144/144;
48 controls latch a timer IRQ. C++17 linkage, eight compatibility gates,
isolated Windows Release and incremental GCC/Clang pass. Unchanged CLI
executable/fixture hashes reuse previous acceptance evidence.

All 3,000 production CPU/WRAM, PPU/picture, APU/SPC, DSP and PCM rows match
both the prior baseline and the kernel with only these operations disabled.
The scenario executes three clear operations and one search. Actual decoder
calls fall from 10,056 to 9,640; no wall-clock speedup is claimed. Two public
operation APIs are added; canonical entries, bindings and C units are not.
Full regression is deferred. Tests remain private; ROM and owner SRAM are
unchanged. Counts remain 1,819 verified, zero drafts and 1,786 bindings.

# Semantic member equipment and bonus restoration - 2026-10-20

The verified 81:ED8E and 81:EE94 owners now share a C word-restoration core.
It restores stored equipment, clears bare-record equipment and restores bare
bonus words. Source and field positions are ordinary values; a thin shell
exports the final CPU registers and flags. The original guest counter is
initialized and re-read every iteration. Its 16-bit decrement writes high
before low. Bare equipment retains TDC's direct-page value, including nonzero
values. Aliased pointers, source/index wrapping and bus order stay literal.
No fixed host iteration limit replaces the guest loop. No public API,
instruction-PC adapter, binding or canonical entry is added.

One combined checkpoint passes 5,286 original-ROM CPU/full-WRAM/exact ordered
data-bus/device cases. It covers system/WRAM banks, both formats and decimal
flags, source/destination/pointer aliases, index and direct-page edges, and
directed counter aliases that terminate after one write instead of seven
or nine. Two in-memory operand faults detect equipment and bonus counts.
The real production bridge passes 4,096 complete cases per owner, with zero
interpreter fallbacks. MSVC, isolated Windows Release and incremental GCC/
Clang pass; unchanged CLI executable/fixture hashes reuse prior evidence.
The final source-only adjustment removes an obsolete comment.

All 3,000 production CPU/WRAM, PPU/picture, APU/SPC, DSP and PCM rows match
the accepted identity-restoration baseline. The shared payload core executes
49 times; decoder count remains 10,056. This improves semantic C structure
without claiming new function coverage or measured speed. Identity, growth
and derived-stat source regions remain byte-identical. Full regression is
deferred. Tests stay private; ROM and owner SRAM are unchanged. Counts remain
1,819 verified, zero drafts and 1,786 ordinary bindings.

# Semantic member identity restoration - 2026-10-20

The verified 81:ED8E and 81:EE94 owners now share ordinary C operations for
record headers, stored levels, name prefixes, terminated names and trailers.
The CPU shell exports the original widths, indexes, accumulator and flags.
Record writes re-read the original long pointer on each access. Source bank
carry, direct-page wrapping, overlapping records, decimal pointer arithmetic,
16-bit name-padding wrap and the three-byte trailer advance remain literal.
Equipment, bonus and final HP/MP stores use the same typed record accessors.
No instruction-PC switch, public API, binding or canonical entry is added.

The combined checkpoint passes 6,060 original-ROM prefix cases with CPU,
full WRAM, exact ordered data-bus vectors and device state comparisons.
Coverage includes both formats, all 256 data banks, names of length 0..36,
overlong names through 1,024 bytes, decimal flags, source/destination aliases,
and bank/index/direct-page wrapping. Two in-memory ROM operand faults detect
incorrect padding length and trailer advancement. The real production bridge
passes 4,096 complete cases per owner, with zero interpreter fallbacks.

An instrumented production replay executes identity restoration 49 times.
All 3,000 CPU/WRAM, PPU/picture, APU/SPC, DSP and PCM rows equal the preceding
accepted baseline. Decoder count remains 10,056. This replaces instruction
shaped semantic code; it does not claim new coverage or a measured speedup.
Strict MSVC, isolated Windows Release and incremental GCC/Clang checks pass.
Unchanged CLI executable/fixture hashes reuse previous acceptance evidence.
Base-stat growth and derived-stat source regions remain byte-identical.
Full regression is deferred; affected complete owners share this checkpoint.
Tests stay private. ROM and owner SRAM remain unchanged. Counts remain
1,819 verified, zero drafts and 1,786 ordinary bindings.

# Semantic party base-stat growth - 2026-10-20

The existing verified 81:F87F routine now separates level-growth accumulation,
fractional scaling and initial base-stat addition from the CPU-state shell.
The calculation uses the existing banked WRAM view and value arithmetic.
It reloads the growth pointer for every field, preserves aliased low/high
updates, decimal behavior, hardware multiplier accesses and original stack
order. The shell exports the original accumulator, overflow, carry, widths
and restored indexes. No instruction-PC switch or new API/binding is added.

One combined checkpoint passes 6,003 original-ROM CPU/full-WRAM/exact ordered
data-bus/device cases. These cover all member bytes, levels 1..99 and row
boundaries, 128 system banks, WRAM banks, decimal inputs, overlapping growth
pointers/work fields and stack edges, DP wrapping, and complete level-zero
and 65,535 counter cases. Large traces are compared exactly in memory.
Three in-memory ROM faults detect row advancement, scaling and base sources.

The current-source production bridge passes 4,096 F87F cases plus 512 cases
for each of ED8E and EE94, all with zero interpreter fallbacks. The fixture
models no paired interpreter bounce; unexpected ancestry/yield services fail
closed. It links the real host-event module without event subscribers.
An actual production replay executes the reconstructed routine 105 times;
all 3,000 CPU/WRAM, PPU/picture, APU/SPC, DSP and PCM rows equal the preceding
accepted baseline. Decoder count remains 10,056; this semantic reconstruction
does not claim new canonical coverage or a wall-clock speed improvement.

Strict MSVC semantic compilation, isolated Windows Release and standalone
GCC/Clang pass. Unchanged CLI/fixture hashes reuse prior evidence. Public
interfaces and runtime binding definitions stay unchanged. Full regression
is deferred. Tests remain private; ROM files and owner SRAM are unchanged.
Counts remain 1,819 verified, zero drafts and 1,786 ordinary bindings.

# Finite session actor operations - 2026-10-20

Three ordinary session-actor APIs now share the verified presentation and
record-restoration algorithms with the canonical field-session owner. The
consumer admits whole finite operations at 83:B327, B364 and B3B0 before
the existing instruction adapter. It emits original opcode/data accesses in
order and charges exact path cycles, including indexed-store page costs.
Record accesses must map entirely to WRAM. Tight refresh-aware budgets,
unsupported contexts, pending interrupts and explicit Stop/Yield contracts
retain the old paths. No new instruction-per-PC switch is added.

The combined original-ROM checkpoint passes 50,416 API/region/data-bus cases
and 94,464 consumer CPU/full-WRAM/fetch/data/device/cycle cases. These include
directed aliases, bank 7F/FF and 24-bit wrapping. The main consumer matrix
admits 48,336 operations and declines 16,176 without side effects. Another
11,776 guard cases access no bus. The 5,104 complete internal installation
cases retain six controlled child services and their unwind/width contracts;
the children are not newly decompiled by that fixture. Three independent
in-memory ROM faults detect masks, reserved IDs and mode selection.

All 120 actual SDK Stop/Yield/deadline/refresh/FastROM controls match CPU,
resume PCs, elapsed clocks, refresh state and WRAM hashes. Twelve additional
observed admissions prove all three operations execute in the actual kernel.
All 3,000 production CPU/WRAM, PPU/picture, APU/SPC, DSP and PCM rows match
both the old-path kernel and the preceding accepted baseline. This scenario
executes one presentation span: old instruction-adapter calls fall from 473
to 465, replaced by one operation. Identity/reserved spans have zero hits in
this scenario; their actual admissions are covered by the controls above.
Decoder count remains 10,056; no wall-clock or whole-game speedup is claimed.

Fourteen availability gates, C++17 linkage, isolated Windows Release and
standalone GCC/Clang pass. Unchanged CLI/fixture hashes reuse prior evidence.
Full regression remains deferred. No canonical entry, binding or C unit is
added; 1,819 remain verified with zero drafts. Tests stay private. ROM files
and owner SRAM remain unchanged; nothing is pushed.

# Semantic session actor records - 2026-10-20

The existing field-session owner now restores actor identities and the
reserved actor through a banked record view with ordinary C parameters and
return values. Destination presentation selection is a separate value
operation. CPU-state shells preserve final flags and the original post-child
16-bit prefix/handoff. Source re-reads, aliasing, field order and child frames
remain literal. Existing single-instruction adapters are unchanged; this
reconstruction adds no public API, binding, canonical function or C unit.

The combined original-ROM checkpoint passes 28,144 CPU/full-WRAM/ordered
data-bus/device cases: 23,040 operation cases and 5,104 complete internal
83:B30D..B3CF actor-installation cases. The latter cover 128 system banks,
six explicitly controlled child services, all six child-unwind sites and
post-child width/bank/index changes. They do not re-prove those children's
algorithms. Another 14,592 directed cases cover source/destination aliases
and bank wrapping. Three in-memory ROM faults detect identity masks,
reserved IDs and presentation selection. Test fixtures distinguish repeated
stop PCs by completed child calls, including non-returning child frames.

Isolated Windows Release and standalone GCC/Clang pass. Unchanged CLI
executables and fixtures reuse prior evidence by exact hashes. Public API
and instruction-adapter evidence is reused for byte-identical headers,
consumer files and step bodies. All 3,000 actual production CPU/WRAM,
PPU/picture, APU/SPC, DSP and PCM rows remain equal; decoder count remains
10,056. No runtime speed or new canonical coverage is claimed. Full
regression is deferred. Tests/probes remain private, with no ROM-file/SRAM writes
or pushes. Counts remain 1,819 verified and zero drafts.

# Finite scene rebuild phases - 2026-10-20

The verified 83:A82E scene owner now shares bounded sprite-slot discovery,
sprite flag reset and party-offset placement with its continuation consumer.
The complete owner retains its original child calls and unwind boundaries.
These three finite operations add no canonical entry, binding or C unit.
Unsupported contexts, pending interrupts, explicit stops/custom yields and
insufficient refresh-aware clock budgets retain original execution.

One combined checkpoint passes 63,744 original-ROM CPU/full-WRAM/data-bus/
device cases and 158,400 consumer cases including all instruction fetches,
data order, device state and path clocks: 74,688 native, 83,712 handoffs.
Full-owner checks pass 16,384 states over 128 system banks, retaining actual
A746/A6DF child execution; 6,656 actual child-entry/unwind cases also pass.
No-access guards pass 5,376 API and 13,056 consumer cases. Three independent
ROM faults detect sentinel, flag-mask and party-offset changes. Six CMake
compatibility gates, C++17 linkage, GCC/Clang and isolated Windows pass.
Unchanged CLI executables/fixtures reuse prior evidence by exact hashes.

All 120 actual SDK Stop/Yield/deadline/refresh/FastROM controls match original
CPU, resume PC, elapsed clocks, refresh state and WRAM hashes. An initial
consumer failure exposed indexed-store page penalties; the corrected final
consumer passes the complete matrix. Production retains every one of 3,000
CPU/WRAM, PPU/picture, APU/SPC, DSP and PCM rows. Decoder work drops from
10,440 to 10,056 opcodes in this scenario, not a whole-game speed claim.
Full regression remains deferred. Tests and controls stay private. No push.
Counts remain 1,819 verified, zero drafts and 1,786 ordinary bindings.

# Bounded menu wait polls - 2026-10-20

Four original menu wait owners now share six complete finite operations:
button tests, mask selection, countdowns and sprite-motion polling. Their
typed poll API preserves literal CPU/bus state. Recomp admits one operation
between the original child calls, emits original fetches in data-bus order,
and charges the exact path cycles. No per-instruction PC switch is added.
Pending interrupts, unsupported contexts, active stop/custom-yield contracts
and insufficient worst-case master-clock budgets retain original execution.

The shared checkpoint passes 9,216 ROM semantic cases and 147,456 consumer
CPU/full-WRAM/ordered-fetch/data-bus/device/cycle cases, including paired word
reads and 46,080 untouched deadline handoffs. Whole-parent checks pass 1,536
normal cases, 384 real returns, 352 child unwinds, 140 post-child boundaries
and 24 child-mutated stack counters. Another 12,288 API and 27,648 consumer
guards access no bus. Three original-ROM faults detect button bits, mask
addresses and countdown direction. Eleven CMake availability cases pass.
GCC/Clang, isolated Windows Release and C++17 API linkage pass; unchanged CLI
hashes reuse prior CLI evidence. Full regression remains deferred.

Actual production retains all 3,000 CPU/WRAM, PPU/picture, APU/SPC, DSP and PCM
hash rows of the preceding accepted scenario. Decoder work falls from 11,511
to 10,425 opcodes; this is no wall-clock or whole-game coverage claim. Twelve
real production Stop/Yield controls intercept inside all six operations.
No canonical function, binding or C unit is added; 1,819 remain verified.
Tests/probes stay private. ROM and owner SRAM are read in place only.

# Shared battle status owners - 2026-10-20

Status snapshots, selected-record clearing and five-party status markers now
have one original algorithm each. Outcome and frame-upkeep callers share the
same bodies with the canonical entries. Existing public guards, child services,
stack frames, banks and observed CPU/bus behavior remain intact. Marker-bank
selection preserves the legacy frame owner's fixed bank and public PHK context.
No public API, canonical entry, binding or instruction-PC switch is added.

One combined checkpoint passes 5,632 original-ROM CPU/full-WRAM/ordered-data-bus/
device cases across the algorithms and complete outcome, upkeep and party-reset
callers; 4,608 unchanged no-access guards; and 1,536 actual production bridge
CPU/full-WRAM cases with zero fallback. Two in-memory operand faults are caught:
record status offset and new-marker sentinel. GCC/Clang and isolated Windows
Release pass. Unchanged CLI executable/fixture hashes reuse prior CLI proofs;
public headers are unchanged. Full regression and a new gameplay replay are
deferred. The upkeep fixture disables unrelated sprite rendering; preceding
sprite-renderer proofs cover that unchanged subtree. No whole-game claim.

The first synthetic fixture set the sprite-count field rather than the renderer
mode and exceeded its trace bound. The corrected fixture sets the original
renderer-mode field; no production behavior or expectations were weakened.
All 1,819 existing functions remain verified; no new functions are counted.
Tests, probes and receipts remain local. The combined final state was tested;
individual intermediate edits were not separately validated.

# Shared battle sprite algorithms - 2026-10-20

Five existing algorithms now have one shared implementation each: ordinary
and mirrored OAM grids, two-row tile grids, character sprite words and packed
sprite coordinates. Canonical public routines retain their original guards;
frame and popup callers use the same unchecked internal bodies with their
original frames. Operation-local DP names describe grid inputs and progress.
No public API, canonical entry, binding, C unit or instruction-PC switch is added.

The shared checkpoint passes 11,368 original-ROM algorithm cases, including
widths, decimal arithmetic, zero counters and boundary destinations; 4,096
complete frame-renderer cases; and 3,072 complete popup-caller cases. CPU,
full WRAM, ordered data bus and device state match. Another 1,024 existing
parent-context and 3,072 public lookup/mirror guards access no bus. Ten actual
production bridges pass 2,560 CPU/full-WRAM cases with zero fallback. Two
faults detect OAM stepping and lookup-table offsets. Incremental GCC/Clang
and isolated Windows Release pass; unchanged public headers and CLI hashes
reuse preceding linkage/CLI proofs. Full regression and a new replay are deferred.

The initial parent fixture bypassed existing party/tilemap decimal guards,
visited unseeded records and exceeded its trace bound. Final parent tests
verify those exact contexts as unchanged no-access handoffs; the shared
algorithms retain independent decimal-mode ROM coverage. No production guard
was changed. All 1,819 existing functions remain verified; tests stay private.

# Shared IP record owner - 2026-10-20

The IP list builder no longer duplicates the 81:F45E pointer calculation.
It shares one internal helper with Lufia2IpRecordPointer. The canonical
public guard remains unchanged; the original internal caller retains its
unguarded CPU/bus contract. Original subtraction, negative exit, TDC,
indexed ROM read, flags and near/long child frames remain literal.
No public API, binding, canonical entry or function count is added.

A shared checkpoint passes 131,072 original-ROM pointer cases covering
every accumulator value with two DP contexts, 1,024 complete IP builders
and 12,288 affected list/entry cases. CPU, full WRAM, ordered data bus and
device state agree under the explicit verifier bus model. Three actual
production bridges pass 3,072 CPU/full-WRAM cases with zero fallback using
ROM-backed item/IP records and original negative/sentinel paths. Two faults
are detected. Incremental GCC/Clang and isolated Windows Release pass;
unchanged public declarations and CLI executable/fixture hashes retain
prior linkage/CLI proofs. Full regression and a new replay are deferred.

Synthetic out-of-table item/IP values can address unmapped space, whose
open-bus behavior differs between the fixture and SDK. The committed
pre-refactor source reproduces the invalid-item discrepancy. Those cases
remain documented for a separate consumer audit; no production equivalence
for arbitrary invalid records is claimed. ROM tables bound 467 item and
168 IP pointers before their first records. Tests and ROM remain private.

# Literal battle-list read order - 2026-10-20

The existing IP/spell/item entry helpers and 81:DFA2 list owner retain two
original access boundaries. IP glyph mode is read after the entry palette
and header advance; tile attributes are read after both glyph writes.
Previously eager C argument evaluation moved these reads earlier. Shared
helpers now own the reads at the original points, preserving overlapping
output/scratch behavior. Local entry-count and list-mode DP names document
the operation. No new function, public API, binding or gameplay patch is added.

The shared checkpoint passes 12,288 original-ROM comparisons across all
three internal entry renderers and the complete eight-row parent, including
DP/high-index and scratch/output alias cases. CPU, full WRAM, ordered data
bus and device state match. Another 1,024 actual production-object bridge
cases match original-ROM CPU/full-WRAM state with zero interpreter fallback.
Independent controls detect each original early-read defect. Incremental
GCC/Clang and isolated Windows Release pass; CLI executable/fixture hashes
remain unchanged. Public C++ declarations are unchanged. Unaffected source
functions retain their prior proofs. Full regression and a new battle replay
are deferred; this is not evidence for a specific reported visual bug.
Internal E019/E094/E100 ownership is recorded without adding duplicate APIs.
All 1,819 existing functions remain verified; tests and ROM remain private.

# Save record-tail copies - 2026-10-20

The 85:C954/85:C60E save owners now share portable copy logic for the
63-byte saved Capsule-stat region and 28-byte battle-submenu choices.
Names follow the existing WRAM catalogue. X descends through saved state,
Y advances through the record, and original signed DEX termination and
M/X behavior remain literal. No new canonical entry or binding is counted.
Four bounded loop paths use the existing clock/fetch-only consumer and
retain deadlines, page penalties, bus order and final branch timing.

One checkpoint passes 16,360 bounded/full-width ROM comparisons, 15,220
consumer CPU/full-WRAM/fetch/data/clock cases (12,574 native, 2,646 fallback),
7,168 portable and 16,384 consumer no-access guards. Affected prior save
checks pass 15,184 spans, 77,464 consumer cases, 23,552 guards and 4,385
complete-parent/child-unwind cases. Two injected faults are detected.
Incremental GCC/Clang, isolated Windows Release and C++17 linkage pass.
CLI results reuse unchanged executable/fixture hashes. Unchanged common
bus, menu and injection sources retain the preceding Stop/Yield proof.
Full regression remains deferred; checks cover the combined final batch.

All 3,000 production frame CPU/WRAM/image/audio hash rows match the prior
accepted build. Actual decoded instructions fall from 15,151 to 11,511
in this local read-only save/menu/field scenario. No whole-game coverage
or wall-clock speedup is claimed. Tests, SRAM and ROM remain private.

# Clocked save snapshot spans - 2026-10-20

The original 85:C954 pack and 85:C60E restore share a readable field/count
copy helper. The four bounded paths preserve banked accesses, signed DEY
termination, CPU widths, flags and ordered bus accesses. The complete parents
retain their original child and stack contracts. The inventory name follows
82:FCED packed count evidence; phase-dependent WRAM overlap remains explicit.
No new canonical function, ordinary binding, C unit or draft is added.
Totals remain 1,819 verified functions, 500 C units and 1,786 own bindings.

The consumer runs at most fourteen complete iterations inside its deadline,
with original fetch order, source page penalties and indexed-store clocks.
Shared plumbing handles only fetches and CPU transfer. Explicit stop PCs and
custom yield conditions disable bulk execution. Waiting, stopped or pending
interrupt states retain fallback. Whole-entry bank guards remain unchanged.

The shared checkpoint passes 15,184 bounded/full signed-width ROM cases,
4,385 complete-parent/child-unwind cases and 77,464 production-consumer CPU,
full-WRAM, ordered fetch/data bus and clock cases (67,639 native, 9,825
fallback). Another 7,168 API and 16,384 consumer guards access no bus.
Affected menu checks pass 15,513 spans, 20,172 consumer cases, 3,072 API
and 8,192 consumer guards. Two injected ROM faults detect destination and
signed-branch errors. Production Stop and Yield regressions now pass.
Incremental GCC/Clang, isolated Windows Release and C++17 linkage pass.
Unchanged CLI executable/fixture hashes reuse accepted CLI results.
Full regression is deferred; all checks cover the combined final batch.

All 3,000 production frame rows match the preceding accepted build in CPU,
full WRAM, PPU/image, APU, DSP and PCM hashes. Actual decoder instructions
fall from 34,751 to 15,151 in the same read-only save/menu/field scenario.
This does not establish whole-game coverage or a wall-clock speedup.
Tests, local SRAM and ROM remain private; no save file is written.

# Bounded menu field-buffer restores - 2026-10-20

The two original 82:F682 copy loops now share a readable byte-copy helper.
Lufia2MenuRestoreFieldBufferSpan exposes a bounded part of either field or
message snapshot. X remains the original byte index; memory order and CPU
flags remain literal. The complete existing entry retains its contract.
No instruction-PC switch, canonical function, ordinary binding or draft
is added. Totals remain 1,819 verified functions and 500 C units.

The consumer executes at most fourteen complete iterations within the current
master deadline, retaining original instruction fetches, data bus order and
indexed-store timing. Insufficient budget or unsupported contexts use the
existing fallback. A partial iteration can still cross a frame as before.
The portable code owns copy semantics; the consumer owns clocks and fetches.
Activation requires the verified original owner and the new portable API.

One shared checkpoint passes 15,513 original-ROM span comparisons, 1,032
complete-parent comparisons and 20,172 selected-production CPU/full-WRAM/
fetch/data-bus/clock comparisons (15,216 native and 4,956 fallback cases).
Another 3,072 portable and 6,144 consumer guards access no bus. Two fault
controls detect a destination offset and an indexed-store timing error.
GCC, Clang, isolated Windows Release and C++17 linkage pass. Unchanged CLI
executables and fixtures reuse accepted CLI proofs. Full regression is deferred.

A 3,000-frame production-object run covers startup, both splashes, intro,
local save loading, savepoint menus and field movement. All 3,000 per-frame
CPU, full-WRAM, PPU/image, APU, DSP and PCM hash rows match the original
production baseline. Actual decoder instructions fall from 104,621 to 34,751
in this scenario. These counts do not establish whole-game coverage or a
wall-clock speedup. Tests, scenario data and ROM remain private.

# Literal reset entry - 2026-10-19

Lufia2ResetSystem reconstructs the complete $80:8000 reset through the
$80:8035 JML, including the $00 ROM mirror. Its explicit entry-emulation
argument keeps the common native CPU ABI unchanged. Original stack PHA/PLB,
old direct-page clears, banked video/NMI/DMA writes, new stack/direct page,
repeated work-byte clears, index truncation and XCE carry are preserved.
An arbitrary native stack can read MMIO or ROM: DB is the actual pulled byte.
No instruction-PC switch or gameplay patch is added.

The shared checkpoint passes 262,144 original-ROM comparisons of CPU, full
WRAM, ordered bus and device state; 65,536 rejected-context guards access no
bus. Native states cover every stack word; emulation states cover every DP
word. Fault controls detect old-DP and old-E/carry mistakes. Incremental
GCC/Clang, isolated Windows Release and C++17 linkage pass. CLI results are
reused only for identical executables and fixtures. Full regression is deferred.

An experimental production adapter also passed 262,144 controlled ABI cases.
However, the physical 1,500-frame startup and reset-jump probes observed zero
native reset entries. Current LLE dispatch bounces calls, not reset/JML entries.
The experimental adapter and binding were withdrawn; this complete semantic
function is verified and UNBOUND. Startup reached both splashes and intro,
with all 1,501 frame/boot rows identical to the accepted production baseline.
This is not evidence of native reset activation, timing or a speedup.

Totals: 1,819 verified functions, 1,786 ordinary bindings, 33 unbound verified,
zero drafts, 500 C units. Tests, tooling and ROM remain private.

# Primary actor party commands - 2026-10-19

The child-enabled primary actor VM now reconstructs commands $49/$4E at
$83:CAE1/$83:CB71. Shared record scans use ordinary C; original readiness,
release/rebuild/action children, slot search, temporary slot selection and
five follow-slot writes retain their original order and CPU boundaries.
The readiness scan skips slot zero. A missing $FD actor leaves X=$FFFF;
the temporary slot word and subsequent byte restore remain literal.

Lufia2ActorPrimaryUpdateWithChildren uses the existing pushed-child contract.
The legacy API delegates without a child service and keeps its previous
unknown-handler boundaries. Existing handlers and the old VM loop are
unchanged outside the new command selection. The consumer C7F8 bridge reuses
its unchanged child/return helpers. No instruction-PC switch is introduced.

One checkpoint passes 20,992 original-ROM boundary comparisons each for the
command workers, full primary VM and selected production bridge: 34,828
controlled child frames, 896 child unwinds and 724 changed-child context
handoffs per arm. Another 2,048 composed VM cases include 1,408 actual ROM/
native record-offset child returns; other children remain controlled services.
All compare CPU/full WRAM and ordered bus effects. The ABI fixture uses exact
production entry/state/child/return functions with controlled SDK bus/dispatch;
it does not claim physical device timing or gameplay coverage.

65,536 unsupported-context/no-child guards perform no bus access. Two injected
faults in readiness bounds and slot-word writes are detected. Strict MSVC,
isolated Windows Release, incremental GCC/Clang and C++17 linkage pass.
Unchanged CLI executable/fixture hashes reuse accepted CLI results. Shared
bridge helper proofs are retained by source equivalence. Full regression and
manual gameplay are deferred; no speed improvement is claimed.

These are two newly reconstructed internal VM handlers, not separate callable
RTS functions. The verified C7F8 owner and its ordinary binding remain the
canonical entry. Totals stay 1,818 verified/1,786 ordinary bindings/zero drafts;
there are now 500 C units. Tests, proofs and ROM remain private.

# Readable party derived totals - 2026-10-19

The existing 81:F4D5 contract now delegates its seven base/modifier totals,
cap, overrides and two derived totals to a record-based C core. CPU state is
confined to the existing entry/checkpoint shell. Chained carries, decimal
arithmetic, wrapped 24-bit addresses, live bus reads and write order remain.
No cached record, new instruction dispatcher, public API or binding is added.

One checkpoint passes 2,731 whole-routine original-ROM CPU/full-WRAM/MMIO
comparisons and 8,192 prior/new C CPU/ordered-bus/checkpoint comparisons.
Normal record cases cover the cap boundary and overrides; arbitrary alias
cases cover stack/DP/index/bank edges against the prior implementation.
Two negative controls detect a changed cap and a dropped chained carry.
An initial cap mutant exposed missing exact-boundary input; that input was
added before acceptance. Arbitrary invalid return-stack cases are not claimed
as successful whole-ROM calls. MSVC strict proof, isolated Windows Release,
and incremental standalone GCC/Clang builds pass. Unchanged CLI executable
and fixture hashes reuse the prior CLI result. Full regression is deferred.

This is semantic reconstruction quality work on one existing verified owner,
not another canonical function or a measured runtime optimization. Totals
remain 1,818 verified, zero drafts and 1,786 ordinary bindings. Tests and ROM
remain private; unrelated owner work is untouched.

# Field actor installation continuations - 2026-10-19

The optional FieldActorInstallStep API exposes all seventy-three boundaries
of actor installation, $83:B30D through $83:B3CC, under three existing verified
session owners. Initialization, the nominal sixty-three-record pass, six
original child calls and the optional actor tail retain their CPU/bus contracts.
The preceding B309 bytes are not an entry; B3CF belongs to the next operation.
Existing semantic bodies and public declarations, including FieldFrameStep,
remain unchanged. No canonical function, ordinary binding or hook is added.

TDC retains its full accumulator and flags. Indexed accesses carry into the
next bank; the SDK's read-page and indexed-store clocks remain distinct.
Index stack/width changes and direct-page wrap remain literal. Original word
services handle both the LDX operand and wide DP index reads without duplicate
fetches. The consumer retains generic children, returns, NMI, clocks and
progress. Unsupported widths, emulation, pending interrupts and unknown
PCs/opcodes remain interpreted. The API and all three named, addressed verified
session owners gate activation; older headers compile without new dependencies.

One shared checkpoint passes 5,827,072 direct and 5,827,072 actual-producer
original-ROM/CPU/full-WRAM/ordered-bus cases across every stack/DP/accumulator/
index word, all statuses and accepted widths, all DB banks and twenty-one
edge addresses. Code/DP word claims, fallback and claimed DP-word wrap agree.
Another 37,839 producer, 280,711 API no-access and 65,463 unknown-PC guards
pass. Ten effective faults are detected. Nineteen CMake contexts, standalone
GCC/Clang, C++17 linkage and isolated MSVC Release pass. An unsigned comparison
cleanup required only an incremental consumer rebuild; portable sources were
unchanged. Identical CLI executable/fixture hashes retain accepted CLI checks.

All seventy-three points execute natively in 15,714 paired hardware contexts,
covering every accepted width contract and three DB/DP/stack/clock classes.
Actual bank carry, DP APU reads/writes, code/DP words, full TDC, stack wrap,
exact exits and one original control step per case agree. Public yield/
exclusion APIs stop before all six children; the generic kernel is unchanged.
Normal paired 10,607-frame runs retain all 77,840 observations and prior-main
artifacts, including real children at 5 of the six call sites. The remaining
call has controlled boundary coverage, not claimed normal replay execution.

Actual decoding falls from 37,180 to 35,213 through 1,967 additional
native phases. Control visits remain 179,420, including 144,207 native slot
phases. Previous native counts, graphics/audio, clocks, epochs and cache agree.
These replay counts are not whole-game coverage or a production speed claim.

Totals remain 1,818 verified / 1,786 ordinary bindings / 32 verified unbound /
791 WRAM fields / 499 C units / zero drafts; 193 hooks remain registered.
Unrelated full regression, other platforms and manual gameplay are deferred.
Tests, proofs and ROM remain local; parallel owner work is unchanged.

# Field frame continuations - 2026-10-19

The optional field_phase.h API exposes all seventy-seven contiguous dispatcher
boundaries from $83:8000 through $83:80CA under two existing verified owners.
The original nineteen calls retain their full targets, banks and stack frames.
Frame waiting, service dispatch, map-word exchange, request-bit clearing,
sound/text/input tests and loop branches keep literal CPU and bus behavior.
PHK/PLB publish the live data bank. Existing semantic bodies and public
declarations stay unchanged; no canonical function or binding is added.

The consumer keeps original generic child/return, interrupt, clock and progress
ownership. Word loads delegate the original word service exactly once. The
actual M1 byte and M0 map contracts, independent control/stack widths and
conditional branch clocks remain explicit. Emulation, pending interrupts,
unknown PCs/opcodes and unsupported widths stay interpreted. The optional API
and both named, addressed verified owners gate use; older headers compile
without new field-phase dependencies.

One shared checkpoint passes 6,368,768 direct and 6,368,768 actual-producer
original-ROM/CPU/full-WRAM/ordered-bus cases across every stack/DP word,
all statuses and accepted widths, and twenty-one edge addresses. Data-word
callback claims and fallback agree. Another 39,871 producer, 281,687 API
no-access and 65,459 unknown-PC guards pass. Nine effective faults are detected.
Fifteen CMake contexts, standalone GCC/Clang, C++17 linkage and isolated MSVC
Release pass; unchanged CLI executable/fixture contracts are reused.

All seventy-seven points execute natively in 19,926 paired hardware contexts,
covering every accepted width contract and three DB/DP/stack/clock classes.
Map words, DP APU BIT reads, TRB, bank restoration, stack wrap, exact exits and
one original control step per case agree. Public yield/exclusion APIs stop
before all nineteen children; the generic kernel is unchanged. Normal paired
10,607-frame runs retain all 77,840 observations and prior-main artifacts,
including real children at 10 of the nineteen call sites. Remaining calls
have controlled boundary coverage, not claimed normal replay execution.

Actual decoding falls from 38,578 to 37,180 through 1,398 additional
native phases. Control visits remain 179,420, including 142,240 native slot
phases. Prior native counts, graphics/audio, clocks, epochs and cache agree.
These replay counts are not whole-game coverage or a production speed claim.

Totals remain 1,818 verified / 1,786 ordinary bindings / 32 verified unbound /
791 WRAM fields / 499 C units / zero drafts; 193 hooks remain registered.
Unrelated full regression, other platforms and manual gameplay are deferred.
Tests, proofs and ROM remain local; parallel owner work is unchanged.

# Sound and window request continuations - 2026-10-19

Two optional APIs expose nineteen sound-command/mode boundaries and twelve
menu-window request boundaries under three existing verified owners. Dynamic
accumulator stack, status restoration, long APU stores, original call/return
frames, window flags and word-immediate reads remain literal. Existing semantic
bodies and declarations are unchanged; no canonical function or binding is added.

The consumer retains generic child/return, interrupts, clocks and progress.
Unsupported widths, emulation, pending interrupts and unknown PCs/opcodes stay
interpreted. Both APIs and all three named, addressed verified owners gate use.
Older headers compile without dependencies on either new API. The earlier
boot-NMI/logo polling path is outside this batch and remains unchanged.

One shared checkpoint passes 2,450,944 direct and 2,450,944 actual-producer
original-ROM/CPU/full-WRAM/ordered-bus cases, covering all stack/DP words,
statuses, accepted widths and twenty-one edge addresses. Word callback claims
and fallback agree. Another 16,073 producer, 532,177 API no-access and 131,041
unknown-PC guards pass. Eight effective faults are detected. Twenty-one CMake
contexts, standalone GCC/Clang, C++17 linkage and isolated MSVC Release pass;
unchanged CLI executable/fixture contracts are reused.

All thirty-one points execute natively in 6,318 paired hardware contexts with
every accepted width contract, three DB/DP/stack classes and clock positions.
APU stores, DP TSB, word operands, stack wrap, exact exits and one original
control step per case agree. Supported yield/exclusion APIs stop before calls;
the generic kernel is unchanged. Normal paired 10,607-frame runs retain all
77,840 observations and artifacts against prior main. The window upload call
executes its real child normally; both sound calls have controlled boundary
coverage and are not claimed as normal replay hits.

Actual decoding falls from 41,056 to 38,578 through 2,478 additional native
phases. Control visits remain 179,420, including 140,842 native slot phases.
Prior native counts, graphics/audio, clocks, epochs and cache agree. These
counts describe this replay, not whole-game coverage or production speedup.

Totals remain 1,818 verified / 1,786 ordinary bindings / 32 verified unbound /
791 WRAM fields / 499 C units / zero drafts; 193 hooks remain registered.
Unrelated full regression, other platforms and manual gameplay are deferred.
Tests, proofs and ROM stay local; parallel owner work is unchanged.

# Select transition continuations - 2026-10-19

The optional select_phase.h API exposes all sixty-three M1X0 boundaries across
object waiting, fade-in/out and input waiting under eight existing verified
owners. Twenty original JSR/JSL sites retain their actual return frames and
complete program banks. Button tests, fade RAM, DP upload flags, index stack,
loop branches, carry results and RTS remain literal. Existing semantic bodies
and public declarations are unchanged; no canonical alias or binding is added.

The consumer uses the original opcode slot and keeps generic child/return,
progress, interrupt and hardware clock ownership. Unknown PCs/opcodes, other
widths, emulation and pending interrupts remain interpreted. The optional API
and all eight named, correctly addressed verified owners gate activation. Old
headers compile without new select-symbol dependencies.

One combined checkpoint passes 4,435,200 direct and 4,435,200 actual-producer
original-ROM/CPU/full-WRAM/ordered-bus cases. Every stack/DP/accumulator/index
word and data bank, nineteen edge addresses and all M1X0 status combinations
are covered.
Another 32,697 producer, 81,790 API no-access and 65,473 unknown-PC guards pass.
These byte/stack operations never claim a word callback. Eight effective faults
cover call banks/targets, button bits, branch offsets, upload/fade operands and
return type. Thirty-nine CMake contexts, MSVC Release, standalone GCC/Clang and
C++17 linkage pass; unchanged CLI executable/fixture contracts are reused.

All sixty-three points execute natively in 5,103 paired hardware contexts,
including three data banks, DP/stack classes and clock positions, APU DP stores
and wrap. Each checks one original control step and its exact exit. Existing
public yield/exclusion APIs stop before children; the kernel is unchanged.
Normal paired 10,607-frame runs additionally execute nineteen of the twenty
call sites with actual children. All 77,840 observations and artifacts match
prior main. Actual decoding falls from 56,088 to 41,056 through 15,032 additional
native phases. Bridge control steps remain 179,420; previous native counts,
graphics/audio, clocks, progress epochs and cache agree.

Totals remain 1,818 verified / 1,786 ordinary bindings / 32 verified unbound /
791 WRAM fields / 499 C units / zero drafts; 193 hooks remain registered.
Unrelated full regression, other platforms, manual gameplay and production
speed measurements are deferred. Tests, proofs and ROM remain local.

# Sound frame continuations - 2026-10-19

Two optional sound_phase.h APIs expose eighteen resource-header phases and
seventeen reply phases under the existing verified owners. They preserve source
pointer/byte access, APU stores and polls, source-bank wrap, the original JSR
frame, TDC's full sixteen bits, saved status, dynamic widths and RTS. Existing
semantic bodies and declarations are unchanged; no canonical alias is added.

The consumer retains the original generic child/return, progress, interrupt and
clock accounting. Its transient adapter delegates atomic pointer words once,
even when later byte data aliases the pointer. The original sixteen-bit LDY
immediate also uses the word service; its ROM operand is read without duplicate
fetches and retains the X-width clock. Unsupported widths, emulation, pending
interrupts and unknown PCs/opcodes stay interpreted. Both APIs and both named,
correctly addressed verified owners gate activation. Old headers compile
without new sound-symbol dependencies.

One combined corrected checkpoint passes 2,686,212 direct and 2,686,212 actual-
producer original-ROM/CPU/full-WRAM/ordered-bus cases across every stack/DP word,
all statuses and accepted widths, with twenty-one edge addresses. Atomic pointer
and code words, fallback, wrap and pointer/data alias cases pass. Another 18,162
producer, 542,205 API and 65,501 unknown-PC guards pass. Eight effective faults
cover pointer/port/bank/branch/call/return and repeated-word access. Sixteen CMake
contexts, MSVC Release, standalone GCC/Clang and C++17 linkage pass. Unchanged
CLI executable/fixture contracts are reused.

All thirty-five points execute natively in 5,913 paired hardware contexts,
covering every accepted width contract, three data banks, DP/stack classes and
clock positions. Actual APU pointer words, wrap and exact exits agree. Existing
public yield/exclusion APIs stop before the original child; the kernel is
unchanged. Every case checks one original control step. Normal paired 10,607-
frame runs also retain actual child execution and all 77,840 observations and
artifacts against prior main. Actual decoding falls from 67,986 to 56,088
through 11,898 additional native phases. Bridge control steps remain 179,420;
previous native counts, graphics/audio, clocks, progress and cache agree.

Totals remain 1,818 verified / 1,786 ordinary bindings / 32 verified unbound /
791 WRAM fields / 499 C units / zero drafts; 193 hooks remain registered.
Unrelated full regression, other platforms, manual gameplay and production
speed measurements are deferred. Tests, proofs and ROM remain local.

# Menu input continuations - 2026-10-19

The optional menu_input_phase.h API exposes all thirty-three M1X0 instruction
boundaries of the existing verified menu input loop. It retains button reads,
cursor decisions, redraw requests, cursor blinking and sprite-frame calls.
All five original JSR/JSL frames keep their return words and complete banks.
Absolute indexed addresses retain carry into the next bank. The old semantic
functions and declarations remain unchanged; no canonical alias is added.

The consumer executes these phases at the original opcode slot, preserving
operand/data/stack bus order, indexed-store clock accounting and atomic word
claims. Generic child dispatch, returns, interrupts, progress and hardware
clocks stay original. Other widths, emulation, pending interrupts and unknown
PCs/opcodes retain interpretation. The named verified owner and optional API
gate activation. Old headers compile without new menu-symbol dependencies.

One shared checkpoint passes 2,314,752 direct and 2,314,752 actual-producer
original-ROM/CPU/full-WRAM/ordered-bus cases across every stack/index/accumulator
word and data bank, with eighteen boundary addresses and word claims/fallbacks.
Another 17,127 producer guards, 74,050 API no-access guards and 65,503 unknown-PC
checks pass. Six effective faults detect wrong call banks/targets, loop offsets,
data operands and a lost cached word byte. Eleven CMake contexts, MSVC Release,
standalone GCC/Clang and C++17 linkage pass. Unchanged CLI contracts are reused.

All thirty-three points execute natively in 2,673 paired hardware contexts.
These cover three data banks, index/stack classes and clock positions, including
APU accesses, address carry and stack wrap. Existing public yield and bounce
exclusion APIs stop before original children; the kernel is unchanged. Each
case checks one original control step and its exact exit. Normal paired
10,607-frame runs additionally retain actual compiled child execution at all
five calls, with all 77,840 observations and artifacts identical to prior main.
Actual interpreter decoding falls from 80,874 to 67,986 through 12,888 additional
native phases. Original bridge control steps stay 179,420; prior native counts,
graphics/audio, clocks, progress epochs and cache agree.

Totals remain 1,818 verified / 1,786 ordinary bindings / 32 verified unbound /
791 WRAM fields / 499 C units / zero drafts; 193 hooks remain registered.
Unrelated full regression, other platforms, manual gameplay and production
speed measurements are deferred. Tests, proofs and ROM remain local.

# Frame register continuations - 2026-10-19

Two optional frame_wait.h APIs expose twenty wait-register points and nine
sprite-frame points under six existing verified owners. They preserve live
status, accumulator/index widths, stack byte order and complete native stack
wrap. The sprite-frame return retains its original RTL bank; simple wait
returns retain RTS. Existing semantic bodies and declarations are unchanged.
No canonical entry, ordinary binding or hook is added.

The consumer selects these complete phases at the original opcode slot.
Generic call/return ownership, progress accounting, interrupt handling and
hardware clocks remain unchanged. Unsupported PCs, opcodes, emulation and
pending interrupts retain interpretation. Both APIs and all six named,
correctly addressed verified owners gate the optional integration. Old headers
produce an actual compilable core without new register-symbol dependencies.

One combined checkpoint passes 2,494,464 direct and 2,494,464 actual-producer
ROM/CPU/full-WRAM/ordered-bus comparisons. Coverage includes every stack word,
all status bytes and four widths at twenty boundary addresses, plus raw high
index robustness states. Another 14,993 producer and 539,165 API guards and
262,115 unknown-PC checks pass. Stack accesses never claim a word callback.
Four effective faults detect incorrect width masks and an RTL return error.
Thirty-two CMake contexts, MSVC Release, standalone GCC/Clang and C++17 linkage
pass. Unchanged CLI executable/fixture contracts are reused.

Paired 10,607-frame runs retain all 77,840 observations and artifacts against
prior main. Actual interpreter decoding falls from 87,525 to 80,874 through
6,651 additional native phases; original bridge control steps remain 179,420.
All twenty-nine points have positive coverage in 1,044 hardware contexts,
including four valid widths, three stack classes, three RTL banks and three
clock positions. CPU, WRAM, graphics/audio hardware, clocks, progress epochs
and cache agree. The existing public stop-PC API handles wrapped returns;
the SDK's original unsigned stack-depth watermark is not changed.

Totals remain 1,818 verified / 1,786 ordinary bindings / 32 verified unbound /
791 WRAM fields / 499 C units / zero drafts; 193 hooks remain registered.
Unrelated full regression, other platforms, manual gameplay and production
speed measurements are deferred. Tests, proofs and ROM remain local.

# Frame-wait decoder boundaries - 2026-10-19

The existing frame-counter APIs now execute at the original opcode slot when
prior native hooks cannot handle that context. The selector covers 45 loads,
comparisons and branches under fourteen verified owners. A complete optional
frame_wait.h return API preserves twelve original RTS boundaries, including
stack/address wrap. ROM bytes confirm RTS; no RTL behavior is substituted.
Existing semantic function bodies and declarations are unchanged.

The consumer keeps the original generic scheduler, quiescence, return-frame,
bus and clock accounting. Its transient word adapter delegates to the same
interpreter word callback, consuming the returned high byte without another
access. Declined word claims retain the original two byte accesses. Unsupported
PCs, opcodes, emulation and pending-interrupt contexts retain interpretation.
All fourteen named verified contracts and both phase APIs gate the feature.

One combined checkpoint passes 4,887,552 actual-producer ROM/CPU/WRAM/bus cases,
786,432 direct return cases, 29,469 producer mode guards, 458,695 unknown-PC
producer guards and 461,800 API no-access guards. Counter tests vary every DP
word and both accumulator widths; return tests vary every stack word. Synthetic
register robustness states are separate from valid physical width contexts.
Atomic word claims/fallbacks and four effective opcode/operand/cache faults
pass. Sixty-eight CMake contexts preserve prior properties. The actual generated
old-API core compiles without new symbol dependencies. MSVC Release, standalone
GCC/Clang and C++17 linkage pass; unchanged CLI contracts are reused.

Paired 10,607-frame runs preserve all 77,840 observations and artifacts against
prior main. Another 19,738 instructions run natively: actual interp816 decoding
falls from 107,263 to 87,525 while bridge control steps remain 179,420. All 57
boundaries have positive native coverage in 1,764 hardware contexts using the
unmodified public kernel stop-PC API. Each executes exactly one original
control step. Actual exits, valid widths, APU words, bank-zero wrap, three clock
positions and two ROM speeds agree, including CPU/WRAM, hardware/audio, clock,
progress epochs and dynamic cache. Legacy NMI phases are unchanged.

No canonical function, ordinary binding or hook is added. Totals remain 1,818
verified / 1,786 ordinary bindings / 32 verified unbound / 791 WRAM fields /
499 C units / zero drafts; 193 hooks remain registered. Unrelated full regression,
other platforms, manual gameplay and production-speed measurements are deferred.
Tests/tools/ROM stay local; concurrent owner work is outside this checkpoint's
pinned compiled snapshot.

# Live NMI callback frames - 2026-10-19

The original runtime initializer writes two RAM JSL/RTS templates at
$00:0067 and $00:006C. New optional nmi_calls.h phases read each live target,
preserve its complete bank and push the original long-call frame. Their
returns consume the live short-call frame with original address wrap.
These are dynamic RAM continuations under the verified NMI owner, not new
canonical ROM functions. Existing semantic bodies remain unchanged.

The consumer extends the existing opcode-slot selector at four RAM boundaries.
Original generic call/return bookkeeping and clock/bus accounting remain
unchanged. Unsupported opcodes and contexts retain interpretation. Previous
headers still compile without new symbol dependencies; fifteen CMake contexts
preserve prior properties and enable the extension only with both new APIs.

Original-ROM execution of the 24-instruction runtime initializer supplies the
template opcodes. Shared interp816 comparisons pass 262,144 direct and 262,144
actual-producer cases across all target banks and stack positions, including
overlap and wrap. Another 263,228 producer and 132,088 API guards pass. The
previous six boundaries pass 393,216 direct and 393,216 producer cases in each
of two feature configurations. Two effective fault controls catch lost bank
bits and a wrong return increment. MSVC Release, standalone GCC/Clang and C++17
linkage pass; unchanged CLI executable/fixture contracts are reused.

Paired 10,607-frame runs preserve all 77,840 observations and artifacts against
prior main. Another 20,178 instructions run natively: actual interp816 decoding
falls from 127,441 to 107,263; bridge control steps remain unchanged. All four
RAM boundaries have positive hardware coverage in 528 controlled contexts.
Both callbacks repeatedly change address and bank; their actual PHK/PLA/STA/RTL
bodies confirm unmasked banks $80/$7E/$BF. Original callback bookkeeping,
hardware/audio state, clocks, progress epochs and dynamic cache remain equal.

No canonical function, ordinary binding or hook is added. Totals remain 1,818
verified / 1,786 ordinary bindings / 32 verified unbound / 791 WRAM fields /
499 C units / zero drafts; 193 hooks remain registered. Full unrelated regression,
other platforms, manual gameplay and production speed measurements are deferred.
Tests/tools/ROM remain local; concurrent owner work is separate from this
checkpoint's pinned compiled snapshot.

# NMI call and return boundaries - 2026-10-19

Optional nmi_calls.h exposes five original NMI child-call frames and the
terminal native-mode interrupt return. Existing whole NMI bodies are unchanged.
The consumer selects these phases in the original opcode execution slot,
keeping all original call/return bookkeeping, stop rules, progress counters,
bus timing and coprocessor accounting. No call is skipped by a PC redirect.
Unsupported ownership, CPU modes and stack contexts retain interpretation.
The RTI phase permits the original cleared NMI latch and stack byte at $2000.

Shared checks pass 393,216 direct and 393,216 actual-producer ROM states,
394,846 producer guards and 67,061 API no-access guards. CPU, full WRAM and
ordered bus agree across all six boundaries, status/width combinations and
stack edges. Sixteen effective in-memory ROM faults exercise the unchanged
semantic phase bodies. Thirteen CMake contexts preserve prior properties and
older APIs. MSVC Release, standalone GCC/Clang and C++17 linkage pass; unchanged
CLI executables and fixtures reuse accepted checks. The final source cleanup
only expands switch formatting without changing tokens.

Paired 10,607-frame runs match all 77,840 observations and artifacts against
prior main. Actual interp816 decoding falls from 179,420 to 127,441 instructions:
51,979 additional phases run natively. Original bridge control steps remain
179,420, preserving scheduler bookkeeping. Another 528 hardware contexts cover
432 event/restore paths and 96 complete NMI call compositions, including all
eight request combinations, four saved status modes, three clock positions
and real RAM JSL/RTL/RTS callbacks. All six boundaries have positive native hits.

An initial positive-count assertion exposed the already-cleared NMI latch;
that run was rejected. Only corrected, positively exercised integration is
accepted. Unrelated full regression, other platforms, manual gameplay and
production speed measurements remain deferred. This is no whole-game claim.
No canonical function or ordinary binding is added. Totals remain 1,818 verified,
1,786 ordinary bindings, 32 verified unbound, 791 WRAM fields, 499 C units and
zero drafts. The 193 hooks are unchanged. Tests/tools/ROM remain local;
concurrent owner changes are outside the pinned compiled snapshot.

# NMI restore-width continuation - 2026-10-19

The verified NMI owner now exposes the complete status-width transition at
$80:8699 through optional nmi_finish.h. Original whole NMI bodies are unchanged.
The consumer preserves play-time subscribers before executing the transition,
including callback order, nested dispatch and pending redirects. Native use
requires the concrete interrupt-owned opcode; task and auto-quiescent contexts
retain interpretation. Original RTI and call bookkeeping remain in place.

The reviewed generated hook registry now has 256 slots. Its 192 prior entries
remain intact; the missing tick forwarder occupies entry 193. Installation
preserves existing foreign hooks. The original SDK source is unchanged.
An initial equal replay with zero new native hits was rejected, exposing the
full registry. Only the corrected, positively exercised integration is accepted.

Shared checks pass 65,536 direct original-ROM states, 763 API guards, 17,408
retained ROM states, 1,664 legacy and 13,312 owned callback compositions,
1,280 interpreted-RTI cases, 835 producer guards and 512 positive cases.
Fourteen event scenarios, two effective ROM faults, twelve CMake contexts,
registry capacity/mirroring/overflow and feature-off compatibility pass.
Windows Release, standalone GCC/Clang and C++17 linkage pass; unchanged CLI
executables and fixtures reuse their accepted contract checks.

Paired 10,607-frame runs match all 77,840 observations and artifacts against
the prior main. Exactly 10,566 additional native transitions reduce interpreted
instructions from 189,986 to 179,420 in this workload. Another 432 real interrupt
contexts compare status, clock rollover, callback mutations and pending
redirects through the original RTI, including CPU/WRAM, hardware, audio,
clock/refresh, progress epochs and dynamic cache. Full unrelated regression,
other platforms, manual gameplay and production speed measurements are deferred.

No new canonical function or ordinary binding is added. Totals remain 1,818
verified / 1,786 ordinary bindings / 32 verified unbound / 791 WRAM fields /
499 C units / zero drafts. Tests, tools and ROM remain local. Concurrent owner
changes are outside this checkpoint's pinned compiled snapshot.

# Menu cursor poll preparation - 2026-10-19

The existing verified menu input owner exposes its complete cursor-preparation
phase at $82:8B0C..8B1B. It converts the selected slot into a table index and
retains the persistent cursor flag. All nine original CPU boundaries preserve
registers, flags, widths, bank carry, ordered bus and checkpoints. The existing
readable parent remains unchanged. Execution stops before the original JSL at
$82:8B1C; call/return and wait ownership stay original.

The producer runs the whole phase only with its complete conservative timing
window and validated WRAM/index context. The optional menu_input_phase.h API
also exposes literal semantics for general data banks and cursor values.
Capability requires that API and its verified owner. One extra hook reaches
the current table capacity of 192/192; further hooks require deliberate work.

Shared verification passes 524,288 original-ROM/actual-producer cases: all
256 valid indices and 256 flag bytes, status variations, four timing windows
and both ROM speeds. Another 65,536 direct semantic states vary all 256 data
banks, cursor words, DP and stack. The producer passes 66,216 pre-access guard
cases and 4,644 positive contexts; the API passes 765 no-access mode guards.
Nine effective in-memory ROM faults, twelve CMake contexts and two actual
producer feature configurations pass. Existing CMake properties are unchanged.
Windows Release, GCC/Clang and C++17 linkage pass; unchanged CLI executable
and fixture hashes reuse accepted checks.

Paired 10,607-frame runs match 77,840 observations and all artifacts. Exactly
1,163 native groups replace 10,467 interpreted instructions: 200,453 becomes
189,986 in this workload. Another 144 actual kernel/hardware contexts compare
four index classes around the page boundary, status, three WRAM banks and
three clock positions. They stop before the JSL and compare CPU/WRAM/PPU,
APU/SPC/FIFO/clocks, progress epochs and the dynamic-value cache. This is not
whole-game or production-speedup proof. Unrelated full regression, other
platforms and manual gameplay remain deferred. Totals remain 1,818 verified /
1,786 ordinary bindings / 32 verified unbound / 791 WRAM fields / 499 C units /
zero drafts. Tests/tools/ROM stay local. Concurrent owner work is separate
from the accepted compiled snapshot.

# Frame wait register entry phases - 2026-10-19

Five verified wait owners expose twelve original PHA/PHP/SEP entry boundaries.
Accumulator and index widths, status, stack order, DP/DB, decimal mode and
ordered bus behavior stay literal. Existing readable whole wait routines and
ordinary bindings are unchanged. The optional frame_wait.h API is enabled only
with its declaration and all five verified owner contracts. Original calls,
wait polling, returns, interrupt ownership and frame-poll hook identity remain.

One shared checkpoint passes 393,216 original-ROM/actual-producer cases across
both M/X widths, status values, eight timing contexts and both ROM speeds.
Another 24,576 direct semantic states include eleven stack boundary classes.
The producer rejects 9,660 unsupported contexts without access; 1,152 positive
boundary contexts pass. The API rejects 1,057,684 unknown-PC, wrong-bank and
malformed-width contexts before access. Twelve effective in-memory ROM faults
are detected. Twenty-eight CMake contexts preserve all prior properties;
the actual producer compiles without the new feature and has no new API link
dependency. Windows Release, GCC/Clang and C++17 linkage pass. Unchanged CLI
executables and fixtures reuse accepted evidence by hashes.

Paired 10,607-frame replays match 77,840 observations and all artifacts. There
are 10,186 new native contexts: interpreted instructions fall from 210,639 to
200,453 in that workload. All twelve sites additionally pass 144 controlled
real scheduler contexts, comparing CPU, WRAM, clocks, PPU/controller state,
APU/SPC/FIFO, progress epochs and the dynamic-value cache. Hook use is 191/192.
This is not a whole-game or production-speedup claim. Full unrelated regression,
other platforms and manual gameplay remain deferred. No new canonical function
or ordinary binding is counted: 1,818 verified, 1,786 ordinary bindings, 32
verified unbound, 791 WRAM fields, 499 C units and zero drafts. Concurrent owner
edits remain separate from the pinned accepted build. Tests/tools/ROM stay local.

# Layered menu rectangles - 2026-10-19

The complete $82:83B5 rectangle clear is verified in shared readable C with
$82:83EB. The former clears both tilemaps, the latter only the back tilemap.
Both retain scratch writes, ordered cell writes, stack contents, bank restore
and caller-visible registers/flags. The contract accepts nonempty, aligned,
in-bounds rectangles in M0X0/D0/binary; other inputs hand off before access.
The existing embedded implementation in the parallel menu owner is untouched.

Thirty original CPU boundaries expose the full $82:83B5 body with exact current
consumer timing. Original call/RTS ownership remains. There is no ordinary HLE
binding: its generated entry watchdog adds a clock charge not present at the
original entry. No clock rewind, SDK clock change or wider ABI change is used.
Capability requires the optional header and verified owner metadata.

Shared checks pass 590,336 original-ROM whole-function cases, covering every
278,784 legal rectangle for each variant. The retained $82:83EB producer passes
4,096 ABI cases and 5,376 rejection guards. Timed continuation checks pass
307,200 ROM/producer cases, 24,276 rejection guards, 720 positives and 138,782
no-access API guards. Sixty-two effective ROM fault controls and eleven CMake
contexts pass. Windows Release, GCC/Clang archives and C++17 linkage pass;
unchanged CLI binaries/fixtures reuse their accepted checks.

Paired 10,607-frame replays match 77,840 observations and their artifacts. They
also match the previous accepted main in CPU/clocks/WRAM/video/audio/hardware
observations. All thirty sites execute: 16,344 native contexts, 179/192 hooks.
Interpreted instructions fall from 226,983 to 210,639 in that workload. This
is not a whole-game or production-speedup claim. Full unrelated regression,
other platforms and manual gameplay remain deferred. Totals: 1,818 verified /
1,786 ordinary bindings / 32 verified unbound / 791 WRAM fields / 499 C units /
zero drafts. Tests, tools and ROM stay private.

# Frame counter clock boundaries - 2026-10-19

All 45 existing frame-counter instruction boundaries can complete across a
scanline or scheduler deadline. Prior admission is retained; the extra route
requires coherent clocks, beam HDMA disabled and existing CPU/stack/IRQ/DMA
checks. Each callback executes one original instruction. Arbitrary direct page,
MMIO reads, word wrapping, flags, open bus, refresh and progress epochs remain
literal. The registered hook identity and separate wait-fast-forward policy
are unchanged. No new semantic API, canonical function or binding is added.

Shared verification passes 1,152,000 original-ROM/actual-producer cases,
36,225 rejection guards, 4,365 positive boundaries and 45 effective ROM fault
controls. Both M/X widths, status combinations, ten direct-page classes and
both ROM speeds are covered. Four compiled older API contexts preserve their
10/18/21/45 sites in both old and extra timing windows. Windows incremental
Release passes. Unchanged semantic/API/CLI/compiler evidence is reused; full
unrelated regression, other platforms and manual gameplay remain deferred.

The paired 10,607-frame replay matches 172,006 CPU/memory/picture/audio/hardware
observation rows and its artifacts. It compares 47,083 extra contexts, including
7,540 scanline crossings, 29 deadline crossings and 669 refresh charges.
Interpreted instructions fall from 274,066 to 226,983 in that workload; this is
not a production speedup or whole-game claim. Another 5,400 controlled hardware
cases cover all 45 sites, four M/X modes, ten direct pages and three positions.
Their terminal CPU/WRAM/PPU/controller state, APU/FIFO and actual progress epochs
and dynamic-value cache match. All observers keep the original hook identity.
Totals remain 1,817 verified / 1,786 ordinary bindings / 31 verified unbound /
791 WRAM fields / 499 C units / zero drafts. Tests, tools and ROM stay private.

# Original sound transfer continuations - 2026-10-19

The existing payload and boot-upload owners expose 58 individual CPU-facing
continuations: 25 payload instructions and 33 boot instructions. Their readable
whole-transfer phases remain unchanged. These bounded adapters preserve the
registers, flags, operand and data bus, width changes, stack, source wrapping,
RMW write order and branch continuations of one original instruction. They add
no canonical function aliases or ordinary bindings.

The consumer still tries its accepted grouped phases first. Otherwise an exact
single instruction may run with coherent clocks, beam HDMA disabled and the
existing CPU/stack/IRQ/DMA guards. Atomic wide source reads and port writes stay
atomic. New capability gating requires both APIs and the existing verified
sound owners; older child interfaces retain the previous runtime paths.

Shared proof passes 296,960 original-ROM/actual-producer cases across all 58
boundaries and five timing contexts, plus 59,392 direct semantic cases with
varying direct page. Native contracts use their original M widths, X0, D0 and
decimal disabled; source banks, pointers, counter edges and both ROM speeds
vary. CPU/full WRAM/bus/open bus/opcode costs/refresh agree. Another 47,096
runtime rejections, 1,392 positive boundaries, 321,188 semantic invalid-input
checks and 58 effective in-memory ROM opcode faults pass. Exact source checks
cover 37 producer helpers. Existing semantic bodies are byte-identical.

Isolated Windows Release, standalone GCC/Clang and C++17 archive linkage pass.
The standalone CLI executables and private fixtures are hash-identical, so
prior CLI acceptance is reused. Sixty CMake contexts and three compiled older-
API contexts pass; gated-off producer objects have no new symbol dependencies.

Paired cold and input-driven replays match across 1,500 and 10,607 frames. The
longer replay compares 237,550 additional upload contexts immediately before
and after execution, including 23,050 scanline crossings, 77 deadline crossings
and 3,724 refresh charges. All 846,756 observation rows and memory/video/audio/
APU/SPC/DSP/clock/FIFO/port artifacts match. LLE instructions fall from 511,616
to 274,066. A separate controlled hardware probe covers every boundary at three
positions with one clock of budget: 174 cases, including stack and wide-port
operations. These kernel contexts do not establish whole-child/gameplay proof.
No production-speedup or whole-game claim. Full unrelated regression, other
platforms and manual gameplay remain deferred. Totals stay 1,817 verified /
1,786 ordinary bindings / 31 verified unbound / 791 WRAM fields / 499 C units /
zero drafts. Tests, tools and ROM remain private.

# Single sound reads at clock boundaries - 2026-10-19

Fifteen existing scalar sound polls can now complete one original CMP/LDA
across a scanline or scheduler deadline. They share the accepted one-opcode
admission guard with sound branches. Original guarded admissions and grouped
poll/transfer/frame windows remain. The additional path requires beam HDMA
disabled, coherent clocks and existing CPU/stack/IRQ/DMA restrictions. Original
operand fetches, port order, atomic wide reads, flags, progress epochs, refresh
charges and APU synchronization remain unchanged. No semantic API or new
canonical entry/binding is added.

The shared affected checkpoint passes 524,288 original-ROM/producer cases
across 15 reads, 17 branches and eight timing contexts, including existing
grouped contexts. CPU, WRAM, full bus, open bus and per-opcode costs agree.
Another 25,775 rejection guards, 1,208 positive boundaries and 15 effective
in-memory ROM opcode fault controls pass. Exact source checks cover 36 helpers;
unrelated actor code and semantic source/API/metadata/timing helpers are
unchanged. Isolated Windows Release passes. Unaffected standalone/CLI evidence
is reused by checked inputs; no new full unrelated regression is claimed.

Paired cold and normal input-driven replays match over 1,500 and 10,607 frames.
The normal replay additionally compares 83,235 read contexts immediately before
and after execution, including 33,462 scanline crossings, 113 deadline crossings
and 1,801 refresh charges. All 371,656 observation rows and memory/video/audio/
APU/SPC/DSP/clock/port artifacts agree. LLE instructions fall 594,851 -> 511,616.
A separate controlled hardware probe covers every read at three positions with
one clock of budget: 45 exact one-opcode cases, including the atomic 16-bit IPL
read. Final CPU, full WRAM, progress epochs, SPC/FIFO/port timing and clocks
agree. These are controlled kernel contexts, not complete child/gameplay proof.
No whole-game or production-speedup claim. Other platforms and manual gameplay
remain deferred. Counts stay 1,817 verified / 1,786 ordinary bindings /
31 verified unbound / 791 WRAM fields / 499 C units / zero drafts.
Tests, tooling and ROM remain private.

# Single sound branches at clock boundaries - 2026-10-19

Seventeen existing verified sound branches can now execute natively when
only one clock of scheduler budget remains or the instruction crosses a
scanline. Each callback completes exactly one original branch. Its two ROM
fetches, CPU state, bus/open-bus behavior and clocks remain literal. The
additional admission requires coherent CPU/coprocessor/beam/refresh state,
beam HDMA disabled, positive deadline room and the existing CPU/stack/IRQ/DMA
restrictions. Prior admission is preserved. Grouped sound and frame waits,
transfer windows, semantic C and public APIs are unchanged.

The affected checkpoint passes 278,528 original-ROM/producer states across
all 17 branches and eight timing contexts. CPU, full WRAM, fetch bus, open bus,
opcode costs and refresh charging agree. Another 13,685 rejection guards,
833 positive boundaries and 17 effective ROM displacement controls pass.
Exact source checks cover 36 producer helpers; unrelated actor code and
semantic sources/APIs/metadata are unchanged. The isolated Windows build
passes; unchanged standalone GCC/Clang/C++/CLI evidence is reused by checked
inputs. No repeated full unrelated regression is claimed.

Current and accepted-baseline producers match over 1,500 cold and 10,607
input-driven replay frames, including full per-frame memory, video, audio,
APU/SPC/DSP, beam, clocks and port timestamps. In the longer replay, 63,673
previously interpreted branch sites also match CPU and hardware-clock state
immediately before and after execution: 19,882 scanline crossings, 67 budget
crossings and 836 refresh charges are included. Per-edge observations include
SPC state and queued-port timing; complete memory/audio/video observations
are per-frame. LLE instructions fall from 658,524 to 594,851 in this replay.
This is not a whole-game or production-performance claim. Other platforms,
manual gameplay and full unrelated regression remain deferred.

Counts remain 1,817 verified / 1,786 ordinary bindings / 31 verified unbound /
791 WRAM fields / 499 C units / zero drafts. No new canonical entries or
ordinary bindings are added. Tests, tooling and ROM remain private.

# Additional original frame waits - 2026-10-19

Three complete waits are verified: $80:9378 preserves status, $84:A5A8
preserves accumulator and status, and $8E:B288 waits without saving registers.
The first two expose dynamic exit widths because they restore live status.
Their original stack effects, counter reads, timeout continuations and RTS
boundaries remain. They have no ordinary busy-wait HLE bindings.

Eight additional literal counter-loop sites now expose 24 individual native
LDA/CMP/BEQ phases. All 15 detected copies of that original loop can execute
with a scheduler boundary after every opcode. The existing counter and poll
APIs keep their accepted entries. A new capability requires the extended API
and seven verified owner contracts. No wait fast-forward was added.

The shared checkpoint passes 92,160 original-ROM/producer opcode states and
37,636 complete-wait cases, including 12,528 new-root cases. CPU, full WRAM,
ordered bus, open bus and opcode costs agree. Another 537,502 rejection
checks, 360 positive edges and 15 effective ROM fault controls pass. Isolated
Windows Release, standalone GCC/Clang, C++ linkage, 35 CMake contexts and four
compiled capability contexts pass. CLI binaries are unchanged; prior CLI
proof is reused by hash. Unaffected semantic and producer code is unchanged.

Normal current/baseline replays match over 10,607 frames, including CPU,
WRAM pages, PPU/pixels, PCM, APU/SPC/DSP, beam, clocks and port timestamps.
Those scenes do not visit the new sites; their LLE count stays at 658,524.
A separate real-NMI/APU/PPU replay covers all 15 wait loops in both accumulator
widths: 30 controlled contexts and 90 frames after a cold baseline. Every new
phase executes, including both battle waits. CPU and hardware observations
match at each original loop tail and each frame. A common test redirect stops
before the following original instructions; this is not complete battle-scene
or hardware epilogue coverage. Whole-function ROM checks cover the epilogues.
Full unrelated regression, other platforms and manual gameplay are deferred.

Totals: 1,817 verified / 1,786 ordinary bindings / 31 verified unbound /
791 WRAM fields / 499 C units / zero drafts. Tests and tools stay local.

# Text frame wait and explicit opcode properties - 2026-10-19

The complete $80:BF0B text frame wait is verified. Existing scene, window
and recovery callers retain their original child boundary. The consumer now
runs its LDA, CMP and BEQ separately, preserving the original scheduler and
quiescence checks. Its ordinary HLE binding stays absent. Frame-hook records
carry an explicit branch property, replacing array-position assumptions.

The combined seven-family checkpoint passes 43,008 original-ROM/producer
opcode states and 25,108 whole-wait states, including 4,176 new text cases.
CPU, full WRAM, ordered bus, open bus and opcode timing agree. Another
430,545 rejection checks, 168 positive edges and six effective ROM fault
controls pass. Isolated Windows Release, standalone GCC/Clang, C++ linkage,
13 CMake contexts and three compiled feature contexts pass. CLI executables
are unchanged; prior CLI evidence is reused by checked hashes.

Current and accepted-baseline producers match over 10,607 input-driven
startup, intro, field, world and menu frames, including CPU, WRAM pages,
PPU/pixels, PCM, APU/SPC/DSP, beam, clocks and ordered port timestamps.
The new text hooks execute 2 loads, 10,639 comparisons and 10,641 branches.
Interpreter instructions fall from 679,806 to 658,524 in this replay. This
is neither whole-game equivalence nor a production benchmark. Full unrelated
regression, other scenes/platforms and manual gameplay remain deferred.

Totals: 1,814 verified / 1,786 ordinary bindings / 28 verified unbound /
791 WRAM fields / 499 C units / zero drafts. Tests and tools stay local.

# Frame counter loads and Cave wait - 2026-10-19

The complete $84:8D4D Ancient-Cave frame wait is verified. Its bounded
continuation preserves the original comparison loop and RTS boundary. It has
no ordinary HLE binding: the consumer executes each original load, comparison
and branch separately through scheduler-aware hooks. Six frame-wait families
now include their initial counter loads; eight previously interpreted opcode
boundaries are added. The prior poll API keeps its existing entry contract.

The shared checkpoint passes 36,864 original-ROM/producer opcode states and
20,932 complete-wait states, including 4,176 Cave cases. CPU, WRAM, ordered
bus, open bus and per-opcode timing agree. Another 425,211 rejection checks,
144 positive edges and eight effective ROM fault controls pass. GCC, Clang,
isolated Windows Release, C++ linkage and nine CMake admission contexts pass.
The CLI executables are unchanged; prior CLI evidence is reused by hash.

Current and accepted-baseline producers match across 10,607 input-driven
frames, including startup, intro, field, world and menu transitions: CPU,
WRAM pages, PPU/pixels, PCM, APU/SPC/DSP, beam, clocks and ordered port times.
Interpreter instructions decrease from 688,844 to 679,806 in this replay.
Cave and item-region wait hooks are covered by synthetic ROM/producer cases,
not by scene coverage in this replay. No whole-game or production performance
claim is made. Full unrelated regression and manual gameplay are deferred.

Totals: 1,813 verified / 1,786 ordinary bindings / 27 verified unbound /
791 WRAM fields / 499 C units / zero drafts. Tests and proof tools stay local.

# Sound poll edge steps — 2026-10-19

Fifteen original sound polls now execute one native LDA/CMP when a complete
bounded poll iteration cannot fit. Seventeen existing native branches use
their individual time bounds and stack-neutral range through $1FFF. The
wide IPL comparison retains the original atomic word interface. Every opcode,
flag, read, branch, elapsed clock and scheduler boundary is preserved.

Eleven transfer windows now weight known ROM fetches at the current region
speed, retaining 12 clocks for other cycles and 40 for a possible refresh.
The phase cannot change MEMSEL; every scheduled opcode contributes a positive
upper bound. Original CPU/DP/stack/IRQ/DMA and refresh-debt restrictions stay.
Each admitted step/phase finishes before scanline, visible HDMA and deadline.
No original semantic function or opcode schedule changes.

The shared original-ROM/actual-producer checkpoint passes 196,608 grouped/
individual poll and branch states plus 56,320 transfer states. CPU, full WRAM,
fetch/data bus, open bus, per-opcode CPU and master costs agree, including the
literal refresh accountant. Guards pass 28,221 edge and 11,816 transfer cases,
with 576 and 198 positive boundaries. Twenty effective ROM fault controls are
detected. Exact producer source, constants, tables, structs and phase bounds
match; unrelated actor code stays unchanged. The isolated Windows build
passes. Unchanged standalone, CLI, C++/feature and word-component evidence is
reused by checked hashes; no repeated unchanged full regression is claimed.

Current/accepted-baseline replays match for 1,500 cold and 10,607 input/world/
menu frames: CPU, WRAM pages, PPU/pixels, PCM, APU/SPC/DSP, beam, open bus,
clocks and ordered port values/timestamps. An additional 1,500-frame actual
APU variant forces the otherwise unobserved IPL scalar path and also matches.
Interpreter instructions fall from 879,466 to 688,844 (-190,622) in the
controlled longer run. Instrumented CPU times were 45.562/45.062s, wall times
46.44/45.59s; this does not establish production or whole-game performance.

Totals remain 1,812 verified / 1,786 ordinary bindings / 26 verified unbound /
791 WRAM fields / 499 C units / zero drafts. No duplicate semantic entries or
ordinary bindings are added. Tests/tools stay local, protected files and ROM
are intact, owner build/windows is untouched and nothing is pushed. Other
scenes/platforms, manual gameplay and full unrelated regression are deferred.

# Sound transfer timing windows — 2026-10-19

Eleven existing verified sound phases now use separate consumer timing
windows. Their original C operations and opcode schedules are unchanged.
Conservative upper bounds range from 8 to 59 CPU cycles: at most 12 master
clocks per cycle plus one 40-clock DRAM refresh. A valid refresh remainder
and current (or initially zero) charge watermark prevent deferred debt.
Each phase finishes before the scanline edge, visible beam-HDMA edge and
active deadline. Prior CPU width, DP0, decimal, stack, IRQ and DMA guards
remain. No original instruction, word transaction or wait is skipped.

The shared checkpoint passes 56,320 original-ROM/actual-producer states
across all eleven phases: original admission, latest safe position/exact
room, worst refresh remainder, visible HDMA and vblank contexts. CPU,
WRAM, full fetch/data bus, open bus and every opcode's entry state and cost
match. The literal refresh accountant also matches an independent final
clock formula. Another 12,412 pre-access guards and 198 positive edges
cover malformed CPU/device/refresh state, debt, overflow and boundaries.
Twelve effective in-memory ROM mutations are detected. Exact producer
bodies, constants, structs and tables match; unrelated actor tokens are
unchanged. The isolated Windows build passes. Unchanged semantic source,
public APIs, GCC/Clang/CLI, feature/C++ and word-interface component proof
is reused by checked source and executable hashes, without another run.

Current/accepted-baseline replays match for 1,500 boot and 10,607 input,
world and menu frames, including CPU, WRAM pages, PPU/pixels, PCM, APU/SPC/
DSP, beam, open bus, clocks and ordered port values/timestamps. Interpreter
instructions fall from 1,160,967 to 879,466 (-281,501). Instrumented full
CPU times were 46.484/45.625s, wall times 47.48/46.61s. These bounded
observations do not establish whole-game or production performance.

No new semantic functions or ordinary bindings are counted. Totals stay
1,812 verified / 1,786 ordinary bindings / 26 verified unbound / 791 WRAM
fields / 499 C units / zero drafts. Full unrelated regression, other scenes/
platforms and manual gameplay are deferred. Tests/tools stay local; ROM,
owner files and build/windows are preserved. No push.

# Native frame poll phases — 2026-10-19

Five original frame-counter waits now expose their CMP $40 and BEQ phases
through `Lufia2FrameWaitPollStep`. Both opcodes execute natively, with the
original scheduler retaining its check between them. These phases belong
to verified parents at 82:93C2, 83:900C, 8E:B550, 86:8B48 and 86:974F;
no duplicate whole functions or additional ordinary bindings are counted.
Existing parent operation streams remain unchanged.

A shared timed bus handles sound transfers and frame comparisons. Wide
Direct-Page reads wrap within bank 0 and delegate to the original word bus;
SPC source words retain their bank-crossing behavior. Stack-neutral polls
accept safe WRAM stacks through $1FFF, including the observed $1FFA sound
initializer context. Stack-mutating return/transfer helpers retain their
prior limits. Every original read, flag, instruction cost and wait remains.

The initial paired frame adapter passed isolated ROM cases but changed the
scheduler's quiescence checkpoints and disabled an existing consumer wait
patch. It failed at cold frame 376 and was replaced before acceptance.
Individual native phases retain the original checks. The existing patch
recognizes only this exact native callback pointer; foreign hooks continue
to suppress it. Semantic decomp code never fast-forwards original waits.

Final archive/actual-producer proof passes 51,200 ROM states: 30,720 sound
runs and 20,480 individual frame phases. CPU, WRAM, full fetch/data bus,
open bus, opcode-entry CPU and CPU/master charges agree. Both widths, all
DB values, varied sound DP and 16 explicit frame DP contexts cover counter
wraps, bank boundaries, mirrored MMIO, decimal state and both ROM speeds.
Affected original sound branches, boot phases and payload phases pass
28,672 additional states. Also green: 23,266 consumer guards, 25 positive
128-clock/position-1240 edges, 175 high-stack admissions, 2,880 changing
sound-run/progress cases, 346,745 portable guards, 18 effective ROM fault
controls, C++ linkage and 48 API/metadata/feature cases. Thirty producer
bodies, three structs, six tables and the separate hook-identity body match
source; prior frame parents and word-interface bodies remain unchanged.
GCC, Clang and isolated Windows Release pass. Unchanged CLI and word-bus
component evidence is reused; full unrelated regression remains deferred.

Paired current/accepted-baseline replays match across 1,500 boot and 10,607
input/world/menu frames: CPU, every WRAM page, PPU, pixels, PCM, APU/SPC/DSP,
beam, open bus, clocks and ordered port values/timestamps. Native frame
phases execute 446,453 opcodes; newly admitted sound contexts add 195,950.
Interpreter instructions fall from 1,803,370 to 1,160,967 (-642,403).
Instrumented CPU times were 73.625/71.672s, wall times 78.73/77.61s.
These observations do not establish whole-game or production performance.

Tests and tools remain local. Owner files, ROM and build/windows are intact;
nothing is pushed. Totals stay 1,812 verified / 1,786 ordinary bindings /
26 verified unbound / 791 WRAM fields / 499 C units / zero drafts.

# Bounded sound poll iterations — 2026-10-19

Fifteen existing sound poll sites now expose one complete original CMP/LDA
and BNE/BEQ iteration. Shared port selection keeps the prior single-step
service unchanged. The consumer joins at most eight iterations per hook,
retaining every original read, branch, flag, opcode cost and progress event.
The IPL word comparison uses the original atomic word interface. No wait or
guest time is skipped, and these continuations add no canonical functions.

Each pair requires a synchronized beam, safe stack, no DMA/IRQ, at least 128
master clocks before the deadline and horizontal position at most 1240.
Visible beam-HDMA lines additionally stop admission at 896. Admission is
rechecked between pairs; the prior 800/256 timing wrapper remains equivalent.
Unknown entries and unsupported CPU contexts return before bus access.

Final archive verification covers 30,720 actual-producer original-ROM states
across 15 families, both ROM speeds, all DB/DP contexts, both index widths,
decimal state and immediate/late/pending replies. CPU, complete WRAM, fetch
and data bus, open bus, per-opcode CPU starts and CPU/master costs agree.
Another 30,720 prior-step cases compare data bus, final CPU and costs; that
legacy check excludes instruction fetches and opcode-start CPU assertions.
Further checks: 2,880 changing run-boundary/progress cases,13,981 consumer
pre-access guards,76,981 portable guards,263,681 old-window equivalence
cases, eight effective in-memory ROM mutations, C++ linkage and 32 feature
selection cases. 28 producer bodies, five tables and three structs match.
GCC/Clang and isolated Windows Release pass; unchanged CLI proof is reused.
Final formatting preserves the tested semantic token stream.

Paired 1,500-frame boot and 10,607-frame input/world/menu replays match CPU,
WRAM pages, PPU, pixels, PCM, APU/SPC/DSP, beam, clocks, open bus and ordered
port values/timestamps. The longer run completes 2,534,914 native opcodes in
217,649 bounded calls; interpreted instructions fall 2,734,664 ->1,803,370.
This does not establish a production speedup: instrumented CPU times were
75. 031/77. 031 s and wall times 80. 56/84. 48 s. No expectations were weakened.
An initial private counter probe altered a word-read condition; it was fixed
before acceptance. Initial divergent logs are excluded from proof.

Full regression, unrelated scenes/platforms, manual gameplay and whole-game
performance remain deferred. Tests/tools stay local; ROM and owner builds
are preserved. Totals remain 1,812 verified / 1,786 ordinary bindings / 26
verified unbound / 791 WRAM fields / 499 C units / zero drafts.

# Sound handshakes and boot block phases — 2026-10-19

Seventeen original BNE/BEQ/BVS sound branches and six bounded SPC boot-transfer
phases now have native continuations. They belong to existing verified sound
parents; no additional whole-function entries or HLE bindings are counted.
`Lufia2SoundPollBranchTarget` shares the flag-only branch calculation between
the portable execution service and consumer, avoiding unnecessary CPU copies.
Boot continuations share block-setup, source-read and pair-output workers with
the complete original boot uploader. Its operation stream is unchanged.
Original polls, ADC retry, bank wrapping, overflow selection, temporary stack
byte, saved status and later PLP/RTS remain literal. No wait is skipped.

The boot adapter uses the existing guarded timing interface with native M1X0
(or M0X0 at 9C3B), DP0, binary arithmetic, synchronized beam, safe WRAM stack,
no active DMA/IRQ, horizontal position at most 400 and 1,024 clocks of room.
The one-instruction branch adapter retains both valid widths and live DP and
decimal state; it keeps the existing 256-clock/position-800 envelope.
Original instruction fetches, data operations, per-opcode CPU states and
cycle charges are preserved. Word loads/stores delegate to the original
consumer word bus, including its atomic APU-port synchronization. Unsupported
word claims retain the original byte fallback. Existing byte-cycle helper
bodies are unchanged. Feature selection requires the new APIs and all six
verified sound parents; older public APIs retain their previous selection.

The final standalone archive passes 28,672 original-ROM cases through actual
producer bodies: 17,408 branch cases, 6,144 boot phase cases and 5,120 prior
payload phase regressions. The same number of additional DP-context runs
includes ordinary admitted branch cases and boot/payload semantic-service
cases using the same bus/timing helpers with admission deliberately bypassed;
56 DP0 executions overlap. These do not broaden runtime admission. CPU,
full WRAM, complete fetch/data bus, open bus, each opcode's entry CPU state
and CPU/master costs agree. All DB values, branch widths/flags, source wraps,
counter edges and ROM speeds are exercised. Eight effective in-memory ROM
mutations are detected; changed branch paths may hit the nonarrival limit.
The initial proposed unchanged displacement was excluded from fault evidence.

The full boot parent also matches its accepted baseline in 512 synthetic
immediate-reply states covering all entry widths, mirrored DB and source-bank
crossings. Consumer pre-access guards pass 35,489 states, portable guards
148,616. Actual word-interface code passes 1,536 mock-backend delegation cases
for atomic claims, byte fallback, address/order/open bus and epoch progress.
Those are component evidence; real APU behavior is checked separately below.
Twenty-four producer helper bodies and five tables match fixture tokens;
prior frame-cycle helpers retain their original tokens. C++ API linkage and
31 API/metadata/feature cases pass. GCC/Clang and isolated Windows Release
builds pass. Unchanged CLI hashes reuse prior accepted CLI evidence.

Current-production on/off replays match for 1,500 boot and 10,607 input plus
controlled-world/menu frames: CPU, every WRAM page, PPU/VRAM/OAM/pixels, PCM,
APU/SPC/DSP, beam/open bus, clocks and ordered port timestamps. Native counters
confirm 817,399 branch hits, 9,888 boot phases, two word reads and 4,727 word
writes in the longer run. Interpreter instructions fall from 3,600,345 to
2,734,664. This does not establish a production speedup: the final instrumented
pair used 82.188/82.672 CPU seconds and 88.20/91.50 wall seconds respectively.
The earlier version's extra branch-state copies were removed and affected
checks repeated. Unchanged boot-parent and word-component evidence was reused.

Full regression, unrelated scenes/platforms and manual gameplay acceptance
were deferred. Historical field-text audio timing questions remain outside
this bounded replay. Private tests/probes stay local, and no ROM or owner
build output is added. Totals remain 1,812 verified, 1,786 ordinary bindings,
26 verified without an ordinary binding, 791 WRAM fields, 499 C units and zero
drafts. These continuation counts are not additional complete game functions.

# Sound payload transfer phases — 2026-10-19

`Lufia2SoundUploadStep` exposes five exact transfer continuations within the
already verified 80:9945 payload owner: 9969, 997D, 9988, 998B and 999D.
They share the original pair-output, read-ahead and sequence-counter workers.
The original complete payload, resource-header and queued-chunk entry points
retain their operation streams and emit no additional checkpoints. Poll limits
and original quirks, including CPX-before-DEX carry and counter underflow, stay.

The bounded service preserves live DB/DP, accumulator high byte, flags and
existing guest frames in M1X0/binary/native PB80 context. It stops before the
original reply CMP, source-bank-wrap JSR or final wait JSR. Wrap paths retain
their actual high-byte/pre-read phases; they never restart the transfer.
Checkpoints identify each original instruction without moving data accesses.
The consumer executes original fetches, retains every ordered data access and
charges each opcode separately, including branch, page and internal cycles.
It uses the existing APU/cache/progress/scheduler interface. Admission additionally
requires DP0, synchronized beam, no active DMA/IRQ, safe WRAM stack and at least
512 clocks before the existing deadline. Unsupported contexts remain original.
The feature requires the new API and the six existing verified sound parents.
No extra whole-function metadata entries or independent HLE bindings are added.

The shared checkpoint passes 5,120 original-ROM cases through the actual
production adapter and another 5,120 portable cases spanning nonzero DP through its
same timing/bus helpers with the adapter admission guard deliberately bypassed.
The second group is semantic-service evidence, not broader runtime admission.
Both compare CPU/full WRAM, complete instruction-fetch/data bus, open bus and
CPU/master charges for each executed instruction. All DB values, source wraps,
length-counter edges and fast/slow ROM are covered. The fresh standalone archive
passes both groups. Five changed-ROM negative controls are detected, including
a CPX threshold whose altered path cannot reach the native boundary within the
fixture limit. The 576 accepted-baseline comparisons of all three complete
parents preserve CPU, WRAM, bus order, child frames and checkpoint counts;
they use explicit synthetic immediate device replies. Consumer pre-access
checks cover 6,591 states and five 512-clock admission edges; portable guards
cover 69,361 states. Twenty-two producer helper bodies and two tables match
fixture tokens. C++ linkage and 28 build-feature/metadata cases pass.

Current production phase hooks on/off replays match across 1,500 boot frames
and 10,607 input/controlled-world/menu frames: CPU, all WRAM pages, PPU/VRAM/OAM,
pixels, PCM, APU/SPC/DSP, beam/open bus, clocks and ordered port timestamps.
The longer replay executes 3,600,345 interpreter instructions versus 4,395,570
without these phase hooks, 795,225 fewer. Existing native poll hooks are active
on both sides. This comparison is bounded workload evidence; it does not prove
whole-game equivalence or a wall-clock speedup. Historical unrelated field-text
audio timing questions remain open. Full regression, unrelated scenes/platforms
and manual gameplay acceptance were deferred. Unchanged shared scheduler code
retains its prior component evidence; the current replay checks its new use.

Standalone GCC/Clang and isolated Windows Release builds pass as one checkpoint.
Unchanged CLI executable hashes reuse their accepted CLI evidence. Private
fixtures and tests remain local. No ROM, extracted assets or owner build output
are added. Totals remain 1,812 verified, 1,786 ordinary bindings, 26 verified
without an ordinary binding, 791 WRAM fields, 499 C units and zero drafts.

# Timed sound polling — 2026-10-19

The six existing verified sound parents now expose a bounded sound-port poll
service. `Lufia2SoundPollStep` executes one original CMP or LDA at fifteen
identified wait sites. The original following branch, child calls, returns,
interrupt sampling and wait duration retain their interpreter owner. This is
an instruction boundary service, not fifteen additional functions or a whole
sound-loop replacement. Existing parent sources and poll limits are unchanged.

The portable service retains live DB/DP, decimal state and both index widths;
80:9C18 uses a 16-bit accumulator, the other sites use 8-bit. The consumer
requires native mode, a safe WRAM stack, synchronized beam, inactive DMA/IRQ
and at least 256 clocks to the next deadline. Original data reads use the
existing bus and APU progress handling. Unsupported contexts stay original.
Activation requires the new API and all six verified parent contracts.

Original-ROM verification covers 15,360 actual producer cases and the same
15,360 cases continued through their real following branch. CPU, full WRAM,
ordered data bus, open bus and per-instruction CPU/master charges match.
A fresh standalone archive passes both sets; five changed-ROM negative
controls are detected. Admission checks cover 1,966,021 consumer states and
76,981 portable states. The shared timing-guard extraction agrees with its
previous implementation in 590,121 states. All seventeen fixture helper
bodies match the production tokens. A further 512 component states check
mixed read caches, wide reads, progress, scheduler and pending APU debt using
the actual cycle include and a mock flush recorder. C++ API linkage passes.

The paired current-production replay compares only the new poll hooks on/off.
Both sides share every other native feature and the same observed core.
The 1,500-frame boot and 10,607-frame input/controlled-world/menu replay match
per-frame CPU, WRAM pages, PPU/VRAM/OAM/pixels, PCM, APU/SPC/DSP, beam/open bus,
CPU/master clocks and ordered port values/timestamps. Word and byte port
reads are observed consistently, including the first pre-flush state and
each returned byte. Controlled world/script entry cases are not unrestricted
gameplay coverage. The longer run executes 4,395,570 interpreter instructions
versus 5,212,855 without these hooks; the boot run removes 307,152. These are
workload instruction counts, not a whole-game completion or speed estimate.

Standalone GCC/Clang and isolated Windows Release builds pass together.
Unchanged CLI executable hashes retain their prior accepted CLI evidence.
The 27 feature/metadata compatibility cases pass. Full regression, unrelated
scenes, other platforms and manual gameplay acceptance were deferred.
Historical field-text audio timing questions remain outside this comparison.
Private tests and probes remain local; no ROM or extracted assets are added.
Totals remain 1,812 verified, 1,786 ordinary bindings, 26 verified without an
ordinary binding and 791 WRAM fields. There are 499 C units and zero drafts.

# Text scene actor and window setup — 2026-10-19

Three complete contracts are verified: 80:BC98 places the next script actor,
80:BC3D handles conditional placement (opcode 68), and 80:BCBC selects window
mode (opcode 69). BC98 has an ordinary binding. The two commands execute
under the existing 80:9CB8 text engine and have no separate dispatch binding.
Existing operand, actor-offset, actor-attribute and actor-cell children retain
their owners. Compatibility entry points without child services are unchanged.

M1X0/PB80/DP0/binary, stack1F10..1FFC and live script DB are admitted.
Original actor-slot increment, skipped operands, 32-byte MVN palette copy,
DB/Y saves, DP73 upload request, frame wait and register restoration remain.
The palette is read through the ROM bus; no extracted asset is introduced.
Unknown children, changed widths/DP/decimal and nested unwinds retain their
actual guest frames and original continuation points. No wait is skipped.

Combined semantic/consumer fixtures pass 2,560 normal original-ROM states
per harness: 1,536 direct contracts and 1,024 engine callers. The consumer
fixture uses the actual BC98 and existing 9CB8 bridge bodies for 1,536 of
those states; unbound command entry tests call semantic C directly. A further
384 actual RTS/RTL chains, 64 direct native-placement compositions, 20 direct/
engine nested unwinds, 40 live-child changes and 768 live-data-bank states
pass per harness. The latter cover all 256 DB values for all three entries;
BC98 adds 256 actual producer cases. Total counted positive executions are
3,836 per harness. 60 post-child cases cover ten physical sites through the
direct entries and engine. Repeated composition-mode controls and command
return-mode controls are excluded from these positive totals. The engine's
already-counted native placement compositions have no additional count.

Real original C0B7 operand-reader and BF0B frame-wait instructions execute
inside the fixtures. Other already-verified child implementations remain
explicit services. Native composition uses stack>=1F20 for its nested child;
ordinary entry cases retain1F10 and1FFC admission edges. 393,291 semantic
and131,352 actual-BC98-ABI pre-access guards and four effective heap-ROM
instruction controls per harness pass. A no-effect BCAB byte change is not
an accepted control. The nested dispatcher fixture was corrected to preserve
a deeper live PB on unwind, matching current cpu_dispatch_call_pc_pushed;
production dispatch and shared bridge helpers were unchanged.

Fresh final archive: 1,260 counted positive executions per harness including
all-DB cases, 60 post-child cases, three C++ API symbols and exact eleven-
helper/two-wrapper production closure pass. Strict GCC, Clang and isolated
Windows Release builds pass. The CLI executable and private-fixture hashes
are unchanged, so the prior accepted CLI proof is reused. The owner Windows
build directory was untouched. The actual production-object1,500-frame
cold/splash/intro/title check keeps identical picture/PCM/WRAM/CPU digests
and2,031,721 interpreted instructions. No new cold/title reduction is claimed
for this scene-command batch or extrapolated to whole-game runtime.

One combined final-source checkpoint covers these dependent changes. Full
regression, unrelated scenes, manual acceptance, physical timing and a whole-
game profile are deferred. Historical field-text PCM timing remains open.
Tests/tools stay private; original ROM and protected owner work are intact.
Totals:1,812 verified contracts,1,786 ordinary bindings,26 verified-unbound,
791 WRAM locations,498 semantic C units and zero drafts.

# Select-screen resumptions — 2026-10-19

Four complete continuation contracts are verified and bound: 86:8333 fade,
86:8352 input countdown, 86:82CE object wait and 86:82FC object-wait repeat.
They share the existing select-screen workers, with four existing owners
86:8305/830F/8339/82C8 covered in the same checkpoint. Entry is M1X0,
PB86/DB7E/DP0, decimal clear, stack 1F00..1FFC. Input resumption begins
above its saved X word. Original flags, stack traffic, child frames,
nonlocal unwinds, live child state and the 4096-iteration budget remain.
Changed child widths, DP, decimal or bank stop at the original continuation.
No child implementation or additional canonical owner is invented.

Original-ROM differential proof covers 3,035 positive states per side:
2,048 normal, 512 actual returns, 36 child unwinds, 144 live-child returns,
36 stack/count edges, three literal budget prefixes and 256 native-owner /
frame-unwind / resumption chains without another guest frame. 180 post-child
handoffs cover 20 physical call sites and 36 selected visits. 1,050,688
pre-access guards and six effective instruction controls pass per side.
A former 512-case composition-mode switch had no matching children; those
redundant cases are excluded. This is not a native-child composition claim.
Fresh final archive: 1,115 positive states per side, 180 post-child checks,
admission guards, 540 actual redirects and eight original C++ symbols pass.

Ordinary function bindings alone do not intercept interpreter RTS landings
or branch arrivals. A separate bounded step API stops before the first
original child call or terminal RTS; it never creates a replacement frame.
Three conservative pre-op hooks execute the useful prefixes. The zero-work
82CE arrival remains under its existing interpreter/call owner. All guest
waits, child dispatch, IRQ/NMI and DMA behavior remain owned by the original
scheduler. Hooks require synchronized safe beam state, no IRQ/NMI/DMA,
valid native CPU context and at least 256 master clocks to the deadline.

3,072 original-ROM producer-prefix cases and 3,072 following real JSR/JSL/RTS
chains check full CPU/128K WRAM/ordered bus and per-opcode CPU/master clocks.
590,580 admission guards, 4,096 cache/read/write/progress/clock/scheduler
checkpoint states, three ROM instruction controls, two effective adapter
controls, nine C++ API symbols, exact 14-helper hook closure and 21 CMake
availability cases pass. The actual ordinary bridge closure is also checked.
Fast/slow ROM, saved-X zero/one/wraparound, stack edges and original return
frames are included. Existing read-only timing/cache infrastructure is reused;
new stack writes preserve the interpreter's progress epochs and flush points.

An actual production-object cold/splash/intro/title run of 1,500 frames
reduces interpreter instructions from 2,033,360 to 2,031,721: 1,639 fewer.
Per-frame aggregate picture, PCM, WRAM and CPU/resume/cycle digests match.
This is a small measured reduction for that workload, not a whole-game ratio.
GCC, Clang and isolated Windows Release builds pass. A missing stddef include
found by GCC/Clang was repaired before the final compiler checkpoint.
CLI acceptance is reused only for unchanged executable/private-fixture hashes.

This checkpoint covers the combined final source state, not each intermediate
commit independently. Full regression, unrelated scenes, manual acceptance,
physical timing and whole-game profiling are deferred. Historical field-text
PCM timing remains open. Tests/tools remain private; ROM and owner WIP intact.
Totals: 1,809 verified contracts, 1,785 ordinary bindings, 791 WRAM locations,
497 semantic C units, 24 verified-unbound entries and zero drafts.

# Pending items and record descriptions — 2026-10-19

Sixteen complete admitted contracts are verified and bound:
82:B183/B1CE/B1FE/9E07/9E1B/FD81/FEC8/FDEC/FDF6/FE44/FE98/FEC0/
FEFE/FF3A/FF99/82E2. They handle pending-item actions and targets,
item-menu closure, primary/secondary descriptions, original layout dispatch,
window masks, centered record names and window dividers. All enter M1X0
except the M0X0 divider. Four description words are catalogued.
Existing item, spell, text, cursor, layer and dispatcher owners are reused;
protected window/equipment helpers keep their original owner.

The original one-word nonlocal close and caller propagation remain literal.
FDEC reuses the existing inline-table dispatcher and its 82803D checkpoint.
Its table has two ordinary targets; all raw byte selectors retain their
original ROM reads without clamping. The divider preserves its original
pattern reset and wrapping word counter. Name centering ignores final spaces
only when calculating padding, copies the original source unchanged, and
retains unsigned padding underflow. MMIO and ordered bus accesses remain.

One shared original-ROM/full-CPU/full-128K-WRAM/ordered-data+MMIO/device
checkpoint passes 7,814 positive states per side: 4,096 normal, 960 actual
returns, 1,024 native compositions, 90 child unwinds, 360 live-child/status
returns, 1,156 original-ROM edge/dispatch cases and 128 existing B104 caller
compositions. Those caller cases include 40 nonlocal propagations per side.
360 post-child boundary cases cover 74 physical sites and 90 selected visits.
2,097,296 semantic and 2,101,376 actual-ABI pre-access guards pass. Sixteen
effective heap-ROM instruction controls per side are detected. Ten positive
heap-ROM data controls are separate from original-ROM state counts.

Fresh compiled library: 3,254 positive original-ROM states per side, 360
post-child cases, admission guards, ten positive ROM-data controls, 1,350
actual redirects, sixteen C++ symbols, exact twenty-helper/sixteen-wrapper
production closure, the existing caller bridge and generated selection pass.
Strict GCC (149.7 s), Clang (129.88 s) and isolated Windows
Release (170.52 s) builds pass. Accepted CLI proof is reused only
for unchanged executable and private-fixture hashes. This is one checkpoint
on the combined final revision; intermediate commits have no separate claim.
Full regression, unrelated scenes, pixels/manual acceptance, physical timing
and whole-game profiling are deferred. Historical field-text PCM timing
remains open. No claim of whole-game equivalence is made.

Tests/tools remain private. ROM and protected owner work are intact.
Totals: 1,805 verified contracts, 1,781 ordinary bindings, 791 WRAM locations,
497 semantic C units and zero drafts. The 24 verified-unbound entries remain.

# Special menu effects and warp selection — 2026-10-18

Nine complete admitted contracts are verified and bound: `$82:AE6F`,
`$82:AF00`, `$82:AF8E`, `$82:AEF3`, `$82:AF78`, `$82:B14B`, `$82:9449`,
`$82:B03F` and `$82:836A`. They handle special item/spell effects, confirmation,
warp destination collection/selection, scroll restoration, layer restoration
and the original item-action inline-table dispatch. All enter M1X0; the two
confirmation helpers retain dynamic exits after their final consuming child.
Existing font, cursor, sound, item/spell and dispatcher owners are reused.

Successful item application discards five original words; spell application
six. The original nonlocal RTS destination, stack bytes, flags and unwinding
remain intact. The two existing selected-item/spell callers propagate these
returns instead of continuing their ordinary menu tails. The condition-sound
loop keeps byte counting, per-iteration sounds and original wait calls.
Warp selection retains every input branch, destination word/map stores,
scroll/layer restoration, frame flags and explicit child continuations.
The collector keeps live ROM records, map flag words, high-first word RMW
writes and its original sentinel. One newly catalogued destination word is
neutral about its components. Existing temporary-buffer storage is reused.

B14B is JSR8028 plus inline data. Its complete native thunk pushes the original
frame and reuses the verified dispatcher. All raw byte selectors retain their
original table reads; there is no five-slot clamp. The existing82803D menu
checkpoint is retained and compared before STA. Hook state changes and live
ROM-table changes retain their corresponding guest behavior. Protected internal
WarpList helpers remain with their owner; no algorithm is duplicated.

One shared original-ROM/full-CPU/full-128K-WRAM/ordered-data+MMIO/device
checkpoint covers 14,381 positive states per side, including dispatch tails:
9,216 normal, 1,024 actual returns, 2,304 native compositions, 48 child unwinds,
192 live-child/status returns, 1,341 original-ROM edge/dispatch cases and 256
existing item/spell caller compositions. Those caller cases include 68 nonlocal
propagations per side. The 192 post-child checks at 43 sites and 48 selected
visits include eight terminal dynamic-return checks and 184 guarded handoffs.
1,179,729 semantic / 1,182,024 actual-ABI pre-access guards pass. Fifteen
effective heap-ROM instruction controls per side are detected. Nine positive
heap-ROM data controls separately prove live record/table reads; they are not
counted as original-ROM states. The source ROM remains byte-identical.

Edges include stack endpoints, five/six-word returns, 255 sound repetitions,
all 256 map-flag bits, raw dispatch selectors, execution-checkpoint state and
record/scratch aliasing. All nine valid actual entry admissions execute natively
with zero entry fallback. The ABI independently checks the original nonlocal
landing target. Fresh compiled library: 2,685 positive states per side, 192
post-child checks, admission guards, nine positive ROM-data controls, 720 actual
redirects, nine C++ symbols, exact twenty-helper/nine-wrapper closure, exact
existing-caller fragments and generated source selection pass.
Strict GCC (170.12 s), Clang (143.5 s) and isolated Windows
Release (241.58 s) builds pass. The accepted CLI proof is reused
only for unchanged executable/fixture hashes. Checks cover the combined final
revision; no independent intermediate-commit proof is claimed. Full regression,
unrelated scenes, pixels/manual checks, physical timing and whole-game profiling
are deferred. The historical field-text PCM timing difference remains open.

Tests/tools remain private. ROM and protected owner work are unchanged.
Totals: 1,789 verified contracts, 1,765 ordinary bindings, 787 WRAM locations,
493 semantic C units and zero drafts. The 24 verified-unbound contracts remain.

# Menu preparation and glyph buffers — 2026-10-18

Twelve complete admitted contracts are verified and bound: `$81:FAF5`,
`$82:9D55`, `$82:9DD3`, `$82:F9C9`, `$82:9984`, `$82:942F`, `$82:9073`,
`$82:90F6`, `$82:9113`, `$82:9133`, `$82:8345` and `$8E:B7E2`.
They resolve the action actor, prepare/refresh item selection, select equipment
attributes and party highlights, subtract three-byte experience totals,
build/pad/upload original glyph buffers, place glyph tile rectangles, preserve
both tile layers, and count object conditions across the original map group.
Existing font, cursor, equipment and object-condition engines retain ownership.

The text parser retains all control-byte branches, the original source-pointer
word wrap, three separate length increments, high-first word RMW writes and
low-byte-only row-padding test. It does not clip the original text stream to
the nominal width. Rectangle writes retain their two tile rows and bank carry.
The field counter retains byte overflow, original ROM tables, live child state
and PLP followed by PLX/PLY in the original swapped order. The actor's zero
mask retains its original nonreturning interpreter path through untouched entry
handoff. Zero packed text/rectangle dimensions likewise retain their original
interpreter owner; the native admitted contract requires both dimensions nonzero.
No cap, timing shortcut, rendered-text substitute or original bug fix is added.
Three WRAM locations are documented. Equipment attributes occupy five newly
catalogued bytes plus the existing battle sprite-timer byte at152A in menu phase;
no overlapping metadata allocation or duplicate function is introduced.

One shared original-ROM/full-CPU/full-128K-WRAM/ordered-data+MMIO/device
checkpoint covers 8,452 positive states per side: 3,072 normal, 1,536 actual
returns, 3,328 native compositions, 89 child unwinds, 356 live-child/status
returns and 71 valid stack/dimension/pointer/alias edges with actual returns.
The compositions include 256 existing81F34D spell-target callers with the new
native81FAF5 actor resolver, fresh-library linkage and actual returns per side.
Its twenty-helper/nine-wrapper production closure remains token-identical.
There are 356 post-child checks at 28 primary sites and 89 selected visits.
The edge proof covers dimensions255x255, long-address bank carry, text pointer
wrap, length overflow, zero/low-byte padding counts, experience scratch aliases
and valid stack endpoints. Seven exceptional admission cases per side preserve
CPU and memory untouched. All twelve valid entry admissions execute natively
with zero entry fallback. 1,572,972 semantic and 1,576,032 actual-ABI pre-access
guards pass. Fifteen effective heap-ROM instruction controls per side are caught.

Fresh compiled library: 1,924 positive states and 356 post-child checks per side,
admission guards, seven exceptional admissions, 1,335 actual redirects, twelve
C++ symbols, exact twenty-helper production closure and generated source
selection pass. Strict GCC (196.02 s), Clang (163.25 s) and
isolated Windows Release (377.98 s) builds pass. The accepted CLI
proof is reused only when executable and private fixture hashes are unchanged.
Checks cover the combined final revision; intermediate commits are not claimed
as separately tested. Full decomp-verify, unrelated scenes, manual/pixel checks,
physical timing and whole-game profiling remain deferred. The prior field-text
PCM timing difference remains open and is outside this checkpoint.

Tests, fixtures and tools remain private. Original ROM and protected owner work
are unchanged. Totals: 1,780 verified contracts, 1,756 ordinary bindings,
786 WRAM locations and 491 semantic C units. The 24 verified-unbound contracts
remain unchanged. Special item/spell nonlocal exits and warp selection are next;
no partial implementations of those routines are promoted here.

# Menu target eligibility and list cursors — 2026-10-18

Seven complete original contracts are verified and bound: `$85:D711`,
`$85:D7CE`, `$85:D82C`, `$85:D8B9`, `$85:D8A1`, `$82:9C7D` and `$82:9DF3`.
The item/spell callers now reach native target eligibility, requested stat and
status masks, and per-character availability checks. Two list-cursor owners
retain their original grid/style IDs, conditional instruction text and flags.
The existing cursor and record engines remain with their original owners.

The original eligibility root shifts A six times, counts bits in Y, then
stores shifted A as its divider. It deliberately does not substitute Y.
Zero division, the forty hardware divisions, both byte/word scaling loops,
PHX/PLX writes, 330 record-clear writes and the early carry return remain.
No host division, clamping or scheduling shortcut is introduced. Three WRAM
words are catalogued; the existing phase-dependent scratch aliases remain.
The M0X0 stat-change leaf keeps A intact during its memory ASL. Other new
entries use M1X0. Original record/cursor children retain explicit frames.

One shared ROM/full-CPU/full-WRAM/ordered-data+MMIO/device-state checkpoint
covers 8,302 positive states per side across seven new entries and two existing
item/spell callers: 4,608 ordinary, 1,152 actual returns, 2,304 native
compositions, 31 child unwinds, 124 live-child/status returns, 36 valid stack
edges, seven divider/mask witnesses and forty alias/high-bit returns. There
are 124 post-child checks at 26 primary sites and 31 selected positions.
The explicit witnesses include all four divisors, 200 hardware division
transactions, six-target traversal and early success. High-bit shifts and
live pointer/scratch overlap retain their original flags and bus ordering.
1,179,729 semantic and 1,182,024 actual-ABI admission guards pass.
Twenty-seven effective ROM instruction controls per side are detected.

Fresh compiled library: 1,374 positive states and 124 post-child checks per
side, admission guards, 465 actual redirects, nine C++ symbols, exact
20-helper production closure and generated source selection pass. Strict
GCC (198.47 s), Clang (168.67 s) and isolated Windows
Release (401.61 s) builds pass. Unchanged CLI executable/fixture
hashes permit reuse of the accepted CLI proof. These checks cover the combined
final revision. Full regression, unrelated scenes, pixels/manual acceptance,
physical divider timing and whole-game profiling remain deferred.

Early ROM analysis recovered D7BB's carry exit behind an unresolved child
boundary. It was included before validation. No partial prefix is promoted.
Tests and tooling remain private. Original ROM and protected owner work are
unchanged. Totals: 1,768 verified contracts; 1,744 ordinary bindings; 783 WRAM
locations; 485 semantic C units. The 24 verified-unbound contracts are unchanged.

# Menu item and spell application — 2026-10-18

Seven complete original contracts are verified and bound: `$81:EF93`,
`$81:EFDF`, `$81:F2E7`, `$81:F34D`, `$81:F3BC`, `$85:CD10` and `$85:CD5D`.
Item/spell application now connects to native per-member effect collection.
Target checks retain the original MP comparison, target-service carry result
and stack restoration. MP consumption retains unsigned underflow and uses the
original cost byte. Five shells restore live PHP status and have dynamic exit
widths. No frame or script behavior is replaced by a timing shortcut.

The original actor-pointer helper81FAF5, relative scripts and target checker
85D711 remain explicit child services. Their internals are not claimed here.
Original clear/save action helpers and the new effect collectors are exercised
in native composition. Effect lookup retains live data-bank reads, long index
carry, stale values for empty results and four original three-byte records.
The0A37 base marker intentionally aliases the existing0A38 preview storage.
No second record loader, interpreter or number-rendering algorithm is added.

One shared original-ROM/full-CPU/full-WRAM/ordered-data+MMIO/device-state
checkpoint covers6,496 positive states per side:3,584 ordinary,896 actual
returns,1,792 native compositions,28 child unwinds,56 live-child changes and
112 additional live-status/actual-return states and28 valid stack-edge/return
states.112 post-child checks cover
all28 primary sites. Status tests restore M0X0, M1X1 and decimal flags from
the correct live stack slots.917,567 semantic and919,352 production-ABI
pre-access guards pass.17 effective ROM instruction controls per side are
detected; one equivalent REP control was rejected during fixture preparation.

Fresh compiled library:1,080 positive states and112 post-child checks per
side, the admission guards,420 actual redirects,seven C++ symbols, exact
20-helper production closure and generated source selection pass. Strict
GCC(168.92 s),Clang(142.67 s) and isolated Windows
Release(243.77 s) builds pass. CLI executable/fixture hashes are
unchanged; the accepted CLI proof is reused. Checks cover this combined final
revision. Full regression, unrelated scenes, pixels/manual acceptance,
physical timing and whole-game profiling are deferred. No speedup is claimed.

Tests and tooling remain private. ROM and protected owner work are unchanged.
Totals:1,761 verified contracts;1,737 ordinary bindings;780 WRAM locations;
483 semantic C units.24 verified contracts remain without a binding.

# Menu effect presentation — 2026-10-18

Twelve complete contracts are verified and bound: `$82:AE64`, `$82:9588`,
`$82:9E2F`, `$81:F034`, `$82:95BF`, `$82:95FA`, `$82:970B`, `$82:975D`,
`$82:9765`, `$82:9770`, `$82:977B` and `$82:9786`. They extend the existing
native item/spell-use owners through target masks, effect reset, capacity hints,
consume-one requests, effect placement, digit animation and staged reveals.

The reveal dispatch reads the original `$82:978E` table, including bank-local
pointer wrapping. That table is not a function. Existing sprite, quantity and
number algorithms retain their owners; the protected number implementation was
not duplicated. The original frame calls remain explicit services. Three live
WRAM fields are catalogued; the phase-dependent use of `$7E:0A38` is documented.
Saved status/register words remain live, including the consume-one shell's
dynamic PLP exit widths. No animation or timing patch is introduced.

One shared ROM/full-CPU/full-WRAM/ordered-data+MMIO/device-state checkpoint
covers 9,510 original-ROM states per side: 6,144 ordinary, 1,536 actual returns,
1,664 native compositions, 39 child unwinds, 78 live-child and 49 raw-index
states. All five indirect handlers and all three digit loads are witnessed.
The 156 post-child checks cover sixteen primary call sites and 39 selected
visits; native composition also covers one existing consume-helper child site.
Another 768 states per side accept a changed ROM table byte, demonstrating
dynamic table use. Twenty-two effective instruction controls per side fail as
expected. Pre-access guards: 1,572,972 semantic and 1,576,032 production ABI.

Fresh library: 1,766 states and 156 post-child checks per side, the guards,
585 actual redirects, twelve C++ symbols, exact production-helper closure and
generated source selection pass. Strict GCC (255.25 s), Clang
(222.25 s) and isolated Windows Release (261.41 s) builds pass.
Unchanged CLI executable/fixture hashes permit reuse of the accepted CLI proof.
Checks cover the combined final state. Full regression, unrelated scenes,
pixels/manual acceptance, physical timing and whole-game profiling are deferred.

Early reconstruction found seven instructions hidden behind unresolved decoder
exits: two further digit loads, their index steps and the reveal index reload.
These were restored before acceptance. Private ABI scaffolding was extended for
JSR(abs,X); its RTL observer now derives the return bank from its own live stack.
Nested composition seeds respect existing child admission. No test or tool is
committed. Original ROM bytes and protected owner work remain intact.

Totals: 1,754 verified contracts; 1,730 ordinary bindings; 778 WRAM locations;
481 semantic C units. Twenty-four verified contracts remain without a binding.

# Menu item and spell actions — 2026-10-18

Fifteen complete contracts are reconstructed and bound: `$82:AAFF`, `$82:AB46`,
`$82:AB5B`, `$82:AB89`, `$82:AB9A`, `$82:ABB1`, `$82:B42D`, `$82:B45B`,
`$82:B564`, `$82:B58A`, `$82:B59A`, `$82:B5CF`, `$82:FBEE`, `$82:FC08`
and `$82:FC1F`. They cover list-action dispatch, selection clearing, inventory
and spell swaps, discarding, item/spell use, target markers and selected records.
The existing native list-input owner reaches the dispatcher at `$82:A770`.

Retries, rejection, cancellation, MP equality, target toggles, saved registers,
live pointers/data banks and carry exits preserve the original behavior.
Zero target counts retain the original 256-iteration byte loop. Shared tails
retain their real frames; no synthetic call or second record engine was added.
Input/effect services remain explicit original child contracts; their internal
algorithms are not claimed by this batch. One existing WRAM byte is catalogued.

Shared ROM/full-CPU/full-WRAM/ordered-data+MMIO/device-state proof: 7,680 ordinary,
3,840 actual-return, 148 child-unwind and 960 native-composition states per side.
Another 296 live-child and 128 pointer-overlap cases pass per side: 13,052 states.
The 592 post-child checks cover 79 call sites and 148 selected repeated visits.
Pre-access guards: 1,966,215 semantic and 1,970,040 production ABI. Twenty-nine
effective heap-ROM controls per side detect changes in dispatch, target counts,
stack/width handling, record flags, MP gates and swaps.

Fresh compiled library: 1,772 states and 592 post-child checks per side, the
guards, 2,220 actual redirects, fifteen C++ symbols and generated source selection
pass. Strict GCC (205.61 s), Clang (178.08 s) and isolated Windows
Release (366.23 s) builds pass. Unchanged CLI executable/fixture proofs
are reused only after matching hashes. Checks cover the combined final state.
Full regression, unrelated scenes, pixels/manual acceptance, physical timing
and whole-game profiling remain deferred. No measured speedup is claimed.

During integration review, a new source-path collision was corrected before
commit: the existing inventory-selection module was restored byte-for-byte;
new actions live in inventory_actions.c. Six existing source labels were also
restored. Their provenance-only correction kept generated runtime configuration
byte-identical; unchanged compiler results are reused. A pointer control exposed
a private seed gap, now covered. The original ROM and protected owner work are
intact. Tests, proof programs and fixtures remain private; nothing is pushed.

Totals: 1,742 verified contracts; 1,718 ordinary bindings; 775 WRAM locations;
479 semantic C units. Twenty-four verified contracts remain without a binding.

# Menu list sorting and spell-member selection — 2026-10-18

Five complete contracts now use native C:

- `$82:A9B5`: inventory sorting through the original twelve category buckets.
- `$82:B3AD`: stable two-group spell-list sorting.
- `$82:B5ED`: selected member's MP and spell-list pointer.
- `$82:B60A`: previous/next eligible spell-list member.
- `$81:F1AB`: item-record word at offset seven.

The native list-input callers reach these routines. Category masks, counter
pointers and group pointers remain ROM-backed. The original item/spell record
helpers are reused. Live counters, saved loop indices, record IDs, data banks
and return status remain observable; no second list or record engine was added.
Three existing WRAM fields are catalogued, without allocating guest storage.

One shared original-ROM/full-CPU/full-WRAM/ordered data+MMIO checkpoint covers
1,280 ordinary states, 320 actual returns, 152 child unwinds and 160 native
compositions per side: 1,912 states. Another 96 states per side change live
child-owned IDs, counters, pointers, data banks and pending stack values.
All twelve buckets are witnessed. The 112 post-child checks cover 19 call
sites and 28 selected repeated iterations. Pre-access guards: 655,404 semantic
and 656,935 production ABI. Nineteen effective heap-ROM mutations per side
detect grouping, vacancy, copy bounds, member wrap and record-field drift.

Eight bounded original nonterminating skip-loop prefixes also match CPU,
WRAM and bus order across both WRAM banks, both directions and counts zero
and four. Production retains the loop; no artificial exit or timing shortcut
was introduced. These prefix checks make no whole-timeline timing claim.

Fresh compiled library: 534 states per side, the guards, five C++ symbols,
generated source selection and 420 actual redirected returns pass. Redirects
cover 19 sites, 28 selected iterations, five alterations and three contexts.
Strict GCC (197.23 s), Clang (159.67 s) and isolated Windows
Release (366.16 s) builds pass. CLI executable and private fixture
hashes remain unchanged; their accepted CLI proof is reused. Full regression,
unrelated scenes, pixel/manual acceptance, physical APU/NMI/cycle/PCM checks
and whole-game profiling remain deferred. No measured speedup is claimed.

The existing `$82:9214`/`$82:922D` metadata now declares dynamic exit widths.
The prior stack-alias proof already shows that PLP can restore changed status;
their semantic source, producers and used macro values remain unchanged.
The shared checkpoint covers the combined final state, not each intermediate
commit independently. Tests, fixtures and tools remain private.

Totals: 1,727 verified contracts; 1,703 ordinary bindings; 774 WRAM locations;
477 semantic C units. Protected owner work and the original ROM remain intact.

# Secondary menu sprite mask - 2026-10-18

82:922D now has a complete verified contract and ordinary binding. It clears
slots16..31 using X's16-bit mask;82:9214 clears slots0..15. Both reuse one
loop in menu_cursor_callers.c. Existing9214 API admission and producer are
unchanged. Original PHP/REP/LSR/STZ/PLP order,DB-relative addresses,flags and
stack-overlap effects remain literal. No separate sprite engine.

Shared affected checkpoint:all65536 masks plus4096 stack/bank-alias cases
per entry,139264 states per side with actual RTL,full CPU/full WRAM and
ordered data/MMIO/device-state comparison. Production ABI reports69632
native invocations per entry and zero fallback.128 existing native-parent
prefixes execute256 complete mask calls per side;remaining children retain
explicit original boundaries. Prefix proofs do not claim complete parent
behavior.516 semantic /1540 producer pre-access guards pass. Eight heap-ROM
controls per side detect shift,destination,loop-limit and PLP drift.

Fresh production library10240 states/side plus128 parent prefixes,guards,
one C++ symbol and generated selection PASS. Production helper/producer
closure matches exact tokens apart from read-only test observers.
GCC76.3s /Clang71.56s /isolated Windows106.31s pass.
CLI executable and fixture hashes unchanged;accepted CLI proof reused.
Full regression,unrelated scenes,pixels/manual,physical APU/NMI/cycle/PCM
and profiling deferred. No measured speedup claim. No test/tool commits.

1722 verified /1698 ordinary bindings /771 WRAM /476 C units.
Owner-dirty menu_screen.c and protected owner/ROM hashes remain intact.

# Save portrait display - 2026-10-18

Seven complete82-bank entries now use native C:F2CE/F2FE slot previews,
F383 sprite tables,F395 placement/animation,F3DD/F3FC selection highlighting,
andF427 level labels. The existing EF25 caller reaches F2CE atEF94;F2D9 is
internal fallthrough. No separate artificial F2D9 contract was added.

The original five-word layout stack,free/occupied cleanup,live preview/name
pointers,byte counters,X8 table calls and live data banks remain intact.
Positions/slot tables stay ROM-backed. F427 retains both bytes of16-bit
position reads before masking00F8. Highlight changes retain original CMP
carry and raw animation IDs. Existing sprite primitives are reused;no second
renderer or copied assets. Four18-byte preview records occupy the catalogued
72-byte range7E:A5F3..A63A,immediately after the existingA498 snapshot.

One shared original-ROM/full-CPU/full-WRAM/ordered-data+MMIO checkpoint:
1792 normal,448 actual RTS,78 child unwinds and112 nested native compositions
per side:2430 states.136 post-child checks cover34 selected repeated
positions,including X8 and zero-count wrap.96 more states per side mutate
pending layout words,occupancy,preview pointers,counters,X/Y and data banks
inside children.917567 semantic and919352 production-ABI guards pass.
Fifteen effective heap-ROM controls per side detect bank/pointer routing,
stack pushes,occupancy branches,counters,origin arithmetic,highlight thresholds,
IDs,position reads,level sources and encoded child calls.

Fresh library402 states/side,510 actual redirected returns across13 sites/
34 selected iterations/five alterations/three contexts,seven C++ symbols and
generated selection pass. Strict incremental GCC167.47s,
Clang141.08s and isolated Windows357.98s builds pass.
CLI binaries and fixture hashes are unchanged;their accepted proof is reused.
Completed unchanged proofs were reused while adding only the live-field cases.
Full regression,unrelated scenes,pixel/manual acceptance,physical APU/NMI/
cycle/PCM checks and whole-game profile remain deferred. No measured speedup
or cycle-exactness claim. Original quirks and frame ownership stay semantic.

1721 verified contracts /1697 ordinary bindings /771 WRAM /476 C units.
Owner files and ROM intact. Tests,fixtures and tools remain private.

# Menu input waits - 2026-10-18

Four complete82-bank contracts now use native C:9330 counted wait or button,
9353 sprite-frame countdown,935D ordered button masks,and9380 sprite motion
or button. Existing button and frame services remain original child contracts.
Both pressed bytes are checked.935D retains word BIT flags,first-mask
priority and clear/set carry exits.9353 keeps the original256-frame wait
for count zero. Counters stay on the guest stack and are reread after children.
1567 is a sprite-motion request,not a generic timer. Three memory-map notes
clarify existing storage;no new allocation or duplicate input/OAM code.

Shared original-ROM/full-CPU/full-WRAM/ordered-data+MMIO checkpoint per side:
1536 normal states,384 actual RTS/RTL,154 child unwinds,192 compositions
through the scenario viewer and selection-message caller:2266 states.
140 post-child checks sample first,last,midpoint and zero-wrap iterations;
24 additional states mutate live stack counters inside children.524452
semantic and525472 unpaired production-ABI guards pass. Twelve effective
heap-ROM controls per side detect count masks,button bytes,count steps,
mask priority,carry polarity,motion source and encoded child routing.
No semantic mismatch or changed expectation was needed.

Fresh public library308 states/side,525 actual redirected child returns
across7 sites/35 selected iterations/five alterations/three contexts,
four C++ symbols and generated native selection pass. Strict incremental
GCC49.42s,Clang46.05s,isolated Windows298.2s
builds pass. Unchanged CLI binaries and fixture reuse accepted CLI proofs.
Full regression,unrelated scenes,pixel/manual acceptance,physical APU/NMI
cycle/PCM checks and whole-game profile remain deferred. No timing shortcut,
cycle-exactness or measured speedup claim. Original frame ownership remains.

1714 verified contracts /1690 ordinary bindings /770 WRAM /475 C units.
Tests,fixtures and tools remain private. Owner files and ROM are intact.

# Scenario viewer - 2026-10-18

The existing82:EEF2 caller now reaches verified8E:E5EF native C. Its three
internal routines draw scenario pages,clear two tile layers and initialize
seven portrait sprites. The original pages1..3 wrap back through portrait
setup; selection carry chooses next page or exit. All local JSR,PHY,PHB,
8-bit sprite-table calls,live DB,DP pointers and ordered writes are retained.
Existing text,input,graphics,sound and sprite contracts remain children;
their internals are not newly claimed. No duplicated sprite renderer or
invented scenario data. E642/E688/E6A7 stay internal,without extra bindings.

One affected checkpoint covers2065 original-ROM states per side,including
1024 ordinary,512 actual RTS/RTL,256 native compositions and273 child
unwinds.364 post-child checks cover all91 repeated occurrences across12
external sites;32 more states per side redirect pending local JSR returns.
Full CPU,WRAM,ordered data/MMIO and device state agree.131113 semantic and
131368 actual-ABI guards pass;11 effective heap-ROM controls per side detect
page wrap,sprite count/stride,positions,clear size,requests and child routing.
One wrong-width control ran into a trace cap; it is retained but excluded
from the accepted controls,replaced by a precise encoded-child-bank change.
Initial private caller RTS/old-boundary assumptions were corrected; no
production mismatch or relaxed original comparison was involved.

Fresh library212 states per side,1365 actual redirected returns spanning
91 occurrences/five alterations/three contexts,C++ linkage and generated
selection pass. Existing caller and native sprite compositions are included.
Strict GCC49.17s,Clang44.72s,isolated Windows283.16s builds pass. CLI binaries
and fixture hashes are unchanged; prior CLI proof is reused. Completed
unchanged normal/return/composition cases were reused while extending only
repeated-boundary coverage. Full regression,unrelated scenes,pixel/manual
acceptance,physical APU/NMI/cycle/PCM checks and whole-game profile deferred.

1710 verified contracts,1686 ordinary bindings,770 WRAM locations,474 C units.
Owner files and ROM intact. Tests,fixtures and tools remain local/uncommitted.

# Actor flash and palette grayscale - 2026-10-18

Complete85:E5C2 FlashActionActor and81:B336/B367 palette wrappers now use
semantic C. Existing81:AFC4 reaches the actor routine through its original
JSL sites. Both palette wrappers call the already verified81:B54A gray
worker with original JSR frames; no second color conversion was added.
The buffer variant retains full16-bit inversion,including bit15. Both
wrappers preserve15 entries,skipped first color,raw palette indices,word
counter initialization and byte decrements,source/buffer routing and dirty flag.

The root retains normal enemy,special enemy,party andcapsule paths. Original
five-frame repeats and final single calls keep all nested JSR frames,counter
bytes,sprite rebuild requests and frame-input calls. Capsule waits do not
invent sprite rebuilds. Normal palettes copy32 bytes,gray the staged group,
invert it and alternate original/temporary palette states. The special enemy
path retains its incoming sprite offset. PHP/PHB and full A/X/Y are restored;
exit_mx is dynamic because PLP restores the entry widths and flags.
Entry PB85,DP0,binary,S1F20..1FFC,either M/X;children resumeM1X16 or hand off
at the exact next PC. Palette wrappers usePB81,M1X16,DP0,binary,S1F00..1FFC.
Data bank stays dynamic. Existing indexed palette and sprite bases are reused;
two memory-map notes clarify phase-specific fields without new allocations.

Shared original-ROM/full-CPU/full-WRAM/ordered-data+MMIO checkpoint per side:
12288 normal,768 actual RTL,272 child unwinds and384 native grayscale
compositions:13712states.1088 post-child M/X/DP/decimal checks cover every
one of16 repeated/final occurrences,including the different single-call
stack shape. All17 JSL sites and four root returns are witnessed. Fifteen
effective heap-ROM controls/side detect palette count/high counter byte,
inversion,source routing,dirty flag,PHP/PLP,slot/stride,frame count,branch
palette selection,child bank and rebuild address.393270 semantic and394805
unpaired production-ABI guards pass. Existing ColorToGray executes natively
inside palette and root compositions. No disk ROM modification/distribution.

Fresh public library:308states/side,1380 actual redirected returns spanning
17sites/92occurrences/five alterations/three owner contexts,three C++ symbols
and the null-child boundary pass. Production helper/producer tokens and new
generated native selections match. Existing presentation/brightness workers
retain their tokens; old metadata rows and generated WRAM definitions are
unchanged. Two differently worded negative-control diagnostics were correctly
detected and classified; no production behavior or comparison was weakened.
Completed unchanged controls were reused while finishing the remaining cases.

Strict incremental GCC80.67s,Clang74.45s and isolated Windows Release
325.05s builds pass. CLI executable/fixture byte hashes remain unchanged,
so their accepted proof is reused. Full decomp-verify,unrelated gameplay,
pixel/manual acceptance,physical APU/NMI timing and whole-game profiling
remain deferred. No cycle,PCM,FPS or full-game completeness claim.

1709verified contracts/1685ordinary bindings/770WRAM/473C units.
Owner/ROM hashes intact. Tests,fixtures and tools remain private and uncommitted.

# Enemy insertion and dependent record ordering - 2026-10-18

Three complete85-bank contracts now use semantic C: E174 ReorderEnemies,
E412 InsertEnemy andE4E5 PrepareEnemyInsertion. The existing81:AC72 action
reachesE412 at81:ACED. Placement,stat,graphics,palette and frame children
reuse their existing contracts; no duplicate loader or renderer was added.

E174 preserves the ordered fast return,equal-key swaps and six-slot
permutation. Eight display arrays,seven-byte auxiliary records,190-byte
battlers and12-byte actions retain their original copy order and hardware
multiplies. The internalE3F9 worker keeps all18 fixed JSR sites and the live
turn-queue site. It remaps both enemy-action targets,party actions,current
action and queued actors; FF queue entries remain untouched. No queue clamp,
identity shortcut or original flag/register behavior was optimized away.

E412 preserves PHB and full saved A/X/Y,free-slot scan,record allocation,
variant roll,display geometry and all18 child sites. E4E5 retains12 child
sites,three queue/frame stages,WMDATA clearing and up to six live enemy tile
draws. Changed child M/X,DP,decimal or PB returns stop at the exact next PC;
nonlocal children keep pending frames. Entry PB85,DP0,binary,S1F20..1FFC,
dynamic DB;E174 requiresM1X16,E412 accepts either width,andE4E5 either M
withX16. E412 returnsM1X16 after its original REP/SEP normalization.

Shared ROM/full-CPU/full-WRAM/ordered-data/MMIO checkpoint per side:
12288 normal,768 actual RTL,480 forced-child unwinds and384 native
compositions:13920states.960 post-child boundaries/side pass. All30 primary
sites and two nested placement calls are witnessed.848 more states/side
cover all720 permutations and64 occupancy masks against two WRAM patterns;
all256 input masks reach the original internal remapper. Fourteen effective
heap-ROM mutations per side detect ordering,record-length,target-mask,
geometry,variant,MMIO and rebuild errors.393334 semantic and395124 actual-
bridge pre-access guards pass. No disk ROM modification or distribution.

Fresh public library adds360 states/side,450 redirected returns across30
sites/five alterations/three owner contexts,three C++ symbols and two null-
child boundaries. Exact production helper/producer tokens and generated
native selection pass. The existing followup helpers retain their tokens;
older metadata/memory-map rows remain unchanged. Three newly catalogued
addresses describe the trailing display bytes,auxiliary-record base and
enemy-action array. The14BC base overlaps menu-phase fields,so only its
base is catalogued; no separate42-byte allocation is claimed.

Strict incremental GCC165.59s,Clang138.17s and isolated Windows Release
248.89s builds pass. Unchanged resource-CLI executable/fixture hashes justify
proof reuse. Validation is shared across all three routines. Full decomp-
verify,whole-game profiling,physical APU/NMI timing,manual gameplay and
pixel-level rendering acceptance remain deferred. Existing non-cycle-exact
field-text PCM evidence is unchanged. No FPS or whole-game coverage claim.

1706verified contracts/1682ordinary bindings/770WRAM/473C units.
Owner/ROM hashes intact. Tests,fixtures and tools stay private and uncommitted.

# Restore battle item reservations - 2026-10-18

85:96BE Lufia2BattleRestoreSelectedItems completes the missing81:C739
command-selection child at81:CAE2. Revisiting a previous party command first
restores the saved battle inventory work area through85:96B0, then consumes
items reserved by the earlier selected actions through81:F15B/81:F114.
Those existing algorithms are reused, with no second inventory implementation.
Three original unrolled records use the existing7F:F564/F570/F57C constants.
The spare RTL at85:972D is not counted as another semantic routine.

The original PHX/PLX and four JSL sites remain. Full-width X is saved inDP54,
CPX reads that word, and the two later DEC operations alter only its low byte.
Zero,1/2/3,larger and high-byte-only indices preserve original behavior;
there is no invented clamp. Each item masks9ID bits and reserves one unit
through0200 before the original child. Data bank remains dynamic. Entry:
PB85,binary,M1X16,DP0,S1F00..1FFC,nonnull child. Unsupported contexts hand off
before guest access. Changed child M/X,DP or decimal status stops at its exact
next instruction; nonlocal child exits retain pending parent frames.

One shared ROM/full-CPU/full-WRAM/ordered-data+MMIO checkpoint per side:
4096 normal states,256 actual RTL returns,64 forced unwinds and512 complete
restore/inventory compositions:4928states.128 post-child boundaries/side pass.
All four original caller sites and the nested81:F18F consume site are reached.
Four effective heap-ROM controls/side alter DP scratch,action type,item mask
or requested quantity and are detected. A high-mask-bit mutation was initially
ineffective in its first64low-ID samples; that result remains recorded. The
effective mask control clears a witnessed low bit instead. No ROM was modified
on disk.131081 semantic and131336 actual-bridge pre-access guards pass.

Fresh MSVC public library:96states/side,60redirects across four sites/five
return alterations/three owner contexts,C++ linkage and null-child boundary
pass. Extracted production bridge helpers match actual source tokens; generated
source selection installs the verified85:96BE binding. Eight complete81:C739
caller cases run the new semantic parent and existing inventory children;
all eight revisit an earlier command and consume its reserved item. Full CPU,
WRAM and streamed write/MMIO order match the ROM. This is synthetic acceptance,
not a claim of manual gameplay or whole-game runtime hit-rate coverage.

Private fixture corrections:full child composition uses sufficient stack for
existing inventory admission; a missing-child check stops at the parent RTL.
The older caller fixture's512write buffer was insufficient, so a65536event
streaming observer now compares every write and rejects overflow. Cancellation
state resets separately for reference/native replay. An old signed comparison
and a quoted Windows compiler output path were corrected in private tooling.
No semantic expectation or production behavior was weakened for these fixes.

Strict incremental GCC65.06s,Clang59.09s and isolated Windows Release204.25s
builds pass. Resource CLI executables and fixture hashes remain unchanged;
their accepted CLI proof is reused. Necessary public ABI checks use the fresh
library after formatting. Full decomp-verify,unrelated gameplay,cycle-accurate
APU/NMI timing and performance profiling are deferred,not reported passing.
Existing non-cycle-exact field-text PCM evidence remains separate and unchanged.

1703 verified contracts/1679ordinary bindings/767WRAM/473C units.
All previous battle-loop helpers retain their tokens. Owner/ROM hashes intact.
Tests,fixtures,tools and captures stay private and uncommitted.

# Preserve field-to-text child services - 2026-10-18

80:9C72 already accepted consumer child services but discarded them in
FinishFieldTextTick. Its active-text and prompt-continuation branches now
pass the same services to the existing TextEngineRun worker. Optional internal
body/prompt entry wrappers preserve the caller's early-return result. Legacy
APIs still pass NULL. No extra interpreter, command algorithm, public API,
metadata contract, binding, C unit or WRAM field was introduced.

The unchanged prompt operations preserve timer comparison, button consumption,
close/release behavior and fallthrough into the text VM. Shared TextEngineRun,
legacy step-body and standalone WithServices workers are token-identical.
Original PHB/PHP, child JSR/JSL, exact unwind and return boundaries remain.
The original lower-level children remain separate contracts; this is caller
integration of existing verified semantic C, not an inventory-count increase.

Current-head private input-driven profiling identified this missing path.
The profile links the isolated agent build and uses a fresh private working
directory, leaving owner saves/config/build untouched. Before/after replay
covers6814 cold-boot, title, new-game, dialogue, close and field-input frames.
All13628 CPU-WRAM andPPU-picture frame hashes match exactly. PCM differs on
4184frames starting at frame2594. A repeat of the unchanged baseline reproduces
all20442hashes, establishing that this is real execution-model timing variation.
The read-only port observer compares8856writes during frames2300..2700: every
frame,address,value and ordering matches. The first changed sound request at
frame2588 occurs2546master clocks earlier; maximum observed within-frame
difference is2738clocks. Native dispatch is not cycle exact, as documented
in the existing verification contract. PCM/timestamp equality is explicitly
NOT claimed. Sound scheduler, wait owners and APU logic remain unchanged.
No artificial delay was added to reproduce the old mixed native/interpreted
clock. Original-ROM CPU/WRAM/bus semantics remain the acceptance contract.

Interpreted instruction observations change from3912506 to3916407
(3901 more); no performance gain is claimed. Native text routing improves,
while altered sound polling phase adds instructions elsewhere. This bounded
replay is not wall-time/FPS or whole-game coverage. Sound-upload/reply polling
still dominates reported transition residuals; those algorithms already have
native semantic owners. Cycle-accurate APU equivalence remains separate work.

Shared original-ROM checkpoint per side:4096 idle/prompt/close/delay/window
states,256 actual RTL returns and64 forced-child unwinds:4416states.
128 post-child width/DP/decimal boundaries per side also pass. CPU, full WRAM,
ordered data/MMIO accesses and device state compare. Four effective ROM
mutations per side detect changed prompt mask, timer source, close mask or
continuation mask, including skipped required child arrivals. An initial
synthetic glyph destination accidentally overlapped the return stack; the
fixture now uses the real D000 glyph workspace. A missing pressed-button bit
was exposed by a negative control and corrected. No production fix or
comparison expectation was weakened for either fixture correction.

Fresh public library adds80states per side and60 real9C72 redirected-return
cases across four original call sites, five redirect types and three owner
contexts. Exact production helper/producer tokens and generated selection
pass. No private opcode adapter is installed or needed in this batch.
GCC/Clang/isolated Windows incremental builds pass in31.27/28.81/12.48seconds.
Unchanged CLI executable and fixture hashes justify accepted proof reuse.
No broad unchanged full regression was repeated. Full decomp-verify, other
scenes, asynchronous APU/NMI timing and whole-game profiling remain deferred.

1702verified contracts/1678ordinary bindings/767WRAM/473C units remain.
Owner/ROM hashes are unchanged. Tests, tools and profile data stay private.

# Event area-transition and sound owners - 2026-10-18

Complete80:E50B area-transition and80:D2BB immediate-sound opcode owners
now share the existing EventRun services inside CBAE. Both retain their
original80:CC35 continuation and remain independently unbound. The existing
transition, header lookup, actor-slot save/restore and sound algorithms are
reused. The shared conditional-call adapter is now named EventActorCall;
its legacy guest operations stay intact, with explicit host handoff propagation.
No new VM, algorithm, source unit, WRAM field or production bridge.

The old callbacks force handoffs for area branch7 at83:B82F and the sound
acknowledgement at80:9A0A. New composition reaches these original child
sites through the consumer. All five primary and seven nested call sites
are witnessed. Nested menu/sound reply interiors are explicit child contracts;
APU handshake timing and menu pixel rendering are not newly verified here.
Null-service and unsupported contexts retain the legacy callbacks.

Public admission:binary PB80,DP0,M1X16,S1F20..1FFC,any DB,nonnull child.
Each child keeps its encoded JSR/JSL frame, pending PHB/PHY and exact unwind
site. Changed child widths/DP/decimal/PB stop at the original next instruction.
The original byte readers, byte exchange, header stride9 and family6,
area state, sound command and final saved Y/DB are preserved.

Shared ROM/CPU/full-WRAM/ordered-data/MMIO checkpoint per side:16384 normal,
1024 composed,160 forced-unwind and128 full existing VM calls:17696 states.
Direct200 and ABI160 post-child boundaries pass. Production paired returns
normalize callee PB; actual rewritten returns are checked separately rather
than pretending leftover PB can redirect a real paired RTL.262224 portable
and262736 private opcode-adapter guards pass. Four effective heap-ROM mutants
per side detect changed family/stride setup, saved bank or encoded child.

64 direct legacy ROM states and128 affected D5F2 native compositions pass.
Legacy area/sound/return workers and the conditional worker are token-conserved
apart from the shared helper rename. Fresh public library adds180 states per
side,2 C++ symbols and2 null-child boundaries.150 redirect checks comprise
75 private opcode-adapter cases and75 actual CBAE cases at all five primary
sites, across five redirect types and three owner contexts. Only CBAE is an
installed producer; both opcode adapters remain private. Exact production
helper tokens and generated selection pass.

Strict GCC/Clang and isolated Windows Release incremental builds pass.
Unchanged CLI executable and fixture hashes justify prior CLI-proof reuse.
Full decomp-verify, unrelated VMs, gameplay/pixels, APU/NMI scheduling and
current whole-game hit profiling are deferred, not passing claims. Owner
sources, ROM and build/windows remain untouched. No tests/tools/fixtures
are committed.1702verified contracts/1678ordinary bindings/767WRAM/473C units
are inventory counts, not whole-game coverage or measured performance gains.

# Event party refresh and conditional regions - 2026-10-18

Complete opcode owners80:D5BC/D5EB refresh party actors,80:DB9D centers
layers, and80:D5F2 updates conditional object regions. All share the existing
event-VM workers and EventRun child services. Their original continuation is
80:CC35; none receives an independent runtime binding. The existing CBAE
producer uses these services. No duplicate VM, child algorithm or C unit.

Party refresh now permits the existing verified actor-slot children to
complete rather than forcing the old callback's interpreter handoff. Two
legacy witnesses stop at83:D508; native compositions continue through the
original83:C7F8/D508 children.83:BBF3 is excluded by this parent's state bit
and is not claimed covered here. Old null-service operations remain conserved.

D5F2 retains the original literal search under complete services. All65536
initial Y values reach a fixed supported-ROM FF sentinel within11241 visits;
RAM/MMIO are conservatively non-sentinels in that bound. A differential long
fixture actually visits2761 entries. Legacy contexts keep their bounded
handoff. Original indexed bank carry, record reads, flags, saved Y/DB and
child frames remain visible. Diagnostic child DB is restored by original
PLB, not constrained unnecessarily. Low-stack children may use the interpreter.

Admission is binary PB80,DP0,M1X16,S1F20..1FFC,any DB,nonnull child. Changed
child widths/DP/decimal/PB stop at original next instructions; explicit
unwinds preserve pending frames. Child interiors outside composed witnesses
remain scoped child contracts, not new decompilation claims.

Shared checkpoint per side:16384 normal,1024 composed,352 forced-unwind,
128 full-VM,256 additional active-actor composed,64 active-actor full-VM,
and one long-search state:18209 states. CPU, full WRAM, ordered data/MMIO
and device state compare.352 post-child boundaries per side also pass.
524448 semantic and525472 private ABI-adapter pre-access guards pass.
Six effective heap-ROM mutants per side are detected. Root VM returns
execute192 actual RTL instructions; composed children execute real returns.

Fresh public library:493 states per side,4 C++ symbols and4 null-child
boundaries pass. Extra checks add32 conditional stack endpoints and16
low-stack full-VM states per side. Rejected child guards delegate to a
separate interp816 instance and compare against the original reference.
The initial extra fixture incorrectly required every child guard to admit;
its correction changes no production source or comparison expectation.

Redirect checks cover165 private opcode-adapter cases plus75 actual CBAE
producer cases, across five redirect types and three owner contexts.
Only CBAE is an installed producer here; four opcode adapters stay private.
Exact production helper tokens and generated selection pass. Strict GCC,
Clang and isolated Windows Release incremental builds pass. CLI executable
and fixture hashes are unchanged, so accepted CLI proofs are reused.
Full decomp-verify, unrelated VMs, gameplay/pixels, asynchronous NMI timing
and current whole-game hit rates are deferred, not passing claims.

Current metadata:1700 verified contracts,1678 ordinary bindings,767 WRAM
markers,473 C units. No tests/tools/fixtures or ROM are committed. Owner
sources and build/windows remain untouched. Counts are not game coverage.

# Event cell-probe service chain - 2026-10-18

80:D2FB now owns the complete event-position/object-state probe through its
original80:E8B9 byte reader,80:EA09 position resolver and83:FB51 object-state
child. Both80:D2E1/D2EE conditional callers share their existing branch and
goto/skip operations. Their complete opcode contracts continue at80:CC35;
they are integrated inside the existing event VM and remain independently
unbound. Only the ordinary D2FB entry receives a new native binding.

The existing EventOpCellType chooses the same shared branch worker directly,
passing its current EventRun services. No extra interpreter or host-only
continuation roundtrip is needed. Null-service and unsupported-context callers
retain the old position/probe/branch operations and original frames. The old
pieces remain token-conserved. No child algorithm, C unit or WRAM allocation
was duplicated. Normal admission: binary PB80,DP0,M1X16,S1F10..1FFC,any DB.
Changed child widths/DP/decimal/PB stop at the exact original next instruction;
unwinds retain the pushed child and pending PHY frames.

Shared original-ROM checkpoint per side:12288 normal,3072 return,1536 native
composition and160 forced-unwind states. It also covers128 complete existing
event-timer/VM calls,256 null-service branch states and128 whole-VM unwinds:
17568 states per side. These include1024 actual root RTS and128 root RTL;
composed children also execute their original returns. CPU, all WRAM, every
ordered data/MMIO access and device state are compared. Both branch senses,
bank/window boundaries, full-width operand samples and all primary sites are
covered; this is not an exhaustive-input or runtime-hit-rate claim.

Additional post-child checks:200 direct and160 production boundaries.
The direct set includes deliberately changed PB. A normal production paired
return normalizes the callee's leftover PB; mismatched actual return PCs are
instead covered by105 real production redirect probes across all five primary
sites and both whole-VM branch senses.393288 portable pre-access guards and
394056 actual producer guards pass. Five effective heap-ROM mutations per
side detect changed stores, byte exchange, branch sense or restored index.

Fresh public-library checks add416 states per side, including the actual
D2FB/CBAE producers, nested composition, full VM and its unwinds, plus the
legacy route. D2E1/D2EE use private ABI test adapters only. Production function
tokens,3 C++ symbols,3 null-child boundaries and generated selection pass.
The final direct shared-worker VM cleanup was rechecked with the affected
256 legacy,128 whole-VM and128 VM-unwind states per side; unchanged entry
proofs/guards are reused rather than repeated.

GCC/Clang initially caught a missing stddef header; it was added. Final strict
GCC/Clang and isolated Windows Release incremental builds pass. Unchanged CLI
executables/private fixture hashes justify prior CLI-proof reuse. A synthetic
VM seed also needed the actual script DB's redraw byte initialized. No ROM
expectation or production contract was weakened. Owner build/windows and
unrelated sources/ROM hashes are unchanged. Tests/tools/fixtures remain local.
Full decomp-verify, unrelated VM regression, gameplay/pixels, asynchronous NMI
timing and current whole-game hit profiling remain deferred, not passing claims.

Current metadata:1696 verified contracts,1678 ordinary native bindings,
767 WRAM markers,473 C units. These counts do not measure whole-game coverage.

# Shared coordinate helpers - 2026-10-18

Three complete original entries now share the existing C workers:
80:C557 packed window position to tilemap offset,80:C52C window border row,
and83:E018 spawned-actor fine coordinates. The old spawn opcode retains
its exact JSL/RTL frames. Its expanded guest-operation tokens and both
window workers are unchanged. No duplicate algorithm or new C unit.

Admission is binary DP0/X16, PB80/M0 for the window helpers and PB83/M1 for
spawn placement, S1F10..1FFC. Every DB is admitted. Unsupported states stop
before semantic memory access; production also rejects emulation. Return
instructions remain outside the portable bodies:80:C56D/C556 RTS,83:E032 RTL.

Shared original-ROM checkpoint:81928 direct and81928 actual-production-ABI
states compare CPU, complete WRAM, every ordered data/MMIO access and device
state. Each case also executes its actual original RTS or RTL. C557 covers
every packed input word; C52C/E018 each add8192 representative states across
all256 DB values, varied full-width indices and stack/flag combinations.
Eight additional C52C cases execute the complete65536-middle-tile underflow.
This is boundary coverage, not an exhaustive input-space or runtime-hit claim.

393276 portable pre-access guards and394056 production guards pass. Three
effective heap-ROM mutations per side detect shifted coordinates or changed
border alternation; the ROM file is unchanged. An initially narrow mutation
sample missed nonzero packed row bytes; the private sample was corrected to
vary both bytes. No assertion or production contract was weakened.

Fresh public-library checks add216 states per side, including deliberate
stack/return/DP aliases and long-address source/destination overlap. The
production adapters must execute natively in these supported states. Their
copied functions match production tokens, with read-only return observers.
Three C++ API symbols and generated binding selection pass. Existing shared
workers and legacy spawn caller remain token-conserved.

Strict standalone GCC/Clang and isolated agent Windows Release pass. CLI
executables and private fixture hashes are unchanged, permitting reuse of
the accepted CLI proof. Owner build/windows remains untouched. No repeated
full decomp-verify, whole-event-VM regression, pixels/gameplay or asynchronous
NMI timing proof is claimed for this small shared-worker exposure batch.
Tests, tools, fixtures and ROM remain private; unrelated owner hashes match.

The metadata now has1693 verified contracts and1677 ordinary native bindings,
with767 WRAM markers and473 C units. Counts do not measure whole-game work.

# Complete scene text window path - 2026-10-18

The native80:C23D preparation now completes its scene-window branch through
80:C2A1..C304. Three new complete contracts are80:C2A1 scene interior,
80:C1FD close-window services and80:9D3E open-window command. The existing
C23D metadata now names its complete WithServices owner. Its legacy API
retains the previous exact prefix/continuation contract for old callers.

The previously missing interior keeps both original BF0B NMI-counter waits,
the C61D upload clear, alignment, full word height and original tile loops.
BF0B remains an original child, with no invented cycles or fast-forward.
Zero width and zero doubled height still underflow; neither is clamped or
optimized away. The height word includes125D, shared with scene-label
placement. Existing WRAM labels are reused; no overlapping allocation added.

Opening and closing share their existing workers, preserving legacy guest
operations and call frames exactly. The service-enabled text VM now uses
the completed open-window command, including opcode08. It consumes the
normal9D00 continuation inside its existing loop and restores host-only
resume bookkeeping. No second interpreter or synthetic return was added.

Shared checkpoint:11538 direct and11538 ABI-adapter states:8192 normal,
2048 original-return,1024 native-child composition,128 complete existing
TextEngineStepWithServices calls,144 child unwinds and two zero-counter
cases. The VM group composes real text measurement, glyph clearing, scene
interior and upload clearing through the new path.1536 actual RTS and128
RTL execute per side. Nine original child sites are all observed.135 actual
redirect probes cover their host-return modes.524384 semantic and525408
production guard states pass.288 additional post-child boundary cases per
side cover changed M/X widths, DP and decimal state. Five effective heap-only
ROM mutations per side demonstrate CPU/WRAM mismatch detection.

Comparisons cover CPU, all WRAM, every ordered data/MMIO access and device
state. Child interiors outside composed owners remain explicit controlled
services or their separately verified originals. Both zero-counter cases
execute the full original65536-step wrap; the combined zero/zero worst-case
input is not enumerated and no exhaustive input-space claim is made.

Fresh public library checks add168 states per side, four C++ symbol links
and four null-child boundaries. Published adapters for C2A1/C23D/C1FD and
the existing9CB8 VM owner match the tested production functions. The9D3E
opcode has a private ABI test adapter only; it is integrated inside the
existing VM and receives no independent zero-hit binding. New ordinary
bindings are C2A1 and C1FD; C23D replaces its prior partial consumer path.

Legacy worker expansion is token-identical in guest operations. Publication
preserves semantic tokens after proven constant aliases and formatting.
Compiler gates caught a missing stddef include and an early wrapper using
later helper definitions. The include and definition order were repaired;
affected GCC/Clang and incremental Windows checks pass on the final state.
CLI binaries/fixture are unchanged, so their accepted proof is reused.
The owner build/windows directory remains untouched. No repeated full
decomp-verify or unrelated VM regression; gameplay, pixels, asynchronous
NMI cycle timing and current whole-game interpreter hit rates are deferred.
Tests, proofs and tooling remain private; ROM and owner hashes are preserved.

The frontier survey also distinguishes existing internal menu-string control
paths from genuinely missing entries. Explicit PC anchors are documented as
internal C without new native/verified claims. Prefix-only VM bindings that
previously produced zero additional hits remain disabled.

# Complete battle target action dispatcher - 2026-10-18

85:D0CD BattleApplyTargetAction now owns the complete original730-instruction
routine through D710. The existing31 child sites retain their13 original
targets; none of those already verified children was reimplemented. The C
separates eligibility/scripts, signed HP/MP scaling, attribute percentages,
status rolls, resource application, damage IP and defeat/rewards into phases.

Original semantics remain literal: the negative MP path complements without
incrementing, the HP path complements and increments, and the high reward
increments remain16-bit read-modify-writes. Every original B8 pointer access
and actor/work field is read live. An overlapping actor HP/action delta case
proves the later sign check cannot reuse a cached pre-write value. Saved X
remains on the stack at original BRK85:D243/D2AB; the actual BRK instructions
are executed in the private comparison, not replaced by a successful return.

Admission: native binary PB85,DP0,M1X16,S1F20..1FFC and writable low-WRAM DB
banks. Unknown child exits remain exact child unwinds. Four real RTS exits
and both BRK boundaries are checked. Four new WRAM markers name requested
HP/MP deltas and added/removed statuses; the existing action buffer is reused.

Shared checkpoint:14879 direct and14879 actual-production-adapter states.
Per side:4096 normal,1024 return,2048 edge,2048 native-child composition,
1024 composed-edge and4639 discovery/unwind states. All31 original JSR/JSL
sites have independent unwind witnesses. Each side executes3387 real RTS
and1719 original BRKs in that shared group.465 additional actual redirect
probes cover all31sites across paired/unpaired/mismatched host-return modes;
their4608 discovery states are additional, not another independent function.
131238 semantic and131368 production pre-access guards pass. Five effective
heap-only ROM mutations per side demonstrate CPU/WRAM mismatch detection.

CPU, all WRAM, ordered data/MMIO accesses and math/device state agree. Native
compositions exercise the real actor lookup, RNG, multiplication, division
and character lookup; other child interiors retain their separately verified
owners or controlled services. This is a complete parent contract, not a
claim that every descendant or all possible synthetic states were enumerated.
Supplemental alias/resource checks close three otherwise unobserved resource
branches. All730 original instruction nodes are observed, including both
BRKs, without claiming exhaustive branch-edge or input-space coverage.

Public code preserves tested semantic tokens after four equal WRAM aliases.
Fresh public library/actual adapter checks add128 states per side, plus a C++
symbol and null-child boundary check. The production producer closure remains
token-identical to the tested copy; generated selection includes the new
binding. Guards and unchanged sources reuse the completed shared checkpoint.

Strict GCC/Clang library and isolated Windows Release builds pass. Resource
CLI binaries and the private fixture remain byte-identical, justifying reuse
of their accepted contract; no repeated unchanged CLI run. Owner build/windows
is untouched. Full decomp-verify, whole-VM regression, gameplay/pixels/NMI cycle
timing and a current whole-game interpreter profile are deferred, not passed.
Tests, proof programs, fixtures and tooling remain private and uncommitted.
The ROM and unrelated owner work retain their hashes. No push is authorized.

# Battle action support and shared actor lookup - 2026-10-18

Five complete entries:81:FB1A BattleRollEnemyItemDrop,85:C168
BattleResolveActorRecord,85:DA11 BattleRunEquipmentScripts,85:DA43
BattleRunEquipmentItemScript and85:DA5F BattleCharacterFromActor.
Four new bodies share battle_action_support.c. The actor lookup reuses its
existing BattleScriptSlot worker; no second algorithm or new WRAM allocation.
Metadata only documents additional uses of existing scratch/record storage.

The drop scanner reads the original bank96 descriptors, preserves the two
separate random calls, and retains every zero/skip/store case. No probability
or random-call optimization was introduced. Equipment scripts keep their six
original offsets in their original order, save the live script byte, and use
the current X left by each child. DA43 saves X only around the item loader and
retains the original relative-script base, bank and offset. Actor-to-character
lookup keeps the original byte-index truncation and full saved X/Y values.

C168 exposes the shared worker with its two real RTS boundaries, preserving
both original callers' simulated JSR/RTS frames. Its original CPU/bus/stack
operations remain identical after expansion; a side-effect-free context flag
is captured only to identify the exit PC. No full legacy BattleScript VM
regression was repeated. The new independent contract is checked against ROM.

All five admit native binary PB81/85,DP0,M1X16,S1F20..1FFC. C168 restricts DB
to writable low-WRAM banks; immutable shifting-bit storage remains original.
Other entries retain every DB. Ten primary JSR/JSL sites plus one existing
nested relative-script site are checked. Item and relative-script owners are
composed with their real bodies; only the existing VM service is controlled.
Every primary site and nested81:FAE9 is observed, including entry case0.

One combined checkpoint per side:10240 normal,2560 original-return,320 parent/
new-child compositions,640 deeper compositions,40 nested-coverage states and
160 child unwinds =13960 direct and13960 actual producer states. All CPU,
full WRAM, every ordered data/MMIO access and device state agree. Each side
executes1024 real RTS and1536 real RTL.150 actual redirect probes pass.
655686 semantic and656840 actual guards pass; three fresh null-child API
checks supplement the semantic guards. Four heap-only ROM mutations are
detected per side. No ROM bytes were copied to artifacts or changed on disk.

Public library, exact current production adapters, five C++ symbol links and
generated binding selection pass. Fresh public checks add300 states per side.
Publication preserves tokens; the new identical bank guard was replaced by
its existing owner. Original worker expansion and equal WRAM aliases are
recorded privately. Tests and tooling remain local and uncommitted.

One shared strict GCC/Clang/isolated Windows Release checkpoint passes.
Unchanged resource CLI bytes and fixture justify prior-result reuse. Only
new nested evidence, negative probes and public links supplement the shared
normal/guard checkpoint. No repeated full decomp-verify; full old-VM regression,
gameplay/pixel/NMI timing and current global interpreter profiles are deferred.

81:FAC9 preserves incoming status. These M1X16 caller compositions are verified;
its older nominal M0X0 metadata must not be used as a guessed M0 exit for these
M1 callers. The existing child's implementation and metadata were not changed.
All13 distinct direct children at31 observed sites in85:D0CD now have their own
verified native bindings. The dispatcher itself remains original and next.

# Targeted actions and enemy graphics placement - 2026-10-18

Two complete entry contracts:85:CE6F BattlePrepareTargetedAction and85:E02D
BattleFindEnemyGraphicsPlacement. Both extend the existing battle_followup.c
owner; no duplicate implementations or new C units. Two WRAM markers name
the target count and dispatcher result byte. The window-size bytes and text
name buffer retain their original reuse as target bits and column workspace.

CE6F counts the six low target bits, retains the side bit, scales signed
six-byte fields and remaining words, copies the original136-byte action,
and visits each selected target. Hardware division keeps its original word/
byte register writes and saved-X traffic. Negative fields use the existing
80:834C service. Original target-bias, result and action children retain their
owners;85:D0CD remains original. Retry failure hands off at BRK85:CF55.

E02D marks existing enemy columns, rounds widths literally, measures free runs
in descending order and chooses the original center-out candidate. Raw widths
can spill beyond the32-byte workspace; no bounds clamp or host allocator was
added. Graphics lookup children retain existing owners. Allocation failure
returns FF; its original assertion hands off at85:E0E0 with exact live state.

Both entries admit PB85,DP0,binary,M1X16,S1F20..1FFC. CE6F requires writable
low-WRAM DB banks; immutable counters can preserve original nontermination
and remain interpreter-owned. E02D admits every DB. Eight encoded JSR/JSL
sites retain their frames and original child-return redirects.

One shared checkpoint per side:4096 normal states,1024 return states,128 child
unwinds,64 allocation-workspace invalidations,64 fault-path states and1024
real action-buffer states =6400 direct and6400 actual producer states.
CPU/full WRAM/every ordered data and MMIO/math/WMADD state agree. Per side,
1152 real RTS and512 real RTL execute. Both original BRKs are also executed
after handoff:64 allocation assertions plus8 target assertions per side.
The workspace-invalidation fixture applies the same synthetic external write
on both sides; it does not claim an NMI timing or real gameplay reproduction.

262350 semantic and262736 actual entry guards pass; two fresh public null-child
checks supplement semantic guards. All120 actual child-return redirect probes
pass. Three heap-only ROM mutations are detected per side. The ROM file was
hash-checked and never changed or copied. Source publication preserves exact
tokens apart from checked equal-valued WRAM names; existing owner text remains
unchanged. Fresh public-library/direct/actual/C++ links and generated binding
selection pass. Public proofs add148 states per side and two C++ symbols.

One combined strict GCC/Clang and isolated Windows Release checkpoint passes.
Unchanged resource CLI executable bytes and fixture justify prior-result reuse.
After the initial shared checkpoint, only missing real-buffer/fault evidence
and fresh public links ran; unchanged normal/guard groups were not repeated.
Full decomp-verify, complete old-caller regressions and gameplay/pixel/NMI-cycle
replay are deferred. No whole-ROM completion, global hit-rate or speedup claim.
Tests and proof tooling stay private and are excluded from both commits.

# Displayed field cells

`Lufia2FieldCellViewOpen` and `Lufia2FieldCellDisplayed` expose the field
streamers' cell substitution and metatile layout over a bank `$7F` snapshot.
They are consumer helpers, without a ROM entry, binding or verification status.
The original drawing routines are unchanged.

# Native entries for existing semantic workers - 2026-10-17

Four real original call targets now expose their existing shared implementations:
84:8862 AncientCaveAppendSavedItem, 84:886D AncientCaveTestBlueItem,
85:8E98 BattleUploadVramQueue and 85:9265 BattleClearActionWorkspace.
These are four newly verified entry contracts and native bindings, not four
newly reconstructed algorithms. The current conservative 1,675-root call survey
showed these targets but source inspection proved their bodies already existed.
No duplicate implementations, new WRAM allocations or C units were added.

cave_entry.c keeps its original simulated JSR/RTS wrappers around the extracted
blue-item test and saved-item append workers. battle_nmi.c keeps its original
queue caller frame. battle_frame.c calls the shared workspace worker inside
the same original JSL/RTL frame. Expanding each new worker into its old caller
reproduces the old CPU/bus/stack token stream exactly; LEGACY_EXPANSION.json
records the four checks. Native entry guards are separate from these unguarded
workers, so old caller admission and unusual old caller states remain unchanged.

The cave test scans the same 41 original words after masking the nine-bit item
ID, with original success/failure carry and distinct RTS boundaries. Appending
an item uses the raw DP60 offset, its original long word store and pointer
increment. Bank carry can map that store into a bank80 low-WRAM mirror and
overwrite DP60 itself or the live caller return word; neither case is clamped.
The queue worker consumes sixteen six-byte descriptors in reverse order,
retains channel6 register/DMA order and clears the original length bytes after
each transfer. The workspace clear keeps PHB/PLB, DB7F and all 764 byte stores.
No NMI scheduling, renderer, DMA engine, timing policy or gameplay change occurs.

All four complete entries admit their original PB, DP0, binary, X16 and
S1F00..1FFC, with any DB. The cave entries require M0; the battle entries M1.
Original callback-free RTS/RTL contracts are checked against actual producer
wrappers, including matching and nonmatching host-return-valid states.

One shared proof per side: 16,384 normal states, 4,096 original-return states
and 256 dedicated saved-item aliases = 20,736 direct and 20,736 actual states.
Return groups execute 3,072 real RTS and 1,024 real RTL; aliases add 256 real
RTS per side. All CPU/full WRAM/every ordered data and MMIO access and
math/Mode7/WMADD device states agree. Alias offsets cover WRAM/ROM bank carry,
DP pointer overlap, the live return word and FFFF. 524,320 semantic and
525,344 actual pre-access guards pass, including 1,024 emulation contexts.

The four existing caller paths have exact lexical expansion conservation;
their entire historical ROM suites were not rerun or reported as fresh passes.
The new actual entry proofs and public library checks complement that evidence.
A private ABI compile rejected unreachable code after an unused leaf-only
dispatcher stub. Making that unused stub return normally changed no reachable
leaf state or semantic source; direct proofs were retained and the previously
unexecuted ABI groups then ran. No expectation was weakened.

The bus proof checks the original DMA register/trigger order, not DMA payload
pixels or cycle timing. Full decomp-verify, complete legacy-caller regressions
and gameplay/pixel/NMI timing replay are deferred. No whole-ROM completion,
current global interpreter-hit percentage or speedup is claimed.

Fresh public checks each:128normal,64real returns and16saved-item aliases;
allguards and4C++symbols pass. Four effective heap-ROM faults andtwo
production stack/frame faults are detected. Public bodies are token-identical
to the privately verified shared workers. No ROM file is copied or modified.

One combined GCC/Clang/metadata/isolated Windows Release checkpoint PASS;
Windows 214.33s. Unchanged resource CLI executable bytes and unchanged
fixture/dependencies justify reuse of their previous accepted CLI test results.
No repeated full semantic regression; no push.

# Battle followups and selection copy - 2026-10-17

Three original caller contracts: 85:8F8A BattleResolveRecoverableStatuses,
85:E0E4 BattleCheckEnemyGraphicsCapacity and 86:9093 SelectScreenQueueTilemapCopy.
One new battle_followup.c unit and a small extension of menu_select_control.c;
all original child implementations retain their existing owners. No additional
WRAM declarations or duplicated table/inline entry bodies are introduced.

8F8A combines the two original DP actor masks, clears a 30-byte effect record
through WMDATA and checks the original three recoverable status bits. The
lookup/random/name/phrase/display order and every call frame remain literal.
Later BIT checks consume the live A left by earlier children, not a cached
status word. No new status roll or gameplay improvement is introduced. The
existing D9C9 owns its original zero-mask behavior; this caller does not fix it.

E0E4 saves DB and full A/X/Y, releases defeated enemies, checks six actor slots
and three graphics groups, then compares packed dimensions and existing group
weights against the original byte threshold. Both bytes of the packed result
feed WRMPYA/WRMPYB. The original eight-bit sum can wrap. Carry marks capacity
failure; both original RTL sites restore registers/DB and force M1X16 exactly.
Placement at E02D remains an explicit original child, including its original
assertion/BRK path. No host allocator replaces that operation.

9093 writes the original source7E:6000, length3000 and VRAM1C00 into DP scratch
and queues the original 82:8044 child. No direct PPU rendering is substituted.
All three callers admit original PB, DP0, binary, any DB and S1F20..1FFC.
8F8A/9093 enter M1X16; E0E4 accepts all native M/X and exits M1X16. Metadata
entry_mx records the nominal M1X0 call mode; guards prove the wider E0E4 input.

One combined proof per side: 12,288 normal states, 3,072 original-return states,
192 compositions and 704 child unwinds = 16,256 direct and 16,256 actual states.
All CPU/full WRAM/ordered data and MMIO/math/Mode7/WMADD state agree. Return
checks execute 2,048 real RTS and 1,024 real RTL per side. Twenty-two encoded
JSR/JSL sites, 330 actual child-return redirect probes and a 17-function actual
consumer closure pass. Compositions use the existing D9C9 and DFB9 semantic
owners; other children remain explicit services. No proof of their internals,
frame/NMI scheduling, pixels or hardware-cycle timing is claimed.

393,336 semantic and 394,869 actual pre-access guards pass, including three
null callbacks and 1,536 emulation contexts. Wrong PB/DP/decimal/width/stack
contexts hand off unchanged before data access. The restricted lower stack
bound leaves room for the original saved registers and child frames.

A forced-unwind fixture initially set only the low byte of a packed dimension
return. Its stale high byte legitimately failed the capacity check before the
desired child. The corrected forced fixture supplies the complete word; only
the affected unwind groups and outstanding redirect/guard groups were run.
Unchanged normal/return/composed proofs were retained. Production semantics
were not changed to force that path. Public formatting is token-identical.

The next related targets CE6F/E02D contain original BRK assertions and must
retain their actual fault boundaries. D0CD spans 730 decoded instructions;
it should be reconstructed as an action dispatcher with existing child owners,
not flattened or approximated. Research is private, not a verified promotion.

Full decomp-verify and gameplay/pixel/timing replay are deferred. No whole-ROM
completion, current global interpreter-hit percentage or speedup is claimed.

Fresh public checks each:96normal,48real returns,24compositions and44child
unwinds; all330actual redirects,allguards and3C++symbols pass. Three effective
heap-ROM faults andtwo production stack/frame faults are detected. No ROM file
is modified or copied; no push.

One combined GCC/Clang/metadata/isolated Windows Release checkpoint PASS;
Windows 221.48s. Unchanged resource CLI executable bytes and unchanged
fixture/dependencies justify reuse of their previous accepted CLI test results.
No repeated full semantic regression; no push.

# Inventory consumption and battle action work - 2026-10-17

Twelve complete original entry contracts: 81:F114/F15B/EBF4 and
85:948C/CE21/CF68/D025/D94F/D9F0/937D/CFAB/D080. Two new semantic units
share the original child-call and CPU helpers. Existing 85:D9C9 stays the sole
BattleEffectRecord implementation; menu_screen.c and all protected work stay
unchanged. Metadata entry_mx records the normal concrete entry width; the
documented guards also admit either M at F15B and all native M/X at EBF4/CE21.

Inventory consumption preserves the 96-slot backward search, nine-bit item IDs,
raw eight-bit selected slots and seven-bit quantities. F114's unusual return
values remain literal: encoded remaining quantity on a partial removal, old
quantity on depletion and full-A zero from TDC on an empty slot. The shared
09F2/09F4 scratch bytes retain their original reuse.

EBF4 restores PHP/PHB/A/X/Y after refreshing portraits and clearing five motion
records. 948C clears inactive enemy draw flags and retains multiply-register
and WRAM-port order for capsule cleanup. CE21 clears 330 bytes, then conditionally
collects action adjustments, forms a bias, invokes the original action dispatcher
and stores a result. CF68 visits eleven stride-30 records via CFAB and negates
the original 56 adjustment words. CFAB reads each source and destination live,
including aliases. D080 is an actor-flag lookup, not population count.

D94F builds the original 30-byte record through the B8 long pointer and copies
it using WMADD/WMDATA. The last buffer byte remains untouched as in the ROM.
D9F0 marks matching three-byte turn records until zero; 937D removes the first
matching actor from twelve records and shifts the rest. A deep D9F0 scan may
overwrite the live caller return word: both semantic and actual consumer tests
preserve that original behavior, including the RTL return bank.

All entries require their original PB, DP0, binary arithmetic and S1F00..1FFC;
DB remains unrestricted. Most enter M1X16, CFAB enters M0X16, F15B accepts either
M with X16, EBF4/CE21 accept all native M/X and restore the saved status. Real
mixed two-/three-byte call and return frames stay intact. D9F0 hands off at
85:D9FA after 4,096 records with its PHA/PHY still live; the proof checks the
4,097th occurrence of that PC and 64 subsequent original instructions. No
CF68 loop budget is needed: its actual CFAB child preserves X and visits eleven
records. Earlier private planning assertions about popcount/CF68 are superseded.

One shared ROM checkpoint per side: 49,152 normal states, 12,288 original-return
states, 768 compositions, 448 forced child unwinds, 768 live-alias states and
four dedicated queue budgets = 63,428 direct and 63,428 actual-consumer states.
The normal group reaches 2,016 queue continuations; the return group reaches
504 continuations plus 5,120 real RTS and 6,664 real RTL returns. Alias checks
add 512 real RTS and 256 real RTL returns. Every continuation checks another
64 original instructions. Full CPU/WRAM and every ordered data/MMIO access,
multiply/divide/Mode7/WMADD device state agree. Opcode-fetch traffic is excluded.

The 26-function actual consumer closure uses all twelve production wrappers,
including nested consumers for the new inventory/adjustment/bias/result helpers.
Fourteen original JSR/JSL service boundaries and 210 actual redirect probes
pass. Original CE6F/D0CD, portrait/upload and actor-address children remain
explicit services in this proof. This is not proof of their internal bodies or
of frame/NMI/pixel timing. The complete requested caller contracts are verified.
1,572,960 semantic and 1,577,817 actual pre-access guards pass, including seven
null callbacks and 4,864 emulation contexts. Wrong PB/DP/width/decimal/stack
contexts remain unchanged and hand off before memory access.

An alias test exposed a private fixture error: its rewritten-return expectation
assumed an RTS bank for an RTL. Correcting the fixture to read the live third
stack byte agrees with the unchanged production return helper. Only affected
actual alias cases and outstanding groups were run after that correction;
unchanged normal/return/composition/unwind proofs were retained. Public source
formatting is token-identical. No ROM file is modified or copied.

Eight WRAM locations are catalogued; F60C is a base marker, not a new disjoint
330-byte allocation, since it overlaps the existing F65B marker. The two new
source units are explicitly listed in the standalone CMake build. All twelve
functions have native bindings only after the combined proof. No duplicate
table/inline labels or bare return stub were counted as new functions.

Full decomp-verify and gameplay/pixel/timing replay are deferred. No whole-ROM
completion, current global interpreter-hit percentage or speedup is claimed.
The separate read-only runtime sample reached 7,444 frames / 4,109,163
interpreted instructions; protected sound/SPC/frame handshakes dominate that
bounded sample. It is guidance for research, not a global performance census.

Fresh public checks each: 384 normal, 192 returns, 96 compositions, 28 child
unwinds, 48 aliases and one queue budget; all 210 actual redirects, all guards
and twelve C++ symbols pass. Six effective heap-ROM faults and two production
stack/frame faults are detected. Private matrices are reused only for token-
identical sources; original services and their limitations remain explicit.

One combined GCC/Clang/metadata/isolated Windows Release checkpoint PASS;
Windows 272.59s. Unchanged resource CLI executable bytes and unchanged
fixture/dependencies justify reuse of their previous accepted CLI test results.
No repeated full semantic regression; no push.

# Selection-screen sequence callers - 2026-10-17

Two complete original entries86:81FB/8298 connect the reconstructed selection
screen into its ordered presentation sequences. The existing control unit
shares action records and cleanup code; no duplicate object/transition bodies.
Primary sequence preserves48/528/216/112-frame waits,all object/style/script
arguments,nine interruption checks and its two carry-result returns8288/8297.
Alternate sequence preserves64/100-frame waits,three interruption points and
PHP before cleanup followed by PLP and RTS82C7. The final alternate BCS targets
the same next instruction; cleanup still runs regardless of its carry result.

Admission PB86/DB7E/DP0/binary/M1X16/S1F00..1FFC matches proven callers.
All27 original JSR/JSL sites keep live frames,including the saved status byte
through alternate cleanup. Audio8096CC/809692 already have verified owners;
no sound/SPC implementation is changed. Original poll828B4B,framewait8B48,
NMI and low-level Title handlers remain explicit services in this proof.

15,392direct and15,392actual states per side:8,192normal,2,048realRTS,
4,096additionalupper-stack realRTS,128composed,864childunwinds and64complete
868000root/RTL compositions. Both root mode branches compose the new sequence
callers,transitions,object setup,layout and asset functions. AllCPU/fullWRAM/
everyordereddata/MMIO/math/Mode7/WMADD agree.17-function actual producer
closure requires native invocation without fallback.405redirect probes across
27sites.262,672semantic and262,672actual guards include2nullcallbacks and
2emulation contexts. Cleanup services deliberately vary N/Z/V/I/C to verify
restoring the pre-cleanup status. Ordinary and early-interruption paths agree.

Private/public bodies are token-identical after formatting. One combined
compiler/CLI/isolated Windows checkpoint covers both related callers; no full
regression per entry. No frame skipping,input replacement,ROM modification or
SPC timing optimization is introduced. Full decomp-verify is deferred; no
pixel/NMI schedule/timing/runtime-hit/speedup/whole-ROM completion claim.

Fresh public checks each:64normal,32RTS,16composed,54unwinds,4rootRTL,
64upper-stack realRTS and64additionalDB80rootRTL.405actualredirects,allguards
and2C++symbols pass. Four effective heap-ROM timing/result/PHP/PLP faults
andtwo production stack/frame faults are detected. No ROM is copied/written.

GCC/Clang/metadata/isolated Windows Release PASS; Windows196.24s.
Resource CLI checks reuse unchanged executable bytes and fixture/dependencies.

# Selection-screen transitions and input - 2026-10-17

Five complete original entries86:8305/830F/8339/835B/82C8 reconstruct fade-in,
fade-out,input waiting,object setup and object-completion waiting in C.
Both fade entries share one body.84/C4 meanings follow verified ScreenFade
8086C1. All existing Title object/layer/palette/particle helpers remain owned
by their existing entries; no duplicate bodies or invented callable tables.
Sound,poll,framewait and NMI internals are retained as explicit original services.

Native admission PB86/DB7E/DP0/binary/X16,S1F00..1FFC matches proven callers;
835B accepts eitherM,other entriesM1. Other contexts keep original ownership.
Raw byte descriptor offsets are not clamped or multiplied.835B preserves the
original PHX/PLX and three live object pointers at15BA/15BC/15BE.8339 retains
X=0 underflow. No host sleep,frame skipping or input substitution is added.

Shared fade returns8338;object setup838B;input wait8357/835A;object wait8302/
8304 retain original carries and flags.22 original child sites keep live two-
or three-byte frames. All loops execute at most4096iterations per native
invocation,then continue original ROM exactly at8317/8339/82CE with balanced
local stack and unchanged observable CPU/memory state. Original indefinite
wait behavior remains possible; these bounds do not finish a waiting operation.

Private proof23,600 direct and23,600 actual-adapter states per side:10,240normal,
2,560realRTS,10,240additionalupper-stack realRTS,160composed,352childunwinds,
32complete868000 root/RTL and16budgetcontinuations. Each budget boundary
executes64 original continuation instructions on both sides (1,024 per side).
AllCPU/fullWRAM/everyordereddata/MMIO/math/Mode7/WMADD agree.20-function
actual producer closure requires native invocation without fallback;330
redirect probes across22new sites. Guards656,679semantic/656,680actual include
5nullcallbacks,all255otherDBbanks and6native-width emulation contexts.

A private fixture initially misclassified a native8339 continuation at its
entry PC as fallback. It now requires no prior child calls for that decision.
Production code and semantic C did not change. Earlier normal/return/root/
unwind cohorts are retained; only failed budgets and added continuation/
upper-stack profiles were run. New tail replay switches the fetch observer
only during original continuation instructions; ordinary paths are unchanged.

Public library/actual-producer wiring and compiler evidence appear in the
sealed receipt. No semantic overrides. One combined compiler/CLI/Windows
checkpoint covers the five-entry batch. Full decomp-verify is deferred.
No pixel/NMI-schedule/SPC-timing/speedup/runtime-hit/whole-ROM completion claim.

Fresh public checks each:160normal,80RTS,40composed,44unwinds,4rootRTL,
4budgetcontinuations,160upper-stack realRTS and64additionalDB80rootRTL.
330actualredirects,allguards and5C++symbols pass.5heap-ROM and2stack/frame
producer faults are detected; no ROM files are modified/copied.

GCC/Clang/metadata/isolated Windows Release PASS; Windows197.44s.
Resource CLI checks reuse unchanged executable bytes and fixture/dependencies.

# Selection-screen control - 2026-10-17

Five complete original entries86:89FD/87D3/8865/8784/81D0 are reconstructed
and selected through ordinary native bindings. New menu_select_control.c
preserves sprite-status clearing, scene-state clearing, raw HDMA descriptor
selection, three map copies and the complete carry-result controller.
Existing TitleCopyTileRows8740 and asset helpers are reused, not duplicated.
Source names follow existing SelectScreenNmi; exact scene identity is unclaimed.

PB86,DP0,binary,anyDB,S1F00..1FFC.89FD accepts all nativeM/X and restoresP
throughPHP/PLP, including X8 truncation;8865 accepts eitherM withX16 and
exitsM1X0;other three requireM1X0. Four fixed RTS8A16/87DF/88A5/87D2;
controller returns81F7 or81FA according to original carry. All14 original
child frames remain live; unwinds stop at their exact original site. Slot
indices remain raw bytes, with no invented descriptor bounds. The existing
868000 root keeps its original three-byte RTL and complete setup call graph.

New canonical RAM7E15B8 is title_animation_frame. Title8695 increments it;
SelectNmiRedraw87E0 adds its low byte to1800 VRAM. It is not a row count.
15B9 remains the existing scene-reused field_recovery_object_updates byte;
no invented choice-state or high-upload meaning. No Title algorithm changed.

Private proof26,208 direct and26,208 actual-adapter states each:20,480normal,
5,120 realRTS,320composed,224childunwinds,64full-root/realRTL. Every CPU
field,fullWRAM and ordered data/MMIO/math/Mode7/WMADD agree.20-function
actual producer closure must invoke native entries, with no fallback.210
redirect probes cover14new child sites. Guards655,399semantic/657,700actual
include3missingcallbacks and2,304emulation contexts. Full actual input,
framewait and NMI internals are not part of this proof:8305/81FB/8298/8B48
remain explicit original services, including their side effects and frames.

Private/public semantic tokens and producer bodies are identical. Fresh public checks per side:160normal,80RTS,40default-composed,28unwinds,
4rootRTL;320additionalDB80composed and64DB80rootRTL;210actualredirects;
allguards and5C++symbols pass.5heap-ROM faults and2stack/frame producer
faults are detected. No ROM files are modified or copied. One combined compiler/CLI/
Windows Release checkpoint covers this batch; unchanged large private
cohorts are retained. Full decomp-verify is deferred, not reported passing.
No timing/pixel/speedup/interpreter-hit/whole-ROM completion claim.

The first build checkpoint exposed the new source missing from the explicit
CMake list. Adding that unit fixed the five-symbol public/Windows link failure.
Only affected incremental compiler/link gates were repeated; the unchanged
private semantic/producer matrices were retained. Initial logs are archived.

GCC/Clang/metadata/isolated Windows Release PASS; Windows67.8s.
Resource CLI checks reuse unchanged executable bytes and fixture/dependencies.

# Selection-screen setup reconstructed - 2026-10-17

Five original routines now have complete C entry contracts:86:80DB graphics,
8157 palette,817A maps,8033 display setup and8000 complete setup/caller.
New code reuses existing resource/source/transfer helpers. Three staged
graphics resources retain their original destinations and lengths; three
map resources retain the single initial bank write. Palette preserves both
256-byte MVNs and DB stack accesses. Display setup preserves ordered PPU
writes, sixteen scroll-shadow bytes and four2048-byte tilemap clears.
Six missing official register-name constants were added without changing
existing definitions. There is no copied algorithm or title/intro claim.

PB86,DP0,binary,anyDB. Four helpers requireX16/S1F00..1FFC; palette accepts
either incomingM, other helpers requireM1. Root forcesM1X0 and accepts all
four incoming nativeM/X modes withS1F20..1FFC. Four helpers return at actual
RTS8156/8179/81A8/80DA;root uses actualRTL8032. Metadata describes ordinary
M1X0 calls. Parent APIs reject missing child callbacks before guest access.
All nineteen original child sites retain their two-/three-byte frames.
Input/selection8681D0,framewait868B48 and installed81A9 NMI remain their
current original/native owner services. No scheduler or renderer replacement.

One shared checkpoint combines14768 direct and14768 actual-adapter states:
10240 normal,3584 real returns (2560RTS/1024RTL),320 composed callers and
304 exact child unwinds per side,plus320 additional composed states per side
with callerDB80 (default compositions useDB86). All nineteen sites also pass285 actual
redirected-return probes. CPU,full WRAM,every ordered operand/data access,
MMIO and live math/Mode7/WMADD state agree with original ROM execution.
Existing decompressor/upload/OAM/request bodies and the new setup routines
are composed with original children. Nested upload wait stays an explicit
service; no native wait-body, gameplay timing or NMI replay is claimed.
Actual nineteen-function producer requires native invocation and exercises
paired/unpaired frames, root widths, DB preservation and RTL return banks.

655432 semantic pre-access guards include four missing callbacks;657732
actual unpaired guards include2304 emulation cases. The first three entries'
cohorts are reused with unchanged relevant bodies/guards; only the two new
entries' extension cohorts were added. An initial RTL fixture consumed its
bank without restoringPB; fixed in the harness. A shadowed local variable
also needed correction. Neither repair changed semantic C. Incomplete
attempts are not counted as passing. Full decomp-verify is deferred for this
scoped setup/caller batch; GCC,Clang,metadata and isolated Windows Release pass (Windows249.2s).
The shared header adds six constants only; CLI checks reuse byte-identical
executables with unchanged fixtures/dependencies.

Fresh public-library checks per side:160 normal,80 real returns,40 default
compositions,38 child unwinds,320 DB80 compositions;285 actual redirect
probes,all guard cases and five C++ link symbols pass. Five effective heap-ROM
faults and two production stack/frame faults are detected. Public test-bank
parameter defaults86; unchanged earlier cohorts are retained rather than
repeated. No semantic body changed for that added caller-bank coverage.
ROM is read in place, never copied/exported into proofs or artifacts. No
PPU/platform/owner build/menu-screen WIP changes. No pixel/timing/hit-rate,
whole-ROM completion or speedup claim.

# Original tile drawing/upload entry contracts - 2026-10-17

Four additional callable original entries share their existing C bodies:
83:9D88 Cave tile block;86:D1A1 world-map upload queue;86:D1E8 column upload;
86:D271 two-pass block upload. Their old JSR/RTS wrappers and call sites remain.
No duplicate algorithms or inline-fragment bindings were introduced.

All require native binary mode, DP0,X16,S1F00..1FFC; Cave PB83/DB7F/M1;
world PB86 and DB register mirrors or7E. Queue requiresM1; column and block
forceM0 and also accept incomingM0. All completed paths returnM1 at original
RTS9DE3,D1E6,D26A,D2CF. Metadata records ordinaryM1X0 calls. Original byte
queue/row counters retain zero-to256 underflow and both DMA passes. Original
size subtraction, source-bank reads, VRAM strides and carry propagation remain.

Cave block dimensions are read dynamically from the bound ROM.63 of256
records have zero dimensions. Word underflow can mean65536 iterations per
axis; destination absolute,Y can also cross7F into bank80 WRAM mirrors and
overwrite DP loop counters. Preserve these behaviors. The new entry executes
literal setup then hands off at83:9DAE for zeros or a conservative last-write
bound aboveFFFF. Bound includes the original source-add carry in row advance.
No extra bus read, clamp, skipped loop or fabricated empty rectangle. The
legacy wrapper passes no handoff and retains the complete original algorithm.
These cases are explicit original continuation coverage, not full native loops.

Shared checkpoint:8192 normal comparisons per direct/actual-producer side,
1024 coordinate/type edges each,1024 return-profile states each (1012 actual
RTS returns and12 original continuations),28 raw queue counts
each and1536 descriptor aliases each:11804 per side.694 Cave cases per side
compare literal prefix plus64 instructions of original continuation. Aliases
include DP scratch, queue, guest stack, PPU/WMADD/DMA and end-of-bank words.
Queue includes both actual child sites/legacy frames. Compare complete CPU,
full WRAM,every ordered operand/data access,MMIO and live math/Mode7/WMADD
device state. Four leaves use the actual16-function production adapter closure;
native invocation is mandatory, with paired/unpaired return and tail contracts.
524954 semantic and525600 actual pre-access guards include646 emulation cases.

Fresh public-library checks:128 normal,64 returns,1024 edges,7 counts and48
aliases per side, all unsupported-context guards and four C++ link symbols.
Four effective heap-only ROM faults and two production stack/frame faults are
detected. No ROM was copied, exported or added to any artifact. The new alias
cohort extends the fixture; prior unchanged cohorts retain their first passing
driver revision. Adding stddef.h fixes NULL portability without changing logic.
The initial synthetic Cave trace ceiling exposed the raw bank-crossing case;
no earlier incomplete run is reported as green.

GCC,Clang,metadata and isolated Windows Release pass (Windows216.8s).
Resource CLI checks reuse byte-identical executables and unchanged fixtures.
Only failed standalone compiles were repeated after the missing header fix. Full decomp-verify, complete
floor replay and in-game replay are deferred for this scoped helper batch.
DMA payload pixels, NMI scheduling, current interpreter hit rate, whole-ROM
completion and speedup are not claimed. No PPU/platform/owner-build changes.

## Original Cave placement and random entry contracts - 2026-10-17

Nine original JSR entries are verified and natively bound:83:99C8 resolves
tile positions;99D3 probes the start/placed-object area;9A1C places an object;
9A6B/9A9F probe nearby/exact chest positions;9ABC tries placing a room chest;
9E1B/9E11/9DE4 draw a scaled random byte, doubled index and wrapped mean.
Existing helpers and new entries share their algorithm bodies. The already
frame-free chest-placement/proximity bodies are reused directly. Legacy
site-based helpers retain all original JSR/JSL/RTS frames and interfaces.
These entry contracts do not represent nine wholly new gameplay algorithms.

All require native execution, PB83, DP0, binary arithmetic, M1 and S1F00..1FFC.
The five object/chest routines require DB7F and X16. Tile-position and random
helpers ignore DB and accept either native X width.99C8 returns M0; the
others return M1, retaining index width. Metadata records ordinary X16 modes.
ChestAt has distinct success9AB6/miss9ABB RTS sites; its shared body reports
the actual site from the carry result.99D3 has only RTS9A1B:99F4 branches to
the common PLX at9A1A. This was confirmed before reconstruction.

Original behavior stays literal. Object count zero still probes the first
slot; chest count zero probes none. The wide-object branch increments its
loaded size byte before comparing column, rather than substituting a column
calculation. Neighbour probes use original asymmetric offsets(-1,+2).
Raw word counts, capacity checks, list wrapping and data aliases are retained.
Zero random limits still advance the RNG and access Mode7 registers. Signed
Mode7 factors and the mean's wrapped byte addition before LSR are preserved.
No range clamp, probabilistic shortcut or original-game bug fix was added.

Both semantic and actual24-function producer layers pass20,480 normal states
across nine leaves and the existing9D11 complete cell-picker caller;2,304
coordinate/limit edges;2,560 real RTS return compositions;and100 raw-count
states across five placement/probe families. Count cases include0,1,7,8,19,
20,255,256,FF00,FFFF, including empty-slot collision and long wrapped scans.
Random factors include raw values>=128, varied RNG indices/refills, incoming
Mode7 latch state, both admitted X widths and every DB for independent leaves.
All groups compare CPU, full WRAM, every ordered data access, ordered MMIO
and live multiplication/division/Mode7/WMADD state. The existing9D11 caller
must reach9E1B; no whole-floor replay or gameplay runtime-hit claim is made.

Admission passes1,180,991 semantic and1,183,044 unpaired actual cases,
including2,053 emulation rejections. Every production-ABI invocation rejects
fallback. Public source formatting preserves private C tokens. Fresh public
library/actual producer wiring, nine C++ symbols, nine effective ROM faults
and two wrong-frame adapter controls PASS. The frame fault is confined to
the new placement-entry adapter; the existing grid adapter is unchanged.
GCC, Clang, metadata and isolated Windows Release pass (Windows188.5s).
Byte-identical CLI checks are reused. Compiler and CLI results remain scoped
in the receipt. Full regression, whole-floor
replay and new gameplay/audio tests are deferred, not reported as passing.

Totals after acceptance:1,639 verified entries,1,624 ordinary bindings,
752 documented WRAM locations and466 semantic C units. Fifteen verified
entries remain deliberately unbound. No whole-ROM percentage or speed claim.

## Original Ancient Cave grid entry contracts - 2026-10-17

Fifteen complete original JSR routines now have verified native entry contracts:
83:9B18/9B27 clear visit bits and count occupied cells;9B3C/9B48 resolve
stair/cell coordinates;9B62/9BDD traverse and merge rooms;9CA0 fills room
rectangles;9CCB/9CD6/9CF3/9D68 resolve cell/block/tile offsets;9D11/9D23
pick/find cells;9D39 appends room links;9D46 replaces an upper tile index.
Their existing internal helpers and new entries share the same algorithm
bodies. Old helpers keep their original JSR/RTS frames and interfaces.
These are new independently verified entry contracts, not fifteen newly
discovered gameplay algorithms. No new semantic C unit was added.

The duplicated9B48 coordinate implementation in cave_header.c was removed.
The existing verified9B44 RTL wrapper calls the original CaveCellPosition
helper with site9B44, retaining the exact9B46 guest JSR return word. Both
the wrapper and the complete8E:B847 map-header caller are verified with the
shared body. Existing metadata and bridge admission for these callers remain
unchanged. M0 input is outside the existing9B44 contract.

All new entries require native execution, PB83, DP0, binary arithmetic,
X16 and S1F00..1FFC. Most require M1;9D46 requires M0;9D68 forces M0 and
accepts either incoming M width. Grid/list/tile entries9B18/9B27/9B62/9BDD/
9D11/9D23/9D39/9D46 require DB7F. Other helpers use DP or long addressing
and are DB independent.9D46 returns M0; the remaining entries return M1.
MergeRoom returns at9BED or9C9F: its one-cell branch at9C7B is BRA9C9F,
not another RTS. FindCell returns at9D33 or9D38. All other return sites are
recorded in the disassembly and RETURNS.json receipt inputs.

Original quirks remain literal: a zero rectangle width/height underflows
its byte counter;256 occupied cells overflow the byte count to zero; room
worklist/link aliasing, visited flags, unmasked coordinates, RNG refill and
carry propagation are retained. No empty-loop substitution, clamping or
host geometry algorithm was introduced.

The shared checkpoint passes17,408 direct and17,408 actual producer states
across15 new entries and the two existing RTL callers. Both layers also pass
960 address/counter edges,2,176 real RTS/RTL return compositions,24,576
arithmetic cases across six coordinate helpers, and384 targeted grid states.
The grid group covers empty/full/visited/checkerboard/single-cell/row/corridor/
FF layouts, including256-cell count and visit-counter overflow. These groups
are reported separately, not claimed to be unique or exhaustive states.

All15 entries and the9B44 wrapper compare CPU, full WRAM, every ordered data
access, MMIO and live multiplication/division/Mode7/WMADD registers. The
complete map-header caller compares CPU, full WRAM, ordered MMIO and those
devices; no complete map-header all-data-read-order claim is made. The two
RTL callers must reach9B48. Actual30-function producer bodies are extracted
without semantic overrides; every invocation rejects interpreter fallback.
Admission passes1,968,239 semantic and1,970,295 unpaired actual checks,
including2,056 emulation rejections.

Original normal/edge/return/arithmetic results retain their accepted driver
revision. The later grid-only driver adds a conditional seed profile and
reuses unchanged relevant semantic/ABI objects, headers, compiler options
and dependencies. Only its384 new cases per layer were executed; unchanged
cohorts were not rerun. Public wiring checks exercise the final test driver.
Initial unused CPU-helper parameter warnings and one missing test include
were fixed before the relevant checks passed. The already-green ABI guard
matrix was retained after the semantic test include repair.

Fresh public-library/actual-producer wiring,15 C++ symbols,15 effective ROM
fault controls and two wrong-frame adapter controls are recorded separately.
GCC, Clang, metadata and isolated Windows Release pass (Windows190.19s).
Byte-identical CLI checks are reused. Compiler and CLI results remain scoped
in the receipt. Full regression, whole-floor
replays and new gameplay/audio tests are deferred, not reported as passing
on this revision. No current interpreter-hit, speedup or timing claim.

Totals after acceptance:1,630 verified entries,1,615 ordinary native bindings,
752 documented WRAM locations and466 semantic C units. Fifteen verified
entries remain deliberately unbound; these counts are not a whole-ROM census.

## Original object motion and animation helpers - 2026-10-17

Seven complete original JSR entries share the existing object VM algorithms:
83:E1AD/E1B9 add display offsets, 83:EE9D/EEA9 add fine positions, 83:EED8
reads a random offset, 83:ED63 selects a low-nibble subcommand, and 83:EE0E
copies animation state. Existing callers retain their original stack frames.
No new C unit or duplicate algorithm was introduced. The new entry contracts
expose previously internal semantic bodies to original generated callers;
they do not represent seven wholly new gameplay algorithms.

All require native execution, PB83, DP0, binary arithmetic and S1F00..1FFC.
Position entries require M0 and ignore DB; the others require M1 and admit
register-mirror DB banks or7E. Both native index widths are supported. Only
EED8 exits M0; the others exit M1. Index width is retained. Metadata describes
the usual X16 convention; additional accepted widths are explicitly verified.
Return sites are E1B8/E1C4/EEA8/EEB4/EEEB/ED6C/EE47 respectively.

The shared subcommand helper now uses the original index-width-aware INY.
The previous internal IncrementY16 differed from ROM for X8,Y=FF. Existing
X16 VM callers keep their behavior. Literal long-indexed accesses, RNG refill,
MMIO multiplication, signed-byte high-byte quirks, data aliasing and writes
that overwrite a guest return word are retained. The dispatch-table lookup
remains after the original RTS, outside the ED63 entry body.

Shared ROM verification passes 26,624 direct and 26,624 actual producer states
across seven leaves and six complete object VM families: F6/F7/FB/FC/F2/F1.
Each complete caller must reach its affected helper. Both layers additionally
pass 28,672 raw-address/alias/stack/MMIO edge states, 3,328 real RTS return
compositions and 1,536 complete caller states with script banks00/80/83/7E.
Leaf checks compare CPU, full WRAM, every ordered data-bus access, ordered
MMIO and live multiplication/division/Mode7/WMADD state. Complete callers
compare CPU, full WRAM, ordered MMIO and those devices; no complete-caller
all-data-read-order claim is made. Actual return assertions compare rewritten
targets against the live guest return word; fallback is forbidden throughout.

Admission passes 917,934 semantic and 920,756 unpaired actual producer cases,
including2,822 emulation rejections. The actual21-function producer closure
uses only read-only entry/return observations. The initial X8 mismatch was a
real shared-helper fidelity defect, corrected before acceptance. Initial
caller fixtures ended F2/F1 with a despawn instruction requiring unrelated
services; the accepted fixtures use the original41 wait instruction. One
signed/unsigned warning in the guard fixture was corrected. Failed initial
logs are retained and excluded from passing counts.

Fresh public-library/actual-producer wiring, C++ linkage, seven effective ROM
fault controls and three actual adapter fault controls are recorded separately.
GCC, Clang, metadata and isolated Windows Release pass (Windows392.27s).
Byte-identical CLI checks are reused. Compiler and CLI results remain scoped
in the receipt. Large private cohorts are
reused only for their unchanged relevant C/producer/harness scope. Later
read-only return counters do not imply a full rerun of those cohorts.
Full regression and new gameplay/audio replay are deferred, not reported as
passing on this revision. No interpreter-hit rate, speedup or timing claim.

Totals after acceptance:1,615 verified entries,1,600 ordinary native bindings,
752 documented WRAM locations and466 semantic C units. Fifteen verified
entries remain deliberately unbound. These counts are not a whole-ROM census.

## Menu pointer, numeric and branch operands — 2026-10-17

Four original subroutines are verified and natively bound: 80:8BA7
Lufia2MenuReadPointerOperand, 80:8BB2 Lufia2MenuReadOffsetPointerOperand,
80:8C6C Lufia2MenuLoadNumberBytes and 80:8C9E Lufia2MenuReadBranchOperands.
Their original RTS sites are 8BB1, 8BC9, 8C9D and 8CFB. Existing menu callers
share the original bodies. The number loader's existing void caller still
executes the full original algorithm. No pointer or numeric algorithm is copied.

All entries require PB80, DP0, binary arithmetic, X16, register-mirror DB or
7E and S1F00..1FFC. The number loader requires M1; the other three accept M0
and M1 because their first instruction is REP20. All return in M1X0. Metadata
records the ordinary M1X0 convention. Long string pointers, short indirect
pointers, bank carries, scratch aliasing and original flags remain literal.

A zero numeric format never obtains a carry from the original shifting size
byte. The new entry transfers at 8C7B after the original three clears, format
load and LDY0, before the first iteration. Its legacy void caller keeps the
original endless behavior. This is an explicit original continuation, not a
clamp or a bug fix. Both layers compare the complete prefix and 64 original
loop iterations in 1,024 states. Nontermination is observed only through that
bounded continuation; no completed return is claimed.

Malformed signed formats 04/0C/14/1C with a negative final byte execute the
original FF fill until Y wraps to 3. Thirty-two states per layer cover the
four formats, DB7E/83 and all four selected stack edges. They preserve every
write, CPU state, full WRAM and final multiplier/divider/Mode7/WMADD state.
Every such case rewrites the guest return word. The actual producer additionally
requires the exact rewritten target and RTS source for paired and unpaired
returns; no cached target may silently be accepted. The bounded internal MMIO
buffer intentionally overflows for mirror-bank fills. Its paired overflow and
prefix are compared, while the complete external ordered bus stream proves
all remaining MMIO accesses. Normal cases still reject unexpected overflow.

The combined private proof passes 16,384 direct and 16,384 actual producer
normal states, another 16,384 pointer-edge states per layer and 1,024 forced
returns per layer. All leaf cases compare CPU, full WRAM and the complete
ordered data-bus stream. The 16-function producer closure uses actual adapter
bodies with read-only entry/return observations; every invocation must remain
native. Admission passes 524,825 semantic and 525,728 unpaired producer cases,
including 903 emulation rejections. Groups are separately reported, not
summed as unique states.

Four complete menu-string caller families additionally pass 4,096 standalone
ROM states: numbers, direct member-name pointers, offset member-name pointers
and branch operands. Every caller must actually reach the affected helper.
These checks compare CPU, full WRAM, ordered MMIO and device state; no complete
data-read-order or whole-parent producer-ABI claim is made for these old VM
bodies. An initial fixture incorrectly composed their RTL as an RTS. The
fixture was corrected to the original three-byte frame and bank; that failed
run is excluded. Semantic source did not change for this fixture correction.

Private/public semantic tokens match. Fresh public-library and actual-producer
wiring, both entry widths, return, pointer-edge, zero-format, finite-fill,
admission, standalone-caller and four-symbol C++ checks are recorded in
menu-operand-entries-next/ACCEPTANCE_RECEIPT.json. Four effective heap-only ROM
faults plus wrong incoming-stack, return-size and rewritten-target producer
faults must fail normally. Large accepted private cohorts retain their exact
scope; the stronger rewritten-target assertion is separately exercised on
the 32 affected producer cases and fresh public fill cases.

Compiler and byte-identical CLI outcomes are recorded separately. No full
decomp regression, new gameplay/audio replay, timing or interpreter-hit
measurement is claimed. ROM files, saved games, editor and rendering patches
are unchanged. Totals after acceptance: 1,608 verified entries, 1,593 ordinary
native bindings, 752 WRAM locations and 466 semantic C units. Fifteen verified
entries remain intentionally unbound; these counts are not a ROM completion
percentage or a measured speedup.

## Actor orbit, radius and signed-operand entries — 2026-10-17

Four original callable entries are verified and natively bound: 83:DE76
Lufia2ActorBuildOrbitOffsets, 83:DE9F Lufia2ActorScaleRadius, 83:FAF4
Lufia2ActorReadSignedOperand and 83:FB05 Lufia2ObjectSignExtendNibble. Each
shares an existing semantic body; old callers retain their original JSR and
RTS frames. No allocation, trigonometry or sign algorithm is copied. Tables
83:DD18/80:84BF and inline projection fragments 83:E73A/E7C2 were excluded
from new function counts and bindings.

All four require PB83, DP0, binary arithmetic and S1F00..1FFC. Three enter
with M1; FAF4 sets M1 before reading its operand and also accepts M0. All
support both native index widths. FB05 is independent of DB; the other
three admit register-mirror banks or 7E. Exit M is zero and index width is
retained. Metadata records the ordinary M1X0 -> M0X0 convention; additional
admitted widths are tested, not silently narrowed by the bridge.

Original return sites are DE9E, DEBC, FB04 and FB11 respectively. DE76
retains both original sine/cosine calls, scale calls and DP54/56 stores.
DE9F retains Mode-7 register writes, its original RMW of MPYL and the
original carry into the result. DB7E therefore accesses WRAM rather than
being replaced by a host multiply. FAF4 keeps TDC before the indexed operand
read. FB05 preserves the original unmasked-input behavior: positive values
retain the incoming high accumulator byte. No sign, carry or range quirk is
optimized away. A disassembler needs DE9F's actual M0 exit to decode the
caller's word stores correctly; its default external-call guess is not proof.

The combined private ROM proof passes 36,864 direct and 36,864 actual
producer states across four entries and five complete affected caller
families: secondary F2/F6/F7/F8 and the object F6/FB route. Every caller
must actually reach its affected helper. Both layers also pass 2,304 original
return compositions. A separate systematic arithmetic group passes 144,640
states per layer: 12,544 angle/radius-edge combinations, all 65,536 A16
inputs for scaling, 1,024 signed-byte/all-MX cases and all 65,536 A16 inputs
for nibble extension. Groups are separately reported, not claimed unique.

Leaf checks compare CPU, complete WRAM, all ordered data-bus accesses,
ordered MMIO and live Mode-7 state. Complete VM callers compare CPU, complete
WRAM, ordered MMIO and live device state; no all-data-read-order claim is made
for those pre-existing VM bodies. Each layer additionally passes 1,280 caller
cases with original script banks 00/80/83/7E. This explicitly tests real
Mode-7 accesses as well as original WRAM-mapped accesses. The first normal
caller cohort used script bank 7E; the added bank group fills that concrete
coverage gap without repeating unchanged leaf cohorts.

Admission passes 524,695 semantic and 526,239 unpaired actual producer
checks, including 1,544 emulation rejections. The 19-function producer closure
contains actual bridge bodies with read-only entry/return observations. Every
invocation must use the native path; fallback is rejected immediately. No
existing timing fence, renderer, sound engine or platform layer changed.

Private/public runtime tokens match. Fresh public-library and actual-bridge
wiring, admission, return, arithmetic, bank-context, C++ linkage and effective
fault controls are recorded in actor-scaling-entries-next/ACCEPTANCE_RECEIPT.json.
Large unchanged private cohorts are reused for their exact scope. GCC, Clang, metadata and isolated Windows Release pass (Windows393.42s).
Byte-identical CLI checks are reused. Compiler and CLI results remain scoped
in the receipt. Full decomp regression, new gameplay/audio
replay, timing measurements and a measured interpreter-hit reduction are
deferred. Original instruction bytes and saved games remain unchanged.

Totals after acceptance: 1,604 verified entries, 1,589 ordinary native bindings,
752 documented WRAM locations and 466 semantic C units. Fifteen verified
entries remain deliberately unbound. These entry counts do not measure a
whole-ROM completion percentage or a gameplay speedup.

## Actor sprite, secondary-script and event-area entries — 2026-10-17

Three original entries share their existing semantic bodies: 83:AA50
Lufia2ActorAllocateSprite, 83:DFFD Lufia2ActorLoadSecondaryScript and 80:E92A
Lufia2FieldResolveEventArea. Existing sprite and event-area callers retain
their original pushed frames. No copied allocation, script or area algorithm
is introduced. Already-bound 80:E912/EA09 were excluded from this batch.

AA50 accepts native M/X widths, PB83, binary arithmetic, DP0, register-mirror
data banks or 7E and S1F20..1FFC. PHP/PLP restores the original status; the
normal return instruction is AA7C. A zero slot count transfers at AA5F before
the original JSL83AB7C, leaving PHP and the original call instruction intact.
The original allocator can loop forever when that count meets occupied slots;
this behavior is preserved, not repaired in semantic C. The old void wrapper
keeps its original behavior. ABE9 changes M to 0 before the original word
store at AA74; a disassembler's external-call width guess is not its contract.

DFFD accepts all native widths and data banks, PB83, DP0, binary arithmetic
and S1F00..1FFC. It shares the existing secondary-script table lookup, long
stores and original RTL E017. X width remains meaningful; metadata records
the ordinary M1X0 call convention, not a restriction to that mode. AA50 has
dynamic exit status. E92A accepts PB80, M1X0, DP0, binary arithmetic, register
mirrors or 7E and S1F00..1FFC. Its four operand paths meet at the single RTS
E99C. The existing list-search work limit transfers at BFBC with live frames.

The private combined ROM proof passes 28,672 direct and 28,672 actual producer
states across the three new entries and four affected callers: A9BA, E15A,
CD88 and E458. It compares CPU, full WRAM and ordered bus accesses. The actual
21-function producer closure preserves existing A9BA admission (M1 only);
the standalone C parent additionally covers M0. No existing guard was widened.
The normal AA50 cohort contains 912 original pre-call boundaries and 3,184
ordinary returns per layer. DFFD returns 4,096 times, including 256 natural
overwrites of guest return frames through the original long script stores.

Each layer also passes 912 zero-count original-instruction continuations:
456 complete finite returns and 456 observations through 64 loop iterations.
The latter are explicitly bounded observations, not claims that an infinite
tail returns. A separate 1,792-state forced-return group per layer includes
57 AA50 boundaries and 32 naturally rewritten DFFD frames. Three original
event-area list-limit states per layer cover all 65,536 iterations. The first
limit fixture stopped at the first occurrence of a repeated PC; only the
fixture was corrected to count the original loop iterations. No ROM file,
save or instruction stream was edited by these cohorts.

Admission passes 393,520 semantic and 395,189 unpaired producer checks,
including 1,669 emulation rejections. The first guard fixture expected a
constant bank for the existing field-condition fallback; it now checks that
adapter's actual incoming-bank protocol. Initial partial/failing fixture runs
are excluded. A missing stddef.h for NULL was found by standalone compilers
and added to both semantic source copies; only affected compiler checks were
repeated. This header addition changes no tested runtime behavior.

Private/public runtime tokens match; only formatting and the explicit standard
header differ from the first private proof. Fresh public-library integration,
actual bridge, admission, C++ linkage and deliberate fault controls are recorded
in actor-area-resource-entries-next/ACCEPTANCE_RECEIPT.json. The large unchanged
private cohorts are reused for their exact scope. Full decomp regression,
new gameplay/audio replay and a measured interpreter-hit or speed claim are
deferred. GCC, Clang, metadata and the isolated Windows Release pass (Windows437.61s).
CLI checks reuse byte-identical accepted executables. Compiler outcomes and
initial failed build diagnostics remain separately recorded in the receipt.
An early public link ran before the new Windows library existed and was
repeated after library creation. One ineffective REP30 fault for already-X16
inputs was excluded and replaced by an effective script-offset fault. No
unchanged successful ROM cohort was rerun for these fixture/tooling repairs.

Totals after acceptance: 1,600 verified entries, 1,585 ordinary native bindings,
752 documented WRAM locations and 466 semantic C units. Fifteen verified
entries remain deliberately unbound. Additional entry coverage is not a
whole-ROM completion percentage or a measured interpreter reduction.

## Field selection and palette/wave child entries — 2026-10-17

Three original entries are verified and natively bound: 83:ACEF
Lufia2FieldSelectSessionSetup, 83:AEED Lufia2FieldAdvancePaletteCycles and
83:AF54 Lufia2FieldStepWaveTable. They reuse existing semantic bodies.
ACB7 now calls an unguarded shared selection helper after its original prefix;
its original entry and AD23 keep their previous consumer admission rules.
The palette caller retains its original JSR frame around the shared cycle
body. The wave body was already shared. No algorithm is copied or optimized.

ACEF is the actual JML target after a world-encounter defeat and TCS1FFF.
Its SEP20/REP10/PHK/PLB normalizes widths and replaces incoming DB86 before
selection, map preparation and field entry. The public entry requires PB83,
DP0, binary arithmetic, a child service and S1F10..1FFF; all incoming data
banks and native widths are supported. It never invents an RTS/RTL. Child
unwinds, width fences, map restarts and original field-loop handoffs retain
their state and frames. Only ACEF bypasses the old session bridge DB0/7E/83
whitelist; explicit controls keep ACB7/AD23 rejecting DB86 as before.

The two effect entries require PB83, DB7F, DP0, binary arithmetic, M1/X0 and
S1F00..1FFC. Their original RTS sites are AF53/AFCC. Palette data remains read
through the original A1 bus mapping; the zero-duration chain hands off at
AF11 after its existing 65,536-step host work limit. Wave timer, pointer,
channel-1 writes and data-bank changes remain literal. PPU/runtime engines,
sound and intentional timing-specific interpreter fences are untouched.

The selection proof passes 14,336 direct and 14,336 actual-producer states,
covering all 67 selection/map child sites, every incoming DB and all four
native M/X combinations. It compares parent instructions against original
ROM with explicit controlled child services, including their complete input
registers and pushed frames; it does not claim those service internals were
executed by original ROM. Each layer also passes 67 live-child interruptions,
134 returned-width variants, 512 affected ACB7 parents and 512 arrivals from
actual original 86:9D9E instructions through TCS/JML83ACEF. The reset performs no invented stack clear; nonzero discarded stack bytes
and subsequent original stack writes are compared. Eighteen small discovery states identify the child
cases for the focused edge selectors; they are not added to the normal matrix.
The actual producer adds 1,005 redirect contracts across all 67 sites and
45 distinct missing-child-target contracts. Its closure includes the actual
selection/session adapter bodies without substituting their algorithms.

Effect proof passes 2,048 direct entry states, 1,024 affected AEB5 parent
states and 2,048 actual producer states. Original return compositions cover
512 direct entries, 256 direct parent states and 512 actual producer states.
Terminating zero-duration links additionally pass 64 direct entry/parent and
32 producer states using unchanged original ROM data. Each layer additionally
passes three zero-duration palette-chain limit
states. Only those limit states use synthetic palette data in the heap-loaded
ROM buffer; original instruction banks remain unchanged. Normal ROM cohorts
read the supported ROM in place. CPU, full WRAM, ordered bus accesses and
hardware-register results are compared.

Combined admission checks pass 393,760 semantic and 394,788 unpaired actual
producer states, plus the semantic null-child contract. The producer count
includes 1,026 emulation rejections and two unchanged old-session DB controls.
The initial selection driver reused a battle-adapter redirect-depth expectation;
it was corrected to the actual session adapter protocol before accepting any
producer states. An initial fixture visit limit also interrupted normal actor
installation too early; it now limits only repeated map-entry passes. Those
early partial runs are excluded. Full accepted selection direct coverage is
1,024 final base states plus 13,312 structured branch states.

Private/public semantic tokens match. Large cohorts are retained for their
unchanged private sources, compiler options and linked dependencies. Fresh
public-library wiring covers 64 selection/arrival/ACB7 states, 48 effect/parent
states, 24 effect return states, 16 finite palette-link states, 48 selection producer
states and 56 effect
producer/return states. Public semantic admission is checked again against
the actual library. Four heap-only ROM faults, a wrong incoming-stack adapter
and a wrong effect return-frame adapter are rejected normally; three C++
symbols link and run. GCC, Clang, metadata and one isolated Windows Release
checkpoint pass (Windows 411.25 seconds). Byte-identical CLI results are reused. No complete public
matrix rerun, full decomp regression, new gameplay/audio replay or speed
measurement is claimed for this revision.

Totals: 1,597 verified entries, 1,582 ordinary native bindings, 752 documented
WRAM locations and 466 semantic C units. Fifteen verified entries remain
intentionally unbound. Additional exposed entries are not a whole-ROM
completion percentage or a measured interpreter reduction. Evidence:
field-selection-entry-next/ACCEPTANCE_RECEIPT.json, maintained field-session
selection, palette/wave, admission and C++ link fixtures.

## Field condition helpers and original indirect calls — 2026-10-17

Nine complete original entries are verified and natively bound: 80:E4C5
StoreConditionResult, E55B NegateConditionResult, E132 TestLeaderPosition,
E15A TestLeaderArea, CD8B FindObjectInBox, CD88 FindObjectInArea, E458
FindCellBitsInArea, E78D FindEventConditionList and E566 TestConditionList
(public names carry the Lufia2Field prefix). E78D returns through the original
RTL; the other entries use their original RTS sites, including conditional
return sites in E15A/E458. Original VM callers share these bodies and retain
their own frames. No algorithm is duplicated. The public entry contract is
PB80, M1/X0, binary arithmetic, DP0, S1F00..1FFC and DB00..3F/80..BF or DB7E.
Unsupported entry contexts hand off before guest bus access.

Ordered-bus comparisons exposed two inherited C fidelity defects. Bounds were
read as C function arguments, allowing compiler-dependent order and reading
the upper bound even when the original lower-bound comparison exits. The
shared range helper now performs those reads lazily in ROM order. Unknown
E566 condition-table targets previously handed off before an already-read
indirect JSR. They now hand off at the actual target after pushing the original
E58D return frame. Known leader/cell/object condition targets remain shared C.
These corrections preserve original behavior rather than patching the game.

The shared checkpoint covers 36,864 direct entry states and 36,864 states
through the actual 20-function producer closure. Each layer additionally
passes 9,216 raw-index, bank-crossing, area-underflow and selection edge states,
4,608 original RTS/RTL compositions and 1,024 unknown-target live-frame states.
Admission checks cover 1,180,863 semantic and 1,180,935 unpaired producer
contexts. Fifteen affected event opcodes pass 3,840 direct original-handler
compositions; this caller cohort is not claimed as a separate producer proof.
CPU state, full WRAM and ordered guest bus operations are compared.

Two long-list handoff families also pass two direct and six producer states
after 65,536 original iterations. These use synthetic script data only in the
heap-loaded ROM buffer, with original instruction banks untouched; they are
reported separately from the unchanged-ROM cohorts. Both original list PCs
and live frames remain available to the interpreter. The actual producer
states include paired, unpaired and mismatched host-return contexts.

Initial list fixtures accidentally seeded bank00:8000 instead of WRAM7E:8000;
the address and missing-list sentinel were corrected. Only complete passing
cohorts from the early direct runs are reused: 12,288 storage/negation/position,
16,384 area/object/cell and 8,192 corrected list states. The failed rows and
earlier unstructured list rows are excluded. A separate popup fixture had
the same high-WRAM seed-address issue. Corrected sparse popup values pass
8,192 direct and 8,192 actual producer states against the unchanged accepted
popup implementation. The maintained popup fixture now seeds the intended
WRAM addresses; no popup semantic source changes in this batch.

Private/public semantic tokens match. Full unchanged scoped proofs are
reused; fresh public wiring checks cover 288 direct, 144 return, 32 unknown,
240 opcode-caller and 320 actual producer states. Four heap-only ROM faults
and a wrong return-frame adapter are rejected normally. All nine C++ symbols
link and run. One shared GCC/Clang/metadata/isolated Windows Release checkpoint
passes. Byte-identical resource CLI results are reused; full decomp regression
and new gameplay/audio replay are deferred, not reported as passing.

Totals: 1,594 verified entries, 1,579 ordinary native bindings, 752 documented
WRAM locations and 466 semantic C units. Fifteen verified entries remain
intentionally unbound. Additional public entries into existing C bodies do
not measure removed interpreter instructions, current hit rates or FPS.
Evidence: field-condition-entries-next/ACCEPTANCE_RECEIPT.json and maintained
lufia2_field_condition_entries_verify.c, lufia2_field_condition_callers_verify.c
and lufia2_field_condition_cpp_link_verify.cpp. No ROM/save redistribution.

## Popup records and single-target status ticks — 2026-10-17

Three complete original JSR entries now have verified native bindings:
81:C3EE Lufia2BattleBuildSlotPopup, 81:C477 Lufia2BattleBuildPopupRecord and
81:C652 Lufia2BattleApplyTargetStatusTick. Their original RTS boundaries are
C476,C585,C738. Popup entries require PB81, DB7E, M1/X0, binary arithmetic,
DP0 and S1F00..1FFC. The status entry admits hardware-register mirror banks
00..3F/80..BF instead of DB7E, and requires an explicit pushed-child service.
All three reuse existing algorithms; the original C600 wrapper still pushes
and consumes its own JSR frame around the same target-tick body. No popup,
damage or random-number algorithm is copied.

Expanded ordered-bus proof found inherited popup C fidelity defects despite
matching final CPU/WRAM. Word DEC of the scan count and word INC of the popup
count wrote low/high instead of the original high/low order. Their two local
sites now use the existing width-aware memory-step helper. Global CPU helpers
are unchanged. The parent also read the mode for inactive/unrecognized slots;
it now reads it only for the four original popup types. Slot lookup preserves
the original comparison carry. These repairs reproduce original ROM behavior,
including signed digits, limits and the original output indexing quirks.

The shared direct checkpoint covers 12,288 new-entry states and 8,192 affected
C35F/C600 caller states; the actual 21-function producer closure passes 20,480
states on the same families. Each layer also passes 5,120 raw-slot, digit,
output-wrap and target-mask edge states, 1,536 focused return states and 2,048
target-tick interruption states. Edges include all 256 popup kinds with 13
signed-value cases, raw slot offsets crossing banks, wrapped output positions,
all 128 accepted register-mirror banks and all byte target masks. A zero staged
mask retains its original nonterminating child; the proof interrupts at that
live child entry and does not invent a successful return.

CPU, full WRAM, ordered bus accesses and MMIO results match original code.
Each of the six target-tick child sites is reached and interrupted. Ninety
additional actual producer probes cover changed return PCs, changed stacks,
ancestor skips and LLE unwinds at all six sites. Admission proof rejects
393,878 semantic and 393,902 unpaired producer contexts before memory access;
the null-child guard also rejects before access. The C600 affected proof stops
at its explicit presentation child when requested; it does not claim gameplay
execution of that asynchronous presentation chain.

One first caller fixture used RTS for the original C35F RTL; its trace ceiling
correctly rejected the invalid run. The corrected fixture uses the actual RTL.
Interruption checks distinguish the first and last D9C9 call by original call
site rather than stopping ambiguously at the repeated target address. Fixture
trace guards now use allocated capacity, including the earlier actor-entry
fixture; ordinary comparisons and semantic expectations remain unchanged.

Private/public semantic tokens match. Full unchanged direct/ABI/edge/return/
guard/redirect proofs are reused. Fresh public wiring passes 160 direct,
48 return and 160 actual producer states plus null-child and C++ link/run
checks. Four heap-only ROM faults and a wrong return-frame adapter fail
normally. CMake selects the three entries among 1,570 ordinary native bindings.
GCC, Clang, metadata indexing and one isolated Windows Release build pass;
resource CLI tests are reused only when executable bytes remain identical.
The receipt records exact compiler durations. Full regression and fresh
gameplay/audio replay remain deferred. No new runtime-hit or speed claim.
The following atomic commits share validation of their combined final state;
intermediate commits were not independently tested. Totals: 1,585 verified
entries, 1,570 ordinary bindings, 752 WRAM fields and 466 semantic C units.

## Secondary movement entries and indirect dispatch — 2026-10-17

Four complete original JSR children now share the existing secondary-VM
algorithms through guarded public entries: 83:DA8A StopSecondaryScript,
83:DA71 MarkLeaderScriptStop, 83:DA5B PrepareStepProbe and 83:D9C6
CommitTileStep. Their RTS boundaries are DA99, DA89, DA70 and DA5A.
The existing VM wrappers retain their original frames and wider contexts.
The new entries admit PB83, M1/X0, binary arithmetic, DP0 and S1F00..1FFC.
The probe and commit retain live movement-child frames and unknown targets.
No arbitrary walk continuation is promoted or counted as a whole function.

An inherited C fidelity defect in 83:FB12 was independently reproduced:
DA5B with facing 1 matched CPU/WRAM but generated 12 bus accesses instead of
the original 10. MovementStep read the indirect target then handed back
before the already-completed JMP; its bridge read the same table again.
The semantic helper now reports the actual program-bank:target boundary;
the bridge uses that result without additional reads. The four recognized
targets and original frames remain unchanged. This repairs native fidelity,
not an original game bug. The reference test executes the original JMP
before comparing the independently decoded target and machine state.

Shared verification passes 40,960 direct and 40,960 actual producer states
across the four new entries and affected FB12 helper. Each layer also passes
5,120 all-byte-direction states, 2,560 focused return compositions and 2,048
actor-state-bit/edge-slot states. The latter covers all 256 state bytes at
slots 0/1/31/39, distributed across WRAM banks 7E/7F. An additional 512 actual
FB12 producer states retain X8/X16 and DP0/20/E0 contexts. CPU, full WRAM,
ordered bus accesses and real RTS/RTL compositions match the original ROM.
Poisoned admission checks reject 524,320 semantic and 524,352 unpaired actual
producer states before access. The ABI proof uses 18 exact producer functions;
observations do not alter entry or return state.

The affected-only legacy proof passes 60,416 movement, indirect-jump,
primary/secondary VM, player and 44 primary-handler states. Forty unused
legacy small-bridge registrations trap if invoked; that semantic proof does
not claim their ABI execution. The old --skip-small still runs unrelated
families, so its overly broad run was stopped and is not accepted regression
evidence. The maintained --movement selector restricts the existing cases
to the affected families. Its fresh public-library cohort passes 6,272 states.

Private/public semantic tokens are identical. Full unchanged matrices,
admission and return proofs are reused rather than repeated. Fresh public
wiring checks pass 160 direct states, 160 unknown-direction states, 80 return
compositions, 32 raw contexts, 256 state-bit states, 160 actual producer
states and 32 raw producer contexts. Four heap-only ROM instruction faults,
an extra-table-read bridge fault and a wrong RTS/RTL adapter fail normally.
A C++ link/run probe exercises all four new declarations inside extern C.
CMake selects all four among 1,567 ordinary native bindings.

GCC, Clang, metadata indexing and one isolated Windows Release build pass.
Windows took 420.20 seconds including native CFG regeneration; GCC/Clang
took 58.36/51.14 seconds. Unchanged resource CLI executable bytes retain
their earlier accepted tests. Full regression and fresh gameplay/audio replay
remain deferred. No new interpreter-hit, timing or speed result is claimed.
Combined final-state validation covers the following atomic commits, not
independent validation of intermediate revisions. Totals: 1,582 verified
entries, 1,567 ordinary bindings, 752 WRAM fields and 466 semantic C units.

## Public field C++ linkage — 2026-10-17

The four recently added attribute/area-spawn declarations now remain inside
the field header's existing extern C block. An actual C++ linker probe
reproduces four unresolved mangled symbols against the previous header and
links/runs successfully with the corrected header and the same accepted
library. The maintained probe references all four functions. Preprocessed C
tokens are identical before and after; only blank lines move. Function bodies,
bindings, library bytes and the scoped ROM/ABI proofs are unchanged. Those
results and the existing compiler checkpoint are reused; no full regression
or whole-game rebuild was repeated for this declaration-only repair.

## Whole random area-spawn entries — 2026-10-17

Two complete original children now have verified contracts and native bindings:
80:D533 Lufia2FieldSpawnActorsInArea and 80:D583
Lufia2FieldRandomAreaCoordinate. Their algorithms were private parts of the
field-event actor module. They now share field_area_spawn.c; event opcode 78
retains its original operand prefix and JSL frame around that same body.
The text AFC0 child also selects the new whole-entry binding. No actor-spawn,
offset or random-number algorithm is copied or replaced.

Both supported entries use PB80, M1/X0, binary arithmetic, DP0 and stacks
1F00..1FFC. D533 ends at its actual RTL at D582, while D583 ends at RTS D5A4
with a 16-bit accumulator. Guest return instructions remain guest returns.
The original zero-count underflow means 256 spawn attempts. Slot exhaustion,
stale slot offsets, coordinate wrap, random refills, multiply accesses, bus
ordering and saved actor-slot restoration remain literal original behavior.

The shared ROM checkpoint passes 20,480 direct states and 20,480 actual
producer states, including original random/spawn/offset children rather than
controlled substitutes. Each layer also passes 1,280 focused RTS/RTL return
compositions and 512 raw-slot/exhaustion/index-wrap edge states. Those edges
include long-index crossings into mirrored RAM or ROM and guest caller-frame
overlap. Poisoned admission checks reject 262,160 semantic and 262,176 actual
producer states before bus access, covering unsupported stacks, DP, banks and
modes. CPU, full WRAM and ordered bus/MMIO accesses match the ROM.

The changed field-event prefix and text AFC0 caller each pass 512 actual child
compositions. The event seeds use direct area-point operands E0..FA; FB..FF
have other original value semantics and are not mislabeled direct point IDs.
Another 512 cases compose the actual text and spawn producers with original
children. A failed first nested fixture was traced to surplus heap-only script
seeding at 83:E000, inside an executed actor child. Removing that unused seed
resolved the mismatch; increasing the interpreter ceiling alone did not.
The ROM file on disk and the semantic reconstruction were unchanged by that
fixture repair. A private missing forward declaration was also corrected.

Fresh public-library checks pass 1,280 entry states, 1,280 focused returns,
512 edge states, both admission matrices, 128 compositions for each caller
and 512 nested producer compositions without semantic overrides. Four heap-
only ROM operand faults and a wrong RTS/RTL adapter fail normally. The actual
CMake selection report includes both entries among 1,563 ordinary bindings.
GCC, Clang, metadata checks and one isolated Windows Release build pass.
Windows took 415.56 seconds, dominated by native CFG regeneration and the
dependent rebuild; it was run once for the batch. Unchanged resource CLI
executables retain their accepted byte-identical tests. Source publication
preserves comment-stripped tokens. Full legacy regression and a new gameplay/
audio replay remain deferred, with no new hit-rate, timing or speed claim.
Totals: 1,578 verified entries, 1,563 ordinary bindings, 752 WRAM fields and
466 semantic C units.

## Text camera and map callers — 2026-10-17

Twelve existing internal text paths now use optional consumer child services:
A2B0/B721 rebuild attributes, B341/B34F center and publish layers, B494
publishes layers, AF6B restores scene display, and AFC0/B62B/B6F4/B72D/
B646/B659 perform scene actions, tile copies, redraws and attribute/upload
requests. Both command families retain one shared implementation and their
callback-null APIs, including the original S1F10 stack admission. The text
engine supplies its existing pushed-child service. No new canonical entry,
metadata promotion, binding or algorithm copy is introduced.

The AFC0 tail resumes with its original PLY and next-command branch after
the D533 child. Original DB/Y frames, operand reads and NEXT/YIELD decisions
remain intact. Width, DP, decimal, PB or stack changes hand back at the exact
post-call instruction. DB changes flow through the original caller accesses
and DB restores; the text engine still fences a changed script bank. Actual
frame waits at BF0B retain their live JSR frame and explicit handoff. The
B659 NMI disable/upload/re-enable sequence remains ordered as in the ROM.
No frame-wait acceleration or hardware/audio implementation change is made.

The shared checkpoint passes 54,784 direct states, 11,776 reached whole-text
states, 10,368 actual consumer-producer states and all 330 return redirects
across 22 caller/site groups. CPU, full WRAM, child-entry frames and ordered
bus/MMIO accesses match the original ROM. Another 131,240 unsupported-context
and unknown-selector cases reject before bus access. A 6,144-state composition
fixture also executes the actual layer-scroll, display-register and tilemap-
request children where reached, rather than substituting their return values.
It covers 1,792 real child executions within those caller states.

Fresh public-library checks pass 6,400 engine states, all admission guards,
6,144 composition states and 4,992 actual producer states with all 330
redirects, without semantic overrides. Six heap-only ROM faults are detected
with normal failure exits. The affected callback-null render fixture passes
32,768 states and 65,639 guards; the preceding system/end fixture passes
4,608 states. The initial render invocation accidentally selected guard-only
mode; its corrected no-argument invocation supplies the full affected result.

The first small private proof caught an incorrect newly supplied scene-display
target; ROM bytes and metadata established F2F3 before publication. Fixture
corrections cover heap-only script seeding in ROM banks and simulated RTL PB
restoration. None changes original ROM bytes on disk or accepted semantics.
The compiler checkpoint passes GCC, Clang, metadata checks and isolated
Windows Release (9.25 seconds). Resource CLI executables are byte-identical,
so unchanged accepted CLI checks are reused. Only changed source ranges were
formatted, preserving comment-stripped tokens. Full legacy regression and
new gameplay/audio replay remain deferred. There is no new runtime hit-rate,
cycle-parity or speed claim. Totals remain 1,576 verified entries, 1,561
ordinary bindings, 752 WRAM fields and 465 semantic C units.

## Whole attribute helper entries — 2026-10-17

Two real original children now have verified standalone contracts and native
bindings: 80:EF2F Lufia2FieldMarkVisibleActorCells and 80:EEEB
Lufia2FieldMarkListedCells. Their existing private algorithms are shared with
the attribute-map builder at ED9C; no algorithm is duplicated. Both supported
entries are PB80, M1/X0, binary, DP0, with stacks 1F00..1FFC. Actor occupancy
sets DB80 and marks one or two cells for visible actors; the listed-cell helper
sets DB7E and marks bit 2. Hardware multiply accesses retain their original
bank, width and order. Actual RTL at EF8D and RTS at EF2E remain guest returns.

The list helper has its complete original logic and a bounded continuation:
after 65,536 iterations it hands off at EEF8 with exact CPU/memory state and
live guest frames. The old parent propagates that result without popping its
call frame; successful parent paths retain the same shared body and frame.
The exhaustion proof covers the standalone entry and the real EE75 JSR site,
not an invented normally reachable whole-map state. A preceding valid list's
FF marker makes an all-zero whole-parent exhaustion seed inappropriate.

The shared checkpoint passes 65,536 direct and 65,536 actual producer states,
plus 2,048 focused real-return compositions and 512 whole-parent compositions
through each layer. The focused returns cover every seeded list length and
both ordinary and wrapped list addresses; the initial empty-list-only return
selection was broadened without repeating the unchanged full direct matrix.
CPU, full WRAM, ordered bus/MMIO accesses and actual guest frame behavior match
the ROM. Both semantic exhaustion compositions and an actual producer handoff
match the exact 65,537th EEF8 entry. Admission rejects 262,160 semantic and
262,176 unpaired producer contexts before any bus access, including mirror-bank
fallback PCs, every rejected stack/DP value and unsupported modes.

Another 1,024 real text-end/window-toggle compositions pass directly and through
nested text and attribute producers. This checks the actual DB change to80:
the text parent hands back at 9DB0 when its script bank changes, preserving the
original PLP/PLB exit. Synthetic services that preserve DB do not establish this
contract. No original sound upload, wait, synchronization or poll kernel changes.

Fresh public-library checks pass 1,024 entry states, 2,048 return compositions,
512 parent states, the semantic guards and exhaustion. Actual public producer
checks pass 2,048 entry states, 64 parent states, producer guards, exhaustion
and 1,024 nested real-child text compositions without semantic overrides.
Four heap-only ROM faults and a wrong RTS/RTL adapter frame fail normally.
The actual CMake function-source report selects both verified native entries.

GCC/Clang/metadata checks and one isolated Windows Release build pass. Resource
CLI binaries are byte-identical, so their unchanged accepted CLI tests are reused.
The Windows checkpoint took 410.89 seconds, largely static CFG regeneration for
changed bindings; it was not repeatedly run per entry. Future binding batches
should amortize that cost where dependencies allow. Formatting preserves source
tokens; expanding the named list limit restores the privately proven tokens.
Full legacy regression and a new gameplay/audio replay remain deferred, with
no current hit-rate, timing-parity or speed claim. Totals: 1,576 verified,
1,561 ordinary native bindings, 752 WRAM fields and 465 semantic C units.

## Text system callers and script completion — 2026-10-16

Eight existing internal paths now continue through optional pushed-child
services: music fade/reset/preparation, immediate sound, sound-value/flag waits,
window toggle and the no-caller script-end branch. Seven system commands share
their existing bodies with the unchanged callback-null API and its S1F10 lower
stack bound. The existing text bridge supplies services; no new binding is added.
Original sound uploads, SPC synchronization, polling and timing helpers are
untouched. The sound waits still rewind or continue under their original tests.

The script-end tail at 80:9D69 preserves prompt/window flags, camera-follow
waiting, actor release and the original JSL at 9DAC. The shared camera helper
now takes its actual return address: 9D97 for script end and the unchanged B544
for camera commands. Window toggle reaches the same 9DAC site through its own
original path. Thus there are ten caller/site groups at nine unique JSL sites.
EF2F remains an external child here; its existing internal occupancy algorithm
is not duplicated or promoted by this batch. Changed child CPU modes, PB/DB,
DP or stack return at the exact next original instruction; unwinds retain frames.

The shared ROM checkpoint passes 68,096 direct states and 10,752 reached whole
text-engine states, comparing CPU, full WRAM, ordered bus accesses and real
child-entry frames. The actual current producer passes 10,112 architectural
comparisons; its initial final assertion incorrectly expected nine rather than
ten redirect groups. With that harness assertion corrected, a focused 2,048
state run passes all 150 redirects and per-group counts. No semantic source
change was needed. Another 65,634 unknown-selector/unsupported-context guards
reject before any memory access or child call.

Fresh public-library checks pass the maintained 10,752-state engine fixture,
all 65,634 guards and 2,432 actual producer states with 150 redirects. Six
heap-only ROM faults in fade duration, reset track, song destination, wait flag,
window mask and script-end state fail normally. Existing system-command tests
pass 45,056 ROM states and 65,679 guards; the affected camera/text-wait fixture
passes 5,632 complete-engine states. One merged fixture array was renamed to
avoid an MSVC shadowing warning; semantic sources were unchanged.

One incremental GCC/Clang/Windows Release checkpoint and metadata checks pass.
Both rebuilt resource CLIs remain byte-identical; accepted unchanged CLI tests
are reused. Publication preserves exact comment-stripped source tokens, with
formatting restricted to changed ranges. Full legacy regression and a new
gameplay/audio replay remain deferred. This batch makes no runtime hit-rate,
cycle-parity or whole-game speed claim. Counts remain 1,574 verified functions,
1,559 ordinary bindings, 752 WRAM fields and 465 C units.

## Text movement, camera and music continuations — 2026-10-16

Four existing text handlers now continue through their previously deferred
branches when pushed-child services and the original CPU context are available:
actor motion at 80:A834, party gathering at B3DF, camera initialization at
B4FE, and the three music/scene calls at B862/B866/B86A. The existing text
engine bridge already supplies these services; no new runtime binding is added.
The callback-null API retains its previous exact handoff boundaries.

Actor motion shares the existing slot/header/occupancy implementations. Party
gathering preserves PHY/PLY and the original wait-or-continue decision. Music
changes retain all three JSL calls, their live frames, context checks and child
unwinds. Sound uploads, polling, synchronization and their implementations are
unchanged. Camera code retains byte increment wrap, cell lookup, leader-follow
waits, full-byte direction indexing and original arithmetic/flag behavior.
The direction table starts inside earlier instruction bytes; unknown targets
retain their pointer reads and pushed JSR frame before exact handoff.

The shared original-ROM checkpoint covers 32,768 direct states, 6,704 legacy,
low-stack and changed-child/unwind states, 4,096 branch-witness states, 2,048
compositions with real party-update/offset/occupancy children, and an explicit
65,536-iteration header-search budget. Its reference stops at the 65,537th
loop entry, preserving live JSR/JSL/PHB frames. CPU, full WRAM, ordered bus
accesses and controlled child-entry states agree. Branch witnesses confirm
all four direction bodies and leader-follow execute, rather than only guards.

Whole-engine coverage includes 5,632 semantic and 4,864 actual producer states.
The current eleven-function consumer closure is extracted unchanged except for
read-only entry/return observations; 60 child redirects pass. The fresh public
library passes the maintained 5,632-state engine fixture and 1,280 producer
integration states with all 60 redirects. Four heap-only ROM operand faults
and an omitted music child are detected with normal failure exits. The affected
script/field fixture passes 36,864 states and 65,657 guards in the same batch.

One incremental GCC/Clang/MSVC and Windows Release checkpoint passes; metadata
checks pass. Both rebuilt resource CLIs are byte-identical to their preceding
accepted binaries, so their unchanged CLI proofs are reused. Named integral
constants expand to the exact privately proven source tokens; formatting also
preserves tokens. Full decomp-verify and a new gameplay/audio replay are deferred;
no current interpreter hit-rate, cycle-parity or whole-game speed claim is made.
Counts remain 1,574 verified functions, 1,559 ordinary bindings, 752 WRAM fields
and 465 C units. These are extensions inside existing entries, not new functions.

## Native frame-return continuations — 2026-10-16

Four complete register-restore tails now have portable semantic continuation
APIs: 82:93CC PLP/PLA ends before 93CE RTS; 86:8B52 PLP/PLA before 8B54 RTS;
86:9756 PLA before 9757 RTS; 86:8B6F PLP/PLX/PLY before 8B72 RTL. They belong
to the existing verified menu/world frame-wait and sprite-frame parents.
Original polling, frame changes and actual guest RTS/RTL remain unchanged.
No new canonical function, ordinary binding, WRAM field or C unit is counted:
1,574 verified functions, 1,559 ordinary bindings, 752 WRAM locations, 465 C
units. Three wait parents remain ordinarily unbound; these four internal
continuations are separately guarded desktop hooks, not whole-parent bindings.

The producer requires canonical banks, native CPU mode, supported widths,
WRAM stack limits, matching live beam/CPU clocks, no active IRQ/NMI or DMA,
horizontal position at most 800 and at least 256 master clocks before the
actual active boundary deadline. At most 160 clocks, including refresh, fit
before the next HDMA/line boundary. Rejected contexts execute original code.
The SDK deadline predicate is not queried speculatively: it mutates unwind
state. The owning boundary runner exposes its actual deadline read-only.

The first private replay exposed a real integration defect: skipping the
interpreter's per-opcode dynamic-read progress checkpoints delayed APU flushes.
At frame 8140 a sound acknowledgement needed one extra poll, eventually moving
a frame boundary and changing PCM. The corrected consumer records progress
per original pull instruction, preserves its exact clock/flush checkpoint and
reconciles the interpreter's progress counters. Normal nested bridges retain
their original threshold policy. The unchanged NMI clock adapter and original
SDK files are reused; only the reviewed desktop generated bridge is extended.

The shared original-ROM proof covers 65,536 semantic and 65,536 actual producer
states, plus 2,048 actual RTS/RTL compositions through each path. CPU, full WRAM,
ordered stack reads and each opcode's CPU/master costs match. There are 460,813
semantic pre-access guards and 524,273 producer guards, including poisoned RAM,
all stack/mode/bank rejects, DMA/interrupt/deadline cases and four admitted edge
contexts. Another 524,288 cases compare exact bus-cache/progress/flush/counter
behavior with warm, cold and mixed caches, both bridge modes and thresholds.
Four heap-only ROM faults and three consumer/registry faults fail normally.

Four additional hooks must not impose four comparisons on every interpreted
opcode. A 4 KiB page bitmap skips only impossible registry pages; exact lookup,
mirrors, replacement, capacity handling and callbacks stay unchanged. Its
2,751,976 comparison queries include every 32,768-page position. A focused
16,777,216-query benchmark measured7-hook lookup at 0.053 s versus 0.029 s filtered,
and 11-hook lookup at 0.077 s versus 0.027 s filtered. These are lookup timings, not an
end-to-end FPS claim. Original SDK sources and sound-poll kernels are untouched.

The final automatic public-library replay covers 10,607 frames, all 53,037 raw
CPU/WRAM/PPU/picture/PCM/stack/ordered-port records, every WRAM page and all
CPU/master/coprocessor clocks exactly. Admissions are 12/1598/1124/945 for the
four tails. Their 7,179 original opcodes become native: 5,262,671 interpreted
instructions become 5,255,492 in this bounded scenario. This is not a whole-game
coverage or speed percentage. The final link has no private semantic override.

The maintained fixture independently composes 2,048 real returns against the
public library and catches a ROM fault. Existing private semantic matrices are
reused by token identity, not repeated merely after publication. The public
CMake matrix covers 22 cases, including absent APIs, draft/missing/wrong parent
contracts, other targets, disabled/reference profiles and malformed generated
source. GCC/Clang and isolated Windows Release pass; both resource CLI binaries
remain byte-identical and reuse their accepted CLI checks. No full semantic
regression or unrelated owner build was run. Both commits share this combined
checkpoint; intermediate revisions are not claimed independently tested.

Fixture: tests/decomp_verify/lufia2_frame_returns_verify.c.
Private evidence: frame-return-slices-next/ACCEPTANCE_RECEIPT.json.

## Event actor reveal continuation — 2026-10-16

The existing event61 handler at 80:DDCC now delegates its original JSL
at DDD9 to the already-verified 8E:BAF6 reveal routine when scoped child
services are available. That routine includes its existing encounter service
at 8E:BB12. A normal return continues at 80:DDDD and restores the saved
event actor slot through EA50. Child unwinds propagate through EventRun;
changed CPU contexts retain the exact post-call boundary. The callback-null
and unsupported paths preserve the original embedded prefix and BB12 handoff.
Hide route60 stays directly native. Only one semantic source file changes;
no public API, metadata function or binding is added. Counts remain 1,574
verified functions, 1,559 ordinary bindings, 752 WRAM locations and 465 C units.

The shared proof covers 8,192 original-ROM hide/show states, including 3,840
actual calls of the full existing native reveal body. Its unchanged encounter
child is an explicit controlled service; CPU, full WRAM, ordered data-bus
accesses and child-entry witnesses match. Timer integration adds 512 normal,
512 legacy and 512 low-stack states, plus 4,096 unwind/changed-return-context
states. The actual current producer closure adds 512 normal, 512 low-stack,
512 missing-target states and 15 redirected/unwound returns. A forwarding
deletion and two heap-only ROM faults all fail differential checks normally.
The final faults select valid original child and slot-save entry points;
an earlier mid-instruction fault crashed the reference and is excluded.

Fixture preparation exposed two test assumptions, not semantic defects:
the simulated whole child must preserve Y to reach the chosen script END,
and the low-stack range differs from D5F2 because DDCC saves fewer bytes.
The missing-entry shim now denies only its intended target. With that
correction, all 512 prior diagnostic missing-target cases were also checked
through the actual producer; its original production availability logic is
unchanged. The earlier broader shim could select the legacy timer wrapper
after the first missing child, while still matching CPU/WRAM/bus states.

The proposed object-animation call layer was discarded. Its event parent
already handles negative brightness before calling the already-native queue.
A change between those reads would require interrupt evidence; no additional
layer, duplicate implementation or runtime-gain claim is justified here.

The maintained self-contained fixture independently passes 512 states and
a valid-entry ROM fault against the actual public library. GCC/Clang libraries,
metadata and the isolated Windows Release build pass. Both CLI executables
remain byte-identical, so their accepted CLI proofs are reused. Unchanged
timer/trigger bodies and internal header retain their preceding shared checks;
no full regression, gameplay replay or repeated unrelated matrices ran.
This is a completed scoped caller continuation, not a full VM or speed claim.

Fixture: tests/decomp_verify/lufia2_event_actor_reveal_verify.c.
Private evidence: field-event-object-services-next/ACCEPTANCE_RECEIPT.json.

## Conditional object diagnostic continuation — 2026-10-16

The existing internal event opcode 8F at 80:D5F2 now returns from its
85:8415 diagnostic child when the original context permits continuation.
The real JSL at D623 returns through D627 to D681, restores Y/DB and
resumes the same event VM. It does not restart the timer or fetch twice.
The optional child service belongs to the local EventRun; the legacy
factory leaves it null and preserves its original interpreter handoff.
No public API, ordinary binding or canonical entry was added. Counts stay
1,574 verified functions, 1,559 ordinary bindings, 752 WRAM locations and
465 C units. This completes a missing child return, not the whole VM.

The parent uses structured search and return helpers with named record
fields. It preserves the high-first table pointer reads, raw operand,
repeated matched-entry read, original call frames and 65,536-visit boundary.
Its five existing native object children are unchanged. The already-native
field trigger forwards the timer service without a new trigger implementation.
Unsupported diagnostic contexts keep their real frame and exact 85:8415
handoff; changed post-child contexts stop at D627, and child unwinds propagate.

The shared checkpoint compares original ROM instructions, CPU, all WRAM,
ordered data-bus accesses and controlled-child entry witnesses. It covers
512 continued diagnostic/end/redraw states, 512 legacy and 512 low-stack
handoffs; the actual current producer closure adds 512 continued, 512
low-stack and 512 missing-dispatch states plus 15 redirected/unwound child
returns. Eight post-diagnostic behaviors cover 4,096 direct states. Nested
trigger/timer/diagnostic composition passes 512 direct and 512 producer
states. Another 2,048 found-object prefixes execute four actual native
children and both coordinate branches (1,024 each), stopping at the existing
D0BA stack boundary. Unchanged diagnostic/redraw internals are explicit
controlled services here; their prior separate proofs are reused, not
claimed as newly executed internals or full gameplay coverage.

Affected timer and trigger matrices pass 4,096 and 16,384 states once.
Three heap-only ROM faults detect the diagnostic target, return branch and
Y restoration. The maintained self-contained fixture independently passes
512 states and its ROM fault against the actual public library. Formatting
preserves semantic tokens. GCC/Clang libraries and metadata checks and the
isolated Windows Release build pass. Both resource CLI executables remain
byte-identical to the preceding accepted checkpoint, so those CLI proofs
are reused. No full regression or gameplay replay was repeated; there is
no new gameplay parity, runtime-hit or speed claim for this rare error path.

Maintained fixture: tests/decomp_verify/lufia2_event_object_conditional_verify.c.
Private evidence: field-object-conditional-next/ACCEPTANCE_RECEIPT.json.

## Native NMI frame entry — 2026-10-16

Lufia2NmiSaveInterruptFrame reconstructs the complete mirrored 00/80:862D
prologue through its original jump to 80:8638. It saves the current status,
data bank and full A/X/Y in original byte order, including B when M1,
then exports M0X0 and PB80. The hardware interrupt frame is already present.
The actual original RTI remains responsible for its later return. Canonical
metadata records nominal M0X0; separate ROM/ABI proofs admit all four input
widths. This is one new verified canonical entry, with no ordinary-call
binding: the desktop interrupt owner installs its mirrored pre-opcode hook.
Totals are 1,574 verified functions, 1,559 ordinary bindings, 752 WRAM
locations and 465 portable C units. No alias or width variant is counted.

Both direct and actual producer paths pass 16,384 entry states covering all
saved status bytes, both ROM mirrors/speeds, four stack positions and register
variants. CPU, all WRAM, ordered data writes and each original opcode's CPU
and master costs agree. Direct and ABI paths each compose 512 saved frames,
the accepted restoration block and a real interpreted RTI. The maintained
public fixture separately passes its 16,384 entry states, 512 real RTIs and
an original-ROM fault. Guards cover 132,108 core and 990,028 consumer cases,
including the old six hooks and both vector mirrors before any bus access.
Three heap-only ROM faults fail in each path. Removing the producer's PB
transfer is also detected. That missing transfer was caught and corrected
before publication; the generic ordinary-call adapter is unchanged.

Actual runtime evidence established that ScreenFade was already reached by
its existing canonical native binding. The proposed extra fade hook and
unused cost model were removed. Its unchanged C received 65,792 private
direct ROM checks and 512 real RTS compositions; none counts as new work
or native gain. The private ABI return banner retains an outdated RTS label;
its compiled entry-only loop executes 512 RTI compositions. The maintained
public fixture reports the correct scope.

The final public-library replay has seven hooks, automatic runtime
registration and no semantic override. Across 10,607 frames, all 53,037 raw
CPU/WRAM, PPU, picture, PCM, stack and ordered sound-port-progress records
and every WRAM page match the preceding accepted integration. Interpreted
instructions decline from 5,336,633 to 5,262,671, exactly 73,962 removed
by 10,566 seven-instruction prologues. This is sampled parity and instruction
reduction, not a whole-game speed estimate. The owner hook at 8699 and its
original REP20 remain interpreted, as do the real guest return instructions.

Seventeen CMake cases pass, including old/missing/draft/wrong-address entry
contracts and disabled/reference configurations. Actual older headers and
libraries still link the four-hook prefix and six-hook finish variants.
GCC/Clang libraries and metadata, strict MSVC fixtures and the isolated
Windows Release build pass. Resource CLI binaries are byte-identical to
their accepted versions, so their CLI proofs are reused. Existing NMI bodies
and SDK bus/cycle adapters remain unchanged; prior exact function-closure
proofs are reused. No full regression or repeated legacy matrix ran.

The maintained fixture is tests/decomp_verify/lufia2_nmi_entry_verify.c.
Private evidence is nmi-entry-fade-next/ACCEPTANCE_RECEIPT.json. The initial
private replay included a fade hook with zero visits; the final public replay
validates its removal. All changes share this combined checkpoint.

## Native NMI display and interrupt restoration — 2026-10-16

The desktop now also uses the original display/clock block at 80:8664 and
frame-restoration block at 80:869B. Two bounded portable APIs share the
existing MainNmi bodies: Lufia2NmiDisplayAndClock stops before 80:8699;
Lufia2NmiRestoreInterruptFrame stops before the real RTI at 80:86BF or the
reset jump at 80:86B1. The existing 8699 owner hook and its original REP20
remain in the interpreter. They run once and retain control of redirects
and mutable CPU context. Unsupported post-hook state continues originally.
There are no additional canonical functions, bindings or count promotions.

The combined original-ROM checkpoint covers 262,144 direct states at both
ROM speeds plus 1,024 hour-rollover edges, with exact CPU, WRAM, ordered
data/MMIO accesses and independently observed per-opcode CPU/master costs.
Production adapter coverage adds 16,384 states plus 1,024 hour edges.
Both paths separately compose 1,280 actual interpreted RTIs. Another 1,664
actual owner-hook compositions vary DB, DP, M/X, decimal mode, stack,
flags, reset state and MEMSEL. They preserve a single callback per side.
Pre-access guards cover 264,204 portable and 857,911 consumer cases.
Three heap-only ROM faults are detected by each direct and ABI fixture.
Affected canonical MainNmi checks pass 65,536 direct states with five child
unwinds and 65,541 actual ABI invocations, zero fallbacks and 15 redirects.

The public-library replay, with automatic runtime registration and no
semantic override, matches the preceding accepted prefix integration across
10,607 frames: all 53,037 raw CPU/WRAM, PPU, picture, PCM, stack and ordered
sound-port-progress records and every WRAM page agree. Interpreted
instructions fall from 5,579,395 to 5,336,633, another 242,762 removed.
The unchanged 8699 REP20 executes 10,566 times. These are scoped replay
results, not a whole-game performance percentage.

Strict MSVC fixtures, GCC/Clang libraries and metadata, and the isolated
Windows Release build pass. Byte-identical resource CLI executables reuse
their accepted CLI proofs. Thirteen CMake cases cover full, prefix-only,
partial-API, disabled, reference and rejected configurations; an actual
older-prefix library links and installs exactly four hooks. Formatting is
token-identical. Exhaustive private proofs are reused for the same published
sources; the final public library separately passes RTI/fault checks and the
runtime replay. The exact-X guard refinement changes only invalid X values;
all positive states have X1 and the guards were freshly run afterwards.
No identical exhaustive run or legacy full regression was repeated.

The maintained test is tests/decomp_verify/lufia2_nmi_finish_verify.c.
Private evidence is nmi-finish-slices-next/ACCEPTANCE_RECEIPT.json. The first
runtime fixture accidentally extended unrelated four-row scenario loops;
only its instrumentation was corrected before the accepted comparison.
The initial callback fixture also toggled shared MEMSEL twice; its corrected
deterministic input is the accepted result. Neither issue changed semantics.

## Original NMI continuations — 2026-10-16

The existing MainNmi implementation now shares six phases with
Lufia2MainNmiResume: 80:8638, 864C, 8653, 865A, 8661 and 8664. They stop
before original child JSRs or at the original reset/RTI boundaries. The
canonical child-service API retains its behavior, including saved registers,
the uncapped original play clock and the 80:8699 checkpoint. An unexpected
M0 child return hands back at its exact next original instruction. MainNmi
exit_mx is dynamic because PLP restores the saved status. No additional
function, binding, memory location or semantic translation unit is counted.

The shared ROM checkpoint passes 393,216 phase states plus 1,024 hour
rollover edges, 4,096 canonical MainNmi states, 80 child unwinds, 80 changed
width boundaries and 1,280 actual interpreted RTI compositions. The permanent
public fixture also passes 852,046 pre-access guards. Three heap-only ROM
faults change the HDMA operand, clock threshold and restored register;
all are detected. The affected older play-time fixtures pass 65,536 direct
states and 65,541 actual canonical-ABI invocations, with zero fallbacks and
15 return redirects. Their real checkpoint also exercises changed DB.

The desktop consumer currently admits only preparation and callback
selection at 8638/864C/8653/865A. It preserves original child JSRs, RTSs,
the play-time hook and RTI. Native entry requires E0, binary arithmetic,
PB80, DP0 and S1F00..1FF3; preparation is M0X0, subsequent phases M1X1
with DB80. Unsupported states stay in the original interpreter. Only the
desktop owner with enabled decomp, verified MainNmi metadata and the new
portable API receives these hooks. Cold startup and execution-state loading
both install them. Reference-only, disabled, older-library and other-runtime
configurations retain their existing sources.

The consumer accounts for the original per-opcode CPU/master costs,
memory speeds, refresh and existing APU batching. Its byte accesses use
the interpreter's own bus callbacks, preserving the dynamic-read cache and
progress epochs used by wait detection. The pinned SDK stays unchanged;
a maintained overlay supplies these narrow adapter functions. Both ROM
speeds pass 132,608 production-prefix ABI states. The instruction cost
sequence is independently checked against original opcodes and raw bus
addresses. There are 590,655 consumer/core guard cases and two additional
production-ABI ROM faults. Eleven CMake selection/rejection cases pass.

The actual public-library replay uses automatic owner registration and no
semantic overrides. Over 10,607 frames, all 53,037 raw CPU/WRAM, PPU,
picture, PCM, stack and ordered sound-port-progress records and every WRAM
page match the preceding accepted native version. Interpreted instructions
decline from 5,716,753 to 5,579,395, a scoped reduction of 137,358. This
is sampled parity; no whole-runtime cycle identity or frame-rate gain is
claimed. A separate full-NMI experiment establishes that missing interpreter
bus bookkeeping caused transient sound-upload progress differences. Its
broader runtime activation remains separate pending callback-context fences.

GCC/Clang libraries, metadata validation, strict MSVC fixtures and isolated
Windows Release builds pass. Resource CLI proofs are reused by identical
rebuilt executable hashes. All changes share the combined checkpoint; no
claim is made that intermediate commits were independently tested. The
historical full decomp-verify target and manifest unit-test file are absent
from the current isolated configuration, so those runs execute no tests and
are not reported as passing. The actual configure-time manifest generation,
AOT generation and production build pass.

## Original field caller continuations — 2026-10-16

Four existing internal text handlers now share optional pushed-child
services: 80:AC6A selects collected objects, BDBB publishes a redrawn
region, and BBEB/BDCA enter an area. Their original bodies are reused.
The legacy APIs retain their native child dispatch and exact fallback
boundaries, including the earlier S1F10 entry range.

All seven calls are JSL, including the two targets in bank80. Saved Y,
saved DB, long destination-map writes, reload flags and the original
NEXT/YIELD decisions are preserved. Changed child contexts hand back at
the exact next original instruction; no register width is forced. A child
unwind retains its live guest frame. The text VM uses its existing NEXT
context fence when a caller leaves DB or CPU modes changed. No additional
standalone function, metadata promotion or binding is counted.

The shared original-ROM checkpoint passes 16,384 direct states and 4,096
reached whole-VM/production-ABI states. Each path adds 56 child unwinds and
336 changed contexts; the actual ABI also passes 105 return redirects
across the seven sites. There are 589,904 pre-access/unknown-handler guards.
Three original-ROM faults change the picker return opcode, field-flags
immediate and reload-flags source bank; all are detected directly and
through the production ABI. Existing script/field, menu-caller and
reward/cave fixtures pass once in the same affected checkpoint.

The WRAM-seeded region-publication scenario reaches the new native caller
once and unwinds through a real child. Over 10,607 paired frames, all
53,037 raw CPU/WRAM, PPU, picture, PCM, stack and ordered SPC-progress
records and every WRAM page hash match the previous native version. The
first 9,767 frames match the accepted baseline. Interpreted instructions
decline from 5,716,777 to 5,716,753, a scoped reduction of 24. Other caller
hits are not claimed. The actual public-library replay reproduces the
private run without semantic overrides or manual dispatch registration.
This is sampled parity, not a cycle-identical execution claim.

GCC/Clang libraries, metadata checks, strict MSVC public fixtures and the
isolated Windows Release build pass. The rebuilt GCC/Clang resource CLIs
are byte-identical to the preceding accepted binaries; their unchanged
CLI proofs are reused by hash instead of repeated. Direct/ABI proofs are
reused across token-identical publication. The public fixture and public
runtime replay are fresh integration checks. All related changes share
this checkpoint; the full legacy regression remains deferred.

## Shared script readers and table branches — 2026-10-16

The text VM now shares the existing 80:C0B7 byte reader, 80:C0D0 word
reader and 80:C102 relative seeker. Their old text helpers retain their
original call frames. The word reader preserves both original reads,
current-M pushes/pulls, accumulator exchange and bank advancement. The
duplicated byte-only implementation previously returned the wrong value
under M0; a frozen pre-change executable reproduces that mismatch against
the supported ROM. This fixes decompilation fidelity, not original gameplay.

The existing C0D0 entry and consumer additionally accept M0 with the same
native, X16, binary, PB80, DP0 and stack guards. Canonical reader entry
names and bindings are reused; no duplicate routines or new bindings are
introduced. C0B7/C0D0 metadata records their preserved runtime exit widths.
Shared internal bodies let legacy callers retain their broader frame
contexts without weakening the canonical entry guards.

With child services, the existing 80:A041 variable-table branch continues
through A3C6 using the existing goto implementation. Without services, its
previous exact A3C6 boundary remains available. The internal route is
documented complete for this contract; the enclosing text VM is not
claimed fully reconstructed.

The combined checkpoint covers 327,680 direct reader/seeker states and
196,608 actual bridge states, with 24 additional intentional bank/frame
aliases on each path. CPU, full WRAM and ordered data-bus accesses agree
with the ROM. Reached A041 compositions cover 4,096 ordinary states plus
64 bank-edge states directly and through the actual VM bridge; 4,096
legacy states retain the old handoff. There are 262,150 semantic and
262,151 consumer pre-access guards. Two original-ROM operand faults are
detected on both direct and actual-ABI paths.

One shared affected union covers 18 existing text/script fixtures. All
positive matrices pass. Two old unknown-command lists omitted existing
implemented routes; only those guard groups were corrected and repeated.
The permanent reader fixture also passes against the actual public static
library, including bank edges, legacy handoffs, guards and fault controls.
No unchanged full regression is repeated for each helper or commit.

Two real-runtime comparisons each cover 10,607 frames and 53,037 raw
comparison records, plus every WRAM page hash. CPU/WRAM, PPU, pictures,
PCM, stack and ordered SPC progress agree with the preceding native
implementation. Their first 9,767 frames also match the accepted baseline.
The ordinary A041 scenario reaches native C once and removes 50 interpreted
instructions. With the text VM deliberately original in both runs, the
new M0 reader admission removes six. These are separate scoped scenarios,
not additive whole-game measurements or cycle-identical execution claims.
The custom script is WRAM-seeded; earlier accepted diagnostic phases are
retained. Neither scenario changes the ROM file or saves.

GCC/Clang library builds, real resource CLI tests, metadata checks, strict
MSVC public fixtures and the isolated Windows Release build pass. The
full legacy regression remains deferred. Source, compiler and proof hashes
and reused/fresh scopes are recorded in the private batch receipt. Several
related changes share this final checkpoint; intermediate commits are not
independently claimed tested.

The final public-library replay also reproduces both scenarios' 53,037
raw records and all WRAM-page hashes exactly. The forced-VM scenario
observes one accepted native M0 word-reader entry at S1FF8/DB7E. No private
semantic objects or manual dispatch registrations are linked. The earlier
direct/ABI proofs are reused only for token-identical semantic bodies and
the checked production bridge closure; the public fixture and generated
runtime replay are fresh integration checks.

## Original reward/cave caller continuations — 2026-10-16

The existing text/menu services now carry both item-reward pages,
80:A414/A41F, through their shared A42B loop. The original count byte
wraps from zero through 256 iterations. Selection feedback retains its
current X width and optional scene reload. The count decrement retains
current-M word side effects. Saved Y and the actual near-return word are
consumed literally; rewritten returns use their original landing PC.
A 4,096-step host budget hands back at A43A without changing guest state.

80:ABD0 now uses the same optional child protocol for cave entry and
exit. The exit's current-width DP:AE test decides whether A350 reloads
the scene. Saved DB/Y and the original text-state write remain intact.
All immediate-width fences, pushed child frames, child unwinds and the
text VM's existing NEXT context fence remain explicit. Existing no-service
APIs keep their previous boundaries. These are four documented internal
contracts, not new standalone verified entries or bindings.

The shared checkpoint passes 3,072 direct ROM states and 3,072 reached
whole-VM ABI states across all count bytes and both cave branches.
Each path covers 72 child unwinds, 432 changed contexts, 64 rewritten
reward returns and two explicit loop-budget cases. Six distinct child
sites are reached; 165 production return redirects pass. Additional
probes reach A45F, A479 and ABEF with M0, and 16 real near-return landing
boundaries per path include bank-local wrap and text-VM control PCs.
Controlled children isolate caller semantics; their interiors are not
reverified here. There are 131,108 pre-access/unknown guards and five
original-ROM operand faults are detected.

Affected earlier callers pass 20,480 direct and 5,120 actual VM/ABI
states, their saved-index/context/unwind cases, 225 redirects and 131,244
service/legacy guards. Legacy menu positives retain 28,672 states;
legacy ABD0 and BA39 each pass 1,024 states plus 253 guards. The existing
recovery ABI passes 256 complete cases with 106 children each and 105
redirects. The earlier service guard list now recognizes the two reward
routes, without weakening unknown-handler assertions.

A real item1/count1 reward script is seeded in WRAM without modifying
the ROM. Its real child path unwinds and rejoins the original guest flow.
All 10,607 paired CPU/WRAM, PPU, picture, PCM, stack and ordered SPC-progress
records and all RAM-page hashes agree; the first 9,767 frames also match
the preceding accepted baseline. One reward invocation reaches native C.
Interpreted instructions decline from 5,717,083 to 5,717,031, a scoped
reduction of 52. Cave and other reward tails have ROM/ABI coverage;
additional runtime hits are not claimed. Full legacy regression is deferred.

The final actual-public-library replay reproduces all 53,037 comparison
records and RAM-page hashes over 10,607 frames, with no private semantic
objects or manual dispatch registration. GCC/Clang libraries, real resource
CLIs and metadata checks, strict MSVC public fixtures and the isolated
Windows Release build pass. Five fault controls also pass against the
public library. The actual original reference forces 80:9CB8 to the
original only during the WRAM-seeded reward phase; other accepted native
selections stay the same. This is sampled-state/output parity, not a
cycle-identical execution claim. Related slices share this checkpoint;
intermediate edits are not separately accepted. Full legacy remains deferred.

## Original text/menu caller continuations — 2026-10-16

The text VM now uses its existing pushed-child service for five related
callers: 80:A32B, A372, A51D, BD99 and B909. Their original tails restore
saved Y/DB, reload the scene, clear text state and resume or exit the VM.
Existing scene helpers A350/A368 remain shared. Script-menu A51D reloads
without adding a fade. Legacy no-service boundaries remain unchanged.

Every original child call retains its JSR/JSL frame and exact unwind.
M0 returns stop before width-dependent immediates. A changed M/X, DP,
decimal mode or script bank hands NEXT back at 80:9D00 instead of
continuing through a CPU context outside the text VM's C contract.
Unknown child semantics remain explicit services; this completes internal
caller continuations, not the entire menus or text VM. No new standalone
function contract or native binding is counted.

One checkpoint covers 20,480 direct original-ROM states, 5,120 reached
whole-text-VM ABI states, 120 child unwinds and 720 changed-child contexts
on each path, all 15 child sites, 225 production return redirects and
131,246 pre-access/unknown-handler guards. Children deliberately change Y
where saved-index restoration matters. Controlled children isolate caller
semantics; their interiors are not reverified by these tests. Five original
caller-tail mutations are detected. Existing positive suites retain 28,672
menu and 36,864 script/field states; 256 complete recovery ABI cases retain
106 child calls each and 105 redirect probes. Two stale unknown-handler
lists now recognize the already-supported BD99 and B7FE routes.

The ordinary 9,767-frame replay exactly matches the accepted baseline but
does not reach these callers. An additional original-script menu scenario,
seeded in WRAM without modifying the ROM, reaches A372 with real children
and correctly unwinds at the first waiting child. All 10,607 paired raw
CPU/WRAM, PPU, picture, PCM, active/discarded-stack and ordered SPC progress
records and all RAM-page hashes agree. Interpreted instructions decline
from 5,888,064 to 5,888,027, a scoped reduction of 37. Other new caller tails
are covered by differential ROM/ABI proofs, not claimed as runtime hits.

The final public-library replay exactly reproduces the private candidate:
53,037 raw comparison records and all RAM-page hashes over 10,607 frames.
No private semantic object or manual dispatch registration is used. This
is sampled-state and output parity, not a cycle-identical execution claim.
GCC/Clang semantic libraries, actual resource CLIs and metadata checks,
strict MSVC public fixtures and the isolated Windows Release build pass.
Corrected legacy guard lists pass 65,626 menu and 65,657 script cases;
unchanged positive bodies reuse the green results within this checkpoint.
The seven reached production ABI helpers match the current consumer tokens
apart from read-only observers. Full legacy regression remains deferred.
Several related slices share this final checkpoint; intermediate edits were
not independently accepted.

## Original field-frame continuation — 2026-10-16

83:8007 resumes the existing field loop after its original 83:900C wait.
The portable owner and resume share one C body. The resumed first iteration
skips only the wait already completed by the original CPU; later iterations
retain every original wait, child frame, width fence and restart boundary.
No second field scheduler or synthetic guest return frame is introduced.

The consumer admits native binary PB83/DB83, DP0, M1/either X width,
S1FFF/HRV0. The read-only integration gate runs only after the real RTS
at83:9012 lands at83:8007, inside a quiescent noninterrupt run before its
deadline. Its dispatch uses frame size 0. Existing scene continuation gates,
ordinary paired-call handling, original RTS timing and RTI ownership remain.

The unchanged earlier direct ROM proof is reused with source/log hashes:
4,096 states and912 boundary cases per owner/resume, plus eight 4,096-frame
compositions per entry. The resumed first wait count is 4,095 per case;
the owner retains 4,096. The current actual ABI reruns both entries together:
8,192 state cases, 1,824 boundary cases and 16 long compositions. This covers
19 child sites per entry, 304 reached child unwinds, 608 M0 return fences,
912 context fences and 3,040 redirect probes. The owner's three incoming
contexts remain tested; the nonreturning resume uses its one real HRV0/S1FFF
context. Two semantic faults that respectively add/remove the first wait
are detected. The permanent direct fixture retains the passing resume logic.

There are 263,830 consumer guards and 131,390 semantic pre-access checks with
no bus accesses. The gate passes 656,320 read-only states across all PC,
stack, DP, bank-pair and return-site axes, including 256 real original-ROM
RTS executions. These checks share one checkpoint; only an affected redirect
fixture was retried after replacing its invalid framed resume contexts.

The current paired runtime reaches 4,599 additional native continuations:
4,597 M1X1 and two M1X0. All 9,767 pictures/PPU states and 860/983-frame scene
durations agree. The complete ordered SPC-write fingerprint is 159,715 writes,
hash41E7208BACC42988 on both paths. Original execution exactly matches the
preceding accepted baseline. Interpreted instructions decline from 4,895,554
to 4,749,879, a scoped reduction of 145,675; no whole-game percentage is claimed.

Raw CPU/WRAM hashes differ on 8,224 frames. Of 72 sampled CPU differences,
61 are in original SPC transfers, ten in original menu frame waits and one
at the original circle-row store. RAM differences occupy DP0 on 45 frames,
stack page1F00 on 8,217 and page1D400 on one frame. At frame 8793 the original
path is before STA 7F:D45B=B8, while the native path is immediately after it;
the page agrees next frame. The prior independent read-only pending-store
diagnostic is reused because every current original raw frame/page record
is identical to that diagnostic's input path. One active-stack difference
occurs during an original SPC transfer at frame 1598. Discarded-stack bytes
differ on 8,217 frames, sampled port progress on 34 and PCM on 8,134. These raw
exceptions are retained. Cycle/audio/all-frame architectural parity is not
claimed; no clock, guest state, frame or hash is adjusted.

Counts become 1,573 verified contracts and 1,559 bindings; 752 WRAM fields and
465 C units remain. Final SDK, CLI/compiler and generated integration checks
share this batch checkpoint. Full legacy regression is deferred.

Final integration checkpoint passes both GCC/Clang semantic libraries,
actual resource CLIs, metadata checks and strict whole-consumer C11 builds.
Isolated Windows Release and strict MSVC compile/link of both permanent
fixtures pass. Native/reference/disabled CMake configurations enable the
new seam only in native mode. All four maintained production seams are
tested by the pinned SDK contract harness: 305 unchanged checks pass and
are reused; the 104 added field checks pass after the synthetic handoff
target receives its missing WAI instruction. Only those affected cases
were retried. No runtime or semantic expectations were weakened.

The actual generated public binding, maintained SDK overlay and public
library also run the complete native scenario. All 53,437 relevant raw
records and all RAM-page hashes exactly reproduce the private candidate.
No manual dispatch entry or private semantic object remains in that replay;
only read-only counters are retained. The unchanged original baseline is
reused after exact record comparison. The permanent direct fixture keeps
its passing algorithm and all 1,024 unique opcode lengths in a readable
constant table. Public/private semantic and consumer C tokens agree.
These commits share the combined checkpoint; intermediate revisions were
not independently tested. Full legacy regression remains deferred.

## Byte-index field position callers — 2026-10-16

The existing 83:A746 actor fine-position adapter additionally accepts the
observed DB87/M1X1 caller profile in native binary mode, PB83, DP0,
S1F10..1FFC and HRV0/3. Existing M1X0 admission is unchanged. The literal
portable position algorithm, metadata and selected bindings are unchanged.

The earlier 327,680 direct ROM states are reused after comparing the
complete relevant semantic source/header closure and seven shared bridge
helpers. The only intervening public actor/field header changes are three
unrelated VM declarations. A current broad ABI experiment passed 163,840
states; its DB83/X8 admission changed intro timing and was rejected.
The final narrower DB87/X8 adapter passes a focused 65,536-state actual-ABI
matrix covering all slot words and both admitted return modes. There are
7,240 consumer guard states, including every excluded DB and independently
invalid mode, width, DP, stack and return-mode conditions. Original entry
frame reads, CPU, full WRAM and ordered operand accesses are checked.

The field-only runtime reaches 22 additional native calls and removes
506 interpreted instructions (4,896,060 to 4,895,554) in this scenario.
All 9,767 raw CPU/WRAM, PPU, PCM, picture, port and stack records agree
exactly with the current original-profile baseline. Both scene durations
remain 860/983 frames. The ordered SPC stream remains 159,715 writes,
hash41E7208BACC42988. No guest clock, frame or hash is adjusted.
The 35 DB83/X8 intro calls remain original: their broad experiment moved
an intro section one frame early and differed on 1,116 picture frames.
Neither this result nor function-level parity justifies enabling that
separate timing-sensitive profile.

Counts remain 1,572 verified contracts, 1,558 bindings, 752 WRAM fields and
465 C units. Consumer compiler/Release checks share this checkpoint.
Unchanged semantic library, metadata and CLI results are reused from
8bc0b5a/24ac544; the full legacy regression remains deferred.

Final consumer checkpoint passes strict MSVC actual-ABI/guard fixtures,
GCC/Clang whole-consumer C11 warnings-as-errors and isolated Windows
Release. Unchanged library, metadata and CLI proofs from the preceding
batch are reused. The combined final sources were checked once before
these two dependent commits; no separately tested intermediate revision
is claimed. Full legacy regression is deferred.

## Menu callers and equipment-selection widths — 2026-10-16

Eleven existing adapters now admit the 129 original system/low-WRAM data
banks: title preparation 82:E8DE/E917/E998, palette selection 86:916C,
save-slot construction 82:F20A/F481/EEF7, slot display 82:EF25, record-list
testing 82:ECAF, action dispatch 82:E9D5 and save-menu entry 82:E8B2.
All other mode, program-bank, DP, stack and child-return guards remain.

The existing 82:9971 equipment-member selector already preserves entry
widths through PHY/PHP/REP/PLP/PLY. Its portable admission now accepts all
four M/X profiles in binary mode. The canonical metadata profile remains
M0X0. Its consumer additionally accepts the observed M1X0 native/binary,
PB82, DP0, S1F10..1FFC, HRV0/2 profile in the same 129 data banks. Legacy
M0X0 admission is unchanged; X8 consumer calls remain original.

One shared checkpoint covers 429,312 bank-profile states per direct/ABI
path, including 9,417 reached child unwinds per path and 120 reached ABI
redirect probes. The width selector adds 294,912 direct  and 147,456 actual
ABI states. Its direct coverage includes all 65,536 DP values at all four
entry widths; bank coverage includes all 256 DB values. A further 24,576
direct states cover stack aliases at three stacks, eight overlapping
destinations and all 256 saved-status bytes. Every positive comparison
checks CPU, full WRAM and ordered operand accesses; bank fixtures also
compare MMIO. Only sequential instruction-fetch bytes are excluded.
There are 5,732 unsupported consumer states  and 16 semantic pre-access
guards. An in-memory original-ROM AND-operand fault is detected at case 14.
The permanent equipment-selection fixture retains the passing direct
width and stack-alias algorithms. No ROM bytes are exported or changed.

The paired runtime reaches each additional profile once. All 9,767 pictures
and PPU states match; scene durations remain 860/983 frames. The original
path exactly reproduces the preceding accepted equipment checkpoint.
The complete ordered SPC-write fingerprint is 159,715 writes/hash
41E7208BACC42988 on both paths. Interpreted instruction counts change from
4,888,416 to 4,896,060: these twelve native calls are not a whole-game
speedup claim, because SPC polling phases also change.

Raw CPU/WRAM hashes differ on 8,226 frames. There are 77 sampled CPU
differences: 71 original SPC transfers, five original menu frame waits and
one original circle-row write. RAM differences occupy DP0 on 66 frames,
stack page1F00 on 8,225 and page1D400 on one frame. At frame 8793 the native
path is at 86:C432, immediately before the taken branch and STA 7F:D45B;
the original path is at 86:C439, immediately after that store. A read-only
diagnostic reproduces every raw native record and proves that the single
pending B8 write accounts for the page hash difference. The page agrees
again on the next frame. This diagnostic does not replace or normalise
any recorded state. Four active-stack differences occur during original
SPC transfers. Discarded-stack bytes differ on 8,225 frames; port progress
on 67 and PCM on 8,136. Cycle, audio and all-frame architectural parity
are not claimed. Original semantic bodies and guest clocks stay literal.

Counts remain 1,572 verified contracts, 1,558 bindings, 752 WRAM fields and
465 C units. Final compiler/library/CLI checks share this checkpoint.
The full legacy regression is deferred; unchanged green bank cases were
reused when correcting only a width-fixture bank constant.

Final checkpoint: strict MSVC ROM/actual-ABI fixtures and permanent
public fixture compilation/linking, GCC/Clang semantic libraries, resource
CLIs, metadata checks and whole-consumer C11 warnings-as-errors all pass.
The isolated Windows Release build passes. Public/private source tokens
match; the public direct fixture keeps the previously verified algorithms.
Unchanged positive cases are not rerun solely for publication or formatting.
The two commits share this combined checkpoint; intermediate commits were
not tested separately. Full legacy regression remains deferred.

## Equipment records in original system data banks — 2026-10-16

Five existing equipment adapters now admit the 129 system/low-WRAM data
banks through the existing shared predicate: 82:F703 selects a slot,
82:F710 loads an item, 82:F722 accumulates modifiers, 82:F846 rebuilds one
member's modifiers, and 82:994E rebuilds the party's equipment statistics.
Observed original callers use DB83. Program bank, DP0, widths, binary/native
mode, stack and child-return guards remain unchanged. No portable body,
public declaration, metadata contract or selected binding is added.

The original 93,783 direct ROM states and 34,560 unsupported consumer
guards are reused after proving the five adapter blocks and 14 common
consumer helpers identical to the earlier passing candidate. Relevant
party/equipment source and CPU/bus helpers are unchanged. The updated
actual-ABI fixture runs 93,783 states against the current library, including
903 reached child unwinds and 13,590 reached redirect probes. It preserves
real child PB on return and unwind, replacing an older fixture assumption.
All five entries compare CPU, full WRAM, MMIO and ordered operand accesses.

The paired current runtime receives 234 additional native calls: 84 slot
selections, 84 item loads, 50 modifier accumulations, 14 member rebuilds and
two party rebuilds. All 9,767 pictures and PPU states match; both scene
durations remain 860/983 frames. All WRAM outside DP0 and stack page1F00
matches at every frame. The complete ordered CPU-to-SPC write fingerprint
remains 159,715 writes, hash41E7208BACC42988. The original path exactly
matches the prior accepted baseline. Interpreted instructions decline from
4,906,088 to 4,888,416, a scoped reduction of 17,672 instructions.

Timing remains different between execution tiers. CPU/WRAM hashes differ
on 8,254 frames: 102 sampled CPU differences occur in original SPC transfer
code (94) or original menu frame waits (8). RAM differences are confined to
DP0 (80 frames) and stack page1F00 (8,228 frames). Four active-stack
differences occur during SPC transfers at frames1566/1582/1598/1601;
discarded-stack bytes differ on 8,228 frames. Port-write progress differs on
76 sampled frames, while its complete ordered fingerprint agrees. PCM
differs on 9,258 frames. These raw observations are retained without
normalisation. No identical-cycle, audio, whole-frame architectural-state
or whole-game performance claim is made. Source semantics remain literal.

Counts remain 1,572 verified contracts, 1,558 bindings, 752 WRAM fields and
465 C units. Final consumer compiler checks share this batch checkpoint;
unchanged semantic/library/CLI checks are reused, and the full legacy
regression is deferred.

Final checkpoint passes strict MSVC current actual-ABI compilation,
GCC/Clang whole-consumer C11 warnings-as-errors and the isolated Windows
Release build. Public/private adapter C tokens match. The unchanged direct
ROM and guard results are reused with their source and log hashes; only
the changed actual-ABI fixture and current runtime were rerun. Unchanged
semantic library, resource CLI and metadata checks are reused from the
preceding checkpoints. Full legacy regression remains deferred. The two
commits share this combined proof; no intermediate commit was separately
tested. No guest state, timing, cycles or recorded hashes were normalised.

## Primary actor script: observed X8 caller — 2026-10-16

The existing 83:D416 adapter now admits its observed M1X1 caller at
83:A96B. The original routine changes only M with REP20/SEP20; LDX and TAX
retain the caller's index width. The existing portable body already models
this behavior. Its ROM script-table lookup, pointer addition, flags and
three-byte record writes are unchanged.

The additional profile requires native mode, binary arithmetic, PB83, DP0,
S1F20..1FFC, host frame0/3 and one of the 129 system/low-WRAM data banks.
All existing M1X0 admission and unsupported-state behavior remain unchanged.
The sibling 83:D3F7 secondary-script loader already supports both index
widths; it is checked as an affected control and receives no extra binding.

The shared checkpoint passes 196,608 primary and 196,608 secondary states
on each of the direct and actual-ABI paths. CPU, full WRAM and every operand
read/write are compared. Direct cases include both index widths, all input
accumulator/record words and all 256 data banks; the primary actual-ABI X8
profile uses its admitted 129 banks. 11,264 unsupported X8 states preserve
the previous CPU/WRAM and exact return-frame reads. Two ROM operand faults
are detected. No semantic body, metadata contract or binding is added.

The paired runtime receives two additional native primary-script calls,
both DB87/M1X1/S1FEE/frame3. All 9,767 sampled CPU/WRAM, PPU, picture, PCM,
port-progress and active/discarded stack records match the accepted baseline.
Scene durations remain 860/983 frames. The ordered CPU-to-SPC stream remains
159,715 writes with hash41E7208BACC42988. Interpreted instructions fall from
4,906,118 to 4,906,088: 30 instructions across these two calls, not a claim
about whole-game performance. Counts stay 1,572 verified and 1,558 selected.

Final consumer compiler/Windows checks follow at this batch checkpoint.
The unchanged semantic library and resource CLI checks from b1bf305 are
reused; the full legacy regression is deferred.

Final consumer checkpoint passes strict MSVC actual-ABI compilation,
GCC and Clang whole-consumer C11 warnings-as-errors, and the isolated
Windows Release build. Public and private adapter C tokens match.
Portable source, public headers, metadata, tables and linked dependencies
remain at the prior semantic checkpoint; its library, resource CLI and
metadata checks are reused. Only the consumer admission and these notes
change. No full legacy regression is claimed, and no intermediate commit
is described as independently tested. Private runtime instrumentation
observes calls without modifying guest state or cycle accounting.

## Literal VM opcode dispatch prefixes — 2026-10-16

Three complete prefix contracts end at their original indirect jumps:
80:CC35 reads the event opcode through the actual 80:E8B9 child and dispatches
at CC3F; 83:E0FC loads the object script pointer and dispatches at E119;
83:E10F dispatches the next object opcode at the same jump. These are not
complete VM roots. Existing event/object loops share the same scalar helpers.
Original opcode ranges, DP-high accumulator bits, carry bit2, bus reads,
bank/Y wrapping and the pending DB byte on the object stack remain literal.

The grouped differential checkpoint passes 458,752 prefix states directly
and 458,752 through private actual-consumer adapters, with full CPU, WRAM,
MMIO and operand-bus comparisons. Two private scalar-core profiles add
262,144 cases covering all accumulator words, DP-high bytes and index widths.
5,376 semantic and 30,720 consumer pre-access guards pass; 1,536 original-reader
post-child context/unwind cases and ten reached consumer redirects pass.
Three changed ROM operands are detected. The standalone public verifier is
tests/decomp_verify/lufia2_vm_opcode_dispatch_verify.c; arguments are ROM path
and suite 0 (event), 1 (object initialization), 2 (next object opcode).

Private runtime binding and executed-JMP resume prototypes each preserve
all 9,767 frame/PPU/picture/port/CPU/stack records against the accepted baseline.
Neither receives a dispatch hit in these scenarios: existing native VM owners
already process those opcodes. Therefore neither adapters, bindings nor the
SDK resume seam are published. No performance improvement is claimed.
The three verified portable contracts remain unbound. Total 1,572 verified,
1,558 selected bindings; 752 WRAM fields and 465 C units remain unchanged.
Compiler/CLI integration is checked once at the final batch checkpoint.

Final batch checkpoint: strict MSVC private ROM/actual-ABI proofs and the
self-contained public verifier compile/link pass. GCC and Clang each build
the actual semantic library and resource CLI, pass the CLI test, metadata
validation and whole-consumer C11 warnings-as-errors check. The initial
metadata check found stale generated counts; after regeneration, only that
failed check and the previously unrun consumer checks were run. The passing
library/CLI builds were reused. Isolated Windows Release passes. An internal
helper was renamed to distinguish it from the existing opcode enum tag;
the function bodies otherwise preserve the proven private C tokens.

Public semantic source retains the proven private bodies, excluding only
proof exports and formatting/internal-symbol spelling. The public verifier
retains the three proven prefix profiles; two private core-only profiles
are excluded. Its standalone binary was strictly compiled and linked;
unchanged positive ROM matrices were not rerun for this publication step.
No runtime adapter, SDK seam or new selected binding is integrated. The
full legacy regression remains deferred; no new full-suite pass is claimed.
Both commits share this combined checkpoint, not independent test runs.

## Session records, save previews and near event return — 2026-10-16

Three existing consumer entries now admit the original 129 system/low-WRAM
data banks: 82:F22B builds a save preview, 82:9AA3 resets session records,
and 82:C2AE initializes saved capsule data. Observed callers use DB83/82.
The original M1X0, DP0, binary, program-bank and stack guards remain.

83:BB76 also supports its observed near-return entry at S1FFD with host
frame0/2. Frame3 is rejected before any return read at 00:2000. The portable
near contract ends at S1FFD; the far B727 caller retains its S1FFC maximum.
Only the supported stack-boundary guard changes; original writes and flags
remain literal, without a second implementation of the helper.

The shared ROM/actual-ABI matrices pass 33,024 session/capsule cases,
66,048 save-preview cases and 16,384 event-helper cases on each path.
All operand accesses, CPU and full WRAM are compared. The two parents add
387/258 reached child-unwind cases per path. All five child sites pass
9,675 reached ABI redirects across all 129 banks. Session reset also executes
the actual capsule child in 4,128 direct and 4,128 nested actual-ABI cases.
27,648 consumer guards and 3,584 pre-access semantic guards pass. Four ROM
operand faults are detected. Existing complete bodies stay shared; no new
function, contract or binding is counted (1,569 verified/1,558 selected).

Final production-library acceptance preserves all 9,767 pictures and PPU
states, both scene durations (860/983 frames), and the complete ordered
159,715-write CPU-to-SPC stream (hash 41E7208BACC42988). Eleven observed calls
now use the existing native bodies: two session resets, two capsule
initializations, four save previews and three event preparations.

Execution timing is not identical. CPU/WRAM hashes differ on 8,253 frames;
all WRAM pages outside direct-page0 and stack-page1F00 agree at every frame.
There are 103 CPU-state differences in original SPC transfers or frame
waits. Four active-stack
differences occur during the SPC upload; 8,230 frames retain different
discarded stack bytes. Seventy-two sampled frames differ in SPC write
progress, while the complete ordered port stream agrees. PCM differs on
9,258 frames. These raw results are retained without normalisation.

Total interpreted instructions are 4,901,067 before versus 4,906,118 after
in this run, alongside the changed SPC polling phase. This is not evidence
of a whole-game speedup or of identical cycle/audio/frame-state timing.
Original function semantics are separately checked at their actual boundaries.

Actual GCC/Clang semantic builds, CLI resource targets/tests, metadata and
complete consumer compilation pass. The isolated Windows Release build
passes. The private/public behaviour tokens agree; the final runtime
uses the actual production library. The combined worktree was validated
before preparing its atomic commits; intermediate commits were not tested
separately. Full legacy regression was deferred.

## Actor scene bank contracts and field colour calls — 2026-10-16

The existing $83:A82E scene constructor now supports the original 128 system
data-bank mirrors in portable C. Program bank $83, DP0 and binary arithmetic
remain required. Its original saved status, forty slots, child frames and
ordered banked accesses are unchanged. All four entry widths are covered;
the metadata row retains the canonical M1X0 profile. The consumer still
requires DB=$83: widening that runtime admission made two scene transitions
one frame early, so that adapter experiment was not accepted.

The existing $83:AEB5 colour dispatcher now admits those same system banks
in the consumer, including observed DB=$37/$87 callers. Its complete semantic
body already supported the original banked operations. Other width, DP,
decimal, program-bank and stack guards remain. No new function or binding
is counted: 1,569 verified contracts, 1,558 selected bindings.

One shared checkpoint covers 16,384 direct and 16,384 actual-ABI constructor
states, with original A6DF/A746 children and eleven explicit service sites.
Every child entry is compared. Both paths pass 6,656 child-unwind states;
3,900 reached ABI redirects cover all thirteen sites. Colour passes 4,096
direct and 4,096 actual-ABI states with original children, 16,384 direct and
16,384 actual-ABI child-unwind states, and 615 reached redirects. CPU, full
WRAM and ordered operand accesses agree. An additional 47,616 consumer
guards and 4,096 pre-access semantic guards pass.

The separately tested A746 X8 adapter remains private despite 327,680
direct and 327,680 actual-ABI ROM cases passing: its runtime phase changed
many transient states. Literal helper code remains shared; no duplicate body
or speculative runtime admission was added.

Final production-object acceptance preserves all 9,767 pictures and PPU
states, both original scene durations, and all 159,715 ordered CPU-to-SPC
writes (stream hash 41E7208BACC42988), also at every sampled frame. Twenty-four
observed DB37/87 calls now use native colour semantics. Original interpreted
instructions fall from 4,901,520 to 4,901,067 in this scoped test, not a
whole-game performance estimate.

Raw CPU/WRAM hashes differ on six frames. Four polling-PC differences
(8293C8/8293CA) and four saved NMI-PC stack-page differences overlap; one
original circle-row intermediate state (86C432/86C439, page1D400) converges
on the next frame. Registers other than those PCs agree. All raw hashes
are retained. PCM differs on 1,718 frames: full cycle, audio-sample and
per-frame guest-state parity is not claimed. No synthetic timing or
normalisation conceals these observations.

Six original-ROM operand faults are caught. Actual GCC and Clang semantic
builds, CLI resource targets/tests, metadata checks and complete consumer
adapter compilation pass; the isolated Windows Release build passes.
Unchanged positive proofs are reused across token-equivalent formatting.
Validation covers the combined semantic/consumer worktree before its atomic
commits; intermediate commits were not tested separately. The full legacy
regression was deferred, not reported green on this revision.

## Native scene continuations after frame waits — 2026-10-16

Six explicit continuation boundaries now share the existing original C owners:
86:C1E3/C237 resume scene setup/the frame loop; B38E/B391/B394/B397 resume
the remaining palette waits. These are continuation contracts, not six newly
discovered whole ROM function bodies. The original C1D2/B350 owners use the
same helpers. Original waits, zero-counter wrap, child frames and bus order
remain literal; no artificial clocks, frames or shadow interpreter.

The consumer admits only the real 869757 RTS and these six landings, M1X0,
PB/DB86, DP0, decimal clear, S1F20..1FFC, with a selected native body and
remaining deadline. It reads the existing outer return from live WRAM without
bus access, pushes no guest frame, and uses the engine's paired bounce/unwind
path. Interrupt/yield owners and unknown contexts retain original dispatch.
The producer checks each pinned-source seam exactly once. Reference-only and
decomp-disabled configurations omit the seam. Existing circle timing stays.

The shared eight-root proof covers 7,424 direct and3,712 actual production-ABI
original-ROM cases: CPU/full128KiB WRAM, ordered operand bus and child states.
All1,392 unwind and3,480 altered-return injections reach their boundary on
each path. Eighty semantic and112 additional consumer guards pass;900 reached
redirects cover60 entry/site contracts and19 distinct sites. Eight ROM faults
are detected. The private/public behavior tokens match; positive matrices are
retained across declarations/formatting. A fixture-only correction distinguishes
a genuine changed-child C237 landing from a pre-entry guard fallback.

The read-only gate passes852,016 cases. The pinned SDK interpreter-bridge
regression passes305 checks, including48 new inherited-frame cases. The three
actual CMake overlay configurations pass. Strict GCC/Clang semantic/CLI builds
and resource tests, metadata and isolated Windows Release pass;24 generated
wrappers and the three produced seams are checked. Full legacy decomp-verify
was deferred, not reported as passing; the changed bridge seam received its
relevant engine regression, while unrelated semantic bodies stay unchanged.

Final production-wrapper transitions0C/0E finish860/983 frames with1,762 script
steps. All9,767 paired PPU/pixel frames agree without alignment gaps; ordinary
7,444 CPU/WRAM hashes agree. 0E CPU/WRAM also agree. 0C has one differing hash
at caseframe788: same-process in-memory diagnosis proves all registers and
WRAM equal, with only resumePC869752 versus869754 in the unchanged CMP/BEQ
poll. The next frame converges. 768 original-ROM poll cases prove unchanged
poll-state equivalence and identical completion after the clock advances.
The final production CPU/WRAM/PPU traces exactly match that diagnosed prototype;
no comparison hash was normalized. PCM differs45/589 scene frames. Full PC,
cycle or audio parity is not claimed. This is an explicit runtime timing limit.

Scoped bank86 interpreter work falls54,984->24,857(0C) and47,359->16,157(0E):
102,343->41,014 combined,61,329 fewer(59.92%). Not whole-game coverage or FPS.
The same required run collected read-only wait-caller addresses for next work.
Counts:1,569 verified contracts,1,558 bindings,752 WRAM fields,465 C units.
Public fixtures: lufia2_world_scene_resume_verify.c and
lufia2_scene_resume_gate_verify.c. Private evidence/receipt:world-resume-next.

Correction to older broad snapshot wording: inherited probes wrote private
register/WRAM diagnostics at seven old ordinary frames. No ROM or serialized
rollback snapshot was exported. The final acceptance and new mismatch probe
remove that writer; the latter compares states in memory and emits differences.
No ROM, private tooling or unrelated owner changes are part of these commits.

## Animation starter accumulator widths — 2026-10-16

Extended the existing 86:E0B9 animation starter for its actual M0X0 call from
BAAE@BAE5. The initial STA10,x writes the full accumulator before REP20;
both entry widths then share the existing lookup/frame body and exit M1X0.
The original temporary frame-byte overwrite remains observable on the bus.
No duplicate function, WRAM alias or binding was introduced. Canonical
metadata remains M1X0; E11F retains its M1X0-only stepping contract.

One checkpoint covers 16,384 direct and 16,384 actual production-ABI cases,
both accumulator widths, all animation-byte values, varying high bytes,
poses, DP0/nonzero, flags and near-return contexts. Full CPU/128KiB WRAM and
ordered operand-bus accesses match original ROM. Two semantic and forty
consumer guards pass, including the unchanged E11F M0 rejection. Eight ROM
faults are detected. A separate C fault omitting the high-byte store leaves
CPU/WRAM equal but fails the ordered bus comparison (38 versus39 accesses).
New guard/mutation additions preserve the positive inputs and source tokens;
previous passing positive matrices are reused, rather than repeated.

The real 0C/0E scene comparison passes all 9,767 paired CPU/WRAM/PPU/pixel
frames without alignment gaps, including 7,444 ordinary and480 closure
frames. Only the new M0 entry is disabled in the reference; all previous
scene bindings remain enabled. Scene0E's58 original E0B9 arrivals disappear;
its bank86 interpreted work falls60,675 to47,359 (13,316 fewer,21.95%).
Control0C remains54,984. Ordinary/control PCM match;55 scene0E PCM hashes
differ. No full cycle/audio/FPS claim. Shared original waits stay unchanged.

Incremental Windows Release and strict GCC/Clang semantic/resource CLI
builds/tests pass. Metadata stays1,563 verified/1,552 selected,752 WRAM and
465 C units. Broad full regression deferred, not reported as passing.
Fixture:tests/decomp_verify/lufia2_world_animation_width_verify.c.
Private receipt:world-animation-width-next/ACCEPTANCE_RECEIPT.json.
Foreign WIP, ROM and owner build/windows remain untouched. Nothing pushed.

## Window animation handlers and palette fade — 2026-10-16

Thirteen complete entries: 86:B454/B47E/B4BE/B4F2/B508/B748/B7CF/B7D3/B7D5/
B7DF/B7F7/B7FE/B350. They grow, rotate, contract and submit windows, animate
three layered records, control the color pulse and repeat ten palette steps
with forty original frame waits. Shared pulse/window tails have one C body;
the prior WindowFrame prefix remains token-identical. No WRAM span was added.
B7CF clears only the emission byte. B7D3 supplies ten; B7D5 is the actual
parameter entry used by B748. B350 copies 7F:DC20 to 7E:0340, then publishes
the original DMA fields. Original waits remain real in production.

The shared 14-root checkpoint includes affected B1DE and its legitimate fourth
sloped handler B4F2. It passes 33,280 direct and 16,640 actual production-ABI
CPU/full-WRAM/ordered-bus/child-snapshot cases. Separate invalid-circle-index2
routing checks pass 256 direct and 128 ABI cases using explicit opaque service
boundaries; B5BB is not invented as a second circle handler. All 704 unwind and
1,760 altered-return injections reach their boundary on each path. There are
137 semantic plus 196 additional consumer guards, 390 redirects across 26
entry/site contracts (22 distinct sites), and 28 detected in-memory ROM faults.

Ordered-bus comparison caught one extra C read: B4C8's nonzero branch skips
the B4D3 load. Only that contraction path was corrected. Two surviving rotation
mutants exposed missing preincrement width30 and age62 seeds. Expanded only
that root's positive boundaries; its 4,096 direct/2,048 ABI cases and three
fault controls were rechecked. Other positive, guard, edge and probe inputs
remain unchanged and their results are retained. No unchanged matrix was
repeated. D378 and 974F are explicit isolated-test services on both sides;
their timing/bodies are not claimed by caller checks. Formatting preserves
C tokens. The old B454 prefix proof and repaired-root source/binary are sealed.

Production transitions 0C/0E finish in 860/983 frames with 1,762 script steps.
All 9,767 paired frames, including 7,444 ordinary and 480 closure frames, match
CPU/WRAM/PPU/pixel hashes without alignment gaps. Ordinary and 0C PCM match;
0E has 589 differing PCM hashes. Full cycle/audio parity is not claimed.
All thirteen new bridges are reached. B748 has 447 native-bridge arrivals
and three original arrivals; the latter remain counted without an additional
dispatch diagnosis in this batch. No global dispatch/admission policy changed.
Other new entries have zero original arrivals on the enabled path.

Read-only bank-86 profiles: 0C falls from 106,434 to 54,984 instructions; 0E
from 76,935 to 60,675. Combined: 183,369 to 115,659, or 67,710 fewer (36.93%).
These are scoped interpreter counts, not whole-game coverage or FPS. Exact
in-memory baselines are restored; no ROM/snapshot export. GCC/Clang semantic
builds and actual resource-CLI tests, metadata and Windows Release pass; 52
real wrappers. Counts: 1,563 verified, 1,552 bindings, 752 WRAM, 465 C units.
Broad full regression was not rerun. Foreign WIP/ROM/owner build and the
existing circle timing guard remain unchanged.

## Scene phases and particle ownership — 2026-10-16

Seventeen complete entries: 86:B857/B954/B95B/B969/B97E/B9A3/B9E5/BA16/BA2C/
BA48/BAA8/BAAE/BAEC/BAFA/BB01/BB08/BB0F. They advance particle/window frames,
start and step fall/rebound/departure phases, align the scene group and load
five window definitions. Shared tails and animation resets have one C body.
The existing six-entry motion prefix remains token-identical. No WRAM alias
or duplicate animation owner was added. B90A remains a shared continuation.

B857 preserves PHX(B909), then reads the indirect bank-0 vector at 122E/122F
before invoking its child at B907. The near return reaches the B90A tail.
BAAE retains the M0 E0B9 call, including its original word animation store;
E0B9's published native contract remains M1, so that child uses original code.

The combined checkpoint covers 36,864 direct and 18,432 actual production-ABI
CPU/full-WRAM/ordered-bus/child-snapshot cases; all 304 unwind and 760 altered
return injections reach their boundary on each path. There are 166 semantic
and 238 additional consumer guards, 390 dispatch probes across 26 entry/site
contracts (18 distinct sites), and 26 detected in-memory ROM faults. D378's
APU wait is an explicit service boundary on both isolated-test sides, including
nested original children; production keeps the real call. The initial 5,120
unaffected direct cases are retained. A separate 32-case counter audit proves
their composed prefix never reaches D378. Formatting preserves all C tokens.

Production transitions 0C/0E finish in 860/983 frames with 1,762 script steps.
All 9,767 paired frames, including 7,444 ordinary and 480 closure frames, match
CPU/WRAM/PPU/pixel hashes without alignment gaps. Ordinary/control-0C PCM match;
0E has 451 differing PCM hashes. Full cycle/audio parity is not claimed.
All 17 bindings execute. B857 has 506 native and six original arrivals; B9A3
has 88 native and four original arrivals. A read-only diagnostic classifies
these as original C246 JMP(C280) and original B857 JMP(B907) continuations,
with valid M1X0/DB86/DP0 frames. They bypass paired-call HLE dispatch, rather
than failing native guards. All diagnostic hashes match the preceding native
run. No global dispatch/admission policy, fake cycles or timing guard changed.

In affected transition 0E, bank-86 interpreted work falls from 260,401 to
76,450 instructions: 183,951 fewer (70.64%). Control 0C stays at 93,934 on both
paths. These are scoped instruction counts, not whole-game coverage or FPS.
GCC/Clang semantic builds and actual resource-CLI tests, metadata and isolated
Windows Release pass; 68 real wrappers. Counts: 1,550 verified, 1,539 bindings,
752 WRAM locations, 465 C units. Broad full regression was not rerun here.

The older B508-wait attribution is corrected: synthetic circle index2 reads
B454 instruction bytes as B5BB, inside the already owned B59E scene function.
B508 itself has no wait. No duplicate B5BB function is introduced. The next
batch will cover legitimate sloped handler4 (B4F2) and affected B1DE routing.

## Scene particles, orbit and circle pulses — 2026-10-16

Six complete contracts: 86:B51C/B56D, BC0F/BCA6 and BD9F/BDF6. They manage
three spark sprites, fourteen burst sprites, two orbiting objects and circle
pulses. Shared free-slot and retirement helpers preserve each caller's carry,
width and bus order. Original 24-bit integration, phase overshoot, slot limits,
animation thresholds, stack saves and near/far child frames remain literal.
Existing world-object, window and streamed-field storage is reused. E0B9 is
already the verified StartAnimation service; no duplicate implementation.

One combined checkpoint passes 16,384 direct and 8,192 actual production-ABI
CPU/full-WRAM/ordered-bus/child-snapshot cases. All 160 unwind and 400 altered
child-return injections are reached on each path. There are 60 semantic and
84 additional consumer guards, 300 redirect probes across all 20 child sites
and 16 detected in-memory ROM faults. Probe-only seeds force dormant sites;
positive input matrices are unchanged. Final formatting preserves C tokens.

Two production transitions (0C/0F) finish in 860/1,021 frames, with 1,810 script
steps. All 9,805 paired frames, including 7,444 ordinary and 480 closure frames,
match CPU/WRAM/PPU/pixel hashes without alignment gaps. All six new bindings
execute; none of their enabled-path arrivals remain interpreted. Ordinary PCM
hashes and all 0F PCM hashes match. Transition 0C has 405 differing PCM hashes;
existing HLE timing limitations remain, so full cycle/audio parity is not claimed.
Each scene starts from the exact in-memory baseline; no ROM/snapshot export.

Read-only bank-86 profiling falls from 401,422 to 136,277 interpreted instructions:
265,145 fewer (66.05%) in these two transitions, not whole-game coverage or FPS.
GCC/Clang semantic builds and actual resource-CLI tests, metadata checks and the
isolated Windows Release build pass. There are 24 real generated wrappers.
Counts: 1,533 verified, 1,522 selected bindings, 752 WRAM locations, 465 C units.
The broad historical regression was not rerun on this revision. Foreign WIP,
owner build/windows, original ROM and existing circle timing guard are preserved.

## Sloped-window callers and frame updates — 2026-10-16

Nine complete contracts:86:C536/C54B/C4E0/C5F8,CB21/CB49/CB9E,B039/B1DE.
They select/load/submit15-byte records, build full sloped windows, clip endpoints
and advance both frame callbacks. C321/C536 share one original hardware selector
with distinct stride/base; aliased1E50/1E68 storage is reused. Existing A417,
A52F and edge/band algorithms are reused. No duplicate multiplier or WRAM span.
C5F8 keeps empty/invalid exits, rounded distances, saved P/DB, the original
drawing bank and all three scanline paths. All four CB41 words still targetCB49.
Dynamic animation handlers remain original services, including actual waits;
this batch does not claim their bodies. All near/far and synthetic callback
frames, flags, widths, carries, wrapping and ordered bus accesses remain exact.

The joint11-root checkpoint includes affectedC321 andA52F M0/M1:43008 direct
and21504 actual production-ABI CPU/full128KiB WRAM/ordered-bus/child-snapshot
cases,100 semantic plus145 additional consumer guards,330 redirect probes at
33 distinct sites,17 in-memory ROM fault controls. Each path runs400 unwind
cases (376 reached injections) and1000 altered-return cases (940 reached).
A surviving growth mutant exposed overclamped B039 seeds: its1024 direct/512ABI
positives, affected edge cases and two mutants were rechecked after adding
unclamped/clamped/wrapped growth; unchanged checks were retained. Final formatting
preserves exact C tokens. B1DE handlers use explicit fixture service boundaries. Synthetic circle index2
reads B454 code bytes as B5BB and reaches a scene script requiring NMI.
The actual circle handler B508 itself has no wait.

Four engine transitions0B/0C/0E/0F complete in379/860/983/1021 frames with3132
script steps. All11647 frames per run, including7444 ordinary and960 closure
frames, match CPU/WRAM/PPU/pixel hashes. All nine entries execute natively.
B1DE retains two original arrivals via the original C246 continuation's indirect
JMP atC280;798 arrivals use its native binding. A targeted read-only trace proves
M1X0/S1FE8, near frameC282 and predecessorsC27A/C27C/C27F/C280. The paired-call
interpreter path does not dispatch that JMP through the compiled binding.
No global admission policy was changed. The diagnostic matches all7444 ordinary
hashes and1100 scene CPU/WRAM/PPU/pixel hashes. Its playback prefix differs;
1100 PCM hashes differ from the multi-scene native run,so no cross-prefix audio
claim is made. In the paired four-scene comparison,1139 scene/closure PCM hashes
differ; existing HLE clock limitations remain. No full-cycle parity is claimed.

Each scene after the first restores the exact in-memory baseline; no snapshot
file or ROM export. Read-only bank86 profiling measures1214221 reference versus
690424 enabled instructions:523797 fewer(43.14%) across these four scenes.
This is scoped interpreter work, not whole-game coverage or an FPS benchmark.
MSVC W4/WX, GCC/Clang semantic and actual CLI checks, metadata and isolated
WindowsRelease pass.1527 verified/1516 selected/752 WRAM/464 portable C units;
36 actual generated wrappers. Full regression is deferred on this revision.
The prior sprite-batch notes now distinguish216/540 attempted cases from202/510
reached injections, confirmed by preserved-object counter-only ROM/ABI runs.
Evidence:world-window-parents-next/checks.json,runtime/comparison.json,
entry-diagnostic/CLASSIFICATION.json andACCEPTANCE_RECEIPT.json. No push.

## World scene sprite trails — 2026-10-16

Four complete entries:86:E71E/E7A2/E7CE/E804. Two original eight-record
sprite groups retain their origins, seven trail records, spawn order, animation
ages/tiles and reverse OAM emission. ExistingE650/A417 and the existing
SceneSpriteSetup child boundary are reused. Original word stores after XBA,
inactive records, age wrapping and byte4 persistence remain unchanged.
The parent retains all nine near-call sites and requires consumer S>=1F24
because its records reach1F21. No new WRAM allocation or duplicate multiplier.

The shared six-root checkpoint also covers existingE84F/E8B5:30208 direct and
15104 production-ABI CPU/full128KiB WRAM/ordered-bus/child-snapshot cases,
216 unwind-injection cases (202 reached) and540 altered-return cases
(510 reached) per path,54 semantic plus86 additional consumer guards,180 redirect probes at12 distinct child sites,10 in-memory
ROM fault controls. Only guard tests were repeated after correcting the old
E8B5 semantic-anyDB expectation; positive and edge branches stay unchanged.
Final formatting preserves exact C tokens. Existing A417 executes original
ROM in composed fixtures; its internals are not newly claimed by this batch.

The affected resource scene15 and control scene10 both complete in771 frames,
with760 script steps each. All9466 frames per run, including7444 ordinary
frames and two240-frame closures, match CPU/WRAM/PPU/pixel/rawPCM hashes.
All four new entries execute natively in15, with no original entries at those
addresses on the enabled path. The second scene restores an exact in-memory
baseline; no snapshot file or ROM export. This scoped result does not establish
general HLE cycle accuracy or arbitrary story-state preconditions.

Read-only bank86 profiling measures684762 reference versus233646 enabled
instructions across the two scenes:451116 fewer(65.88%). This is scoped
interpreter work, not whole-game coverage or an FPS benchmark. MSVC W4/WX,
GCC/Clang semantic and actual resource CLI checks, metadata and isolated
WindowsRelease pass.1518 verified/1507 selected/752 WRAM/462 portable C units;
16 actual new generated wrappers. Full regression is deferred on this revision.
Evidence:world-scene-sprite-motion-next/checks.json,compilers.json,runtime/
comparison.json andACCEPTANCE_RECEIPT.json. Nothing pushed; private tools,
ROM and unrelated work remain excluded.

## Sloped world windows — 2026-10-16

Ten complete entries:86:C831/C843/C854/C928/CA06/CA2F/CA5E/CA83/CAB2/CAD7.
They count edge rows, initialize scanlines, rasterize four signed slopes and
build upper/lower HDMA bands. Four directional leaves share one implementation.
Zero count still draws256 rows; zero complement angle and arithmetic overflow
retain M0 returns, while row exhaustion retains M1. CA06 accepts both widths,
restores its saved Y, and keeps the original indexed JSR frame and read order.
Header pointers, clipping, carries, wrapping and data-bank behavior remain exact.
The original C5F8 caller and its CB21 clipping subtree remain interpreted.
Operand-byte false entries from raw scans were excluded; no duplicates added.

One combined checkpoint passes45056 direct and22528 actual production-ABI
CPU/full128KiB WRAM/ordered-bus/child-snapshot cases,8 unwinds and20 altered
child returns per path,91 semantic plus140 additional consumer guards,15 child
redirects and14 in-memory ROM fault controls. Consumer contracts require
PB86/DB7F/DP0/D0/X16 andS1F20..1FFC with the documented entry M width.
Formatting afterward preserves exact C token streams; proofs are reused.

Three affected engine transitions0C/0E/0F complete in860/983/1021 frames with
2772 script steps. Across11028 frames per run, including7444 ordinary frames,
all PPU and pixel hashes match without alignment gaps. All ten new entries
execute natively, with no original entries at those addresses on the enabled
path. Ordinary CPU/WRAM/audio also match; scene/closure496 CPU/WRAM and1138
rawPCM hashes differ. Existing HLE clock limitations remain; no full-cycle or
whole-runtime audio equivalence is claimed. Existing circle timing eligibility
is retained on both sides. No synthetic delays or duplicate frames were added.

Read-only bank86 profiling of these three transitions measures5332021 original
instructions in the reference versus1136470 with the new bindings:4195551 fewer
(78.69%). This is scoped interpreter work, not whole-game coverage or an FPS
benchmark. GCC/Clang semantic and actual resource CLI checks, MSVC W4/WX proofs,
metadata and isolated WindowsRelease pass.1514 verified/1503 selected/752 WRAM,
462 portable C units;40 actual generated wrappers for ten new bindings.
Full regression is deferred for this revision. Private evidence is under
world-sloped-windows-next; ROM, unrelated WIP and tools remain uncommitted.

## Circular windows and scene sprite setup - 2026-10-16

Six complete contracts are reconstructed:86:C358/C40A/C444/C4A8 andE84F/E8B5.
Circular window construction keeps its zero-radius RTL, saved data bank, exact
headers/pointers and both row directions. Byte count0 still means256 rows.
The extent table retains PHP/SEP30, X/Y truncation, carried midpoint arithmetic
and status restoration. Lower-half field56 controls the original row stepping.
E84F keeps sixteen OAM attribute/high-field updates, both eight-record groups
and scene parameters. E8B5 keeps ADC6 carry across all eight records;byte4 is
untouched. ExistingE5E2/E5ED/E5FA/E609 packers are reused,not duplicated or recounted.

Three nonoverlapping WRAM spans are catalogued:1467 index,1EC2..1F21 records,
and neutral1219. The7F:DE00 extent table aliases existing actor/object storage;
no overlapping table allocation is invented. The original1467 counter naming
in world_map_objects has457 identical normalRelease executable/data/unwind and
relocation sections;debug/checksum sections are excluded from that comparison.

The shared12-root checkpoint covers six new contracts plusC2D9 and five resource
scene callers.41472 direct/20736 actual production-ABI cases compare CPU,all128KiB
WRAM,ordered operand/MMIO accesses and child snapshots. Each path adds480 child
unwinds and1200 changed returns.112 semantic plus167 additional consumer guards
pass;the ABI log repeats112 semantic guards.675 redirects cover17 distinct sites
and45 root/site occurrences. Fourteen in-memory ROM faults are detected.
The four affected ABI entries are rechecked after the consumer timing guard:
14336 cases and the ABI edge union pass;6400 unchanged ABI positives and all
41472 unchanged direct cases are reused. No identical semantic proof restart.

Consumer eligibility preserves two timing-sensitive preparation calls inB63B.
Exact live frames identify C2D9'sC314 far return,layer0 and itsB678/B67F caller
returns. C358 and its three row/table children retain the original path only
inside those frames. Stack peeks use the known WRAM buffer without guest bus
accesses. Twelve actual fallback frames and152 near misses prove the guard
without guest CPU/WRAM/bus mutation. E84F also rejects stacks below1F24 because
the original record stores reach1F21. Other tested contexts remain native.
Semantic C,software waits and original quirks are unchanged;no artificial cycles,
duplicate presentation frames or production shadow interpreter are introduced.

An initial runtime comparison exposed a real presentation difference at0E:
one fewer preparation frame also shifted later pictures. The bounded original
timing path above removes that regression. A separate sequential probe failed
during field closure before13 on both native/reference paths (vector00862D,
resumeD2C704). That experiment is retained as initial evidence. The final three
scenes therefore start from an exact saved runtime baseline rather than inherit
that unrelated carry-over. This does not claim arbitrary scene chaining or all
real-story preconditions. The in-process rollback API excludes ROM data;its
CPU/master-clock/fullWRAM/PPU fingerprint is checked before and after restoration.
DSP output-ring delivery is restored as well. No snapshot file or ROM export.

The final nine scene pairs all complete without failure:17094 frames per run,
including7444 ordinary and9650 scene/closure frames. All PPU/register/memory and
pixel fingerprints match. Frames for0C/0E/0F/10/11/12/13/14/15 are860/983/1021/
771/771/771/771/771/771;all7332 script steps match. Only the twelve explicitly
excluded preparation entries use original circle code on the enabled path.
Ordinary7444 CPU/WRAM/PPU/pixel/PCM hashes match. Scene/closure2750 CPU/WRAM and
604 rawPCM hashes differ,so whole-runtime CPU/audio/cycle parity is not claimed.
The pre-existing HLE cycle-accounting limitation remains separately documented.

Read-only original-instruction profiling of those nine scenes measures bank86
at9899677 reference versus7957355 enabled instructions:1942322 fewer (19.62%).
This is a scoped interpreter-work reduction,not whole-game coverage or an FPS
benchmark. Remaining hot blocks lie mainly in86:C8xx..CBxx and guide next work.

Strict MSVC,GCC/Clang semantic builds,actual resource CLI targets/tests,metadata,
isolated WindowsRelease and the incremental consumer rebuild pass.1504 verified,
zero drafts,1493 selected bindings,752 WRAM locations,461 portable C units
(460 sources plus CLI). Generation emits24 actual wrappers for six new entries.
Full regression is deferred on this revision;no new full-suite pass is claimed.

Private evidence:world-window-shapes-next/checks.json,timing-abi-checks.json,
timing-guards.log,counter-name-equivalence.json,compilers.json,runtime/comparison.json,
runtime/interpretation-profile.json andACCEPTANCE_RECEIPT.json. Initial failed
probe/logs remain separately named *-initial. ROM/foreign WIP/private tools stay
outside the commits. No push.

## Remaining world scene callers and window records - 2026-10-16

Fourteen complete contracts are reconstructed and bound:86:B15A/B59E/B63B,
BB5C/BE30/C04E,B19A,BBC4/BE72/BE7A/BEC4 andC321/C336/C2D9. All nineteen distinct
nonempty authored scene callbacks inAEB1's23-slot table now have semantic entry
contracts. Two empty RTS entries,the0000 target and theC00F alias remain exactly
as authored; this does not claim all descendants or a whole native scene VM.
Unknown descendants includingC358/E84F remain explicit original services.

B19A shares the existing BBE6/C08C loader. Actor group setup shares the existing
AFB2/AFE3 implementation with its original anchor coordinates. The transition
retains both scripts, both window submissions, fade, resource290, palette MVN
and object startup. BEC4 keeps its actualM0X0 entry/exit;BE7A guardsM0 returns
inside itsM1 parent. Positions, animation writes and exact child frames remain
observable. No software waits, duplicate stores or original quirks are removed.

C321 reads the hardware multiply result after the original register writes.
C336 copies all six original record bytes. C2D9 preserves four parameter reads,
the saved layer byte, buffer selection, farC358 child and selector increment.
Inspection ofC358/C4A8/C40A/C444 identifies circular-mask centerX/Y and radius;
the remaining field keeps its neutral name. No inferred rectangle replacement.
Existing1E50 records and battle/field scratch aliases are reused, without new
overlapping WRAM catalog entries.

The shared18-root checkpoint includes four affected prior callers.17920 direct
and8960 production-ABI cases compare CPU/full128KiB WRAM/ordered operand-MMIO
accesses and child snapshots. Each path also passes656 child unwinds and1640
changed-return contexts.178 semantic and252 additional consumer guards pass;
the ABI log includes the repeated178 semantic guards.1230 redirect probes cover
all82 distinct call sites. Fourteen in-memory ROM faults are detected.

The first ABI pass exposed2048 BEC4 fallbacks:its wrapper selected theM1 helper.
The corrected M0X0 adapter passes all2048 affected cases withzero fallback.
One fault control exposed correlated synthetic window-buffer inputs; independent
first-buffer parity covers the missing branch. Only1024 direct/512 ABI window
cases are repeated.16896 direct/6400 ABI positive cases and unchanged edge inputs
are reused, rather than restarting all checks. The final local-name correction
has identical137 COFF code/data/debug/unwind sections and relocation bytes;
all414 symbol records match after normalizing the one private helper name.

Strict MSVC,GCC/Clang semantic builds,actual resource CLI targets/tests,metadata
and isolated Windows Release pass. Final GCC/Clang rebuilds cover the naming
correction; unchanged CLI dependencies/results are reused.1498 verified entries,
zero drafts,1487 selected bindings,749 WRAM locations and459 portable C units
(458 sources plus CLI). Generation emits56 actual wrappers for14 new entries.
Full1965-job regression is deferred on this revision; it is not reported green.

All six original production scenes complete withzero failure. Each run records
13651 frames:7444 ordinary and6207 scene/closure frames. All PPU/register/memory/
pixel fingerprints match. Scene selectors0C/0D/0E/0F/10/16 take860/431/983/1021/
771/701 frames on both paths; script steps are800/420/962/1010/760/690 on both.
All fourteen new entry addresses execute, withno original entry for those roots
on the enabled path. No visible animation frame or script step is omitted.

Runtime equality remains scoped:ordinary7444 CPU/WRAM/PPU/pixel/PCM fingerprints
match. Across scenes/closures3804 CPU/WRAM and1104 rawPCM fingerprints differ.
Those samples are recorded, not newly localized individually. The prior
scene-resource timing review already establishes that the HLE bridge lacks
original instruction-cycle accounting and can shift unchanged sound-upload
progress. These unchanged timing contracts are not re-proven per entry; no
whole-runtime CPU/audio/cycle equivalence is claimed. No timing compensation,
semantic patch or shadow interpreter is introduced.

Private evidence:world-remaining-scenes-next/checks.json,name-equivalence.json,
compilers.json,compiler-final.json,generated-witnesses.json,runtime/comparison.json
andACCEPTANCE_RECEIPT.json. ROM remains at its original path; no ROM/private
tooling/foreign WIP included. No push.

## Scene resources, palette and actor groups - 2026-10-16

Fourteen complete bank86 entry contracts are reconstructed and bound: AFB2,
AFE3, BF70/BFF5/C002/C00F/C041, C2B3, C582, 966C, BB8C, BF99, BBE6 and C08C.
Five resource-scene variants share their actual BF7A tail, rather than duplicate
algorithms. Group scenes reuse actor/track/frame-script children. Palette
components saturate in original red/blue/green order; count zero retains256
iterations. C2B3 preserves incoming SBC carry. Window records preserve aliased
table writes and ADC carry. Palette MVN retains PHB/PLB and all register results.
Graphics upload reads the decoder's returned DP fields and preserves DMA/NMI
register order. Unknown children, including E84F, remain explicit services.

The14-root checkpoint passes20480 direct and10240 production-ABI original-ROM
cases, comparing CPU, all128KiB WRAM, ordered operand/MMIO data accesses and
child-entry snapshots. Each path adds408 child unwinds and1020 changed returned
contexts.137 semantic plus196 additional consumer guards pass; the ABI log also
repeats137 semantic guards.765 production redirect probes cover23 distinct
sites and51 root/site occurrences. No supported-entry fallback. Sixteen in-memory
ROM faults are detected. Unchanged positive binaries/sources are sealed and
reused for the subsequent edge/fault checkpoint, rather than rerun identically.
First four positive profiles per parent compose canonical new children and the
existing decompressor against actual original child instructions. Other child
bodies use explicit paired contracts; their internals are not claimed here.

Strict MSVC, GCC/Clang semantic builds, actual resource CLI targets/tests,
metadata and isolated Windows Release pass.1484 verified contracts, zero drafts,
1473 bindings,749 WRAM locations,458 portable C units (457 sources plus CLI).
Generation emits56 actual wrappers for the14 new entries. Only one new memory
span is added:1E50..1EC1 records. Existing7F field/actor storage reused for scene
window/palette work is documented as aliases, without overlapping new entries.

Three production scenes complete without failure, exercising eight new entry
addresses. Ordinary7444-frame CPU/WRAM/PPU/pixel/PCM fingerprints match exactly.
Selector0A takes448 frames on both paths and matches all688 recorded frames,
including menu closure. Selector0B takes379 frames on both paths; PPU/pictures
match all619 frames, while CPU/WRAM and rawPCM hashes differ in618/230 frames.
Selector14 takes771 native versus772 reference frames. Both execute760 script
steps. The reference has one additional black graphics-preparation picture;
after aligning that sample, all remaining PPU/register/memory/pixel states and
pictures match. No animation step is removed. Totals are9762/9763 frames.

Native integration is not cycle-exact. A focused read-only snapshot pair keeps
its8134-frame prefixes identical to the completed runs. CPU/full WRAM match at
0A completion and the following field frame, but clocks differ by14. The next
field frame reaches80:998B versus80:997D in the unchanged sound-payload sender;
A/Y/P/N and scratch0060 reflect asynchronous upload progress. Generated HLE
wrappers/generic bridges do not account the original instruction stream and
use plain, unpaced bus accessors. Clock/loader differences are therefore an
existing consumer timing limitation, not a claim of full runtime CPU/audio
parity. No invented cycles/waits or semantic fixes were added to conceal them.

One shared final-state checkpoint covers the semantic and consumer commits;
intermediate commits are not independently tested. Full1965 regression is
deferred on this local additive batch; no old CPU/ABI helper body changed.
Fixture:tests/decomp_verify/lufia2_world_scene_resources_verify.c. Private
recipes/results:world-scene-callbacks-next/build_direct.cmd,build_engine_abi.cmd,
positive-receipt.json,checks.json,compilers.json,runtime/comparison.json and
TIMING_REVIEW.md. No ROM/private tools/foreign WIP included. No push.

## World frame scripts and scene actor setup - 2026-10-16

Five complete bank86 contracts are reconstructed and bound: C1D2 runs the
scene frame script, C246 processes timed/default callbacks, C284/C2AD start
five scene actors/the individual actor, and D385 applies the incoming fade
control. D385 shares the existing D381/D37D fade writes. C1EC remains an
internal loop, with no speculative standalone binding. Original child waits,
conditional updates, plane suppression, carry/brightness exits, frame-counter
wrap and exact child continuations are preserved.

C246 reads 16-bit duration operands and masks their low byte. It retains
16-bit cumulative wrap and the timed callback followed by the default callback.
Original hand-pushed C271/C282 frames precede the indirect target reads. The
callbacks land at C272/C283. Unknown callback bodies remain child services;
returned unsupported contexts hand off at the original landing. Long host
slices retain exact C1EC/C24D continuations instead of fast-forwarding waits.
Metadata documents 1224/1226 reuse without overlapping existing memory entries.

The combined seven-root checkpoint includes affected D381/D37D:25600 direct
and12800 production-ABI original-ROM cases, CPU/full128KiB WRAM, ordered operand
data accesses/MMIO and child-entry snapshots. Each path adds344 child unwinds
and860 changed-child-return cases across repeated frames.49 semantic guards
and98 additional consumer guards pass (ABI also repeats the49 semantic cases).
330 production redirect probes cover all22 distinct child sites; zero supported
entry fallback. Twelve in-memory ROM fault controls are caught. Timed records
include zero duration, matches/misses and257/258-record cumulative wrap.
The fixture changes only heap test data at86:F000; the ROM file stays intact.
Canonical C246/C2AD compose with actual original callbacks/animation children;
unknown frame-update internals are explicit paired contracts. Test stops match
the repeated child ordinal and stack, not just a recurring PC.

Strict MSVC, GCC/Clang semantic builds, promoted metadata and isolated Windows
Release pass. Actual GCC/Clang resource CLI targets/tests pass. After the final
scene readability cleanup, only semantic targets are rebuilt; unchanged CLI
and resource-format dependencies reuse their recorded gate. Generation selects
1459 bindings and emits20 actual wrappers for the five new entries. Metadata:
1470 verified, zero drafts,748 WRAM locations,456 portable C units (455 library
sources plus CLI). No unrelated shared CPU/ABI helpers changed.

The production-runtime native/reference comparison denies only these five new
bindings. All8296 recorded CPU-register/flag/resume, WRAM, PPU, pixel andPCM
fingerprints match:7444 ordinary frames,240 menu-closing frames and612 real
world-scene frames. Sixteen additional early-scene CPU/WRAM snapshots also
match. Both runs finish without failure. Native entries rise from596 at the
prior checkpoint to1784:593 C246 calls now invoke593 native AF71 callbacks,
plus the C1D2 and D385 entries. The reference observer counts denied attempts
separately; its1191 attempted entries include595 denied new entries. These are
scenario observations, not whole-game coverage or complete VM/cycle parity.
The script uses the authentic83:B471 field-text JSL and original BA39/BA69
route, with five bytes in unused in-memory cart padding; no ROM file is edited.

One shared final-state checkpoint covers both commits. Full1965 regression
is deferred on this revision; no intermediate-commit or deferred-test pass
claim. Fixture:tests/decomp_verify/lufia2_world_frame_script_verify.c. Private
recipes/results:world-frame-script-next/build_direct.cmd,build_engine_abi.cmd,
checks.json,compilers.json,compilers-semantic-final.json,runtime/comparison.json.
No ROM, private tools or foreign WIP included. No push.

## World scene sequences and motion callbacks - 2026-10-16

Ten complete bank86 entry contracts now have verified native bindings: AEB1,
D330,9934,AF3A,AF88,AF90,AF98,AFA0,AFA8 and AF71. D330 and9934 expose shared
tails of existing D32D/992B; they do not duplicate those algorithms. Six motion
variants share one implementation. A357 and the new callbacks share the exact
position-copy operation. AEB1 retains every 8-bit selector alias, all128 indirect
table reads, original saves/restores, scene fields and child frames. Unknown
callbacks and the C1D2 frame-script owner remain original child services.

The combined checkpoint covers ten new and three affected roots:29696 direct
and14848 production-ABI original-ROM cases, comparing CPU, full128KiB WRAM,
ordered data accesses/MMIO and child-entry states. Each path adds304 unwind
and620 changed-child-return cases.128 semantic and180 additional consumer guard
cases pass; the ABI log also repeats the128 semantic guards.570 production
redirect probes cover16 distinct call sites and38 root/site occurrences, with
zero supported-entry fallback. Eleven in-memory ROM fault controls are caught.
Canonical D330/9934 are composed with their actual original nested children;
CDF5,D37D,D3A5,E0B9,E709,A2C6 also execute original instructions where valid.
Other children use explicit paired contracts, not claims about their internals.

MSVC W4/WX, GCC/Clang semantic builds, actual resource CLI targets/tests,
metadata validation and isolated Windows Release pass. Generation selects1454
bindings and emits four real dispatch witnesses for each new entry. Metadata
contains1465 verified contracts, zero drafts,748 WRAM locations and455 portable
C units (454 library sources plus CLI). Address1230 aliases the existing122E
span rather than creating an overlapping memory entry.

A private production-runtime replay compares all ten new bindings enabled versus
denied. All7444 ordinary frames have identical CPU/WRAM,PPU,pixel andPCM hashes.
An authentic field-text JSL invokes the original BA39/BA69 world-scene path with
selector2 and an in-memory script in unused cart padding; no ROM file is changed.
Both variants complete in612 frames. The native run replaces596 actual entries
(AEB1,AF3A,D330 and593 pose refreshes); AF71 still executes593 original times
under the original C1D2 owner. All852 closing-menu/scenario frames have identical
PPU,pixel andPCM hashes. One scenario CPU/WRAM fingerprint differs; exact cycle
or whole-scene CPU/WRAM equality is not claimed. Against the older recovery
instrumented replay, all7444 pictures/PPU hashes also match, but5908 CPU/WRAM
and5822 rawPCM hashes differ; that older harness is not a timing oracle for this
revision. Seven diagnostic snapshots differ only in inactive stack bytes.

One shared final-state checkpoint covers both commits, not independent tests of
intermediate revisions. Full1965 regression is deferred on this revision: the
13-root ROM/ABI union, compiler/CLI gates, full Release and runtime comparison
cover this local world-scene batch. No unrelated semantic/ABI helpers changed.
Fixture:tests/decomp_verify/lufia2_world_scene_sequence_verify.c. Private build
recipes/results:world-scene-sequence-next/build_direct.cmd,build_engine_abi.cmd,
checks.json,compilers.json,runtime/comparison.json. No ROM, tools or foreign WIP
is included. No push.

## Scene sequence and world-owner callbacks - 2026-10-16

BA39 callbacks BA69,BAC9 and BB94 now carry their complete original tails
through the optional80:9CB8 child services. Scene/map/text reloads reuse existing
children. BB94 preserves original A/X/Y/DB/P saves,operand-based world selection,
child carry,field flags and restoration order. Uncertain86:AEB1/8E:B2EE/world
owner internals remain explicit original services;no invented child semantics.
All seven regular scene callbacks have reconstructed bodies with services.
BA67 self-loop,BAED reset and out-of-table targets retain exact original
continuations. No new metadata roots/bindings or whole-VM closure claim.

Combined new+affected checkpoint:8192 direct and8192 productionABI states,
CPU/full128KiBWRAM/ordered data bus and all child-entry snapshots. Eight fixture
suites cover32 call occurrences and27 distinct sites;the world alternatives
share five sites and differ at their selected world-call site.256 unwinds and512
changed-child M/X/DP/decimal returns per path pass.480 production redirect
probes (27 distinct sites),15 direct/18 ABI guards,512 legacy index/flag states
and13 in-memory ROM fault controls pass,zero supported entry fallback.
Tests execute canonical80:A368 against its actual original instructions;
BF0B also runs actual instructions with explicitly synthetic counter changes.
Other child internals are paired contracts. Saved Y/DB,carry and overflow are
deliberately clobbered by new opaque children to exercise caller restoration.

MSVC W4/WX,full isolated Windows Release,GCC/Clang semantic targets/metadata
pass. CLI/resource dependency hashes remain unchanged and reuse recorded gates.
Recovery proof is reused from text-scene-display-next:its engine,body,shared
child helper and production bridge are byte-identical;its route returns before
the modified scene dispatcher. No shared mutable state is introduced. The same
combined-final-state checkpoint covers both planned commits;intermediate
commits are not independently tested. Full1965 and live7444 replay deferred
current revision,not reported as passing.1455 verified/1444 bindings/747 WRAM
locations/454 portable units (453src plus CLI). No ROM/tools/foreign WIP staged.
Fixture:tests/decomp_verify/lufia2_text_scene_owner_verify.c. Private reproduction
text-scene-owner-next/build_direct.cmd andbuild_engine_abi.cmd; supported ROM
plus --unwind,--returns,--guards;direct --indices and --fault are also available.
Receipt:text-scene-owner-next/ACCEPTANCE_RECEIPT.json. No push.

## Scene display callback tails - 2026-10-16

The optional80:9CB8 services path now completes BA39 callbacks BA97,BAA2 and
BAF1 (both operand branches). Original fixed-graphics uploads,force-blank/NMI
switches,two HDMA stream setups,scroll and speaker updates retain their order.
All eleven child sites preserve pushed frames and exact continuations. Frame
waits remain real original children. Existing verified graphics/upload children
are reused through the consumer; no copied algorithms or new standalone roots.
The shared index read,near frame and return/state test also serve the unchanged
no-service API. Other BA39 callbacks remain explicit original prefixes.
The child-call guard is shared with recovery without changing its contract.

One combined final-state checkpoint:4096 direct and4096 productionABI scene
states compare original CPU/full128KiBWRAM/ordered data bus (including MMIO)
and every child-entry CPU/frame snapshot. Additional88 child unwinds and176
changed-child M/X/DP/decimal returns per path cover all eleven call sites.
165 production redirect probes cover all sites with zero supported fallback.
15 direct and18 ABI guards pass.512 old-API cases cover every byte index and
both field-state flags,including out-of-table targets and8-bit shift aliases.
Six in-memory ROM faults are detected. The wait callback executes actual BF0B;
other child internals use explicit paired contracts,not new internal proofs.

Affected recovery rerun:512 direct/256 ABI complete states,424 unwinds each,
112 changed returns each,16 fade stalls each,15/18 guards and105 redirects pass.
MSVC W4/WX,full isolated Windows Release,GCC/Clang semantic targets/metadata
pass. CLI/resource linked-input hashes remain identical to recorded green
gates;those checks are reused. Full1965 and live7444-frame replay are deferred
on this revision. The preceding recovery replay is historical evidence,not a
new-source timing/frame acceptance claim. No original waits are optimized away.
1455 verified/1444 bindings/747 WRAM names/454 portable units (453src plus CLI).
The complete BA39 owner remains partial;three complete callback slices do not
promote the other scene operations or imply complete VM closure/native hits.
Fixture:tests/decomp_verify/lufia2_text_scene_display_verify.c. Reproduction:
text-scene-display-next/build_direct.cmd andbuild_engine_abi.cmd; supported ROM
argument plus --unwind,--returns,--guards;direct also --indices and --fault.
Private receipt:text-scene-display-next/ACCEPTANCE_RECEIPT.json. No push.

## Recovery text command and child services - 2026-10-16

80:A2E1 now composes its original seven child sites through the optional
Lufia2TextEngineStepWithServices API and existing 80:9CB8 production binding.
No new standalone binding or function promotion. The legacy no-service API
retains its boundary contract. Canonical party recovery and gold spending are
reused. Original C1/81 fade requests, bounded literal acknowledgement polls,
100 loop waits and one final wait preserve CPU, frames and ordered bus access.
Child unwinds retain live runtime state; cross-bank handoffs forward PB.
All nominal non-BRK text targets are now recognized when services are supplied:
199 unique targets /201 of205 table routes. Recognition does not prove full
VM closure; partial handlers and original asynchronous continuations remain.

Combined original-ROM checkpoint:512 direct and256 production-ABI recovery
states;424 child-unwind cases per path,112 changed-child contexts per path,
16 stalled-fade cases per path,15 direct and18 ABI guards. Each complete case
compares CPU/full128KiBWRAM/ordered data bus and106 child-entry snapshots.
105 production redirect probes cover all seven sites;zero supported fallback.
10240 affected gold-text states and128 legacy cross-bank cases pass. Nine
in-memory ROM faults and one missing-PB-forwarding source fault are detected.
The frame-wait child executes actual original instructions in both paths;
audio/background child internals use paired explicit contracts in this fixture.

Real production-object integration:7444 ordinary frames match the accepted
Gold checkpoint in every CPU/WRAM,PPU,PCM and pixel fingerprint;1495 original
text CPU/full-WRAM pairs pass. A synthetic script in unused in-memory ROM
padding, entered through actual83:B471 JSL, then exercises real NMI/APU recovery.
Native and original configurations both finish after357 frames:eligible party
healed,10000->9999 gold,recovery counter incremented,100+1 waits,no failures.
The native entry yields at its first asynchronous child;the original tail
completes. This is not proof that all101 frames now remain native, nor a cycle
or audio parity claim for this injected sequence. The ROM file is unchanged.

MSVC W4/WX,isolated Windows Release,GCC/Clang semantic targets and metadata
checks pass. Linked CLI/resource sources match the previously recorded green
gates and reuse those results. Full1965 regression is deferred on this revision.
One combined final-state checkpoint covers the semantic and consumer commits;
intermediate commits were not tested separately.1455 verified/1444 selected
bindings/747 WRAM locations/453 portable units (452 semantic C plus CLI).
Fixture:tests/decomp_verify/lufia2_text_recovery_verify.c. Private reproduction:
text-recovery-caller-next/build_direct.cmd andbuild_engine_abi.cmd;run the
binaries with the supported ROM and --unwind,--stalls,--returns,--guards;
the ABI fixture also accepts --legacy-banks. No ROM/private tools committed.
Receipt:text-recovery-caller-next/ACCEPTANCE_RECEIPT.json.

## Gold arithmetic and scene reload caller - 2026-10-15

Windows Release passes for this combined source.7444-frame replay has1495
original text CPU/fullWRAM pairs (1472returns/23boundaries),all green. All7444
pixel hashes,PPU hashes and43 images equal prior0dda828. Seven whole-frame
CPU/WRAM hashes differ only in the unchanged8293C8/8293CA wait PC and,four
frames,its NMI-saved PC low byte at1FEC. Exact fingerprint reconstruction
explains all seven;two also have direct prior full-WRAM byte comparisons.
Two current runs reproduce every CPU/PPU/PCM fingerprint.146 raw PCM hashes
differ from prior;audio/cycle timing parity remains open. No new-entry
whole-run hit rate is claimed because observations reset before menus.
Detailed evidence:gold-scene-callers-next/runtime-acceptance.json.

80:BF12 GoldAdd and80:BF43 GoldSpend share the existing text arithmetic.
No duplicate algorithm. Preserve the original two-part cap test and the
subtraction/undo writes when spending fails, plus PHP/PLP and near frames.
New public entries support PB80,M1X0,DP0,binary,stack1F20..1FFC and return
atBF42/BF6E. The legacy text wrapper retains its previous behavior.
80:A350 SceneReloadAndFadeIn sets DB80,writes FF separately to7FD0C2/C3,
and calls canonical83:85DC,83:AFCD,80:A368 via exact far/far/near frames.
All child returns/unwinds remain explicit. No unsafe inline text reload.
The two reload bytes have a neutral name; further meaning is unproven.

Shared original-ROM CPU/full128KiBWRAM/ordered-data-bus checkpoint:
173056 direct and35072 productionABI/fullVM cases. Covers both gold roots,
scene parent and affected text add/subtract routes. Each direct gold entry
covers all65536 amounts plus16384 structured balance/cap/borrow/context
states. Additional192 child unwinds per path cover all three scene sites;
43 guards each comprise42 public contexts and one missing-service check.
45 production redirect/nonlocal-return probes span all three child sites.
Full child-entry CPU snapshots compared. Two scene children are opaque
contract fixtures; third oracle executes original text-reset instructions.
No new proof of opaque child internals or full-game completeness claimed.
Ten in-memory ROM fault controls detected;zero supported entry fallback.
MSVC W4/WX fixtures and GCC/Clang semantic targets/metadata pass. Unchanged
linked CLI/resource inputs reuse the earlier recorded CLI gates. Full1965
regression deferred. One final-source checkpoint spans the planned commits;
intermediate commits are not independently tested.

Catalogue83:858B is a suspect D8 table read into following code bytes,not
an invented native entry. Its actual original dispatch edge stays visible.
Receipt:gold-scene-callers-next/ACCEPTANCE_RECEIPT.json.

## Cave entry, party recovery and scene callbacks - 2026-10-15

81:F5BD PartyRestoreVitals restores both values per eligible party member in
original order and increments the saturating recovery counter. Existing
F5ED/F60B share the extended loop; their original single-field behavior stays
verified. No duplicate restoration algorithms.84:878A AncientCaveEnter
carries blue inventory/equipment, saves the84:8B20 ranges bytewise viaWMDATA
and calls the already verified84:8888 reset through its exact near frame.
Both new public entries have complete child/boundary contracts and bindings.
Internal8862/886D helpers are documented through878A, not new public bindings.
New WRAM names: recovery count0B59,Cave entry count0B6A and neutralE75D bytes.

TextABD0 composes canonical Cave entry/reset/exit and preserves unknown child
frames. Unreachable post-Cave tails were pruned; if a child can later finish,
ABDD/ABE8 remain explicit original continuations. TextBA39 reads the actual
indirect table and preserves all256 byte operands, including8-bit shift
aliases. BB70 color-effect callback completes natively. BA67 self-loop,
BAED reset and unknown fade/frame/map children retain exact original targets
and live frames. Both text entries remain partial; no fullVMclosure claim.

15360 direct original-ROM CPU/full128KiB WRAM/ordered-bus cases:4096 each
new recovery and affectedF5ED/F60B,1024 each Cave entry/ABD0/BA39.4608 production
ABI/fullVM cases:1024 each three party entries,256 each Cave entry/ABD0,
1024 scene dispatch. Actual text-handler visits required; all byte operands,
nonzero/high-bit parameters and both post-effect continuations covered.
Additional64 child-unwind states each path:15424 direct/edge,4672 ABI/edge.
253 guards each:22 new public-entry contexts,28 internal contexts,203 other
actual ROM opcode routes. Zero supported entry fallback; intentional text
child handoffs are retained. Full child-entry CPU snapshots and15 production
redirect probes verify arguments, frames and nonlocal returns. Twelve in-memory
ROM fault controls detected, including both RTL kinds and saturation/masks.

Shared checkpoint composes unchanged four public groups fromdirect-final/
abi-final with final ABD0/BA39 focused runs after fixture expansion. ABD0
pruned-tail checks supersede its earlier runs; related library/compiler
checks rerun incrementally. No claim that every intermediate source or planned
commit was independently checked. The earlier PB handoff defect was fixed;
ambiguous80:C0B7 stops require the independentBA44 near-return marker.

MSVC direct/productionABI fixtures and GCC/Clang warnings-as-errors libraries
and metadata PASS. Consumer manifest selects1441 verified bindings without
drafts. Unchanged CLI/resource-format linked inputs match preceding green
CLI checks. Full Windows GUI/Rust regeneration and new gameplay replay are
DEFERRED to a larger integration checkpoint; latest full Release/replay is
the preceding0dda828 consumer revision. Full1965-job regression also deferred
current source. None is reported passing on this revision.

1452 verified entries /1441 bindings /746 WRAM names /451 portable units
(450 semantic sources+CLI). Text198unique/200of205 nominal routes recognized;
remaining9DD4,A2E1,BF08,BF09,BF0A include four originalBRK targets. Prefix
recognition is not dependency closure or a measured runtime hit rate.
Foreign WIP/ROM intact; no owner build/windows changes. Receipt:
text-cave-callbacks-next/ACCEPTANCE_RECEIPT.json. No push.

## Encounter callers and text rendering - 2026-10-15

81:8351 FieldGenerateEncounter now selects map/Cave encounter records through
the existing81:839F generator. It preserves original P/DB/fullA/X/Y, multiply
MMIO and saved frames. Unsupported contexts and slots>=32 hand off before any
changes; the original81:835E BRK remains intact. No duplicate generator, RNG,
sprite or field-record algorithms. The complete child-service contract is
verified and bound. New textAB51/BC55 compose those canonical children; ACA7
can finish through the native encounter chain. Existing BC98 placement body
is exported internally unchanged, with affected opcode68 verified throughVM.

TextB7FE centers measured windows with original Mode7 writes. Text9EC8 selects
font page3 and shares existing glyph/window completion via internal PRINT
flow. No second renderer. Original carry, lookup misses and row-underflow
behavior remain literal; no original bug was fixed in semantic C.

28,672 direct original-ROM CPU/full128KiB WRAM/ordered-bus states:8,192 selector
cases cover all256 map values and32 slots;4,096 each AB51,BC55,B7FE,9EC8,ACA7.
14,336 productionABI/fullVM cases:8,192 selector and1,024 each six text groups,
including affected68. Zero supported fallback. Actual original handler visits
required; child-entry fullCPU snapshots check restored-state-hidden arguments.
Additional128 child-unwind cases on each path and128 direct empty-window
prefixes. Totals28,928 direct/edge and14,464 ABI/edge.308 guards each fixture:
238 selector guards (224 invalid slots+14 contexts),70 internal contexts.
Production callback machinery includes15 redirect probes. Eight in-memory ROM
fault controls detected, including return opcode and restored child arguments.
Open-window extended-glyph rows and zero-width centering prefixes covered;
empty full-window underflow is retained, not collapsed or silently repaired.

GCC/Clang warnings-as-errors semantic libraries+metadata PASS. Unchanged CLI
linked inputs hash-match the preceding green CLI proof. Isolated Windows
Release PASS; initial registry-denied MSBuild was rerun with appropriate host
access, reusing generated analysis. No owner build/windows changes.
Shared7444-frame cold/intro/field/menu replay:1495 original text CPU/fullWRAM
pairs pass (1472 returns/23 boundaries). All CPU/WRAM,PPU andPCM frame hashes,
all7444 pixel hashes and43 images identical to preceding native replay. New
text routes were not visited. Selector hits are not claimed: its reported
observer resets before menus. Synthetic ROM/ABI cases cover the new paths.
This is not whole-game interpreter/cycle/audio equivalence.
Full1965-job regression deferred on current sources, not reported green.

1450 verified entries /1439 bindings /743 WRAM names /449 portable units
(448 semantic sources+CLI). Text196unique/198of205 nominal routes recognized,
seven unique remain:9DD4,A2E1,ABD0,BA39,BF08,BF09,BF0A. Four are originalBRK
targets. Recognition includes earlier partial prefixes, not fullVMclosure.
One combined checkpoint covers both planned semantic commits and consumer
binding/gitlink; intermediate commits are not independently tested. Receipt:
text-encounter-callers-next/ACCEPTANCE_RECEIPT.json. Foreign WIP and ROM intact.

## Shared actor visibility and sprite callers - 2026-10-15

80:BF6F FieldFindWindowActor reuses the existing lookup body. 8E:BAF6
FieldRevealActor and8E:BB17 FieldHideActor share one state implementation with
EventOpHideActor. Event $60/$61 keep their previous exact continuation; show
still stops at8E:BB12 BEFORE the81:8351 frame. The public reveal function has
the complete child-service contract throughBB16. 81:8351 generates a field
encounter record, NOT graphics; its inner81:839F is already verified.
No cloned lookup, party offsets, allocation or map-occupancy implementations.

TextA8F8 replaces actor sprites through existing canonical children; ACA7
reveals/marks the encounter actor and retains exact81:8351 handoffs. The
original lookup-miss behavior and stale-slot use remain literal.

28,672 direct ROM CPU/full128KiB WRAM/ordered-bus states across3 roots,2 text
routes and2 affected event routes. 7,168 production-ABI/full-VM states, zero
positive fallback; all four VM groups actually visit their original handlers.
Additional128 child-unwind and256 wide/stack-alias slot states on each path:
29,056 direct and7,552 ABI/edge cases total. 70 context guards plus203 other-ROM
route guards on each fixture; ABI fixture covers42 production root guards,
with28 internal guards and203 default-dispatch checks. Callback redirect probes
cover the production service machinery. Eleven in-memory ROM controls detected,
including all three RTL kinds and child actor-argument corruption. The latter
keeps final CPU/WRAM/bus identical but fails the full child-entry CPU snapshot.

MSVC isolated full Windows Release and GCC/Clang semantic libraries+metadata
PASS. Unchanged CLI/resource-format inputs hash-match the prior green tests;
reuse CLI checks. Full1965 regression deferred. Shared7444-frame replay passes
all1495 direct ROM text-body CPU/fullWRAM pairs (1472 returns,23 boundaries).
Text9DFA runs32 times andA8F8 twice in that replay. All43 pictures and allPPU
hashes match previous native replay. CombinedCPU/WRAM raw hashes match7442
frames; at7175/7385 only PC differs between CMP8293C8 andBEQ8293CA in the same
unchanged menu polling loop. Isolating BF6F original restores both hashes.
Focused byte snapshots prove equal WRAM; exact FNV inversion/reconstruction
proves all other recorded CPU fields match. Raw2 differences remain recorded.
PCM differs4399 frames, first2594; no cycle/audio parity claimed. Focus replays
were justified by these concrete differences, not routine repeated regression.

1449 verified entries /1438 bindings /743 WRAM names /448 portable units
(447 semantic sources +CLI). Text192unique/194of205 routes,11 unique remain;
native prefix recognition is not full opcode dependency closure.

## Text script/field caller batch - 2026-10-15

Nine internal routes: 9DFA,9E83,9F10,A041,AC6A,BBEB,BDBB,BDCA,BD99.
Typing-tone/pause and packed script repeats retain byte overflow, bank crossings,
original fetch/previous-byte frames, and flag effects. Choice-table setup enters
the existing native window path. A041 reaches A3C6 with M0: preserve that exact
boundary rather than reuse the M1-only goto reader. Shared helper unchanged.
Field callers reuse canonical collected-choice, header lookup, area transition,
edge tracing and layer publishing. Unknown children retain their original frames.
BD99 keeps the title/menu checkpoint and stops before the original reload tail.
AC6A/BDBB retain explicit unknown child boundaries; no complete UI claim.

36,864 direct ROM CPU/full128KiB WRAM cases; 9,216 production ABI/full-engine
cases, zero fallback. Full ordered data-bus comparison for eight routes;
BDBB compares CPU/full WRAM, all writes and MMIO order. Its unchanged canonical
tracer performs a host read-only WRAM preflight, excluded from read-bus identity.
65,658 pre-access/unknown-handler guards; nine in-memory ROM faults detected.
All result groups pass. Accepted results compose unchanged groups from staged
checkpoint runs; failed fixture setup or changed dependency reruns only affected
groups. Pause/repeat/window fixtures now contain valid terminal text; initial
unbounded measurement and ambiguous repeated-stop states were fixture issues.

MSVC Release semantic library + GCC/Clang semantic library and metadata PASS.
Unchanged CLI/resource-format linked inputs hash-match the prior green tests;
reuse those CLI checks. A final generated pause-counter name adds one otherwise
unused macro; expanded C tokens are identical (naming-identity.json), so no
repeated semantic proofs/compiler builds for that alias. Full GUI/full1965 and
new gameplay replay deferred; prior7444-frame replay remains earlier evidence.

1446 verified entries /1435 bindings /743 WRAM names /446 portable units
(445 semantic sources +CLI). No new public promotion or binding. Text recognition
190 unique /192 of205 routes,13 unique remain; not complete VM dependency closure.

# Auxiliary actor slots and object-sprite far entry - 2026-10-15

Two complete guarded entries now prepare auxiliary actor slots and call the
existing object-sprite builder through its original far wrapper. The text
object-actor command uses those same entries. Slot setup shares the existing
record-offset and fine-position routines. Palette copying, sprite tables and
object appearance remain in the already verified83F6CA implementation.
An experimental duplicate of that existing entry was removed before testing.

The far wrapper consumes the actual near return bytes. If original writes
change that return, it hands back at the resulting original address with the
correct stack. Eight direct and eight production-ABI cases prove both low- and
high-byte changes at two stack positions; a control suppressing that handoff
fails immediately. Bank-zero sprite writes remain bank-zero in the reused
builder, separately from DB-relative slot setup.

The shared checkpoint passes16384 direct ROM states and4096 production-ABI/ROM
states across the two new roots, existing sprite builder and text caller. The
text engine retains384 exact child boundaries, with no fallback.46 direct and
15 production guards preserve rejected states. Five ROM mutation controls cover
sprite values, banks, operand bias and both RTL return kinds. Existing actor,
graphics and text-field source is exact outside the additions. The initial
palette/near-entry candidate was already verified; no duplicate API/body/binding
or new guard is added to that existing routine.

MSVC Windows Release, GCC/Clang standalone, CLI and metadata checks pass once
on the combined source. A fresh production replay checks the combined recent
item/equipment, text-menu and actor batches. All1495 actual whole-text calls
match original CPU/full WRAM, with1439 returns and56 boundaries. All7444 frame
CPU/WRAM and PPU/image hashes match the preceding native replay, and all43
pictures match. The newly supported map-change command is reached once; other
new entries retain their synthetic proof scope.4787 PCM hashes differ beginning
at frame2656, so cycle/audio parity is not claimed. Cold boot reaches all seven
expected scene states. A new full1965-job regression remains deferred.

Totals:1446 verified entries,1435 native bindings,742 WRAM locations and445
portable C units. Text entry recognition is181 unique handlers/183 of205 routes;
22 unique handlers remain. Recognition includes guarded continuations and
native prefixes, not complete VM dependency closure or measured ROM coverage.

# Text menu and scene handoffs - 2026-10-15

Seven additional script entries now use native caller code: map-change request,
field menus, two item-reward pages and scripted battle. Existing fade, menu
selection, field-value counting and encounter routines are reused. Their actual
child frames and target banks remain intact. Menu selection keeps its82803D
consumer checkpoint, including the complete CPU state at that instruction.

One map-change handler completes natively. The other six caller prefixes retain
explicit original continuations at open fade, menu or encounter children. Their
post-menu tails remain original code and are not claimed as verified functions.
An initial reconstruction included unreachable tails; those were removed before
acceptance. In particular the sensitive full8385DC map reload is not inlined.
No public entry, binding or complete-menu implementation is newly promoted.

The shared checkpoint passes28672 direct original-ROM cases,7168 complete
text-engine/production-ABI cases,6144 live child boundaries and65627 untouched
unknown/unsupported guards. CPU/full WRAM, ordered data bus and reached menu
checkpoints match. Seven ROM mutations are detected. A separate omission control
removes only the menu checkpoint: CPU, WRAM and40 bus events still match while
the hook-presence assertion fails, proving that the observer is meaningful.

Existing engine source is exact after removing the added dispatch block;
read-only bridge instrumentation reverses exactly to production. Names and
proven WRAM constants were cleaned up with checked equivalent normalized tokens.
After pruning unproved tails, the affected ROM/ABI cases and semantic compiler
targets were checked again. Unchanged CLI/resource-format inputs reuse their
passing GCC/Clang CLI checks. MSVC, GCC and Clang semantic-library checks pass.
A fresh whole desktop build/full1965 regression is deferred: production bridge,
public ABI, bindings and CFG roots are unchanged. The preceding Windows Release
and runtime replay remain evidence for their respective earlier revisions.

Totals remain1444 verified entries,1433 bindings and742 WRAM locations;444
portable C units. Text entry recognition is180 unique handlers/182 of205 routes,
with23 unique handlers remaining. Recognition includes explicit native prefixes
and continuations; it is not complete opcode dependency closure or runtime load.

# Item, equipment and party restoration callers - 2026-10-15

Eight complete guarded ROM routines now handle party-status clearing, requested
item removal, possession flags, packed inventory subtraction and equipment-count
nibbles. Four text commands share these routines and the existing party restore
and equipment-slot helpers. The previously verified82FAA0 quantity-increase
routine is reused; its experimental duplicate was removed before integration.

Possession bits,96 inventory words and packed equipment-count nibbles remain
separate formats. Original missing-item return asymmetry, wrapping subtraction,
neighbor-nibble pollution on underflow, saturation flags and empty equipment
counting stay literal. Equipment addresses are reread through(dp2A),Y for each
access. Four byte shifts retain four separate read/modify/write bus operations.
The new fixed-byte direct-page shift leaves every previous CPU helper unchanged.

The guarded M0/X16 item routines require binary arithmetic, DP0 and bounded
stacks; quantity subtraction accepts even inventory offsets below192. Equipment
clearing accepts member IDs0..6 after low-byte masking. Party-status clearing
uses the original four record pointers, M1/X16 and unrestricted DP/DB. Rejected
states retain the original entry; nested callers retain their actual frames.

One shared checkpoint passes139264 original-ROM CPU/full-WRAM/ordered-bus cases,
including32768 cases for the reused quantity-increase routine and4096 byte-shift
cases. Production bridges/full text engine pass13312 additional ROM cases with
no fallback.153 direct and62 production-ABI guards pass, as do256 exact nested
continuations and12 injected ROM-fault controls. An initial guard expectation
omitted resume_pc; correcting the fixture required no semantic change. Private
ABI observation initially selected an older helper header; pinning its intended
header restored exact semantic-bus comparisons without changing production code.

GCC and Clang warning-clean standalone builds and resource-CLI tests pass.
Generated metadata lists were refreshed; only that outstanding documentation
check was repeated. The consumer receipt records the isolated Windows Release
checkpoint. Existing helper tokens and read-only bridge-observation identity
are checked. Formatting and comment placement preserve compiler tokens.

Totals:1444 verified entries,1433 native bindings,742 WRAM locations and443
portable C units. Text recognition is173 unique handlers/175 of205 routes,
with30 unique handlers remaining. Recognition includes exact continuations;
it is not full VM dependency closure or measured runtime coverage. A new full
1965-job regression is deferred; the prior7444-frame replay remains evidence
for its earlier combined revision, not a newly run whole-game comparison.

# Scene rendering and tile-text callers - 2026-10-15

Eight additional internal text handlers use existing native scene-pair lookup,
object copying, layer/attribute rendering and text-buffer clearing. Three share
the original rectangle setup. The two tile-text writers retain their ROM-backed
ASCII-indexed tables, byte arithmetic, separate row writes and16-bit cursor wrap.
Original NMI-register addressing, upload requests and frame waits stay literal.
Unknown upload/action children retain their actual targets and live frames.

C093 already exists as Lufia2SceneReadActorPair; the new text adapter reuses it
and the existing BFAA callback. No coordinate lookup, tile copier or renderer is
duplicated. Removing the added helper/dispatch block reproduces the preceding
scene-command/engine sources exactly. A4096-character host quantum hands back
at B137/B1C2 without changing original state or terminating the guest loop.

A shared checkpoint passes32768 direct original-ROM cases and8192 full
text-engine/production-ABI cases, including3584 exact child continuations and
no fallback. Three additional direct and three ABI cases prove exact65536-record
header and4096-character continuations.65640 pre-access guards and eight injected
ROM faults pass. CPU/full WRAM/ordered data-bus events match. An FF-ID fixture
initially read unseeded bytes after its sentinel and produced zero-sized copy
loops; valid trailing record data fixed the fixture. Original FF comparison
precedence remains unchanged. Formatting preserves C tokens.

The shared MSVC semantic-library, GCC/Clang standalone, CLI and metadata checks
pass. A fresh7444-frame production replay covers the combined three text batches:
1495 actual text-body calls match original CPU/full WRAM, with1438 returns and
57 boundaries. CPU/WRAM and PPU/image hashes match the prior native replay on
all7444 frames; all43 pictures match.2475 PCM hashes differ beginning at4970;
no cycle/audio parity is claimed. Actual newly recorded commands are A29C once
and B07823 times. This route does not reach the eight render/tile handlers;
their evidence remains the synthetic ROM/ABI cases above. The probe links the
fresh semantic library with unchanged isolated consumer/core objects, without
rebuilding the entire desktop UI.

Compiler and broader runtime results are recorded in the consumer acceptance
receipt. A new whole desktop Release/full1965 regression is deferred: generated
CFGs, metadata, bindings and production bridges are unchanged. Necessary MSVC
semantic-library and production-ABI checks share this checkpoint. No individual
intermediate-commit or reused whole-game result is claimed as a new full run.
Totals remain1436 verified entries,1425 bindings and742 WRAM locations, with
441 portable C units. Text recognition is169 unique handlers/171 of205 routes;
34 unique handlers remain. Recognition includes explicit continuations and
is not whole-VM completeness or current runtime coverage.

# Text exits, audio callers and party rebuild - 2026-10-15

Eleven more internal text handlers use C: six audio command callers, text exit,
nonlocal field restart, window-state toggle, screen-effect reset and party actor
rebuild. Existing native driver/immediate-sound helpers are reused. APU replies,
song-loader hooks, music fade hooks and unknown scene children keep their exact
original targets and pushed frames. Handler recognition includes these explicit
continuations; it does not claim every audio child or continuation tail is native.
No music root is inlined with a null consumer checkpoint.

The original C11C actor-release body is shared with window closure. Reversing
its name/return-argument change and removing the new dispatch block reproduces
the previous complete engine source exactly. The existing prompt still supplies
9CA5. Restart keeps the original S1FFF reset before JML83ACEF, without popping
old text frames. Screen reset keeps its original long word writes and widths.

The combined checkpoint passes45056 direct original-ROM states and11264
complete text-engine/production-ABI states, including7452 exact continuations
and no fallback. CPU/full WRAM/ordered data bus match.65679 pre-access guard
states and six injected ROM faults pass. The eleven direct groups passed first;
the harness selection was then narrowed to avoid routing unrelated existing
party commands through the unknown-handler check. Only the open guards were
rerun. No production semantic fix was required. Formatting preserves C tokens.

Windows Release, GCC/Clang standalone, CLI and metadata checks pass once on
the combined source. Full1965 and a fresh whole-game replay remain deferred;
no result from the previous source is presented as a run of this revision.
No public entries/bindings are added:1436 verified entries,1425 bindings and
742 WRAM locations, with439 portable C units. Text recognition is161 unique
handlers/163 of205 routes;42 unique handlers remain. These are dispatch counts,
not complete-game coverage or runtime hit rates.

# Party and inventory text commands - 2026-10-15

Twelve additional internal text handlers execute in portable C: party membership,
spell learning, member markers and percentage stat gains, actor visibility,
display offsets and object locations, plus flag/item/gold/object-bit conditions.
The existing lookup, spell-record, multiplication, stat and occupancy routines
are shared. Signed-byte widening and script jumps have one internal implementation.
Unknown party sprite children retain their original targets and live call frames.

The complete 81:F723/F74F menu-party routines are verified and selected through
two production bridges. Their four menu slots remain distinct from the text
callers' five field slots. Empty-before-duplicate precedence, rejection codes,
zero-count underflow, repeated count writes and byte-index truncation are kept.
The semantic entry accepts PB81/M1 and stack1F10..1FFC; direct page, data bank,
index width and decimal state remain observable. The consumer conservatively
selects native M1/X16/binary contexts; unsupported states retain its fallback.

One shared checkpoint passes 73728 direct original-ROM cases, including 16384
additional membership edge cases, and 14336 production-ABI/ROM cases. CPU,
full WRAM and ordered data-bus events match. The ABI cases include 1024 exact
child continuations and no fallback. There are 65706 pre-access guard cases;
six deliberate ROM faults are detected. Complete text callers use sufficient
stack for native handler entry; smaller direct-handler stacks are guarded.
The initial ABI observer stopped before an unchanged computed-jump table read
in unsupported shallow-stack cases; its caller fixtures were separated from
the direct guard proofs. No game-semantic correction was required.

Formatting preserves C tokens, joining adjacent fixture string literals.
Compiler/CLI checks belong to this combined source checkpoint; intermediate
commits are not claimed independently tested. The historical 1965-job full
regression and a fresh whole-game replay are deferred; the preceding replay
is not claimed on this revision. Totals:1436 verified entries,1425 bindings,
742 WRAM locations and438 portable C units. Text recognition reaches150 unique
handlers/152 of205 routes;53 unique handlers remain. These counts do not measure
whole-game completeness or runtime hit rates.

# Scene-control text commands - 2026-10-15

Twenty-three additional internal text routes use portable C. Object commands
apply motion/spin parameters, restart matching objects, wait for parameter
completion, face actors or groups, request animations and set scene targets.
Camera commands center the view/layers, rebuild attributes, publish layers
and restore the scene display. Color commands set channel and packed-color
interpolation. Existing native children remain shared implementations.

The object script initializer and scene-coordinate scaler are exported only
inside the library. Their bodies are unchanged after name/linkage substitution.
Wait commands retain their original byte rewinds and bank behavior. Facing
keeps first-match/missing behavior, tie-breaking and selector changes. Color
setup keeps the ignored duration operand, channel masks, signed intensity,
width changes and the original ordering of color writes. Unknown animation
and redraw children remain exact live-frame continuations.

A shared checkpoint covers 94208 direct original-ROM states, 23552 complete
text-engine/production-ABI states, two direct and two engine search-budget
continuations, and 65835 unsupported/unknown contexts. CPU, full WRAM and
ordered data-bus events match. Six ROM fault controls are detected. The first
18 direct groups were retained when two synthetic attribute-list terminators
were corrected; only the five remaining groups were run afterward. Formatting
preserves semantic C tokens; adjacent C strings in the fixture are joined for
its token comparison. No independent intermediate-commit validation is claimed.

GCC/Clang standalone, CLI and metadata checks pass once on the combined final
source. The isolated Windows Release build passes. Its first configuration took 279 seconds;
future checkpoints reuse this independent cache. A 7444-frame runtime replay
compares 1495 actual text-body calls with the ROM: 1421 returns and 74 boundaries
match CPU/full WRAM. B92C is reached twice and B93D once. All PPU/image hashes
and 43 pictures match the preceding native replay. Two combined CPU/WRAM frame
hashes and 4787 PCM hashes differ; cycle/audio equality is not established.
The historical 1965-job full regression is deferred, not a result on this source.
No public entries or runtime bindings are added. Totals remain 1434 verified
entries, 1423 bindings and 742 WRAM locations, with 437 portable C units.
Text recognition reaches 138 unique handlers / 140 of 205 routes; 65 unique
handlers remain. Recognition does not imply whole-VM or whole-game completeness.

# Text field effects and action callers - 2026-10-15

Thirteen additional internal text handlers execute in C: six radial-effect
spawners, scene/actor object placement and five actor-action callers. They
reuse the existing radial generator, complete object spawn, scene-cell chain,
filtered/raw actor lookups and primary-action implementation. No public entry
or runtime binding is added, and no child implementation is duplicated.

Count zero retains 256 radial spawns. Full object lists retain stale selectors.
Scene scaling writes all four coordinate words, including the stale fourth
byte. Actor-relative placement saves only the original saved selector. Action
frames, linked actor records, facing offsets and pending-child continuations
retain their original behavior. The scene-cell helper export preserves all
870 operation tokens after its internal name/linkage substitution.

The shared checkpoint passes 53248 direct original-ROM states and 13312
complete text-engine/production-ABI states, comparing CPU, full WRAM and exact
ordered data-bus events. Three additional direct and three full-engine cases
preserve the exact 65536-record search continuation with live frames. All
65705 unsupported/unknown contexts leave state and memory untouched. Three
semantic ROM faults are detected. Windows Release, GCC/Clang standalone,
CLI and metadata checks pass. Only whitespace formatting follows these
checks; complete C-token identity preserves their source coverage.

The previous 7444-frame gameplay replay is unchanged-source evidence for its
own checkpoint only. A new whole-game replay and the full 1965-job regression
are deferred; neither is claimed as passing on this batch. An owner link
failure overlapped an agent build in build/windows. The agent build completed
successfully, but future agent Windows builds use a separate build directory.

Totals remain 1434 verified entries, 1423 bindings and 742 WRAM locations,
with 434 portable C units. Text recognition reaches 115 unique handlers and
117 of 205 routes; 88 unique handlers remain. These are internal routes,
not new standalone functions or a percentage of whole-game functionality.

# Speaker, scene-cell and relative-position commands - 2026-10-15

Fourteen additional internal text handlers are reconstructed: seven speaker
presets, operand-selected speaker and speaker refresh, three scene-cell
callers and two relative actor-position commands. The existing speaker
updater is shared with wait-for-button; its143 operation tokens are unchanged.
Speaker4/5 retain their original preset order. Signed offsets preserve the
original byte flags while widening A, then add to16-bit fine coordinates.

Scene cells use the existing80:C05C and80:BFAA C implementations. Successful
and missing lookups preserve their exact original behavior. A65536-record
search limit hands off at80:BFBC with all caller, status and bank frames
still present. No decoder, actor lookup or positioning routine is duplicated.
Native commands require their proven PB80/M1/X16/DP0/binary/stack context;
other contexts retain the original handler. No public entry/binding is added.

The shared checkpoint passes61440 actorROM cases,1024 affected eventBC
cases,12288 sceneROM cases,three exact search-budget continuations and15360
full-engine/productionABI cases. Those include2048 originalC2A1 window
continuations.65731 actor and65575 scene guards leave unsupported states
untouched. Three ROM faults are detected. Windows Release and GCC/Clang
standalone/CLI/metadata pass once on this combined batch. Speaker formatting
was checked by complete C-token identity; no redundant semantic rerun follows.

A 7444-frame replay compares 1495 actual text-body calls with the ROM:
1421 returns and74 boundaries match CPU/full WRAM. Field callers share this
body; their canonical return9CB7 is compared at the original9DB2 RTL.
Dispatcher9D3B visits use the native handled-opcode count before stopping;
the initial first-visit checker was corrected against an isolated captured
state. Game semantics needed no fix. The corrected observer produces exactly
the same frame hashes as the earlier entry-only observer.

All pictures and PPU hashes match the preceding native replay. Two combined
CPU/WRAM frame hashes and4380PCM hashes differ; no cycle/audio equality is
claimed. Full1965 remains an earlier baseline and is deferred here. Counts
remain1434 verified entries,1423 bindings and742 WRAM locations, now433 C
units. Direct text recognition reaches102 unique handlers/104 of205 routes;
101 unique handlers remain. Recognition is not whole-VM completeness.

# Actor placement and animation controls - 2026-10-15

Four more internal text handlers now run in C:2F/A71B places an actor,
B6/AC8A writes its presentation flags, AF/ACBD sets the secondary timer
and flag0736bit7, and B0/ACE2 clears both animation-control high bits.
The first uses the existing filteredBF6F lookup; the other three use
the existing rawBF92 lookup. Their different missing/excluded behavior
and absolute-versus-long addressing remain literal.

The already reconstructed80:A74D position routine is shared with eventBC.
Its extracted body has409 identical C tokens after substituting the caller's
return address. There is one implementation of fine/cell positioning,
occupancy removal/restoration, script restart and per-actor flag changes.
The new text caller and existing event caller preserve their actual frames.

One affected checkpoint covers53248 directROM actor states across13
handlers,1024 complete eventBC cases,4096 full text-engine/productionABI
cases,65705 unknown/context guards and three detected ROM faults. The
direct groups passed first; only malformed synthetic event banks and an
unterminated list fixture needed correction. Retained passing groups were
not rerun. No semantic implementation change followed those proofs.
Windows Release and GCC/Clang standalone/CLI/metadata pass together.

Full1965 and a new whole-game replay are deferred for a broader integration
checkpoint. The preceding7444-frame replay belongs to the previous revision;
it is not claimed as a run of this new revision. All new and affected
boundaries are checked directly against original CPU/full WRAM/ordered bus.
Counts remain1434 verified entries,1423 bindings,742 locations and432 C
units. Direct text recognition is88 unique handlers across90/205 routes,
with115 unique handlers remaining; this is not whole-VM completeness.

# Dialogue actor commands - 2026-10-15

Nine additional internal handlers cover ten dialogue opcode routes inside
80:9CB8: actor script requests, facing writes, object-record bindings,
conditional actor starts, individual/aggregate motion waits, temporary
script refresh and single/all actor script restarts. The complete actor
lookup and script-pointer helpers are shared with existing reconstructed
code. No duplicate public entry or runtime binding was added.

The native boundary is PB80, M1/X16, DP0, binary arithmetic and
S1F10..1FFC. The preceding state-command module gains the same bank/stack
guards while retaining its proven decimal arithmetic. Other contexts
continue through the original handler. Missing actors deliberately retain
the original stale-slot behavior; ignored carry results are not repaired.
Original call frames, script-byte wrapping, first-match exclusion mask,
script selector restoration and9D00/9DB0 continuations remain observable.

The shared checkpoint covers36864 direct original-ROM actor states,
10240 complete engine/production-ABI cases,65653 actor guards and three
detected ROM faults. Affected state-command coverage passes184320 ROM
states and65705 guards. Seven unchanged actor groups retain their first
passing proof; only the two script reload groups needed a CPU-bank fix
and focused reruns. Full1965 is the older baseline and is deferred here.
Windows Release and GCC/Clang standalone/CLI/metadata gates pass together.

A 7444-frame input-driven replay verifies eight actual command calls
against original CPU/full WRAM. PPU state and all sampled pictures are
identical to the preceding native baseline. One combinedCPU/WRAM hash
and3765PCM hashes differ; this is not cycle/audio equality. The diagnostic
interpreter PC is restored after each live check. Counts remain1434
verified entries,1423 bindings and742 WRAM locations, now432 C units.
The text table recognizes84 unique handlers across86/205 routes;
119 unique handlers remain outside direct dispatch. These are route
counts, not whole-game functionality or complete VM verification.

# Text palette and state commands - 2026-10-15

Thirteen additional text/dialogue commands now execute in portable C inside
80:9CB8: variable subtraction, a no-op, region/layer coordinates, actor and
sound-control flags, a layer section word, both brightness fades, mosaic
transition, three palette-word operations and a Mode7-based glyph offset.
They use a shared state-command module; no duplicate public entries or
runtime bindings were introduced. The dispatch table now has75 recognized
unique handlers across76 of205 routes.128 unique handlers remain outside
this direct dispatch; recognition does not establish complete VM semantics.

The new internal command boundary requires M1/X16 and DP0. Other contexts
retain the existing original-code handoff. The original eight-bit palette
index doubling, bank-crossing operands, stack frames, CPU flags, MMIO order
and unusual subtraction sequence remain literal. Binary and decimal byte
subtraction is exhaustively checked; no algebraic shortcut replaces it.

Shared final-source verification passes184320 direct original-ROM states,
13312 complete engine/production-ABI cases,65627 unknown/context guards
and three detected ROM faults. Both standalone compilers/CLI gates and
Windows Release pass. Counts stay1434 verified entries,1423 bindings and
742 WRAM locations; the portable implementation now has431 C units.

A normal7444-frame replay reaches both new brightness-fade commands.
Their actual caller states also match the original ROM for CPU/full WRAM.
Pictures and PPU state match the preceding native baseline throughout.
PCM hashes differ from frame3580 onward; a run without the diagnostic
observer reproduces the same hashes. This is semantic verification, not
cycle-level scheduling or audio-sample equality. Full1965 is an earlier
baseline and was not repeated for this component-local command extension.

# Animated menu cursor and observed menu banks - 2026-10-15

The verified cursor interpolation at82:89FA now runs its original frame
children while other menu sprites advance their animation. The old static
animation/OAM preflight and127-pixel limit are removed. Byte coordinates,
child context changes, live unwinds and bounded-loop continuations are
covered; no frame wait or original behavior is optimized away.

The related82:9246/92F9/9971 consumer entries now accept the observed DB83
callers and other literal data banks, retaining their original width and
stack contracts. This broadens four existing native paths; counts remain
1434 verified functions,1423 bindings,742 locations and430 C units.

One combined checkpoint covers24576 menu caller ROM/ABI cases,4361 cursor
ROM/ABI cases,15 dispatcher redirections,13440 guards and two detected ROM
faults. The tracked cursor fixture independently reconstructs its four
animated sprite slots from the ROM and passes4361 cases without a capture.
Current Windows Release and GCC/Clang standalone/CLI gates pass.

The7444-frame input-driven replay has identical full WRAM, CPU registers,
PPU state, pictures and PCM audio. Only the saved resume PC differs on two
frames:82:93C8 versus93CA inside the same original frame-wait loop. Paired
frame diagnostics reproduce both baselines exactly and isolate that sole
difference. This is not cycle-level scheduling equality. Full1965 remains
an earlier baseline; the affected batch checks replace repeated full runs.

# Observed event and menu-buffer contexts - 2026-10-15

The index-independent83:C0EF position probe and83:C0FA control-change latch
now admit X8 and the valid top two-byte JSR frame atS1FFD. Their indexed
83:F0BC sibling retains its original strict guard; its BRK7B path is unchanged.
82:F660 now admits the real DB83 caller and other literal data-bank contexts.
Its first copy still reads0100..11D7 through DB before its long-address copies.
M1/X16 remains mandatory for this copy; no loop or data access is optimized.

Shared actual-source ROM/ABI proof passes12288 cases with zero fallback,
plus9216 unsupported-entry/sibling guards and two detected faults. All256
data banks, both leaf index widths and the top JSR frame are represented.
The copy verifier needed a larger instruction ceiling; its failed fixture
was corrected independently of the already matching leaf functions.

Actual6814-frame production replay records1029 native position probes,
35 native latch calls and one native buffer copy, with zero own interpreted
entries. The latter removes23297 interpreted instructions in this observed
call.31 early-menu animation frames match nearby prior-reference images at
one/two-frame offsets; no unmatched new picture appears. One sampled confirm
image differs by animation phase. CPU/WRAM, PPU and PCM frame hashes also
differ; this is not original/native cycle or audio equality.

Windows Release and both standalone compiler/CLI gates pass. Counts remain
1434 verified functions,1423 bindings,742 locations and430 C units. Full1965
is still the prior baseline, not repeated for these three localized contexts.

# Frame-wait contracts - 2026-10-15

Five additional ROM contracts are reconstructed together in system/frame_wait.c:
83:900C field wait,8E:B550 item-region wait,86:8B48 menu wait,86:974F world
wait and86:9748 counted world wait. All entry M/X combinations retain their
original data accesses, saved accumulator/status, flags and return frames.
Zero-count wrapping remains literal. A stalled clock hands off at the original
CMP after65536 polls; the counted caller hands off at its next JSR after4096
completed waits. Pending saved values and stack frames are not unwound early.

Shared original-ROM validation passes16756 cases:16696 returns,48 bounded
continuations and12 child unwinds.1276 wrong-bank/null-child guards touch no
bus. Cases include saved-status bit aliases, high-byte-only counter changes,
all M/X combinations and both poll/count limit edges. Four targeted faults
are detected. The standalone superproject verifier is retained as
tests/decomp_verify/lufia2_frame_wait_verify.c, without private opcode tables.

GCC13/Clang18 standalone library/tool/CLI gates, metadata validation, Windows
Release and the archive identity audit pass. These five timing routines are
verified but deliberately unbound: clock/NMI scheduling remains a separate
consumer contract. Existing runtime wait patches and bindings are unchanged.
Counts are1434 verified,1423 selected bindings,742 locations and430 C units.
Full1965 remains the preceding baseline, not a regression on this revision;
this additive unbound batch uses its combined affected proof and compiler gates.

# Field entry widths - 2026-10-15

PrepareSecondaryScript at83:D3F7 and TryItemRegion at8E:B63B now accept
both index widths in their existing M1 contracts. SpriteGraphicsUpload
at84:8193 already normalizes both widths; its consumer now admits all four
entry combinations. Canonical metadata contracts and function counts remain.

The existing D350 X8 reconstruction is now used by the consumer. Its explicit
83:D38D handoff preserves the original F9D4 PHX/PLA stack quirk. No original
behavior is repaired inside semantic C. The secondary-script installation
inside D350 remains shared rather than becoming another dispatcher call.

One combined current-source batch passes458752 ROM/ABI cases and53376
guard cases, including4685 D350 continuation cases. The unchanged private
direct proofs and eight three-mode fault controls are retained; two additional
primary-action faults are detected. Current standalone GCC/Clang/CLI checks,
Windows Release and6814 input-driven runtime frames pass their stated scopes.
All runtime CPU/WRAM/PPU/PCM and pixel hashes match the prior three-mode native
candidate. Its47-frame fade offset relative to the original admission baseline
remains documented; no original/native cycle or audio equality is claimed.

Full1965 remains the preceding regression baseline and was not rerun for these
four localized entry changes under the owner's batched-validation policy.
Counts remain1429 verified functions,1423 selected bindings,742 locations and
429 C units. No new function or binding is counted for broader entry support.

# Field animation and colour caller admission - 2026-10-15

Real field callers enter83:8682 and83:AEB5 atS1FFD. Both are two-byte JSR
frames with return bytes at1FFE/1FFF; the previous1FFC ceiling unnecessarily
selected the interpreter. These two existing bindings now admit that frame.
All other width/bank/DP/decimal/stack guards remain. JSL ceilings stay unchanged.

The old animation continuation and complete eight-slot function now share one
worker. The continuation stops before the child JSR, retaining the original
PHX/stack and CPU state. Unsupported legacy M0 entry hands off before access;
its verified ROM contract isM1. No function, binding or WRAM count increases.

140288 direct and140288 ABI slot states include both X widths,256 banks,
all256 slot byte values, all158 supported stack values, both child sites and
legacy boundaries.132096 ABI calls select native with15 redirects.8192 colour
states per direct/ABI suite execute5460 original child returns,45 redirects.
11904 unsupported guards and six detected faults/three baselines pass.
22 actual CPU/WRAM entry captures each replay directly and through the ABI.

Current production passes3570 intro/field frames with zero own interpreted
entries for both routines. CPU/fullWRAM, PPU/picture andPCM hashes match the
private ROM-verified native candidate. All pictures match the old admission
baseline. CPU/stack andPCM timing differ from that baseline; native dispatch
is not cycle exact, and original/native timing or audio equality is not claimed.
Original wait/startup timing owners remain unbound.

Normal Windows Release, strict429-unit C11, GCC13/Clang18 standalone library
and tool builds plus nine ROM-free CLI methods pass. Full1965 passes. Four
stale admission fixtures are replaced by five actual current-source jobs;
1588 unchanged-contract legacy object graphs relink the current library.
Repaired fade,18 battle,6title,15default,7object,7geometry and7title-motion
fixtures rebuild.194 historical fixture sources remain unavailable; this is
not a complete clean-source historical rebuild.1429 metadata names/addresses,
1423 bindings and both animation API archive definitions are unique.

# Title projection and particle animation - 2026-10-15

86:83F2 projects a title object and selects its depth; 86:856F advances
its 24 particle slots. Both public entries reuse the existing workers.
The old internal callers retain their unlimited depth search. The new
projection boundary returns at 8502 or hands off exactly at 84F4 after 4096
comparisons. The live CPU/stack/bus state is retained. Animation restores
its live saved status;its exit widths can change through stack aliases.
No private budget accessor or copied ROM tables enter the public API.

Seven actual-source jobs cover 156,672 original-ROM states and 156,672 native
adapter states, all 256 data banks, all 65,536 object/pool offsets and the full
supported 1F20..1FFD stack range. CPU/full WRAM/MMIO and ordered reads/writes
agree. Two additional 4096-comparison probes use synthetic table responses
with original ROM instructions;they are not real-gameplay evidence.
The private proof adds 8192 smaller-budget cases and 905 exact continuations.
There are 27,648 unique adapter guards and 131,588 C++ guards. Nine C, two bridge
and two memory-only ROM faults are detected with six positive baselines.
Changed return targets are checked through known and unknown destinations.

All 1964 regression jobs, strict C11 across 429 units and normal Release pass.
Four actual generated-table original/native pairs agree at literal caller
waits. Boot/reset/intro/real audio passes 4680 frames plus 90 replay frames.
Projection is observed natively with zero own interpreted instructions.
Animation may have zero boot hits because its native parent inlines it.
Inlining also reduces child-binding hit counts;binding counts alone do
not measure whole-game completeness or speedup.

The regression relinks 1592 hashed historical object graphs and rebuilds
the repaired fade, 18 battle, 6 prior title, 15 default-record, 7 title-object and
7 geometry fixtures. 194 historical fixture-source paths remain unavailable;
no complete clean-source historical rebuild is claimed. 1429 verified
function addresses/names and 1423 bindings are unique; each function has
exactly one archive definition. Six intentional timing/startup owners
remain verified without runtime bindings.

# Title geometry entry boundaries - 2026-10-15

Four original entry contracts now reuse existing semantic implementations:
86:8503 applies a product sign, 86:85C2 spawns a title particle, and
80:8450/8486 calculate sine/cosine through the original ROM table. The sign
operation and trig save/calculate/restore phases are shared once. Existing
pushed-call APIs retain their original frames; no ROM data is embedded.
The particle-pool pointer at 15BE is catalogued. The 85EB fallthrough is
not counted as another function.

Sign correction enters/exits M0X0. Particle spawning enters M1X0 and
restores live saved status. Trig supports all four width combinations;
M1X0 is its canonical metadata entry and exits are dynamic. The original
ROM accesses for angles 180..255 remain; no input clamping is introduced.
Binary arithmetic/DP0 and original data-bank addressing are preserved.

Seven actual-source jobs cover 705,536 original-ROM states and 705,536
actual adapter states, plus 32,768 original JSL/RTL caller compositions.
CPU/full WRAM/MMIO and ordered bus accesses agree. All 24 free particle
positions, exhaustion/inactive cases, saved-status aliases and eleven
altered return targets are covered. There are 24,576 consumer guards and
263,170 C++ guards. Seven C, four bridge and six memory-only ROM faults
are detected, with four positive replacement/proxy baselines.
All 1,427 metadata addresses and function names are unique; each has
exactly one definition in the current archive. All 1,421 bindings are unique.

All 1,957 regression jobs, strict C11 across 429 units and normal Release
pass. Eight real generated-table original/native pairs agree at literal
JSR/JSL caller waits, including mixed widths. Boot/reset/intro/real audio
passes over 4,680 frames and ninety snapshot replay frames. Sign/trig
entries are observed natively, with zero own interpreted instructions.
Spawning can have zero normal boot hits because the native title-object
parent already contains it internally. Binding count alone proves no
additional speedup; no whole-game or original/native timing claim is made.

The regression relinks 1,592 hashed historical object graphs with the
current library and rebuilds the repaired fade, eighteen battle, six prior
title, fifteen default-record and seven title-object fixtures. There remain
194 absent historical fixture-source paths. No complete clean-source
historical rebuild is claimed. The six intentional timing/startup owners
remain verified without runtime bindings.

# Title object activation - 2026-10-15

Three additional original entries are verified and selected natively:
86:8513 selects the depth-dependent sprite, 86:8530 sets object speed,
and 86:866A activates an object through its three original child calls.
Existing sprite/speed implementations are reused. The sprite-record
pointer is catalogued in WRAM. No ROM tables are embedded or duplicated.

All three enter with M1X0, DP0 and binary arithmetic. Saved live status
can change the speed helper's and activation parent's exit widths;
both exits are dynamic. Return-frame and status aliases, all data banks,
child unwinds and the original projection child remain explicit.

Verification covers 214,020 original-ROM states and 214,020 actual
adapter states, CPU/full WRAM/MMIO and ordered own-span bus accesses.
These include 256 original-projection/native-child compositions and all
four final child-return width combinations. Sixteen dedicated return/status
aliases, sixty parent redirects, 13,824 leaf guards and 197,382 C++ guards
pass. Four C, three bridge, three memory-only ROM and two reverted-core
faults are detected. All seven new jobs and the full 1,950-job matrix pass.
Strict C11 covers all 429 units and the normal Release build passes.

Six production original/native pairs agree at the literal caller wait.
Each new entry executes eighteen times over 4,680 boot/reset/intro/audio
frames with zero own interpreted instructions; ninety snapshot replays
agree. Original projection remains an original child; it is not promoted
by these tests. No original/native timing or whole-game coverage is claimed.

The regression links 1,592 hashed historical reference/adapter object
graphs with the current library. The repaired fade fixture, eighteen battle,
six prior title and fifteen default-record fixtures are rebuilt from
accepted sources. There remain 194 absent historical fixture-source paths;
no complete clean-source historical rebuild is claimed.

# Word shift bus order - 2026-10-15

The shared absolute-word LSR and ROR helpers now write the high byte before
the low byte, matching original 65816 read-modify-write bus order. Reads,
final values and CPU flags are unchanged. This corrects semantic fidelity
in title signed products and system multiplication; no game behavior is
intentionally patched and no native entry or binding is added by this fix.

All 786,432 original-ROM comparisons cover every word value, both carry
inputs and every data bank, including exact ordered reads and writes.
Reverting either helper is detected. The combined title checkpoint also
passes all 1,950 regression jobs, strict C11 across 429 units, normal
Release, six production pairs and 4,680 boot/reset/intro/audio frames
with ninety snapshot replays. Historical fixture-source limits remain;
this is not a claim of complete source rebuild or game coverage.

# Title record and tile helpers - 2026-10-14

Three original title entries now have portable C and native selection:
`$86:8637` finds the first active sprite record, `$86:8655` copies nineteen
ROM-backed bytes, and `$86:8740` copies word rows with the original stride.
The existing particle-stride helper is shared; the earlier title C remains
unchanged. The seventy-five thirteen-byte records and the bank-relative
current-object pointer are catalogued in WRAM. No ROM data is embedded.

The first two entries require M1X0, DP0 and binary arithmetic. The row
copier is reached with M0X0, supports all four width combinations, and
restores status/data bank from the live stack. Its exit width is dynamic.
Aliases between source, destination, counters and return frames retain
their original effects. Zero dimensions retain the original loops. The
4,096-word host budget hands off exactly before the next original load.

Verification covers 279,616 original-ROM states and 279,616 actual adapter
states, CPU/full WRAM/ordered bus, 35,328 consumer guards and 394,750 C++
guard calls. Twenty-five altered return targets and 157 exact budget
boundaries pass. Seven C, four bridge and eight memory-only ROM faults
are detected. All six new jobs, all 1,934 shared jobs, strict C11 across
429 units and normal Release pass. Actual generated selections execute
all three entries natively during 4,680 boot/reset/intro/audio frames and
ninety snapshot replays, with zero own interpreted instructions.

The historical regression run retains 1,592 hashed reference/adapter
object graphs relinked against the current library; 194 old fixture
source paths are absent. The repaired fade fixture and eighteen recent
battle fixtures are rebuilt from accepted sources. No clean rebuild of
all historical fixture sources is claimed. The boot sample establishes
neither whole-game coverage nor original/native timing or cycle equality.

# Shared battle session exit - 2026-10-14

`$84:9B3E` reuses the existing battle lifecycle to finish its original
nonlocal cancellation tail at `$81:8855`. The old preview API remains. The
battle entry and extended preview share one finish core; no function body
or metadata count is duplicated. The extended API restores the caller's
runtime widths and data bank, so its declared exit is dynamic.

The proof covers 16,384 direct and 16,384 owner-adapter session tails,
4,096 complete preview states and their actual-adapter counterparts,
2,048 original/native child compositions and 1,024 full legacy entries.
Boundary, missing-child, redirect, legacy-profile and negative-control
checks pass. Portable C++ guards pass 24,576 calls. The actual SDK owner
matches 256 original nonlocal-return pairs using controlled child services.

All 1,928 shared jobs, strict C11 across 429 units and Release pass. Two
production-object prefixes match CPU/full WRAM at the original frame wait;
4,680 boot/reset/intro/audio/replay frames pass. Frame waits stay original.
The common path after a frame yield can still resume in the interpreter;
complete battle gameplay, video and cycle equality are not claimed.

Historical verifier sources are partly absent from this workspace. Their
hashed reference and adapter objects are retained and relinked against the
current semantic library. The eighteen new fixtures compile the current
public semantic and bridge sources. This is an incremental regression run;
no clean-source rebuild of those historical fixtures is claimed.

The first full run found one obsolete palette-fade fixture guard. It
expected M1X1 rejection after an earlier verified extension admitted that
mode. A separate fixture retains both M0 guards and compares 16,384 ROM
states across both supported X widths. Reinstating the old C restriction
fails the new positive matrix. The entire 1,928-job matrix was rerun with
the repaired fixture; the failed first run remains archived. No palette
semantics or runtime selection changed in this batch.

# Battle entry clear width - 2026-10-14

The original `$81:8828` selects 8-bit accumulator accesses before clearing
the frame byte and first battle-work byte. C now retains that width change.
The prior reconstruction wrote two extra bytes. This corrects C to the ROM;
it adds no function or binding and changes no original game behavior.

1,024 original full-entry states compare CPU, full WRAM and ordered bus,
including child-produced width changes and the original block move. The
old entry with only the width correction also passes. The combined exit
batch passes all 1,928 shared jobs, strict C11 and normal Release.

# Field service declaration layout - 2026-10-14

The new field/menu service helpers follow the repository formatter after
their public API names were finalized. Every non-whitespace character and
the legacy suffix remain unchanged. No function, binding or semantic change
is added. All 1,910 shared verification jobs and strict C11 across 429 units
pass with the consumer save-profile integration.

# Field menu and window services - 2026-10-14

`$83:83A0` completes the menu-request children and original S1FFF/JMP83ACEF
tail. Its shared button predicate retains the ROM's high-first word TRB.
`$80:C21A` is the independent fourteen-instruction window timer and clear
call. `$80:9C72` now delegates its original screen/timer children while
sharing the existing text tail. The actual `$83:819D` caller is M1X1;
both index widths are tested. Legitimate child-width changes retain an
exact original continuation. Existing text VM code is reused.

49,152 direct, 4,096 extended text, 32,768 legacy-prefix, 3,072 original/native
composition, 49,152 owner-helper and 49,152 canonical-adapter states pass.
All six child sites pass 1,536 direct/1,344 adapter boundaries, 192 missing
handoffs and 135 redirects. Raw service guards pass 8,448 cases; portable
C++ guards pass 32,768 calls. Prior CBAE/81C6 regressions pass 20,480 states.
Three ROM/five actual-C/seven adapter/lookup faults are detected.

All 1,907 shared jobs, strict C11 across 429 units and Release pass. Five
production-object original/native pairs retain CPU/full WRAM: both window
timer widths, both frame-caller widths and the menu prefix at its original
frame wait. Three boot/reset/session runs retain logos, intro, real audio
and ninety snapshot replay hashes. The original menu wait stays owned by
the interpreter. No complete menu/field gameplay or cycle-equality claim.
Only C21A adds an entry; shared cores preserve the legacy APIs.

# Original text prompt reads - 2026-10-14

The prompt timer increments its elapsed counter, then the ROM reads that
counter again for CMP. C now retains that read instead of comparing the
cached incremented value. This corrects the reconstruction; it changes no
original game behavior and adds no function or runtime binding.

Original-ROM prompt/close/delay states and ordered-bus fault controls prove
the read. The combined caller batch also passes its full 1,907-job suite,
strict C11, normal Release and production lifecycle checks. The subsequent
caller checkpoint records that combined integration's complete scope.

# Field/event caller services — 2026-10-14

`$80:CBAE` now completes its original `$80:CC0E` redraw continuation through
the existing upload children. `$83:81C6` delegates all fourteen original
trigger children through explicit JSR/JSL frames. Both extend existing
entries; neither the internal redraw tail nor the existing VM is counted
as a new function. One trigger core serves both APIs. Legacy callers retain
their original boundaries and supported data-bank/direct-page profiles.

20,480 direct states, 20,480 owner-helper states and 20,480 canonical-adapter
states pass. Another 32,768 cases retain the legacy trigger contract; 512
actual event-VM message handoffs preserve PB85/DB91/X8743. All sixteen child
sites pass 4,096 direct/3,584 adapter boundaries, 512 missing-entry handoffs
and 240 redirects. Direct checks compare CPU/full WRAM and ordered bus.
The 1,024 original/native-child compositions compare ordered MMIO; remaining
children are explicit services. 18,432 C++ guard calls, strict C11 across
429 units, seven ROM/five actual-C/five adapter faults, all 1,877 shared
jobs and normal Release pass.

Three production-object caller pairs cover idle timers, idle triggers and
an original-ROM redraw script. CPU/full WRAM match at the caller wait;
each native parent executes zero interpreted instructions. Three further
boot/reset/new-session runs retain both logos, intro, audio and ninety
snapshot replay hashes. These fixtures do not claim complete field gameplay
or cycle equality. Shared owner adapters retain bank changes and missing
children without exporting stale state after an unwind.

# Field diagnostic controllers — 2026-10-14

`$85:8045/$85:8322/$85:8415` preserve the original diagnostic menu,
save/load flow and message display. Registers, text-pointer widths, input
masks, PPU writes and child frames follow the ROM. Successful loading keeps
the original stack reset and field-session tail. The message entry supports
the event caller's 16-bit text pointer and an additional tested 8-bit profile.
`$85:851C` preserves every counter read and its exact bounded continuation;
it remains unbound so the interpreter retains frame-wait ownership.

15,360 direct states, 7,168 production-adapter states and all 379 nonreturn
instructions pass. All 62 child sites pass 15,872 direct/13,888 adapter
boundaries, 930 redirects and 1,984 missing-entry handoffs. Another 988
original/native-child compositions compare CPU, WRAM, SRAM where relevant
and ordered MMIO; button/frame/save services remain explicit. Unsupported
states pass 9,600 consumer/20,480 portable guards and 40,960 C++ guard calls.
Eight ROM, eight actual-C and five adapter faults are detected.

All 1,859 shared jobs, strict C11 across 429 units and normal Release pass.
Three production-object original/native controller prefixes match CPU and
full WRAM at the original frame wait. Three cold/reset/new-session boots
reach both logos and intro with real audio; ninety snapshot replay hashes
match CPU/WRAM/PPU/APU/picture. This is no complete diagnostic gameplay or
cycle-equality claim. Three scoped adapters hand missing children to the
owning interpreter without changing other runtime paths.

The production save-prefix fixture uses DB85, while the older `$80:905F`
consumer adapter requires DB7E. Its existing guard therefore hands off:
15 parent instructions run natively and the remaining 29 are interpreted,
starting at `$85:8348`. CPU/full WRAM still match the original wait state.
Expanding that older callee's consumer profile is separate work.

# Boot controller and original handoffs — 2026-10-14

`$80:8035` preserves all fifteen calls, startup audio branches, SRAM mirror
checks, title-button test and original field/reset tails. Its bounded NMI
continuation preserves all 4,096 reads. `$80:8281` preserves the literal
field-session jump. Both APIs are verified without runtime bindings:
interpreted JML arrivals keep their original owner. Only `$80:80D3` is
selected natively; its descending clear deliberately leaves bytes 0000/0001.

15,360 ROM comparisons and matching adapter-model cases, 512 original-child
compositions, all child boundaries, stack edges, guards, C++ and fault controls
pass. Unbound-entry adapters remain private. The shared 1,836-job regression,
strict C11 across 428 units and normal Release pass. Three production boots
reach both logos and the intro with real audio, zero interpreted RAM-clear,
save-validation or video instructions, and original startup ownership.
Ninety snapshot replay hashes match CPU/WRAM/PPU/APU/picture state. Driver
waits use explicit services in synthetic tests; cycle equality is not claimed.

# Field redraw publication — 2026-10-14

`$80:FBCE` unpacks cell attributes, retains the original CFFF mask, blanks
the display, publishes redrawn layers and restores brightness. Dimension
selection reads one byte; the cell base always comes from the first layer.
The zero-product loop still processes 65,536 words. `$83:8E4C` preserves
saved registers/status, redraws layers, publishes immediate A8 and calls
the original frame-wait owner. Existing child bodies are reused.

Direct ROM, actual production-bridge, original-child compositions, all five
field/world call sites, boundaries, redirects, guards, C++ and deliberate
ROM/C/bridge faults pass. The shared 1,836-job regression, strict C11 and
normal Release pass. Frame waits remain explicit test services; live field
NMI timing and cycle-identical execution are not claimed.

# World-map sound caller — 2026-10-14

`$86:D378` forwards the original sound command through its JSL frame and
returns through its own RTS. It reuses the existing sound implementation,
retains all registers/banks and supports explicit child handoffs.

The joint field/world proof passes 10,496 direct and 10,496 actual-bridge
states, 384 original-child compositions, 1,280 direct/1,120 ABI boundaries,
75 redirects, guards and 36,864 C++ calls. Another 384 recursive native-child
compositions match CPU/full WRAM and ordered MMIO. Eight ROM, three private C and
three private frame faults are detected; integrated-source controls also
pass. All 1,836 shared jobs and normal Release pass. APU waits are explicit
services; synthetic SRAM tests never read or write real save files.

# Save validation and startup repair — 2026-10-14

`$80:90E5` preserves the original checksum comparison and carry result.
`$80:9037` checks all four save files, clears invalid files and records
the original four-bit result. Its saved status and byte-width registers
are restored literally. Existing checksum, address, clearing and encrypted
write implementations are reused; no duplicate bodies or new WRAM fields.

Joint ROM comparisons cover 17,408 direct and 17,408 production-bridge
states, complete WRAM and synthetic SRAM, ordered bus accesses and child
inputs. Compositions include original children and the native save chain.
1,024 child boundaries, 60 redirects, 7,680 consumer guards, 12,288 portable
guards and 786,432 C++ calls pass. Strict C11 across 426 units, all 1,820
verification jobs and the normal Windows Release pass. Three 1,500-frame
boots (cold, reset, new session) show both logos, intro and real audio,
with zero original save-validation or video instructions. Ninety snapshot
replay frame hashes match. No real save files are used by these tests.

# Preview controller and clear-save operation — 2026-10-14

`$84:9B3E` retains RGB editing, labels, party-status invalidation and original
selection wraps. Its original JML81:8855 stays nonreturning. The consumer
exports PB explicitly at that handoff. Both loop continuations retain the
exact CPU, WRAM, bus and child-input state after 4,096 original iterations.
`$84:9C94` remains an internal suffix, without a duplicate implementation.

`$80:911C` retains the selected-X buffer offset, 512 original pairs of word
writes, existing file writer and F0 SRAM marker. Full synthetic SRAM is
compared, including compositions with original RNG, seed and checksum
children and the original80:904F interpreter caller. No real save files
are read or written by verification.

Private ROM, bridge, boundary, owner, guard, C++ and fault-control proofs
pass. No timing, PCM, rendered-output or gameplay claim.

All 18 actual-library jobs, 26 ROM faults, four bridge faults, two actual-source
C faults, five positive controls and 786,432 executed C++ API calls pass.
Strict C11 across 426 source units, joint 1,815-job verification, normal
Windows Release, metadata and binding uniqueness pass.

# Party rendering and enemy preview — 2026-10-14

`$85:8A96` exposes the six-child party-render suffix through one shared
helper. The enclosing `$85:8A39` mode-2 behavior remains unchanged and is
checked against its previous implementation.

`$84:9D2C` retains ROM sprite selection, original record initialization,
dimension-dependent placement, graphics groups, party tilemap and frame
publication. `$84:9CD4` retains the unchanged-background fast return and
two original graphics queue records. All original child frames and context
fences remain explicit. The queue's existing M0 behavior is checked in
composition without changing its default M1 metadata or consumer profile.

Private ROM, production-bridge, child-input, owner-prefix, boundary,
fault-control and portable C/C++ checks pass. Original frame waits remain
explicit services in preview compositions. No timing, PCM, rendered-output,
whole diagnostic-loop or gameplay claim.

All 28 actual-library jobs, 25 ROM faults, six bridge faults, seven positive
controls, 1,179,648 executed C++ calls and 4,096 previous-parent regressions
pass. Strict C11 across 426 source units, joint 1,797-job verification,
normal Windows Release, metadata and binding uniqueness pass.

# Startup and enemy graphics contracts — 2026-10-14

`$80:80E1` installs the original RAM call templates and runtime values. It
restores P and retains the original index truncation caused by SEP30.
`$80:91DF` keeps the decompressor call, map clears, image copies, palette
and NMI callback installation. Malformed palette loops retain an exact
4,096-iteration continuation. Original startup caller frames are preserved.

`$84:9E1C` writes sixteen words without changing registers or flags. Direct
proof covers every X/A/DP value, all data banks and decimal modes. Consumer
selection follows the original DB7E/DP0/D0 caller profile, including stack
overlap and bank carry. Existing block-move fields share catalogued names.

`$85:DF44` rebuilds the three shared enemy graphics groups from six records.
Original lookups, rounding, multiply-register writes and reads stay ordered.
BRK A5 remains an original interpreter trap continuation; malformed loops
retain the exact DF55 state. Child unwinds and context fences stay explicit.

Private original-ROM and actual-bridge proofs, caller-owner compositions,
negative controls and portable C/C++ checks pass. CPU, full WRAM and ordered
data/MMIO are compared. No timing, PCM, rendered-output or gameplay claim.

All 27 actual-library jobs, 43 ROM faults, four bridge faults, eight positive
controls, 1,179,648 executed C++ API calls and strict C11 across 426 source
units pass. Constant names preserve identical machine code and relocations.
The joint 1,769-job verification and normal Windows Release pass. Metadata,
binding uniqueness and source checks pass.

# Palette fidelity, battle exit and startup â€” 2026-10-14

Four existing palette APIs now preserve original operand rereads, word TSB
write order and the Gray helper's X-dependent LDY/STY width. Canonical values
remain unchanged; the existing admitted X8 path also matches the ROM.
266,240 direct and 266,240 production-bridge comparisons include ordered
data/MMIO. Six effective C faults, C++ linkage and strict C11 pass privately.

`$85:EAEF` reconstructs the complete special-exit fade, palette copy, frame
hold and brightness fade. Its 11 child sites, exact mode/continuation fences,
four 4,096-iteration handoffs and original frame/NMI compositions are proved.
The shared palette address is catalogued as a base without overlapping ranges.

`$82:A4BE` reuses the verified inline dispatcher. Original table contents and
incoming frames remain literal; independent menu handlers retain boundaries.
1,024 actual interpreter owner pairs cover all eight choices under the real
native main-menu parent. No handler completeness is implied.

`$80:8117` reconstructs both original video-register streams, DMA setup and
RAM initialization. All four M/X modes and 256 DB values pass. The negative
DFF8 countdown, OAM length2020 and VRAM count0000 remain unchanged. The actual
startup JSR enters at S1FFD;128 original-prefix owner pairs preserve that ABI.

Original DMA devices remain owned by the existing bus. No rendered-output,
runtime timing, PCM or gameplay acceptance is claimed.

All 20 actual-library jobs, 17 ROM faults, three bridge faults, one public
palette-order fault, five positive controls, 327,685 executed C++ API calls
and strict C11 across 426 source units pass. The joint 1,742-job verification
and normal Windows Release pass. Metadata, binding and source checks pass.

# Party order and inline menu dispatch — 2026-10-14

`$85:9419` reconstructs the original party reversal for zero through four
members. It preserves eight-bit count scaling and actual indexed table reads;
other table targets remain literal post-JMP continuations. 16,384 direct and
consumer states each cover all count bytes, eight mirrored banks and full
register patterns. All 41 non-return states, 3,840 runtime and 4,096 semantic
zero-access guards, 262,144 C++ guards, ten ROM faults and one frame fault pass.

`$82:8028` consumes its original JSR frame and selects the inline table target.
`$82:E8CB` retains its incoming caller frame, pushes the original inner JSR,
and uses that same dispatcher. Both original A widths pass 32,768 states each
per direct and consumer path. The existing E746 prefix now shares that body;
16,384 original prefix states preserve its fixed82 bank and dispatch predicate.
Save, name and alternate handlers stay independent verified contracts.

Both APIs pass 6,912 runtime and 4,096 semantic zero-access guards and 393,216
shared C++ guard calls. Eight original-ROM faults, two host-context faults and
positive controls pass. Actual interp_bridge ownership passes 1,024 normal,
1,024 nonlocal and 768 native-parent pairs. The last group uses verified E8B2
and actual nested interp_tier_run_call_frame. Controlled caller shims, terminal
handlers and leaf services are explicit; no timing, PCM or gameplay claim.
No handler body is invented or duplicated. Invalid selectors retain original
table behavior. All private sources remain separate from the committed code.

All 12 actual-library jobs, 18 ROM mutations, three bridge faults, six
positive controls, 655,360 executed C++ API calls and strict C11 across426
source units pass. The joint 1,722-job full verification and default Windows
Release pass. Duplicate metadata and binding checks pass. The original803D
checkpoint preserves the separate Sound-menu correction;32,768 patch-aware
ROM/consumer states and an effective wrong-target mutation pass.

# Field reward application and byte extension — 2026-10-14

`$8E:C1DE` reconstructs the complete object reward presentation and application:
item/spell description, announcement, original frame/counter waits, inventory
selection, remaining value handling and field restoration. All 15 original
child sites retain JSL frames and explicit continuation boundaries. The
consumer profile is DB83/DP0/M1X0. The text child may change the saved bank;
original PLB restores it. The internal waits receive no independent binding.

4,096 direct and consumer states each cover 126 instruction states and all
15 sites. Each path passes 120 child unwinds, 360 width and 352 context fences.
3,840 runtime and 7,168 semantic guard states make no body access. 1,800 child
redirects preserve the original landing/frame contract. Both exact wait
boundaries pass eight states per path, with 64 actual original NMI resumptions.
16 actual FieldFrameServices compositions each preserve 3,200 original NMIs;
900C and NMI execute ROM, other leaves stay controlled. All 14 ROM mutations
and one incoming-frame mutation are detected. Positive controls, C++ and
strict C11 pass. Existing WRAM names preserve complete COFF except timestamp.

`$83:FAFA` reconstructs the signed-low-byte helper used by83:AA30. Negative
bytes fill the high byte withFF; positive bytes retain the original high
byte. Eight-bit NZ and the original M0 exit are retained. All 2,097,152
semantic states and 524,288 consumer states pass, covering all16-bitA and
relevant flags/widths. 3,072 runtime guard states and 131,072 unsupported C++
states make no body access. Four ROM faults and one frame fault are detected.
No timing, PCM, performance or manual gameplay acceptance is claimed.

All 15 actual-library jobs, 18 ROM mutations, two bridge mutations, four
positive controls, 131,073 executed C++ API calls and strict C11 across426
source units pass. The joint 1,710-job full verification and default Windows
Release pass. Duplicate metadata and binding checks pass.

# Field loop and scene transitions — 2026-10-14

`$83:8000` now reconstructs initialization and the complete field frame loop.
It preserves all 19 child sites and the original JMP to the existing B18E
session sibling. No new incoming guest frame or internal-loop binding is
created. `$83:84E1` reconstructs map/actor reload and both service-driven
fades; `$8E:B28F` reconstructs layer redraw, blanking and display upload.
The consumer selects these transitions only with the field caller's DB83.
Original waits, counter saturation, widths and nonlocal exits remain literal.

The loop passes 4,096 states per direct and consumer path, with 16,096 real
original NMIs each. All 19 sites are reached. 152 child unwinds, 304 M0 return
fences and 456 decimal/bank/DP fences pass per path. 2,304 runtime and 4,096
semantic unsupported states perform no bus access. All 2,280 redirects and
152 unavailable-child ownership tails pass. Eight 4,096-frame continuations
per path preserve 32,768 original NMIs and the exact8004 handoff.

The two transitions pass 8,192 canonical states plus 6,144 supplemental entry-
width states per path. 120 canonical states and all 30 sites are covered.
Each path passes 240 child unwinds, 720 width and 720 context fences. 5,760
runtime and 10,240 semantic guard states and all 3,600 redirects pass. Five
4,096-iteration poll boundaries preserve CPU, full WRAM and bus order. Their
40-state resumption witnesses run 160 original NMIs per path. 128 actual
FieldFrameServices compositions per path preserve 3,840 original NMIs.
Other leaf services stay controlled; no timing, PCM or gameplay acceptance
is claimed. Source-derived sound instruction notes are separate private work.

All 28 ROM mutations, two effective bridge/owner-stack mutations, positive
controls, three C++ calls and strict C11 pass. Constant cleanup preserves
complete COFF contents except the timestamp. The loop's field-owner depth
contract was compared exactly with the existing shared bridge. Unknown
counter/reset fields retain neutral names. No wait binding is added.

All 24 actual-library jobs, 28 original-ROM mutation checks, two bridge/
owner-stack mutations, four positive controls, three C++ API calls and
strict C11 across 426 source units pass. The complete 1,695-job verification
and default Windows Release pass. Duplicate entry and binding checks pass.

# World session owner — 2026-10-14

`$86:919E` now reconstructs initialization and the complete world-map frame
loop. Its incoming JSL at `$83:B21C` is documented in the verified field
caller. `$86:924C` and `$86:925C` remain internal continuations. The native
caller preserves its saved P/DB, stored stack, original waits, region return,
resource restart and encounter stack-reset JML. No additional wait binding
is introduced. The two plane-gating bytes retain neutral catalog names.

4,096 states per direct and runtime-ABI path cover all 74 canonical states
and 19 child sites. Each passes 152 child unwinds, 456 changed-width returns
and 152 decimal fences. 3,456 runtime and 7,168 semantic guard cases perform
no bus access. All 2,280 redirect probes pass. Eight 4,096-frame continuations
per path preserve 32,768 original NMIs and the exact `$86:925C` handoff.

512 region compositions per path run the actual region function through its
nonlocal RTL to `$83:B220`, preserving the incoming JSL and saved bytes.
512 nested input compositions per path preserve the `$86:924C` resource
restart. Other leaf services remain controlled; ordered parent accesses,
CPU, WRAM and MMIO match. Child operand reads are excluded in compositions.

The actual interpreter and current ancestor resolver pass 768 owner pairs
across region return, input restart and cross-bank encounter exit. These use
verified child bridges, controlled services and explicit synthetic successor
markers. They establish ownership, not timing or gameplay acceptance.
All 15 original-ROM mutation checks, the effective incoming-stack bridge
mutation, positive controls, executed C++ call and strict C11 pass. Naming
cleanup preserves complete object contents apart from build timestamps.

All 15 actual-library jobs, 15 original-ROM mutation checks, the incoming-
stack bridge mutation, two positive controls, the C++ API call and strict
C11 across 426 source units pass. The complete 1,671-job verification and
default Windows Release pass. Duplicate entry and binding checks pass.

# World menu callers — 2026-10-13

`$86:9B91`, `$86:9BBE`, `$86:9B30` and `$86:9ADB` preserve player
animation, scene positioning, menu return transitions and menu entry.
The menu's JSL return restores saved DB/P before continuing with fixed
register widths. The special return path skips the original restoration
calls at `$86:9B29` and `$86:9B2C`. No separate wait or sound-wrapper binding
is introduced. Text, rendering and recomp enhancements are unaffected.

16,384 states per path cover all 94 canonical nonreturn states, the actual
branch at `$86:9B27` and 40 child sites. Each path passes 320 child unwinds,
936 changed-width returns and 312 decimal fences. 4,800 redirect checks and
eight 4,096-frame continuations with 32,768 original NMIs pass. Unsupported
states perform no bus access: 15,360 runtime and 32,768 semantic cases.

512 compositions per path execute 2,459 native services across six kinds,
8,010 nested calls and 6,059 original NMIs. The counted wait stays literal.
Other child services remain controlled. CPU, WRAM, MMIO and ordered parent
accesses match; child operand reads are excluded only in compositions.
All 14 original-ROM mutation checks, the complete positive control, four
C++ API calls and nine opcode-length audits pass.

All ten actual-library jobs, 14 original-ROM mutation checks, the full
positive control, four C++ API calls and strict C11 across 426 source units
pass. The full 1,656-job verification and default Windows Release pass.
Entry, binding and implementation duplicate checks pass.

# World encounter transitions — 2026-10-13

`$86:D34A`, `$86:9DD6` and `$86:9C71` now have complete caller contracts.
They preserve scene restoration, camera/fade animation, original random
eligibility and weighted ROM selections, battle calls and saved positions.
The three normal encounter RTS exits stay distinct from the stack-reset
JML to `$83:ACEF`. The specific bridge preserves the cross-bank state before
handing that tail to its original interpreter owner. Shared scene helpers
are reused; no duplicate routines or timing/poll bindings are introduced.

Private comparisons cover 8,192 caller and 32,768 root states per path,
all 191 canonical nonreturn states and 25 child sites. Width, decimal,
unwind, unsupported-state and 3,000 redirect checks pass. Eight 4,096-frame
continuations per path execute 32,776 original NMIs. Invalid RNG outputs
exercise 26 selection boundaries after 4,096 records; these are not normal
RNG acceptance states. Original overflow, zero-weight and lookahead behavior
is retained. The 256-pair interpreter-owner test uses a synthetic successor
at `$83:ACEF`; it does not establish whole-field-start or gameplay acceptance.

512 caller compositions per path execute 25,344 native services across eight
kinds, 3,418 nested calls and 6,400 original NMIs. Fades finish through the
original NMI. The 1,024 root compositions execute 123 RNG and cell-center
services plus 99 random/multiply children. CPU, WRAM, MMIO and ordered parent
accesses match. Child operand reads are excluded only from composition traces.
All 23 original-ROM mutation checks and two bridge mutation checks pass.

All 20 actual-library jobs, 23 original-ROM mutation checks, two bridge
mutation checks, two positive controls, three C++ API calls and strict C11
across 426 source units pass. The complete 1,646-job verification and default
Windows Release pass. Entry, binding and implementation duplicate checks
pass; original timing owners remain unbound.

# World scene construction and player input — 2026-10-13

`$86:CC3C`, `$86:9766` and `$86:9758` have complete caller contracts.
Scene construction reuses the original object slots and ROM scene table.
Input retains alignment, alternate view, direction table, motion updates,
vehicle animation, fade and saved-stack resource transitions. Shared scene
helpers are reused; no internal block is duplicated or separately bound.
The perpetual `$86:924C` owner remains an explicit interpreter continuation.
The 4096-frame native boundary resumes the original loop without skipping waits.

Private comparisons cover 4096 scene states and 32768 input/caller states
per path, all 74/183 canonical parent states and 16/33 child sites. Width,
decimal, unwind, guard, redirect and long-continuation checks pass. Original
NMI runs in waits and compositions. Nested caller/root owner tests use the
actual interpreter with an explicit synthetic successor marker; they do
not claim whole-loop gameplay acceptance. CPU, WRAM, MMIO and ordered parent
accesses match. Child operand reads are excluded only in composition tests.

All28 actual-library jobs,18 effective ROM faults,two full positive controls,
three C++ API calls and strict426 C11 pass. The complete1626-job differential
verification and default Windows Release pass. Original timing owners remain
unbound; metadata, bindings and function definitions contain no duplicates.

# World region transition caller — 2026-10-13

`$86:9E4F` now has a complete original caller contract. It preserves the
region lookup, pose classification, destination copies, fade children and
saved-stack PLB/PLP/RTL exit. Existing region search and scene helpers are
reused; normal RTS paths and all restored status widths remain distinct.
Metadata records a dynamic exit width rather than inventing a fixed mode.

Private proof: 4096 parent states, all57 canonical states/five child sites;
40 child unwinds,120 changed-width returns,40 decimal fences;3840 runtime
and8192 semantic guards;600 redirects; eight original-ROM faults and two
final bridge faults.256 actual interpreter-owner pairs cover every savedP
byte. The exact production ancestor resolver covers eight host-ancestry
shapes. Both controlled-search and ROM-region compositions use existing
native services and original NMI. Child operand reads are outside composition
traces; independent parent-read comparisons remain exact. No gameplay claim.
Eleven actual-library jobs, ten effective faults, a full positive control,
one C++ API and strict426 C11 pass. The complete1598-job verification and
normal Windows Release pass. Both metadata readers accept dynamic exits
without relaxing fixed entry modes. No duplicate entry or implementation.

# Decompilation status

## World fade, tile and input callers - 2026-10-13

Six complete original-ROM callers reuse the existing scene context and child
frames. Fade progress runs through the original NMI; waiting never becomes a
successful timeout. Tile classification selects the existing vehicle scenes.
Up/down/A/B input retains the original TRB behavior and both paired-object Y
resets. Object19/20 coordinates derive from the original29-byte record layout.
Original CPU flags, widths, memory accesses and all child boundaries remain.

The independent fade/caller and tile/input parent proofs cover2560/1280 states
per path, all31/53 canonical non-return states and all13/10 child sites.
Boundary matrices retain104/80 unwinds,312/240 width changes and104/80 decimal
fences. Runtime guards15360/7680 and semantic guards32768/16384 perform no
accesses. All1560/1200 runtime redirects,15 effective faults and six C++ calls
pass. The paired Y naming refinement produces identical optimized object bytes.

Native compositions512/256 states execute27056/26272 native service returns
and10152/6112 original NMI events per path. All10/15 native service kinds are
reached, including nested scene03, both vehicle scenes and actual tile lookup.
CPU/WRAM/MMIO/write order remain exact. Child RAM/ROM reads are outside that
separate composition contract; all parent reads are independently compared.
Eight long cases per group and path retain4096-iteration original continuations
and32768 original NMI events. Other services remain explicit. Opcode lengths
are independently audited; private buffer extensions preserve every assertion.
No fully native frame-loop or hardware-timing acceptance claim is made.

Twenty actual-library jobs, 15 faults, two full controls and six C++ API
calls pass. All 426 source units pass strict portable Clang C11. The full
1587-job verification and normal Windows Release pass. No duplicate entry,
binding or scene implementation is introduced. No gameplay acceptance claim.

## World polar quadrants and vehicle scenes - 2026-10-13

Six complete entries share the existing polar quadrant operation and scene
helpers. The old sprite-chain polar caller now uses that same operation; no
duplicate internal switch is retained. Scene palette copying takes its original
source parameter, retaining B400 for the previous scenes and B200 for the new
vehicle scenes. No original timing, flags, widths, frames or waits are changed.

Polar direct proof covers 16384 original-ROM states and all 46 non-return
states with CPU, WRAM, MMIO and ordered operand-bus equality. The production ABI
selects 8192 states and retains 8192 existing decimal-policy handoffs. Runtime
and semantic guards each cover 12288 rejected states without bus accesses.

Vehicle direct and production-ABI matrices each cover 1024 states, all 205
non-return states, all 54 child sites and 40310 original NMI/RTI events in the
initial matrix. A separate symmetric controlled child-RAM change covers the
additional parent control-byte branch; it is not a claim about original child
behavior. Boundary proofs each cover 432 unwinds, 1296 width changes and 432
decimal fences. Guards cover 7680 runtime and 16384 semantic rejected entries
with zero accesses. All 6480 actual runtime redirects pass.

Vehicle composition covers 256 states per path, 91172 actual native service
returns and 36434 original NMI/RTI events. Existing child contracts compare CPU,
WRAM and MMIO/write order; child RAM/ROM operand reads are outside that separate
composition contract. Independent matrices compare every parent operand read.
Twenty-four cases per path retain exact 4096-iteration live continuations.
Thirteen effective faults and six C++ API calls pass. Private long-trace buffer
extensions preserve every assertion. Other services remain explicit; no fully
native frame loop or original hardware-timing acceptance is claimed.

Thirteen actual-library jobs, four shared-caller regressions, 13 faults, two
full controls and six C++ API
calls pass. All 426 source units pass strict portable Clang C11. The full
1567-job verification and normal Windows Release pass. No duplicate entry,
binding or quadrant implementation is introduced. No gameplay acceptance claim.

## World-map entry and scene callers - 2026-10-13

Four complete entries apply an entry position, run scene animations 03/06 and
handle scene input. They reuse the existing region and scene contexts. The two
scene entries share one body. Original coordinate branches, view-word copies,
palette segments, input edges, child frames and open waits remain literal.
No timing optimization or new source unit is introduced.

Entry-position direct and production ABI proofs cover 4096 states each, all 72
non-return states and seven child sites. They execute 5648 original children;
native composition uses 7536 callbacks without interpreted native children.
Each boundary path covers 56 unwinds, 168 width changes and 56 decimal fences.
The 64 malformed-list states retain their exact 4096-record continuation.
Guards cover 3840 runtime and 8192 semantic rejected states without bus accesses.
All 840 redirects, six effective faults and two C++ API calls pass. Corrected
complete opcode-length tables independently repeat all original private proofs.

The scene direct and production ABI proofs cover 12288 states each, all 233
non-return states and 44 child sites, with ordered parent operand accesses.
Each path executes 21054 original NMI/RTI events; other services are controlled
contracts. Boundary proofs cover 352 unwinds, 1056 width changes and 352 decimal
fences each. Guards cover 11520 runtime and 24576 semantic rejected entries
without bus accesses. All 5280 redirects, seven faults and six C++ calls pass.

Scene composition covers 3072 states per path, 263304 actual native service
returns and 78992 original NMI/RTI events. All nine native services are reached.
Existing child contracts compare CPU, WRAM and MMIO/write order; their RAM/ROM
operand reads are outside this composition trace contract. The independent
parent proof compares every parent operand access. Other services remain
controlled, so a fully native frame loop is not claimed. Twenty cases per path
retain exact 4096-iteration scene continuations without successful timeouts.

Nineteen actual-library jobs, 13 faults, two full controls and eight C++ API
calls pass. All 426 source units pass strict portable Clang C11. The full
1554-job verification and normal Windows Release pass. No duplicate entry,
binding or scene implementation is introduced. No gameplay acceptance claim.

## Battle status message callers - 2026-10-13

Three original entries display queued status updates, display a status message,
and expire a target's status-message timer. They share the original 45-frame
display loop and preserve literal child frames, target checks, flags, stack
effects and bus-visible register writes. Existing NMI waits and zero-mask
nontermination remain in the original runtime path. No timing patch is added.

Direct and production-core ABI matrices each cover 6144 ROM states, all 86
non-return instruction/mode states and all 27 child sites, 216331 actual child
returns, 2112 live boundaries and 95420 real ROM NMI/RTI events per path.
Each boundary proof covers 216 child unwinds, 648 changed-width returns and 216
decimal fences. 9216 runtime and 18432 semantic guards preserve rejected states.
Native top-service composition covers 1536 states, retaining 47616 original wait
tails and 97152 original nested contracts. This is not a fully native frame loop.

The existing target-mask record helper repeats the original RAM read before
each shift. 16320 all-mask ROM states prove CPU,WRAM and ordered bus equality;
its production ABI selects 4080 states and preserves 12240 DB/DP handoffs and 64
zero-mask guards. Nine effective faults,two unchanged full controls and 3240
runtime redirect probes pass.
The existing recovery and expiry scanners share these target/message operations.
Their adapters preserve the old callback boundaries. A 5162-case legacy proof
and 1024-state direct/production-ABI whole-root proofs pass, including 33074
actual child returns and 4480 original ROM NMI/RTI events per path.
Twelve actual-library jobs and the full 1535-job verification run pass.
All 426 source units pass strict Clang C11 with warnings as errors.
Four C++ APIs are executed. No duplicate entry, binding or status operation.
Default Windows Release passes.


## Field scroll mode entries - 2026-10-13

Eight complete entries share the existing field-scroll operations: camera
following, retained scroll, parallax, scaling, wrapping and shift services.
They preserve both literal child frames and all original operand-bus accesses.
The existing two-argument scroll API and its M1 entry contract are unchanged.
The original caller now reads its indirect mode table and repeats the original
vertical-scroll RAM reads rather than caching them. CPU and WRAM behavior stay
identical. Malformed return frames retain their exact continuation boundary.

Private direct and production-core ABI matrices each cover 65,536 ROM states,
218 non-return instruction/mode states, 64,639 original child returns and 96
live-frame boundaries. Native DP-zero composition uses 65,152 callbacks with
no interpreted native children. Each boundary path checks 512 child unwinds,
1,536 changed-width returns and 512 decimal fences. The guards cover 24,576
runtime and 43,008 semantic rejected entries with zero bus accesses. Fifteen
effective ROM fault controls, an unchanged full positive control, 960 runtime
redirect probes and eight executed C++ API calls pass. An ineffective low-bit
ORA fault was excluded because the next shift discards that bit.

The existing whole-root regression covers 8,192 states per direct/ABI path,
6,295 returns and 1,897 boundaries, with CPU, WRAM and ordered bus equality.
The production bridge selects 7,680 cases and preserves 512 X16 entry handoffs.
Nine actual-library jobs and the full 1523-job verification run pass.
All 426 source units pass strict Clang C11 with warnings as errors. A separate
Windows Release validation links the existing pending PPU-host renderer module
without editing its sources; the default desktop target still lacks that module
and fails on its foreign host-cost flag. The validation executable is private,
normal build settings are restored, and rendered-output acceptance is not
claimed. Public ABI checks repeat the ROM matrices, guards, redirects and
effective fault controls. No duplicate entry, binding or scroll operation.


## Actor coordinate entry contracts - 2026-10-12

Nine complete original entries share the existing target-direction, facing
comparison andaction-boundary operations. The existing actor implementation
prefix andpublic legacy ABIs remain unchanged. Both direct andproduction-core
ABI paths cover294912 states,all112 canonical instruction/mode states,both
index widths,all256 DBs andfour DP contexts,with full CPU,WRAM,MMIO andordered
operand-bus equality. There are no child calls,live cuts orinterpreted native
children.48384 runtime and36864 semantic guard cases,18 separate owned-root
instruction faults,one unchanged full control andnine executed C++ APIs pass.
Three actual-library jobs, one full 1514-job verification run and Windows
Release pass. All 426 source units pass strict Clang C11 with Wall, Wextra,
Wpedantic and Werror. The existing actor implementation prefix is unchanged.
No duplicate source, entry, API, binding or coordinate operation.


## Field screen effects - 2026-10-12

Six complete original entries share brightness, fixed-color and screen-shake
operations with the existing field event tick. Both entry index widths retain
literal original flags, bus accesses, live child frames and continuation cuts.
The existing palette entry accepts X8 and still exits X16. No duplicate palette
or screen dispatcher is introduced. Private direct and production-core ABI
proofs cover196608 states each; native composition covers98304 states each.
Existing event tick and palette entry each have16384-state direct and ABI
regressions. Eleven actual-library jobs,one full1511-job verification run andWindows
Release pass. All426 source units pass strict Clang C11 with Wall,Wextra,
Wpedantic andWerror. All312 canonical states andnine sites are covered.
Each direct/ABI path preserves125288 original child returns and8160 live cuts.
Native composition uses84448 callbacks acrossseven services,with zero
interpreted native children. Each path also passes1152 unwinds,3456 changed
widths and1152 decimal fences.2160 redirects,58880 guard states,twelve
bounded ROM controls,one positive control andsix C++ APIs pass. Existing
M0 palette guards remain intact;X8 acceptance is proved against the ROM.
Thirteen identified fields have distinct occupied extents andgenerated names.
No duplicate source,entry,API,binding orshared screen dispatcher.


## World region pointer andposition chain - 2026-10-12

Two complete original callers reuse the existing verified resource pointer.
They preserve region position scans andpending player-position requests. The
existing pointer returns M0X0 andchanges DB;the search restores DB. Combined
three-entry direct andactual ABI regression paths cover
81920 original-ROM states,12288 original child returns,all41 canonical states
andtwo call sites. Native composition covers81920 states,16384 callbacks and
zero interpreted native children. Each path passes128 entry unwinds,384
changed-width returns and128 decimal continuations. Long tests use symmetric
post-original-child fixtures:320 states,192 matches,128 exact4096 boundaries.
240 helper redirects,26752 guard states,six bounded ROM controls,one positive
control andthree C++ APIs pass. No original behavior orloop limit changes.
Existing WRAM aliases andthe common bridge are reused;no fields are added.
Nine grouped public-library jobs, one full 1,500-job verification run and
Windows Release pass. All 426 source units pass strict Clang C11 with Wall,
Wextra, Wpedantic and Werror. No duplicate source, entry, API or binding.


## World-player object and movement services - 2026-10-12

Seven complete original entry contracts preserve player-song scratch, vehicle
activation/restoration and the shared direction/tile-arrival tail. Both direct
and actual ABI paths cover all195 canonical states and31 call sites across
114688 original-ROM states,264904 child returns and46188 live cuts each.
1984 unwinds,5952 changed-width returns,3720 redirects,74368 guards,14 bounded
faults,one positive control andseven C++ APIs pass. Native composition covers
114688 states,131184 callbacks and106388 exact frontiers per path;6 of9
available services complete with zero interpreted children in the native path.
Composition frontiers include already verified services outside this test
router. They do not establish missing bindings. Timing ownership remains
independent. Existing
WRAM aliases and the common entry bridge are reused. Fetch-table correction
was audited against1024 opcode/mode pairs and affected proofs rerun.
Seven grouped public-library jobs,one full1491-job verification run and
Windows Release pass. All425 source units pass strict Clang C11 with Wall,
Wextra,Wpedantic andWerror. No source,entry,API orbinding duplicates.


## Field item selection controller - 2026-10-11

Ten complete entry contracts, nine intended finite bindings. 84:8363 remains
unbound: a synthetic frame-counter peer proves values and exact continuations,
not real NMI timing. All 425 observed non-return states and 49 child sites pass
on direct and actual production-ABI paths. Each covers 184,576 states, 418,553
original child returns and 4,072 live cuts, with full CPU, WRAM, MMIO and
ordered operand-bus comparison. Each also covers 3,136 entry unwinds, 9,408
changed-width returns and 128 decimal fences. 5,880 actual-helper redirects,
106,496 pre-access guards, twenty bounded faults, one positive control and ten
C++ APIs pass. Strict portable Clang C11 and MSVC W4/WX compile cleanly.

Native composition covers 73,728 states, 228,609 callbacks and 8,312 exact
frontiers per path; fourteen of fifteen available services actually complete.
Zero interpreted children in the native path. The original NMI wait owns its
original live frame. Two retained IO direct-page cases independently preserve
8,193 and 16,391 original child returns and exact 4,096-loop continuations on
both direct and actual ABI paths. Only isolated test resource capacities grow;
the ROM, semantic loop limits and proof expectations stay unchanged.

Diagnostics retained: wrong 1619/1699 catalog association corrected; overlapping
DP/stack addresses keep non-ROM child targets as exact live frontiers. The
private fetch table missed opcode13; all 1,024 opcode/width pairs were checked
against the local decoder and the affected batch rerun. The original twelve-
character name scan required 128 reference steps to reach its unchanged decimal
fence. No fault, trace truncation, timeout or step limit was accepted as proof.

Ten complete functions are integrated; nine finite entries are selected.
84:8363 remains verified and unbound until real timing is proven. Original
behavior, duplicate writes, input consumption and live frames stay unchanged.
Existing catalogued fields and the common field bridge are reused.
Nine grouped public-library jobs, one full 1,484-job verification run and
Windows Release pass. All 424 source units pass strict Clang C11 with Wall,
Wextra, Wpedantic and Werror. No duplicate source, entry, API or binding.


## Player action and field-item callers — 2026-10-11

Five complete M1X0, D0 entry contracts retain dynamic DP/DB: 83:BC28 player
input, 83:BCC8 movement, 83:BF3E actor release, 83:C460 field-item selection
and 83:C01C object push. All 75 child sites retain original live frames.

Six shared fixtures cover all 437 non-return instruction/mode states on both
direct and production-ABI paths: 245,760 states, 2,798,047 original child
returns and 33,453 live cuts per path. Each also covers 4,800 child-entry
unwinds, 14,400 changed-width returns and 256 decimal continuation fences.
9,000 production-helper redirect probes and 61,440 pre-access guards pass.
Twenty bounded ROM fault controls, one positive control and five C++ APIs pass.

Native composition covers 40,960 states, 205,654 native callbacks and 29,154
exact unknown frontiers per path. Thirty-nine of 56 available services are
exercised, with zero interpreted children in the native path. CPU, full WRAM,
MMIO and ordered operand-bus behavior agree. Unknown and asynchronous children
retain interpreter ownership. Existing WRAM aliases and bridge helpers are
reused; original behavior and timing are not optimized or corrected.

Seven grouped public-library jobs, one full 1,475-job verification run and
Windows Release pass. Strict Clang C11 with Wall, Wextra, Wpedantic and Werror
passes across all 423 standalone source units. Existing event push constants
are preserved as an explicit same-address alias. No source or API duplicates.


## Field interaction callers — 2026-10-11

Four complete M1X0, D0 entry contracts preserve dynamic DP/DB and 38 original
child frames: 83:BE90 leader action, 80:E817 facing-point event, 83:BFB4 actor
claim and 83:C375 facing-cell action. Gameplay phases share existing native
probe, event, actor and sound services; unknown children remain explicit.

163,840 original-ROM states per direct/production-ABI path cover all 161
canonical instruction/mode states, 716,565 actual child returns and 12,852
live cuts. Each path also checks 4,864 child-entry unwinds, 14,592 changed-width
returns and 256 decimal fences. There are 4,560 production redirect/ancestor
probes; 28,672 runtime and 20,480 semantic guards reject without bus access.
Sixteen bounded original-ROM fault controls and four C++ interfaces pass.

Native composition covers 32,768 states, 285,451 native returns and 6,594 exact
frontier cases per path across 31 services, with zero interpreted children in
the native path. Complete CPU, full WRAM, MMIO and ordered operand-bus behavior
agree. Unknown routines and asynchronous owner timing are not claimed.

Fifteen grouped public-library jobs, one full 1,468-job verification run and
Windows Release pass. Strict Clang C11 with Wall, Wextra, Wpedantic and Werror
passes across all 421 standalone source units. Existing interaction code stays
unchanged; the second claimed-actor byte reuses its existing array definition.


| Status | Meaning |
| --- | --- |
| `identified` | Boundary/address known; no semantic implementation yet. |
| `draft` | Readable semantic source exists; equivalence is not yet proven. |
| `verified` | The required decomp verification passed; consumers may select it. |
| `disabled` | Retained for reference but deliberately excluded. |

`verified` describes the reconstruction, not runtime use: whether a verified
function replaces the original at runtime is decided by the consumer's
`recomp/decomp_bindings.toml`.

## Original boot sound and menu frame synchronization

80:9383/93D6 reconstruct original boot sound initialization and resource
reset through five exact child sites.80:9C0F decodes and transfers the IPL
blocks;82:93C2 waits for the primary NMI counter's low byte to change.
All original widths,flags,DP/DB,live frames and ordered byte bus remain.
Bounded polls hand off at their exact original comparison or block header.
Boot sound roots are selected; IPL upload and menu wait remain unbound.
The current consumer word-bus pacing and asynchronous NMI contracts need
separate proof before those two bindings can be enabled. Byte-stream
equivalence does not establish real SPC timing or full native startup.
65,536 menu states per direct/ABI path;65,536 IPL header prefixes
checked by the direct helper in both fixtures;8,228 actual IPL ABI calls;
8,192 complete controlled IPL programs,32 poll and4 block-limit cuts.
256 boot/256 reset states and1,280 actual child returns per path;
640 child-unwind/changed-width states and64 root poll cuts per path.
600 production redirected-return probes,26 bounded ROM controls;
18,432 runtime/7,168 semantic guards. Native composition uses existing
sound routines:1,664 native callbacks across256 states;8,192 nested
original services remain explicit. No hardware-timing claim.
Fourteen grouped original-ROM/production-ABI jobs,26 bounded fault
controls,three positive controls and four C++ APIs pass. ONE full1,453
verification jobs and Windows Release pass. Strict Clang C11,Wall,Wextra,
Wpedantic and Werror pass across420 standalone source units.

## Complete original player controller

The existing 83:C1B4 entry now reconstructs all twenty-one child sites.
Interaction, input, terrain probes, turning and movement use shared native
services. The old two-argument API delegates to the same controller and
native children. Unknown children retain their original live call frames.
No wait, NMI, actor interaction or vehicle transition is invented.
Both index widths, varied Direct Pages and data banks are verified. FC56
and FC69 retain their original dynamic input addressing in both index widths.
The canonical metadata modes do not describe every early-return context.

Private direct and production-ABI proofs each cover 131,072 states and all
79 required non-return instruction/mode states; 853,300 actual original child
returns and 36,152 live child cuts per path. Native-prefix composition covers
65,536 states and 262,867 native child returns per path, with no interpreted
children. The compatibility API adds 196,608 independent whole-ROM prefix
states and 131,072 production-ABI states, including altered return frames.
Two input leaves add 65,536 direct and ABI states per path. CPU, full WRAM,
MMIO and ordered bus effects remain original. Twenty bounded ROM controls,
child unwinds, changed widths, decimal boundaries and pre-access guards pass.
Twenty-three grouped jobs, ONE full 1,439 and Windows Release pass.
Four extended public replays reproduce all 79 required instruction states.
Return-opcode regressions add 65,536 states per direct and ABI path, including
5,947 indexed JSR calls into the original RTL at AB60 per path. A further
65,536 original callee states check both caller-frame sizes. A native error
control detects the former invented return immediately. Original malformed
indices, rewritten return addresses and live stacks remain unchanged.

## Function counts

Generated from `metadata/functions.toml` by `scripts/metadata_index.py`; edit
the metadata, not these counts.

<!-- metadata-counts:begin (scripts/metadata_index.py) -->
1900 functions in `metadata/functions.toml`: 1900 verified, 0 draft, 0 identified, 0 disabled.
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

### Name entry and adjacent menu services

Twelve complete contracts cover original name text preparation, live tile
pointers, glyph drawing, record insertion/removal/clear, graphics staging,
selection messages, saved-buffer restoration and equipment-item flag testing.
The equipment test enters and returns in M0X0; the other eleven use M1X0.
Byte counters, separate word reads, pointer aliasing, bank carries, two-tile
glyphs, snapshot copy order, CPU flags and pushed child frames stay literal.

Shared ROM and actual production-root ABI proofs pass 36,884 states each,
including 12,288 nested parent states and 192,843 real child returns. All
41,472 guards, 435 redirects, 20 forced unwinds, 132 negative ROM controls
and twelve C++ APIs pass. Unknown text/graphics children remain services.
The existing save buffer and destination fields are reused; three neutral
nonoverlapping fields catalog the name cursor and original snapshot blocks.
Public-library proofs and all 1,105 full verification jobs pass. Windows Release passes.


### Save-menu action dispatch and shared record editing

82:E9D5 and 82:EA3B preserve the original menu/slot action loops and shared
EBF2..ECAC record actions. All 47 distinct child sites keep live call frames.
EA90 deliberately removes the caller return word before the shared tail.
Indexed directional bounds, retry paths, current flags and the SRAM flag
read/modify/write remain original. Unproven child internals stay explicit.

6,191 ROM and production-root ABI cases each include 2,048 actual slot-child
compositions, 1,708 child returns and 600 caller-owned stack handoffs. The
two roots also pass 6,912 unsupported-entry guards, 735 redirect probes,
47 forced child unwinds, 129 altered-ROM controls and two C++ API checks.
All 182 original instruction boundaries are observed. Return-word wrap,
hidden A, varied DB/DP, full WRAM/SRAM and ordered bus access are checked.
Public-library proofs and all 1,095 full verification jobs pass. Windows Release passes.


### Save-menu alternate selection and display helpers

Five complete contracts test the original list, reset the slot cursor, preserve
alternate member-selection retries, and prepare the two original alternate
displays. The byte entry check and subsequent word list count stay distinct;
indirect pointers, bank crossings, packed window geometry, widths, flags and
pushed child frames remain literal. Unknown child internals remain services.

Shared original-ROM and actual production-root ABI proofs pass 10,272 cases each,
including 32 forced child unwinds. All 17,280 guards, 540 dispatcher redirects,
113 negative ROM controls and five C++ APIs pass. No gameplay replay or actual
unknown-child composition is claimed. Existing WRAM locations are reused.
Public-library proofs and all 1,095 full verification jobs pass. Windows Release passes.


### Save-menu construction and cursor preparation

Four original contracts build all four save-slot descriptions, populate one
description from the original save reader, restore stored cursor selection,
and prepare the original cursor styles, grids, and zero visual mode. Empty and
occupied slots, dynamic indirect pointers, register restoration, byte ordering,
and nested child unwind sites remain literal. Unproven fields retain neutral names.

Shared ROM/actual production-root ABI proofs pass8,730 cases each,
including527 compositions and2,585 actual semantic child returns. All13,824
guards,375 redirects,26 forced child unwinds,62 ROM code controls andfour C++ APIs
pass. An altered ROM pointer-table byte remains a shared-data positive control.
Unknown graphics/save-reader internals remain separate services in this proof.
The same batch includes three complete save-menu callers: open the original
menu, preserve alternate action/restart loops, and prepare slot windows and
display services. Their separate proofs pass6,178 ROM/actual ABI cases each,
10,368 guards,555 redirects,34 forced child unwinds,78 negative ROM controls
andthree C++ APIs. Two transient carry changes are positive controls because
original later operations overwrite them before any observable boundary.
Unknown child internals remain explicit services. Combined batch proof has
14,908 ROM andactual ABI cases each,24,192 guards,930 redirects,60 forced
unwinds,140 negative code controls andseven C++ APIs.

Public-library proofs and all 1,087 full verification jobs pass. Windows Release passes.


### Title and save-selection services

Six original contracts cover save-selection graphics, zero/nonzero visual
setup, SRAM selection bits, indexed file flags, selection palettes, and the
file-read validity check. Live child frames, original register restoration,
status writes, indexed/unindexed SRAM reads, and palette bus order remain.
The three selection scratch fields keep neutral names.

Shared ROM/ABI fixtures pass 45,077 cases each, including 8,192
actual-child compositions and 4,096 nested flag-bridge calls. Another 23,424
unsupported states, 100 original-ROM controls, 405 redirects, and 21 forced
child unwinds pass. Six distinct C++ APIs link. Unknown child internals
remain explicit services; only the proven compositions claim those bodies.
Public-library proofs and all 1,079 full verification jobs pass. Windows Release passes.


### Active-party equipment and default start callers

`$81:EC35` starts the original cleared F000 area and default records.
`$82:9971` selects the active record from the original party-pointer table,
preserving Y and status. `$82:E893` refreshes active members' equipment and
derived stats. Its zero-count do-loop still executes 65,536 iterations.
Five exact child sites, original byte ordering and rewritten returns remain.

Shared original-ROM/production-ABI proofs each pass 18,182 cases,
including 768 actual-child compositions at 23 original call sites; 10,368
unsupported states, 26 selected ROM controls and 210 redirects pass.
All three APIs link from C++. Another ten-entry replay proves 5,505 ROM/ABI
states at stack1FF8..1FFC, 34,560 guards and 224 nested member-bridge calls.
The seven existing service bridges and these three callers now accept1FFC;
1FFD remains unsupported. Semantic code is unchanged by this guard expansion.
Public-library proofs and all 1,067 full verification jobs pass. Windows Release passes.


### Default party records and equipment services

Five equipment entries select the original slot, load its item, accumulate
seven modifiers, rebuild six slots, and refresh all seven party records.
`$81:EC56` initializes party, inventory and capsule records from `$85:B2A1`,
including all 2,024 individual WRAM-port writes. `$81:ED35` resets the seven
party records from `$85:B395`; its first member-counter clear remains one byte.
Nonzero equipped words are tested before masking their nine-bit item IDs,
preserving the original item-zero path. All original child frames and returns
remain live; unknown execution unwinds at its exact call site.

The existing full and bare member unpackers now preserve the ROM's high-byte
first 16-bit DEC/INC write order. Both bridges accept either accumulator width
with 16-bit indices, as proven directly and inside the original DB85 callers.
The canonical member metadata still records its usual M1X0 entry.

Shared original-ROM and production-ABI proofs each pass 118,171 cases,
with 28,800 unsupported states, 172 altered-ROM controls and two independent
write-order regressions. All 27 forced child unwinds and 480 redirected returns
pass. Another 2,196 composition replay states prove 896 actual member-bridge
calls. New own spans compare ordered WRAM reads and writes; existing child
interiors retain their CPU/full-WRAM/MMIO/ordered-write contracts. All seven
public APIs link from C++. Shared full decomp-verify passes all1,053 independent jobs. Windows Release passes.


### Save record callers and complete state codec

`Lufia2SaveDerivePartyStatBonuses` (`$85:CC69`..`$85:CCCD`) computes the
seven saved stat bonuses through the original base-stat child.
`Lufia2SaveRestorePartyBaseStats` (`$85:CBDC`..`$85:CC68`) restores all seven
party records and next-level experience thresholds through the original
stat/experience children. `Lufia2SavePackPartyRecord` (`$85:CA22`..`$85:CB7A`)
packs vitals, equipment, six-bit fields, stat planes and original status bits.
Their joint ROM and production-ABI batches each pass49,932 cases; all12 pushed
child sites and225 redirected runtime returns pass. All10,368 unsupported
entries and309 original operand controls pass. These bridges require DB7E,
DP0, binary native M1X0; unsupported entries retain original fallback.

`Lufia2SavePackState` (`$85:C954`..`$85:CA21`) and
`Lufia2SaveRestoreState` (`$85:C60E`..`$85:C753`) complete the remaining save
codec. Reversed field blocks, seven party records, capsule state, item tails,
checksum validation, party selection and level-mismatch services retain their
original ordering and live return frames. Unknown service bodies remain
explicit child contracts. Each shared ROM/production-ABI batch passes4,385
cases, including256 native record compositions and33 exact child unwinds.
All525 runtime child redirects,6,912 unsupported entries and300 changed-ROM
operand controls pass. Direct codec spans compare WRAM reads and writes in
order; existing native child compositions retain their separately verified
CPU/full-WRAM/MMIO/ordered-write contracts. A bank-sensitive fixture prevents
7E/7F aliasing. The original checksum-failure and level-mismatch service paths
remain unchanged. Shared full decomp-verify passes all1,041 jobs. Windows Release passes.


### Party and capsule save records

`Lufia2SaveUnpackCapsuleRecord` (`$85:C8CF`..`$85:C931`) and
`Lufia2SavePackCapsuleRecord` (`$85:CB7B`..`$85:CBD1`) reconstruct the original
capsule record format. Two header bytes, seven split form bytes and seven
packed word pairs retain their original bit order and bus rereads. Live DB:X
addresses the save stream; DP30 and derived DP2A address the live record.
Registers, scratch bytes and final flags retain the original values.

`Lufia2SaveFlagPartyLevelMismatch` (`$85:C932`..`$85:C953`) compares the three
current-experience bytes with the next-level threshold, most significant
first. A reached threshold increments the counter unless the level is99. The
8-bit DP counter retains its original wrap. All three complete contracts
use native M1X0, with DP0 required by their production bridges.

Shared original-ROM and production-ABI batches each pass 198,656 cases,
including 1,024 pack/unpack compositions. The party cases cover all seven
comparison relations and levels 1,98,99,100. Direct comparisons check every
WRAM data read and write in order; ABI comparisons check CPU, full WRAM,
ordered writes, MMIO and all three host-return modes. All 9,216 isolated
unsupported entries and 101 altered-ROM operand controls pass. All three
production bridges select their complete routines without positive fallback.
`Lufia2SaveUnpackPartyRecord` (`$85:C754`..`$85:C8CE`) restores the original
vitals, experience, equipment bytes, six-bit fields and packed high bits.
The six-bit empty value still becomes `$FF`; stat planes retain TDC seeding
and the status conversion retains its original arithmetic.
`Lufia2SavePackStatBits` (`$85:CBD2`..`$85:CBDB`) collects the original three
stat bit planes, including the shifted accumulator and scratch-byte flags.
Both contracts use native M1X0 with DP0 required by the production bridges.
Their shared ROM and production-ABI batches each pass 131,072 cases;
6,144 isolated invalid-entry guards and 177 altered-ROM controls pass.
Their comparisons retain CPU, full WRAM and ordered bus effects.
The five contracts have no child calls; their parent integrations are separate.
Shared full decomp-verify passes all1,041 independent jobs. Windows Release passes.


### Saved field state and record checksum

`Lufia2RestoreSavedFieldState` (`$8E:B993`..`$8E:BA0C`) and
`Lufia2CaptureFieldSaveState` (`$8E:BA0D`..`$8E:BA80`) reconstruct the five
original ascending block transfers and scalar field bytes. The save-buffer
ranges still overlap at `$3BAA`. Restore clears the map and layer high bytes;
capture stores only their low bytes. Entry-width stack saves and the original
8-bit accumulator high-byte residue remain observable. Both accept all four
native register-width combinations and preserve live DB and DP.

`Lufia2SaveRecordChecksum` (`$85:DE6C`..`$85:DE8D`) reconstructs the original
alternating-byte carry checksum. The source and length remain DP-backed.
TDC seeds both accumulator bytes; signed Y determines the loop, including the
one-byte iteration for length zero. The last PLB determines N/Z after PLP.
The complete contract accepts all four native widths in binary mode.

The shared original-ROM and production-ABI batches each pass 223,616 cases,
including 1,024 capture/restore compositions and 384 signed-length edges.
Original comparisons include every WRAM data read and write in order; ABI
comparisons check CPU, full WRAM, writes, MMIO and all three host-return modes.
All 6,144 isolated unsupported entries preserve state except the existing
return-frame open-bus reads. All 148 altered-ROM operand controls are detected.
The production bridges select each complete routine without positive fallback.
Their save/load parents still use original services for unreconstructed children.
Shared full decomp-verify passes all1,041 independent jobs. Windows Release passes.


### Menu selection restore and result-window HDMA

`Lufia2BattleConfigureResultHdma` reconstructs `$85:A972` through the RTL at
`$85:AA3C`. Four descending scroll runs, the `$85:AA3D` child frame, four
HDMA descriptors and the original request flags retain their write order.
The two runs beginning at `$FF40` remain separate. Zero descriptor controls
retain the original TDC value, including a nonzero direct page.

`Lufia2MenuRestoreSelectionState` reconstructs `$8E:E751` through the RTL
at `$8E:E7BF`. It compares the cached selection byte, clears cursor indices
1 through 5, optionally restores indices Y through 5 and three state words
through the original ROM pointer table, then records the selection word.
Cursor index 0 is not cleared. Names and offsets describe the
original state; no modern UI behavior is introduced here.

The shared proof checks 133,121 original-ROM cases and 133,121 actual
production-ABI cases, including 1,024 exact ripple-child compositions,
one forced child unwind and 30 redirected production returns. CPU state,
all WRAM, MMIO and write order agree. 6,528 unsupported production entries
fall back without changing guest state, apart from original return-frame
open-bus reads. 105 altered-ROM controls are detected. Runtime selection
requires native mode, binary arithmetic, M1X0, the expected PB, DP0 and a
safe return stack; selection restore additionally requires Y0..5.

All 18 original menu branch/index combinations are exercised. The integrated
checkpoint passes all 1,026 independent `decomp-verify` jobs and the Windows
Release build. Both complete contracts are selected by the production bindings.

The field-session follow-ups `$83:84E1` and `$8E:B28F` are still original
execution. Their NMI-dependent waits require an interrupt-aware continuation
contract before a native binding can be selected.

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
deliberate fault variants are detected. The record loader retains the childÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢s
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

`$82:89FA` interpolates a cursor between two sprite slots. `$82:8AD8`
and `$82:8AE9` correct the other axis; `$82:8AFA` requests the original
`$86:8B55` sprite frame every sixteen steps. These contracts are verified.
The cursor now admits live animation changes and the full byte-coordinate
range. It executes each original frame child through its pushed callback;
there is no animation prediction, frame omission or timing shortcut.

A changed child CPU mode, DB or DP transfers at the following original BRA
($82:8A9B/$82:8AD6). A nonreturning movement transfers after4096 steps at
$82:8A75/$82:8AA6. Original frame unwinds keep all pending guest frames.
The parent boundary does not claim that the frame scheduler is reconstructed.

The tracked ROM verifier builds four animated slots with the original table
and animation constructors, without a running game or saved capture. It
compares CPU, full WRAM, write order fingerprints and exact MMIO order.
A separate production-ABI proof also covers the observed DB83 movement,
child return rewrites/unwinds and guarded unsupported states. A synthetic
clock advances only inside the original frame-wait reads; this is not NMI
or cycle-level timing verification.

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

`$85:8F4A` shifts the 88-bit register at `$122F`ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â€šÂ¬Ã…â€œ`$1239` left by one through the carry. It returns the new low bit in `$122F`. Both routines are native: the first is a JSL entry that needs M8/X16 and hands back at the entry otherwise, the second is a JSR routine that needs M8.

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

## Original sound resource queue - 2026-10-07

Four complete queue/upload callers are verified and bound. Preserve the
original four-slot consumption, pending-song byte, driver-inhibit transition,
source pointer setup, widths, port order and pushed child frames.
Unknown upload/wait children remain explicit original continuations.
81,920 semantic and 81,920 production-ABI comparisons pass, including the
composed queue setup; 11 child unwinds, 4,992 guards and 19 ROM controls pass.
The shared regression batch with field animation passes 631 independent jobs,
strict C and Release. Counts at this sound checkpoint: 635 verified entries,
zero drafts, 382 WRAM locations. These are static selections, not runtime hits.

## Original field object animation - 2026-10-07

Ten complete animation helpers are verified and bound. The original region
snapshot, eight-call redraw sequence, alternate/initial rectangle transitions,
object-bit updates and sound suppression remain literal. Unknown children
retain their original JSR/JSL boundaries; no host animation changes were added.
180,224 semantic and 180,224 production-ABI comparisons pass, including native
snapshot/restore composition; 35 child unwinds, 31,488 guards and 36 ROM controls
pass. The shared regression batch with sound queues passes 631 independent jobs,
strict C and Release. 645 verified entries, zero drafts, 386 WRAM locations and
2,584 generated native calls. These are static counts, not measured runtime hits.
The eight-slot owner is a separate follow-up; its prior contract stays unchanged.

## Complete field animation slot owner - 2026-10-07

The existing 83:8682 entry now processes all eight animation slots and resumes
after both original child calls. Native action/control helpers compose with
region redraw, snapshots and sound dispatch. The active-mask order, phase
flags, status restoration and nested frames remain literal. 83:898E marks the
original row, including the zero-width wrap and one-byte index advance.
65,536 slot ROM comparisons and 65,559 production-ABI cases pass, including
23 nested unwinds, 2,688 guards and 21 altered-ROM controls. Row and existing
cell-clear checks pass 16,384 semantic and 16,384 ABI cases, 3,072 row guards
and 16 controls. The row is the only new cell entry; clear was already native.
The former prefix ABI target is superseded by the complete-owner suite; its
semantic prefix tests remain. Full 636 jobs, strict C and Release pass.
646 verified entries, zero drafts, 389 WRAM fields and 2,588 generated calls.
These are static counts, not measured interpreter usage or gameplay coverage.

## Original sound payload and reply wait - 2026-10-07

80:9911 loads the original payload length, advances its banked source twice,
sends mode01 and preserves both child-call frames. 80:9A0A implements all
four reply phases, original port order and PHP/SEP/PLP restoration. A stuck
phase preserves the exact original continuation after 65,536 native polls.
The payload supports both M widths with X16; the wait preserves either width.
24,576 payload ROM cases, including 8,192 native reply compositions, pass;
24,578 actual ABI cases, two unwinds, 1,536 guards and 12 ROM controls pass.
The wait passes 8,192 delayed-response cases and 16 stuck-phase continuations,
8,208 actual ABI cases, 1,152 guards and 11 controls. External APU responses
are controlled by the fixture; this does not establish SPC700 hardware timing.
Shared full645, strict C and Release pass. Checkpoint:648 verified entries,
zero drafts,389 WRAM fields and2,596 generated calls. Static counts only.

## Original sound transfer callers - 2026-10-07

80:9945,80:98A5 and80:97E5 implement payload pairs, resource headers and
queued chunks. Original speculative reads, source-bank rollover, sequence
wrap, port order, shared length/chunk word and child frames remain literal.
24,582 ROM states, four exact child unwinds and6,144 compositions with
native reply waits pass. Twelve stuck-phase cases preserve the original
continuations after65,536 polls; no timeout is treated as success.
Production ABI:30,742 native cases, zero positive fallbacks,45 redirects.
4,992 guards and25 altered-ROM controls pass. Full ordered streaming traces
cover long payloads beyond the bounded MMIO summary. APU responses are
controlled; this does not establish SPC700 hardware timing.
Shared full645, strict C and Release pass.651 verified, zero drafts,
392 WRAM fields and2,608 generated calls. Static counts, not runtime hits.

## Original scene bootstrap - 2026-10-07

Nine complete entries cover field OAM reset, object animation reset,
map record selection, text reset and initial/resume/transition/map scripts.
All saved widths, DB-relative writes, original frames, temporary record-base
replacement and literal TDC contents remain exact. Descriptor searches and
the text engine remain explicit children; their internals are not redefined.
86,016 original-ROM cases include12,288 composed start-record calls.
86,023 production ABI cases, seven child unwinds,75 redirect probes,
11,904 unsupported-state guards and24 altered-ROM controls pass.
Shared full651, strict C and Release pass. Checkpoint:660 verified entries,
zero drafts,397 WRAM fields and2,644 generated calls. Static counts only.

## Original actor sprite bootstrap - 2026-10-07

Eight complete entries cover transient reset, fine/cell position, special
sprite detection, object reset, descriptor and animation/motion tables,
height offset and saved-register sprite refresh. Original rounding carry,
TDC payloads, X-width accesses, table bounds and child frames remain exact.
73,728 ROM cases include8,192 compositions with native descriptor/tables.
73,731 production ABI cases, three child unwinds,30 redirect probes,
9,984 unsupported-state guards and24 altered-ROM controls pass.
Shared full651, strict C and Release pass.668 verified, zero drafts,
402 WRAM fields and2,676 generated calls. No measured runtime-hit claim.

## Original actor scene records and uploads - 2026-10-07

Seven complete entries cover forty-slot reset, header attributes/cell/pair,
actor placement and actor/object sprite upload records. Original repeated
clears, DB-relative versus bank-zero writes, widths, flags and bus order stay
literal. Multiplier ports use the original accesses; hardware timing is not
inferred from controlled fixture values.65,536 ROM cases include8,192 native
child compositions;65,541 production ABI cases, five child unwinds,
60 redirects,10,752 guards and21 altered-ROM controls pass.
Shared full657, W4WX and Release pass. Checkpoint675 verified, zero drafts,
414 WRAM locations,2,704 generated calls. Static counts only.

## Original scene actor state rebuild - 2026-10-07

83:A82E..A97D rebuilds all forty actors, descriptors, fine coordinates,
random delays, party offsets, scene attributes and primary scripts. All
thirteen original child frames, conditional paths and register widths stay
literal. DB83/DP0 binary owners support either M/X width; other contexts
retain the original interpreter.12,288 ROM cases include4,096 compositions;
12,301 production ABI cases,13 exact child unwinds,15 redirects,1,920 guards
and24 altered-ROM controls pass.83:A76D was already verified and is not
counted as new. Shared full657, W4WX and Release pass.676 verified,
zero drafts,414 WRAM locations,2,708 generated calls.
These are static selections; no current runtime-hit claim is made.

## Original field scene reset and display callers - 2026-10-07

Seven complete entries restore saved scene actors, reset options, mark maps,
resume music and upload requested tilemaps. Text clear and event flag mask
share their original cores with existing embedded callers. Original TDC,
repeated writes, widths, flags, DB/DP and child frames remain literal.
71,680 ROM cases include14,336 native child compositions;71,691 production
ABI cases,11 child unwinds,75 redirects,11,136 guards and21 error controls
pass. Shared full663, W4WX and Release pass. Checkpoint683 verified,
zero drafts,430 WRAM locations and2,736 generated calls. Static counts only.

## Original field party actor rebuild - 2026-10-07

Five complete entries rebuild the five party actors, release six prior sprite
slots, calculate actor offsets, clear one or two map occupancy cells and
frame sprite uploads. Original ASL carry, TDC bytes, conditional single/hidden
party paths, child frames, widths and bus ordering remain literal.
51,200 ROM cases include10,240 native child compositions;51,211 production
ABI cases,11 child unwinds,60 redirects,7,680 guards and15 error controls
pass. Remaining original presentation children are explicit services.
Shared full663, W4WX and Release pass.688 verified, zero drafts,
430 WRAM locations and2,756 generated calls. Static counts only.

## Original battle display queue callers - 2026-10-07

Thirty complete upload callers share the existing display queue helper.
Source, VRAM destination and length retain original order and child frames.
Unknown block roles retain neutral names.307,200 ROM states include61,440
native queue compositions;307,230 actual ABI cases,30 child unwinds,
450 redirects,57,600 guards and90 error controls pass.1,920 full-queue
BRK boundaries remain original. Shared full669,W4WX and Release PASS.
718 verified, zero drafts,430 fields,2,876 generated calls. Static counts only.

## Original field fades and display callers - 2026-10-07

Four complete entries preserve brightness guards, fade controls, DMA register
order and layer redraws. Frame waits remain explicit original child services.
40,960 ROM states include8,192 native compositions of inactive colour effects
and empty redraw layers;40,967 actual ABI cases,7 child unwinds,60 redirects,
6,144 guards and12 error controls pass. No timing optimization or unknown
child reconstruction is claimed. Shared full669,W4WX and Release PASS.
722 verified, zero drafts,430 fields,2,892 generated calls. Static counts only.

## Original battle action messages - 2026-10-07

Five complete entries connect item, spell and IP names, status strings,
seven tile rows, action windows and frame confirmation. The existing 9DD4
upload is reused and tested as a sixth regression entry. Original cleared
tile rows ignore the loaded style values; this behavior remains unchanged.
61,440 ROM states include 12,288 native child compositions; 61,472 actual
ABI cases, 32 child unwinds, 45 redirects, 8,064 unsupported-entry guards
and 26 error controls pass. C4F1 resource parsing, text drawing and frame
services remain explicit original children. Multiplication port order is
compared; hardware timing is not inferred. W4WX, Release and full 672 pass.
The 9906 battle contract requires DP0 and M1X0. The row child supports M0
and M1 with X16; X8 misdecodes original immediate operands and is rejected.
IP and status entry widths are restored from the original saved status.
Shared phase buffers remain shared; the status header is two bytes.
727 verified, zero drafts, 435 fields, 2,912 generated calls. Static counts.

## Original battle message resource parser - 2026-10-07

The complete C4F1 parser preserves literal bytes, short and long dictionaries,
the action-ID remap, multiplication register order and original ADC carry.
15,520 original-ROM and actual ABI cases cover 970 bounded original IDs,
all six command classes and eight parser branches. 1,536 entry guards and
18 public-library error controls pass. Whole action-message composition adds
61,440 ROM cases, 12,288 native compositions and 32 exact child unwinds.
Saved widths, registers, flags, stack residue and ordered writes remain original.
Out-of-ROM and nonterminating synthetic resource IDs are not production fixes.
Shared full679, W4WX and Release PASS. Hardware timing is not inferred.
728 verified, zero drafts, 436 fields, 2,916 generated calls. Static counts.

## Original world object foundations - 2026-10-07

Eight complete entries pack four sprite high-bit fields, initialize slot and
object records, and select or index a 29-byte record. The existing bit-packing
core is reused. Initializers preserve the original DB:D8B6+X source and clear
order. Record indexing retains DB-specific multiplication ports and DP writes.
65,536 original-ROM and actual ABI cases, 13,824 guards and 28 public-library
error controls pass. X8 and nonzero DP remain supported where the ROM does.
The 12A5 slot phase aliases the existing 1291 array; no overlapping WRAM field
is invented. Shared full679, W4WX and Release PASS. No hardware timing claim.
736 verified, zero drafts, 437 fields, 2,948 generated calls. Static counts.

## Original scene text foundations - 2026-10-07

Six complete entries expand scene strings, draw glyphs and window rows,
clear upload rows, format 24-bit numbers and append their original digits.
The existing glyph, row and number cores are reused, without duplicate bodies.
49,152 ROM and actual ABI states, 11,136 guards and 23 public-library error
controls pass. Number seeds include the complete 24-bit range and caller DB.
The literal 81-pass clear, dictionary references and six-byte hexadecimal
format remain original. Number PLY flags and window word-DEC high-byte-first
write order are now exact; whole caller composition proves the latter.
Entry guards retain the proven M/X/DP contracts. Shared full685, W4WX and
Release PASS. No hardware timing claim. 742 verified, zero drafts, 440 fields,
2,972 generated calls. Static counts, not a runtime hit-rate measurement.

## Original scene label callers - 2026-10-07

Two complete callers update field or battle labels, compare decoded strings,
centre their window, render literals and dictionary glyphs, and request upload.
20,480 original-ROM states include 4,096 native compositions across all ten
child targets. Fifteen original sites have exact unwind tests; actual ABI
20,495 cases, 30 redirects, 3,840 guards and 20 public-library error controls
pass. Original C825 pushes P and finally pulls DB; this quirk is preserved.
Shared previous-label buffers and the existing D0C4/D0C5 reference bytes are
catalogued without overlapping fields. Shared full685, W4WX and Release PASS.
744 verified, zero drafts, 444 fields, 2,980 generated calls. Static counts.

## Original menu graphics setup - 2026-10-07

Six complete entries reset display requests, initialize menu state and stage
the original primary, auxiliary, large and medium graphics resources.
50,688 ROM states and 50,697 actual ABI cases, nine exact child unwinds,
60 redirects, 11,136 guards and 24 public-library error controls pass.
1,536 compositions use the original resource decoder and video queue;
frame services remain explicit original children. Request and MMIO order,
DB/DP, widths and the 999 equipment-cursor sentinel remain original.
Shared full700, W4WX and Release PASS. 750 verified, zero drafts, 450
documented fields and 3,004 generated calls. Counts are not runtime hit rates.

## Original actor record index widths - 2026-10-07

83:AB4F uses no index-register instruction. Its original 8E:BC99 caller
passes 16-bit indices; the whole wrapper and binding now accept either
index width. Binary, M1, DP0, PB83 and existing stack guards remain.
All 256 slot values, both widths and caller flags pass 65,536 ROM/ABI
cases, 2,304 unsupported contexts and nine public-library error controls.
The original shift carry in the triple-slot offset remains observable.
The other two sprite-resource entries retain their 8-bit-index guards.
The earlier fixture replaces only the obsolete AB4F X16 rejection with
positive coverage; every other rejection remains. Shared full700, W4WX
and Release PASS. Function/field/binding counts remain 750/450/3,004.

## Original menu actor callers - 2026-10-07

Two complete entries select the menu actor and initialize eleven auxiliary
sprite slots. The selection checks slots7..1 and falls back to unchecked
slot0. Original array bases share already named menu-phase cells; no
disjoint array extent is invented. 20,480 ROM states, 20,482 actual ABI
cases, two exact unwinds, 30 redirects, 3,456 guards and eleven public-library
error controls pass. 4,096 compositions execute both original native
children. Unrelated actor-state bits are independently varied. Shared
full700, W4WX and Release PASS. 752 verified, zero drafts, 452 fields,
3,012 generated calls. No ROM timing or current runtime hit-rate claim.

## Original field and main menu callers - 2026-10-07

Four complete entries connect field-menu entry, main screen construction,
field-selection restoration and the original party actor update scripts.
The field entry keeps all three original outcomes, including the FF map
destination, special F0/F1 maps and live accumulator comparisons after
the kind3 transition. The refresh entry requires M1X0: X8 decodes the
original three-byte LDY differently. Main screen setup accepts either
entry width and retains the original loading and input-loop children.
Actor updates accept starting slots0..4, preserve opcode12 script bytes,
the primary script/timer/state fields and original occupancy clearing.
Unknown images, palettes, input and frame services remain explicit children.

Callers:24,960 ROM states,24,985 actual ABI cases,25 exact child unwinds,
45 redirects,4,608 unsupported contexts,36 public-library error controls.
Updates:9,216 ROM states,9,218 actual ABI cases,two exact child unwinds,
15 redirects,2,304 unsupported contexts,27 public-library error controls.
Caller compositions384 and actor offset/occupancy compositions1,024 pass.
CPU,WRAM,child stack frames and bus order remain original. Shared full700,
W4WX and Release PASS. 756 verified,zero drafts,455 fields,3,028 generated
calls. Counts describe static selection,not measured interpreter load.

## Original main menu dispatcher - 2026-10-07

Four complete entries implement the main input dispatcher, spell-list
application, alternate selection and main display refresh. Normal selection,
rejection,cancel and auxiliary display toggling remain original. The057C-gated
debug item/spell,buffer-clear,temporary-price and scene shortcuts remain
literal. A4BE still owns its original inline-table child; unknown children
remain explicit services. Embedded A4C1..A4D0,A54D..A556,A604..A62C tables
are data,not semantic instructions. Shared fields retain canonical phase
names; buffer F080 overlaps existing message storage.

33,792 ROM states include1,024 native compositions of the recovered sibling
helpers and existing layer clearing. All45 original child sites retain their
frames and exact unwind boundaries. 33,837 actual ABI cases,60 redirects,
7,680 unsupported contexts and79 public-library error controls pass. Button
and selection bits vary independently,including unrelated mask bits.
CPU,WRAM,MMIO order and original live-index word writes remain unchanged.
Shared full703,W4WX and Release PASS. 760 verified,zero drafts,461 fields,
3,044 generated calls. No measured runtime-hit or whole-ROM coverage claim.

## Original menu display callers - 2026-10-07

Six complete entries place party portraits,initialize status animations,
build the main windows,present the display and configure alternate cursors.
Original byte-count loops retain256 iterations for a zero count. Status
bit4 precedes bit1;HP<=maxHP/8 selects animation2. Sprite tables remain
original ROM resources. Window descriptors and display requests stay literal.
All17 child frames retain exact unwind boundaries; unknown services remain
explicit. Native SpriteSetTable and SpriteSetAnimation compositions pass,
including original X8 portrait-table calls. Their wider binding is not claimed.

26,112 ROM states include1,536 composition-context states. 26,129 actual ABI cases,
90 redirects,11,520 unsupported states and52 public-library error controls
pass. CPU,WRAM,MMIO and bus-write order remain unchanged. Shared full706,
W4WX and Release PASS. 766 verified,zero drafts,461 documented fields,
3,068 generated calls. No measured runtime-hit or whole-ROM coverage claim.

## Original cursor and menu presentation callers - 2026-10-07

Nine complete entries load original seven-byte cursor grids,select sprite
styles,validate party selection,construct and move cursor pairs,clear sprite
masks,wait for screen fades and prepare display text. Existing native cursor
placement and movement remain their original children. Unchecked style/table
indices,byte arithmetic,status restoration and child frames remain literal.
Origin low/high bytes and cell dimensions are indexed bases; no larger
disjoint arrays are inferred.0596/0597 retain the existing scroll-field allocation and neutral phase alias.

Cursor proof:26,112 ROM states include1,536 composition-context states,
26,120 actual ABI cases,eight child unwinds,75 redirects,11,520 unsupported
states and55 public-library error controls. Composition uses the recovered
grid/style/pair routines and existing native placement and sprite animation.
Presentation proof:13,056 ROM states include768 composition-context states;
512 execute original80:86C1 fade updates under explicit frame services.
13,058 actual ABI cases,two child unwinds,30 redirects,5,760 unsupported
states and32 public-library error controls pass. Remaining frame internals
stay original services. No frame skip,host redraw or timing patch.
CPU,WRAM,MMIO and bus-write order remain unchanged. Shared full712,
W4WX and Release PASS.775 verified,zero drafts,467 documented fields,
3,104 generated calls. No measured runtime-hit or whole-ROM coverage claim.

## Original menu list controls - 2026-10-07

Five complete entries construct mode-dependent list windows,initialize scroll
geometry,replace cursors and run the nested action/list-selection loops.
First/second selection,help,description,page,sort,cancel and input comparisons
retain original flag,width,stack and memory behavior. Proven sprite-slot
arrays keep their original indices. Existing auxiliary-sprite initialization
is reused. Window/cursor setup composes with native clear/text and list
children; remaining unknown services retain exact original child contracts.

42,240 original-ROM states include1,280 composition-context states. All59
child unwinds,42,299 actual ABI cases,75 redirected returns,9,600 unsupported
states and74 public-library error controls pass. Selection/mode/overflow
word writes are documented without truncating their high bytes. Inventory
words retain96 entries,packed item/quantity bits and original unchecked access.
153F keeps its existing battle allocation and a separate menu-phase alias.

86:8CDA now accepts its original M1X8 callers as well as M1X16. 65,536 X8
register pairs and65,536 X16 cases pass both ROM and actual ABI comparisons.
1,152 remaining guard states preserve original return-frame reads/open bus;
nine public-library controls pass. Semantic SpriteSetTable code is unchanged.
No widened contract is inferred for other sprite children.

Shared full718,W4WX and Release PASS.780 verified,zero drafts,471
documented fields,3,124 generated calls. CPU,WRAM,MMIO and bus-write order
remain literal. No timing patch or measured runtime-hit coverage claim.

## Shared original DMA entries - 2026-10-07

80:882E and80:884F have complete M1X16 RTS contracts and native bindings.
The 2KiB 7E:X tilemap upload and listed VRAM/CGRAM transfers share their
complete CPU adapters with existing NMI callers. DMA masks,address writes,
register high bytes,flags,stack and MMIO order retain original behavior.
Portable leaves retain decimal flags; production binary-mode guards remain.
No upload acceleration or timing patch is introduced.

16,384 original-ROM states and16,384 actual ABI cases pass;96 additional
original/actual ABI cases check reverse-DMA return-frame changes in all
three host-return contexts.3,072 unsupported CPU states and19 public-library
error controls pass.1,024 original NMI compositions retain the existing
DP0,hardware DB and stack contracts; no wider parent contract is claimed.
Shared full722,W4WX and Release PASS.782 verified,zero drafts,471 fields,
3,132 generated calls. These counts do not measure runtime interpreter load.

## Original menu list scrolling - 2026-10-07

Six complete entries:82:FBE5,8CE9,8CC1,8C43,8C85 and8C9C.
Selection offsets,page changes,step division,thumb positioning,range setup
and sprite initialization have verified native bindings. Carry-selected
direction,one/two-column parity,word wrap,endpoint animations and original
child frames remain exact. FBE5 reads the full14B3word,including its adjacent
index byte. Existing field metadata now records that word access.

26,112 original-ROM caller/composed states and26,118 actual ABI cases pass,
including six child unwinds and60 redirect probes.11,136 unsupported states
and82 public-library altered-ROM controls pass.1,536 additional full native
chains include the existing multiply,divide and animation children.
CPU,WRAM,MMIO and observed bus order remain literal; no timing patch.
Shared full725,W4WX and Release PASS.788 verified,zero drafts,471 fields,
3,156 generated calls. Counts do not measure runtime interpreter load.

## Original menu list presentation and motion - 2026-10-07

Seven complete entries:86:8D47 and82:9C52,AC84,ADA3,AE53,AD20,ADB4.
List parameters,page content,cursor setup,marker movement and both animated
scroll directions have verified native bindings. Shared gameplay phases use
structured C. Parameter stores,low-byte page updates,marker visibility,three
frame calls,word thumb movement and all original child frames remain exact.
Existing display-request and battle-phase allocations are reused; only the
marker row and five parameter words receive new non-overlapping metadata.

30,464 original-ROM caller/composed states and30,491 actual ABI cases pass,
including27 child unwinds and60 redirect probes.13,440 unsupported states,
768 additional original/actual stack-alias leaf cases,and160 public-library
altered-ROM controls pass. CPU,WRAM,MMIO and observed bus order are literal.
Frame wait services retain their original interpreter owner;no timing patch.
Shared full728,W4WX and Release PASS.795 verified,zero drafts,473 fields,
3,184 generated calls. Counts do not measure runtime interpreter load.

## Shared original menu glyph entries - 2026-10-07

Four complete entries80:8DB3,8DF9,8E0F,8D5D now share the existing glyph,
row advance,width reset and palette cores. Literal raw-tile translation,
CC quirk,stack aliases,bank wrapping and flag effects remain unchanged.
32,768 original-ROM states and32,768 actual ABI cases,9,216 unsupported
states and32 public-library altered-ROM controls pass. Production bindings
require binary M1X16 with return frames in bank-zero WRAM;other entries
retain LLE. The portable proof additionally covers arbitrary stack accesses.
80:88DA and88C8 remain unbound:their nested child redirects need a separate
complete child-frame contract. Existing parent contracts are unchanged.
Shared full731,W4WX and Release PASS.799 verified,zero drafts,478 fields,
3,200 generated calls. Counts do not measure runtime interpreter load.

## Original field frame and recovery services - 2026-10-07

Eleven new complete entries and the completed83:8103 dispatcher connect
frame services,HP/MP record restoration,recovery graphics,object lookup,
palette preparation and both packed channel scales. Existing8103 prefix
API remains compatible;its runtime binding now selects the complete caller.
The original900C wait child retains its live frame and time owner.
82A0 searches31slots while834A configures32;the discrepancy is literal.
Recovery object slots reuse menu/battle storage in their original phase.

175,104 original-ROM states and175,134 actual ABI cases pass;30 exact
child unwinds,135 redirects,21,888 unsupported states and158 public-library
altered-ROM controls pass. Both scales cover all65,536 accumulator words;
lookup covers each first-match slot,ignored31 and absent keys. No graphics,
timing or original bug is repaired inside this semantic package.
Shared full734,W4WX and Release PASS.810 verified,zero drafts,488 fields,
3,244 generated calls. Counts do not measure runtime interpreter load.

## Original menu character callers - 2026-10-07

Complete80:88DA and88C8 entries retain four live glyph and row child frames.
Raw control preserves its tested byte across XBA;character-count underflow,
two-row advances and original overwritten child returns remain literal.
98,304 original-ROM states and98,308 actual ABI cases pass;four forced
unwinds,30 redirects,4,608 unsupported states and14 public-library
altered-ROM controls pass. Two naturally overwritten child returns occur
in the main composition matrix. Another38 small/MMIO-stack compositions
include seven natural redirects;production MMIO-stack entries retain LLE.
Shared full738,W4WX and Release PASS.812 verified,zero drafts,488 fields,
3,252 generated calls. Counts do not measure runtime interpreter load.

## Original turn support entries - 2026-10-07

49,152 original-ROM cases, 49,152 actual ABI cases, 6,144 guards
and 55 public-library ROM error controls pass. Child probes include
0 forced unwinds, 0 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 750-job verification and Windows Release build
with the complete 15-entry integration batch. Its final combined stage has
827 verified entries, zero drafts, 503 fields and 3,312 generated calls.
Counts do not describe measured runtime interpreter load.

## Original field cell address entries - 2026-10-07

131,072 original-ROM cases, 131,076 actual ABI cases, 8,832 guards
and 111 public-library ROM error controls pass. Child probes include
4 forced unwinds, 15 redirects and 684 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 750-job verification and Windows Release build
with the complete 15-entry integration batch. Its final combined stage has
827 verified entries, zero drafts, 503 fields and 3,312 generated calls.
Counts do not describe measured runtime interpreter load.

## Original capsule record entries - 2026-10-07

81,920 original-ROM cases, 81,921 actual ABI cases, 11,520 guards
and 72 public-library ROM error controls pass. Child probes include
1 forced unwinds, 15 redirects and 1,602 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 750-job verification and Windows Release build
with the complete 15-entry integration batch. Its final combined stage has
827 verified entries, zero drafts, 503 fields and 3,312 generated calls.
Counts do not describe measured runtime interpreter load.

## Original battle priority/mirror entries - 2026-10-07

98,304 original-ROM cases, 98,308 actual ABI cases, 6,912 guards
and 99 public-library ROM error controls pass. Child probes include
4 forced unwinds, 30 redirects and 1,112 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 750-job verification and Windows Release build
with the complete 15-entry integration batch. Its final combined stage has
827 verified entries, zero drafts, 503 fields and 3,312 generated calls.
Counts do not describe measured runtime interpreter load.

## Original field metatile entries - 2026-10-07

65,537 original-ROM cases, 65,539 actual ABI cases, 4,608 guards
and 98 public-library ROM error controls pass. Child probes include
2 forced unwinds, 30 redirects and 1,200 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 762-job verification and Windows Release build
with the complete 10-entry integration batch. Its final combined stage has
837 verified entries, zero drafts, 509 fields and 3,352 generated calls.
Counts do not describe measured runtime interpreter load.

## Original field event reader entries - 2026-10-07

229,376 original-ROM cases, 229,378 actual ABI cases, 6,528 guards
and 30 public-library ROM error controls pass. Child probes include
2 forced unwinds, 15 redirects and 1,368 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 762-job verification and Windows Release build
with the complete 10-entry integration batch. Its final combined stage has
837 verified entries, zero drafts, 509 fields and 3,352 generated calls.
Counts do not describe measured runtime interpreter load.

## Original field event value entries - 2026-10-07

68,096 original-ROM cases, 68,097 actual ABI cases, 4,608 guards
and 39 public-library ROM error controls pass. Child probes include
1 forced unwinds, 15 redirects and 673 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 762-job verification and Windows Release build
with the complete 10-entry integration batch. Its final combined stage has
837 verified entries, zero drafts, 509 fields and 3,352 generated calls.
Counts do not describe measured runtime interpreter load.

## Original field event coordinate entries - 2026-10-07

98,304 original-ROM cases, 98,310 actual ABI cases, 6,912 guards
and 42 public-library ROM error controls pass. Child probes include
6 forced unwinds, 45 redirects and 4,567 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 762-job verification and Windows Release build
with the complete 10-entry integration batch. Its final combined stage has
837 verified entries, zero drafts, 509 fields and 3,352 generated calls.
Counts do not describe measured runtime interpreter load.

## Original field sections entries - 2026-10-07

172,034 original-ROM cases, 172,034 actual ABI cases, 9,600 guards
and 121 public-library ROM error controls pass. Child probes include
0 forced unwinds, 0 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 771-job verification and Windows Release build
with the complete 10-entry integration batch. Its final combined stage has
847 verified entries, zero drafts, 514 fields and 3,392 generated calls.
Counts do not describe measured runtime interpreter load.


## Original actor battle entries - 2026-10-07

98,304 original-ROM cases, 98,306 actual ABI cases, 6,144 guards
and 47 public-library ROM error controls pass. Child probes include
2 forced unwinds, 30 redirects and 1,352 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 771-job verification and Windows Release build
with the complete 10-entry integration batch. Its final combined stage has
847 verified entries, zero drafts, 514 fields and 3,392 generated calls.
Counts do not describe measured runtime interpreter load.


The relative script caller restores the original word-INC write order.

## Original field sprite selector entries - 2026-10-07

60,240 original-ROM cases, 60,251 actual ABI cases, 3,456 guards
and 55 public-library ROM error controls pass. Child probes include
11 forced unwinds, 15 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 771-job verification and Windows Release build
with the complete 10-entry integration batch. Its final combined stage has
847 verified entries, zero drafts, 514 fields and 3,392 generated calls.
Counts do not describe measured runtime interpreter load.


Sprite selection separately checks 6,832 unchanged entry fallbacks.
Its supported saved-register frame requires 000C <= S <= 1FFC.

## Original capsule experience entries - 2026-10-07

65,536 original-ROM cases, 65,537 actual ABI cases, 4,608 guards
and 125 public-library ROM error controls pass. Child probes include
1 forced unwinds, 15 redirects and 534 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 786-job verification and Windows Release build
with the complete 14-entry integration batch. Its final combined stage has
861 verified entries, zero drafts, 519 fields and 3,448 generated calls.
Counts do not describe measured runtime interpreter load.


## Original pointer coordinates entries - 2026-10-07

163,840 original-ROM cases, 163,840 actual ABI cases, 3,456 guards
and 18 public-library ROM error controls pass. Child probes include
0 forced unwinds, 0 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 786-job verification and Windows Release build
with the complete 14-entry integration batch. Its final combined stage has
861 verified entries, zero drafts, 519 fields and 3,448 generated calls.
Counts do not describe measured runtime interpreter load.


## Original world resources entries - 2026-10-07

16,384 original-ROM cases, 16,384 actual ABI cases, 2,304 guards
and 29 public-library ROM error controls pass. Child probes include
0 forced unwinds, 0 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 786-job verification and Windows Release build
with the complete 14-entry integration batch. Its final combined stage has
861 verified entries, zero drafts, 519 fields and 3,448 generated calls.
Counts do not describe measured runtime interpreter load.


## Original capsule stat callers entries - 2026-10-07

98,304 original-ROM cases, 98,307 actual ABI cases, 4,992 guards
and 25 public-library ROM error controls pass. Child probes include
3 forced unwinds, 15 redirects and 684 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 786-job verification and Windows Release build
with the complete 14-entry integration batch. Its final combined stage has
861 verified entries, zero drafts, 519 fields and 3,448 generated calls.
Counts do not describe measured runtime interpreter load.


## Original field talk entries - 2026-10-07

262,144 original-ROM cases, 262,156 actual ABI cases, 13,440 guards
and 77 public-library ROM error controls pass. Child probes include
12 forced unwinds, 90 redirects and 7,645 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 786-job verification and Windows Release build
with the complete 14-entry integration batch. Its final combined stage has
861 verified entries, zero drafts, 519 fields and 3,448 generated calls.
Counts do not describe measured runtime interpreter load.


The dynamic talk target covers all 65,536 PB-relative table indices.
Native compositions preserve 7,645 original abnormal child returns.

## Original field contact entries - 2026-10-07

245,760 original-ROM cases, 245,764 actual ABI cases, 1,920 guards
and 143 public-library ROM error controls pass. Child probes include
4 forced unwinds, 15 redirects and 852 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 795-job verification and Windows Release build
with the complete 8-entry integration batch. Its final combined stage has
869 verified entries, zero drafts, 525 fields and 3,480 generated calls.
Counts do not describe measured runtime interpreter load.


## Original contact edge entries - 2026-10-07

655,360 original-ROM cases, 655,364 actual ABI cases, 12,288 guards
and 23 public-library ROM error controls pass. Child probes include
4 forced unwinds, 75 redirects and 13,660 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 795-job verification and Windows Release build
with the complete 8-entry integration batch. Its final combined stage has
869 verified entries, zero drafts, 525 fields and 3,480 generated calls.
Counts do not describe measured runtime interpreter load.


Contact edge dispatch is restricted to the four original direction indices.
Other table indices retain original interpretation.

## Original capsule stat growth entries - 2026-10-07

196,736 original-ROM cases, 196,744 actual ABI cases, 5,376 guards
and 106 public-library ROM error controls pass. Child probes include
8 forced unwinds, 15 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 795-job verification and Windows Release build
with the complete 8-entry integration batch. Its final combined stage has
869 verified entries, zero drafts, 525 fields and 3,480 generated calls.
Counts do not describe measured runtime interpreter load.


The growth leaf requires DP0 and0200<=S<=1FFC. Zero-level wrap remains literal.

## Original world scene entries - 2026-10-08

409,600 original-ROM cases, 409,621 actual ABI cases, 16,128 guards
and 137 public-library ROM error controls pass. Child probes include
21 forced unwinds, 60 redirects and 4,140 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 807-job verification and Windows Release build
with the complete 16-entry integration batch. Its final combined stage has
885 verified entries, zero drafts, 527 fields and 3,544 generated calls.
Counts do not describe measured runtime interpreter load.


Tile-plane upload requires DP0 and1F00<=S<=1FFC. Original64-row loops remain exact.

## Original effect scroll entries - 2026-10-08

589,824 original-ROM cases, 589,824 actual ABI cases, 7,680 guards
and 84 public-library ROM error controls pass. Child probes include
0 forced unwinds, 0 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 807-job verification and Windows Release build
with the complete 16-entry integration batch. Its final combined stage has
885 verified entries, zero drafts, 527 fields and 3,544 generated calls.
Counts do not describe measured runtime interpreter load.


The second signed operand reads a word and advances only one byte.

## Original world stream entries - 2026-10-08

491,520 original-ROM cases, 491,526 actual ABI cases, 9,984 guards
and 119 public-library ROM error controls pass. Child probes include
6 forced unwinds, 30 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 807-job verification and Windows Release build
with the complete 16-entry integration batch. Its final combined stage has
885 verified entries, zero drafts, 527 fields and 3,544 generated calls.
Counts do not describe measured runtime interpreter load.


Stream parents require DP0 and1F00<=S<=1FFC. Column savedY->X remains literal.

## Original field row buffer entries - 2026-10-08

163,840 original-ROM cases, 163,841 actual ABI cases, 2,304 guards
and 35 public-library ROM error controls pass. Child probes include
1 forced unwinds, 15 redirects and 684 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 807-job verification and Windows Release build
with the complete 16-entry integration batch. Its final combined stage has
885 verified entries, zero drafts, 527 fields and 3,544 generated calls.
Counts do not describe measured runtime interpreter load.

## Original world perspective entries - 2026-10-08

294,912 original-ROM cases, 294,914 actual ABI cases, 5,376 guards
and 111 public-library ROM error controls pass. Child probes include
2 forced unwinds, 30 redirects and 1,976 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 828-job verification and Windows Release build
with the complete 16-entry integration batch. Its final combined stage has
901 verified entries, zero drafts, 533 fields and 3,608 generated calls.
Counts do not describe measured runtime interpreter load.


## Original world math resources entries - 2026-10-08

327,680 original-ROM cases, 327,680 actual ABI cases, 8,832 guards
and 112 public-library ROM error controls pass. Child probes include
0 forced unwinds, 0 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 828-job verification and Windows Release build
with the complete 16-entry integration batch. Its final combined stage has
901 verified entries, zero drafts, 533 fields and 3,608 generated calls.
Counts do not describe measured runtime interpreter load.


Skyline HDMA requires DP0 and1F00<=S<=1FFC; byte products remain original.

## Original world depth entries - 2026-10-08

196,608 original-ROM cases, 196,612 actual ABI cases, 4,608 guards
and 55 public-library ROM error controls pass. Child probes include
4 forced unwinds, 30 redirects and 952 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 828-job verification and Windows Release build
with the complete 16-entry integration batch. Its final combined stage has
901 verified entries, zero drafts, 533 fields and 3,608 generated calls.
Counts do not describe measured runtime interpreter load.


## Original system flag read entries - 2026-10-08

294,912 original-ROM cases, 294,914 actual ABI cases, 6,144 guards
and 13 public-library ROM error controls pass. Child probes include
2 forced unwinds, 30 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 828-job verification and Windows Release build
with the complete 16-entry integration batch. Its final combined stage has
901 verified entries, zero drafts, 533 fields and 3,608 generated calls.
Counts do not describe measured runtime interpreter load.


Flag read wrappers retain original nested frames and supported low-stack guards.

## Original world flagged blocks entries - 2026-10-08

32,768 original-ROM cases, 32,771 actual ABI cases, 3,072 guards
and 26 public-library ROM error controls pass. Child probes include
3 forced unwinds, 15 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 828-job verification and Windows Release build
with the complete 16-entry integration batch. Its final combined stage has
901 verified entries, zero drafts, 533 fields and 3,608 generated calls.
Counts do not describe measured runtime interpreter load.


Block copies retain all five original event gates and word writes.

## Original world palette step entries - 2026-10-08

180,224 original-ROM cases, 180,225 actual ABI cases, 4,992 guards
and 49 public-library ROM error controls pass. Child probes include
1 forced unwinds, 15 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 828-job verification and Windows Release build
with the complete 16-entry integration batch. Its final combined stage has
901 verified entries, zero drafts, 533 fields and 3,608 generated calls.
Counts do not describe measured runtime interpreter load.


The112-color step preserves original channel arithmetic and queue layout.

## Original world map load entries - 2026-10-08

65,728 original-ROM cases, 65,735 actual ABI cases, 3,456 guards
and 26 public-library ROM error controls pass. Child probes include
7 forced unwinds, 15 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 828-job verification and Windows Release build
with the complete 16-entry integration batch. Its final combined stage has
901 verified entries, zero drafts, 533 fields and 3,608 generated calls.
Counts do not describe measured runtime interpreter load.


All16-bit resource offsets,real compressed streams and nested unwinds compared.

## Original battle text tiles entries - 2026-10-08

81,920 original-ROM cases, 81,920 actual ABI cases, 1,536 guards
and 99 public-library ROM error controls pass. Child probes include
0 forced unwinds, 0 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 846-job verification and Windows Release build
with the complete 8-entry integration batch. Its final combined stage has
909 verified entries, zero drafts, 533 fields and 3,640 generated calls.
Counts do not describe measured runtime interpreter load.


## Original battle glyph caller entries - 2026-10-08

98,304 original-ROM cases, 98,306 actual ABI cases, 2,688 guards
and 3 public-library ROM error controls pass. Child probes include
2 forced unwinds, 15 redirects and 2 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 846-job verification and Windows Release build
with the complete 8-entry integration batch. Its final combined stage has
909 verified entries, zero drafts, 533 fields and 3,640 generated calls.
Counts do not describe measured runtime interpreter load.


## Original battle name tiles entries - 2026-10-08

9,216 original-ROM cases, 9,228 actual ABI cases, 6,144 guards
and 38 public-library ROM error controls pass. Child probes include
12 forced unwinds, 30 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 846-job verification and Windows Release build
with the complete 8-entry integration batch. Its final combined stage has
909 verified entries, zero drafts, 533 fields and 3,640 generated calls.
Counts do not describe measured runtime interpreter load.


Both original name resources and all nested glyph calls compose.

## Original effect target motion entries - 2026-10-08

147,456 original-ROM cases, 147,456 actual ABI cases, 1,920 guards
and 58 public-library ROM error controls pass. Child probes include
0 forced unwinds, 0 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 846-job verification and Windows Release build
with the complete 8-entry integration batch. Its final combined stage has
909 verified entries, zero drafts, 533 fields and 3,640 generated calls.
Counts do not describe measured runtime interpreter load.


Repeated party origin and signed stream reads remain original.

## Original effect display transition entries - 2026-10-08

65,728 original-ROM cases, 65,740 actual ABI cases, 3,456 guards
and 60 public-library ROM error controls pass. Child probes include
12 forced unwinds, 15 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 846-job verification and Windows Release build
with the complete 8-entry integration batch. Its final combined stage has
909 verified entries, zero drafts, 533 fields and 3,640 generated calls.
Counts do not describe measured runtime interpreter load.


## Original effect saved actor entries - 2026-10-08

132,096 original-ROM cases, 132,103 actual ABI cases, 6,144 guards
and 75 public-library ROM error controls pass. Child probes include
7 forced unwinds, 30 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 846-job verification and Windows Release build
with the complete 8-entry integration batch. Its final combined stage has
909 verified entries, zero drafts, 533 fields and 3,640 generated calls.
Counts do not describe measured runtime interpreter load.

## Original vector division entries - 2026-10-08

81,920 original-ROM cases, 81,920 actual ABI cases, 2,304 guards
and 119 public-library ROM error controls pass. Child probes include
0 forced unwinds, 0 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 867-job verification and Windows Release build
with the complete 9-entry integration batch. Its final combined stage has
918 verified entries, zero drafts, 542 fields and 3,676 generated calls.
Counts do not describe measured runtime interpreter load.


## Original vector angle entries - 2026-10-08

98,304 original-ROM cases, 98,306 actual ABI cases, 2,688 guards
and 63 public-library ROM error controls pass. Child probes include
2 forced unwinds, 15 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 867-job verification and Windows Release build
with the complete 9-entry integration batch. Its final combined stage has
918 verified entries, zero drafts, 542 fields and 3,676 generated calls.
Counts do not describe measured runtime interpreter load.


Mixed-width exits and retained positive-vertical scratch remain literal.

## Original effect current target entries - 2026-10-08

66,048 original-ROM cases, 66,050 actual ABI cases, 3,072 guards
and 41 public-library ROM error controls pass. Child probes include
2 forced unwinds, 15 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 867-job verification and Windows Release build
with the complete 9-entry integration batch. Its final combined stage has
918 verified entries, zero drafts, 542 fields and 3,676 generated calls.
Counts do not describe measured runtime interpreter load.


## Original battle ripple setup entries - 2026-10-08

66,048 original-ROM cases, 66,049 actual ABI cases, 3,072 guards
and 20 public-library ROM error controls pass. Child probes include
1 forced unwinds, 15 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 867-job verification and Windows Release build
with the complete 9-entry integration batch. Its final combined stage has
918 verified entries, zero drafts, 542 fields and 3,676 generated calls.
Counts do not describe measured runtime interpreter load.


Separate row delay and cleared state retain original HDMA setup.

## Original hardware division entries - 2026-10-08

81,920 original-ROM cases, 81,920 actual ABI cases, 3,072 guards
and 16 public-library ROM error controls pass. Child probes include
0 forced unwinds, 0 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 867-job verification and Windows Release build
with the complete 9-entry integration batch. Its final combined stage has
918 verified entries, zero drafts, 542 fields and 3,676 generated calls.
Counts do not describe measured runtime interpreter load.


## Original effect enemy scripts entries - 2026-10-08

66,049 original-ROM cases, 66,052 actual ABI cases, 3,072 guards
and 66 public-library ROM error controls pass. Child probes include
3 forced unwinds, 15 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 867-job verification and Windows Release build
with the complete 9-entry integration batch. Its final combined stage has
918 verified entries, zero drafts, 542 fields and 3,676 generated calls.
Counts do not describe measured runtime interpreter load.


## Original capsule selection addresses entries - 2026-10-08

245,760 original-ROM cases, 245,760 actual ABI cases, 6,912 guards
and 13 public-library ROM error controls pass. Child probes include
0 forced unwinds, 0 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 867-job verification and Windows Release build
with the complete 9-entry integration batch. Its final combined stage has
918 verified entries, zero drafts, 542 fields and 3,676 generated calls.
Counts do not describe measured runtime interpreter load.

## Original capsule item choice entries - 2026-10-08

66,048 original-ROM cases, 66,049 actual ABI cases, 3,072 guards
and 8 public-library ROM error controls pass. Child probes include
1 forced unwinds, 15 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 891-job verification and Windows Release build
with the complete 24-entry integration batch. Its final combined stage has
942 verified entries, zero drafts, 547 fields and 3,772 generated calls.
Counts do not describe measured runtime interpreter load.


## Original capsule status address entries - 2026-10-08

81,920 original-ROM cases, 81,920 actual ABI cases, 2,304 guards
and 5 public-library ROM error controls pass. Child probes include
0 forced unwinds, 0 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 891-job verification and Windows Release build
with the complete 24-entry integration batch. Its final combined stage has
942 verified entries, zero drafts, 547 fields and 3,772 generated calls.
Counts do not describe measured runtime interpreter load.


## Original menu tile clear entries - 2026-10-08

295,168 original-ROM cases, 295,168 actual ABI cases, 5,376 guards
and 19 public-library ROM error controls pass. Child probes include
0 forced unwinds, 0 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 891-job verification and Windows Release build
with the complete 24-entry integration batch. Its final combined stage has
942 verified entries, zero drafts, 547 fields and 3,772 generated calls.
Counts do not describe measured runtime interpreter load.


All278784 legal rectangles and5376 geometry/context guards pass.

## Original capsule menu chain entries - 2026-10-08

264,192 original-ROM cases, 264,206 actual ABI cases, 12,288 guards
and 68 public-library ROM error controls pass. Child probes include
14 forced unwinds, 60 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 891-job verification and Windows Release build
with the complete 24-entry integration batch. Its final combined stage has
942 verified entries, zero drafts, 547 fields and 3,772 generated calls.
Counts do not describe measured runtime interpreter load.


Four complete parent contracts compose all14 original child sites natively.

## Original effect angle motion entries - 2026-10-08

266,240 original-ROM cases, 266,246 actual ABI cases, 12,288 guards
and 74 public-library ROM error controls pass. Child probes include
6 forced unwinds, 60 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 891-job verification and Windows Release build
with the complete 24-entry integration batch. Its final combined stage has
942 verified entries, zero drafts, 547 fields and 3,772 generated calls.
Counts do not describe measured runtime interpreter load.


All six direct/nested child sites and mixed M0/M1 angle returns pass.

## Original effect draw state entries - 2026-10-08

409,600 original-ROM cases, 409,600 actual ABI cases, 15,360 guards
and 79 public-library ROM error controls pass. Child probes include
0 forced unwinds, 0 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 891-job verification and Windows Release build
with the complete 24-entry integration batch. Its final combined stage has
942 verified entries, zero drafts, 547 fields and 3,772 generated calls.
Counts do not describe measured runtime interpreter load.


Packed and individual draw attributes retain their original dirty latch.

## Original effect parameter words entries - 2026-10-08

133,120 original-ROM cases, 133,121 actual ABI cases, 6,144 guards
and 23 public-library ROM error controls pass. Child probes include
1 forced unwinds, 15 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 891-job verification and Windows Release build
with the complete 24-entry integration batch. Its final combined stage has
942 verified entries, zero drafts, 547 fields and 3,772 generated calls.
Counts do not describe measured runtime interpreter load.


Set/randomize entries retain all offsets and native random-scale calls.

## Original effect byte acceleration entries - 2026-10-08

399,360 original-ROM cases, 399,366 actual ABI cases, 18,432 guards
and 92 public-library ROM error controls pass. Child probes include
6 forced unwinds, 90 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 891-job verification and Windows Release build
with the complete 24-entry integration batch. Its final combined stage has
942 verified entries, zero drafts, 547 fields and 3,772 generated calls.
Counts do not describe measured runtime interpreter load.

## Original effect palette copy entries - 2026-10-08

82,184 original-ROM cases, 82,184 actual ABI cases, 3,072 guards
and 21 public-library ROM error controls pass. Child probes include
0 forced unwinds, 0 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 909-job verification and Windows Release build
with the complete 23-entry integration batch. Its final combined stage has
965 verified entries, zero drafts, 551 fields and 3,864 generated calls.
Counts do not describe measured runtime interpreter load.


All index/count bytes, nine zero-count loops and M0 exits pass.

## Original effect vector placement entries - 2026-10-08

133,120 original-ROM cases, 133,122 actual ABI cases, 6,144 guards
and 43 public-library ROM error controls pass. Child probes include
2 forced unwinds, 30 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 909-job verification and Windows Release build
with the complete 23-entry integration batch. Its final combined stage has
965 verified entries, zero drafts, 551 fields and 3,864 generated calls.
Counts do not describe measured runtime interpreter load.


## Original effect delay presets entries - 2026-10-08

737,280 original-ROM cases, 737,280 actual ABI cases, 27,648 guards
and 27 public-library ROM error controls pass. Child probes include
0 forced unwinds, 0 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 909-job verification and Windows Release build
with the complete 23-entry integration batch. Its final combined stage has
965 verified entries, zero drafts, 551 fields and 3,864 generated calls.
Counts do not describe measured runtime interpreter load.


## Original effect stream fields entries - 2026-10-08

573,440 original-ROM cases, 573,440 actual ABI cases, 21,504 guards
and 57 public-library ROM error controls pass. Child probes include
0 forced unwinds, 0 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 909-job verification and Windows Release build
with the complete 23-entry integration batch. Its final combined stage has
965 verified entries, zero drafts, 551 fields and 3,864 generated calls.
Counts do not describe measured runtime interpreter load.


## Original effect target state entries - 2026-10-08

245,760 original-ROM cases, 245,760 actual ABI cases, 9,216 guards
and 90 public-library ROM error controls pass. Child probes include
0 forced unwinds, 0 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 909-job verification and Windows Release build
with the complete 23-entry integration batch. Its final combined stage has
965 verified entries, zero drafts, 551 fields and 3,864 generated calls.
Counts do not describe measured runtime interpreter load.


## Original effect palette load entries - 2026-10-08

82,184 original-ROM cases, 82,184 actual ABI cases, 3,072 guards
and 32 public-library ROM error controls pass. Child probes include
0 forced unwinds, 0 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 909-job verification and Windows Release build
with the complete 23-entry integration batch. Its final combined stage has
965 verified entries, zero drafts, 551 fields and 3,864 generated calls.
Counts do not describe measured runtime interpreter load.

## Original effect circle window entries - 2026-10-08

133,120 original-ROM cases, 133,122 actual ABI cases, 6,144 guards
and 50 public-library ROM error controls pass. Child probes include
2 forced unwinds, 30 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 948-job verification and Windows Release build
with the complete 35-entry integration batch. Its final combined stage has
1000 verified entries, zero drafts, 560 fields and 4,004 generated calls.
Counts do not describe measured runtime interpreter load.


## Original battle layer color dma entries - 2026-10-08

81,920 original-ROM cases, 81,920 actual ABI cases, 3,072 guards
and 26 public-library ROM error controls pass. Child probes include
0 forced unwinds, 0 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 948-job verification and Windows Release build
with the complete 35-entry integration batch. Its final combined stage has
1000 verified entries, zero drafts, 560 fields and 4,004 generated calls.
Counts do not describe measured runtime interpreter load.


## Original effect actor commands entries - 2026-10-08

819,200 original-ROM cases, 819,200 actual ABI cases, 30,720 guards
and 112 public-library ROM error controls pass. Child probes include
0 forced unwinds, 0 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 948-job verification and Windows Release build
with the complete 35-entry integration batch. Its final combined stage has
1000 verified entries, zero drafts, 560 fields and 4,004 generated calls.
Counts do not describe measured runtime interpreter load.


## Original effect script commands entries - 2026-10-08

245,760 original-ROM cases, 245,760 actual ABI cases, 9,216 guards
and 36 public-library ROM error controls pass. Child probes include
0 forced unwinds, 0 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 948-job verification and Windows Release build
with the complete 35-entry integration batch. Its final combined stage has
1000 verified entries, zero drafts, 560 fields and 4,004 generated calls.
Counts do not describe measured runtime interpreter load.


## Original effect record controls entries - 2026-10-08

409,600 original-ROM cases, 409,600 actual ABI cases, 15,360 guards
and 23 public-library ROM error controls pass. Child probes include
0 forced unwinds, 0 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 948-job verification and Windows Release build
with the complete 35-entry integration batch. Its final combined stage has
1000 verified entries, zero drafts, 560 fields and 4,004 generated calls.
Counts do not describe measured runtime interpreter load.


## Original effect palette presets entries - 2026-10-08

245,760 original-ROM cases, 245,760 actual ABI cases, 9,216 guards
and 41 public-library ROM error controls pass. Child probes include
0 forced unwinds, 0 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 948-job verification and Windows Release build
with the complete 35-entry integration batch. Its final combined stage has
1000 verified entries, zero drafts, 560 fields and 4,004 generated calls.
Counts do not describe measured runtime interpreter load.


The common upload helper also rechecks existing8196C6:82184 ROM and ABI states, plus3072 guards.

## Original effect tile rectangle entries - 2026-10-08

149,008 original-ROM cases, 149,008 actual ABI cases, 3,072 guards
and 48 public-library ROM error controls pass. Child probes include
0 forced unwinds, 0 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 948-job verification and Windows Release build
with the complete 35-entry integration batch. Its final combined stage has
1000 verified entries, zero drafts, 560 fields and 4,004 generated calls.
Counts do not describe measured runtime interpreter load.


All65536 coordinate pairs and stream positions,1024 bounded dimensions, each byte width/height and16 extremes retain original forward overlap.

## Original effect conditional script entries - 2026-10-08

81,920 original-ROM cases, 81,920 actual ABI cases, 3,072 guards
and 13 public-library ROM error controls pass. Child probes include
0 forced unwinds, 0 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 948-job verification and Windows Release build
with the complete 35-entry integration batch. Its final combined stage has
1000 verified entries, zero drafts, 560 fields and 4,004 generated calls.
Counts do not describe measured runtime interpreter load.


Both width-dependent stream advances and1321 exhausted original script owners pass.

## Original effect palette brightness entries - 2026-10-08

139,264 original-ROM cases, 139,266 actual ABI cases, 6,144 guards
and 31 public-library ROM error controls pass. Child probes include
2 forced unwinds, 30 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 948-job verification and Windows Release build
with the complete 35-entry integration batch. Its final combined stage has
1000 verified entries, zero drafts, 560 fields and 4,004 generated calls.
Counts do not describe measured runtime interpreter load.


All selector/level and current/delta byte pairs, plus4096 native palette compositions per root, preserve the original saturation branches.

## Original effect target toggle entries - 2026-10-08

163,840 original-ROM cases, 163,840 actual ABI cases, 6,144 guards
and 13 public-library ROM error controls pass. Child probes include
0 forced unwinds, 0 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 948-job verification and Windows Release build
with the complete 35-entry integration batch. Its final combined stage has
1000 verified entries, zero drafts, 560 fields and 4,004 generated calls.
Counts do not describe measured runtime interpreter load.


## Original effect target toggle callers entries - 2026-10-08

133,120 original-ROM cases, 133,122 actual ABI cases, 6,144 guards
and 9 public-library ROM error controls pass. Child probes include
2 forced unwinds, 30 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 948-job verification and Windows Release build
with the complete 35-entry integration batch. Its final combined stage has
1000 verified entries, zero drafts, 560 fields and 4,004 generated calls.
Counts do not describe measured runtime interpreter load.


## Original effect graphics resource entries - 2026-10-08

65,674 original-ROM cases, 65,675 actual ABI cases, 3,072 guards
and 14 public-library ROM error controls pass. Child probes include
1 forced unwinds, 15 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 948-job verification and Windows Release build
with the complete 35-entry integration batch. Its final combined stage has
1000 verified entries, zero drafts, 560 fields and 4,004 generated calls.
Counts do not describe measured runtime interpreter load.


## Original effect tilemap callers entries - 2026-10-08

133,120 original-ROM cases, 133,122 actual ABI cases, 6,144 guards
and 2 public-library ROM error controls pass. Child probes include
2 forced unwinds, 30 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 948-job verification and Windows Release build
with the complete 35-entry integration batch. Its final combined stage has
1000 verified entries, zero drafts, 560 fields and 4,004 generated calls.
Counts do not describe measured runtime interpreter load.

## Exact gauge counter writes — 2026-10-08

The two word DEC operations in81:E5C1/E604 now write high byte before
low byte, matching original65816 bus order. Values and CPU state remain
literal. The turn-display composition detects the uncorrected trace;
both changed operand controls are detected. Shared full981, Release,
strict C and781824 ROM/781842 ABI cases across eleven new callers pass.
No new entry or binding is counted for this existing helper correction.


## Original effect portrait pose entries - 2026-10-08

66,560 original-ROM cases, 66,561 actual ABI cases, 3,072 guards
and 14 public-library ROM error controls pass. Child probes include
1 forced unwinds, 15 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 981-job verification and Windows Release build
with the complete 11-entry integration batch. Its final combined stage has
1011 verified entries, zero drafts, 561 fields and 4,048 generated calls.
Counts do not describe measured runtime interpreter load.


## Original effect portrait restore entries - 2026-10-08

66,560 original-ROM cases, 66,561 actual ABI cases, 3,072 guards
and 15 public-library ROM error controls pass. Child probes include
1 forced unwinds, 15 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 981-job verification and Windows Release build
with the complete 11-entry integration batch. Its final combined stage has
1011 verified entries, zero drafts, 561 fields and 4,048 generated calls.
Counts do not describe measured runtime interpreter load.


## Original effect portrait status entries - 2026-10-08

66,560 original-ROM cases, 66,562 actual ABI cases, 3,072 guards
and 22 public-library ROM error controls pass. Child probes include
2 forced unwinds, 30 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 981-job verification and Windows Release build
with the complete 11-entry integration batch. Its final combined stage has
1011 verified entries, zero drafts, 561 fields and 4,048 generated calls.
Counts do not describe measured runtime interpreter load.


## Original effect turn display entries - 2026-10-08

66,560 original-ROM cases, 66,563 actual ABI cases, 3,072 guards
and 5 public-library ROM error controls pass. Child probes include
3 forced unwinds, 45 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 981-job verification and Windows Release build
with the complete 11-entry integration batch. Its final combined stage has
1011 verified entries, zero drafts, 561 fields and 4,048 generated calls.
Counts do not describe measured runtime interpreter load.


## Original effect sprite rebuild entries - 2026-10-08

66,560 original-ROM cases, 66,561 actual ABI cases, 3,072 guards
and 10 public-library ROM error controls pass. Child probes include
1 forced unwinds, 15 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 981-job verification and Windows Release build
with the complete 11-entry integration batch. Its final combined stage has
1011 verified entries, zero drafts, 561 fields and 4,048 generated calls.
Counts do not describe measured runtime interpreter load.


## Original effect sound command entries - 2026-10-08

66,560 original-ROM cases, 66,561 actual ABI cases, 3,072 guards
and 5 public-library ROM error controls pass. Child probes include
1 forced unwinds, 15 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 981-job verification and Windows Release build
with the complete 11-entry integration batch. Its final combined stage has
1011 verified entries, zero drafts, 561 fields and 4,048 generated calls.
Counts do not describe measured runtime interpreter load.


## Original effect centered parameter entries - 2026-10-08

66,560 original-ROM cases, 66,561 actual ABI cases, 3,072 guards
and 35 public-library ROM error controls pass. Child probes include
1 forced unwinds, 15 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 981-job verification and Windows Release build
with the complete 11-entry integration batch. Its final combined stage has
1011 verified entries, zero drafts, 561 fields and 4,048 generated calls.
Counts do not describe measured runtime interpreter load.


## Original effect vector actor entries - 2026-10-08

82,944 original-ROM cases, 82,945 actual ABI cases, 3,072 guards
and 42 public-library ROM error controls pass. Child probes include
1 forced unwinds, 15 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 981-job verification and Windows Release build
with the complete 11-entry integration batch. Its final combined stage has
1011 verified entries, zero drafts, 561 fields and 4,048 generated calls.
Counts do not describe measured runtime interpreter load.


## Original effect conditional sound entries - 2026-10-08

82,944 original-ROM cases, 82,945 actual ABI cases, 3,072 guards
and 11 public-library ROM error controls pass. Child probes include
1 forced unwinds, 15 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 981-job verification and Windows Release build
with the complete 11-entry integration batch. Its final combined stage has
1011 verified entries, zero drafts, 561 fields and 4,048 generated calls.
Counts do not describe measured runtime interpreter load.


## Original effect popup actor entries - 2026-10-08

82,944 original-ROM cases, 82,944 actual ABI cases, 3,072 guards
and 28 public-library ROM error controls pass. Child probes include
0 forced unwinds, 0 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 981-job verification and Windows Release build
with the complete 11-entry integration batch. Its final combined stage has
1011 verified entries, zero drafts, 561 fields and 4,048 generated calls.
Counts do not describe measured runtime interpreter load.


## Original effect target popups entries - 2026-10-08

67,072 original-ROM cases, 67,078 actual ABI cases, 3,072 guards
and 108 public-library ROM error controls pass. Child probes include
6 forced unwinds, 90 redirects and 1,107 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 981-job verification and Windows Release build
with the complete 11-entry integration batch. Its final combined stage has
1011 verified entries, zero drafts, 561 fields and 4,048 generated calls.
Counts do not describe measured runtime interpreter load.

## Original effect frame setup entries - 2026-10-08

66,560 original-ROM cases, 66,563 actual ABI cases, 3,072 guards
and 12 public-library ROM error controls pass. Child probes include
3 forced unwinds, 45 redirects and 1,024 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 999-job verification and Windows Release build
with the complete 9-entry integration batch. Its final combined stage has
1020 verified entries, zero drafts, 574 fields and 4,084 generated calls.
Counts do not describe measured runtime interpreter load.


## Original effect actor tiles entries - 2026-10-08

198,160 original-ROM cases, 198,160 actual ABI cases, 3,072 guards
and 69 public-library ROM error controls pass. Child probes include
0 forced unwinds, 0 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 999-job verification and Windows Release build
with the complete 9-entry integration batch. Its final combined stage has
1020 verified entries, zero drafts, 574 fields and 4,084 generated calls.
Counts do not describe measured runtime interpreter load.


## Original battle window fades entries - 2026-10-08

133,120 original-ROM cases, 133,126 actual ABI cases, 6,144 guards
and 19 public-library ROM error controls pass. Child probes include
6 forced unwinds, 90 redirects and 2,048 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 999-job verification and Windows Release build
with the complete 9-entry integration batch. Its final combined stage has
1020 verified entries, zero drafts, 574 fields and 4,084 generated calls.
Counts do not describe measured runtime interpreter load.


## Original battle portrait state entries - 2026-10-08

197,632 original-ROM cases, 197,633 actual ABI cases, 3,072 guards
and 37 public-library ROM error controls pass. Child probes include
1 forced unwinds, 15 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 999-job verification and Windows Release build
with the complete 9-entry integration batch. Its final combined stage has
1020 verified entries, zero drafts, 574 fields and 4,084 generated calls.
Counts do not describe measured runtime interpreter load.


## Original battle capsule graphics entries - 2026-10-08

66,560 original-ROM cases, 66,562 actual ABI cases, 3,072 guards
and 42 public-library ROM error controls pass. Child probes include
2 forced unwinds, 30 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 999-job verification and Windows Release build
with the complete 9-entry integration batch. Its final combined stage has
1020 verified entries, zero drafts, 574 fields and 4,084 generated calls.
Counts do not describe measured runtime interpreter load.


## Original enemy sprite setup entries - 2026-10-08

330,752 original-ROM cases, 330,754 actual ABI cases, 9,216 guards
and 56 public-library ROM error controls pass. Child probes include
2 forced unwinds, 30 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 999-job verification and Windows Release build
with the complete 9-entry integration batch. Its final combined stage has
1020 verified entries, zero drafts, 574 fields and 4,084 generated calls.
Counts do not describe measured runtime interpreter load.

## Original enemy graphics entries - 2026-10-08

199,680 original-ROM cases, 199,685 actual ABI cases, 9,216 guards
and 78 public-library ROM error controls pass. Child probes include
5 forced unwinds, 75 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 1023-job verification and Windows Release build
with the complete 23-entry integration batch. Its final combined stage has
1043 verified entries, zero drafts, 597 fields and 4,176 generated calls.
Counts do not describe measured runtime interpreter load.


## Original transition setup entries - 2026-10-08

201,728 original-ROM cases, 201,757 actual ABI cases, 9,216 guards
and 180 public-library ROM error controls pass. Child probes include
29 forced unwinds, 435 redirects and 1,536 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 1023-job verification and Windows Release build
with the complete 23-entry integration batch. Its final combined stage has
1043 verified entries, zero drafts, 597 fields and 4,176 generated calls.
Counts do not describe measured runtime interpreter load.


## Original encounter state entries - 2026-10-08

327,680 original-ROM cases, 327,680 actual ABI cases, 12,288 guards
and 94 public-library ROM error controls pass. Child probes include
0 forced unwinds, 0 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 1023-job verification and Windows Release build
with the complete 23-entry integration batch. Its final combined stage has
1043 verified entries, zero drafts, 597 fields and 4,176 generated calls.
Counts do not describe measured runtime interpreter load.


## Original encounter generation entries - 2026-10-08

133,120 original-ROM cases, 133,132 actual ABI cases, 5,760 guards
and 142 public-library ROM error controls pass. Child probes include
12 forced unwinds, 180 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 1023-job verification and Windows Release build
with the complete 23-entry integration batch. Its final combined stage has
1043 verified entries, zero drafts, 597 fields and 4,176 generated calls.
Counts do not describe measured runtime interpreter load.


## Original encounter loader entries - 2026-10-08

133,120 original-ROM cases, 133,126 actual ABI cases, 5,760 guards
and 113 public-library ROM error controls pass. Child probes include
6 forced unwinds, 90 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 1023-job verification and Windows Release build
with the complete 23-entry integration batch. Its final combined stage has
1043 verified entries, zero drafts, 597 fields and 4,176 generated calls.
Counts do not describe measured runtime interpreter load.


## Original record reset entries - 2026-10-08

337,920 original-ROM cases, 337,926 actual ABI cases, 13,440 guards
and 38 public-library ROM error controls pass. Child probes include
6 forced unwinds, 90 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 1023-job verification and Windows Release build
with the complete 23-entry integration batch. Its final combined stage has
1043 verified entries, zero drafts, 597 fields and 4,176 generated calls.
Counts do not describe measured runtime interpreter load.


## Original status cache entries - 2026-10-08

135,168 original-ROM cases, 135,168 actual ABI cases, 5,376 guards
and 71 public-library ROM error controls pass. Child probes include
0 forced unwinds, 0 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 1023-job verification and Windows Release build
with the complete 23-entry integration batch. Its final combined stage has
1043 verified entries, zero drafts, 597 fields and 4,176 generated calls.
Counts do not describe measured runtime interpreter load.


## Original sprite descriptors entries - 2026-10-08

133,120 original-ROM cases, 133,122 actual ABI cases, 5,376 guards
and 35 public-library ROM error controls pass. Child probes include
2 forced unwinds, 30 redirects and 0 natural overwritten
returns. Original registers, WRAM, bus order and return frames are retained.

This group shares the full 1023-job verification and Windows Release build
with the complete 23-entry integration batch. Its final combined stage has
1043 verified entries, zero drafts, 597 fields and 4,176 generated calls.
Counts do not describe measured runtime interpreter load.

### Menu confirmations and item prices

Four complete M1X0 callers cover exit confirmation, alternate selection,
display restoration and selection prices. Their 50 original child sites,
retry branches, fade waits, word/byte prices, stack values, flags and live
data-bank reads remain literal. Two neutral fields reuse the original bytes.

The existing item-name stack operand now uses explicit low/high reads. This
corrects a C evaluation-order discrepancy demonstrated by real price calls;
the original ROM behavior and all registers, flags and writes stay unchanged.
The item-record-byte bridge additionally accepts the verified M0X0 menu
context (PB81, DB7E, DP0, S1F10..1FFC). Its existing M1X0 contract is retained;
canonical metadata stays M1X0. Other M0 contexts still use the interpreter.

Shared original-ROM and production-ABI proofs pass 49,202 unique states each,
including 8,192 nested parent states and 32,682 actual child returns. All 512
item IDs and both accumulator modes are covered; record loading also covers
its existing index modes. All 18,048 guards, 810 redirects, 50 forced unwinds,
186 negative ROM controls and six C++ APIs pass. Other child internals remain
explicit services. Public-library proofs and all 1,113 full verification jobs pass.
Windows Release passes.

### Selection gold and equipment updates

Five complete original callers retain the 24-bit gold comparison and debit,
alternate-selection update, eight-step feedback and six flagged equipment
increments. Low/high borrow, CPU flags, hidden accumulator, live indirect
pointers, DB-relative feedback writes and all nine pushed child frames are
preserved. Four roots enter M1X0; flagged equipment enters M0X0.

Shared original-ROM and production-ABI proofs pass 48,137 unique states each,
including 3,072 nested parents with 41,088 actual child returns through twelve
reconstructed contracts. All 17,280 guards, 225 redirects, nine forced unwinds,
51 negative ROM controls and five C++ APIs pass. Unknown display/input services
remain explicit. Public-library proofs, all 1,128 full jobs and Windows Release pass.

### Menu inventory additions and possession lookup

Seven complete M0X0 entries store packed slots, update/cap quantities, test
the original equality99 limit, scan96 slots, register possession and add
the selected menu item. The possession-bit body is shared with the existing
FB1F count entry; no duplicate table scanner was introduced. DP scratch,
hidden accumulator, original overflow, DB-relative word overlap and ordered
RTS/RTL frames stay literal. The 09CF field is phase-dependent byte/word.

Original-ROM and production-ABI root proofs cover65,543 states each,
including16,384 complete parents with43,696 actual reconstructed child
returns. All24,192 guards,165 redirects,seven forced unwinds,83 meaningful
ROM controls and eight C++ APIs pass. All106 reachable noncall/return
boundaries are covered. The original table terminates at entry34, making
the64-entry exhaustion BRA unreachable in the supported immutable ROM;
the original bounded exhaustion path remains in C.
Public-library proofs, all 1,128 full jobs and Windows Release pass.

### Menu selection and session services

Six complete M1X0 contracts preserve selection retries, five input-result
returns, cursor style restoration, the original session reset, Capsule arrays,
24-bit gold addition with its original wrap and cap, and byte-ordered field
buffer copies. All fourteen pushed child sites and the PHP/PLP status frame
remain original. Three neutral persistent reset fields are catalogued.

68,622 unique original-ROM and production-ABI root states each pass, including
6,144 nested roots with 16,332 actual cursor/style/animation/Capsule returns.
Input, sound and remaining cursor-slide internals use explicit child contracts
in these parent proofs. 20,736 guards, 315 redirects, fourteen forced unwinds,
127 effective bounded ROM mutations and six C++ APIs pass. All 156 reachable
noncall/return boundaries across 180 original instruction states are covered.
Public-library proofs, all 1,128 full jobs and Windows Release pass.

### Capsule ability flags and spell registration

Six complete M1X0 contracts preserve saved-stat writes, per-Capsule ability
words, original three-bit form shifts, ROM-backed ability values, unlocking
and the original36-slot party spell list. Applicability, duplicates, full
lists, FF insertion, carry, live pointers and CPU/ordered bus effects remain
literal. The form-zero16-bit-wrap shift loop remains unchanged and tested.
The final ability word11D6 is catalogued without overlapping existing flags;
the original20-byte flag clear still ends at11D5.

53,258 original-ROM and production-ABI root states each pass, including
8,192 complete parents with15,694 actual returns through all five child APIs.
20,736 guards,255 redirects,nine forced unwinds,84 effective bounded ROM
controls and six C++ APIs pass. All114 reachable noncall-return boundaries
across132 original instruction states are covered. Public-library proofs, all1,148 full jobs and Windows Release pass.

### Save-selection callers and shared display

Six complete M1X0 contracts preserve selection setup, alternate modes,
callback restoration, shared display, original frame flags and pushed
JSR/JSL frames. Five original RTL callers share one original RTS display
service. The inlineE8CB dispatch table remains an explicit child, and the
already recoveredE8DE routine is neither duplicated nor decoded as data.

30,742 original-ROM and production-ABI root states each pass, including
6,144 nested roots with30,720 actual returns through six recovered child
APIs. Remaining child internals are explicit controlled contracts, not
claims of complete original gameplay.20,736 guards,555 redirects,22 forced
unwinds,30 bounded altered-ROM controls and six C++ APIs pass. All30
noncall-return boundaries across58 original instruction states are covered.
Public-library proofs, all1,148 full jobs and Windows Release pass.

### Capsule and received-item selection callers

Two complete M1X0 contracts retain original selection setup, saved Capsule
IDs, ROM-backed initial levels, stat transfers and conditional item display.
The shared item return restores M1 on both M0 and M1 arrivals. The original
09D0 bit update belongs to the existing selected-spell word; no overlapping
field is introduced. The unexplained1562 byte remains neutrally named.

20,500 original-ROM and production-ABI root states each pass, including
4,096 nested roots with34,648 actual returns through16 recovered child APIs.
Unknown graphics, fades, Capsule services and inline dispatch children remain
explicit controlled contracts.6,912 guards,465 redirects,20 forced unwinds,
47 bounded altered-ROM controls and two C++ APIs pass. All47 reachable
noncall-return boundaries across71 instruction states are covered.
Public-library proofs, all1,148 full jobs and Windows Release pass.

### Name-entry and item selection loops

Three complete M1X0 RTS contracts retain input repeats, character insertion,
deletion, cursor-edge behavior, item actions, conditional menu closure and
the original scenario handoff. Name text edits reuse the recovered record
and glyph routines. Cursor and item flags stay live; original loops and
their flags, byte widths, stack frames and ordered bus effects remain exact.
Name-entry frame flags74 now share one catalogued name; the existing name
routines change only that constant spelling.

15,387 original-ROM and production-ABI root states each pass, including
3,072 nested roots with13,808 actual returns through six name and item APIs.
Unknown input, presentation and inline-table children remain explicit.
10,368 guards,540 redirects,27 forced unwinds,80 bounded altered-ROM controls
and three C++ APIs pass. All97 reachable noncall-return boundaries across127
instruction states are covered. Initial reduced control cases omitted one
cursor origin and an item flag; both were added. Altered unconditional jumps
outside the valid loop are excluded, not counted as detected mismatches.
Public-library proofs, all1,148 full jobs and Windows Release pass.

### Save confirmation and erase selection

Three complete M1X0 RTS contracts retain alternate slot views, overwrite
confirmation, cancellation, repeated input, SRAM header flags, two erase
confirmations and the original8192 byte erase loop. Unknown save/input and
presentation children remain explicit pushed-frame contracts.

15,404 original-ROM and production-ABI roots each pass, including3072 nested
roots with15104 actual returns through six recovered APIs. Full WRAM,8KiB
mirrored SRAM, CPU, MMIO and ordered data access are compared.10368 guards,
780 redirects,44 forced unwinds,77 bounded altered-ROM controls and three
C++ contracts pass. All88 noncall-return boundaries across135 instruction
states are covered. Counter-removal and invalid unconditional-jump mutants
are excluded; none is claimed as a detected semantic mismatch.

Nested parents exposed existing decomp fidelity gaps: original wordDEC
writes high byte first, three text dispatches read original ROM tables,
and branch operands reload the original format byte. Local changes restore
these exact bus effects without changing shared CPU helpers or ROM quirks.
4096 window geometries and4096 generated strings, including91 exact
interpreter handoffs, pass CPU/full WRAM/ordered bus comparison.
Seven public-library jobs, all1,155 full jobs and Windows Release pass.

### Cursor selection state persistence

The complete8E:E710 writer retains the six column/row records, original
ROM-selected bank7F cache, starting-index behavior and three scroll/list
words. The existing8E:E751 reader is reverified in the same batch; no
duplicate reader or speculative menu layout is introduced.

16,384 direct original-ROM and production-ABI states each cover both
contracts, all18 original state keys, six starting indices and all reader
branches. CPU, full WRAM, MMIO and ordered pointer-table/WRAM access match.
4,096 save/change/restore transactions additionally check both original
boundaries and restored cursor/scroll values;8,192 native ABI calls pass.
6,912 rejected entry states,24 bounded altered-ROM controls and two C++
contracts pass. All34 nonreturn boundaries of the35-state writer are covered.
The writer remains literal for other starting indices; runtime selection
requires the same proven Y<6 contract as the existing reader. Unknown
DP relocation effects remain original; transaction value assertions use D0.
Five public-library jobs, all1,160 full jobs and Windows Release pass.

### Initial map object setup

Three complete M1X0 parents and two exact reset routines preserve conditional object records, the literal
64-entry saved-control loop and pending-object tile/coordinate initialization.
Original banks, widths, frames, FF terminators and bus access order remain.
Headers and region updates remain explicit pushed-frame services.

26,634 original-ROM and production-ABI parent states each pass, with2,048
nested parents and59,094 actual child returns.15,360 unsupported states,
90 redirected entries,10 forced unwinds,64 bounded altered-ROM controls
and five C++ contracts pass.137 instruction states/122 noncall-return
boundaries are covered. CPU/full WRAM/MMIO/ordered data access match.
Pending-object event compositions cover the FF no-script path and timer
states without script resumption; they do not claim complete event execution.
The pending flags are48 original byte records, not a single-byte field.
Thirteen public-library jobs, all 1,173 full jobs and Windows Release pass.

### Save-menu dispatch and character summary

Four complete entry contracts preserve the original save-menu call chain,
character-summary status and inclusive HP color thresholds, and tilemap
upload bit conversion. Related text uses the original string child. Fades,
save dispatch and map reload remain explicit pushed-frame services.

79,453 original-ROM and production-ABI parent states each pass; 2,048
compositions execute the actual original text child. The proof covers all
256 data banks, 12,288 entry guards, 45 redirects, five child unwinds,
67 bounded altered-ROM controls and four C++ contracts. All 80 noncall
return boundaries across 94 instruction states are covered. CPU, full
WRAM, MMIO and ordered data operands stay literal. Existing map-data C is
reused without duplication: 3,672 states cover all 27 primary resources,
17 alternate variants, four destinations and two data banks, with 17,928
actual decompressor, advance and offset returns in each proof path.
Eleven public-library jobs, all 1,184 full jobs and Windows Release pass.

### Party status drawing and upload callers

Three complete callers retain the original party-window, level, HP, MP,
IP, portrait-state and sprite upload sequences. Drawing phases preserve
the original pushed frames and exact child-unwind boundaries. The existing
shared gauge-fill interior remains an explicit original child.

16,395 original-ROM and production-ABI parent states each pass, including
4,096 mixed actual parents and 26,624 actual child returns. Entry checks
cover 9,216 rejected states, 120 redirect probes and eleven forced unwinds.
79 bounded altered-ROM controls and three C++ contracts pass. All 195
noncall-return boundaries across 209 original instruction states are covered.
CPU, full WRAM, MMIO and ordered data access are retained.

Window/frame counters now use the original high-byte-first RMW order.
Four existing APIs pass 4,096 additional original-ROM and production-ABI
states each. Portrait composition covers empty slots; the nonempty loader
is independently reconstructed. The existing M1X0 allocator binding falls
back in 1,024 composed M0X0 calls; those fallbacks are explicitly checked.
Twelve public-library jobs, all 1,196 full jobs and Windows Release pass.

### Shared party gauge and queue entry modes

The independent party-gauge entry reuses the existing four-part gauge core.
Three redundant direct-page reads are removed to retain the original bus
sequence. The shared division uses the original SBC operation, including
decimal mode. Existing M1X0 queue selection remains intact; the additional
M0X0 context retains both proven inherited data banks and the full-queue
original BRK continuation. No second gauge or allocator is reconstructed.

106,496 original-ROM and 106,496 production-ABI states pass across the gauge
core, existing labeled gauge, division and queue. The direct gauge matrix
covers all 256 data banks, three direct pages and both decimal modes;
division additionally covers all four M/X combinations. 6,528 entry guards,
39 bounded altered-ROM controls and four C++ interfaces pass. All 68
executed gauge instruction states and both queue accumulator widths are
covered. CPU, full WRAM, MMIO and ordered data access remain literal.

2,048 party-detail compositions execute all six original digit/gauge children,
with 12,288 actual returns and 6,144 native gauge calls without fallback.
2,048 additional mixed display parents execute 20,480 original child returns;
the allocator now executes 1,024 M0X0 calls natively, without fallback.
Those portrait compositions cover empty slots. Unknown portrait-loader paths
are not claimed by these compositions.
Six public-library jobs, all 1,202 full jobs and Windows Release pass.

### Independent menu-window edges

Four original M0X0 edge entries reuse the existing window-pattern renderer.
The routines preserve the complete horizontal, vertical, two-row corner and
three-row corner paths. Their production selection retains binary mode,
DB82, DP0, valid stack and the proved even pattern indices; other contexts
retain original execution. Original dimensions are never clamped.

36,864 original-ROM and 36,864 production-ABI states pass, including 4,096
complete window parents and 32,188 original edge returns per proof path.
The direct matrices cover all 256 data banks and three direct pages. All
74 executed edge instruction states, the four RTS boundaries, CPU, full
WRAM, MMIO and ordered data accesses pass. 16,896 unsupported entry guards,
20 bounded original-ROM controls and four C++ interfaces pass.
Three public-library jobs, all 1,205 full jobs and Windows Release pass.

### Field diagnostic flag and value display

Nine complete entries preserve the original diagnostic flag/value editor,
its two event-state sources, sixteen display rows, cursor tiles, tilemap
clear and button consumption. The bitmask table remains original ROM data.
Shared cursor and flag addressing remain explicit original child calls.
The cursor-address entry returns M0X0; its callers restore M1X0. The clear
routine retains the direct-page-derived word and both incoming index widths.

73,728 original-ROM and 73,728 production-ABI cases pass across all entries.
The direct matrices cover all 256 data banks and three direct pages. All
146 executed instruction states and every original return boundary pass.
CPU, full WRAM, MMIO and ordered data operands remain literal. 30,720 entry
guards, 90 child redirects, six child unwinds, 31 bounded altered-ROM controls
and nine C++ interfaces pass. 1,024 complete display compositions per proof
path execute 49,152 actual original flag/string returns each. Unsupported
entry contexts and pure frame polling retain original interpreter execution.
Six public-library jobs, all 1,211 full jobs and Windows Release pass.

### Radial field objects and shared object services

Five complete original entries preserve radial object spawning, sprite-frame
selection, secondary slot initialization, secondary actor probe lookup and
pending-object type lookup. Existing frame, slot and probe cores remain shared
with their original callers. Radial spawning preserves all incoming M/X
widths, saved status, hardware 256/count division and 256 zero-count iterations.
The primary metadata mode remains the original M1X0 caller contract.

40,960 original-ROM and 40,960 production-ABI entry cases pass across all five
entries. Direct matrices cover every data bank and three direct pages. The
three service matrices execute 24,576 actual original child returns each.
Another 1,024 complete radial parents per proof path execute 26,432 original
spawn returns each. CPU, full WRAM, MMIO and ordered data accesses match.
Three host-return modes, 16,512 rejected entries, 15 child redirects, four
unwind width states, 34 bounded original-ROM controls and five C++ APIs pass.
All original instruction states and complete return contracts are covered.
Unproved production contexts retain original execution.
Twenty-one public-library jobs, all1,232 full jobs and Windows Release pass.

### Shared actor states, battle messages and field reset

Three complete original leaves mark both state arrays with bit40, copy an
indexed battle message including its terminator, and reset field presentation
and animation state. All incoming M/X widths, saved registers/status, banks,
stack accesses and original TDC-derived fills are retained. The message
contract covers all78 actual A5:DF00 entries; other selectors remain original
interpreter work without a semantic clamp or new memory access.

24,576 original-ROM and24,576 production-ABI states pass. Direct matrices
cover all256 data banks,three direct pages and both decimal states. All117
executed instruction states include every required nonreturn instruction and
three terminal boundaries. CPU,fullWRAM,MMIO and ordered operands match.
1,424 invalid semantic selector handoffs,11,214 production guards,15 bounded
original-ROM controls,three host return modes and three C++ interfaces pass.
No child services or gameplay-capture claims are involved. Runtime bindings
select proven binary contexts; unproved contexts keep original execution.
Twenty-one public-library jobs, all1,232 full jobs and Windows Release pass.

### Object parameters, sprite grids and prefixed names

Three complete M1X0 leaves preserve the original object-parameter table,
five-byte sprite-grid construction and prefixed field-name composition.
The parameter routine scans32 kinds and all nine original ROM records.
Sprite grids retain zero-size256 loops,16-pixel spacing and tile transitions;
all scratch reads remain bus-backed without a clamp or cached dimensions.
Names preserve source-bank differences, indirect bank carry, trailing-space
trimming and TDC-derived fills. Original terminal and stack behavior remains.

24,576 original-ROM and24,576 production-ABI states pass across three entries.
Direct parameter/grid proofs cover all256 data banks; name proofs cover130
WRAM-backed banks and64 bank-crossing cases. Each uses three direct pages
and both decimal states. All148 required nonreturn instruction states and
three complete terminals are covered. CPU,fullWRAM,MMIO and ordered data
accesses match.576 semantic-width handoffs,9,984 runtime guards,40 bounded
altered-ROM controls,three host return modes and three C++ interfaces pass.
No actual-parent composition or gameplay-capture claim. Runtime bindings
keep unproved CPU contexts on original execution. No copied ROM data.
Twenty-one public-library jobs, all1,232 full jobs and Windows Release pass.

### Shared field sprite sizes and tile layouts

Nine complete original entries write single,vertical,horizontal and square
OAM layouts,the square's four orientations and the original high-table bits.
Existing callers share these cores and retain original nested stack frames.
IndexedJSR pointer reads remain ordered before its stack writes. Decimal
coordinate additions use the existing complete CPU adapter. Pointer tables
at83:A48A..A491 and83:A589..A590 remain original ROM data.

73,728 original-ROM and73,728 production-ABI cases cover all217 required
nonreturn states,the square's three terminals and original width changes.
The direct proof executes62,144 nested original RTS returns; ABI proof
executes63,224. Native cores retain corresponding stack/bus accesses.
CPU,fullWRAM,MMIO and ordered reads/writes match across all256 data banks,
three direct pages,both decimal states and boundary Y coordinates. Runtime
guards retain unproved contexts on original execution.1,728 unsupported
semantic handoffs,31,104 runtime guards,35 bounded altered-ROM controls,
three host return modes and nine C++ interfaces pass. No gameplay/profile
claim or copied ROM data. Three public jobs, all 1,235 full jobs and Windows Release pass.

### Original field palette children

Two complete palette routines84:8D54/8E07 scale or fade original9B colors
into CGRAM staging. Mode7 product reads,read-only-register RMW writes,
rounding,channel clamps,TDC values,DP aliasing and decimal flags stay exact.
Both explicit palette child sites in83:AEB5 now finish through original
call frames. Palette cycles and HDMA wave work retain their shared native
cores and existing zero-duration-chain continuation. This extends that
verified contract;it does not claim whole-VM or infinite-chain completion.
The legacy portable API and continuation bridge stay compatible.

24,576 ROM and24,576 ABI entry cases,223 required new instruction states.
Native composition adds8,192 ROM and8,192 ABI parent cases:5,300/5,972
native palette returns and260/512 actual original-copy returns. Separate
child compositions retain520/1,024 original-copy and4,788/5,460 palette
returns. CPU,fullWRAM,MMIO and ordered accesses match.896 semantic handoffs,
9,600 runtime guards,60 redirects,192 child unwinds,26 bounded controls,
three return modes and three C++ interfaces pass. Unproved contexts keep
original execution. No gameplay/profile claim or copied ROM artifacts.
Nine public jobs, all 1,244 full jobs and Windows Release pass.

### Original world-map motion services

Eight complete entries load five referenced motion records and an optional
control record, retain tile-position bytes, test arrival conditions, sum
overlapping motion terms, disable display uploads, and start screen fades.
Both arrival entries share their original tail; both fades share one body.
Discarded reads, stack transfers, byte widths and original bus order remain
observable. Reused motion-table bytes retain their existing field metadata.

65,536 original-ROM cases and 65,536 production-ABI cases cover all 99
required nonreturn instruction states. CPU, full WRAM, MMIO and ordered
accesses match. Direct tests cover all 256 DB values, three DP layouts and
both decimal states. Runtime guards retain the proven binary PB/DB86,
DP0, M1X0 and stack contract. 1,536 semantic handoffs, 27,648 runtime guards,
16 bounded ROM controls, eight C++ APIs and three return modes pass.
No original wait or nonterminating scene loop is replaced by a finite stub.
Six shared public jobs, all 1,250 full jobs and Windows Release pass.

### Original world-map resource and view services

Six complete entries resolve banked resources, center tile targets, read
positions and pose from the second original world object, configure shared
view parameters, and restore display brightness. Resource resolution keeps
its ASL carry, repeated table read, stack access, changed DB and M0X0 exit.
Both view variants share their backward original tail. Original pose and
coordinates stay in their real record; no host copy replaces bus accesses.

49,152 direct and 49,152 production-ABI cases cover all 77 required states.
CPU, full WRAM, MMIO and ordered accesses match across all 256 DB values,
three DP layouts and both decimal states. 1,152 semantic handoffs, 20,736
runtime guards, twelve bounded ROM controls, six C++ APIs and three return
modes pass. Six shared public jobs, all 1,250 full jobs and Windows Release pass.

### Original world-map frame callers

Six complete entries connect object heading, paired animations, original
record positioning, display reset, palette DMA and tile classification.
All nine child sites retain their original JSR frames and unwind points.
Existing child implementations remain shared. World X spans its documented
low byte and the existing sprite-upload high byte; metadata does not overlap.

49,152 direct and 49,152 production-ABI cases retain 73,696 original child
returns per path. Another 49,152 cases per path compose native children with
90,080 returns. All 101 required states, CPU, WRAM, MMIO and ordered accesses
match. Original PB/DB 86, DP zero, binary M1X0 contexts use real object records.
3,072 semantic handoffs, 20,736 runtime guards, 135 redirects, 576 child-entry
unwinds per path, seventeen bounded controls and six C++ APIs pass.
Seven public jobs, all 1,257 full jobs and Windows Release pass.

### Original finite field camera and scene services

Six original entries consume pressed latches, advance a banked scene stream,
mark encounter actor timer/flags, count value2B with original saturation and
read ROM-backed map-resource flags. The stream differs from the text helper
in status-stack accesses; its original bank transition remains observable.
Value2B purpose stays neutral. The frame wait remains an original boundary.

49,152 direct and 49,152 production-ABI cases cover all 41 required states.
CPU, full WRAM, MMIO and ordered accesses match across all 256 DB values,
three DP layouts and both decimal states in semantic tests. The production ABI
accepts all data banks with DP0 and binary arithmetic. Original M0X0 counter entry and
five M1X0 entries retain their distinct RTS/RTL frames. 1,152 semantic
handoffs, 18,432 runtime guards, sixteen bounded ROM controls, six C++ APIs
and three return modes pass. Three public jobs, all 1,260 full jobs and Windows Release pass.

### Original battle HDMA command dispatch

The original indirect dispatcher and eighteen enable/disable handlers preserve
nine builder calls, descriptor writes, banked MMIO and both JSR stop wrappers.
The dispatcher retains PHY/PHK/PEA92E6 and the real 81:92E4 JML through
bank-zero129B; bytes +10..+12 alias existing unk_7E1291 storage.
Unknown deeper builders remain original child services.

155,648 direct and 155,648 production-ABI cases cover all 259 required states,
with 98,304 actual original child returns per path. CPU, full WRAM, MMIO
and ordered accesses match. 65,792 indirect boundaries cover all command and
data-bank values, three DP layouts, decimal states and stack aliases.
5,184 semantic handoffs, 58,752 runtime guards, 180 redirects, 8,192 native
handler compositions per path, 768 original child-entry unwinds per path and
1,728 symmetric changed-width return handoffs per path pass. Fifty-five
bounded ROM controls, nineteen C++ APIs and three return modes pass.
Eight public jobs, all 1,268 full jobs and Windows Release pass.

### Original battle HDMA pattern builders

Six original ROM-pattern builders and two stop cores use shared named C
fill/phase helpers. Four fills retain original decimal scroll addition; two
phase slices preserve distinct forward/reverse endpoints and their stop tails.
The offset scroll aliases nmi_scroll_registers +8 and the upload table aliases
field_map_attributes. Original stack/bank behavior and register outputs remain exact.

65,536 direct and 65,536 production-ABI cases cover all 171 required states.
Direct states include all 256 DB values, three DP layouts, decimal arithmetic,
phase endpoints and 4,096 stack-overlap/wrap cases within the same batch.
1,536 semantic handoffs and 24,576 runtime guards pass. The production ABI
retains binary/DP0/native M1X0 contexts and three actual RTS return modes.
Another 155,648 original nineteen-command chains per path compose 8,192
native handler returns and 69,176 native builder returns; 25,942 remaining
original wave returns retain their services. Each builder returns 8,647 times.
CPU, full WRAM, MMIO and ordered accesses match. Sixteen bounded controls
and eight C++ interfaces pass. Twelve grouped public jobs, ONE full 1,280 jobs and Windows Release pass.

### Original battle transition HDMA

Two paired-descriptor setups and two original carry-chained pattern builders
retain the 224-word reverse index and upload tables. Negative sums retain
the original DP result, CMP carry feeds the next row, and nine-step phase
updates preserve decimal/negative behavior. Long bank-zero4317/4327 stores
remain distinct from bank-relative channel writes. Existing aliases suffice.

32,768 direct and32,768 ABI cases cover all184 non-return instruction states;
16,384 original builder returns per path. All256DB/threeDP/decimal and phase
boundaries match CPU, fullWRAM, MMIO andordered access. 1,152 semantic handoffs
and12,288 runtime guards pass. Another16,384 fully native compositions per
path,256 actual child-entry unwinds and768 symmetric changed-width handoffs
per path pass. Sixteen bounded ROM controls andfour CPP APIs pass.
Twelve grouped public jobs, ONE full 1,280 jobs and Windows Release pass.

### Original primary wave caller and shared NMI table

85:A804 retains the original four offset bands/four descriptors and bank-relative
channel stores. Its original85:C7A0 caller now has a native child.85:A8E7 was
already reconstructed inside BattleNMI; one shared moving/fixed table core
replaces that internal copy and provides its direct entry contract. A804 is
the newly reconstructed functionality; do not double-count existing NMI work.

16,384 direct/16,384 ABI states cover all166 required instruction states,
with8,192 original table returns per path. All256DB/threeDP/decimal andboth
branches match CPU/fullWRAM/MMIO/ordered accesses;576 semantic handoffs and
6,144 runtime guards pass.8,192 fully native compositions per path,128 original
child-entry unwinds and384 symmetric changed-width handoffs per path pass.
Twelve bounded controls/two CPP APIs and16,384 original NMI reuse states pass.
The NMI reuse proof retains its existing binary-context contract. Additional
synthetic decimal1 case128 fails identically with unchanged legacy NMI; that
diagnostic is retained without a decimal NMI verification or behavior repair.
Sixteen grouped public jobs, ONE full 1,296 jobs and Windows Release pass.

### Complete field object animation request and shared slot queue

83:F559 retains four child calls and both brightness paths. Its existing
Event VM slot queue now uses one shared core; the original negative path
still hands off at83:F564. No original behavior repaired or timing removed.
8,192 direct/8,192 ABI states, all37 non-return states,5,120 original child
returns per path;320 semantic handoffs/6,144 runtime guards PASS.8,192
native compositions per path include5,120 request/6,144 region children.
512 child-entry unwinds/1,536 changed-width returns per path,8,192 actual
Event VM child calls,twelve bounded controls andone C++ API PASS.
Sixteen grouped public jobs, ONE full 1,296 jobs and Windows Release pass.

### Original field item-region callers

8E:B63B retains item-region/facing selection and inventory/notice paths;
8E:B760 retains text service boundaries, animation and input/timer dismissal.
Nineteen child sites preserve exact JSR/JSL frames and changed-width tails.
16,384 direct/16,384 ABI states cover164 required instruction states;
753,856 actual original child returns per path,768 semantic handoffs and
12,288 runtime guards pass.16,384 native chains per path retain184,169
original frame waits and368,338 original NMI executions across both sides.
Item/name/text and unrelated frame grandchildren are controlled boundaries;
their returns are excluded from actual-original-child counts. CPU,fullWRAM,
MMIO,ordered accesses and live stack match.2,432 unwinds and7,296 changed-
width returns per path,21 bounded ROM controls andtwo C++ APIs pass.
Seven grouped public jobs, ONE full 1,303 jobs and Windows Release pass.

### Original object-reward and choice callers

8E:C05F preserves two reward contexts and eleven child sites.8E:BCAD preserves
collection enumeration and eight choices;8E:BD5E searches available choices.
86:9E3B builds a random word and preserves the original multiply/PLP tail.
8,192 direct/8,192 ABI states cover233 instruction states;734,704 actual
original child returns per path,1,152 semantic handoffs and24,576 runtime
guards pass.8,192 native compositions per path reuse twelve services and
retain1,792 original child returns.2,560 unwinds/7,680 changed-width returns
per path,22 bounded original-ROM controls andfour C++ APIs pass.
Presentation8E:C1DE is an explicit controlled boundary, with2,176 returns
excluded from actual-child execution.128 chooser states use one declared
synthetic ROM-data fixture for a dormant branch;ROM instructions/file remain
unchanged. No presentation, frame-wait or NMI execution is claimed here.
Existing random refill/seed reads now follow ROM order explicitly;four random
services match8,192 strict bus-order states andfive bounded ROM controls.
Eight grouped public jobs, ONE full 1,311 jobs and Windows Release pass.

### Original battle party-layout services

85:9255 reuses one predicate with85:9236; no duplicated party test or claim
that the existing parent's predicate was previously interpreted.
85:9326 writes four original command bytes;85:942D/9445 swap members and
85:9460 reverses their original identity/record arrays. Registers, flags,
widths, bank addressing and ordered writes are preserved.
49,152 direct/49,152 ABI states include8,192 existing-parent reuse cases.
47 required instruction states pass.8,192 twelve-service chains per path
perform98,304 native operations;2,048 writable-stack overlap states pass.
30,720 runtime guards/192 semantic handoffs,13 bounded ROM controls and
five C++ interfaces pass. No asynchronous/frame/NMI behavior claimed.
Thirteen grouped public jobs, ONE full 1,324 jobs and Windows Release pass.

### Original special-party sprite-grid preparation

85:97D1 preserves the caller-bank early exit, saved DB, two original WRAM-port
copies,8x7 grid setup, group counts and tile row. Both child sites reuse
verified original services; no display timing or rendering is changed.
8,192 direct/8,192 ABI states cover79 required instructions, with13,106
actual original child returns per path.8,192 fully native chains per path
use6,553 grid and6,553 tile-row calls.256 original child-entry unwinds and
768 symmetric changed-width returns per path pass, including the final
width-independent SEP/PLB tail.6,144 runtime/320 semantic guards,18 bounded
ROM controls andone C++ interface pass. No frame waits or NMI are claimed.
Shared full 1,324 jobs and Windows Release pass.
The cleared1577 byte reuses the catalogued multiply-product storage.

CLI CI fixes were fast-forwarded before checkpointing. The prior full batch
was retained for equivalent formatting/initialization/include changes; all
13 new public jobs and Release were rebuilt after the merge. All 406 C units
pass Clang C11/Wall/Wextra/Wpedantic/Werror; the original 16-bit graphics
offset is explicit and its seven public jobs pass after that clarification.

### Original party-action publication and shared turn services

85:92CE publishes staged action words;85:92FF stages and inserts a turn.
The canonical parent entry is M0X0; both independently proven accumulator
widths preserve saved registers and status. Native child boundaries remain
explicit and retain original call frames and changed-width handoffs.
32,768 direct/32,768 ABI cases across both entry widths;44 required states,
49,152 actual original child returns per path. Native compositions call the
pointer32,768/insertion16,384 times per path.768 unwinds/2,304 changed-width
returns per path,12,288 both-width guards,12 controls andtwo CPP APIs pass.

Existing85:9275,81:C254 and81:C294 reuse one action, insertion andpriority
implementation each.10,240 direct/10,240 production-ABI states cover89
required states with exact CPU/full WRAM/MMIO/ordered bus. Six addressing
controls and8,192 independent fraction/8,192 multiply comparisons pass.
Absolute record fields preserve DB addressing, rather than accidental DP
addressing in the previous helper. Original queue reads remain ordered.
Existing parent behavior is not counted again as newly native functionality.
Proofs use supported D0/DP0 contracts; no NMI or frame-wait claim is made.
Fifteen grouped public jobs, ONE full 1,339 jobs and Windows Release pass.

### Original field player-command leaves

83:C161 D-pad selection and83:C652 party-list search share the existing
native implementations; no duplicate body or whole-game progress claim.
83:C180 resets original secondary-action state and83:C692 reads the selected
item record byte.32,768 direct/32,768 ABI states cover73 required states,
21,504 runtime guards,14 bounded ROM controls andfour CPP APIs pass.
8,192 exact83:C1E3 JSR/RTS windows verify the existing controller's direction
core reuse. CPU/full WRAM/MMIO/ordered bus comparisons preserve original DB,
dynamic/unaligned DP, register and flag behavior. The party-list inner loop
reads the search value once, matching the original CMP-loop bus order.
No original gameplay behavior, interpreter owner, wait or NMI was changed.
Twelve grouped field jobs, ONE full 1,351 jobs and Windows Release pass.

### Original field action setup

83:C2FE/C311/C32D preserve action-actor spawn and leader-facing selection.
83:D3A1/D3F7 share the existing original secondary-script installer.
40,960 direct/40,960 ABI cases cover43 required original instructions and
114,688 original child returns per path. Full-native compositions compare
40,960 cases per path,65536 native child returns andzero interpreted children.
768 unwinds/2304 changed-width returns/256 decimal boundaries per path,
29,952 runtime guards/7,168 semantic guards/25 controls/five CPP APIs pass.
CPU/full WRAM/MMIO/ordered bus comparison retains original DB and DP behavior.
Nine adjacent player-command/action contracts share one public integration
batch. No original gameplay bug, interpreter wait or NMI behavior changed.
Twelve grouped field jobs, ONE full 1,351 jobs and Windows Release pass.

### Original field action callers

Seven complete original contracts connect facing transitions, item-animation
selection, field-tool dispatch, tile-height/object probes and secondary actor
spawning. Existing actor, item-record, map andscript cores remain shared.
Three grouped proofs compare57,344 direct and57,344 runtime-ABI states per
path, with359,524 original nested child returns perpath. Separate full-native
compositions compare57,344 states and234,301 native callback returns perpath;
zero interpreted children. All167 required non-return instructions and35
root child sites are covered. Original CPU/full WRAM/MMIO/ordered bus remain
unchanged. No original gameplay bug fix or asynchronous/NMI execution claim.
Twenty-four grouped jobs, ONE full 1,375 and Windows Release pass.

### Original field probe dispatchers and indexed routes

Nine complete original contracts retain BE33/D8A3 target-table reads before
the original indexed JSR frames. Three lower-height routes share one body;
the four collision routes serve both new entries and existing actor callers.
Actor collision setup is shared with the existing secondary-script caller.
No duplicate collision/prefix implementation or data-table function binding.
73,728 direct/ABI states per path cover 138 required instruction states;
185,368 actual original nested child returns per path. Full-native compositions
compare 73,728 states and 118,145 native callback returns per path, zero
interpreted children. Shared legacy D89E comparison adds 32,768 original states
over four directions, four Direct Pages and five data banks. Original CPU,
WRAM, MMIO and ordered bus behavior preserved. No original bug fix or NMI claim.
Seventeen grouped jobs, ONE full 1,392, two hardened adapter replays and Windows Release pass.

### Original player control services

Eight complete original entries retain joypad/facing lookup, vehicle rejection and
actor updates, header lookup, original frames and width restoration.
One terrain body serves both the transition and existing player controller.
Sprite descriptors and sign-byte handling share the existing actor kernels.
16,384 direct and runtime-ABI states per path cover 92 required instructions
and 15 root child sites; 246,446 actual original child returns per path.
Native composition retains 138,022 callbacks per path without interpreted
children. Legacy terrain prefix adds 8,192 original states; sprite descriptor
adds 20,480 states over all 256 IDs, four widths/Direct Pages and five banks.
The sign-byte core covers all 65,536 accumulators in both index widths.
Two input leaves add 147,456 direct/ABI states, both index widths, eight
original-ROM controls and 24,576 original controller JSR/RTS windows.
Original CPU, WRAM, MMIO and ordered bus preserved. No gameplay or NMI claim.
Four interaction contracts add65,536 states perpath,131 original states,
2,166,053 actual original child returns and493,353 native callbacks perpath.
Nine child sites,ordered actor flags,scene records and facing ties are retained.
An interaction variant is selected;an animation classification is unproven.
Twenty-four grouped jobs, ONE full 1,416 and Windows Release pass.
