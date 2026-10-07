#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/dp_scratch.h"
#include "system/wram.h"

enum {
    EVENT_ORIGIN_NORMALIZE = 0xae,
    REGION_LEFT = 0x9f,
    REGION_TOP = 0xa0,
    REGION_RIGHT = 0xa1,
    REGION_BOTTOM = 0xa2,
    REGION_CHANGED_FLAGS = 0x54,
    PENDING_RECORD_COUNT = 48
};

static bool EventMapStateContext(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit &&
        !cpu->decimal && !cpu->direct_page && cpu->program_bank == 0x80u &&
        cpu->stack >= 0x1f04u && cpu->stack <= 0x1ffcu;
}

Lufia2ExecutionResult Lufia2FieldSaveObjectRegion(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EventMapStateContext(cpu))
        return ExecutionHandoff(cpu, 0x80d136u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, WRAM_FIELD_PENDING_OBJECT_X);
    OpSta(memory, cpu, WRAM_FIELD_SAVED_OBJECT_POSITION);
    OpLda(memory, cpu, WRAM_FIELD_OBJECT_SOURCE_X);
    OpSta(memory, cpu, WRAM_FIELD_SAVED_OBJECT_SOURCE);
    OpLda(memory, cpu, WRAM_FIELD_OBJECT_WIDTH);
    OpSta(memory, cpu, WRAM_FIELD_SAVED_OBJECT_SIZE);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, WRAM_FIELD_OBJECT_FLAGS);
    OpSta(memory, cpu, WRAM_FIELD_SAVED_OBJECT_FLAGS);
    return ExecutionReturned(0x80d15au);
}

Lufia2ExecutionResult Lufia2FieldRestoreObjectRegion(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EventMapStateContext(cpu))
        return ExecutionHandoff(cpu, 0x80d15bu);
    OpLda(memory, cpu, WRAM_FIELD_SAVED_OBJECT_POSITION);
    OpSta(memory, cpu, WRAM_FIELD_PENDING_OBJECT_X);
    OpSta(memory, cpu, OpDp(cpu, DP_PROBE_X));
    OpLda(memory, cpu, WRAM_FIELD_SAVED_OBJECT_POSITION + 1u);
    OpSta(memory, cpu, WRAM_FIELD_PENDING_OBJECT_Y);
    OpSta(memory, cpu, OpDp(cpu, DP_PROBE_Y));
    OpLda(memory, cpu, WRAM_FIELD_SAVED_OBJECT_FLAGS);
    OpSta(memory, cpu, WRAM_FIELD_OBJECT_FLAGS);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, WRAM_FIELD_SAVED_OBJECT_SOURCE);
    OpSta(memory, cpu, WRAM_FIELD_OBJECT_SOURCE_X);
    OpLda(memory, cpu, WRAM_FIELD_SAVED_OBJECT_SIZE);
    OpSta(memory, cpu, WRAM_FIELD_OBJECT_WIDTH);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x80d18bu);
}

Lufia2ExecutionResult Lufia2FieldNormalizeObjectOrigin(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EventMapStateContext(cpu))
        return ExecutionHandoff(cpu, 0x80d18cu);
    OpLda(memory, cpu, OpDp(cpu, EVENT_ORIGIN_NORMALIZE));
    if (!cpu->zero) {
        OpLda(memory, cpu, WRAM_FIELD_PENDING_OBJECT_Y);
        cpu->carry = true;
        Sbc8(cpu, Read8(memory, WRAM_FIELD_OBJECT_HEIGHT));
        OpIncA(cpu);
        OpSta(memory, cpu, WRAM_FIELD_PENDING_OBJECT_Y);
    }
    return ExecutionReturned(0x80d19eu);
}

Lufia2ExecutionResult Lufia2FieldMarkRegionObjects(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EventMapStateContext(cpu))
        return ExecutionHandoff(cpu, 0x80d227u);
    OpStz(memory, cpu, OpDp(cpu, REGION_CHANGED_FLAGS));
    PushDataBank(memory, cpu);
    OpLoadA(cpu, 0x7fu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpLda(memory, cpu, OpAbs(cpu, (WRAM_FIELD_PENDING_OBJECT_X & 0xffffu)));
    OpSta(memory, cpu, OpDp(cpu, REGION_LEFT));
    cpu->carry = false;
    OpAdc(memory, cpu, OpAbs(cpu, (WRAM_FIELD_OBJECT_WIDTH & 0xffffu)));
    OpSta(memory, cpu, OpDp(cpu, REGION_RIGHT));
    OpLda(memory, cpu, OpAbs(cpu, (WRAM_FIELD_PENDING_OBJECT_Y & 0xffffu)));
    OpSta(memory, cpu, OpDp(cpu, REGION_TOP));
    cpu->carry = false;
    OpAdc(memory, cpu, OpAbs(cpu, (WRAM_FIELD_OBJECT_HEIGHT & 0xffffu)));
    OpIncA(cpu);
    OpSta(memory, cpu, OpDp(cpu, REGION_BOTTOM));
    OpLdx(cpu, PENDING_RECORD_COUNT - 1u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, (WRAM_FIELD_PENDING_RECORD_FLAGS & 0xffffu)));
        if (cpu->negative) {
            OpLda(memory, cpu, OpAbsX(cpu, (WRAM_FIELD_PENDING_RECORD_X & 0xffffu)));
            OpCmpValue(cpu, OpReadM(memory, cpu, OpDp(cpu, REGION_LEFT)));
            if (cpu->carry) {
                OpCmpValue(cpu, OpReadM(memory, cpu, OpDp(cpu, REGION_RIGHT)));
                if (!cpu->carry) {
                    OpLda(memory, cpu, OpAbsX(cpu, (WRAM_FIELD_PENDING_RECORD_Y & 0xffffu)));
                    OpCmpValue(cpu, OpReadM(memory, cpu, OpDp(cpu, REGION_TOP)));
                    if (cpu->carry) {
                        OpCmpValue(cpu, OpReadM(memory, cpu, OpDp(cpu, REGION_BOTTOM)));
                        if (!cpu->carry) {
                            OpLda(memory, cpu, OpAbsX(cpu, (WRAM_FIELD_PENDING_RECORD_FLAGS & 0xffffu)));
                            OpOraValue(cpu, 1u);
                            OpSta(memory, cpu, OpAbsX(cpu, (WRAM_FIELD_PENDING_RECORD_FLAGS & 0xffffu)));
                            OpTestBits(memory, cpu, OpDp(cpu, REGION_CHANGED_FLAGS), 1u);
                        }
                    }
                }
            }
        }
        OpDex(cpu);
    } while (!cpu->negative);
    PullDataBank(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, REGION_CHANGED_FLAGS));
    return ExecutionReturned(0x80d273u);
}
