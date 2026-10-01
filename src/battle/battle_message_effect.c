#include "battle/battle_internal.h"

/* $85:AADC: install the message display record and its original DMA bank. */
Lufia2ExecutionResult Lufia2BattleStartMessageEffect(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu) {
    const uint16_t positions[] = {0x1258u, 0x125au, 0x125cu};
    for (unsigned i = 0; i < 3u; ++i) {
        OpLdx(cpu, 0xdfu);
        OpWriteX(memory, cpu, OpAbs(cpu, positions[i]), cpu->x);
    }
    OpLda(memory, cpu, 0x85a0deu);
    OpSta(memory, cpu, OpAbs(cpu, 0x1255u));
    OpLoadA(cpu, 0x42u);
    OpSta(memory, cpu, OpAbs(cpu, 0x1b03u));
    OpLoadA(cpu, 0x10u);
    OpSta(memory, cpu, OpAbs(cpu, 0x1b04u));
    OpLdx(cpu, 0xa0ceu);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x1b05u), cpu->x);
    OpLoadA(cpu, 0x85u);
    OpSta(memory, cpu, OpAbs(cpu, 0x1b07u));
    OpLoadA(cpu, 0x85u);
    OpSta(memory, cpu, 0x004347u);
    OpLoadA(cpu, 0x10u);
    OpTestBits(memory, cpu, OpDp(cpu, 0xdau), 1u);
    OpLoadA(cpu, 0x0fu);
    OpSta(memory, cpu, OpAbs(cpu, 0x1b29u));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, 0x1b28u));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, 0x1b27u));
    OpLoadA(cpu, 4u);
    OpTestBits(memory, cpu, OpDp(cpu, 0xdbu), 1u);
    return ExecutionReturned(0x85ab27u);
}

/* $85:AB28: zero keeps the message; a decrement to zero clears it. */
Lufia2ExecutionResult Lufia2BattleTickMessageEffect(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_WAIT_COUNTER));
    if (!cpu->zero) {
        OpStepMem(memory, cpu, OpAbs(cpu, WRAM_BATTLE_WAIT_COUNTER), -1);
        if (cpu->zero) {
            const uint16_t positions[] = {0x1258u, 0x125au, 0x125cu};
            for (unsigned i = 0; i < 3u; ++i) {
                OpLoadA(cpu, 0xffffu);
                OpSta(memory, cpu, OpAbs(cpu, positions[i]));
            }
            SetAccumulatorWidth(cpu, 1);
            OpLoadA(cpu, 4u);
            OpTestBits(memory, cpu, OpDp(cpu, 0xdbu), 0u);
            OpStz(memory, cpu, OpAbs(cpu, 0x1268u));
            OpStz(memory, cpu, OpAbs(cpu, 0x1b27u));
            return ExecutionReturned(0x85ab52u);
        }
    }
    SetAccumulatorWidth(cpu, 1);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, 0x1b28u));
    return ExecutionReturned(0x85ab5au);
}

/* $85:AB5B: install the cleanup record and update the original request bits. */
Lufia2ExecutionResult Lufia2BattleQueueMessageCleanup(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu) {
    TransferDirectToA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, 0x1afeu));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, 0x1affu));
    OpLdx(cpu, 0xa081u);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x1b00u), cpu->x);
    OpLoadA(cpu, 0x85u);
    OpSta(memory, cpu, OpAbs(cpu, 0x1b02u));
    OpLoadA(cpu, 8u);
    OpTestBits(memory, cpu, OpDp(cpu, 0xdau), 1u);
    OpLoadA(cpu, 4u);
    OpTestBits(memory, cpu, OpDp(cpu, 0xdbu), 0u);
    return ExecutionReturned(0x85ab77u);
}
