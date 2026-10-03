/* Cursor interpolation, yielding a frame every sixteen steps. */

#include <stdbool.h>

#include "core/cpu_internal.h"
#include "core/plain_ops.h"
#include "core/wram_view.h"
#include "lufia2/menu.h"

enum {
    SLOT_X = 0x1388u,
    SLOT_Y = 0x13e8u,
    SCRATCH_SLOT = 0x00u,
    SCRATCH_ROW = 0x01u,
    FROM_X = 0xdfu,
    FROM_Y = 0xe0u,
    TO_X = 0xe1u,
    TO_Y = 0xe2u,
    SPAN_X = 0xe1u,                     /* TO_X and TO_Y, reused for the span */
    SPAN_Y = 0xe2u,
    DELTA_X = 0xe3u,
    DELTA_Y = 0xe4u,
    DIRECTION_X = 0xe5u,
    DIRECTION_Y = 0xe6u,
    WAIT_COUNT = 0xedu,
    WAIT_EVERY = 0x10u,
    DIRECTION_BACK = 0xffu,
    SLIDE_DONE = 0x828b07u
};

/* Move one axis and grow the other axis error. */
static void CorrectStep(Lufia2Wram wram, Lufia2CpuState *cpu, uint32_t slots,
    uint32_t direction, uint32_t error, uint32_t span) {
    Byte8Result moved;
    Byte8Result grown;

    moved.value = WramReadAt(wram, slots, cpu->y);
    moved = Sum8Mode(moved.value, WramRead(wram, direction), false, cpu->decimal);
    WramWriteAt(wram, slots, cpu->y, moved.value);
    grown.value = WramRead(wram, error);
    grown = Sum8Mode(grown.value, WramRead(wram, span), false, cpu->decimal);
    WramWrite(wram, error, grown.value);
    LeaveByteSum(cpu, grown);
}

/* $82:8AD8: horizontal correction step. */
Lufia2ExecutionResult Lufia2MenuSlideCorrectX(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x828ad8u);
    CorrectStep(WramViewOfCaller(memory, cpu), cpu, SLOT_X, DIRECTION_X,
        DELTA_Y, SPAN_Y);
    return ExecutionReturned(0x828ae8u);
}

/* $82:8AE9: the vertical counterpart of the above. */
Lufia2ExecutionResult Lufia2MenuSlideCorrectY(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x828ae9u);
    CorrectStep(WramViewOfCaller(memory, cpu), cpu, SLOT_Y, DIRECTION_Y,
        DELTA_X, SPAN_X);
    return ExecutionReturned(0x828af9u);
}

/* $82:8AFA: yield before the frame-wait JSL every sixteenth step. */
Lufia2ExecutionResult Lufia2MenuSlideCount(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);

    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x828afau);
    SetNz8(cpu, WramStep8(wram, WAIT_COUNT, -1));
    if (!cpu->zero)
        return ExecutionReturned(0x828b06u);
    LoadA8(cpu, WAIT_EVERY);
    WramWrite(wram, WAIT_COUNT, WAIT_EVERY);
    return ExecutionHandoff(cpu, 0x828b02u);
}

/* A frame wait retains the count routine's pushed return. */
static bool SlideStep(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    bool overflow, uint16_t return_address, Lufia2ExecutionResult *result) {
    cpu->overflow = overflow;
    SimulateJsrFrame(memory, cpu, return_address);
    *result = Lufia2MenuSlideCount(memory, cpu);
    if (result->flow != LUFIA2_EXECUTION_RETURNED)
        return true;
    SimulateRtsFrame(memory, cpu);
    return false;
}

/* JSR to one of the two correction steps. */
static void Correct(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    bool vertical, uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    (void)(vertical ? Lufia2MenuSlideCorrectY(memory, cpu)
                    : Lufia2MenuSlideCorrectX(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}

/* Direction is negative when the destination exceeds the start. */
static void Distance(Lufia2Wram wram, Lufia2CpuState *cpu, uint32_t from,
    uint32_t to, uint32_t delta, uint32_t direction) {
    const uint8_t start = WramRead(wram, from);
    const uint8_t end = WramRead(wram, to);
    const bool backwards = start < end;
    const Byte8Result gap =
        backwards ? Difference8Mode(end, start, cpu->decimal)
                  : Difference8Mode(start, end, cpu->decimal);

    WramWrite(wram, delta, gap.value);
    WramWrite(wram, direction, backwards ? DIRECTION_BACK : 1u);
    cpu->overflow = gap.overflow;
}

/* A step of a slot position along or against its direction. */
static Byte8Result SlideAdd(Lufia2Wram wram, bool decimal, uint16_t slot, uint32_t table,
    uint32_t direction) {
    Byte8Result moved;

    moved.value = WramReadAt(wram, table, slot);
    moved = Sum8Mode(moved.value, WramRead(wram, direction), false, decimal);
    WramWriteAt(wram, table, slot, moved.value);
    return moved;
}

static void SlideSubtract(Lufia2Wram wram, bool decimal, uint16_t slot, uint32_t table,
    uint32_t direction) {
    Byte8Result moved;

    moved.value = WramReadAt(wram, table, slot);
    moved = Difference8Mode(moved.value, WramRead(wram, direction), decimal);
    WramWriteAt(wram, table, slot, moved.value);
}

/* Borrow from the error subtraction requests an axis correction. */
static bool ErrorRunsOut(Lufia2Wram wram, bool decimal, uint32_t error, uint32_t other) {
    Byte8Result left;

    left.value = WramRead(wram, error);
    left = Difference8Mode(left.value, WramRead(wram, other), decimal);
    WramWrite(wram, error, left.value);
    return !left.carry;
}

/* $82:89FA: slide between slots, preserving the frame-wait continuation. */
Lufia2ExecutionResult Lufia2MenuCursorSlide(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    Lufia2ExecutionResult result;
    const uint16_t other = cpu->x;
    uint16_t slot;
    Byte8Result moved;
    uint8_t position;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x8289fau);
    WramWrite16(wram, SCRATCH_SLOT, other);
    if (A8(cpu) != 0)
        WramWrite16(wram, SCRATCH_SLOT, cpu->y);
    WramWrite(wram, FROM_X, WramReadAt(wram, SLOT_X, cpu->y));
    WramWrite(wram, FROM_Y, WramReadAt(wram, SLOT_Y, cpu->y));
    WramWrite(wram, TO_X, WramReadAt(wram, SLOT_X, other));
    WramWrite(wram, TO_Y, WramReadAt(wram, SLOT_Y, other));
    slot = WramRead16(wram, SCRATCH_SLOT);
    cpu->y = slot;
    WramWriteAt(wram, SLOT_X, slot, WramRead(wram, TO_X));
    WramWriteAt(wram, SLOT_Y, slot, WramRead(wram, TO_Y));
    WramWrite(wram, WAIT_COUNT, WAIT_EVERY);
    Distance(wram, cpu, FROM_X, TO_X, DELTA_X, DIRECTION_X);
    Distance(wram, cpu, FROM_Y, TO_Y, DELTA_Y, DIRECTION_Y);
    /* The spans reuse the words of the destination. */
    cpu->x = WramRead16(wram, DELTA_X);
    WramWrite16(wram, SPAN_X, cpu->x);
    LoadA8(cpu, WramRead(wram, DELTA_X));
    Compare8(cpu, A8(cpu), WramRead(wram, DELTA_Y));
    if (!cpu->carry) {
        SlideSubtract(wram, cpu->decimal, slot, SLOT_Y, DIRECTION_Y);
        for (;;) {
            if (ErrorRunsOut(wram, cpu->decimal, DELTA_Y, DELTA_X))
                Correct(memory, cpu, false, 0x8a80u);
            moved = SlideAdd(wram, cpu->decimal, slot, SLOT_Y, DIRECTION_Y);
            LoadA8(cpu, moved.value);
            Compare8(cpu, moved.value, WramRead(wram, FROM_Y));
            if (cpu->zero) {
                position = WramReadAt(wram, SLOT_X, slot);
                LoadA8(cpu, position);
                cpu->overflow = moved.overflow;
                Compare8(cpu, position, WramRead(wram, FROM_X));
                if (cpu->zero)
                    return ExecutionReturned(SLIDE_DONE);
            }
            if (SlideStep(memory, cpu, moved.overflow, 0x8a9au, &result))
                return result;
        }
    }
    SlideSubtract(wram, cpu->decimal, slot, SLOT_X, DIRECTION_X);
    for (;;) {
        if (ErrorRunsOut(wram, cpu->decimal, DELTA_X, DELTA_Y))
            Correct(memory, cpu, true, 0x8ab1u);
        moved = SlideAdd(wram, cpu->decimal, slot, SLOT_X, DIRECTION_X);
        WramWrite(wram, SCRATCH_SLOT, moved.value);
        WramWrite(wram, SCRATCH_ROW, WramReadAt(wram, SLOT_Y, slot));
        position = WramReadAt(wram, SLOT_X, slot);
        LoadA8(cpu, position);
        Compare8(cpu, position, WramRead(wram, FROM_X));
        if (cpu->zero) {
            position = WramReadAt(wram, SLOT_Y, slot);
            LoadA8(cpu, position);
            cpu->overflow = moved.overflow;
            Compare8(cpu, position, WramRead(wram, FROM_Y));
            if (cpu->zero)
                return ExecutionReturned(SLIDE_DONE);
        }
        if (SlideStep(memory, cpu, moved.overflow, 0x8ad5u, &result))
            return result;
    }
}
