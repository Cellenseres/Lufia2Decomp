#ifndef LUFIA2_LEGACY_FRAGMENTS_H
#define LUFIA2_LEGACY_FRAGMENTS_H

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2LegacyResolveRecordDirection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

Lufia2ExecutionResult Lufia2BattleLegacyCompareConditionByte(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldLegacyComputeRecordOffset(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleDrawLegacyTileFrame(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleBuildLegacyPopup(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldClearLegacyBackground(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2LegacyLoadRecordValue(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2LegacyAccumulateRecordValue(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2LegacyClampRecordDelta(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2LegacyCheckRecordSeparation(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2LegacyCopyRecordDeltas(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2LegacyPrepareScaledRecordDelta(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2LegacyProbeNearbyRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2LegacyRunSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2LegacyPreserveRegisters(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2LegacyInitializeState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuContinueLegacyEquipmentCursor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2ActorContinueLegacyCollisionBranch(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2FieldTakeLegacyHighButtons(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleConfigureLegacyLayerColorDma(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
