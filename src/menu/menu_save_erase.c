#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/title.h"

enum {
    SAVE_HEADER = 0x700000u,
    SAVE_RAM_SIZE = 0x2000u,
    DP_SAVE_TEXT_BANK = 0x5fu,
};

static bool SaveMenuSupported(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit && !cpu->decimal;
}

static Lufia2ExecutionResult SaveMenuUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

#define SAVE_MENU_CALL(site, target, frame) \
    do { \
        if (!CallChildWithFrame(memory, cpu, child, context, \
                site, target, frame, cpu->program_bank)) \
            return SaveMenuUnwound(site); \
    } while (0)

static Lufia2ExecutionResult SaveMenuWriteSelectedFile(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    SAVE_MENU_CALL(0x82eb55u, 0x82efc5u, 2u);
    OpLdx(cpu, 2u);
    SAVE_MENU_CALL(0x82eb5bu, 0x8288a0u, 2u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_INDEX));
    SAVE_MENU_CALL(0x82eb61u, 0x8090c9u, 3u);
    OpLda(memory, cpu, SAVE_HEADER);
    OpAndValue(cpu, 0xfcu);
    OpOra(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_INDEX));
    OpSta(memory, cpu, SAVE_HEADER);
    OpLoadA(cpu, 0x7au);
    SAVE_MENU_CALL(0x82eb74u, 0x80953bu, 3u);
    OpLoadA(cpu, 0x88u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_MENU_FRAME_FLAGS), true);
    OpLoadA(cpu, 0x40u);
    SAVE_MENU_CALL(0x82eb7eu, 0x829330u, 2u);
    SAVE_MENU_CALL(0x82eb81u, 0x868b32u, 3u);
    return ExecutionReturned(0u);
}

Lufia2ExecutionResult Lufia2SaveRunSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SaveMenuSupported(cpu))
        return ExecutionHandoff(cpu, 0x82eaf4u);
    SAVE_MENU_CALL(0x82eaf4u, 0x82f20au, 2u);
    SAVE_MENU_CALL(0x82eaf7u, 0x82e998u, 2u);
    SAVE_MENU_CALL(0x82eafau, 0x82f481u, 2u);
    SAVE_MENU_CALL(0x82eafdu, 0x82eef7u, 2u);
    OpLdx(cpu, 7u);
    SAVE_MENU_CALL(0x82eb03u, 0x82ef25u, 2u);
    for (;;) {
        OpLoadA(cpu, 0u);
        SAVE_MENU_CALL(0x82eb08u, 0x828b08u, 2u);
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            OpLoadA(cpu, 5u);
            OpLdx(cpu, 7u);
            SAVE_MENU_CALL(0x82eb2au, 0x82895bu, 2u);
            SAVE_MENU_CALL(0x82eb2du, 0x868b55u, 3u);
            OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_INDEX));
            SAVE_MENU_CALL(0x82eb34u, 0x80905fu, 3u);
            if (!cpu->carry) {
                OpLoadA(cpu, 2u);
                SAVE_MENU_CALL(0x82eb3cu, 0x80953bu, 3u);
                SAVE_MENU_CALL(0x82eb40u, 0x82eff6u, 2u);
                SAVE_MENU_CALL(0x82eb43u, 0x82eb9au, 2u);
                if (cpu->carry) {
                    OpLoadA(cpu, 0u);
                    SAVE_MENU_CALL(0x82eb4au, 0x82e917u, 2u);
                    OpLdx(cpu, 7u);
                    SAVE_MENU_CALL(0x82eb50u, 0x82ef25u, 2u);
                    continue;
                }
            }
            Lufia2ExecutionResult result = SaveMenuWriteSelectedFile(
                memory, cpu, child, context);
            if (result.flow == LUFIA2_EXECUTION_CHILD_UNWOUND)
                return result;
            break;
        }
        OpCmpValue(cpu, 3u);
        if (cpu->zero) {
            SAVE_MENU_CALL(0x82eb87u, 0x868b32u, 3u);
            break;
        }
        OpCmpValue(cpu, 1u);
        if (cpu->zero) {
            SAVE_MENU_CALL(0x82eb91u, 0x82f3ddu, 2u);
            SAVE_MENU_CALL(0x82eb94u, 0x82f3fcu, 2u);
        }
    }
    SAVE_MENU_CALL(0x82eb8du, 0x82838fu, 2u);
    return ExecutionReturned(0x82eb90u);
}

Lufia2ExecutionResult Lufia2SaveDrawErasePrompt(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SaveMenuSupported(cpu))
        return ExecutionHandoff(cpu, 0x82eec2u);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x1c6u);
    OpLdx(cpu, 0x1a03u);
    SAVE_MENU_CALL(0x82eecau, 0x8283ebu, 2u);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, DP_SAVE_TEXT_BANK));
    OpLdy(cpu, 0xd5cau);
    SAVE_MENU_CALL(0x82eedbu, 0x808878u, 3u);
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, DP_SAVE_TEXT_BANK));
    OpLdy(cpu, 0xca3fu);
    OpLdx(cpu, 0x335eu);
    SAVE_MENU_CALL(0x82eee9u, 0x808878u, 3u);
    OpLoadA(cpu, 0x88u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_MENU_FRAME_FLAGS), true);
    return ExecutionReturned(0x82eef1u);
}

Lufia2ExecutionResult Lufia2SaveRunEraseSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SaveMenuSupported(cpu))
        return ExecutionHandoff(cpu, 0x82ee28u);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x1c6u);
    OpLdx(cpu, 0x1a03u);
    SAVE_MENU_CALL(0x82ee30u, 0x82810eu, 2u);
    OpLoadA(cpu, 0x318u);
    OpLdx(cpu, 0x905u);
    SAVE_MENU_CALL(0x82ee39u, 0x82810eu, 2u);
    OpSepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_SELECTION));
    SAVE_MENU_CALL(0x82ee41u, 0x82eec2u, 2u);
    OpLoadA(cpu, 1u);
    OpLdx(cpu, 6u);
    SAVE_MENU_CALL(0x82ee49u, 0x82895bu, 2u);
    OpLdy(cpu, 0x69u);
    OpLdx(cpu, 1u);
    SAVE_MENU_CALL(0x82ee52u, 0x82891eu, 2u);
    OpLoadA(cpu, 1u);
    SAVE_MENU_CALL(0x82ee57u, 0x8293f6u, 2u);
    SAVE_MENU_CALL(0x82ee5au, 0x828704u, 2u);
    for (;;) {
        SAVE_MENU_CALL(0x82ee5du, 0x828b08u, 2u);
        OpCmpValue(cpu, 2u);
        if (!cpu->zero) {
            OpCmpValue(cpu, 3u);
            if (cpu->zero) break;
            OpLoadA(cpu, 0u);
            continue;
        }
        OpLoadA(cpu, 2u);
        SAVE_MENU_CALL(0x82ee6eu, 0x80953bu, 3u);
        OpLoadA(cpu, 5u);
        OpLdx(cpu, 6u);
        SAVE_MENU_CALL(0x82ee77u, 0x82895bu, 2u);
        SAVE_MENU_CALL(0x82ee7au, 0x868b55u, 3u);
        OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_INDEX));
        if (!cpu->zero) break;
        OpStepMem(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_SELECTION), 1);
        OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_SELECTION));
        OpCmpValue(cpu, 2u);
        if (!cpu->zero) {
            SAVE_MENU_CALL(0x82ee8du, 0x82eec2u, 2u);
            OpLoadA(cpu, 3u);
            OpLdx(cpu, 6u);
            SAVE_MENU_CALL(0x82ee95u, 0x82895bu, 2u);
            OpLoadA(cpu, 0u);
            continue;
        }
        OpLoadA(cpu, 0u);
        OpLdx(cpu, 0u);
        do {
            OpSta(memory, cpu, LongIndexedAddress(SAVE_HEADER, cpu->x));
            OpInx(cpu);
            OpCpx(cpu, SAVE_RAM_SIZE);
        } while (!cpu->zero);
        SAVE_MENU_CALL(0x82eea9u, 0x809037u, 3u);
        SAVE_MENU_CALL(0x82eeadu, 0x82eec2u, 2u);
        OpLdx(cpu, 0x8080u);
        OpWriteX(memory, cpu, OpAbs(cpu, WRAM_UNK_7E156C), cpu->x);
        OpWriteX(memory, cpu, OpAbs(cpu, WRAM_UNK_7E156E), cpu->x);
        SAVE_MENU_CALL(0x82eeb9u, 0x82935du, 3u);
        break;
    }
    SAVE_MENU_CALL(0x82eebdu, 0x868b32u, 3u);
    return ExecutionReturned(0x82eec1u);
}

#undef SAVE_MENU_CALL
