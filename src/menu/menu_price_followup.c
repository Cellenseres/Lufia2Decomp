#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/menu.h"

enum {
    DP_SELECTION_PORTRAIT = 0x22u,
    DP_SELECTION_RECORD = 0x2au,
    DP_SELECTION_KIND = 0x31u,
    DP_SELECTION_TEXT_BANK = 0x5fu,
    SELECTION_FEEDBACK_TABLE = 0x829800u,
    SELECTION_FEEDBACK_VALUE = 0x2106u
};

static bool SelectionSupported(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit && !cpu->decimal;
}

static Lufia2ExecutionResult SelectionChildUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static void SubtractSelectionPrice(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, bool charge) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_GOLD));
    cpu->carry = true;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, WRAM_MENU_PURCHASE_PRICE)));
    if (charge)
        OpSta(memory, cpu, OpAbs(cpu, WRAM_GOLD));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_GOLD + 2u));
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, WRAM_MENU_PURCHASE_PRICE + 2u)));
    if (charge)
        OpSta(memory, cpu, OpAbs(cpu, WRAM_GOLD + 2u));
}

Lufia2ExecutionResult Lufia2MenuCheckSelectionGold(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!SelectionSupported(cpu))
        return ExecutionHandoff(cpu, 0x829855u);
    SubtractSelectionPrice(memory, cpu, false);
    return ExecutionReturned(0x829866u);
}

Lufia2ExecutionResult Lufia2MenuSpendSelectionGold(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!SelectionSupported(cpu))
        return ExecutionHandoff(cpu, 0x82983du);
    SubtractSelectionPrice(memory, cpu, true);
    return ExecutionReturned(0x829854u);
}

Lufia2ExecutionResult Lufia2MenuApplyAlternateSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SelectionSupported(cpu))
        return ExecutionHandoff(cpu, 0x82eddfu);
    OpLda(memory, cpu, OpDp(cpu, DP_SELECTION_KIND));
    if (cpu->zero) {
        OpLoadA(cpu, 0xffu);
        OpLoadA(cpu, (uint8_t)(A8(cpu) ^ 5u));
        OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SELECTION_RECORD)));
        OpAndValue(cpu, OpReadM(memory, cpu, OpAbsY(cpu, 15u)));
        OpSta(memory, cpu, OpAbsY(cpu, 15u));
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbsY(cpu, 0x25u));
        OpSta(memory, cpu, OpAbsY(cpu, 0x11u));
        OpSepWidths(cpu, 0x20u);
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SELECTION_PORTRAIT)));
        OpPushX(memory, cpu);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82edfcu, 0x8299beu, 2u, cpu->program_bank))
            return SelectionChildUnwound(0x82edfcu);
        OpLoadA(cpu, 1u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82ee01u, 0x82a318u, 2u, cpu->program_bank))
            return SelectionChildUnwound(0x82ee01u);
        OpPullX(memory, cpu);
        OpWriteX(memory, cpu, OpDp(cpu, DP_SELECTION_PORTRAIT), cpu->x);
    } else {
        OpRepWidths(cpu, 0x20u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82ee0bu, 0x82f8c6u, 2u, cpu->program_bank))
            return SelectionChildUnwound(0x82ee0bu);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82ee0eu, 0x82f846u, 2u, cpu->program_bank))
            return SelectionChildUnwound(0x82ee0eu);
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SELECTION_RECORD)));
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82ee13u, 0x81f4d5u, 3u, cpu->program_bank))
            return SelectionChildUnwound(0x82ee13u);
    }
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, DP_SELECTION_TEXT_BANK));
    OpLdy(cpu, 0xd4deu);
    OpLdx(cpu, 0x3548u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82ee23u, 0x808878u, 3u, cpu->program_bank))
        return SelectionChildUnwound(0x82ee23u);
    return ExecutionReturned(0x82ee27u);
}

Lufia2ExecutionResult Lufia2MenuPlaySelectionFeedback(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SelectionSupported(cpu))
        return ExecutionHandoff(cpu, 0x8297e3u);
    OpLoadA(cpu, 3u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x8297e5u, 0x80953bu, 3u, cpu->program_bank))
        return SelectionChildUnwound(0x8297e5u);
    OpLdx(cpu, 0u);
    do {
        OpPushX(memory, cpu);
        OpLda(memory, cpu, OpLongX(cpu, SELECTION_FEEDBACK_TABLE));
        OpSta(memory, cpu, OpAbs(cpu, SELECTION_FEEDBACK_VALUE));
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x8297f4u, 0x868b55u, 3u, cpu->program_bank))
            return SelectionChildUnwound(0x8297f4u);
        OpPullX(memory, cpu);
        OpInx(cpu);
        OpCpx(cpu, 8u);
    } while (!cpu->zero);
    return ExecutionReturned(0x8297ffu);
}

Lufia2ExecutionResult Lufia2MenuAdvanceFlaggedEquipment(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x82f8c6u);
    OpLoadA(cpu, 0u);
    do {
        PushAccumulator16(memory, cpu);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82f8cau, 0x82f893u, 2u, cpu->program_bank))
            return SelectionChildUnwound(0x82f8cau);
        if (cpu->carry) {
            OpLda(memory, cpu, AbsoluteIndexedAddress(cpu,
                Read16Direct(memory, cpu, DP_SELECTION_RECORD), cpu->y));
            OpIncA(cpu);
            OpSta(memory, cpu, AbsoluteIndexedAddress(cpu,
                Read16Direct(memory, cpu, DP_SELECTION_RECORD), cpu->y));
        }
        PullAccumulator16(memory, cpu);
        OpIncA(cpu);
        OpCmpValue(cpu, 6u);
    } while (!cpu->zero);
    return ExecutionReturned(0x82f8dbu);
}
