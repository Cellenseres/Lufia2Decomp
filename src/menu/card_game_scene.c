#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/card_game.h"
#include "system/wram.h"

enum {
    CARD_CHOICES = WRAM_CARD_GAME_OPENING_CHOICES & 0xffffu,
    CARD_DECK = WRAM_CARD_GAME_DECK & 0xffffu,
    CARD_HANDS = WRAM_CARD_GAME_HANDS & 0xffffu,
    CARD_COUNTS = WRAM_CARD_GAME_HAND_COUNTS & 0xffffu,
    CARD_CURSOR = WRAM_SPECIAL_MENU_CURSOR_SELECTIONS & 0xffffu,
    CARD_CURSOR_OAM = WRAM_SPECIAL_MENU_CURSOR_OAM & 0xffffu,
    CARD_STAKE = WRAM_CARD_GAME_STAKE & 0xffffu,
    CARD_SCENE_WORK_LIMIT = 65536u
};

typedef struct CardScene {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
    unsigned scans;
} CardScene;

static bool SceneReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x84u && !cpu->index_is_8_bit &&
           !cpu->direct_page && !cpu->decimal;
}

static bool SceneCall(CardScene *scene, uint32_t site, uint32_t target) {
    Lufia2CpuState *cpu = scene->cpu;
    const bool byte_accumulator = cpu->accumulator_is_8_bit != 0u;
    const uint8_t frame = (target >> 16) == 0x84u ? 2u : 3u;
    if (!CallChildWithFrame(scene->memory, cpu, scene->child, scene->context,
                            site, target, frame, 0x84u)) {
        scene->result = ExecutionReturned(site);
        scene->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (!SceneReady(cpu) || cpu->accumulator_is_8_bit != byte_accumulator) {
        scene->result = ExecutionHandoff(cpu, site + frame + 1u);
        return false;
    }
    return true;
}

static void ReadOpeningMotion(const Lufia2Memory *memory,
                               Lufia2CpuState *cpu, bool clear_carry) {
    OpTxa(cpu);
    if (clear_carry)
        cpu->carry = 0u;
    RolA16(cpu);
    OpTay(cpu);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, 8u));
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpIny(cpu);
    OpIny(cpu);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, 8u));
    OpSta(memory, cpu, OpDp(cpu, 4u));
}

static bool RevealOpeningChoice(CardScene *scene, uint16_t hand,
                                 uint32_t site) {
    const Lufia2Memory *memory = scene->memory;
    Lufia2CpuState *cpu = scene->cpu;
    OpLda(memory, cpu, OpDp(cpu, 0x22u));
    OpSta(memory, cpu, OpDp(cpu, 2u));
    OpLda(memory, cpu, OpAbs(cpu, (uint16_t)(CARD_HANDS + hand * 10u)));
    OpSta(memory, cpu, OpDp(cpu, 0x22u));
    return SceneCall(scene, site, 0x84b11eu);
}

static bool FindOpeningChoice(CardScene *scene, bool random_choice) {
    const Lufia2Memory *memory = scene->memory;
    Lufia2CpuState *cpu = scene->cpu;
    for (;;) {
        if (scene->scans++ == CARD_SCENE_WORK_LIMIT) {
            scene->result = ExecutionHandoff(cpu,
                random_choice ? 0x84b0a4u : 0x84b0e8u);
            return false;
        }
        OpInx(cpu);
        OpInx(cpu);
        if (random_choice) {
            OpLda(memory, cpu, OpAbsX(cpu, CARD_CHOICES));
            OpDecA(cpu);
        } else {
            OpStepMem(memory, cpu, OpAbsX(cpu, CARD_CHOICES), -1);
        }
        if (!cpu->negative) {
            if (!random_choice)
                return true;
            OpDey(cpu);
            if (cpu->negative)
                return true;
        }
    }
}

static void SelectOpeningSprite(const Lufia2Memory *memory,
                                 Lufia2CpuState *cpu) {
    OpTxa(cpu);
    cpu->carry = 0u;
    RorA16(cpu);
    OpIncA(cpu);
    OpSta(memory, cpu, OpDp(cpu, 2u));
    OpSta(memory, cpu, OpDp(cpu, 0x22u));
}

Lufia2ExecutionResult Lufia2CardGameSelectOpeningCards(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SceneReady(cpu) || cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x84b027u);
    CardScene scene = {memory, cpu, child, context,
                       ExecutionReturned(0x84b11du), 0u};
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, 0x180u);
    OpWriteX(memory, cpu, OpAbs(cpu, CARD_CURSOR_OAM), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, CARD_CURSOR_OAM + 2u), cpu->x);
    OpLoadA(cpu, 0x84u);
    OpSta(memory, cpu, OpDp(cpu, 0x0au));
    OpLdx(cpu, 0xb7e5u);
    OpWriteX(memory, cpu, OpDp(cpu, 8u), cpu->x);
    if (!SceneCall(&scene, 0x84b03bu, 0x84a000u) ||
        !SceneCall(&scene, 0x84b03eu, 0x84a02au))
        return scene.result;
    OpLoadA(cpu, 0u);
    if (!SceneCall(&scene, 0x84b043u, 0x84a112u) ||
        !SceneCall(&scene, 0x84b046u, 0x84a0dau))
        return scene.result;
    OpLoadA(cpu, 0x84u);
    OpSta(memory, cpu, OpDp(cpu, 0x0au));
    OpLdx(cpu, 0xb779u);
    OpWriteX(memory, cpu, OpDp(cpu, 8u), cpu->x);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 1u);
    for (unsigned choice = 0u; choice < 3u; ++choice)
        OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(CARD_CHOICES + choice * 2u)));
    OpLda(memory, cpu, OpAbs(cpu, CARD_CURSOR));
    OpIncA(cpu);
    OpSta(memory, cpu, OpDp(cpu, 2u));
    OpSta(memory, cpu, OpDp(cpu, 0x22u));
    OpDecA(cpu);
    cpu->carry = 0u;
    RolA16(cpu);
    OpTax(cpu);
    OpStepMem(memory, cpu, OpAbs(cpu, CARD_COUNTS), 1);
    OpLda(memory, cpu, OpAbsX(cpu, CARD_DECK));
    OpSta(memory, cpu, OpAbs(cpu, CARD_HANDS));
    OpStz(memory, cpu, OpAbsX(cpu, CARD_CHOICES));
    ReadOpeningMotion(memory, cpu, false);
    if (!SceneCall(&scene, 0x84b085u, 0x84b13du) ||
        !RevealOpeningChoice(&scene, 0u, 0x84b091u))
        return scene.result;
    OpLoadA(cpu, 2u);
    if (!SceneCall(&scene, 0x84b097u, 0x808299u))
        return scene.result;
    OpLdx(cpu, 0xb785u);
    OpWriteX(memory, cpu, OpDp(cpu, 8u), cpu->x);
    OpLdx(cpu, 0xfffeu);
    OpTay(cpu);
    if (!FindOpeningChoice(&scene, true))
        return scene.result;
    OpStepMem(memory, cpu, OpAbs(cpu, CARD_COUNTS + 2u), 1);
    OpSta(memory, cpu, OpAbsX(cpu, CARD_CHOICES));
    OpLda(memory, cpu, OpAbsX(cpu, CARD_DECK));
    OpSta(memory, cpu, OpAbs(cpu, CARD_HANDS + 10u));
    SelectOpeningSprite(memory, cpu);
    ReadOpeningMotion(memory, cpu, true);
    if (!SceneCall(&scene, 0x84b0d1u, 0x84b13du) ||
        !RevealOpeningChoice(&scene, 1u, 0x84b0ddu))
        return scene.result;
    OpLdx(cpu, 0xb791u);
    OpWriteX(memory, cpu, OpDp(cpu, 8u), cpu->x);
    OpLdx(cpu, 0xfffeu);
    if (!FindOpeningChoice(&scene, false))
        return scene.result;
    OpStepMem(memory, cpu, OpAbs(cpu, CARD_COUNTS + 4u), 1);
    OpLda(memory, cpu, OpAbsX(cpu, CARD_DECK));
    OpSta(memory, cpu, OpAbs(cpu, CARD_HANDS + 20u));
    SelectOpeningSprite(memory, cpu);
    ReadOpeningMotion(memory, cpu, true);
    if (!SceneCall(&scene, 0x84b10eu, 0x84b13du) ||
        !RevealOpeningChoice(&scene, 2u, 0x84b11au))
        return scene.result;
    return ExecutionReturned(0x84b11du);
}

Lufia2ExecutionResult Lufia2CardGameLoadSceneTilemap(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SceneReady(cpu))
        return ExecutionHandoff(cpu, 0x84b523u);
    CardScene scene = {memory, cpu, child, context,
                       ExecutionReturned(0x84b54eu), 0u};
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, 0x29eu);
    OpWriteX(memory, cpu, OpDp(cpu, 0x54u), cpu->x);
    OpLdx(cpu, 0x4000u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x60u), cpu->x);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, 0x62u));
    if (!SceneCall(&scene, 0x84b534u, 0x808e9du))
        return scene.result;
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0u);
    OpLdy(cpu, 0x400u);
    do {
        OpLda(memory, cpu, OpLongX(cpu, 0x7e4000u));
        OpSta(memory, cpu, OpLongX(cpu, 0x7e2800u));
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x84b54eu);
}

Lufia2ExecutionResult Lufia2CardGameDrawSceneLabels(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SceneReady(cpu) || cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x84b54fu);
    CardScene scene = {memory, cpu, child, context,
                       ExecutionReturned(0x84b57eu), 0u};
    OpLdx(cpu, 0x31au);
    OpLoadA(cpu, 0x246au);
    OpLdy(cpu, 6u);
    do {
        OpSta(memory, cpu, OpAbsX(cpu, 0x2000u));
        OpIncA(cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    OpLdx(cpu, 0x35au);
    OpLdy(cpu, 6u);
    OpLoadA(cpu, 0x247au);
    do {
        OpSta(memory, cpu, OpAbsX(cpu, 0x2000u));
        OpIncA(cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, 0x74u));
    if (!SceneCall(&scene, 0x84b579u, 0x84a5a8u))
        return scene.result;
    OpRepWidths(cpu, 0x20u);
    return ExecutionReturned(0x84b57eu);
}

Lufia2ExecutionResult Lufia2CardGameDrawScoreFrame(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SceneReady(cpu) || cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x84adc0u);
    CardScene scene = {memory, cpu, child, context,
                       ExecutionReturned(0x84adf2u), 0u};
    OpLoadA(cpu, 0x4c8u);
    OpLdx(cpu, 0x5c6u);
    OpLdy(cpu, 4u);
    OpWriteX(memory, cpu, OpDp(cpu, 0u), cpu->y);
    do {
        OpLdy(cpu, 6u);
        do {
            OpSta(memory, cpu, OpAbsX(cpu, 0x2000u));
            OpInx(cpu);
            OpInx(cpu);
            OpIncA(cpu);
            OpDey(cpu);
        } while (!cpu->zero);
        OpTay(cpu);
        OpTxa(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, 52u);
        OpTax(cpu);
        OpTya(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, 10u);
        OpStepMem(memory, cpu, OpDp(cpu, 0u), -1);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, 0x74u));
    if (!SceneCall(&scene, 0x84adedu, 0x84a5a8u))
        return scene.result;
    OpRepWidths(cpu, 0x20u);
    return ExecutionReturned(0x84adf2u);
}

Lufia2ExecutionResult Lufia2CardGameDrawHandScore(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SceneReady(cpu) || cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x84adf3u);
    CardScene scene = {memory, cpu, child, context,
                       ExecutionReturned(0x84ae2fu), 0u};
    if (!SceneCall(&scene, 0x84adf3u, 0x84ae5eu))
        return scene.result;
    OpLda(memory, cpu, OpDp(cpu, 0u));
    if (!SceneCall(&scene, 0x84adf8u, 0x84ae43u))
        return scene.result;
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x960u);
    OpLdx(cpu, 0x60au);
    if (!SceneCall(&scene, 0x84ae02u, 0x84ae30u))
        return scene.result;
    OpLda(memory, cpu, OpDp(cpu, 0u));
    OpTay(cpu);
    if (!SceneCall(&scene, 0x84ae08u, 0x84ae43u))
        return scene.result;
    cpu->carry = 0u;
    RolA16(cpu);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    RolA16(cpu);
    RolA16(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, 0u));
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpTya(cpu);
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0u)));
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x960u);
    OpLdx(cpu, 0x60cu);
    if (!SceneCall(&scene, 0x84ae21u, 0x84ae30u))
        return scene.result;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, 0x74u));
    if (!SceneCall(&scene, 0x84ae2au, 0x84a5a8u))
        return scene.result;
    OpRepWidths(cpu, 0x20u);
    return ExecutionReturned(0x84ae2fu);
}

Lufia2ExecutionResult Lufia2CardGameDrawScoreDigit(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!SceneReady(cpu) || cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x84ae30u);
    OpSta(memory, cpu, OpAbsX(cpu, 0x2000u));
    OpTay(cpu);
    OpTxa(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, 64u);
    OpTax(cpu);
    OpTya(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, 16u);
    OpSta(memory, cpu, OpAbsX(cpu, 0x2000u));
    return ExecutionReturned(0x84ae42u);
}

static bool DrawSceneBalance(CardScene *scene) {
    OpLdx(scene->cpu, 0x3656u);
    OpLdy(scene->cpu, 0xf945u);
    return SceneCall(scene, 0x84af17u, 0x808878u);
}

Lufia2ExecutionResult Lufia2CardGameDrawRoundValues(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SceneReady(cpu) || !cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x84aee1u);
    CardScene scene = {memory, cpu, child, context,
                       ExecutionReturned(0x84af1bu), 0u};
    OpLoadA(cpu, 99u);
    if (!SceneCall(&scene, 0x84aee3u, 0x808299u))
        return scene.result;
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLdx(cpu, 0x3386u);
    OpLdy(cpu, 0xf922u);
    if (!SceneCall(&scene, 0x84aeefu, 0x808878u))
        return scene.result;
    OpLoadA(cpu, 99u);
    if (!SceneCall(&scene, 0x84aef5u, 0x808299u))
        return scene.result;
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLdx(cpu, 0x33aeu);
    if (!SceneCall(&scene, 0x84aefeu, 0x808878u))
        return scene.result;
    OpLda(memory, cpu, OpAbs(cpu, CARD_STAKE));
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLdx(cpu, 0x3574u);
    OpLdy(cpu, 0xf932u);
    if (!SceneCall(&scene, 0x84af0du, 0x808878u) ||
        !DrawSceneBalance(&scene))
        return scene.result;
    return ExecutionReturned(0x84af1bu);
}

Lufia2ExecutionResult Lufia2CardGameDrawBalance(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SceneReady(cpu) || !cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x84af11u);
    CardScene scene = {memory, cpu, child, context,
                       ExecutionReturned(0x84af1bu), 0u};
    if (!DrawSceneBalance(&scene))
        return scene.result;
    return ExecutionReturned(0x84af1bu);
}
