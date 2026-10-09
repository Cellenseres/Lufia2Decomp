/* Multiply ($82:8000), sine ($80:8450) and cosine ($80:8486). */

#include "core/cpu_ops.h"
#include "system/system_internal.h"
#include "system/wram.h"

/* $82:8000: 16x16 multiply by shift and add. */
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
    Write16Long(memory, AbsoluteIndexedAddress(cpu, WRAM_SYSTEM_MULTIPLY_PRODUCT, 0),
                0x0000u);
    Write16Long(memory,
                AbsoluteIndexedAddress(cpu, (WRAM_SYSTEM_MULTIPLY_PRODUCT + 2u), 0),
                0x0000u);
    LoadX16(cpu, 0x0010u);
    do {
        RorAbsolute16(memory, cpu, 0x1572u);                   /* 800D */
        if (cpu->carry) {
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu,
                                               (WRAM_SYSTEM_MULTIPLY_PRODUCT + 2u), 0));
            cpu->carry = 0;
            Add16Value(cpu,
                       Read16AbsoluteIndexed(memory, cpu, WRAM_SYSTEM_MULTIPLY_A, 0));
            StoreAAbsolute16(memory, cpu, (WRAM_SYSTEM_MULTIPLY_PRODUCT + 2u), 0);
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
    return Read8(memory, ROM_QUARTER_SINE_TABLE + step);
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

/* $80:8378: $4E = $4E / $51, A = remainder; 16 shift-subtract steps. */
Lufia2ExecutionResult Lufia2Divide16(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    unsigned i;

    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    LoadA16(cpu, 0x0000u);
    for (i = 0; i < 16u; ++i) {
        const uint16_t quotient = Read16Direct(memory, cpu, 0x4eu);

        cpu->carry = (quotient & 0x8000u) != 0;                /* ASL $4E */
        const uint16_t shifted = (uint16_t)(quotient << 1);
        const uint32_t address = OpDp(cpu, 0x4eu);
        /* Original 16-bit ASL writes the high byte first. */
        Write8(memory, OpNextByte(address), (uint8_t)(shifted >> 8));
        Write8(memory, address, (uint8_t)shifted);
        SetNz16(cpu, shifted);
        RolA16(cpu);
        if (!cpu->carry) {
            Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, 0x51u));
            if (!cpu->carry)
                continue;
        }
        OpSbcValue(cpu, Read16Direct(memory, cpu, 0x51u));
        OpStepMem(memory, cpu, OpDp(cpu, 0x4eu), 1);
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x80844fu);
}

/* $81:E808: X to decimal digits $B4 (hundreds), $B3, $B2. */
Lufia2ExecutionResult Lufia2DecimalDigits3(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const uint16_t kSteps[2] = {0x0064u, 0x000au};
    unsigned i;

    Write8(memory, (uint16_t)(cpu->direct_page + 0xb4u), 0x00u);
    Write8(memory, (uint16_t)(cpu->direct_page + 0xb3u), 0x00u);
    Write8(memory, (uint16_t)(cpu->direct_page + 0xb2u), 0x00u);
    for (i = 0; i < 2u; ++i) {
        for (;;) {
            SetAccumulatorWidth(cpu, 0);
            TransferXToA(cpu);
            Subtract16(cpu, kSteps[i]);
            if (!cpu->carry)
                break;
            TransferAToX(cpu);
            SetAccumulatorWidth(cpu, 1);
            if (i == 0) {
                const uint32_t at = AbsoluteIndexedAddress(cpu, 0x00b4u, 0);
                const uint8_t value = (uint8_t)(Read8(memory, at) + 1u);

                Write8(memory, at, value);
                SetNz8(cpu, value);
            } else {
                IncrementDirect8(memory, cpu, 0xb3u);
            }
        }
    }
    TransferXToA(cpu);
    SetAccumulatorWidth(cpu, 1);
    StoreADirect8(memory, cpu, 0xb2u);
    return ExecutionReturned(0x81e834u);
}
