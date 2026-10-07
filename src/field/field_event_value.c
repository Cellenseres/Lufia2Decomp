#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "field/event_script_internal.h"
#include "lufia2/field.h"

enum {
    VALUE_POINTER = 0x5du,
    VALUE_POINTER_BANK = 0x5fu,
    VALUE_SLOT = 0xa7u,
    VALUE_SLOT_FIRST = 0xfbu,
    VALUE_SLOT_STRIDE = 8u,
    VALUE_LOCAL_FIRST = 0xc0u,
    VALUE_LOCAL_END = 0xe0u,
    VALUE_GLOBAL_FIRST = 0xa0u
};

static uint8_t EventValueReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x80u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal;
}

Lufia2ExecutionResult Lufia2FieldResolveEventVariable(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    if (!EventValueReady(cpu))
        return ExecutionHandoff(cpu, 0x80e9bcu);
    OpCmpValue(cpu, VALUE_SLOT_FIRST);
    if (cpu->carry) {
        cpu->carry = 1u;
        OpSbcValue(cpu, VALUE_SLOT_FIRST);
        OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
        OpLoadA(cpu, VALUE_SLOT_STRIDE);
        OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
        OpLoadA(cpu, 0x7fu);
        OpSta(memory, cpu, OpDp(cpu, VALUE_POINTER_BANK));
        OpRepWidths(cpu, 0x20u);
        cpu->carry = 0u;
        OpLda(memory, cpu, OpDp(cpu, VALUE_SLOT));
        OpAdcValue(cpu, EVENT_SLOT_VARIABLES_LOW);
        OpAdc(memory, cpu, OpAbs(cpu, SNES_RDMPYL));
        OpSta(memory, cpu, OpDp(cpu, VALUE_POINTER));
        OpSepWidths(cpu, 0x20u);
        TransferDirectToA(cpu);
        OpLda(memory, cpu, DirectLongPointer(memory, cpu, VALUE_POINTER));
        OpCmpValue(cpu, VALUE_LOCAL_END);
        if (!cpu->carry) {
            OpCmpValue(cpu, VALUE_LOCAL_FIRST);
            if (cpu->carry) {
                cpu->carry = 1u;
                OpSbcValue(cpu, VALUE_LOCAL_FIRST);
            }
        }
    }
    return ExecutionReturned(0x80e9ecu);
}

Lufia2ExecutionResult Lufia2FieldResolveEventValue(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!EventValueReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x80e9edu);
    OpCmpValue(cpu, VALUE_SLOT_FIRST);
    if (!cpu->zero) {
        if (!CallChildWithFrame(memory, cpu, child, context, 0x80e9f1u, 0x80e9bcu, 2u, 0x80u)) {
            Lufia2ExecutionResult result = ExecutionReturned(0x80e9f1u);
            result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
            return result;
        }
        OpCmpValue(cpu, VALUE_LOCAL_FIRST);
        if (!cpu->carry) {
            OpCmpValue(cpu, VALUE_GLOBAL_FIRST);
            if (cpu->carry) {
                PushIndex(memory, cpu);
                ExchangeAccumulatorBytes(cpu);
                OpLoadA(cpu, 0u);
                ExchangeAccumulatorBytes(cpu);
                OpTax(cpu);
                TransferDirectToA(cpu);
                OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_EVENT_VALUE_VARIABLES - VALUE_GLOBAL_FIRST));
                cpu->x = PullIndexValue(memory, cpu);
            }
        }
    }
    return ExecutionReturned(0x80ea08u);
}
