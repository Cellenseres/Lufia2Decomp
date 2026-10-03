/* Small operations of the battle effect interpreter. Each one works on a
 * slot addressed by Y inside bank $7E and reads its operands from the
 * script stream pointed to by the long pointer at $C3. */

#include "core/cpu_internal.h"
#include "lufia2/battle.h"

enum {
    STREAM = 0xc3u,
    SCRATCH = 0x15edu,
    FIELD_BASE = 0x0013u,
    ANGLE = 0x54u,
    SPEED = 0x5au,
    VELOCITY_A = 0x56u,
    VELOCITY_B = 0x58u
};

/* INC dp, 16-bit: the high byte is stored first. */
static void IncrementStream(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const uint16_t value = (uint16_t)(Read16Direct(memory, cpu, STREAM) + 1u);

    Write8(memory, DirectAddress(cpu, STREAM + 1u), (uint8_t)(value >> 8));
    Write8(memory, DirectAddress(cpu, STREAM), (uint8_t)value);
    SetNz16(cpu, value);
}

/* $81:A40B: reads a field offset byte and a word from the stream and adds
 * the word to the slot field at that offset; M8/X16, JSR. */
Lufia2ExecutionResult Lufia2BattleEffectAddToField(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x81a40bu);
    SelectDataBank(memory, cpu, 0x7eu);
    LoadA8(cpu, Read8(memory, DirectLongPointer(memory, cpu, STREAM)));
    SetAccumulatorWidth(cpu, 0);
    IncrementStream(memory, cpu);
    And16(cpu, 0x00ffu);
    StoreAAbsolute16(memory, cpu, SCRATCH, 0);
    LoadA16(cpu, cpu->y);
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, SCRATCH, 0));
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, DirectLongPointer(memory, cpu, STREAM)));
    IncrementStream(memory, cpu);
    IncrementStream(memory, cpu);
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, FIELD_BASE, cpu->x));
    StoreAAbsolute16(memory, cpu, FIELD_BASE, cpu->x);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x81a430u);
}

/* $81:953F: counts down the repeat byte of the slot and loops the stream
 * back to the pointer stored at offset 8 until it reaches zero; M8/X16. */
Lufia2ExecutionResult Lufia2BattleEffectRepeat(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    uint8_t count;
    uint32_t address;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x81953fu);
    SelectDataBank(memory, cpu, 0x7eu);
    cpu->x = cpu->y;
    SetNz16(cpu, cpu->x);
    address = AbsoluteIndexedAddress(cpu, 0x0007u, cpu->x);
    count = (uint8_t)(Read8(memory, address) - 1u);
    Write8(memory, address, count);
    SetNz8(cpu, count);
    if (!cpu->zero) {
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0008u, cpu->y));
        StoreADirect16(memory, cpu, STREAM);
        SetAccumulatorWidth(cpu, 1);
    }
    return ExecutionReturned(0x819552u);
}

/* $81:9169: remembers the stream position in the slot, copies its byte at
 * offset 2 to offset 1 and continues at $81:8C58; M8/X16. */
Lufia2ExecutionResult Lufia2BattleEffectMarkLoop(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x819169u);
    SelectDataBank(memory, cpu, 0x7eu);
    SetAccumulatorWidth(cpu, 0);
    LoadADirect16(memory, cpu, STREAM);
    StoreAAbsolute16(memory, cpu, 0x0003u, cpu->y);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, AbsoluteByte(memory, cpu, 0x0002u, cpu->y));
    StoreAAbsolute8(memory, cpu, 0x0001u, cpu->y);
    return ExecutionHandoff(cpu, 0x818c58u);
}

/* $81:A598: velocity of the slot from its angle and speed words at offsets
 * $13 and $15 through $85:DD63, stored at offsets $1B and $1D; M8/X16. */
Lufia2ExecutionResult Lufia2BattleEffectVelocity(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x81a598u);
    SelectDataBank(memory, cpu, 0x7eu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0013u, cpu->y));
    StoreADirect16(memory, cpu, ANGLE);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0015u, cpu->y));
    StoreADirect16(memory, cpu, SPEED);
    SetAccumulatorWidth(cpu, 1);
    SimulateJslFrame(memory, cpu, 0x81u, 0xa5adu);
    result = Lufia2BattleVelocityOfAngle(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    SimulateRtlFrame(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadADirect16(memory, cpu, VELOCITY_A);
    StoreAAbsolute16(memory, cpu, 0x001bu, cpu->y);
    LoadADirect16(memory, cpu, VELOCITY_B);
    StoreAAbsolute16(memory, cpu, 0x001du, cpu->y);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x81a5bcu);
}
