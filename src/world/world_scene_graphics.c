#include "core/snes_registers.h"
#include "lufia2/world_map.h"
#include "world/world_scene_oam_internal.h"

enum {
    DP_SCENE_TILE = 0x11u,
    DP_SCENE_X = 0x15u,
    DP_SCENE_Y = 0x17u,
    DP_SCENE_COLUMNS = 0x24u,
    DP_SCENE_COLUMNS_LEFT = 0x26u,
    DP_SCENE_ROWS_LEFT = 0x28u,
    DP_SCENE_FIRST_TILE = 0x54u,
    DP_SCENE_ORIGIN_X = 0x58u,
    DP_SCENE_ORIGIN_Y = 0x5au,
    SCENE_COMMAND_STRIDE = 6u,
    SCENE_GRAPHICS_RESOURCE = 0x28du,
    ROM_SCENE_SPRITE_TILES = 0xf34cu,
    ROM_SCENE_SPRITE_FLAGS = 0xf34eu,
    SCENE_INITIAL_SPRITES = 22u
};

typedef struct SceneGraphics {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
} SceneGraphics;

static bool SceneGraphicsCall(SceneGraphics *graphics, uint32_t site,
                               uint32_t target, uint8_t frame) {
    if (CallChildWithFrame(graphics->memory, graphics->cpu, graphics->child,
                          graphics->context, site, target, frame, 0x86u))
        return true;
    graphics->result = ExecutionReturned(site);
    graphics->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return false;
}

#define SCENE_GRAPHICS_CALL(site, target, frame) \
    do { \
        if (!SceneGraphicsCall(&graphics, site, target, frame)) \
            return graphics.result; \
    } while (0)

Lufia2ExecutionResult Lufia2WorldMapLoadAlternateSceneGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86ce68u);
    SceneGraphics graphics = {memory, cpu, child, context, {0}};
    SCENE_GRAPHICS_CALL(0x86ce68u, 0x86d3a5u, 2u);
    OpStz(memory, cpu, OpAbs(cpu, SNES_NMITIMEN));
    OpStz(memory, cpu, OpAbs(cpu, SNES_HDMAEN));
    OpLdx(cpu, SCENE_GRAPHICS_RESOURCE);
    OpWriteX(memory, cpu, OpDp(cpu, 0x54u), cpu->x);
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x60u), cpu->x);
    OpLoadA(cpu, 0x7fu);
    OpSta(memory, cpu, OpDp(cpu, 0x62u));
    SCENE_GRAPHICS_CALL(0x86ce7fu, 0x808e9du, 3u);
    OpLdx(cpu, 0x4000u);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_VMADDL), cpu->x);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_DMAP(7)));
    OpLoadA(cpu, 0x18u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_BBAD(7)));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x60u)));
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_A1TL(7)), cpu->x);
    OpLda(memory, cpu, OpDp(cpu, 0x62u));
    OpSta(memory, cpu, OpAbs(cpu, SNES_A1B(7)));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x58u)));
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_DASL(7)), cpu->x);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_MDMAEN));
    OpLdy(cpu, 0x320u);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_WMADDL), cpu->y);
    OpStz(memory, cpu, OpAbs(cpu, SNES_WMADDH));
    OpStz(memory, cpu, OpAbs(cpu, SNES_DMAP(7)));
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_BBAD(7)));
    OpLdx(cpu, 0xb800u);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_A1TL(7)), cpu->x);
    OpLoadA(cpu, 0xa6u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_A1B(7)));
    OpLdy(cpu, 0x100u);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_DASL(7)), cpu->y);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_MDMAEN));
    OpLdx(cpu, 0xea20u);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_A1TL(7)), cpu->x);
    OpLoadA(cpu, 0xa6u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_A1B(7)));
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_DASL(7)), cpu->y);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_MDMAEN));
    SCENE_GRAPHICS_CALL(0x86cee1u, 0x86cbbfu, 2u);
    OpStz(memory, cpu, OpAbs(cpu, 0x16feu));
    OpLdx(cpu, 0x294au);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x16ffu), cpu->x);
    SCENE_GRAPHICS_CALL(0x86ceedu, 0x8695cau, 2u);
    OpLoadA(cpu, 0x81u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_NMITIMEN));
    return ExecutionReturned(0x86cef5u);
}

Lufia2ExecutionResult Lufia2WorldMapPrepareAlternateSceneSprites(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86e9d4u);
    Lufia2ExecutionResult result;
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_WORLD_SCENE_SPRITE_INDEX), cpu->x);
    OpLoadA(cpu, SCENE_INITIAL_SPRITES);
    OpSta(memory, cpu, OpDp(cpu, 4u));
    do {
        OpLdy(cpu, 0x6878u);
        OpLoadA(cpu, 2u);
        OpSta(memory, cpu, OpDp(cpu, 5u));
        OpLda(memory, cpu, OpAbsX(cpu, ROM_SCENE_SPRITE_FLAGS));
        if (cpu->zero) {
            OpStz(memory, cpu, OpDp(cpu, 5u));
            OpLdy(cpu, 0x6c7cu);
        }
        OpRepWidths(cpu, 0x20u);
        OpTya(cpu);
        OpSta(memory, cpu, OpAbsX(cpu, (WRAM_WORLD_SCENE_WINDOW_RECORD_STORAGE + 0xau)));
        OpStz(memory, cpu, OpAbsX(cpu, (WRAM_WORLD_SCENE_WINDOW_RECORD_STORAGE + 8u)));
        PushIndex(memory, cpu);
        OpLda(memory, cpu, OpAbs(cpu, WRAM_WORLD_SCENE_SPRITE_INDEX));
        OpAslA(cpu);
        OpAslA(cpu);
        OpTay(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, ROM_SCENE_SPRITE_TILES));
        OpSta(memory, cpu, OpAbsY(cpu, WRAM_FIELD_OAM_LOW_BUFFER + 2u));
        if (!WorldSceneStoreSpriteHighBits(memory, cpu, child, context,
                                           0x86ea18u, &result))
            return result;
        OpPullX(memory, cpu);
        for (unsigned step = 0u; step < 4u; ++step)
            OpInx(cpu);
        OpStepMem(memory, cpu, OpAbs(cpu, WRAM_WORLD_SCENE_SPRITE_INDEX), 1);
        OpStepMem(memory, cpu, OpDp(cpu, 4u), -1);
    } while (!cpu->zero);
    static const uint16_t values[] = {0x7f00u, 0x2000u, 0x200u, 0x80u};
    for (unsigned field = 0u; field < sizeof(values) / sizeof(values[0]); ++field) {
        OpLdx(cpu, values[field]);
        OpWriteX(memory, cpu, OpAbs(cpu, (uint16_t)(WRAM_WORLD_SCENE_WINDOW_RECORD_STORAGE + 2u * field)), cpu->x);
    }
    return ExecutionReturned(0x86ea42u);
}

static Lufia2ExecutionResult EmitSceneSprite(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    Lufia2ExecutionResult result;
    PushIndex(memory, cpu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_WORLD_SCENE_SPRITE_INDEX));
    OpAslA(cpu);
    OpAslA(cpu);
    OpTay(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, 4u));
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_FIELD_OAM_LOW_BUFFER));
    OpLda(memory, cpu, OpDp(cpu, DP_SCENE_Y));
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_FIELD_OAM_LOW_BUFFER + 1u));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DP_SCENE_TILE));
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_FIELD_OAM_LOW_BUFFER + 2u));
    if (!WorldSceneStoreSpriteHighBits(memory, cpu, child, context,
                                       0x86eb45u, &result))
        return result;
    OpStepMem(memory, cpu, OpAbs(cpu, WRAM_WORLD_SCENE_SPRITE_INDEX), 1);
    OpRepWidths(cpu, 0x20u);
    OpPullX(memory, cpu);
    return ExecutionReturned(0x86eb51u);
}

Lufia2ExecutionResult Lufia2WorldMapEmitLargeSceneSprite(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !WorldSceneResourceMode(cpu, false))
        return ExecutionHandoff(cpu, 0x86eb11u);
    OpLda(memory, cpu, OpDp(cpu, DP_SCENE_X));
    OpOraValue(cpu, 0x200u);
    OpSta(memory, cpu, OpDp(cpu, 4u));
    return EmitSceneSprite(memory, cpu, child, context);
}

Lufia2ExecutionResult Lufia2WorldMapEmitSmallSceneSprite(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !WorldSceneResourceMode(cpu, false))
        return ExecutionHandoff(cpu, 0x86eb52u);
    OpLda(memory, cpu, OpDp(cpu, DP_SCENE_X));
    OpAndValue(cpu, 0xfdffu);
    OpSta(memory, cpu, OpDp(cpu, 4u));
    return EmitSceneSprite(memory, cpu, child, context);
}

Lufia2ExecutionResult Lufia2WorldMapExpandSceneSpriteCommand(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !WorldSceneResourceMode(cpu, false))
        return ExecutionHandoff(cpu, 0x86ea58u);
    SceneGraphics graphics = {memory, cpu, child, context, {0}};
    OpSta(memory, cpu, OpDp(cpu, DP_SCENE_FIRST_TILE));
    OpSta(memory, cpu, OpDp(cpu, DP_SCENE_TILE));
    OpLda(memory, cpu, OpAbsX(cpu, 2u));
    OpAndValue(cpu, 0xffu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, DP_SCENE_ORIGIN_X));
    OpSta(memory, cpu, OpDp(cpu, DP_SCENE_X));
    OpLda(memory, cpu, OpAbsX(cpu, 3u));
    OpAndValue(cpu, 0xffu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, DP_SCENE_ORIGIN_Y));
    OpSta(memory, cpu, OpDp(cpu, DP_SCENE_Y));
    OpLda(memory, cpu, OpAbsX(cpu, 4u));
    OpLsrA(cpu);
    if (cpu->carry) {
        OpLda(memory, cpu, OpDp(cpu, DP_SCENE_Y));
        PushAccumulator16(memory, cpu);
        OpLda(memory, cpu, OpAbsX(cpu, 5u));
        OpAndValue(cpu, 0xffu);
        OpSta(memory, cpu, OpDp(cpu, DP_SCENE_ROWS_LEFT));
        do {
            SCENE_GRAPHICS_CALL(0x86ea83u, 0x86eb52u, 2u);
            OpLda(memory, cpu, OpDp(cpu, DP_SCENE_TILE));
            cpu->carry = 0u;
            OpAdcValue(cpu, 0x10u);
            OpSta(memory, cpu, OpDp(cpu, DP_SCENE_TILE));
            OpLda(memory, cpu, OpDp(cpu, DP_SCENE_Y));
            OpAdcValue(cpu, 8u);
            OpSta(memory, cpu, OpDp(cpu, DP_SCENE_Y));
            OpStepMem(memory, cpu, OpDp(cpu, DP_SCENE_ROWS_LEFT), -1);
        } while (!cpu->zero);
        OpLda(memory, cpu, OpDp(cpu, DP_SCENE_FIRST_TILE));
        OpIncA(cpu);
        OpSta(memory, cpu, OpDp(cpu, DP_SCENE_TILE));
        OpLda(memory, cpu, OpDp(cpu, DP_SCENE_X));
        cpu->carry = 0u;
        OpAdcValue(cpu, 8u);
        OpSta(memory, cpu, OpDp(cpu, DP_SCENE_X));
        PullAccumulator16(memory, cpu);
        OpSta(memory, cpu, OpDp(cpu, DP_SCENE_Y));
    }
    OpLda(memory, cpu, OpAbsX(cpu, 4u));
    OpAndValue(cpu, 0xffu);
    OpLsrA(cpu);
    if (cpu->zero)
        return ExecutionReturned(0x86eb10u);
    OpSta(memory, cpu, OpDp(cpu, DP_SCENE_COLUMNS));
    OpLda(memory, cpu, OpAbsX(cpu, 5u));
    OpAndValue(cpu, 0xffu);
    OpLsrA(cpu);
    if (!cpu->zero) {
        OpSta(memory, cpu, OpDp(cpu, DP_SCENE_ROWS_LEFT));
        do {
            OpLda(memory, cpu, OpDp(cpu, DP_SCENE_TILE));
            PushAccumulator16(memory, cpu);
            OpLda(memory, cpu, OpDp(cpu, DP_SCENE_X));
            PushAccumulator16(memory, cpu);
            OpLda(memory, cpu, OpDp(cpu, DP_SCENE_COLUMNS));
            OpSta(memory, cpu, OpDp(cpu, DP_SCENE_COLUMNS_LEFT));
            do {
                SCENE_GRAPHICS_CALL(0x86eac9u, 0x86eb11u, 2u);
                OpStepMem(memory, cpu, OpDp(cpu, DP_SCENE_TILE), 1);
                OpStepMem(memory, cpu, OpDp(cpu, DP_SCENE_TILE), 1);
                OpLda(memory, cpu, OpDp(cpu, DP_SCENE_X));
                cpu->carry = 0u;
                OpAdcValue(cpu, 0x10u);
                OpSta(memory, cpu, OpDp(cpu, DP_SCENE_X));
                OpStepMem(memory, cpu, OpDp(cpu, DP_SCENE_COLUMNS_LEFT), -1);
            } while (!cpu->zero);
            PullAccumulator16(memory, cpu);
            OpSta(memory, cpu, OpDp(cpu, DP_SCENE_X));
            PullAccumulator16(memory, cpu);
            cpu->carry = 0u;
            OpAdcValue(cpu, 0x20u);
            OpSta(memory, cpu, OpDp(cpu, DP_SCENE_TILE));
            OpLda(memory, cpu, OpDp(cpu, DP_SCENE_Y));
            OpAdcValue(cpu, 0x10u);
            OpSta(memory, cpu, OpDp(cpu, DP_SCENE_Y));
            OpStepMem(memory, cpu, OpDp(cpu, DP_SCENE_ROWS_LEFT), -1);
        } while (!cpu->zero);
    }
    OpLda(memory, cpu, OpAbsX(cpu, 5u));
    OpLsrA(cpu);
    if (cpu->carry) {
        OpLda(memory, cpu, OpAbsX(cpu, 4u));
        OpAndValue(cpu, 0xfeu);
        OpSta(memory, cpu, OpDp(cpu, DP_SCENE_COLUMNS_LEFT));
        do {
            SCENE_GRAPHICS_CALL(0x86eaffu, 0x86eb52u, 2u);
            OpStepMem(memory, cpu, OpDp(cpu, DP_SCENE_TILE), 1);
            OpLda(memory, cpu, OpDp(cpu, DP_SCENE_X));
            cpu->carry = 0u;
            OpAdcValue(cpu, 8u);
            OpSta(memory, cpu, OpDp(cpu, DP_SCENE_X));
            OpStepMem(memory, cpu, OpDp(cpu, DP_SCENE_COLUMNS_LEFT), -1);
        } while (!cpu->zero);
    }
    return ExecutionReturned(0x86eb10u);
}

Lufia2ExecutionResult Lufia2WorldMapDrawSceneSpriteCommands(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86ea43u);
    SceneGraphics graphics = {memory, cpu, child, context, {0}};
    OpRepWidths(cpu, 0x20u);
    for (;;) {
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        if (cpu->negative)
            break;
        SCENE_GRAPHICS_CALL(0x86ea4au, 0x86ea58u, 2u);
        OpTxa(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, SCENE_COMMAND_STRIDE);
        OpTax(cpu);
    }
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x86ea57u);
}
