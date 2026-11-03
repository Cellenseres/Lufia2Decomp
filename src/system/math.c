/* Multiply ($82:8000), sine ($80:8450) and cosine ($80:8486). */

#include "core/cpu_ops.h"
#include "lufia2/menu.h"
#include "system/system_internal.h"
#include "system/wram.h"

/* $82:8000: 16x16 multiply by shift and add. */
void Lufia2CallMultiply(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t return_bank,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, return_bank, return_address);
    (void)Lufia2MenuMultiply(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

enum {
    TRIG_QUARTER_TURN = 0x2d,
    TRIG_HALF_TURN = 0x5a,
    TRIG_THREE_QUARTER_TURN = 0x87,
    TRIG_FULL_TURN = 0xb4
};

static uint8_t TrigTableOffset(
    uint8_t angle, uint8_t origin, bool reverse, bool decimal) {
    if (!decimal)
        return reverse ? (uint8_t)(origin - angle) : (uint8_t)(angle - origin);
    const uint8_t difference = (uint8_t)ArithmeticValue(
        angle, origin, true, true, true, 8u).value;
    /* Negation stays binary after decimal subtraction. */
    return reverse ? (uint8_t)(0u - difference) : difference;
}

/* $80:84BF: sine of a quarter turn in 46 steps, $00-$7F. */
static uint8_t QuarterSine(const Lufia2Memory *memory, uint8_t step) {
    return Read8(memory, ROM_QUARTER_SINE_TABLE + step);
}

/* PHX/PHY/PHP/SEP #$30 */
static void SaveTrigRegisters(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 1);
}

/* PLP/PLY/PLX/RTL with the result in A. */
static void RestoreTrigRegisters(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t value) {
    LoadA8(cpu, value);
    UnpackStatus(cpu, Pull8(memory, cpu));
    cpu->y = PullIndexValue(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
}

/* $80:8450: sine of angle A (180 steps per turn); bit 7 = minus. */
static void CalculateSine(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t angle) {
    uint8_t value;

    SaveTrigRegisters(memory, cpu);
    if (angle < TRIG_QUARTER_TURN)
        value = QuarterSine(memory, angle);
    else if (angle < TRIG_HALF_TURN)
        value = QuarterSine(memory, TrigTableOffset(
            angle, TRIG_HALF_TURN, true, cpu->decimal));
    else if (angle < TRIG_THREE_QUARTER_TURN)
        value = (uint8_t)(QuarterSine(memory, TrigTableOffset(
            angle, TRIG_HALF_TURN, false, cpu->decimal)) | 0x80u);
    else
        value = (uint8_t)(QuarterSine(memory, TrigTableOffset(
            angle, TRIG_FULL_TURN, true, cpu->decimal)) | 0x80u);
    RestoreTrigRegisters(memory, cpu, value);
}

void Lufia2CallSine(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                    uint8_t return_bank, uint16_t return_address) {
    const uint8_t angle = A8(cpu);
    SimulateJslFrame(memory, cpu, return_bank, return_address);
    CalculateSine(memory, cpu, angle);
    SimulateRtlFrame(memory, cpu);
}


/* $80:8486: cosine of angle A (180 steps per turn); bit 7 = minus. */
static void CalculateCosine(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t angle) {
    uint8_t value;

    SaveTrigRegisters(memory, cpu);
    if (angle < TRIG_QUARTER_TURN)
        value = QuarterSine(memory, TrigTableOffset(
            angle, TRIG_QUARTER_TURN, true, cpu->decimal));
    else if (angle < TRIG_HALF_TURN)
        value = (uint8_t)(QuarterSine(memory, TrigTableOffset(
            angle, TRIG_QUARTER_TURN, false, cpu->decimal)) | 0x80u);
    else if (angle < TRIG_THREE_QUARTER_TURN)
        value = (uint8_t)(QuarterSine(memory, TrigTableOffset(
            angle, TRIG_THREE_QUARTER_TURN, true, cpu->decimal)) | 0x80u);
    else
        value = QuarterSine(memory, TrigTableOffset(
            angle, TRIG_THREE_QUARTER_TURN, false, cpu->decimal));
    RestoreTrigRegisters(memory, cpu, value);
}

void Lufia2CallCosine(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                    uint8_t return_bank, uint16_t return_address) {
    const uint8_t angle = A8(cpu);
    SimulateJslFrame(memory, cpu, return_bank, return_address);
    CalculateCosine(memory, cpu, angle);
    SimulateRtlFrame(memory, cpu);
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
            cpu->carry = true;
            OpSbcValue(cpu, kSteps[i]);
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

static bool TrigEntrySupported(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x80u && !cpu->direct_page;
}

Lufia2ExecutionResult Lufia2SineAngle(const Lufia2Memory *memory,
                                            Lufia2CpuState *cpu) {
    if (!TrigEntrySupported(cpu))
        return ExecutionHandoff(cpu, 0x808450u);
    CalculateSine(memory, cpu, A8(cpu));
    return ExecutionReturned(0x808485u);
}

Lufia2ExecutionResult Lufia2CosineAngle(const Lufia2Memory *memory,
                                            Lufia2CpuState *cpu) {
    if (!TrigEntrySupported(cpu))
        return ExecutionHandoff(cpu, 0x808486u);
    CalculateCosine(memory, cpu, A8(cpu));
    return ExecutionReturned(0x8084beu);
}
