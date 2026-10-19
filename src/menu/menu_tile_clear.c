#include "core/plain_ops.h"
#include "core/cpu_ops.h"
#include "core/wram_view.h"
#include "lufia2/menu.h"
#include "lufia2/menu_rectangle.h"

enum {
    WORK_POSITION = 0x54u,
    WORK_COLUMNS = 0x56u,
    WORK_ROWS = 0x58u,
    FRONT_TILES = 0x7e2000u,
    BACK_TILES = 0x7e3000u,
    TILEMAP_BYTES = 0x0800u,
    ROW_BYTES = 0x40u,
    ROW_TILES = 32u
};

static bool RectangleSupported(const Lufia2CpuState *cpu) {
    const unsigned columns = cpu->x >> 8;
    const unsigned rows = cpu->x & 0xffu;
    return cpu->program_bank == 0x82u && !cpu->accumulator_is_8_bit &&
           !cpu->index_is_8_bit && !cpu->decimal && !cpu->direct_page &&
           cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu &&
           !(cpu->accumulator & 1u) && cpu->accumulator < TILEMAP_BYTES &&
           columns && rows &&
           ((cpu->accumulator & (ROW_BYTES - 1u)) >> 1) + columns <= ROW_TILES &&
           cpu->accumulator / ROW_BYTES + rows <= ROW_TILES;
}

static void ClearRectangle(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                           bool both_layers) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint16_t cell;
    Word16Result next_row;

    WramWrite16(wram, WORK_POSITION, cpu->accumulator);
    WramWrite16(wram, WORK_COLUMNS, 0);
    WramWrite16(wram, WORK_ROWS, 0);
    WramWrite(wram, WORK_ROWS, (uint8_t)cpu->x);
    WramWrite(wram, WORK_COLUMNS, (uint8_t)(cpu->x >> 8));
    PushDataBank(memory, cpu);
    Push8(memory, cpu, 0x7eu);
    PullDataBank(memory, cpu);
    do {
        uint16_t columns = WramRead16(wram, WORK_COLUMNS);
        cell = WramRead16(wram, WORK_POSITION);
        do {
            if (both_layers)
                WramWrite16At(wram, FRONT_TILES, cell, 0);
            WramWrite16At(wram, BACK_TILES, cell, 0);
            cell = (uint16_t)(cell + 2u);
            columns = (uint16_t)(columns - 1u);
        } while (columns);
        next_row = Sum16(WramRead16(wram, WORK_POSITION), ROW_BYTES, false);
        WramWrite16(wram, WORK_POSITION, next_row.value);
    } while (WramStep16(wram, WORK_ROWS, -1));
    cpu->accumulator = next_row.value;
    cpu->x = cell;
    cpu->y = 0;
    SetSumFlags(cpu, next_row);
    PullDataBank(memory, cpu);
}

Lufia2ExecutionResult Lufia2MenuClearLayerRectangle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!RectangleSupported(cpu))
        return ExecutionHandoff(cpu, 0x8283b5u);
    ClearRectangle(memory, cpu, true);
    return ExecutionReturned(0x8283eau);
}

Lufia2ExecutionResult Lufia2MenuClearTileRectangle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!RectangleSupported(cpu))
        return ExecutionHandoff(cpu, 0x8283ebu);
    ClearRectangle(memory, cpu, false);
    return ExecutionReturned(0x82841du);
}

Lufia2ExecutionResult Lufia2MenuClearLayerRectangleStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry) {
    if (cpu->program_bank != 0x82u || cpu->index_is_8_bit ||
        cpu->accumulator_is_8_bit > 1u || cpu->direct_page || cpu->decimal)
        return ExecutionHandoff(cpu, entry);
    const bool byte_phase = entry == 0x8283beu || entry == 0x8283c0u ||
                            entry == 0x8283c1u || entry == 0x8283c3u ||
                            entry == 0x8283c8u || entry == 0x8283cau ||
                            entry == 0x8283cbu || entry == 0x8283ccu;
    if (cpu->accumulator_is_8_bit != byte_phase)
        return ExecutionHandoff(cpu, entry);
    uint32_t next;
    switch (entry) {
    case 0x8283b5u:
        OpSta(memory, cpu, OpDp(cpu, WORK_POSITION));
        next = 0x8283b7u;
        break;
    case 0x8283b7u:
        OpStz(memory, cpu, OpDp(cpu, WORK_COLUMNS));
        next = 0x8283b9u;
        break;
    case 0x8283b9u:
        OpStz(memory, cpu, OpDp(cpu, WORK_ROWS));
        next = 0x8283bbu;
        break;
    case 0x8283bbu:
        OpTxa(cpu);
        next = 0x8283bcu;
        break;
    case 0x8283bcu:
        OpSepWidths(cpu, 0x20u);
        next = 0x8283beu;
        break;
    case 0x8283beu:
        OpSta(memory, cpu, OpDp(cpu, WORK_ROWS));
        next = 0x8283c0u;
        break;
    case 0x8283c0u:
        ExchangeAccumulatorBytes(cpu);
        next = 0x8283c1u;
        break;
    case 0x8283c1u:
        OpSta(memory, cpu, OpDp(cpu, WORK_COLUMNS));
        next = 0x8283c3u;
        break;
    case 0x8283c3u:
        OpRepWidths(cpu, 0x20u);
        next = 0x8283c5u;
        break;
    case 0x8283c5u:
        PushDataBank(memory, cpu);
        next = 0x8283c6u;
        break;
    case 0x8283c6u:
        OpSepWidths(cpu, 0x20u);
        next = 0x8283c8u;
        break;
    case 0x8283c8u:
        OpLoadA(cpu, 0x7eu);
        next = 0x8283cau;
        break;
    case 0x8283cau:
        PushAccumulator8(memory, cpu);
        next = 0x8283cbu;
        break;
    case 0x8283cbu:
        PullDataBank(memory, cpu);
        next = 0x8283ccu;
        break;
    case 0x8283ccu:
        OpRepWidths(cpu, 0x20u);
        next = 0x8283ceu;
        break;
    case 0x8283ceu:
        OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, WORK_COLUMNS)));
        next = 0x8283d0u;
        break;
    case 0x8283d0u:
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, WORK_POSITION)));
        next = 0x8283d2u;
        break;
    case 0x8283d2u:
        OpStz(memory, cpu, OpAbsX(cpu, 0x2000u));
        next = 0x8283d5u;
        break;
    case 0x8283d5u:
        OpStz(memory, cpu, OpAbsX(cpu, 0x3000u));
        next = 0x8283d8u;
        break;
    case 0x8283d8u:
        OpInx(cpu);
        next = 0x8283d9u;
        break;
    case 0x8283d9u:
        OpInx(cpu);
        next = 0x8283dau;
        break;
    case 0x8283dau:
        OpDey(cpu);
        next = 0x8283dbu;
        break;
    case 0x8283dbu:
        next = cpu->zero ? 0x8283ddu : 0x8283d2u;
        break;
    case 0x8283ddu:
        OpLda(memory, cpu, OpDp(cpu, WORK_POSITION));
        next = 0x8283dfu;
        break;
    case 0x8283dfu:
        cpu->carry = 0;
        next = 0x8283e0u;
        break;
    case 0x8283e0u:
        OpAdcValue(cpu, ROW_BYTES);
        next = 0x8283e3u;
        break;
    case 0x8283e3u:
        OpSta(memory, cpu, OpDp(cpu, WORK_POSITION));
        next = 0x8283e5u;
        break;
    case 0x8283e5u:
        OpStepMem(memory, cpu, OpDp(cpu, WORK_ROWS), -1);
        next = 0x8283e7u;
        break;
    case 0x8283e7u:
        next = cpu->zero ? 0x8283e9u : 0x8283ceu;
        break;
    case 0x8283e9u:
        PullDataBank(memory, cpu);
        next = 0x8283eau;
        break;
    default:
        return ExecutionHandoff(cpu, entry);
    }
    return ExecutionHandoff(cpu, next);
}
