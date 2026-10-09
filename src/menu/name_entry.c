#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/menu.h"

enum {
    DP_NAME_UPPER_TILES = 0x08u,
    DP_NAME_LOWER_TILES = 0x0bu,
    DP_NAME_TEXT_BANK = 0x5fu,
    DP_NAME_RECORD = 0x2au,
    NAME_GRID_SLOT = 5u,
    NAME_GRID_ORIGIN = 0x34u,
    NAME_GLYPH_LIMIT = 5u,
    NAME_DOUBLE_TILE_GLYPH = 0xcdu,
    NAME_UPPER_GLYPH_TABLE = 0x808e49u,
    NAME_LOWER_GLYPH_TABLE = 0x808e16u,
    NAME_CHARACTER_TABLE = 0x8ecb9fu
};

static Lufia2ExecutionResult NameEntryChildUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static bool NameEntrySupported(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit && !cpu->decimal;
}

Lufia2ExecutionResult Lufia2MenuPrepareNameEntryUpperText(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !NameEntrySupported(cpu))
        return ExecutionHandoff(cpu, 0x82f0f6u);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x008cu);
    OpLdx(cpu, 0x1402u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f0feu, 0x8283ebu, 2u, cpu->program_bank))
        return NameEntryChildUnwound(0x82f0feu);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, DP_NAME_TEXT_BANK));
    OpLdy(cpu, 0xcaefu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f10fu, 0x808878u, 3u, cpu->program_bank))
        return NameEntryChildUnwound(0x82f10fu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f113u, 0x82f50au, 2u, cpu->program_bank))
        return NameEntryChildUnwound(0x82f113u);
    return ExecutionReturned(0x82f116u);
}

Lufia2ExecutionResult Lufia2MenuPrepareNameEntryLowerText(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !NameEntrySupported(cpu))
        return ExecutionHandoff(cpu, 0x82f117u);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x018cu);
    OpLdx(cpu, 0x1413u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f11fu, 0x8283ebu, 2u, cpu->program_bank))
        return NameEntryChildUnwound(0x82f11fu);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, DP_NAME_TEXT_BANK));
    OpLdy(cpu, 0xcb01u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f130u, 0x808878u, 3u, cpu->program_bank))
        return NameEntryChildUnwound(0x82f130u);
    OpLoadA(cpu, 8u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_MENU_FRAME_FLAGS), true);
    return ExecutionReturned(0x82f138u);
}

Lufia2ExecutionResult Lufia2MenuPrepareNameEntryTilePointers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!NameEntrySupported(cpu))
        return ExecutionHandoff(cpu, 0x82f4f4u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E14AD));
    OpSta(memory, cpu, OpDp(cpu, DP_NAME_LOWER_TILES));
    cpu->carry = true;
    OpSbcValue(cpu, 0x40u);
    OpSta(memory, cpu, OpDp(cpu, DP_NAME_UPPER_TILES));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, DP_NAME_LOWER_TILES + 2u));
    OpSta(memory, cpu, OpDp(cpu, DP_NAME_UPPER_TILES + 2u));
    return ExecutionReturned(0x82f509u);
}

Lufia2ExecutionResult Lufia2MenuAppendNameEntryGlyph(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!NameEntrySupported(cpu))
        return ExecutionHandoff(cpu, 0x82f556u);
    OpRepWidths(cpu, 0x20u);
    OpLdy(cpu, 1u);
    OpAndValue(cpu, 0x00ffu);
    OpSepWidths(cpu, 0x20u);
    OpCmpValue(cpu, NAME_DOUBLE_TILE_GLYPH);
    if (cpu->carry) {
        OpSbcValue(cpu, NAME_DOUBLE_TILE_GLYPH);
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, NAME_UPPER_GLYPH_TABLE));
        OpSta(memory, cpu, DirectLongPointer(memory, cpu, DP_NAME_UPPER_TILES));
        OpLoadA(cpu, 0x20u);
        OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, DP_NAME_UPPER_TILES));
        OpLda(memory, cpu, OpLongX(cpu, NAME_LOWER_GLYPH_TABLE));
    }
    OpSta(memory, cpu, DirectLongPointer(memory, cpu, DP_NAME_LOWER_TILES));
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, DP_NAME_LOWER_TILES));
    OpStepMem(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_COUNT), 1);
    OpStepMem(memory, cpu, OpAbs(cpu, WRAM_UNK_7E14AD), 1);
    OpStepMem(memory, cpu, OpAbs(cpu, WRAM_UNK_7E14AD), 1);
    return ExecutionReturned(0x82f584u);
}

Lufia2ExecutionResult Lufia2MenuDrawNameEntryRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !NameEntrySupported(cpu))
        return ExecutionHandoff(cpu, 0x82f50au);
    OpLdx(cpu, Read16Direct(memory, cpu, DP_NAME_RECORD));
    OpLdy(cpu, NAME_GLYPH_LIMIT);
    do {
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82f50fu, 0x82f4f4u, 2u, cpu->program_bank))
            return NameEntryChildUnwound(0x82f50fu);
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        if (cpu->zero)
            return ExecutionReturned(0x82f522u);
        OpPushX(memory, cpu);
        PushY(memory, cpu);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82f519u, 0x82f556u, 2u, cpu->program_bank))
            return NameEntryChildUnwound(0x82f519u);
        OpPullY(memory, cpu);
        OpPullX(memory, cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    return ExecutionReturned(0x82f522u);
}

Lufia2ExecutionResult Lufia2MenuInsertNameEntryCharacter(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !NameEntrySupported(cpu))
        return ExecutionHandoff(cpu, 0x82f523u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_COUNT));
    OpCmpValue(cpu, NAME_GLYPH_LIMIT);
    if (cpu->zero)
        return ExecutionReturned(0x82f555u);
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_INDEX)));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_ITEM_ORIGIN_X_LOW + NAME_GRID_SLOT));
    OpAndValue(cpu, 0x00ffu);
    OpCmpValue(cpu, NAME_GRID_ORIGIN);
    if (!cpu->zero) {
        OpTxa(cpu);
        cpu->carry = false;
        OpAdcValue(cpu, 0x23u);
        OpTax(cpu);
    }
    OpSepWidths(cpu, 0x20u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f542u, 0x82f4f4u, 2u, cpu->program_bank))
        return NameEntryChildUnwound(0x82f542u);
    OpLda(memory, cpu, OpLongX(cpu, NAME_CHARACTER_TABLE));
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_COUNT)));
    OpSta(memory, cpu, AbsoluteIndexedAddress(cpu,
        Read16Direct(memory, cpu, DP_NAME_RECORD), cpu->y));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f54eu, 0x82f556u, 2u, cpu->program_bank))
        return NameEntryChildUnwound(0x82f54eu);
    OpLoadA(cpu, 8u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_MENU_FRAME_FLAGS), true);
    return ExecutionReturned(0x82f555u);
}

Lufia2ExecutionResult Lufia2MenuRemoveNameEntryCharacter(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !NameEntrySupported(cpu))
        return ExecutionHandoff(cpu, 0x82f585u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_COUNT));
    if (cpu->zero) {
        cpu->carry = true;
        return ExecutionReturned(0x82f5aeu);
    }
    OpStepMem(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_COUNT), -1);
    OpStepMem(memory, cpu, OpAbs(cpu, WRAM_UNK_7E14AD), -1);
    OpStepMem(memory, cpu, OpAbs(cpu, WRAM_UNK_7E14AD), -1);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f593u, 0x82f4f4u, 2u, cpu->program_bank))
        return NameEntryChildUnwound(0x82f593u);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_COUNT)));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, AbsoluteIndexedAddress(cpu,
        Read16Direct(memory, cpu, DP_NAME_RECORD), cpu->y));
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x205fu);
    OpSta(memory, cpu, DirectLongPointer(memory, cpu, DP_NAME_LOWER_TILES));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, DirectLongPointer(memory, cpu, DP_NAME_UPPER_TILES));
    OpSepWidths(cpu, 0x20u);
    cpu->carry = false;
    return ExecutionReturned(0x82f5acu);
}

Lufia2ExecutionResult Lufia2MenuClearNameEntryRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !NameEntrySupported(cpu))
        return ExecutionHandoff(cpu, 0x82f5afu);
    do {
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82f5afu, 0x82f585u, 2u, cpu->program_bank))
            return NameEntryChildUnwound(0x82f5afu);
    } while (!cpu->carry);
    return ExecutionReturned(0x82f5b4u);
}
