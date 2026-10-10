#include "lufia2/battle.h"
#include "core/cpu_ops.h"
#include "system/wram.h"

enum { MESSAGE_TABLE_BASE = 0xdf00u, MESSAGE_TABLE_COUNT = 78u };

static void CopyIndexedMessage(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpAndValue(cpu, 0xffu);
    OpAslA(cpu);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpSetDataBank(memory, cpu, 0xa5u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, MESSAGE_TABLE_BASE));
    OpTax(cpu);
    OpLdy(cpu, 0u);
    OpSepWidths(cpu, 0x20u);
    for (;;) {
        OpLda(memory, cpu, OpAbsX(cpu, MESSAGE_TABLE_BASE));
        OpSta(memory, cpu, OpAbsY(cpu, WRAM_TEXT_WAIT_ACTOR));
        if (cpu->zero)
            break;
        OpInx(cpu);
        OpIny(cpu);
    }
    OpLoadA(cpu, 1u);
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_MESSAGE_COUNT));
}

Lufia2ExecutionResult Lufia2BattleLoadIndexedMessage(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (A8(cpu) >= MESSAGE_TABLE_COUNT)
        return ExecutionHandoff(cpu, 0x8595c6u);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    PushAccumulator16(memory, cpu);
    OpPushX(memory, cpu);
    Push8(memory, cpu, (uint8_t)(cpu->y >> 8));
    Push8(memory, cpu, (uint8_t)cpu->y);
    CopyIndexedMessage(memory, cpu);
    OpRepWidths(cpu, 0x30u);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x8595fdu);
}

