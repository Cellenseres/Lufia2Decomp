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
    EQUIPMENT_SLOT_COUNT = 6u
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

static bool EquipmentAnimateScroll(PartyMenuCall *call, bool down) {
    static const uint32_t sites[2][3] = {
        {0x82bcdcu, 0x82bce5u, 0x82bceeu},
        {0x82bd22u, 0x82bd2bu, 0x82bd34u}
    };
    for (unsigned frame = 0u; frame < 3u; ++frame) {
        OpLoadA(call->cpu, (uint16_t)((down ? 8u : 2u) * (frame ? 2u : 1u)));
        OpSta(call->memory, call->cpu, OpAbs(call->cpu, EQUIPMENT_SCROLL_REQUEST));
        if (!PartyMenuChild(call, sites[down ? 1u : 0u][frame], 0x868b55u, 3u, 1u))
            return false;
    }
    return true;
}

static bool EquipmentFinishScroll(PartyMenuCall *call, bool down) {
    if (!PartyMenuChild(call, down ? 0x82bd38u : 0x82bcf2u,
            down ? 0x829009u : 0x828f5eu, 2u, 1u))
        return false;
    OpLoadA(call->cpu, 1u);
    OpSta(call->memory, call->cpu, OpAbs(call->cpu, EQUIPMENT_SCROLL_REQUEST));
    OpRepWidths(call->cpu, 0x20u);
    OpStepMem(call->memory, call->cpu,
        OpAbs(call->cpu, WRAM_MENU_SCROLL_ROW), down ? 1 : -1);
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
            !EquipmentAnimateScroll(&call, false) ||
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
        if (!EquipmentAnimateScroll(&call, true) ||
            !EquipmentFinishScroll(&call, true))
            return call.result;
    }
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82bd47u);
}
