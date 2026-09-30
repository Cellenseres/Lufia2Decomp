#include "battle/battle_internal.h"

static void WaitForBattleFrame(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, 0x40u));
    do {
        OpCmp(memory, cpu, OpDp(cpu, 0x40u));
    } while (cpu->zero);
}

/* $85:EC81: status/sprite upkeep, original frame waits and pause input. */
Lufia2ExecutionResult Lufia2BattleFrameInput(const Lufia2Memory *memory,
                                             Lufia2CpuState *cpu,
                                             Lufia2PushedChildCall child,
                                             void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x85u);
    if (!BattleCall(&battle, 0xec81u, 0x85919cu, 3u))
        return BattleChildUnwound(&battle);
    OpLda(memory, cpu, 0x0012f3u);
    if (!cpu->zero) {
        if (!BattleCall(&battle, 0xec8bu, 0x81b5c4u, 3u))
            return BattleChildUnwound(&battle);
        TransferDirectToA(cpu);
        OpSta(memory, cpu, 0x0012f3u);
    }
    WaitForBattleFrame(memory, cpu);
    OpLda(memory, cpu, OpAbs(cpu, 0x057cu));
    if (!cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, 0x47u));
        OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0x4bu)));
        OpAndValue(cpu, 0x10u);
        if (!cpu->zero) {
            OpTestBits(memory, cpu, OpDp(cpu, 0x4bu), 0u);
            OpLda(memory, cpu, OpDp(cpu, 0xd9u));
            PushAccumulator8(memory, cpu);
            OpLda(memory, cpu, OpAbs(cpu, 0x129au));
            PushAccumulator8(memory, cpu);
            OpLoadA(cpu, 1u);
            OpTestBits(memory, cpu, OpDp(cpu, 0xd9u), 0u);
            OpStz(memory, cpu, OpAbs(cpu, 0x129au));
            do {
                WaitForBattleFrame(memory, cpu);
                OpLda(memory, cpu, OpDp(cpu, 0x47u));
                OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0x4bu)));
                OpAndValue(cpu, 0x10u);
            } while (cpu->zero);
            OpTestBits(memory, cpu, OpDp(cpu, 0x4bu), 0u);
            LoadA8(cpu, Pull8(memory, cpu));
            OpSta(memory, cpu, OpAbs(cpu, 0x129au));
            LoadA8(cpu, Pull8(memory, cpu));
            OpSta(memory, cpu, OpDp(cpu, 0xd9u));
        }
    }
    SetAccumulatorWidth(cpu, 0);
    OpLda(memory, cpu, OpDp(cpu, 0x46u));
    OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0x4au)));
    /* Original TRB $4A word writes high before low and only changes Z. */
    const uint32_t buttons = OpDp(cpu, 0x4au);
    const uint16_t old = OpRead16(memory, buttons);
    const uint16_t value = (uint16_t)(old & (uint16_t)~cpu->accumulator);
    cpu->zero = (old & cpu->accumulator) == 0u;
    Write8(memory, OpNextByte(buttons), (uint8_t)(value >> 8));
    Write8(memory, buttons, (uint8_t)value);
    OpSta(memory, cpu, OpDp(cpu, 0xddu));
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x85ecdau);
}
