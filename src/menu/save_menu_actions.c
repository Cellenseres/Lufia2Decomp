#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/menu.h"

enum {
    DP_SAVE_MEMBER = 0x22u,
    DP_SAVE_RECORD = 0x2au,
    DP_SAVE_RESULT = 0x31u,
    DP_SAVE_CURSOR_LAYOUT = 0x54u,
    DP_SAVE_CURSOR_KIND = 0x56u,
    DP_SAVE_TEXT_BANK = 0x5fu,
    DP_SAVE_FRAME_FLAGS = 0x74u,
    SAVE_LIST_MATCH_BYTE = 0x10u,
    SAVE_TEXT_BANK = 0x8eu,
    SAVE_ALTERNATE_TEXT = 0xd423u,
    SAVE_ALTERNATE_ACTION_TEXT = 0xd4deu,
    SAVE_ALTERNATE_LABEL_TEXT = 0xca3fu,
    SAVE_ALTERNATE_LABEL_DESTINATION = 0x356eu,
    SAVE_ALTERNATE_ACTION_DESTINATION = 0x3546u
};

static Lufia2ExecutionResult SaveActionChildUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static void PrepareSaveTextBank(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    OpLoadA(cpu, SAVE_TEXT_BANK);
    OpSta(memory, cpu, OpDp(cpu, DP_SAVE_TEXT_BANK));
}

Lufia2ExecutionResult Lufia2SaveTestRecordList(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x82ecafu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_COUNT));
    if (!cpu->zero) {
        OpLdy(cpu, 0u);
        do {
            const uint16_t pointer = Read16Direct(memory, cpu, DP_SAVE_RECORD);
            OpLda(memory, cpu, AbsoluteIndexedAddress(cpu, pointer, cpu->y));
            OpCmpValue(cpu, SAVE_LIST_MATCH_BYTE);
            if (!cpu->zero) {
                cpu->carry = false;
                return ExecutionReturned(0x82ecc6u);
            }
            OpIny(cpu);
            OpCpy(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_COUNT)));
        } while (!cpu->zero);
    }
    cpu->carry = true;
    return ExecutionReturned(0x82ecc4u);
}

Lufia2ExecutionResult Lufia2SaveResetSlotCursor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x82efb5u);
    OpLdx(cpu, 0xffffu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82efb8u, 0x829214u, 3u, cpu->program_bank))
        return SaveActionChildUnwound(0x82efb8u);
    OpLoadA(cpu, 1u);
    OpLdx(cpu, 7u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82efc1u, 0x82895bu, 2u, cpu->program_bank))
        return SaveActionChildUnwound(0x82efc1u);
    return ExecutionReturned(0x82efc4u);
}

Lufia2ExecutionResult Lufia2SaveSelectAlternatePartyMember(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x82ecf3u);
    for (;;) {
        OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E14BF));
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82ecf6u, 0x829246u, 2u, cpu->program_bank))
            return SaveActionChildUnwound(0x82ecf6u);
        if (cpu->carry)
            return ExecutionReturned(0x82ecfbu);
        OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_INDEX)));
        OpWriteX(memory, cpu, OpDp(cpu, DP_SAVE_MEMBER), cpu->x);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82ed01u, 0x829971u, 2u, cpu->program_bank))
            return SaveActionChildUnwound(0x82ed01u);
        OpLda(memory, cpu, OpDp(cpu, DP_SAVE_RESULT));
        if (cpu->zero) {
            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SAVE_RECORD)));
            OpLda(memory, cpu, OpAbsX(cpu, 15u));
            OpBitValue(cpu, 5u);
        } else {
            OpRepWidths(cpu, 0x20u);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ed15u, 0x82f87du, 2u, cpu->program_bank))
                return SaveActionChildUnwound(0x82ed15u);
            OpSepWidths(cpu, 0x20u);
            OpCmpValue(cpu, 0u);
        }
        if (cpu->zero) {
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ed20u, 0x8297e3u, 2u, cpu->program_bank))
                return SaveActionChildUnwound(0x82ed20u);
            continue;
        }
        OpLoadA(cpu, 2u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82ed27u, 0x80953bu, 3u, cpu->program_bank))
            return SaveActionChildUnwound(0x82ed27u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82ed2bu, 0x829984u, 2u, cpu->program_bank))
            return SaveActionChildUnwound(0x82ed2bu);
        OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_ITEM_COLUMN + 2u));
        OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_ITEM_ROW + 2u));
        OpLdx(cpu, 0x00c4u);
        OpWriteX(memory, cpu, OpDp(cpu, DP_SAVE_CURSOR_LAYOUT), cpu->x);
        OpLdx(cpu, 1u);
        OpWriteX(memory, cpu, OpDp(cpu, DP_SAVE_CURSOR_KIND), cpu->x);
        OpLdx(cpu, 6u);
        OpLdy(cpu, 7u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82ed44u, 0x8289ecu, 2u, cpu->program_bank))
            return SaveActionChildUnwound(0x82ed44u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82ed47u, 0x82f5b5u, 2u, cpu->program_bank))
            return SaveActionChildUnwound(0x82ed47u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82ed4au, 0x82ed50u, 2u, cpu->program_bank))
            return SaveActionChildUnwound(0x82ed4au);
    }
}

Lufia2ExecutionResult Lufia2SavePrepareAlternateDisplay(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x82f139u);
    OpLdx(cpu, 0xffffu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f13cu, 0x829214u, 3u, cpu->program_bank))
        return SaveActionChildUnwound(0x82f13cu);
    OpLdx(cpu, 0xffffu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f143u, 0x82922du, 3u, cpu->program_bank))
        return SaveActionChildUnwound(0x82f143u);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x0382u);
    OpLdx(cpu, 0x1e0au);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f14fu, 0x8283b5u, 2u, cpu->program_bank))
        return SaveActionChildUnwound(0x82f14fu);
    OpLoadA(cpu, 0x0468u);
    OpLdx(cpu, 0x0a07u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f158u, 0x82810eu, 2u, cpu->program_bank))
        return SaveActionChildUnwound(0x82f158u);
    OpSepWidths(cpu, 0x20u);
    PrepareSaveTextBank(memory, cpu);
    OpLdy(cpu, SAVE_ALTERNATE_TEXT);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f169u, 0x808878u, 3u, cpu->program_bank))
        return SaveActionChildUnwound(0x82f169u);
    OpLoadA(cpu, 1u);
    OpLdx(cpu, 5u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f172u, 0x82895bu, 2u, cpu->program_bank))
        return SaveActionChildUnwound(0x82f172u);
    OpLdy(cpu, 0xbdu);
    OpLdx(cpu, 0u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f17bu, 0x82891eu, 2u, cpu->program_bank))
        return SaveActionChildUnwound(0x82f17bu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f17eu, 0x82999bu, 2u, cpu->program_bank))
        return SaveActionChildUnwound(0x82f17eu);
    OpLoadA(cpu, 1u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f183u, 0x82a318u, 2u, cpu->program_bank))
        return SaveActionChildUnwound(0x82f183u);
    OpLoadA(cpu, 8u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f188u, 0x8293f6u, 2u, cpu->program_bank))
        return SaveActionChildUnwound(0x82f188u);
    OpLoadA(cpu, 0x88u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_SAVE_FRAME_FLAGS), true);
    return ExecutionReturned(0x82f18fu);
}

Lufia2ExecutionResult Lufia2SavePrepareAlternateActionDisplay(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x82f190u);
    OpLdx(cpu, 14u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_SAVE_CURSOR_LAYOUT), cpu->x);
    OpLdx(cpu, 0x0406u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_SAVE_CURSOR_KIND), cpu->x);
    OpLdx(cpu, 5u);
    OpLdy(cpu, 6u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f1a0u, 0x8289ecu, 2u, cpu->program_bank))
        return SaveActionChildUnwound(0x82f1a0u);
    OpLdx(cpu, 0xffb0u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f1a6u, 0x829214u, 3u, cpu->program_bank))
        return SaveActionChildUnwound(0x82f1a6u);
    OpLdx(cpu, 0xffffu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f1adu, 0x82922du, 3u, cpu->program_bank))
        return SaveActionChildUnwound(0x82f1adu);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x0468u);
    OpLdx(cpu, 0x0a07u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f1b9u, 0x8283b5u, 2u, cpu->program_bank))
        return SaveActionChildUnwound(0x82f1b9u);
    static const uint16_t positions[] = {0x0382u, 0x04c2u, 0x04e4u};
    static const uint16_t dimensions[] = {0x1e05u, 0x1105u, 0x0d05u};
    for (unsigned window = 0u; window < 3u; ++window) {
        const uint32_t site = 0x82f1c2u + window * 9u;
        OpLoadA(cpu, positions[window]);
        OpLdx(cpu, dimensions[window]);
        if (!CallChildWithFrame(memory, cpu, child, context,
                site, 0x82810eu, 2u, cpu->program_bank))
            return SaveActionChildUnwound(site);
    }
    OpSepWidths(cpu, 0x20u);
    PrepareSaveTextBank(memory, cpu);
    OpLdy(cpu, SAVE_ALTERNATE_LABEL_TEXT);
    OpLdx(cpu, SAVE_ALTERNATE_LABEL_DESTINATION);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f1e8u, 0x808878u, 3u, cpu->program_bank))
        return SaveActionChildUnwound(0x82f1e8u);
    OpLoadA(cpu, SAVE_TEXT_BANK);
    OpSta(memory, cpu, OpDp(cpu, DP_SAVE_TEXT_BANK));
    OpLdy(cpu, SAVE_ALTERNATE_ACTION_TEXT);
    OpLdx(cpu, SAVE_ALTERNATE_ACTION_DESTINATION);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f1f6u, 0x808878u, 3u, cpu->program_bank))
        return SaveActionChildUnwound(0x82f1f6u);
    OpLdy(cpu, 0u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f1fdu, 0x82f637u, 2u, cpu->program_bank))
        return SaveActionChildUnwound(0x82f1fdu);
    OpLoadA(cpu, 9u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f202u, 0x8293f6u, 2u, cpu->program_bank))
        return SaveActionChildUnwound(0x82f202u);
    OpLoadA(cpu, 0x88u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_SAVE_FRAME_FLAGS), true);
    return ExecutionReturned(0x82f209u);
}
