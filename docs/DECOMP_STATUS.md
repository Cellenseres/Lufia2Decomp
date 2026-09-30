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
the metadata, not this table. The notes below describe the current state.

<!-- metadata-counts:begin (scripts/metadata_index.py) -->
226 functions in `metadata/functions.toml`: 226 verified, 0 draft, 0 identified, 0 disabled.
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
| `$81:8000` | `Lufia2BattleSetup` | verified | `src/battle/battle_setup.c` |
| `$81:851E` | `Lufia2BattleDisplaySetup` | verified | `src/battle/battle_display_setup.c` |
| `$81:876B` | `Lufia2BattleExit` | verified | `src/battle/battle_exit.c` |
| `$81:8821` | `Lufia2BattleEntry` | verified | `src/battle/battle_entry.c` |
| `$81:886F` | `Lufia2BattleMainLoop` | verified | `src/battle/battle_loop.c` |
| `$81:890A` | `Lufia2BattleExecuteTurns` | verified | `src/battle/battle_actions.c` |
| `$81:A79A` | `Lufia2BattlePrepareAction` | verified | `src/battle/battle_actions.c` |
| `$81:B264` | `Lufia2BattleActiveMask` | verified | `src/battle/battle_util.c` |
| `$81:B2B5` | `Lufia2BattleTargetRecord` | verified | `src/battle/battle_util.c` |
| `$81:B2DB` | `Lufia2BattleTargetSlot` | verified | `src/battle/battle_util.c` |
| `$81:B444` | `Lufia2BattlePaletteFade` | verified | `src/battle/battle_util.c` |
| `$81:B48B` | `Lufia2BattleFadeColor` | verified | `src/battle/battle_util.c` |
| `$81:B505` | `Lufia2BattleBlend` | verified | `src/battle/battle_util.c` |
| `$81:B54A` | `Lufia2ColorToGray` | verified | `src/battle/battle_util.c` |
| `$81:B5A3` | `Lufia2BattleHideOam` | verified | `src/battle/battle_util.c` |
| `$81:B8B1` | `Lufia2BattleTargetCoordinates` | verified | `src/battle/battle_target_helpers.c` |
| `$81:B974` | `Lufia2BattleLoadPalette` | verified | `src/battle/battle_util.c` |
| `$81:B9AF` | `Lufia2BattleCommitPalettes` | verified | `src/battle/battle_util.c` |
| `$81:B9C7` | `Lufia2BattleBackgroundPrepare` | verified | `src/battle/battle_background.c` |
| `$81:BAE8` | `Lufia2BattlePortraits` | verified | `src/battle/battle_portrait.c` |
| `$81:BAFB` | `Lufia2BattlePortrait` | verified | `src/battle/battle_portrait.c` |
| `$81:BB75` | `Lufia2BattlePortraitUpload` | verified | `src/battle/battle_portrait.c` |
| `$81:BD47` | `Lufia2BattleSpriteBlockFar` | verified | `src/battle/battle_util.c` |
| `$81:BD4B` | `Lufia2BattleSpriteBlock` | verified | `src/battle/battle_util.c` |
| `$81:BE54` | `Lufia2BattleTileBlockFar` | verified | `src/battle/battle_util.c` |
| `$81:BE58` | `Lufia2BattleTileBlock` | verified | `src/battle/battle_util.c` |
| `$81:BEBC` | `Lufia2BattleCommandTiles` | verified | `src/battle/battle_command_display.c` |
| `$81:BEED` | `Lufia2BattleActionMenuTiles` | verified | `src/battle/battle_command_display.c` |
| `$81:BF3F` | `Lufia2BattleItemCommands` | verified | `src/battle/battle_command_lists.c` |
| `$81:C031` | `Lufia2BattleSpellCommands` | verified | `src/battle/battle_command_lists.c` |
| `$81:C129` | `Lufia2BattleIpSkills` | verified | `src/battle/battle_ip.c` |
| `$81:C240` | `Lufia2BattlePrepareNextFrame` | verified | `src/battle/battle_loop_children.c` |
| `$81:C254` | `Lufia2BattleQueueEnemyTurns` | verified | `src/battle/battle_turn_order.c` |
| `$81:C294` | `Lufia2BattleQueueCapsuleTurn` | verified | `src/battle/battle_turn_order.c` |
| `$81:C2C0` | `Lufia2BattleLoadDisplayDefaults` | verified | `src/battle/battle_buffers.c` |
| `$81:C2D0` | `Lufia2BattleClearBackgroundTilemap` | verified | `src/battle/battle_buffers.c` |
| `$81:C2E3` | `Lufia2BattleResetPartyTilemap` | verified | `src/battle/battle_buffers.c` |
| `$81:C2FB` | `Lufia2BattleClearWindowTilemap` | verified | `src/battle/battle_buffers.c` |
| `$81:C30E` | `Lufia2BattleClearTilemap3800` | verified | `src/battle/battle_buffers.c` |
| `$81:C35F` | `Lufia2BattlePopups` | verified | `src/battle/battle_popup.c` |
| `$81:C5CF` | `Lufia2BattleTargetPointer` | verified | `src/battle/battle_util.c` |
| `$81:C600` | `Lufia2BattleStatusTick` | verified | `src/battle/battle_status_tick.c` |
| `$81:C739` | `Lufia2BattleCollectCommands` | verified | `src/battle/battle_commands.c` |
| `$81:CB77` | `Lufia2BattleChooseCommand` | verified | `src/battle/battle_command.c` |
| `$81:CC2E` | `Lufia2BattleChoosePartyAction` | verified | `src/battle/battle_party_commands.c` |
| `$81:D12F` | `Lufia2BattleActionSubmenuStart` | verified | `src/battle/battle_submenus.c` |
| `$81:D19A` | `Lufia2BattleActionSubmenuResume` | verified | `src/battle/battle_submenus.c` |
| `$81:D4E0` | `Lufia2BattleChooseTargets` | verified | `src/battle/battle_targets.c` |
| `$81:D920` | `Lufia2BattleEnemyCursor` | verified | `src/battle/battle_target_helpers.c` |
| `$81:D92C` | `Lufia2BattlePartyCursor` | verified | `src/battle/battle_target_helpers.c` |
| `$81:D938` | `Lufia2BattleEnemyMarkedCursor` | verified | `src/battle/battle_target_helpers.c` |
| `$81:D948` | `Lufia2BattlePartyMarkedCursor` | verified | `src/battle/battle_target_helpers.c` |
| `$81:D975` | `Lufia2BattleConfirmCommand` | verified | `src/battle/battle_target_helpers.c` |
| `$81:D9D0` | `Lufia2BattleCommandFrame` | verified | `src/battle/battle_target_helpers.c` |
| `$81:D9E1` | `Lufia2BattleResults` | verified | `src/battle/battle_results.c` |
| `$81:DD7F` | `Lufia2BattleResultWindowPrepare` | verified | `src/battle/battle_result_window.c` |
| `$81:DDE7` | `Lufia2BattleResultWindowLine` | verified | `src/battle/battle_result_window.c` |
| `$81:DE55` | `Lufia2BattleResultWindowScroll` | verified | `src/battle/battle_result_window.c` |
| `$81:DE9E` | `Lufia2BattleResultWindowWait` | verified | `src/battle/battle_result_window.c` |
| `$81:DEF4` | `Lufia2BattleActionWindow` | verified | `src/battle/battle_command_display.c` |
| `$81:DF0A` | `Lufia2BattlePartyWindows` | verified | `src/battle/battle_command_display.c` |
| `$81:DFA2` | `Lufia2BattleListRows` | verified | `src/battle/battle_ip.c` |
| `$81:E16F` | `Lufia2BattleClearActionWindow` | verified | `src/battle/battle_command_display.c` |
| `$81:E3AE` | `Lufia2BattleWindowE3AE` | verified | `src/battle/battle_frame_rows.c` |
| `$81:E3CD` | `Lufia2BattleWindowE3CD` | verified | `src/battle/battle_frame_rows.c` |
| `$81:E3EC` | `Lufia2BattleTileWindow` | verified | `src/battle/battle_frame_rows.c` |
| `$81:E405` | `Lufia2BattleTileFrame` | verified | `src/battle/battle_frame_rows.c` |
| `$81:E479` | `Lufia2BattleFrameTop` | verified | `src/battle/battle_frame_rows.c` |
| `$81:E4AD` | `Lufia2BattleFrameSides` | verified | `src/battle/battle_frame_rows.c` |
| `$81:E4D1` | `Lufia2BattlePartyName` | verified | `src/battle/battle_command_display_helpers.c` |
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
| `$83:83E0` | `Lufia2FieldEncounterHandoff` | verified | `src/field/encounter.c` |
| `$83:845B` | `Lufia2EncounterBattleSequence` | verified | `src/field/encounter.c` |
| `$83:85DC` | `Lufia2FieldReloadSetup` | verified | `src/field/field_update.c` |
| `$83:867B` | `Lufia2FieldTakeButtons` | verified | `src/field/field_update.c` |
| `$83:8682` | `Lufia2FieldAnimationTicks` | verified | `src/field/field_update.c` |
| `$83:9E31` | `Lufia2AncientCaveGenerateFloor` | verified | `src/cave/ancient_cave.c` |
| `$83:9FA9` | `Lufia2FieldNmiUploads` | verified | `src/field/field_nmi.c` |
| `$83:A21A` | `Lufia2FieldActorSprites` | verified | `src/field/field_sprites.c` |
| `$83:A746` | `Lufia2ActorSyncFinePosition` | verified | `src/actor/actor_movement.c` |
| `$83:A9BA` | `Lufia2ActorLoadSprite` | verified | `src/actor/actor_sprites.c` |
| `$83:AEB5` | `Lufia2FieldColourEffects` | verified | `src/field/field_effects.c` |
| `$83:B062` | `Lufia2FieldRestore` | verified | `src/field/field_restore.c` |
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
| `$84:8BC7` | `Lufia2BattleVisualTransition` | verified | `src/battle/battle_transition.c` |
| `$85:89E5` | `Lufia2BattleClearSpriteOffsets` | verified | `src/battle/battle_loop_children.c` |
| `$85:8A2F` | `Lufia2BattleSprites` | verified | `src/battle/battle_frame.c` |
| `$85:8DC5` | `Lufia2BattleNmiUploads` | verified | `src/battle/battle_nmi.c` |
| `$85:9236` | `Lufia2BattlePartyStatusGate` | verified | `src/battle/battle_loop_children.c` |
| `$85:9275` | `Lufia2BattleQueuePartyTurns` | verified | `src/battle/battle_turn_order.c` |
| `$85:93B7` | `Lufia2BattleCheckOutcome` | verified | `src/battle/battle_outcome.c` |
| `$85:96A2` | `Lufia2BattleSaveWorkArea` | verified | `src/battle/battle_loop_children.c` |
| `$85:96B0` | `Lufia2BattleRestoreWorkArea` | verified | `src/battle/battle_loop_children.c` |
| `$85:9CD7` | `Lufia2BattleQueueActionWindow` | verified | `src/battle/battle_command_display_helpers.c` |
| `$85:9CEE` | `Lufia2BattleQueueListWindow` | verified | `src/battle/battle_command_display_helpers.c` |
| `$85:AB78` | `Lufia2BattleStageTransfer` | verified | `src/battle/battle_loop_children.c` |
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

## Current checkpoint

The full Windows Release verifier passes **333 independent jobs**. The normal
application build also passes. The consumer selects 188 functions, including
the complete command-collection and turn-execution callers. The generated CFG
contains 1,403 nodes; no previously eligible AOT function was lost.

Verification uses the original ROM interpreter as the reference. It checks CPU
state, complete WRAM and, where needed, the order of memory and register writes.
Tests also force child calls to unwind and inject deliberate mistakes to check
that the comparisons can detect them. Detailed run logs belong in the consumer's
worklog rather than this status page.

## Battle commands and targets

The battle lifecycle, main loop, turn execution and command-selection callers
are reconstructed. Unknown children remain explicit calls with their original
stack frames. A verified caller does not make those children verified.

The latest work covers command collection, party action choices, spell/item/IP
submenus and target selection. It retains the original polling, scrolling,
backtracking, invalid-target skipping and side changes. Target cancellation can
remove a mark and continue; pair selection waits for exactly two party members.

| Slice | ROM comparison cases | Result |
| --- | ---: | --- |
| Main-loop children (BL6.1) | 8,576 | pass |
| Turn/command control (BL6.2) | 5,120 + 128 control probes | pass |
| Command collection (BL6.3) | 4,159 | pass |
| Party action selection (BL6.4) | 4,151 + 128 stack-read probes | pass |
| Action submenus (BL6.5) | 4,119 | pass |
| Target selection (BL6.6) | 8,207 | pass |
| Target/confirmation helpers (BL6.7) | 13,326 | pass |
| Command/turn runtime bridges (BL6.8) | 15,615 ABI checks | pass |
| Item/spell command lists (BL6.9) | 4,100 | pass |
| Command display helpers (BL6.10) | 6,666 | pass |
| Party names/window uploads (BL6.11) | 8,196 | pass |
| Result windows (BL6.12) | 6,159 | pass |
| Experience/gold/results (BL6.13) | 5,746 | pass |

Target coordinates, all four cursor variants, command frame upkeep and the
confirmation prompt are also reconstructed. They retain original coordinate
wrapping, hardware register accesses, title drawing and input waits. Item and
spell lists also preserve availability checks, MP limits and hardware division.
Window setup and cleanup are reconstructed, with explicit upload and frame-wait
children. Party names and tilemap queue entries are also reconstructed; full
queues retain the original BRK handoff. The result caller now covers rewards, party and capsule progression, gold and
result messages. Its level, sound and item children remain explicit calls.

The two new runtime bridges require A8, X/Y16, DB `$97`, DP zero and native binary
arithmetic. Other states use the original interpreter. Their tests cover stack
frames, child unwinds and three host-return modes, detecting ten deliberate
bridge mistakes. The other recent command helpers remain unbound. Declarations
and metadata give each entry's contract.

Some unusual original exits matter:

- Command collection can jump to `$81:8855` with saved frames still on the stack.
- Malformed command selection can reach BRK `$81:C8BC`; its successful branch
  starts inside that instruction's operand at `$C8BD`.
- Party action selection can reach BRK `$81:CF78` or the original `$D12C` self-loop.
- Unreachable submenu blocks at `$D1E0` and `$D3FE` remain unreachable.
- Stack-relative word operands wrap within bank zero, including their high byte.

The reconstruction preserves these behaviors. Rendering readiness, widescreen
policy and optional performance patches are consumer responsibilities.

## Text and menus

The text engine includes character rendering, dictionary expansion, windows,
prompts and conditional expressions. Unsupported script paths hand back to the
original code at explicit boundaries.

Window preparation deliberately stops at `$80:C2A1` before the frame waits,
keeping the outstanding frames intact. Window construction retains both width
interpretations at `$C431`; it does not repair the unusual original path.
Measurement, glyph clearing, row upload and item-possession helpers have their
own ROM comparisons. Conditional expressions retain all eight comparators and
original wraparound behavior.

Menu strings, windows, inventory, equipment and capsule helpers are listed in
the function index. Runtime use is decided by the consumer's bindings file.

## Ancient Cave

The floor generator `$83:9E31` is verified and unbound. It uses A8, X/Y16,
DP zero and a DB that maps low WRAM. Verification covers 16,384 floors, five
helper groups of 4,096 cases each, and APU-return probes.

The APU handshake and map loader remain explicit children. Binding this entry
still needs a consumer bridge and ABI tests; its semantic verification alone
is insufficient. Original generator loops and degenerate-map behavior are
preserved. Host decoding or caching belongs in a separate patch layer.

## Actors, field and shared helpers

Actor routines use slot views and generated memory-map constants while keeping
the original addressing and access order. The field loop, scene NMIs, scrolling,
world-map helpers and shared resource/menu/math routines are covered by the
consumer's differential and bridge suites. The index is the source of truth for
individual functions.

Further battle work closes the remaining child dependencies before considering
new runtime bindings. See [ARCHITECTURE.md](ARCHITECTURE.md) for the boundary
model and [CAPTURE_RESEARCH.md](CAPTURE_RESEARCH.md) for scene evidence.
