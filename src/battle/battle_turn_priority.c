#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/battle.h"

enum {
    TURN_SPREAD = 0x54u,
    TURN_PRIORITY = 0x56u,
    PRODUCT_SOURCE = 0x4eu,
    PRODUCT_FACTOR = 0x50u,
    PRODUCT_HIGH = 0x52u,
    RANDOM_RANGE = 0x58u,
    RANDOM_SUM = 0x5au
};

static Lufia2ExecutionResult PriorityUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2BattleRandomizeTurnPriority(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
            cpu->index_is_8_bit || cpu->decimal || !child)
        return ExecutionHandoff(cpu, 0x85dd19u);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, 0x85u);
    PullDataBank(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, TURN_SPREAD));
    OpSta(memory, cpu, OpDp(cpu, PRODUCT_FACTOR));
    OpLdx(cpu, OpRead16(memory, OpDp(cpu, TURN_PRIORITY)));
    OpWrite16(memory, OpDp(cpu, PRODUCT_SOURCE), cpu->x);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x85dd24u, 0x80834cu, 3u, 0x85u))
        return PriorityUnwound(0x85dd24u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, PRODUCT_HIGH));
    OpIncA(cpu);
    OpCmpValue(cpu, 2u);
    OpAdcValue(cpu, 0u);
    OpSta(memory, cpu, OpDp(cpu, RANDOM_RANGE));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x85dd35u, 0x85dceau, 3u, 0x85u))
        return PriorityUnwound(0x85dd35u);
    OpSta(memory, cpu, OpDp(cpu, RANDOM_SUM));
    OpLda(memory, cpu, OpDp(cpu, RANDOM_RANGE));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x85dd3du, 0x85dceau, 3u, 0x85u))
        return PriorityUnwound(0x85dd3du);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, RANDOM_SUM));
    OpSta(memory, cpu, OpDp(cpu, RANDOM_SUM));
    OpLda(memory, cpu, OpDp(cpu, PRODUCT_SOURCE));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, RANDOM_RANGE)));
    const uint8_t below_zero = !cpu->carry;
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, RANDOM_SUM));
    if (below_zero && !cpu->carry)
        OpLoadA(cpu, cpu->direct_page);
    else if (!below_zero && cpu->carry)
        OpLoadA(cpu, 0xffffu);
    OpSta(memory, cpu, OpDp(cpu, TURN_PRIORITY));
    OpSepWidths(cpu, 0x20u);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x85dd62u);
}
