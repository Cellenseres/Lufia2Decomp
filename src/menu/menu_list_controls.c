#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "menu/menu_sprite_slots.h"
#include "lufia2/menu.h"
#include "system/wram.h"

enum {
    LIST_HELD_BUTTONS = 0x47u,
    LIST_REFRESH = 0x74u,
    LIST_MEMBER_POINTER = 0x2au,
    LIST_TEXT_BANK = 0x5fu,
    LIST_FIRST_SELECTION = WRAM_MENU_LIST_SELECTION,
    LIST_SECOND_SELECTION = WRAM_MENU_LIST_SECOND_SELECTION,
    LIST_CURSOR = WRAM_MENU_CURSOR_SLOT,
    LIST_ACTION = WRAM_MENU_LIST_INDEX,
    LIST_MODE = WRAM_MENU_LIST_MODE,
    LIST_COLUMN = WRAM_MENU_SPELL_WINDOW_STATE_2,
    LIST_ROW = WRAM_MENU_SPELL_WINDOW_STATE_2_ALT,
    LIST_DESCRIPTION_ID = WRAM_MENU_DESCRIPTION_ID,
    LIST_OVERFLOW_COUNT = WRAM_MENU_LIST_OVERFLOW,
    LIST_SPECIAL_COLUMNS = WRAM_MENU_SECONDARY_LIST_COLUMNS,
    LIST_SPECIAL_LIMIT = WRAM_BATTLE_PARTY_IDS + 2u,
    LIST_ORDINARY_RECORDS = WRAM_INVENTORY_PACKED_ITEMS,
    LIST_CURSOR_SPRITE = 5u,
    LIST_PAIR_FIRST = 8u,
    LIST_PAIR_SECOND = 9u
};

static uint8_t ListControlsReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x82u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal;
}

static uint8_t ListCall(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t site,
    uint32_t target, uint8_t frame) {
    return CallChildWithFrame(memory, cpu, child, context,
        site, target, frame, 0x82u);
}

static Lufia2ExecutionResult ListUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static uint8_t ListMatches(Lufia2CpuState *cpu, uint16_t value) {
    OpCmpValue(cpu, value);
    return cpu->zero;
}

Lufia2ExecutionResult Lufia2MenuReplaceCursor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ListControlsReady(cpu))
        return ExecutionHandoff(cpu, 0x8289bcu);
    OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ACTIVE));
    OpTyx(cpu);
    if (!ListCall(memory, cpu, child, context,
            0x8289c0u, 0x82895bu, 2u))
        return ListUnwound(0x8289c0u);
    return ExecutionReturned(0x8289c3u);
}

Lufia2ExecutionResult Lufia2MenuInitializeListScroll(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ListControlsReady(cpu))
        return ExecutionHandoff(cpu, 0x829c10u);
    OpLdy(cpu, 0xf08au);
    if (!ListCall(memory, cpu, child, context,
            0x829c13u, 0x828c9cu, 2u))
        return ListUnwound(0x829c13u);
    OpLdx(cpu, 0x4800u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_SCROLL_TRACK), cpu->x);
    OpLda(memory, cpu, OpAbs(cpu, LIST_MODE));
    if (ListMatches(cpu, 2u)) {
        OpLdx(cpu, 36u);
        OpLdy(cpu, 24u);
        OpLoadA(cpu, 2u);
        OpSta(memory, cpu, OpAbs(cpu, LIST_SPECIAL_COLUMNS));
        OpSta(memory, cpu, OpAbs(cpu, LIST_SPECIAL_LIMIT));
        cpu->carry = 1u;
    } else if (ListMatches(cpu, 7u)) {
        OpLdx(cpu, 64u);
        OpLdy(cpu, 58u);
        OpStz(memory, cpu, OpAbs(cpu, MENU_SPRITE_ACTIVE + 7u));
        cpu->carry = 0u;
    } else {
        OpLdx(cpu, 96u);
        OpLdy(cpu, 90u);
        cpu->carry = 0u;
    }
    OpWriteX(memory, cpu, OpAbs(cpu, LIST_OVERFLOW_COUNT), cpu->y);
    if (!ListCall(memory, cpu, child, context,
            0x829c4eu, 0x828c85u, 2u))
        return ListUnwound(0x829c4eu);
    return ExecutionReturned(0x829c51u);
}

static uint32_t ListBuildColumns(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    OpLda(memory, cpu, OpAbs(cpu, LIST_MODE));
    uint32_t first_site, second_site;
    uint16_t first_position, second_position, second_text;
    if (ListMatches(cpu, 2u)) {
        first_site = 0x829ba0u;
        first_position = 0x0903u;
        second_site = 0x829ba9u;
        second_position = 0x1503u;
        second_text = 0x0354u;
    } else if (ListMatches(cpu, 7u)) {
        OpLoadA(cpu, 0x0342u);
        OpLdx(cpu, 0x1103u);
        return ListCall(memory, cpu, child, context,
            0x829bb4u, 0x82810eu, 2u) ? 0u : 0x829bb4u;
    } else {
        first_site = 0x829b8cu;
        first_position = 0x0803u;
        second_site = 0x829b95u;
        second_position = 0x1603u;
        second_text = 0x0352u;
    }
    OpLoadA(cpu, 0x0342u);
    OpLdx(cpu, first_position);
    if (!ListCall(memory, cpu, child, context,
            first_site, 0x82810eu, 2u))
        return first_site;
    OpLoadA(cpu, second_text);
    OpLdx(cpu, second_position);
    return ListCall(memory, cpu, child, context,
        second_site, 0x82810eu, 2u) ? 0u : second_site;
}

Lufia2ExecutionResult Lufia2MenuBuildListWindows(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ListControlsReady(cpu))
        return ExecutionHandoff(cpu, 0x829b51u);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_WINDOW_REFRESH));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_CURSOR_ENABLED));
    OpLdx(cpu, 0xfff0u);
    if (!ListCall(memory, cpu, child, context,
            0x829b5au, 0x829214u, 3u))
        return ListUnwound(0x829b5au);
    OpLdx(cpu, 0u);
    if (!ListCall(memory, cpu, child, context,
            0x829b61u, 0x868d47u, 3u))
        return ListUnwound(0x829b61u);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x0350u);
    OpLdx(cpu, 0x1713u);
    if (!ListCall(memory, cpu, child, context,
            0x829b6du, 0x8283b5u, 2u))
        return ListUnwound(0x829b6du);
    OpLoadA(cpu, 0x0402u);
    OpLdx(cpu, 0x1e0bu);
    if (!ListCall(memory, cpu, child, context,
            0x829b76u, 0x82810eu, 2u))
        return ListUnwound(0x829b76u);
    uint32_t interrupted = ListBuildColumns(memory, cpu, child, context);
    if (interrupted)
        return ListUnwound(interrupted);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, LIST_TEXT_BANK));
    OpLdy(cpu, 0xd23du);
    if (!ListCall(memory, cpu, child, context,
            0x829bc5u, 0x808878u, 3u))
        return ListUnwound(0x829bc5u);
    OpStz(memory, cpu, OpAbs(cpu, MENU_SPRITE_ACTIVE + 5u));
    OpStz(memory, cpu, OpAbs(cpu, LIST_COLUMN));
    OpStz(memory, cpu, OpAbs(cpu, LIST_ROW));
    OpLoadA(cpu, 5u);
    OpLdx(cpu, 7u);
    if (!ListCall(memory, cpu, child, context,
            0x829bd7u, 0x82895bu, 2u))
        return ListUnwound(0x829bd7u);
    OpLoadA(cpu, 1u);
    OpLdx(cpu, LIST_PAIR_SECOND);
    if (!ListCall(memory, cpu, child, context,
            0x829bdfu, 0x82895bu, 2u))
        return ListUnwound(0x829bdfu);
    OpLda(memory, cpu, OpAbs(cpu, LIST_MODE));
    OpLdy(cpu, ListMatches(cpu, 2u) ? 70u : 42u);
    OpLdx(cpu, 2u);
    if (!ListCall(memory, cpu, child, context,
            0x829bf4u, 0x82891eu, 2u))
        return ListUnwound(0x829bf4u);
    OpLdy(cpu, 49u);
    OpLdx(cpu, 4u);
    if (!ListCall(memory, cpu, child, context,
            0x829bfdu, 0x82891eu, 2u))
        return ListUnwound(0x829bfdu);
    if (!ListCall(memory, cpu, child, context,
            0x829c00u, 0x829c10u, 2u))
        return ListUnwound(0x829c00u);
    if (!ListCall(memory, cpu, child, context,
            0x829c03u, 0x82ac84u, 2u))
        return ListUnwound(0x829c03u);
    OpLoadA(cpu, 3u);
    if (!ListCall(memory, cpu, child, context,
            0x829c08u, 0x8293f6u, 2u))
        return ListUnwound(0x829c08u);
    OpLoadA(cpu, 0x88u);
    OpTestBits(memory, cpu, OpDp(cpu, LIST_REFRESH), 1u);
    return ExecutionReturned(0x829c0fu);
}

static uint32_t ListChangePage(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context,
    uint8_t selecting) {
    OpLoadA(cpu, OpA(cpu) ^ 1u);
    OpLsrA(cpu);
    uint32_t direction_site = selecting ? 0x82a7f8u : 0x82a68au;
    if (!ListCall(memory, cpu, child, context,
            direction_site, 0x828ce9u, 2u))
        return direction_site;
    if (!cpu->carry) {
        uint32_t update_site = selecting ? 0x82a7fdu : 0x82a68fu;
        uint32_t update = selecting ? 0x82a918u : 0x82ac84u;
        if (!ListCall(memory, cpu, child, context,
                update_site, update, 2u))
            return update_site;
    }
    uint32_t refresh_site = selecting ? 0x82a800u : 0x82a692u;
    if (!ListCall(memory, cpu, child, context,
            refresh_site, 0x828c43u, 2u))
        return refresh_site;
    if (selecting && !ListCall(memory, cpu, child, context,
            0x82a803u, 0x868b55u, 3u))
        return 0x82a803u;
    return 0u;
}

static uint32_t ListConfirmAction(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    OpLda(memory, cpu, OpAbs(cpu, LIST_ACTION));
    if (!ListMatches(cpu, 1u)) {
        if (!ListCall(memory, cpu, child, context,
                0x82a6d7u, 0x829c52u, 2u))
            return 0x82a6d7u;
        return ListCall(memory, cpu, child, context,
            0x82a6dau, 0x82a711u, 2u) ? 0u : 0x82a6dau;
    }
    OpLda(memory, cpu, OpAbs(cpu, LIST_MODE));
    if (ListMatches(cpu, 0u))
        return ListCall(memory, cpu, child, context,
            0x82a6e6u, 0x82a9b5u, 2u) ? 0u : 0x82a6e6u;
    return ListCall(memory, cpu, child, context,
        0x82a6ebu, 0x82b3adu, 2u) ? 0u : 0x82a6ebu;
}

static uint32_t ListFinishInput(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x0342u);
    OpLdx(cpu, 0x1e13u);
    if (!ListCall(memory, cpu, child, context,
            0x82a6f8u, 0x8283b5u, 2u))
        return 0x82a6f8u;
    OpLoadA(cpu, 0x0082u);
    OpLdx(cpu, 0x1e0au);
    OpLdy(cpu, 3u);
    if (!ListCall(memory, cpu, child, context,
            0x82a704u, 0x8280cau, 2u))
        return 0x82a704u;
    OpSepWidths(cpu, 0x20u);
    if (!ListCall(memory, cpu, child, context,
            0x82a709u, 0x829b10u, 2u))
        return 0x82a709u;
    OpLoadA(cpu, 0x88u);
    OpTestBits(memory, cpu, OpDp(cpu, LIST_REFRESH), 1u);
    return 0u;
}

Lufia2ExecutionResult Lufia2MenuRunListInput(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ListControlsReady(cpu))
        return ExecutionHandoff(cpu, 0x82a658u);
    if (!ListCall(memory, cpu, child, context,
            0x82a658u, 0x829b51u, 2u))
        return ListUnwound(0x82a658u);
    if (!ListCall(memory, cpu, child, context,
            0x82a65bu, 0x82a711u, 2u))
        return ListUnwound(0x82a65bu);
    OpLoadA(cpu, 0u);
    for (;;) {
        if (!ListCall(memory, cpu, child, context,
                0x82a660u, 0x828b08u, 2u))
            return ListUnwound(0x82a660u);
        uint32_t interrupted = 0u;
        uint8_t handled = 1u;
        if (ListMatches(cpu, 9u) || ListMatches(cpu, 5u)) {
            if (!ListCall(memory, cpu, child, context,
                    0x82a698u, 0x82b60au, 2u))
                interrupted = 0x82a698u;
        } else if (ListMatches(cpu, 1u)) {
            OpLdx(cpu, 2u);
            OpLda(memory, cpu, OpAbsX(cpu, WRAM_MENU_ITEM_COLUMN));
            if (ListMatches(cpu, 2u)) {
                OpLda(memory, cpu, OpAbsX(cpu,
                    MENU_SPRITE_X_LOW + LIST_CURSOR_SPRITE));
                cpu->carry = 0u;
                OpAdcValue(cpu, 8u);
                OpSta(memory, cpu, OpAbsX(cpu,
                    MENU_SPRITE_X_LOW + LIST_CURSOR_SPRITE));
            } else {
                OpLda(memory, cpu, OpDp(cpu, LIST_HELD_BUTTONS));
                OpBitValue(cpu, 4u);
                if (!cpu->zero) {
                    OpLda(memory, cpu, OpAbs(cpu, LIST_ACTION));
                    if (ListMatches(cpu, 1u)) {
                        OpStz(memory, cpu, OpAbs(cpu, LIST_COLUMN));
                        if (!ListCall(memory, cpu, child, context,
                                0x82a6c2u, 0x8288a0u, 2u))
                            interrupted = 0x82a6c2u;
                        else if (!ListCall(memory, cpu, child, context,
                                0x82a6c5u, 0x8288cbu, 2u))
                            interrupted = 0x82a6c5u;
                    }
                    if (!interrupted)
                        interrupted = ListConfirmAction(memory, cpu, child, context);
                }
            }
        } else if (ListMatches(cpu, 2u)) {
            OpLoadA(cpu, 2u);
            if (!ListCall(memory, cpu, child, context,
                    0x82a6ccu, 0x80953bu, 3u))
                interrupted = 0x82a6ccu;
            else
                interrupted = ListConfirmAction(memory, cpu, child, context);
        } else if (ListMatches(cpu, 3u)) {
            interrupted = ListFinishInput(memory, cpu, child, context);
            return interrupted ? ListUnwound(interrupted)
                               : ExecutionReturned(0x82a710u);
        } else if (ListMatches(cpu, 6u) || ListMatches(cpu, 7u)) {
            interrupted = ListChangePage(memory, cpu, child, context, 0u);
        } else {
            handled = 0u;
        }
        if (interrupted)
            return ListUnwound(interrupted);
        OpLoadA(cpu, handled);
    }
}

static uint32_t ListSelectFirst(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    OpLoadA(cpu, 2u);
    if (!ListCall(memory, cpu, child, context,
            0x82a777u, 0x80953bu, 3u))
        return 0x82a777u;
    static const uint16_t fields[] = {
        MENU_SPRITE_X_LOW + LIST_PAIR_FIRST,
        MENU_SPRITE_Y_LOW + LIST_PAIR_FIRST,
        MENU_SPRITE_X_HIGH + LIST_PAIR_FIRST,
        MENU_SPRITE_Y_HIGH + LIST_PAIR_FIRST, WRAM_MENU_ITEM_ROW + 3u
    };
    for (unsigned field = 0u; field < sizeof(fields) / sizeof(fields[0]); ++field) {
        OpLda(memory, cpu, OpAbs(cpu, (uint16_t)(fields[field] + 1u)));
        OpSta(memory, cpu, OpAbs(cpu, fields[field]));
    }
    OpRepWidths(cpu, 0x20u);
    if (!ListCall(memory, cpu, child, context,
            0x82a79bu, 0x82fbe5u, 2u))
        return 0x82a79bu;
    OpSta(memory, cpu, OpAbs(cpu, LIST_FIRST_SELECTION));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 4u);
    OpLdx(cpu, LIST_PAIR_FIRST);
    if (!ListCall(memory, cpu, child, context,
            0x82a7a8u, 0x82895bu, 2u))
        return 0x82a7a8u;
    OpLoadA(cpu, 2u);
    OpLdx(cpu, LIST_PAIR_SECOND);
    return ListCall(memory, cpu, child, context,
        0x82a7b0u, 0x82895bu, 2u) ? 0u : 0x82a7b0u;
}

static uint32_t ListCancelSelection(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context,
    uint8_t *finished) {
    OpLda(memory, cpu, OpAbs(cpu, LIST_FIRST_SELECTION));
    if (!ListMatches(cpu, 0xffu)) {
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpAbs(cpu, LIST_FIRST_SELECTION));
        OpLoadA(cpu, 1u);
        OpLdx(cpu, LIST_PAIR_FIRST);
        OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, LIST_CURSOR)));
        return ListCall(memory, cpu, child, context,
            0x82a7cau, 0x8289bcu, 2u) ? 0u : 0x82a7cau;
    }
    OpLda(memory, cpu, OpAbs(cpu, LIST_MODE));
    if (ListMatches(cpu, 0xffu)) {
        if (!ListCall(memory, cpu, child, context,
                0x82a7d7u, 0x829dd3u, 2u))
            return 0x82a7d7u;
        OpLoadA(cpu, 0x88u);
        OpTestBits(memory, cpu, OpDp(cpu, LIST_REFRESH), 1u);
    }
    OpLoadA(cpu, 0u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, LIST_CURSOR)));
    OpLdy(cpu, 7u);
    if (!ListCall(memory, cpu, child, context,
            0x82a7e6u, 0x8289fau, 2u))
        return 0x82a7e6u;
    OpLoadA(cpu, 3u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, LIST_CURSOR)));
    OpLdy(cpu, 7u);
    if (!ListCall(memory, cpu, child, context,
            0x82a7f1u, 0x8289bcu, 2u))
        return 0x82a7f1u;
    *finished = 1u;
    return 0u;
}

static uint32_t ListShowDescription(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, LIST_MODE));
    OpLoadA(cpu, OpA(cpu) & 0x00ffu);
    uint8_t spells = ListMatches(cpu, 2u);
    uint32_t record_site = spells ? 0x82a854u : 0x82a843u;
    if (!ListCall(memory, cpu, child, context,
            record_site, 0x82fbe5u, 2u))
        return record_site;
    uint8_t unavailable;
    if (spells) {
        OpLsrA(cpu);
        OpTay(cpu);
        uint32_t address = (((uint32_t)cpu->data_bank << 16) +
            Read16Direct(memory, cpu, LIST_MEMBER_POINTER) + cpu->y) & 0x00ffffffu;
        OpLoadA(cpu, Read16Long(memory, address));
        OpLoadA(cpu, OpA(cpu) & 0x00ffu);
        unavailable = ListMatches(cpu, 0xffu);
        if (!unavailable)
            OpLoadA(cpu, OpA(cpu) | 0x0200u);
    } else {
        OpTax(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, LIST_ORDINARY_RECORDS));
        OpLoadA(cpu, OpA(cpu) & 0x01ffu);
        unavailable = cpu->zero;
    }
    if (!unavailable) {
        OpSta(memory, cpu, OpAbs(cpu, LIST_DESCRIPTION_ID));
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbs(cpu, LIST_MODE));
        if (ListMatches(cpu, 4u))
            OpLdy(cpu, 0x060du);
        else if (ListMatches(cpu, 0xffu))
            OpLdy(cpu, 0x100eu);
        else
            OpLdy(cpu, 0x030cu);
        OpLdx(cpu, 1u);
        if (!ListCall(memory, cpu, child, context,
                0x82a886u, 0x82fd81u, 2u))
            return 0x82a886u;
    }
    OpSepWidths(cpu, 0x20u);
    return 0u;
}

static uint32_t ListSortSelection(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context,
    uint8_t *finished) {
    OpLda(memory, cpu, OpAbs(cpu, LIST_MODE));
    if (!ListMatches(cpu, 0xffu)) {
        OpLda(memory, cpu, OpAbs(cpu, LIST_FIRST_SELECTION));
        if (ListMatches(cpu, 0xffu)) {
            OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_SCROLL_ROW)));
            if (cpu->zero) {
                OpLda(memory, cpu, OpAbs(cpu, LIST_ACTION));
                OpCmpValue(cpu, 2u);
                if (!cpu->carry)
                    return ListCancelSelection(memory, cpu, child, context, finished);
            }
        }
    }
    return ListCall(memory, cpu, child, context,
        0x82a824u, 0x82ad20u, 2u) ? 0u : 0x82a824u;
}

Lufia2ExecutionResult Lufia2MenuRunListSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ListControlsReady(cpu))
        return ExecutionHandoff(cpu, 0x82a711u);
    OpLdx(cpu, 0xffu);
    OpWriteX(memory, cpu, OpAbs(cpu, LIST_FIRST_SELECTION), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, LIST_SECOND_SELECTION), cpu->x);
    OpLoadA(cpu, 0u);
    for (;;) {
        if (!ListCall(memory, cpu, child, context,
                0x82a71cu, 0x828b08u, 2u))
            return ListUnwound(0x82a71cu);
        uint32_t interrupted = 0u;
        uint8_t finished = 0u, handled = 1u, refresh = 0u;
        if (ListMatches(cpu, 9u) || ListMatches(cpu, 5u)) {
            if (!ListCall(memory, cpu, child, context,
                    0x82a75au, 0x82b60au, 2u))
                interrupted = 0x82a75au;
        } else if (ListMatches(cpu, 2u)) {
            OpLda(memory, cpu, OpAbs(cpu, LIST_FIRST_SELECTION));
            if (ListMatches(cpu, 0xffu)) {
                interrupted = ListSelectFirst(memory, cpu, child, context);
            } else {
                OpRepWidths(cpu, 0x20u);
                if (!ListCall(memory, cpu, child, context,
                        0x82a768u, 0x82fbe5u, 2u))
                    interrupted = 0x82a768u;
                else {
                    OpSta(memory, cpu, OpAbs(cpu, LIST_SECOND_SELECTION));
                    OpSepWidths(cpu, 0x20u);
                    if (!ListCall(memory, cpu, child, context,
                            0x82a770u, 0x82aaffu, 2u))
                        interrupted = 0x82a770u;
                }
            }
        } else if (ListMatches(cpu, 3u)) {
            interrupted = ListCancelSelection(memory, cpu, child, context, &finished);
        } else if (ListMatches(cpu, 4u)) {
            interrupted = ListShowDescription(memory, cpu, child, context);
            handled = 0u;
        } else if (ListMatches(cpu, 6u) || ListMatches(cpu, 7u)) {
            interrupted = ListChangePage(memory, cpu, child, context, 1u);
        } else if (ListMatches(cpu, 10u)) {
            interrupted = ListSortSelection(memory, cpu, child, context, &finished);
            refresh = !finished;
        } else if (ListMatches(cpu, 11u)) {
            if (!ListCall(memory, cpu, child, context,
                    0x82a829u, 0x82adb4u, 2u))
                interrupted = 0x82a829u;
            refresh = 1u;
        } else {
            handled = 0u;
        }
        if (interrupted)
            return ListUnwound(interrupted);
        if (finished)
            return ExecutionReturned(0x82a7f4u);
        if (refresh) {
            if (!ListCall(memory, cpu, child, context,
                    0x82a82cu, 0x828c43u, 2u))
                return ListUnwound(0x82a82cu);
            OpLoadA(cpu, 8u);
            OpTestBits(memory, cpu, OpDp(cpu, LIST_REFRESH), 1u);
        }
        OpLoadA(cpu, handled);
    }
}
