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
| `$83:C1B4` | `Lufia2PlayerSlotStandardUpdate` | `identified` | Standard BBF3 child. One 900-frame gameplay capture observed 62 entries; all observed direct child calls had native/AOT bodies. Return paths can preserve or narrow X width, so the consumer ABI is path-dependent. |
| `$83:C7F8` | `Lufia2ActorPrimaryUpdate` | `identified` | Hot actor callback. The same capture observed 3,226 entries; 2,978 reached the `$83:C83B` return and 248 continued through `$83:C83C`. |
| `$83:D508` | `Lufia2ActorSecondaryUpdate` | `identified` | Hot actor callback. The same capture observed 3,648 entries; 2,190 reached the `$83:D599` return and 1,458 continued through `$83:D59A`. Runtime entry M/X was not uniform. |
