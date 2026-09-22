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
| `$83:C7F8` | `Lufia2ActorPrimaryUpdate` | `draft` | Portable front-end reconstructs the known gates, `$83:C829` flag clear and timer path; `$83:C83C..C866` now reconstructs script-bank/pointer setup, stack/DB state and indirect handler dispatch. Supplied dispatcher snapshots matched data/stack/table bus effects and handler targets exactly; full handler coverage is still incomplete. |
| `$83:D508` | `Lufia2ActorSecondaryUpdate` | `draft` | Portable front-end reconstructs the known return-side gates through `$83:D599`; `$83:D59A..D5D3` plus the D/E/F low-nibble subdispatchers now reconstruct script-bank/pointer setup, stack/DB state and handler selection. Remaining continuations and handler bodies keep the whole function at `draft`. |
