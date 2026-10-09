#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/party.h"
#include "lufia2/menu.h"

enum {
    DP_MENU_SELECTION_VALUE = 0x31u,
    DP_MENU_STAT_SOURCE = 0x2au,
    ROM_CAPSULE_INITIAL_LEVELS = 0x8ee4c4u
};

static bool MenuInventoryCapsuleSupported(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit && !cpu->decimal;
}

static Lufia2ExecutionResult MenuInventoryCapsuleUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2CapsuleOpenSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !MenuInventoryCapsuleSupported(cpu))
        return ExecutionHandoff(cpu, 0x82e7aau);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e7aau, 0x82e8deu, 2u, cpu->program_bank))
        return MenuInventoryCapsuleUnwound(0x82e7aau);
    OpLdx(cpu, 0x10u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTION_REFERENCE), cpu->x);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e7b3u, 0x828069u, 2u, cpu->program_bank))
        return MenuInventoryCapsuleUnwound(0x82e7b3u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_SELECTED_ID));
    PushAccumulator8(memory, cpu);
    OpLoadA(cpu, 0u);
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpDp(cpu, DP_MENU_SELECTION_VALUE));
    OpTax(cpu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_SELECTED_ID));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_CAPSULE_FORM_STATES));
    OpLda(memory, cpu, OpLongX(cpu, ROM_CAPSULE_INITIAL_LEVELS));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_CAPSULE_SAVED_LEVELS));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e7d0u, 0x82c3f8u, 2u, cpu->program_bank))
        return MenuInventoryCapsuleUnwound(0x82e7d0u);
    OpLdx(cpu, WRAM_CAPSULE_WORK_STATS);
    OpWriteX(memory, cpu, OpDp(cpu, DP_MENU_STAT_SOURCE), cpu->x);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e7d8u, 0x82e8b2u, 2u, cpu->program_bank))
        return MenuInventoryCapsuleUnwound(0x82e7d8u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e7dbu, 0x82ce23u, 2u, cpu->program_bank))
        return MenuInventoryCapsuleUnwound(0x82e7dbu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e7deu, 0x82c443u, 2u, cpu->program_bank))
        return MenuInventoryCapsuleUnwound(0x82e7deu);
    OpLoadA(cpu, 0x80u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_PLAY_TIME_FRAMES), false);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E0A7F));
    OpCmpValue(cpu, 7u);
    if (!cpu->zero) {
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_SELECTED_FORM));
        OpLoadA(cpu, 7u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E0A7F));
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82e7f7u, 0x82c515u, 3u, cpu->program_bank))
            return MenuInventoryCapsuleUnwound(0x82e7f7u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82e7fbu, 0x82c261u, 3u, cpu->program_bank))
            return MenuInventoryCapsuleUnwound(0x82e7fbu);
        LoadA8(cpu, Pull8(memory, cpu));
        return ExecutionReturned(0x82e800u);
    }
    LoadA8(cpu, Pull8(memory, cpu));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_SELECTED_ID));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e805u, 0x82c3f8u, 2u, cpu->program_bank))
        return MenuInventoryCapsuleUnwound(0x82e805u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e808u, 0x82ce52u, 2u, cpu->program_bank))
        return MenuInventoryCapsuleUnwound(0x82e808u);
    return ExecutionReturned(0x82e80bu);
}

Lufia2ExecutionResult Lufia2MenuOpenItemSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !MenuInventoryCapsuleSupported(cpu))
        return ExecutionHandoff(cpu, 0x82e80cu);
    OpRepWidths(cpu, 0x20u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82e80eu, 0x82f9f2u, 2u, cpu->program_bank))
        return MenuInventoryCapsuleUnwound(0x82e80eu);
    if (cpu->carry) {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_ITEM_QUANTITY));
        OpCmpValue(cpu, 2u);
        if (!cpu->carry) {
            OpSepWidths(cpu, 0x20u);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82e81du, 0x868e6bu, 3u, cpu->program_bank))
                return MenuInventoryCapsuleUnwound(0x82e81du);
            OpLda(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
            OpAndValue(cpu, 0x0fu);
            if (!cpu->zero) {
                if (!CallChildWithFrame(memory, cpu, child, context,
                        0x82e828u, 0x868b32u, 3u, cpu->program_bank))
                    return MenuInventoryCapsuleUnwound(0x82e828u);
            }
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82e82cu, 0x82e8deu, 2u, cpu->program_bank))
                return MenuInventoryCapsuleUnwound(0x82e82cu);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82e82fu, 0x868ef1u, 3u, cpu->program_bank))
                return MenuInventoryCapsuleUnwound(0x82e82fu);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82e833u, 0x868f6fu, 3u, cpu->program_bank))
                return MenuInventoryCapsuleUnwound(0x82e833u);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82e837u, 0x8690d3u, 3u, cpu->program_bank))
                return MenuInventoryCapsuleUnwound(0x82e837u);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82e83bu, 0x86911fu, 3u, cpu->program_bank))
                return MenuInventoryCapsuleUnwound(0x82e83bu);
            OpLoadA(cpu, 0x80u);
            OpSta(memory, cpu, OpDp(cpu, DP_NMI_UPLOAD_FLAGS));
            OpLdx(cpu, 0u);
            OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTION_REFERENCE), cpu->x);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82e849u, 0x828069u, 2u, cpu->program_bank))
                return MenuInventoryCapsuleUnwound(0x82e849u);
            OpLda(memory, cpu, OpDp(cpu, DP_MENU_SELECTION_VALUE));
            OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E1562));
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82e851u, 0x82e8b2u, 2u, cpu->program_bank))
                return MenuInventoryCapsuleUnwound(0x82e851u);
            OpLoadA(cpu, 0x80u);
            OpTestBits(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTED_SPELL + 1u), true);
            OpLoadA(cpu, 0x80u);
            OpTestBits(memory, cpu, OpAbs(cpu, WRAM_PLAY_TIME_FRAMES), false);
        }
    }
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82e860u);
}
