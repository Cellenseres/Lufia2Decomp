/* Random number generator ($80:8299). */

#include "core/cpu_internal.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/system.h"
#include "system/system_internal.h"

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

Lufia2ExecutionResult Lufia2SeedRandom(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->decimal)
        return ExecutionHandoff(cpu, 0x8082e7u);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x30u);
    OpLda(memory, cpu, OpDp(cpu, 0u));
    PushAccumulator8(memory, cpu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_RANDOM_SEED_WORK));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_RANDOM_TABLE + 54u));
    OpLdx(cpu, 1u);
    OpWriteX(memory, cpu, OpDp(cpu, 0u), cpu->x);
    OpDex(cpu);
    OpLdy(cpu, 55u);
    do {
        OpTxa(cpu);
        cpu->carry = 0;
        OpAdcValue(cpu, 21u);
        OpCmpValue(cpu, 55u);
        if (cpu->carry)
            OpSbcValue(cpu, 55u);
        OpTax(cpu);
        OpLda(memory, cpu, OpAbs(cpu, WRAM_RANDOM_SEED_WORK));
        cpu->carry = 1;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0u)));
        ExchangeAccumulatorBytes(cpu);
        OpLda(memory, cpu, OpDp(cpu, 0u));
        OpSta(memory, cpu, OpAbs(cpu, WRAM_RANDOM_SEED_WORK));
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_RANDOM_TABLE));
        ExchangeAccumulatorBytes(cpu);
        OpSta(memory, cpu, OpDp(cpu, 0u));
        OpDey(cpu);
    } while (!cpu->zero);
    OpLoadA(cpu, 54u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_RANDOM_NEXT_INDEX));
    SimulateJsrFrame(memory, cpu, 0x8321u);
    RandomRefill(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    SimulateJsrFrame(memory, cpu, 0x8324u);
    RandomRefill(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    SimulateJsrFrame(memory, cpu, 0x8327u);
    RandomRefill(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    LoadA8(cpu, Pull8(memory, cpu));
    OpSta(memory, cpu, OpDp(cpu, 0u));
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x80832cu);
}
