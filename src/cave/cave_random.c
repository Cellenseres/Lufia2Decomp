/* Ancient Cave random draws through the PPU multiplier ($83:9DE4-$83:9E30). */

#include "cave/cave_internal.h"
#include "core/snes_registers.h"
#include "lufia2/system.h"
#include "system/dp_scratch.h"
#include "system/system_internal.h"

/* Random product through the Mode 7 multiplier: M7A takes the random byte and
 * then D's low byte (TDC) as its high half; M7B holds the other factor. */
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

/* $83:9E1B: A = (random * A) >> 8 for DP 0; B = DP high. */
void Lufia2CaveRandomBelow(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    Write8(memory, SNES_M7B, A8(cpu));                         /* 9E1B */
    CaveMultiplyRandom(memory, cpu, 0x9e22u);                  /* 9E1F */
    SimulateRtsFrame(memory, cpu);                             /* 9E30 */
}

/* $83:9E11: X = 2 * RandomBelow(A). */
void Lufia2CaveRandomIndex(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    Lufia2CaveRandomBelow(memory, cpu, 0x9e11u);               /* 9E11 */
    ExchangeAccumulatorBytes(cpu);                             /* 9E14 */
    LoadA8(cpu, 0x00u);
    ExchangeAccumulatorBytes(cpu);
    AslA8(cpu);
    OpTax(cpu);
    SimulateRtsFrame(memory, cpu);                             /* 9E1A */
}

/* $83:9DE4: mean of two draws below A (the second reuses $211C). */
void Lufia2CaveRandomMean(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    Write8(memory, SNES_M7B, A8(cpu));                         /* 9DE4 */
    CaveMultiplyRandom(memory, cpu, 0x9debu);                  /* 9DE8 */
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_A), A8(cpu)); /* 9DF9 */
    CaveMultiplyRandom(memory, cpu, 0x9dfeu);                  /* 9DFB */
    cpu->carry = 0;                                            /* 9E0C */
    OpAdc(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    LsrA8(cpu);
    SimulateRtsFrame(memory, cpu);                             /* 9E10 */
}
