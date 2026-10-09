#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/menu.h"

enum {
    DP_SAVE_PREVIEW = 0x1cu,
    DP_SAVE_PREVIEW_BANK = 0x1eu,
    SAVE_RECORD_UNK_03C7 = WRAM_SAVE_FILE_BUFFER + 0x3c7u,
    SAVE_RECORD_PARTY = WRAM_SAVE_FILE_BUFFER + 0x3cdu,
    SAVE_RECORD_UNK_03F4 = WRAM_SAVE_FILE_BUFFER + 0x3f4u,
    SAVE_RECORD_UNK_03F7 = WRAM_SAVE_FILE_BUFFER + 0x3f7u,
    SAVE_RECORD_UNK_03F8 = WRAM_SAVE_FILE_BUFFER + 0x3f8u,
    ROM_SAVE_PARTY_RECORDS_LONG = 0x82f2c0u,
    SAVE_PREVIEW_COUNT = 4u,
    SAVE_PREVIEW_FIRST = 0xa5f3u,
    SAVE_PREVIEW_STRIDE = 0x12u
};

static Lufia2ExecutionResult SaveMenuChildUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static void CopySavePreviewBytes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t source) {
    do {
        OpLda(memory, cpu, OpLongX(cpu, source));
        OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, DP_SAVE_PREVIEW));
        OpIny(cpu);
        OpDex(cpu);
    } while (!cpu->zero);
}

static void BuildSavePreviewParty(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, DirectLongPointer(memory, cpu, DP_SAVE_PREVIEW));
    OpLdy(cpu, 11u);
    OpLdx(cpu, SAVE_PREVIEW_COUNT);
    do {
        OpLda(memory, cpu, OpLongX(cpu, SAVE_RECORD_PARTY));
        OpCmpValue(cpu, 0xffu);
        if (!cpu->zero) {
            OpRepWidths(cpu, 0x20u);
            OpPushX(memory, cpu);
            OpAndValue(cpu, 0xffu);
            OpAslA(cpu);
            OpTax(cpu);
            OpLda(memory, cpu, OpLongX(cpu, ROM_SAVE_PARTY_RECORDS_LONG));
            OpTax(cpu);
            OpSepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpAbsX(cpu, 0u));
            OpLsrA(cpu);
            OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, DP_SAVE_PREVIEW));
            OpPullX(memory, cpu);
            OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_SAVE_PREVIEW));
            OpIncA(cpu);
            OpSta(memory, cpu, DirectLongPointer(memory, cpu, DP_SAVE_PREVIEW));
        }
        OpIny(cpu);
        OpDex(cpu);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
}

Lufia2ExecutionResult Lufia2SaveBuildMenuSlot(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x82f22bu);
    OpWriteX(memory, cpu, OpDp(cpu, DP_SAVE_PREVIEW), cpu->y);
    PushAccumulator8(memory, cpu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f22eu, 0x80905fu, 3u, cpu->program_bank))
        return SaveMenuChildUnwound(0x82f22eu);
    if (cpu->carry) {
        OpLoadA(cpu, 0x7eu);
        OpSta(memory, cpu, OpDp(cpu, DP_SAVE_PREVIEW_BANK));
        OpLoadA(cpu, 0u);
        OpSta(memory, cpu, DirectLongPointer(memory, cpu, DP_SAVE_PREVIEW));
        OpLoadA(cpu, Pull8(memory, cpu));
        return ExecutionReturned(0x82f23du);
    }
    OpLoadA(cpu, Pull8(memory, cpu));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f23fu, 0x80914bu, 3u, cpu->program_bank))
        return SaveMenuChildUnwound(0x82f23fu);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, DP_SAVE_PREVIEW_BANK));
    OpLdy(cpu, 1u);
    OpLdx(cpu, 5u);
    CopySavePreviewBytes(memory, cpu, SAVE_RECORD_UNK_03C7);
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, DP_SAVE_PREVIEW));
    OpLdy(cpu, 7u);
    OpLdx(cpu, SAVE_PREVIEW_COUNT);
    CopySavePreviewBytes(memory, cpu, SAVE_RECORD_PARTY);
    BuildSavePreviewParty(memory, cpu);
    OpLdy(cpu, 15u);
    OpLda(memory, cpu, SAVE_RECORD_UNK_03F8);
    OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, DP_SAVE_PREVIEW));
    OpLdy(cpu, 16u);
    OpLda(memory, cpu, SAVE_RECORD_UNK_03F7);
    OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, DP_SAVE_PREVIEW));
    OpLdy(cpu, 17u);
    OpLda(memory, cpu, SAVE_RECORD_UNK_03F4);
    OpAndValue(cpu, 3u);
    OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, DP_SAVE_PREVIEW));
    return ExecutionReturned(0x82f2bfu);
}

Lufia2ExecutionResult Lufia2SaveBuildMenuSlots(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x82f20au);
    for (unsigned slot = 0u; slot < SAVE_PREVIEW_COUNT; ++slot) {
        const uint32_t site = 0x82f20fu + slot * 8u;
        OpLoadA(cpu, (uint16_t)slot);
        OpLdy(cpu, (uint16_t)(SAVE_PREVIEW_FIRST + slot * SAVE_PREVIEW_STRIDE));
        if (!CallChildWithFrame(memory, cpu, child, context,
                site, 0x82f22bu, 2u, cpu->program_bank))
            return SaveMenuChildUnwound(site);
    }
    return ExecutionReturned(0x82f22au);
}

Lufia2ExecutionResult Lufia2SaveRestoreMenuSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x82f481u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09C2)));
    for (unsigned step = 0u; step < 5u; ++step)
        OpDex(cpu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09C1));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_MENU_ITEM_COLUMN));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09C0));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_MENU_ITEM_ROW));
    return ExecutionReturned(0x82f495u);
}

Lufia2ExecutionResult Lufia2SavePrepareMenuCursors(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x82eef7u);
    OpLoadA(cpu, 1u);
    OpLdx(cpu, 6u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82eefcu, 0x82895bu, 2u, cpu->program_bank))
        return SaveMenuChildUnwound(0x82eefcu);
    OpLdy(cpu, 0x9au);
    OpLdx(cpu, 1u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82ef05u, 0x82891eu, 2u, cpu->program_bank))
        return SaveMenuChildUnwound(0x82ef05u);
    OpLoadA(cpu, 1u);
    OpLdx(cpu, 7u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82ef0du, 0x82895bu, 2u, cpu->program_bank))
        return SaveMenuChildUnwound(0x82ef0du);
    OpLdy(cpu, 0xa1u);
    OpLdx(cpu, 2u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82ef16u, 0x82891eu, 2u, cpu->program_bank))
        return SaveMenuChildUnwound(0x82ef16u);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_BATTLE_EFFECT_SPECIAL_PARTY));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E11DF));
    OpLoadA(cpu, 0u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82ef21u, 0x82e917u, 2u, cpu->program_bank))
        return SaveMenuChildUnwound(0x82ef21u);
    return ExecutionReturned(0x82ef24u);
}
