/* Scene tracks: five script-driven value tracks that are stepped once per
 * frame and copied to the scene registers. */

#include "core/cpu_internal.h"
#include "lufia2/field.h"

enum {
    SCRIPT_POINTERS = 0x1206u,
    TRACK_VALUES = 0x1210u,
    TRACK_TIMERS = 0x121au,
    TRACK_COUNT = 5u,
    POINTER_DIRECT = 0x02u,
    COUNTER_DIRECT = 0x04u
};

/* INC/DEC dp, 16-bit: the word is stored high byte first. */
static void StepDirect16(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t offset, int delta) {
    const uint16_t address = (uint16_t)(cpu->direct_page + offset);
    const uint16_t value =
        (uint16_t)(Read16Direct(memory, cpu, offset) + delta);

    Write8(memory, (uint16_t)(address + 1u), (uint8_t)(value >> 8));
    Write8(memory, address, (uint8_t)value);
    SetNz16(cpu, value);
}

/* DEC abs,X, 16-bit, high byte first. */
static void DecrementTimer(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const uint32_t low = AbsoluteIndexedAddress(cpu, TRACK_TIMERS, cpu->x);
    const uint16_t value = (uint16_t)(
        Read16AbsoluteIndexed(memory, cpu, TRACK_TIMERS, cpu->x) - 1u);

    Write8(memory, (low + 1u) & 0x00ffffffu, (uint8_t)(value >> 8));
    Write8(memory, low, (uint8_t)value);
    SetNz16(cpu, value);
}

/* $86:94D4: advances every track by its script step, or fetches the next
 * step when the timer runs out. Carry is set when a script ends. The
 * script words are read from the data bank. Any M, X16; leaves M8. */
Lufia2ExecutionResult Lufia2SceneTrackStep(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x8694d4u);
    SetAccumulatorWidth(cpu, 0);
    LoadX16(cpu, SCRIPT_POINTERS);
    StoreXDirect16(memory, cpu, POINTER_DIRECT);
    LoadX16(cpu, 0);
    LoadA16(cpu, TRACK_COUNT);
    StoreADirect16(memory, cpu, COUNTER_DIRECT);
    do {
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu,
            Read16Direct(memory, cpu, POINTER_DIRECT), 0));
        TransferAToY(cpu);
        DecrementTimer(memory, cpu);
        if (cpu->negative) {
            LoadA16(cpu, cpu->y);
            cpu->carry = 0;
            Add16Value(cpu, 4u);
            Write16Long(memory, AbsoluteIndexedAddress(cpu,
                Read16Direct(memory, cpu, POINTER_DIRECT), 0),
                cpu->accumulator);
            TransferAToY(cpu);
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0, cpu->y));
            if (cpu->zero) {
                SetAccumulatorWidth(cpu, 1);
                cpu->carry = 1;
                return ExecutionReturned(0x869536u);
            }
            StoreAAbsolute16(memory, cpu, TRACK_TIMERS, cpu->x);
        } else {
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, TRACK_VALUES,
                cpu->x));
            cpu->carry = 0;
            Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 2, cpu->y));
            StoreAAbsolute16(memory, cpu, TRACK_VALUES, cpu->x);
        }
        IncrementX16(cpu);
        IncrementX16(cpu);
        StepDirect16(memory, cpu, POINTER_DIRECT, 1);
        StepDirect16(memory, cpu, POINTER_DIRECT, 1);
        StepDirect16(memory, cpu, COUNTER_DIRECT, -1);
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1214u, 0));
    Write16Absolute(memory, cpu, 0x11fcu, cpu->y);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1216u, 0));
    Write16Absolute(memory, cpu, 0x11feu, cpu->y);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1210u, 0));
    Write16Absolute(memory, cpu, 0x1247u, cpu->y);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1212u, 0));
    Write16Absolute(memory, cpu, 0x1249u, cpu->y);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1218u, 0));
    Write16Absolute(memory, cpu, 0x1200u, cpu->y);
    cpu->carry = 0;
    return ExecutionReturned(0x869532u);
}

/* $86:A791: the screen origin and the two scroll values derived from the
 * view position at $11E8/$11EA. M8/X16, JSR. */
Lufia2ExecutionResult Lufia2SceneViewOrigin(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86a791u);
    LoadY16(cpu, 0x0070u);
    LoadAAbsolute8(memory, cpu, 0x11ddu, 0);
    if (!cpu->zero) {
        LoadAAbsolute8(memory, cpu, 0x11deu, 0);
        if (cpu->negative)
            LoadY16(cpu, 0);
    }
    StoreYDirect16(memory, cpu, 0x06u);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x11e8u, 0));
    And16(cpu, 0x0fffu);
    StoreAAbsolute16(memory, cpu, 0x11f8u, 0);
    Subtract16(cpu, 0x0080u);
    StoreAAbsolute16(memory, cpu, 0x0594u, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x11eau, 0));
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x06u));
    And16(cpu, 0x0fffu);
    StoreAAbsolute16(memory, cpu, 0x11fau, 0);
    Subtract16(cpu, Read16Direct(memory, cpu, 0x06u));
    Subtract16(cpu, 0x0070u);
    StoreAAbsolute16(memory, cpu, 0x0596u, 0);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x86a7cdu);
}
