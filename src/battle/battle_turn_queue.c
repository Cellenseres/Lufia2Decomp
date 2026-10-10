#include "core/cpu_ops.h"
#include "lufia2/battle.h"
#include "system/wram.h"

enum {
    TURN_ACTOR = 0x54u,
    TURN_PRIORITY = 0x56u,
    TURN_INSERT_OFFSET = 0xcau,
    TURN_ENTRY_BYTES = 3u
};

Lufia2ExecutionResult BattleInsertTurnBody(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    Push8(memory, cpu, 0x85u);
    PullDataBank(memory, cpu);
    OpLdx(cpu, 0u);
    for (;;) {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_TURN_QUEUE));
        if (cpu->zero)
            break;
        OpLdy(cpu, OpRead16(memory, OpAbsX(cpu, WRAM_BATTLE_TURN_QUEUE + 1u)));
        OpCpy(cpu, OpRead16(memory, OpDp(cpu, TURN_PRIORITY)));
        if (!cpu->carry)
            break;
        OpLdx(cpu, (uint16_t)(cpu->x + TURN_ENTRY_BYTES));
    }
    OpWrite16(memory, OpDp(cpu, TURN_INSERT_OFFSET), cpu->x);
    OpLdy(cpu, (WRAM_BATTLE_TURN_QUEUE_COUNT - 1u) * TURN_ENTRY_BYTES);
    do {
        OpLdy(cpu, (uint16_t)(cpu->y - TURN_ENTRY_BYTES));
        for (unsigned byte = 0u; byte < TURN_ENTRY_BYTES; ++byte) {
            const uint16_t address = (uint16_t)(WRAM_BATTLE_TURN_QUEUE + byte);
            OpLda(memory, cpu, OpAbsY(cpu, address));
            OpSta(memory, cpu, OpAbsY(cpu, address + TURN_ENTRY_BYTES));
        }
        OpCpy(cpu, OpRead16(memory, OpDp(cpu, TURN_INSERT_OFFSET)));
    } while (!cpu->zero);
    static const uint8_t staged[] = {TURN_ACTOR, TURN_PRIORITY, TURN_PRIORITY + 1u};
    for (unsigned byte = 0u; byte < TURN_ENTRY_BYTES; ++byte) {
        OpLda(memory, cpu, OpDp(cpu, staged[byte]));
        OpSta(memory, cpu, OpAbsX(cpu, (uint16_t)(WRAM_BATTLE_TURN_QUEUE + byte)));
    }
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x85937cu);
}

Lufia2ExecutionResult Lufia2BattleInsertTurn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x859337u);
    return BattleInsertTurnBody(memory, cpu);
}
