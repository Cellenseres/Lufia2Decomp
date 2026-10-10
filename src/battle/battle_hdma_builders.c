#include "lufia2/battle.h"
#include "core/cpu_ops.h"
#include "core/plain_ops.h"
#include "core/wram_view.h"
#include "system/wram.h"

enum {
    HDMA_PATTERN_BANK = 0x85u,
    HDMA_SCROLL_BASE = (WRAM_NMI_SCROLL_REGISTERS & 0xffffu) + 8u,
    HDMA_PHASE = WRAM_BATTLE_RESULT_SCROLL_PHASE & 0xffffu,
    HDMA_TABLE_READY = WRAM_BATTLE_RESULT_SCROLL_READY & 0xffffu,
    HDMA_TABLE = WRAM_FIELD_MAP_ATTRIBUTES,
    HDMA_SHORT_PATTERN = 0xa31cu,
    HDMA_PHASE_PATTERN = 0xa3b2u,
    HDMA_WIDE_PATTERN = 0xa532u,
    HDMA_SHORT_BYTES = 0x20u,
    HDMA_WIDE_BYTES = 0x40u,
    HDMA_FINAL_PHASE = 0x180u,
    HDMA_DISABLE_REQUEST = 0xd9u,
    HDMA_ACTIVE_REQUEST = WRAM_BATTLE_RESULT_HDMA_REQUEST & 0xffffu
};

typedef struct HdmaPatternFill {
    uint16_t phase;
    Word16Result last;
} HdmaPatternFill;

static uint8_t PatternWidths(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit;
}

static Lufia2Wram EnterPatternBank(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, HDMA_PATTERN_BANK);
    return WramViewOfCaller(memory, cpu);
}

static HdmaPatternFill FillOffsetPattern(
    Lufia2Wram wram, uint16_t source, uint16_t phase,
    uint16_t size, uint8_t decimal) {
    HdmaPatternFill fill;
    uint16_t offset = 0u;

    fill.phase = phase;
    do {
        const uint16_t pattern = WramRead16At(wram, source, fill.phase);
        const uint16_t base = WramRead16(wram, HDMA_SCROLL_BASE);
        fill.last = Sum16Mode(pattern, base, false, decimal);
        WramWrite16At(wram, HDMA_TABLE, offset, fill.last.value);
        fill.phase = (uint16_t)(fill.phase + 2u);
        offset = (uint16_t)(offset + 2u);
    } while (offset != size);
    return fill;
}

static void PublishPattern(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2Wram wram, uint16_t accumulator,
    uint8_t ready) {
    cpu->accumulator = accumulator;
    LoadA8(cpu, ready);
    WramWrite(wram, HDMA_TABLE_READY, ready);
    PullDataBank(memory, cpu);
}

static Lufia2ExecutionResult BuildOffsetPattern(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint32_t entry, uint32_t terminal, uint16_t source,
    uint16_t size, int phase_step) {
    if (!PatternWidths(cpu))
        return ExecutionHandoff(cpu, entry);

    const Lufia2Wram wram = EnterPatternBank(memory, cpu);
    const uint16_t phase = phase_step ? WramRead16(wram, HDMA_PHASE) : 0u;
    PushStackWord(memory, cpu, cpu->x);
    const HdmaPatternFill fill = FillOffsetPattern(
        wram, source, phase, size, cpu->decimal);
    cpu->x = PullStackWord(memory, cpu);
    cpu->y = fill.phase;
    cpu->carry = true;
    cpu->overflow = fill.last.overflow;
    uint16_t accumulator = fill.last.value;
    if (phase_step) {
        uint16_t next = (uint16_t)(WramRead16(wram, HDMA_PHASE) + phase_step);
        if (phase_step > 0) {
            cpu->carry = next >= HDMA_SHORT_BYTES;
            if (next == HDMA_SHORT_BYTES)
                next = 0u;
        } else if (next & 0x8000u) {
            next = HDMA_SHORT_BYTES - 2u;
        }
        cpu->y = next;
        WramWrite16(wram, HDMA_PHASE, next);
        accumulator = next;
    }
    PublishPattern(memory, cpu, wram, accumulator, 1u);
    return ExecutionReturned(terminal);
}

static Lufia2ExecutionResult StopPattern(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint32_t entry, uint32_t terminal) {
    if (!PatternWidths(cpu))
        return ExecutionHandoff(cpu, entry);
    OpLoadA(cpu, 2u);
    OpTestBits(memory, cpu, OpDp(cpu, HDMA_DISABLE_REQUEST), 0u);
    OpStz(memory, cpu, OpAbs(cpu, HDMA_ACTIVE_REQUEST));
    return ExecutionReturned(terminal);
}

static Lufia2ExecutionResult BuildPhasePattern(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint32_t entry, uint32_t terminal, uint32_t stopped, int direction) {
    if (!PatternWidths(cpu))
        return ExecutionHandoff(cpu, entry);

    const Lufia2Wram wram = EnterPatternBank(memory, cpu);
    uint16_t phase = WramRead16(wram, HDMA_PHASE);
    cpu->y = phase;
    if (direction > 0)
        cpu->carry = phase >= HDMA_FINAL_PHASE;
    if (direction > 0 ? phase == HDMA_FINAL_PHASE : (phase & 0x8000u) != 0u) {
        PullDataBank(memory, cpu);
        return StopPattern(memory, cpu, entry, stopped);
    }

    PushStackWord(memory, cpu, cpu->x);
    uint16_t offset = direction > 0 ? 0u : HDMA_WIDE_BYTES - 2u;
    uint16_t last;
    do {
        last = WramRead16At(wram, HDMA_PHASE_PATTERN, phase);
        WramWrite16At(wram, HDMA_TABLE, offset, last);
        phase = (uint16_t)(phase + 2 * direction);
        offset = (uint16_t)(offset + 2 * direction);
    } while (direction > 0 ? offset != HDMA_WIDE_BYTES : (offset & 0x8000u) == 0u);
    if (direction > 0)
        cpu->carry = true;
    cpu->x = PullStackWord(memory, cpu);
    cpu->y = phase;
    WramWrite16(wram, HDMA_PHASE, phase);
    PublishPattern(memory, cpu, wram, last, 4u);
    return ExecutionReturned(terminal);
}

Lufia2ExecutionResult Lufia2BattleHdmaBuildMode5(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return BuildOffsetPattern(memory, cpu, 0x85af5eu, 0x85af9eu,
        HDMA_SHORT_PATTERN, HDMA_SHORT_BYTES, 2);
}

Lufia2ExecutionResult Lufia2BattleHdmaBuildMode6(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return BuildOffsetPattern(memory, cpu, 0x85afe0u, 0x85b01du,
        HDMA_SHORT_PATTERN, HDMA_SHORT_BYTES, -2);
}

Lufia2ExecutionResult Lufia2BattleHdmaBuildMode7(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return BuildOffsetPattern(memory, cpu, 0x85b058u, 0x85b081u,
        HDMA_SHORT_PATTERN, HDMA_SHORT_BYTES, 0);
}

Lufia2ExecutionResult Lufia2BattleHdmaBuildMode8(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return BuildPhasePattern(memory, cpu, 0x85b0c3u, 0x85b0f2u, 0x85b0fbu, 1);
}

Lufia2ExecutionResult Lufia2BattleHdmaBuildMode9(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return BuildPhasePattern(memory, cpu, 0x85b13bu, 0x85b164u, 0x85b16du, -1);
}

Lufia2ExecutionResult Lufia2BattleHdmaBuildMode10(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return BuildOffsetPattern(memory, cpu, 0x85b1a4u, 0x85b1cdu,
        HDMA_WIDE_PATTERN, HDMA_WIDE_BYTES, 0);
}

Lufia2ExecutionResult Lufia2BattleHdmaStopMode8(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return StopPattern(memory, cpu, 0x85b0f4u, 0x85b0fbu);
}

Lufia2ExecutionResult Lufia2BattleHdmaStopMode9(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return StopPattern(memory, cpu, 0x85b166u, 0x85b16du);
}
