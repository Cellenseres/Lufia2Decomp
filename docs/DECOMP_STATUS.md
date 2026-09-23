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
| `$80:8E9D` | `Lufia2DecompressResourceFile` | `identified` | 232-instruction generated function containing the resource decompressor identified by LEdit from `$80:8EAF`. The current generated path is AOT-eligible; exact portable semantics and a ROM differential boundary are not implemented yet. External research was independently checked against all 680 resource streams in the supported US ROM. |
| `$83:BB93` | `Lufia2UpdateActorSlots` | `draft` | 40-slot actor traversal, WRAM bookkeeping and child-call decisions reconstructed; exact CPU/return ABI remains to be integrated. |
| `$83:BBF3` | `Lufia2PlayerSlotSpecialUpdate` | `verified` | Original-ROM differential verification passed 65,792 semantic cases and 12,288 consumer bridge/stack boundary cases with zero mismatches. |
| `$83:C1B4` | `Lufia2PlayerSlotStandardUpdate` | `identified` | Standard BBF3 child. The current map corpus observed 618 entries: 245 returned at `$83:C1E2`, 373 reached the longer tail, including 52 traversals of `$83:C1FC..C217`. Entry is M1X0; exits can preserve or narrow X, so the consumer ABI is path-dependent. |
| `$83:C7F8` | `Lufia2ActorPrimaryUpdate` | `draft` | Portable front-end reconstructs the gates/timer path and `$83:C83C..C866` VM dispatch. Native handler semantics now include the direct D350 action cluster at `$83:C867/$83:C86B/$83:C86F/$83:C873/$83:C877/$83:C87C/$83:C880/$83:C884/$83:C888`, `$83:C8C7`, `$83:C891` (via D3F7), `$83:C8AF/$83:C8D4` (via `$80:8299`), `$83:C8EE`, `$83:C8FC/$83:C90A`, `$83:CBB7` (via F9D4), `$83:CBE1`, `$83:CC1B/$83:CC2E`, `$83:CC41/$83:CC63` (via D350), `$83:CC85/$83:CCA3`, full `$83:D14D -> D350 -> C8C7`, `$83:D2B4/$83:D2BD`, and `$83:D2C4/$83:D2D5`. Absolute-indexed reads carry into the next bank, matching the CPU. The portable CPU state now carries V/D/I so PHP/PLP round-trip exactly; ADC/SBC are binary-mode only. Full handler coverage remains incomplete. |
| `$83:D350` | `Lufia2ActorPrimaryActionCore` | `draft` | Common primary-VM action/movement helper. The full pre-RTL body is now reconstructed, including the four `$83:D3B7/$83:D3C5/$83:D3D7/$83:D3E5` boundary helpers, `$83:FB12` coordinate stepping, `$83:F9D4/$83:F9F7` map-cell resolution, `$83:FB71` map probing and the `$83:D3F7` secondary-script installer. Differential verification is required before promotion. |
| `$83:D508` | `Lufia2ActorSecondaryUpdate` | `draft` | Portable front-end reconstructs the known return-side gates through `$83:D599`; `$83:D59A..D5D3` plus the D/E/F low-nibble subdispatchers now reconstruct script-bank/pointer setup, stack/DB state and handler selection. Remaining continuations and handler bodies keep the whole function at `draft`. |
