#include "core/cpu_ops.h"
#include "lufia2/menu.h"
#include "system/wram.h"

enum {
    DP_SCROLL_SOURCE = 8u,
    DP_SCROLL_DESTINATION = 0x0bu,
    DP_SCROLL_ORIGIN = 0x1cu,
    MENU_SCROLL_POSITION = WRAM_BATTLE_PARTY_COUNT,
    MENU_SCROLL_TILEMAP = 0x3000u,
    MENU_SCROLL_STRIDE = 0x40u,
    MENU_SCROLL_FIRST_COLUMN = 4u,
    MENU_SCROLL_COLUMN_END = 0x3cu
};

static bool ScrollTransferReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x82u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && !cpu->direct_page &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static uint32_t ScrollPointer(
    const Lufia2Memory *memory, const Lufia2CpuState *cpu, uint8_t offset) {
    return AbsoluteIndexedAddress(cpu, Read16Direct(memory, cpu, offset), cpu->y);
}

static void EnterScrollTilemapBank(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    PushDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpRepWidths(cpu, 0x20u);
}

static void CopyScrollRow(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdy(cpu, MENU_SCROLL_FIRST_COLUMN);
    do {
        OpLda(memory, cpu, ScrollPointer(memory, cpu, DP_SCROLL_SOURCE));
        OpSta(memory, cpu, ScrollPointer(memory, cpu, DP_SCROLL_DESTINATION));
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, MENU_SCROLL_COLUMN_END);
    } while (!cpu->zero);
}

static void AdvanceScrollPointers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, bool down) {
    OpLda(memory, cpu, OpDp(cpu, DP_SCROLL_DESTINATION));
    cpu->carry = down ? 0u : 1u;
    if (down)
        OpAdcValue(cpu, MENU_SCROLL_STRIDE);
    else
        OpSbcValue(cpu, MENU_SCROLL_STRIDE);
    OpSta(memory, cpu, OpDp(cpu, DP_SCROLL_DESTINATION));
    OpLda(memory, cpu, OpDp(cpu, DP_SCROLL_SOURCE));
    cpu->carry = down ? 0u : 1u;
    if (down)
        OpAdcValue(cpu, MENU_SCROLL_STRIDE);
    else
        OpSbcValue(cpu, MENU_SCROLL_STRIDE);
    OpSta(memory, cpu, OpDp(cpu, DP_SCROLL_SOURCE));
}

static void ClearScrollRows(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdy(cpu, MENU_SCROLL_FIRST_COLUMN);
    OpLoadA(cpu, 0u);
    do {
        OpSta(memory, cpu, ScrollPointer(memory, cpu, DP_SCROLL_SOURCE));
        OpSta(memory, cpu, ScrollPointer(memory, cpu, DP_SCROLL_DESTINATION));
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, MENU_SCROLL_COLUMN_END);
    } while (!cpu->zero);
}

Lufia2ExecutionResult Lufia2MenuShiftScrollTilesUp(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ScrollTransferReady(cpu))
        return ExecutionHandoff(cpu, 0x828f5eu);
    EnterScrollTilemapBank(memory, cpu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_SCROLL_WINDOW_ROWS));
    OpAslA(cpu);
    OpDecA(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, MENU_SCROLL_POSITION)));
    ExchangeAccumulatorBytes(cpu);
    OpLsrA(cpu);
    OpLsrA(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, MENU_SCROLL_TILEMAP);
    OpSta(memory, cpu, OpDp(cpu, DP_SCROLL_DESTINATION));
    cpu->carry = 1u;
    OpSbcValue(cpu, 2u * MENU_SCROLL_STRIDE);
    OpSta(memory, cpu, OpDp(cpu, DP_SCROLL_SOURCE));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_SCROLL_WINDOW_ROWS));
    OpDecA(cpu);
    OpAslA(cpu);
    OpTax(cpu);
    do {
        CopyScrollRow(memory, cpu);
        AdvanceScrollPointers(memory, cpu, false);
        OpDex(cpu);
    } while (!cpu->zero);
    OpLda(memory, cpu, OpAbs(cpu, MENU_SCROLL_POSITION));
    ExchangeAccumulatorBytes(cpu);
    OpLsrA(cpu);
    OpLsrA(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, MENU_SCROLL_TILEMAP);
    OpSta(memory, cpu, OpDp(cpu, DP_SCROLL_DESTINATION));
    OpSta(memory, cpu, OpDp(cpu, DP_SCROLL_ORIGIN));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_SCROLL_WINDOW_ROWS));
    ExchangeAccumulatorBytes(cpu);
    OpLsrA(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_SCROLL_DESTINATION)));
    OpSta(memory, cpu, OpDp(cpu, DP_SCROLL_SOURCE));
    OpLdx(cpu, 2u);
    do {
        CopyScrollRow(memory, cpu);
        AdvanceScrollPointers(memory, cpu, true);
        OpDex(cpu);
    } while (!cpu->zero);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_SCROLL_WINDOW_ROWS));
    ExchangeAccumulatorBytes(cpu);
    OpLsrA(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_SCROLL_ORIGIN)));
    OpSta(memory, cpu, OpDp(cpu, DP_SCROLL_DESTINATION));
    cpu->carry = 0u;
    OpAdcValue(cpu, MENU_SCROLL_STRIDE);
    OpSta(memory, cpu, OpDp(cpu, DP_SCROLL_SOURCE));
    ClearScrollRows(memory, cpu);
    PullDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x829008u);
}

Lufia2ExecutionResult Lufia2MenuShiftScrollTilesDown(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ScrollTransferReady(cpu))
        return ExecutionHandoff(cpu, 0x829009u);
    EnterScrollTilemapBank(memory, cpu);
    OpLda(memory, cpu, OpAbs(cpu, MENU_SCROLL_POSITION));
    ExchangeAccumulatorBytes(cpu);
    OpLsrA(cpu);
    OpLsrA(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, MENU_SCROLL_TILEMAP);
    OpSta(memory, cpu, OpDp(cpu, DP_SCROLL_DESTINATION));
    OpSta(memory, cpu, OpDp(cpu, DP_SCROLL_ORIGIN));
    cpu->carry = 0u;
    OpAdcValue(cpu, 2u * MENU_SCROLL_STRIDE);
    OpSta(memory, cpu, OpDp(cpu, DP_SCROLL_SOURCE));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_SCROLL_WINDOW_ROWS));
    OpAslA(cpu);
    OpTax(cpu);
    do {
        CopyScrollRow(memory, cpu);
        AdvanceScrollPointers(memory, cpu, true);
        OpDex(cpu);
    } while (!cpu->zero);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_SCROLL_WINDOW_ROWS));
    ExchangeAccumulatorBytes(cpu);
    OpLsrA(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_SCROLL_ORIGIN)));
    OpSta(memory, cpu, OpDp(cpu, DP_SCROLL_DESTINATION));
    cpu->carry = 0u;
    OpAdcValue(cpu, MENU_SCROLL_STRIDE);
    OpSta(memory, cpu, OpDp(cpu, DP_SCROLL_SOURCE));
    ClearScrollRows(memory, cpu);
    PullDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x829072u);
}
