#include <stdbool.h>

#include "core/cpu_ops.h"
#include "lufia2/battle.h"
#include "lufia2/system.h"

/* Direct page cells of the gauge and digit routines. */
enum {
    GAUGE_DP_VALUE = 0x11,      /* current value */
    GAUGE_DP_MAXIMUM = 0x13,    /* value at which the gauge is full */
    GAUGE_DP_TILE_BASE = 0x15,  /* first tile of the gauge graphics */
    GAUGE_DP_LABEL_TILE = 0x16, /* tile of the label; also a per-part count */
    GAUGE_DP_REMAINING = 0x4e,  /* gauge cells still to distribute */
    GAUGE_DP_MULTIPLIER = 0x50, /* gauge length in cells */
    GAUGE_DP_PRODUCT = 0x51,    /* multiply result, then the divisor */
    GAUGE_DP_ONES = 0xb2,       /* decimal digits of the value */
    GAUGE_DP_TENS = 0xb3,
    GAUGE_DP_HUNDREDS = 0xb4
};

/* Shared $81:E308 tail, including the original DB changes and carry chains. */
static void GaugeFill(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, GAUGE_DP_VALUE));
    OpSta(memory, cpu, OpDp(cpu, GAUGE_DP_REMAINING));
    if (!cpu->zero) {
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, 0x1au);
        OpSta(memory, cpu, OpDp(cpu, GAUGE_DP_MULTIPLIER));
        Push8(memory, cpu, cpu->program_bank);
        PullDataBank(memory, cpu);
        SimulateJslFrame(memory, cpu, 0x81u, 0xe31bu);
        (void)Lufia2Multiply16By8(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        OpSetDataBank(memory, cpu, 0x7eu);
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpDp(cpu, GAUGE_DP_PRODUCT));
        OpSta(memory, cpu, OpDp(cpu, GAUGE_DP_REMAINING));
        OpLda(memory, cpu, OpDp(cpu, GAUGE_DP_MAXIMUM));
        OpSta(memory, cpu, OpDp(cpu, GAUGE_DP_PRODUCT));
        SimulateJslFrame(memory, cpu, 0x81u, 0xe32du);
        (void)Lufia2Divide16(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        OpCmpValue(cpu, 0u);
        if (!cpu->zero) {
            OpLda(memory, cpu, OpDp(cpu, GAUGE_DP_REMAINING));
            if (cpu->zero)
                OpStepMem(memory, cpu, OpDp(cpu, GAUGE_DP_REMAINING), 1);
        }
    }
    OpSepWidths(cpu, 0x20u);
    for (unsigned part = 0; part < 4u; ++part) {
        const uint8_t cap = part == 0u || part == 3u ? 5u : 8u;
        if (part == 0u)
            OpLda(memory, cpu, OpDp(cpu, GAUGE_DP_REMAINING));
        OpCmpValue(cpu, (uint8_t)(cap + 1u));
        if (cpu->carry)
            OpLoadA(cpu, cap);
        if (part < 3u)
            OpSta(memory, cpu, OpDp(cpu, GAUGE_DP_LABEL_TILE));
        cpu->carry = 0;
        OpAdc(memory, cpu, OpDp(cpu, GAUGE_DP_TILE_BASE));
        if (part == 1u || part == 2u)
            OpAdcValue(cpu, 6u);
        else if (part == 3u)
            OpAdcValue(cpu, 0x0fu);
        OpSta(memory, cpu, OpAbsY(cpu, (uint16_t)(0x0au + 2u * part)));
        if (part < 3u) {
            OpLda(memory, cpu, OpDp(cpu, GAUGE_DP_REMAINING));
            cpu->carry = 1;
            OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, GAUGE_DP_LABEL_TILE)));
            OpSta(memory, cpu, OpDp(cpu, GAUGE_DP_REMAINING));
        }
    }
}

/* $81:E2AF: two label tiles and the four-part proportional gauge. */
Lufia2ExecutionResult Lufia2BattleStatusGauge(const Lufia2Memory *memory,
                                              Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, GAUGE_DP_LABEL_TILE));
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
    bool show_tens;

    PushY(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, GAUGE_DP_LABEL_TILE));
    OpSta(memory, cpu, OpAbsY(cpu, 0u));
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, 2u));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, GAUGE_DP_VALUE)));
    OpCpx(cpu, 0x3e8u);
    if (cpu->carry)
        OpLdx(cpu, 0x3e7u);
    SimulateJsrFrame(memory, cpu, 0xe2deu);
    (void)Lufia2DecimalDigits3(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, 4u));
    OpSta(memory, cpu, OpAbsY(cpu, 6u));
    OpLda(memory, cpu, OpDp(cpu, GAUGE_DP_HUNDREDS));
    if (!cpu->zero) {
        cpu->carry = 0;
        OpAdcValue(cpu, 0x40u);
        OpSta(memory, cpu, OpAbsY(cpu, 4u));
        OpLda(memory, cpu, OpDp(cpu, GAUGE_DP_TENS));
        show_tens = true;
    } else {
        OpLda(memory, cpu, OpDp(cpu, GAUGE_DP_TENS));
        show_tens = !cpu->zero;
    }
    if (show_tens) {
        cpu->carry = 0;
        OpAdcValue(cpu, 0x40u);
        OpSta(memory, cpu, OpAbsY(cpu, 6u));
    }
    OpLda(memory, cpu, OpDp(cpu, GAUGE_DP_ONES));
    cpu->carry = 0;
    OpAdcValue(cpu, 0x40u);
    OpSta(memory, cpu, OpAbsY(cpu, 8u));
    OpPullY(memory, cpu);
    return ExecutionReturned(0x81e307u);
}

Lufia2ExecutionResult Lufia2BattleFillStatusGauge(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x81e308u);
    GaugeFill(memory, cpu);
    return ExecutionReturned(0x81e38eu);
}
