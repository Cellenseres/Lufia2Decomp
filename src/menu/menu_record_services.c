#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/menu.h"

enum {
    DP_MESSAGE_SELECTION = 0x00u,
    DP_GRAPHICS_DESTINATION = 0x00u,
    DP_GRAPHICS_SOURCE = 0x08u,
    DP_MESSAGE_TEXT_BANK = 0x5fu,
    DP_MESSAGE_FRAME_FLAGS = 0x74u,
    DP_EQUIPMENT_MEMBER = 0x2au,
    ALTERNATE_GRAPHICS_SELECTOR = WRAM_SAVE_FILE_BUFFER + 0x10u,
    ALTERNATE_GRAPHICS_POINTERS = 0x878016u,
    ALTERNATE_GRAPHICS_BUFFER = 0x4000u,
    FIELD_SNAPSHOT_SOURCE = WRAM_UNK_7E93C0,
    FIELD_SNAPSHOT_DESTINATION = WRAM_FIELD_OAM_LOW_BUFFER,
    FIELD_SNAPSHOT_BYTES = WRAM_UNK_7E93C0_COUNT,
    MESSAGE_SNAPSHOT_SOURCE = WRAM_UNK_7EA498,
    MESSAGE_SNAPSHOT_DESTINATION = WRAM_UNK_7FF080,
    MESSAGE_SNAPSHOT_BYTES = WRAM_UNK_7EA498_COUNT,
    EQUIPMENT_ITEM_MASK = 0x01ffu,
    EQUIPMENT_ITEM_FLAG = 0x08u
};

static Lufia2ExecutionResult MenuRecordChildUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static bool MenuRecordSupported(const Lufia2CpuState *cpu, bool narrow) {
    return cpu->accumulator_is_8_bit == narrow &&
        !cpu->index_is_8_bit && !cpu->decimal;
}

Lufia2ExecutionResult Lufia2MenuPrepareAlternateGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !MenuRecordSupported(cpu, true))
        return ExecutionHandoff(cpu, 0x82f496u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, ALTERNATE_GRAPHICS_SELECTOR);
    OpAndValue(cpu, 0x00ffu);
    for (unsigned shift = 0u; shift < 3u; ++shift)
        OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, ALTERNATE_GRAPHICS_POINTERS));
    OpTax(cpu);
    OpLdy(cpu, ALTERNATE_GRAPHICS_BUFFER);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f4abu, 0x80c9c0u, 3u, cpu->program_bank))
        return MenuRecordChildUnwound(0x82f4abu);
    OpLoadA(cpu, 0x007eu);
    OpSta(memory, cpu, OpDp(cpu, DP_GRAPHICS_SOURCE + 2u));
    OpLdx(cpu, ALTERNATE_GRAPHICS_BUFFER);
    OpWriteX(memory, cpu, OpDp(cpu, DP_GRAPHICS_SOURCE), cpu->x);
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, 0x1a01u);
    cpu->carry = false;
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f4bfu, 0x829073u, 2u, cpu->program_bank))
        return MenuRecordChildUnwound(0x82f4bfu);
    OpLdx(cpu, 0x7000u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f4c5u, 0x829113u, 2u, cpu->program_bank))
        return MenuRecordChildUnwound(0x82f4c5u);
    OpLdx(cpu, 0x2200u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_GRAPHICS_DESTINATION), cpu->x);
    OpLdx(cpu, 0x1a01u);
    OpLdy(cpu, 0x03c8u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f4d3u, 0x829133u, 2u, cpu->program_bank))
        return MenuRecordChildUnwound(0x82f4d3u);
    return ExecutionReturned(0x82f4d6u);
}

Lufia2ExecutionResult Lufia2MenuDrawSelectionMessage(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !MenuRecordSupported(cpu, true))
        return ExecutionHandoff(cpu, 0x82f637u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_MESSAGE_SELECTION), cpu->y);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x0386u);
    OpLdx(cpu, 0x1c05u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f641u, 0x8283ebu, 2u, cpu->program_bank))
        return MenuRecordChildUnwound(0x82f641u);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, DP_MESSAGE_TEXT_BANK));
    OpLdy(cpu, 0xcbe6u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f652u, 0x808878u, 3u, cpu->program_bank))
        return MenuRecordChildUnwound(0x82f652u);
    OpLoadA(cpu, 8u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_MESSAGE_FRAME_FLAGS), true);
    OpLda(memory, cpu, OpDp(cpu, DP_MESSAGE_SELECTION + 1u));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f65cu, 0x829330u, 2u, cpu->program_bank))
        return MenuRecordChildUnwound(0x82f65cu);
    return ExecutionReturned(0x82f65fu);
}

Lufia2ExecutionResult Lufia2MenuRestoreSavedFieldBuffers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!MenuRecordSupported(cpu, true))
        return ExecutionHandoff(cpu, 0x82f682u);
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpLongX(cpu, FIELD_SNAPSHOT_SOURCE));
        OpSta(memory, cpu, OpAbsX(cpu, FIELD_SNAPSHOT_DESTINATION));
        OpInx(cpu);
        OpCpx(cpu, FIELD_SNAPSHOT_BYTES);
    } while (!cpu->zero);
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpLongX(cpu, MESSAGE_SNAPSHOT_SOURCE));
        OpSta(memory, cpu, OpLongX(cpu, MESSAGE_SNAPSHOT_DESTINATION));
        OpInx(cpu);
        OpCpx(cpu, MESSAGE_SNAPSHOT_BYTES);
    } while (!cpu->zero);
    return ExecutionReturned(0x82f6a3u);
}

Lufia2ExecutionResult Lufia2MenuTestEquipmentItemFlag(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !MenuRecordSupported(cpu, false))
        return ExecutionHandoff(cpu, 0x82f893u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f893u, 0x82f703u, 2u, cpu->program_bank))
        return MenuRecordChildUnwound(0x82f893u);
    OpLda(memory, cpu, AbsoluteIndexedAddress(cpu,
        Read16Direct(memory, cpu, DP_EQUIPMENT_MEMBER), cpu->y));
    OpAndValue(cpu, EQUIPMENT_ITEM_MASK);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
    if (!cpu->zero) {
        PushY(memory, cpu);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82f8a1u, 0x81f194u, 3u, cpu->program_bank))
            return MenuRecordChildUnwound(0x82f8a1u);
        OpPullY(memory, cpu);
        OpAndValue(cpu, EQUIPMENT_ITEM_FLAG);
        if (!cpu->zero) {
            cpu->carry = true;
            return ExecutionReturned(0x82f8acu);
        }
    }
    cpu->carry = false;
    return ExecutionReturned(0x82f8aeu);
}
