#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "lufia2/text.h"
#include "system/wram.h"
#include "text/text_internal.h"

enum {
    SCENE_LABEL_NUMBER = WRAM_MENU_NUMBER,
    SCENE_LABEL_DIGITS = WRAM_MENU_NUMBER_DIGITS,
    SCENE_LABEL_WIDTH = WRAM_SCENE_LABEL_WIDTH,
    SCENE_LABEL_ROW_POSITION = WRAM_SCENE_LABEL_ROW_POSITION,
    SCENE_LABEL_REFERENCE = WRAM_UNK_7FD0C4,
    SCENE_LABEL_PREVIOUS = 0x7e0000u | WRAM_ANCIENT_CAVE_CARRY_COUNT,
    SCENE_LABEL_CURRENT = 0x7e0000u | WRAM_SCENE_LABEL_CURRENT,
    SCENE_LABEL_BATTLE_BUFFER = 0x7e0000u | WRAM_SCENE_LABEL_BATTLE_BUFFER,
    SCENE_LABEL_MAP_TABLE = 0x878016u,
    SCENE_LABEL_DICTIONARY_BASE = 0x8000u,
    SCENE_LABEL_BATTLE_PREFIX = 0x858019u,
    SCENE_LABEL_GLYPH_HIGH = 0x0009b0u,
    SCENE_LABEL_GLYPH_LOW = 0x0009afu,
    SCENE_LABEL_UPLOAD_REQUEST = 0x75u,
    SCENE_LABEL_LAYER_REQUEST = 0x74u,
    SCENE_LABEL_VRAM_TARGET = 0x79u
};

typedef struct SceneLabelContext {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *child_context;
    uint32_t interrupted_site;
} SceneLabelContext;

static uint8_t SceneLabelCall(SceneLabelContext *label, uint32_t site,
    uint32_t target, uint8_t frame) {
    if (CallChildWithFrame(label->memory, label->cpu, label->child,
            label->child_context, site, target, frame, 0x80u))
        return 1u;
    label->interrupted_site = site;
    return 0u;
}

static Lufia2ExecutionResult SceneLabelUnwound(const SceneLabelContext *label) {
    Lufia2ExecutionResult result = ExecutionReturned(label->interrupted_site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static uint8_t SceneLabelPrepareBattle(SceneLabelContext *label) {
    const Lufia2Memory *memory = label->memory;
    Lufia2CpuState *cpu = label->cpu;
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, 0u);
    for (;;) {
        OpLda(memory, cpu, OpLongX(cpu, SCENE_LABEL_BATTLE_PREFIX));
        if (cpu->zero)
            break;
        OpSta(memory, cpu, OpLongX(cpu, SCENE_LABEL_BATTLE_BUFFER));
        OpInx(cpu);
    }
    OpStz(memory, cpu, OpAbs(cpu, SCENE_LABEL_NUMBER + 2u));
    OpStz(memory, cpu, OpAbs(cpu, SCENE_LABEL_NUMBER + 1u));
    OpLda(memory, cpu, WRAM_UNK_7FE696);
    OpDecA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_LABEL_NUMBER));
    OpPushX(memory, cpu);
    if (!SceneLabelCall(label, 0x80c851u, 0x8089aau, 2u))
        return 0u;
    OpPullX(memory, cpu);
    OpLdy(cpu, 0u);
    do {
        OpLda(memory, cpu, OpAbsY(cpu, SCENE_LABEL_DIGITS));
        OpIny(cpu);
        OpCmpValue(cpu, 0x30u);
    } while (cpu->zero);
    for (;;) {
        OpLda(memory, cpu, OpAbsY(cpu, SCENE_LABEL_DIGITS - 1u));
        OpSta(memory, cpu, OpLongX(cpu, SCENE_LABEL_BATTLE_BUFFER));
        if (cpu->zero)
            break;
        OpInx(cpu);
        OpIny(cpu);
    }
    OpLdx(cpu, SCENE_LABEL_BATTLE_BUFFER & 0xffffu);
    OpWriteX(memory, cpu, OpAbs(cpu, TEXT_SCRIPT_POINTER), cpu->x);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpAbs(cpu, TEXT_SCRIPT_BANK));
    return 1u;
}

static uint8_t SceneLabelPrepareField(SceneLabelContext *label,
    uint8_t *changed) {
    const Lufia2Memory *memory = label->memory;
    Lufia2CpuState *cpu = label->cpu;
    TransferDirectToA(cpu);
    OpSta(memory, cpu, SCENE_LABEL_PREVIOUS);
    OpLda(memory, cpu, SCENE_LABEL_REFERENCE);
    if (!cpu->zero) {
        OpTax(cpu);
        OpLdy(cpu, SCENE_LABEL_PREVIOUS & 0xffffu);
        if (!SceneLabelCall(label, 0x80c889u, 0x80c9c0u, 3u))
            return 0u;
    }
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID));
    for (unsigned bit = 0; bit < 3u; ++bit)
        OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, SCENE_LABEL_MAP_TABLE));
    OpSta(memory, cpu, SCENE_LABEL_REFERENCE);
    OpTax(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, SCENE_LABEL_DICTIONARY_BASE);
    OpSta(memory, cpu, OpAbs(cpu, TEXT_SCRIPT_POINTER));
    OpLdy(cpu, SCENE_LABEL_CURRENT & 0xffffu);
    if (!SceneLabelCall(label, 0x80c8a7u, 0x80c9c0u, 3u))
        return 0u;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x87u);
    OpSta(memory, cpu, OpAbs(cpu, TEXT_SCRIPT_BANK));
    TransferDirectToA(cpu);
    OpSta(memory, cpu, WRAM_FIELD_CONTROL_CHANGE_PENDING);
    OpLdx(cpu, 0u);
    for (;;) {
        OpLda(memory, cpu, OpLongX(cpu, SCENE_LABEL_CURRENT));
        if (cpu->zero) {
            *changed = 0u;
            return 1u;
        }
        OpCmp(memory, cpu, OpLongX(cpu, SCENE_LABEL_PREVIOUS));
        if (!cpu->zero) {
            *changed = 1u;
            return 1u;
        }
        OpInx(cpu);
    }
}

Lufia2ExecutionResult Lufia2TextUpdateSceneLabel(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x80u || cpu->direct_page)
        return ExecutionHandoff(cpu, 0x80c825u);
    SceneLabelContext label = {memory, cpu, child, context, 0u};
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_FLAGS));
    OpBitValue(cpu, 1u);
    uint8_t changed = 1u;
    if (!cpu->zero) {
        if (!SceneLabelPrepareBattle(&label))
            return SceneLabelUnwound(&label);
    } else if (!SceneLabelPrepareField(&label, &changed)) {
        return SceneLabelUnwound(&label);
    }
    if (changed) {
        OpLdx(cpu, 0x40u);
        OpWriteX(memory, cpu, OpAbs(cpu, SCENE_LABEL_ROW_POSITION), cpu->x);
        if (!SceneLabelCall(&label, 0x80c8cfu, 0x80c8d5u, 3u))
            return SceneLabelUnwound(&label);
    }
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x80c8d4u);
}

static uint8_t SceneLabelDrawGlyph(SceneLabelContext *label, uint32_t site) {
    PushY(label->memory, label->cpu);
    if (!SceneLabelCall(label, site, 0x80c7c2u, 3u))
        return 0u;
    OpPullY(label->memory, label->cpu);
    return 1u;
}

static uint8_t SceneLabelDrawReference(SceneLabelContext *label) {
    const Lufia2Memory *memory = label->memory;
    Lufia2CpuState *cpu = label->cpu;
    if (!SceneLabelCall(label, 0x80c92au, 0x80c0b7u, 2u))
        return 0u;
    OpSta(memory, cpu, SCENE_LABEL_GLYPH_LOW);
    if (!SceneLabelCall(label, 0x80c931u, 0x80c0b7u, 2u))
        return 0u;
    OpSta(memory, cpu, SCENE_LABEL_GLYPH_HIGH);
    PushY(memory, cpu);
    OpLda(memory, cpu, SCENE_LABEL_GLYPH_HIGH);
    for (unsigned bit = 0; bit < 4u; ++bit)
        OpLsrA(cpu);
    OpIncA(cpu);
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_LABEL_WIDTH));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, SCENE_LABEL_GLYPH_LOW);
    OpAndValue(cpu, 0x0fffu);
    cpu->carry = 0u;
    OpAdcValue(cpu, SCENE_LABEL_DICTIONARY_BASE);
    OpTay(cpu);
    OpSepWidths(cpu, 0x20u);
    do {
        if (!SceneLabelCall(label, 0x80c956u, 0x80c0b7u, 2u))
            return 0u;
        OpSta(memory, cpu, SCENE_LABEL_GLYPH_LOW);
        TransferDirectToA(cpu);
        OpSta(memory, cpu, SCENE_LABEL_GLYPH_HIGH);
        if (!SceneLabelDrawGlyph(label, 0x80c963u))
            return 0u;
        OpStepMem(memory, cpu, OpAbs(cpu, SCENE_LABEL_WIDTH), -1);
    } while (!cpu->zero);
    OpPullY(memory, cpu);
    return 1u;
}

static void SceneLabelQueueGlyphUpload(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, SNES_DMAP(0));
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, TEXT_GLYPH_BUFFER);
    OpSta(memory, cpu, SNES_A1TL(0));
    OpLoadA(cpu, 0x1800u);
    OpSta(memory, cpu, SCENE_LABEL_VRAM_TARGET);
    OpLoadA(cpu, 0x400u);
    OpSta(memory, cpu, SNES_DASL(0));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, SNES_A1B(0));
    OpLoadA(cpu, 0x18u);
    OpSta(memory, cpu, SNES_BBAD(0));
    OpLoadA(cpu, 0x41u);
    OpSta(memory, cpu, OpDp(cpu, SCENE_LABEL_UPLOAD_REQUEST));
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpDp(cpu, SCENE_LABEL_LAYER_REQUEST));
    OpLoadA(cpu, 0x78u);
    OpSta(memory, cpu, WRAM_FIELD_CONTROL_CHANGE_PENDING);
    OpLoadA(cpu, 1u);
    OpTestBits(memory, cpu, OpAbs(cpu, TEXT_WINDOW_STATE), 1u);
}

Lufia2ExecutionResult Lufia2TextRenderSceneLabel(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page)
        return ExecutionHandoff(cpu, 0x80c8d5u);
    SceneLabelContext label = {memory, cpu, child, context, 0u};
    if (!SceneLabelCall(&label, 0x80c8d5u, 0x848328u, 3u))
        return SceneLabelUnwound(&label);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, TEXT_SCRIPT_POINTER)));
    OpLda(memory, cpu, OpAbs(cpu, TEXT_SCRIPT_BANK));
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    if (!SceneLabelCall(&label, 0x80c8e1u, 0x80c652u, 2u))
        return SceneLabelUnwound(&label);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpAbs(cpu, SCENE_LABEL_WIDTH));
    if (cpu->zero)
        return ExecutionReturned(0x80c9bfu);
    OpRepWidths(cpu, 0x20u);
    cpu->carry = 1u;
    OpSbcValue(cpu, 0x1eu);
    OpLoadA(cpu, (uint16_t)~cpu->accumulator);
    OpIncA(cpu);
    OpAndValue(cpu, 0xfffeu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, SCENE_LABEL_ROW_POSITION));
    OpSta(memory, cpu, OpAbs(cpu, SCENE_LABEL_ROW_POSITION));
    OpStz(memory, cpu, OpAbs(cpu, TEXT_WINDOW_TILE_BASE));
    OpSepWidths(cpu, 0x20u);
    if (!SceneLabelCall(&label, 0x80c906u, 0x80c305u, 2u))
        return SceneLabelUnwound(&label);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, TEXT_WINDOW_ROW_COUNT);
    if (!SceneLabelCall(&label, 0x80c910u, 0x80c5ddu, 2u) ||
        !SceneLabelCall(&label, 0x80c913u, 0x80c784u, 3u))
        return SceneLabelUnwound(&label);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, TEXT_SCRIPT_POINTER)));
    OpLda(memory, cpu, OpAbs(cpu, TEXT_SCRIPT_BANK));
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    for (;;) {
        if (!SceneLabelCall(&label, 0x80c91fu, 0x80c0b7u, 2u))
            return SceneLabelUnwound(&label);
        OpOraValue(cpu, 0u);
        if (cpu->zero)
            break;
        OpCmpValue(cpu, 0x0au);
        if (cpu->zero) {
            if (!SceneLabelDrawReference(&label))
                return SceneLabelUnwound(&label);
        } else {
            OpSta(memory, cpu, SCENE_LABEL_GLYPH_LOW);
            TransferDirectToA(cpu);
            OpSta(memory, cpu, SCENE_LABEL_GLYPH_HIGH);
            if (!SceneLabelDrawGlyph(&label, 0x80c97au))
                return SceneLabelUnwound(&label);
        }
    }
    SceneLabelQueueGlyphUpload(memory, cpu);
    return ExecutionReturned(0x80c9bfu);
}
