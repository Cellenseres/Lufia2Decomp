/* Battle entry and return wrapper at $81:8821. */

#include "core/cpu_ops.h"
#include "lufia2/battle.h"

static Lufia2ExecutionResult BattleEntryChildUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);

    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static uint8_t BattleEntryCall(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context,
    uint32_t site,
    uint32_t target,
    uint8_t frame_size) {
    if (frame_size == 2u)
        SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    else
        SimulateJslFrame(memory, cpu, 0x81u, (uint16_t)(site + 3u));
    return child(child_context, cpu, target, site, frame_size);
}

Lufia2ExecutionResult Lufia2BattleEntry(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context) {
    PushDataBank(memory, cpu);                                 /* 8821 */
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    PushAccumulator16(memory, cpu);
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpDp(cpu, 0x40u));                    /* 882A */
    OpStz(memory, cpu, OpAbs(cpu, 0x11d8u));
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0x11d8u);
    OpLdy(cpu, 0x11d9u);
    LoadA16(cpu, 0x0a33u);
    OpMoveNext(memory, cpu, 0x00u, 0x00u);                  /* 883A */
    OpSepWidths(cpu, 0x20u);
    LoadA8(cpu, 0x97u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);                               /* DB = $97 */
    OpStz(memory, cpu, OpAbs(cpu, 0x11a5u));
    OpLdx(cpu, cpu->stack);                                  /* TSX */
    OpWriteX(memory, cpu, OpAbs(cpu, 0x1395u), cpu->x);
    LoadA8(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, 0x0a13u));
    if (!BattleEntryCall(memory, cpu, child, child_context,
                         0x81884fu, 0x818000u, 2u))
        return BattleEntryChildUnwound(0x81884fu);
    if (!BattleEntryCall(memory, cpu, child, child_context,
                         0x818852u, 0x81886fu, 2u))
        return BattleEntryChildUnwound(0x818852u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, 0x1395u)));
    cpu->stack = cpu->x;                                     /* 8858 TXS */
    if (!BattleEntryCall(memory, cpu, child, child_context,
                         0x818859u, 0x85edbbu, 3u))
        return BattleEntryChildUnwound(0x818859u);
    if (!BattleEntryCall(memory, cpu, child, child_context,
                         0x81885du, 0x85eea1u, 3u))
        return BattleEntryChildUnwound(0x81885du);
    if (!BattleEntryCall(memory, cpu, child, child_context,
                         0x818861u, 0x81876bu, 2u))
        return BattleEntryChildUnwound(0x818861u);
    OpStz(memory, cpu, OpAbs(cpu, 0x0a13u));                /* 8864 */
    OpRepWidths(cpu, 0x30u);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81886eu);                     /* RTL */
}
