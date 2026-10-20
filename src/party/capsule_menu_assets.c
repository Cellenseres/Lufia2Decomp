#include "party/capsule_presentation_internal.h"

Lufia2ExecutionResult Lufia2CapsuleLoadFormPalette(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CapsulePresentationReady(cpu, true))
        return ExecutionHandoff(cpu, 0x82cf72u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, 0u));
    OpAndValue(cpu, 0xffu);
    for (unsigned shift = 0; shift < 5u; ++shift)
        OpAslA(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x0420u);
    OpTay(cpu);
    OpLda(memory, cpu, OpDp(cpu, 2u));
    OpAndValue(cpu, 0xffu);
    for (unsigned shift = 0; shift < 5u; ++shift)
        OpAslA(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, 0xd258u);
    OpTax(cpu);
    OpLoadA(cpu, 0x1fu);
    PushDataBank(memory, cpu);
    OpMoveNext(memory, cpu, 0u, 0x97u);
    PullDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82cf9cu);
}

Lufia2ExecutionResult Lufia2CapsulePlaceSelectedSprite(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CapsulePresentationReady(cpu, true))
        return ExecutionHandoff(cpu, 0x82d248u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_SELECTED_ID));
    OpAndValue(cpu, 0xffu);
    OpAslA(cpu);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLdy(cpu, 1u);
    OpLda(memory, cpu, 0x8ee458u + cpu->x);
    OpSta(memory, cpu, OpAbsY(cpu, 0x1388u));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbsY(cpu, 0x13b8u));
    OpLda(memory, cpu, 0x8ee459u + cpu->x);
    OpSta(memory, cpu, OpAbsY(cpu, 0x13e8u));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbsY(cpu, 0x1418u));
    return ExecutionReturned(0x82d26fu);
}
