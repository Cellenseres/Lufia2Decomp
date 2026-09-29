/* Battle frame loop at $81:886F; attack/menu children retain ROM semantics. */

#include "core/cpu_ops.h"
#include "lufia2/battle.h"

static Lufia2ExecutionResult LoopChildUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static uint8_t LoopCall(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                        Lufia2PushedChildCall child, void *context,
                        uint32_t site, uint32_t target, uint8_t frame_size) {
    if (frame_size == 2u)
        SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    else
        SimulateJslFrame(memory, cpu, 0x81u, (uint16_t)(site + 3u));
    return child(context, cpu, target, site, frame_size);
}

#define LOOP_CALL(site, target, frame) \
    do { \
        if (!LoopCall(memory, cpu, child, child_context, \
                      0x810000u | (site), (target), (frame))) \
            return LoopChildUnwound(0x810000u | (site)); \
    } while (0)

Lufia2ExecutionResult Lufia2BattleMainLoop(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    OpLdx(cpu, cpu->stack);                                 /* 886F TSX */
    OpWriteX(memory, cpu, OpAbs(cpu, 0x1397u), cpu->x);
    LOOP_CALL(0x8873u, 0x8593b7u, 3u);
    for (;;) {
        LOOP_CALL(0x8877u, 0x85ecf0u, 3u);                /* frame upkeep */
        OpLda(memory, cpu, OpAbs(cpu, 0x11e7u));
        OpBitValue(cpu, 0x80u);
        if (!cpu->zero) {
            PushAccumulator8(memory, cpu);
            LoadA8(cpu, 3u);
            OpTestBits(memory, cpu, OpAbs(cpu, 0x11e7u), 0u);
            LoadA8(cpu, Pull8(memory, cpu));
        }
        OpBitValue(cpu, 0x40u);
        OpLda(memory, cpu, OpAbs(cpu, 0x11e7u));
        OpBitValue(cpu, 1u);
        if (!cpu->zero) {
            LOOP_CALL(0x8894u, 0x859236u, 3u);
            if (!cpu->carry) {
                LOOP_CALL(0x889au, 0x8596a2u, 3u);
                LOOP_CALL(0x889eu, 0x81c739u, 2u);
                LOOP_CALL(0x88a1u, 0x8596b0u, 3u);
            }
            LOOP_CALL(0x88a5u, 0x859275u, 3u);
            LOOP_CALL(0x88a9u, 0x81c294u, 2u);
        }
        OpLda(memory, cpu, OpAbs(cpu, 0x11e7u));
        OpBitValue(cpu, 2u);
        if (!cpu->zero)
            LOOP_CALL(0x88b3u, 0x81c254u, 2u);
        OpStz(memory, cpu, OpAbs(cpu, 0x129au));
        LOOP_CALL(0x88b9u, 0x85ab78u, 3u);
        LOOP_CALL(0x88bdu, 0x8589e5u, 3u);
        LOOP_CALL(0x88c1u, 0x81890au, 2u);
        LoadA8(cpu, 0xffu);
        OpSta(memory, cpu, OpAbs(cpu, 0x129au));
        OpLda(memory, cpu, OpAbs(cpu, 0x11e7u));
        OpBitValue(cpu, 0x80u);
        if (!cpu->zero) break;
        LOOP_CALL(0x88d0u, 0x81c240u, 2u);
    }

    OpLdx(cpu, 0x2800u);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x2181u), cpu->x);
    OpStz(memory, cpu, OpAbs(cpu, 0x2183u));
    OpLdx(cpu, 0x0280u);
    LoadA8(cpu, 0x21u);
    do {
        OpStz(memory, cpu, OpAbs(cpu, 0x2180u));
        OpSta(memory, cpu, OpAbs(cpu, 0x2180u));
        OpDex(cpu);
    } while (!cpu->zero);
    OpLda(memory, cpu, 0x7ff8a2u);
    OpCmpValue(cpu, 0u);
    if (cpu->zero) {
        LOOP_CALL(0x88fau, 0x85e7bcu, 3u);
        LoadA8(cpu, 29u);
        LOOP_CALL(0x8900u, 0x8093feu, 3u);
        LOOP_CALL(0x8904u, 0x81d9e1u, 2u);
    }
    return ExecutionReturned(0x818909u);
}
