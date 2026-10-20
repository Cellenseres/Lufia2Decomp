#include "core/snes_registers.h"
#include "lufia2/world_map.h"
#include "system/wram.h"
#include "world/world_scene_resources_internal.h"
#include "world/world_scene_internal.h"

enum {
    SCENE_GROUP_START = WRAM_WORLD_MAP_OBJECT_TABLE + 2u * 0x1du,
    SCENE_GROUP_ANCHOR = WRAM_WORLD_MAP_OBJECT_TABLE + 6u * 0x1du,
    SCENE_GROUP_STRIDE = 0x1du,
    SCENE_GROUP_COUNT = 4u,
    SCENE_GROUP_SPACING = WRAM_FIELD_STREAMED_ROW_VRAM,
    SCENE_WINDOW_SHORT_RECORDS = WRAM_WORLD_SCENE_WINDOW_RECORD_STORAGE,
    SCENE_WINDOW_LONG_RECORDS = WRAM_WORLD_SCENE_WINDOW_RECORD_STORAGE + 0x18u,
    SCENE_WINDOW_RECORD_COUNT = 6u,
    SCENE_WINDOW_STREAMS = 0x7fd400u,
    SCENE_WINDOW_STREAM_SIZE = 0x800u,
    PALETTE_SUBTRACT_COLOR = 0x15u,
    PALETTE_COLORS_LEFT = 0x26u,
    PALETTE_RED = 0u,
    PALETTE_BLUE = 1u,
    PALETTE_GREEN = 6u,
    PALETTE_RESULT = 4u,
    PALETTE_RESULT_OFFSET = 0x7fd8e0u
};

Lufia2ExecutionResult Lufia2WorldMapOffsetSceneObject(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu) {
    if (!WorldSceneResourceMode(cpu, false))
        return ExecutionHandoff(cpu, 0x86bec4u);
    enum { HORIZONTAL_OFFSET = 4u, VERTICAL_OFFSET = 6u };
    OpSta(memory, cpu, OpDp(cpu, HORIZONTAL_OFFSET));
    OpWrite16(memory, OpDp(cpu, VERTICAL_OFFSET), cpu->y);
    OpLda(memory, cpu, OpAbs(cpu, SCENE_GROUP_ANCHOR + 2u));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, HORIZONTAL_OFFSET));
    OpSta(memory, cpu, WorldSceneObjectField(cpu, 2u));
    OpLda(memory, cpu, OpAbs(cpu, SCENE_GROUP_ANCHOR + 5u));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, VERTICAL_OFFSET));
    OpSta(memory, cpu, WorldSceneObjectField(cpu, 5u));
    return ExecutionReturned(0x86bed8u);
}

Lufia2ExecutionResult Lufia2WorldMapPositionSceneGroup(const Lufia2Memory *memory,
                                                       Lufia2CpuState *cpu) {
    if (!WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86c2b3u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, SCENE_GROUP_ANCHOR + 2u));
    for (unsigned actor = 0u; actor < SCENE_GROUP_COUNT; ++actor)
        OpSta(memory, cpu,
              OpAbs(cpu,
                    (uint16_t)(SCENE_GROUP_START + actor * SCENE_GROUP_STRIDE + 2u)));
    OpLda(memory, cpu, OpAbs(cpu, SCENE_GROUP_ANCHOR + 5u));
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, SCENE_GROUP_SPACING)));
    for (unsigned actor = 0u; actor < SCENE_GROUP_COUNT; ++actor)
        OpSta(memory, cpu,
              OpAbs(cpu,
                    (uint16_t)(SCENE_GROUP_START + actor * SCENE_GROUP_STRIDE + 5u)));
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x86c2d8u);
}

static void SetSceneWindowRegisters(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdy(cpu, 0xffu);
    OpWrite16(memory, OpAbs(cpu, SNES_WH0), cpu->y);
    OpWrite16(memory, OpAbs(cpu, SNES_WH2), cpu->y);
    OpStz(memory, cpu, OpAbs(cpu, SNES_W12SEL));
    OpLoadA(cpu, 0xa0u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WOBJSEL));
    OpLoadA(cpu, 0x11u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_TMW));
    OpStz(memory, cpu, OpAbs(cpu, SNES_WOBJLOG));
    OpLoadA(cpu, 0x31u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_CGADSUB));
    OpLoadA(cpu, 0xe0u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_COLDATA));
    OpLoadA(cpu, 0x10u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_CGWSEL));
}

static void ResetSceneWindowStreams(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLoadA(cpu, 0u);
    for (unsigned stream = 0u; stream < 4u; ++stream)
        OpSta(memory, cpu, SCENE_WINDOW_STREAMS + stream * 0x200u);
    OpSta(memory, cpu, SCENE_WINDOW_STREAMS + SCENE_WINDOW_STREAM_SIZE - 1u);
    OpDecA(cpu);
    OpSta(memory, cpu, SCENE_WINDOW_STREAMS + SCENE_WINDOW_STREAM_SIZE - 2u);
}

static uint32_t SceneWindowRecordField(const Lufia2CpuState *cpu, uint8_t offset) {
    return DirectIndexedAddress(cpu, offset, cpu->x) | OP_DP_WRAP;
}

static void ResetSceneWindowRecords(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, SCENE_WINDOW_LONG_RECORDS);
    OpLdy(cpu, SCENE_WINDOW_RECORD_COUNT);
    OpRepWidths(cpu, 0x20u);
    OpTxa(cpu);
    cpu->carry = 0u;
    do {
        OpStz(memory, cpu, SceneWindowRecordField(cpu, 0x0au));
        OpStz(memory, cpu, SceneWindowRecordField(cpu, 0x0cu));
        OpAdcValue(cpu, 0x0fu);
        OpTax(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    OpLdx(cpu, SCENE_WINDOW_SHORT_RECORDS);
    OpLdy(cpu, SCENE_WINDOW_RECORD_COUNT);
    OpTxa(cpu);
    do {
        OpStz(memory, cpu, SceneWindowRecordField(cpu, 2u));
        OpStz(memory, cpu, SceneWindowRecordField(cpu, 4u));
        OpAdcValue(cpu, 6u);
        OpTax(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
}

Lufia2ExecutionResult Lufia2WorldMapPrepareSceneWindowTables(const Lufia2Memory *memory,
                                                             Lufia2CpuState *cpu) {
    if (!WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86c582u);
    SetSceneWindowRegisters(memory, cpu);
    ResetSceneWindowStreams(memory, cpu);
    ResetSceneWindowRecords(memory, cpu);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_BACKGROUND_ID));
    return ExecutionReturned(0x86c5f3u);
}

static uint32_t ScenePaletteColor(const Lufia2CpuState *cpu, uint8_t offset) {
    return DirectIndexedAddress(cpu, offset, cpu->x) | OP_DP_WRAP;
}

static void SubtractPaletteComponent(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                     uint8_t offset, uint16_t mask, uint8_t subtract) {
    OpLda(memory, cpu, ScenePaletteColor(cpu, offset));
    OpAndValue(cpu, mask);
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, subtract)));
    if (!cpu->carry) {
        cpu->carry = 1u;
        OpLoadA(cpu, 0u);
    }
}

static void PrepareScenePaletteOffset(const Lufia2Memory *memory,
                                      Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, PALETTE_SUBTRACT_COLOR));
    OpAndValue(cpu, 0x1fu);
    OpSta(memory, cpu, OpDp(cpu, PALETTE_RED));
    OpLda(memory, cpu, OpDp(cpu, PALETTE_SUBTRACT_COLOR + 1u));
    OpAndValue(cpu, 0x7cu);
    OpSta(memory, cpu, OpDp(cpu, PALETTE_BLUE));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, PALETTE_SUBTRACT_COLOR));
    OpAndValue(cpu, 0x3e0u);
    OpSta(memory, cpu, OpDp(cpu, PALETTE_GREEN));
    OpSepWidths(cpu, 0x20u);
}

Lufia2ExecutionResult Lufia2WorldMapDarkenScenePalette(const Lufia2Memory *memory,
                                                       Lufia2CpuState *cpu) {
    if (!WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86966cu);
    PrepareScenePaletteOffset(memory, cpu);
    cpu->carry = 1u;
    do {
        SubtractPaletteComponent(memory, cpu, 0u, 0x1fu, PALETTE_RED);
        OpSta(memory, cpu, OpDp(cpu, PALETTE_RESULT));
        SubtractPaletteComponent(memory, cpu, 1u, 0x7cu, PALETTE_BLUE);
        OpSta(memory, cpu, OpDp(cpu, PALETTE_RESULT + 1u));
        OpRepWidths(cpu, 0x20u);
        SubtractPaletteComponent(memory, cpu, 0u, 0x3e0u, PALETTE_GREEN);
        OpOra(memory, cpu, OpDp(cpu, PALETTE_RESULT));
        OpSta(memory, cpu, OpLongX(cpu, PALETTE_RESULT_OFFSET));
        OpInx(cpu);
        OpInx(cpu);
        OpSepWidths(cpu, 0x20u);
        OpStepMem(memory, cpu, OpDp(cpu, PALETTE_COLORS_LEFT), -1);
    } while (!cpu->zero);
    return ExecutionReturned(0x8696bbu);
}

static void AddPaletteComponent(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                 uint8_t offset, uint16_t mask,
                                 uint8_t component, uint16_t overflow) {
    OpLda(memory, cpu, ScenePaletteColor(cpu, offset));
    OpAndValue(cpu, mask);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, component));
    OpBitValue(cpu, overflow);
    if (!cpu->zero)
        OpLoadA(cpu, mask);
}

Lufia2ExecutionResult Lufia2WorldMapBrightenScenePalette(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x869618u);
    PrepareScenePaletteOffset(memory, cpu);
    do {
        AddPaletteComponent(memory, cpu, 0u, 0x1fu, PALETTE_RED, 0xe0u);
        OpSta(memory, cpu, OpDp(cpu, PALETTE_RESULT));
        OpLda(memory, cpu, ScenePaletteColor(cpu, 1u));
        OpAndValue(cpu, 0x7cu);
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpDp(cpu, PALETTE_BLUE));
        if (cpu->negative)
            OpLoadA(cpu, 0x7cu);
        OpSta(memory, cpu, OpDp(cpu, PALETTE_RESULT + 1u));
        OpRepWidths(cpu, 0x20u);
        AddPaletteComponent(memory, cpu, 0u, 0x3e0u, PALETTE_GREEN, 0xfc00u);
        OpOra(memory, cpu, OpDp(cpu, PALETTE_RESULT));
        OpSta(memory, cpu, OpLongX(cpu, PALETTE_RESULT_OFFSET));
        OpInx(cpu);
        OpInx(cpu);
        OpSepWidths(cpu, 0x20u);
        OpStepMem(memory, cpu, OpDp(cpu, PALETTE_COLORS_LEFT), -1);
    } while (!cpu->zero);
    return ExecutionReturned(0x86966bu);
}
