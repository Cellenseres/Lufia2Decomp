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

The five map captures contain 48,776 C7F8 entries, 52,955 D508 entries and
458 C1B4 entries. Intro, save-select, battle and world-map captures did not
enter these callbacks during their recorded windows.

Sampler v3 is the first corpus with both actor-slot and temporal diversity.
C7F8/D508 snapshots are spaced by roughly 30 frames while rotating across
recently observed slots instead of exhausting the per-target budget in the
first frame.

## $83:C7F8

Across the five map captures:

- 45,829 / 48,776 calls (93.96%) reached the RTS at $83:C83B.
- 2,947 / 48,776 calls (6.04%) continued through $83:C83C.
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
- 666 continued through the still-unreconstructed $83:C829..$83:C82D block.
- 14,006 / 15,966 calls (87.72%) followed the exact final taken-BEQ,
  write-free path covered by the consumer's differential fast-path contract.
- The 16 temporal snapshots cover slots 8..23; 13 are the full write-free
  path and three are the immediate flags-$80 return.

This makes the next semantic boundary unusually clear. A portable front-end
can model the known gates and the $83:C82E timer path, returning explicit
continuations for the unobserved $83:C808 block, the rare $83:C829 block and
the established $83:C83C continuation.

## $83:D508

Across the five map captures:

- 40,748 / 52,955 calls (76.95%) reached the RTS at $83:D599.
- 12,207 / 52,955 calls (23.05%) continued through $83:D59A.
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

The return-side path itself has several internal branches and occasional
writes, but this top-level split is suitable for an incremental semantic
reconstruction with a D59A continuation boundary.

## $83:C1B4

The combined map corpus observed 458 entries:

- 175 / 458 (38.21%) reached the early RTS at $83:C1E2.
- 283 / 458 (61.79%) continued into the longer path.
- 51 of the long-path calls traversed $83:C1FC..$83:C217 before rejoining the
  common tail.
- Sampler v3 captured three complete instances of that C1FC path, including
  entry/exit registers, WRAM before/after and bus writes.
- $83:C1C5..$83:C1E1 has still never executed in the available corpus.

All 458 observed calls reached $83:C1C0 and then branched from $83:C1C3 to
$83:C1E3, so the missing block is tied to a state not represented by current
gameplay samples rather than ordinary city movement.

The common prefix, early return, common long tail and C1FC side path now have
strong runtime evidence. The C1C5..C1E1 block remains the only major uncovered
piece before a complete source reconstruction can be claimed.
