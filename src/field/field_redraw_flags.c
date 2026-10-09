#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/field.h"

enum {
    DP_TILEMAP_UPLOAD_REQUESTS = 0x74u
};

Lufia2ExecutionResult Lufia2FieldRequestTilemapUploads(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x80cc26u);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_EVENT_REDRAW_FLAGS));
    OpLsrA(cpu);
    ExchangeAccumulatorBytes(cpu);
    RorA8(cpu);
    OpLsrA(cpu);
    ExchangeAccumulatorBytes(cpu);
    OpLsrA(cpu);
    ExchangeAccumulatorBytes(cpu);
    RorA8(cpu);
    OpTestBits(memory, cpu, OpDp(cpu, DP_TILEMAP_UPLOAD_REQUESTS), 1u);
    return ExecutionReturned(0x80cc34u);
}
