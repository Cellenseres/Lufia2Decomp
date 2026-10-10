/* Random number generator ($80:8299). */

#include "core/cpu_internal.h"
#include "core/cpu_ops.h"
#include "core/wram_view.h"
#include "lufia2/system.h"
#include "system/system_internal.h"
#include "system/wram.h"

enum {
    RANDOM_LONG_LAG = 31,  /* first pass mixes in the entry 31 ahead */
    RANDOM_SHORT_LAG = 24, /* second pass mixes in the entry 24 behind */
};

/* $80:832D: lagged XOR refill; X size, A last entry. */
static void RandomRefill(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint8_t value = 0u;
    unsigned i;

    for (i = 0; i < RANDOM_SHORT_LAG; ++i) {
        const uint8_t current = WramReadAt(wram, WRAM_RANDOM_TABLE, (uint16_t)i);
        const uint8_t lagged =
            WramReadAt(wram, WRAM_RANDOM_TABLE, (uint16_t)(i + RANDOM_LONG_LAG));
        value = (uint8_t)(current ^ lagged);
        WramWriteAt(wram, WRAM_RANDOM_TABLE, (uint16_t)i, value);
    }
    for (; i < WRAM_RANDOM_TABLE_COUNT; ++i) {
        const uint8_t current = WramReadAt(wram, WRAM_RANDOM_TABLE, (uint16_t)i);
        const uint8_t lagged =
            WramReadAt(wram, WRAM_RANDOM_TABLE, (uint16_t)(i - RANDOM_SHORT_LAG));
        value = (uint8_t)(current ^ lagged);
        WramWriteAt(wram, WRAM_RANDOM_TABLE, (uint16_t)i, value);
    }
    cpu->x = (uint16_t)i;
    LoadA8(cpu, value);
}

/* PHB/PHK/PLB/PHX/PHY/PHP/SEP #$30 */
static void RandomEnter(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    Push8(memory, cpu, 0x80u);
    PullDataBank(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 1);
}

/* Next table index in X, refilling past $36. */
static void RandomAdvance(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t refill_return) {
    LoadX8(
        cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_RANDOM_NEXT_INDEX, 0)));
    LoadX8(cpu, (uint8_t)(cpu->x + 1u));
    Compare8(cpu, (uint8_t)cpu->x, 0x37u);
    if (cpu->carry) {
        SimulateJsrFrame(memory, cpu, refill_return);
        RandomRefill(memory, cpu);
        SimulateRtsFrame(memory, cpu);
        LoadX8(cpu, 0x00u);
    }
    Write8(
        memory, AbsoluteIndexedAddress(cpu, WRAM_RANDOM_NEXT_INDEX, 0),
        (uint8_t)cpu->x);
}

/* PLP/PLY/PLX/PLB */
static void RandomLeave(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    UnpackStatus(cpu, Pull8(memory, cpu));
    cpu->y = PullIndexValue(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    PullDataBank(memory, cpu);
}

/* $80:82C7: A = next byte from the table. */
void Lufia2RandomByte(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    RandomEnter(memory, cpu);                                  /* 82C7 */
    RandomAdvance(memory, cpu, 0x82d9u);                       /* 82CF */
    LoadA8(
        cpu, Read8(
            memory, AbsoluteIndexedAddress(cpu, WRAM_RANDOM_TABLE, cpu->x)));
    RandomLeave(memory, cpu);                                  /* 82E2 */
}

/* $80:8299: next byte scaled to 0..A-1. */
void Lufia2RandomScale(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    RandomEnter(memory, cpu);                                  /* 8299 */
    ExchangeAccumulatorBytes(cpu);                             /* 82A1 */
    RandomAdvance(memory, cpu, 0x82acu);                       /* 82A2 */
    ExchangeAccumulatorBytes(cpu);                             /* 82B2 */
    Write8(
        memory, AbsoluteIndexedAddress(cpu, SNES_WRMPYA, 0), A8(cpu));
    LoadA8(
        cpu, Read8(
            memory, AbsoluteIndexedAddress(cpu, WRAM_RANDOM_TABLE, cpu->x)));
    Write8(
        memory, AbsoluteIndexedAddress(cpu, SNES_WRMPYB, 0), A8(cpu));
    LoadA8(cpu, 0x00u);                                        /* 82BC */
    ExchangeAccumulatorBytes(cpu);                             /* 82BE */
    LoadA8(
        cpu, Read8(memory, AbsoluteIndexedAddress(cpu, SNES_RDMPYH, 0)));
    RandomLeave(memory, cpu);                                  /* 82C2 */
}

/* JSL $80:8299 with the caller's return address on the stack. */
void Lufia2CallRandomScale(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x83u, return_address);
    Lufia2RandomScale(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* JSL $80:82C7 with the caller's return address on the stack. */
void Lufia2CallRandomByte(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x83u, return_address);
    Lufia2RandomByte(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

enum {
    RANDOM_SEED_SCRATCH = 0x00, /* direct-page byte the seeding borrows */
    RANDOM_SEED_STRIDE = 21,    /* table step per round */
    RANDOM_TABLE_SIZE = 55,
    RANDOM_SEED_ROUNDS = 55,
    RANDOM_SEED_REFILLS = 3,
};

/* Table fill exit: last index, A's high byte. */
typedef struct SeedFillResult {
    uint8_t last_index;
    uint8_t held;
} SeedFillResult;

/* Subtractive fill from the seed byte, step 21 mod 55. */
static SeedFillResult SeedFillTable(Lufia2Wram wram) {
    SeedFillResult result = {0u, 0u};
    uint8_t index = 0u;
    unsigned round;

    WramWriteAt(wram, WRAM_RANDOM_TABLE, RANDOM_TABLE_SIZE - 1u,
                WramRead(wram, WRAM_RANDOM_SEED_WORK));
    WramWrite(wram, RANDOM_SEED_SCRATCH, 1u);
    for (round = 0; round < RANDOM_SEED_ROUNDS; ++round) {
        uint8_t difference;
        uint8_t previous;

        index = (uint8_t)(index + RANDOM_SEED_STRIDE);
        if (index >= RANDOM_TABLE_SIZE)
            index = (uint8_t)(index - RANDOM_TABLE_SIZE);
        const uint8_t seed = WramRead(wram, WRAM_RANDOM_SEED_WORK);
        const uint8_t scratch = WramRead(wram, RANDOM_SEED_SCRATCH);
        difference = (uint8_t)(seed - scratch);
        previous = WramRead(wram, RANDOM_SEED_SCRATCH);
        WramWrite(wram, WRAM_RANDOM_SEED_WORK, previous);
        WramWriteAt(wram, WRAM_RANDOM_TABLE, index, previous);
        WramWrite(wram, RANDOM_SEED_SCRATCH, difference);
        result.held = previous;
    }
    result.last_index = index;
    return result;
}

/* $80:82E7: seed the generator, mix the table three times. */
Lufia2ExecutionResult Lufia2SeedRandom(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint16_t refill_returns[RANDOM_SEED_REFILLS] = {0x8321u, 0x8324u,
                                                                 0x8327u};
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    SeedFillResult fill;
    unsigned i;

    if (cpu->decimal)
        return ExecutionHandoff(cpu, 0x8082e7u);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x30u);
    LoadA8(cpu, WramRead(wram, RANDOM_SEED_SCRATCH));
    PushAccumulator8(memory, cpu);
    fill = SeedFillTable(wram);
    cpu->x = fill.last_index;
    cpu->y = 0u;
    cpu->accumulator = (uint16_t)((uint16_t)fill.held << 8);
    LoadA8(cpu, RANDOM_TABLE_SIZE - 1u);
    WramWrite(wram, WRAM_RANDOM_NEXT_INDEX, A8(cpu));
    for (i = 0; i < RANDOM_SEED_REFILLS; ++i) {
        SimulateJsrFrame(memory, cpu, refill_returns[i]);
        RandomRefill(memory, cpu);
        SimulateRtsFrame(memory, cpu);
    }
    LoadA8(cpu, Pull8(memory, cpu));
    WramWrite(wram, RANDOM_SEED_SCRATCH, A8(cpu));
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x80832cu);
}

Lufia2ExecutionResult Lufia2RefillRandomTable(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        !cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x80832du);
    RandomRefill(memory, cpu);
    Compare8(cpu, (uint8_t)cpu->x, WRAM_RANDOM_TABLE_COUNT);
    return ExecutionReturned(0x80834bu);
}
