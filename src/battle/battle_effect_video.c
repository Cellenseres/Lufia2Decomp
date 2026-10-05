#include "core/cpu_ops.h"
#include "core/plain_ops.h"
#include "core/snes_registers.h"
#include "core/wram_view.h"
#include "lufia2/battle.h"

enum {
    EFFECT_STREAM = 0xc3u,
    VIDEO_SHADOW = 0x123cu,
    WINDOW_SELECT = 0x123eu,
    WINDOW_LEFT = 0x1241u,
    WINDOW_RIGHT = 0x1242u,
    BACKGROUND_MAP_STATE = 0x1260u,
    BACKGROUND_REQUEST = 0x15b4u,
    EFFECT_SCRATCH = 0x15edu,
    SLOT_RADIUS = 0x13u,
    BACKGROUND_MAP = 0x2000u,
    BACKGROUND_MAP_COPY = 0x3800u,
    BACKGROUND_MAP_BYTES = 0x800u,
    WORK_BANK = 0x7eu
};

static bool VideoContext(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit &&
        cpu->direct_page == 0u && cpu->program_bank == 0x81u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static uint8_t StreamByte(const Lufia2Memory *memory,
                          const Lufia2CpuState *cpu) {
    return Read8(memory, DirectLongPointer(memory, cpu, EFFECT_STREAM));
}

static void AdvanceStream(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const uint16_t next = WramStep16(
        WramViewOfCaller(memory, cpu), EFFECT_STREAM, 1);
    SetNz16(cpu, next);
}

Lufia2ExecutionResult Lufia2BattleEffectVideoRegister(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    uint8_t offset;
    uint8_t value;

    if (!VideoContext(cpu))
        return ExecutionHandoff(cpu, 0x81963au);
    offset = StreamByte(memory, cpu);
    cpu->accumulator = offset;
    cpu->x = offset;
    AdvanceStream(memory, cpu);
    value = StreamByte(memory, cpu);
    LoadA8(cpu, value);
    Write8(memory, VIDEO_SHADOW + offset, value);
    AdvanceStream(memory, cpu);
    return ExecutionReturned(0x819652u);
}

Lufia2ExecutionResult Lufia2BattleEffectBg3Map(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    uint8_t map;

    if (!VideoContext(cpu))
        return ExecutionHandoff(cpu, 0x819653u);
    map = StreamByte(memory, cpu);
    AdvanceStream(memory, cpu);
    LoadA8(cpu, (uint8_t)(map | 8u));
    Write8(memory, SNES_BG3SC, A8(cpu));
    return ExecutionReturned(0x819661u);
}

Lufia2ExecutionResult Lufia2BattleEffectBackgroundRelease(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2Wram wram;

    if (!VideoContext(cpu))
        return ExecutionHandoff(cpu, 0x8199a5u);
    wram = WramViewOfCaller(memory, cpu);
    LoadA8(cpu, 0xffu);
    WramWrite(wram, BACKGROUND_MAP_STATE, A8(cpu));
    LoadA8(cpu, 4u);
    WramWrite(wram, BACKGROUND_REQUEST, A8(cpu));
    return ExecutionReturned(0x8199afu);
}

Lufia2ExecutionResult Lufia2BattleEffectBackgroundRequest(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!VideoContext(cpu))
        return ExecutionHandoff(cpu, 0x819999u);
    LoadA8(cpu, StreamByte(memory, cpu));
    WramWrite(WramViewOfCaller(memory, cpu), BACKGROUND_REQUEST, A8(cpu));
    AdvanceStream(memory, cpu);
    return ExecutionReturned(0x8199a4u);
}

Lufia2ExecutionResult Lufia2BattleEffectBackgroundCopy(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2Wram wram;

    if (!VideoContext(cpu))
        return ExecutionHandoff(cpu, 0x8199b0u);
    OpSetDataBank(memory, cpu, WORK_BANK);
    wram = WramViewOfCaller(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    cpu->x = BACKGROUND_MAP_BYTES - 2u;
    do {
        const uint16_t tile = WramRead16At(wram, BACKGROUND_MAP, cpu->x);
        cpu->accumulator = tile;
        WramWrite16At(wram, BACKGROUND_MAP_COPY, cpu->x, tile);
        cpu->x = (uint16_t)(cpu->x - 2u);
        SetNz16(cpu, cpu->x);
    } while (!cpu->negative);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 1u);
    WramWrite(wram, BACKGROUND_MAP_STATE, A8(cpu));
    LoadA8(cpu, 4u);
    WramWrite(wram, BACKGROUND_REQUEST, A8(cpu));
    return ExecutionReturned(0x8199cfu);
}

Lufia2ExecutionResult Lufia2BattleEffectWindowBand(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2Wram wram;
    uint8_t offset;
    uint8_t radius;
    Word16Result slot;

    if (!VideoContext(cpu))
        return ExecutionHandoff(cpu, 0x819ab1u);
    OpSetDataBank(memory, cpu, WORK_BANK);
    wram = WramViewOfCaller(memory, cpu);
    offset = StreamByte(memory, cpu);
    AdvanceStream(memory, cpu);
    WramWrite16(wram, EFFECT_SCRATCH, offset);
    slot = Sum16Mode(cpu->y, WramRead16(wram, EFFECT_SCRATCH),
                     false, cpu->decimal);
    cpu->x = slot.value;
    LeaveSum(cpu, slot);
    SetAccumulatorWidth(cpu, 1);
    radius = Read8(memory, ((uint32_t)WORK_BANK << 16) + SLOT_RADIUS + cpu->x);
    LoadA8(cpu, radius);
    Compare8(cpu, radius, 124u);
    if (radius < 124u) {
        WramWrite(wram, EFFECT_SCRATCH, radius);
        LeaveByteSum(cpu, Sum8Mode(radius, 127u, false, cpu->decimal));
        WramWrite(wram, WINDOW_RIGHT, A8(cpu));
        LoadA8(cpu, 128u);
        LeaveByteSum(cpu, Difference8Mode(A8(cpu),
            WramRead(wram, EFFECT_SCRATCH), cpu->decimal));
        WramWrite(wram, WINDOW_LEFT, A8(cpu));
        LoadA8(cpu, 0xc3u);
        WramWrite(wram, WINDOW_SELECT, A8(cpu));
        return ExecutionReturned(0x819ae7u);
    }
    /* Empty the second window. */
    LoadA8(cpu, 0xffu);
    WramWrite(wram, WINDOW_LEFT, A8(cpu));
    cpu->accumulator = cpu->direct_page;
    SetNz16(cpu, cpu->accumulator);
    WramWrite(wram, WINDOW_RIGHT, A8(cpu));
    LoadA8(cpu, 0x33u);
    WramWrite(wram, WINDOW_SELECT, A8(cpu));
    return ExecutionReturned(0x819af6u);
}
