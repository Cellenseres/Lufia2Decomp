#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "field/event_script_internal.h"
#include "lufia2/field.h"

enum {
    POSITION_OPERAND = 0x54u,
    POSITION_VALUE = 0x55u,
    POSITION_SLOT = 0xa7u,
    POSITION_OWN_SLOT = 0xfbu,
    POSITION_POINT_FIRST = 0xe0u,
    POSITION_ACTOR_FIRST = 0x20u,
    POSITION_ACTOR_HEADER = 0x22u,
    POSITION_ACTOR_STRIDE = 3u
};

static uint8_t EventCoordinateReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x80u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal;
}

static uint8_t CallCoordinateChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target, uint8_t frame) {
    return CallChildWithFrame(memory, cpu, child, context, site, target, frame, 0x80u);
}

static Lufia2ExecutionResult CoordinateChildUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2FieldLookupEventActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!EventCoordinateReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x80e912u);
    ExchangeAccumulatorBytes(cpu);
    OpLoadA(cpu, POSITION_ACTOR_STRIDE);
    ExchangeAccumulatorBytes(cpu);
    OpLdx(cpu, POSITION_ACTOR_HEADER);
    if (!CallCoordinateChild(memory, cpu, child, context, 0x80e919u, 0x80bfaau, 3u))
        return CoordinateChildUnwound(0x80e919u);
    if (cpu->carry) {
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, EVENT_LIST_RECORD + POSITION_ACTOR_HEADER);
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
        cpu->carry = 0u;
    }
    return ExecutionReturned(0x80e929u);
}

Lufia2ExecutionResult Lufia2FieldResolveEventPosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!EventCoordinateReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x80ea09u);
    OpSta(memory, cpu, OpDp(cpu, POSITION_OPERAND));
    if (!CallCoordinateChild(memory, cpu, child, context, 0x80ea0bu, 0x80e9bcu, 2u))
        return CoordinateChildUnwound(0x80ea0bu);
    OpSta(memory, cpu, OpDp(cpu, POSITION_VALUE));
    OpLda(memory, cpu, OpDp(cpu, POSITION_OPERAND));
    OpCmpValue(cpu, POSITION_OWN_SLOT);
    if (cpu->zero) {
        OpLdx(cpu, OpRead16(memory, OpDp(cpu, POSITION_SLOT)));
        OpLda(memory, cpu, OpLongX(cpu, EVENT_SLOT_Y));
        ExchangeAccumulatorBytes(cpu);
        OpLda(memory, cpu, OpLongX(cpu, EVENT_SLOT_X));
        return ExecutionReturned(0x80ea21u);
    }
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpDp(cpu, POSITION_VALUE));
    OpCmpValue(cpu, POSITION_POINT_FIRST);
    cpu->carry = 1u;
    if (A8(cpu) >= POSITION_POINT_FIRST) {
        OpSbcValue(cpu, POSITION_POINT_FIRST);
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, EVENT_POINT_Y));
        ExchangeAccumulatorBytes(cpu);
        OpLda(memory, cpu, OpLongX(cpu, EVENT_POINT_X));
        return ExecutionReturned(0x80ea36u);
    }
    OpSbcValue(cpu, POSITION_ACTOR_FIRST);
    if (!CallCoordinateChild(memory, cpu, child, context, 0x80ea3au, 0x80e912u, 2u))
        return CoordinateChildUnwound(0x80ea3au);
    OpLda(memory, cpu, OpLongX(cpu, EVENT_LIST_RECORD + 2u));
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpLongX(cpu, EVENT_LIST_RECORD + 1u));
    return ExecutionReturned(0x80ea46u);
}

Lufia2ExecutionResult Lufia2FieldReadEventVariableOperands(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!EventCoordinateReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x80d9f0u);
    TransferDirectToA(cpu);
    if (!CallCoordinateChild(memory, cpu, child, context, 0x80d9f1u, 0x80e8b9u, 2u))
        return CoordinateChildUnwound(0x80d9f1u);
    if (!CallCoordinateChild(memory, cpu, child, context, 0x80d9f4u, 0x80e9bcu, 2u))
        return CoordinateChildUnwound(0x80d9f4u);
    OpTax(cpu);
    if (!CallCoordinateChild(memory, cpu, child, context, 0x80d9f8u, 0x80e8b9u, 2u))
        return CoordinateChildUnwound(0x80d9f8u);
    return ExecutionReturned(0x80d9fbu);
}
