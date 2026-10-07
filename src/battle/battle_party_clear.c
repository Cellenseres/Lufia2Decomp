#include "core/cpu_ops.h"
#include "lufia2/battle.h"
#include "system/wram.h"

enum { PARTY_CLEAR_START = 0x15u, PARTY_CLEAR_BYTES = 16u };

Lufia2ExecutionResult Lufia2BattleClearPartyRecordBytes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x85eddbu);
    LoadY16(cpu, 2u * WRAM_BATTLE_PARTY_RECORDS_COUNT);
    do {
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_BATTLE_PARTY_RECORDS, cpu->y));
        if (!cpu->zero) {
            LoadA8(cpu, PARTY_CLEAR_BYTES);
            do {
                OpStz(memory, cpu, OpAbsX(cpu, PARTY_CLEAR_START));
                IncrementX16(cpu);
                OpDecA(cpu);
            } while (!cpu->zero);
        }
        OpDey(cpu);
        OpDey(cpu);
    } while (!cpu->negative);
    return ExecutionReturned(0x85edf0u);
}
