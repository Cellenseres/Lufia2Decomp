#include "lufia2/world_map.h"
#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"

enum {
    WORLD_ACTIVE_OBJECT = 0x11e7,
    WORLD_PAIR_PHASE = 0x1242,
    WORLD_OBJECT_BYTES = 0x1d,
    WORLD_FIRST_PAIRED_OBJECT = WRAM_WORLD_MAP_OBJECT_TABLE + 19 * WORLD_OBJECT_BYTES,
    WORLD_SECOND_PAIRED_OBJECT = WRAM_WORLD_MAP_OBJECT_TABLE + 20 * WORLD_OBJECT_BYTES,
    OBJECT_X = 0x02,
    OBJECT_Y = 0x05,
    OBJECT_POSE = 0x0f,
    OBJECT_ANIMATION = 0x10,
    DP_TARGET_X = 0x58,
    DP_TARGET_Y = 0x5a,
    DP_TILE_OFFSET = 0x00,
    DP_BLOCK_POINTER = 0x08,
    DP_PRIMARY_TILE_LIST = 0xdf,
    DP_SECONDARY_TILE_LIST = 0xe3,
    DP_TILE_CLASSES = 0xed,
    DISPLAY_INTERRUPTS = 0x4200,
    DISPLAY_HDMA = 0x420c,
    DISPLAY_DMA = 0x420b,
    DISPLAY_PALETTE_INDEX = 0x2121,
    DISPLAY_PALETTE_DATA = 0x2122,
    DISPLAY_WINDOW_LEFT = 0x2126,
    DISPLAY_WINDOW_RIGHT = 0x2128,
    DISPLAY_DMA_CONTROL = 0x4370,
    DISPLAY_DMA_PORT = 0x4371,
    DISPLAY_DMA_SOURCE = 0x4372,
    DISPLAY_DMA_BANK = 0x4374,
    DISPLAY_DMA_BYTES = 0x4375
};

static uint32_t WorldObjectField(const Lufia2CpuState *cpu, uint8_t field) {
    return DirectIndexedAddress(cpu, field, cpu->x) | OP_DP_WRAP;
}

static uint8_t FrameWidths(const Lufia2CpuState *cpu,
    Lufia2PushedChildCall child) {
    return cpu->program_bank == 0x86u && cpu->data_bank == 0x86u &&
        !cpu->direct_page && !cpu->decimal &&
        cpu->accumulator_is_8_bit && !cpu->index_is_8_bit && child;
}

static uint8_t RunWorldChild(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target) {
    return CallChildWithFrame(memory, cpu, child, context, site, target, 2u, 0x86u);
}

static Lufia2ExecutionResult WorldChildUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2WorldMapUpdateObjectHeading(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!FrameWidths(cpu, child))
        return ExecutionHandoff(cpu, 0x86992bu);
    OpWrite16(memory, OpAbs(cpu, WRAM_WORLD_MAP_MOTION_HEADING), cpu->y);
    OpLda(memory, cpu, OpAbs(cpu, WORLD_ACTIVE_OBJECT));
    if (!RunWorldChild(memory, cpu, child, context, 0x869931u, 0x86e709u))
        return WorldChildUnwound(0x869931u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_MOTION_HEADING + 1u));
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x20u);
    for (unsigned bit = 0; bit < 6u; ++bit)
        OpLsrA(cpu);
    OpCmp(memory, cpu, WorldObjectField(cpu, OBJECT_POSE));
    if (!cpu->zero) {
        OpSta(memory, cpu, WorldObjectField(cpu, OBJECT_POSE));
        OpLda(memory, cpu, WorldObjectField(cpu, OBJECT_ANIMATION));
        if (!RunWorldChild(memory, cpu, child, context, 0x869948u, 0x86e0b9u))
            return WorldChildUnwound(0x869948u);
    }
    return ExecutionReturned(0x86994bu);
}

Lufia2ExecutionResult Lufia2WorldMapPositionAnimatedObject(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!FrameWidths(cpu, child))
        return ExecutionHandoff(cpu, 0x86a357u);
    OpStz(memory, cpu, WorldObjectField(cpu, OBJECT_POSE));
    if (!RunWorldChild(memory, cpu, child, context, 0x86a359u, 0x86e0b9u))
        return WorldChildUnwound(0x86a359u);
    OpLdy(cpu, OpRead16(memory, OpAbs(cpu, WRAM_WORLD_MAP_POSITION_X_LOW)));
    OpWrite16(memory, WorldObjectField(cpu, OBJECT_X), cpu->y);
    OpLdy(cpu, OpRead16(memory, OpAbs(cpu, WRAM_WORLD_MAP_POSITION_Y)));
    OpWrite16(memory, WorldObjectField(cpu, OBJECT_Y), cpu->y);
    return ExecutionReturned(0x86a366u);
}

Lufia2ExecutionResult Lufia2WorldMapAnimatePairedObjects(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!FrameWidths(cpu, child))
        return ExecutionHandoff(cpu, 0x86a338u);
    OpLdx(cpu, WORLD_FIRST_PAIRED_OBJECT);
    OpLoadA(cpu, 9u);
    OpStepMem(memory, cpu, OpAbs(cpu, WORLD_PAIR_PHASE), -1);
    if (!cpu->zero)
        OpLoadA(cpu, 8u);
    if (!RunWorldChild(memory, cpu, child, context, 0x86a344u, 0x86a357u))
        return WorldChildUnwound(0x86a344u);
    OpLdx(cpu, WORLD_SECOND_PAIRED_OBJECT);
    OpLoadA(cpu, 11u);
    OpStepMem(memory, cpu, OpAbs(cpu, WORLD_PAIR_PHASE), 1);
    if (!cpu->negative)
        OpLoadA(cpu, 10u);
    if (!RunWorldChild(memory, cpu, child, context, 0x86a353u, 0x86a357u))
        return WorldChildUnwound(0x86a353u);
    return ExecutionReturned(0x86a356u);
}

Lufia2ExecutionResult Lufia2WorldMapResetDisplayState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!FrameWidths(cpu, child))
        return ExecutionHandoff(cpu, 0x86d32du);
    if (!RunWorldChild(memory, cpu, child, context, 0x86d32du, 0x86a2c6u))
        return WorldChildUnwound(0x86d32du);
    static const uint8_t scratch[] = {0x6au, 0x72u, 0x73u, 0x74u, 0x81u};
    for (unsigned field = 0; field < sizeof(scratch); ++field)
        OpStz(memory, cpu, OpDp(cpu, scratch[field]));
    OpStz(memory, cpu, OpAbs(cpu, DISPLAY_HDMA));
    if (!RunWorldChild(memory, cpu, child, context, 0x86d33du, 0x86d3a5u))
        return WorldChildUnwound(0x86d33du);
    OpLdy(cpu, 0xff00u);
    OpWrite16(memory, OpAbs(cpu, DISPLAY_WINDOW_LEFT), cpu->y);
    OpWrite16(memory, OpAbs(cpu, DISPLAY_WINDOW_RIGHT), cpu->y);
    return ExecutionReturned(0x86d349u);
}

Lufia2ExecutionResult Lufia2WorldMapClearDisplayPalette(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!FrameWidths(cpu, child))
        return ExecutionHandoff(cpu, 0x86cbbfu);
    if (!RunWorldChild(memory, cpu, child, context, 0x86cbbfu, 0x86d3a5u))
        return WorldChildUnwound(0x86cbbfu);
    OpStz(memory, cpu, OpAbs(cpu, DISPLAY_INTERRUPTS));
    OpStz(memory, cpu, OpAbs(cpu, DISPLAY_HDMA));
    OpLdx(cpu, 0u);
    OpWrite16(memory, OpAbs(cpu, WRAM_CGRAM_BUFFER), cpu->x);
    OpStz(memory, cpu, OpAbs(cpu, DISPLAY_PALETTE_INDEX));
    OpLdx(cpu, WRAM_CGRAM_BUFFER);
    OpWrite16(memory, OpAbs(cpu, DISPLAY_DMA_SOURCE), cpu->x);
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbs(cpu, DISPLAY_DMA_BANK));
    OpStz(memory, cpu, OpAbs(cpu, DISPLAY_DMA_CONTROL));
    OpLoadA(cpu, DISPLAY_PALETTE_DATA & 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, DISPLAY_DMA_PORT));
    OpLdx(cpu, 0x200u);
    OpWrite16(memory, OpAbs(cpu, DISPLAY_DMA_BYTES), cpu->x);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, DISPLAY_DMA));
    return ExecutionReturned(0x86cbefu);
}

Lufia2ExecutionResult Lufia2WorldMapReadTileClass(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!FrameWidths(cpu, child))
        return ExecutionHandoff(cpu, 0x869a71u);
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7fu);
    OpRepWidths(cpu, 0x20u);
    if (!RunWorldChild(memory, cpu, child, context, 0x869a78u, 0x86ae05u))
        return WorldChildUnwound(0x869a78u);
    OpLdy(cpu, 0u);
    OpLda(memory, cpu, OpDp(cpu, DP_TARGET_Y));
    OpLsrA(cpu);
    if (cpu->carry)
        OpLdy(cpu, 4u);
    OpLda(memory, cpu, OpDp(cpu, DP_TARGET_X));
    OpLsrA(cpu);
    if (cpu->carry) {
        OpIny(cpu);
        OpIny(cpu);
    }
    OpWrite16(memory, OpDp(cpu, DP_TILE_OFFSET), cpu->y);
    OpLda(memory, cpu, OpAbs(cpu, OpRead16(memory, OpDp(cpu, DP_BLOCK_POINTER))));
    for (unsigned bit = 0; bit < 3u; ++bit)
        OpAslA(cpu);
    OpAdc(memory, cpu, OpDp(cpu, DP_TILE_OFFSET));
    OpTay(cpu);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu,
        cpu->negative ? DP_SECONDARY_TILE_LIST : DP_PRIMARY_TILE_LIST));
    OpSta(memory, cpu, OpDp(cpu, DP_TILE_OFFSET));
    OpAslA(cpu);
    OpAslA(cpu);
    cpu->carry = 1u;
    OpAdc(memory, cpu, OpDp(cpu, DP_TILE_OFFSET));
    OpTay(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, DP_TILE_CLASSES));
    OpAndValue(cpu, 0xf0u);
    OpSta(memory, cpu, OpDp(cpu, DP_TILE_OFFSET));
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x869ab0u);
}
