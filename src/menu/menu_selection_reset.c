#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/menu.h"

Lufia2ExecutionResult Lufia2MenuResetSaveSelectionRecords(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x8ee6eau);
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_MODE), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09D3), cpu->x);
    OpLoadA(cpu, 0u);
    do {
        OpSta(memory, cpu, OpLongX(cpu, WRAM_MENU_SAVED_SELECTION_RECORDS));
        OpInx(cpu);
        OpCpx(cpu, WRAM_MENU_SAVED_SELECTION_RECORDS_COUNT);
    } while (!cpu->zero);
    return ExecutionReturned(0x8ee6ffu);
}
