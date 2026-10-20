#include "party/capsule_presentation_internal.h"

enum {
    DP_FORM_RED = 0x54u,
    DP_FORM_GREEN = 0x56u,
    DP_FORM_BLUE = 0x58u,
    FORM_RED_MASK = 0x001fu,
    FORM_GREEN_MASK = 0x03e0u,
    FORM_BLUE_MASK = 0x7c00u,
    DP_FORM_BLEND_CONTROL = 0xf2u
};

Lufia2ExecutionResult Lufia2CapsuleRotateFormColors(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CapsulePresentationReady(cpu, false))
        return ExecutionHandoff(cpu, 0x82d397u);
    OpLda(memory, cpu, OpAbsX(cpu, 4u));
    PushAccumulator16(memory, cpu);
    OpLda(memory, cpu, OpAbsX(cpu, 2u));
    OpSta(memory, cpu, OpAbsX(cpu, 4u));
    OpLda(memory, cpu, OpAbsX(cpu, 0u));
    OpSta(memory, cpu, OpAbsX(cpu, 2u));
    PullAccumulator16(memory, cpu);
    OpSta(memory, cpu, OpAbsX(cpu, 0u));
    return ExecutionReturned(0x82d3abu);
}

Lufia2ExecutionResult Lufia2CapsuleTickFormPalette(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CapsulePresentationReady(cpu, true) || !child)
        return ExecutionHandoff(cpu, 0x82d378u);
    CapsuleFeedCall call = {memory, cpu, child, context, {0}};
    OpStepMem(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_PANEL_CONTROL), -1);
    if (cpu->zero) {
        OpLoadA(cpu, 6u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_PANEL_CONTROL));
        OpRepWidths(cpu, 0x20u);
        OpLdx(cpu, FORM_PALETTE_TARGET);
        if (!CapsuleFeedChild(&call, 0x82d387u, 0x82d397u, 2u))
            return call.result;
        OpLdx(cpu, FORM_PALETTE_CURRENT);
        if (!CapsuleFeedChild(&call, 0x82d38du, 0x82d397u, 2u))
            return call.result;
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, 0x80u);
        OpSta(memory, cpu, OpDp(cpu, DP_NMI_UPLOAD_FLAGS));
    }
    return ExecutionReturned(0x82d396u);
}

static void AdvanceFormComponent(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t mask, uint16_t step, uint8_t scratch) {
    OpLda(memory, cpu, OpAbsY(cpu, 0u));
    OpAndValue(cpu, mask);
    OpSta(memory, cpu, OpDp(cpu, scratch));
    OpLda(memory, cpu, OpAbsX(cpu, 0u));
    OpAndValue(cpu, mask);
    OpCmp(memory, cpu, OpDp(cpu, scratch));
    if (!cpu->zero) {
        if (step == 1u) {
            OpIncA(cpu);
        } else {
            cpu->carry = 0u;
            OpAdcValue(cpu, step);
        }
        OpSta(memory, cpu, OpDp(cpu, scratch));
    }
}

static void StoreFormColor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, DP_FORM_RED));
    OpOra(memory, cpu, OpDp(cpu, DP_FORM_GREEN));
    OpOra(memory, cpu, OpDp(cpu, DP_FORM_BLUE));
    OpSta(memory, cpu, OpAbsX(cpu, 0u));
}

Lufia2ExecutionResult Lufia2CapsuleAdvanceFormColors(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CapsulePresentationReady(cpu, true))
        return ExecutionHandoff(cpu, 0x82d3acu);
    OpLoadA(cpu, 6u);
    OpCmp(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_PANEL_CONTROL));
    if (cpu->zero) {
        OpRepWidths(cpu, 0x20u);
        OpLdx(cpu, FORM_PALETTE_CURRENT);
        OpLdy(cpu, FORM_PALETTE_TARGET);
        do {
            AdvanceFormComponent(memory, cpu, FORM_RED_MASK, 1u, DP_FORM_RED);
            AdvanceFormComponent(memory, cpu, FORM_GREEN_MASK, 0x20u, DP_FORM_GREEN);
            AdvanceFormComponent(memory, cpu, FORM_BLUE_MASK, 0x400u, DP_FORM_BLUE);
            StoreFormColor(memory, cpu);
            OpIny(cpu);
            OpIny(cpu);
            OpInx(cpu);
            OpInx(cpu);
            OpCpx(cpu, FORM_PALETTE_END);
        } while (!cpu->zero);
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, 0x80u);
        OpSta(memory, cpu, OpDp(cpu, DP_NMI_UPLOAD_FLAGS));
        OpRepWidths(cpu, 0x20u);
        OpLdx(cpu, FORM_PALETTE_CURRENT);
        OpLdy(cpu, FORM_PALETTE_TARGET);
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        OpCmp(memory, cpu, OpAbsY(cpu, 0u));
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbsX(cpu, 2u));
            OpCmp(memory, cpu, OpAbsY(cpu, 2u));
            if (cpu->zero) {
                OpLda(memory, cpu, OpAbsX(cpu, 4u));
                OpCmp(memory, cpu, OpAbsY(cpu, 4u));
                if (cpu->zero) {
                    OpSepWidths(cpu, 0x20u);
                    cpu->carry = 0u;
                    return ExecutionReturned(0x82d43eu);
                }
            }
        }
    }
    OpSepWidths(cpu, 0x20u);
    cpu->carry = 1u;
    return ExecutionReturned(0x82d442u);
}

static void FadeFormComponent(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t mask, uint16_t step, uint8_t scratch) {
    OpLda(memory, cpu, OpAbsX(cpu, 0u));
    OpAndValue(cpu, mask);
    if (!cpu->zero) {
        if (step == 1u) {
            OpDecA(cpu);
        } else {
            cpu->carry = 1u;
            OpSbcValue(cpu, step);
        }
        OpSta(memory, cpu, OpDp(cpu, scratch));
    }
}

Lufia2ExecutionResult Lufia2CapsuleFadeFormColors(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CapsulePresentationReady(cpu, true))
        return ExecutionHandoff(cpu, 0x82d443u);
    OpLoadA(cpu, 6u);
    OpCmp(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_PANEL_CONTROL));
    if (cpu->zero) {
        OpRepWidths(cpu, 0x20u);
        OpLdx(cpu, FORM_PALETTE_CURRENT);
        do {
            OpStz(memory, cpu, OpDp(cpu, DP_FORM_RED));
            OpStz(memory, cpu, OpDp(cpu, DP_FORM_GREEN));
            OpStz(memory, cpu, OpDp(cpu, DP_FORM_BLUE));
            FadeFormComponent(memory, cpu, FORM_RED_MASK, 1u, DP_FORM_RED);
            FadeFormComponent(memory, cpu, FORM_GREEN_MASK, 0x20u, DP_FORM_GREEN);
            FadeFormComponent(memory, cpu, FORM_BLUE_MASK, 0x400u, DP_FORM_BLUE);
            StoreFormColor(memory, cpu);
            OpInx(cpu);
            OpInx(cpu);
            OpCpx(cpu, FORM_PALETTE_END);
        } while (!cpu->zero);
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, 0x80u);
        OpSta(memory, cpu, OpDp(cpu, DP_NMI_UPLOAD_FLAGS));
        OpLdx(cpu, FORM_PALETTE_CURRENT);
        OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, 0u)));
        if (cpu->zero) {
            OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, 2u)));
            if (cpu->zero) {
                OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, 4u)));
                if (cpu->zero) {
                    cpu->carry = 0u;
                    return ExecutionReturned(0x82d4a5u);
                }
            }
        }
    }
    cpu->carry = 1u;
    return ExecutionReturned(0x82d4a7u);
}

Lufia2ExecutionResult Lufia2CapsuleBeginFormBlend(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CapsulePresentationReady(cpu, true))
        return ExecutionHandoff(cpu, 0x82d4a8u);
    OpLdx(cpu, 0xe44eu);
    OpWriteX(memory, cpu, OpDp(cpu, 0xf4u), cpu->x);
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, 0xf6u));
    OpLoadA(cpu, 0x20u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_FORM_BLEND_CONTROL), 1u);
    OpLoadA(cpu, 0x50u);
    OpSta(memory, cpu, OpDp(cpu, 0xf3u));
    OpLoadA(cpu, 0x2cu);
    OpSta(memory, cpu, OpDp(cpu, 0xfau));
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpDp(cpu, 0xfbu));
    OpLdx(cpu, 0x81c0u);
    OpWriteX(memory, cpu, OpDp(cpu, 0xf7u), cpu->x);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, 0xf9u));
    OpLoadA(cpu, 7u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DISPLAY_REQUESTS));
    return ExecutionReturned(0x82d4cfu);
}

Lufia2ExecutionResult Lufia2CapsuleEndFormBlend(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CapsulePresentationReady(cpu, true) || !child)
        return ExecutionHandoff(cpu, 0x82d4d0u);
    CapsuleFeedCall call = {memory, cpu, child, context, {0}};
    OpLoadA(cpu, 0x1fu);
    OpSta(memory, cpu, OpAbs(cpu, 0x212cu));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbs(cpu, 0x212du));
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpAbs(cpu, 0x2130u));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbs(cpu, 0x2131u));
    OpLoadA(cpu, 0x20u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_FORM_BLEND_CONTROL), 0u);
    OpLoadA(cpu, 6u);
    if (!CapsuleFeedChild(&call, 0x82d4eau, 0x8293f6u, 2u))
        return call.result;
    return ExecutionReturned(0x82d4edu);
}
