#include "core/cpu_ops.h"
#include "lufia2/battle.h"

enum { EFFECT_RELOAD_DELAY = 2u };

static Lufia2ExecutionResult SetEffectFrameDelay(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint8_t delay, uint32_t entry) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, entry);
    OpSetDataBank(memory, cpu, 0x7eu);
    LoadA8(cpu, delay);
    Write8(memory, AbsoluteIndexedAddress(cpu, EFFECT_RELOAD_DELAY, cpu->y), delay);
    return ExecutionReturned(entry + 9u);
}

Lufia2ExecutionResult Lufia2BattleEffectSetDelayOne(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return SetEffectFrameDelay(memory, cpu, 1u, 0x81a605u);
}

Lufia2ExecutionResult Lufia2BattleEffectSetDelayTwo(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return SetEffectFrameDelay(memory, cpu, 2u, 0x81a60fu);
}

Lufia2ExecutionResult Lufia2BattleEffectSetDelayThree(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return SetEffectFrameDelay(memory, cpu, 3u, 0x81a619u);
}

Lufia2ExecutionResult Lufia2BattleEffectSetDelayFour(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return SetEffectFrameDelay(memory, cpu, 4u, 0x81a623u);
}

Lufia2ExecutionResult Lufia2BattleEffectSetDelayFive(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return SetEffectFrameDelay(memory, cpu, 5u, 0x81a62du);
}

Lufia2ExecutionResult Lufia2BattleEffectSetDelaySix(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return SetEffectFrameDelay(memory, cpu, 6u, 0x81a637u);
}

Lufia2ExecutionResult Lufia2BattleEffectSetDelayEight(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return SetEffectFrameDelay(memory, cpu, 8u, 0x81a641u);
}

Lufia2ExecutionResult Lufia2BattleEffectSetDelayTen(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return SetEffectFrameDelay(memory, cpu, 10u, 0x81a64bu);
}

Lufia2ExecutionResult Lufia2BattleEffectSetDelayTwenty(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return SetEffectFrameDelay(memory, cpu, 20u, 0x81a655u);
}
