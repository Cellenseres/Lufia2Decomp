#include "battle/battle_internal.h"
#include "core/snes_registers.h"
#include "system/dp_scratch.h"

/* $81:E73B: prepare the message tilemap and render the encoded byte string. */
Lufia2ExecutionResult Lufia2BattleRenderMessage(const Lufia2Memory *memory,
                                                Lufia2CpuState *cpu,
                                                Lufia2PushedChildCall child,
                                                void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);
    OpLdx(cpu, 0x3800u);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_WMADDL), cpu->x);
    OpStz(memory, cpu, OpAbs(cpu, SNES_WMADDH));
    OpLdx(cpu, 0x3cu);
    OpLoadA(cpu, 0xffu);
    do {
        OpLdy(cpu, 8u);
        do {
            OpStz(memory, cpu, OpAbs(cpu, SNES_WMDATA));
            OpStz(memory, cpu, OpAbs(cpu, SNES_WMDATA));
            OpDey(cpu);
        } while (!cpu->zero);
        OpLdy(cpu, 8u);
        do {
            OpStz(memory, cpu, OpAbs(cpu, SNES_WMDATA));
            OpSta(memory, cpu, OpAbs(cpu, SNES_WMDATA));
            OpDey(cpu);
        } while (!cpu->zero);
        OpDex(cpu);
    } while (!cpu->zero);
    if (!BattleCall(&battle, 0xe764u, 0x859a7du, 3u))
        return BattleChildUnwound(&battle);
    OpLoadA(cpu, 0x1fu);
    /* Original SBC consumes the carry returned by the measuring child. */
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0x22u)));
    OpSta(memory, cpu, OpDp(cpu, 0x23u));
    OpLdx(cpu, 0x1269u);
    for (;;) {
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        if (cpu->zero)
            break;
        OpInx(cpu);
        OpCmpValue(cpu, 0x10u);
        if (!cpu->carry) {
            OpSbcValue(cpu, 4u);
            OpIncA(cpu);
            OpSta(memory, cpu, OpDp(cpu, 0x25u));
            OpLda(memory, cpu, OpAbsX(cpu, 0u));
            OpInx(cpu);
        } else {
            OpStz(memory, cpu, OpDp(cpu, 0x25u));
        }
        OpSta(memory, cpu, OpDp(cpu, 0x24u));
        OpPushX(memory, cpu);
        if (!BattleCall(&battle, 0xe78bu, 0x81ea35u, 2u))
            return BattleChildUnwound(&battle);
        OpPullX(memory, cpu);
    }
    return ExecutionReturned(0x81e791u);
}

/* $81:E792: load graphics resource $023F into the message buffer. */
Lufia2ExecutionResult Lufia2BattleLoadMessageGraphics(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu,
                                                      Lufia2PushedChildCall child,
                                                      void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);
    OpLdx(cpu, 0x3000u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x60u), cpu->x);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, 0x62u));
    OpLdx(cpu, 0x23fu);
    OpWriteX(memory, cpu, OpDp(cpu, DP_SCRATCH_A), cpu->x);
    if (!BattleCall(&battle, 0xe7a0u, BATTLE_ROUTINE_DECOMPRESS_RESOURCE, 3u))
        return BattleChildUnwound(&battle);
    return ExecutionReturned(0x81e7a4u);
}
