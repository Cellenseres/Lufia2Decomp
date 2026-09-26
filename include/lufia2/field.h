#ifndef LUFIA2_FIELD_H
#define LUFIA2_FIELD_H

/* Field loop, NMI, scrolling, sprites and triggers. */

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

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

/* $83:A21A field OAM from the Y-sorted visible actors. */
Lufia2ExecutionResult Lufia2FieldActorSprites(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:ED9C map cell attributes of map A; M=1. */
Lufia2ExecutionResult Lufia2FieldBuildAttributes(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
