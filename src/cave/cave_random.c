/* Ancient Cave random draws through the PPU multiplier ($83:9DE4-$83:9E30). */

#include "cave/cave_internal.h"
#include "core/snes_registers.h"
#include "lufia2/system.h"
#include "system/dp_scratch.h"
#include "system/system_internal.h"

/* Random product via the Mode 7 multiplier. */
static void CaveMultiplyRandom(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t random_return) {
    Lufia2CallRandomByte(memory, cpu, random_return);
    Write8(memory, SNES_M7A, A8(cpu));
    TransferDirectToA(cpu);
    Write8(memory, SNES_M7A, A8(cpu));
    LoadA8(cpu, Read8(memory, SNES_MPYM));
}

/* $83:9E1B: Scale a random byte through Mode7. */
static uint32_t Lufia2CaveRandomBelowBody(const Lufia2Memory *memory,
                                          Lufia2CpuState *cpu) {
    Write8(memory, SNES_M7B, A8(cpu));        /* 9E1B */
    CaveMultiplyRandom(memory, cpu, 0x9e22u); /* 9E1F */
    return 0x839e30u;                         /* 9E30 */
}

void Lufia2CaveRandomBelow(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    (void)Lufia2CaveRandomBelowBody(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $83:9E11: Resolve a doubled random index. */
static uint32_t Lufia2CaveRandomIndexBody(const Lufia2Memory *memory,
                                          Lufia2CpuState *cpu) {
    Lufia2CaveRandomBelow(memory, cpu, 0x9e11u);               /* 9E11 */
    ExchangeAccumulatorBytes(cpu);                             /* 9E14 */
    LoadA8(cpu, 0x00u);
    ExchangeAccumulatorBytes(cpu);
    AslA8(cpu);
    OpTax(cpu);
    return 0x839e1au; /* 9E1A */
}

void Lufia2CaveRandomIndex(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                           uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    (void)Lufia2CaveRandomIndexBody(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $83:9DE4: Average two wrapped random draws. */
static uint32_t Lufia2CaveRandomMeanBody(const Lufia2Memory *memory,
                                         Lufia2CpuState *cpu) {
    Write8(memory, SNES_M7B, A8(cpu));                         /* 9DE4 */
    CaveMultiplyRandom(memory, cpu, 0x9debu);                  /* 9DE8 */
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_A), A8(cpu)); /* 9DF9 */
    CaveMultiplyRandom(memory, cpu, 0x9dfeu);                  /* 9DFB */
    cpu->carry = 0;                                            /* 9E0C */
    OpAdc(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    LsrA8(cpu);
    return 0x839e10u; /* 9E10 */
}

void Lufia2CaveRandomMean(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                          uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    (void)Lufia2CaveRandomMeanBody(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

static uint8_t CaveRandomEntryContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x83u && !cpu->direct_page && !cpu->decimal &&
           cpu->accumulator_is_8_bit && cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

Lufia2ExecutionResult Lufia2CaveDrawRandomBelow(const Lufia2Memory *memory,
                                                Lufia2CpuState *cpu) {
    if (!CaveRandomEntryContext(cpu))
        return ExecutionHandoff(cpu, 0x839e1bu);
    return ExecutionReturned(Lufia2CaveRandomBelowBody(memory, cpu));
}

Lufia2ExecutionResult Lufia2CaveDrawRandomIndex(const Lufia2Memory *memory,
                                                Lufia2CpuState *cpu) {
    if (!CaveRandomEntryContext(cpu))
        return ExecutionHandoff(cpu, 0x839e11u);
    return ExecutionReturned(Lufia2CaveRandomIndexBody(memory, cpu));
}

Lufia2ExecutionResult Lufia2CaveDrawRandomMean(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu) {
    if (!CaveRandomEntryContext(cpu))
        return ExecutionHandoff(cpu, 0x839de4u);
    return ExecutionReturned(Lufia2CaveRandomMeanBody(memory, cpu));
}
