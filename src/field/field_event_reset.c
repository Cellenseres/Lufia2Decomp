#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/field.h"

enum {
    MAP_EVENT_STATE_BYTES = 48u,
    MAP_EVENT_VARIABLE_BYTES = 32u
};

Lufia2ExecutionResult Lufia2FieldResetMapEventState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x83b581u);
    OpLdx(cpu, MAP_EVENT_STATE_BYTES - 1u);
    TransferDirectToA(cpu);
    do {
        OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7FD104));
        OpDex(cpu);
    } while (!cpu->negative);
    OpLdx(cpu, WRAM_FIELD_EVENT_POINT_X_COUNT - 1u);
    do {
        OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_EVENT_POINT_X));
        OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_EVENT_POINT_Y));
        OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_EVENT_POINT_COPY_X));
        OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_EVENT_POINT_COPY_Y));
        OpDex(cpu);
    } while (!cpu->negative);
    OpLdx(cpu, MAP_EVENT_VARIABLE_BYTES - 1u);
    do {
        OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_EVENT_VALUE_VARIABLES));
        OpDex(cpu);
    } while (!cpu->negative);
    return ExecutionReturned(0x83b5acu);
}

