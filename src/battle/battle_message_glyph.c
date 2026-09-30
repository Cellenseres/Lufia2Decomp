#include "battle/battle_internal.h"

/* $85:9ACE: convert the encoded character word while preserving A and P. */
Lufia2ExecutionResult Lufia2BattleNormalizeGlyph(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 0);
    PushAccumulator16(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, 0x24u));
    cpu->carry = 1;
    OpSbcValue(cpu, 0x20u);
    OpSta(memory, cpu, OpDp(cpu, 0x24u));
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x859adcu);
}

static void StoreGlyphRows(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                           uint16_t end, uint8_t merge) {
    do {
        OpLda(memory, cpu, OpAbsY(cpu, 0x120fu));
        if (merge)
            OpOra(memory, cpu, OpAbsX(cpu, 0x3801u));
        OpSta(memory, cpu, OpAbsX(cpu, 0x3800u));
        OpSta(memory, cpu, OpAbsX(cpu, 0x3801u));
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
        OpSta(memory, cpu, OpAbsX(cpu, 0x3811u));
        OpInx(cpu);
        OpInx(cpu);
        OpIny(cpu);
        OpCpy(cpu, end);
    } while (!cpu->zero);
}

/* $81:EA35: render at the half-tile position in $23, then advance it by two. */
Lufia2ExecutionResult Lufia2BattleRenderGlyph(const Lufia2Memory *memory,
                                              Lufia2CpuState *cpu,
                                              Lufia2PushedChildCall child,
                                              void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);
    if (!BattleCall(&battle, 0xea35u, 0x859aceu, 3u))
        return BattleChildUnwound(&battle);
    Push8(memory, cpu, cpu->data_bank);
    OpLda(memory, cpu, OpDp(cpu, 0x23u));
    OpBitValue(cpu, 1u);
    const uint8_t shifted = !cpu->zero;
    if (!BattleCall(&battle, shifted ? 0xea93u : 0xea43u,
                    shifted ? 0x81eb62u : 0x81eb34u, 2u))
        return BattleChildUnwound(&battle);
    OpSetDataBank(memory, cpu, 0x7eu);
    if (!shifted)
        TransferDirectToA(cpu);
    OpLda(memory, cpu, OpDp(cpu, 0x23u));
    SetAccumulatorWidth(cpu, 0);
    if (shifted)
        OpAndValue(cpu, 0xfeu);
    for (unsigned i = 0; i < 5u; ++i)
        OpAslA(cpu);
    OpTax(cpu);
    OpLdy(cpu, 0u);
    SetAccumulatorWidth(cpu, 1);
    const unsigned segments = shifted ? 4u : 2u;
    for (unsigned segment = 0; segment < segments; ++segment) {
        if (segment) {
            SetAccumulatorWidth(cpu, 0);
            OpTxa(cpu);
            cpu->carry = 0;
            OpAdcValue(cpu, 0x10u);
            OpTax(cpu);
            SetAccumulatorWidth(cpu, 1);
        }
        StoreGlyphRows(memory, cpu, (uint16_t)((segment + 1u) * 8u), shifted);
    }
    OpStepMem(memory, cpu, OpDp(cpu, 0x23u), 1);
    OpStepMem(memory, cpu, OpDp(cpu, 0x23u), 1);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81eb33u);
}
