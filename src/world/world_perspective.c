#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "lufia2/world_map.h"
#include "system/wram.h"

enum {
    DP_COLOR_LENGTH = 0x00u,
    DP_COLOR_REMAINING = 0x01u,
    DP_COLOR_DIVIDEND_HIGH = 0x02u,
    DP_COLOR_DIVISOR = 0x04u,
    DP_COLOR_SCANLINE = 0x06u,
    DP_COLOR_FAR_SCALE = 0x11u,
    DP_COLOR_NEAR_SCALE = 0x12u,
    DP_COLOR_DISTANCE = 0x4eu,
    DP_COLOR_PRODUCT = 0x51u,
    DP_COLOR_PRODUCT_HIGH = 0x52u,
    DP_COLOR_PRODUCT_TOP = 0x53u,
    WORLD_COLOR_TILT = 0x11ffu,
    WORLD_COLOR_TABLE_POINTER = WRAM_WORLD_MAP_COLOR_HDMA_TABLE & 0xffffu,
    WORLD_COLOR_EMPTY_TABLE = 0x1e2fu,
    WORLD_COLOR_TABLE_END = 0x1e4du,
    WORLD_COLOR_TILT_LIMIT = 0x3eu,
    WORLD_COLOR_SCANLINE_END = 0xe0u,
    WORLD_COLOR_BAND_LIMIT = 15u,
    ROM_WORLD_COLOR_SCALE = 0x97b226u
};

static uint8_t PerspectiveEntry(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x86u && !cpu->decimal;
}

static Lufia2ExecutionResult PerspectiveUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2WorldMapUploadInitialTilemap(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!PerspectiveEntry(cpu) || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86cd67u);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_VMAIN));
    OpLdx(cpu, 0u);
    OpWrite16(memory, OpAbs(cpu, SNES_VMADDL), cpu->x);
    OpStz(memory, cpu, OpAbs(cpu, SNES_DMAP(7)));
    OpLoadA(cpu, 0x19u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_BBAD(7)));
    OpLdx(cpu, 0u);
    OpWrite16(memory, OpAbs(cpu, SNES_A1TL(7)), cpu->x);
    OpLoadA(cpu, 0x7fu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_A1B(7)));
    OpLdx(cpu, 0x4000u);
    OpWrite16(memory, OpAbs(cpu, SNES_DASL(7)), cpu->x);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_MDMAEN));
    return ExecutionReturned(0x86cd90u);
}

static void BuildDistanceProduct(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint8_t factor) {
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, factor));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
    OpLda(memory, cpu, OpDp(cpu, DP_COLOR_DISTANCE));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
    OpStz(memory, cpu, OpDp(cpu, DP_COLOR_PRODUCT_TOP));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, SNES_RDMPYL));
    OpSta(memory, cpu, OpDp(cpu, DP_COLOR_PRODUCT));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DP_COLOR_DISTANCE + 1u));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DP_COLOR_PRODUCT_HIGH));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, SNES_RDMPYL));
}

Lufia2ExecutionResult Lufia2WorldMapProjectDistance(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    if (!PerspectiveEntry(cpu) || !child)
        return ExecutionHandoff(cpu, 0x86e356u);
    BuildDistanceProduct(memory, cpu, DP_COLOR_NEAR_SCALE);
    OpAdcValue(cpu, 0x100u);
    OpSta(memory, cpu, OpDp(cpu, DP_COLOR_DIVISOR));
    BuildDistanceProduct(memory, cpu, DP_COLOR_FAR_SCALE);
    OpSta(memory, cpu, OpDp(cpu, DP_COLOR_DIVIDEND_HIGH));
    OpStz(memory, cpu, OpDp(cpu, DP_COLOR_LENGTH));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x86e3a7u, 0x86a5a9u, 2u, 0x86u))
        return PerspectiveUnwound(0x86e3a7u);
    return ExecutionReturned(0x86e3aau);
}

static uint32_t ColorTableAddress(const Lufia2CpuState *cpu, uint8_t offset) {
    return (uint16_t)(cpu->direct_page + cpu->x + offset) | OP_DP_WRAP;
}

static void PublishColorBand(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSta(memory, cpu, ColorTableAddress(cpu, 0u));
    OpLda(memory, cpu, OpDp(cpu, DP_COLOR_SCANLINE));
    OpSta(memory, cpu, ColorTableAddress(cpu, 1u));
}

Lufia2ExecutionResult Lufia2WorldMapBuildColorHdma(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    if (!PerspectiveEntry(cpu) || !child)
        return ExecutionHandoff(cpu, 0x86a7f8u);
    OpSepWidths(cpu, 0x30u);
    OpLda(memory, cpu, OpAbs(cpu, WORLD_COLOR_TILT));
    OpCmpValue(cpu, WORLD_COLOR_TILT_LIMIT);
    if (cpu->carry) {
        OpRepWidths(cpu, 0x10u);
        OpLdx(cpu, WORLD_COLOR_EMPTY_TABLE);
        OpWrite16(memory, OpAbs(cpu, WORLD_COLOR_TABLE_POINTER), cpu->x);
        OpStz(memory, cpu, ColorTableAddress(cpu, 0u));
        return ExecutionReturned(0x86a80bu);
    }
    OpIncA(cpu);
    OpIncA(cpu);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, LongIndexedAddress(ROM_WORLD_COLOR_SCALE, cpu->x));
    OpSta(memory, cpu, OpDp(cpu, DP_COLOR_FAR_SCALE));
    OpLoadA(cpu, WORLD_COLOR_TILT_LIMIT);
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, WORLD_COLOR_TILT)));
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, LongIndexedAddress(ROM_WORLD_COLOR_SCALE, cpu->x));
    OpSta(memory, cpu, OpDp(cpu, DP_COLOR_NEAR_SCALE));
    OpRepWidths(cpu, 0x10u);
    OpLdy(cpu, WORLD_COLOR_SCANLINE_END);
    OpWrite16(memory, OpDp(cpu, DP_COLOR_DISTANCE), cpu->y);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x86a82bu, 0x86e356u, 2u, 0x86u))
        return PerspectiveUnwound(0x86a82bu);
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, WORLD_COLOR_TABLE_END);
    OpStz(memory, cpu, ColorTableAddress(cpu, 2u));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, ColorTableAddress(cpu, 0u));
    OpLdy(cpu, 0xc0u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_RESOURCE_ID));
    OpDecA(cpu);
    if (cpu->zero)
        OpLdy(cpu, WORLD_COLOR_SCANLINE_END);
    OpTya(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_COLOR_SCANLINE));
    OpSta(memory, cpu, ColorTableAddress(cpu, 1u));
    OpLoadA(cpu, WORLD_COLOR_SCANLINE_END);
    cpu->carry = 1u;
    OpSbcValue(cpu, Read8(memory, DirectAddress(cpu, DP_COLOR_REMAINING)));
    OpSbcValue(cpu, 0x28u);
    if (!cpu->carry)
        OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpDp(cpu, DP_COLOR_REMAINING));
    OpLoadA(cpu, 0x10u);
    OpSta(memory, cpu, OpDp(cpu, DP_COLOR_LENGTH));
    OpLoadA(cpu, WORLD_COLOR_BAND_LIMIT);
    OpSta(memory, cpu, OpDp(cpu, DP_COLOR_DIVISOR));
    do {
        OpLda(memory, cpu, OpDp(cpu, DP_COLOR_FAR_SCALE));
        OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
        OpLda(memory, cpu, OpDp(cpu, DP_COLOR_LENGTH));
        OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
        OpDex(cpu);
        OpDex(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, DP_COLOR_SCANLINE), 1);
        OpLda(memory, cpu, OpDp(cpu, DP_COLOR_REMAINING));
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, SNES_RDMPYH)));
        if (cpu->zero || !cpu->carry)
            break;
        OpSta(memory, cpu, OpDp(cpu, DP_COLOR_REMAINING));
        OpLda(memory, cpu, OpAbs(cpu, SNES_RDMPYH));
        OpSta(memory, cpu, OpDp(cpu, DP_COLOR_LENGTH));
        PublishColorBand(memory, cpu);
        OpStepMem(memory, cpu, OpDp(cpu, DP_COLOR_DIVISOR), -1);
    } while (!cpu->zero);
    OpLda(memory, cpu, OpDp(cpu, DP_COLOR_REMAINING));
    PublishColorBand(memory, cpu);
    OpWrite16(memory, OpAbs(cpu, WORLD_COLOR_TABLE_POINTER), cpu->x);
    return ExecutionReturned(0x86a893u);
}
