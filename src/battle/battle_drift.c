/* Battle party sprite drift: the random bit source and the per-record
 * wobble that updates the two offset words of each party record. */

#include <stdbool.h>

#include "core/cpu_internal.h"
#include "core/plain_ops.h"
#include "core/wram_view.h"
#include "lufia2/battle.h"

enum {
    RANDOM_TOP = 0x122fu,
    RANDOM_WORDS = 0x1230u,
    RECORDS = 0x13dau,
    RECORD_SIZE = 0x000fu,
    RECORD_END = 0x005au,
    TIMERS = 0x152au,
    SHAKE_TABLE = 0x859e37u,
    ACTIVE_FLAG = 0x129au,
    BIT_SHAKE = 0x01u,
    BIT_FLIP = 0x02u,
    BIT_RANDOM = 0x20u,
    STATE_ACTIVE = 0x80u,
    RANDOM_WORD_COUNT = 5,
    SHAKE_STEP_MASK = 0xfeu,
    SHAKE_TIMER_START = 0x3fu,
    FLIP_TIMER_START = 0x02u,
    RANDOM_TIMER_START = 0x0cu,
    DRIFT_BANK = 0x85u,
    OFFSET_X = 0x09u,
    OFFSET_Y = 0x0bu
};

/* A word rotated left through the carry; the high byte is stored first. */
static bool RotateWordLeft(Lufia2Wram wram, uint32_t location, bool carry) {
    const uint16_t old = WramRead16(wram, location);
    const uint16_t value = (uint16_t)((old << 1) | (carry ? 1u : 0u));
    const uint32_t low = WramAddress(wram, location, 0);

    Write8(wram.memory, WramNextAddress(location, low), (uint8_t)(value >> 8));
    Write8(wram.memory, low, (uint8_t)value);
    return (old & 0x8000u) != 0;
}

/* The 88-bit register at $122F moves left by one bit: bit 0 of its first
 * byte enters at the far end, and the bit that comes out of the far end
 * becomes bit 0 of the new first byte. The original forms that byte by
 * rotating the low byte of the direct page register (zero in the game) with
 * the carry. Returns the byte stored; `carry` receives the bit rotated out of
 * the direct page byte. */
static uint8_t ShiftRandomRegister(Lufia2Wram wram, bool *carry) {
    bool shifted = (WramRead(wram, RANDOM_TOP) & 1u) != 0;
    uint8_t top;
    int word;

    for (word = RANDOM_WORD_COUNT - 1; word >= 0; --word)
        shifted = RotateWordLeft(
            wram, RANDOM_WORDS + 2u * (uint32_t)word, shifted);
    top = (uint8_t)((wram.direct_page << 1) | (shifted ? 1u : 0u));
    WramWrite(wram, RANDOM_TOP, top);
    *carry = (wram.direct_page & 0x80u) != 0;
    return top;
}

/* $85:8F4A: shifts the 88-bit register at $122F left by one through the
 * carry; the bit shifted in is bit 0 of $122F before the shift. M8. */
Lufia2ExecutionResult Lufia2BattleRandomBit(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    bool carry;
    uint8_t top;

    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x858f4au);
    top = ShiftRandomRegister(wram, &carry);
    cpu->accumulator = (uint16_t)((cpu->direct_page & 0xff00u) | top);
    cpu->carry = carry;
    SetNz8(cpu, top);
    return ExecutionReturned(0x858f66u);
}

/* One JSR to the random bit routine; the offset word it yields is all ones
 * unless the byte stored came out zero, when only the direct page's high
 * byte is left. */
static uint16_t RandomOffset(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2Wram wram, uint16_t return_address) {
    bool carry;
    uint8_t top;

    SimulateJsrFrame(memory, cpu, return_address);
    top = ShiftRandomRegister(wram, &carry);
    SimulateRtsFrame(memory, cpu);
    return top == 0 ? (uint16_t)(wram.direct_page & 0xff00u) : 0xffffu;
}

/* Counts a record's timer down; returns the new value. */
static uint8_t StepTimer(Lufia2Wram wram, uint16_t slot) {
    const uint8_t value = (uint8_t)(WramReadAt(wram, TIMERS, slot) - 1u);

    WramWriteAt(wram, TIMERS, slot, value);
    return value;
}

/* The shake table is read at the timer's step; the direct page's high byte
 * is part of the index (the game keeps it zero). */
static uint16_t ShakeOffset(
    const Lufia2Memory *memory, const Lufia2Wram wram, uint16_t slot) {
    const uint8_t step = (uint8_t)(
        (WramReadAt(wram, TIMERS, slot) >> 2) & SHAKE_STEP_MASK);
    const uint16_t index = (uint16_t)((wram.direct_page & 0xff00u) | step);

    return WramRead16At(WramViewLong(memory), SHAKE_TABLE, index);
}

/* $85:894A: updates the offsets of the six party records at $13DA. The
 * state bits of a record select the shake table, a sign flip or random
 * offsets. M8/X16, JSL. */
Lufia2ExecutionResult Lufia2BattleDriftRecords(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result = ExecutionReturned(0x8589e4u);
    Lufia2Wram wram;
    uint16_t slot = 0;
    uint16_t record;
    uint16_t offset;
    uint8_t active;

    result.dispatches = 0;
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x85894au);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, DRIFT_BANK);
    PullDataBank(memory, cpu);
    wram = WramViewOfCaller(memory, cpu);
    active = WramRead(wram, ACTIVE_FLAG);
    if ((active & STATE_ACTIVE) == 0) {
        LoadA8(cpu, active);
        PullDataBank(memory, cpu);
        return result;
    }
    for (record = 0; record != RECORD_END;
         record = (uint16_t)(record + RECORD_SIZE), ++slot) {
        const uint8_t mode = WramReadAt(wram, RECORDS, record);

        if ((WramReadAt(wram, RECORDS + 1u, record) & STATE_ACTIVE) == 0)
            continue;
        if ((mode & BIT_SHAKE) != 0) {
            if ((StepTimer(wram, slot) & 0x80u) != 0)
                WramWriteAt(wram, TIMERS, slot, SHAKE_TIMER_START);
            PushStackWord(memory, cpu, slot);
            offset = ShakeOffset(memory, wram, slot);
            (void)PullStackWord(memory, cpu);
            WramWrite16At(wram, RECORDS + OFFSET_Y, record, offset);
        } else if ((mode & BIT_FLIP) != 0) {
            if (StepTimer(wram, slot) == 0) {
                WramWriteAt(wram, TIMERS, slot, FLIP_TIMER_START);
                WramWrite16At(wram, RECORDS + OFFSET_X, record,
                    (uint16_t)(WramRead16At(wram, RECORDS + OFFSET_X, record) ^
                        0xffffu));
            }
        } else if ((mode & BIT_RANDOM) != 0) {
            if (StepTimer(wram, slot) != 0)
                continue;
            WramWriteAt(wram, TIMERS, slot, RANDOM_TIMER_START);
            WramWrite16At(wram, RECORDS + OFFSET_X, record,
                RandomOffset(memory, cpu, wram, 0x8979u));
            WramWrite16At(wram, RECORDS + OFFSET_Y, record,
                RandomOffset(memory, cpu, wram, 0x8988u));
        }
    }
    /* The step to the end of the table neither overflows nor leaves a
     * carry; the compare with the end sets it. */
    cpu->x = slot;
    cpu->y = RECORD_END;
    cpu->accumulator = RECORD_END;
    cpu->carry = true;
    cpu->overflow = false;
    PullDataBank(memory, cpu);
    return result;
}
