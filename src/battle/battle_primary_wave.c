#include "lufia2/battle.h"
#include "primary_wave_internal.h"
#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/plain_ops.h"
#include "core/wram_view.h"
#include "system/wram.h"

enum {
    PRIMARY_WAVE_PHASE = WRAM_BATTLE_RESULT_SCROLL_PHASE,
    PRIMARY_WAVE_FIXED = WRAM_BATTLE_MESSAGE_SCROLL_INCREMENT,
    PRIMARY_WAVE_READY = WRAM_BATTLE_RESULT_SCROLL_READY,
    PRIMARY_WAVE_TABLE = WRAM_FIELD_MAP_ATTRIBUTES + 0xccu,
    PRIMARY_WAVE_INDEX = WRAM_BATTLE_TRANSITION_MASK,
    PRIMARY_WAVE_DESCRIPTOR = 0x1b12u,
    PRIMARY_WAVE_PERIOD = 0x33u
};

static uint8_t PrimaryWaveWidths(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit;
}

static void FillFixedWave(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint16_t values[] = {0x15fu, 0x15bu, 0x11fu};
    static const uint16_t counts[] = {8u, 56u, 8u};
    PushStackWord(memory, cpu, cpu->x);
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7eu);
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint16_t offset = 0u;
    for (unsigned band = 0u; band < 3u; ++band)
        for (uint16_t left = counts[band]; left; --left) {
            WramWrite16At(wram, PRIMARY_WAVE_TABLE & 0xffffu,
                offset, values[band]);
            offset = (uint16_t)(offset + 2u);
        }
    cpu->accumulator = values[2];
    cpu->y = 0u;
    PullDataBank(memory, cpu);
    cpu->x = PullStackWord(memory, cpu);
}

static void FillMovingWave(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram caller = WramViewOfCaller(memory, cpu);
    const uint8_t phase = WramRead(caller, PRIMARY_WAVE_PHASE);
    PushStackWord(memory, cpu, cpu->x);
    SetIndexWidth(cpu, 1u);
    WramWrite(caller, PRIMARY_WAVE_PERIOD, phase & 3u);
    const Byte8Result distance = Difference8Mode(4u,
        WramRead(caller, PRIMARY_WAVE_PERIOD), cpu->decimal);
    WramWrite(caller, PRIMARY_WAVE_PERIOD, distance.value);
    const Byte8Result period = Sum8Mode((uint8_t)(distance.value << 1),
        WramRead(caller, PRIMARY_WAVE_PERIOD), distance.value >> 7, cpu->decimal);
    const Byte8Result index = Sum8Mode((uint8_t)(phase << 1), 8u, 0u, cpu->decimal);
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7eu);
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint16_t value = Read16Long(memory, 0x859ffau + index.value);
    uint8_t remaining = period.value;
    cpu->overflow = index.overflow;
    for (uint16_t offset = 0u; offset < 0x90u; offset += 2u) {
        WramWrite16At(wram, PRIMARY_WAVE_TABLE & 0xffffu, offset, value);
        remaining = (uint8_t)(remaining - 1u);
        if (!remaining) {
            remaining = 12u;
            const Word16Result next = Sum16Mode(value, 4u, 0u, cpu->decimal);
            value = next.value;
            cpu->overflow = next.overflow;
        }
    }
    cpu->accumulator = value;
    cpu->y = remaining;
    cpu->carry = 1u;
    PullDataBank(memory, cpu);
    SetIndexWidth(cpu, 0u);
    cpu->x = PullStackWord(memory, cpu);
}

uint32_t Lufia2BattleFillPrimaryWave(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const uint8_t fixed = WramRead(wram, PRIMARY_WAVE_FIXED);
    if (fixed)
        FillFixedWave(memory, cpu);
    else
        FillMovingWave(memory, cpu);
    LoadA8(cpu, 1u);
    WramWrite(wram, PRIMARY_WAVE_READY, 1u);
    return fixed ? 0x85a971u : 0x85a932u;
}

Lufia2ExecutionResult Lufia2BattleBuildPrimaryWaveTable(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    if (!PrimaryWaveWidths(cpu) || cpu->program_bank != 0x85u)
        return ExecutionHandoff(cpu, 0x85a8e7u);
    return ExecutionReturned(Lufia2BattleFillPrimaryWave(memory, cpu));
}

static void FillPrimaryOffsets(Lufia2Wram wram, uint16_t *offset,
    uint16_t count, uint16_t value, uint8_t descending) {
    for (; count; --count) {
        WramWrite16At(wram, WRAM_FIELD_MAP_ATTRIBUTES, *offset, value);
        *offset = (uint16_t)(*offset + 2u);
        if (descending)
            value = (uint16_t)(value - 1u);
    }
}

static void ConfigurePrimaryDescriptor(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t descriptor, uint8_t control,
    uint8_t reg, uint16_t table) {
    OpLoadA(cpu, control);
    OpSta(memory, cpu, OpAbs(cpu, descriptor));
    OpLoadA(cpu, reg);
    OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(descriptor + 1u)));
    OpLdx(cpu, table);
    OpWriteX(memory, cpu, OpAbs(cpu, (uint16_t)(descriptor + 2u)), cpu->x);
    OpLoadA(cpu, 0x85u);
    OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(descriptor + 4u)));
}

Lufia2ExecutionResult Lufia2BattleBeginPrimaryWave(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!PrimaryWaveWidths(cpu) || !child || cpu->program_bank != 0x85u)
        return ExecutionHandoff(cpu, 0x85a804u);
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    cpu->y = 0u;
    cpu->x = 0u;
    WramWrite16(wram, PRIMARY_WAVE_PHASE, 0u);
    uint16_t offset = 0u;
    FillPrimaryOffsets(wram, &offset, 8u, 0u, 1u);
    FillPrimaryOffsets(wram, &offset, 28u, 0x94u, 0u);
    FillPrimaryOffsets(wram, &offset, 32u, 0xff70u, 1u);
    FillPrimaryOffsets(wram, &offset, 32u, 0xff40u, 1u);
    WramWrite16At(wram, WRAM_FIELD_MAP_ATTRIBUTES, offset, 0xfff4u);
    WramWrite16At(wram, WRAM_FIELD_MAP_ATTRIBUTES, (uint16_t)(offset + 2u), 0x17u);
    LoadA16(cpu, 0x17u);
    LoadX16(cpu, 0u);
    cpu->carry = 1u;
    WramWrite16(wram, PRIMARY_WAVE_PHASE, 0u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x85a85fu, 0x85a8e7u, 2u, 0x85u)) {
        Lufia2ExecutionResult result = ExecutionReturned(0x85a85fu);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    if (!PrimaryWaveWidths(cpu))
        return ExecutionHandoff(cpu, 0x85a862u);
    ConfigurePrimaryDescriptor(memory, cpu, PRIMARY_WAVE_DESCRIPTOR,
        0x42u, 0x12u, 0x9feau);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpAbs(cpu, 0x4377u));
    ConfigurePrimaryDescriptor(memory, cpu, WRAM_BATTLE_TRANSITION_HDMA_CONTROL,
        0x41u, 0x28u, 0xa024u);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpAbs(cpu, 0x4317u));
    TransferDirectToA(cpu);
    for (unsigned index = 0u; index < 4u; ++index) {
        static const uint16_t order[] = {0u, 2u, 1u, 3u};
        OpSta(memory, cpu, PRIMARY_WAVE_INDEX + order[index]);
    }
    ConfigurePrimaryDescriptor(memory, cpu, WRAM_BATTLE_RESULT_COLOR_HDMA_DESCRIPTOR,
        1u, 0x30u, 0xa013u);
    ConfigurePrimaryDescriptor(memory, cpu, WRAM_BATTLE_LAYER_HDMA_DESCRIPTOR,
        1u, 0x2cu, 0xa034u);
    OpLoadA(cpu, 0xb2u);
    OpTestBits(memory, cpu, OpDp(cpu, 0xdau), 1u);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_RESULT_HDMA_CHANNELS));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_RESULT_HDMA_REQUEST));
    OpStz(memory, cpu, OpAbs(cpu, PRIMARY_WAVE_FIXED));
    OpLoadA(cpu, 2u);
    OpTestBits(memory, cpu, OpDp(cpu, 0xdbu), 1u);
    return ExecutionReturned(0x85a8e6u);
}
