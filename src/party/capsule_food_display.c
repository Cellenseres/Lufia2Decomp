#include "party/capsule_presentation_internal.h"

Lufia2ExecutionResult Lufia2CapsuleRefreshFoodMeter(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CapsulePresentationReady(cpu, true) || !child)
        return ExecutionHandoff(cpu, 0x82d13au);
    CapsuleFeedCall call = {memory, cpu, child, context, {0}};
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x01dau);
    OpLdx(cpu, 0x1201u);
    if (!CapsuleFeedChild(&call, 0x82d142u, 0x8283ebu, 2u))
        return call.result;
    OpSepWidths(cpu, 0x20u);
    if (!CapsuleFeedChild(&call, 0x82d147u, 0x82cc3cu, 2u))
        return call.result;
    if (!cpu->carry &&
        !CapsuleFeedChild(&call, 0x82d14cu, 0x82d150u, 2u))
        return call.result;
    return ExecutionReturned(0x82d14fu);
}

static void ClearFoodMeter(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 0x01dau);
    OpLoadA(cpu, 0x3136u);
    OpSta(memory, cpu, OpLongX(cpu, 0x7e3000u));
    OpLoadA(cpu, 0x3137u);
    OpSta(memory, cpu, OpLongX(cpu, 0x7e3002u));
    OpLdx(cpu, 0x01deu);
    OpLdy(cpu, 0x10u);
    OpLoadA(cpu, 0x3d38u);
    do {
        OpSta(memory, cpu, OpLongX(cpu, 0x7e3000u));
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
}

Lufia2ExecutionResult Lufia2CapsuleDrawFoodMeter(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CapsulePresentationReady(cpu, true) || !child)
        return ExecutionHandoff(cpu, 0x82d150u);
    CapsuleFeedCall call = {memory, cpu, child, context, {0}};
    OpRepWidths(cpu, 0x20u);
    ClearFoodMeter(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    if (!CapsuleFeedChild(&call, 0x82d177u, 0x82c492u, 2u))
        return call.result;
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, 0u));
    OpAndValue(cpu, 0xffu);
    if (!cpu->zero) {
        OpSta(memory, cpu, OpDp(cpu, DP_FORM_COPY_ROWS));
        OpLdx(cpu, 0x01deu);
        do {
            OpLoadA(cpu, 0x3d39u);
            OpSta(memory, cpu, OpLongX(cpu, 0x7e3000u));
            OpStepMem(memory, cpu, OpDp(cpu, DP_FORM_COPY_ROWS), -1);
            if (cpu->zero)
                break;
            OpLoadA(cpu, 0x3d3au);
            OpSta(memory, cpu, OpLongX(cpu, 0x7e3000u));
            OpInx(cpu);
            OpInx(cpu);
            OpStepMem(memory, cpu, OpDp(cpu, DP_FORM_COPY_ROWS), -1);
        } while (!cpu->zero);
    }
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82d1a3u);
}
