#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/menu.h"
#include "system/wram.h"

static uint8_t CharacterReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x80u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal;
}

static Lufia2ExecutionResult CharacterUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2MenuWriteCharacter(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CharacterReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x8088dau);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x8088dau, 0x808db3u, 2u, 0x80u))
        return CharacterUnwound(0x8088dau);
    OpStepMem(memory, cpu, OpAbs(cpu, WRAM_MENU_TEXT_CHARACTERS_LEFT), -1);
    if (cpu->zero) {
        for (unsigned row = 0u; row < 2u; ++row) {
            const uint32_t site = 0x8088e2u + row * 3u;
            if (!CallChildWithFrame(memory, cpu, child, context,
                    site, 0x808df9u, 2u, 0x80u))
                return CharacterUnwound(site);
        }
    }
    return ExecutionReturned(0x8088e8u);
}

Lufia2ExecutionResult Lufia2MenuWriteRawControl(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CharacterReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x8088c8u);
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_TEXT_RAW_TILE));
    const uint8_t raw_tile = !cpu->zero;
    ExchangeAccumulatorBytes(cpu);
    if (!raw_tile) {
        cpu->carry = 0u;
        return ExecutionReturned(0x8088d9u);
    }
    OpLdx(cpu, OpRead16(memory, OpAbs(cpu, WRAM_MENU_TEXT_CURSOR)));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x8088d2u, 0x8088dau, 2u, 0x80u))
        return CharacterUnwound(0x8088d2u);
    cpu->carry = 1u;
    return ExecutionReturned(0x8088d6u);
}
