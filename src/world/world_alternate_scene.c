#include "core/child_call.h"
#include "core/snes_registers.h"
#include "lufia2/world_map.h"
#include "system/wram.h"
#include "world/world_scene_resources_internal.h"

enum {
    SCENE_CANCELLED = WRAM_BATTLE_SPECIAL_ENCOUNTER_FLAGS,
    SCENE_FADE_STATUS = 0x0583u,
    SCENE_PALETTE_PHASE = 0x16feu,
    SCENE_PALETTE_COLOR = 0x16ffu,
    SCENE_PLANE_MODE = 0x11ddu,
    SCENE_VIEW_MODE = 0x11deu,
    SCENE_ACTOR_ACTIVE = 0x11d8u,
    SCENE_CAMERA_FRAME = 0x42u,
    SCENE_PALETTE_UPLOAD_PENDING = 0x16e7u,
    DP_PALETTE_COLOR = 0x15u,
    DP_PALETTE_COUNT = 0x26u,
    ROM_SCENE_FIRST_TRACK = 0xdc8fu,
    ROM_SCENE_FIRST_ORIGIN = 0xdc81u,
    ROM_SCENE_LAST_TRACK = 0xdd07u
};

typedef struct AlternateScene {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
} AlternateScene;

static bool AlternateSceneCall(AlternateScene *scene, uint32_t site, uint32_t target) {
    if (CallChildWithFrame(scene->memory, scene->cpu, scene->child, scene->context,
                          site, target, 2u, 0x86u))
        return true;
    scene->result = ExecutionReturned(site);
    scene->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return false;
}

#define ALTERNATE_CALL(site, target) \
    do { \
        if (!AlternateSceneCall(&scene, site, target)) \
            return scene.result; \
    } while (0)

Lufia2ExecutionResult Lufia2WorldMapCheckSceneCancellation(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!WorldSceneResourceEntry(cpu) || !child)
        return ExecutionHandoff(cpu, 0x86944eu);
    AlternateScene scene = {memory, cpu, child, context, {0}};
    OpLda(memory, cpu, OpAbs(cpu, SCENE_FADE_STATUS));
    if (cpu->negative) {
        cpu->carry = 1u;
        return ExecutionReturned(0x86946fu);
    }
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FADE_CONTROL));
    if (cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, 0x47u));
        OpBitValue(cpu, 0x90u);
        if (cpu->zero) {
            OpLda(memory, cpu, OpDp(cpu, 0x46u));
            OpBitValue(cpu, 0x80u);
        }
        if (!cpu->zero) {
            OpLoadA(cpu, 0xffu);
            OpSta(memory, cpu, OpAbs(cpu, SCENE_CANCELLED));
            ALTERNATE_CALL(0x869469u, 0x86d37du);
        }
    }
    cpu->carry = 0u;
    return ExecutionReturned(0x86946du);
}

static void SelectScenePaletteTransfer(const Lufia2Memory *memory,
                                       Lufia2CpuState *cpu, uint8_t mode) {
    OpLdx(cpu, 0xdc00u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_PALETTE_TRANSFER_SOURCE), cpu->x);
    OpLoadA(cpu, 0x7fu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_PALETTE_TRANSFER_BANK));
    OpLoadA(cpu, mode);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_PALETTE_TRANSFER_MODE));
    OpLdx(cpu, 0xe0u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_PALETTE_TRANSFER_LENGTH), cpu->x);
}

static void BuildSceneColorBand(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                uint16_t table, uint16_t marker) {
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0x3eu);
    OpLdy(cpu, 0x7cu);
    do {
        OpLoadA(cpu, marker);
        OpSta(memory, cpu, OpAbsY(cpu, table));
        OpLda(memory, cpu, OpAbsX(cpu, 0xdd20u));
        OpSta(memory, cpu, OpAbsY(cpu, (uint16_t)(table + 2u)));
        OpDex(cpu);
        OpDex(cpu);
        OpDey(cpu);
        OpDey(cpu);
        OpDey(cpu);
        OpDey(cpu);
    } while (!cpu->negative);
    OpSepWidths(cpu, 0x20u);
}

Lufia2ExecutionResult Lufia2WorldMapPrepareSceneColorBands(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!WorldSceneResourceEntry(cpu) || !child)
        return ExecutionHandoff(cpu, 0x869537u);
    AlternateScene scene = {memory, cpu, child, context, {0}};
    OpLdx(cpu, 0x420u);
    OpLoadA(cpu, 0x70u);
    OpSta(memory, cpu, OpDp(cpu, DP_PALETTE_COUNT));
    ALTERNATE_CALL(0x86953eu, 0x869618u);
    SelectScenePaletteTransfer(memory, cpu, 0x80u);
    PushDataBank(memory, cpu);
    OpLoadA(cpu, 0x7fu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpLoadA(cpu, 0x70u);
    OpSta(memory, cpu, OpAbs(cpu, 0xde00u));
    OpLdy(cpu, 0xde10u);
    OpWriteX(memory, cpu, OpAbs(cpu, 0xde01u), cpu->y);
    OpLoadA(cpu, 0xa0u);
    OpSta(memory, cpu, OpAbs(cpu, 0xde03u));
    OpSta(memory, cpu, OpAbs(cpu, 0xe003u));
    OpLdy(cpu, 0xde10u);
    OpWriteX(memory, cpu, OpAbs(cpu, 0xde04u), cpu->y);
    OpStz(memory, cpu, OpAbs(cpu, 0xde06u));
    OpStz(memory, cpu, OpAbs(cpu, 0xe006u));
    BuildSceneColorBand(memory, cpu, 0xde10u, 0xdcdcu);
    OpLoadA(cpu, 0x50u);
    OpSta(memory, cpu, OpAbs(cpu, 0xe000u));
    OpLdy(cpu, 0xe010u);
    OpWriteX(memory, cpu, OpAbs(cpu, 0xe001u), cpu->y);
    OpLdy(cpu, 0xe010u);
    OpWriteX(memory, cpu, OpAbs(cpu, 0xe004u), cpu->y);
    BuildSceneColorBand(memory, cpu, 0xe010u, 0xdfdfu);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x8695c9u);
}

Lufia2ExecutionResult Lufia2WorldMapCycleAlternateScenePalette(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!WorldSceneResourceEntry(cpu) || !child)
        return ExecutionHandoff(cpu, 0x8695cau);
    AlternateScene scene = {memory, cpu, child, context, {0}};
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, SCENE_PALETTE_COLOR)));
    OpWriteX(memory, cpu, OpDp(cpu, DP_PALETTE_COLOR), cpu->x);
    OpLdx(cpu, 0x340u);
    OpLoadA(cpu, 0x70u);
    OpSta(memory, cpu, OpDp(cpu, DP_PALETTE_COUNT));
    ALTERNATE_CALL(0x8695d6u, 0x86966cu);
    SelectScenePaletteTransfer(memory, cpu, 0x10u);
    OpLda(memory, cpu, OpAbs(cpu, SCENE_PALETTE_PHASE));
    OpLdy(cpu, 1u);
    OpAndValue(cpu, 3u);
    if (!cpu->zero) {
        OpLdy(cpu, 0x20u);
        OpDecA(cpu);
        if (!cpu->zero) {
            OpLdy(cpu, 0x400u);
            OpStepMem(memory, cpu, OpAbs(cpu, SCENE_PALETTE_PHASE), 1);
        }
    }
    OpStepMem(memory, cpu, OpAbs(cpu, SCENE_PALETTE_PHASE), 1);
    OpWriteX(memory, cpu, OpDp(cpu, 0u), cpu->y);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, SCENE_PALETTE_COLOR));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0u)));
    OpSta(memory, cpu, OpAbs(cpu, SCENE_PALETTE_COLOR));
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x869617u);
}

static void ConfigureAlternateSceneDisplay(const Lufia2Memory *memory,
                                          Lufia2CpuState *cpu) {
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_OBSEL));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_TM));
    OpLoadA(cpu, 0x10u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_TS));
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_CGWSEL));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_CGADSUB));
    OpLoadA(cpu, 0x7fu);
    static const uint16_t banks[] = {
        SNES_A1B(4), SNES_A1B(5), SNES_DASB(4), SNES_DASB(5)
    };
    for (unsigned field = 0u; field < sizeof(banks) / sizeof(banks[0]); ++field)
        OpSta(memory, cpu, OpAbs(cpu, banks[field]));
    OpLoadA(cpu, 0x21u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_BBAD(4)));
    OpSta(memory, cpu, OpAbs(cpu, SNES_BBAD(5)));
    OpStz(memory, cpu, OpAbs(cpu, SCENE_PALETTE_UPLOAD_PENDING));
}

Lufia2ExecutionResult Lufia2WorldMapRunAlternateScene(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->program_bank != 0x86u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page)
        return ExecutionHandoff(cpu, 0x8692dfu);
    AlternateScene scene = {memory, cpu, child, context, {0}};
    PushDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    OpLoadA(cpu, 0x86u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpStz(memory, cpu, OpAbs(cpu, 0x09eau));
    ALTERNATE_CALL(0x8692ebu, 0x86cdf5u);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_PLANE_MODE));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_VIEW_MODE));
    OpStz(memory, cpu, OpAbs(cpu, SCENE_CANCELLED));
    ALTERNATE_CALL(0x8692fbu, 0x86ccfcu);
    OpLdx(cpu, ROM_SCENE_FIRST_TRACK);
    OpLdy(cpu, ROM_SCENE_FIRST_ORIGIN);
    ALTERNATE_CALL(0x869304u, 0x869470u);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_RESOURCE_DIRTY));
    ALTERNATE_CALL(0x86930au, 0x8692a1u);
    ALTERNATE_CALL(0x86930du, 0x86ce68u);
    OpLoadA(cpu, 6u);
    ALTERNATE_CALL(0x869312u, 0x86d35du);
    ALTERNATE_CALL(0x869315u, 0x86e650u);
    ConfigureAlternateSceneDisplay(memory, cpu);
    ALTERNATE_CALL(0x86934au, 0x86973eu);
    ALTERNATE_CALL(0x86934du, 0x86974fu);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_ACTOR_ACTIVE));
    ALTERNATE_CALL(0x869355u, 0x86e9d4u);
    OpLoadA(cpu, 2u);
    ALTERNATE_CALL(0x86935au, 0x86d385u);
    OpStz(memory, cpu, OpAbs(cpu, SCENE_FADE_STATUS));
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpDp(cpu, SCENE_CAMERA_FRAME), cpu->x);
    bool cancelled = false;
    for (;;) {
        ALTERNATE_CALL(0x869365u, 0x86944eu);
        if (cpu->carry) {
            cancelled = true;
            break;
        }
        ALTERNATE_CALL(0x86936du, 0x8694d4u);
        if (cpu->carry)
            break;
        ALTERNATE_CALL(0x869372u, 0x86995bu);
        ALTERNATE_CALL(0x869375u, 0x8699bfu);
        ALTERNATE_CALL(0x869378u, 0x86a791u);
        ALTERNATE_CALL(0x86937bu, 0x86e8ceu);
        OpLda(memory, cpu, OpDp(cpu, SCENE_CAMERA_FRAME));
        OpAndValue(cpu, 0x7fu);
        if (cpu->zero)
            ALTERNATE_CALL(0x869384u, 0x8695cau);
        ALTERNATE_CALL(0x869387u, 0x86973eu);
        ALTERNATE_CALL(0x86938au, 0x86974fu);
        ALTERNATE_CALL(0x86938du, 0x8694d4u);
        if (cpu->carry)
            break;
        ALTERNATE_CALL(0x869392u, 0x86995bu);
        ALTERNATE_CALL(0x869395u, 0x86a7ceu);
        ALTERNATE_CALL(0x869398u, 0x86a791u);
        ALTERNATE_CALL(0x86939bu, 0x86e8ceu);
        ALTERNATE_CALL(0x86939eu, 0x86973eu);
        ALTERNATE_CALL(0x8693a1u, 0x86974fu);
    }
    if (!cancelled) {
        OpLoadA(cpu, 0x11u);
        OpSta(memory, cpu, OpAbs(cpu, SNES_TM));
        OpStz(memory, cpu, OpAbs(cpu, SNES_CGWSEL));
        OpStz(memory, cpu, OpAbs(cpu, SNES_CGADSUB));
        OpLdx(cpu, 0u);
        OpWriteX(memory, cpu, OpAbs(cpu, 0x1467u), cpu->x);
        OpLdx(cpu, 0x18u);
        OpWriteX(memory, cpu, OpDp(cpu, 0x58u), cpu->x);
        OpLdx(cpu, 0x38u);
        OpWriteX(memory, cpu, OpDp(cpu, 0x5au), cpu->x);
        OpLdx(cpu, 0xf32cu);
        ALTERNATE_CALL(0x8693c4u, 0x86ea43u);
        OpLdx(cpu, 0x7fffu);
        OpWriteX(memory, cpu, OpDp(cpu, DP_PALETTE_COLOR), cpu->x);
        do {
            ALTERNATE_CALL(0x8693ccu, 0x869537u);
            OpLoadA(cpu, 0xffu);
            OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E11DF));
            ALTERNATE_CALL(0x8693d4u, 0x86973eu);
            ALTERNATE_CALL(0x8693d7u, 0x86974fu);
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpDp(cpu, DP_PALETTE_COLOR));
            cpu->carry = 1u;
            OpSbcValue(cpu, 0x421u);
            OpSta(memory, cpu, OpDp(cpu, DP_PALETTE_COLOR));
            OpSepWidths(cpu, 0x20u);
        } while (!cpu->zero);
        do {
            ALTERNATE_CALL(0x8693e8u, 0x8695cau);
            Push8(memory, cpu, PackStatus(cpu));
            ALTERNATE_CALL(0x8693ecu, 0x86973eu);
            ALTERNATE_CALL(0x8693efu, 0x86974fu);
            ALTERNATE_CALL(0x8693f2u, 0x86974fu);
            ALTERNATE_CALL(0x8693f5u, 0x86974fu);
            UnpackStatus(cpu, Pull8(memory, cpu));
        } while (!cpu->negative);
        OpLdx(cpu, 0u);
        OpWriteX(memory, cpu, OpDp(cpu, SCENE_CAMERA_FRAME), cpu->x);
        OpWriteX(memory, cpu, OpAbs(cpu, 0x1210u), cpu->x);
        OpLdx(cpu, 0x100u);
        OpWriteX(memory, cpu, OpAbs(cpu, 0x1212u), cpu->x);
        OpLdx(cpu, ROM_SCENE_LAST_TRACK);
        OpLdy(cpu, 0u);
        ALTERNATE_CALL(0x86940fu, 0x869470u);
        ALTERNATE_CALL(0x869412u, 0x8694d4u);
        for (;;) {
            ALTERNATE_CALL(0x869415u, 0x86944eu);
            if (cpu->carry)
                break;
            ALTERNATE_CALL(0x86941du, 0x8694d4u);
            if (cpu->carry)
                break;
            ALTERNATE_CALL(0x869422u, 0x86995bu);
            ALTERNATE_CALL(0x869425u, 0x86a7ceu);
            ALTERNATE_CALL(0x869428u, 0x86a791u);
            ALTERNATE_CALL(0x86942bu, 0x86973eu);
            ALTERNATE_CALL(0x86942eu, 0x86974fu);
            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, SCENE_CAMERA_FRAME)));
            OpCpx(cpu, 0x5a0u);
            if (cpu->zero)
                ALTERNATE_CALL(0x869438u, 0x86d37du);
            OpLda(memory, cpu, OpAbs(cpu, SCENE_FADE_STATUS));
            if (cpu->negative)
                break;
        }
    }
    ALTERNATE_CALL(0x869440u, 0x86d397u);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E11DF));
    ALTERNATE_CALL(0x869446u, 0x86d32du);
    OpLsrMem(memory, cpu, OpAbs(cpu, SCENE_CANCELLED));
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x86944du);
}
