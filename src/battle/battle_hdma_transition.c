#include "lufia2/battle.h"
#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/plain_ops.h"
#include "core/wram_view.h"
#include "system/wram.h"

enum {
    TRANSITION_PATTERN_BANK = 0x85u,
    TRANSITION_LAST_WORD = 0x1beu,
    TRANSITION_START_PHASE = 0x249u,
    TRANSITION_PHASE_LIMIT = 0x300u,
    TRANSITION_PATTERN_INDEX = 0x0eu,
    TRANSITION_PHASE_STEP = 9u,
    TRANSITION_TABLE = WRAM_FIELD_MAP_ATTRIBUTES,
    TRANSITION_INDEX_TABLE = WRAM_BATTLE_TRANSITION_MASK,
    TRANSITION_PHASE = WRAM_BATTLE_RESULT_SCROLL_PHASE & 0xffffu,
    TRANSITION_READY = WRAM_BATTLE_RESULT_SCROLL_READY & 0xffffu,
    TRANSITION_REQUEST = WRAM_BATTLE_RESULT_HDMA_REQUEST & 0xffffu,
    TRANSITION_MODE = WRAM_BATTLE_RESULT_HDMA_CHANNELS & 0xffffu,
    TRANSITION_FIRST_DESCRIPTOR = WRAM_BATTLE_TRANSITION_HDMA_CONTROL & 0xffffu,
    TRANSITION_SECOND_DESCRIPTOR = WRAM_BATTLE_COLOR_HDMA_DESCRIPTOR & 0xffffu,
    TRANSITION_ENABLE = 0xdau,
    TRANSITION_UPLOAD = 0xdbu,
    TRANSITION_FIRST_INDIRECT_BANK = 0x004317u,
    TRANSITION_SECOND_INDIRECT_BANK = 0x004327u
};

static uint8_t TransitionWidths(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit;
}

static uint16_t WriteClampedPatternWord(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2Wram wram, uint16_t offset,
    uint16_t phase, uint32_t pattern, uint8_t *carry) {
    PushStackWord(memory, cpu, offset);
    const uint16_t delta = Read16Long(memory,
        pattern + (offset & TRANSITION_PATTERN_INDEX));
    const Word16Result sum = Sum16Mode(phase, delta, *carry, cpu->decimal);
    uint16_t value = sum.value & 0x8000u ? cpu->direct_page : sum.value;
    *carry = value >= TRANSITION_PHASE_LIMIT;
    if (*carry)
        value = TRANSITION_PHASE_LIMIT;
    offset = PullStackWord(memory, cpu);
    WramWrite16At(wram, TRANSITION_TABLE, offset, value);
    return offset;
}

static uint16_t FillClampedPattern(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2Wram wram, uint16_t phase, uint32_t pattern) {
    uint16_t offset = TRANSITION_LAST_WORD;
    uint8_t carry = cpu->carry;

    do {
        offset = WriteClampedPatternWord(
            memory, cpu, wram, offset, phase, pattern, &carry);
        phase = (uint16_t)(phase - 1u);
        if (phase & 0x8000u)
            phase = 0u;
        offset = (uint16_t)(offset - 2u);
        offset = WriteClampedPatternWord(
            memory, cpu, wram, offset, phase, pattern, &carry);
        offset = (uint16_t)(offset - 2u);
    } while (!(offset & 0x8000u));
    return phase;
}

static Lufia2ExecutionResult BuildTransitionPattern(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint32_t entry, uint32_t terminal, uint32_t pattern) {
    if (!TransitionWidths(cpu) || cpu->program_bank != TRANSITION_PATTERN_BANK)
        return ExecutionHandoff(cpu, entry);

    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, TRANSITION_PATTERN_BANK);
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const uint16_t phase = WramRead16(wram, TRANSITION_PHASE);
    PushStackWord(memory, cpu, cpu->x);
    cpu->y = FillClampedPattern(memory, cpu, wram, phase, pattern);
    cpu->x = PullStackWord(memory, cpu);
    const uint16_t previous = WramRead16(wram, TRANSITION_PHASE);
    const Word16Result next = Difference16Mode(
        previous, TRANSITION_PHASE_STEP, cpu->decimal);
    if (!(next.value & 0x8000u))
        WramWrite16(wram, TRANSITION_PHASE, next.value);
    cpu->accumulator = next.value;
    cpu->carry = next.carry;
    cpu->overflow = next.overflow;
    LoadA8(cpu, 1u);
    WramWrite(wram, TRANSITION_READY, 1u);
    PullDataBank(memory, cpu);
    return ExecutionReturned(terminal);
}

static void ConfigureTransitionDescriptor(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t descriptor, uint8_t reg,
    uint16_t table, uint32_t indirect_bank) {
    OpLoadA(cpu, 0x42u);
    OpSta(memory, cpu, OpAbs(cpu, descriptor));
    OpLoadA(cpu, reg);
    OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(descriptor + 1u)));
    OpLdx(cpu, table);
    OpWriteX(memory, cpu, OpAbs(cpu, (uint16_t)(descriptor + 2u)), cpu->x);
    OpLoadA(cpu, TRANSITION_PATTERN_BANK);
    OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(descriptor + 4u)));
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, indirect_bank);
}

static Lufia2ExecutionResult BeginTransitionPattern(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t entry, uint32_t terminal, uint32_t site,
    uint32_t builder, uint16_t first_table, uint16_t second_table,
    uint8_t mode) {
    if (!TransitionWidths(cpu) || !child ||
        cpu->program_bank != TRANSITION_PATTERN_BANK)
        return ExecutionHandoff(cpu, entry);

    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, TRANSITION_LAST_WORD);
    TransferDirectToA(cpu);
    do {
        OpSta(memory, cpu, OpLongX(cpu, TRANSITION_INDEX_TABLE));
        OpIncA(cpu);
        OpDex(cpu);
        OpDex(cpu);
    } while (!cpu->negative);
    OpSepWidths(cpu, 0x20u);
    OpLdy(cpu, TRANSITION_START_PHASE);
    OpWriteX(memory, cpu, OpAbs(cpu, TRANSITION_PHASE), cpu->y);
    if (!CallChildWithFrame(memory, cpu, child, context,
            site, builder, 2u, TRANSITION_PATTERN_BANK)) {
        Lufia2ExecutionResult result = ExecutionReturned(site);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    if (!TransitionWidths(cpu))
        return ExecutionHandoff(cpu, site + 3u);
    ConfigureTransitionDescriptor(memory, cpu, TRANSITION_FIRST_DESCRIPTOR,
        0x11u, first_table, TRANSITION_FIRST_INDIRECT_BANK);
    ConfigureTransitionDescriptor(memory, cpu, TRANSITION_SECOND_DESCRIPTOR,
        0x12u, second_table, TRANSITION_SECOND_INDIRECT_BANK);
    OpLoadA(cpu, 6u);
    OpTestBits(memory, cpu, OpDp(cpu, TRANSITION_ENABLE), 1u);
    OpLoadA(cpu, mode);
    OpSta(memory, cpu, OpAbs(cpu, TRANSITION_MODE));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, TRANSITION_REQUEST));
    OpLoadA(cpu, 2u);
    OpTestBits(memory, cpu, OpDp(cpu, TRANSITION_UPLOAD), 1u);
    return ExecutionReturned(terminal);
}

Lufia2ExecutionResult Lufia2BattleHdmaBeginTransitionMode13(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return BeginTransitionPattern(memory, cpu, child, context,
        0x85ac16u, 0x85ac78u, 0x85ac2du, 0x85ac79u,
        0xa0dfu, 0xa0e6u, 13u);
}

Lufia2ExecutionResult Lufia2BattleHdmaBeginTransitionMode14(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return BeginTransitionPattern(memory, cpu, child, context,
        0x85acdfu, 0x85ad41u, 0x85acf6u, 0x85ad42u,
        0xa10du, 0xa10du, 14u);
}

Lufia2ExecutionResult Lufia2BattleHdmaBuildTransitionMode13(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    return BuildTransitionPattern(memory, cpu, 0x85ac79u, 0x85acdeu, 0x85a0edu);
}

Lufia2ExecutionResult Lufia2BattleHdmaBuildTransitionMode14(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    return BuildTransitionPattern(memory, cpu, 0x85ad42u, 0x85ada7u, 0x85a10du);
}
