# Entries for the consumer

This page lists every routine added as a `draft` since the semantic layer was
split off, with the entry the consumer has to provide to use it. It is
derived from `metadata/functions.toml` and the headers in `include/lufia2/`;
the metadata stays the source of truth. No consumer changes are part of this
repository.

## Entry contract

Every routine has the signature

```c
Lufia2ExecutionResult Name(const Lufia2Memory *memory, Lufia2CpuState *cpu);
```

and is declared in the header named in the tables. The CPU state at the call
is the state at the original entry address, with the return frame of the
original call already on the stack:

- `JSR / RTS` entries return `LUFIA2_EXECUTION_RETURNED` with the address of
  the final `RTS`; the consumer pops the frame.
- `JSL / RTL` entries return the address of the final `RTL` in the same way.
- A routine whose entry contract is narrower than the original (a fixed M/X
  width, a data bank or a stub layout) returns `LUFIA2_EXECUTION_BOUNDARY`
  with `resume_pc` at its own entry when the state does not match, so the
  consumer continues in the original code.
- Calls to other routines are made inside the routine with the call frame
  written to the stack as the original does. A child that is reconstructed is
  called directly; any other child goes through the consumer's child-call
  callback (`core/child_call.h`) or is left to the original code by a
  hand-off. Only the entries in the tables below are needed from the
  consumer; the internal sections of a routine are not entries.
- Mode columns give the M/X widths at entry and exit as in the metadata
  (`M1` is an 8-bit accumulator, `X0` is 16-bit index registers).

Every entry below is `draft`: the reconstruction is written and tested against
the original ROM by the maintainer, but a consumer should bind it only after
its own differential check.

## Special cases

- `$00:057D` (`Lufia2RamBlockMove`) is the block-move stub that the game
  copies into WRAM. The routine expects the bytes `54 xx xx 60` there and
  hands off otherwise. It is reached by `JSR $057D` from several banks and
  returns at the `RTS` of the stub, in the bank of the caller.
- Routines marked table or jump entry have no direct `JSR` or `JSL` caller in
  the ROM. They are the world map plane row routines and the battle effect
  interpreter handlers, which the game reaches through handler tables.

## World map (`world_map.h`)

| Address | Function | Call | Entry | Exit |
| --- | --- | --- | --- | --- |
| `$86:995B` | `Lufia2WorldScrollAdvance` | JSR / RTS | M1X0 | M1X0 |
| `$86:A417` | `Lufia2WorldStepOffsets` | JSR / RTS | M1X0 | M1X0 |
| `$86:A583` | `Lufia2WorldProduct16By8` | JSR / RTS | M1X0 | M1X0 |
| `$86:A5A9` | `Lufia2WorldMapDivide32` | JSR / RTS | M0X0 | M0X0 |
| `$86:A894` | `Lufia2WorldMapPlane` | JSR / RTS | M1X0 | M1X0 |
| `$86:A9B0` | `Lufia2WorldPlaneRows0` | table or jump entry | M0X0 | M0X0 |
| `$86:AA5B` | `Lufia2WorldPlaneRows1` | table or jump entry | M0X0 | M0X0 |
| `$86:AB0E` | `Lufia2WorldPlaneRows2` | table or jump entry | M0X0 | M0X0 |
| `$86:ABC1` | `Lufia2WorldPlaneRows3` | table or jump entry | M0X0 | M0X0 |
| `$86:E0B9` | `Lufia2WorldMapStartAnimation` | JSR / RTS | M1X0 | M1X0 |
| `$86:E11F` | `Lufia2WorldMapStepAnimations` | JSR / RTS | M1X0 | M1X0 |
| `$86:E1B9` | `Lufia2WorldMapUpdateObjects` | JSR / RTS | M1X0 | M1X0 |
| `$86:E287` | `Lufia2WorldMapTestObjects` | JSR / RTS | M0X0 | M0X0 |
| `$86:E295` | `Lufia2WorldMapTestObject` | JSR / RTS | M0X0 | M0X0 |
| `$86:E2D2` | `Lufia2WorldMapProjectObjects` | JSR / RTS | M0X0 | M0X0 |
| `$86:E3AB` | `Lufia2WorldMapDrawObjects` | JSR / RTS | M0X0 | M0X0 |
| `$86:E3D2` | `Lufia2WorldMapDrawObjectByKind` | JSR / RTS | M0X0 | M0X0 |
| `$86:E430` | `Lufia2WorldMapAssignSlot` | JSR / RTS | M0X0 | M0X0 |
| `$86:E479` | `Lufia2WorldMapDrawSprite` | JSR / RTS | M0X0 | M0X0 |
| `$86:E4E7` | `Lufia2WorldMapDrawSmallSprite` | JSR / RTS | M0X0 | M0X0 |
| `$86:E555` | `Lufia2WorldMapDrawSpritePair` | JSR / RTS | M0X0 | M0X0 |
| `$86:E5BB` | `Lufia2WorldMapStoreHighBits` | JSR / RTS | M0X0 | M0X0 |
| `$86:E640` | `Lufia2WorldMapClearSlotFlags` | JSR / RTS | M1X0 | M1X0 |
| `$86:E650` | `Lufia2WorldMapClearSprites` | JSR / RTS | M1X0 | M1X0 |
| `$86:E686` | `Lufia2WorldMapSortVisible` | JSR / RTS | M0X0 | M0X0 |

## System (`system.h`)

| Address | Function | Call | Entry | Exit |
| --- | --- | --- | --- | --- |
| `$00:057D` | `Lufia2RamBlockMove` | JSR / RTS in WRAM | M1X0 | M1X0 |
| `$80:8703` | `Lufia2NmiSpritesPaletteAndPads` | JSR / RTS | M1X1 | M1X1 |
| `$80:87A7` | `Lufia2NmiScrollAndUploads` | JSR / RTS | M1X1 | M1X1 |
| `$80:87FC` | `Lufia2NmiTilemapUploads` | JSR / RTS | M1X0 | M1X0 |

## Battle (`battle.h`)

| Address | Function | Call | Entry | Exit |
| --- | --- | --- | --- | --- |
| `$81:8E92` | `Lufia2BattleActorSprites` | JSR / RTS | M1X0 | M1X0 |
| `$81:8EEA` | `Lufia2BattleActorSprite` | JSR / RTS | M0X0 | M0X0 |
| `$81:9169` | `Lufia2BattleEffectMarkLoop` | table or jump entry | M1X0 | M1X0 |
| `$81:953F` | `Lufia2BattleEffectRepeat` | table or jump entry | M1X0 | M1X0 |
| `$81:A40B` | `Lufia2BattleEffectAddToField` | table or jump entry | M1X0 | M1X0 |
| `$81:A598` | `Lufia2BattleEffectVelocity` | table or jump entry | M1X0 | M1X0 |
| `$81:B396` | `Lufia2BattlePaletteBrightness` | JSR / RTS | M1X0 | M1X0 |
| `$81:B3F8` | `Lufia2BattleScaleColor` | JSR / RTS | M1X0 | M0X0 |
| `$81:BCCC` | `Lufia2BattleBlitTileRows` | JSL / RTL | M1X0 | M1X0 |
| `$85:894A` | `Lufia2BattleDriftRecords` | JSL / RTL | M1X0 | M1X0 |
| `$85:8A39` | `Lufia2BattleFrameSetup` | JSL / RTL | M1X0 | M1X0 |
| `$85:8AAF` | `Lufia2BattleColorsInit` | JSL / RTL | M1X0 | M1X0 |
| `$85:8AF4` | `Lufia2BattleColorsParty` | JSL / RTL | M1X0 | M1X0 |
| `$85:8B22` | `Lufia2BattleColorsMonster` | JSL / RTL | M1X0 | M1X0 |
| `$85:8B4B` | `Lufia2BattleSpriteRecordsEntry` | JSL / RTL | M1X0 | M1X0 |
| `$85:8BC0` | `Lufia2BattleSpriteSingleEntry` | JSL / RTL | M1X0 | M1X0 |
| `$85:8C27` | `Lufia2BattleSpriteMarkersEntry` | JSL / RTL | M1X0 | M1X0 |
| `$85:8C98` | `Lufia2BattleSpritePartyEntry` | JSL / RTL | M1X0 | M1X0 |
| `$85:8D2E` | `Lufia2BattlePartyTilemapEntry` | JSL / RTL | M1X0 | M1X0 |
| `$85:8F4A` | `Lufia2BattleRandomBit` | JSR / RTS | M1X0 | M1X0 |
| `$85:972E` | `Lufia2BattleTileGridEntry` | JSL / RTL | M1X0 | M1X0 |
| `$85:9790` | `Lufia2BattleTileRow` | JSR / RTS | M0X0 | M0X0 |
| `$85:A736` | `Lufia2BattleRippleRow` | JSR / RTS | M1X0 | M1X0 |
| `$85:AA3D` | `Lufia2BattleRippleWords` | JSR / RTS | M1X0 | M1X0 |
| `$85:ADE1` | `Lufia2BattleWaveBackward` | JSR / RTS | M1X0 | M1X0 |
| `$85:AE68` | `Lufia2BattleWaveForward` | JSR / RTS | M1X0 | M1X0 |
| `$85:AEEB` | `Lufia2BattleWaveFill` | JSR / RTS | M1X0 | M1X0 |
| `$85:B208` | `Lufia2BattleCircleWindow` | JSR / RTS | M1X0 | M1X0 |
| `$85:B26D` | `Lufia2BattleCircleWidths` | JSR / RTS | M1X0 | M1X0 |
| `$85:DD63` | `Lufia2BattleVelocityOfAngle` | JSL / RTL | M1X0 | M1X0 |
| `$85:DE1E` | `Lufia2BattleCosineOfAngle` | JSL / RTL | M1X0 | M1X0 |
| `$85:DE2A` | `Lufia2BattleSineOfAngle` | JSL / RTL | M1X0 | M1X0 |

## Menu (`menu.h`)

| Address | Function | Call | Entry | Exit |
| --- | --- | --- | --- | --- |
| `$82:8000` | `Lufia2MenuMultiply` | JSL / RTL | M1X0 | M1X0 |
| `$82:8044` | `Lufia2MenuQueueVideoWrite` | JSL / RTL | M1X0 | M1X0 |
| `$82:8069` | `Lufia2MenuTileGridFill` | JSR / RTS | M1X0 | M1X0 |
| `$82:80A5` | `Lufia2MenuTileBlockFill` | JSR / RTS | M0X0 | M0X0 |
| `$82:80CA` | `Lufia2MenuRecolorRect` | JSR / RTS | M0X0 | M1X0 |
| `$82:838F` | `Lufia2MenuClearLayers` | JSR / RTS | M1X0 | M1X0 |
| `$82:8720` | `Lufia2MenuCursor` | JSR / RTS | M1X0 | M1X0 |
| `$82:88A0` | `Lufia2MenuItemIndex` | JSR / RTS | M1X0 | M1X0 |
| `$82:88CB` | `Lufia2MenuItemPosition` | JSR / RTS | M1X0 | M1X0 |
| `$82:89FA` | `Lufia2MenuCursorSlide` | JSR / RTS | M1X0 | M1X0 |
| `$82:8AD8` | `Lufia2MenuSlideCorrectX` | JSR / RTS | M1X0 | M1X0 |
| `$82:8AE9` | `Lufia2MenuSlideCorrectY` | JSR / RTS | M1X0 | M1X0 |
| `$82:8AFA` | `Lufia2MenuSlideCount` | JSR / RTS | M1X0 | M1X0 |
| `$82:8B08` | `Lufia2MenuInputLoop` | JSR / RTS | M1X0 | M1X0 |
| `$86:8DD7` | `Lufia2MenuScreenSetup` | JSL / RTL | M1X0 | M1X0 |
| `$86:8E6B` | `Lufia2SpriteClearSlots` | JSL / RTL | M1X0 | M1X0 |
| `$86:8F6F` | `Lufia2MenuLoadImageSet` | JSL / RTL | M1X0 | M1X0 |
| `$86:8FF6` | `Lufia2MenuCopyImageBlock` | JSR / RTS | M1X0 | M0X0 |
| `$86:9009` | `Lufia2MenuCopyImageRow256` | JSR / RTS | M1X0 | M1X0 |
| `$86:9022` | `Lufia2MenuLoadImageGrid` | JSL / RTL | M1X0 | M1X0 |
| `$86:906A` | `Lufia2MenuCopyImageRow128` | JSR / RTS | M1X0 | M0X0 |
| `$86:90C0` | `Lufia2MenuLoadPalette0` | JSL / RTL | M1X0 | M1X0 |
| `$86:90D3` | `Lufia2MenuLoadPalette1` | JSL / RTL | M1X0 | M1X0 |
| `$86:90E6` | `Lufia2MenuLoadPalette2` | JSL / RTL | M1X0 | M1X0 |
| `$86:90F9` | `Lufia2MenuLoadPalette3` | JSL / RTL | M1X0 | M1X0 |
| `$86:910C` | `Lufia2MenuLoadPalette4` | JSL / RTL | M1X0 | M1X0 |
| `$86:911F` | `Lufia2MenuLoadSlotPalettes` | JSL / RTL | M1X0 | M1X0 |

## Field (`field.h`)

| Address | Function | Call | Entry | Exit |
| --- | --- | --- | --- | --- |
| `$80:C195` | `Lufia2FieldMarkObjectSlots` | JSL / RTL | M1X0 | M1X0 |
| `$80:ED0E` | `Lufia2FieldUnpackAttributes` | JSL / RTL | M1X0 | M1X0 |
| `$80:F821` | `Lufia2FieldTraceCellEdges` | JSL / RTL | M1X0 | M1X0 |
| `$83:F9D0` | `Lufia2FieldCellPointer` | JSL / RTL | M1X0 | M1X0 |
| `$86:94D4` | `Lufia2SceneTrackStep` | JSR / RTS | M1X0 | M1X0 |
| `$86:A791` | `Lufia2SceneViewOrigin` | JSR / RTS | M1X0 | M1X0 |

## Party (`party.h`)

| Address | Function | Call | Entry | Exit |
| --- | --- | --- | --- | --- |
| `$81:F481` | `Lufia2PartyStatTotalsOfCopy` | JSL / RTL | M1X0 | M1X0 |
