#include "lufia2/menu_member_screens.h"
#include "menu/menu_action_internal.h"
#include "menu/menu_sprite_slots.h"
#include "system/wram.h"

enum {
    MEMBER_SELECTION_LIMIT = 0x14bfu,
    MEMBER_SELECTION = WRAM_MENU_LIST_INDEX,
    MEMBER_SPELL_FLAGS = 0x0fu,
    MEMBER_DISPLAY_SLOT = 4u,
    MEMBER_DISPLAY_TEXT = 0x0a03u,
    MEMBER_STATUS_TEXT = 2u,
    MEMBER_SPELL_TEXT_POSITIONS = 0x8ed8bbu,
    MEMBER_REFRESH = 0x74u,
    MEMBER_SELECTED_ID = 0x22u,
    MEMBER_CURSOR_FIRST = 0x54u,
    MEMBER_CURSOR_COUNT = 0x56u
};

static bool MemberScreenChild(MenuActionCall *call, uint32_t site,
                                uint32_t target, uint8_t frame, uint8_t width) {
    Lufia2CpuState *cpu = call->cpu;
    if (!CallChildWithFrame(call->memory, cpu, call->child, call->context,
                            site, target, frame, 0x82u)) {
        call->result = (Lufia2ExecutionResult){
            LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return false;
    }
    if (cpu->program_bank != 0x82u || cpu->accumulator_is_8_bit != width ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal) {
        const uint32_t continuation = (uint32_t)cpu->program_bank << 16 |
            (uint16_t)(site + frame + 1u);
        call->result = ExecutionHandoff(cpu, continuation);
        return false;
    }
    return true;
}

static void SetMemberSelectionLimit(const Lufia2Memory *memory,
                                      Lufia2CpuState *cpu) {
    OpLoadA(cpu, 3u);
    OpSta(memory, cpu, OpAbs(cpu, MEMBER_SELECTION_LIMIT));
}

static bool InspectSpellMember(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, MEMBER_SELECTION));
    OpAslA(cpu);
    OpTay(cpu);
    OpLda(memory, cpu, OpAbsY(cpu, WRAM_MENU_PARTY_CHARACTER_OFFSET));
    OpTay(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsY(cpu, MEMBER_SPELL_FLAGS));
    OpBitValue(cpu, 4u);
    return cpu->zero;
}

static void BindSpellMember(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, MEMBER_SELECTION)));
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_MEMBER), cpu->x);
    OpLda(memory, cpu, OpLongX(cpu, MEMBER_SPELL_TEXT_POSITIONS));
    OpSta(memory, cpu, OpAbs(cpu, MEMBER_DISPLAY_TEXT));
}

static bool ChooseMember(MenuActionCall *call, uint32_t poll_site) {
    SetMemberSelectionLimit(call->memory, call->cpu);
    return MemberScreenChild(call, poll_site, 0x829246u, 2u, 1u);
}

Lufia2ExecutionResult Lufia2MenuChooseSpellMember(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82b370u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    if (!MemberScreenChild(&call, 0x82b370u, 0x829ef2u, 2u, 1u))
        return call.result;
    for (;;) {
        if (!ChooseMember(&call, 0x82b378u))
            return call.result;
        if (cpu->carry)
            return ExecutionReturned(0x82b3acu);
        if (InspectSpellMember(memory, cpu))
            break;
        if (!MemberScreenChild(&call, 0x82b391u, 0x8297e3u, 2u, 1u))
            return call.result;
    }
    BindSpellMember(memory, cpu);
    if (!MemberScreenChild(&call, 0x82b3a3u, 0x82b5edu, 2u, 1u) ||
        !MemberScreenChild(&call, 0x82b3a6u, 0x829984u, 2u, 1u) ||
        !MemberScreenChild(&call, 0x82b3a9u, 0x82a658u, 2u, 1u))
        return call.result;
    return ExecutionReturned(0x82b3acu);
}

static bool DrawMemberStatus(MenuActionCall *call, uint32_t cycle_site,
                              uint32_t text_site) {
    if (!MemberScreenChild(call, cycle_site, 0x829a14u, 2u, 1u))
        return false;
    OpLoadA(call->cpu, MEMBER_STATUS_TEXT);
    return MemberScreenChild(call, text_site, 0x82a318u, 2u, 1u);
}

static Lufia2ExecutionResult CloseMemberStatus(MenuActionCall *call) {
    if (!MemberScreenChild(call, 0x82b332u, 0x868b32u, 3u, 1u) ||
        !MemberScreenChild(call, 0x82b336u, 0x82838fu, 2u, 1u) ||
        !MemberScreenChild(call, 0x82b339u, 0x829ae0u, 2u, 1u))
        return call->result;
    return ExecutionReturned(0x82b33cu);
}

static bool ConfirmMemberStatus(MenuActionCall *call, bool *finished) {
    Lufia2CpuState *cpu = call->cpu;
    OpLoadA(cpu, 2u);
    if (!MemberScreenChild(call, 0x82b33fu, 0x80953bu, 3u, 1u))
        return false;
    OpLoadA(cpu, 5u);
    OpLdx(cpu, 7u);
    if (!MemberScreenChild(call, 0x82b348u, 0x82895bu, 2u, 1u) ||
        !MemberScreenChild(call, 0x82b34bu, 0x868b55u, 3u, 1u))
        return false;
    OpLda(call->memory, cpu, OpAbs(cpu, MEMBER_SELECTION));
    *finished = !cpu->zero;
    if (*finished)
        return true;
    OpLoadA(cpu, 3u);
    OpLdx(cpu, 7u);
    if (!MemberScreenChild(call, 0x82b359u, 0x82895bu, 2u, 1u))
        return false;
    OpLoadA(cpu, 1u);
    return true;
}

Lufia2ExecutionResult Lufia2MenuRunMemberStatus(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82b2feu);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    if (!MemberScreenChild(&call, 0x82b2feu, 0x829ef2u, 2u, 1u) ||
        !ChooseMember(&call, 0x82b306u))
        return call.result;
    if (cpu->carry)
        return ExecutionReturned(0x82b33cu);
    if (!MemberScreenChild(&call, 0x82b30bu, 0x868b32u, 3u, 1u) ||
        !MemberScreenChild(&call, 0x82b30fu, 0x82838fu, 2u, 1u) ||
        !MemberScreenChild(&call, 0x82b312u, 0x829e67u, 2u, 1u))
        return call.result;
    OpLoadA(cpu, 0u);
    for (;;) {
        if (!MemberScreenChild(&call, 0x82b317u, 0x828b08u, 2u, 1u))
            return call.result;
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            bool finished;
            if (!ConfirmMemberStatus(&call, &finished))
                return call.result;
            if (finished)
                return CloseMemberStatus(&call);
        } else {
            OpCmpValue(cpu, 3u);
            if (cpu->zero)
                return CloseMemberStatus(&call);
            OpCmpValue(cpu, 6u);
            if (!cpu->zero) {
                OpCmpValue(cpu, 7u);
                if (!cpu->zero) {
                    OpLoadA(cpu, 0u);
                    continue;
                }
            }
        }
        OpLoadA(cpu, OpA(cpu) ^ 1u);
        OpLsrA(cpu);
        if (!DrawMemberStatus(&call, 0x82b361u, 0x82b366u))
            return call.result;
        OpLoadA(cpu, 8u);
        OpSta(memory, cpu, OpDp(cpu, MEMBER_REFRESH));
        OpLoadA(cpu, 1u);
    }
}

static void CopyStatusPortrait(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, MEMBER_DISPLAY_SLOT);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, MEMBER_SELECTION)));
    static const uint16_t member_fields[] = {
        MENU_SPRITE_LIST_LOW, MENU_SPRITE_LIST_HIGH, MENU_SPRITE_BANK};
    for (unsigned field = 0u; field < 3u; ++field) {
        OpLda(memory, cpu, OpAbsY(cpu, member_fields[field]));
        OpSta(memory, cpu, OpAbsX(cpu, member_fields[field]));
    }
    static const uint16_t position_fields[] = {
        MENU_SPRITE_X_LOW, MENU_SPRITE_X_HIGH,
        MENU_SPRITE_Y_LOW, MENU_SPRITE_Y_HIGH};
    for (unsigned field = 0u; field < 4u; ++field) {
        OpLda(memory, cpu, OpAbs(cpu, position_fields[field]));
        OpSta(memory, cpu, OpAbsX(cpu, position_fields[field]));
    }
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ACTIVE));
    OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_FRAME_INDEX));
    OpLda(memory, cpu, OpAbsY(cpu, MENU_SPRITE_ANIMATION));
}

Lufia2ExecutionResult Lufia2MenuBuildMemberStatus(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x829e67u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_WINDOW_REFRESH));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_CURSOR_ENABLED));
    OpLdx(cpu, 0x006fu);
    if (!MemberScreenChild(&call, 0x829e70u, 0x829214u, 3u, 1u))
        return call.result;
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x05c2u);
    OpLdx(cpu, 0x0a03u);
    if (!MemberScreenChild(&call, 0x829e7cu, 0x82810eu, 2u, 0u))
        return call.result;
    OpLoadA(cpu, 0x05d6u);
    OpLdx(cpu, 0x1403u);
    if (!MemberScreenChild(&call, 0x829e85u, 0x82810eu, 2u, 0u))
        return call.result;
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, MEMBER_SELECTION));
    OpSta(memory, cpu, OpDp(cpu, MEMBER_SELECTED_ID));
    if (!MemberScreenChild(&call, 0x829e8fu, 0x829971u, 2u, 1u))
        return call.result;
    OpLoadA(cpu, MEMBER_STATUS_TEXT);
    if (!MemberScreenChild(&call, 0x829e94u, 0x82a318u, 2u, 1u))
        return call.result;
    CopyStatusPortrait(memory, cpu);
    if (!MemberScreenChild(&call, 0x829ed2u, 0x868cf5u, 3u, 1u))
        return call.result;
    OpLdx(cpu, 7u);
    OpWriteX(memory, cpu, OpDp(cpu, MEMBER_CURSOR_FIRST), cpu->x);
    OpLdx(cpu, 1u);
    OpWriteX(memory, cpu, OpDp(cpu, MEMBER_CURSOR_COUNT), cpu->x);
    OpLdx(cpu, 6u);
    OpLdy(cpu, 7u);
    if (!MemberScreenChild(&call, 0x829ee6u, 0x8289c4u, 2u, 1u))
        return call.result;
    OpLoadA(cpu, 1u);
    if (!MemberScreenChild(&call, 0x829eebu, 0x8293f6u, 2u, 1u) ||
        !MemberScreenChild(&call, 0x829eeeu, 0x828704u, 2u, 1u))
        return call.result;
    return ExecutionReturned(0x829ef1u);
}

Lufia2ExecutionResult Lufia2MenuRunMemberScreenPhase(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry) {
    const uint8_t byte_mode = entry != 0x829e7fu && entry != 0x829e88u;
    if (cpu->program_bank != 0x82u || cpu->accumulator_is_8_bit != byte_mode ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal)
        return ExecutionHandoff(cpu, entry);
    uint32_t next;
    switch (entry) {
    case 0x829e67u:
        OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_WINDOW_REFRESH));
        OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_CURSOR_ENABLED));
        OpLdx(cpu, 0x006fu);
        next = 0x829e70u;
        break;
    case 0x829e74u:
        OpRepWidths(cpu, 0x20u);
        OpLoadA(cpu, 0x05c2u);
        OpLdx(cpu, 0x0a03u);
        next = 0x829e7cu;
        break;
    case 0x829e7fu:
        OpLoadA(cpu, 0x05d6u);
        OpLdx(cpu, 0x1403u);
        next = 0x829e85u;
        break;
    case 0x829e88u:
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbs(cpu, MEMBER_SELECTION));
        OpSta(memory, cpu, OpDp(cpu, MEMBER_SELECTED_ID));
        next = 0x829e8fu;
        break;
    case 0x829e92u:
        OpLoadA(cpu, MEMBER_STATUS_TEXT);
        next = 0x829e94u;
        break;
    case 0x829e97u:
        CopyStatusPortrait(memory, cpu);
        next = 0x829ed2u;
        break;
    case 0x829ed6u:
        OpLdx(cpu, 7u);
        OpWriteX(memory, cpu, OpDp(cpu, MEMBER_CURSOR_FIRST), cpu->x);
        OpLdx(cpu, 1u);
        OpWriteX(memory, cpu, OpDp(cpu, MEMBER_CURSOR_COUNT), cpu->x);
        OpLdx(cpu, 6u);
        OpLdy(cpu, 7u);
        next = 0x829ee6u;
        break;
    case 0x829ee9u:
        OpLoadA(cpu, 1u);
        next = 0x829eebu;
        break;
    case 0x82b301u:
    case 0x82b373u:
    case 0x82b394u:
        SetMemberSelectionLimit(memory, cpu);
        next = entry == 0x82b301u ? 0x82b306u : 0x82b378u;
        break;
    case 0x82b309u:
        next = cpu->carry ? 0x82b33cu : 0x82b30bu;
        break;
    case 0x82b315u:
        OpLoadA(cpu, 0u);
        next = 0x82b317u;
        break;
    case 0x82b31au:
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            OpLoadA(cpu, 2u);
            next = 0x82b33fu;
            break;
        }
        OpCmpValue(cpu, 3u);
        if (cpu->zero) {
            next = 0x82b332u;
            break;
        }
        OpCmpValue(cpu, 6u);
        if (!cpu->zero)
            OpCmpValue(cpu, 7u);
        if (cpu->zero) {
            OpLoadA(cpu, OpA(cpu) ^ 1u);
            OpLsrA(cpu);
            next = 0x82b361u;
        } else {
            OpLoadA(cpu, 0u);
            next = 0x82b317u;
        }
        break;
    case 0x82b343u:
        OpLoadA(cpu, 5u);
        OpLdx(cpu, 7u);
        next = 0x82b348u;
        break;
    case 0x82b34fu:
        OpLda(memory, cpu, OpAbs(cpu, MEMBER_SELECTION));
        if (!cpu->zero) {
            next = 0x82b332u;
        } else {
            OpLoadA(cpu, 3u);
            OpLdx(cpu, 7u);
            next = 0x82b359u;
        }
        break;
    case 0x82b35cu:
        OpLoadA(cpu, 1u);
        OpLoadA(cpu, OpA(cpu) ^ 1u);
        OpLsrA(cpu);
        next = 0x82b361u;
        break;
    case 0x82b364u:
        OpLoadA(cpu, MEMBER_STATUS_TEXT);
        next = 0x82b366u;
        break;
    case 0x82b369u:
        OpLoadA(cpu, 8u);
        OpSta(memory, cpu, OpDp(cpu, MEMBER_REFRESH));
        OpLoadA(cpu, 1u);
        next = 0x82b317u;
        break;
    case 0x82b37bu:
        if (cpu->carry) {
            next = 0x82b3acu;
        } else if (InspectSpellMember(memory, cpu)) {
            BindSpellMember(memory, cpu);
            next = 0x82b3a3u;
        } else {
            next = 0x82b391u;
        }
        break;
    default:
        return ExecutionHandoff(cpu, entry);
    }
    return ExecutionHandoff(cpu, next);
}
