/* Battle party sprite drift: the random bit source and the per-record
 * wobble that updates the two offset words of each party record. */

#include "core/cpu_internal.h"
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
    BIT_RANDOM = 0x20u
};

/* ROL abs, 16-bit: the word is stored high byte first. */
static void RolAbsolute16(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t address) {
    const uint32_t low = AbsoluteIndexedAddress(cpu, address, 0);
    const uint16_t old = Read16AbsoluteIndexed(memory, cpu, address, 0);
    const uint16_t value = (uint16_t)((old << 1) | (cpu->carry ? 1u : 0u));

    cpu->carry = (old & 0x8000u) != 0;
    Write8(memory, (low + 1u) & 0x00ffffffu, (uint8_t)(value >> 8));
    Write8(memory, low, (uint8_t)value);
    SetNz16(cpu, value);
}

/* $85:8F4A: shifts the 88-bit register at $122F left by one through the
 * carry; the bit shifted in is bit 0 of $122F before the shift. M8. */
Lufia2ExecutionResult Lufia2BattleRandomBit(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    int word;

    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x858f4au);
    LoadAAbsolute8(memory, cpu, RANDOM_TOP, 0);
    LsrA8(cpu);
    SetAccumulatorWidth(cpu, 0);
    for (word = 4; word >= 0; --word)
        RolAbsolute16(memory, cpu, (uint16_t)(RANDOM_WORDS + 2 * word));
    SetAccumulatorWidth(cpu, 1);
    TransferDirectToA(cpu);
    RolA8(cpu);
    StoreAAbsolute8(memory, cpu, RANDOM_TOP, 0);
    return ExecutionReturned(0x858f66u);
}

/* One JSR to the random bit routine at the given last byte. */
static int RandomBit(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t return_address, Lufia2ExecutionResult *result) {
    Lufia2ExecutionResult child;

    SimulateJsrFrame(memory, cpu, return_address);
    child = Lufia2BattleRandomBit(memory, cpu);
    if (child.flow != LUFIA2_EXECUTION_RETURNED) {
        *result = child;
        return 0;
    }
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* Z selects between the accumulator and all ones; stores a word. */
static void StoreRandomWord(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t field) {
    SetAccumulatorWidth(cpu, 0);
    if (!cpu->zero)
        LoadA16(cpu, 0xffffu);
    Write16Long(memory, AbsoluteIndexedAddress(cpu, field, cpu->y),
        cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
}

/* $85:894A: updates the offsets of the six party records at $13DA. The
 * state bits of a record select the shake table, a sign flip or random
 * offsets. M8/X16, JSL. */
Lufia2ExecutionResult Lufia2BattleDriftRecords(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result = ExecutionReturned(0x8589e4u);

    result.dispatches = 0;
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x85894au);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, 0x85u);
    PullDataBank(memory, cpu);
    LoadAAbsolute8(memory, cpu, ACTIVE_FLAG, 0);
    if (!cpu->negative) {
        PullDataBank(memory, cpu);
        return result;
    }
    LoadY16(cpu, 0);
    TransferYToX(cpu);
    do {
        LoadAAbsolute8(memory, cpu, RECORDS + 1u, cpu->y);
        if (!cpu->negative)
            goto next;
        LoadAAbsolute8(memory, cpu, RECORDS, cpu->y);
        BitImmediate8(cpu, BIT_SHAKE);
        if (!cpu->zero) {
            StepMemory8(memory, cpu,
                AbsoluteIndexedAddress(cpu, TIMERS, cpu->x), -1);
            if (cpu->negative) {
                LoadA8(cpu, 0x3fu);
                StoreAAbsolute8(memory, cpu, TIMERS, cpu->x);
            }
            TransferDirectToA(cpu);
            LoadAAbsolute8(memory, cpu, TIMERS, cpu->x);
            LsrA8(cpu);
            LsrA8(cpu);
            And8(cpu, 0xfeu);
            SetAccumulatorWidth(cpu, 0);
            PushIndex(memory, cpu);
            TransferAToX(cpu);
            LoadA16(cpu, Read16Long(memory, LongIndexedAddress(SHAKE_TABLE,
                cpu->x)));
            cpu->x = PullIndexValue(memory, cpu);
            Write16Long(memory, AbsoluteIndexedAddress(cpu, RECORDS + 0xbu,
                cpu->y), cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            goto next;
        }
        BitImmediate8(cpu, BIT_FLIP);
        if (!cpu->zero) {
            StepMemory8(memory, cpu,
                AbsoluteIndexedAddress(cpu, TIMERS, cpu->x), -1);
            if (cpu->zero) {
                LoadA8(cpu, 0x02u);
                StoreAAbsolute8(memory, cpu, TIMERS, cpu->x);
                SetAccumulatorWidth(cpu, 0);
                LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu,
                    RECORDS + 9u, cpu->y));
                LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffffu));
                Write16Long(memory, AbsoluteIndexedAddress(cpu,
                    RECORDS + 9u, cpu->y), cpu->accumulator);
                SetAccumulatorWidth(cpu, 1);
            }
            goto next;
        }
        BitImmediate8(cpu, BIT_RANDOM);
        if (cpu->zero)
            goto next;
        StepMemory8(memory, cpu, AbsoluteIndexedAddress(cpu, TIMERS, cpu->x),
            -1);
        if (!cpu->zero)
            goto next;
        LoadA8(cpu, 0x0cu);
        StoreAAbsolute8(memory, cpu, TIMERS, cpu->x);
        if (!RandomBit(memory, cpu, 0x8979u, &result))
            return result;
        StoreRandomWord(memory, cpu, RECORDS + 9u);
        if (!RandomBit(memory, cpu, 0x8988u, &result))
            return result;
        StoreRandomWord(memory, cpu, RECORDS + 0xbu);
next:
        IncrementX16(cpu);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, cpu->y);
        cpu->carry = 0;
        Add16Value(cpu, RECORD_SIZE);
        TransferAToY(cpu);
        SetAccumulatorWidth(cpu, 1);
        Compare16(cpu, cpu->y, RECORD_END);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
    return result;
}
