#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/menu.h"

enum {
    DP_SELECTION_RECORD = 0x2au,
    DP_SELECTION_KIND = 0x31u,
    DP_SELECTION_TEXT_BANK = 0x5fu,
    SELECTION_UNK_14DC = WRAM_UNK_7E14DC,
    SELECTION_UNK_155A = WRAM_UNK_7E155A,
    SELECTION_PRICE_BASE = WRAM_RECORD_BUFFER + 0x12u
};

static Lufia2ExecutionResult SelectionChildUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static bool SelectionSupported(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit && !cpu->decimal;
}

Lufia2ExecutionResult Lufia2MenuConfirmExit(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SelectionSupported(cpu))
        return ExecutionHandoff(cpu, 0x82eb9au);
    bool repeat_selection = false;
    for (;;) {
        OpLoadA(cpu, repeat_selection ? 1u : 0u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82eb9cu, 0x828b08u, 2u, cpu->program_bank))
            return SelectionChildUnwound(0x82eb9cu);
        OpCmpValue(cpu, 8u);
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E1558));
            if (!cpu->zero) {
                repeat_selection = true;
                continue;
            }
            OpLoadA(cpu, 2u);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ebe0u, 0x80953bu, 3u, cpu->program_bank))
                return SelectionChildUnwound(0x82ebe0u);
            cpu->carry = false;
            return ExecutionReturned(0x82ebe5u);
        }
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            OpLoadA(cpu, 5u);
            OpLdx(cpu, 8u);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ebb8u, 0x82895bu, 2u, cpu->program_bank))
                return SelectionChildUnwound(0x82ebb8u);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ebbbu, 0x868b55u, 3u, cpu->program_bank))
                return SelectionChildUnwound(0x82ebbbu);
            OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_INDEX));
            if (cpu->zero) {
                OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E1558));
                if (cpu->zero) {
                    OpLoadA(cpu, 2u);
                    if (!CallChildWithFrame(memory, cpu, child, context,
                            0x82ebcbu, 0x80953bu, 3u, cpu->program_bank))
                        return SelectionChildUnwound(0x82ebcbu);
                }
                cpu->carry = false;
                return ExecutionReturned(0x82ebd0u);
            }
            OpLoadA(cpu, 2u);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ebd3u, 0x80953bu, 3u, cpu->program_bank))
                return SelectionChildUnwound(0x82ebd3u);
            break;
        }
        OpCmpValue(cpu, 3u);
        if (cpu->zero)
            break;
        repeat_selection = false;
    }
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82ebe6u, 0x868b32u, 3u, cpu->program_bank))
        return SelectionChildUnwound(0x82ebe6u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82ebeau, 0x82838fu, 2u, cpu->program_bank))
        return SelectionChildUnwound(0x82ebeau);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82ebedu, 0x82f682u, 2u, cpu->program_bank))
        return SelectionChildUnwound(0x82ebedu);
    cpu->carry = true;
    return ExecutionReturned(0x82ebf1u);
}

Lufia2ExecutionResult Lufia2MenuConfirmAlternateSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SelectionSupported(cpu))
        return ExecutionHandoff(cpu, 0x82ed50u);
    OpLdy(cpu, 1u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82ed53u, 0x82f637u, 2u, cpu->program_bank))
        return SelectionChildUnwound(0x82ed53u);
    for (;;) {
        OpLoadA(cpu, 0u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82ed58u, 0x828b08u, 2u, cpu->program_bank))
            return SelectionChildUnwound(0x82ed58u);
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_INDEX));
            if (!cpu->zero) {
                OpLoadA(cpu, 2u);
                if (!CallChildWithFrame(memory, cpu, child, context,
                        0x82eda6u, 0x80953bu, 3u, cpu->program_bank))
                    return SelectionChildUnwound(0x82eda6u);
                break;
            }
            OpLoadA(cpu, 5u);
            OpLdx(cpu, 7u);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ed71u, 0x82895bu, 2u, cpu->program_bank))
                return SelectionChildUnwound(0x82ed71u);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ed74u, 0x868b55u, 3u, cpu->program_bank))
                return SelectionChildUnwound(0x82ed74u);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ed78u, 0x829855u, 2u, cpu->program_bank))
                return SelectionChildUnwound(0x82ed78u);
            if (cpu->carry) {
                if (!CallChildWithFrame(memory, cpu, child, context,
                        0x82ed7du, 0x82983du, 2u, cpu->program_bank))
                    return SelectionChildUnwound(0x82ed7du);
                if (!CallChildWithFrame(memory, cpu, child, context,
                        0x82ed80u, 0x82eddfu, 2u, cpu->program_bank))
                    return SelectionChildUnwound(0x82ed80u);
                OpLoadA(cpu, 0x7bu);
                if (!CallChildWithFrame(memory, cpu, child, context,
                        0x82ed85u, 0x80953bu, 3u, cpu->program_bank))
                    return SelectionChildUnwound(0x82ed85u);
                OpLdy(cpu, 0x1802u);
                if (!CallChildWithFrame(memory, cpu, child, context,
                        0x82ed8cu, 0x82f637u, 2u, cpu->program_bank))
                    return SelectionChildUnwound(0x82ed8cu);
                break;
            }
            OpLdy(cpu, 3u);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ed94u, 0x82f637u, 2u, cpu->program_bank))
                return SelectionChildUnwound(0x82ed94u);
            OpLoadA(cpu, 3u);
            OpLdx(cpu, 7u);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ed9cu, 0x82895bu, 2u, cpu->program_bank))
                return SelectionChildUnwound(0x82ed9cu);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ed9fu, 0x8297e3u, 2u, cpu->program_bank))
                return SelectionChildUnwound(0x82ed9fu);
        } else {
            OpCmpValue(cpu, 3u);
            if (cpu->zero)
                break;
        }
    }
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x0082u);
    OpLdx(cpu, 0x1e0au);
    OpLdy(cpu, 3u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82edb5u, 0x8280cau, 2u, cpu->program_bank))
        return SelectionChildUnwound(0x82edb5u);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 6u);
    OpLdx(cpu, 7u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82edbfu, 0x82895bu, 2u, cpu->program_bank))
        return SelectionChildUnwound(0x82edbfu);
    OpLoadA(cpu, 0u);
    OpLdx(cpu, 7u);
    OpLdy(cpu, 6u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82edcau, 0x8289fau, 2u, cpu->program_bank))
        return SelectionChildUnwound(0x82edcau);
    OpLoadA(cpu, 6u);
    OpLdx(cpu, 7u);
    OpLdy(cpu, 6u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82edd5u, 0x8289bcu, 2u, cpu->program_bank))
        return SelectionChildUnwound(0x82edd5u);
    OpLdy(cpu, 0u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82eddbu, 0x82f637u, 2u, cpu->program_bank))
        return SelectionChildUnwound(0x82eddbu);
    return ExecutionReturned(0x82eddeu);
}

Lufia2ExecutionResult Lufia2MenuRestoreAlternateSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SelectionSupported(cpu))
        return ExecutionHandoff(cpu, 0x82eff6u);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FADE_LEVEL));
    OpLoadA(cpu, 0xd0u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FADE_CONTROL));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_INDEX));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f001u, 0x80914bu, 3u, cpu->program_bank))
        return SelectionChildUnwound(0x82f001u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f005u, 0x85c60eu, 3u, cpu->program_bank))
        return SelectionChildUnwound(0x82f005u);
    do {
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82f009u, 0x8293c2u, 2u, cpu->program_bank))
            return SelectionChildUnwound(0x82f009u);
        OpLda(memory, cpu, OpAbs(cpu, WRAM_FADE_CONTROL));
    } while (!cpu->zero);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f011u, 0x82838fu, 2u, cpu->program_bank))
        return SelectionChildUnwound(0x82f011u);
    OpLdx(cpu, 0xffffu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f017u, 0x829214u, 3u, cpu->program_bank))
        return SelectionChildUnwound(0x82f017u);
    OpLdx(cpu, 0xffffu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f01eu, 0x82922du, 3u, cpu->program_bank))
        return SelectionChildUnwound(0x82f01eu);
    OpLoadA(cpu, 1u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f024u, 0x82e917u, 2u, cpu->program_bank))
        return SelectionChildUnwound(0x82f024u);
    OpLoadA(cpu, 1u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f029u, 0x82a318u, 2u, cpu->program_bank))
        return SelectionChildUnwound(0x82f029u);
    OpRepWidths(cpu, 0x20u);
    static const uint16_t positions[] = {0x0344u, 0x0444u, 0x0466u};
    static const uint16_t dimensions[] = {0x1d04u, 0x1105u, 0x0c0au};
    for (unsigned window = 0u; window < 3u; ++window) {
        const uint32_t site = 0x82f034u + window * 9u;
        OpLoadA(cpu, positions[window]);
        OpLdx(cpu, dimensions[window]);
        if (!CallChildWithFrame(memory, cpu, child, context,
                site, 0x82810eu, 2u, cpu->program_bank))
            return SelectionChildUnwound(site);
    }
    OpSepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpAbs(cpu, SELECTION_UNK_14DC));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_MARKER_ROW));
    OpLoadA(cpu, 1u);
    OpLdx(cpu, 8u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f056u, 0x82895bu, 2u, cpu->program_bank))
        return SelectionChildUnwound(0x82f056u);
    OpLdy(cpu, 0xa8u);
    OpLdx(cpu, 3u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f05fu, 0x82891eu, 2u, cpu->program_bank))
        return SelectionChildUnwound(0x82f05fu);
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, DP_SELECTION_TEXT_BANK));
    OpLdy(cpu, 0xca9au);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f06eu, 0x808878u, 3u, cpu->program_bank))
        return SelectionChildUnwound(0x82f06eu);
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, DP_SELECTION_TEXT_BANK));
    OpLdy(cpu, 0xca3fu);
    OpLdx(cpu, 0x366eu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f07cu, 0x808878u, 3u, cpu->program_bank))
        return SelectionChildUnwound(0x82f07cu);
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, DP_SELECTION_TEXT_BANK));
    OpLdy(cpu, 0xd4deu);
    OpLdx(cpu, 0x34c8u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f08au, 0x808878u, 3u, cpu->program_bank))
        return SelectionChildUnwound(0x82f08au);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f08eu, 0x82f496u, 2u, cpu->program_bank))
        return SelectionChildUnwound(0x82f08eu);
    OpLda(memory, cpu, OpAbs(cpu, SELECTION_UNK_155A));
    if (!cpu->zero) {
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82f096u, 0x82f682u, 2u, cpu->program_bank))
            return SelectionChildUnwound(0x82f096u);
    }
    OpLoadA(cpu, 7u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f09bu, 0x8293f6u, 2u, cpu->program_bank))
        return SelectionChildUnwound(0x82f09bu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f09eu, 0x828704u, 2u, cpu->program_bank))
        return SelectionChildUnwound(0x82f09eu);
    return ExecutionReturned(0x82f0a1u);
}

Lufia2ExecutionResult Lufia2MenuComputeSelectionPrice(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SelectionSupported(cpu))
        return ExecutionHandoff(cpu, 0x82f5b5u);
    OpLda(memory, cpu, OpDp(cpu, DP_SELECTION_KIND));
    if (cpu->zero) {
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SELECTION_RECORD)));
        OpLda(memory, cpu, OpAbsX(cpu, 14u));
        OpSta(memory, cpu, OpAbs(cpu, WRAM_SYSTEM_MULTIPLY_A));
        OpStz(memory, cpu, OpAbs(cpu, WRAM_SYSTEM_MULTIPLY_A + 1u));
        OpStz(memory, cpu, OpAbs(cpu, WRAM_SYSTEM_MULTIPLY_B));
        OpStz(memory, cpu, OpAbs(cpu, WRAM_SYSTEM_MULTIPLY_B + 1u));
        OpLda(memory, cpu, OpAbsX(cpu, 15u));
        OpBitValue(cpu, 1u);
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, WRAM_SYSTEM_MULTIPLY_B));
            cpu->carry = false;
            OpAdcValue(cpu, 2u);
            OpSta(memory, cpu, OpAbs(cpu, WRAM_SYSTEM_MULTIPLY_B));
        }
        OpLda(memory, cpu, OpAbsX(cpu, 15u));
        OpBitValue(cpu, 4u);
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, WRAM_SYSTEM_MULTIPLY_B));
            cpu->carry = false;
            OpAdcValue(cpu, 10u);
            OpSta(memory, cpu, OpAbs(cpu, WRAM_SYSTEM_MULTIPLY_B));
        }
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82f5eau, 0x828000u, 3u, cpu->program_bank))
            return SelectionChildUnwound(0x82f5eau);
        OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_SYSTEM_MULTIPLY_PRODUCT)));
        OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_PURCHASE_PRICE), cpu->x);
        OpLda(memory, cpu, OpAbs(cpu, WRAM_SYSTEM_MULTIPLY_PRODUCT + 2u));
        OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_PURCHASE_PRICE + 2u));
        return ExecutionReturned(0x82f5fau);
    }
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_PURCHASE_PRICE), cpu->x);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_PURCHASE_PRICE + 2u));
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0u);
    do {
        PushAccumulator16(memory, cpu);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82f60au, 0x82f893u, 2u, cpu->program_bank))
            return SelectionChildUnwound(0x82f60au);
        if (cpu->carry) {
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82f60fu, 0x81f1c5u, 3u, cpu->program_bank))
                return SelectionChildUnwound(0x82f60fu);
            OpLda(memory, cpu, OpAbs(cpu, SELECTION_PRICE_BASE));
            for (unsigned shift = 0u; shift < 4u; ++shift)
                OpLsrA(cpu);
            cpu->carry = false;
            OpAdc(memory, cpu, OpAbs(cpu, WRAM_MENU_PURCHASE_PRICE));
            OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_PURCHASE_PRICE));
            OpSepWidths(cpu, 0x20u);
            OpLoadA(cpu, 0u);
            OpAdc(memory, cpu, OpAbs(cpu, WRAM_MENU_PURCHASE_PRICE + 2u));
            OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_PURCHASE_PRICE + 2u));
            OpRepWidths(cpu, 0x20u);
        }
        PullAccumulator16(memory, cpu);
        OpIncA(cpu);
        OpCmpValue(cpu, 6u);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82f636u);
}
