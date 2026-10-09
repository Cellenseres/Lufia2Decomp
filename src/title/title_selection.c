#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/title.h"

enum {
    DP_SELECTION_DESCRIPTION = 0x5du,
    DP_SELECTION_DESCRIPTION_BANK = 0x5fu,
    DP_SELECTION_PENDING = 0x71u,
    SRAM_SELECTION_FLAGS_LONG = 0x700000u,
    ROM_SELECTION_DESCRIPTION_LONG = 0x8ed8e3u
};

static Lufia2ExecutionResult SelectionChildUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2TitlePrepareSaveSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x82e8deu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e8deu, 0x868dbbu, 3u, cpu->program_bank))
        return SelectionChildUnwound(0x82e8deu);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, DP_SELECTION_PENDING));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e8e6u, 0x868dd7u, 3u, cpu->program_bank))
        return SelectionChildUnwound(0x82e8e6u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e8eau, 0x868e6bu, 3u, cpu->program_bank))
        return SelectionChildUnwound(0x82e8eau);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e8eeu, 0x868eb0u, 3u, cpu->program_bank))
        return SelectionChildUnwound(0x82e8eeu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e8f2u, 0x868f1bu, 3u, cpu->program_bank))
        return SelectionChildUnwound(0x82e8f2u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e8f6u, 0x868f45u, 3u, cpu->program_bank))
        return SelectionChildUnwound(0x82e8f6u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e8fau, 0x8690c0u, 3u, cpu->program_bank))
        return SelectionChildUnwound(0x82e8fau);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e8feu, 0x8690e6u, 3u, cpu->program_bank))
        return SelectionChildUnwound(0x82e8feu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e902u, 0x8690f9u, 3u, cpu->program_bank))
        return SelectionChildUnwound(0x82e902u);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, DP_NMI_UPLOAD_FLAGS));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e90au, 0x8289a4u, 2u, cpu->program_bank))
        return SelectionChildUnwound(0x82e90au);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e90du, 0x868e79u, 3u, cpu->program_bank))
        return SelectionChildUnwound(0x82e90du);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E156B));
    return ExecutionReturned(0x82e916u);
}

Lufia2ExecutionResult Lufia2TitlePrepareSelectionMode(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x82e917u);
    OpCmpValue(cpu, 0u);
    if (cpu->zero) {
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82e91bu, 0x869022u, 3u, cpu->program_bank))
            return SelectionChildUnwound(0x82e91bu);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82e91fu, 0x86916cu, 3u, cpu->program_bank))
            return SelectionChildUnwound(0x82e91fu);
    } else {
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82e925u, 0x868f6fu, 3u, cpu->program_bank))
            return SelectionChildUnwound(0x82e925u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82e929u, 0x86911fu, 3u, cpu->program_bank))
            return SelectionChildUnwound(0x82e929u);
        OpLoadA(cpu, ROM_SELECTION_DESCRIPTION_LONG >> 16);
        OpSta(memory, cpu, OpDp(cpu, DP_SELECTION_DESCRIPTION_BANK));
        OpLdx(cpu, ROM_SELECTION_DESCRIPTION_LONG & 0xffffu);
        OpWriteX(memory, cpu, OpDp(cpu, DP_SELECTION_DESCRIPTION), cpu->x);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82e936u, 0x8293cfu, 2u, cpu->program_bank))
            return SelectionChildUnwound(0x82e936u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82e939u, 0x82999bu, 2u, cpu->program_bank))
            return SelectionChildUnwound(0x82e939u);
    }
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, DP_NMI_UPLOAD_FLAGS));
    return ExecutionReturned(0x82e940u);
}

Lufia2ExecutionResult Lufia2TitleLoadSaveSelectionBits(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x82e998u);
    OpLda(memory, cpu, SRAM_SELECTION_FLAGS_LONG);
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpAndValue(cpu, 3u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E1559));
    OpLda(memory, cpu, SRAM_SELECTION_FLAGS_LONG);
    OpAndValue(cpu, 3u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e9a9u, 0x80905fu, 3u, cpu->program_bank))
        return SelectionChildUnwound(0x82e9a9u);
    if (cpu->carry) {
        OpLdx(cpu, 6u);
        OpWriteX(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09C2), cpu->x);
        OpLdx(cpu, 0u);
        OpWriteX(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09C0), cpu->x);
        return ExecutionReturned(0x82e9bbu);
    }
    OpLdx(cpu, 7u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09C2), cpu->x);
    OpLda(memory, cpu, SRAM_SELECTION_FLAGS_LONG);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09C1));
    OpLsrA(cpu);
    OpAndValue(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09C0));
    OpLoadA(cpu, 0xfeu);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09C1), false);
    return ExecutionReturned(0x82e9d4u);
}
