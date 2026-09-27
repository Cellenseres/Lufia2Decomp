#ifndef LUFIA2_TEXT_H
#define LUFIA2_TEXT_H

/* Text and cutscene script engine. */

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

/* $80:9CB8 text engine step; unknown children expose exact continuations. */
Lufia2ExecutionResult Lufia2TextEngineStep(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:C652: measure upcoming text. M1X0, script in DB:Y; RTS C743. */
Lufia2ExecutionResult Lufia2TextMeasure(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $80:C784: clear glyph buffer; restores P/DB, RTL C7BD. */
Lufia2ExecutionResult Lufia2TextClearGlyphBuffer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $80:C56E: tilemap row and DMA setup. M1X0, restores P/Y; RTS C5DC. */
Lufia2ExecutionResult Lufia2TextQueueWindowRow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $80:C305: framed tilemap and actor-relative tail. M1X0; DB becomes $7E. */
Lufia2ExecutionResult Lufia2TextBuildWindow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $80:C23D: returns after placement or hands off before the C2A1 frame wait. */
Lufia2ExecutionResult Lufia2TextPrepareWindow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
