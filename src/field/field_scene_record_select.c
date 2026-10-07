#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/dp_scratch.h"
#include "system/wram.h"

Lufia2ExecutionResult Lufia2SceneScriptSelectRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page ||
        cpu->stack < 0x1f20u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x80bfe7u);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    OpLda(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    OpLdx(cpu, 6u);
    SimulateJslFrame(memory, cpu, 0x80u, 0xbff4u);
    Lufia2ExecutionResult result = Lufia2SceneScriptFindRecord(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    uint8_t low = Pull8(memory, cpu);
    uint8_t high = Pull8(memory, cpu);
    uint8_t bank = Pull8(memory, cpu);
    uint16_t back = (uint16_t)(low | ((uint16_t)high << 8));
    cpu->program_bank = bank;
    if (back != 0xbff4u || bank != 0x80u)
        return ExecutionHandoff(cpu, ((uint32_t)bank << 16) | (uint16_t)(back + 1u));
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09B7), cpu->y);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x80bff9u);
}
