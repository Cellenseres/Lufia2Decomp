#ifndef LUFIA2_BATTLE_LIFECYCLE_DETAIL_H
#define LUFIA2_BATTLE_LIFECYCLE_DETAIL_H

#include "battle/battle_internal.h"

bool BattlePrepareOpening(BattleContext *battle);
void BattleCopyPartyFormation(BattleContext *battle);
bool BattleInitializeRecords(BattleContext *battle);
void BattleApplyScenario(BattleContext *battle);
bool BattleFinalizeSetup(BattleContext *battle);

void BattleResetDisplayWork(BattleContext *battle);
bool BattleClearDisplayBuffers(BattleContext *battle);
void BattleBuildPartyDisplayState(BattleContext *battle);
void BattleInitializeDisplayRecords(BattleContext *battle);
bool BattlePrepareDisplayRecords(BattleContext *battle);
void BattleApplyPpuTable(BattleContext *battle);
bool BattleLoadBaseGraphics(BattleContext *battle);
void BattleUploadBaseTiles(BattleContext *battle);
bool BattleLoadPresentationAssets(BattleContext *battle);
void BattlePrepareSpriteState(BattleContext *battle);
bool BattleFinishDisplay(BattleContext *battle);

#endif
