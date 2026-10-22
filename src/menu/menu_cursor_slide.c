/* Cursor interpolation, yielding a frame every sixteen steps. */

#include <stdbool.h>

#include "core/cpu_internal.h"
#include "core/child_call.h"
#include "core/plain_ops.h"
#include "core/wram_view.h"
#include "lufia2/menu.h"
#include "lufia2/menu_cursor_step.h"
#include "menu/menu_sprite_slots.h"

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
    STEP_BUDGET = 4096u,
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

/* $82:8AFA up to its JSL: count down, reload at zero. */
static Lufia2ExecutionResult DecrementWaitCount(
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

/* $82:8AFA: every sixteenth step runs the $86:8B55 sprite frame. */
Lufia2ExecutionResult Lufia2MenuSlideCount(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    Lufia2ExecutionResult result;

    if (!child || cpu->program_bank != 0x82u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page != 0u || cpu->decimal ||
        cpu->stack < 0x1f10u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x828afau);
    result = DecrementWaitCount(memory, cpu);
    if (result.flow == LUFIA2_EXECUTION_RETURNED)
        return result;
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x828b02u, 0x868b55u, 3u, 0x82u)) {
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        result.pc = cpu->resume_pc = 0x828b02u;
        return result;
    }
    return ExecutionReturned(0x828b06u);
}

/* A frame wait retains the count routine's pushed return. */
static bool SlideStep(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    bool overflow, uint16_t return_address, Lufia2ExecutionResult *result,
    Lufia2PushedChildCall child, void *context) {
    const uint32_t site = 0x820000u | (uint16_t)(return_address - 2u);
    const uint8_t data_bank = cpu->data_bank;
    const uint16_t direct_page = cpu->direct_page;

    cpu->overflow = overflow;
    if (!CallChildWithFrame(memory, cpu, child, context,
            site, 0x828afau, 2u, 0x82u)) {
        result->flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        result->pc = cpu->resume_pc = site;
        return true;
    }
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal ||
        cpu->data_bank != data_bank || cpu->direct_page != direct_page) {
        *result = ExecutionHandoff(cpu, site + 3u);
        return true;
    }
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
    const uint8_t larger = WramRead(wram, backwards ? to : from);
    const uint8_t smaller = WramRead(wram, backwards ? from : to);
    const Byte8Result gap = Difference8Mode(larger, smaller, cpu->decimal);

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
static Byte8Result SubtractCursorError(
    Lufia2Wram wram, bool decimal, uint32_t error, uint32_t other) {
    Byte8Result left;

    left.value = WramRead(wram, error);
    left = Difference8Mode(left.value, WramRead(wram, other), decimal);
    WramWrite(wram, error, left.value);
    return left;
}

static bool ErrorRunsOut(Lufia2Wram wram, bool decimal, uint32_t error, uint32_t other) {
    return !SubtractCursorError(wram, decimal, error, other).carry;
}

static bool SlideRegistersSupported(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x82u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->direct_page && !cpu->decimal &&
        cpu->stack >= 0x1f12u && cpu->stack <= 0x1ffcu &&
        cpu->y < MENU_SPRITE_SLOTS && (cpu->data_bank < 0x40u ||
            (cpu->data_bank >= 0x80u && cpu->data_bank < 0xc0u));
}

static bool SlideContext(const Lufia2CpuState *cpu,
    Lufia2PushedChildCall child) {
    return child && SlideRegistersSupported(cpu);
}

static bool AdvanceCursorVertically(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const uint16_t slot = cpu->y;
    const Byte8Result moved =
        SlideAdd(wram, cpu->decimal, slot, SLOT_Y, DIRECTION_Y);
    LoadA8(cpu, moved.value);
    Compare8(cpu, moved.value, WramRead(wram, FROM_Y));
    if (cpu->zero) {
        const uint8_t position = WramReadAt(wram, SLOT_X, slot);
        LoadA8(cpu, position);
        Compare8(cpu, position, WramRead(wram, FROM_X));
    }
    cpu->overflow = moved.overflow;
    return cpu->zero != 0u;
}

static bool AdvanceCursorHorizontally(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const uint16_t slot = cpu->y;
    const Byte8Result moved =
        SlideAdd(wram, cpu->decimal, slot, SLOT_X, DIRECTION_X);
    WramWrite(wram, SCRATCH_SLOT, moved.value);
    WramWrite(wram, SCRATCH_ROW, WramReadAt(wram, SLOT_Y, slot));
    uint8_t position = WramReadAt(wram, SLOT_X, slot);
    LoadA8(cpu, position);
    Compare8(cpu, position, WramRead(wram, FROM_X));
    if (cpu->zero) {
        position = WramReadAt(wram, SLOT_Y, slot);
        LoadA8(cpu, position);
        Compare8(cpu, position, WramRead(wram, FROM_Y));
    }
    cpu->overflow = moved.overflow;
    return cpu->zero != 0u;
}

Lufia2ExecutionResult Lufia2MenuAdvanceCursorSlide(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2MenuSlideAxis axis, Lufia2MenuSlidePhase phase) {
    if ((axis != LUFIA2_MENU_SLIDE_HORIZONTAL && axis != LUFIA2_MENU_SLIDE_VERTICAL) ||
        (phase != LUFIA2_MENU_SLIDE_ERROR && phase != LUFIA2_MENU_SLIDE_POSITION))
        return ExecutionHandoff(cpu, 0u);
    const bool vertical = axis == LUFIA2_MENU_SLIDE_VERTICAL;
    if (!SlideRegistersSupported(cpu))
        return ExecutionHandoff(cpu, phase == LUFIA2_MENU_SLIDE_ERROR
            ? (vertical ? 0x828a75u : 0x828aa6u)
            : (vertical ? 0x828a81u : 0x828ab2u));
    if (phase == LUFIA2_MENU_SLIDE_ERROR) {
        const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
        const Byte8Result error = SubtractCursorError(wram, cpu->decimal,
            vertical ? DELTA_Y : DELTA_X, vertical ? DELTA_X : DELTA_Y);
        LeaveByteSum(cpu, error);
        return ExecutionHandoff(cpu, vertical
            ? (error.carry ? 0x828a81u : 0x828a7eu)
            : (error.carry ? 0x828ab2u : 0x828aafu));
    }
    if (vertical ? AdvanceCursorVertically(memory, cpu)
                 : AdvanceCursorHorizontally(memory, cpu))
        return ExecutionReturned(SLIDE_DONE);
    return ExecutionHandoff(cpu, vertical ? 0x828a98u : 0x828ad3u);
}

static Lufia2ExecutionResult SlideVertically(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    Lufia2ExecutionResult result;
    unsigned steps = 0;
    for (;;) {
        if (steps++ == STEP_BUDGET)
            return ExecutionHandoff(cpu, 0x828a75u);
        if (ErrorRunsOut(WramViewOfCaller(memory, cpu), cpu->decimal, DELTA_Y, DELTA_X))
            Correct(memory, cpu, false, 0x8a80u);
        if (AdvanceCursorVertically(memory, cpu))
            return ExecutionReturned(SLIDE_DONE);
        if (SlideStep(memory, cpu, cpu->overflow, 0x8a9au, &result, child, context))
            return result;
    }
}

static Lufia2ExecutionResult SlideHorizontally(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    Lufia2ExecutionResult result;
    unsigned steps = 0;
    for (;;) {
        if (steps++ == STEP_BUDGET)
            return ExecutionHandoff(cpu, 0x828aa6u);
        if (ErrorRunsOut(WramViewOfCaller(memory, cpu), cpu->decimal, DELTA_X, DELTA_Y))
            Correct(memory, cpu, true, 0x8ab1u);
        if (AdvanceCursorHorizontally(memory, cpu))
            return ExecutionReturned(SLIDE_DONE);
        if (SlideStep(memory, cpu, cpu->overflow, 0x8ad5u, &result, child, context))
            return result;
    }
}

/* $82:89FA: slide between slots, preserving the frame-wait continuation. */
Lufia2ExecutionResult Lufia2MenuCursorSlide(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const uint16_t other = cpu->x;
    uint16_t slot;

    if (!SlideContext(cpu, child) || cpu->x >= MENU_SPRITE_SLOTS)
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
        return SlideVertically(memory, cpu, child, context);
    }
    SlideSubtract(wram, cpu->decimal, slot, SLOT_X, DIRECTION_X);
    return SlideHorizontally(memory, cpu, child, context);
}

Lufia2ExecutionResult Lufia2MenuResumeHorizontalCursorSlide(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!SlideContext(cpu, child))
        return ExecutionHandoff(cpu, 0x828ad6u);
    return SlideHorizontally(memory, cpu, child, context);
}

Lufia2ExecutionResult Lufia2MenuResumeVerticalCursorSlide(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!SlideContext(cpu, child))
        return ExecutionHandoff(cpu, 0x828a9bu);
    return SlideVertically(memory, cpu, child, context);
}
