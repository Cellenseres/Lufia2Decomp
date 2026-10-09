#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/menu.h"

enum {
    ROM_MENU_SAVED_STATE_POINTERS = 0x8ee5d7u,
    DP_MENU_SAVED_STATE_POINTER = 0x5du,
    MENU_CURSOR_COUNT = 6u
};

Lufia2ExecutionResult Lufia2MenuSaveSelectionState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLoadA(cpu, 0x7fu);
    OpSta(memory, cpu, OpDp(cpu, DP_MENU_SAVED_STATE_POINTER + 2u));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09D3));
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, ROM_MENU_SAVED_STATE_POINTERS));
    OpSta(memory, cpu, OpDp(cpu, DP_MENU_SAVED_STATE_POINTER));
    OpTyx(cpu);
    OpTya(cpu);
    OpAslA(cpu);
    OpTay(cpu);
    OpSepWidths(cpu, 0x20u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_MENU_ITEM_COLUMN));
        OpSta(memory, cpu,
            DirectLongIndirectY(memory, cpu, DP_MENU_SAVED_STATE_POINTER));
        OpIny(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_MENU_ITEM_ROW));
        OpSta(memory, cpu,
            DirectLongIndirectY(memory, cpu, DP_MENU_SAVED_STATE_POINTER));
        OpIny(cpu);
        OpInx(cpu);
        OpCpx(cpu, MENU_CURSOR_COUNT);
    } while (!cpu->zero);
    OpRepWidths(cpu, 0x20u);
    const uint16_t fields[] = {
        WRAM_MENU_SCROLL_ROW, WRAM_MENU_LIST_TOP_ROW, WRAM_MENU_LIST_SELECTION};
    for (unsigned field = 0u; field < 3u; ++field) {
        OpLda(memory, cpu, OpAbs(cpu, fields[field]));
        OpSta(memory, cpu,
            DirectLongIndirectY(memory, cpu, DP_MENU_SAVED_STATE_POINTER));
        if (field < 2u) {
            OpIny(cpu);
            OpIny(cpu);
        }
    }
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x8ee750u);
}
