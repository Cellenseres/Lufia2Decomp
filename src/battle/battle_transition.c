/* Battle transition. */

#include "battle/battle_internal.h"
#include "core/snes_registers.h"

enum {
    TRANSITION_ANGLE = 0x00u,
    TRANSITION_ANGLE_HI = 0x01u,
    TRANSITION_RADIUS = 0x04u,
    TRANSITION_RADIUS_HI = 0x05u,
    TRANSITION_CONTROL = 0x74u,
    TRANSITION_HDMA_FLAGS = 0x81u,
};

static void ClearTransitionPlanes(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 0x07fcu);
    TransferDirectToA(cpu);
    OpRepWidths(cpu, 0x20u);

    do {
        OpSta(memory, cpu, OpLongX(cpu, 0x7e2000u));
        OpSta(memory, cpu, OpLongX(cpu, 0x7e2002u));
        OpSta(memory, cpu, OpLongX(cpu, 0x7e2800u));
        OpSta(memory, cpu, OpLongX(cpu, 0x7e2802u));
        OpSta(memory, cpu, OpLongX(cpu, 0x7e3000u));
        OpSta(memory, cpu, OpLongX(cpu, 0x7e3002u));
        OpDex(cpu);
        OpDex(cpu);
        OpDex(cpu);
        OpDex(cpu);
    } while (!cpu->negative);

    OpSepWidths(cpu, 0x20u);
}

static void SeedMosaicTransition(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadA8(cpu, 0x02u);
    OpSta(memory, cpu, SNES_BGMODE);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, SNES_BG3HOFS);
    OpSta(memory, cpu, SNES_BG3HOFS);
    OpSta(memory, cpu, 0x00059cu);
    OpSta(memory, cpu, 0x00059du);
    OpSta(memory, cpu, SNES_BG3VOFS);
    OpSta(memory, cpu, SNES_BG3VOFS);
    OpSta(memory, cpu, 0x00059eu);
    OpSta(memory, cpu, 0x00059fu);

    LoadA8(cpu, 0x01u);
    OpSta(memory, cpu, WRAM_FIELD_MOSAIC_STATE);
    LoadA8(cpu, 0x80u);
    OpSta(memory, cpu, WRAM_FIELD_MOSAIC_ACCUMULATOR);
    LoadA8(cpu, 0x01u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_TRANSITION_FLAGS));

    OpStz(memory, cpu, OpDp(cpu, TRANSITION_ANGLE));
    OpStz(memory, cpu, OpDp(cpu, TRANSITION_ANGLE_HI));
    OpStz(memory, cpu, OpDp(cpu, TRANSITION_RADIUS));
    OpStz(memory, cpu, OpDp(cpu, TRANSITION_RADIUS_HI));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FADE_LEVEL));
    LoadA8(cpu, 0xc4u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FADE_CONTROL));
}

static void BuildSwirlTilemaps(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, TRANSITION_ANGLE));
    OpIncA(cpu);
    OpIncA(cpu);
    OpAndValue(cpu, 0x00ffu);
    OpSta(memory, cpu, OpDp(cpu, TRANSITION_ANGLE));
    cpu->carry = 0;
    OpAdcValue(cpu, 0x0050u);
    OpAndValue(cpu, 0x00ffu);
    OpTax(cpu);

    OpLda(memory, cpu, OpDp(cpu, TRANSITION_RADIUS));
    cpu->carry = 0;
    OpAdcValue(cpu, 0x0008u);
    OpAndValue(cpu, 0x00ffu);
    OpSta(memory, cpu, OpDp(cpu, TRANSITION_RADIUS));
    OpSepWidths(cpu, 0x20u);

    OpSta(memory, cpu, SNES_M7A);
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, SNES_M7A);

    OpLdy(cpu, 0x0000u);
    do {
        OpLda(memory, cpu, OpLongX(cpu, 0x8084edu));
        OpSta(memory, cpu, SNES_M7B);
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, SNES_MPYM);
        cpu->carry = 0;
        OpAdc(memory, cpu, 0x001228u);
        OpAndValue(cpu, 0x03ffu);
        OpOraValue(cpu, 0x6000u);
        OpSta(memory, cpu, OpAbsY(cpu, 0x3040u));

        OpTxa(cpu);
        cpu->carry = 0;
        OpAdcValue(cpu, 0x0008u);
        OpAndValue(cpu, 0x00ffu);
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, 0x0040u);
    } while (!cpu->zero);

    LoadA8(cpu, 0xf0u);
    OpSta(memory, cpu, OpAbs(cpu, 0x3100u));
    OpSta(memory, cpu, OpAbs(cpu, 0x31e1u));
    OpStz(memory, cpu, OpAbs(cpu, 0x32c1u));

    OpLdy(cpu, 0x0000u);
    OpLda(memory, cpu, OpDp(cpu, TRANSITION_RADIUS));
    OpSta(memory, cpu, SNES_M7A);
    OpLda(memory, cpu, OpDp(cpu, TRANSITION_RADIUS_HI));
    OpSta(memory, cpu, SNES_M7A);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpDp(cpu, TRANSITION_ANGLE));
    OpTax(cpu);

    do {
        OpLda(memory, cpu, OpLongX(cpu, 0x8084edu));
        OpSta(memory, cpu, SNES_M7B);
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, SNES_MPYM);
        cpu->carry = 0;
        OpAdc(memory, cpu, 0x001220u);
        OpSta(memory, cpu, OpAbsY(cpu, 0x3101u));

        OpTxa(cpu);
        OpIncA(cpu);
        OpAndValue(cpu, 0x00ffu);
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, 0x00e0u);
        if (cpu->zero)
            OpIny(cpu);
        OpCpy(cpu, 0x01c1u);
    } while (!cpu->zero);
}

static void ConfigureSwirlHdma(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    LoadA16(cpu, 0x3100u);
    OpSta(memory, cpu, SNES_A1TL(0));
    OpSta(memory, cpu, SNES_A1TL(1));
    OpSepWidths(cpu, 0x20u);

    LoadA8(cpu, 0x02u);
    OpSta(memory, cpu, SNES_DMAP(0));
    OpSta(memory, cpu, SNES_DMAP(1));
    LoadA8(cpu, 0x0du);
    OpSta(memory, cpu, SNES_BBAD(0));
    LoadA8(cpu, 0x0fu);
    OpSta(memory, cpu, SNES_BBAD(1));
    LoadA8(cpu, 0x7eu);
    OpSta(memory, cpu, SNES_A1B(0));
    OpSta(memory, cpu, SNES_A1B(1));

    LoadA8(cpu, 0x03u);
    OpTestBits(memory, cpu, OpDp(cpu, TRANSITION_HDMA_FLAGS), 1u);
    LoadA8(cpu, 0x08u);
    OpTestBits(memory, cpu, OpDp(cpu, TRANSITION_CONTROL), 1u);
}

Lufia2ExecutionResult Lufia2BattleVisualTransition(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *child_context) {
    Lufia2BattleChildCalls calls = {memory, cpu, child, child_context, 0x84u, 0u};

    PushDataBank(memory, cpu);
    ClearTransitionPlanes(memory, cpu);

    LoadA8(cpu, 0x08u);
    OpSta(memory, cpu, OpDp(cpu, TRANSITION_CONTROL));
    if (!Lufia2BattleCallChild(&calls, 0x8bf2u, 0x848d4du, 2u))
        return Lufia2BattleChildUnwound(&calls);

    OpLda(memory, cpu, 0x7f0001u);
    if (cpu->zero)
        LoadA8(cpu, 0x0fu);
    else
        OpAndValue(cpu, 0x0fu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_TM));

    OpLdx(cpu, 0x0000u);
    if (!Lufia2BattleCallChild(&calls, 0x8c07u, 0x80f47au, 3u))
        return Lufia2BattleChildUnwound(&calls);
    OpLdx(cpu, 0x0002u);
    if (!Lufia2BattleCallChild(&calls, 0x8c0eu, 0x80f47au, 3u))
        return Lufia2BattleChildUnwound(&calls);

    LoadA8(cpu, 0xa0u);
    OpSta(memory, cpu, OpDp(cpu, TRANSITION_CONTROL));
    if (!Lufia2BattleCallChild(&calls, 0x8c16u, 0x848d4du, 2u))
        return Lufia2BattleChildUnwound(&calls);

    SeedMosaicTransition(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7eu);

    do {
        BuildSwirlTilemaps(memory, cpu);
        ConfigureSwirlHdma(memory, cpu);

        if (!Lufia2BattleCallChild(&calls, 0x8d3fu, 0x848d4du, 2u))
            return Lufia2BattleChildUnwound(&calls);

        OpLda(memory, cpu, OpDp(cpu, TRANSITION_ANGLE));
        OpCmpValue(cpu, 0x40u);
    } while (!cpu->zero);

    PullDataBank(memory, cpu);
    return ExecutionReturned(0x848d4cu);
}
