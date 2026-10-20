#include "core/child_call.h"
#include "lufia2/world_map.h"
#include "system/wram.h"
#include "world/world_scene_internal.h"
#include "world/world_scene_oam_internal.h"
#include "world/world_scene_resources_internal.h"

enum {
    SCENE_SPRITE_INDEX = WRAM_WORLD_SCENE_SPRITE_INDEX,
    SCENE_SPRITE_RECORDS = WRAM_WORLD_SCENE_SPRITE_RECORD_STORAGE,
    SCENE_SPRITE_RECORD_SIZE = 6u,
    SCENE_SPRITE_RECORD_COUNT = 8u,
    SCENE_SPRITE_GROUP_SIZE = 16u,
    SCENE_SPRITES_LEFT = 4u,
    SCENE_SPRITE_HIGH_BITS = 5u,
    SCENE_WINDOW_PARAMETER_1231 = WRAM_FIELD_STREAMED_ROW_SOURCE + 3u,
    SCENE_WINDOW_PARAMETER_1234 = WRAM_FIELD_STREAMED_COLUMN_SOURCE - 2u,
    SCENE_WINDOW_PARAMETER_1237 = WRAM_FIELD_STREAMED_COLUMN_SOURCE + 1u,
    SCENE_WINDOW_PARAMETER_1238 = WRAM_FIELD_STREAMED_COLUMN_SOURCE + 2u
};

Lufia2ExecutionResult Lufia2WorldMapClearSceneSpriteRecords(const Lufia2Memory *memory,
                                                            Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x86u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal)
        return ExecutionHandoff(cpu, 0x86e8b5u);
    OpLdy(cpu, SCENE_SPRITE_RECORD_COUNT);
    cpu->carry = 0u;
    do {
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, WorldSceneObjectField(cpu, 5u));
        OpRepWidths(cpu, 0x20u);
        OpStz(memory, cpu, WorldSceneObjectField(cpu, 0u));
        OpStz(memory, cpu, WorldSceneObjectField(cpu, 2u));
        OpTxa(cpu);
        OpAdcValue(cpu, SCENE_SPRITE_RECORD_SIZE);
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
        OpDey(cpu);
    } while (!cpu->zero);
    return ExecutionReturned(0x86e8cdu);
}

typedef struct SceneSpriteSetup {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
} SceneSpriteSetup;

static bool CallSpriteSetupChild(SceneSpriteSetup *setup, uint32_t site,
                                 uint32_t target) {
    if (!CallChildWithFrame(setup->memory, setup->cpu, setup->child, setup->context,
                            site, target, 2u, 0x86u)) {
        setup->result = ExecutionReturned(site);
        setup->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (!WorldSceneResourceEntry(setup->cpu)) {
        setup->result = ExecutionHandoff(setup->cpu, site + 3u);
        return false;
    }
    return true;
}

static bool PrepareSceneSprite(SceneSpriteSetup *setup) {
    const Lufia2Memory *memory = setup->memory;
    Lufia2CpuState *cpu = setup->cpu;
    OpStz(memory, cpu, OpDp(cpu, SCENE_SPRITE_HIGH_BITS));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, SCENE_SPRITE_INDEX));
    OpAslA(cpu);
    OpAslA(cpu);
    OpTay(cpu);
    OpLoadA(cpu, 0x3e00u);
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_FIELD_OAM_LOW_BUFFER + 2u));
    if (!WorldSceneStoreSpriteHighBits(memory, cpu, setup->child, setup->context,
                                       0x86e87cu, &setup->result))
        return false;
    OpStepMem(memory, cpu, OpAbs(cpu, SCENE_SPRITE_INDEX), 1);
    OpStepMem(memory, cpu, OpDp(cpu, SCENE_SPRITES_LEFT), -1);
    return true;
}

static bool PrepareSceneSpriteGroups(SceneSpriteSetup *setup) {
    const Lufia2Memory *memory = setup->memory;
    Lufia2CpuState *cpu = setup->cpu;
    OpLdx(cpu, SCENE_SPRITE_RECORDS);
    if (!CallSpriteSetupChild(setup, 0x86e88cu, 0x86e8b5u))
        return false;
    OpLdx(cpu,
          SCENE_SPRITE_RECORDS + SCENE_SPRITE_RECORD_COUNT * SCENE_SPRITE_RECORD_SIZE);
    if (!CallSpriteSetupChild(setup, 0x86e892u, 0x86e8b5u))
        return false;
    OpLdy(cpu, 0x80u);
    OpWrite16(memory, OpAbs(cpu, SCENE_WINDOW_PARAMETER_1231), cpu->y);
    OpLdy(cpu, 0x70u);
    OpWrite16(memory, OpAbs(cpu, SCENE_WINDOW_PARAMETER_1234), cpu->y);
    OpLdy(cpu, 0xc00u);
    OpWrite16(memory, OpAbs(cpu, SCENE_WINDOW_PARAMETER_1238), cpu->y);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E1211));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, WRAM_WORLD_SCENE_UNK_1219)));
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x40u);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_WINDOW_PARAMETER_1237));
    return true;
}

Lufia2ExecutionResult Lufia2WorldMapPrepareSceneSprites(const Lufia2Memory *memory,
                                                        Lufia2CpuState *cpu,
                                                        Lufia2PushedChildCall child,
                                                        void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86e84fu);
    SceneSpriteSetup setup = {memory, cpu, child, context,
                              ExecutionReturned(0x86e8b4u)};
    OpLdx(cpu, 0u);
    OpWrite16(memory, OpAbs(cpu, SCENE_SPRITE_INDEX), cpu->x);
    OpLoadA(cpu, SCENE_SPRITE_GROUP_SIZE);
    OpSta(memory, cpu, OpDp(cpu, SCENE_SPRITES_LEFT));
    for (unsigned step = 0u; step < 65536u; ++step) {
        if (!PrepareSceneSprite(&setup))
            return setup.result;
        if (cpu->zero) {
            PrepareSceneSpriteGroups(&setup);
            return setup.result;
        }
    }
    return ExecutionHandoff(cpu, 0x86e859u);
}

enum {
    TRAIL_POSITION_X = 0u,
    TRAIL_POSITION_Y = 2u,
    TRAIL_AGE = 4u,
    TRAIL_TILE = 5u,
    TRAIL_RECORD_SIZE = 6u,
    TRAIL_LENGTH = 7u,
    TRAIL_SAVED_X = 4u,
    TRAIL_SAVED_Y = 6u,
    TRAIL_STEP_X = 8u,
    TRAIL_STEP_Y = 0x0bu,
    TRAIL_RECORDS = WRAM_WORLD_SCENE_SPRITE_RECORD_STORAGE,
    TRAIL_SECOND_GROUP = TRAIL_RECORDS + 8u * TRAIL_RECORD_SIZE,
    TRAIL_FRAME_COUNTER = WRAM_FIELD_LAYER_SCROLL_Y,
    TRAIL_ANGLE = WRAM_FIELD_STREAMED_COLUMN_SOURCE + 1u,
    TRAIL_DISTANCE = WRAM_FIELD_STREAMED_COLUMN_SOURCE + 2u,
    TRAIL_ORIGIN_X = WRAM_FIELD_STREAMED_ROW_SOURCE + 3u,
    TRAIL_ORIGIN_Y = WRAM_FIELD_STREAMED_COLUMN_SOURCE - 2u,
    TRAIL_MOTION_ANGLE = 0x1248u,
    TRAIL_MOTION_DISTANCE = 0x1249u
};

static void MoveTrailRecord(Lufia2CpuState *cpu, bool forward) {
    OpRepWidths(cpu, 0x20u);
    OpTxa(cpu);
    cpu->carry = forward ? 0u : 1u;
    if (forward)
        OpAdcValue(cpu, TRAIL_RECORD_SIZE);
    else
        OpSbcValue(cpu, TRAIL_RECORD_SIZE);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
}

Lufia2ExecutionResult Lufia2WorldMapSpawnSceneSpriteTrail(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu) {
    if (!WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86e7a2u);
    OpLdy(cpu, OpRead16(memory, WorldSceneObjectField(cpu, TRAIL_POSITION_X)));
    OpWrite16(memory, OpDp(cpu, TRAIL_SAVED_X), cpu->y);
    OpLdy(cpu, OpRead16(memory, WorldSceneObjectField(cpu, TRAIL_POSITION_Y)));
    OpWrite16(memory, OpDp(cpu, TRAIL_SAVED_Y), cpu->y);
    OpLdy(cpu, TRAIL_LENGTH);
    do {
        MoveTrailRecord(cpu, true);
        OpLda(memory, cpu, WorldSceneObjectField(cpu, TRAIL_TILE));
        if (cpu->negative) {
            OpLoadA(cpu, 0x20u);
            OpSta(memory, cpu, WorldSceneObjectField(cpu, TRAIL_TILE));
            OpStz(memory, cpu, WorldSceneObjectField(cpu, TRAIL_AGE));
            OpLdy(cpu, OpRead16(memory, OpDp(cpu, TRAIL_SAVED_X)));
            OpWrite16(memory, WorldSceneObjectField(cpu, TRAIL_POSITION_X), cpu->y);
            OpLdy(cpu, OpRead16(memory, OpDp(cpu, TRAIL_SAVED_Y)));
            OpWrite16(memory, WorldSceneObjectField(cpu, TRAIL_POSITION_Y), cpu->y);
            return ExecutionReturned(0x86e7cdu);
        }
        OpDey(cpu);
    } while (!cpu->zero);
    return ExecutionReturned(0x86e7beu);
}

Lufia2ExecutionResult Lufia2WorldMapAdvanceSceneSpriteTrail(const Lufia2Memory *memory,
                                                            Lufia2CpuState *cpu) {
    if (!WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86e7ceu);
    OpLda(memory, cpu, OpAbs(cpu, TRAIL_FRAME_COUNTER));
    OpLsrA(cpu);
    OpAndValue(cpu, 3u);
    OpAslA(cpu);
    OpSta(memory, cpu, WorldSceneObjectField(cpu, TRAIL_TILE));
    OpLdy(cpu, TRAIL_LENGTH);
    do {
        MoveTrailRecord(cpu, true);
        OpLda(memory, cpu, WorldSceneObjectField(cpu, TRAIL_TILE));
        if (!cpu->negative) {
            OpLda(memory, cpu, WorldSceneObjectField(cpu, TRAIL_AGE));
            OpCmpValue(cpu, 6u);
            if (cpu->carry) {
                OpStz(memory, cpu, WorldSceneObjectField(cpu, TRAIL_AGE));
                OpLda(memory, cpu, WorldSceneObjectField(cpu, TRAIL_TILE));
                OpIncA(cpu);
                OpIncA(cpu);
                OpSta(memory, cpu, WorldSceneObjectField(cpu, TRAIL_TILE));
                OpCmpValue(cpu, 0x2cu);
                if (cpu->carry) {
                    OpLoadA(cpu, 0xffu);
                    OpSta(memory, cpu, WorldSceneObjectField(cpu, TRAIL_TILE));
                }
            }
            OpStepMem(memory, cpu, WorldSceneObjectField(cpu, TRAIL_AGE), 1);
        }
        OpDey(cpu);
    } while (!cpu->zero);
    return ExecutionReturned(0x86e803u);
}

static void PreviousTrailSprite(Lufia2CpuState *cpu) {
    for (unsigned byte = 0u; byte < 4u; ++byte)
        OpDey(cpu);
}

Lufia2ExecutionResult Lufia2WorldMapDrawSceneSpriteTrail(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu) {
    if (!WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86e804u);
    OpLoadA(cpu, TRAIL_LENGTH);
    OpSta(memory, cpu, OpDp(cpu, TRAIL_AGE));
    do {
        OpLda(memory, cpu, WorldSceneObjectField(cpu, TRAIL_TILE));
        if (!cpu->negative) {
            OpSta(memory, cpu, OpAbsY(cpu, WRAM_FIELD_OAM_LOW_BUFFER + 2u));
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, WorldSceneObjectField(cpu, TRAIL_POSITION_X));
            cpu->carry = 0u;
            OpAdc(memory, cpu, OpDp(cpu, TRAIL_STEP_X));
            OpSta(memory, cpu, WorldSceneObjectField(cpu, TRAIL_POSITION_X));
            ExchangeAccumulatorBytes(cpu);
            OpSta(memory, cpu, OpAbsY(cpu, WRAM_FIELD_OAM_LOW_BUFFER));
            OpLda(memory, cpu, WorldSceneObjectField(cpu, TRAIL_POSITION_Y));
            cpu->carry = 0u;
            OpAdc(memory, cpu, OpDp(cpu, TRAIL_STEP_Y));
            OpSta(memory, cpu, WorldSceneObjectField(cpu, TRAIL_POSITION_Y));
            ExchangeAccumulatorBytes(cpu);
            OpSepWidths(cpu, 0x20u);
            OpSta(memory, cpu, OpAbsY(cpu, WRAM_FIELD_OAM_LOW_BUFFER + 1u));
            PreviousTrailSprite(cpu);
        }
        MoveTrailRecord(cpu, false);
        OpStepMem(memory, cpu, OpDp(cpu, TRAIL_AGE), -1);
    } while (!cpu->zero);
    static const uint8_t fields[] = {1u, 3u, TRAIL_TILE};
    for (unsigned field = 0u; field < 3u; ++field) {
        OpLda(memory, cpu, WorldSceneObjectField(cpu, fields[field]));
        OpSta(memory, cpu, OpAbsY(cpu, (uint16_t)(WRAM_FIELD_OAM_LOW_BUFFER + field)));
    }
    PreviousTrailSprite(cpu);
    return ExecutionReturned(0x86e84eu);
}

static void PlaceSceneTrailOrigin(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                  uint16_t record, bool opposite) {
    static const uint16_t origins[] = {TRAIL_ORIGIN_X, TRAIL_ORIGIN_Y};
    static const uint8_t steps[] = {TRAIL_STEP_X + 1u, TRAIL_STEP_Y + 1u};
    for (unsigned axis = 0u; axis < 2u; ++axis) {
        OpLda(memory, cpu, OpAbs(cpu, origins[axis]));
        cpu->carry = opposite ? 1u : 0u;
        if (opposite)
            OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, steps[axis])));
        else
            OpAdc(memory, cpu, OpDp(cpu, steps[axis]));
        OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(record + axis * 2u + 1u)));
    }
}

Lufia2ExecutionResult
Lufia2WorldMapAnimateSceneSpriteTrails(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                       Lufia2PushedChildCall child, void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86e71eu);
    SceneSpriteSetup setup = {memory, cpu, child, context,
                              ExecutionReturned(0x86e7a1u)};
    if (!CallSpriteSetupChild(&setup, 0x86e71eu, 0x86e650u))
        return setup.result;
    OpLda(memory, cpu, OpAbs(cpu, TRAIL_ANGLE));
    OpSta(memory, cpu, OpAbs(cpu, TRAIL_MOTION_ANGLE));
    OpLdy(cpu, OpRead16(memory, OpAbs(cpu, TRAIL_DISTANCE)));
    OpWrite16(memory, OpAbs(cpu, TRAIL_MOTION_DISTANCE), cpu->y);
    if (!CallSpriteSetupChild(&setup, 0x86e72du, 0x86a417u))
        return setup.result;
    PlaceSceneTrailOrigin(memory, cpu, TRAIL_RECORDS, false);
    PlaceSceneTrailOrigin(memory, cpu, TRAIL_SECOND_GROUP, true);
    OpLda(memory, cpu, OpAbs(cpu, TRAIL_FRAME_COUNTER));
    OpAndValue(cpu, 7u);
    if (cpu->zero) {
        OpLdx(cpu, TRAIL_RECORDS);
        if (!CallSpriteSetupChild(&setup, 0x86e75eu, 0x86e7a2u))
            return setup.result;
        OpLdx(cpu, TRAIL_SECOND_GROUP);
        if (!CallSpriteSetupChild(&setup, 0x86e764u, 0x86e7a2u))
            return setup.result;
    }
    OpLdx(cpu, TRAIL_RECORDS);
    if (!CallSpriteSetupChild(&setup, 0x86e76au, 0x86e7ceu))
        return setup.result;
    OpLdx(cpu, TRAIL_SECOND_GROUP);
    if (!CallSpriteSetupChild(&setup, 0x86e770u, 0x86e7ceu))
        return setup.result;
    OpLda(memory, cpu, OpAbs(cpu, TRAIL_SECOND_GROUP + TRAIL_TILE));
    cpu->carry = 0u;
    OpAdcValue(cpu, 8u);
    OpSta(memory, cpu, OpAbs(cpu, TRAIL_SECOND_GROUP + TRAIL_TILE));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E1211));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, WRAM_WORLD_SCENE_UNK_1219)));
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, TRAIL_MOTION_ANGLE));
    OpLdy(cpu, 0x180u);
    OpWrite16(memory, OpAbs(cpu, TRAIL_MOTION_DISTANCE), cpu->y);
    if (!CallSpriteSetupChild(&setup, 0x86e78fu, 0x86a417u))
        return setup.result;
    OpLdy(cpu, 0x3cu);
    OpLdx(cpu, TRAIL_RECORDS + TRAIL_LENGTH * TRAIL_RECORD_SIZE);
    if (!CallSpriteSetupChild(&setup, 0x86e798u, 0x86e804u))
        return setup.result;
    OpLdx(cpu, TRAIL_SECOND_GROUP + TRAIL_LENGTH * TRAIL_RECORD_SIZE);
    if (!CallSpriteSetupChild(&setup, 0x86e79eu, 0x86e804u))
        return setup.result;
    return setup.result;
}
