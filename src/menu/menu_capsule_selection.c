#include "lufia2/menu_capsule_selection.h"
#include "core/snes_registers.h"
#include "menu/menu_screen_child_internal.h"
#include "menu/menu_sprite_slots.h"
#include "system/wram.h"

enum {
    CAPSULE_INVENTORY_BACKUP = 0x8000u,
    CAPSULE_TEXT_BANK = 0x5fu,
    CAPSULE_TEXT_PARAMETER_BANK = 0x0au,
    CAPSULE_TEXT_PARAMETER_ADDRESS = 0x08u,
    CAPSULE_TYPE_CURSOR = 0x14deu,
    CAPSULE_FORM_CURSOR = 0x14f0u,
    CAPSULE_LAST_FORM = 5u,
    CAPSULE_TYPE_COUNT = WRAM_CAPSULE_FORM_STATES_COUNT,
    CAPSULE_SELECTION_SCRATCH = 0u,
    CAPSULE_REFRESH_FLAGS = 0x74u
};

static bool CapsuleSelectionChild(MenuActionCall *call, uint32_t site,
                                   uint32_t target, uint8_t frame, uint8_t width) {
    return MenuScreenChild(call, site, target, frame, width, 0u);
}

Lufia2ExecutionResult Lufia2MenuBackupInventory(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x82u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x82fba7u);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x7eu);
    Push8(memory, cpu, A8(cpu));
    PullDataBank(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
        OpSta(memory, cpu, OpAbsX(cpu, CAPSULE_INVENTORY_BACKUP));
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, WRAM_INVENTORY_PACKED_ITEMS_COUNT * 2u);
    } while (!cpu->zero);
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x82fbc5u);
}

static void PlaceFeedingCursor(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                uint16_t slot) {
    OpLdx(cpu, slot);
    OpLoadA(cpu, 0x36u);
    OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_X_LOW));
    OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_X_HIGH));
    OpLoadA(cpu, 0x3au);
    OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_Y_LOW));
    OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_Y_HIGH));
}

Lufia2ExecutionResult Lufia2MenuBuildCapsuleFeedingPanels(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82a233u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpLdx(cpu, 0x18u);
    if (!CapsuleSelectionChild(&call, 0x82a236u, 0x868d47u, 3u, 1u))
        return call.result;
    OpLdx(cpu, 0xfffeu);
    if (!CapsuleSelectionChild(&call, 0x82a23du, 0x829214u, 3u, 1u) ||
        !CapsuleSelectionChild(&call, 0x82a241u, 0x82838fu, 2u, 1u))
        return call.result;
    OpRepWidths(cpu, 0x20u);
    static const struct { uint16_t position, size; uint32_t site; } panels[] = {
        {0x0342u, 0x0803u, 0x82a24cu}, {0x0352u, 0x1603u, 0x82a255u},
        {0x0402u, 0x1e0bu, 0x82a25eu}, {0x025au, 0x1203u, 0x82a267u}
    };
    for (unsigned panel = 0u; panel < sizeof panels / sizeof panels[0]; ++panel) {
        OpLoadA(cpu, panels[panel].position);
        OpLdx(cpu, panels[panel].size);
        if (!CapsuleSelectionChild(&call, panels[panel].site, 0x82810eu, 2u, 0u))
            return call.result;
    }
    OpLoadA(cpu, 0x0244u);
    OpLdx(cpu, 0x0a03u);
    OpLdy(cpu, 0x1490u);
    if (!CapsuleSelectionChild(&call, 0x82a273u, 0x82831au, 2u, 0u))
        return call.result;
    OpSepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_WINDOW_STATE_2));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_WINDOW_STATE_2_ALT));
    OpLoadA(cpu, 5u);
    OpLdx(cpu, 7u);
    if (!CapsuleSelectionChild(&call, 0x82a283u, 0x82895bu, 2u, 1u))
        return call.result;
    OpLdy(cpu, 0x54u);
    OpLdx(cpu, 2u);
    if (!CapsuleSelectionChild(&call, 0x82a28cu, 0x82891eu, 2u, 1u))
        return call.result;
    OpLoadA(cpu, 1u);
    OpLdx(cpu, 9u);
    if (!CapsuleSelectionChild(&call, 0x82a294u, 0x82895bu, 2u, 1u))
        return call.result;
    OpLdy(cpu, 0x31u);
    OpLdx(cpu, 4u);
    if (!CapsuleSelectionChild(&call, 0x82a29du, 0x82891eu, 2u, 1u))
        return call.result;
    PlaceFeedingCursor(memory, cpu, 0u);
    PlaceFeedingCursor(memory, cpu, 1u);
    Push8(memory, cpu, cpu->program_bank);
    OpLoadA(cpu, Pull8(memory, cpu));
    OpSta(memory, cpu, OpDp(cpu, CAPSULE_TEXT_PARAMETER_BANK));
    OpLdx(cpu, WRAM_CAPSULE_WORK_STATS);
    OpWriteX(memory, cpu, OpDp(cpu, CAPSULE_TEXT_PARAMETER_ADDRESS), cpu->x);
    OpLoadA(cpu, 0x3cu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, CAPSULE_TEXT_BANK));
    OpLdy(cpu, 0xc8efu);
    OpLdx(cpu, 0x3082u);
    if (!CapsuleSelectionChild(&call, 0x82a2deu, 0x808878u, 3u, 1u))
        return call.result;
    return ExecutionReturned(0x82a2e2u);
}

Lufia2ExecutionResult Lufia2MenuBuildCapsuleFeedingScreen(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82a1d8u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_WINDOW_REFRESH));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_CURSOR_ENABLED));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09C8));
    OpOraValue(cpu, 2u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09C8));
    if (!CapsuleSelectionChild(&call, 0x82a1e8u, 0x82a233u, 2u, 1u))
        return call.result;
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, CAPSULE_TEXT_BANK));
    OpLdy(cpu, 0xd2f8u);
    if (!CapsuleSelectionChild(&call, 0x82a1f7u, 0x808878u, 3u, 1u) ||
        !CapsuleSelectionChild(&call, 0x82a1fbu, 0x82c5a2u, 2u, 1u) ||
        !CapsuleSelectionChild(&call, 0x82a1feu, 0x82d13au, 2u, 1u))
        return call.result;
    OpLdy(cpu, 0xf08au);
    if (!CapsuleSelectionChild(&call, 0x82a204u, 0x828c9cu, 2u, 1u))
        return call.result;
    OpLdx(cpu, 0x4800u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_SCROLL_TRACK), cpu->x);
    OpLdx(cpu, 0x60u);
    OpLdy(cpu, 0x5au);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_OVERFLOW), cpu->y);
    cpu->carry = 0u;
    if (!CapsuleSelectionChild(&call, 0x82a217u, 0x828c85u, 2u, 1u))
        return call.result;
    OpLoadA(cpu, 0x1fu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_TM));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_TS));
    if (!CapsuleSelectionChild(&call, 0x82a224u, 0x829313u, 2u, 1u) ||
        !CapsuleSelectionChild(&call, 0x82a227u, 0x82ac84u, 2u, 1u))
        return call.result;
    OpLoadA(cpu, 6u);
    if (!CapsuleSelectionChild(&call, 0x82a22cu, 0x8293f6u, 2u, 1u) ||
        !CapsuleSelectionChild(&call, 0x82a22fu, 0x828704u, 2u, 1u))
        return call.result;
    return ExecutionReturned(0x82a232u);
}

static bool StepAvailableCapsule(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PRESSED_B));
    OpBitValue(cpu, 3u);
    if (cpu->zero)
        return false;
    OpLda(memory, cpu, OpAbs(cpu, CAPSULE_TYPE_CURSOR));
    for (;;) {
        OpSta(memory, cpu, OpAbs(cpu, CAPSULE_TYPE_CURSOR));
        OpRepWidths(cpu, 0x20u);
        OpAndValue(cpu, 0xffu);
        cpu->carry = 0u;
        OpAdcValue(cpu, WRAM_CAPSULE_FORM_STATES);
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        if (!cpu->zero)
            return true;
        OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PRESSED_B));
        OpBitValue(cpu, 2u);
        const bool previous = !cpu->zero;
        OpLda(memory, cpu, OpAbs(cpu, CAPSULE_TYPE_CURSOR));
        if (!previous) {
            cpu->carry = 0u;
            OpAdcValue(cpu, 1u);
            OpCmpValue(cpu, CAPSULE_TYPE_COUNT);
            if (cpu->zero)
                OpLoadA(cpu, 0u);
        } else {
            cpu->carry = 1u;
            OpSbcValue(cpu, 1u);
            if (!cpu->carry)
                OpLoadA(cpu, CAPSULE_TYPE_COUNT - 1u);
        }
    }
}

static bool RefreshSelectedCapsule(MenuActionCall *call) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    if (StepAvailableCapsule(memory, cpu)) {
        OpLdx(cpu, 5u);
        if (!CapsuleSelectionChild(call, 0x82c1f7u, 0x8288cbu, 2u, 1u))
            return false;
    }
    OpLoadA(cpu, CAPSULE_LAST_FORM);
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, CAPSULE_FORM_CURSOR)));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_SELECTED_FORM));
    OpSta(memory, cpu, OpDp(cpu, CAPSULE_SELECTION_SCRATCH));
    OpLda(memory, cpu, OpAbs(cpu, CAPSULE_TYPE_CURSOR));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_SELECTED_ID));
    if (!CapsuleSelectionChild(call, 0x82c20bu, 0x82c482u, 2u, 1u))
        return false;
    OpLda(memory, cpu, OpAbsX(cpu, 0u));
    OpAndValue(cpu, 8u);
    bool reposition;
    if (!cpu->zero) {
        OpLoadA(cpu, CAPSULE_LAST_FORM);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_SELECTED_FORM));
        reposition = true;
    } else {
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        OpAndValue(cpu, 7u);
        OpCmp(memory, cpu, OpDp(cpu, CAPSULE_SELECTION_SCRATCH));
        reposition = !cpu->carry;
        if (reposition) {
            OpSta(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_SELECTED_FORM));
            OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PRESSED_B));
            OpBitValue(cpu, 8u);
            if (!cpu->zero) {
                OpLoadA(cpu, 1u);
                OpSta(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_SELECTED_FORM));
            }
        }
    }
    if (reposition) {
        OpLoadA(cpu, CAPSULE_LAST_FORM);
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_SELECTED_FORM)));
        OpSta(memory, cpu, OpAbs(cpu, CAPSULE_FORM_CURSOR));
        OpLdx(cpu, 5u);
        if (!CapsuleSelectionChild(call, 0x82c240u, 0x8288cbu, 2u, 1u))
            return false;
    }
    if (!CapsuleSelectionChild(call, 0x82c243u, 0x82d248u, 2u, 1u))
        return false;
    OpLoadA(cpu, 4u);
    if (!CapsuleSelectionChild(call, 0x82c248u, 0x82cf9du, 2u, 1u) ||
        !CapsuleSelectionChild(call, 0x82c24bu, 0x82cfd4u, 2u, 1u) ||
        !CapsuleSelectionChild(call, 0x82c24eu, 0x82c3f8u, 2u, 1u) ||
        !CapsuleSelectionChild(call, 0x82c251u, 0x82ce52u, 2u, 1u) ||
        !CapsuleSelectionChild(call, 0x82c254u, 0x82d270u, 2u, 1u) ||
        !CapsuleSelectionChild(call, 0x82c257u, 0x82d07bu, 2u, 1u))
        return false;
    OpLoadA(cpu, 8u);
    TestBitsDirect(memory, cpu, CAPSULE_REFRESH_FLAGS, 1u);
    return true;
}

Lufia2ExecutionResult Lufia2MenuChooseCapsule(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82c170u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    if (!CapsuleSelectionChild(&call, 0x82c170u, 0x82a2e3u, 2u, 1u))
        return call.result;
    OpLoadA(cpu, 0u);
    for (;;) {
        if (!CapsuleSelectionChild(&call, 0x82c175u, 0x828b08u, 2u, 1u))
            return call.result;
        OpCmpValue(cpu, 1u);
        if (cpu->zero) {
            if (!RefreshSelectedCapsule(&call))
                return call.result;
            OpLoadA(cpu, 1u);
            continue;
        }
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            OpLoadA(cpu, 2u);
            if (!CapsuleSelectionChild(&call, 0x82c18eu, 0x80953bu, 3u, 1u))
                return call.result;
            break;
        }
        OpCmpValue(cpu, 3u);
        if (cpu->zero)
            break;
        OpLoadA(cpu, 0u);
    }
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x0358u);
    OpLdx(cpu, 0x120bu);
    if (!CapsuleSelectionChild(&call, 0x82c19au, 0x8283b5u, 2u, 0u))
        return call.result;
    OpSepWidths(cpu, 0x20u);
    if (!CapsuleSelectionChild(&call, 0x82c19fu, 0x82d07bu, 2u, 1u) ||
        !CapsuleSelectionChild(&call, 0x82c1a2u, 0x82d121u, 2u, 1u))
        return call.result;
    OpLoadA(cpu, 0x88u);
    TestBitsDirect(memory, cpu, CAPSULE_REFRESH_FLAGS, 1u);
    OpLoadA(cpu, 3u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_CURSOR_SLOT)));
    OpLdy(cpu, 6u);
    if (!CapsuleSelectionChild(&call, 0x82c1b1u, 0x8289bcu, 2u, 1u))
        return call.result;
    return ExecutionReturned(0x82c1b4u);
}

static bool RunCapsuleFeedingSelection(MenuActionCall *call, bool *leave) {
    Lufia2CpuState *cpu = call->cpu;
    const Lufia2Memory *memory = call->memory;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_INDEX));
    const uint32_t previous_resume = cpu->resume_pc;
    SimulateJsrFrame(memory, cpu, 0xc106u);
    const Lufia2ExecutionResult dispatch = Lufia2MenuDispatchInlineTable(memory, cpu);
    if (dispatch.pc != 0x82c10du && dispatch.pc != 0x82c115u &&
        dispatch.pc != 0x82c11au) {
        call->result = dispatch;
        return false;
    }
    cpu->resume_pc = previous_resume;
    if (dispatch.pc == 0x82c10du)
        return CapsuleSelectionChild(call, 0x82c10du, 0x829c52u, 2u, 1u) &&
            CapsuleSelectionChild(call, 0x82c110u, 0x82a711u, 2u, 1u);
    if (dispatch.pc == 0x82c115u)
        return CapsuleSelectionChild(call, 0x82c115u, 0x82a9b5u, 2u, 1u);
    OpLoadA(cpu, 5u);
    OpLdx(cpu, 7u);
    if (!CapsuleSelectionChild(call, 0x82c11fu, 0x82895bu, 2u, 1u) ||
        !CapsuleSelectionChild(call, 0x82c122u, 0x868b55u, 3u, 1u))
        return false;
    *leave = true;
    return true;
}

static bool ScrollCapsuleFood(MenuActionCall *call) {
    Lufia2CpuState *cpu = call->cpu;
    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 1u));
    OpLsrA(cpu);
    if (!CapsuleSelectionChild(call, 0x82c133u, 0x828ce9u, 2u, 1u))
        return false;
    if (!cpu->carry &&
        !CapsuleSelectionChild(call, 0x82c138u, 0x82ac84u, 2u, 1u))
        return false;
    return CapsuleSelectionChild(call, 0x82c13bu, 0x828c43u, 2u, 1u) &&
        CapsuleSelectionChild(call, 0x82c13eu, 0x868b55u, 3u, 1u);
}

static bool FeedCapsuleDebugItem(MenuActionCall *call) {
    Lufia2CpuState *cpu = call->cpu;
    const Lufia2Memory *memory = call->memory;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E057C));
    if (cpu->zero)
        return true;
    if (!CapsuleSelectionChild(call, 0x82c14au, 0x82cc3cu, 2u, 1u))
        return false;
    if (cpu->carry)
        return true;
    OpLoadA(cpu, 2u);
    if (!CapsuleSelectionChild(call, 0x82c151u, 0x80953bu, 3u, 1u) ||
        !CapsuleSelectionChild(call, 0x82c155u, 0x82c504u, 2u, 1u))
        return false;
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, 0u));
    OpAndValue(cpu, 0x01ffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
    if (!CapsuleSelectionChild(call, 0x82c163u, 0x81f1c5u, 3u, 0u))
        return false;
    OpSepWidths(cpu, 0x20u);
    cpu->carry = 1u;
    return CapsuleSelectionChild(call, 0x82c16au, 0x82c683u, 2u, 1u);
}

Lufia2ExecutionResult Lufia2MenuRunCapsuleFeeding(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82c0acu);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    if (!CapsuleSelectionChild(&call, 0x82c0acu, 0x868b32u, 3u, 1u) ||
        !CapsuleSelectionChild(&call, 0x82c0b0u, 0x82fba7u, 2u, 1u) ||
        !CapsuleSelectionChild(&call, 0x82c0b3u, 0x82ca62u, 2u, 1u) ||
        !CapsuleSelectionChild(&call, 0x82c0b6u, 0x82a1d8u, 2u, 1u) ||
        !CapsuleSelectionChild(&call, 0x82c0b9u, 0x82a711u, 2u, 1u))
        return call.result;
    OpLoadA(cpu, 0u);
    bool leave = false;
    while (!leave) {
        if (!CapsuleSelectionChild(&call, 0x82c0beu, 0x828b08u, 2u, 1u))
            return call.result;
        OpCmpValue(cpu, 1u);
        if (cpu->zero) {
            OpLda(memory, cpu, OpDp(cpu, 0x47u));
            OpBitValue(cpu, 4u);
            if (!cpu->zero) {
                OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_INDEX));
                bool activate = cpu->zero;
                if (!activate) {
                    OpCmpValue(cpu, 2u);
                    if (!cpu->zero) {
                        OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_WINDOW_STATE_2));
                        if (!CapsuleSelectionChild(&call, 0x82c0f3u, 0x8288a0u, 2u, 1u) ||
                            !CapsuleSelectionChild(&call, 0x82c0f6u, 0x8288cbu, 2u, 1u))
                            return call.result;
                        activate = true;
                    }
                }
                if (activate && !RunCapsuleFeedingSelection(&call, &leave))
                    return call.result;
            }
        } else {
            OpCmpValue(cpu, 2u);
            if (cpu->zero) {
                OpLoadA(cpu, 2u);
                if (!CapsuleSelectionChild(&call, 0x82c0fdu, 0x80953bu, 3u, 1u) ||
                    !RunCapsuleFeedingSelection(&call, &leave))
                    return call.result;
            } else {
                OpCmpValue(cpu, 3u);
                if (cpu->zero)
                    break;
                OpCmpValue(cpu, 6u);
                bool scroll = cpu->zero;
                if (!scroll) {
                    OpCmpValue(cpu, 7u);
                    scroll = cpu->zero;
                }
                if (scroll) {
                    if (!ScrollCapsuleFood(&call))
                        return call.result;
                } else {
                    OpCmpValue(cpu, 4u);
                    if (!cpu->zero) {
                        OpLoadA(cpu, 0u);
                        continue;
                    }
                    if (!FeedCapsuleDebugItem(&call))
                        return call.result;
                }
            }
        }
        if (!leave)
            OpLoadA(cpu, 1u);
    }
    if (!CapsuleSelectionChild(&call, 0x82c128u, 0x868b32u, 3u, 1u) ||
        !CapsuleSelectionChild(&call, 0x82c12cu, 0x82a0e3u, 2u, 1u))
        return call.result;
    return ExecutionReturned(0x82c12fu);
}
