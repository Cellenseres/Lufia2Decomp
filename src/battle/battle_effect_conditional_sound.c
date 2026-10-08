#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/battle.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum {
    DP_EFFECT_STREAM = 0xc3u,
    FIRST_SOUND_SCRIPT_SLOT = 0x4fe7u
};

static Lufia2ExecutionResult FindSoundScript(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpSetDataBank(memory, cpu, 0x7eu);
    SimulateJsrFrame(memory, cpu, 0x9f91u);
    const Lufia2ExecutionResult result =
        Lufia2BattleEffectFindScriptSlot(memory, cpu);
    if (result.flow == LUFIA2_EXECUTION_RETURNED)
        SimulateRtsFrame(memory, cpu);
    return result;
}

Lufia2ExecutionResult Lufia2BattleEffectSendConditionalSound(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f02u || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x819f8bu);
    const Lufia2ExecutionResult result = FindSoundScript(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpCpx(cpu, FIRST_SOUND_SCRIPT_SLOT);
    if (cpu->carry) {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_EFFECT_SPECIAL_PARTY));
        if (cpu->zero) {
            OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_EFFECT_STREAM));
            PushY(memory, cpu);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x819f9fu, 0x80953bu, 3u, 0x81u)) {
                const Lufia2ExecutionResult unwind = {
                    LUFIA2_EXECUTION_CHILD_UNWOUND, 0x819f9fu
                };
                return unwind;
            }
            cpu->y = PullIndexValue(memory, cpu);
        }
    }
    OpRepWidths(cpu, 0x20u);
    OpStepMem(memory, cpu, OpDp(cpu, DP_EFFECT_STREAM), 1);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x819faau);
}
