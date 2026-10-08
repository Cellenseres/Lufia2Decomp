#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/system.h"

enum {
    DP_VECTOR_ANGLE = 0x54u,
    DP_VECTOR_HORIZONTAL = 0x56u,
    DP_VECTOR_VERTICAL = 0x58u,
    DP_VECTOR_VERTICAL_MAGNITUDE = 0x5au,
    DP_VECTOR_QUOTIENT = 0x63u,
    DP_VECTOR_DIVIDEND_HIGH = 0x65u,
    ROM_VECTOR_ANGLE_TABLE = 0x97b126u,
    VECTOR_QUARTER_TURN = 0x40u,
    VECTOR_HALF_TURN = 0x80u,
    VECTOR_RATIO_LIMIT = 0x100u
};

static void NegateVectorComponent(Lufia2CpuState *cpu) {
    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffffu));
    OpIncA(cpu);
}

static void RotateVectorAxes(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpCmp(memory, cpu, OpDp(cpu, DP_VECTOR_VERTICAL_MAGNITUDE));
    if (cpu->carry) {
        OpLoadA(cpu, VECTOR_QUARTER_TURN);
        OpSta(memory, cpu, OpDp(cpu, DP_VECTOR_ANGLE));
        OpLda(memory, cpu, OpDp(cpu, DP_VECTOR_VERTICAL));
        NegateVectorComponent(cpu);
        OpLdx(cpu, OpRead16(memory, OpDp(cpu, DP_VECTOR_HORIZONTAL)));
        OpSta(memory, cpu, OpDp(cpu, DP_VECTOR_HORIZONTAL));
        OpWriteX(memory, cpu, OpDp(cpu, DP_VECTOR_VERTICAL), cpu->x);
    }
    OpLda(memory, cpu, OpDp(cpu, DP_VECTOR_VERTICAL));
    if (cpu->negative) {
        NegateVectorComponent(cpu);
        OpSta(memory, cpu, OpDp(cpu, DP_VECTOR_VERTICAL));
        OpLda(memory, cpu, OpDp(cpu, DP_VECTOR_HORIZONTAL));
        NegateVectorComponent(cpu);
        OpSta(memory, cpu, OpDp(cpu, DP_VECTOR_HORIZONTAL));
        OpLoadA(cpu, VECTOR_HALF_TURN);
        OpTestBits(memory, cpu, OpDp(cpu, DP_VECTOR_ANGLE), 1u);
    }
}

static void ApplyVectorAngle(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t subtract) {
    OpLda(memory, cpu, OpDp(cpu, DP_VECTOR_ANGLE));
    OpLdx(cpu, OpRead16(memory, OpDp(cpu, DP_VECTOR_QUOTIENT)));
    OpCpx(cpu, VECTOR_RATIO_LIMIT);
    if (cpu->carry)
        OpLdx(cpu, VECTOR_RATIO_LIMIT);
    cpu->carry = subtract;
    const uint16_t angle = OpRead16(memory, OpLongX(cpu, ROM_VECTOR_ANGLE_TABLE));
    if (subtract)
        OpSbcValue(cpu, angle);
    else
        OpAdcValue(cpu, angle);
    OpSta(memory, cpu, OpDp(cpu, DP_VECTOR_ANGLE));
}

Lufia2ExecutionResult Lufia2SystemCalculateVectorAngle(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || cpu->index_is_8_bit || cpu->decimal ||
        cpu->direct_page != 0u || cpu->stack < 0x1f00u ||
        cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x85dac7u);
    Push8(memory, cpu, cpu->data_bank);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpDp(cpu, DP_VECTOR_ANGLE));
    OpLda(memory, cpu, OpDp(cpu, DP_VECTOR_VERTICAL));
    if (cpu->zero) {
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, VECTOR_QUARTER_TURN);
        OpLdx(cpu, OpRead16(memory, OpDp(cpu, DP_VECTOR_HORIZONTAL)));
        if (cpu->negative)
            OpLoadA(cpu, VECTOR_QUARTER_TURN + VECTOR_HALF_TURN);
        OpSta(memory, cpu, OpDp(cpu, DP_VECTOR_ANGLE));
        PullDataBank(memory, cpu);
        return ExecutionReturned(0x85dadfu);
    }
    if (cpu->negative) {
        NegateVectorComponent(cpu);
        OpSta(memory, cpu, OpDp(cpu, DP_VECTOR_VERTICAL_MAGNITUDE));
    }
    OpLda(memory, cpu, OpDp(cpu, DP_VECTOR_HORIZONTAL));
    if (cpu->zero) {
        OpSepWidths(cpu, 0x20u);
        LoadA16(cpu, cpu->direct_page);
        OpLdx(cpu, OpRead16(memory, OpDp(cpu, DP_VECTOR_VERTICAL)));
        if (cpu->negative)
            OpLoadA(cpu, VECTOR_HALF_TURN);
        OpSta(memory, cpu, OpDp(cpu, DP_VECTOR_ANGLE));
        PullDataBank(memory, cpu);
        return ExecutionReturned(0x85daf8u);
    }
    if (cpu->negative)
        NegateVectorComponent(cpu);
    RotateVectorAxes(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, DP_VECTOR_HORIZONTAL));
    const uint8_t subtract = cpu->negative;
    if (subtract)
        NegateVectorComponent(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_VECTOR_DIVIDEND_HIGH));
    OpStz(memory, cpu, OpDp(cpu, DP_VECTOR_QUOTIENT));
    const uint32_t site = subtract ? 0x85db54u : 0x85db33u;
    if (!CallChildWithFrame(memory, cpu, child, context,
            site, 0x85db6du, 3u, 0x85u)) {
        Lufia2ExecutionResult result = ExecutionReturned(site);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    ApplyVectorAngle(memory, cpu, subtract);
    PullDataBank(memory, cpu);
    return ExecutionReturned(subtract ? 0x85db6cu : 0x85db4bu);
}
