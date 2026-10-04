/* Small operations of the battle effect interpreter. Each one works on a
 * slot addressed by Y inside bank $7E and reads its operands from the
 * script stream pointed to by the long pointer at $C3. */

#include "core/cpu_internal.h"
#include "core/cpu_ops.h"
#include "core/plain_ops.h"
#include "core/wram_view.h"
#include "lufia2/battle.h"

enum {
    STREAM = 0xc3u,
    SCRATCH = 0x15edu,
    SLOT = 0x7e0000u,
    ANGLE = 0x54u,
    SPEED = 0x5au,
    VELOCITY_A = 0x56u,
    VELOCITY_B = 0x58u,
    SLOT_LOOP_START = 0x03u,
    SLOT_COPY_FROM = 0x02u,
    SLOT_COPY_TO = 0x01u,
    SLOT_REPEAT = 0x07u,
    SLOT_REPEAT_TARGET = 0x08u,
    SLOT_FIELDS = 0x13u,
    SLOT_ANGLE = 0x13u,
    SLOT_SPEED = 0x15u,
    SLOT_VELOCITY_A = 0x1bu,
    SLOT_VELOCITY_B = 0x1du,
    WORK_BANK = 0x7eu,
    VELOCITY_CALL_STACK_FIRST = 0x1f03u,
    VELOCITY_CALL_STACK_LAST = 0x1ffcu
};

/* $81:A40B: reads a field offset byte and a word from the stream and adds
 * the word to the slot field at that offset; M8/X16, JSR. */
Lufia2ExecutionResult Lufia2BattleEffectAddToField(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;
    uint8_t offset;
    uint16_t field;
    uint16_t amount;
    Word16Result sum;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x81a40bu);
    OpSetDataBank(memory, cpu, WORK_BANK);
    wram = WramViewOfCaller(memory, cpu);
    offset = Read8(memory, DirectLongPointer(memory, cpu, STREAM));
    (void)WramStep16(wram, STREAM, 1);
    WramWrite16(wram, SCRATCH, offset);
    field = Sum16Mode(cpu->y, offset, false, cpu->decimal).value;
    amount = Read16Long(memory, DirectLongPointer(memory, cpu, STREAM));
    (void)WramStep16(wram, STREAM, 1);
    (void)WramStep16(wram, STREAM, 1);
    sum = Sum16Mode(amount, WramRead16At(wram, SLOT + SLOT_FIELDS, field), false, cpu->decimal);
    WramWrite16At(wram, SLOT + SLOT_FIELDS, field, sum.value);
    cpu->x = field;
    LeaveSum(cpu, sum);
    return ExecutionReturned(0x81a430u);
}

/* $81:953F: counts down the repeat byte of the slot and loops the stream
 * back to the pointer stored at offset 8 until it reaches zero; M8/X16. */
Lufia2ExecutionResult Lufia2BattleEffectRepeat(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;
    uint8_t count;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x81953fu);
    OpSetDataBank(memory, cpu, WORK_BANK);
    wram = WramViewOfCaller(memory, cpu);
    cpu->x = cpu->y;
    count = (uint8_t)(WramReadAt(wram, SLOT + SLOT_REPEAT, cpu->y) - 1u);
    WramWriteAt(wram, SLOT + SLOT_REPEAT, cpu->y, count);
    if (count == 0) {
        SetNz8(cpu, count);
    } else {
        const uint16_t target =
            WramRead16At(wram, SLOT + SLOT_REPEAT_TARGET, cpu->y);

        WramWrite16(wram, STREAM, target);
        LeaveWord(cpu, target);
    }
    return ExecutionReturned(0x819552u);
}

/* $81:9169: remembers the stream position in the slot, copies its byte at
 * offset 2 to offset 1 and continues at $81:8C58; M8/X16. */
Lufia2ExecutionResult Lufia2BattleEffectMarkLoop(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;
    uint16_t stream;
    uint8_t copied;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x819169u);
    OpSetDataBank(memory, cpu, WORK_BANK);
    wram = WramViewOfCaller(memory, cpu);
    stream = WramRead16(wram, STREAM);
    WramWrite16At(wram, SLOT + SLOT_LOOP_START, cpu->y, stream);
    copied = WramReadAt(wram, SLOT + SLOT_COPY_FROM, cpu->y);
    WramWriteAt(wram, SLOT + SLOT_COPY_TO, cpu->y, copied);
    cpu->accumulator = (uint16_t)((stream & 0xff00u) | copied);
    SetNz8(cpu, copied);
    return ExecutionHandoff(cpu, 0x818c58u);
}

/* $81:A598: velocity of the slot from its angle and speed words at offsets
 * $13 and $15 through $85:DD63, stored at offsets $1B and $1D; M8/X16. */
Lufia2ExecutionResult Lufia2BattleEffectVelocity(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;
    Lufia2Wram wram;
    uint16_t velocity;

    /* Reserve the child's three-byte frame within its native stack band. */
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit ||
        cpu->direct_page != 0u || cpu->stack < VELOCITY_CALL_STACK_FIRST ||
        cpu->stack > VELOCITY_CALL_STACK_LAST)
        return ExecutionHandoff(cpu, 0x81a598u);
    OpSetDataBank(memory, cpu, WORK_BANK);
    wram = WramViewOfCaller(memory, cpu);
    WramWrite16(wram, ANGLE, WramRead16At(wram, SLOT + SLOT_ANGLE, cpu->y));
    WramWrite16(wram, SPEED, WramRead16At(wram, SLOT + SLOT_SPEED, cpu->y));
    SimulateJslFrame(memory, cpu, 0x81u, 0xa5adu);
    result = Lufia2BattleVelocityOfAngle(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    SimulateRtlFrame(memory, cpu);
    WramWrite16At(wram, SLOT + SLOT_VELOCITY_A, cpu->y,
        WramRead16(wram, VELOCITY_A));
    velocity = WramRead16(wram, VELOCITY_B);
    WramWrite16At(wram, SLOT + SLOT_VELOCITY_B, cpu->y, velocity);
    LeaveWord(cpu, velocity);
    return ExecutionReturned(0x81a5bcu);
}
