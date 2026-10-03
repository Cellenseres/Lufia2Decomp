/* Scene tracks: five script-driven value tracks that are stepped once per
 * frame and copied to the scene registers. */

#include <stdbool.h>
#include <stddef.h>

#include "core/cpu_internal.h"
#include "core/plain_ops.h"
#include "core/wram_view.h"
#include "lufia2/field.h"

enum {
    SCRIPT_POINTERS = 0x1206u,
    TRACK_VALUES = 0x1210u,
    TRACK_TIMERS = 0x121au,
    TRACK_COUNT = 5u,
    POINTER_DIRECT = 0x02u,
    COUNTER_DIRECT = 0x04u,
    STEP_BYTES = 4u,
    VIEW_ORIGIN_DIRECT = 0x06u,
    VIEW_WIDTH_FLAG = 0x11ddu,
    VIEW_WIDTH_SIGN = 0x11deu,
    VIEW_X = 0x11e8u,
    VIEW_Y = 0x11eau,
    SCROLL_X = 0x11f8u,
    SCROLL_Y = 0x11fau,
    SCREEN_X = 0x0594u,
    SCREEN_Y = 0x0596u,
    VIEW_WINDOW = 0x0fffu,
    VIEW_HALF_WIDTH = 0x0080u,
    VIEW_HALF_HEIGHT = 0x0070u
};

/* An address in the data bank, plus an index that carries into the next
 * bank. The script words are addressed this way, not by a catalogued
 * location. */
static uint32_t DataAddress(Lufia2Wram wram, uint16_t address, uint16_t index) {
    return (((uint32_t)wram.data_bank << 16) + address + index) & 0x00ffffffu;
}

static uint16_t ReadDataWord(Lufia2Wram wram, uint16_t address, uint16_t index) {
    const uint32_t low = DataAddress(wram, address, index);
    const uint8_t low_byte = Read8(wram.memory, low);

    return (uint16_t)(low_byte |
        ((uint16_t)Read8(wram.memory, (low + 1u) & 0x00ffffffu) << 8));
}

static void WriteDataWord(
    Lufia2Wram wram, uint16_t address, uint16_t index, uint16_t value) {
    const uint32_t low = DataAddress(wram, address, index);

    Write8(wram.memory, low, (uint8_t)value);
    Write8(wram.memory, (low + 1u) & 0x00ffffffu, (uint8_t)(value >> 8));
}

/* The track values that go to the scene registers: the address of the track
 * value and where it is copied. */
typedef struct {
    uint16_t source;
    uint16_t target;
} SceneCopy;

static const SceneCopy kSceneCopies[] = {
    { 0x1214u, 0x11fcu },
    { 0x1216u, 0x11feu },
    { 0x1210u, 0x1247u },
    { 0x1212u, 0x1249u },
    { 0x1218u, 0x1200u }
};

/* $86:94D4: advances every track by its script step, or fetches the next
 * step when the timer runs out. Carry is set when a script ends. The
 * script words are read from the data bank. Any M, X16; leaves M8. */
Lufia2ExecutionResult Lufia2SceneTrackStep(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint16_t track = 0;
    uint16_t step = 0;
    uint16_t last = 0;
    bool overflow = false;
    size_t i;

    if (cpu->index_is_8_bit || !DirectWorkWordAvailable(cpu, COUNTER_DIRECT))
        return ExecutionHandoff(cpu, 0x8694d4u);
    SetAccumulatorWidth(cpu, 0);
    WramWrite16(wram, POINTER_DIRECT, SCRIPT_POINTERS);
    WramWrite16(wram, COUNTER_DIRECT, TRACK_COUNT);
    do {
        const uint16_t script = WramRead16(wram, POINTER_DIRECT);

        step = ReadDataWord(wram, script, 0);
        if ((WramStep16At(wram, TRACK_TIMERS, track, -1) & 0x8000u) != 0) {
            const Word16Result next = Sum16Mode(step, STEP_BYTES, false, cpu->decimal);

            WriteDataWord(wram, script, 0, next.value);
            step = next.value;
            last = ReadDataWord(wram, 0, step);
            overflow = next.overflow;
            if (last == 0) {
                cpu->x = track;
                cpu->y = step;
                cpu->accumulator = 0;
                cpu->carry = true;
                cpu->overflow = overflow;
                SetNz16(cpu, 0);
                SetAccumulatorWidth(cpu, 1);
                return ExecutionReturned(0x869536u);
            }
            WramWrite16At(wram, TRACK_TIMERS, track, last);
        } else {
            const Word16Result moved = Sum16Mode(
                WramRead16At(wram, TRACK_VALUES, track),
                ReadDataWord(wram, 2u, step), false, cpu->decimal);

            last = moved.value;
            overflow = moved.overflow;
            WramWrite16At(wram, TRACK_VALUES, track, last);
        }
        track = (uint16_t)(track + 2u);
        (void)WramStep16(wram, POINTER_DIRECT, 1);
        (void)WramStep16(wram, POINTER_DIRECT, 1);
    } while (WramStep16(wram, COUNTER_DIRECT, -1) != 0);
    SetAccumulatorWidth(cpu, 1);
    for (i = 0; i < sizeof(kSceneCopies) / sizeof(kSceneCopies[0]); ++i) {
        cpu->y = ReadDataWord(wram, kSceneCopies[i].source, 0);
        WramWrite16(wram, kSceneCopies[i].target, cpu->y);
    }
    cpu->x = track;
    cpu->accumulator = last;
    SetNz16(cpu, cpu->y);
    cpu->carry = false;
    cpu->overflow = overflow;
    return ExecutionReturned(0x869532u);
}

/* $86:A791: the screen origin and the two scroll values derived from the
 * view position at $11E8/$11EA. M8/X16, JSR. */
Lufia2ExecutionResult Lufia2SceneViewOrigin(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint16_t height = VIEW_HALF_HEIGHT;
    uint16_t scroll;
    Word16Result screen;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86a791u);
    if (WramRead(wram, VIEW_WIDTH_FLAG) != 0 &&
        (WramRead(wram, VIEW_WIDTH_SIGN) & 0x80u) != 0)
        height = 0;
    WramWrite16(wram, VIEW_ORIGIN_DIRECT, height);
    scroll = (uint16_t)(WramRead16(wram, VIEW_X) & VIEW_WINDOW);
    WramWrite16(wram, SCROLL_X, scroll);
    WramWrite16(wram, SCREEN_X,
        Difference16Mode(scroll, VIEW_HALF_WIDTH, cpu->decimal).value);
    scroll = (uint16_t)(
        Sum16Mode(WramRead16(wram, VIEW_Y), WramRead16(wram, VIEW_ORIGIN_DIRECT),
            false, cpu->decimal).value & VIEW_WINDOW);
    WramWrite16(wram, SCROLL_Y, scroll);
    screen = Difference16Mode(
        Difference16Mode(scroll, WramRead16(wram, VIEW_ORIGIN_DIRECT), cpu->decimal).value,
        VIEW_HALF_HEIGHT, cpu->decimal);
    WramWrite16(wram, SCREEN_Y, screen.value);
    cpu->y = height;
    cpu->accumulator = screen.value;
    cpu->carry = screen.carry;
    cpu->overflow = screen.overflow;
    SetNz16(cpu, screen.value);
    return ExecutionReturned(0x86a7cdu);
}
