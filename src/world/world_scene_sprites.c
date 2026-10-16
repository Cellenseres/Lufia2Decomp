#include "core/child_call.h"
#include "lufia2/world_map.h"
#include "system/wram.h"
#include "world/world_scene_internal.h"
#include "world/world_scene_resources_internal.h"

enum {
    SCENE_SPRITE_INDEX = WRAM_WORLD_SCENE_SPRITE_INDEX,
    SCENE_SPRITE_RECORDS = WRAM_WORLD_SCENE_SPRITE_RECORD_STORAGE,
    SCENE_SPRITE_RECORD_SIZE = 6u,
    SCENE_SPRITE_RECORD_COUNT = 8u,
    SCENE_SPRITE_GROUP_SIZE = 16u,
    SCENE_SPRITES_LEFT = 4u,
    SCENE_SPRITE_HIGH_BITS = 5u,
    SCENE_HIGH_BITS_DISPATCH = 0xe5dau,
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
    OpLda(memory, cpu, OpAbs(cpu, SCENE_SPRITE_INDEX));
    OpAndValue(cpu, 3u);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbs(cpu, SCENE_SPRITE_INDEX));
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpTay(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsY(cpu, WRAM_FIELD_OAM_HIGH_BUFFER));
    const uint16_t pointer = (uint16_t)(SCENE_HIGH_BITS_DISPATCH + cpu->x);
    const uint16_t target =
        Read16Pair(memory, 0x860000u | pointer, 0x860000u | (uint16_t)(pointer + 1u));
    if (!CallSpriteSetupChild(setup, 0x86e87cu, 0x860000u | target))
        return false;
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_FIELD_OAM_HIGH_BUFFER));
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
