/* World map scroll step: a distance and an angle become signed offsets, and
 * the offsets move the scroll position. */

#include "core/cpu_ops.h"
#include "core/cpu_internal.h"
#include "lufia2/world_map.h"

enum {
    MULTIPLICAND = 0x4eu,
    MULTIPLIER = 0x50u,
    PRODUCT = 0x51u,
    PRODUCT_MIDDLE = 0x52u,
    PRODUCT_TOP = 0x53u,
    ANGLE = 0x1248u,
    DISTANCE = 0x1249u,
    SCALE_TABLE = 0x97b226u,
    HARDWARE_A = 0x4202u,
    HARDWARE_B = 0x4203u,
    HARDWARE_PRODUCT = 0x4216u,
    SAVED_SCALE = 0x0eu,
    QUADRANT = 0x10u,
    STEP_X_LOW = 0x08u,
    STEP_X_HIGH = 0x0au,
    STEP_Y_LOW = 0x0bu,
    STEP_Y_HIGH = 0x0du,
    STEP_ANGLE = 0x05u
};

/* $86:A583: 24-bit product of the word at $4E and the byte at $50 stored
 * from $51; the multiplier is fed one byte of the word at a time. M8/X16. */
Lufia2ExecutionResult Lufia2WorldProduct16By8(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86a583u);
    LoadA8(cpu, DirectByte(memory, cpu, MULTIPLIER));
    StoreAAbsolute8(memory, cpu, HARDWARE_A, 0);
    LoadA8(cpu, DirectByte(memory, cpu, MULTIPLICAND));
    StoreAAbsolute8(memory, cpu, HARDWARE_B, 0);
    LoadA8(cpu, DirectByte(memory, cpu, MULTIPLICAND + 1u));
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, HARDWARE_PRODUCT, 0));
    StoreXDirect16(memory, cpu, PRODUCT);
    StoreAAbsolute8(memory, cpu, HARDWARE_B, 0);
    Write8(memory, DirectAddress(cpu, PRODUCT_TOP), 0);
    SetAccumulatorWidth(cpu, 0);
    LoadADirect16(memory, cpu, PRODUCT_MIDDLE);
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, HARDWARE_PRODUCT, 0));
    StoreADirect16(memory, cpu, PRODUCT_MIDDLE);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x86a5a8u);
}

/* JSR $A583 from the step routine. */
static int Scale(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t last_byte, Lufia2ExecutionResult *result) {
    SimulateJsrFrame(memory, cpu, last_byte);
    *result = Lufia2WorldProduct16By8(memory, cpu);
    if (result->flow != LUFIA2_EXECUTION_RETURNED)
        return 0;
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* 16-bit negation as EOR #$FFFF, INC A. */
static void Negate16(Lufia2CpuState *cpu) {
    LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffffu));
    IncrementA16(cpu);
}

/* Stores the sign extension byte of a negated word: $FF unless the
 * negation came out as zero. Flags come from the INC before it. */
static void StoreSignByte(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t offset) {
    SetAccumulatorWidth(cpu, 1);
    if (!cpu->zero)
        LoadA8(cpu, 0xffu);
    StoreADirect8(memory, cpu, offset);
}

/* The four quadrant handlers selected through the table at $A46B. */
static void QuadrantOffsets(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    unsigned quadrant) {
    switch (quadrant) {
    case 0:
        LoadADirect16(memory, cpu, SAVED_SCALE);
        Negate16(cpu);
        StoreADirect16(memory, cpu, STEP_X_LOW);
        StoreSignByte(memory, cpu, STEP_X_HIGH);
        SetAccumulatorWidth(cpu, 0);
        LoadADirect16(memory, cpu, PRODUCT_MIDDLE);
        Negate16(cpu);
        StoreADirect16(memory, cpu, STEP_Y_LOW);
        StoreSignByte(memory, cpu, STEP_Y_HIGH);
        break;
    case 1:
        LoadADirect16(memory, cpu, SAVED_SCALE);
        StoreADirect16(memory, cpu, STEP_Y_LOW);
        LoadADirect16(memory, cpu, PRODUCT_MIDDLE);
        Negate16(cpu);
        StoreADirect16(memory, cpu, STEP_X_LOW);
        StoreSignByte(memory, cpu, STEP_X_HIGH);
        Write8(memory, DirectAddress(cpu, STEP_Y_HIGH), 0);
        break;
    case 2:
        LoadADirect16(memory, cpu, SAVED_SCALE);
        StoreADirect16(memory, cpu, STEP_X_LOW);
        LoadADirect16(memory, cpu, PRODUCT_MIDDLE);
        StoreADirect16(memory, cpu, STEP_Y_LOW);
        SetAccumulatorWidth(cpu, 1);
        Write8(memory, DirectAddress(cpu, STEP_X_HIGH), 0);
        Write8(memory, DirectAddress(cpu, STEP_Y_HIGH), 0);
        break;
    default:
        LoadADirect16(memory, cpu, PRODUCT_MIDDLE);
        StoreADirect16(memory, cpu, STEP_X_LOW);
        LoadADirect16(memory, cpu, SAVED_SCALE);
        Negate16(cpu);
        StoreADirect16(memory, cpu, STEP_Y_LOW);
        StoreSignByte(memory, cpu, STEP_Y_HIGH);
        Write8(memory, DirectAddress(cpu, STEP_X_HIGH), 0);
        break;
    }
}

/* $86:A417: signed offsets $08/$0A (horizontal) and $0B/$0D (vertical) for
 * the distance at $1249 in the direction $1248 (angle in the low six
 * bits, quadrant in the top two). M8/X16. */
Lufia2ExecutionResult Lufia2WorldStepOffsets(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86a417u);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, DISTANCE, 0));
    StoreXDirect16(memory, cpu, MULTIPLICAND);
    LoadA8(cpu, AbsoluteByte(memory, cpu, ANGLE, 0));
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    StoreADirect8(memory, cpu, QUADRANT);
    LoadA8(cpu, 0x00u);
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, AbsoluteByte(memory, cpu, ANGLE, 0));
    And8(cpu, 0x3fu);
    if (!cpu->zero) {
        StoreADirect8(memory, cpu, STEP_ANGLE);
        AslA8(cpu);
        TransferAToX(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(SCALE_TABLE, cpu->x)));
        StoreADirect8(memory, cpu, MULTIPLIER);
        if (!Scale(memory, cpu, 0xa43du, &result))
            return result;
        LoadXDirect16(memory, cpu, PRODUCT_MIDDLE);
        StoreXDirect16(memory, cpu, SAVED_SCALE);
        LoadA8(cpu, 0x00u);
        ExchangeAccumulatorBytes(cpu);
        LoadA8(cpu, 0x40u);
        cpu->carry = 1;
        Sbc8(cpu, DirectByte(memory, cpu, STEP_ANGLE));
        AslA8(cpu);
        TransferAToX(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(SCALE_TABLE, cpu->x)));
        StoreADirect8(memory, cpu, MULTIPLIER);
        if (!Scale(memory, cpu, 0xa454u, &result))
            return result;
    } else {
        StoreXDirect16(memory, cpu, PRODUCT_MIDDLE);
        LoadX16(cpu, 0);
        StoreXDirect16(memory, cpu, SAVED_SCALE);
    }
    LoadA8(cpu, DirectByte(memory, cpu, QUADRANT));
    AslA8(cpu);
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x0006u);
    TransferAToX(cpu);
    SimulateJsrFrame(memory, cpu, 0xa460u);
    QuadrantOffsets(memory, cpu, cpu->x >> 1);
    SetAccumulatorWidth(cpu, 1);
    SimulateRtsFrame(memory, cpu);
    return ExecutionReturned(0x86a461u);
}

/* Adds a 24-bit amount to the scroll fraction and position words. */
static void AddStepWord(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t address, uint8_t step_low, uint8_t step_high) {
    SetAccumulatorWidth(cpu, 0);
    LoadADirect16(memory, cpu, step_low);
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, address, 0));
    StoreAAbsolute16(memory, cpu, address, 0);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, DirectByte(memory, cpu, step_high));
    Adc8(cpu, AbsoluteByte(memory, cpu, (uint16_t)(address + 2u), 0));
    StoreAAbsolute8(memory, cpu, (uint16_t)(address + 2u), 0);
}

/* $86:995B: applies the step of $86:A417 to the 24-bit scroll offsets at
 * $11EC and $11EF, then wraps the positions $11E8 and $11EA to 12 bits and
 * stores them divided by 16 at $11F2 and $11F4. M8/X16. */
Lufia2ExecutionResult Lufia2WorldScrollAdvance(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86995bu);
    SimulateJsrFrame(memory, cpu, 0x995du);
    result = Lufia2WorldStepOffsets(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    SimulateRtsFrame(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    Write16Absolute(memory, cpu, 0x11edu, 0);
    AddStepWord(memory, cpu, 0x11ecu, STEP_X_LOW, STEP_X_HIGH);
    SetAccumulatorWidth(cpu, 0);
    Write16Absolute(memory, cpu, 0x11f0u, 0);
    AddStepWord(memory, cpu, 0x11efu, STEP_Y_LOW, STEP_Y_HIGH);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x11e8u, 0));
    StoreADirect16(memory, cpu, 0x04u);
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x11edu, 0));
    And16(cpu, 0x0fffu);
    StoreAAbsolute16(memory, cpu, 0x11e8u, 0);
    LsrA16(cpu);
    LsrA16(cpu);
    LsrA16(cpu);
    LsrA16(cpu);
    StoreAAbsolute16(memory, cpu, 0x11f2u, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x11eau, 0));
    StoreADirect16(memory, cpu, 0x06u);
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x11f0u, 0));
    And16(cpu, 0x0fffu);
    StoreAAbsolute16(memory, cpu, 0x11eau, 0);
    LsrA16(cpu);
    LsrA16(cpu);
    LsrA16(cpu);
    LsrA16(cpu);
    StoreAAbsolute16(memory, cpu, 0x11f4u, 0);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x8699beu);
}
