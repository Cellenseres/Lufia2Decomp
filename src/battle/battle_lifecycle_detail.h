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

bool BattleBackgroundIsBlank(BattleContext *battle, uint8_t *background_id);
void BattleClearBackground(BattleContext *battle);
bool BattleLoadBackground(BattleContext *battle, uint8_t background_id);
bool BattleFinishBackground(BattleContext *battle);

typedef enum BattleFrameResult {
    BATTLE_FRAME_UNWOUND = 0,
    BATTLE_FRAME_FINISHED,
    BATTLE_FRAME_CONTINUE,
} BattleFrameResult;

void BattleSaveLoopStack(BattleContext *battle);
bool BattleBeginMainLoop(BattleContext *battle);
BattleFrameResult BattleRunFrame(BattleContext *battle);
void BattleClearTilemap(BattleContext *battle);
bool BattleFinishMainLoop(BattleContext *battle);

void BattleBeginSession(BattleContext *battle);
void BattleRestoreSessionStack(BattleContext *battle);
void BattleEndSession(BattleContext *battle);

bool BattlePrepareExit(BattleContext *battle);
bool BattleFadeOut(BattleContext *battle);
bool BattleTearDownDisplay(BattleContext *battle);

#endif
