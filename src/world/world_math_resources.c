#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "system/wram.h"
#include "lufia2/world_map.h"

enum {
    DP_MULTIPLICAND = 0x00u,
    DP_PRODUCT_HIGH = 0x02u,
    DP_MULTIPLIER = 0x04u,
    DP_SKYLINE_BAND_COUNT = 0x06u,
    DP_SKYLINE_DATA = 0x08u,
    WORLD_SKYLINE_BAND_COUNT = WRAM_WORLD_MAP_SKYLINE_BAND_COUNT & 0xffffu,
    WORLD_SKYLINE_BANDS = 0x16e8u,
    WORLD_SKYLINE_MASK_OFFSET = 0x4038u,
    WORLD_SKYLINE_DATA_BASE = 0x4018u,
    WORLD_RESOURCE_COLORS_A = 0xce38u,
    WORLD_RESOURCE_COLORS_B = 0xce41u,
    ROM_WORLD_ANGLE_SCALE = 0x97b226u
};

static uint8_t WorldMathEntry(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x86u && !cpu->decimal;
}

Lufia2ExecutionResult Lufia2WorldMapResolveAngleScale(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!WorldMathEntry(cpu) || !cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x86a4fau);
    PushAccumulator8(memory, cpu);
    OpBitValue(cpu, 0x40u);
    if (!cpu->zero) {
        OpAndValue(cpu, 0x3fu);
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
        OpIncA(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x40u);
    }
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0x7fu);
    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffffu));
    OpIncA(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x40u);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, ROM_WORLD_ANGLE_SCALE));
    OpSta(memory, cpu, OpDp(cpu, DP_MULTIPLICAND));
    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffffu));
    OpIncA(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, Pull8(memory, cpu));
    OpAndValue(cpu, 0xc0u);
    if (!cpu->zero) {
        OpCmpValue(cpu, 0xc0u);
        if (!cpu->zero) {
            cpu->accumulator = (uint16_t)((cpu->accumulator << 8) |
                (cpu->accumulator >> 8));
            SetNz8(cpu, A8(cpu));
            OpSta(memory, cpu, OpDp(cpu, DP_MULTIPLICAND + 1u));
        }
    }
    return ExecutionReturned(0x86a52eu);
}

static void MultiplyLowBytes(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DP_MULTIPLICAND));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
    OpLda(memory, cpu, OpDp(cpu, DP_MULTIPLIER));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
    OpLda(memory, cpu, OpDp(cpu, DP_MULTIPLICAND + 1u));
    PushAccumulator8(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, SNES_RDMPYL));
    OpSta(memory, cpu, OpDp(cpu, DP_MULTIPLICAND));
}

static void AccumulateByteProduct(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint8_t destination) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, destination));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, SNES_RDMPYL));
    OpSta(memory, cpu, OpDp(cpu, destination));
}

Lufia2ExecutionResult Lufia2WorldMapMultiply16(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!WorldMathEntry(cpu))
        return ExecutionHandoff(cpu, 0x86a52fu);
    Push8(memory, cpu, PackStatus(cpu));
    MultiplyLowBytes(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DP_MULTIPLIER + 1u));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
    OpRepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpDp(cpu, DP_PRODUCT_HIGH));
    AccumulateByteProduct(memory, cpu, DP_MULTIPLICAND + 1u);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, Pull8(memory, cpu));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
    OpLda(memory, cpu, OpDp(cpu, DP_MULTIPLIER));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
    AccumulateByteProduct(memory, cpu, DP_MULTIPLICAND + 1u);
    OpSepWidths(cpu, 0x20u);
    OpRolMem8(memory, cpu, OpDp(cpu, DP_PRODUCT_HIGH + 1u));
    OpLda(memory, cpu, OpDp(cpu, DP_MULTIPLIER + 1u));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
    AccumulateByteProduct(memory, cpu, DP_PRODUCT_HIGH);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x86a582u);
}

static void LoadColorResource(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t table) {
    OpLdy(cpu, OpRead16(memory, OpAbsX(cpu, table)));
    OpWrite16(memory, OpAbs(cpu, SNES_A1TL(7)), cpu->y);
    OpLda(memory, cpu, OpAbsX(cpu, (uint16_t)(table + 2u)));
    OpSta(memory, cpu, OpAbs(cpu, SNES_A1B(7)));
    OpLdy(cpu, 0x100u);
    OpWrite16(memory, OpAbs(cpu, SNES_DASL(7)), cpu->y);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_MDMAEN));
}

Lufia2ExecutionResult Lufia2WorldMapLoadResourceColors(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!WorldMathEntry(cpu) || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86cd91u);
    OpLdy(cpu, 0x320u);
    OpWrite16(memory, OpAbs(cpu, SNES_WMADDL), cpu->y);
    OpStz(memory, cpu, OpAbs(cpu, SNES_WMADDH));
    OpStz(memory, cpu, OpAbs(cpu, SNES_DMAP(7)));
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_BBAD(7)));
    OpLdx(cpu, OpRead16(memory, OpAbs(cpu, WRAM_WORLD_MAP_RESOURCE_TABLE_OFFSET)));
    LoadColorResource(memory, cpu, WORLD_RESOURCE_COLORS_A);
    LoadColorResource(memory, cpu, WORLD_RESOURCE_COLORS_B);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_RESOURCE_ID));
    if (cpu->zero) {
        OpLdy(cpu, 0x500u);
        OpWrite16(memory, OpAbs(cpu, SNES_WMADDL), cpu->y);
        OpLdy(cpu, 0xc000u);
        OpWrite16(memory, OpAbs(cpu, SNES_A1TL(7)), cpu->y);
        OpLoadA(cpu, 0xa6u);
        OpSta(memory, cpu, OpAbs(cpu, SNES_A1B(7)));
        OpLdy(cpu, 0x20u);
        OpWrite16(memory, OpAbs(cpu, SNES_DASL(7)), cpu->y);
        OpLoadA(cpu, 0x80u);
        OpSta(memory, cpu, OpAbs(cpu, SNES_MDMAEN));
    }
    return ExecutionReturned(0x86cdf4u);
}

static uint32_t SkylineBandAddress(const Lufia2CpuState *cpu, uint8_t offset) {
    return (uint16_t)(cpu->direct_page + cpu->x + offset) | OP_DP_WRAP;
}

static uint32_t SkylineDataAddress(const Lufia2Memory *memory,
    const Lufia2CpuState *cpu) {
    return OpAbs(cpu, OpRead16(memory, OpDp(cpu, DP_SKYLINE_DATA)));
}

static void AppendSkylineBand(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpWrite16(memory, SkylineBandAddress(cpu, 0u), cpu->y);
    OpLda(memory, cpu, SkylineDataAddress(memory, cpu));
    OpIncA(cpu);
    OpSta(memory, cpu, SkylineBandAddress(cpu, 1u));
    OpStz(memory, cpu, SkylineBandAddress(cpu, 3u));
    OpStepMem(memory, cpu, OpDp(cpu, DP_SKYLINE_BAND_COUNT), 1);
    OpTxa(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, 5u);
    OpTax(cpu);
}

Lufia2ExecutionResult Lufia2WorldMapBuildSkylineHdma(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!WorldMathEntry(cpu) || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit ||
        cpu->direct_page || cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x86cbf0u);
    OpStz(memory, cpu, OpAbs(cpu, WORLD_SKYLINE_BAND_COUNT));
    PushDataBank(memory, cpu);
    OpLoadA(cpu, 0x7fu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpDp(cpu, DP_SKYLINE_BAND_COUNT));
    OpLda(memory, cpu, OpAbs(cpu, WORLD_SKYLINE_MASK_OFFSET));
    cpu->carry = 0u;
    OpAdcValue(cpu, WORLD_SKYLINE_DATA_BASE);
    OpSta(memory, cpu, OpDp(cpu, DP_SKYLINE_DATA));
    OpLda(memory, cpu, SkylineDataAddress(memory, cpu));
    OpStepMem(memory, cpu, OpDp(cpu, DP_SKYLINE_DATA), 1);
    OpStepMem(memory, cpu, OpDp(cpu, DP_SKYLINE_DATA), 1);
    OpLdx(cpu, WORLD_SKYLINE_BANDS);
    OpLdy(cpu, 1u);
    do {
        OpLsrA(cpu);
        PushAccumulator16(memory, cpu);
        if (cpu->carry)
            AppendSkylineBand(memory, cpu);
        OpStepMem(memory, cpu, OpDp(cpu, DP_SKYLINE_DATA), 1);
        OpStepMem(memory, cpu, OpDp(cpu, DP_SKYLINE_DATA), 1);
        OpTya(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x10u);
        OpTay(cpu);
        PullAccumulator16(memory, cpu);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    PullDataBank(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, DP_SKYLINE_BAND_COUNT));
    OpSta(memory, cpu, OpAbs(cpu, WORLD_SKYLINE_BAND_COUNT));
    return ExecutionReturned(0x86cc3bu);
}
