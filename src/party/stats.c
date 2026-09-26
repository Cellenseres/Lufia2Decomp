/* Derived character stats ($81:F4D5). */

#include "core/cpu_internal.h"
#include "lufia2/party.h"

static uint16_t Field(const Lufia2Memory *memory, const Lufia2CpuState *cpu,
    uint16_t offset) {
    return Read16AbsoluteIndexed(memory, cpu, offset, cpu->x);
}

static void StoreField(const Lufia2Memory *memory, const Lufia2CpuState *cpu,
    uint16_t offset) {
    const uint32_t at = AbsoluteIndexedAddress(cpu, offset, cpu->x);

    Write8(memory, at, (uint8_t)cpu->accumulator);
    Write8(memory, (at + 1u) & 0x00ffffffu, (uint8_t)(cpu->accumulator >> 8));
}

/* A = base + bonus (+ second bonus). */
static void Sum(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t base, uint16_t bonus, uint16_t bonus2) {
    LoadA16(cpu, Field(memory, cpu, base));
    cpu->carry = 0;
    Add16Value(cpu, Field(memory, cpu, bonus));
    if (bonus2)
        Add16Value(cpu, Field(memory, cpu, bonus2));
}

/* $81:F4ED: stats of the block at [$C1]. */
static void DerivedStats(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadX16(cpu, Read16Direct(memory, cpu, 0xc1u));
    SetAccumulatorWidth(cpu, 0);
    Sum(memory, cpu, 0x0051u, 0x0074u, 0);
    StoreField(memory, cpu, 0x0025u);
    Sum(memory, cpu, 0x0053u, 0x0076u, 0);
    StoreField(memory, cpu, 0x0027u);
    Sum(memory, cpu, 0x005du, 0x0084u, 0x0092u);
    StoreField(memory, cpu, 0x0035u);
    Sum(memory, cpu, 0x005bu, 0x0082u, 0x0090u);
    Compare16(cpu, cpu->accumulator, 0x00c8u);
    if (cpu->carry)
        LoadA16(cpu, 0x00c7u);                                 /* cap 199 */
    StoreField(memory, cpu, 0x0033u);
    Sum(memory, cpu, 0x0059u, 0x0080u, 0x008eu);
    StoreField(memory, cpu, 0x0031u);
    Sum(memory, cpu, 0x0057u, 0x007eu, 0x008cu);
    StoreField(memory, cpu, 0x002fu);
    Sum(memory, cpu, 0x0055u, 0x007cu, 0x008au);
    StoreField(memory, cpu, 0x002du);
    cpu->carry = 0;
    Add16Value(cpu, Field(memory, cpu, 0x0086u));
    StoreField(memory, cpu, 0x0029u);
    LoadA16(cpu, Field(memory, cpu, 0x0078u));
    if (!cpu->zero)
        StoreField(memory, cpu, 0x0029u);
    LoadA16(cpu, Field(memory, cpu, 0x002fu));                 /* F55D */
    cpu->carry = 0;
    Add16Value(cpu, Field(memory, cpu, 0x002du));
    LsrA16(cpu);
    cpu->carry = 0;
    Add16Value(cpu, Field(memory, cpu, 0x0088u));
    StoreField(memory, cpu, 0x002bu);
    LoadA16(cpu, Field(memory, cpu, 0x007au));
    if (!cpu->zero)
        StoreField(memory, cpu, 0x002bu);
    SetAccumulatorWidth(cpu, 1);
}

/* $81:F4D5: derived stats of block X; keeps A, X, Y, P. */
Lufia2ExecutionResult Lufia2PartyDerivedStats(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    PushAccumulator16(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    StoreXDirect16(memory, cpu, 0xc1u);
    SimulateJsrFrame(memory, cpu, 0xf4e1u);
    DerivedStats(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    cpu->y = PullIndexValue(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x81f4e8u);
}
