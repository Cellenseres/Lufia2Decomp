#include "core/cpu_ops.h"
#include "field/event_script_internal.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    OBJECT_CONDITION_TABLE = 0x91b8b5u
};

static bool ObjectConditionContext(const Lufia2CpuState *cpu, uint8_t bank) {
    return cpu->program_bank == bank && cpu->accumulator_is_8_bit && !cpu->index_is_8_bit &&
        !cpu->direct_page && !cpu->decimal &&
        cpu->stack >= 0x1f20u && cpu->stack <= 0x1ffcu;
}

static Lufia2ExecutionResult ObjectConditionHeader(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t back) {
    SimulateJslFrame(memory, cpu, 0x83u, back);
    cpu->program_bank = 0x80u;
    Lufia2ExecutionResult result = Lufia2FieldFindHeaderRecord(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    uint16_t address = Pull8(memory, cpu);
    address |= (uint16_t)Pull8(memory, cpu) << 8;
    cpu->program_bank = Pull8(memory, cpu);
    if (address != back || cpu->program_bank != 0x83u)
        return ExecutionHandoff(cpu,
            ((uint32_t)cpu->program_bank << 16) | (uint16_t)(address + 1u));
    return result;
}

Lufia2ExecutionResult Lufia2FieldLoadObjectActionHeader(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ObjectConditionContext(cpu, 0x83u))
        return ExecutionHandoff(cpu, 0x8387a3u);
    OpLoadA(cpu, 4u);
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, WRAM_FIELD_SELECTED_OBJECT_RECORD_ID);
    OpAndValue(cpu, 0x7fu);
    OpLdx(cpu, 0x26u);
    Lufia2ExecutionResult result = ObjectConditionHeader(memory, cpu, 0x87b2u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpLda(memory, cpu, OpLongX(cpu, 0x7ef001u));
    OpSta(memory, cpu, WRAM_FIELD_PENDING_OBJECT_X);
    OpLda(memory, cpu, OpLongX(cpu, 0x7ef002u));
    OpSta(memory, cpu, WRAM_FIELD_PENDING_OBJECT_Y);
    OpLda(memory, cpu, OpLongX(cpu, 0x7ef003u));
    OpSta(memory, cpu, WRAM_FIELD_REGION_RECORD_ID);
    return ExecutionReturned(0x8387cbu);
}

Lufia2ExecutionResult Lufia2FieldLoadObjectControlHeader(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ObjectConditionContext(cpu, 0x83u))
        return ExecutionHandoff(cpu, 0x838848u);
    ExchangeAccumulatorBytes(cpu);
    OpLoadA(cpu, 15u);
    ExchangeAccumulatorBytes(cpu);
    OpLdx(cpu, 2u);
    Lufia2ExecutionResult result = ObjectConditionHeader(memory, cpu, 0x8852u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpLda(memory, cpu, OpLongX(cpu, 0x7ef001u));
    OpSta(memory, cpu, WRAM_FIELD_PENDING_OBJECT_X);
    OpLda(memory, cpu, OpLongX(cpu, 0x7ef002u));
    OpSta(memory, cpu, WRAM_FIELD_PENDING_OBJECT_Y);
    OpLda(memory, cpu, OpLongX(cpu, 0x7ef00du));
    OpSta(memory, cpu, WRAM_FIELD_REGION_RECORD_ID);
    OpLda(memory, cpu, OpLongX(cpu, 0x7ef00eu));
    OpSta(memory, cpu, WRAM_FIELD_OBJECT_CONTROL_KIND);
    return ExecutionReturned(0x838873u);
}

Lufia2ExecutionResult Lufia2FieldLoadObjectRegionHeader(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ObjectConditionContext(cpu, 0x83u))
        return ExecutionHandoff(cpu, 0x838874u);
    OpLoadA(cpu, 10u);
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, WRAM_FIELD_REGION_RECORD_ID);
    OpLdx(cpu, 4u);
    Lufia2ExecutionResult result = ObjectConditionHeader(memory, cpu, 0x8881u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpLda(memory, cpu, OpLongX(cpu, 0x7ef001u));
    OpSta(memory, cpu, WRAM_FIELD_OBJECT_FLAGS);
    cpu->carry = 1u;
    return ExecutionReturned(0x83888bu);
}

Lufia2ExecutionResult Lufia2FieldSelectObjectCondition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ObjectConditionContext(cpu, 0x8eu))
        return ExecutionHandoff(cpu, 0x8ec338u);
    OpLda(memory, cpu, OpLongX(cpu, OBJECT_CONDITION_TABLE));
    OpAndValue(cpu, 0x80u);
    if (!cpu->zero)
        OpLoadA(cpu, 1u);
    OpSta(memory, cpu, WRAM_FIELD_OBJECT_CONDITION_EXTENDED);
    OpLda(memory, cpu, OpLongX(cpu, OBJECT_CONDITION_TABLE + 1u));
    OpSta(memory, cpu, WRAM_FIELD_OBJECT_CONDITION_FLAG);
    return ExecutionReturned(0x8ec34eu);
}

Lufia2ExecutionResult Lufia2FieldResolveObjectCondition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ObjectConditionContext(cpu, 0x8eu))
        return ExecutionHandoff(cpu, 0x8ec34fu);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, WRAM_FIELD_OBJECT_CONDITION_FLAG);
    Lufia2EventFlagBitFrom(memory, cpu, 0x8eu, 0xc357u);
    OpSta(memory, cpu, OpDp(cpu, 0x54u));
    OpLda(memory, cpu, WRAM_FIELD_OBJECT_CONDITION_EXTENDED);
    if (!cpu->zero) {
        SetAccumulatorWidth(cpu, 0u);
        OpTxa(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, 32u);
        OpTax(cpu);
        SetAccumulatorWidth(cpu, 1u);
    }
    OpLda(memory, cpu, OpDp(cpu, 0x54u));
    return ExecutionReturned(0x8ec36cu);
}
