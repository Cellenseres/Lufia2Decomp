/* Field object state bits ($83:8AC9-$83:8B0D). */

#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    OBJECT_BIT_MASK = 0x54,
    OBJECT_BIT_ID = 0x55,
};

enum ObjectBitAccess {
    OBJECT_BIT_TEST,
    OBJECT_BIT_SET,
    OBJECT_BIT_CLEAR,
};

Lufia2ExecutionResult Lufia2FieldObjectBitIndex(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSta(memory, cpu, OpDp(cpu, OBJECT_BIT_ID));
    /* TDC's high byte reaches both X16 table indices. */
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpDp(cpu, OBJECT_BIT_ID));
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpTax(cpu);
    OpPushX(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, OBJECT_BIT_ID));
    OpAndValue(cpu, 0x07u);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x80be45u));
    OpSta(memory, cpu, OpDp(cpu, OBJECT_BIT_MASK));
    OpPullX(memory, cpu);
    return ExecutionReturned(0x838b0du);
}

static Lufia2ExecutionResult ObjectBitApply(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    enum ObjectBitAccess access) {
    static const uint16_t frames[] = {0x8accu, 0x8ad8u, 0x8ae8u};
    static const uint32_t exits[] = {0x838ad4u, 0x838ae4u, 0x838af6u};
    uint8_t low, high;
    uint16_t frame;

    OpPushX(memory, cpu);
    SimulateJsrFrame(memory, cpu, frames[access]);
    (void)Lufia2FieldObjectBitIndex(memory, cpu);
    low = Pull8(memory, cpu);
    high = Pull8(memory, cpu);
    frame = (uint16_t)(low | ((uint16_t)high << 8));
    /* Scratch can overwrite the child's live RTS frame. */
    if (frame != frames[access])
        return ExecutionHandoff(cpu, 0x830000u | (uint16_t)(frame + 1u));

    switch (access) {
    case OBJECT_BIT_TEST:
        OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_OBJECT_STATE_BITS));
        OpPullX(memory, cpu);
        OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, OBJECT_BIT_MASK)));
        break;
    case OBJECT_BIT_SET:
        OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_OBJECT_STATE_BITS));
        OpOra(memory, cpu, OpDp(cpu, OBJECT_BIT_MASK));
        OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_OBJECT_STATE_BITS));
        OpPullX(memory, cpu);
        break;
    default:
        OpLda(memory, cpu, OpDp(cpu, OBJECT_BIT_MASK));
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
        OpAndValue(cpu, OpReadM(memory, cpu,
            OpLongX(cpu, WRAM_FIELD_OBJECT_STATE_BITS)));
        OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_OBJECT_STATE_BITS));
        OpPullX(memory, cpu);
        break;
    }
    return ExecutionReturned(exits[access]);
}

Lufia2ExecutionResult Lufia2FieldObjectBitTest(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return ObjectBitApply(memory, cpu, OBJECT_BIT_TEST);
}

Lufia2ExecutionResult Lufia2FieldObjectBitSet(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return ObjectBitApply(memory, cpu, OBJECT_BIT_SET);
}

Lufia2ExecutionResult Lufia2FieldObjectBitClear(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return ObjectBitApply(memory, cpu, OBJECT_BIT_CLEAR);
}
