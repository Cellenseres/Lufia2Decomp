#ifndef LUFIA2_CARD_TABLE_H
#define LUFIA2_CARD_TABLE_H

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2CardTableRunSession(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);
Lufia2ExecutionResult Lufia2CardTableRunAlternateSession(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2CardTableUpdateHdma(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2CardTableBuildHdma(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2CardTableLoadAlternateTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2CardTableInitialize(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);
Lufia2ExecutionResult Lufia2CardTableLoadGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);
Lufia2ExecutionResult Lufia2CardTableLoadAlternateGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);
Lufia2ExecutionResult Lufia2CardTableWaitFrame(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2CardTableAdvanceColumns(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);
Lufia2ExecutionResult Lufia2CardTableRefreshColumns(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);
Lufia2ExecutionResult Lufia2CardTableDrawColumn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2CardTableResetRoundBuffers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);
Lufia2ExecutionResult Lufia2CardTableRestoreLineAttributes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);
Lufia2ExecutionResult Lufia2CardTableAnimatePanelAttributes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2CardTableAnimateLineAttributes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);
Lufia2ExecutionResult Lufia2CardTableAnimateAccentAttributes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);
Lufia2ExecutionResult Lufia2CardTablePaintTileAttributes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
