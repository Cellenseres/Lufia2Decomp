#include "core/cpu_ops.h"
#include "lufia2/menu.h"
#include "system/wram.h"

enum {
    MENU_SELECTION_KEY = WRAM_UNK_7E09D3,
    MENU_CACHED_SELECTION_KEY = WRAM_UNK_7E14BD,
    MENU_RESTORE_ENABLED = WRAM_MENU_RESTORE_SELECTION_ENABLED,
    MENU_SAVED_STATE_POINTERS = 0x8ee5d7u,
    MENU_SAVED_STATE_POINTER = 0x5du,
    MENU_CURSOR_COUNT = 6u
};

static void RestoreMenuCursorRows(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLoadA(cpu, 0x7fu);
    OpSta(memory, cpu, OpDp(cpu, MENU_SAVED_STATE_POINTER + 2u));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, MENU_SELECTION_KEY));
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, MENU_SAVED_STATE_POINTERS));
    OpSta(memory, cpu, OpDp(cpu, MENU_SAVED_STATE_POINTER));
    OpTyx(cpu);
    OpTya(cpu);
    OpAslA(cpu);
    OpTay(cpu);
    OpSepWidths(cpu, 0x20u);
    do {
        OpLda(memory, cpu,
            DirectLongIndirectY(memory, cpu, MENU_SAVED_STATE_POINTER));
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_MENU_ITEM_COLUMN));
        OpIny(cpu);
        OpLda(memory, cpu,
            DirectLongIndirectY(memory, cpu, MENU_SAVED_STATE_POINTER));
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_MENU_ITEM_ROW));
        OpIny(cpu);
        OpInx(cpu);
        OpCpx(cpu, MENU_CURSOR_COUNT);
    } while (!cpu->zero);
    OpRepWidths(cpu, 0x20u);
    const uint16_t fields[] = {
        WRAM_MENU_SCROLL_ROW, WRAM_MENU_LIST_TOP_ROW, WRAM_MENU_LIST_SELECTION};
    for (unsigned field = 0u; field < 3u; ++field) {
        OpLda(memory, cpu,
            DirectLongIndirectY(memory, cpu, MENU_SAVED_STATE_POINTER));
        OpSta(memory, cpu, OpAbs(cpu, fields[field]));
        if (field < 2u) {
            OpIny(cpu);
            OpIny(cpu);
        }
    }
    OpSepWidths(cpu, 0x20u);
}

Lufia2ExecutionResult Lufia2MenuRestoreSelectionState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, MENU_CACHED_SELECTION_KEY));
    OpCmp(memory, cpu, OpAbs(cpu, MENU_SELECTION_KEY));
    if (!cpu->zero) {
        OpLdx(cpu, 0u);
        OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_SCROLL_ROW), cpu->x);
        OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW), cpu->x);
        OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_SELECTION), cpu->x);
        OpLdx(cpu, 1u);
        do {
            OpStz(memory, cpu, OpAbsX(cpu, WRAM_MENU_ITEM_COLUMN));
            OpStz(memory, cpu, OpAbsX(cpu, WRAM_MENU_ITEM_ROW));
            OpInx(cpu);
            OpCpx(cpu, MENU_CURSOR_COUNT);
        } while (!cpu->zero);
        OpLda(memory, cpu, OpAbs(cpu, MENU_RESTORE_ENABLED));
        if (!cpu->zero)
            RestoreMenuCursorRows(memory, cpu);
    }
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, MENU_SELECTION_KEY)));
    OpWriteX(memory, cpu, OpAbs(cpu, MENU_CACHED_SELECTION_KEY), cpu->x);
    return ExecutionReturned(0x8ee7bfu);
}
