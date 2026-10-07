#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "menu/menu_sprite_slots.h"
#include "lufia2/menu.h"
#include "system/wram.h"

enum {
    CURSOR_SLOT = WRAM_MENU_CURSOR_SLOT,
    CURSOR_ITEM_FLAGS = WRAM_MENU_ITEM_FLAGS,
    CURSOR_ITEM_LIMIT = WRAM_MENU_ITEM_LIMIT,
    CURSOR_ORIGIN_X_LOW = WRAM_MENU_ITEM_ORIGIN_X_LOW,
    CURSOR_ORIGIN_X_HIGH = WRAM_MENU_ITEM_ORIGIN_X_HIGH,
    CURSOR_COLUMN = WRAM_MENU_ITEM_COLUMN,
    CURSOR_ORIGIN_Y_LOW = WRAM_MENU_ITEM_ORIGIN_Y_LOW,
    CURSOR_ORIGIN_Y_HIGH = WRAM_MENU_ITEM_ORIGIN_Y_HIGH,
    CURSOR_ROW = WRAM_MENU_ITEM_ROW,
    CURSOR_COLUMNS = WRAM_MENU_ITEM_COLUMNS,
    CURSOR_ROWS = WRAM_MENU_ITEM_ROWS,
    CURSOR_CELL_WIDTH = WRAM_MENU_ITEM_CELL_WIDTH,
    CURSOR_CELL_HEIGHT = WRAM_MENU_ITEM_CELL_HEIGHT,
    CURSOR_PARTY_COUNT = WRAM_MENU_PARTY_MEMBER_COUNT,
    CURSOR_PAIR_GRID = 0x54u,
    CURSOR_PAIR_STYLE = 0x56u,
    CURSOR_PAIR_SECOND_STYLE = 0x57u,
    CURSOR_PAIR_FIRST = 0x58u,
    CURSOR_PAIR_SECOND = 0x5au,
    CURSOR_PAIR_ITEM = 0x63u,
    ROM_CURSOR_GRID = 0xf518u,
    ROM_CURSOR_STYLES = 0x82897du,
    CURSOR_SPRITE_BASE = 5u
};

static uint8_t CursorCallerReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x82u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit;
}

static uint8_t CursorCall(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t site,
    uint32_t target, uint8_t frame) {
    return CallChildWithFrame(memory, cpu, child, context,
        site, target, frame, 0x82u);
}

static Lufia2ExecutionResult CursorUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2MenuClearSpriteMask(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    if (!CursorCallerReady(cpu))
        return ExecutionHandoff(cpu, 0x829214u);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    OpTxa(cpu);
    OpLdx(cpu, 0u);
    do {
        OpLsrA(cpu);
        if (cpu->carry) {
            OpSepWidths(cpu, 0x20u);
            OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ACTIVE));
            OpRepWidths(cpu, 0x20u);
        }
        OpInx(cpu);
        OpCpx(cpu, 16u);
    } while (!cpu->zero);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x82922cu);
}

Lufia2ExecutionResult Lufia2MenuLoadCursorGrid(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CursorCallerReady(cpu))
        return ExecutionHandoff(cpu, 0x82891eu);
    PushDataBank(memory, cpu);
    OpLoadA(cpu, 0xa6u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpLda(memory, cpu, OpAbsY(cpu, ROM_CURSOR_GRID));
    OpSta(memory, cpu, OpAbsX(cpu, CURSOR_ORIGIN_X_LOW));
    OpLda(memory, cpu, OpAbsY(cpu, ROM_CURSOR_GRID + 1u));
    OpSta(memory, cpu, OpAbsX(cpu, CURSOR_ORIGIN_Y_LOW));
    OpStz(memory, cpu, OpAbsX(cpu, CURSOR_ORIGIN_X_HIGH));
    OpStz(memory, cpu, OpAbsX(cpu, CURSOR_ORIGIN_Y_HIGH));
    OpStz(memory, cpu, OpAbsX(cpu, CURSOR_ITEM_LIMIT));
    static const uint16_t destinations[] = {
        CURSOR_COLUMNS, CURSOR_ROWS, CURSOR_CELL_WIDTH,
        CURSOR_CELL_HEIGHT, CURSOR_ITEM_FLAGS
    };
    for (unsigned field = 0u; field < 5u; ++field) {
        OpLda(memory, cpu, OpAbsY(cpu,
            (uint16_t)(ROM_CURSOR_GRID + 2u + field)));
        OpSta(memory, cpu, OpAbsX(cpu, destinations[field]));
    }
    PullDataBank(memory, cpu);
    if (!CursorCall(memory, cpu, child, context,
            0x828957u, 0x8288cbu, 2u))
        return CursorUnwound(0x828957u);
    return ExecutionReturned(0x82895au);
}

Lufia2ExecutionResult Lufia2MenuSetCursorStyle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CursorCallerReady(cpu))
        return ExecutionHandoff(cpu, 0x82895bu);
    OpWriteX(memory, cpu, OpAbs(cpu, CURSOR_SLOT), cpu->x);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, OpA(cpu) & 0x00ffu);
    OpDecA(cpu);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, ROM_CURSOR_STYLES));
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, CURSOR_SLOT)));
    OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_FRAME_INDEX));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ACTIVE));
    ExchangeAccumulatorBytes(cpu);
    if (!CursorCall(memory, cpu, child, context,
            0x828978u, 0x868cf5u, 3u))
        return CursorUnwound(0x828978u);
    return ExecutionReturned(0x82897cu);
}

Lufia2ExecutionResult Lufia2MenuValidatePartyCursor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CursorCallerReady(cpu))
        return ExecutionHandoff(cpu, 0x82898du);
    OpLda(memory, cpu, OpAbsX(cpu, CURSOR_ROW));
    OpAslA(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbsX(cpu, CURSOR_COLUMN));
    OpCmp(memory, cpu, OpAbs(cpu, CURSOR_PARTY_COUNT));
    if (cpu->carry) {
        OpStz(memory, cpu, OpAbsX(cpu, CURSOR_COLUMN));
        OpStz(memory, cpu, OpAbsX(cpu, CURSOR_ROW));
        if (!CursorCall(memory, cpu, child, context,
                0x8289a0u, 0x8288cbu, 2u))
            return CursorUnwound(0x8289a0u);
    }
    return ExecutionReturned(0x8289a3u);
}

Lufia2ExecutionResult Lufia2MenuBuildCursorPair(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CursorCallerReady(cpu))
        return ExecutionHandoff(cpu, 0x8289c4u);
    OpWriteX(memory, cpu, OpDp(cpu, CURSOR_PAIR_FIRST), cpu->x);
    OpWriteX(memory, cpu, OpDp(cpu, CURSOR_PAIR_SECOND), cpu->y);
    OpLda(memory, cpu, OpDp(cpu, CURSOR_PAIR_SECOND_STYLE));
    if (cpu->zero) {
        OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ACTIVE));
    } else if (!CursorCall(memory, cpu, child, context,
            0x8289d1u, 0x82895bu, 2u)) {
        return CursorUnwound(0x8289d1u);
    }
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, CURSOR_PAIR_SECOND)));
    for (unsigned slot = 0u; slot < CURSOR_SPRITE_BASE; ++slot)
        OpDex(cpu);
    OpWriteX(memory, cpu, OpDp(cpu, CURSOR_PAIR_ITEM), cpu->x);
    OpLda(memory, cpu, OpDp(cpu, CURSOR_PAIR_STYLE));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, CURSOR_PAIR_SECOND)));
    if (!CursorCall(memory, cpu, child, context,
            0x8289e1u, 0x82895bu, 2u))
        return CursorUnwound(0x8289e1u);
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, CURSOR_PAIR_GRID)));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, CURSOR_PAIR_ITEM)));
    if (!CursorCall(memory, cpu, child, context,
            0x8289e8u, 0x82891eu, 2u))
        return CursorUnwound(0x8289e8u);
    return ExecutionReturned(0x8289ebu);
}

Lufia2ExecutionResult Lufia2MenuMoveCursorPair(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CursorCallerReady(cpu))
        return ExecutionHandoff(cpu, 0x8289ecu);
    if (!CursorCall(memory, cpu, child, context,
            0x8289ecu, 0x8289c4u, 2u))
        return CursorUnwound(0x8289ecu);
    OpLoadA(cpu, 1u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, CURSOR_PAIR_FIRST)));
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, CURSOR_SLOT)));
    if (!CursorCall(memory, cpu, child, context,
            0x8289f6u, 0x8289fau, 2u))
        return CursorUnwound(0x8289f6u);
    return ExecutionReturned(0x8289f9u);
}
