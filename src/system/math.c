/* Multiply ($82:8000), sine ($80:8450) and cosine ($80:8486). */

#include "core/cpu_internal.h"
#include "system/system_internal.h"

/* $82:8000: $1576:$1574 = $1570 * $1572, shift and add. $1572 is
   rotated away; P and X come back, A keeps the last partial sum. */
void Lufia2CallMultiply(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t return_bank,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, return_bank, return_address);
    PushIndex(memory, cpu);                                    /* 8000 */
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x1574u, 0), 0x0000u);
    Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x1576u, 0), 0x0000u);
    LoadX16(cpu, 0x0010u);
    do {
        RorAbsolute16(memory, cpu, 0x1572u);                   /* 800D */
        if (cpu->carry) {
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1576u, 0));
            cpu->carry = 0;
            Add16Value(cpu,
                Read16AbsoluteIndexed(memory, cpu, 0x1570u, 0));
            StoreAAbsolute16(memory, cpu, 0x1576u, 0);
        }
        RorAbsolute16(memory, cpu, 0x1576u);                   /* 801C */
        RorAbsolute16(memory, cpu, 0x1574u);
        LoadX16(cpu, (uint16_t)(cpu->x - 1u));
    } while (!cpu->zero);
    UnpackStatus(cpu, Pull8(memory, cpu));                     /* 8025 */
    cpu->x = PullIndexValue(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $80:84BF: sine of a quarter turn in 46 steps, $00-$7F. */
static uint8_t QuarterSine(const Lufia2Memory *memory, uint8_t step) {
    return Read8(memory, 0x8084bfu + step);
}

/* PHX/PHY/PHP/SEP #$30 */
static void TrigEnter(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t return_bank,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, return_bank, return_address);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 1);
}

/* PLP/PLY/PLX/RTL with the result in A. */
static void TrigLeave(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t value) {
    LoadA8(cpu, value);
    UnpackStatus(cpu, Pull8(memory, cpu));
    cpu->y = PullIndexValue(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $80:8450: sine of angle A (180 steps per turn); bit 7 = minus. */
void Lufia2CallSine(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t return_bank,
    uint16_t return_address) {
    const uint8_t angle = A8(cpu);
    uint8_t value;

    TrigEnter(memory, cpu, return_bank, return_address);
    if (angle < 0x2du)                                         /* 8455 */
        value = QuarterSine(memory, angle);
    else if (angle < 0x5au)
        value = QuarterSine(memory, (uint8_t)(0x5au - angle));
    else if (angle < 0x87u)
        value = (uint8_t)(QuarterSine(memory, (uint8_t)(angle - 0x5au)) | 0x80u);
    else
        value = (uint8_t)(QuarterSine(memory, (uint8_t)(0xb4u - angle)) | 0x80u);
    TrigLeave(memory, cpu, value);
}

/* $80:8486: cosine of angle A (180 steps per turn); bit 7 = minus. */
void Lufia2CallCosine(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t return_bank,
    uint16_t return_address) {
    const uint8_t angle = A8(cpu);
    uint8_t value;

    TrigEnter(memory, cpu, return_bank, return_address);
    if (angle < 0x2du)                                         /* 848B */
        value = QuarterSine(memory, (uint8_t)(0x2du - angle));
    else if (angle < 0x5au)
        value = (uint8_t)(QuarterSine(memory, (uint8_t)(angle - 0x2du)) | 0x80u);
    else if (angle < 0x87u)
        value = (uint8_t)(QuarterSine(memory, (uint8_t)(0x87u - angle)) | 0x80u);
    else
        value = QuarterSine(memory, (uint8_t)(angle - 0x87u));
    TrigLeave(memory, cpu, value);
}
