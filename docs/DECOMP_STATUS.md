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
| `$83:C7F8` | `Lufia2ActorPrimaryUpdate` | `draft` | Portable front-end reconstructs the known gates, the `$83:C829` actor-flag clear, and `$7F:E3C6+slot` timer update through `$83:C83B`, with continuations at `$83:C808` and `$83:C83C`. Across 64 supplied C7F8 snapshots the reconstructed prefix matched flow and bus effects with zero deviations. |
| `$83:D508` | `Lufia2ActorSecondaryUpdate` | `draft` | Portable front-end reconstructs the dispatcher, `$7F:E48E+slot` counter and `$066A+slot` toggle through `$83:D599`, preserving both observed X widths and exposing continuations at `$83:D512`, `$83:D550`, `$83:D585` and `$83:D59A`. Supplied sampler-v3 snapshots matched with zero deviations. |
