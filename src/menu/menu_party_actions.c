/* Party ordering and equipment menu entry points. */

#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/wram_view.h"
#include "lufia2/menu_party_actions.h"
#include "lufia2/menu_equipment_commands.h"
#include "lufia2/menu.h"
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
    EQUIPMENT_CURSOR_SLOT = 7u,
    EQUIPMENT_MEMBER_POINTER = 0x2au,
    EQUIPMENT_COMMAND_INDEX = WRAM_MENU_LIST_INDEX,
    EQUIPMENT_LIST_MARKER = WRAM_MENU_LIST_MARKER_ROW,
    EQUIPMENT_TEXT_DESTINATION = 0x11u,
    EQUIPMENT_TEXT_ROWS = 0x15u,
    EQUIPMENT_TEXT_INDEX = 0x00u,
    EQUIPMENT_SCROLL_REQUEST = WRAM_MENU_DISPLAY_REQUESTS + 1u,
    EQUIPMENT_DISCARD_LOCK = 0x20u,
    EQUIPMENT_SLOT_COUNT = 6u,
    EQUIPMENT_ROW_DESTINATION = 0x17u,
    EQUIPMENT_ROW_LIST = 0xfcu,
    EQUIPMENT_STRING_BANK = 0x5fu,
    EQUIPMENT_ROW_ATTRIBUTES = 0x7e9cf0u,
    EQUIPMENT_ROW_EMPTY = 0xffffu,
    EQUIPMENT_GROUP_POINTERS = 0x8ee538u,
    EQUIPMENT_MEMBER_MASK = 0x00u,
    EQUIPMENT_GROUP_OFFSET = 0x04u,
    EQUIPMENT_ITEM_MEMBER_MASK = WRAM_RECORD_BUFFER + 0x15u,
    EQUIPMENT_RECORD_STATS = WRAM_RECORD_BUFFER + 0x1eu,
    EQUIPMENT_CHARACTER_STATS = 0x37u,
    EQUIPMENT_STAT_WORDS = 7u,
    EQUIPMENT_ROW_AVAILABLE = 0x20u,
    EQUIPMENT_ROW_UNAVAILABLE = 0x24u
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

static bool EquipmentCloseSelection(PartyMenuCall *call) {
    return PartyMenuChild(call, 0x82b846u, 0x868b32u, 3u, 1u) &&
        PartyMenuChild(call, 0x82b84au, 0x82838fu, 2u, 1u) &&
        PartyMenuChild(call, 0x82b84du, 0x829ae0u, 2u, 1u);
}

static bool EquipmentRunCommand(PartyMenuCall *call, uint32_t target) {
    if (target == 0x82b818u) {
        return PartyMenuChild(call, 0x82b818u, 0x829fb0u, 2u, 1u) &&
            PartyMenuChild(call, 0x82b81bu, 0x82b863u, 2u, 1u) &&
            PartyMenuChild(call, 0x82b81eu, 0x829f6fu, 2u, 1u);
    }
    if (target == 0x82b824u)
        return PartyMenuChild(call, 0x82b824u, 0x82ba65u, 2u, 1u);
    return PartyMenuChild(call, 0x82b82au, 0x82bbe8u, 2u, 1u);
}

Lufia2ExecutionResult Lufia2MenuRunEquipmentSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!PartyMenuReady(cpu, child))
        return ExecutionHandoff(cpu, 0x82b7e5u);
    cpu->resume_pc = 0u;
    PartyMenuCall call = {memory, cpu, child, context, {0}};
    OpLoadA(cpu, 0u);
    for (;;) {
        if (!PartyMenuChild(&call, 0x82b7e7u, 0x828b08u, 2u, 1u))
            return call.result;
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            OpLoadA(cpu, 2u);
            if (!PartyMenuChild(&call, 0x82b804u, 0x80953bu, 3u, 1u))
                return call.result;
            OpLda(memory, cpu, OpAbs(cpu, EQUIPMENT_COMMAND_INDEX));
            const uint32_t previous_resume = cpu->resume_pc;
            SimulateJsrFrame(memory, cpu, 0xb80du);
            Lufia2ExecutionResult dispatch =
                Lufia2MenuDispatchInlineTable(memory, cpu);
            if (dispatch.pc != 0x82b818u && dispatch.pc != 0x82b824u &&
                dispatch.pc != 0x82b82au)
                return dispatch;
            cpu->resume_pc = previous_resume;
            if (!EquipmentRunCommand(&call, dispatch.pc))
                return call.result;
            OpLoadA(cpu, 1u);
            continue;
        }
        OpCmpValue(cpu, 3u);
        if (cpu->zero) {
            if (!EquipmentCloseSelection(&call))
                return call.result;
            return ExecutionReturned(0x82b850u);
        }
        OpCmpValue(cpu, 6u);
        if (!cpu->zero) {
            OpCmpValue(cpu, 7u);
            if (!cpu->zero) {
                OpLoadA(cpu, 0u);
                continue;
            }
        }
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 1u));
        OpLsrA(cpu);
        if (!PartyMenuChild(&call, 0x82b854u, 0x829a14u, 2u, 1u))
            return call.result;
        OpLoadA(cpu, 3u);
        if (!PartyMenuChild(&call, 0x82b859u, 0x82a318u, 2u, 1u))
            return call.result;
        OpLoadA(cpu, 8u);
        OpSta(memory, cpu, OpDp(cpu, EQUIPMENT_DISPLAY_REQUEST));
        OpLoadA(cpu, 1u);
    }
}

Lufia2ExecutionResult Lufia2MenuWeightEquipmentStats(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x82u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x82bb6eu);
    const uint16_t primary = WRAM_RECORD_BUFFER + 0x1eu;
    const uint16_t secondary = WRAM_RECORD_BUFFER + 0x20u;
    OpLda(memory, cpu, OpAbs(cpu, primary));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, WRAM_RECORD_BUFFER + 0x22u));
    OpSta(memory, cpu, OpAbs(cpu, primary));
    static const uint16_t bonuses[] = {
        WRAM_RECORD_BUFFER + 0x24u, WRAM_RECORD_BUFFER + 0x22u
    };
    for (unsigned part = 0u; part < 2u; ++part) {
        OpLda(memory, cpu, OpAbs(cpu, bonuses[part]));
        OpLsrA(cpu);
        OpLsrA(cpu);
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpAbs(cpu, secondary));
        OpSta(memory, cpu, OpAbs(cpu, secondary));
    }
    return ExecutionReturned(0x82bb90u);
}

Lufia2ExecutionResult Lufia2MenuUnequipSelectedSlot(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!PartyMenuReady(cpu, child))
        return ExecutionHandoff(cpu, 0x82bb91u);
    PartyMenuCall call = {memory, cpu, child, context, {0}};
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, EQUIPMENT_LIST_MARKER));
    cpu->carry = 1u;
    if (!PartyMenuChild(&call, 0x82bb97u, 0x82f8dcu, 2u, 0u))
        return call.result;
    OpSepWidths(cpu, 0x20u);
    if (cpu->carry) {
        cpu->carry = 1u;
        return ExecutionReturned(0x82bbe7u);
    }
    OpLoadA(cpu, 2u);
    if (!PartyMenuChild(&call, 0x82bba0u, 0x80953bu, 3u, 1u))
        return call.result;
    OpLoadA(cpu, 5u);
    OpLdx(cpu, 8u);
    if (!PartyMenuChild(&call, 0x82bba9u, 0x82895bu, 2u, 1u) ||
        !PartyMenuChild(&call, 0x82bbacu, 0x868b55u, 3u, 1u))
        return call.result;
    OpRepWidths(cpu, 0x20u);
    if (!PartyMenuChild(&call, 0x82bbb2u, 0x82f846u, 2u, 0u))
        return call.result;
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, EQUIPMENT_MEMBER_POINTER)));
    if (!PartyMenuChild(&call, 0x82bbb7u, 0x81f4d5u, 3u, 0u) ||
        !PartyMenuChild(&call, 0x82bbbbu, 0x82bd48u, 2u, 0u))
        return call.result;
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_SCROLL_ROW), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW), cpu->x);
    OpLda(memory, cpu, OpAbs(cpu, EQUIPMENT_LIST_MARKER));
    if (!PartyMenuChild(&call, 0x82bbcau, 0x82be04u, 2u, 0u) ||
        !PartyMenuChild(&call, 0x82bbcdu, 0x82f846u, 2u, 0u))
        return call.result;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 3u);
    if (!PartyMenuChild(&call, 0x82bbd4u, 0x82a318u, 2u, 1u) ||
        !PartyMenuChild(&call, 0x82bbd7u, 0x82bc96u, 2u, 1u))
        return call.result;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 3u);
    OpLdx(cpu, 8u);
    if (!PartyMenuChild(&call, 0x82bbe1u, 0x82895bu, 2u, 1u))
        return call.result;
    cpu->carry = 0u;
    return ExecutionReturned(0x82bbe5u);
}

Lufia2ExecutionResult Lufia2MenuUnequipAllSlots(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    static const uint32_t sites[] = {
        0x82bbfau, 0x82bc01u, 0x82bc08u, 0x82bc0fu, 0x82bc16u, 0x82bc1du
    };
    if (!PartyMenuReady(cpu, child))
        return ExecutionHandoff(cpu, 0x82bbe8u);
    PartyMenuCall call = {memory, cpu, child, context, {0}};
    OpLoadA(cpu, 5u);
    OpLdx(cpu, 7u);
    if (!PartyMenuChild(&call, 0x82bbedu, 0x82895bu, 2u, 1u) ||
        !PartyMenuChild(&call, 0x82bbf0u, 0x868b55u, 3u, 1u))
        return call.result;
    OpRepWidths(cpu, 0x20u);
    for (unsigned slot = 0u; slot < EQUIPMENT_SLOT_COUNT; ++slot) {
        OpLoadA(cpu, (uint16_t)slot);
        cpu->carry = 1u;
        if (!PartyMenuChild(&call, sites[slot], 0x82f8dcu, 2u, 0u))
            return call.result;
    }
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_SCROLL_ROW));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW));
    if (!PartyMenuChild(&call, 0x82bc26u, 0x82bd48u, 2u, 0u) ||
        !PartyMenuChild(&call, 0x82bc29u, 0x82f846u, 2u, 0u))
        return call.result;
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, EQUIPMENT_MEMBER_POINTER)));
    if (!PartyMenuChild(&call, 0x82bc2eu, 0x81f4d5u, 3u, 0u))
        return call.result;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 3u);
    OpLdx(cpu, 7u);
    if (!PartyMenuChild(&call, 0x82bc39u, 0x82895bu, 2u, 1u))
        return call.result;
    OpLoadA(cpu, 3u);
    if (!PartyMenuChild(&call, 0x82bc3eu, 0x82a318u, 2u, 1u))
        return call.result;
    OpLoadA(cpu, 8u);
    OpTestBits(memory, cpu, OpDp(cpu, EQUIPMENT_DISPLAY_REQUEST), 1u);
    return ExecutionReturned(0x82bc45u);
}

static uint32_t EquipmentSelectedAddress(
    const Lufia2Memory *memory, const Lufia2CpuState *cpu) {
    return AbsoluteIndexedAddress(cpu,
        Read16Direct(memory, cpu, EQUIPMENT_MEMBER_POINTER), cpu->y);
}

Lufia2ExecutionResult Lufia2MenuDiscardSelectedSlot(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!PartyMenuReady(cpu, child))
        return ExecutionHandoff(cpu, 0x82bc46u);
    PartyMenuCall call = {memory, cpu, child, context, {0}};
    if (!PartyMenuChild(&call, 0x82bc46u, 0x81f194u, 3u, 1u))
        return call.result;
    OpAndValue(cpu, EQUIPMENT_DISCARD_LOCK);
    if (!cpu->zero) {
        cpu->carry = 1u;
        return ExecutionReturned(0x82bc95u);
    }
    OpLoadA(cpu, 2u);
    if (!PartyMenuChild(&call, 0x82bc50u, 0x80953bu, 3u, 1u))
        return call.result;
    OpLoadA(cpu, 5u);
    OpLdx(cpu, 8u);
    if (!PartyMenuChild(&call, 0x82bc59u, 0x82895bu, 2u, 1u) ||
        !PartyMenuChild(&call, 0x82bc5cu, 0x868b55u, 3u, 1u))
        return call.result;
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, EQUIPMENT_LIST_MARKER));
    if (!PartyMenuChild(&call, 0x82bc65u, 0x82f703u, 2u, 0u))
        return call.result;
    OpLda(memory, cpu, EquipmentSelectedAddress(memory, cpu));
    OpTax(cpu);
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, EquipmentSelectedAddress(memory, cpu));
    OpLoadA(cpu, 1u);
    if (!PartyMenuChild(&call, 0x82bc73u, 0x82fc97u, 2u, 0u))
        return call.result;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 3u);
    OpLdx(cpu, 8u);
    if (!PartyMenuChild(&call, 0x82bc7du, 0x82895bu, 2u, 1u) ||
        !PartyMenuChild(&call, 0x82bc80u, 0x82f846u, 2u, 1u))
        return call.result;
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, EQUIPMENT_MEMBER_POINTER)));
    if (!PartyMenuChild(&call, 0x82bc85u, 0x81f4d5u, 3u, 1u))
        return call.result;
    OpLoadA(cpu, 3u);
    if (!PartyMenuChild(&call, 0x82bc8bu, 0x82a318u, 2u, 1u))
        return call.result;
    OpLoadA(cpu, 8u);
    OpTestBits(memory, cpu, OpDp(cpu, EQUIPMENT_DISPLAY_REQUEST), 1u);
    cpu->carry = 0u;
    return ExecutionReturned(0x82bc93u);
}

Lufia2ExecutionResult Lufia2MenuDrawEquipmentRows(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!PartyMenuReady(cpu, child))
        return ExecutionHandoff(cpu, 0x82bc96u);
    PartyMenuCall call = {memory, cpu, child, context, {0}};
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x0482u);
    OpLdx(cpu, 0x1e0eu);
    if (!PartyMenuChild(&call, 0x82bc9eu, 0x8283ebu, 2u, ORDER_EITHER_WIDTH))
        return call.result;
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, 0x3488u);
    OpWriteX(memory, cpu, OpDp(cpu, EQUIPMENT_TEXT_DESTINATION), cpu->x);
    OpLdx(cpu, 5u);
    OpWriteX(memory, cpu, OpDp(cpu, EQUIPMENT_TEXT_ROWS), cpu->x);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW)));
    OpWriteX(memory, cpu, OpDp(cpu, EQUIPMENT_TEXT_INDEX), cpu->x);
    if (!PartyMenuChild(&call, 0x82bcb2u, 0x828579u, 2u, 1u))
        return call.result;
    return ExecutionReturned(0x82bcb5u);
}

Lufia2ExecutionResult Lufia2MenuDrawEquipmentRow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!PartyMenuReady(cpu, child))
        return ExecutionHandoff(cpu, 0x82bcb6u);
    PartyMenuCall call = {memory, cpu, child, context, {0}};
    OpLdx(cpu, 0x3708u);
    OpWriteX(memory, cpu, OpDp(cpu, EQUIPMENT_TEXT_DESTINATION), cpu->x);
    OpLdx(cpu, 1u);
    OpWriteX(memory, cpu, OpDp(cpu, EQUIPMENT_TEXT_ROWS), cpu->x);
    if (!PartyMenuChild(&call, 0x82bcc0u, 0x828579u, 2u, 1u))
        return call.result;
    return ExecutionReturned(0x82bcc3u);
}

static bool EquipmentAnimateScrollFrom(
    PartyMenuCall *call, bool down, unsigned first_frame) {
    static const uint32_t sites[2][3] = {
        {0x82bcdcu, 0x82bce5u, 0x82bceeu},
        {0x82bd22u, 0x82bd2bu, 0x82bd34u}
    };
    for (unsigned frame = first_frame; frame < 3u; ++frame) {
        OpLoadA(call->cpu, (uint16_t)((down ? 8u : 2u) * (frame ? 2u : 1u)));
        OpSta(call->memory, call->cpu, OpAbs(call->cpu, EQUIPMENT_SCROLL_REQUEST));
        if (!PartyMenuChild(call, sites[down ? 1u : 0u][frame], 0x868b55u, 3u, 1u))
            return false;
    }
    return true;
}

static void FinishEquipmentScrollRow(PartyMenuCall *call, bool down) {
    OpLoadA(call->cpu, 1u);
    OpSta(call->memory, call->cpu, OpAbs(call->cpu, EQUIPMENT_SCROLL_REQUEST));
    OpRepWidths(call->cpu, 0x20u);
    OpStepMem(call->memory, call->cpu,
        OpAbs(call->cpu, WRAM_MENU_SCROLL_ROW), down ? 1 : -1);
}

static bool EquipmentFinishScroll(PartyMenuCall *call, bool down) {
    if (!PartyMenuChild(call, down ? 0x82bd38u : 0x82bcf2u,
            down ? 0x829009u : 0x828f5eu, 2u, 1u))
        return false;
    FinishEquipmentScrollRow(call, down);
    return true;
}

Lufia2ExecutionResult Lufia2MenuScrollEquipmentUp(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!PartyMenuReady(cpu, child))
        return ExecutionHandoff(cpu, 0x82bcc4u);
    PartyMenuCall call = {memory, cpu, child, context, {0}};
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW));
    if (!cpu->zero) {
        OpStepMem(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW), -1);
        OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW)));
        OpWriteX(memory, cpu, OpDp(cpu, EQUIPMENT_TEXT_INDEX), cpu->x);
        if (!PartyMenuChild(&call, 0x82bcd4u, 0x82bcb6u, 2u, 1u) ||
            !EquipmentAnimateScrollFrom(&call, false, 0u) ||
            !EquipmentFinishScroll(&call, false))
            return call.result;
    }
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82bd01u);
}

Lufia2ExecutionResult Lufia2MenuScrollEquipmentDown(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!PartyMenuReady(cpu, child))
        return ExecutionHandoff(cpu, 0x82bd02u);
    PartyMenuCall call = {memory, cpu, child, context, {0}};
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW));
    OpCmp(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_OVERFLOW));
    if (cpu->negative) {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW));
        cpu->carry = 0u;
        OpAdcValue(cpu, 5u);
        OpSta(memory, cpu, OpDp(cpu, EQUIPMENT_TEXT_INDEX));
        OpStz(memory, cpu, OpDp(cpu, EQUIPMENT_TEXT_INDEX + 1u));
        if (!PartyMenuChild(&call, 0x82bd17u, 0x82bcb6u, 2u, 1u))
            return call.result;
        OpStepMem(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW), 1);
        if (!EquipmentAnimateScrollFrom(&call, true, 0u) ||
            !EquipmentFinishScroll(&call, true))
            return call.result;
    }
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82bd47u);
}

Lufia2ExecutionResult Lufia2MenuResumeEquipmentScrollDown(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t resume_pc,
    Lufia2PushedChildCall child, void *context) {
    if (!PartyMenuReady(cpu, child))
        return ExecutionHandoff(cpu, resume_pc);
    unsigned first_frame;
    switch (resume_pc) {
    case 0x82bd1au: first_frame = 0u; break;
    case 0x82bd26u: first_frame = 1u; break;
    case 0x82bd2fu: first_frame = 2u; break;
    case 0x82bd38u: first_frame = 3u; break;
    case 0x82bd3bu: first_frame = 3u; break;
    default: return ExecutionHandoff(cpu, resume_pc);
    }
    PartyMenuCall call = {memory, cpu, child, context, {0}};
    if (resume_pc == 0x82bd1au)
        OpStepMem(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW), 1);
    if (resume_pc == 0x82bd3bu) {
        FinishEquipmentScrollRow(&call, true);
    } else if (!EquipmentAnimateScrollFrom(&call, true, first_frame) ||
               !EquipmentFinishScroll(&call, true)) {
        return call.result;
    }
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82bd47u);
}

static Lufia2ExecutionResult FinishEquipmentRows(PartyMenuCall *call) {
    OpSepWidths(call->cpu, 0x20u);
    OpLoadA(call->cpu, 8u);
    OpTestBits(call->memory, call->cpu,
        OpDp(call->cpu, EQUIPMENT_DISPLAY_REQUEST), 1u);
    return ExecutionReturned(0x82859au);
}

static Lufia2ExecutionResult RenderEquipmentRow(PartyMenuCall *call) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    OpSta(memory, cpu, OpDp(cpu, EQUIPMENT_ROW_DESTINATION));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, EQUIPMENT_ROW_ATTRIBUTES));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    OpRepWidths(cpu, 0x20u);
    OpTxy(cpu);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, EQUIPMENT_ROW_LIST));
    OpCmpValue(cpu, EQUIPMENT_ROW_EMPTY);
    if (cpu->zero) {
        /* The sentinel discards the row call's return frame. */
        OpPullX(memory, cpu);
        return FinishEquipmentRows(call);
    }
    PushIndex(memory, cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
    OpSta(memory, cpu, OpDp(cpu, EQUIPMENT_TEXT_INDEX));
    OpLdy(cpu, 0xc9b1u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, EQUIPMENT_ROW_DESTINATION)));
    if (!PartyMenuChild(call, 0x8285bfu, 0x808878u, 3u, 0u))
        return call->result;
    OpLsrMem(memory, cpu, OpDp(cpu, EQUIPMENT_TEXT_INDEX));
    OpLdy(cpu, 0xc9b7u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, EQUIPMENT_ROW_DESTINATION)));
    if (!PartyMenuChild(call, 0x8285cau, 0x808878u, 3u, 0u))
        return call->result;
    OpPullX(memory, cpu);
    OpInx(cpu);
    OpInx(cpu);
    return ExecutionReturned(0x8285d1u);
}

Lufia2ExecutionResult Lufia2MenuRenderEquipmentListRow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->program_bank != 0x82u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal ||
        !PartyMenuBankSupported(cpu->data_bank) ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x82859bu);
    PartyMenuCall call = {memory, cpu, child, context, {0}};
    return RenderEquipmentRow(&call);
}

Lufia2ExecutionResult Lufia2MenuRenderEquipmentListRows(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!PartyMenuReady(cpu, child))
        return ExecutionHandoff(cpu, 0x828579u);
    PartyMenuCall call = {memory, cpu, child, context, {0}};
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, EQUIPMENT_STRING_BANK));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, EQUIPMENT_TEXT_INDEX));
    OpAslA(cpu);
    OpTax(cpu);
    do {
        OpLda(memory, cpu, OpDp(cpu, EQUIPMENT_TEXT_DESTINATION));
        SimulateJsrFrame(memory, cpu, 0x8587u);
        const Lufia2ExecutionResult row = RenderEquipmentRow(&call);
        if (row.flow != LUFIA2_EXECUTION_RETURNED || row.pc == 0x82859au)
            return row;
        SimulateRtsFrame(memory, cpu);
        OpLda(memory, cpu, OpDp(cpu, EQUIPMENT_TEXT_DESTINATION));
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x80u);
        OpSta(memory, cpu, OpDp(cpu, EQUIPMENT_TEXT_DESTINATION));
        OpStepMem(memory, cpu, OpDp(cpu, EQUIPMENT_TEXT_ROWS), -1);
    } while (!cpu->zero);
    return FinishEquipmentRows(&call);
}

Lufia2ExecutionResult Lufia2MenuCycleEquipmentMember(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!PartyMenuReady(cpu, child))
        return ExecutionHandoff(cpu, 0x829a14u);
    PartyMenuCall call = {memory, cpu, child, context, {0}};
    if (cpu->carry) {
        OpStepMem(memory, cpu, OpDp(cpu, EQUIPMENT_MEMBER_SLOT), -1);
        if (cpu->negative) {
            OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PARTY_MEMBER_COUNT));
            DecrementA8(cpu);
            OpSta(memory, cpu, OpDp(cpu, EQUIPMENT_MEMBER_SLOT));
        }
    } else {
        OpStepMem(memory, cpu, OpDp(cpu, EQUIPMENT_MEMBER_SLOT), 1);
        OpLda(memory, cpu, OpDp(cpu, EQUIPMENT_MEMBER_SLOT));
        OpCmp(memory, cpu, OpAbs(cpu, WRAM_MENU_PARTY_MEMBER_COUNT));
        if (!cpu->negative)
            OpStz(memory, cpu, OpDp(cpu, EQUIPMENT_MEMBER_SLOT));
    }
    OpStz(memory, cpu, OpDp(cpu, EQUIPMENT_MEMBER_SLOT + 1u));
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, EQUIPMENT_MEMBER_SLOT)));
    OpLdx(cpu, EQUIPMENT_PORTRAIT_SLOT);
    OpLda(memory, cpu, OpAbsY(cpu, MENU_SPRITE_LIST_LOW));
    OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_LIST_LOW));
    OpLda(memory, cpu, OpAbsY(cpu, MENU_SPRITE_LIST_HIGH));
    OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_LIST_HIGH));
    OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_FRAME_INDEX));
    OpLda(memory, cpu, OpAbsY(cpu, MENU_SPRITE_ANIMATION));
    if (!PartyMenuChild(&call, 0x829a46u, 0x868cf5u, 3u, 1u) ||
        !PartyMenuChild(&call, 0x829a4au, 0x829971u, 2u, 1u))
        return call.result;
    return ExecutionReturned(0x829a4du);
}

static bool EquipmentListReady(const Lufia2CpuState *cpu,
    Lufia2PushedChildCall child) {
    return child && cpu->program_bank == 0x82u && !cpu->direct_page &&
        !cpu->decimal && PartyMenuBankSupported(cpu->data_bank) &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

Lufia2ExecutionResult Lufia2MenuPrepareEquipmentList(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!EquipmentListReady(cpu, child))
        return ExecutionHandoff(cpu, 0x82be04u);
    PartyMenuCall call = {memory, cpu, child, context, {0}};
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    OpAndValue(cpu, 0xffu);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, EQUIPMENT_GROUP_POINTERS));
    OpSta(memory, cpu, OpDp(cpu, EQUIPMENT_ROW_LIST));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, EQUIPMENT_ROW_LIST + 2u));
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, EQUIPMENT_MEMBER_SLOT)));
    if (!PartyMenuChild(&call, 0x82be1au, 0x829302u, 2u, 1u))
        return call.result;
    OpSta(memory, cpu, OpDp(cpu, EQUIPMENT_MEMBER_MASK));
    OpStz(memory, cpu, OpDp(cpu, EQUIPMENT_MEMBER_MASK + 1u));
    OpRepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpDp(cpu, EQUIPMENT_GROUP_OFFSET));
    for (;;) {
        OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, EQUIPMENT_GROUP_OFFSET)));
        OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, EQUIPMENT_ROW_LIST));
        OpCmpValue(cpu, EQUIPMENT_ROW_EMPTY);
        if (cpu->zero)
            break;
        OpTax(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
        OpSta(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
        if (!PartyMenuChild(&call, 0x82be35u, 0x81f1abu, 3u, 0u))
            return call.result;
        OpLda(memory, cpu, OpAbs(cpu, EQUIPMENT_ITEM_MEMBER_MASK));
        OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, EQUIPMENT_MEMBER_MASK)));
        OpLoadA(cpu, cpu->zero ? EQUIPMENT_ROW_UNAVAILABLE : EQUIPMENT_ROW_AVAILABLE);
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, EQUIPMENT_GROUP_OFFSET)));
        OpSta(memory, cpu, OpLongX(cpu, EQUIPMENT_ROW_ATTRIBUTES));
        OpStepMem(memory, cpu, OpDp(cpu, EQUIPMENT_GROUP_OFFSET), 1);
        OpStepMem(memory, cpu, OpDp(cpu, EQUIPMENT_GROUP_OFFSET), 1);
    }
    OpLda(memory, cpu, OpDp(cpu, EQUIPMENT_GROUP_OFFSET));
    OpLsrA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_COUNT));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_OVERFLOW));
    cpu->carry = 1u;
    OpSbcValue(cpu, 5u);
    if (cpu->carry)
        OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_OVERFLOW));
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_COUNT)));
    cpu->carry = 0u;
    if (!PartyMenuChild(&call, 0x82be6cu, 0x828c85u, 2u, 1u))
        return call.result;
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x82be70u);
}

Lufia2ExecutionResult Lufia2MenuLoadEquipmentSlotStats(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!EquipmentListReady(cpu, child))
        return ExecutionHandoff(cpu, 0x82be71u);
    PartyMenuCall call = {memory, cpu, child, context, {0}};
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, EQUIPMENT_MEMBER_POINTER)));
    for (unsigned stat = 0u; stat < EQUIPMENT_STAT_WORDS; ++stat)
        OpStz(memory, cpu,
            OpAbsX(cpu, (uint16_t)(EQUIPMENT_CHARACTER_STATS + stat * 2u)));
    for (unsigned stat = 0u; stat < EQUIPMENT_STAT_WORDS; ++stat)
        OpStz(memory, cpu,
            OpAbs(cpu, (uint16_t)(EQUIPMENT_RECORD_STATS + stat * 2u)));
    OpLda(memory, cpu, EquipmentSelectedAddress(memory, cpu));
    if (!cpu->zero) {
        OpSta(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
        if (!PartyMenuChild(&call, 0x82bea7u, 0x81f1c5u, 3u, 0u))
            return call.result;
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, EQUIPMENT_MEMBER_POINTER)));
        for (unsigned stat = 0u; stat < EQUIPMENT_STAT_WORDS; ++stat) {
            OpLda(memory, cpu,
                OpAbs(cpu, (uint16_t)(EQUIPMENT_RECORD_STATS + stat * 2u)));
            OpSta(memory, cpu,
                OpAbsX(cpu, (uint16_t)(EQUIPMENT_CHARACTER_STATS + stat * 2u)));
        }
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x82bed8u);
}

Lufia2ExecutionResult Lufia2MenuResolveEquipmentListSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!EquipmentListReady(cpu, child) || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x82bef0u);
    PartyMenuCall call = {memory, cpu, child, context, {0}};
    OpTxy(cpu);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, EQUIPMENT_ROW_LIST));
    OpTax(cpu);
    OpLda(memory, cpu, OpAbs(cpu, EQUIPMENT_LIST_MARKER));
    if (!PartyMenuChild(&call, 0x82bef7u, 0x82f703u, 2u, 0u))
        return call.result;
    return ExecutionReturned(0x82befau);
}

Lufia2ExecutionResult Lufia2MenuClassifyEquipmentListSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!PartyMenuReady(cpu, child))
        return ExecutionHandoff(cpu, 0x82bed9u);
    PartyMenuCall call = {memory, cpu, child, context, {0}};
    OpRepWidths(cpu, 0x20u);
    if (!PartyMenuChild(&call, 0x82bedbu, 0x82fbe5u, 2u, 0u))
        return call.result;
    OpTax(cpu);
    OpLsrA(cpu);
    OpCmp(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_COUNT));
    unsigned status = 2u;
    uint32_t return_pc = 0x82bf12u;
    if (!cpu->carry) {
        OpLda(memory, cpu, OpLongX(cpu, EQUIPMENT_ROW_ATTRIBUTES));
        OpCmpValue(cpu, EQUIPMENT_ROW_AVAILABLE);
        const bool available = cpu->zero;
        if (!PartyMenuChild(&call, available ? 0x82befbu : 0x82bf04u,
                0x82bef0u, 2u, 0u))
            return call.result;
        status = available ? 0u : 1u;
        return_pc = available ? 0x82bf03u : 0x82bf0cu;
    }
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, (uint16_t)status);
    cpu->carry = status != 0u;
    return ExecutionReturned(return_pc);
}

enum {
    EQUIPMENT_PICKER_LAYOUT = 0x23u,
    EQUIPMENT_PICKER_SHAPE = 0x0401u,
    EQUIPMENT_SLOT_PICKER_CURSOR = 8u,
    EQUIPMENT_ITEM_PICKER_CURSOR = 9u
};

enum {
    EQUIPMENT_INPUT_MOVE = 1u,
    EQUIPMENT_INPUT_ACCEPT = 2u,
    EQUIPMENT_INPUT_CANCEL = 3u,
    EQUIPMENT_INPUT_DESCRIPTION = 4u,
    EQUIPMENT_INPUT_PREVIOUS_MEMBER = 6u,
    EQUIPMENT_INPUT_NEXT_MEMBER = 7u,
    EQUIPMENT_INPUT_SCROLL_UP = 10u,
    EQUIPMENT_INPUT_SCROLL_DOWN = 11u,
    EQUIPMENT_SLOT_ACTION = WRAM_MENU_SPELL_WINDOW_STATE_2_ALT,
    EQUIPMENT_REMOVE_ACTION = 2u,
    EQUIPMENT_DISCARD_ACTION = 4u,
    EQUIPMENT_AUXILIARY_CURSOR = 11u,
    EQUIPMENT_ITEM_NUMBER_MASK = 0x01ffu,
    EQUIPMENT_DESCRIPTION_SCRIPT = 0x020fu
};

enum {
    EQUIPMENT_STRONGEST_SLOT = 0x0eu,
    EQUIPMENT_STRONGEST_ROW = 0x1fu,
    EQUIPMENT_STRONGEST_LIMIT = 8u,
    EQUIPMENT_RECORD_SLOTS = 0x66u,
    EQUIPMENT_BEST_OFFSETS = 0x7e9db2u,
    EQUIPMENT_SCORE_HANDLERS = 0x82bb66u,
    EQUIPMENT_SCORE_TARGET = 0x08u
};

Lufia2ExecutionResult Lufia2MenuConfigureEquipmentPicker(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!PartyMenuReady(cpu, child))
        return ExecutionHandoff(cpu, 0x829ffbu);
    PartyMenuCall call = {memory, cpu, child, context, {0}};
    OpLdx(cpu, EQUIPMENT_PICKER_LAYOUT);
    OpWriteX(memory, cpu, OpDp(cpu, EQUIPMENT_SELECTION_TABLE), cpu->x);
    OpLdx(cpu, EQUIPMENT_PICKER_SHAPE);
    OpWriteX(memory, cpu, OpDp(cpu, EQUIPMENT_SELECTION_SIZE), cpu->x);
    OpLdx(cpu, EQUIPMENT_SLOT_PICKER_CURSOR);
    OpLdy(cpu, EQUIPMENT_ITEM_PICKER_CURSOR);
    if (!PartyMenuChild(&call, 0x82a00bu, 0x8289ecu, 2u, 1u))
        return call.result;
    return ExecutionReturned(0x82a00eu);
}

Lufia2ExecutionResult Lufia2MenuRefreshEquipmentPicker(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!PartyMenuReady(cpu, child))
        return ExecutionHandoff(cpu, 0x82ba0cu);
    PartyMenuCall call = {memory, cpu, child, context, {0}};
    OpLoadA(cpu, 3u);
    OpLdx(cpu, EQUIPMENT_ITEM_PICKER_CURSOR);
    OpLdy(cpu, EQUIPMENT_SLOT_PICKER_CURSOR);
    if (!PartyMenuChild(&call, 0x82ba14u, 0x8289bcu, 2u, 1u))
        return call.result;
    OpLoadA(cpu, 3u);
    if (!PartyMenuChild(&call, 0x82ba19u, 0x82a318u, 2u, 1u))
        return call.result;
    return ExecutionReturned(0x82ba1cu);
}

Lufia2ExecutionResult Lufia2MenuPrepareEquipmentPreview(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!EquipmentListReady(cpu, child))
        return ExecutionHandoff(cpu, 0x82f864u);
    PartyMenuCall call = {memory, cpu, child, context, {0}};
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    PushAccumulator16(memory, cpu);
    if (!PartyMenuChild(&call, 0x82f868u, 0x81f1c5u, 3u, 0u) ||
        !PartyMenuChild(&call, 0x82f86cu, 0x82f76bu, 2u, 0u))
        return call.result;
    PullAccumulator16(memory, cpu);
    if (!PartyMenuChild(&call, 0x82f870u, 0x82f703u, 2u, 0u) ||
        !PartyMenuChild(&call, 0x82f873u, 0x82f710u, 2u, 0u))
        return call.result;
    if (!cpu->carry && !PartyMenuChild(&call, 0x82f878u, 0x82f7fdu, 2u, 0u))
        return call.result;
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x82f87cu);
}

static bool RefreshEquipmentSlotRows(PartyMenuCall *call,
    uint32_t prepare_site, uint32_t draw_site) {
    OpLda(call->memory, call->cpu, OpAbs(call->cpu, EQUIPMENT_LIST_MARKER));
    return PartyMenuChild(call, prepare_site, 0x82be04u, 2u, 1u) &&
        PartyMenuChild(call, draw_site, 0x82bc96u, 2u, 1u);
}

static bool CycleEquipmentPickerMember(PartyMenuCall *call,
    uint32_t member_site, uint32_t refresh_site) {
    OpLoadA(call->cpu, (uint16_t)(OpA(call->cpu) ^ 1u));
    OpLsrA(call->cpu);
    if (!PartyMenuChild(call, member_site, 0x829a14u, 2u, 1u))
        return false;
    OpLoadA(call->cpu, 3u);
    return PartyMenuChild(call, refresh_site, 0x82a318u, 2u, 1u);
}

Lufia2ExecutionResult Lufia2MenuRunEquipmentSlots(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!PartyMenuReady(cpu, child))
        return ExecutionHandoff(cpu, 0x82b863u);
    PartyMenuCall call = {memory, cpu, child, context, {0}};
    OpLoadA(cpu, 0u);
    for (;;) {
        if (!PartyMenuChild(&call, 0x82b865u, 0x828b08u, 2u, 1u))
            return call.result;
        const uint16_t command = OpA(cpu);
        const bool known = command == EQUIPMENT_INPUT_MOVE ||
            command == EQUIPMENT_INPUT_ACCEPT || command == EQUIPMENT_INPUT_CANCEL ||
            command == EQUIPMENT_INPUT_DESCRIPTION ||
            command == EQUIPMENT_INPUT_PREVIOUS_MEMBER ||
            command == EQUIPMENT_INPUT_NEXT_MEMBER;
        /* Only the final comparison reaches another routine. */
        OpCmpValue(cpu, known ? command : EQUIPMENT_INPUT_NEXT_MEMBER);
        switch (command) {
        case EQUIPMENT_INPUT_MOVE:
            OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PRESSED_B));
            OpAndValue(cpu, 0x0cu);
            if (!cpu->zero) {
                OpLdx(cpu, 0u);
                OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_SCROLL_ROW), cpu->x);
                OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW), cpu->x);
                if (!RefreshEquipmentSlotRows(&call, 0x82b89bu, 0x82b89eu))
                    return call.result;
            }
            OpLoadA(cpu, 1u);
            break;
        case EQUIPMENT_INPUT_ACCEPT: {
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpAbs(cpu, EQUIPMENT_LIST_MARKER));
            if (!PartyMenuChild(&call, 0x82b8a8u, 0x82f893u, 2u, 0u))
                return call.result;
            OpSepWidths(cpu, 0x20u);
            bool rejected = cpu->carry;
            if (!rejected) {
                OpLda(memory, cpu, OpAbs(cpu, EQUIPMENT_SLOT_ACTION));
                OpCmpValue(cpu, EQUIPMENT_REMOVE_ACTION);
                if (cpu->zero) {
                    if (!PartyMenuChild(&call, 0x82b8bfu, 0x82bb91u, 2u, 1u))
                        return call.result;
                    rejected = cpu->carry;
                } else {
                    OpCmpValue(cpu, EQUIPMENT_DISCARD_ACTION);
                    if (cpu->zero) {
                        if (!PartyMenuChild(&call, 0x82b8c6u, 0x82bc46u, 2u, 1u))
                            return call.result;
                        rejected = cpu->carry;
                    } else if (!PartyMenuChild(&call, 0x82b8bau, 0x82b907u, 2u, 1u)) {
                        return call.result;
                    }
                }
            }
            if (rejected && !PartyMenuChild(&call, 0x82b8cdu, 0x8297e3u, 2u, 1u))
                return call.result;
            OpLoadA(cpu, 1u);
            break;
        }
        case EQUIPMENT_INPUT_CANCEL:
            OpStz(memory, cpu,
                OpAbs(cpu, MENU_SPRITE_ACTIVE + EQUIPMENT_SLOT_PICKER_CURSOR));
            OpStz(memory, cpu,
                OpAbs(cpu, MENU_SPRITE_ACTIVE + EQUIPMENT_AUXILIARY_CURSOR));
            return ExecutionReturned(0x82b8d8u);
        case EQUIPMENT_INPUT_PREVIOUS_MEMBER:
        case EQUIPMENT_INPUT_NEXT_MEMBER:
            if (!CycleEquipmentPickerMember(&call, 0x82b8dcu, 0x82b8e1u) ||
                !RefreshEquipmentSlotRows(&call, 0x82b89bu, 0x82b89eu))
                return call.result;
            OpLoadA(cpu, 1u);
            break;
        case EQUIPMENT_INPUT_DESCRIPTION:
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpAbs(cpu, EQUIPMENT_LIST_MARKER));
            if (!PartyMenuChild(&call, 0x82b8ecu, 0x82f703u, 2u, 0u))
                return call.result;
            OpLda(memory, cpu, EquipmentSelectedAddress(memory, cpu));
            OpAndValue(cpu, EQUIPMENT_ITEM_NUMBER_MASK);
            OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DESCRIPTION_ID));
            OpSepWidths(cpu, 0x20u);
            if (!cpu->zero) {
                OpLdx(cpu, 0u);
                OpLdy(cpu, EQUIPMENT_DESCRIPTION_SCRIPT);
                if (!PartyMenuChild(&call, 0x82b901u, 0x82fd81u, 2u, 1u))
                    return call.result;
            }
            OpLoadA(cpu, 0u);
            break;
        default:
            OpLoadA(cpu, 0u);
            break;
        }
    }
}

static bool PreviewEquipmentChoice(PartyMenuCall *call) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    OpLdx(cpu, 4u);
    if (!PartyMenuChild(call, 0x82b953u, 0x8288a0u, 2u, 1u) ||
        !PartyMenuChild(call, 0x82b956u, 0x82bed9u, 2u, 1u))
        return false;
    if (!cpu->carry) {
        OpTxy(cpu);
        OpLdx(cpu, OpReadX(memory, cpu, OpAbsY(cpu, WRAM_INVENTORY_PACKED_ITEMS)));
        OpWriteX(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID), cpu->x);
        OpLda(memory, cpu, OpAbs(cpu, EQUIPMENT_LIST_MARKER));
        if (!PartyMenuChild(call, 0x82b965u, 0x82f864u, 2u, 1u))
            return false;
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, EQUIPMENT_MEMBER_POINTER)));
        if (!PartyMenuChild(call, 0x82b96au, 0x81f481u, 3u, 1u))
            return false;
        OpLoadA(cpu, 0x30u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
        OpLdx(cpu, WRAM_MENU_EQUIPMENT_CURSOR);
        OpWriteX(memory, cpu, OpDp(cpu, 0xe7u), cpu->x);
        OpLoadA(cpu, 0x8eu);
        OpSta(memory, cpu, OpDp(cpu, EQUIPMENT_STRING_BANK));
        OpLdy(cpu, 0xc50cu);
        if (!PartyMenuChild(call, 0x82b97fu, 0x808878u, 3u, 1u))
            return false;
    } else {
        OpLoadA(cpu, 0x30u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
        OpLoadA(cpu, 0x8eu);
        OpSta(memory, cpu, OpDp(cpu, EQUIPMENT_STRING_BANK));
        OpLdy(cpu, 0xc3ccu);
        if (!PartyMenuChild(call, 0x82b991u, 0x808878u, 3u, 1u))
            return false;
    }
    OpLoadA(cpu, 8u);
    OpTestBits(memory, cpu, OpDp(cpu, EQUIPMENT_DISPLAY_REQUEST), 1u);
    return true;
}

static bool CloseEquipmentPicker(PartyMenuCall *call) {
    OpLoadA(call->cpu, 0u);
    OpLdx(call->cpu, EQUIPMENT_ITEM_PICKER_CURSOR);
    OpLdy(call->cpu, EQUIPMENT_SLOT_PICKER_CURSOR);
    if (!PartyMenuChild(call, 0x82ba01u, 0x8289fau, 2u, 1u) ||
        !PartyMenuChild(call, 0x82ba04u, 0x82ba0cu, 2u, 1u))
        return false;
    OpLoadA(call->cpu, 8u);
    OpTestBits(call->memory, call->cpu,
        OpDp(call->cpu, EQUIPMENT_DISPLAY_REQUEST), 1u);
    return true;
}

static bool ApplyEquipmentChoice(PartyMenuCall *call, bool *applied) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    *applied = false;
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, EQUIPMENT_LIST_MARKER));
    if (!PartyMenuChild(call, 0x82b9a1u, 0x82f893u, 2u, 0u))
        return false;
    OpSepWidths(cpu, 0x20u);
    if (cpu->carry)
        return true;
    if (!PartyMenuChild(call, 0x82b9a8u, 0x82bed9u, 2u, 1u))
        return false;
    OpCmpValue(cpu, 1u);
    if (cpu->zero)
        return true;
    OpCmpValue(cpu, 2u);
    if (!cpu->zero) {
        OpRepWidths(cpu, 0x20u);
        if (!PartyMenuChild(call, 0x82b9b5u, 0x82f925u, 2u, 0u))
            return false;
        OpSepWidths(cpu, 0x20u);
    } else {
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbs(cpu, EQUIPMENT_LIST_MARKER));
        cpu->carry = 1u;
        if (!PartyMenuChild(call, 0x82b9c4u, 0x82f8dcu, 2u, 0u))
            return false;
        OpSepWidths(cpu, 0x20u);
    }
    if (cpu->carry)
        return true;
    OpLoadA(cpu, 2u);
    if (!PartyMenuChild(call, 0x82b9cdu, 0x80953bu, 3u, 1u) ||
        !PartyMenuChild(call, 0x82b9d1u, 0x82f846u, 2u, 1u))
        return false;
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, EQUIPMENT_MEMBER_POINTER)));
    if (!PartyMenuChild(call, 0x82b9d6u, 0x81f4d5u, 3u, 1u) ||
        !PartyMenuChild(call, 0x82b9dau, 0x82bd48u, 2u, 1u))
        return false;
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_SCROLL_ROW), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW), cpu->x);
    OpLda(memory, cpu, OpAbs(cpu, EQUIPMENT_LIST_MARKER));
    if (!PartyMenuChild(call, 0x82b9e9u, 0x82be04u, 2u, 1u) ||
        !PartyMenuChild(call, 0x82b9ecu, 0x82ba0cu, 2u, 1u) ||
        !PartyMenuChild(call, 0x82b9efu, 0x82bc96u, 2u, 1u))
        return false;
    *applied = true;
    return true;
}

static bool DescribeEquipmentChoice(PartyMenuCall *call) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    if (!PartyMenuChild(call, 0x82ba2cu, 0x82bed9u, 2u, 1u))
        return false;
    OpCmpValue(cpu, 2u);
    if (cpu->zero)
        return true;
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
    OpAndValue(cpu, EQUIPMENT_ITEM_NUMBER_MASK);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DESCRIPTION_ID));
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, 0u);
    OpLdy(cpu, EQUIPMENT_DESCRIPTION_SCRIPT);
    return PartyMenuChild(call, 0x82ba46u, 0x82fd81u, 2u, 1u);
}

static Lufia2ExecutionResult RunEquipmentChoiceInput(
    PartyMenuCall *call, bool command_ready) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    bool preview = !command_ready;
    for (;;) {
        if (!command_ready) {
            if (preview) {
                if (!PreviewEquipmentChoice(call))
                    return call->result;
                OpLoadA(cpu, 1u);
                preview = false;
            }
            if (!PartyMenuChild(call, 0x82b916u, 0x828b08u, 2u, 1u))
                return call->result;
        }
        command_ready = false;
        const uint16_t command = OpA(cpu);
        const bool known = command == EQUIPMENT_INPUT_MOVE ||
            command == EQUIPMENT_INPUT_ACCEPT || command == EQUIPMENT_INPUT_CANCEL ||
            command == EQUIPMENT_INPUT_DESCRIPTION ||
            command == EQUIPMENT_INPUT_PREVIOUS_MEMBER ||
            command == EQUIPMENT_INPUT_NEXT_MEMBER ||
            command == EQUIPMENT_INPUT_SCROLL_UP || command == EQUIPMENT_INPUT_SCROLL_DOWN;
        OpCmpValue(cpu, known ? command : EQUIPMENT_INPUT_SCROLL_DOWN);
        switch (command) {
        case EQUIPMENT_INPUT_MOVE:
            preview = true;
            break;
        case EQUIPMENT_INPUT_ACCEPT: {
            bool applied;
            if (!ApplyEquipmentChoice(call, &applied))
                return call->result;
            if (applied)
                return ExecutionReturned(0x82b9f2u);
            if (!PartyMenuChild(call, 0x82b9f3u, 0x8297e3u, 2u, 1u))
                return call->result;
            OpLoadA(cpu, 1u);
            break;
        }
        case EQUIPMENT_INPUT_CANCEL:
            if (!CloseEquipmentPicker(call))
                return call->result;
            return ExecutionReturned(0x82ba0bu);
        case EQUIPMENT_INPUT_DESCRIPTION:
            if (!DescribeEquipmentChoice(call))
                return call->result;
            OpLoadA(cpu, 0u);
            break;
        case EQUIPMENT_INPUT_PREVIOUS_MEMBER:
        case EQUIPMENT_INPUT_NEXT_MEMBER:
            if (!CycleEquipmentPickerMember(call, 0x82ba4fu, 0x82ba54u) ||
                !RefreshEquipmentSlotRows(call, 0x82ba5au, 0x82ba5du))
                return call->result;
            OpStz(memory, cpu, OpDp(cpu, EQUIPMENT_DISPLAY_REQUEST));
            preview = true;
            break;
        case EQUIPMENT_INPUT_SCROLL_UP:
        case EQUIPMENT_INPUT_SCROLL_DOWN: {
            const bool down = command == EQUIPMENT_INPUT_SCROLL_DOWN;
            if (!PartyMenuChild(call, down ? 0x82ba23u : 0x82ba1du,
                    down ? 0x82bd02u : 0x82bcc4u, 2u, 1u) ||
                !PartyMenuChild(call, 0x82ba26u, 0x828c43u, 2u, 1u))
                return call->result;
            preview = true;
            break;
        }
        default:
            OpLoadA(cpu, 0u);
            break;
        }
    }
}

Lufia2ExecutionResult Lufia2MenuChooseEquipmentItem(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!PartyMenuReady(cpu, child))
        return ExecutionHandoff(cpu, 0x82b907u);
    PartyMenuCall call = {memory, cpu, child, context, {0}};
    OpLoadA(cpu, 2u);
    if (!PartyMenuChild(&call, 0x82b909u, 0x80953bu, 3u, 1u) ||
        !PartyMenuChild(&call, 0x82b90du, 0x829ffbu, 2u, 1u) ||
        !PartyMenuChild(&call, 0x82b910u, 0x82f846u, 2u, 1u))
        return call.result;
    return RunEquipmentChoiceInput(&call, false);
}

Lufia2ExecutionResult Lufia2MenuResumeEquipmentChoice(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t resume_pc,
    Lufia2PushedChildCall child, void *context) {
    if (!PartyMenuReady(cpu, child) ||
        (resume_pc != 0x82b910u && resume_pc != 0x82b919u))
        return ExecutionHandoff(cpu, resume_pc);
    PartyMenuCall call = {memory, cpu, child, context, {0}};
    if (resume_pc == 0x82b910u &&
        !PartyMenuChild(&call, 0x82b910u, 0x82f846u, 2u, 1u))
        return call.result;
    return RunEquipmentChoiceInput(&call, resume_pc == 0x82b919u);
}

static void UpdateStrongestEquipmentCandidate(PartyMenuCall *call, bool primary) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    const uint16_t column = primary ? 0u : 2u;
    OpLda(memory, cpu, OpAbsY(cpu, (uint16_t)(EQUIPMENT_CHARACTER_STATS + column)));
    OpCmp(memory, cpu, OpAbs(cpu, (uint16_t)(EQUIPMENT_RECORD_STATS + column)));
    if (cpu->carry)
        return;
    OpLda(memory, cpu, OpAbs(cpu, (uint16_t)(EQUIPMENT_RECORD_STATS + column)));
    OpSta(memory, cpu, OpAbsY(cpu, (uint16_t)(EQUIPMENT_CHARACTER_STATS + column)));
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, EQUIPMENT_STRONGEST_ROW)));
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, EQUIPMENT_ROW_LIST));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, EQUIPMENT_STRONGEST_SLOT)));
    OpSta(memory, cpu, OpLongX(cpu, EQUIPMENT_BEST_OFFSETS));
}

static bool FindStrongestEquipmentItem(PartyMenuCall *call) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    OpLda(memory, cpu, OpDp(cpu, EQUIPMENT_STRONGEST_SLOT));
    cpu->carry = 0u;
    OpAdcValue(cpu, EQUIPMENT_RECORD_SLOTS);
    OpTay(cpu);
    if (!PartyMenuChild(call, 0x82ba98u, 0x82be71u, 2u, 0u) ||
        !PartyMenuChild(call, 0x82ba9bu, 0x82bb6eu, 2u, 0u))
        return false;
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, EQUIPMENT_MEMBER_POINTER)));
    OpLda(memory, cpu, OpAbs(cpu, EQUIPMENT_RECORD_STATS));
    OpSta(memory, cpu, OpAbsX(cpu, EQUIPMENT_CHARACTER_STATS));
    OpLda(memory, cpu, OpAbs(cpu, EQUIPMENT_RECORD_STATS + 2u));
    OpSta(memory, cpu, OpAbsX(cpu, EQUIPMENT_CHARACTER_STATS + 2u));
    OpLda(memory, cpu, OpDp(cpu, EQUIPMENT_STRONGEST_SLOT));
    OpLsrA(cpu);
    if (!PartyMenuChild(call, 0x82baafu, 0x82be04u, 2u, 0u))
        return false;
    for (;;) {
        OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, EQUIPMENT_STRONGEST_ROW)));
        OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, EQUIPMENT_ROW_LIST));
        OpCmpValue(cpu, EQUIPMENT_ROW_EMPTY);
        if (cpu->zero)
            return true;
        OpTay(cpu);
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, EQUIPMENT_STRONGEST_ROW)));
        OpLda(memory, cpu, OpLongX(cpu, EQUIPMENT_ROW_ATTRIBUTES));
        OpCmpValue(cpu, EQUIPMENT_ROW_AVAILABLE);
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbsY(cpu, WRAM_INVENTORY_PACKED_ITEMS));
            OpSta(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
            if (!PartyMenuChild(call, 0x82bacdu, 0x81f1c5u, 3u, 0u) ||
                !PartyMenuChild(call, 0x82bad1u, 0x82bb6eu, 2u, 0u))
                return false;
            OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, EQUIPMENT_MEMBER_POINTER)));
            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, EQUIPMENT_STRONGEST_SLOT)));
            OpLda(memory, cpu, OpLongX(cpu, EQUIPMENT_SCORE_HANDLERS));
            OpSta(memory, cpu, OpDp(cpu, EQUIPMENT_SCORE_TARGET));
            const uint32_t target = 0x820000u | OpRead16(memory, EQUIPMENT_SCORE_TARGET);
            if (target != 0x82bb3bu && target != 0x82bb4bu) {
                call->result = ExecutionHandoff(cpu, target);
                return false;
            }
            UpdateStrongestEquipmentCandidate(call, target == 0x82bb3bu);
        }
        OpStepMem(memory, cpu, OpDp(cpu, EQUIPMENT_STRONGEST_ROW), 1);
        OpStepMem(memory, cpu, OpDp(cpu, EQUIPMENT_STRONGEST_ROW), 1);
    }
}

Lufia2ExecutionResult Lufia2MenuEquipStrongestItems(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!PartyMenuReady(cpu, child))
        return ExecutionHandoff(cpu, 0x82ba65u);
    PartyMenuCall call = {memory, cpu, child, context, {0}};
    OpLoadA(cpu, 5u);
    OpLdx(cpu, 7u);
    if (!PartyMenuChild(&call, 0x82ba6au, 0x82895bu, 2u, 1u) ||
        !PartyMenuChild(&call, 0x82ba6du, 0x868b55u, 3u, 1u))
        return call.result;
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_SCROLL_ROW), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW), cpu->x);
    OpStz(memory, cpu, OpDp(cpu, EQUIPMENT_STRONGEST_SLOT));
    do {
        OpStz(memory, cpu, OpDp(cpu, EQUIPMENT_STRONGEST_ROW));
        OpLoadA(cpu, EQUIPMENT_ROW_EMPTY);
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, EQUIPMENT_STRONGEST_SLOT)));
        OpSta(memory, cpu, OpLongX(cpu, EQUIPMENT_BEST_OFFSETS));
        OpLda(memory, cpu, OpDp(cpu, EQUIPMENT_STRONGEST_SLOT));
        OpLsrA(cpu);
        if (!PartyMenuChild(&call, 0x82ba8cu, 0x82f893u, 2u, 0u))
            return call.result;
        if (!cpu->carry && !FindStrongestEquipmentItem(&call))
            return call.result;
        OpStepMem(memory, cpu, OpDp(cpu, EQUIPMENT_STRONGEST_SLOT), 1);
        OpStepMem(memory, cpu, OpDp(cpu, EQUIPMENT_STRONGEST_SLOT), 1);
        OpLda(memory, cpu, OpDp(cpu, EQUIPMENT_STRONGEST_SLOT));
        OpCmpValue(cpu, EQUIPMENT_STRONGEST_LIMIT);
    } while (!cpu->zero);
    OpLdy(cpu, 0u);
    do {
        OpTyx(cpu);
        OpLda(memory, cpu, OpLongX(cpu, EQUIPMENT_BEST_OFFSETS));
        OpCmpValue(cpu, EQUIPMENT_ROW_EMPTY);
        if (!cpu->zero) {
            PushY(memory, cpu);
            OpTax(cpu);
            OpTya(cpu);
            cpu->carry = 0u;
            OpAdcValue(cpu, EQUIPMENT_RECORD_SLOTS);
            OpTay(cpu);
            if (!PartyMenuChild(&call, 0x82bb07u, 0x82f925u, 2u, 0u))
                return call.result;
            OpPullY(memory, cpu);
        }
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, EQUIPMENT_STRONGEST_LIMIT);
    } while (!cpu->zero);
    if (!PartyMenuChild(&call, 0x82bb12u, 0x82f846u, 2u, 0u))
        return call.result;
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, EQUIPMENT_MEMBER_POINTER)));
    if (!PartyMenuChild(&call, 0x82bb17u, 0x81f4d5u, 3u, 0u) ||
        !PartyMenuChild(&call, 0x82bb1bu, 0x82bd48u, 2u, 0u) ||
        !PartyMenuChild(&call, 0x82bb1eu, 0x82f846u, 2u, 0u))
        return call.result;
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, EQUIPMENT_AUXILIARY_CURSOR);
    OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ACTIVE));
    OpLoadA(cpu, 3u);
    OpLdx(cpu, 7u);
    if (!PartyMenuChild(&call, 0x82bb2eu, 0x82895bu, 2u, 1u))
        return call.result;
    OpLoadA(cpu, 3u);
    if (!PartyMenuChild(&call, 0x82bb33u, 0x82a318u, 2u, 1u))
        return call.result;
    OpLoadA(cpu, 8u);
    OpTestBits(memory, cpu, OpDp(cpu, EQUIPMENT_DISPLAY_REQUEST), 1u);
    return ExecutionReturned(0x82bb3au);
}
