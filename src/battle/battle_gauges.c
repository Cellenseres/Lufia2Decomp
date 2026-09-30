#include "core/cpu_ops.h"
#include "lufia2/battle.h"
#include "lufia2/system.h"

/* Shared $81:E308 tail, including the original DB changes and carry chains. */
static void GaugeFill(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, 0x11u));
    OpSta(memory, cpu, OpDp(cpu, 0x4eu));
    if (!cpu->zero) {
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, 0x1au);
        OpSta(memory, cpu, OpDp(cpu, 0x50u));
        Push8(memory, cpu, cpu->program_bank);
        PullDataBank(memory, cpu);
        SimulateJslFrame(memory, cpu, 0x81u, 0xe31bu);
        (void)Lufia2Multiply16By8(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        OpSetDataBank(memory, cpu, 0x7eu);
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpDp(cpu, 0x51u));
        OpSta(memory, cpu, OpDp(cpu, 0x4eu));
        OpLda(memory, cpu, OpDp(cpu, 0x13u));
        OpSta(memory, cpu, OpDp(cpu, 0x51u));
        SimulateJslFrame(memory, cpu, 0x81u, 0xe32du);
        (void)Lufia2Divide16(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        OpCmpValue(cpu, 0u);
        if (!cpu->zero) {
            OpLda(memory, cpu, OpDp(cpu, 0x4eu));
            if (cpu->zero)
                OpStepMem(memory, cpu, OpDp(cpu, 0x4eu), 1);
        }
    }
    OpSepWidths(cpu, 0x20u);
    for (unsigned part = 0; part < 4u; ++part) {
        const uint8_t cap = part == 0u || part == 3u ? 5u : 8u;
        OpLda(memory, cpu, OpDp(cpu, 0x4eu));
        OpCmpValue(cpu, (uint8_t)(cap + 1u));
        if (cpu->carry)
            OpLoadA(cpu, cap);
        if (part < 3u)
            OpSta(memory, cpu, OpDp(cpu, 0x16u));
        cpu->carry = 0;
        OpAdc(memory, cpu, OpDp(cpu, 0x15u));
        if (part == 1u || part == 2u)
            OpAdcValue(cpu, 6u);
        else if (part == 3u)
            OpAdcValue(cpu, 0x0fu);
        OpSta(memory, cpu, OpAbsY(cpu, (uint16_t)(0x0au + 2u * part)));
        if (part < 3u) {
            OpLda(memory, cpu, OpDp(cpu, 0x4eu));
            cpu->carry = 1;
            OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0x16u)));
            OpSta(memory, cpu, OpDp(cpu, 0x4eu));
        }
    }
}

/* $81:E2AF: two label tiles and the four-part proportional gauge. */
Lufia2ExecutionResult Lufia2BattleStatusGauge(const Lufia2Memory *memory,
                                              Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, 0x16u));
    OpSta(memory, cpu, OpAbsY(cpu, 0u));
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, 2u));
    OpLoadA(cpu, 0x21u);
    OpSta(memory, cpu, OpAbsY(cpu, 1u));
    OpSta(memory, cpu, OpAbsY(cpu, 3u));
    for (unsigned i = 0; i < 6u; ++i)
        OpDey(cpu);
    GaugeFill(memory, cpu);
    return ExecutionReturned(0x81e38eu);
}

/* $81:E2C8: clamped three-digit value with blank leading zero tiles. */
Lufia2ExecutionResult Lufia2BattleStatusDigits(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu) {
    PushY(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, 0x16u));
    OpSta(memory, cpu, OpAbsY(cpu, 0u));
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, 2u));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x11u)));
    OpCpx(cpu, 0x3e8u);
    if (cpu->carry)
        OpLdx(cpu, 0x3e7u);
    SimulateJsrFrame(memory, cpu, 0xe2deu);
    (void)Lufia2DecimalDigits3(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, 4u));
    OpSta(memory, cpu, OpAbsY(cpu, 6u));
    OpLda(memory, cpu, OpDp(cpu, 0xb4u));
    if (!cpu->zero) {
        cpu->carry = 0;
        OpAdcValue(cpu, 0x40u);
        OpSta(memory, cpu, OpAbsY(cpu, 4u));
        OpLda(memory, cpu, OpDp(cpu, 0xb3u));
        goto tens;
    }
    OpLda(memory, cpu, OpDp(cpu, 0xb3u));
    if (!cpu->zero) {
    tens:
        cpu->carry = 0;
        OpAdcValue(cpu, 0x40u);
        OpSta(memory, cpu, OpAbsY(cpu, 6u));
    }
    OpLda(memory, cpu, OpDp(cpu, 0xb2u));
    cpu->carry = 0;
    OpAdcValue(cpu, 0x40u);
    OpSta(memory, cpu, OpAbsY(cpu, 8u));
    OpPullY(memory, cpu);
    return ExecutionReturned(0x81e307u);
}
