# Gameplay capture evidence

The decompilation uses owner-run observational captures from the reference
interpreter path to prioritize hot code and constrain semantic reconstruction.
The capture files themselves are not distributed in this repository.

## Current corpus

| Scene | Sampler | Frames | LLE instructions | $83:C7F8 | $83:D508 | $83:C1B4 |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| Earlier mixed map capture | v1 | 900 | 13,420,426 | 3,226 | 3,648 | 62 |
| Intro | v2 | 900 | 15,162,106 | 0 | 0 | 0 |
| Save select | v2 | 595 | 8,153,147 | 0 | 0 | 0 |
| City and interiors | v2 | 900 | 14,781,839 | 7,526 | 8,449 | 142 |
| Battle and world map | v2 | 900 | 12,043,139 | 0 | 0 | 0 |
| Dungeon with many enemy actors | v2 | 900 | 14,345,787 | 14,922 | 15,751 | 0 |
| City and interiors | v3 | 900 | 14,852,306 | 7,136 | 8,254 | 254 |
| Dungeon with many enemy actors | v3 | 900 | 14,238,422 | 15,966 | 16,853 | 0 |
| Follow-up map capture A | v3 | 900 | 14,665,519 | 6,831 | 7,688 | 160 |
| Follow-up map capture B | v3 | 900 | 14,067,222 | 13,428 | 14,174 | 0 |

The seven map captures contain 69,035 C7F8 entries, 74,817 D508 entries and
618 C1B4 entries. Intro, save-select, battle and world-map captures did not
enter these callbacks during their recorded windows.

Sampler v3 is the first corpus with both actor-slot and temporal diversity.
C7F8/D508 snapshots are spaced by roughly 30 frames while rotating across
recently observed slots instead of exhausting the per-target budget in the
first frame.

## $83:C7F8

Across the seven map captures:

- 64,863 / 69,035 calls (93.96%) reached the RTS at $83:C83B.
- 4,172 / 69,035 calls (6.04%) continued through $83:C83C.
- Every observed entry was M1X1.
- Sampler evidence confirms $00A7 is the BB93 slot index (0..39), not an
  8-byte actor offset.

The two v3 scenes expose two different dominant front-end shapes:

### City/interiors v3

- 7,136 total entries.
- 2,053 calls returned immediately after the actor-flags bit-$80 test.
- The remaining 5,083 calls all reached $83:C82E; none reached $83:C815.
  The recorded states have global $09A7 bit 0 clear, so the $83:C813 branch
  sends the common city path directly to the per-slot timer.
- 4,222 of those timer-path calls returned at $83:C83B and 861 continued at
  $83:C83C.
- The captured C82E path includes real writes to $7F:E3C6+slot when a nonzero
  timer is decremented.

### Dungeon v3

- 15,966 total entries.
- 1,294 calls returned at the initial flags-$80 gate.
- 14,672 reached the final $83:C827 branch.
- 666 continued through $83:C829..$83:C82D. A follow-up capture recorded a
  complete call through this rare block and established its semantics as
  `AND #$BF`, `STA $0622,X`, then the common $83:C82E timer path.
- 14,006 / 15,966 calls (87.72%) followed the exact final taken-BEQ,
  write-free path covered by the consumer's differential fast-path contract.
- The 16 temporal snapshots cover slots 8..23; 13 are the full write-free
  path and three are the immediate flags-$80 return.

The portable front-end now includes the $83:C829 actor-flag clear as well as
the known gates and $83:C82E timer path. The $83:C83C dispatcher prefix has
also been reconstructed through its indirect jump at $83:C864: it clears the
per-actor flags, preserves the caller DB on the stack, loads the actor's
24-bit script cursor from $7F:E506..E508, and dispatches the current bytecode
through the ROM-backed table at $83:D467. The remaining primary boundary
before that VM is the still-unobserved $83:C808 block; individual bytecode
handler bodies remain incremental decompilation targets.

Across the supplied dispatcher snapshots, every reconstructed C83C data,
stack and jump-table bus access and every selected handler target matched the
reference trace.

## $83:D508

Across the seven map captures:

- 57,580 / 74,817 calls (76.96%) reached the RTS at $83:D599.
- 17,237 / 74,817 calls (23.04%) continued through $83:D59A.
- Entry X width is path dependent. M1X0 entries track the preceding C7F8
  $83:C83C continuation; M1X1 is the normal BB93 path.
- BB93 initializes $00A8 to zero before the callbacks, so a 16-bit LDX $A7
  still produces the same numerical slot index while changing bus width and
  ABI state.

A stronger structural invariant now holds in every captured map scene:

- hits($83:D508) = hits($83:D540) + hits($83:D59A)
- hits($83:D540) = hits($83:D599)

The branch at $83:D53E tests actor flags bit $80. In the complete observed
corpus that gate cleanly partitions dispatcher entries from the return-side
path: every call reaching $83:D540 eventually reached the $83:D599 RTS, while
every observed $83:D59A entry came from the opposite side of that top-level
split.

Scene dependence remains substantial:

- City/interiors v3: 5,195 / 8,254 (62.94%) returned at $83:D599.
- Dungeon v3: 14,768 / 16,853 (87.63%) returned at $83:D599.

The $83:D59A continuation is now reconstructed through handler selection.
It performs the slot-zero global-state preamble, preserves the caller DB,
loads the actor's secondary script cursor from $7F:E3EE..E3F0, dispatches on
the opcode high nibble through $83:DF17, and resolves the D/E/F families
through the low-nibble tables at $83:DF77/$83:DF57/$83:DF37. The table words
remain ROM-backed reads rather than copied data. Supplied D59A dispatcher
snapshots matched data, stack and table bus effects and selected handler PCs
exactly; the handler bodies remain the next reconstruction layer.

## $83:C1B4

The combined map corpus observed 618 entries:

- 245 / 618 (39.64%) reached the early RTS at $83:C1E2.
- 373 / 618 (60.36%) continued into the longer path.
- 52 of the long-path calls traversed $83:C1FC..$83:C217 before rejoining the
  common tail.
- Sampler v3 captured three complete instances of that C1FC path, including
  entry/exit registers, WRAM before/after and bus writes.
- $83:C1C5..$83:C1E1 has still never executed in the available corpus.

All 618 observed calls reached $83:C1C0 and then branched from $83:C1C3 to
$83:C1E3, so the missing block is tied to a state not represented by current
gameplay samples rather than ordinary city movement.

The common prefix, early return, common long tail and C1FC side path now have
strong runtime evidence. The C1C5..C1E1 block remains the only major uncovered
piece before a complete source reconstruction can be claimed.


## $80:8E9D resource decompression candidate

External LEdit research identifies the code beginning at $80:8EAF as the
Lufia II resource-file decompressor. The generated recompilation groups that
code inside the function beginning at $80:8E9D; the current program manifest
marks its M0X0 and M1X0 variants AOT-eligible with 232 analyzed instructions.

This identification lines up with the gameplay profile. Across the four
current sampler-v3 sessions, hot PCs inside this function accumulated:

- $80:8EFA: 61,761 hits.
- $80:8EFC: 61,761 hits.
- $80:8F66: 55,499 hits.

LEdit's English-ROM implementation places the 3-byte resource pointer table at
PC offset $138000 and uses 0x2A8 entries. Owner-side validation against the
supported headerless US ROM found all 680 table entries in range and
successfully decompressed all 680 streams to their declared output lengths
using the documented algorithm.

This validates the resource format and makes $80:8E9D a strong semantic
decompilation target, but it does not yet prove a replacement ABI for the
original routine. Because the function is already statically AOT-eligible,
plain decompilation is primarily useful for readability and verification.
A later, explicitly separate patch may have a larger performance payoff by
using a host-side decoder and/or decompressed-resource cache instead of
executing the original byte/backreference loops repeatedly.
