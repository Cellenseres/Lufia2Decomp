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

Lufia2ExecutionResult Lufia2TextEngineStepWithServices(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

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

/* Legacy placement API; scene mode retains its original continuation. */
Lufia2ExecutionResult Lufia2TextPrepareWindow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2TextClearWindowBuffer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2TextExpandSceneString(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2TextDrawSceneGlyph(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2TextWriteSceneWindowRow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2TextClearUploadRows(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2TextUpdateSceneLabel(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2TextRenderSceneLabel(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2TextBuildSceneWindowInterior(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu,
                                                         Lufia2PushedChildCall child,
                                                         void *context);

Lufia2ExecutionResult Lufia2TextPrepareWindowWithServices(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu,
                                                          Lufia2PushedChildCall child,
                                                          void *context);

Lufia2ExecutionResult Lufia2TextCloseWindowWithServices(const Lufia2Memory *memory,
                                                        Lufia2CpuState *cpu,
                                                        Lufia2PushedChildCall child,
                                                        void *context);

Lufia2ExecutionResult Lufia2TextOpenWindowWithServices(const Lufia2Memory *memory,
                                                       Lufia2CpuState *cpu,
                                                       Lufia2PushedChildCall child,
                                                       void *context);

/* Original coordinate helpers; DP0, binary, X16. */
Lufia2ExecutionResult Lufia2TextWindowTileOffset(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2TextDrawWindowBorderRow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2TextPlaceActorFromScript(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2TextPlaceNextSceneActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2TextSetWindowMode(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2AlternateDisplayMeasureText(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2AlternateDisplayDrawText(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2AlternateDisplayDrawTileStrip(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2AlternateDisplayUploadTilemap(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2AlternateDisplayAdvanceRow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
