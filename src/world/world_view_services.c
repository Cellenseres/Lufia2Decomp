#include "lufia2/world_map.h"
#include "core/cpu_ops.h"
#include "system/wram.h"

enum {
    ROM_RESOURCE_POINTER_INDEX = 0xce36,
    ROM_RESOURCE_POINTERS = 0xcffcbc,
    DP_RESOURCE_BANK = 0x10,
    DP_TARGET_PIXEL_X = 0x04,
    DP_TARGET_PIXEL_Y = 0x06,
    TARGET_POSE = 0x09e2,
    TARGET_TILE_X = 0x09e3,
    TARGET_TILE_Y = 0x09e4,
    WORLD_OBJECT_BYTES = 0x1d,
    SECOND_OBJECT = WRAM_WORLD_MAP_OBJECT_TABLE + WORLD_OBJECT_BYTES,
    OBJECT_X = 0x02,
    OBJECT_Y = 0x05,
    OBJECT_POSE = 0x0f,
    VIEW_MODE = 0x11de,
    ROM_SECONDARY_VIEW_ORIGIN = 0xdc55,
    DISPLAY_CONTROL = 0x2100
};

static uint8_t ViewWidths(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit;
}

Lufia2ExecutionResult Lufia2WorldMapResolveResourcePointer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ViewWidths(cpu))
        return ExecutionHandoff(cpu, 0x869f35u);
    OpRepWidths(cpu, 0x20u);
    OpLdy(cpu, OpRead16(memory,
        OpAbs(cpu, WRAM_WORLD_MAP_RESOURCE_TABLE_OFFSET)));
    OpLda(memory, cpu, OpAbsY(cpu, ROM_RESOURCE_POINTER_INDEX));
    OpAslA(cpu);
    OpAdc(memory, cpu, OpAbsY(cpu, ROM_RESOURCE_POINTER_INDEX));
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, ROM_RESOURCE_POINTERS + 2u));
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_RESOURCE_BANK));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, ROM_RESOURCE_POINTERS));
    OpTax(cpu);
    cpu->carry = 0u;
    return ExecutionReturned(0x869f54u);
}

static void CenterTargetTile(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t tile, uint8_t pixel) {
    OpLda(memory, cpu, OpAbs(cpu, tile));
    OpAndValue(cpu, 0xffu);
    for (unsigned bit = 0; bit < 4u; ++bit)
        OpAslA(cpu);
    OpAdcValue(cpu, 8u);
    OpSta(memory, cpu, OpDp(cpu, pixel));
}

Lufia2ExecutionResult Lufia2WorldMapCenterTargetTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ViewWidths(cpu))
        return ExecutionHandoff(cpu, 0x86a28bu);
    OpRepWidths(cpu, 0x20u);
    CenterTargetTile(memory, cpu, TARGET_TILE_X, DP_TARGET_PIXEL_X);
    CenterTargetTile(memory, cpu, TARGET_TILE_Y, DP_TARGET_PIXEL_Y);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x86a2adu);
}

static void ReadObjectTile(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint8_t coordinate) {
    OpLda(memory, cpu, OpAbs(cpu, SECOND_OBJECT + coordinate));
    for (unsigned bit = 0; bit < 4u; ++bit)
        OpLsrA(cpu);
}

Lufia2ExecutionResult Lufia2WorldMapReadSecondObjectTarget(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ViewWidths(cpu))
        return ExecutionHandoff(cpu, 0x86a2c6u);
    OpLda(memory, cpu, OpAbs(cpu, SECOND_OBJECT + OBJECT_POSE));
    OpSta(memory, cpu, OpAbs(cpu, TARGET_POSE));
    OpRepWidths(cpu, 0x20u);
    ReadObjectTile(memory, cpu, OBJECT_X);
    OpTay(cpu);
    ReadObjectTile(memory, cpu, OBJECT_Y);
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, TARGET_TILE_Y));
    OpTya(cpu);
    OpSta(memory, cpu, OpAbs(cpu, TARGET_TILE_X));
    return ExecutionReturned(0x86a2e6u);
}

static Lufia2ExecutionResult ConfigureWorldView(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSta(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_VIEW_COLOUR_MATH));
    OpWrite16(memory, OpAbs(cpu, WRAM_WORLD_MAP_VIEW_PARAMETER), cpu->x);
    OpLoadA(cpu, 0x28u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_VIEW_TILT));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, VIEW_MODE));
    return ExecutionReturned(0x86a38bu);
}

Lufia2ExecutionResult Lufia2WorldMapConfigurePrimaryView(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ViewWidths(cpu))
        return ExecutionHandoff(cpu, 0x86a376u);
    OpLoadA(cpu, 1u);
    OpLdx(cpu, 0x800u);
    return ConfigureWorldView(memory, cpu);
}

Lufia2ExecutionResult Lufia2WorldMapConfigureSecondaryView(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ViewWidths(cpu))
        return ExecutionHandoff(cpu, 0x86a38cu);
    OpLdy(cpu, 0u);
    OpWrite16(memory, OpAbs(cpu, WRAM_UNK_7E16CF), cpu->y);
    OpLda(memory, cpu, OpAbs(cpu, ROM_SECONDARY_VIEW_ORIGIN));
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x70u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_VIEW_ORIGIN));
    OpLoadA(cpu, 0x81u);
    OpLdx(cpu, 0x200u);
    return ConfigureWorldView(memory, cpu);
}

Lufia2ExecutionResult Lufia2WorldMapEnableDisplay(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ViewWidths(cpu))
        return ExecutionHandoff(cpu, 0x86d3aeu);
    OpLoadA(cpu, BRIGHTNESS_FULL);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
    OpSta(memory, cpu, OpAbs(cpu, DISPLAY_CONTROL));
    return ExecutionReturned(0x86d3b6u);
}
