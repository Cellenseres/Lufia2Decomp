# Decompilation status

| Status | Meaning |
| --- | --- |
| `identified` | Boundary/address known; no semantic implementation yet. |
| `draft` | Readable semantic source exists; equivalence is not yet proven. |
| `verified` | The required decomp verification passed; consumers may select it. |
| `disabled` | Retained for reference but deliberately excluded. |

`verified` describes the reconstruction, not runtime use: whether a verified
function replaces the original at runtime is decided by the consumer's
`recomp/decomp_bindings.toml`.

## Function index

Generated from `metadata/functions.toml` by `scripts/metadata_index.py`; edit
the metadata, not this table. The sections after the index are the research
and verification notes for each task and may describe a status that a later
task changed; the index and the metadata are authoritative.

<!-- metadata-counts:begin (scripts/metadata_index.py) -->
175 functions in `metadata/functions.toml`: 175 verified, 0 draft, 0 identified, 0 disabled.
<!-- metadata-counts:end -->

<!-- metadata-index:begin (scripts/metadata_index.py) -->
| Address | Symbol | Status | Source |
| --- | --- | --- | --- |
| `$80:8299` | `Lufia2RandomScale` | verified | `src/system/random.c` |
| `$80:82C7` | `Lufia2RandomByte` | verified | `src/system/random.c` |
| `$80:8378` | `Lufia2Divide16` | verified | `src/system/math.c` |
| `$80:86C1` | `Lufia2ScreenFade` | verified | `src/system/screen.c` |
| `$80:8878` | `Lufia2MenuDrawString` | verified | `src/menu/menu_string.c` |
| `$80:8E9D` | `Lufia2DecompressResource` | verified | `src/system/decompress.c` |
| `$80:92A4` | `Lufia2IntroNmi` | verified | `src/title/title.c` |
| `$80:9C72` | `Lufia2FieldEventTick` | verified | `src/field/field_update.c` |
| `$80:9CB8` | `Lufia2TextEngineStep` | verified | `src/text/text_engine.c` |
| `$80:C23D` | `Lufia2TextPrepareWindow` | verified | `src/text/text_window.c` |
| `$80:C305` | `Lufia2TextBuildWindow` | verified | `src/text/text_window.c` |
| `$80:C56E` | `Lufia2TextQueueWindowRow` | verified | `src/text/text_window.c` |
| `$80:C652` | `Lufia2TextMeasure` | verified | `src/text/text_layout.c` |
| `$80:C784` | `Lufia2TextClearGlyphBuffer` | verified | `src/text/text_window.c` |
| `$80:CBAE` | `Lufia2FieldEventTimerTick` | verified | `src/field/field_event_script.c` |
| `$80:ED9C` | `Lufia2FieldBuildAttributes` | verified | `src/field/field_attributes.c` |
| `$81:B264` | `Lufia2BattleActiveMask` | verified | `src/battle/battle_util.c` |
| `$81:B2B5` | `Lufia2BattleTargetRecord` | verified | `src/battle/battle_util.c` |
| `$81:B2DB` | `Lufia2BattleTargetSlot` | verified | `src/battle/battle_util.c` |
| `$81:B444` | `Lufia2BattlePaletteFade` | verified | `src/battle/battle_util.c` |
| `$81:B48B` | `Lufia2BattleFadeColor` | verified | `src/battle/battle_util.c` |
| `$81:B505` | `Lufia2BattleBlend` | verified | `src/battle/battle_util.c` |
| `$81:B54A` | `Lufia2ColorToGray` | verified | `src/battle/battle_util.c` |
| `$81:B5A3` | `Lufia2BattleHideOam` | verified | `src/battle/battle_util.c` |
| `$81:B974` | `Lufia2BattleLoadPalette` | verified | `src/battle/battle_util.c` |
| `$81:B9AF` | `Lufia2BattleCommitPalettes` | verified | `src/battle/battle_util.c` |
| `$81:BAE8` | `Lufia2BattlePortraits` | verified | `src/battle/battle_portrait.c` |
| `$81:BAFB` | `Lufia2BattlePortrait` | verified | `src/battle/battle_portrait.c` |
| `$81:BB75` | `Lufia2BattlePortraitUpload` | verified | `src/battle/battle_portrait.c` |
| `$81:BD47` | `Lufia2BattleSpriteBlockFar` | verified | `src/battle/battle_util.c` |
| `$81:BD4B` | `Lufia2BattleSpriteBlock` | verified | `src/battle/battle_util.c` |
| `$81:BE54` | `Lufia2BattleTileBlockFar` | verified | `src/battle/battle_util.c` |
| `$81:BE58` | `Lufia2BattleTileBlock` | verified | `src/battle/battle_util.c` |
| `$81:C129` | `Lufia2BattleIpSkills` | verified | `src/battle/battle_ip.c` |
| `$81:C2C0` | `Lufia2BattleCopyC2C0` | verified | `src/battle/battle_util.c` |
| `$81:C2D0` | `Lufia2BattleClear2000` | verified | `src/battle/battle_util.c` |
| `$81:C2E3` | `Lufia2BattleFill2800` | verified | `src/battle/battle_util.c` |
| `$81:C2FB` | `Lufia2BattleClear3000` | verified | `src/battle/battle_util.c` |
| `$81:C30E` | `Lufia2BattleClear3800` | verified | `src/battle/battle_util.c` |
| `$81:C35F` | `Lufia2BattlePopups` | verified | `src/battle/battle_popup.c` |
| `$81:C5CF` | `Lufia2BattleTargetPointer` | verified | `src/battle/battle_util.c` |
| `$81:DFA2` | `Lufia2BattleListRows` | verified | `src/battle/battle_ip.c` |
| `$81:E3AE` | `Lufia2BattleWindowE3AE` | verified | `src/battle/battle_frame_rows.c` |
| `$81:E3CD` | `Lufia2BattleWindowE3CD` | verified | `src/battle/battle_frame_rows.c` |
| `$81:E3EC` | `Lufia2BattleTileWindow` | verified | `src/battle/battle_frame_rows.c` |
| `$81:E405` | `Lufia2BattleTileFrame` | verified | `src/battle/battle_frame_rows.c` |
| `$81:E479` | `Lufia2BattleFrameTop` | verified | `src/battle/battle_frame_rows.c` |
| `$81:E4AD` | `Lufia2BattleFrameSides` | verified | `src/battle/battle_frame_rows.c` |
| `$81:E503` | `Lufia2BattleWindow` | verified | `src/battle/battle_frame_rows.c` |
| `$81:E542` | `Lufia2BattleFrameRow` | verified | `src/battle/battle_frame_rows.c` |
| `$81:E570` | `Lufia2BattleFrameEnds` | verified | `src/battle/battle_frame_rows.c` |
| `$81:E593` | `Lufia2BattleGaugePanel` | verified | `src/battle/battle_frame_rows.c` |
| `$81:E5C1` | `Lufia2BattleGaugeBlock` | verified | `src/battle/battle_frame_rows.c` |
| `$81:E604` | `Lufia2BattleGaugeColumn` | verified | `src/battle/battle_frame_rows.c` |
| `$81:E7D2` | `Lufia2BattleFillRect` | verified | `src/battle/battle_frame_rows.c` |
| `$81:E808` | `Lufia2DecimalDigits3` | verified | `src/system/math.c` |
| `$81:E835` | `Lufia2BattleGlyph` | verified | `src/battle/battle_ip.c` |
| `$81:EB34` | `Lufia2BattlePaletteCopy` | verified | `src/battle/battle_util.c` |
| `$81:EB62` | `Lufia2BattlePaletteSplit` | verified | `src/battle/battle_util.c` |
| `$81:EC41` | `Lufia2BattleClearF000` | verified | `src/battle/battle_util.c` |
| `$81:ED8E` | `Lufia2PartyUnpackMember` | verified | `src/party/stats.c` |
| `$81:EE94` | `Lufia2PartyUnpackMemberBare` | verified | `src/party/stats.c` |
| `$81:F057` | `Lufia2InventoryCount` | verified | `src/item/inventory.c` |
| `$81:F0A2` | `Lufia2InventoryAdd` | verified | `src/item/inventory.c` |
| `$81:F194` | `Lufia2ItemRecordByte` | verified | `src/item/item_records.c` |
| `$81:F1C5` | `Lufia2LoadItemRecord` | verified | `src/item/item_records.c` |
| `$81:F291` | `Lufia2ItemTextPointer` | verified | `src/item/item_records.c` |
| `$81:F2A9` | `Lufia2ItemNameTrimmed` | verified | `src/item/item_records.c` |
| `$81:F3F4` | `Lufia2SpellRecordByteC` | verified | `src/item/item_records.c` |
| `$81:F404` | `Lufia2SpellRecordByte8` | verified | `src/item/item_records.c` |
| `$81:F414` | `Lufia2LoadSpellRecord` | verified | `src/item/item_records.c` |
| `$81:F446` | `Lufia2SpellTextPointer` | verified | `src/item/item_records.c` |
| `$81:F4D5` | `Lufia2PartyDerivedStats` | verified | `src/party/stats.c` |
| `$81:F4E9` | `Lufia2PartyStatTotalsFar` | verified | `src/party/stats.c` |
| `$81:F4ED` | `Lufia2PartyStatTotals` | verified | `src/party/stats.c` |
| `$81:F5ED` | `Lufia2PartyRestore11` | verified | `src/party/party_records.c` |
| `$81:F60B` | `Lufia2PartyRestore13` | verified | `src/party/party_records.c` |
| `$81:F789` | `Lufia2PartyPointersFar` | verified | `src/party/party_records.c` |
| `$81:F78D` | `Lufia2PartyPointers` | verified | `src/party/party_records.c` |
| `$81:F7BD` | `Lufia2BattleTable9EBA` | verified | `src/party/party_records.c` |
| `$81:F87F` | `Lufia2PartyBaseStats` | verified | `src/party/stats.c` |
| `$81:F979` | `Lufia2PartyLevelUpCheck` | verified | `src/party/level_up.c` |
| `$81:F9E9` | `Lufia2PartyExperienceForLevel` | verified | `src/party/experience.c` |
| `$81:FB79` | `Lufia2CharacterSpriteByte` | verified | `src/battle/battle_character.c` |
| `$81:FBA2` | `Lufia2SpriteSizePacked` | verified | `src/battle/battle_character.c` |
| `$81:FBDB` | `Lufia2CharacterSpriteBox` | verified | `src/battle/battle_character.c` |
| `$81:FC0B` | `Lufia2PartyNewRecord` | verified | `src/party/level_up.c` |
| `$81:FCE2` | `Lufia2CharacterSpritePointer` | verified | `src/battle/battle_character.c` |
| `$82:810E` | `Lufia2MenuDrawWindow` | verified | `src/menu/menu_window.c` |
| `$82:8B4B` | `Lufia2MenuButtons` | verified | `src/menu/menu.c` |
| `$82:9313` | `Lufia2MenuWindowRequest` | verified | `src/menu/menu.c` |
| `$82:939C` | `Lufia2MenuNmi` | verified | `src/menu/menu.c` |
| `$82:950E` | `Lufia2MenuMemberStatus` | verified | `src/menu/menu_screen.c` |
| `$82:9CB2` | `Lufia2MenuWarpList` | verified | `src/menu/menu_screen.c` |
| `$82:9F6F` | `Lufia2MenuEquipCommands` | verified | `src/menu/menu_screen.c` |
| `$82:A2E3` | `Lufia2MenuCapsuleScreen` | verified | `src/menu/menu_screen.c` |
| `$82:A918` | `Lufia2MenuListCursor` | verified | `src/menu/menu_screen.c` |
| `$82:ACDB` | `Lufia2MenuListRow` | verified | `src/menu/menu_screen.c` |
| `$82:B2C5` | `Lufia2MenuEquipUpgrade` | verified | `src/menu/menu_screen.c` |
| `$82:C261` | `Lufia2CapsuleLoadStats` | verified | `src/party/capsule.c` |
| `$82:C2FD` | `Lufia2CapsuleReset` | verified | `src/party/capsule.c` |
| `$82:C352` | `Lufia2CapsuleSetAll` | verified | `src/party/capsule.c` |
| `$82:C515` | `Lufia2CapsuleSetForms` | verified | `src/party/capsule.c` |
| `$82:C627` | `Lufia2MenuCursorBlink` | verified | `src/menu/menu.c` |
| `$82:CD1F` | `Lufia2CapsuleTryLearn` | verified | `src/party/capsule.c` |
| `$82:CD83` | `Lufia2CapsuleLevelUp` | verified | `src/party/capsule.c` |
| `$82:CE23` | `Lufia2CapsuleExperienceRange` | verified | `src/party/capsule.c` |
| `$82:D07B` | `Lufia2MenuCapsuleStatus` | verified | `src/menu/menu_screen.c` |
| `$82:D721` | `Lufia2MenuShopWindows` | verified | `src/menu/menu_screen.c` |
| `$82:D749` | `Lufia2MenuShopParty` | verified | `src/menu/menu_screen.c` |
| `$82:DCC1` | `Lufia2MenuShopRows` | verified | `src/menu/menu_screen.c` |
| `$82:DCF4` | `Lufia2MenuShopRow` | verified | `src/menu/menu_screen.c` |
| `$82:E297` | `Lufia2MenuShopSetup` | verified | `src/menu/menu_screen.c` |
| `$82:E49E` | `Lufia2MenuShopTitle` | verified | `src/menu/menu_screen.c` |
| `$82:E5E1` | `Lufia2MenuShopCompare` | verified | `src/menu/menu_screen.c` |
| `$82:E746` | `Lufia2TitleStateDispatch` | verified | `src/title/title.c` |
| `$82:EFC5` | `Lufia2MenuSavedWindow` | verified | `src/menu/menu_screen.c` |
| `$82:F0A2` | `Lufia2MenuNameEntryWindows` | verified | `src/menu/menu_screen.c` |
| `$82:FB1F` | `Lufia2ItemPossessionCount` | verified | `src/item/inventory.c` |
| `$83:80CD` | `Lufia2FieldIdleTest` | verified | `src/field/field_update.c` |
| `$83:8103` | `Lufia2FieldStatusRequests` | verified | `src/field/field_update.c` |
| `$83:81C6` | `Lufia2FieldTriggerUpdate` | verified | `src/field/field_triggers.c` |
| `$83:83A0` | `Lufia2FieldMenuRequest` | verified | `src/field/field_update.c` |
| `$83:85DC` | `Lufia2FieldReloadSetup` | verified | `src/field/field_update.c` |
| `$83:867B` | `Lufia2FieldTakeButtons` | verified | `src/field/field_update.c` |
| `$83:8682` | `Lufia2FieldAnimationTicks` | verified | `src/field/field_update.c` |
| `$83:9E31` | `Lufia2AncientCaveGenerateFloor` | verified | `src/cave/ancient_cave.c` |
| `$83:9FA9` | `Lufia2FieldNmiUploads` | verified | `src/field/field_nmi.c` |
| `$83:A21A` | `Lufia2FieldActorSprites` | verified | `src/field/field_sprites.c` |
| `$83:A746` | `Lufia2ActorSyncFinePosition` | verified | `src/actor/actor_movement.c` |
| `$83:A9BA` | `Lufia2ActorLoadSprite` | verified | `src/actor/actor_sprites.c` |
| `$83:AEB5` | `Lufia2FieldColourEffects` | verified | `src/field/field_effects.c` |
| `$83:B66E` | `Lufia2FieldStairRects` | verified | `src/field/field_triggers.c` |
| `$83:B711` | `Lufia2FieldEventRects` | verified | `src/field/field_triggers.c` |
| `$83:B747` | `Lufia2FieldAreaRects` | verified | `src/field/field_triggers.c` |
| `$83:BB93` | `Lufia2UpdateActorSlots` | verified | `src/actor/actor_slots.c` |
| `$83:BBF3` | `Lufia2PlayerSlotSpecialUpdate` | verified | `src/actor/player_update.c` |
| `$83:C1B4` | `Lufia2PlayerSlotStandardUpdate` | verified | `src/actor/player_controller.c` |
| `$83:C7F8` | `Lufia2ActorPrimaryUpdate` | verified | `src/actor/actor_primary.c` |
| `$83:C947` | `Lufia2ActorPrimaryReset` | verified | `src/actor/actor_primary.c` |
| `$83:CA68` | `Lufia2ActorBlockedEvent` | verified | `src/actor/actor_primary.c` |
| `$83:CB65` | `Lufia2ActorClearSlotLinks` | verified | `src/actor/actor_primary.c` |
| `$83:D350` | `Lufia2ActorPrimaryActionCore` | verified | `src/actor/actor_action.c` |
| `$83:D416` | `Lufia2ActorLoadPrimaryScript` | verified | `src/actor/actor_primary.c` |
| `$83:D508` | `Lufia2ActorSecondaryUpdate` | verified | `src/actor/actor_secondary.c` |
| `$83:E03E` | `Lufia2ObjectSlotsUpdate` | verified | `src/actor/object_vm.c` |
| `$83:F9D4` | `Lufia2ActorResolveMapCellOffset` | verified | `src/actor/actor_movement.c` |
| `$83:FA3F` | `Lufia2ActorMarkMapOccupancy` | verified | `src/actor/actor_movement.c` |
| `$83:FA81` | `Lufia2ActorMoveFinePosition` | verified | `src/actor/actor_movement.c` |
| `$83:FACB` | `Lufia2ActorAddDisplayOffset` | verified | `src/actor/actor_movement.c` |
| `$83:FB12` | `Lufia2ActorMovementStep` | verified | `src/actor/actor_movement.c` |
| `$83:FB71` | `Lufia2ActorReadMapCellValue` | verified | `src/actor/actor_movement.c` |
| `$84:8193` | `Lufia2SpriteGraphicsUpload` | verified | `src/actor/actor_sprites.c` |
| `$84:8766` | `Lufia2QueueDeferredSound` | verified | `src/system/sound.c` |
| `$85:8A2F` | `Lufia2BattleSprites` | verified | `src/battle/battle_frame.c` |
| `$85:8DC5` | `Lufia2BattleNmiUploads` | verified | `src/battle/battle_nmi.c` |
| `$85:B452` | `Lufia2BattleScript` | verified | `src/battle/battle_script.c` |
| `$85:ECDB` | `Lufia2BattleVramQueueSlot` | verified | `src/battle/battle_frame.c` |
| `$85:ECF0` | `Lufia2BattleFrameUpkeep` | verified | `src/battle/battle_frame.c` |
| `$86:81A9` | `Lufia2SelectScreenNmi` | verified | `src/menu/menu.c` |
| `$86:838C` | `Lufia2TitleObjects` | verified | `src/title/title.c` |
| `$86:86ED` | `Lufia2TitleLayers` | verified | `src/title/title.c` |
| `$86:88BE` | `Lufia2TitleParticleSprites` | verified | `src/title/title.c` |
| `$86:8996` | `Lufia2TitlePaletteCycle` | verified | `src/title/title.c` |
| `$86:8B55` | `Lufia2SpriteFrame` | verified | `src/menu/menu_sprites.c` |
| `$86:8B73` | `Lufia2SpriteAnimateAll` | verified | `src/menu/menu_sprites.c` |
| `$86:8BCF` | `Lufia2SpriteClearOam` | verified | `src/menu/menu_sprites.c` |
| `$86:8BF5` | `Lufia2SpriteBuildOam` | verified | `src/menu/menu_sprites.c` |
| `$86:8CDA` | `Lufia2SpriteSetTable` | verified | `src/menu/menu_sprites.c` |
| `$86:8CF5` | `Lufia2SpriteSetAnimation` | verified | `src/menu/menu_sprites.c` |
| `$86:99BF` | `Lufia2WorldMapStreamEdges` | verified | `src/world/world_map.c` |
| `$86:9EDD` | `Lufia2WorldMapRegionSearch` | verified | `src/world/world_map.c` |
| `$86:CEF6` | `Lufia2WorldMapNmiUploads` | verified | `src/world/world_map.c` |
| `$86:E8CE` | `Lufia2WorldSpriteChain` | verified | `src/world/world_map.c` |
| `$8E:BD77` | `Lufia2FieldScrollUpdate` | verified | `src/field/field_scroll.c` |
<!-- metadata-index:end -->

## Function notes

M2 consumer inspection uses the original scene NMI stub at `$00:0067-$006B`
(`JSL target; RTS`). Boot sets its opcodes at `$80:80E6/$80:80EC`; the NMI
dispatch at `$80:8650` is gated by the target bank at DP `$6A`. Battle setup
clears that bank at `$81:8644`, writes `$8DC5` at `$81:8649`, and publishes
bank `$85` at `$81:864D`, after setting display shadow `$0583=$80` at
`$81:8528` but before resources finish loading. Exit clears the bank at
`$81:87A8`; Field restoration installs `$83:9FA9` at `$83:B0E5-$83:B0EE`.
The callback implementations below are verified; the remaining connecting
setup and return functions (`$81:8821`,
`$81:851E`, `$81:876B`, `$83:B062`) are identified research boundaries,
without semantic promotion or new bindings. Complete lifecycle reconstruction
remains separate future semantic work. Renderer readiness and margin policy
stay in the consumer.

BL1 adds `$83:83E0` as a verified and bound whole `JSR` wrapper. It writes
`$7F:F8A3 = FF`, calls `$83:83EB` through a pushed JSL frame, and executes
RTS at `$83:83EA` only if the child returns. The child is still original-ROM
execution. Private differential verification covered 512 synthetic child
return/unwind cases with CPU state, complete WRAM and ordered writes. The
child's wait address `$7F:D4F8` is distinct from the wrapper's `$7F:F8A3`.
The `$83:845B` wrapper is also verified and bound. It saves A/X/Y/DB/P,
calls the visual transition `$84:8BC7`, clears `$09A9` bit 0 and DP `$6A`,
then calls Battle entry `$81:8821`. After that child returns, it re-reads
`$7F:F8A3` to select the Field continuation, clears `$05B3`, calls
`$83:85DC`, and restores saved P/DB/Y/X/A before RTL; final `PLA` sets
N/Z from the restored A rather than retaining the original P's N/Z. Private synthetic
child verification covered 768 whole-wrapper cases, seven child edges,
CPU/WRAM and ordered writes. Child functions retain their own separate
semantic status.

BL2 verifies and binds `$84:8BC7` as the pre-Battle visual transition. It
clears `$7E:2000-37FF` in three 2 KiB planes, selects `$212C`, calls
`$80:F47A` twice, then sets PPU mode 2 and seeds `$7F:D0F2=01`,
`$7F:D0F3=80`, `$09AA=01`. A 32-step loop fills two sine-derived WRAM
tile buffers using the PPU multiplier and configures two HDMA channels.
The function calls the `$84:8D4D` DP `$40` change-wait 34 times in total;
NMI increments DP `$40` at `$80:869B`. The verified Field NMI owns the
`$2106` mosaic progression while `$09AA` bit 0 remains set. `$84:8BC7`
does not directly write `$2100` or `$2106`, and returning from it does not
prove the Battle picture is ready. Private verification covered 96 cases,
2,892 child calls, complete WRAM, CPU state and exact ordered MMIO writes.

BL3 starts with verified and bound `$81:8821`. It saves DB/P/A/X/Y, resets
DP `$40`, uses an overlapping `MVN 00,00` to clear the Battle working area
`$11D8-$1C0C`, sets DB `$97`, saves SP at `$1395`, and sets `$0A13=FF`
before calling setup `$81:8000` and main loop `$81:886F`. On normal
main-loop return it restores SP, calls `$85:EDBB`, `$85:EEA1` and exit
`$81:876B`, clears `$0A13`, then restores the saved registers/status; final
`PLB` sets N/Z from the restored DB.
Its child functions remain separate; this wrapper does not prove picture
readiness. Private 320-case differential covered CPU, WRAM and ordered
writes across both return and child-unwind paths.

| Address | Symbol | Status | Notes |
| --- | --- | --- | --- |
| `$83:83E0` | `Lufia2FieldEncounterHandoff` | `verified` | M1X0 `JSR` from `$83:C1CC`, observed DB `$83`, DP 0. Writes `$7F:F8A3 = FF`, calls `$83:83EB` with a pushed JSL frame, then RTS at `$83:83EA` on normal return. Child unwind propagates. Private 512-case `interp816` differential compared CPU, 128 KiB WRAM and ordered writes. Runtime-bound. |
| `$83:845B` | `Lufia2EncounterBattleSequence` | `verified` | M1X0 `JSL` from `$83:83FD`; saves A/X/Y/DB/P, calls `$84:8BC7` and `$81:8821`, then selects Field continuation from `$7F:F8A3/$F8A2/$F8A5`, clears `$05B3`, calls `$83:85DC`, restores saved P/DB/Y/X/A and RTL at `$83:84E0` (final PLA sets N/Z). Child unwind propagates. Private 768-case differential covered 600 returns, 168 unwinds and 3,338 child edges with CPU, WRAM and ordered writes. Runtime-bound. |
| `$84:8BC7` | `Lufia2BattleVisualTransition` | `verified` | M1X0 `JSL` from `$83:8466`; three tile planes cleared, `$212C` selected, mode 2/scrollers initialized, `$D0F2/$D0F3/$09AA` seeded, 32 sine-buffer/HDMA phases separated by DP `$40` waits. Returns at `$84:8D4C` after restoring DB, or propagates a child unwind. Private 96-case differential covered 72 RTLs, 24 unwinds, 2,892 child calls, CPU/WRAM and exact MMIO order. Runtime-bound. |
| `$81:8821` | `Lufia2BattleEntry` | `verified` | M1X0 `JSL` from `$83:8474` (also `$86:9D9A`); saves DB/P/A/X/Y, clears `$11D8-$1C0C`, changes DB to `$97`, marks `$0A13=FF`, calls `$81:8000` setup and `$81:886F` loop, then restores SP, calls cleanup/exit children, clears `$0A13`, restores saved registers/status (final PLB sets N/Z) and RTL at `$81:886E`. Private 320-case differential covered 240 returns, 80 child unwinds, 1,440 child edges, complete WRAM and ordered writes. Runtime-bound. |
| `$83:81C6` | `Lufia2FieldTriggerUpdate` | `verified` | Per-frame field trigger check, `JSR` from the field loop `$83:808C` (M1X1, DB $83), `PHP` ... `PLP; RTS` at `$83:829F`. Native: `$80:CBAE` event-timer tick (8 timers at `$7F:D18C`, bit 7 = armed, `$05B5` bit 1 = any armed; shared with its own binding), `$83:80CD` idle test (`$09A8/$0622/$05B7/$05B5/$17AA/$099B` and `$7F:D057..D05E`), `$83:B8BF` touch scan of actors 8..39 around the leader (height via F988, wide actors from `$7F:E216`, `$83:D927` edge table D932/D93E/D948/D952 with its persistent `$8F/$91` steps). Exact LLE boundaries: `$80:CC3F` (unported event script opcode, since E1), `$80:CC0E`, `$83:B96D` (touch event start via `$80:BFE7`/`$83:83EB`), `$83:8225` (trigger queue `$80:E722`), `$83:8251` (door/warp `$83:B66E`). Chosen from the 2026-09-23 gameplay profile: ~5,600 interpreter entries at ~837 instructions each on the saved field state. Verification: 16,384 (RTS 12,855; LLE 80:CBCC 1,908, B96D 100, 8225 97, 8251 1,424) whole-function cases vs ROM, 8,704 bridge/ABI cases. Runtime-bound through `Lufia2DecompBridge_81C6`. |
| `$83:E03E` | `Lufia2ObjectSlotsUpdate` | `verified` | Per-frame object-slot update, `JSR` from the field loop `$83:8081` (and `$83:81A8`), M1X1 in, `SEP #$10; RTS` at `$83:E0FB`. 32 slots with `$064A` bit 7 active: blink countdown `$7F:E386` (low nibble, reload from the high nibble, toggles `$7F:E33E` bit 7), `$09A8` mask from `$7F:DB0C` bit 6, animation step `$7F:DAEC` to $20 (TDC stores the DP low byte) flipping `$7F:DB2C` bit 0 and setting the frame through `$83:E200` (CPU multiplier with `$83:ABF8`), script wait `$7F:DFAE`. An expiring wait runs the object script VM `$83:E0FC` natively: PHB, DB/cursor from `$7F:DEEE..DEF0`, dispatch `$83:E10F` (`$F2C8` by high nibble; `$83:ED63` low nibble into `$F2E8/$F308/$F328/$F348/$F368`), exits `$83:E11C` (advance, yield on `$064A` bit 4) and `$83:E143` (store cursor). Native handlers (slice O1, the hot profile sites): 4x `$83:E831` wait, F6 `$83:E168` nibble offsets (FB05/E1AD/E1B9), F2 `$83:EEB5` random jitter (EED8 via `$80:8299`), 1A `$83:E589` sprite frame, 80 `$83:EAD5` loop counter `$7F:E08E` with jump to `$7F:DFCE`; slice O2 adds 7x `$83:EAB5` loop start, 3x/2C/25/FD/8F frame setters (E1C5/E1D3/E1E6/E1F4/E59F via E200), 2F E150, 2E E270, 1D E5B5, 12 E700, 13 E810, 22 EA7D, 23 EA8C (TDC stores DP low), 8D EA94, 29 EB50, 2B EC6D, Cx ED11, 84 ED9B, 89 EDC2, EA EDD7, F8/F9 EFD1/EFDE, E2 F0E8, 82/83 F0FA/F10B, EB F11C (each byte negated on its own), EC F137, ED F155 (script pointer for every free `$7F:D0A6` slot); slice O3a adds Dx ED32 goto (word + $8EC7 via ED38), 5x E840 / 28 EB38 jump tables (facing, `$7F:DA2C`), FB EE48 and FC/E3 EE71/F19F signed offsets to `$7F:DDFE/DE8E` (EE56/EE84; E3 advances by the TDC value), F7 E191 offsets to `$7F:DCDC/DD6C`, 20 E99A velocity reversal, F1 EDE6 animation takeover from another slot (EE0E; a miss writes slot 32, one past the table). Slice O3b adds 19/8E E437/E445 (animation lookup by `$7F:E1AE`/`$7F:DB2C` key: takeover via EE0E, else `$7F:DB0C` bit 5 and EC9C), 26 EAFF and A0 EC8E (EC9C), the EC9C animation setup (`$83:F388` 4-byte records, ECDE sprite allocation through `$83:AB7C` and VRAM base `$83:ABE9`), 60/85/8A/EE E854/E860/E87C/E937 child spawn via E8A6 (DF87, AB4F, position/facing copy) and 0x E13D despawn (`$83:F205` frees the sprite slots through `$83:ABCC`, then yields). AB7C never resets Y after an occupied slot and spins forever on a size-0 sprite (`$83:ABF4` entries 9/11/13/14): those handlers hand off at their entry PC when the looked-up size is 0. Other handlers stop at their entry PC; a script that never yields stops at `$83:E10F` after 65,536 dispatches. Verification: 16,384 (RTS 7,840, LLE 8,544, 166,289 object VM dispatches) whole-function cases vs ROM (object scripts in bank $7E/$00), 8,704 bridge/ABI cases. Runtime-bound through `Lufia2DecompBridge_E03E`. Since O14 (2026-09-26) object opcode `$14` (`$83:E27D`, spin and zoom) is native: scale `$1579` steps by the high nibble of `$15D9` every (`$15D9` & 15) ticks with an optional limit (`$1619` bit 7), the angles `$14D9/$14F9` step the same way (`$1639/$1699`, limits `$1679/$16D9`); a scale change rebuilds the offset `$1539/$1519` with the PPU multiplier (`$80:84ED/852D` sines, `ASL $2134`), `$83:E703` rotates and projects it (PPU multiplier, divider `$4204/$4206` by the depth `$51`) into `$7F:DDFE/DE8E`, sets the behind flag in `$7F:E33E`; a zoom shrunk to nothing ends the object (`$83:E13D`). gameplay6 had 1,055 hand-offs there (13,520 entries with the fallout). E03E mutations for O14 25/25. |
| `$83:9FA9` | `Lufia2FieldNmiUploads` | `verified` | Field NMI work, reached through the RAM vector `$00:0067` (`JSL $83:9FA9; RTS`, called from the NMI handler at `$80:8650` while `$6A` is set). `PHP; PHB; PHK; PLB`, RTL at `$83:9FE0`. Native, no boundaries: `$83:A033` palette uploads (`$73` bits 0-1, CGRAM DMA channel 6 via `$83:A052`), `$83:A0E1` queued VRAM tile uploads (`$1246/$1236` and `$123E/$122E`, two DMA bursts each), `$83:A1A2` column uploads listed at `$7F:D4F8` with 2 KiB/1 KiB wrap, `$83:A070` eight block uploads at `$05C2`, `$83:9FE1` colour-window fade (`$2106`, `$7F:D0F2/D0F3`, `$09AA` bit 0), window positions `$2126/$2127` from `$7F:D081` (TDC on the negative path). Chosen from the gameplay profile (6,461 entries). Verification: 8,192 whole-function cases vs ROM comparing the complete MMIO write sequence (verify bus MMIO log), 8,704 bridge/ABI cases (JSL frame). Runtime-bound through `Lufia2DecompBridge_9FA9`. |
| `$8E:BD77` | `Lufia2FieldScrollUpdate` | `verified` | Per-frame BG scroll targets, JSL from the field loop at `$83:8084`. Camera `$05A4/$05A6` from `$7F:D08B/D08D` (`$1261` bit 3) or the leader position minus (`$80`, `$70`); then three layers (`$7F:D020`, bit 7 = off) through `JSR ($BE6E,x)`: 0 `$8E:BE78` follow (targets `$7F:D0CE/D0D6` with speeds `$7F:D0DE/D0E6` when `$1261` bit 6, else the camera with speed `$05A8`, cleared on arrival), 1 `BF24` static, 2 `BF25` parallax (camera >> `$8E:BF93` count, `$BF70` sign-extending; a negative count keeps the camera), 3 `BFEE` fixed offsets `$8E:C04D` wrapped by the map size `$7F:D010/D018` * 32, 4 `BF9B` camera << count (the second nibble is read with X = the first nibble, a ROM quirk kept). Stores `$121E/$1226` and the BG registers at `$0594/$0596` + `$80:F4ED/F4F5` (+8 and the shake offsets `$7F:D081/D083`). The tile streamers it JSLs into run natively too (only BD77 calls them): `$80:F4FD/F518` stream a map column at the right/left edge and `$80:F589/F5A2` a row at the top/bottom edge, through `$80:F734` (cell and buffer offsets, wrapped by the map size with the CPU divider `$4204/$4206` -> `$4216`), `$80:F5ED/F64E` (16x16 metatiles from `$7F` cell data into the `$7E` tilemap buffer at `[$2A]`, `$7F:D008` escape for cells with bits 12-13 set) and `$80:F6AA` (cell index through the multiplier); they queue the NMI uploads at `$1236/$1246` and `$122E/$123E`. Exact LLE hand-offs remain at `BDD7` for an unknown mode (>= 5) and at the entry for X16 callers; `dispatches` counts completed BDD7 layer calls so the verifier stops at the right visit. Verification: 16,384/16,384 whole-function cases vs ROM (RTL 12,696, LLE BD77 1,018, BDD7 2,670) comparing the complete MMIO write sequence (113,002 divider/multiplier writes; the verify bus now emulates the CPU divider like the runtime core), 8,704/8,704 bridge/ABI cases (JSL frame). Runtime-bound through `Lufia2DecompBridge_BD77` (new `recomp/bank8E.cfg`, which only declares the bank; offline v2_emit diff vs baseline: BD77's four width variants route to the bridge, nothing else changes). |
| `$83:80CD` | `Lufia2FieldIdleTest` | `verified` | Field-loop idle test (JSR at `$83:80BB`; also inlined in 81C6): Z set when `$09A8` bit 3, `$0622` & `$88`, `$05B7` & 7, `$05B5` & `$A2`, `$17AA`, `$099B` bit 7 are clear and none of the eight `$7F:D057` slots is active. X8 only; an X16 caller hands off at the entry. Verification: 16,384/16,384 whole-function cases vs ROM (RTS 14,414, LLE entry 1,970), 8,704/8,704 bridge/ABI cases. Runtime-bound through `Lufia2DecompBridge_80CD`. |
| `$83:8682` | `Lufia2FieldAnimationTicks` | `verified` | Per-frame ticks for the eight animation slots at `$7F:D057` (JSR at `$83:807A`): active slots (bit 7) build the mask `$17AA` and advance by 8; when bits 3-4 reach `11` the slot is re-armed and its handler runs from `$7F:D04F` via `$7F:D04E`/`$7F:D049` - the handlers `$83:8783`/`$83:87CC` stay LLE (exact hand-offs at `86D6`/`86DB`). `PHP/PLP`, X16 inside. Verification: 16,384/16,384 whole-function cases vs ROM (RTS 14,458, LLE 1,926), 8,704/8,704 bridge/ABI cases. Runtime-bound through `Lufia2DecompBridge_8682`. |
| `$83:AEB5` | `Lufia2FieldColourEffects` | `verified` | Per-frame field colour effects (JSR at `$83:8070`), gated by `$09A9`: bit 1 counts down `$1280` and bit 2 runs bank-84 effects (`$84:8E07`, `$84:8D54`; both stay LLE, exact hand-offs at `AEC7`/`AED3`); bit 0 runs, with DB = `$7F`, `$83:AEED` palette cycles (`$7F:D0F7` slots with timers `$7F:ED01` and script pointers `$7F:EC00` into bank `$A1`; zero-length frames jump by the word at +3 plus `$7F:D0F5` + 1; the colour word goes to the CGRAM buffer `$0320` and sets upload flag `$73` bit 0) and `$83:AF54` (HDMA wave table: `$7F:D0C9/D0CA` row pointer into `$7E:C000`, `$7F:D0C8` delay, source `$4312` = `$7F:D0C6` + row offset, channel 1 set up at `$4310/$4311/$4314/$4315`, `$7B/$76`). A zero-length frame chain that never ends hands off at `AF11` after 65,536 visits. Verification: 16,384/16,384 whole-function cases vs ROM (RTS 12,938, LLE 3,446) comparing the MMIO write sequence (25,704 writes), 8,704/8,704 bridge/ABI cases. Runtime-bound through `Lufia2DecompBridge_AEB5`. |
| `$80:9C72` | `Lufia2FieldEventTick` | `verified` | Field-loop JSL at `$83:8073`, idle frames: `$84:8000` screen-effect checks (`$1261` bits 0-2/4-5/7 and `$1262` bit 0 all clear, else the whole call hands off at its entry), the `$80:C21A` event timer `$7F:D0C1` (hand-off at `$80:C229` when it fires), then `$099B` gates (bits 1/3 -> hand-off at `9C80`, bit 7 text engine -> `9CB8`). Verification: 16,384/16,384 whole-function cases vs ROM with the MMIO order compared since F6 (RTL 15,769, LLE 615 after F8), 8,704/8,704 bridge/ABI cases, mutations T4 11/11, F6 16/16, F7 15/15, F8 4/4. Since F6 the screen effects `$84:8000` run natively too instead of handing the whole tick to LLE: `$1261` bit 2 shake (`$84:8145`, random offsets into `$7F:D081/D083` when a `$80:8299` roll is below `$7F:D080`), bits 1/0 brightness fade in/out (`$84:80E6/8115`, every `$7F:D08F` frames `$7F:D091` +/- `$7F:D090` into `$0583`, ending at 15 or in forced blank), bits 4/5 color math intensity `$1271` down/up every 6 frames (`$84:80C0/80D3`), bit 7 COLDATA fade (`$1286-$1289` into `$2132`) and since F7 `$1262` bit 0, the palette fade (`$84:8E07`: levels `$1274/$1276/$1278` step by `$127A/$127C/$127E` down or up by `$1283` bit 7, then every color of `$9B:[$7F:D0F8]` from row `$7F:D0FA` to `$E0` gets red/green/blue shifted by them with clamping and lands in `$0320`, `$73` bit 0 requests the upload, `$1282` counts the steps; `$84:80A6` ends it when `$1288` reaches 31). Since F8 the event timer firing (`$80:C229`: timer `$7F:D0C1` back to FF, `$099C` bits 7/5 cleared, window cleared through `$84:8328`, `$74` = 8) is native too, so 9C72 itself never hands off; only unported text opcodes inside the text step do. Since T4 the `$099B` bits 1/3 path is native too (`$80:9C80`, a text box waiting: timer `$1266` counts up to `$1265`, or with `$1265` = 0 the A/X buttons via `$80:C81E` (`$46` pressed, consumed from `$4A`); then bit 3 closes the window (`$80:C1FD`, `$80:C11C` clears bit 0 of the actor flags `$0622-$0649`) or bit 1 is cleared and the text step `$80:9CB8` continues). Runtime-bound through `Lufia2DecompBridge_9C72` (`recomp/bank80.cfg`); the chain helpers `$82:E746` and `$83:85DC` are bound too so no node loses AOT. The text step `$80:9CB8` now runs natively from here. |
| `$80:CBAE` | `Lufia2FieldEventTimerTick` | `verified` | Wait timers of the eight field event script slots. JSL'd with M1X0 from the field trigger check `$83:81C6` (`$83:81CB`, and `$83:8237` after a queued trigger) and from `$80:EAE0`, `$83:8530`, `$83:85A9`, `$83:B4F6`, `$83:B667`, `$8E:B02A` and `$8E:B2AB`. `PHB; PHP; REP #$10` ... `PLP; PLB; RTL` at `$80:CC25`, so exit widths and DB are the caller's; DB is used as is for `$1273`/`$05B5`. Clears the script redraw requests `$1273` and `$05B5` bit 1, then counts every armed timer `$7F:D18C` (bit 7) down: `$81` becomes `$80` and makes its slot due, while an armed `$80` wraps to `$7F` and disarms itself (ROM quirk, kept). Afterwards `$05B5` bit 1 is set when a slot is still armed. Slots are filled by `$80:E722` (timer `$81`, script pointer `$7F:D134 + 3n` from `$7F:D197/D199`, position `$7F:D17C/D184`). A due slot resumes its script natively since slice E1 (`$80:CBCC`: `$A7` = slot, `$83:AB4F`, pointer and bank from `$7F:D134 + 3n` into `$7F:D197/D199`, DB = script bank, `JSR $CC35`). Event script VM `$80:CC35`: `$80:E8B9` fetches `DB:Y` (a Y wrap to 0 steps `$7F:D199` and DB and restarts at `$8000`), `JMP ($E5A4,x)`. Native opcodes (E1): `$00` and aliases end (the slot timer gets the DP low byte from `TDC`), `$11` wait n frames (timer `n | $80`, resume point saved), `$0A` goto (`$80:E8AD` word + base `$7F:D194`, `$80:E8F4` steps a bank below `$8000`), `$01/$0C` goto if script flag n (`$7F:D100`, bit from `$80:BE45` via `$80:E898`; the DP high byte enters the index through `TDC`) is set/clear, `$08/$09` set/clear it, `$FB-$FF` operands read slot variables through `$80:E9BC` (`$7F:D15C + slot + 8k`, via the multiplier `$4202/$4216` addressed DB-relative), `$0D/$71` goto if `$0692` equals/differs, `$19/$1E/$2B` branches on the slot bits `$7F:D14C`, `$1B` PPU write `$2100 + n` (DB-relative), `$1C` screen shake, `$57` nothing; since E2 the script variables `$7F:D074`: `$2F` set to a value operand (`$80:E9ED`: `$A0-$BF` read variable n, `$FB` stays), `$30-$34` add/subtract/increment/decrement/set (`$80:D9F0`), `$35-$40` compare with a byte or value operand (minus 1 for four of them) and goto on equal/not equal/carry; since E3 the script points `$7F:D1A3/D1E3` (64 x/y pairs named by `$E0-$FF`): `$83/$84` set `$7F:D223/D263` to point x/y + n (`$80:DAE5`), `$9D/$9E` copy point x to/from a variable (`$80:CFFC`), and `$79` (`$7F:E316`), `$86` (`$05B5` bit 7, `$05B3` bit 3), `$A2` (`$05B5` bit 5), `$AB` (stair state `$7F:D0BF` = FF), `$B5` (clear `$1261` bit 3), `$B8` (`$72` bit 0); since E4 `$5F/$68/$6B` wait while an actor moves (`$0622,x` bit 7 without bit 2, DB-relative; `$5F` maps its operand through `$7F:D72C`, `$6B` watches the leader): the script steps back over the opcode with `$80:E8D0` and sleeps one frame via `$80:D31C`, and `$58` moves the scroll target of layer n (`$7F:D0CE/D0D6` by signed bytes through `$80:DBD2`, which only extends negative bytes; speeds `$7F:D0DE/D0E6`, high bytes get the DP low byte; `$1261` bit 6); since E5 `$24/$25` spawn a secondary actor (`$83:DF87`, fine position through `$83:E018`, `$A7` restored and `$83:AB4F` rerun) at a tile or at a position operand (`$80:EA09`: `$FB` the slot's own position `$7F:D17C/D184`, `$E0-$FF` a script point; other operands name map actor n - `$20` through `$80:E912`, native since E7), and `$29` sets or clears script flag `$7F:D15C,slot` by the last condition bit (`$7F:D14C` bit 7, copied to `$7F:D19A`); since E6 `$A9` calls a sub-script: the call depth `$7F:D4E6,slot` rises, a frame tagged depth << 4 | slot is taken from the 13 frames at `$7F:D466` (10 bytes: tag, return pointer and bank, the four slot variables, the condition bytes; when all are busy frame 0 is overwritten), `$80:D73A` passes the argument list up to `$FF` as the new slot variables (`$FB-$FE` pass the caller's own), then the word target; `$AA` returns through the frame with the current tag (without one, X ends at `$80` past the frames, a ROM quirk kept); since E7 the list search `$80:BFAA` (key and stride in A/B, list offset at `$7E:F000 + X`, `TXA` with M=1 keeps the high byte in B; carry clear = found; a list without `$FF` hands off at `$80:BFBC` after 65,536 entries), map actors `$80:E912` (`$7E:F022`, stride 3, the first entry when not found) and the operand box `$80:E92A` (`$9F-$A2` from the slot, a script point, a map entity `$60-$DF` in `$7E:F024` with stride 5, or a position), `$55/$69` set a script point from such a box and `$85` sets `$05BD-$05BF` from a position operand and a byte; since E8 the actor placement opcodes: `$80:E004` takes the box of an operand as target and steps it once in the opcode's direction (`$83:FB12`, stored back to the point), `$80:DFB5/DFC2` claim a free actor (`$83:C108` with `$83:FC3C`: bit 2 set and `$05D2` = FF, none gives slot `$28`), give it map object n (`$83:8B40`: `$7E:F016` list, stride 10, into `$7F:D04A/D04C/D05F`) at the target and its script (`$83:D416`), and `$83:D350` starts action `$4C` + direction (`$64-$67`, `$7C-$7F`, `$A3-$A6`; the last two groups also set `$7F:E316`) or `$5C` (`$80`, `$81`); `$AF-$B2` send an existing actor with action `$74` + direction. The running slot is kept in `$7F:D2A3` around them (`$80:EA47/EA50`). A hand-off inside `$83:D350` or `$83:FB12` stays exact (PB `$83`). Since E9 `$B4` moves the camera (target `$7F:D08B/D08D` = scroll of layer `$05AA` + signed dx/dy, speed `$05A8`, `$1261` bit 3; `$B5` releases it), `$B6/$B7` set/clear the leader's `$0622` bit 1 and clear/set its map occupancy (`$83:FA12/FA3F`), `$BD` fills `$7F:E33E-E35D` (`$83:E033`) and `$82` sets a point from a placed object (`$7F:D69C/D6CC` when `$7F:D75C` bit 7). Since E10 the conditions (`field_event_conditions.c`): the result is `$7F:D19A` (bit 7 true) with the operand in `$7F:D19B`; `$80:E4C5` stores both into the slot, `$80:E4BA/E4AF` goto when true/false, `$80:E40C` only stores, `$80:E55B` inverts, `$80:E3D9` keeps bit 7 and sets bit 0 when the result differs from script flag n. Leader conditions (`$12`, `$6D`, `$13`; `$80:E0DC/E105`): at a position (`$80:E132`), in a box (`$80:E15A`) or in a condition list; map cell conditions with mask `$AE` = 9, 8 or 1 (`$15-$18`, `$6E`, `$6F`, `$72-$74`, `$14` gated by a script flag; `$80:E365`, cell at a position `$80:E37B` via `$83:F9A9`, cells in a box `$80:E458`); placed objects of type n + `$10` (`$04`, `$05`, `$70`; `$80:CD41`, boxes `$80:CD88/CD8B`, a hit moves the slot there); `$7F:D0F4` match (`$75-$77`, `$80:E1CE`). Condition lists `$80:E566` are not recursive: `$80:E78D` finds list n in the (key, word) table at base + `$0A` and each entry + `$60` is a box for one of the three box tests (`JSR ($E59E,x)`, X = 0/2/4). Since E11 `$6A` (leader action n) and `$23` (n + facing offset from `$83:C1A5` by `$0692`, then leader flags) run `$83:D350` for actor 0 with `$80:D49B` (clears leader `$0622` bit 3 and records the blocked step `$83:CA68` when `$7F:D0A1` bit 2), `$AE` runs it for the last claimed actor `$7F:D0A2`, `$0F/$6C` goto on bit 0 of the cell type (`$83:FB51`: `$7F:D296` by the cell's high nibble), `$A7` stores the cell value (`$83:FB71`) in `$7F:D133`, `$9C` a cell's height bits (`$83:F988`) in a variable, `$9F` sets `$7F:D10B` bit 7 when two positions are equal, `$5E` stores the placed object at a position (`$83:FB9F`, FF when none). Since E12 the 20 entries `$41-$54` (one handler, `$80:D9FC`): the hardware divider splits opcode - `$41` by 5 into the point coordinate (x, y, x2, y2 via `$80:DA4B`) and the operation (`$80:DA53`: set, add, subtract, +1, -1); the divider registers are addressed DB-relative, so a script in a WRAM bank reads WRAM there, and a remainder table word outside the five hands off at `$80:CC3F` before the opcode starts (predicted from the remainder byte outside the MMIO banks). Since E13 `$27` (`$80:D6E9`) forks: it claims the first free slot (`$80:E99D`, slot 0 when none), passes the argument list to it (`$80:D73A`), points it at the target and runs it through a nested `JSR $CC35` until it yields; `$26` (`$80:D6CA`) forks only when slot bit 0 is set and else skips arguments and target. The native loop counts `$80:CC3F` passes per nesting depth, since the reference counts handoff visits per stack height. Since E14 the object state bits at `$7F:D095` (`$83:8AF7` index and mask, `$83:8AC9` test, `$83:8AD5` set, `$83:8AE5` clear; operand via `$80:E9B0`): `$A0/$A1` set and clear directly; `$02` sets and `$03` clears through the object animation queue `$7F:D057/D04F` (`$83:F559`, type 0 or `$40`, `|$90`, deduplicated), directly when `$0583` bit 7 is set (DB-relative), and `$03` queues only when the object's rows do not cover `$06E2` (`$80:CCDB`, two `$80:BFAA` list searches); `$28` is `$02` on a set result, else `$03`. The long `$00:0583` read in `$83:F559` can still see bit 7 when the script bank is `$7F`; that path (`$83:8848`, map tile redraw) hands off at `$83:F564` with program bank `$83`. Since E15 `$BA` sets entry n of the `$7E:F026` list (stride 4) to a position (a missing key writes over the `$FF` end), `$1D` (`$80:B97E`) clears `$1261` (DB-relative) and `$7F:D081/D083` with D, and `$63` starts event 0 of the header list through `$80:E722` (list `$80:E78D`, now exported; first free slot or slot 0, timer `$81`, position `$8F/$91`; nothing when the base bank is `$FF`) and then ends the slot. Since E16 `$87/$88/$89` (`$80:E203`) compare the tiles (low 10 bits) of two positions in each layer of a mask (`$7F:D008` bases, cell offset `$83:F9F7`, now exported as `Lufia2MapCellOffset`) and keep, goto on, or goto unless any match; `$20` (`$80:E09D`) clears the result when a listed actor id (+`$4F`, looked up in `$05FA` by `$80:BF92`, DB-relative; a missing id tests entry `$28`) lacks `$0736` bit 5, then gotos when false. A read-only pre-scan (`Lufia2EventListEnds`) hands `$20` off at `$80:CC3F` before it starts when no `$FF` follows within 64 ids: random scripts in ROM banks produced lists of hundreds of ids that the reference could not finish in its budget. Since E17 `$10` redraws all four layers (`$83:8E66`, exported as `Lufia2FieldRedrawLayers`; per layer `$80:F47A`: skipped when `$7F:D020,x` is `$FF`, else 16 rows through `$80:F734` and the new row-buffer builder `$80:F6C6` in `field_scroll.c`) and sets `$1273` bits 0-1; `$7B` runs the field scroll step `$8E:BD77` nested with X8. A handoff inside it (`$8E:BDD7`, unknown layer mode) reports the completed `BDD7` layer calls of this depth (accumulated over earlier `$7B`) as its visit count; the tick and `$83:81C6` pass it on for `$8E:BDD7` like for `$80:CC3F`. Heavy opcodes count more toward the 4096 per-tick cap (`$10` 256, `$7B` 64) so looping scripts stay within the reference budget; the weight only moves the native/LLE boundary. Since E18 `$94/$95/$96` (`$80:E28C`) keep, goto on, or goto unless the tiles at a position match map object n's block (`$83:8B40`, now exported as `Lufia2EventMapObject`; size `$7F:D04C/D04D`, layers `$7F:D05F` bits 0-1, compared by `$80:E2ED` with DB `$7F`). A block size of 0 means 65536, so a read-only prediction (`Lufia2EventPeekByte` plus the `$80:BFAA` search) hands the opcode off at `$80:CC3F` when the worst case exceeds 4096 tile pairs. Since E19 `$97-$9A` step listed actor id n (+`$4F`, `$80:CF20` via `$80:BF92`, now exported as `Lufia2EventFindActorId`; a missing id keeps the running slot) one cell down, left, up or right: target `$7F:E5A6/E5CE`, action `$20` in `$070A`, `$0622` bit 0 cleared, script restarted (`$83:D416`), running slot restored. Since E20 `$8A` runs action `$5D` (`$83:D350`) for actor slot n and `$8B` frees it (occupancy `$83:FA12`, `$0622` bit 2 set and bit 7 cleared, `$05D2` = `$FF`, `$7F:E48E` = 0); both clear the claimed actors (`$83:F7D4`). Since E21 `$60` hides listed actor id n (`$8E:BB17`: `$0622` bit 2 and `$0736` bit 5 set, occupancy cleared) and `$61` shows it (`$8E:BAF6`: bit 2 cleared; a hidden one gets bit 5 cleared and hands off at `$8E:BB12`, the `JSL $81:8351` with id - `$50`, program bank `$8E`; a native `$61` never passes that point, so it is always the first visit). Since E22 `$78` spawns `$00` actors of id `$01` (`$83:DF87`; 0 spawns 256) at random fine positions inside an area (`$80:D533`, coordinates `$80:D583` = 16 * (low + random(range)) + random(16) with `$80:8299`) into `$7F:DDFE/DE8E`; it weighs 64 against the per-tick cap. Since E23 `$59` (`$80:DC0D`): with `$1261` bit 6 set (and cleared) layers 0-1 move to the leader camera (`$7F:DDAE/DE3E` - `$80/$70`) and both are redrawn around it through the region redraw `$83:8E85` (exported as `Lufia2FieldRedrawRegion` in `field_scroll.c`: clips region `$7F:D046` + `$D04C` to the visible cells via `$83:9000/9004`, then writes the metatiles into the layer's row buffers); otherwise it saves the layer positions to `$7F:D0CE-D0F0`. Since E24 `$21` (cell x, y) and `$22` (position operand) toggle the map object there (`$80:D357`): a pending object from the `$7F:D69C` table (`$80:D393`) gets its tiles set from `$7F:D5DC` (`$83:F750`/`F784`), else a cell object (cell value `$10`+, `$83:FB71`) gets its tiles cleared (`$83:8A6F`); both clear attributes (`$83:F442`), use the object origin (`$83:F422`) and redraw its region (`$80:D3C6` -> `$83:8E85`). Blocks over 4096 cells (0 counts as 65536) hand off at `$80:D38B`, and a region redraw hands off at `$80:D3CC` when layer `$05AA` (plus D's high byte) is no layer or more than 4096 cells would be drawn (`Lufia2FieldRegionCells`); both report their earlier native passes at that depth as visits. `EventRun` now lives in `event_script_internal.h` and reaches the actor opcodes, and the timer body reports visits for any handoff pc (0 for a first visit). The `$80:D38B` bound also hands off when the cleared block would cover its own size bytes `$7F:D04C/D04D` (which would turn the width into 65536 mid-loop). Since E25 `$2A` (`$80:D3D6`) places map object n as pending in the first free `$7F:D69C` slot (`$80:D426`) at a position: `$83:F86B` marks the attributes (`$83:F80D`: bit 3, the row above bit 6), hands over the saved tiles of an object already pending there (`$83:FB9F`, `$83:F85A`), saves the replaced map tiles to `$7F:D5DC` (`$D5DE` the row above) and copies the object's tiles in (`$83:F91F`, cells via `$83:F9D4/F9D9`); `$83:8E76` then redraws layers 0 and 1 (over 1024 cells hand off at `$83:8E79` with per-depth visits). `$2A` weighs 256 against the per-tick cap, and since E26 so do `$21/$22`. Since E26 `$BC` moves listed actor id n to the cell of a position (`$80:A74D`): occupancy cleared (`$83:FA12`) and set again (`$83:FA3F`), fine and cell position via `$83:A71C`, `$7F:E316` bit 7 and `$0622` bit 7 cleared, slots from 8 on restart their script with action 9 (`$83:D416`). Since E27 `$B3` (`$80:DF40`) spawns an actor for map object n (a variable operand) in an area: `$80:E004` with no step, `$80:DFC2` (now split out of the `$80:DFB5` wrapper), a byte into `$7F:E316`, then `$83:F5EA` gives it the object's origin (`$83:8B40` and `$83:F422` called from bank `$83`, `$83:F7F8`) and sprite (`$83:F6CA`: palette copied with `MVN` by `$83:F731`, animation tables `$83:AA7D`, sprite ids and flags). `$83:AA7D` is exported as `Lufia2ActorSpriteTables`; its `LDY $A7`/`LDY $A9` now follow the index width (the X8 caller is unchanged, `$B3` runs it with X16). Since E28 `$5A-$5D` push pending map object n (the `$7F:D69C` table) one cell down, up, right or left (`$80:DCDA`): the collision check `$83:C079` (cell of the step free of secondary actors `$83:C33D`, either marked with attribute bit 7 or not blocked by `$83:D89E`, a cell value of 0, 1 or 9 (a pending type-1 object below counts by the attribute of its saved tile, `$83:F410`/`$83:FB7A`) and the same height `$83:F988`); when it passes, a claimed actor (`$83:C108`, now with a return address) takes the object's place, restarts its script with action 9 and walks the step (`$83:D350`, action `$48` + facing) while the cell occupancy moves along. The VM lives in `field_event_script.c` (core), `field_event_actors.c` (positions, points, actors, and the opcode dispatcher), `field_event_objects.c` (map-object bits, tiles, pending placement and pushing) and `field_event_conditions.c`, sharing `event_script_internal.h`. Other opcodes hand off at `$80:CC3F` with the pass count; a tick stops after 4,096 opcodes. Remaining exact LLE boundaries: `$80:CC3F`, `$80:CBCC` only with D set (binary ADC), and `$80:CC0E` (`$1273` set: `$80:CC26` turns it into `$74` tilemap upload bits, uploaded at once through `$80:8285` in forced blank; natively only reachable when DB maps `$1273` to ROM). M=0 hands off at the entry. Verification: 16,384/16,384 whole-function cases vs ROM from the `$83:8237` call (E1: return 10,417, LLE 5,967; seeds are forward-only scripts of the native opcodes in `$7E/$7F:8000+`, some crossing a bank), 8,704/8,704 bridge/ABI cases (DB also `$7F`, `$8E` and `$C0`, where `$1273` reads ROM and reaches `CC0E`), mutations 15/15 for the tick (the `$1273` check is caught by the bridge cases only) 22/22 for E1 and 17/17 for E28 (one needs a rare push onto a type-1 object), 14/14 for E27, 8/8 for E26 (two equivalent through $BC), 16/16 for E25 (one needs a row starting exactly at $D04D), 18/18 for E24 (one needs an odd layer index), 14/14 for E23 (its surviving clip path is caught through E24), 7/7 for E22 (cost weight boundary-only), 8/8 for E21, 9/9 for E20, 8/8 for E19 (one equivalent), 10/10 for E18 (one boundary-only), 12/12 for E17 (four justified), 12/12 for E16 (three equivalent), 12/12 for E15 (one equivalent), 16/16 for E14, 9/9 for E13, 8/8 for E12, 12/12 for E11, 18/19 for E10 (survivor: V after the cell `BIT $AE`, always clear because the masks 9/8/1 lack bit 6; visible only when V was set before), 7/7 for E9, 18/18 for E8, 12/12 for E7, 13/13 for E6, 9/9 for E5, 13/13 for E4, 12/12 for E3, 14/15 for E2 (survivor: the `PLX` after `$80:E9ED` in `$2F`, equivalent because X is unchanged there and the next fetch overwrites N/Z). Runtime-bound through `Lufia2DecompBridge_CBAE`. Codegen: 1,131 AOT / 238 lle_only nodes (was 1,104 / 240); `$80:EA5B`, `$83:B53B` and `$8E:B28F` leave lle_only, 24 newly reached nodes compile, none lost AOT. |
| `$85:ECDB` | `Lufia2BattleVramQueueSlot` | `verified` | Returns in Y the first free slot of the 16-entry `$1A8F` VRAM upload queue (6 bytes per slot). JSL'd from 42 battle sites and had no compiled variant, so every call entered the interpreter. A full queue executes `BRK #$6B`, which hands off exactly at `$85:ECEE`; an X=1 entry hands off at the entry. Verification: 16,384/16,384 whole-function cases, 8,704/8,704 bridge/ABI cases, mutations 5/5. Runtime-bound through `Lufia2DecompBridge_ECDB`. |
| `$80:86C1` | `Lufia2ScreenFade` | `verified` | Screen fade, JSR from the NMI handler at `$80:8661` every frame: while `$0581` bit 7 is set, `$0581 & $3F` accumulates in `$0582` and `$0582 >> 3` becomes the brightness `$0583`, rising to `$0F` or (bit 6) falling to forced blank `$80`, then `$0581` clears. Only its M1X1 variant was compiled, so NMIs taken with X=0 (battle) entered the interpreter. Verification: 16,384/16,384 whole-function cases, 8,704/8,704 bridge/ABI cases, mutations 5/5. Runtime-bound through `Lufia2DecompBridge_86C1`. |
| `$83:B66E` | `Lufia2FieldStairRects` | `verified` | Field rectangle check (JSR at `$83:8265`, DB `$7E`, every frame): `$83:B882` searches the list at `$7E:F000 + [$F002]` (stride `$0F`), then `[$F00A]` (stride 5), for a rectangle holding `$8F/$91`; a hit updates the stair state `$7F:D0BF` and, on the top or bottom row, `$05BD-$05BF` and `$05B5` bit 5. A list without an `$FF` end hands off at `$83:B88B` after 65,536 entries. Verification: 16,384/16,384 whole-function cases, 8,704/8,704 bridge/ABI cases, mutations 11/11. Runtime-bound through `Lufia2DecompBridge_B66E`. |
| `$83:B711` | `Lufia2FieldEventRects` | `verified` | List `[$F00C]` (stride 5); a hit hands off at its `JSL $83:B727` (`$83:B722`). Verification: 16,384/16,384, 8,704/8,704, mutations 1/1 plus the shared search. Runtime-bound through `Lufia2DecompBridge_B711`. |
| `$83:B747` | `Lufia2FieldAreaRects` | `verified` | List `[$F006]` (stride 9); a hit sets `$05B5` bit 3 for type 2 when `$0692` is 4 and hands off at its `JSL $83:B76E` (`$83:B769`). Verification: 16,384/16,384, 8,704/8,704, mutations 2/2 plus the shared search. Runtime-bound through `Lufia2DecompBridge_B747`. |
| `$85:B452` | `Lufia2BattleScript` | `verified` | Battle script VM (JSL from `$81:FAE9`; `$81:FAC9` sets `$BB` = `$0A42` + Y and `$BD` = `$0A44`). Saves B/P/A/X/Y, `$0A60` = 0, then per opcode: `$85:C4DE` (`$85:C168` turns the battler bytes `$7F:F450`/`$7F:F44E` into slot numbers - lowest set bit of 0-5, +5 when bit 7 - and loads the variable bases `$C1`/`$BE` from `$0A64,x` or `$0A80,x` by `$0A13`), opcode byte from `[$BB]`, `JMP ($B483,x)` (96 entries). Operand helpers: `$85:BFBF` byte, `$85:BFCA` word, `$85:BFD8` word or (bit 15) variable, `$85:BFED` read variable (selector bit 7 clear: global `$7F:F40E + 2n`; set: local `$0015 + base + 2n`, base `$C1` when `$0A52` else `$BE`), `$85:C023` write variable. Native opcodes: 00/4F end (`$0A5B` = 0/FF), 03 jump (`$BB` = `$0A42` + word), 05 random jump (`$80:8299`), 06/07 jump if equal/not equal, 0A/0B signed >= / <=, 0C set, 0D add, 0E subtract, 16/17/18 and/or/xor, 19 abs, 1A negate, 1B sign, 1C leader `$11E8`, 1F `$7F:F45C` word, 20 `$7F:F45E/F460` bytes, 42 call (sub-script `$96:FADD` table, return state in `$0A45-$0A4A`), 43 return; since B6 also 04 jump if `$7F:F42E`, 08/09 signed > / <, 21 and 23 movement setups (`$7F:F454/F45A/F45C/F462/F464`), 24-27 battler record bytes `$7F:F44E + $85:9F04/9F0F[n]`, 28/29/2A/2B/2E/3E/41 action codes in `$7F:F454` (1/4/6/3/13/11/12, 28 also `$1262` = FF, `$1269` = 0), 2F count of battlers with `$000F,x` bit 2 (slots `$0A64`), 30 side leader (`$15FE`, `$153C` or `$0A7A`), 35/36 `$0A62`, 50 mask `$7F:F450` with `$0A5D`, 56 timer `$1264`; since B7 also 0F multiply (`$85:DCA3`: 16x16 -> 32 bit in `$63-$66` from three hardware multiplies through `$4202/$4203`), 10 signed divide by a byte (`$85:DC6F`: 24/8 bit `$5D-$5F / $54` through `$4204-$4206`), 11 random scale (`$85:DCEA`: three `$80:8299` rolls build a random 16-bit fraction, A times it via `$85:DCA3`), 12 long divide (`$85:DB6D`: 32/16 bit shift-subtract, 16 unrolled steps), 22/37/3C movement from battler stats (`$85:C05F` reads `($BE),y` with Y from `$85:9E47` via `$85:C099`), 4D mask of battlers without status bit 2 (`$85:C117`). The busy-wait 49 (`LDX $1264; BNE`) and the opcodes calling other banks stay on LLE. All others hand off at the `JMP ($B483,x)` `$85:B470` (the result counts the native opcodes before it). Verification: 16,384/16,384 whole-function cases with the MMIO order compared (return 3,993, LLE 12,391 after B7; seeds are scripts of these opcodes with forward jumps), 8,704/8,704 bridge/ABI cases, mutations 28/29 for B5 (survivor: X after `$85:C023`, dead because `$85:C168` reloads it) and 19/20 (survivor: the dead A of opcode 23) for B6, 23/24 for B7 (survivor: the byte mask for stat offsets `$0E/$BC`, which the four stat indices used here never produce). Runtime-bound through `Lufia2DecompBridge_B452` (`recomp/bank85.cfg`; the ROM node was lle_only because three table entries point at data). |
| `$80:92A4` | `Lufia2IntroNmi` | `verified` | Intro NMI (RAM vector `$00:0067`, profile frames 8-381, 374 interpreted entries): PHP/PHB, DB = PB, state `$50` through `JSR ($92B7,x)`. States: 0 `$80:92CB` DMA channel 6 from `$7E:4000 + [$7E:4004]`, `[$7E:4006]` bytes, to VRAM `$4000`; 1 `$80:92FE` and 5 `$80:9346` logo tiles `$7E:2000` / `$7E:2800`, `$700` bytes to VRAM 0 (`$80:9357`), timer `$4E` = 0; 2 `$80:930C` fade in, brightness `$0583` = `$4E` / 2 for 32 frames; 3 `$80:9320` hold until `$4E` = 120, then `$4E` = 32; 4 `$80:9330` fade out, forced blank `$80` at the end; 6 `$80:9354` `STZ $6A`. Each state ends with `INC $50`. Other table words hand off at `$80:92B1`. Verification: 16,384/16,384 whole-function cases with the MMIO order compared (return 12,683, LLE 3,701), 8,704/8,704 bridge/ABI cases, mutations 12/13 (survivor: sending state 6 to LLE, exact). Runtime-bound through `Lufia2DecompBridge_92A4` (`recomp/bank80.cfg`). |
| `$86:81A9` | `Lufia2SelectScreenNmi` | `verified` | NMI installed by `$86:8000` (profile frames 432-585, after the title: most likely the file select screen; 154 interpreted entries in profile 3). Same frame as `$82:939C`: `$1565` -> `$82:8DA6`; `$1566` -> `$86:87E0` (bit 0: `$86:8809` patches the HDMA tables `$7E:80C0/81C0/82C0` with the pairs `$1597,x/$15A7,x` until a zero count; bit 1: VRAM row upload, VMAIN $81, DMA channel 6 from `$7E:C3C0` to VRAM `$1800 + $15B8`, `$3C` bytes); `$1568` -> `$82:8D44` (VMAIN $80, bits C0/30/0C upload `$800` bytes from `$7E:2000/2800/3000` to VRAM `$1000/$1400/$1800` via `$82:8D83`, then TRB `$AA`). Verification: 16,384/16,384 whole-function cases with the MMIO order compared, 8,704/8,704 bridge/ABI cases, mutations in the 939C set. Runtime-bound through `Lufia2DecompBridge_81A9` (`recomp/bank86.cfg`). |
| `$82:939C` | `Lufia2MenuNmi` | `verified` | Menu NMI (RAM vector `$00:0067`), now without LLE hand-offs. Pushes A/X/Y/P at entry widths, then three redraw requests: `$1565` -> `$82:8DA6` (bit 0: copy the 3-byte HDMA table `[$F4]` to `[$F7]` until a zero count, then program channel `$F3` (`$4300,y` = `$FB`, `$4301,y` = `$FA`, `$4302,y` = word `$F7`, `$4304,y` = `$F9`); bit 1: `$420C` = `$F2`; bit 2: window registers `$212D/$2130/$2131` = 04/02/10; each bit is cleared with TRB), `$1566` -> `$82:8E12` (window line table `$7E:80C0`: bit 0 copies `[$F4]` to `[$F7]`; bit 1 rebuilds `$1530` rows from `$1532/$1534/$1536` into the entries after `$1538`, end markers 03 at `$1538` and 09 at `$153A`; bit 2 shrinks, bits 3/4 grow the window by three lines via `$82:8F14`; `$1566` is cleared), `$1567` -> `$82:97A6` (menu sprites `$11E8-$11FF`: add the next offset from `$8E:E4D8` indexed by `$1208,x` to `$13E8,x` until the terminator `$AA`; clears `$1567` when all are done). Verification: 16,384/16,384 whole-function cases with the MMIO order compared (all return natively), 8,704/8,704 bridge/ABI cases, mutations 25/27 with the NMI tree below (survivors: the dead X of `$82:8DD2`, restored by PLX, and the idle-sprite case before the seed fix). Runtime-bound through `Lufia2DecompBridge_939C`. |
| `$82:8B4B` | `Lufia2MenuButtons` | `verified` | Menu input: 4 checks of `$4A` against held `$46` into `$14AB`, 8 of `$4B`/`$47` into `$14AC`, each consumed with `TRB`; carry set when nothing was pressed. Verification: 16,384/16,384, 8,704/8,704, mutations 3/3. Runtime-bound through `Lufia2DecompBridge_8B4B`. |
| `$82:9313` | `Lufia2MenuWindowRequest` | `verified` | `$156A` gate; the window upload setup (`$0564`, `$5F`, Y/X) is native, the `JSL $80:8878` upload hands off at `$82:9327`. Verification: 16,384/16,384, 8,704/8,704, mutations 2/2. Runtime-bound through `Lufia2DecompBridge_9313`. |
| `$82:C627` | `Lufia2MenuCursorBlink` | `verified` | Cursor blink: `$09C8` and `$1553` gates, `$1554` counts to `$20`, then `$1552` toggles. Verification: 16,384/16,384, 8,704/8,704, mutations 2/2. Runtime-bound through `Lufia2DecompBridge_C627`. |
| `$83:85DC` | `Lufia2FieldReloadSetup` | `verified` | Field reload (`PHP ... PLP; RTL`, 12 callers): NMI off, forced blank, the field state resets (`$0562/$0563/$0583`, `$6A/$6F/$72-$74/$81`, `$1254-$1256`, `$1261/$1262/$1269`, `$099B/$099C/$09A6/$09AD`, `$17AC`, `$7F:D0B0/$7F:D0FF/$7F:D4F5`, `$420C`) are native; the map loading from `JSR $B062` on hands off exactly at `$83:8637`. Verification: 16,384/16,384 whole-function cases with the MMIO order compared, 8,704/8,704 bridge/ABI cases, mutations 10/10. Runtime-bound through `Lufia2DecompBridge_85DC`. |
| `$83:83A0` | `Lufia2FieldMenuRequest` | `verified` | Field loop (JSR at `$83:8077`, every frame): `TRB $05B5` bit 6, then `$09A7` bit 1 with the `$9080` buttons in `$46`/`$4A`. The menu itself (stack reset, `JMP $ACEF`) hands off exactly at `$83:83BD`. Verification: 16,384/16,384 whole-function cases, 8,704/8,704 bridge/ABI cases, mutations 5/5. Runtime-bound through `Lufia2DecompBridge_83A0`. |
| `$83:867B` | `Lufia2FieldTakeButtons` | `verified` | `AND $46`, `TRB $4A` on a hit; JSR'd from the field loop and the controllers. Only its M1X0 variant was compiled, so M1X1 callers entered the interpreter. Verification: 16,384/16,384, 8,704/8,704, mutations 2/2. Runtime-bound through `Lufia2DecompBridge_867B`. |
| `$83:8103` | `Lufia2FieldStatusRequests` | `verified` | Field loop (JSR at `$83:808F`): `$09A8` bit 3 gate, then `$05B7` bits 1/2/0; a set request hands off exactly at its call (`$83:8113`, `$83:811D`, `$83:8127`). Verification: 16,384/16,384, 8,704/8,704, mutations 3/3. Runtime-bound through `Lufia2DecompBridge_8103`. |
| `$82:E746` | `Lufia2TitleStateDispatch` | `verified` | `LDA $30; JSR $8028` inline jump table (9 handlers `$82:E75D-$82:E893`, all ending `RTL` with M1X0). Native up to the `JMP ($0060)`, including the `$5D-$61` writes and the stack residue; hands off at the handler. Verification: 16,384/16,384 whole-function cases (the ROM passes `E746`/`E748` once more when the table yields those, counted), mutations 3/3. Runtime-bound through `Lufia2DecompBridge_E746`, together with `$80:9C72`. |
| `$80:9CB8` | `Lufia2TextEngineStep` | `verified` | Text and event script engine step, reached from `$80:9C72` when `$099B` bit 7 is set and JSL'd from 10 more sites. Native: the `$7F:D0FF` print delay, the `$1259` text return stack, the next byte from `[$09B9:$09B7]`; plain characters (`$80:BCE4`, `$80:C0B7` next byte with bank step past `$FFFF`, `$80:BD38` glyph draw via `$80:C7C2` from font `$9A:F970` with attributes `$80:C815` into `$7E:[$09B1]`, the 32-byte VRAM upload setup, the `$84:8766` typing sound); script mode (`$099B` bit 0 clear): `$80:C1FD` window close (`$099C` bit 0 set and `$09A7` bit 1 clear: `$84:8328` clears the window buffer `$7E:3000-37FF`, clears `$099C` bit 0, `$059C-$059F` = 00 00 FC FF, `$74` = 8), the opcode fetch `$80:9D31` (table `$80:CA14`, 216 handlers) and opcode `$33` (`$80:A80F`, wait for actor `$1269`: while `$0622,x` & `$88` it steps the pointer back with `$80:C0EC` and returns; when done `$1269` = FF, two argument bytes are skipped and the next opcode follows in the same frame); since T3 also `$37` (`$80:B2EB`, wait until the frame counter `$42` reaches the argument; `$099B` bit 5 marks a running wait), `$3C` (`$80:B397`, wait until no party actor `$0622-$0626` has bit 3, then lock them with bit 2 or free the follow slots `$09A1-$09A5`; `$1269` negative hands off at `$80:B3DF`), `$00/$42` (`$80:9D4C`, return from a sub-script to `$1252/$1254` and reload at `$80:9CD9`; without a caller it hands off at `$80:9D69`) and `$68` (`$80:BC3D`: with `$05B3` bit 4 set it skips two bytes; since T7 the other path is native too: `$80:BC98` gives actor `$A7` the id and state bytes `$05FA/$05D2` and the position of map entity id - `$4F` (`$80:C01D`: `$7E:F010` list, stride 8, through the shared `$80:BFAA` search, exported as `Lufia2FieldListSearch`, into `$120A-$1212`; `$80:C1A7` clears `$0736` and `$0622` bit 2), then `$A7` steps; a list search over 65,536 entries hands off at `$80:BFBC`); since T8 `$2E` (`$80:A679`: hide listed actor id n via `$80:BF92`, `$0622` bit 2, occupancy `$83:FA12`, ids `$10-$4F` also set bit 7 of `$081E + id - $10`) and `$4B` (`$80:B849`: music n into `$099D`/`$7F:D0FD`; a change with `$099C` bit 6 clear hands off at `$80:B862`, the `JSL $80:93FE` to the APU); since T9 `$01` (`$80:9DB3`: wait for a button via `$099B` bit 1, typing sound from the speaker `$09AC`'s `$1291` through `$80:C1DF`), `$0B` (`$80:9F37`: choice cursor, re-run each frame until A/X picks row `$126A` from the word table `$126D:$126B` or B closes the window; cursor tiles `$80:9FE7/A019`) and `$69` (`$80:BCBC`: window mode `$09A7`; bit 1 copies the window palette `$A6:BFE0` to `$0500` (MVN) and hands off at the frame wait `$80:BF0B`). T9 mutations 25/25 (seeds: choice rows 1-4, speakers `$FE/$FF` or listed in `$05FA`, line starts off the stack page); since T10 `$38` (`$80:B30C`: wait n seconds, `$42` frames to 60, `$125F` seconds, re-run each frame through `$099B` bit 5; 8,657 hand-offs in gameplay6) and the waiting form of `$41` (`$80:B4C4` with `$FF` as second byte: until view `$05AA`'s `$121E/$1226` reach `$7F:D08B/D08D`; the scroll modes hand off at `$80:B4FE`), mutations 12/12; since T5 also dictionary words in text mode (bytes >= `$80` via `$80:9D22`, capped at 4,096 per step with an exact hand-off there), `$03` new line (`$80:9DDB`: `$1250` += `$400` into `$09B1`, `$099C` bit 4, column `$09B3` = 0), `$05/$06` sub-script call (`$80:9E54`: return point `$1252/$1254`, entry from the `$8E:EA00` table, page `$09B0` 0/1, 2 for words), `$0F` period plus new line, `$15` goto if event flag set (`$80:BE1E`/`$80:BE30`: flag n is bit `$80:BE45[n & 7]` of `$077E + n/8`), `$1A/$1B` set/clear event flag, `$1C` goto (`$80:A3C6`: script base `$099E/$09A0` plus a word, `$80:C102` moves to the next bank below `$8000`), `$1D` byte into `$079E + n`; since T6 also `$0C-$0E` (?, !, comma plus new line), `$09` (print the name buffer `$00:0BAD` as a sub-script), `$1E` (add into `$079E + n`), `$22/$26` (gold `$0A8A-$0A8C` plus/minus a word, capped at 9,999,999 or undone below zero, `$80:BF12/BF43`), `$27/$2A/$4F/$5F` (skip 2/3/1/1 argument bytes), `$3E/$3F` (brightness `$0583` = `$80`/`$0F`), `$47/$49/$4A/$52/$74/$7C-$80/$CB` (store the argument byte, table `kTextByteStores`), `$50` (typing sound off), `$57` (wait while `$1261` has an effect), `$5A/$8A` (start/stop the shake, `$7F:D07E-D084`), `$60` (write a PPU register `$2100 + n`), `$71` (wait `$7F:D0FC` frames), `$76` (wait for `$0581` and fade bits 0/1), `$94-$96` (COLDATA fade setup `$80:AF77`, rate from a `$4204` division), `$AA`, `$B5`, `$C1`, `$C5` (stop the COLDATA and palette fades), `$CC`. Other opcodes hand off at the `JMP ($CA14,x)` `$80:9D3B` (the result counts the `$33` passes before it), `$1269` negative at `$80:A834`, dictionary words at `$80:9D22`; since T11 the `$80:BCF2` call to C56E is native. Opcode `$08` measures text via C652 and runs the C23D glyph-clear/upload prefix, then exposes C274 (before C2A1) or C279 (before C305), retaining all frames. C23D was draft at T11 and is verified since T12. T11: 49,152 standalone helper cases, 16,384 whole-engine cases (return 8,350, LLE 8,034 in the isolated run), 19/19 meaningful mutations caught. Verification: 16,384/16,384 whole-function cases with the MMIO order compared (return 7,033, LLE 9,351 after T6), 8,704/8,704 bridge/ABI cases, mutations 16/17 for T2 (survivor: REP before the 16-bit ASL, equivalent in the model), 15/15 for T3, 20/20 for T5 and 20/21 for T6 (survivor: `$CC` stores unconditionally, equivalent because the compare only runs with `$0B62` = 0); the 9C72 cases cover the step from the field loop. Runtime-bound through `Lufia2DecompBridge_9CB8`. |
| `$86:9EDD` | `Lufia2WorldMapRegionSearch` | `verified` | World map region search (JSR from `$86:9E65` on every cell step and `$86:9796`): `$86:9F35` looks up the data bank and list for region `$09EB` through `$86:CE36` and `$CF:FCBC/FCBE`, then 9-byte rectangles are tested against `$58/$5A` until an entry with bit 7; carry clear on a hit. The world map list has 79 entries, about 1,060 interpreted instructions per search on the world map save state. Verification: 16,384/16,384 whole-function cases (seeded with cells inside the world map's rectangles), 8,704/8,704 bridge/ABI cases, mutations 6/6. Runtime-bound through `Lufia2DecompBridge_9EDD`. |
| `$86:99BF` | `Lufia2WorldMapStreamEdges` | `verified` | World map edge streaming, JSR from the `$86:924C` main loop every frame. When the camera cell `$11F2/$11F4` moved since `$11F6/$11F7`, streams the new column (`$86:ACFE` into `$7F:DF00/$7F:DF80`, VRAM address `$1714`) and/or row (`$86:AC6C` into the `$7F` buffer at `$0B`, VRAM address `$1712`), 64 cells each through the metatile tables `[$DF]`/`[$E3]` and the tile tables `[$E7]`/`[$EA]`/`[$ED]` (helpers `$86:ADEE` cell offset, `$86:AE05` block pointer), flags `$1711/$1710` for the NMI, counts moves in `$11E3` and remembers the cell (`$86:9A44`). The ROM restores the pushed `$5A` into `$58` in `$86:ACFE`; that is kept. Verification: 16,384/16,384 whole-function cases, 8,704/8,704 bridge/ABI cases, mutations 22: 19 caught, 3 equivalent (bit 7 of the `$86:ADEE` offset is always clear and its `ASL` never carries, so two VRAM masks and the `ADC #$0000` cannot differ). Runtime-bound through `Lufia2DecompBridge_99BF`; it had no compiled variant. |
| `$86:CEF6` | `Lufia2WorldMapNmiUploads` | `verified` | World map NMI through the `$00:0067` vector; `PHP`/`PLP`, so any entry width. Forced blank, the `$86:D1A1` tile upload list (`$1365` entries at `$1367`, column uploads `$86:D1E8` and two-pass block uploads `$86:D271` on DMA channels 6/7), the `$1710`/`$1711` VRAM uploads, the `$1702` CGRAM upload, the `$16E7` palette cycles (five bytes each at `$16E8`, colours from `$0320`), the Mode 7 matrix from `$1707-$170E` or the HDMA channel setups (`$11DA/$11DD/$11DF/$11E1`, window tables `$D400-$DA00`), `$420C`, the Mode 7 centre and the BG scroll copies. Verification: 16,384/16,384 whole-function cases vs ROM comparing the complete MMIO write sequence (8,456,905 writes; the verify bus log now holds 16,384 writes), 8,704/8,704 bridge/ABI cases, mutations 33/33. Runtime-bound through `Lufia2DecompBridge_CEF6` (new `recomp/bank86.cfg`). |
| `$83:A21A` | `Lufia2FieldActorSprites` | `verified` | Field OAM builder from the field loop (JSL at `$83:8088`). Clears the previous frame's entries at `$0140` and the `$0302-$0310` size bits, picks the camera (`$1220/$1228` or `$7F:D0EE/$7F:D0F0` when `$1261` bit 6), insertion-sorts the 72 actor slots inside the camera window by Y into `$E200/$E300`, then per sorted actor: object slots (`>= $28`) reload their sprite through `$83:A492`/`$83:FCD1` (and a linked child via `$7F:E37E`), actors queue a new animation frame through `$83:AAE5` (`$4202/$4203` row offset), screen position, attribute bits in `$97`, and the `$83:A48A` size handlers (`$83:A4A2` 16x16, `$83:A4BF` tall, `$83:A501` wide, `$83:A534` 32x32 with the four `$83:A589` layouts) with the `$83:A669` high-table bit. A direct page with a non-zero high byte sends the `$A48A` index out of range; that case hands off exactly at `$83:A461`. Verification: 16,384/16,384 whole-function cases vs ROM (RTL 11,715, LLE 4,669), mutations 41/41. Stock entry remains exact. `Lufia2FieldActorSpritesWithVisibility` shares the same collection, sort, uploads and OAM with an optional consumer-owned horizontal padding and pure world-point filter. The recomp consumer binds A21A through its tested bridge and supplies widescreen policy; no renderer or room state lives in this library. The former hand HLE and A321 continuation are retired. |
| `$85:8A2F` | `Lufia2BattleSprites` | `verified` | Battle sprite builder, JSL'd from the battle loop and from about 45 battle menu sites. `$15AB` picks the layout. Layout 1 runs `$85:8B4B`, `$85:8BC0` and `$85:8C27`, then either `$85:8D2E` (party tilemap at `$7E:2800`: WMDATA fill, `$4202/$4203` row offsets, `$81:BE58` blocks) or `$85:972E` (15 rows of tile ids from `$3710`). Layout 2 runs `$85:8B4B`, `$85:8BC0`, `$85:8C27` and `$85:8C98`. The OAM strips `$81:BD4B`/`$81:BDCC` (five bytes per sprite, mirrored variant for attribute bit 6) are native, including their JSL/JSR wrapper frames. All three RTL exits are reported exactly. Only the M=1 X=0 entry is native; other widths hand off at the entry. Verification: 16,384/16,384 whole-function cases vs ROM with the complete MMIO write sequence (2,578,813 writes; the verify bus now models WMADD/WMDATA), 8,704/8,704 bridge/ABI cases, mutations 41: 40 caught, one equivalent (carry into `$81:BE90` can never be set here, the attribute byte is at most `$3F`). Runtime-bound through `Lufia2DecompBridge_8A2F`. |
| `$85:ECF0` | `Lufia2BattleFrameUpkeep` | `verified` | Per-frame battle upkeep from the `$81:8877` loop: `$85:91A1` refreshes the five `$147A` state bytes, `$85:8A2F` sprites, `$85:9265` clears `$7F:F44E-$7F:F749`, clears `$1B8B-$1BAF`, saturating `$11E8` counter, clears bit 0 of `$10` in every entity from the `$0A64` and `$0A6E` lists, and counts down the `$14E6` timers. Measured on the battle save state: 5,432 interpreted instructions per frame (2,548 sprites, 2,299 memset), the hottest battle routine. Verification: 16,384/16,384 whole-function cases (2,525,900 MMIO writes compared), 8,704/8,704 bridge/ABI cases. Runtime-bound through `Lufia2DecompBridge_ECF0`. |
| `$85:8DC5` | `Lufia2BattleNmiUploads` | `verified` | Battle NMI work, reached through the RAM vector `$00:0067` (JSL from the NMI handler). `PHB; PHK; PLB`, X16, RTL at `$85:8E97`. Native: `$85:8E98` sixteen queued VRAM DMA uploads (`$1A8F` slots, stride 6, channel 6, `$420B`), BG scroll copies `$0594-$059F` to `$210D-$2112` when `$15B3` is set, `$85:8ED2` HDMA channel latches from `$1AEF` (5 bytes per channel, mask `$D8/$D9/$DA`, `$420C`), window/colour registers from `$123C-$124B`, `$1252/$1250` shadows unless `$1268`, `$2100` from `$0583`, and the `$85:8F1B` timer loop (eight slots at `$1B17`, handlers through the `PEA #$8F3B; PHA; RTS` trick on the `$85:9E05` table, including its stack residue). Handler 1 `$85:A8E7` (battle HDMA wave table at `$7E:40CC` from `$85:9FFA`, or three flat bands when `$1B23` is set) is native; every other handler and a non-empty task queue `$12E3` hand off exactly (handler entry with the synthetic frame on the stack, `$85:8E7F`). M=0 callers hand off at the entry. Measured on the battle save state: 837 interpreted instructions per frame, 540 of them in handler 1. Verification: 16,384/16,384 whole-function cases vs ROM comparing the complete MMIO write sequence (1,162,417 writes), 8,704/8,704 bridge/ABI cases. Runtime-bound through `Lufia2DecompBridge_8DC5` (new `recomp/bank85.cfg`). |
| `$83:BB93` | `Lufia2UpdateActorSlots` | `verified` | Field-loop actor traversal, `JSL` from `$83:807D` (M1X1, DB $83), RTL at `$83:BBF2`. Portable `Lufia2UpdateActorSlots` on the shared CPU model: `$7F:D0FE` -> `$7F:E216` = 1/3 around the loop, 40 slots with `STZ $A7` / AB4F, `$0622` bit 2 skip, bits 3-4 or `slot != 0` -> C7F8 unless `$7F:D0A1 & $2C` (leader still runs), bit 0 skips the primary child, slot 0 -> BBF3, D508 for every non-skipped slot, `SEP #$30` after the children, `$09A1` bit 7 cleared unless `$FF`. Children run through a JSR callback (`Lufia2ActorSlotChild`, sites `$83:BBB9/$83:BBC9/$83:BBCC`), so the runtime keeps each child on its own binding and child boundaries stay local. A D flag left by a child stops at `$83:BBA1` (AB4F's ADC would run BCD; the portable ADC is binary only). Verification: 2,048 whole-function cases with children in interp816 against a full ROM run, 2,560 bridge/ABI cases (JSL frame, paired/dispatched/mismatched return, D handoff, LLE entry, child unwind propagated one level, JSR site checked against the ROM). Cases whose random child script never returns are counted and skipped (the ROM loops forever there). Runtime-bound through `Lufia2DecompBridge_BB93`; replaces the callback draft in `src/actor_update.c`. |
| `$83:BBF3` | `Lufia2PlayerSlotSpecialUpdate` | `verified` | Original-ROM differential verification passed 65,792 semantic cases and 12,288 consumer bridge/stack boundary cases with zero mismatches. |
| `$83:C1B4` | `Lufia2PlayerSlotStandardUpdate` | `verified` | Leader controller, called by BBF3 with `JSR` at `$83:BC22` (M1X0, DB $83, DP 0 in the field loop). Whole function through its RTS (`$83:C1E2` early or `$83:C245` after `SEP #$10`, so exit X width is path-dependent) or an exact LLE boundary (`resume_pc`). Native: `$099B` gate, `$83:867B/$83:8674` pressed/latch tests, `$83:BA06` talk-target search (F988, `$BA54` probe table BA5C/BA80/BA96/BAAC with ledge steps, BAC2 actor scan incl. its duplicated wide-column test), `$83:C161` D-pad (`$83:D437`, `$83:C1B0`; TDC keeps DP high in the index), `$83:FC56`, C0EF/C246, `$83:FBBD` (`$FBC6` edge table, FBF1/F9B6), `$8E:BBAF` vehicle-tile test (FB71/FB12), the `$83:C219` step probe (FB12 + F9AD), `$83:C206` blocked turn (`$22 + $18` into D350 with X8), `$8E:B63B` door rectangles at `$7E:F000` (15-byte entries, second rectangle optional), `$83:FC69` walk speed, C232 D350 call (X16 via C1EB, X8 via C230; D350 stops at `$83:D38D` under X8), `$83:C0FA`, `$05B5` bit 3. Exact LLE boundaries: `$83:C1CC` (JSR 83E0 -> field loop), `$83:C1D7` (JSR BB08 dialog), `$83:C1DC` (`JSL $8E:C05F`), `$8E:BBD1` (vehicle boarding), `$8E:B6D4` (door transition via `$80:C12E`), `$83:D38D`/`$83:D370` (inside D350), unknown `$FBC6`/`$BA54` targets (`$83:FBC2`/`$83:BA1C`). Verification: 16,384 whole-function cases vs ROM (random WRAM, DP $00/$20/$400, DB $83/$00/$80/$7E, X16/X8 entry), 8,704 bridge/ABI cases. Runtime-bound through `Lufia2DecompBridge_C1B4`. |
| `$83:C7F8` | `Lufia2ActorPrimaryUpdate` | `verified` | Whole function through its RTS (`$83:C83B` or `$83:C8D3`) via `Lufia2ActorPrimaryUpdate`: front-end incl. the `$83:C808` `$1291` countdown, `$83:C83C..C866` dispatch and the handler chain. Portable front-end reconstructs the gates/timer path and `$83:C83C..C866` VM dispatch. Native handler semantics now include the direct D350 action cluster at `$83:C867/$83:C86B/$83:C86F/$83:C873/$83:C877/$83:C87C/$83:C880/$83:C884/$83:C888`, `$83:C8C7`, `$83:C891` (via D3F7), `$83:C8AF/$83:C8D4` (via `$80:8299`), `$83:C8EE`, `$83:C8FC/$83:C90A`, `$83:CBB7` (via F9D4), `$83:CBE1`, `$83:CC1B/$83:CC2E`, `$83:CC41/$83:CC63` (via D350), `$83:CAB9`, `$83:CCF0/$83:CD0D`, `$83:CD2E`, `$83:CD32`, `$83:CD4F`, `$83:CD92`, `$83:CE73`, `$83:D125`, `$83:D132`, `$83:D320` (via `$80:82C7`), `$83:D340`, `$83:D176`, `$83:D188`, `$83:D196`, `$83:D1C1`, `$83:D1D0`, `$83:D1E6`, `$83:D210` and `$83:D2E6/$83:D2F6/$83:D30B` (via `$83:D27F`), `$83:D293`, `$83:C98A/$83:CCD7` and `$83:D01E` (via `$83:C9C5`), `$83:CA19/$83:D09A` (via `$83:D0AA`), `$83:CF6E`, `$83:CF8C`, `$83:CFB9`, `$83:D112`, `$83:C918/$83:CAD3/$83:CBB1` (via C947), `$83:CA29` (via CA68), `$83:D135` (via A746), `$83:D1B5` (via `$84:8766`), `$83:D1FE/$83:D207` (via FACB/FA81), `$83:CDA5`, `$83:CE7D/$83:CF1A` and `$83:D03F` (via the passability helpers `$83:D89E`, `$83:F988`, `$83:F9AD`, `$83:CDF6`, `$83:CE3B`, `$83:CEA8`), `$83:CC85/$83:CCA3`, full `$83:D14D -> D350 -> C8C7`, `$83:D2B4/$83:D2BD`, and `$83:D2C4/$83:D2D5`. Absolute-indexed reads carry into the next bank, matching the CPU. The portable CPU state now carries V/D/I so PHP/PLP round-trip exactly; ADC/SBC are binary-mode only. Handlers without native semantics (opcodes `$49` CAE1 and `$4E` CB71 via `$83:AAAF`/`$84:8204`, BRK targets for opcodes `$04..$07`/`$14`, table reads past opcode `$4F`) and scripts that never yield (65,536 handlers) stop at an exact ROM PC (`resume_pc`: handler entry, `$83:D370`, `$83:FB17`, D89E/CD4F table targets, `$83:CDFD`, `$83:D0C7`) where the consumer finishes in LLE. `$83:CE41` is unreachable (D89E rejects the same index first). Verification: 16,384 whole-function cases (random WRAM, 8/16-bit X entry, DP incl. `$0400`, short and chained scripts) compared to the ROM at the RTS or at the k-th dispatch/resume PC; 2 x 2,048 resume-exact cases; bridge: 8,192 call-boundary + 512 LLE-entry cases. Runtime-bound. |
| `$83:A746` | `Lufia2ActorSyncFinePosition` | `verified` | 1/16 position `$7F:DDAE/$7F:DE3E` from tile coordinates. Whole-function differential: 4,096 random-WRAM cases from entry to the RTS/RTL, full CPU state and WRAM compared. Not runtime-bound. |
| `$83:C947` | `Lufia2ActorPrimaryReset` | `verified` | Occupancy (FA3F), flag reset, `$7F:E4DE = 8`, script reload (D416). JSR/RTS. Whole-function differential: 4,096 random-WRAM cases from entry to the RTS/RTL, full CPU state and WRAM compared. Not runtime-bound. |
| `$83:CA68` | `Lufia2ActorBlockedEvent` | `verified` | Blocked-step event record at `$7F:DFAE/$7F:DEEE` indexed by `$1724`, gated by `$7F:D0A1` bit 6. Whole-function differential: 4,096 random-WRAM cases from entry to the RTS/RTL, full CPU state and WRAM compared. Not runtime-bound. |
| `$83:CB65` | `Lufia2ActorClearSlotLinks` | `verified` | `$09A1..$09A5 = $FF`. JSR/RTS. Whole-function differential: 4,096 random-WRAM cases from entry to the RTS/RTL, full CPU state and WRAM compared. Not runtime-bound. |
| `$83:D416` | `Lufia2ActorLoadPrimaryScript` | `verified` | Primary script pointer from `$91:A1D4[$070A*2] + $A1D4`, bank `$91`. Whole-function differential: 4,096 random-WRAM cases from entry to the RTS/RTL, full CPU state and WRAM compared. Not runtime-bound. |
| `$83:FA3F` | `Lufia2ActorMarkMapOccupancy` | `verified` | Sets bit 0 in the `$7E:4000` map at the actor tile (and the next column for wide actors except `$05D2` = `$71..$73`). Whole-function differential: 4,096 random-WRAM cases from entry to the RTS/RTL, full CPU state and WRAM compared. Not runtime-bound. |
| `$83:FA81` | `Lufia2ActorMoveFinePosition` | `verified` | Adds signed operands to the 1/16 position and rounds back into `$06BA/$06E2`; exits M=0 with A = `$2A + 3`. JSR/RTS. Whole-function differential: 4,096 random-WRAM cases from entry to the RTS/RTL, full CPU state and WRAM compared. Not runtime-bound. |
| `$83:FACB` | `Lufia2ActorAddDisplayOffset` | `verified` | Adds signed operands to `$7F:DC8C/$7F:DD1C`; exits M=0 with A = Y + 3. JSR/RTS. Whole-function differential: 4,096 random-WRAM cases from entry to the RTS/RTL, full CPU state and WRAM compared. Not runtime-bound. |
| `$84:8766` | `Lufia2QueueDeferredSound` | `verified` | Stores A to `$17AC` (later sent via `$80:953B` to APU port `$2140`) unless `$05B6` bit 1 is set. Whole-function differential: 4,096 random-WRAM cases from entry to the RTS/RTL, full CPU state and WRAM compared. Not runtime-bound. |
| `$83:D350` | `Lufia2ActorPrimaryActionCore` | `verified` | Common primary-VM action/movement helper, full body through RTL `$83:D3AE` including D3B7..D3E5, FB12, F9D4/F9F7, FB71 and D3F7. Consumer bridge verified against the original call boundary: 6,144 random-WRAM cases (paired JSL/JSR host return, hrv=0 dispatch, mismatched frame size) plus 512 LLE-entry cases, full CPU state and WRAM compared. Native ABI: M=1, D=0, native mode, 16-bit X, DP in page 0 (`TDC` feeds DP high into the D370/D385 table index); other entries run in LLE. Portable X8 contract (C1B4 calls D350 with 8-bit X at `$83:C20F`/`$83:C237`): exact through the D370 helpers, FB12 and the D39D install path; before `JSL $83:FB71` it stops at `$83:D38D` (`LUFIA2_ACTOR_PRIMARY_ACTION_X8_BOUNDARY_D38D`) because F9D4's `PHX` (1 byte) / `PLA` (2 bytes) unbalances the stack and its `RTS` leaves the routine in the ROM. Verified with 8,192 C1B4-shaped cases (X8, DB $83/$00/$7E, DP $00/$20, JSL frame from `$83:C237` checked through the RTL): 5,271 returns, 1,537 installs, 1,384 D38D handoffs. The standalone bridge keeps the 16-bit-X contract. |
| `$83:F9D4` | `Lufia2ActorResolveMapCellOffset` | `verified` | Map-cell offset via F9F7, RTS at `$83:F9ED`. Consumer bridge verified against the original call boundary: 6,144 random-WRAM cases (paired JSL/JSR host return, hrv=0 dispatch, mismatched frame size) plus 512 LLE-entry cases, full CPU state and WRAM compared. Native ABI: M=1, D=0, native mode, 16-bit X (8-bit X pushes 1 byte and pulls 2). |
| `$83:FB12` | `Lufia2ActorMovementStep` | `verified` | One coordinate step through `JMP ($FB1A,X)`, RTL at FB24/FB27/FB2A/FB2D. Consumer bridge verified against the original call boundary: 6,144 random-WRAM cases (paired JSL/JSR host return, hrv=0 dispatch, mismatched frame size) plus 512 LLE-entry cases, full CPU state and WRAM compared. Native ABI: M=1, D=0, native mode, 8- or 16-bit X; an unknown table target finishes in LLE from `$83:FB17`, like the generated code; 512 out-of-range cases verified. |
| `$83:FB71` | `Lufia2ActorReadMapCellValue` | `verified` | Map value probe via F9D4, RTL at `$83:FB8A`. Consumer bridge verified against the original call boundary: 6,144 random-WRAM cases (paired JSL/JSR host return, hrv=0 dispatch, mismatched frame size) plus 512 LLE-entry cases, full CPU state and WRAM compared. Native ABI: M=1, D=0, native mode, 16-bit X. |
| `$83:D508` | `Lufia2ActorSecondaryUpdate` | `verified` | Whole function through its RTS (`$83:D599` front-end return or `$83:D60E` after the `$83:D60D` PLB) or an exact LLE boundary (`resume_pc`, same contract as C7F8). Front-end: `$83:D512` (`$7F:E35E` countdown, period reload from the high nibble, `$7F:E316` bit 7 toggle), `$83:D550` (-> walk counter `$7F:E48E`), `$83:D585` (`$7F:DDAE[$A9]` bit 0 toggle). Dispatch `$83:D59A` and redispatch `$83:D5C3` (two-level tables `$DF17` and `$DF37/$DF57/$DF77`; `$DF77` entries 8..F are not handlers). Native handlers: `$83:D5FD` (stop via DA8A + DA71), `$83:D60F`, `$83:D615`, `$83:D626`, `$83:D6AD`, `$83:D6C7`, `$83:D6EF`, `$83:D833`, `$83:D844`, `$83:DA9A`, `$83:DEE9` (wait via `$7F:E48E` bit 7), `$83:D7B2/$83:D7CC` (loop counter `$7F:DB4C` / target `$7F:DB9C`), `$83:D80E`, `$83:D969`, `$83:DB77`, `$83:DBDC`, `$83:DAB3`, `$83:DAC3`, `$83:DAC9`, `$83:DD54`, `$83:DEBD`, `$83:DB54/$83:DB5A` (FA81), `$83:DB6D` (FA3F), `$83:D760` (`$84:8766`), `$83:D7EB`, `$83:DBC2/$83:DC04` (A746), `$83:D78C/$83:D79C` (D7A5 + FB12), `$83:D95E/$83:D992` (D99F), `$83:DB63/$83:DDFB` (FA12 occupancy clear via F9B6), `$83:DD6E` (its `JSR $FAFA` runs with M=0 and decodes as `ORA #$1000 / TSB $EB / LDA #$EBFF`, so `$7F:DC8C` receives $EBFF and DP `$EB` is ORed), `$83:DE11/$83:DE29/$83:DE5D` (DE76 orbit: `$80:8450` sine and `$80:8486` cosine of a 180-step angle, scaled by DE9F on the PPU Mode 7 multiplier; DE9F masks the table sign bit, so both offsets are non-negative magnitudes times the signed radius), `$83:D70C`, `$83:D638/$83:D63E` (D661), `$83:DB95` (D67A) with the spawn chain `$83:DF87 -> DFA5 -> AB4F/DFFD` (first free slot of 32, script pointer from `$91:8EC7`; TDC stores the DP low byte), `$83:D76E` (native unless the FB71 cell value is 9, then an exact boundary at `$83:D782` before `JSL $80:E7DF`), `$83:DC45` (walk: D87D probe into the D89E body, D9C6 occupancy move + follower trail, `$DD18` signed sub-steps, `$83:DD44` bob table). `$83:DAE9` (6 opcodes: sprite reload keeping frame/state: `$83:AAAF` frees the sprite slots via ABCC and clears occupancy via FA12, `$83:A9BA` reads the sprite descriptor from `$CF:F000` (A9E5), allocates slots with AB7C/ABE9 (AA50) and sets the animation tables `$83:ABFC/AC14` (AA7D), `$83:AA30` sets the height offset `$7F:DD1C`). Exact LLE boundaries: `$83:DDC6/$83:DDE3/$83:DDD0/$83:DDEF/$83:DDD9` (F3/F4/E7/E8/D2, tile-graphics family F5B9/F5EA/F620/F795/F7B1). Verification: 16,384 whole-function cases vs ROM, 8,704 bridge/ABI cases. Runtime-bound through `Lufia2DecompBridge_D508`. |
| `$83:A9BA` | `Lufia2ActorLoadSprite` | `verified` | Sprite load for actor `$A7` (sprite A), JSL'd with M1X1 from `$83:A78E` (the hot caller in the gameplay profile, 52 interpreter entries), with M1X0 from the secondary VM and with M0X0 from bank `$84`. `PHA` (by M), `PHX`, `PHY`, `PHB`, `PHP`; `SEP #$30`, DB `$83`; `$83:A9E5` reads the sprite descriptor from `$CF:F000`, `$83:AA50` allocates sprite slots (`$83:AB7C`/`$83:ABE9`) and sets the animation tables (`$83:AA7D`); everything is restored, RTL at `$83:A9CF`. The code already existed inside the secondary VM (`$83:DAE9`); the body is now exported. The bridge takes M=1 entries (M1X1 and M1X0) natively, M=0 entries keep falling back to the interpreter exactly as before. Before the binding only M1X0 was AOT. Verification: 16,384 whole-function cases vs ROM from `$83:A78E` (all return natively, random M/X), bridge/ABI cases with M=1 and M=0 entries, mutations 3/3 plus one equivalent (the `LDY $A9` width in `$83:AA7D`, always X8 here). Runtime-bound through `Lufia2DecompBridge_A9BA`; codegen: all four `bank_83_A9BA_M?X?` variants are HLE wrappers now, manifest unchanged (1,131 AOT / 238 lle_only). |
| `$84:8193` | `Lufia2SpriteGraphicsUpload` | `verified` | VRAM DMA of the queued sprite graphics, JSL'd with M1X1 from the sprite reload loop `$83:A7B1` (right after `$83:A9BA` for each actor; 52 interpreter entries in the gameplay profile). `SEP #$20; REP #$10`; for each of the 8 queue entries with a source word `$05C2,x` (cleared afterwards): source to `$4302`/`$54`, bank `$11D9,x` to `$4304`, VRAM address `$11E9,x` to `$2116`/`$56`, size `$11F9,x` to `$4305`/`$58`, DMA channel 0 mode 1 to `$2118` (`$4300`=1, `$4301`=`$18`, `$420B`=1); then the next row: VRAM + `$100`, source + size. Clears `$0732/$0733`; RTL at `$84:8203` with M1X0. All stores are DB-relative, as in the ROM. Verification: 16,384 whole-function cases vs ROM from `$83:A7B1` (MMIO write order compared), bridge/ABI cases, mutations 6/6. Runtime-bound through `Lufia2DecompBridge_8193`, which needed a bare `recomp/bank84.cfg`; codegen: all four `bank_84_8193_M?X?` variants are HLE wrappers, manifest unchanged (1,131 AOT / 238 lle_only). |
| `$86:88BE` | `Lufia2TitleParticleSprites` | `verified` | Title screen, JSR'd once per frame by the title loop `$86:82C8` (and the fade loop `$86:830F`): OAM entries for the 75 particle records at `$C448` (13 bytes, byte 1 = active; the first three are the title objects' sprites). Per active record `$86:88E5`: sprite list `[$0003]` in bank `$86` (size flag, y/x offsets, tile word), OAM x/y = record `$05`/`$07` minus the offsets at `$0100 + $06`, tile word = record `$0B` + list word, then the size bit and x bit 8 rotated into the OAM high table word `$0300 + ($00 >> 3 & $FE)` (`CLC; ROR` and `($00 & 15) + 1` times `ROL`); `$00` counts entries by 2. gameplay5: 6,740 + 370 interpreter entries, ~2,300 interpreted instructions per title frame. Verification: 16,384 whole-function cases (return 7,512, exact entry hand-off 8,872), seeds aim the x low byte at the list offset and the x high byte at `$00/$01/$FF`; mutations 19/19. Runtime-bound through `Lufia2DecompBridge_88BE`. |
| `$86:838C` | `Lufia2TitleObjects` | `verified` | The three title objects (`$C400/$C418/$C430`, pointers `$15BA/$15BC/$15BE` from `$86:8A41/8A47/8A4D`), each only while flag `$12` bit 7 is set: `$86:83C3` counts timer `+8` down (at 0 the sprite is hidden) and `$86:83F2` advances radii and angles (8.8, wrap at `$B400` by flag bits 0/1), places the sprite at centre + r cos / centre - r sin through the shared helpers `$80:8486`/`$80:8450` (sine table `$80:84BF`, 180 steps per turn, bit 7 = minus) and `$82:8000` (16x16 shift-add multiply into `$1576:$1574`; signs kept in `$1583`, 8-bit `ROL` then 16-bit `LSR` in `$86:8503`), and derives the depth 0-10 from the second angle (`$86:8A5B`); a new depth reloads the sprite list (`$86:8AC0`) and the angle steps (`$86:8A71`, signed by the flags, `$86:8530`). `$86:85C2` starts one free particle of the object's 24-slot pool at the sprite, `$86:856F` animates the pool (next frame every 2 ticks up to 10, lifetime 21). The math helpers live in `src/system/math.c` with their exact stack residue and restored P/X/Y. Verification: 16,384 whole-function cases (return 7,566, entry hand-off 8,818), mutations 35/36 (survivor: the `$86:83C3` JSR return byte, overwritten by the next JSR, equivalent). Runtime-bound through `Lufia2DecompBridge_838C`. |
| `$86:86ED` | `Lufia2TitleLayers` | `verified` | `$86:86FC` 13 layer positions as 24-bit fixed point (`$1587/$1597/$15A7`, steps `$86:8726`, the top byte only takes the carry) and `$86:8695` tile animation: when layer 10 (`$15A1 >> 3`) changes, frame `$15B8` (0-63) copies 30 words from `$7E:B3C4 + 2n` (stride `$80`) to `$7E:C3C0` with DB = `$7E`, `$1566` = 2; then `$1566` bit 0. Verification: 16,384 whole-function cases (return 7,715, entry hand-off 8,669), mutations 13/14 (survivor: `$1566` = 3 instead of 2, equivalent because the only caller ORs bit 0). Runtime-bound through `Lufia2DecompBridge_86ED`. |
| `$86:8996` | `Lufia2TitlePaletteCycle` | `verified` | Every 3rd frame (`$14B5`) rotate colours 1-8 of the palette rows `$04A0/$04C0/$04E0/$0500` by one (first colour saved on the stack), step `$15B9` down mod 8 and request the palette upload (`$73` = `$80`). Verification: 16,384 whole-function cases (return 7,836, entry hand-off 8,548), mutations 6/6. Runtime-bound through `Lufia2DecompBridge_8996`. |
| `$86:E8CE` | `Lufia2WorldSpriteChain` | `verified` | World map scene loop `$86:9365` (JSR'd every frame; 1,752 interpreter entries in gameplay6, frames 6,004-9,282): the shake `$1E50/$1E52` decays by `$1E54/$1E56` (`$1E54` -= 4, `$1E56` -= 1 each frame) into `$15/$17`; past frame `$4AA` (`$42`) the chain drifts; angle `$1248` = `$1211` - `$1219` + `$80` (+ random 0-4 from `$80:8299`), radius 3-4; `$86:A417` turns angle and radius into a 24-bit step (`$97:B226` quarter sine, `$86:A583` multiply through `$4202/$4216`, quadrant handlers `$86:A473/A496/A4AD/A4BC`); then 22 OAM entries (`$0100`), each one step (`$09/$0C`) from the previous plus its wobble `$1E5A/$1E5B`; y in `$E0-$F7` hides a sprite (tile `$3C`); the head's attribute blinks. Verification: 16,384 whole-function cases (return 7,707, entry hand-off 8,677), bridge/ABI cases, mutations 25/25. Runtime-bound through `Lufia2DecompBridge_E8CE`; codegen: manifest unchanged (1,131 AOT / 238 lle_only). |
| `$81:F9E9` | `Lufia2PartyExperienceForLevel` | `verified` | Experience needed for level `$09FE` of member `$09FA` (JSL'd from `$81:EE37/EF36/F988/F9CA` and `$85:CC02`, M1X0, DB = `$97`): the step `$58-$5A` starts at the member's base word `$97:B99E` (bytes swapped), every level adds the step to the sum `$63-$66` and grows it by the factor (`$97:B633` + member * `$70`, + 1, a new factor every 8 levels) through `$4202/$4203`; the result is the sum minus 10 in `$0A2A-$0A2C`, 9,999,999 from level 98 on. The AOT body checks the frame deadline at entry and in the loop and unwinds to the interpreter when it is reached, so in busy frames (status screens) the curve ran interpreted: about 4 million interpreted steps in profile 8 (`$81:FA42-FA9C`, ~130,000 visits each). The bound native body runs to the end. Verification: 16,384 whole-function cases (return 7,647, entry hand-off 8,737; seeds keep level 1-99 and `$09FF` = 0, since level 0 loops 65,536 times and overflows the MMIO log), bridge/ABI cases, mutations 16/17 (survivor: `INC $57` 8 vs 16 bit, equivalent because `$57` is the high byte of an 8x8 product, at most `$FE`). Runtime-bound through `Lufia2DecompBridge_F9E9` with a new bare `recomp/bank81.cfg`. |
| `$80:ED9C` | `Lufia2FieldBuildAttributes` | `verified` | Map cell attributes `$7E:4000` at map load (JSL'd from `$80:A2B3/B725/B761`, `$83:8664`, `$8E:B0FD/B2E2/BB97` and JSR'd from `$86:995F`): width/height of map A from `$7F:D010/D018` into `$05B9/$05BB`; per cell the tile class of its metatile (`$7F:[$D03E]`: bits 4-7 set -> 8, 8 -> 2, 9 -> `$80`) plus bits 2-3 of the cell's high byte as bits 4-5; `$80:EF2F` sets bit 0 under the 40 map actors not hidden by `$0622` bits 1-2 (size 2+ and not kinds `$71-$73` also the next cell; leaves DB = `$80`); bit 0 under the `$7E:[F026]` list; `$80:EEEB` bit 2 under list `$1E` (leaves DB = `$7E`); bits 3 (and 6 for size 2, one row lower) under the 48 map objects `$7F:D69C/D6CC/D6FC` found in list `$16` (`$80:BFAA`, cell via `$83:F9A5`). AOT already, but ~2.4 million interpreted steps in profile 8 after deadline unwinds during map loads; the bound body runs to the end. Verification: 16,384 whole-function cases (return 15,399, entry hand-off 985; seeds: maps of 1-32 cells a side, terminated lists), bridge/ABI cases, mutations 25/25. Runtime-bound through `Lufia2DecompBridge_ED9C`. |
| `$80:8E9D` | `Lufia2DecompressResource` | `verified` | Resource decompressor, JSL'd from 55 sites (map data, graphics, text): resource `$54` from the 3-byte table `$A7:8000` (15-bit address with bit 15 set, bank `$A7` plus the bits above); the stream starts with the unpacked length (`$58`), the destination is `$7E:[$60]` or, with `$62` bit 0, `$7F:[$60]` (DB = that bank). A control byte (`$65`, 8 flags in `$66`, bit 7 first) precedes each group; bytes below `$80` are literals and take no flag, bytes from `$80` take one: 0 literal, 1 back reference (`$80:8F40`): low nibble of the next byte non-zero is the short form (12-bit offset, nibble + 2 bytes), zero the long form (14-bit offset, `(m & $3F) + 3` bytes); the copy is an `MVN` inside the destination bank (`$80:8F5C`). The stream continues at `$8000` of the next bank. Ends when X reaches `$60` + length. Resource 680 has length 0 (the original loop would wrap the whole bank), so the seeds leave it out. AOT, but the deadline checks in its loop sent it back to the interpreter in busy frames: the largest interpreted block in profile 8 (bank 80 ~31M steps, most of it `$80:8EF4-8F66`); the bound native body runs to the end. Verification: 16,384 whole-function cases (all 680 real resources, both banks, return 16,384), bridge/ABI cases, mutations 29/29. Runtime-bound through `Lufia2DecompBridge_8E9D`. |
| `$80:8878` | `Lufia2MenuDrawString` | `verified` | Menu string byte code, JSL'd from about 150 sites (menus, shops, status, battle windows): string at `$5F:Y`, tilemap `$7E:X` (DB = `$7E`), 32 characters a line (`$0565`/`$0566`; a wrap moves two rows). Bytes from `$10` are tiles with attribute `$0564`; from `$CC` a two-row glyph from `$80:8E16`/`$80:8E49` unless `$04` asked for a plain tile. Ops `$00-$0F` (`$80:8E7C`): `$00` end; `$01` number (format bits 7-5 size 1-3 bytes, 2 signed, 3 hex, 1 keep leading zeros, 4 left-justify; 8 decimal digits by powers of ten `$80:8A57` into `$0567-$056E`); `$02` nested strings one row lower (`$80:8BCA`: pointer, (pointer), long, (long)) or member/item/spell names (`$0A80` table, `$81:F1C5` item icon and name, `$81:F414` spell name); `$03` branch (equal or any common bit, `>=`, `<`, always; operands `$80:8C9E`); `$04` raw tile; `$05`/`$06` cursor mark and mark + n; `$07`/`$08` save and restore the palette; `$09` attribute; `$0A` new line; `$0B` position; `$0C-$0E` palette by value, table or pointer. The unused ends of the three jump tables (`$02` 10-127, `$03` 4-7, and a raw control byte dispatched on the attribute) hand off exactly at their `JMP (table,X)` with the visit count. AOT already. Verification: 16,384 whole-function cases over generated strings with every op, two-deep nesting and forward branches (return 16,066, hand-off 318), bridge/ABI cases with any M, mutations 33/34 (survivor: a JSR frame slot that the next JSR always rewrites). Runtime-bound through `Lufia2DecompBridge_8878` (any width). |
| `$81:F1C5` | `Lufia2LoadItemRecord` | `verified` | Item `$0A06` (& `$1FF`): clear `$0B77-$0BAC`, name from `$9E:C7E8` + 12n with trailing spaces cleared (`$81:F2A9`), record pointer from `$96:CF69` (`$81:F291`): 9 bytes to `$0B84`, 4 mask bytes to `$09FC-$09FF`, then one word per set mask bit into the 32 word slots after them. 467 items. Verification: 16,384 whole-function cases, bridge/ABI cases, mutations 9/11 (survivors rewrite bytes that are always written again: `$0B77` and a JSL frame slot). Runtime-bound through `Lufia2DecompBridge_F1C5` (any width). |
| `$81:F414` | `Lufia2LoadSpellRecord` | `verified` | Spell `$0A0B` (& `$FF`, 40 spells): record pointer from `$95:FA5B` (`$81:F446`), 8-character name, a terminator that is the low byte of D (`TDC`), 11 record bytes to `$0B77-$0B8A`. Verification: 16,384 whole-function cases (M1X0), bridge/ABI cases, mutations 8/8. Runtime-bound through `Lufia2DecompBridge_F414`. |
| `$82:810E` | `Lufia2MenuDrawWindow` | `verified` | Menu window frame (JSR'd from 55 menu sites, M0X0): A = tilemap offset in `$7E:2000`, X = width << 8 | height (tiles); interior `$080F`, edges and corners from the pattern pointers at `$82:827E` (`$82:81E6` row, `$82:820B` 2-row corner, `$82:8230` column, `$82:8259` 3-row corner for height 3). Verification: 16,384 whole-function cases (width 4-32, height 3-24), mutations 10/10. Not bound: codegen does not reach it (the menu runs through `JSR $8028` tables). |
| `$81:F4D5` | `Lufia2PartyDerivedStats` | `verified` | Derived stats of the character block at X (base `+$51-$5D` plus bonuses `+$74-$92` into `+$25-$35`; one stat capped at 199; `+$78`/`+$7A` override two results). Verification: 16,384 whole-function cases, bridge/ABI cases (any M), mutations 7/7 over this and the capsule target. Runtime-bound through `Lufia2DecompBridge_F4D5` (any width). |
| `$82:C261` | `Lufia2CapsuleLoadStats` | `verified` | Capsule monster stats when `$0A7F` is 7: monster `$11A3`, form `$11A4`; saved level and experience from `$7F:F180-$7F:F1AC` (`$82:C3F8`), the experience curve (`$82:CE52`, step `$1400` grown by `$8E:E4CB` per 8 levels through `$4202`, capped at 9,999,999, minus 10), stats from the record at `$97:DCB8` plus growth over the levels (`$82:D31C`, `$A6:F420`), derived stats (`$81:F4D5`). Verification: 16,384 whole-function cases (levels 1-99), bridge/ABI cases, mutations 13/13. Runtime-bound through `Lufia2DecompBridge_C261`. |
| `$82:C515` | `Lufia2CapsuleSetForms` | `verified` | Forms `$11A6-$11AC` from `$8E:E485` for present capsules (`$11BB-$11C1`). Mutations 2/2. Runtime-bound through `Lufia2DecompBridge_C515`. |
| `$82:C352` | `Lufia2CapsuleSetAll` | `verified` | Present flags = A, clear `$11A5`/`$11C2-$11D5`, capsule 0 at form 1, forms and stats. Mutations 2/2. Runtime-bound through `Lufia2DecompBridge_C352`. |
| `$82:C2FD` | `Lufia2CapsuleReset` | `verified` | Every capsule back to level 1 with no experience, present flags made 0/1, then stats. Mutations 4/4. Not bound (not reached by codegen). |
| `$82:CD83` | `Lufia2CapsuleLevelUp` | `verified` | Level up when the experience reached the next level (not at 99): stats recomputed, gains into `$0A38-$0A3E`, carry clear; otherwise the state is saved back (`$82:C443`) and carry set. Mutations 5/5. Not bound (not reached by codegen). |
| `$86:8CF5` | `Lufia2SpriteSetAnimation` | `verified` | Sprite slot X (0-47) plays animation A: animation list `[$1238/$1268/$1298,X]`, frame `$1448,X`, frame pointer to `$12C8/$12F8`, `$1328/$1358`, first byte to `$1478,X`. Verification: 16,384 whole-function cases, mutations 4/4. Not bound: codegen does not reach it. |
| `$81:F194` | `Lufia2ItemRecordByte` | `verified` | A = first record byte of item `$0A06` (`$81:F291`, bank `$96`), P kept. Verification: 16,384 cases, mutations 2/2. Not bound: codegen does not reach it. |
| `$82:9F6F` | `Lufia2MenuEquipCommands` | `verified` | Equipment command window (EQUIP, STRONGEST, REMOVE, REMOVE ALL, DROP): clear rect `$82:83B5`, frame, string `$8E:D2AE`, cursor sprite `$82:895B` (animation table `$82:897D`, `$86:8CF5`) placed from `$A6:F518` (`$82:891E`, `$82:88CB` via the `$82:8000` multiply), window HDMA `$82:93F6` (`$8E:E57E/E5A0`), `$74` |= `$88`. Verification: 16,384 cases (hand-off inside `$80:8878` exact, PB `$80`), mutations 8/8. Not bound: codegen does not reach it. |
| `$82:D721` | `Lufia2MenuShopWindows` | `verified` | Shop windows (the first only with `$1540` bit 1). Mutations 2/2. Not bound: codegen does not reach it. |
| `$82:E49E` | `Lufia2MenuShopTitle` | `verified` | Shop kind title (string `$8E:D366` branches on `$1540`) and its window, width from `$82:E4D0`. Mutations 2/2. Not bound: codegen does not reach it. |
| `$82:EFC5` | `Lufia2MenuSavedWindow` | `verified` | "Game saved." window, layout by carry. Mutations 2/2. Not bound: codegen does not reach it. |
| `$82:F0A2` | `Lufia2MenuNameEntryWindows` | `verified` | Name entry: clear sprite flags (`$82:9214`/`$82:922D`), three windows, cursor sprite at layout `$AF`, cursor home `$3162`. Mutations 3/3. Not bound: codegen does not reach it. |
| `$82:D749` | `Lufia2MenuShopParty` | `verified` | Shop party screen: strings `$8E:CCFD`/`$8E:CD27`, each member's seven stats as small 3-digit tiles (`$82:E4D5`/`$82:E504`, `$82:9169`, digits `$82:91AB`, tail `$82:91F0`, palette `$155C`), cursor, HDMA table 10. Mutations 10/10. Not bound: codegen does not reach it. |
| `$82:A2E3` | `Lufia2MenuCapsuleScreen` | `verified` | Capsule monster screen: clear layer 2 (`$82:83EB`), cursor at form/monster, portrait and present-mark tile blocks (`$82:D1A4`, `$82:831A`). Mutations 6/6. Not bound: codegen does not reach it. |
| `$82:950E` | `Lufia2MenuMemberStatus` | `verified` | Member [$2A] at X: level, HP and MP as small numbers (`$82:9169`, `$82:9189`), palette `$3500` when poisoned/down or HP <= max/8, `$3100` when HP <= max/4. Mutations 6/6. Not bound: codegen does not reach it. |
| `$82:D07B` | `Lufia2MenuCapsuleStatus` | `verified` | Capsule status: strings, class from the record, three skill names (`$82:C4B3` learned test by `$8E:D8C3` mask per form, names from `$A5:DF00`). The `[$08]` strings read `$0008` absolute, so this needs D = 0 (as in the game). Mutations 6/6. Not bound: codegen does not reach it. |
| `$82:E297` | `Lufia2MenuShopSetup` | `verified` | Shop `$30`: record from `$97:EE9F`, kind `$1543`, flags `$1540`; owned-item list from the `$7F:F080` nibbles into `$7E:97E0` (`$82:E433`, `$82:FC7D`), list starts and lengths at `$7E:93C0`/`$7E:93D0`. Mutations 6/6. Not bound: codegen does not reach it. |
| `$82:CE23` | `Lufia2CapsuleExperienceRange` | `verified` | Experience at the start of the capsule level (`$113E`) and for the next (`$1141`). Mutations 2/2. Not bound: codegen does not reach it. |
| `$82:CD1F` | `Lufia2CapsuleTryLearn` | `verified` | 1 in 8 (`$80:8299`), a random skill slot 0-2 the capsule has not learned but its form offers (`$82:CD41`) is learned; carry clear with the skill id (`$82:C4B3`). Mutations 3/4 (survivor: OR vs XOR on a bit known to be clear, equivalent). Not bound: codegen does not reach it. |
| `$82:E5E1` | `Lufia2MenuShopCompare` | `verified` | Per member: the seven stats with the shop item as small numbers, green `$3100` if better, red `$3500` if worse, blank if equal or not equippable (`$82:E624`); sprite pose by the equip flags (`$82:E664`, `$86:8CF5`). Mutations 5/5. Not bound: codegen does not reach it. |
| `$82:DCF4` | `Lufia2MenuShopRow` | `verified` | One shop row at `$3708` through `$82:85D2`. M0 entry. Not bound: codegen does not reach it. |
| `$82:DCC1` | `Lufia2MenuShopRows` | `verified` | Five shop rows from `$3488` (`$82:85D2`: item name `$8E:C9B1`, price `$7E:93E0` halved by `$82:9918` unless a member holds item `$167`, times 200 in `$82:98C7` when `$154C` is 1, price string `$8E:C9C2`). Mutations 6/6. Not bound: codegen does not reach it. |
| `$82:B2C5` | `Lufia2MenuEquipUpgrade` | `verified` | With `$0A62` = `$25`: count the member's six equipment slots whose item has record flag 8 (`$82:F87D`/`$82:F893`, `$81:F194`), step those items to the next id (`$82:F8C6`), recompute the equipment bonuses (`$82:F846`: `$82:F6A4`, `$81:F1C5`, `$82:F722`) and derived stats (`$81:F4D5`); carry clear when anything changed. Mutations 6/6. Not bound: codegen does not reach it. |
| `$80:8378` | `Lufia2Divide16` | `verified` | `$4E` = `$4E` / `$51`, A = remainder, 16 unrolled shift-subtract steps; P kept. Mutations 2/3 (the ROL carry-out path cannot occur with a 16-bit dividend). Not bound: codegen does not reach it. |
| `$86:8CDA` | `Lufia2SpriteSetTable` | `verified` | Sprite slot X animation list pointer from `$8E:D9A9,Y` into `$1238/$1268/$1298,X`. Mutations 2/2. Not bound: codegen does not reach it. |
| `$81:F3F4` | `Lufia2SpellRecordByteC` | `verified` | A = byte `$0C` of the record of spell `$0A0B`. |
| `$81:F404` | `Lufia2SpellRecordByte8` | `verified` | A = byte 8 of the record of spell `$0A0B`. Mutations 1/2 (the JSR frame is overwritten by PHB/PHA). Not bound: codegen does not reach it. |
| `$82:9CB2` | `Lufia2MenuWarpList` | `verified` | Warp destination list (PLACE WHERE YOU MOVE TO): windows, two cursor sprites (`$82:89C4`), scrollbar thumb (`$82:8C9C`, range `$82:8C85`, step by `$80:8378`, position by `$82:8000`), rows of town names from `$7E:943C` through `[$08]` (`$82:8680`, `$8E:D062`; entries are 3 bytes then the name). Mutations 6/7 (frame 5 unreachable here). Not bound: codegen does not reach it. |
| `$82:A918` | `Lufia2MenuListCursor` | `verified` | Item/spell list: cursor sprite y from the selection `$14AF` relative to the top `$14B5` (halved for spells, two columns) times `$1507` via `$82:8000`, visibility `$11E0` inside the six-row window, then the page `$82:AC84` by mode `$09D1` (0 items, 2 spells, 4/`$FF` items with `$153E` 1/2, 7 scenario items). Rows: items `$82:84F7` (name `$8E:C9B1`, count `$8E:C9B7`, attribute `$82:841E`: `$28` special, `$20` usable, `$24` not, items `$29/$2A/$2D` by `$09A7`), spells `$82:854F` (name `$8E:CA0C`, cost `$8E:CA34`, attribute `$82:8483` by record flag `$40` and cost vs `$151D`), scenario items `$82:86E5` (`$82:FC3F`: nth set bit of `$091E-$0925`, item from `$97:FDA0`). Mutations 4/4. Not bound: codegen does not reach it. |
| `$82:ACDB` | `Lufia2MenuListRow` | `verified` | One list row at `$3748` by the same mode. Mutations 12/12. Not bound: codegen does not reach it. |
| `$86:8B55` | `Lufia2SpriteFrame` | `verified` | Once per menu/title frame: animate (`$86:8B73`), clear (`$86:8BCF`) and build (`$86:8BF5`) the OAM buffer, request its upload (`$72` = `$80`), then the frame wait `$86:8B48`, where it hands off exactly to LLE (the wait is batched by the frame-wait fast-forward). Runtime-bound through `Lufia2DecompBridge_8B55` (any width). Mutations 2/2. |
| `$86:8B73` | `Lufia2SpriteAnimateAll` | `verified` | For the 48 sprite slots (`$11D8` active): timer `$1478` counts down; at 0 the next frame of the slot's list `[$12C8/$12F8/$1238]` (`$FE` holds the previous frame, `$FF` restarts); frame pointer to `$1328/$1358`, its first byte is the new timer (`$86:8B87`). 8-bit X. Bound. Mutations 5/5. |
| `$86:8BCF` | `Lufia2SpriteClearOam` | `verified` | OAM buffer `$0100`: 128 entries at y `$F0`, 32 high-table bytes clear. Bound. Mutations 3/3. |
| `$86:8BF5` | `Lufia2SpriteBuildOam` | `verified` | Active slots 12-47 then 0-11 into the OAM buffer (`$86:8C1F`): per piece (after the size byte) x and y relative to `$1388/$13E8` (with x bit 8 from `$13B8`), tile and attributes, size and x-high bits rotated into the high table at `$0300`. Bound. Mutations 7/7. |
| `$81:F87F` | `Lufia2PartyBaseStats` | `verified` | Stats of member `$09FA` at level `$09FE` into `$0A1C-$0A28`: the growth rows `$97:B62C` + member * `$70` (a new 7-byte row every 8 levels) summed per level, / 16, plus the base `$97:B93C` + member * 14. DB = `$97`. Reached by codegen, left unbound (rare). Mutations 7/7. |
| `$81:ED8E` | `Lufia2PartyUnpackMember` | `verified` | Member block `$7E:Y` from its stored form at X: header words, level, name (to `$FF`, padded with `$FF` to `$BA`), `$5F-$62`, 7 equipment words, `$BC`; then base stats (`$81:F87F`), experience (`$81:F9E9`), derived stats (`$81:F4D5`), HP and MP to their maxima; `$09FA` + 1. Mutations 8/8. Left unbound (runs at loads only). |
| `$81:EE94` | `Lufia2PartyUnpackMemberBare` | `verified` | As `$81:ED8E` for a member without equipment (the 7 equipment words get the low byte of D, 9 bonus words to `$74`). Mutations 3/3. Not reached by codegen. |
| `$81:C129` | `Lufia2BattleIpSkills` | `verified` | Battle IP table `$7E:DF00` (`$30` per slot) for member `$1BE8` (sets the WRAM port `$2181` first): per equipment slot the slot label (`$85:9E8F`), the item name (`$81:F1C5`), its IP skill `$0BAB` with cost and 13-byte text from the skill record (`$81:F45E`, `$84:8F10`), and usable (0) when a skill exists and level `$BC` >= its level. Mutations 8/8. Not bound. |
| `$81:DFA2` | `Lufia2BattleListRows` | `verified` | Eight battle list rows at `$7E:3080` from entry X - 2: each row is cleared through WMDATA (`$2180`, 128 writes), then by mode `$1B` two IP entries (`$81:E019`), a spell (`$81:E094`, 24 bytes each) or an item (`$81:E100`) from `$7E:DF00`, skipped when at or past count `$17` or marked (bit 7); characters through the battle font (`$81:E835`: `$10-$CC` shifted by `$DF`, two-row glyphs from `$97:B2C9` over `$CD`/`$CE`). Mutations 10/10. Not bound. |
| `$81:C35F` | `Lufia2BattlePopups` | `verified` | Battle damage popups for mode A: for the 11 slots at `$7F:F60C` (`$1E` each) whose type matches the mode, value `$15` and kind `$13` (`$81:C3EE`: first nonzero word, kind table `$85:A6C6`), then an 11-byte record at `$7E:4F0B` (`$81:C477`): target from `$95:FFED`, position (`$81:B80A`, multiplier over `$1399`/`$13DA`) and size (`$81:B7D9`, sprite tables `$96:85C0`, `$97:CA64`), sign and up to four decimal digits through the divider, clamped to 9999. Mutations 18/18. Bound.
| `$81:B264` | `Lufia2BattleActiveMask` | `verified` | Active mask `$0022` (DB-relative) of the six enemies (`$0A6E`, A bit 7, result ORed with `$80`) or five party members (`$0A64`): a bit per record that exists and lacks status bit 2 in `$0F`. Mutations 3/3. Bound. |
| `$81:B2B5` | `Lufia2BattleTargetRecord` | `verified` | X = battler record of target mask A: lowest set bit of bits 0-5, plus 5 for enemies (bit 7), through `$0A64`; RTL. Mutations 2/2. Bound. |
| `$81:B2DB` | `Lufia2BattleTargetSlot` | `verified` | X = `$1499` + 7 * target index of mask A (same index as `$81:B2B5`, product through `$4202`); RTL. Mutations 2/2. Bound. |
| `$81:B505` | `Lufia2BattleBlend` | `verified` | A = `$CA` + (A - `$CA`) * `$29` / 256, rounded (`+$80`); when A < `$CA` the two swap and the factor is 256 - `$29`, leaving A in `$CA`. Mutations 2/2. Bound. |
| `$81:B54A` | `Lufia2ColorToGray` | `verified` | BGR555 colour `$15` to gray: R * 77 + G * 151 + B * 28 through the multiplier, level (0-31) in `$17` and as all three channels in `$15`; exits M0. Mutations 3/3. Bound (M0X0 entry, any-width bridge). |
| `$81:B5A3` | `Lufia2BattleHideOam` | `verified` | OAM buffer reset: high table `$0300-$031F` = `$55`, every low entry at `$0100-$02FF` = X 1, Y `$E0`. Mutations 1/1. Bound. |
| `$81:B974` | `Lufia2BattleLoadPalette` | `verified` | Palette slot A: source Y and bank `$24` noted at `$12B3 + 3A` (`$12B5` the slot), then 32 bytes from `$24:Y` to `$7F:F1DB + 32 * (A & 15)`. Mutations 2/2. Bound. |
| `$81:B9AF` | `Lufia2BattleCommitPalettes` | `verified` | 512 bytes `$7F:F1DB` to the CGRAM buffer `$0320`, upload flag `$73` = `$80`; RTL. Mutations 1/1. Bound. |
| `$81:C2C0` | `Lufia2BattleCopyC2C0` | `verified` | 16 bytes from `$B401` (DB) to `$123C`. Mutations 1/1. Bound. |
| `$81:C2D0` | `Lufia2BattleClear2000` | `verified` | Clears `$7E:2000-$27FF` through WMDATA. Bound. |
| `$81:C2E3` | `Lufia2BattleFill2800` | `verified` | Fills `$7E:2800-$2FFF` with tile `$2100` through WMDATA. Mutations 1/1. Bound. |
| `$81:C2FB` | `Lufia2BattleClear3000` | `verified` | Clears `$7E:3000-$37FF` through WMDATA. Bound. |
| `$81:C30E` | `Lufia2BattleClear3800` | `verified` | Clears `$7E:3800-$3FFF` through WMDATA. Mutations 1/1. Bound. |
| `$81:C5CF` | `Lufia2BattleTargetPointer` | `verified` | X = record of target mask A (nonzero): lowest set bit, party from `$0A64`, enemies (bit 7) from `$85:9EC8`. Mutations 1/1. Bound. |
| `$81:BE58` | `Lufia2BattleTileBlock` | `verified` | `$02` x `$03` blocks of 2x2 tiles from sheet tile `$00` (attribute `$04`) at `$7E:$08`, rows `$80` apart, the tile advancing by 2 and wrapping to the next sheet row pair. Mutations 3/3. Bound. |
| `$81:BD4B` | `Lufia2BattleSpriteBlock` | `verified` | `$02` x `$03` 5-byte sprite entries at `$7E:$08` (X from `$06` - `$07`, Y `$05` - 1, 16 px steps, tile as `$81:BE58`, attribute `$04`, size bits from `$18`); `$08` advanced, A = count. Mutations 3/3. Bound. |

## Exported entry points

The public headers export a few entry points besides the whole functions of
`metadata/functions.toml`:

| Address | Symbol | Metadata | Evidence and reason |
| --- | --- | --- | --- |
| `$80:8299` | `Lufia2RandomScale` | `verified` | Whole function: `PHB/PHK/PLB/PHX/PHY/PHP/SEP #$30`, table index `$0559`, lagged-XOR refill `$80:832D` past `$36`, `$4202/$4203` product, `PLP/PLY/PLX/PLB` and its RTL at `$80:82C6`. Differential against the ROM in the consumer's actor-dispatch suite: 4,096/4,096 cases over all four M/X entry widths, DB `$7E/$83/$00/$80`, D `$0000/$0020`, full CPU state and WRAM compared. Recorded with the M1X0 caller contract; PHP/PLP keep any caller width. Not runtime-bound. |
| `$80:82C7` | `Lufia2RandomByte` | `verified` | Same table and index, the next byte in A.low, RTL at `$80:82E6`. Same differential, 4,096/4,096 cases. Not runtime-bound. |
| `$83:C83C` | `Lufia2ActorPrimaryScriptDispatch` | none | Continuation inside `$83:C7F8`, reached only by its own branches (no `JSR`, `JSL` or `JMP` in the ROM targets it). Exported as a prefix up to the selected handler PC for the consumer's prefix verifier (2,048/2,048 cases); the whole-function record is `$83:C7F8`. |
| `$83:D59A` | `Lufia2ActorSecondaryScriptDispatch` | none | Continuation inside `$83:D508`, likewise reached only by its own branches; prefix verifier 2,048/2,048 cases; the whole-function record is `$83:D508`. |
| `$83:C7F8`, `$83:D508` front-ends | `Lufia2ActorPrimaryUpdateFrontend`, `Lufia2ActorSecondaryUpdateFrontend` | covered by `$83:C7F8`/`$83:D508` | Gate prefixes that stop at named continuations; the verified whole functions call them. |

A continuation prefix is not a function entry, so it never gets its own
`functions.toml` record; the record belongs to the routine it continues.

## Recovered bank-$81 helpers (S12–S18)

Recovered from the interrupted local batch. All 41 standalone baselines, 66 mutation checks, and the full MSVC Release checkpoint are green. Each entry passes 16,384 standalone and 8,704 bridge cases. Release built successfully; all previous 80 bindings remain selected, for 121 total, with no lost AOT nodes. The existing item helpers are shared by their callers and the standalone entries. All supported contracts use X16; F291 also safely widens X internally.

| Address | Function | Recovery status | Semantics / evidence |
|---|---|---|---|
| `$81:E479` | `Lufia2BattleFrameTop` | verified; runtime-bound | Window top edge at Y: tile `$09F4`, `+1`, width - 1 fills of `+2`, then the mirrored pair (`$4000`); Y = next row (`$09FC` + `$40`). Entered M0X0. Bound (M0X0 bridge). Standalone 16,384 cases; mutations 3/3. |
| `$81:E4AD` | `Lufia2BattleFrameSides` | verified; runtime-bound | Window side edges: tile `$09F4` at Y and mirrored at Y + 2 * width + 4; next row. Entered M0X0. Bound (M0X0 bridge). Standalone 16,384 cases; mutations 1/1. |
| `$81:E542` | `Lufia2BattleFrameRow` | verified; runtime-bound | Window row from a 3-word template at `$01:X` (left, width + 1 fills, right); next row. Entered M0X0. Bound (M0X0 bridge). Standalone 16,384 cases; mutations 1/1. |
| `$81:E570` | `Lufia2BattleFrameEnds` | verified; runtime-bound | Window middle row ends from template `$01:X` (left, right); next row. Entered M0X0. Bound (M0X0 bridge). Standalone 16,384 cases; mutations 1/1. |
| `$81:E5C1` | `Lufia2BattleGaugeBlock` | verified; runtime-bound | Gauge block of tile A at X (DB-relative): ends `A+5`/`A+6`, four rows of `A+2` then six `A`, closed by mirrored `A+5`/`A+4`. Entered M0X0. Bound (M0X0 bridge). Standalone 16,384 cases; mutations 2/2. |
| `$81:E604` | `Lufia2BattleGaugeColumn` | verified; runtime-bound | Gauge column of tile A at X: `A+5`/`A+6`, three `A+1` 16 bytes apart, then mirrored `A+5`/`A+4`. Entered M0X0. Bound (M0X0 bridge). Standalone 16,384 cases; mutations 1/1. |
| `$81:F291` | `Lufia2ItemTextPointer` | verified; runtime-bound | `$0A09` = `$96:CF69` + word `$96:CF69`[item `$0A06` & `$1FF`]; PHP-guarded, RTL. Bound (any width). Standalone 16,384 cases; mutations 1/1. |
| `$81:F446` | `Lufia2SpellTextPointer` | verified; runtime-bound | `$0A0D` = `$95:FA5B` + word `$95:FA5B`[spell `$0A0B`]. Bound. Standalone 16,384 cases; mutations 1/1. |
| `$81:F5ED` | `Lufia2PartyRestore11` | verified; runtime-bound | For each party record in `$0A80` (4 words) without status bit 2: `$11` = `$25`; RTL. Bound. Standalone 16,384 cases; mutations 2/2. |
| `$81:F60B` | `Lufia2PartyRestore13` | verified; runtime-bound | Same with `$13` = `$27`; RTL. Bound. Standalone 16,384 cases; mutations 1/1. |
| `$81:F7BD` | `Lufia2BattleTable9EBA` | verified; runtime-bound | X = word X of `$85:9EBA`; P kept. Bound (any width). Standalone 16,384 cases; mutations 1/1. |
| `$81:FB79` | `Lufia2CharacterSpriteByte` | verified; runtime-bound | A = byte `$96:85CE` of character A's record (`$96:85C0` table); X kept; RTL. Bound. Standalone 16,384 cases; mutations 1/1. |
| `$81:EC41` | `Lufia2BattleClearF000` | verified; runtime-bound | Clears `$7F:F000-$FFFF` through WMDATA. Bound. Standalone 16,384 cases; mutations 1/1. |
| `$81:F2A9` | `Lufia2ItemNameTrimmed` | verified; runtime-bound | Item `$0A06` name, 12 bytes of `$9E:C7E8`, to `$0B77` with a terminator, trailing spaces zeroed; RTL. Bound. Standalone 16,384 cases; mutations 2/2. |
| `$81:F4ED` | `Lufia2PartyStatTotals` | verified; runtime-bound | Member `$C1` stats: `$25`/`$27` = base + equipment, `$2D-$35` = base + two bonus sets (`$33` capped at 199), `$29` and `$2B` (average of `$2D`/`$2F` plus bonus) with fixed overrides `$78`/`$7A`. Bound. Standalone 16,384 cases; mutations 3/3. |
| `$81:F78D` | `Lufia2PartyPointers` | verified; runtime-bound | `$0A80` = records (`$85:9EBA`) of the four members `$0A7B`, 0 when bit 7 is set. Bound. Standalone 16,384 cases; mutations 1/1. |
| `$81:FBA2` | `Lufia2SpriteSizePacked` | verified; runtime-bound | A = packed size of sprite A - 1 from `$97:CA64` (low byte >> 5, high bits `$E0` >> 2); P and X kept; RTL. Bound (any width). Standalone 16,384 cases; mutations 1/1. |
| `$81:FBDB` | `Lufia2CharacterSpriteBox` | verified; runtime-bound | `$09FC-$09FF` = four bytes `$96:85DD` of character `$09FA`'s record; RTL. Bound. Standalone 16,384 cases; mutations 1/1. |
| `$81:FCE2` | `Lufia2CharacterSpritePointer` | verified; runtime-bound | X = `$96:85C0` + word `$96:85C0`[character A]; Y kept; RTL. Bound. Standalone 16,384 cases; mutations 1/1. |
| `$81:E7D2` | `Lufia2BattleFillRect` | verified; runtime-bound | Fills `$09F2` x `$09F3` words of `$09F6` at `$7E:X`, rows `$40` apart. Bound. Standalone 16,384 cases; mutations 1/1. |
| `$81:E808` | `Lufia2DecimalDigits3` | verified; runtime-bound | X to decimal digits: hundreds in `$B4` (DB-relative increment), tens `$B3`, ones `$B2`. Bound. Standalone 16,384 cases; mutations 1/1. |
| `$81:EB34` | `Lufia2BattlePaletteCopy` | verified; runtime-bound | 16 bytes of palette `$24` (`$9A:F970` + 16 * `$24`) to `$120F`, 16 zeros after. Bound. Standalone 16,384 cases; mutations 1/1. |
| `$81:EB62` | `Lufia2BattlePaletteSplit` | verified; runtime-bound | Palette `$24` split per byte: low nibble << 4 to `$121F`, high nibble to `$120F`. Bound. Standalone 16,384 cases; mutations 1/1. |
| `$82:FB1F` | `Lufia2ItemPossessionCount` | `verified` | M0/X0 item ID in A; ownership bit or first packed quantity. DB/DP unchanged, Y preserved; absent A0/C1. Three RTL sites, 16,384 original-ROM cases, 9/9 mutations. Not runtime-bound. |
| `$81:F057` | `Lufia2InventoryCount` | verified; runtime-bound | A = count (bits 9-15) of item `$0A06` in the 96-slot inventory `$0A8D`, 0 when absent; RTL. Bound. Standalone 16,384 cases; mutations 2/2. |
| `$81:E503` | `Lufia2BattleWindow` | verified; runtime-bound | Window template `$01:$09F4`: top row, `$09F3` middle rows, bottom row; X advances through the template by six then four bytes. Width is `$09F2 - 1`. Standalone 16,384 cases; mutations 2/2. |
| `$81:E593` | `Lufia2BattleGaugePanel` | verified; runtime-bound | Gauge panel at `$7E:2D80`: block `$2155`, four columns `$2157`, block `$A155` (`$81:E5C1`/`$81:E604`); P kept. Bound. Standalone 16,384 cases; mutations 2/2. |
| `$81:F4E9` | `Lufia2PartyStatTotalsFar` | verified; runtime-bound | Far entry (JSR + RTL) of `$81:F4ED`. Bound. Standalone 16,384 cases; mutations 1/1. |
| `$81:F789` | `Lufia2PartyPointersFar` | verified; runtime-bound | Far entry of `$81:F78D`. Bound. Standalone 16,384 cases; mutations 1/1. |
| `$81:BD47` | `Lufia2BattleSpriteBlockFar` | verified; runtime-bound | Far entry of `$81:BD4B`. Bound. Standalone 16,384 cases; mutations 1/1. |
| `$81:BE54` | `Lufia2BattleTileBlockFar` | verified; runtime-bound | Far entry of `$81:BE58`. Bound. Standalone 16,384 cases; mutations 1/1. |
| `$81:E3AE` | `Lufia2BattleWindowE3AE` | verified; runtime-bound | Inside at X + `$42` filled with `$2154`, then window template `$87E3` at X; RTL. Bound. Standalone 16,384 cases; mutations 2/2. |
| `$81:E3CD` | `Lufia2BattleWindowE3CD` | verified; runtime-bound | Same with fill `$2167` and template `$87F9`. Bound. Standalone 16,384 cases; mutations 1/1. |
| `$81:BAE8` | `Lufia2BattlePortraits` | verified; runtime-bound | Portraits of slots 3 to 0 with DB `$97`; A kept; RTL. Bound. Standalone 16,384 cases; mutations 1/1. |
| `$81:BAFB` | `Lufia2BattlePortrait` | verified; runtime-bound | Portrait slot A: pose 5 for status bit 2, pose 2 for bits `$29`, otherwise pose 0. An absent portrait clears 257 bytes with overlapping 16-bit stores in the first loop and 256 bytes in the second loop; the gap is retained. Standalone 16,384 cases; mutations 3/3. |
| `$81:BB75` | `Lufia2BattlePortraitUpload` | verified; runtime-bound | Portrait `$153D[$11]` (source from `$8E:E5C2`, pose offset `$B534[$12]`) to `$7E:$B52C[$11]` and +`$200`, 256 bytes each, through WMDATA. Bound. Standalone 16,384 cases; mutations 2/2. |
| `$81:B48B` | `Lufia2BattleFadeColor` | verified; runtime-bound | `$22` = gray colour `$22` blended per channel toward colour `$24` by level `$13` (0 keeps gray, `$40` takes `$24`) through `$81:B505`. Bound. Standalone 16,384 cases; mutations 3/3. |
| `$81:B444` | `Lufia2BattlePaletteFade` | verified; runtime-bound | Colours 1-15 of palette `$11` from `$7F:F1DB`: gray (`$81:B54A`) with blue reduced, faded by `$13` (`$81:B48B`), to the CGRAM buffer `$0320`; RTL. Bound. Standalone 16,384 cases; mutations 2/2. |
| `$81:E405` | `Lufia2BattleTileFrame` | verified; runtime-bound | Frame of tiles `$10F1` (`$81:E479`/`$81:E4AD`, bottom mirrored with `$8000`) at `$7E:X` + `$8C0`, `$09F2` x `$09F3` (`$09F3` - 2 middle rows). Bound. Standalone 16,384 cases; mutations 3/3. |
| `$81:E3EC` | `Lufia2BattleTileWindow` | verified; runtime-bound | Inside at X + `$902` filled with `$10F0` (`$81:E7D2`), then `$81:E405` at X. Bound. Standalone 16,384 cases; mutations 1/1. |
| `$81:F979` | `Lufia2PartyLevelUpCheck` | verified; runtime-bound | Member `$09FA` (record via `$81:F7BD`): next-level experience (`$81:F9E9`) against the stored one (A = 2 when stale), then when current experience reaches it and level < 99: level + 1, new next-level experience stored, A = 1; else A = 0. Bound. Standalone 16,384 cases; mutations 3/3. |
| `$81:FC0B` | `Lufia2PartyNewRecord` | verified; runtime-bound | New member record at `$7E:[$B2]` (`$BE` bytes cleared) for character `$09F2` from its `$96:85C0` data: name, level, base stats, doubled bytes, equipment list (entries >= 6 at `$48` + 2 * (id - 6)), current = max HP/MP, then derived stats (`$81:F4D5`); RTL. Bound. The initial fill is the low byte of D, as in the ROM. Standalone 16,384 cases; mutations 5/5. |

## S19 inventory and battle glyph

| Address | Function | Contract / evidence |
|---|---|---|
| `$81:F0A2` | `Lufia2InventoryAdd` | M1X0, binary arithmetic; first nine-bit item match, else first empty word among 96 slots. Eight-bit quantity addition, cap 99 and packed remainder. RTS F0CC/F0D8/F113. 16,384 standalone cases, mutations 11/11; 8,704 bridge cases; full MSVC Release PASS, zero failures, Release built. Exact ROM RTS site compared on every case. Empty slots copy the request verbatim; cap 99 applies only to merges. Unbound: real caller F096, absent from the current generated graph. |
| `$81:E835` | `Lufia2BattleGlyph` | Exported existing helper; M1X0, X preserved, DB/DP unused. ROM $97:B2C9 bottom tiles, original control-character and top-tile rules, RTS E847/E86E/E871. All 256 characters; 16,384 standalone cases, mutations 9/9; 8,704 bridge cases; full MSVC Release PASS, zero failures, Release built. Exact ROM RTS site compared on every case. Runtime-bound; all previous replacements preserved, no lost AOT. Existing generated battle-text callers justify binding. |

E8EE remains deferred because its WMDATA port state belongs to the caller.

162 verified functions; 122 runtime replacements after T12 (T11 baseline: 160/122).


## T11 shared text helpers (2026-09-27)

| Address | Function | Status | Contract and evidence |
| --- | --- | --- | --- |
| `$80:C652` | `Lufia2TextMeasure` | verified, unbound | Complete measurement through RTS C743. M1X0, script DB:Y; DP $54-$57; shared C0B7/C102 bank crossing, name/dictionary/numeric widths, ROM skip/dimension tables, eight-bit width/line wrap, restored script bank/pointer. 16,384 differential cases, exact RTS checked; 5/5 mutations caught. |
| `$80:C784` | `Lufia2TextClearGlyphBuffer` | verified, unbound | Complete 4 KB $7E:D000-DFFF fill using the ROM attribute table, reset glyph/destination/line state, PHP/PHB restores P and DB, RTL C7BD. 16,384 differential cases across entry widths, exact RTL checked; 4/4 mutations caught. Metadata names the M1X0 caller contract. |
| `$80:C56E` | `Lufia2TextQueueWindowRow` | verified, unbound | Complete tilemap row helper C5DD and DMA-channel 1/2 setup through RTS C5DC. M1X0, P/Y restored; shift carry and TDC high byte preserved; zero width retains the ROM's 65,536-iteration row loop. 16,384 differential cases including zero/255 widths, address wrapping and DP $0100 carry; exact RTS and MMIO order checked; 6/6 mutations caught. Native through the already-bound text engine at BCF2. |
| `$80:C23D` | `Lufia2TextPrepareWindow` | draft at T11; verified in T12 | At T11 only the prefix was implemented. Clears glyphs and optionally prepares channel 0; exact continuation C274 before C2A1 or C279 before C305, parent JSR + PHY + PHB intact. Both paths verified through $80:9CB8; an additional isolated 16,384-case prefix probe covers hardware-mirrored DB values and channel 0 MMIO order (C274 8,142, C279 8,242), with 3/3 opcode-prefix mutations and one shared glyph-path mutation caught. No separate runtime binding. |

T11 adds three verified helpers (160 total), while the selected runtime replacements remain 122. The existing text-engine bridge contract is unchanged; its synthetic seeds also cover the new paths. C305 placement, C2A1 frame-wait continuation and opcode $14 / A074 remain upcoming work; they are unimplemented, not blocked on gameplay.

T11 checkpoint validation: all four MSVC Release decomp-verify suites PASS, zero failures; 2,654,848 actor-dispatch cases. Whole-engine MSVC cases: 8,210 returns and 8,174 continuations, coverage C274 1,711 / C279 1,673 / C56E 4,630. Existing 9CB8 bridge: 8,704/8,704 (host return 1,548; dispatch return 3,104; native continuation 3,540; LLE entry 512). Windows Release built. All 1,369 graph-node dispositions and all 122 replacement selections remain identical to S19; no lost AOT nodes.

## T12 text window contract (2026-09-27)

`$80:C305 Lufia2TextBuildWindow` is verified and unbound: complete frame and
speech-tail construction through RTS C513, entry/exit M1X0, DB becomes $7E.
It pads width, selects explicit or actor-relative placement, clips and adjusts
the tail, composes alternating borders and paired body rows, and propagates
carry across six ROM-backed tail tiles. BF6F, C557 and C52C are internal
semantic children; the existing actor-record-offset helper is reused.

The C431 join genuinely has two decodings. Actor placement reaches it in M0
(`LDA #$FFFC; STA $059E`); explicit placement reaches it in M1 (`LDA #$FC;
SBC $059E8D,X`). The original indexed read is preserved. No correction or
optimization of this unusual path is part of the semantic source.

`$80:C23D Lufia2TextPrepareWindow` is now verified and unbound. Placement
returns through RTS C2A0, restoring Y/DB and completing the window state.
The other branch constructs C274's JSR frame and hands off at C2A1 before
its first BF0B frame wait. Parent JSR, PHY, PHB and C274 JSR remain outstanding.
The original ROM owns both waits, the intervening C61D upload, subsequent
centered tile fill and parent cleanup. This is an exact temporal boundary;
it does not claim native execution after the wait.

Both paths run through the existing bound text engine. C305 and C23D each
pass 16,384 original-ROM cases; C23D returns 8,192 and hands off 8,192.
C305 join coverage: M0 8,192 / M1 8,192; tail directions 256 / 2,304 / 768 /
4,864; actor lookup miss 4,096 / hit 4,096. CPU, all WRAM, exact return PCs,
ordered MMIO and streaming stack-write/join-read observations are compared.
Meaningful mutations: C305 14/14, C23D 7/7, integrated engine 4/4.

Synthetic states keep active stack/script regions separate from the tilemap
and exclude impossible above/below placements that the ROM retries forever.
Bank-crossing engine seeds also initialize the old bank's dimensions:
measurement writes dimensions in the new DB before restoring the entry DB.
Engine entry clears the explicit position, so both C431 widths are required
by the standalone C305 test; the engine exercises its reached actor path.

T12 checkpoint: all four MSVC Release verification suites PASS, zero failures;
2,687,616 actor-dispatch cases. Full 9CB8: 10,373 returns / 6,011 continuations,
wait 1,934 / actor join 1,936 / C56E 6,598. Existing bridge 8,704/8,704
(host return 1,935; dispatch return 3,870; LLE boundary 2,387; LLE entry 512).
Release built; all 1,369 node dispositions, roots and exit-mode sets and all
122 runtime selections unchanged from T11. 162 verified entries, no metadata
drafts. T13 opcode $14 / A074 is next; Ancient Cave contract research follows.

## T13: conditional text expressions (2026-09-27)

Opcode `$14` (`$80:A074`) now runs natively inside the existing bound
`Lufia2TextEngineStep`. Its complete expression loop supports normal/inverted
event flags with assignment/AND/OR, byte variables, item quantities, record
fields and 36-byte searches, the `$0005AE/$0005B2` nibble, actor flags, and
conditional script gotos through the existing A3C6 helper. All eight comparison
selectors are retained. The original greater-than DEC/CMP wraps zero to FFFF;
D8 retains the old high byte at DP:$55; false F8 retains the preceding result.
A074 is an internal opcode entry with joins 9D00/A3C6, not a standalone callable.

A1CC preserves ordered writes to DB:$4202/$4203 and the word product read
from DB:$4216. These are ordinary WRAM accesses in DB7E/7F and hardware in CPU
register banks. No host multiplication replaces them. A1E4 preserves word
fetches, PHY/JSL/PLY, M transitions and DP:$54/$55 output.

`Lufia2ItemPossessionCount` (`$82:FB1F`) has an independently evidenced JSL/RTL
contract (text callers A1E4 and AC28, item callers F8E9/F925). M0/X0 throughout,
DB/DP unchanged and Y preserved; nine-bit A selects an item. A listed ownership
bit produces A=1/C=0. Otherwise the first matching ID in 96 packed slots yields
its seven-bit quantity/C=0, including zero quantity. No match yields A=0/C=1.
The internal FB51 table search and all original stack effects are retained.
This verified function is not runtime-bound; no new runtime selection is added.

Targeted original-ROM differential verification: FB1F, A1CC, A1E4 and complete
A074 each pass 16,384 cases. Coverage requires every condition family, all eight
comparators, both comparison results and both handler joins; FB1F requires all
three return sites. Full CPU, WRAM, ordered MMIO and temporary stack writes are
compared. Seeds cover independent WRAM products, actual CPU multiply registers,
isolated possession masks, first/last and duplicate inventory IDs, zero/max
quantities, DP offsets, actor hits/misses and script bank crossings. Targeted integrated
9CB8 passes 16,384 cases (7,479 returns, 8,905 exact LLE continuations).
Meaningful mutations: 31/31 caught (9 item, 6 condition-child, 13 expression,
3 engine integration). All four MSVC Release verification suites PASS, zero failures;
2,753,152 actor-dispatch cases. Release executable built. All 1,369 node
dispositions, roots and exit-mode sets and all 122 runtime selections
unchanged from T12. 163 verified metadata entries, zero drafts. T14 AC1
research follows; its actual 9E31 entry is M1/X0, independently proved
from the ROM caller and original-ROM probes.

T13 full-run boundary and coverage counts:

```text
Full actor-dispatch cases: 2,753,152
$80:9CB8 whole-function cases passed:      16384 / 16384 (return 7435, LLE 8949)
$80:9CB8 T12 coverage: wait 1910, explicit 0, C56E 5973, actor 1922
$80:9CB8 T13 families: 6868 733 731 356 368 361 356 359 369 359 358 363 359 371 370; comparisons: 311 324 325 312 315 316 311 320; false/true 1336/1198; joins 0/0
$80:9CB8 search miss/hit 179/180; goto true skip/take 186/185; false skip/take 182/188
$82:FB1F return coverage: possession 2731, absent 2731, packed 10922
$80:A1CC bus coverage: WRAM 9831, CPU MMIO 6553
$80:A074 whole-function cases passed:      16384 / 16384 (return 0, LLE 16384)
$80:A074 T13 families: 19456 2048 2048 1024 1024 1024 1024 1024 1024 1024 1024 1024 1024 1024 1024; comparisons: 896 896 896 896 896 896 896 896; false/true 3608/3560; joins 15361/1023
$80:A074 search miss/hit 512/512; goto true skip/take 514/510; false skip/take 511/513
$80:9CB8 8704/8704 (host return 1334, dispatch return 2663, LLE boundary 4195, LLE entry 512, child never returned 0)
```

## T14: Ancient Cave floor generator (2026-09-27)

`$83:9E31` (`Lufia2AncientCaveGenerateFloor`, `src/cave/ancient_cave.c`) is
native as a whole function, with the builder `$83:9013` and all of its bank-83
children. It is **verified, unbound**: the runtime keeps its AOT/LLE path; no
binding or generated-graph change is part of T14.

### AC1 contract

Entry is M1/X0 (A8, X/Y16) (the T13 correction stands). Evidence: the ROM immediates,
the one ROM caller `$83:B26D` (every static path to it arrives M1/X0 (A8, X/Y16) with
DB = `$83` from `PHK; PLB` at `$83:ACBB`), a static mode-tracking walk of the
whole call tree (`scripts/ancient_cave_contracts.py` in the consumer) and
112 original-ROM probe runs tracing every JSR/JSL entry and return (a scratch
probe; the committed verifier exercises the same sites).

| Routine | ROM callers | Entry -> exit | Stack, DB, DP | Disposition |
| --- | --- | --- | --- | --- |
| `$83:9E31` | 1 (`$83:B26D`) | M1/X0 (A8, X/Y16) -> M1/X0 (A8, X/Y16); RTL `$9EA1` (floor 99) or `$9F3E` | balanced; DB and D used as is (`$05AC/$05B6/$099D` DB-relative) | native, verified |
| `$83:9013` | 1 (`$83:9F32`) | M1/X0 (A8, X/Y16) -> M1/X0 (A8, X/Y16); RTS `$99C7` | `PHB ... PLB`; sets DB `$7E/$96/$7F/$83`, MVN leaves `$7F` | native, internal |
| `$80:93FE` | 16 | M1/X0 (A8, X/Y16) -> M1/X0 (A8, X/Y16); RTL `$9419` | balanced | APU handshake (`$2140-$2143` compare loops): stays external, called through the pushed-frame child |
| `$83:B5D3` | 6 | M1/X0 (A8, X/Y16) -> M1/X0 (A8, X/Y16); RTL `$B66D` | `PHP/PHB` balanced | map loader, stays external through the child (see below) |
| `$00:057D` stub | `JSR $057D` from `$83:B618`; operands also patched at `$86:900B/906E` | M0/X0 (A16, X/Y16) -> M0/X0 (A16, X/Y16), DB = `$7E`, A = `$FFFF` | RTS through the caller's bank | `MVN dst,src ; RTS`; `$057D = $54`, `$0580 = $60` written only by boot `$80:80E1`; callers write `$057E` (dst) and `$057F` (src) |
| `$83:99C8` | 4 (builder) | M1 -> **M0** | balanced | internal |
| `$83:9D46` | 4 (builder) | M0 -> M0 | balanced | internal |
| `$80:EC98` | 2 (`$80:EB96`, `$83:99B8`) | **M0** -> M1 | balanced | native, shared (`Lufia2FieldDecompressMapData`) |
| `$80:EBAA` | 3 (`$80:EB40/EB83`, `$83:99A7`) | M1 -> M1 | exits with DB = `$7F` | native, shared (`Lufia2FieldReadSections`) |
| `$80:EC18/EC78` | 2 each | M1 -> M1 | balanced | native, shared |
| `$83:C652` | 2 (`$83:919A`, `$83:C51E`) | M1 -> M1, carry = found | `PHB ... PLB` | native, shared (`Lufia2PartyListHasEntry`) |
| `$83:9B48` | 11 (builder) + `$83:9B44` (JSL from `$8E:B8E5`) | M1 -> M1 | balanced | native (`Lufia2CaveCellPosition`) |
| other `$83:99D3-$9E1B` | builder only | M1 -> M1 | balanced | native, internal |
| `$80:82C7`, `$80:8E9D`, `$80:BFAA`, `$80:E898`, `$80:BE1E` | many | M1 -> M1 (RNG also under M0) | balanced | existing verified natives reused |

Every return is balanced; the only non-RTS/RTL transfer in the tree is the
`$80:CC35` event dispatch `JMP ($E5A4,x)` inside `$80:CBAE`, reached only
through `$83:B5D3`. The probes confirm each exercised site: exit M/X, DB and
DP equal the static prediction, including the M1->M0 (`$99C8`), M0->M1
(`$80:EC98`, `$8E:B847`) and DB->`$7E` (MVN stub) transitions.

DP: `TDC` is observable. D's low byte becomes the dividend high byte
(`$4205`) and the multiplicand high byte of every `$83:9E1B/$9DE4` draw; D's
high byte enters table indexes through `TDC ... TAX`. With D != 0
(`$0100/$0200/$1000/$0080/$0001`) the original ran away in 34 of 40 probe
cases (no return within 40M instructions); the 6 that returned matched the
native result exactly. D = `$0000` is the only meaningful DP state. DB must
map `$05AC-$099D` to WRAM: system banks (`$83/$80/$00`, and `$96` 8/8) and
`$7E` behave identically; `$7F` and `$C0` ran away in 7 of 16 cases and the
other 9 matched. The native code computes every D/DB-relative address from
the live registers, so it is exact wherever the original terminates.

### Native structure

- `src/cave/ancient_cave.c`: `$83:9E31` and the builder phases (grid clear,
  item lists, chest contents, room rectangles, linking and corridors, link
  deduplication, start/stairs, treasure room, objects and chests, cell
  shapes, block drawing, tile sets, decorations, final tiles and sections).
- `src/cave/cave_grid.c`, `cave_objects.c`, `cave_random.c`: the bank-83
  children, each with its exact JSR frame.
- `src/field/field_sections.c`: `$80:EBAA/EC18/EC78/EC98` (shared with the
  ordinary map loader).
- `src/core/cpu_ops.h`: width-aware instruction helpers (new, additive).
- `src/cave/wram.h`: established floor, grids, counts, placement arrays and
  tile-coordinate DP names. DB-relative offsets and explicit `$7F` long
  addresses stay distinct; reused scratch remains numeric.
- `Lufia2PushedChildCall` (`execution.h`): external children whose exact JSL
  frame the native code has already pushed. `$80:93FE` and `$83:B5D3` use it.
- `Lufia2EventFlagBitFrom` adds the return bank to the verified `$80:E898`.

Kept quirks: the grid "clear" stores D; the four diagonal `TRB $55` checks
of the shape pass clear nothing (A is 0 at each TRB) and are dead; the 2x2
chest-room counter `$59` is only zeroed as a side effect of `$83:9CA0`, and
its limit of 8 never triggers because `$E734` reaches 8 first; the chest
proximity test looks at x-1/x+2 and y-1/y+2; `$83:99D3` compares an object's
size value + 1 (`LDA $E216,x; INC`) instead of its column + 1 with `$8F`,
which never matches (columns are at least 7); `ADC #$0100` (`$9831`) and
`ADC #$00C0` (`$9DD7`) have no `CLC` (their carry is provably 0);
`$4390`/`$4458`/`$474E`/`$4754` are `$7F` tile map reads (DB), not hardware;
the `STZ $28` at `$99B2` is 16-bit and also clears `$29`, so the second
resource of `$80:EC98` is never loaded from the cave.

The `$80:BFBC` list-search handoff is propagated as an exact boundary but is
not reachable from ROM map data (the builder's own `$7F:C000` writes contain
`$FF` inside the scanned window); it is not covered by whole-function cases.

### AC4 verification

Suite `Lufia2AncientCaveVerify` (consumer `decomp-verify`). Each case seeds
WRAM (RNG table and index, `$40` warmup, floor, max floor, `$05B6/$05AC/$099D`,
party lists, event and item flags, DP scratch, stale `$7F:0000-7FFF` and
`$7F:C000-C4FF`), runs the original from the `$83:B26D` JSL frame to the RTL
landing at `$83:B271` on `interp816` and the native function on a copy, and
compares A/X/Y, S, D, DB, PB, all flags and M/X, the return PC, all of WRAM,
the ordered hardware write and read streams (`$211B/$211C/$2134-$2135`,
`$4202-$4206/$4214/$4216`) and the CPU state plus WRAM digest at the
`$80:93FE` boundary. Windows acceptance strengthens the original cloud
fixture: after recording this entry, both sides execute the entire original
CPU subtree through RTL `$80:9419`, with deterministic APU replies at known
polling sites. `$83:B5D3` then runs on `interp816` from that exact returned
native state inside the child callback. No synchronous child is an identity.

The `$80:93FE` save/restore wrapper keeps A.low, X/Y, DB, DP, balanced S and C/V/D/I/M/X
on return. Its `PLB; PLP; PLY; PLX; PLA` at `$9414-$9418` leaves N/Z from
the restored A.low in the Cave's M1/X0 contract. M1 `PHA/PLA` does not save
A.high; the child can change it. Its `$941A` child and
descendants write DP scratch `$54-$66`, audio state `$0584/$0586/$0588`,
music data at `$7E:2020` (decompressor call `$943B`) and, on the tagged
sample path, `$7E:2000-$201F` (`$94B0/$94E9`). Balanced frames still leave
temporary stack writes. The verifier retains all of these effects; the
APU response fixture makes no timing or audio-emulation claim.

Structured cases: floors 1/10/11/50/90/98/99/100 x warmups 1..32 x both music
paths; every floor 1..100 at RNG index 0, `$35` and `$36` (refill on the next
call); DB `$83/$80/$00/$7E/$96`; then seeded cases to 16,384. D = `$0000`
throughout (see AC1). Standalone original-ROM cases for the shared helpers
cover inputs the cave never produces.

```text
$83:9E31 whole-function cases passed: 16384 / 16384 (floor 99 225, $80:93FE boundaries 8102, handoffs 0, unwound 0)
$83:9013 coverage: objects max 20 (full 28), chests max 8 (full 1012), event chests 121, floor-21 chests 5312, second stairs 1025, refill-boundary cases 386
$80:EC98 Lufia2FieldDecompressMapData standalone cases passed: 4096 / 4096 ($29 set in half)
$80:EBAA Lufia2FieldReadSections standalone cases passed: 4096 / 4096
$80:EC18 Lufia2FieldPackSectionAttributes standalone cases passed: 4096 / 4096
$80:EC78 Lufia2FieldSectionSize standalone cases passed: 4096 / 4096
$83:C652 Lufia2PartyListHasEntry standalone cases passed: 4096 / 4096 (RTL C691 x3445, C68D x651)
```

Return sites: `$83:9EA1` (floor 99) and `$83:9F3E`; `$80:93FE` taken as an
external boundary in 8,102 cases, `$83:B5D3` in every non-99 case. The
`$80:BFBC` handoff is not reached (see above). Off-contract probes: with
D != 0 or DB `$7F/$C0` the native result matched in every case where the
original returned (15 of 56).

Negative mutations (`tests/decomp_verify/ancient_cave_mutations.py` in the
consumer): 60/60 caught (entry and builder 34, grid 9, random 2, placement
8, sections 5, party list 2). Two more replace a `TDC` value by 0 and are
equivalent for D = 0 (the only meaningful DP). Candidates shown equivalent
while building the list, and replaced by observable variants or dropped:
`ORA #$0300` for `#$0200` (every `$91:FFCA` word has bit 8), the upper
corridor bound `#$20` (row 2 never holds a room), `$59` limit 9, the diagonal
TRB block, the `$54` reload before `$EA0F`, `CLC` before `ADC #$0100/#$00C0`
(carry provably 0), `SBC #$0F` in `$83:9B48` (the extra bit shifts out), tile
masks `#$01FF/#$00FF` (tiles read there are below `$100`), object size
`AND #$07` (no object has bit 2), a one-byte-shorter tile map clear (the map
data at `$7F:7E0E` overwrites it), `TAX` before `REP` (X16 copies all of C)
and the `$83:99D3` size test (never matches). One survivor found a real
source defect: the `$AE` chest threshold was tested twice in C; the chest
branch now follows the original compare chain.

T14 checkpoint (Linux GCC Release; no MSVC in the cloud session): all five
decomp verification suites PASS with zero failures (the four existing ones
unchanged: BBF3 semantic and bridge, actor bridges, and the actor-dispatch
output byte-identical to the pre-T14 run), 164 verified metadata entries
(163 + `$83:9E31`), zero drafts. The effective recompiler cfg is
byte-identical to T13; regenerating from it gives byte-identical generated
sources, so all 1,369 node dispositions, roots, exit-mode sets and the 122
runtime selections are unchanged. `Lufia2Recomp` Release builds. This is
the historical cloud result; the Windows acceptance checkpoint follows it.
T15 (`$85:B452`) remains queued and is not part of T14 acceptance.

### Not bound, and why

`$83:9E31` runs once per floor and is already AOT-compiled. Binding it needs
a bridge for pushed-frame children (`cpu_dispatch_call_pc_pushed`) plus a
bridge-harness reference that can stop at `$80:93FE` instead of spinning in
its APU handshake; both are new and would be verified without a gameplay
test. A later binding needs: the bridge (guard M1, X16, native mode,
decimal flag clear, DP = `$0000`), bridge/ABI cases through that harness,
and a generated graph comparison.

### Follow-ups for the cleanup milestone (not done in T14)

- Instruction helpers are spread over `core/cpu_internal.h` (partly
  fixed-width), module-private copies and the new width-aware
  `core/cpu_ops.h`; they should converge on one width-aware set.
- The consumer verifiers repeat bus/interp adapters (interp <-> native state,
  JSL frames, flag packing) per suite; `lufia2_actor_dispatch_verify.c` and
  `lufia2_actor_bridge_verify.c` are monolithic.
- Earlier milestones' mutations were manual; `ancient_cave_mutations.py`
  could become a shared runner with per-suite mutation lists.


### Windows T14 acceptance (2026-09-27)

The untouched canonical cloud heads (decomp `b1cacb3366fd3ebafbf28398584ae60a0c6f1f75`,
consumer `984f345fdb9c305a91cc7ded89e513c8b8407ba7`) passed all five MSVC
Release suites and the Windows Release build before source edits. The final
full MSVC Release checkpoint and Release build also pass, with zero failures.
All 1,369 node dispositions, roots and exit-mode sets and all 122 runtime
selections remain identical; no AOT regression. 164 verified entries, zero
drafts; Ancient Cave remains verified/unbound. T15 has not started.

The four pre-T14 verifier stdout streams and all report files are byte-identical
to the Windows baseline. The complete Cave/shared-helper stdout was separately
byte-identical after the non-semantic cleanup, before strengthening the child
fixture. Final Cave verification includes 16,384 floor cases, 8,102 real
`$80:93FE` CPU returns, and 4,096 cases for each of the five shared helpers.
256 independent ROM return probes pass: A.high changed in 256, DP scratch in
100, audio state in 63; A.low/X/Y/DB/DP and the documented flag/frame contract
match. All synchronous WRAM and stack effects are retained through `$83:B5D3`.

61/61 distinct observable mutations caught across the short run and a full
object-cap rerun; two D=0 equivalents, zero invalid checks. The shortened
corpus did not reach 20 objects, so the runner now forces the full corpus for
that mutation. The new identity-APU-child mutation is caught. CFG research
tests pass 11/11; their output is not semantic verification evidence.
No gameplay or smoke-test gate was used.

## Actor slot views

`$83:BB93` (`Lufia2UpdateActorSlots`) with `$83:AB4F`, and the `$83:C7F8` and
`$83:D508` front-ends, now read and write their per-slot fields through
`Lufia2ActorSlotView` (`src/actor/actor_slot_view.h`) and the generated
memory-map constants instead of raw array addresses; 41 of the 42 raw WRAM
and direct-page literals in these routines are gone (`$AD`, not catalogued,
remains). Every access keeps its original order, addressing mode and CPU
effects. No status or binding changed.

Original-ROM differential after the change, all output identical to the
run before it: actor-dispatch suite (`$83:BB93` 2,048/2,048, `$83:C7F8`
16,384/16,384, `$83:D508` 16,384/16,384 whole-function cases, every other
case of the suite unchanged), actor-bridge suite (`$83:BB93` 2,560/2,560,
`$83:C7F8` and `$83:D508` 8,704/8,704), and the BBF3, BBF3 bridge, visibility
and Ancient Cave suites. The suites seed DB as `$00`, `$7E`, `$80` or `$83`,
which all reach the same low WRAM, so they do not distinguish DB-relative
from fixed-bank addressing; the view keeps the original DB-relative form.
