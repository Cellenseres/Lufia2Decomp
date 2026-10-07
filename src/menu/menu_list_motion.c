#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/menu.h"
#include "menu/menu_sprite_slots.h"
#include "system/wram.h"

enum {
    LIST_TYPE = WRAM_MENU_LIST_MODE,
    LIST_TOP = WRAM_MENU_LIST_TOP_ROW,
    LIST_LAST_PAGE = WRAM_MENU_LIST_OVERFLOW,
    LIST_MARKED_ROW = WRAM_MENU_LIST_SELECTION,
    LIST_MARKER_ROW = WRAM_MENU_LIST_MARKER_ROW,
    LIST_MARKER_ACTIVE = MENU_SPRITE_ACTIVE + 8u,
    LIST_MARKER_Y = MENU_SPRITE_Y_LOW + 8u,
    LIST_COLUMNS = WRAM_MENU_SECONDARY_LIST_COLUMNS,
    LIST_CURSOR = WRAM_MENU_CURSOR_SLOT,
    LIST_FILTER = WRAM_BATTLE_PARTY_IDS + 1u,
    LIST_THUMB_ROW = WRAM_MENU_SCROLL_ROW,
    LIST_UPLOAD_ROWS = WRAM_MENU_DISPLAY_REQUESTS + 1u,
    LIST_PARAMETERS = WRAM_MENU_SCROLL_WINDOW_ROWS,
    LIST_PARAMETER_TABLE = 0x8d93u,
    LIST_COLUMN_COUNT = WRAM_BATTLE_PARTY_IDS + 2u,
    DRAW_INDEX = 0x00u,
    DRAW_DESTINATION = 0x11u,
    DRAW_ROWS = 0x15u,
    CURSOR_WINDOW = 0x54u,
    CURSOR_SIZE = 0x56u,
    UNUSED_MARKER = 0xffu
};

static uint8_t ListReady(const Lufia2CpuState *cpu, uint8_t bank) {
    return cpu->program_bank == bank && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal;
}

static uint8_t ListChild(const Lufia2Memory *memory, Lufia2CpuState *cpu,
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

Lufia2ExecutionResult Lufia2MenuLoadListParameters(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    if (!ListReady(cpu, 0x86u))
        return ExecutionHandoff(cpu, 0x868d47u);
    PushDataBank(memory, cpu);
    LoadA8(cpu, 0x86u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    for (unsigned parameter = 0u; parameter < 7u; ++parameter) {
        OpLda(memory, cpu, OpAbsX(cpu,
            (uint16_t)(LIST_PARAMETER_TABLE + parameter)));
        OpSta(memory, cpu, OpAbs(cpu,
            (uint16_t)(LIST_PARAMETERS + parameter * 2u)));
        OpStz(memory, cpu, OpAbs(cpu,
            (uint16_t)(LIST_PARAMETERS + parameter * 2u + 1u)));
    }
    OpLda(memory, cpu, OpAbsX(cpu, LIST_PARAMETER_TABLE + 7u));
    OpSta(memory, cpu, OpAbs(cpu, LIST_COLUMN_COUNT));
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x868d92u);
}

Lufia2ExecutionResult Lufia2MenuInitializeListCursor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ListReady(cpu, 0x82u) || !child)
        return ExecutionHandoff(cpu, 0x829c52u);
    OpLdx(cpu, 0x0031u);
    OpWrite16(memory, OpDp(cpu, CURSOR_WINDOW), cpu->x);
    OpLdx(cpu, 0x0401u);
    OpWrite16(memory, OpDp(cpu, CURSOR_SIZE), cpu->x);
    OpLdx(cpu, 7u);
    OpLdy(cpu, 9u);
    if (!ListChild(memory, cpu, child, context, 0x829c62u, 0x8289c4u, 2u))
        return ListUnwound(0x829c62u);
    LoadA8(cpu, 1u);
    OpLdx(cpu, 7u);
    OpLdy(cpu, OpRead16(memory, OpAbs(cpu, LIST_CURSOR)));
    if (!ListChild(memory, cpu, child, context, 0x829c6du, 0x8289fau, 2u))
        return ListUnwound(0x829c6du);
    OpLda(memory, cpu, OpAbs(cpu, LIST_TYPE));
    OpCmpValue(cpu, 2u);
    if (cpu->zero) {
        LoadA8(cpu, 2u);
        OpSta(memory, cpu, OpAbs(cpu, LIST_COLUMNS));
    }
    return ExecutionReturned(0x829c7cu);
}

static Lufia2ExecutionResult DrawListContents(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    uint32_t site = 0x82acbau;
    uint32_t target = 0x8284d5u;
    uint32_t end = 0x82acbdu;
    uint8_t filter = 0u;
    OpLda(memory, cpu, OpAbs(cpu, LIST_TYPE));
    OpCmpValue(cpu, 0u);
    if (!cpu->zero) {
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            site = 0x82acc1u;
            target = 0x828526u;
            end = 0x82acc4u;
        } else {
            OpCmpValue(cpu, 4u);
            if (cpu->zero) {
                filter = 1u;
                site = 0x82accau;
                end = 0x82accdu;
            } else {
                OpCmpValue(cpu, 7u);
                if (cpu->zero) {
                    site = 0x82acceu;
                    target = 0x8286c4u;
                    end = 0x82acd1u;
                } else {
                    OpCmpValue(cpu, 0xffu);
                    if (cpu->zero) {
                        filter = 2u;
                        site = 0x82acd7u;
                        end = 0x82acdau;
                    }
                }
            }
        }
    }
    if (site != 0x82acceu) {
        if (filter) {
            LoadA8(cpu, filter);
            OpSta(memory, cpu, OpAbs(cpu, LIST_FILTER));
        } else {
            OpStz(memory, cpu, OpAbs(cpu, LIST_FILTER));
        }
    }
    if (!ListChild(memory, cpu, child, context, site, target, 2u))
        return ListUnwound(site);
    return ExecutionReturned(end);
}

Lufia2ExecutionResult Lufia2MenuDrawListPage(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ListReady(cpu, 0x82u) || !child)
        return ExecutionHandoff(cpu, 0x82ac84u);
    OpRepWidths(cpu, 0x20u);
    LoadA16(cpu, 0x0402u);
    OpLdx(cpu, 0x1e10u);
    if (!ListChild(memory, cpu, child, context, 0x82ac8cu, 0x8283ebu, 2u))
        return ListUnwound(0x82ac8cu);
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, 0x3448u);
    OpWrite16(memory, OpDp(cpu, DRAW_DESTINATION), cpu->x);
    OpLdx(cpu, 6u);
    OpWrite16(memory, OpDp(cpu, DRAW_ROWS), cpu->x);
    OpLdx(cpu, OpRead16(memory, OpAbs(cpu, LIST_TOP)));
    OpWrite16(memory, OpDp(cpu, DRAW_INDEX), cpu->x);
    return DrawListContents(memory, cpu, child, context);
}

static Lufia2ExecutionResult MoveListMarker(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, int step,
    uint32_t entry, uint32_t end) {
    if (!ListReady(cpu, 0x82u))
        return ExecutionHandoff(cpu, entry);
    OpLda(memory, cpu, OpAbs(cpu, LIST_MARKED_ROW));
    OpCmpValue(cpu, UNUSED_MARKER);
    if (!cpu->zero) {
        for (unsigned pixel = 0u; pixel < 3u; ++pixel)
            OpStepMem(memory, cpu, OpAbs(cpu, LIST_MARKER_Y), step);
    }
    return ExecutionReturned(end);
}

Lufia2ExecutionResult Lufia2MenuMoveListMarkerDown(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    return MoveListMarker(memory, cpu, 1, 0x82ada3u, 0x82adb3u);
}

Lufia2ExecutionResult Lufia2MenuMoveListMarkerUp(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    return MoveListMarker(memory, cpu, -1, 0x82ae53u, 0x82ae63u);
}

static void AdvanceMarkedRow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, int step,
    uint8_t hiddenRow) {
    OpLda(memory, cpu, OpAbs(cpu, LIST_MARKED_ROW));
    OpCmpValue(cpu, UNUSED_MARKER);
    if (!cpu->zero) {
        OpStepMem(memory, cpu, OpAbs(cpu, LIST_MARKER_ROW), step);
        OpLda(memory, cpu, OpAbs(cpu, LIST_MARKER_ACTIVE));
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, LIST_MARKER_ROW));
            OpCmpValue(cpu, hiddenRow);
            if (cpu->zero)
                OpStz(memory, cpu, OpAbs(cpu, LIST_MARKER_ACTIVE));
        }
    }
}

static void RestoreMarkedRow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint8_t firstRow) {
    OpLda(memory, cpu, OpAbs(cpu, LIST_MARKED_ROW));
    OpCmpValue(cpu, UNUSED_MARKER);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, LIST_MARKER_ACTIVE));
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, LIST_MARKER_ROW));
            if (firstRow)
                OpCmpValue(cpu, firstRow);
            if (cpu->zero) {
                LoadA8(cpu, 1u);
                OpSta(memory, cpu, OpAbs(cpu, LIST_MARKER_ACTIVE));
            }
        }
    }
}

static Lufia2ExecutionResult AnimateListRows(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint8_t down) {
    static const uint32_t markerSites[2][4] = {
        {0x82ad56u, 0x82ad62u, 0x82ad6eu, 0x82ad7du},
        {0x82ae04u, 0x82ae10u, 0x82ae1cu, 0x82ae2bu}
    };
    static const uint32_t frameSites[2][3] = {
        {0x82ad5eu, 0x82ad6au, 0x82ad76u},
        {0x82ae0cu, 0x82ae18u, 0x82ae24u}
    };
    static const uint8_t uploadRows[2][3] = {{2u, 4u, 4u}, {8u, 16u, 16u}};
    const uint32_t marker = down ? 0x82ae53u : 0x82ada3u;
    for (unsigned frame = 0u; frame < 3u; ++frame) {
        uint32_t site = markerSites[down][frame];
        if (!ListChild(memory, cpu, child, context, site, marker, 2u))
            return ListUnwound(site);
        LoadA8(cpu, uploadRows[down][frame]);
        OpSta(memory, cpu, OpAbs(cpu, LIST_UPLOAD_ROWS));
        site = frameSites[down][frame];
        if (!ListChild(memory, cpu, child, context, site, 0x868b55u, 3u))
            return ListUnwound(site);
    }
    uint32_t site = down ? 0x82ae28u : 0x82ad7au;
    uint32_t target = down ? 0x829009u : 0x828f5eu;
    if (!ListChild(memory, cpu, child, context, site, target, 2u))
        return ListUnwound(site);
    site = markerSites[down][3];
    if (!ListChild(memory, cpu, child, context, site, marker, 2u))
        return ListUnwound(site);
    LoadA8(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, LIST_UPLOAD_ROWS));
    RestoreMarkedRow(memory, cpu, down ? 5u : 0u);
    OpRepWidths(cpu, 0x20u);
    OpStepMem(memory, cpu, OpAbs(cpu, LIST_THUMB_ROW), down ? 1 : -1);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(down ? 0x82ae52u : 0x82ada2u);
}

Lufia2ExecutionResult Lufia2MenuAnimateListUp(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ListReady(cpu, 0x82u) || !child)
        return ExecutionHandoff(cpu, 0x82ad20u);
    OpLda(memory, cpu, OpAbs(cpu, LIST_TOP));
    if (cpu->zero) {
        OpSepWidths(cpu, 0x20u);
        return ExecutionReturned(0x82ada2u);
    }
    AdvanceMarkedRow(memory, cpu, 1, 6u);
    OpLda(memory, cpu, OpAbs(cpu, LIST_TYPE));
    OpCmpValue(cpu, 2u);
    if (cpu->zero)
        OpStepMem(memory, cpu, OpAbs(cpu, LIST_TOP), -1);
    OpStepMem(memory, cpu, OpAbs(cpu, LIST_TOP), -1);
    OpLdx(cpu, OpRead16(memory, OpAbs(cpu, LIST_TOP)));
    OpWrite16(memory, OpDp(cpu, DRAW_INDEX), cpu->x);
    if (!ListChild(memory, cpu, child, context, 0x82ad53u, 0x82acdbu, 2u))
        return ListUnwound(0x82ad53u);
    return AnimateListRows(memory, cpu, child, context, 0u);
}

Lufia2ExecutionResult Lufia2MenuAnimateListDown(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ListReady(cpu, 0x82u) || !child)
        return ExecutionHandoff(cpu, 0x82adb4u);
    OpLda(memory, cpu, OpAbs(cpu, LIST_TOP));
    OpCmp(memory, cpu, OpAbs(cpu, LIST_LAST_PAGE));
    if (cpu->carry) {
        OpSepWidths(cpu, 0x20u);
        return ExecutionReturned(0x82ae52u);
    }
    AdvanceMarkedRow(memory, cpu, -1, 0xffu);
    OpLda(memory, cpu, OpAbs(cpu, LIST_TYPE));
    OpCmpValue(cpu, 2u);
    const uint8_t twoColumns = cpu->zero;
    OpLda(memory, cpu, OpAbs(cpu, LIST_TOP));
    cpu->carry = 0u;
    OpAdcValue(cpu, twoColumns ? 12u : 6u);
    OpSta(memory, cpu, OpDp(cpu, DRAW_INDEX));
    OpStz(memory, cpu, OpDp(cpu, DRAW_INDEX + 1u));
    uint32_t site = twoColumns ? 0x82adfbu : 0x82ade9u;
    if (!ListChild(memory, cpu, child, context, site, 0x82acdbu, 2u))
        return ListUnwound(site);
    OpStepMem(memory, cpu, OpAbs(cpu, LIST_TOP), 1);
    if (twoColumns)
        OpStepMem(memory, cpu, OpAbs(cpu, LIST_TOP), 1);
    return AnimateListRows(memory, cpu, child, context, 1u);
}
