# Gameplay capture evidence

The decompilation uses owner-run observational captures from the reference
interpreter path to prioritize hot code and constrain semantic reconstruction.
The capture files themselves are not distributed in this repository.

## Current corpus

| Scene | Frames | LLE instructions | $83:C7F8 | $83:D508 | $83:C1B4 |
| --- | ---: | ---: | ---: | ---: | ---: |
| Earlier mixed map capture | 900 | 13,420,426 | 3,226 | 3,648 | 62 |
| Intro | 900 | 15,162,106 | 0 | 0 | 0 |
| Save select | 595 | 8,153,147 | 0 | 0 | 0 |
| City and interiors | 900 | 14,781,839 | 7,526 | 8,449 | 142 |
| Battle and world map | 900 | 12,043,139 | 0 | 0 | 0 |
| Dungeon with many enemy actors | 900 | 14,345,787 | 14,922 | 15,751 | 0 |

The three map captures therefore contain 25,674 C7F8 entries and 27,848 D508
entries. Intro, save-select, battle and world-map captures did not enter these
callbacks during their recorded windows.

## $83:C7F8

Across the three map captures:

- 24,200 / 25,674 calls (94.26%) reached the RTS at $83:C83B.
- 1,474 / 25,674 calls (5.74%) continued through $83:C83C.
- In the dungeon capture, 13,627 / 14,922 calls (91.32%) traversed the exact
  19-opcode write-free prefix already covered by the consumer's differential
  fast-path contract.
- Sampler v2 captured real BB93 slot indices across many actors: city/interior
  snapshots included slots 1,2,3,8,9,10,12 and the dungeon snapshots covered
  8..23. This confirms $00A7 is a slot index, not an 8-byte actor offset.

The consumer fast path can therefore safely test BB93's nonzero slot range
1..39 while retaining all existing state, bus, cycle and interrupt guards.

## $83:D508

Across the three map captures:

- 20,785 / 27,848 calls (74.64%) reached the RTS at $83:D599.
- 7,063 / 27,848 calls (25.36%) continued through $83:D59A.
- Return rate is highly scene dependent: 58.15% in city/interiors versus
  86.86% in the dungeon capture.
- Entry X width is path dependent. The counts of M1X0 entries are exactly the
  preceding C7F8 continuation counts in every map capture:
  248, 522 and 704 respectively.
- BB93 initializes $00A8 to zero before the callbacks, so a 16-bit LDX $A7
  still produces the same numerical slot index while changing bus width and
  ABI state.

This width relationship must be preserved by a future portable bridge rather
than represented as one fixed entry/exit M/X state.

## $83:C1B4

The combined map corpus observed 204 entries:

- 56 reached the early RTS at $83:C1E2.
- 148 continued into the longer path.
- Two earlier calls traversed the rare $83:C1FC..$83:C217 branch; sampler
  snapshots did not land on those two executions.
- The $83:C1C5..$83:C1E1 branch has not yet executed in the available corpus.

The common long path and early return are well observed, but the missing branch
coverage is still insufficient to mark the function draft or verified.
