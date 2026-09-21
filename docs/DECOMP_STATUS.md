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
| `$83:BBF3` | `Lufia2PlayerSlotSpecialUpdate` | `draft` | Five gates and two child branches reconstructed; the recomp integration branch can opt into this draft for owner runtime validation. |
