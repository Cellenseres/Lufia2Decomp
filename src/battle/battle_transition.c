/* Encounter presentation before Battle takes over. */

#include "core/cpu_ops.h"
#include "lufia2/battle.h"

static Lufia2ExecutionResult TransitionChildUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);

    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static uint8_t TransitionCall(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context,
    uint32_t site,
    uint32_t target,
    uint8_t frame_size) {
    if (frame_size == 2u)
        SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    else
        SimulateJslFrame(memory, cpu, 0x84u, (uint16_t)(site + 3u));
    return child(child_context, cpu, target, site, frame_size);
}

Lufia2ExecutionResult Lufia2BattleVisualTransition(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context) {
    PushDataBank(memory, cpu);                                 /* 8BC7 */
    OpLdx(cpu, 0x07fcu);
    TransferDirectToA(cpu);
    OpRepWidths(cpu, 0x20u);
    do {
        OpSta(memory, cpu, OpLongX(cpu, 0x7e2000u));         /* 8BCE */
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
    LoadA8(cpu, 0x08u);
    OpSta(memory, cpu, OpDp(cpu, 0x74u));                    /* 8BF0 */
    if (!TransitionCall(memory, cpu, child, child_context,
                        0x848bf2u, 0x848d4du, 2u))
        return TransitionChildUnwound(0x848bf2u);

    OpLda(memory, cpu, 0x7f0001u);
    if (cpu->zero)
        LoadA8(cpu, 0x0fu);
    else
        OpAndValue(cpu, 0x0fu);
    OpSta(memory, cpu, OpAbs(cpu, 0x212cu));                 /* 8C01 */
    OpLdx(cpu, 0x0000u);
    if (!TransitionCall(memory, cpu, child, child_context,
                        0x848c07u, 0x80f47au, 3u))
        return TransitionChildUnwound(0x848c07u);
    OpLdx(cpu, 0x0002u);
    if (!TransitionCall(memory, cpu, child, child_context,
                        0x848c0eu, 0x80f47au, 3u))
        return TransitionChildUnwound(0x848c0eu);
    LoadA8(cpu, 0xa0u);
    OpSta(memory, cpu, OpDp(cpu, 0x74u));                    /* 8C14 */
    if (!TransitionCall(memory, cpu, child, child_context,
                        0x848c16u, 0x848d4du, 2u))
        return TransitionChildUnwound(0x848c16u);

    LoadA8(cpu, 0x02u);
    OpSta(memory, cpu, 0x002105u);                           /* mode 2 */
    TransferDirectToA(cpu);
    OpSta(memory, cpu, 0x002111u);
    OpSta(memory, cpu, 0x002111u);
    OpSta(memory, cpu, 0x00059cu);
    OpSta(memory, cpu, 0x00059du);
    OpSta(memory, cpu, 0x002112u);
    OpSta(memory, cpu, 0x002112u);
    OpSta(memory, cpu, 0x00059eu);
    OpSta(memory, cpu, 0x00059fu);
    LoadA8(cpu, 0x01u);
    OpSta(memory, cpu, 0x7fd0f2u);
    LoadA8(cpu, 0x80u);
    OpSta(memory, cpu, 0x7fd0f3u);
    LoadA8(cpu, 0x01u);
    OpSta(memory, cpu, OpAbs(cpu, 0x09aau));                 /* 8C4E */
    OpStz(memory, cpu, OpDp(cpu, 0x00u));
    OpStz(memory, cpu, OpDp(cpu, 0x01u));
    OpStz(memory, cpu, OpDp(cpu, 0x04u));
    OpStz(memory, cpu, OpDp(cpu, 0x05u));
    OpStz(memory, cpu, OpAbs(cpu, 0x0582u));
    LoadA8(cpu, 0xc4u);
    OpSta(memory, cpu, OpAbs(cpu, 0x0581u));
    LoadA8(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);                               /* DB = $7E */

    do {
        OpRepWidths(cpu, 0x20u);                            /* 8C65 */
        OpLda(memory, cpu, OpDp(cpu, 0x00u));
        OpIncA(cpu);
        OpIncA(cpu);
        OpAndValue(cpu, 0x00ffu);
        OpSta(memory, cpu, OpDp(cpu, 0x00u));
        cpu->carry = 0;
        OpAdcValue(cpu, 0x0050u);
        OpAndValue(cpu, 0x00ffu);
        OpTax(cpu);
        OpLda(memory, cpu, OpDp(cpu, 0x04u));
        cpu->carry = 0;
        OpAdcValue(cpu, 0x0008u);
        OpAndValue(cpu, 0x00ffu);
        OpSta(memory, cpu, OpDp(cpu, 0x04u));
        OpSepWidths(cpu, 0x20u);
        OpSta(memory, cpu, 0x00211bu);
        ExchangeAccumulatorBytes(cpu);
        OpSta(memory, cpu, 0x00211bu);

        OpLdy(cpu, 0x0000u);
        do {
            OpLda(memory, cpu, OpLongX(cpu, 0x8084edu));  /* 8C91 */
            OpSta(memory, cpu, 0x00211cu);
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, 0x002135u);
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
        OpLda(memory, cpu, OpDp(cpu, 0x04u));
        OpSta(memory, cpu, 0x00211bu);
        OpLda(memory, cpu, OpDp(cpu, 0x05u));
        OpSta(memory, cpu, 0x00211bu);
        TransferDirectToA(cpu);
        OpLda(memory, cpu, OpDp(cpu, 0x00u));
        OpTax(cpu);
        do {
            OpLda(memory, cpu, OpLongX(cpu, 0x8084edu)); /* 8CDD */
            OpSta(memory, cpu, 0x00211cu);
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, 0x002135u);
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

        OpRepWidths(cpu, 0x20u);
        LoadA16(cpu, 0x3100u);
        OpSta(memory, cpu, 0x004302u);
        OpSta(memory, cpu, 0x004312u);
        OpSepWidths(cpu, 0x20u);
        LoadA8(cpu, 0x02u);
        OpSta(memory, cpu, 0x004300u);
        OpSta(memory, cpu, 0x004310u);
        LoadA8(cpu, 0x0du);
        OpSta(memory, cpu, 0x004301u);
        LoadA8(cpu, 0x0fu);
        OpSta(memory, cpu, 0x004311u);
        LoadA8(cpu, 0x7eu);
        OpSta(memory, cpu, 0x004304u);
        OpSta(memory, cpu, 0x004314u);
        LoadA8(cpu, 0x03u);
        OpTestBits(memory, cpu, OpDp(cpu, 0x81u), 1);
        LoadA8(cpu, 0x08u);
        OpTestBits(memory, cpu, OpDp(cpu, 0x74u), 1);
        if (!TransitionCall(memory, cpu, child, child_context,
                            0x848d3fu, 0x848d4du, 2u))
            return TransitionChildUnwound(0x848d3fu);
        OpLda(memory, cpu, OpDp(cpu, 0x00u));
        OpCmpValue(cpu, 0x40u);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);                                 /* 8D4B */
    return ExecutionReturned(0x848d4cu);                      /* RTL */
}
