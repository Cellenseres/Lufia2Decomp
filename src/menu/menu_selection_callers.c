#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/menu.h"
#include "lufia2/title.h"

enum {
    DP_SELECTION_MODE = 0x30u,
    DP_SELECTION_CALLBACK = 0x68u,
    DP_SELECTION_CALLBACK_BANK = 0x6au,
    DP_SELECTION_PENDING = 0x71u,
    ROM_SELECTION_CALLBACK = 0x82939cu
};

static bool SelectionCallerSupported(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit && !cpu->decimal;
}

static Lufia2ExecutionResult SelectionCallerUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static void SelectionCallerFinish(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLoadA(cpu, 0x80u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_PLAY_TIME_FRAMES), false);
}

Lufia2ExecutionResult Lufia2TitleOpenSaveSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SelectionCallerSupported(cpu))
        return ExecutionHandoff(cpu, 0x82e75du);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e75du, 0x82e8deu, 2u, cpu->program_bank))
        return SelectionCallerUnwound(0x82e75du);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e760u, 0x8ee6eau, 3u, cpu->program_bank))
        return SelectionCallerUnwound(0x82e760u);
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTION_REFERENCE), cpu->x);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e76au, 0x828069u, 2u, cpu->program_bank))
        return SelectionCallerUnwound(0x82e76au);
    OpWriteM(memory, cpu, OpAbs(cpu, WRAM_UNK_7E155A), 0u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e770u, 0x82e998u, 2u, cpu->program_bank))
        return SelectionCallerUnwound(0x82e770u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e773u, 0x82e8b2u, 2u, cpu->program_bank))
        return SelectionCallerUnwound(0x82e773u);
    SelectionCallerFinish(memory, cpu);
    return ExecutionReturned(0x82e77bu);
}

Lufia2ExecutionResult Lufia2TitleRestoreSaveSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SelectionCallerSupported(cpu))
        return ExecutionHandoff(cpu, 0x82e77cu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e77cu, 0x82e8b2u, 2u, cpu->program_bank))
        return SelectionCallerUnwound(0x82e77cu);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, DP_SELECTION_PENDING));
    OpLdx(cpu, ROM_SELECTION_CALLBACK & 0xffffu);
    OpWriteX(memory, cpu, OpDp(cpu, DP_SELECTION_CALLBACK), cpu->x);
    OpLoadA(cpu, ROM_SELECTION_CALLBACK >> 16);
    OpSta(memory, cpu, OpDp(cpu, DP_SELECTION_CALLBACK_BANK));
    return ExecutionReturned(0x82e78cu);
}

Lufia2ExecutionResult Lufia2TitleOpenAlternateSaveSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SelectionCallerSupported(cpu))
        return ExecutionHandoff(cpu, 0x82e78du);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e78du, 0x82e8deu, 2u, cpu->program_bank))
        return SelectionCallerUnwound(0x82e78du);
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTION_REFERENCE), cpu->x);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e796u, 0x828069u, 2u, cpu->program_bank))
        return SelectionCallerUnwound(0x82e796u);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E155A));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e79eu, 0x82e998u, 2u, cpu->program_bank))
        return SelectionCallerUnwound(0x82e79eu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e7a1u, 0x82e8b2u, 2u, cpu->program_bank))
        return SelectionCallerUnwound(0x82e7a1u);
    SelectionCallerFinish(memory, cpu);
    return ExecutionReturned(0x82e7a9u);
}

Lufia2ExecutionResult Lufia2MenuOpenAlternateSaveSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SelectionCallerSupported(cpu))
        return ExecutionHandoff(cpu, 0x82e861u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e861u, 0x82e8deu, 2u, cpu->program_bank))
        return SelectionCallerUnwound(0x82e861u);
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTION_REFERENCE), cpu->x);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e86au, 0x828069u, 2u, cpu->program_bank))
        return SelectionCallerUnwound(0x82e86au);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E155A));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e872u, 0x82e8b2u, 2u, cpu->program_bank))
        return SelectionCallerUnwound(0x82e872u);
    SelectionCallerFinish(memory, cpu);
    return ExecutionReturned(0x82e87au);
}

Lufia2ExecutionResult Lufia2MenuOpenClearedSaveSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SelectionCallerSupported(cpu))
        return ExecutionHandoff(cpu, 0x82e87bu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e87bu, 0x82e8deu, 2u, cpu->program_bank))
        return SelectionCallerUnwound(0x82e87bu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e87eu, 0x82838fu, 2u, cpu->program_bank))
        return SelectionCallerUnwound(0x82e87eu);
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTION_REFERENCE), cpu->x);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e887u, 0x828069u, 2u, cpu->program_bank))
        return SelectionCallerUnwound(0x82e887u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e88au, 0x82e8b2u, 2u, cpu->program_bank))
        return SelectionCallerUnwound(0x82e88au);
    SelectionCallerFinish(memory, cpu);
    return ExecutionReturned(0x82e892u);
}

Lufia2ExecutionResult Lufia2MenuRunSaveSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SelectionCallerSupported(cpu))
        return ExecutionHandoff(cpu, 0x82e8b2u);
    OpLda(memory, cpu, OpDp(cpu, DP_SELECTION_MODE));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E1558));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e8b7u, 0x82f660u, 2u, cpu->program_bank))
        return SelectionCallerUnwound(0x82e8b7u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E1558));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e8bdu, 0x82e8cbu, 2u, cpu->program_bank))
        return SelectionCallerUnwound(0x82e8bdu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e8c0u, 0x82838fu, 2u, cpu->program_bank))
        return SelectionCallerUnwound(0x82e8c0u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e8c3u, 0x8293c2u, 2u, cpu->program_bank))
        return SelectionCallerUnwound(0x82e8c3u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e8c6u, 0x868dbbu, 3u, cpu->program_bank))
        return SelectionCallerUnwound(0x82e8c6u);
    return ExecutionReturned(0x82e8cau);
}
