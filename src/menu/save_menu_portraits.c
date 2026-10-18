#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/menu.h"
#include "menu/menu_sprite_slots.h"
#include "system/wram.h"

enum {
    SAVE_PREVIEW_POINTER = 0x1cu,
    SAVE_NAME_POINTER = 0x19u,
    SAVE_TEXT_BANK = 0x5fu,
    SAVE_TEXT_VALUE = 0x11u,
    SAVE_TEXT_FLAG_13 = 0x13u,
    SAVE_TEXT_FLAG_14 = 0x14u,
    SAVE_TEXT_FLAG_15 = 0x15u,
    SAVE_PORTRAIT_COUNT = 0x04u,
    SAVE_ANIMATION_COUNT = 0x05u,
    SAVE_PORTRAIT_ORIGIN = 0x02u,
    SAVE_LEVEL_COLUMN = 0x00u,
    SAVE_PREVIEW_FIRST = WRAM_SAVE_MENU_SLOT_PREVIEWS & 0xffffu,
    SAVE_PREVIEW_STRIDE = 18u,
    SAVE_PREVIEW_SLOTS = 4u,
    SAVE_PORTRAIT_FIRST = 16u,
    SAVE_PORTRAIT_END = 32u,
    SAVE_ANIMATION_HIGHLIGHT = 7u,
    SAVE_SLOT_SPRITE_TABLE = 0x82f36bu,
    SAVE_SLOT_ORIGIN_TABLE = 0x82f373u,
    SAVE_SLOT_TEXT_TABLE = 0x82f37bu,
    SAVE_PORTRAIT_OFFSETS = 0xd8fbu,
    SAVE_SLOT_TEXT = 0xca47u,
    SAVE_LEVEL_TEXT = 0xca92u
};

typedef struct SavePortraitCall {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
} SavePortraitCall;

static bool SavePortraitReady(const Lufia2CpuState *cpu, Lufia2PushedChildCall child) {
    return child && cpu->program_bank == 0x82u && cpu->accumulator_is_8_bit &&
           !cpu->index_is_8_bit && !cpu->direct_page && !cpu->decimal &&
           cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static bool SavePortraitChild(SavePortraitCall *call, uint32_t site, uint32_t target,
                              uint8_t frame, uint8_t index_width) {
    Lufia2CpuState *cpu = call->cpu;
    if (!CallChildWithFrame(call->memory, cpu, call->child, call->context, site, target,
                            frame, 0x82u)) {
        call->result =
            (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return false;
    }
    if (cpu->program_bank != 0x82u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit != index_width || cpu->direct_page || cpu->decimal) {
        call->result = ExecutionHandoff(cpu, site + frame + 1u);
        return false;
    }
    return true;
}

Lufia2ExecutionResult Lufia2SaveDrawSlotPreviews(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu,
                                                 Lufia2PushedChildCall child,
                                                 void *context) {
    if (!SavePortraitReady(cpu, child))
        return ExecutionHandoff(cpu, 0x82f2ceu);
    SavePortraitCall call = {memory, cpu, child, context, {0}};
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, SAVE_PREVIEW_POINTER + 2u));
    OpSta(memory, cpu, OpDp(cpu, SAVE_NAME_POINTER + 2u));
    for (unsigned slot = 0u; slot < SAVE_PREVIEW_SLOTS; ++slot) {
        OpLdx(cpu, (uint16_t)(slot * 2u));
        OpLdy(cpu, (uint16_t)(SAVE_PREVIEW_FIRST + slot * SAVE_PREVIEW_STRIDE));
        if (!SavePortraitChild(&call, 0x82f2dfu + slot * 9u, 0x82f2feu, 2u, 0u))
            return call.result;
    }
    return ExecutionReturned(0x82f2fdu);
}

static void PushSlotPreviewLayout(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, SAVE_SLOT_SPRITE_TABLE));
    PushAccumulator16(memory, cpu);
    OpLda(memory, cpu, OpLongX(cpu, SAVE_SLOT_ORIGIN_TABLE));
    PushAccumulator16(memory, cpu);
    OpLda(memory, cpu, OpLongX(cpu, SAVE_SLOT_SPRITE_TABLE));
    PushAccumulator16(memory, cpu);
    PushAccumulator16(memory, cpu);
    OpLda(memory, cpu, OpLongX(cpu, SAVE_SLOT_TEXT_TABLE));
    PushAccumulator16(memory, cpu);
}

static void PrepareSlotPreviewText(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, SAVE_PREVIEW_POINTER));
    cpu->carry = 0u;
    OpAdcValue(cpu, 1u);
    OpSta(memory, cpu, OpDp(cpu, SAVE_NAME_POINTER));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, SAVE_PREVIEW_POINTER));
    OpSta(memory, cpu, OpDp(cpu, SAVE_TEXT_VALUE));
    static const uint8_t fields[] = {SAVE_TEXT_FLAG_14, SAVE_TEXT_FLAG_13,
                                     SAVE_TEXT_FLAG_15};
    for (unsigned field = 0u; field < 3u; ++field) {
        OpLdy(cpu, (uint16_t)(15u + field));
        OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, SAVE_PREVIEW_POINTER));
        OpSta(memory, cpu, OpDp(cpu, fields[field]));
    }
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, SAVE_TEXT_BANK));
    OpLdy(cpu, SAVE_SLOT_TEXT);
    OpPullX(memory, cpu);
}

Lufia2ExecutionResult Lufia2SaveDrawSlotPreview(const Lufia2Memory *memory,
                                                Lufia2CpuState *cpu,
                                                Lufia2PushedChildCall child,
                                                void *context) {
    if (!SavePortraitReady(cpu, child))
        return ExecutionHandoff(cpu, 0x82f2feu);
    SavePortraitCall call = {memory, cpu, child, context, {0}};
    OpWriteX(memory, cpu, OpDp(cpu, SAVE_PREVIEW_POINTER), cpu->y);
    PushSlotPreviewLayout(memory, cpu);
    PrepareSlotPreviewText(memory, cpu);
    if (!SavePortraitChild(&call, 0x82f342u, 0x808878u, 3u, 0u))
        return call.result;
    OpLda(memory, cpu, OpDp(cpu, SAVE_TEXT_VALUE));
    if (cpu->zero) {
        for (unsigned saved = 0u; saved < 4u; ++saved)
            OpPullX(memory, cpu);
        return ExecutionReturned(0x82f362u);
    }
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, SAVE_PREVIEW_POINTER));
    OpPullX(memory, cpu);
    if (!SavePortraitChild(&call, 0x82f34du, 0x82f383u, 2u, 0u))
        return call.result;
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, SAVE_PREVIEW_POINTER));
    OpPullX(memory, cpu);
    OpPullY(memory, cpu);
    if (!SavePortraitChild(&call, 0x82f354u, 0x82f395u, 2u, 0u))
        return call.result;
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, SAVE_PREVIEW_POINTER));
    OpPullX(memory, cpu);
    if (!SavePortraitChild(&call, 0x82f35au, 0x82f427u, 2u, 0u))
        return call.result;
    return ExecutionReturned(0x82f35du);
}

Lufia2ExecutionResult Lufia2SaveInitializePortraitTables(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu,
                                                         Lufia2PushedChildCall child,
                                                         void *context) {
    if (!SavePortraitReady(cpu, child))
        return ExecutionHandoff(cpu, 0x82f383u);
    SavePortraitCall call = {memory, cpu, child, context, {0}};
    OpSepWidths(cpu, 0x10u);
    OpSta(memory, cpu, OpDp(cpu, SAVE_PORTRAIT_COUNT));
    OpLdy(cpu, 8u);
    do {
        if (!SavePortraitChild(&call, 0x82f389u, 0x868cdau, 3u, 1u))
            return call.result;
        OpInx(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, SAVE_PORTRAIT_COUNT), -1);
    } while (!cpu->zero);
    OpRepWidths(cpu, 0x10u);
    return ExecutionReturned(0x82f394u);
}

static void PlacePreviewPortraits(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdy(cpu, 0u);
    do {
        OpLda(memory, cpu, OpAbsY(cpu, SAVE_PORTRAIT_OFFSETS));
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpDp(cpu, SAVE_PORTRAIT_ORIGIN + 1u));
        OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_X_LOW));
        OpLda(memory, cpu, OpAbsY(cpu, SAVE_PORTRAIT_OFFSETS + 1u));
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpDp(cpu, SAVE_PORTRAIT_ORIGIN));
        OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_Y_LOW));
        OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_X_HIGH));
        OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_Y_HIGH));
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ACTIVE));
        OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_FRAME_INDEX));
        OpInx(cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, SAVE_PORTRAIT_COUNT), -1);
    } while (!cpu->zero);
}

Lufia2ExecutionResult Lufia2SavePlaceSlotPortraits(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    if (!SavePortraitReady(cpu, child))
        return ExecutionHandoff(cpu, 0x82f395u);
    SavePortraitCall call = {memory, cpu, child, context, {0}};
    OpSta(memory, cpu, OpDp(cpu, SAVE_PORTRAIT_COUNT));
    OpSta(memory, cpu, OpDp(cpu, SAVE_ANIMATION_COUNT));
    OpWriteX(memory, cpu, OpDp(cpu, SAVE_PORTRAIT_ORIGIN), cpu->y);
    OpPushX(memory, cpu);
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x8eu);
    PlacePreviewPortraits(memory, cpu);
    PullDataBank(memory, cpu);
    OpPullX(memory, cpu);
    OpLdy(cpu, 7u);
    do {
        OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, SAVE_PREVIEW_POINTER));
        if (!SavePortraitChild(&call, 0x82f3d2u, 0x868cf5u, 3u, 0u))
            return call.result;
        OpInx(cpu);
        OpIny(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, SAVE_ANIMATION_COUNT), -1);
    } while (!cpu->zero);
    return ExecutionReturned(0x82f3dcu);
}

Lufia2ExecutionResult Lufia2SaveUnhighlightPortraits(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu,
                                                     Lufia2PushedChildCall child,
                                                     void *context) {
    if (!SavePortraitReady(cpu, child))
        return ExecutionHandoff(cpu, 0x82f3ddu);
    SavePortraitCall call = {memory, cpu, child, context, {0}};
    OpLdx(cpu, SAVE_PORTRAIT_FIRST);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ACTIVE));
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ANIMATION));
            OpCmpValue(cpu, SAVE_ANIMATION_HIGHLIGHT);
            if (cpu->carry) {
                OpSbcValue(cpu, SAVE_ANIMATION_HIGHLIGHT);
                OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_FRAME_INDEX));
                if (!SavePortraitChild(&call, 0x82f3f1u, 0x868cf5u, 3u, 0u))
                    return call.result;
            }
        }
        OpInx(cpu);
        OpCpx(cpu, SAVE_PORTRAIT_END);
    } while (!cpu->zero);
    return ExecutionReturned(0x82f3fbu);
}

Lufia2ExecutionResult Lufia2SaveHighlightSlotPortraits(const Lufia2Memory *memory,
                                                       Lufia2CpuState *cpu,
                                                       Lufia2PushedChildCall child,
                                                       void *context) {
    if (!SavePortraitReady(cpu, child))
        return ExecutionHandoff(cpu, 0x82f3fcu);
    SavePortraitCall call = {memory, cpu, child, context, {0}};
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_INDEX));
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, SAVE_SLOT_SPRITE_TABLE));
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLdy(cpu, 4u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ACTIVE));
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ANIMATION));
            OpCmpValue(cpu, SAVE_ANIMATION_HIGHLIGHT);
            if (!cpu->carry) {
                OpAdcValue(cpu, SAVE_ANIMATION_HIGHLIGHT);
                OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_FRAME_INDEX));
                if (!SavePortraitChild(&call, 0x82f41eu, 0x868cf5u, 3u, 0u))
                    return call.result;
            }
        }
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    return ExecutionReturned(0x82f426u);
}

static void SelectPortraitLevelText(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, MENU_SPRITE_X_LOW));
    OpAndValue(cpu, 0x00f8u);
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpDecA(cpu);
    OpDecA(cpu);
    OpSta(memory, cpu, OpDp(cpu, SAVE_LEVEL_COLUMN));
    OpLda(memory, cpu, OpAbsX(cpu, MENU_SPRITE_Y_LOW));
    OpAndValue(cpu, 0x00f8u);
    OpAslA(cpu);
    OpAslA(cpu);
    OpAslA(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x30c0u);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, SAVE_LEVEL_COLUMN));
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, SAVE_PREVIEW_POINTER));
    OpSta(memory, cpu, OpDp(cpu, SAVE_TEXT_VALUE));
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, SAVE_TEXT_BANK));
    OpLdy(cpu, SAVE_LEVEL_TEXT);
}

Lufia2ExecutionResult Lufia2SaveDrawPortraitLevels(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    if (!SavePortraitReady(cpu, child))
        return ExecutionHandoff(cpu, 0x82f427u);
    SavePortraitCall call = {memory, cpu, child, context, {0}};
    OpSta(memory, cpu, OpDp(cpu, SAVE_PORTRAIT_COUNT));
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    OpLdy(cpu, 11u);
    do {
        OpPushX(memory, cpu);
        PushY(memory, cpu);
        SelectPortraitLevelText(memory, cpu);
        if (!SavePortraitChild(&call, 0x82f45fu, 0x808878u, 3u, 0u))
            return call.result;
        OpPullY(memory, cpu);
        OpPullX(memory, cpu);
        OpInx(cpu);
        OpIny(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, SAVE_PORTRAIT_COUNT), -1);
    } while (!cpu->zero);
    return ExecutionReturned(0x82f46bu);
}
