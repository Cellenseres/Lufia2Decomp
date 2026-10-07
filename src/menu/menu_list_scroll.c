#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "menu/menu_sprite_slots.h"
#include "lufia2/menu.h"
#include "system/wram.h"

enum {
    SCROLL_SPRITE = 11u,
    LIST_SELECTION_WORD = WRAM_MENU_LIST_INDEX,
    LIST_PAGE_OFFSET = WRAM_MENU_LIST_TOP_ROW,
    SCROLL_ORIGIN_Y = WRAM_MENU_SCROLL_THUMB_Y,
    SCROLL_ORIGIN_Y_HIGH = WRAM_MENU_SCROLL_THUMB_Y + 1u,
    SCROLL_TRAVEL = WRAM_MENU_SCROLL_TRACK,
    SCROLL_STEP = WRAM_MENU_SCROLL_STEP,
    SCROLL_POSITION = WRAM_MENU_SCROLL_ROW,
    SCROLL_LIMIT = WRAM_MENU_SCROLL_RANGE,
    SCROLL_PAGE_ROWS = WRAM_MENU_SCROLL_WINDOW_ROWS,
    LIST_COLUMNS = WRAM_BATTLE_PARTY_IDS + 2u,
    DIVIDE_VALUE = 0x4eu,
    DIVIDE_BY = 0x51u,
    SCROLL_PRODUCT_LEFT = WRAM_SYSTEM_MULTIPLY_A,
    SCROLL_PRODUCT_RIGHT = WRAM_SYSTEM_MULTIPLY_B,
    SCROLL_PRODUCT_LOW = WRAM_SYSTEM_MULTIPLY_PRODUCT,
    SCROLL_ENDPOINT_ANIMATION = 4u,
    SCROLL_INTERIOR_ANIMATION = 5u
};

static uint8_t ScrollReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x82u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal;
}

static uint8_t ScrollChild(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t site,
    uint32_t target, uint8_t frame) {
    return CallChildWithFrame(memory, cpu, child, context,
        site, target, frame, 0x82u);
}

static Lufia2ExecutionResult ScrollUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2MenuSelectedListOffset(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    if (cpu->program_bank != 0x82u || cpu->accumulator_is_8_bit ||
        cpu->decimal)
        return ExecutionHandoff(cpu, 0x82fbe5u);
    OpLda(memory, cpu, OpAbs(cpu, LIST_SELECTION_WORD));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, LIST_PAGE_OFFSET));
    OpAslA(cpu);
    return ExecutionReturned(0x82fbedu);
}

Lufia2ExecutionResult Lufia2MenuSetScrollPosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ScrollReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x828c43u);
    OpLdx(cpu, SCROLL_SPRITE);
    OpLda(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ACTIVE));
    if (cpu->zero)
        return ExecutionReturned(0x828c84u);
    OpRepWidths(cpu, 0x30u);
    OpLdy(cpu, OpRead16(memory, OpAbs(cpu, SCROLL_POSITION)));
    OpWrite16(memory, OpAbs(cpu, SCROLL_PRODUCT_LEFT), cpu->y);
    OpLdx(cpu, OpRead16(memory, OpAbs(cpu, SCROLL_STEP)));
    OpWrite16(memory, OpAbs(cpu, SCROLL_PRODUCT_RIGHT), cpu->x);
    if (!ScrollChild(memory, cpu, child, context,
        0x828c59u, 0x828000u, 3u))
        return ScrollUnwound(0x828c59u);
    OpLdx(cpu, SCROLL_SPRITE);
    OpLda(memory, cpu, OpAbs(cpu, SCROLL_ORIGIN_Y));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, SCROLL_PRODUCT_LOW));
    OpSepWidths(cpu, 0x20u);
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_Y_LOW));
    OpLoadA(cpu, SCROLL_ENDPOINT_ANIMATION);
    OpLdy(cpu, OpRead16(memory, OpAbs(cpu, SCROLL_POSITION)));
    if (!cpu->zero) {
        OpCpy(cpu, OpRead16(memory, OpAbs(cpu, SCROLL_LIMIT)));
        if (!cpu->zero)
            OpLoadA(cpu, SCROLL_INTERIOR_ANIMATION);
    }
    OpCmp(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ANIMATION));
    if (!cpu->zero && !ScrollChild(memory, cpu, child, context,
        0x828c80u, 0x868cf5u, 3u))
        return ScrollUnwound(0x828c80u);
    return ExecutionReturned(0x828c84u);
}

Lufia2ExecutionResult Lufia2MenuInitializeScrollRange(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ScrollReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x828c85u);
    OpRepWidths(cpu, 0x20u);
    OpTxa(cpu);
    if (cpu->carry) {
        OpIncA(cpu);
        OpLsrA(cpu);
    }
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, SCROLL_PAGE_ROWS)));
    OpSta(memory, cpu, OpAbs(cpu, SCROLL_LIMIT));
    OpSepWidths(cpu, 0x20u);
    if (!ScrollChild(memory, cpu, child, context,
        0x828c95u, 0x828cc1u, 2u))
        return ScrollUnwound(0x828c95u);
    if (!ScrollChild(memory, cpu, child, context,
        0x828c98u, 0x828c43u, 2u))
        return ScrollUnwound(0x828c98u);
    return ExecutionReturned(0x828c9bu);
}

Lufia2ExecutionResult Lufia2MenuInitializeScrollSprite(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ScrollReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x828c9cu);
    OpRepWidths(cpu, 0x20u);
    OpTya(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, SCROLL_SPRITE);
    OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_Y_LOW));
    OpSta(memory, cpu, OpAbs(cpu, SCROLL_ORIGIN_Y_HIGH));
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_X_LOW));
    OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_Y_HIGH));
    OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_X_HIGH));
    OpStz(memory, cpu, OpAbs(cpu, SCROLL_ORIGIN_Y));
    OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_FRAME_INDEX));
    OpLoadA(cpu, SCROLL_ENDPOINT_ANIMATION);
    if (!ScrollChild(memory, cpu, child, context,
        0x828cbcu, 0x868cf5u, 3u))
        return ScrollUnwound(0x828cbcu);
    return ExecutionReturned(0x828cc0u);
}

Lufia2ExecutionResult Lufia2MenuInitializeScrollStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ScrollReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x828cc1u);
    OpLoadA(cpu, 0u);
    OpLdx(cpu, OpRead16(memory, OpAbs(cpu, SCROLL_LIMIT)));
    OpCpx(cpu, 0u);
    if (!cpu->negative && !cpu->zero) {
        OpLdx(cpu, OpRead16(memory, OpAbs(cpu, SCROLL_TRAVEL)));
        OpWrite16(memory, OpDp(cpu, DIVIDE_VALUE), cpu->x);
        OpLdx(cpu, OpRead16(memory, OpAbs(cpu, SCROLL_LIMIT)));
        OpWrite16(memory, OpDp(cpu, DIVIDE_BY), cpu->x);
        if (!ScrollChild(memory, cpu, child, context,
            0x828cd7u, 0x808378u, 3u))
            return ScrollUnwound(0x828cd7u);
        OpLdx(cpu, OpRead16(memory, OpDp(cpu, DIVIDE_VALUE)));
        OpWrite16(memory, OpAbs(cpu, SCROLL_STEP), cpu->x);
        OpLoadA(cpu, 1u);
    }
    OpLdx(cpu, SCROLL_SPRITE);
    OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ACTIVE));
    return ExecutionReturned(0x828ce8u);
}

static void ScrollUpdateListOffset(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSta(memory, cpu, OpAbs(cpu, SCROLL_POSITION));
    OpLda(memory, cpu, OpAbs(cpu, LIST_COLUMNS));
    OpAndValue(cpu, 0xffu);
    OpCmpValue(cpu, 1u);
    if (cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, SCROLL_POSITION));
    } else {
        OpLda(memory, cpu, OpAbs(cpu, LIST_PAGE_OFFSET));
        OpAndValue(cpu, 1u);
        OpSta(memory, cpu, OpAbs(cpu, LIST_PAGE_OFFSET));
        OpLda(memory, cpu, OpAbs(cpu, SCROLL_POSITION));
        OpAslA(cpu);
        OpOra(memory, cpu, OpAbs(cpu, LIST_PAGE_OFFSET));
    }
    OpSta(memory, cpu, OpAbs(cpu, LIST_PAGE_OFFSET));
}

Lufia2ExecutionResult Lufia2MenuScrollListPage(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    if (!ScrollReady(cpu))
        return ExecutionHandoff(cpu, 0x828ce9u);
    OpLda(memory, cpu, OpAbs(cpu, MENU_SPRITE_ACTIVE + SCROLL_SPRITE));
    if (!cpu->zero) {
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbs(cpu, SCROLL_POSITION));
        if (cpu->carry) {
            OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, SCROLL_PAGE_ROWS)));
            if (!cpu->carry) {
                OpLoadA(cpu, 0u);
                OpCmp(memory, cpu, OpAbs(cpu, SCROLL_POSITION));
                if (cpu->zero) {
                    OpSepWidths(cpu, 0x20u);
                    cpu->carry = 1u;
                    return ExecutionReturned(0x828d43u);
                }
            }
        } else {
            OpAdc(memory, cpu, OpAbs(cpu, SCROLL_PAGE_ROWS));
            OpCmp(memory, cpu, OpAbs(cpu, SCROLL_LIMIT));
            if (cpu->carry) {
                OpLda(memory, cpu, OpAbs(cpu, SCROLL_LIMIT));
                OpCmp(memory, cpu, OpAbs(cpu, SCROLL_POSITION));
                if (cpu->zero) {
                    OpSepWidths(cpu, 0x20u);
                    cpu->carry = 1u;
                    return ExecutionReturned(0x828d43u);
                }
            }
        }
        ScrollUpdateListOffset(memory, cpu);
        OpSepWidths(cpu, 0x20u);
        cpu->carry = 0u;
        return ExecutionReturned(0x828d3fu);
    }
    OpSepWidths(cpu, 0x20u);
    cpu->carry = 1u;
    return ExecutionReturned(0x828d43u);
}
