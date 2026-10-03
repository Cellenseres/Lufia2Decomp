/* Random number generator ($80:8299). */

#include "core/cpu_internal.h"
#include "core/cpu_ops.h"
#include "core/wram_view.h"
#include "lufia2/system.h"
#include "system/system_internal.h"
#include "system/wram.h"

/* $80:832D: lagged XOR refill, lags 24 and 31. */
static void RandomRefill(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadX8(cpu, 0x00u);                                        /* 832D */
    do {
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, WRAM_RANDOM_TABLE, cpu->x)));
        LoadA8(
            cpu, (uint8_t)(A8(cpu) ^ Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0540u, cpu->x))));
        Write8(
            memory, AbsoluteIndexedAddress(cpu, WRAM_RANDOM_TABLE, cpu->x),
            A8(cpu));
        LoadX8(cpu, (uint8_t)(cpu->x + 1u));
        Compare8(cpu, (uint8_t)cpu->x, 0x18u);                 /* 8339 */
    } while (!cpu->zero);
    do {
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, WRAM_RANDOM_TABLE, cpu->x)));
        LoadA8(
            cpu, (uint8_t)(A8(cpu) ^ Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0509u, cpu->x))));
        Write8(
            memory, AbsoluteIndexedAddress(cpu, WRAM_RANDOM_TABLE, cpu->x),
            A8(cpu));
        LoadX8(cpu, (uint8_t)(cpu->x + 1u));
        Compare8(cpu, (uint8_t)cpu->x, 0x37u);                 /* 8347 */
    } while (!cpu->zero);
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

void Lufia2CallRandomScale(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x83u, return_address);
    Lufia2RandomScale(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

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

/* What the table fill leaves in the registers: the last table index and the
 * byte the accumulator's high half ends up holding. */
typedef struct SeedFillResult {
    uint8_t last_index;
    uint8_t held;
} SeedFillResult;

/* Fills the table with a subtractive sequence started from the seed byte. Each
 * round steps the index by 21 (mod 55), stores the previous scratch value
 * there and in the seed byte, and keeps `seed - scratch` as the next scratch. */
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
        difference = (uint8_t)(WramRead(wram, WRAM_RANDOM_SEED_WORK) -
                               WramRead(wram, RANDOM_SEED_SCRATCH));
        previous = WramRead(wram, RANDOM_SEED_SCRATCH);
        WramWrite(wram, WRAM_RANDOM_SEED_WORK, previous);
        WramWriteAt(wram, WRAM_RANDOM_TABLE, index, previous);
        WramWrite(wram, RANDOM_SEED_SCRATCH, difference);
        result.held = previous;
    }
    result.last_index = index;
    return result;
}

/* $80:82E7: seed the generator from the seed byte, then mix the table three
 * times. The status, the scratch byte and the stack are restored. */
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
