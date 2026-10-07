#include "core/cpu_ops.h"
#include "field/field_internal.h"
#include "lufia2/field.h"
#include "system/wram.h"

Lufia2ExecutionResult Lufia2FieldAnimationTickSlots(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->program_bank != 0x83u ||
        !cpu->accumulator_is_8_bit || cpu->direct_page || cpu->decimal ||
        cpu->stack < 0x1f60u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x838682u);
    Push8(memory, cpu, PackStatus(cpu));
    SetIndexWidth(cpu, 0u);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_ANIMATION_MASK));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_ANIMATION_NEXT_MASK));
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpLongX(cpu, EVENT_ANIMATION_SLOT_STATE));
        if (cpu->negative) {
            ExchangeAccumulatorBytes(cpu);
            OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_ANIMATION_NEXT_MASK));
            OpTestBits(memory, cpu, OpAbs(cpu, WRAM_ANIMATION_MASK), 1u);
            OpAslA(cpu);
            OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_ANIMATION_NEXT_MASK));
            ExchangeAccumulatorBytes(cpu);
            cpu->carry = 0u;
            OpAdcValue(cpu, 8u);
            OpSta(memory, cpu, OpLongX(cpu, EVENT_ANIMATION_SLOT_STATE));
            OpAndValue(cpu, 0x18u);
            OpCmpValue(cpu, 0x18u);
            if (cpu->zero) {
                OpLda(memory, cpu, OpLongX(cpu, EVENT_ANIMATION_SLOT_STATE));
                OpAndValue(cpu, 0xe7u);
                OpSta(memory, cpu, OpLongX(cpu, EVENT_ANIMATION_SLOT_STATE));
                PushIndex(memory, cpu);
                OpSta(memory, cpu, WRAM_FIELD_REGION_CHANGED_FLAGS);
                OpIncA(cpu);
                OpBitValue(cpu, 2u);
                if (!cpu->zero)
                    TransferDirectToA(cpu);
                OpSta(memory, cpu, OpLongX(cpu, EVENT_ANIMATION_SLOT_STATE));
                OpLda(memory, cpu, OpLongX(cpu, EVENT_ANIMATION_SLOT_OBJECT));
                OpSta(memory, cpu, WRAM_FIELD_SELECTED_OBJECT_RECORD_ID);
                OpLda(memory, cpu, WRAM_FIELD_SELECTED_OBJECT_RECORD_ID);
                const uint32_t site = cpu->negative ? 0x8386d6u : 0x8386dbu;
                const uint32_t target = cpu->negative ? 0x838783u : 0x8387ccu;
                SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
                if (!child(context, cpu, target, site, 2u)) {
                    Lufia2ExecutionResult result = ExecutionReturned(site);
                    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
                    return result;
                }
                OpPullX(memory, cpu);
            }
        }
        OpInx(cpu);
        OpCpx(cpu, 8u);
    } while (!cpu->zero);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x8386e9u);
}
