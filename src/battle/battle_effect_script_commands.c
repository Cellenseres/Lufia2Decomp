#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/battle.h"
#include "lufia2/battle.h"

enum {
    DP_SPAWN_STREAM = 0xc3u,
    SPAWN_SCRIPT = WRAM_BATTLE_EFFECT_SPAWN_STREAM,
    SPAWN_POSITION = WRAM_BATTLE_EFFECT_SPAWN_POSITION,
    SPAWN_ANGLE = WRAM_BATTLE_EFFECT_SPAWN_ATTRIBUTES
};

static uint8_t ScriptCommandContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x81u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && cpu->direct_page == 0u &&
        cpu->stack >= 0x1f02u && cpu->stack <= 0x1ffcu;
}

static void AdvanceSpawnStream(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpStepMem(memory, cpu, OpDp(cpu, DP_SPAWN_STREAM), 1);
}

static void ReadSpawnWord(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_SPAWN_STREAM));
    AdvanceSpawnStream(memory, cpu);
    AdvanceSpawnStream(memory, cpu);
}

Lufia2ExecutionResult Lufia2BattleEffectSpawnMovingActorFromStream(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ScriptCommandContext(cpu))
        return ExecutionHandoff(cpu, 0x81a1ccu);
    OpRepWidths(cpu, 0x20u);
    ReadSpawnWord(memory, cpu);
    OpSta(memory, cpu, OpAbs(cpu, SPAWN_SCRIPT));
    ReadSpawnWord(memory, cpu);
    OpSta(memory, cpu, OpAbs(cpu, SPAWN_POSITION));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_SPAWN_STREAM));
    AdvanceSpawnStream(memory, cpu);
    OpSta(memory, cpu, OpAbs(cpu, SPAWN_ANGLE));
    return Lufia2BattleEffectSpawnMovingActor(memory, cpu);
}

static Lufia2ExecutionResult SpawnScriptCommand(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint32_t entry, uint8_t centered) {
    if (!ScriptCommandContext(cpu))
        return ExecutionHandoff(cpu, entry);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpRepWidths(cpu, 0x20u);
    ReadSpawnWord(memory, cpu);
    OpSta(memory, cpu, OpAbs(cpu, SPAWN_SCRIPT));
    if (centered) {
        OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_SPAWN_STREAM));
        AdvanceSpawnStream(memory, cpu);
        OpSepWidths(cpu, 0x20u);
        OpSta(memory, cpu, OpAbs(cpu, SPAWN_POSITION + 1u));
        LoadA8(cpu, 0x80u);
        OpSta(memory, cpu, OpAbs(cpu, SPAWN_POSITION));
    } else {
        OpStz(memory, cpu, OpAbs(cpu, SPAWN_POSITION));
        OpSepWidths(cpu, 0x20u);
    }
    return Lufia2BattleEffectSpawnScript(memory, cpu);
}

Lufia2ExecutionResult Lufia2BattleEffectSpawnZeroPositionScript(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return SpawnScriptCommand(memory, cpu, 0x81a65fu, 0u);
}

Lufia2ExecutionResult Lufia2BattleEffectSpawnCenteredScript(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return SpawnScriptCommand(memory, cpu, 0x81a676u, 1u);
}
