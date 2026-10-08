#include "core/cpu_ops.h"
#include "system/wram.h"
#include "system/wram.h"
#include "lufia2/battle.h"
#include "lufia2/battle.h"

enum {
    DP_CONDITIONAL_STREAM = 0xc3u,
    CONDITIONAL_SCRIPT_ENABLED = WRAM_BATTLE_EFFECT_SCRIPT_ENABLED
};

Lufia2ExecutionResult Lufia2BattleEffectSpawnScriptWhenEnabled(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f02u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x819976u);
    OpLda(memory, cpu, CONDITIONAL_SCRIPT_ENABLED);
    if (cpu->zero) {
        OpStepMem(memory, cpu, OpDp(cpu, DP_CONDITIONAL_STREAM), 1);
        OpStepMem(memory, cpu, OpDp(cpu, DP_CONDITIONAL_STREAM), 1);
        return ExecutionReturned(0x819997u);
    }
    OpSetDataBank(memory, cpu, 0x7eu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_CONDITIONAL_STREAM));
    OpStepMem(memory, cpu, OpDp(cpu, DP_CONDITIONAL_STREAM), 1);
    OpStepMem(memory, cpu, OpDp(cpu, DP_CONDITIONAL_STREAM), 1);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_EFFECT_SPAWN_STREAM));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_BATTLE_EFFECT_SPAWN_POSITION));
    OpSepWidths(cpu, 0x20u);
    return Lufia2BattleEffectSpawnScript(memory, cpu);
}
