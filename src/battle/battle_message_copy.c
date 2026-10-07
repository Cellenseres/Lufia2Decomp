#include "core/cpu_ops.h"
#include "lufia2/battle.h"
#include "system/wram.h"

enum {
    MESSAGE_NAME_BUFFER = WRAM_TEXT_WAIT_ACTOR,
    MESSAGE_NAME_BYTES = 37u
};

Lufia2ExecutionResult Lufia2BattleCopyMessageName(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x85u || cpu->decimal)
        return ExecutionHandoff(cpu, 0x8594e7u);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    PushAccumulator16(memory, cpu);
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    Push8(memory, cpu, 0x85u);
    PullDataBank(memory, cpu);
    OpLdy(cpu, MESSAGE_NAME_BUFFER);
    OpWrite16(memory, OpAbs(cpu, SNES_WMADDL), cpu->y);
    OpStz(memory, cpu, OpAbs(cpu, SNES_WMADDH));
    OpLdy(cpu, MESSAGE_NAME_BYTES);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        OpSta(memory, cpu, OpAbs(cpu, SNES_WMDATA));
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    OpRepWidths(cpu, 0x30u);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x85950fu);
}
