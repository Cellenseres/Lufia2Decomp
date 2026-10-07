#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/battle.h"
#include "system/wram.h"

enum {
    SCRIPT_BASE = (WRAM_BATTLE_MESSAGE_NAME_POINTER & 0xffffu),
    SCRIPT_BANK = (WRAM_BATTLE_MESSAGE_NAME_BANK & 0xffffu),
    SCRIPT_POINTER = 0xbbu,
    SCRIPT_POINTER_BANK = 0xbdu
};

Lufia2ExecutionResult Lufia2BattleRunRelativeScript(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x81u || cpu->decimal || !child)
        return ExecutionHandoff(cpu, 0x81fac9u);
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    OpRepWidths(cpu, 0x30u);
    PushAccumulator16(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    Compare16(cpu, cpu->y, 0u);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, SCRIPT_BANK));
        OpSta(memory, cpu, OpDp(cpu, SCRIPT_POINTER_BANK));
        OpRepWidths(cpu, 0x20u);
        OpTya(cpu);
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpAbs(cpu, SCRIPT_BASE));
        OpSta(memory, cpu, OpDp(cpu, SCRIPT_POINTER));
        OpSepWidths(cpu, 0x20u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x81fae9u, 0x85b452u, 3u, 0x81u)) {
            Lufia2ExecutionResult result = ExecutionReturned(0x81fae9u);
            result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
            return result;
        }
    }
    OpRepWidths(cpu, 0x30u);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    PullAccumulator16(memory, cpu);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x81faf4u);
}
