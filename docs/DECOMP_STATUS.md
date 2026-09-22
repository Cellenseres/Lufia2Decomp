# Decompilation status

| Status | Meaning |
| --- | --- |
| `identified` | Boundary/address known; no semantic implementation yet. |
| `draft` | Readable semantic source exists; equivalence is not yet proven. |
| `verified` | The required decomp verification passed; consumers may select it. |
| `disabled` | Retained for reference but deliberately excluded. |

## Current functions

| Address | Symbol | Status | Notes |
| --- | --- | --- | --- |
| `$83:BB93` | `Lufia2UpdateActorSlots` | `draft` | 40-slot actor traversal, WRAM bookkeeping and child-call decisions reconstructed; exact CPU/return ABI remains to be integrated. |
| `$83:BBF3` | `Lufia2PlayerSlotSpecialUpdate` | `verified` | Original-ROM differential verification passed 65,792 semantic cases and 12,288 consumer bridge/stack boundary cases with zero mismatches. |
| `$83:C1B4` | `Lufia2PlayerSlotStandardUpdate` | `identified` | Standard BBF3 child. Map captures observed 204 entries total: 56 returned at `$83:C1E2`, 148 reached the longer tail, including two rare `$83:C1FC` paths. Entry is M1X0; exits can preserve or narrow X, so the consumer ABI is path-dependent. |
| `$83:C7F8` | `Lufia2ActorPrimaryUpdate` | `identified` | Hot map-actor callback. Three map captures observed 25,674 entries; 24,200 reached `$83:C83B` and 1,474 continued through `$83:C83C`. New sampler-v2 evidence confirms `$00A7` is the BB93 slot index (0..39), not an 8-byte actor offset. |
| `$83:D508` | `Lufia2ActorSecondaryUpdate` | `identified` | Hot map-actor callback. Three map captures observed 27,848 entries; 20,785 reached `$83:D599` and 7,063 continued through `$83:D59A`. Every observed M1X0 entry count exactly matches the preceding C7F8 `$83:C83C` count, tying its path-dependent X width to the primary callback. |
