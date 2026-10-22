/* Party ordering and equipment menu entry points. */

#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/wram_view.h"
#include "lufia2/menu_party_actions.h"
#include "menu/menu_sprite_slots.h"
#include "system/wram.h"

enum {
    ORDER_FIRST_MEMBER = WRAM_MENU_LIST_SELECTION,
    ORDER_PREVIOUS_MEMBER = WRAM_MENU_LIST_SELECTION + 1u,
    ORDER_CHOSEN_MEMBER = WRAM_MENU_LIST_INDEX,
    ORDER_SELECTION_OPTIONS = WRAM_UNK_7E14BF,
    ORDER_NO_MEMBER = 0xffffu,
    ORDER_FIRST_SELECTION = 0x80u,
    ORDER_SAVED_CHARACTER = 0x00u,
    ORDER_CURSOR_SLOT = 6u,
    ORDER_PARTY_SLOT = 5u,
    ORDER_EITHER_WIDTH = 2u,
    EQUIPMENT_MEMBER_SLOT = 0x22u,
    EQUIPMENT_SELECTION_TABLE = 0x54u,
    EQUIPMENT_SELECTION_SIZE = 0x56u,
    EQUIPMENT_DISPLAY_REQUEST = 0x74u,
    EQUIPMENT_PORTRAIT_SLOT = 4u,
    EQUIPMENT_CURSOR_SLOT = 7u
};

typedef struct PartyMenuCall {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
} PartyMenuCall;

static bool PartyMenuBankSupported(uint8_t bank) {
    return bank < 0x40u || (bank >= 0x80u && bank < 0xc0u) || bank == 0x7eu;
}

static bool PartyMenuRegistersReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x82u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->direct_page && !cpu->decimal &&
        PartyMenuBankSupported(cpu->data_bank);
}

static bool PartyMenuReady(const Lufia2CpuState *cpu, Lufia2PushedChildCall child) {
    return child && PartyMenuRegistersReady(cpu) &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static bool PartyMenuChild(PartyMenuCall *call, uint32_t site, uint32_t target,
    uint8_t frame, uint8_t accumulator_width) {
    Lufia2CpuState *cpu = call->cpu;
    if (!CallChildWithFrame(call->memory, cpu, call->child, call->context,
            site, target, frame, 0x82u)) {
        call->result =
            (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return false;
    }
    if (cpu->program_bank != 0x82u || cpu->index_is_8_bit ||
        cpu->direct_page || cpu->decimal || !PartyMenuBankSupported(cpu->data_bank) ||
        (accumulator_width != ORDER_EITHER_WIDTH &&
            cpu->accumulator_is_8_bit != accumulator_width)) {
        call->result = ExecutionHandoff(cpu, site + frame + 1u);
        return false;
    }
    return true;
}

static void SwapPartyColumn(Lufia2Wram wram, Lufia2CpuState *cpu,
    uint32_t column) {
    const uint8_t left = WramReadAt(wram, column, cpu->x);
    const uint8_t right = WramReadAt(wram, column, cpu->y);
    WramWriteAt(wram, column, cpu->x, right);
    WramWriteAt(wram, column, cpu->y, left);
    cpu->accumulator = (uint16_t)(((uint16_t)right << 8) | left);
    SetNz8(cpu, left);
}

Lufia2ExecutionResult Lufia2MenuSwapPartyMembers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint16_t columns[] = {
        MENU_SPRITE_LIST_LOW, MENU_SPRITE_LIST_HIGH, MENU_SPRITE_BANK,
        MENU_SPRITE_SCRIPT_LOW, MENU_SPRITE_SCRIPT_HIGH,
        MENU_SPRITE_FRAME_LOW, MENU_SPRITE_FRAME_HIGH,
        MENU_SPRITE_FRAME_INDEX, MENU_SPRITE_TIMER, MENU_SPRITE_ANIMATION
    };
    if (!PartyMenuRegistersReady(cpu))
        return ExecutionHandoff(cpu, 0x82b6efu);
    OpSepWidths(cpu, 0x10u);
    OpLda(memory, cpu, OpAbs(cpu, ORDER_FIRST_MEMBER));
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbs(cpu, ORDER_PREVIOUS_MEMBER));
    OpAslA(cpu);
    OpTay(cpu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_MENU_PARTY_CHARACTER_OFFSET));
    OpSta(memory, cpu, OpDp(cpu, ORDER_SAVED_CHARACTER));
    OpLda(memory, cpu, OpAbsY(cpu, WRAM_MENU_PARTY_CHARACTER_OFFSET));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_MENU_PARTY_CHARACTER_OFFSET));
    OpLda(memory, cpu, OpDp(cpu, ORDER_SAVED_CHARACTER));
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_MENU_PARTY_CHARACTER_OFFSET));
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, ORDER_FIRST_MEMBER)));
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, ORDER_PREVIOUS_MEMBER)));
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    SwapPartyColumn(wram, cpu, WRAM_MENU_PARTY_FIRST_ID);
    for (unsigned i = 0u; i < sizeof(columns) / sizeof(columns[0]); ++i)
        SwapPartyColumn(wram, cpu, columns[i]);
    OpRepWidths(cpu, 0x10u);
    return ExecutionReturned(0x82b7b1u);
}

Lufia2ExecutionResult Lufia2MenuReorderParty(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!PartyMenuReady(cpu, child))
        return ExecutionHandoff(cpu, 0x82b67cu);
    PartyMenuCall call = {memory, cpu, child, context, {0}};
    if (!PartyMenuChild(&call, 0x82b67cu, 0x829ef2u, 2u, 1u))
        return call.result;
    OpLdx(cpu, ORDER_NO_MEMBER);
    OpWriteX(memory, cpu, OpAbs(cpu, ORDER_FIRST_MEMBER), cpu->x);
    for (;;) {
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, OpAbs(cpu, ORDER_SELECTION_OPTIONS));
        if (!PartyMenuChild(&call, 0x82b68au, 0x829246u, 2u, 1u))
            return call.result;
        if (!cpu->carry) {
            OpCmpValue(cpu, 0u);
            if (!cpu->zero)
                continue;
            OpLda(memory, cpu, OpAbs(cpu, ORDER_FIRST_MEMBER));
            OpSta(memory, cpu, OpAbs(cpu, ORDER_PREVIOUS_MEMBER));
            ExchangeAccumulatorBytes(cpu);
            OpLda(memory, cpu, OpAbs(cpu, ORDER_CHOSEN_MEMBER));
            OpSta(memory, cpu, OpAbs(cpu, ORDER_FIRST_MEMBER));
            ExchangeAccumulatorBytes(cpu);
            OpAndValue(cpu, ORDER_FIRST_SELECTION);
            if (!cpu->zero) {
                if (!PartyMenuChild(&call, 0x82b6b1u, 0x829984u, 2u, 1u))
                    return call.result;
                continue;
            }
            if (!PartyMenuChild(&call, 0x82b6a7u, 0x82b6efu, 2u, 1u))
                return call.result;
            OpLoadA(cpu, 0u);
            if (!PartyMenuChild(&call, 0x82b6acu, 0x82a318u, 2u, 1u))
                return call.result;
        }
        OpRepWidths(cpu, 0x20u);
        OpLoadA(cpu, 0x0082u);
        OpLdx(cpu, 0x1e0au);
        OpLdy(cpu, 3u);
        if (!PartyMenuChild(&call, 0x82b6c2u, 0x8280cau, 2u, ORDER_EITHER_WIDTH))
            return call.result;
        OpSepWidths(cpu, 0x20u);
        OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, ORDER_FIRST_MEMBER)));
        OpCpx(cpu, ORDER_NO_MEMBER);
        if (!cpu->zero) {
            OpLdx(cpu, ORDER_NO_MEMBER);
            OpWriteX(memory, cpu, OpAbs(cpu, ORDER_FIRST_MEMBER), cpu->x);
            continue;
        }
        OpLoadA(cpu, 0u);
        OpLdx(cpu, ORDER_CURSOR_SLOT);
        OpLdy(cpu, ORDER_PARTY_SLOT);
        if (!PartyMenuChild(&call, 0x82b6e0u, 0x8289fau, 2u, 1u))
            return call.result;
        OpLoadA(cpu, 1u);
        OpLdx(cpu, ORDER_CURSOR_SLOT);
        OpLdy(cpu, ORDER_PARTY_SLOT);
        if (!PartyMenuChild(&call, 0x82b6ebu, 0x8289bcu, 2u, 1u))
            return call.result;
        return ExecutionReturned(0x82b6eeu);
    }
}

Lufia2ExecutionResult Lufia2MenuOpenEquipment(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!PartyMenuReady(cpu, child))
        return ExecutionHandoff(cpu, 0x82b7b2u);
    PartyMenuCall call = {memory, cpu, child, context, {0}};
    if (!PartyMenuChild(&call, 0x82b7b2u, 0x829ef2u, 2u, 1u))
        return call.result;
    OpLoadA(cpu, 3u);
    OpSta(memory, cpu, OpAbs(cpu, ORDER_SELECTION_OPTIONS));
    if (!PartyMenuChild(&call, 0x82b7bau, 0x829246u, 2u, 1u))
        return call.result;
    if (cpu->carry)
        return ExecutionReturned(0x82b7e4u);
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_SCROLL_ROW), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW), cpu->x);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E14DD));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E14EF));
    if (!PartyMenuChild(&call, 0x82b7ceu, 0x82bd48u, 2u, 1u) ||
        !PartyMenuChild(&call, 0x82b7d1u, 0x868b32u, 3u, 1u) ||
        !PartyMenuChild(&call, 0x82b7d5u, 0x82838fu, 2u, 1u) ||
        !PartyMenuChild(&call, 0x82b7d8u, 0x829f0cu, 2u, 1u) ||
        !PartyMenuChild(&call, 0x82b7dbu, 0x829f6fu, 2u, 1u) ||
        !PartyMenuChild(&call, 0x82b7deu, 0x828704u, 2u, 1u) ||
        !PartyMenuChild(&call, 0x82b7e1u, 0x82b7e5u, 2u, 1u))
        return call.result;
    return ExecutionReturned(0x82b7e4u);
}

Lufia2ExecutionResult Lufia2MenuPrepareEquipmentPortrait(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    static const uint16_t columns[] = {
        MENU_SPRITE_LIST_LOW, MENU_SPRITE_LIST_HIGH, MENU_SPRITE_BANK
    };
    static const uint16_t position[] = {
        MENU_SPRITE_X_LOW, MENU_SPRITE_X_HIGH, MENU_SPRITE_Y_LOW, MENU_SPRITE_Y_HIGH
    };
    if (!PartyMenuReady(cpu, child))
        return ExecutionHandoff(cpu, 0x829f0cu);
    PartyMenuCall call = {memory, cpu, child, context, {0}};
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_WINDOW_REFRESH));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_CURSOR_ENABLED));
    OpLdx(cpu, 0xffffu);
    if (!PartyMenuChild(&call, 0x829f15u, 0x829214u, 3u, 1u))
        return call.result;
    OpLdx(cpu, 0x10u);
    if (!PartyMenuChild(&call, 0x829f1cu, 0x868d47u, 3u, 1u))
        return call.result;
    OpLda(memory, cpu, OpAbs(cpu, ORDER_CHOSEN_MEMBER));
    OpSta(memory, cpu, OpDp(cpu, EQUIPMENT_MEMBER_SLOT));
    OpLoadA(cpu, 3u);
    if (!PartyMenuChild(&call, 0x829f27u, 0x82a318u, 2u, 1u))
        return call.result;
    OpLdx(cpu, EQUIPMENT_PORTRAIT_SLOT);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, ORDER_CHOSEN_MEMBER)));
    for (unsigned i = 0u; i < sizeof(columns) / sizeof(columns[0]); ++i) {
        OpLda(memory, cpu, OpAbsY(cpu, columns[i]));
        OpSta(memory, cpu, OpAbsX(cpu, columns[i]));
    }
    for (unsigned i = 0u; i < sizeof(position) / sizeof(position[0]); ++i) {
        OpLda(memory, cpu, OpAbs(cpu, position[i]));
        OpSta(memory, cpu, OpAbsX(cpu, position[i]));
    }
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ACTIVE));
    OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_FRAME_INDEX));
    OpLda(memory, cpu, OpAbsY(cpu, MENU_SPRITE_ANIMATION));
    if (!PartyMenuChild(&call, 0x829f65u, 0x868cf5u, 3u, 1u))
        return call.result;
    OpLoadA(cpu, 2u);
    if (!PartyMenuChild(&call, 0x829f6bu, 0x8293f6u, 2u, 1u))
        return call.result;
    return ExecutionReturned(0x829f6eu);
}

Lufia2ExecutionResult Lufia2MenuPresentEquipmentList(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!PartyMenuReady(cpu, child))
        return ExecutionHandoff(cpu, 0x829fb0u);
    PartyMenuCall call = {memory, cpu, child, context, {0}};
    OpLdx(cpu, 0x1cu);
    OpWriteX(memory, cpu, OpDp(cpu, EQUIPMENT_SELECTION_TABLE), cpu->x);
    OpLdx(cpu, 0x0401u);
    OpWriteX(memory, cpu, OpDp(cpu, EQUIPMENT_SELECTION_SIZE), cpu->x);
    OpLdx(cpu, EQUIPMENT_CURSOR_SLOT);
    OpLdy(cpu, 8u);
    if (!PartyMenuChild(&call, 0x829fc0u, 0x8289ecu, 2u, 1u))
        return call.result;
    OpStz(memory, cpu, OpAbs(cpu, MENU_SPRITE_ACTIVE + EQUIPMENT_CURSOR_SLOT));
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x0442u);
    OpLdx(cpu, 0x1e0bu);
    if (!PartyMenuChild(&call, 0x829fceu, 0x8283b5u, 2u, 0u))
        return call.result;
    OpLoadA(cpu, 0x0442u);
    OpLdx(cpu, 0x1e0au);
    if (!PartyMenuChild(&call, 0x829fd7u, 0x82810eu, 2u, ORDER_EITHER_WIDTH))
        return call.result;
    OpSepWidths(cpu, 0x20u);
    OpLdy(cpu, 0xf090u);
    if (!PartyMenuChild(&call, 0x829fdfu, 0x828c9cu, 2u, 1u))
        return call.result;
    OpLdx(cpu, 0x4000u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_SCROLL_TRACK), cpu->x);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_MARKER_ROW));
    if (!PartyMenuChild(&call, 0x829febu, 0x82be04u, 2u, 1u) ||
        !PartyMenuChild(&call, 0x829feeu, 0x82bc96u, 2u, 1u))
        return call.result;
    OpLoadA(cpu, 2u);
    if (!PartyMenuChild(&call, 0x829ff3u, 0x8293f6u, 2u, 1u))
        return call.result;
    OpLoadA(cpu, 0x88u);
    OpTestBits(memory, cpu, OpDp(cpu, EQUIPMENT_DISPLAY_REQUEST), 1u);
    return ExecutionReturned(0x829ffau);
}
