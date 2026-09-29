/* Battle background preparation at $81:B9C7. */

#include "core/cpu_ops.h"
#include "lufia2/battle.h"

static Lufia2ExecutionResult BackgroundChildUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static uint8_t BackgroundCall(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                              Lufia2PushedChildCall child, void *context,
                              uint32_t site, uint32_t target, uint8_t frame_size) {
    if (frame_size == 2u)
        SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    else
        SimulateJslFrame(memory, cpu, 0x81u, (uint16_t)(site + 3u));
    return child(context, cpu, target, site, frame_size);
}

#define BACKGROUND_CALL(site, target, frame) \
    do { \
        if (!BackgroundCall(memory, cpu, child, child_context, \
                            0x810000u | (site), (target), (frame))) \
            return BackgroundChildUnwound(0x810000u | (site)); \
    } while (0)

Lufia2ExecutionResult Lufia2BattleBackgroundPrepare(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    TransferDirectToA(cpu);                                 /* B9C7 */
    OpLda(memory, cpu, OpAbs(cpu, 0x11e1u));
    OpCmpValue(cpu, 0x18u);
    if (!cpu->zero) {
        OpAslA(cpu);                                        /* B9D2 */
        OpAslA(cpu);
        OpTax(cpu);
        for (uint16_t i = 0; i < 4u; ++i) {
            OpLda(memory, cpu, OpLongX(cpu, 0x97fd40u + i));
            OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(0x11e2u + i)));
        }
        OpLdx(cpu, 0xc000u);
        OpWriteX(memory, cpu, OpDp(cpu, 0x60u), cpu->x);
        LoadA8(cpu, 0x7eu);
        OpSta(memory, cpu, OpDp(cpu, 0x62u));
        OpLda(memory, cpu, OpAbs(cpu, 0x11e3u));
        OpRepWidths(cpu, 0x20u);
        OpAndValue(cpu, 0x00ffu);
        cpu->carry = 0;
        OpAdcValue(cpu, 0x016cu);
        OpSta(memory, cpu, OpDp(cpu, 0x54u));
        BACKGROUND_CALL(0xba08u, 0x808e9du, 3u);
        OpLda(memory, cpu, OpAbs(cpu, 0x11e2u));
        OpAndValue(cpu, 0x00ffu);
        cpu->carry = 0;
        OpAdcValue(cpu, 0x0179u);
        OpSta(memory, cpu, OpDp(cpu, 0x54u));
        OpSepWidths(cpu, 0x20u);
        OpLdx(cpu, 0x2000u);
        OpWriteX(memory, cpu, OpDp(cpu, 0x60u), cpu->x);
        LoadA8(cpu, 0x7eu);
        OpSta(memory, cpu, OpDp(cpu, 0x62u));
        BACKGROUND_CALL(0xba23u, 0x808e9du, 3u);
        OpLda(memory, cpu, OpAbs(cpu, 0x11e4u));
        OpSta(memory, cpu, OpAbs(cpu, 0x4202u));
        LoadA8(cpu, 0x40u);
        OpSta(memory, cpu, OpAbs(cpu, 0x4203u));
        LoadA8(cpu, 0x97u);
        OpSta(memory, cpu, OpDp(cpu, 0x24u));
        OpRepWidths(cpu, 0x20u);
        cpu->carry = 0;
        LoadA16(cpu, 0xcd58u);
        OpAdc(memory, cpu, OpAbs(cpu, 0x4216u));
        OpTay(cpu);
        OpSepWidths(cpu, 0x20u);
        LoadA8(cpu, 2u);
        BACKGROUND_CALL(0xba44u, 0x81b974u, 2u);
        LoadA8(cpu, 3u);
        BACKGROUND_CALL(0xba49u, 0x81b974u, 2u);
        PushDataBank(memory, cpu);
        OpSetDataBank(memory, cpu, 0x7eu);
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbs(cpu, 0x2000u));
        OpLdx(cpu, 2u);
        do {
            cpu->carry = 1;
            OpSbcValue(cpu, OpRead16(memory, OpAbsX(cpu, 0x2000u)));
            OpSta(memory, cpu, OpAbsX(cpu, 0x2000u));
            OpInx(cpu);
            OpInx(cpu);
            OpCpx(cpu, 0x0800u);
        } while (!cpu->zero);
        OpSepWidths(cpu, 0x20u);
        PullDataBank(memory, cpu);
    } else {
        PushDataBank(memory, cpu);                          /* BA6C */
        OpRepWidths(cpu, 0x20u);
        TransferDirectToA(cpu);
        OpSta(memory, cpu, 0x7ec000u);
        OpLdx(cpu, 0xc000u);
        OpLdy(cpu, 0xc001u);
        LoadA16(cpu, 0x0ffeu);
        OpMoveNext(memory, cpu, 0x7eu, 0x7eu);
        LoadA16(cpu, 0x00ffu);
        for (uint16_t i = 0; i < 8u; ++i)
            OpSta(memory, cpu, 0x7ec000u + 2u * i);
        LoadA16(cpu, 0x0900u);
        OpSta(memory, cpu, 0x7e2000u);
        OpLdx(cpu, 0x2000u);
        OpLdy(cpu, 0x2002u);
        LoadA16(cpu, 0x07fdu);
        OpMoveNext(memory, cpu, 0x7eu, 0x7eu);
        OpSepWidths(cpu, 0x20u);
        PullDataBank(memory, cpu);
    }

    LoadA8(cpu, 1u);                                        /* BAB9 */
    OpTestBits(memory, cpu, OpDp(cpu, 0xd9u), 0u);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, 0x1b17u));
    OpLda(memory, cpu, OpAbs(cpu, 0x11e1u));
    if (cpu->zero)
        BACKGROUND_CALL(0xbac6u, 0x85a701u, 3u);
    return ExecutionReturned(0x81bacau);
}
