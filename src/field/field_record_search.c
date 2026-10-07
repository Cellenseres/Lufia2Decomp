#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/dp_scratch.h"
#include "system/wram.h"

enum {
    FIELD_RECORD_STORAGE = 0x7ef000,
    RECORD_STRIDE = 0x54,
    RECORD_END = 0xff,
    POINT_X = 1,
    POINT_Y = 2,
    RECTANGLE_RIGHT = 3,
    RECTANGLE_BOTTOM = 4
};

static bool FieldRecordSearchContext(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit &&
        !cpu->decimal && !cpu->direct_page && cpu->program_bank == 0x83u &&
        cpu->stack >= 0x1f04u && cpu->stack <= 0x1ffcu && cpu->y;
}

static void FieldRecordSearchStart(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    Write16Direct(memory, cpu, RECORD_STRIDE, cpu->y);
    OpLda(memory, cpu, OpLongX(cpu, FIELD_RECORD_STORAGE));
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
}

static void FieldRecordSearchNext(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpTxa(cpu);
    cpu->carry = false;
    OpAdc(memory, cpu, OpDp(cpu, RECORD_STRIDE));
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
}

Lufia2ExecutionResult Lufia2FieldFindPointRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!FieldRecordSearchContext(cpu))
        return ExecutionHandoff(cpu, 0x83b851u);
    FieldRecordSearchStart(memory, cpu);
    for (;;) {
        OpLda(memory, cpu, OpLongX(cpu, FIELD_RECORD_STORAGE));
        OpCmpValue(cpu, RECORD_END);
        cpu->carry = false;
        if (cpu->zero)
            return ExecutionReturned(0x83b881u);
        OpLda(memory, cpu, OpLongX(cpu, FIELD_RECORD_STORAGE + POINT_X));
        OpCmpValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_PROBE_X)));
        if (cpu->zero) {
            OpLda(memory, cpu, OpLongX(cpu, FIELD_RECORD_STORAGE + POINT_Y));
            OpCmpValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_PROBE_Y)));
            cpu->carry = true;
            if (cpu->zero)
                return ExecutionReturned(0x83b881u);
        }
        FieldRecordSearchNext(memory, cpu);
    }
}

Lufia2ExecutionResult Lufia2FieldFindRectangleRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!FieldRecordSearchContext(cpu))
        return ExecutionHandoff(cpu, 0x83b882u);
    FieldRecordSearchStart(memory, cpu);
    for (;;) {
        OpLda(memory, cpu, OpLongX(cpu, FIELD_RECORD_STORAGE));
        OpCmpValue(cpu, RECORD_END);
        cpu->carry = false;
        if (cpu->zero)
            return ExecutionReturned(0x83b8beu);
        OpLda(memory, cpu, OpDp(cpu, DP_PROBE_X));
        OpCmpValue(cpu, OpReadM(memory, cpu, OpLongX(cpu, FIELD_RECORD_STORAGE + POINT_X)));
        if (cpu->carry) {
            OpCmpValue(cpu, OpReadM(memory, cpu, OpLongX(cpu, FIELD_RECORD_STORAGE + RECTANGLE_RIGHT)));
            if (!cpu->carry) {
                OpLda(memory, cpu, OpDp(cpu, DP_PROBE_Y));
                OpCmpValue(cpu, OpReadM(memory, cpu, OpLongX(cpu, FIELD_RECORD_STORAGE + POINT_Y)));
                if (cpu->carry) {
                    OpCmpValue(cpu, OpReadM(memory, cpu, OpLongX(cpu, FIELD_RECORD_STORAGE + RECTANGLE_BOTTOM)));
                    if (!cpu->carry) {
                        cpu->carry = true;
                        return ExecutionReturned(0x83b8beu);
                    }
                }
            }
        }
        FieldRecordSearchNext(memory, cpu);
    }
}
