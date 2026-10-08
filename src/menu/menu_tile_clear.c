#include "core/cpu_ops.h"
#include "lufia2/menu.h"

enum {
    DP_MENU_TILE_POSITION = 0x54u,
    DP_MENU_TILE_COLUMNS = 0x56u,
    DP_MENU_TILE_ROWS = 0x58u,
    MENU_LAYER_TWO_TILES = 0x3000u
};

static uint8_t MenuTileRectangleContext(const Lufia2CpuState *cpu) {
    const unsigned columns = cpu->x >> 8;
    const unsigned rows = cpu->x & 0xffu;
    return cpu->program_bank == 0x82u && !cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && cpu->direct_page == 0u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu &&
        !(cpu->accumulator & 1u) && cpu->accumulator < 0x0800u &&
        columns && rows && ((cpu->accumulator & 0x3fu) >> 1) + columns <= 32u &&
        (cpu->accumulator >> 6) + rows <= 32u;
}

Lufia2ExecutionResult Lufia2MenuClearTileRectangle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!MenuTileRectangleContext(cpu))
        return ExecutionHandoff(cpu, 0x8283ebu);
    OpSta(memory, cpu, OpDp(cpu, DP_MENU_TILE_POSITION));
    OpStz(memory, cpu, OpDp(cpu, DP_MENU_TILE_COLUMNS));
    OpStz(memory, cpu, OpDp(cpu, DP_MENU_TILE_ROWS));
    OpTxa(cpu);
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpDp(cpu, DP_MENU_TILE_ROWS));
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_MENU_TILE_COLUMNS));
    OpRepWidths(cpu, 0x20u);
    PushDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    do {
        OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_MENU_TILE_COLUMNS)));
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_MENU_TILE_POSITION)));
        do {
            OpStz(memory, cpu, OpAbsX(cpu, MENU_LAYER_TWO_TILES));
            OpInx(cpu);
            OpInx(cpu);
            OpDey(cpu);
        } while (!cpu->zero);
        OpLda(memory, cpu, OpDp(cpu, DP_MENU_TILE_POSITION));
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x40u);
        OpSta(memory, cpu, OpDp(cpu, DP_MENU_TILE_POSITION));
        OpStepMem(memory, cpu, OpDp(cpu, DP_MENU_TILE_ROWS), -1);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x82841du);
}
