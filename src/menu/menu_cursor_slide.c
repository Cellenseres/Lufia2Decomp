/* Menu cursor slide: a sprite is moved from its own position to the position
 * of another slot along a straight line, one step at a time, with a frame wait
 * after every sixteen steps. */

#include "core/cpu_internal.h"
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
    SPRITE_FRAME = 0x868b55u,
    SLIDE_DONE = 0x828b07u
};

static void StoreAWork(Lufia2Wram wram, const Lufia2CpuState *cpu, uint32_t at) {
    WramWrite(wram, at, A8(cpu));
}

/* $82:8AD8: the slot's horizontal position takes one step and the vertical
 * error grows by the destination's vertical distance. */
Lufia2ExecutionResult Lufia2MenuSlideCorrectX(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);

    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x828ad8u);
    LoadA8(cpu, WramReadAt(wram, SLOT_X, cpu->y));
    cpu->carry = 0;
    Adc8(cpu, WramRead(wram, DIRECTION_X));
    WramWriteAt(wram, SLOT_X, cpu->y, A8(cpu));
    LoadA8(cpu, WramRead(wram, DELTA_Y));
    cpu->carry = 0;
    Adc8(cpu, WramRead(wram, SPAN_Y));
    StoreAWork(wram, cpu, DELTA_Y);
    return ExecutionReturned(0x828ae8u);
}

/* $82:8AE9: the vertical counterpart of the above. */
Lufia2ExecutionResult Lufia2MenuSlideCorrectY(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);

    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x828ae9u);
    LoadA8(cpu, WramReadAt(wram, SLOT_Y, cpu->y));
    cpu->carry = 0;
    Adc8(cpu, WramRead(wram, DIRECTION_Y));
    WramWriteAt(wram, SLOT_Y, cpu->y, A8(cpu));
    LoadA8(cpu, WramRead(wram, DELTA_X));
    cpu->carry = 0;
    Adc8(cpu, WramRead(wram, SPAN_X));
    StoreAWork(wram, cpu, DELTA_X);
    return ExecutionReturned(0x828af9u);
}

/* $82:8AFA: counts a step; every sixteenth step the sprites are shown for a
 * frame, which stays with the caller (the hand off is the JSL). */
Lufia2ExecutionResult Lufia2MenuSlideCount(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint8_t left;

    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x828afau);
    left = (uint8_t)(WramRead(wram, WAIT_COUNT) - 1u);
    WramWrite(wram, WAIT_COUNT, left);
    SetNz8(cpu, left);
    if (!cpu->zero)
        return ExecutionReturned(0x828b06u);
    LoadA8(cpu, WAIT_EVERY);
    StoreAWork(wram, cpu, WAIT_COUNT);
    return ExecutionHandoff(cpu, 0x828b02u);
}

/* One step of the loop, as a JSR to $82:8AFA: a hand off from the count stops
 * the slide with the count's frame pushed. */
static int SlideStep(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t return_address, Lufia2ExecutionResult *result) {
    SimulateJsrFrame(memory, cpu, return_address);
    *result = Lufia2MenuSlideCount(memory, cpu);
    if (result->flow != LUFIA2_EXECUTION_RETURNED)
        return 1;
    SimulateRtsFrame(memory, cpu);
    return 0;
}

static void Correct(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    int vertical, uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    (void)(vertical ? Lufia2MenuSlideCorrectY(memory, cpu)
                    : Lufia2MenuSlideCorrectX(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}

/* Absolute difference of two work bytes as the original forms it: the
 * larger minus the smaller, with a direction of 1, or -1 when the
 * destination is larger. */
static void Distance(Lufia2Wram wram, Lufia2CpuState *cpu, uint32_t from,
    uint32_t to, uint32_t delta, uint32_t direction) {
    LoadA8(cpu, WramRead(wram, from));
    Compare8(cpu, A8(cpu), WramRead(wram, to));
    if (!cpu->carry) {
        LoadA8(cpu, WramRead(wram, to));
        cpu->carry = 1;
        Sbc8(cpu, WramRead(wram, from));
        StoreAWork(wram, cpu, delta);
        LoadA8(cpu, DIRECTION_BACK);
        StoreAWork(wram, cpu, direction);
        return;
    }
    LoadA8(cpu, WramRead(wram, from));
    cpu->carry = 1;
    Sbc8(cpu, WramRead(wram, to));
    StoreAWork(wram, cpu, delta);
    LoadA8(cpu, 1u);
    StoreAWork(wram, cpu, direction);
}

static void SlideAdd(Lufia2Wram wram, Lufia2CpuState *cpu, uint32_t at,
    uint32_t direction) {
    LoadA8(cpu, WramReadAt(wram, at, cpu->y));
    cpu->carry = 0;
    Adc8(cpu, WramRead(wram, direction));
    WramWriteAt(wram, at, cpu->y, A8(cpu));
}

static void SlideSubtract(Lufia2Wram wram, Lufia2CpuState *cpu, uint32_t at,
    uint32_t direction) {
    LoadA8(cpu, WramReadAt(wram, at, cpu->y));
    cpu->carry = 1;
    Sbc8(cpu, WramRead(wram, direction));
    WramWriteAt(wram, at, cpu->y, A8(cpu));
}

/* Error step: the error drops by the other axis' distance; true when it ran
 * out (a borrow), which asks for a correction step. */
static int ErrorRunsOut(Lufia2Wram wram, Lufia2CpuState *cpu, uint32_t error,
    uint32_t other) {
    LoadA8(cpu, WramRead(wram, error));
    cpu->carry = 1;
    Sbc8(cpu, WramRead(wram, other));
    StoreAWork(wram, cpu, error);
    return !cpu->carry;
}

/* $82:89FA: a sprite slides between the positions of slots Y and X. With
 * A = 0 slot X slides to the position of slot Y; otherwise slot Y is put at
 * the position of slot X and slides back to where it was. M1X0. Every sixteenth
 * step hands off at the frame wait of $82:8AFA. */
Lufia2ExecutionResult Lufia2MenuCursorSlide(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    Lufia2ExecutionResult result;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x8289fau);
    Write16Direct(memory, cpu, SCRATCH_SLOT, cpu->x);
    Compare8(cpu, A8(cpu), 0u);
    if (!cpu->zero)
        Write16Direct(memory, cpu, SCRATCH_SLOT, cpu->y);
    LoadA8(cpu, WramReadAt(wram, SLOT_X, cpu->y));
    StoreAWork(wram, cpu, FROM_X);
    LoadA8(cpu, WramReadAt(wram, SLOT_Y, cpu->y));
    StoreAWork(wram, cpu, FROM_Y);
    LoadA8(cpu, WramReadAt(wram, SLOT_X, cpu->x));
    StoreAWork(wram, cpu, TO_X);
    LoadA8(cpu, WramReadAt(wram, SLOT_Y, cpu->x));
    StoreAWork(wram, cpu, TO_Y);
    LoadYDirect16(memory, cpu, SCRATCH_SLOT);
    LoadA8(cpu, WramRead(wram, TO_X));
    WramWriteAt(wram, SLOT_X, cpu->y, A8(cpu));
    LoadA8(cpu, WramRead(wram, TO_Y));
    WramWriteAt(wram, SLOT_Y, cpu->y, A8(cpu));
    LoadA8(cpu, WAIT_EVERY);
    StoreAWork(wram, cpu, WAIT_COUNT);
    Distance(wram, cpu, FROM_X, TO_X, DELTA_X, DIRECTION_X);
    Distance(wram, cpu, FROM_Y, TO_Y, DELTA_Y, DIRECTION_Y);
    LoadXDirect16(memory, cpu, DELTA_X);
    StoreXDirect16(memory, cpu, TO_X);
    LoadA8(cpu, WramRead(wram, DELTA_X));
    Compare8(cpu, A8(cpu), WramRead(wram, DELTA_Y));
    if (!cpu->carry) {
        SlideSubtract(wram, cpu, SLOT_Y, DIRECTION_Y);
        for (;;) {
            if (ErrorRunsOut(wram, cpu, DELTA_Y, DELTA_X))
                Correct(memory, cpu, 0, 0x8a80u);
            SlideAdd(wram, cpu, SLOT_Y, DIRECTION_Y);
            Compare8(cpu, A8(cpu), WramRead(wram, FROM_Y));
            if (cpu->zero) {
                LoadA8(cpu, WramReadAt(wram, SLOT_X, cpu->y));
                Compare8(cpu, A8(cpu), WramRead(wram, FROM_X));
                if (cpu->zero)
                    return ExecutionReturned(SLIDE_DONE);
            }
            if (SlideStep(memory, cpu, 0x8a9au, &result))
                return result;
        }
    }
    SlideSubtract(wram, cpu, SLOT_X, DIRECTION_X);
    for (;;) {
        if (ErrorRunsOut(wram, cpu, DELTA_X, DELTA_Y))
            Correct(memory, cpu, 1, 0x8ab1u);
        SlideAdd(wram, cpu, SLOT_X, DIRECTION_X);
        WramWrite(wram, SCRATCH_SLOT, A8(cpu));
        LoadA8(cpu, WramReadAt(wram, SLOT_Y, cpu->y));
        WramWrite(wram, SCRATCH_ROW, A8(cpu));
        LoadA8(cpu, WramReadAt(wram, SLOT_X, cpu->y));
        Compare8(cpu, A8(cpu), WramRead(wram, FROM_X));
        if (cpu->zero) {
            LoadA8(cpu, WramReadAt(wram, SLOT_Y, cpu->y));
            Compare8(cpu, A8(cpu), WramRead(wram, FROM_Y));
            if (cpu->zero)
                return ExecutionReturned(SLIDE_DONE);
        }
        if (SlideStep(memory, cpu, 0x8ad5u, &result))
            return result;
    }
}
