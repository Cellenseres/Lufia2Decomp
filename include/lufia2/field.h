#ifndef LUFIA2_FIELD_H
#define LUFIA2_FIELD_H

/* Field loop, NMI, scrolling, sprites and triggers. */

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

/* $83:B062 restores Field display state and republishes its NMI callback. */
Lufia2ExecutionResult Lufia2FieldRestore(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context);

/* $83:83E0 encounter handoff; the $83:83EB child uses its pushed JSL frame. */
Lufia2ExecutionResult Lufia2FieldEncounterHandoff(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context);

/* $83:845B encounter transition through Battle and Field continuation. */
Lufia2ExecutionResult Lufia2EncounterBattleSequence(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context);

/* $83:80CD field idle test (X8); zero = idle. */
Lufia2ExecutionResult Lufia2FieldIdleTest(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:83A0 field menu request check. */
Lufia2ExecutionResult Lufia2FieldMenuRequest(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:867B consume pressed buttons in A. */
Lufia2ExecutionResult Lufia2FieldTakeButtons(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:8103 field $05B7 requests. */
Lufia2ExecutionResult Lufia2FieldStatusRequests(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:85DC field reload setup; loading stays LLE. */
Lufia2ExecutionResult Lufia2FieldReloadSetup(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:8682 animation slot ticks at $7F:D057. */
Lufia2ExecutionResult Lufia2FieldAnimationTicks(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:9C72 idle frame: effects, event timer, text gate. */
Lufia2ExecutionResult Lufia2FieldEventTick(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:CBAE event slot timers; a due slot runs on LLE. */
Lufia2ExecutionResult Lufia2FieldEventTimerTick(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:81C6 field trigger checks; exact LLE boundaries. */
Lufia2ExecutionResult Lufia2FieldTriggerUpdate(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:B66E stair rectangles at $7E:F000. */
Lufia2ExecutionResult Lufia2FieldStairRects(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:B711 event rectangles; hits run on LLE. */
Lufia2ExecutionResult Lufia2FieldEventRects(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:B747 area rectangles; hits run on LLE. */
Lufia2ExecutionResult Lufia2FieldAreaRects(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:AEB5 palette cycles and HDMA wave table. */
Lufia2ExecutionResult Lufia2FieldColourEffects(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:9FA9 field NMI uploads (DMA, fade, windows). */
Lufia2ExecutionResult Lufia2FieldNmiUploads(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $8E:BD77 BG scroll targets for three layers. */
Lufia2ExecutionResult Lufia2FieldScrollUpdate(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:F4FD/F518/F589/F5A2: queue the newly exposed map edge. */
Lufia2ExecutionResult Lufia2FieldStreamRightColumn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2FieldStreamLeftColumn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2FieldStreamTopRow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2FieldStreamBottomRow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $80:F47A / $83:8E66: rebuild one layer or all four layers; M=1. */
Lufia2ExecutionResult Lufia2FieldRedrawLayer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2FieldRedrawAllLayers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $83:A21A field OAM from the Y-sorted visible actors. */
Lufia2ExecutionResult Lufia2FieldActorSprites(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* Optional host visibility filter; none keeps stock behavior. */
typedef struct Lufia2FieldActorVisibility {
    uint16_t horizontal_padding;
    uint8_t (*accept)(void *context, uint16_t world_x, uint16_t world_y);
    void *context;
} Lufia2FieldActorVisibility;

Lufia2ExecutionResult Lufia2FieldActorSpritesWithVisibility(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    const Lufia2FieldActorVisibility *visibility);

/* $83:B5D3: map header copy, object attributes and startup events. */
Lufia2ExecutionResult Lufia2FieldLoadMapHeader(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

/* $80:E722: queue the event list entry Y/2, using slot zero if full. */
Lufia2ExecutionResult Lufia2FieldStartEvent(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $80:E844: clear event work records and select the map event script. */
Lufia2ExecutionResult Lufia2FieldInitializeMapEvents(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $80:ED9C map cell attributes of map A; M=1. */
Lufia2ExecutionResult Lufia2FieldBuildAttributes(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
