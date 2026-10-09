#include "core/cpu_ops.h"
#include "lufia2/system.h"
#include "system/wram.h"

enum {
    SAVE_FIELD_BUFFER = WRAM_SAVE_FILE_BUFFER & 0xffffu,
    SAVE_FIELD_MAP = SAVE_FIELD_BUFFER + 0x10u,
    SAVE_FIELD_LAYER = SAVE_FIELD_BUFFER + 0x11u,
    SAVE_FIELD_FLAGS = SAVE_FIELD_BUFFER + 0x12u,
    SAVE_FIELD_ACTORS = SAVE_FIELD_BUFFER + 0x23eu,
    SAVE_FIELD_OBJECT_BITS = SAVE_FIELD_BUFFER + 0x3a2u,
    SAVE_FIELD_SCENE_BYTES = SAVE_FIELD_BUFFER + 0x3aau,
    SAVE_FIELD_SCRIPT_FLAGS = SAVE_FIELD_BUFFER + 0x3b3u,
    SAVE_FIELD_FIRST_STATE = SAVE_FIELD_BUFFER + 0x3b7u,
    SAVE_FIELD_SECOND_STATE = SAVE_FIELD_BUFFER + 0x3b8u,
    SAVE_FIELD_THIRD_STATE = SAVE_FIELD_BUFFER + 0x3b9u,
    SAVE_FIELD_BACKGROUND = SAVE_FIELD_BUFFER + 0x3bau
};

typedef struct FieldSaveBlock {
    uint16_t live_address;
    uint16_t saved_address;
    uint16_t size;
    uint8_t live_bank;
} FieldSaveBlock;

static void PreserveFieldSaveRegisters(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit)
        PushAccumulator8(memory, cpu);
    else
        PushAccumulator16(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
}

static void RestoreFieldSaveRegisters(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    if (cpu->accumulator_is_8_bit)
        LoadA8(cpu, Pull8(memory, cpu));
    else
        PullAccumulator16(memory, cpu);
}

static void TransferFieldSaveBlocks(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint8_t restore) {
    static const FieldSaveBlock blocks[] = {
        {WRAM_EVENT_FLAGS, SAVE_FIELD_FLAGS, 0x22cu, 0x7eu},
        {WRAM_UNK_7E05D2, SAVE_FIELD_ACTORS, 0x164u, 0x7eu},
        {WRAM_FIELD_OBJECT_STATE_BITS & 0xffffu, SAVE_FIELD_OBJECT_BITS, 9u, 0x7fu},
        {WRAM_UNK_7E09DF, SAVE_FIELD_SCENE_BYTES, 9u, 0x7eu},
        {WRAM_UNK_7FD100 & 0xffffu, SAVE_FIELD_SCRIPT_FLAGS, 4u, 0x7fu}
    };
    for (unsigned block = 0u; block < sizeof(blocks) / sizeof(blocks[0]); ++block) {
        const FieldSaveBlock *range = &blocks[block];
        OpLdx(cpu, restore ? range->saved_address : range->live_address);
        OpLdy(cpu, restore ? range->live_address : range->saved_address);
        OpLoadA(cpu, (uint16_t)(range->size - 1u));
        OpMoveNext(memory, cpu,
            restore ? range->live_bank : 0x7eu,
            restore ? 0x7eu : range->live_bank);
    }
    OpSepWidths(cpu, 0x20u);
    OpSetDataBank(memory, cpu, 0x7eu);
}

static void CopyFieldSaveByte(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t source, uint16_t destination) {
    OpLda(memory, cpu, OpAbs(cpu, source));
    OpSta(memory, cpu, OpAbs(cpu, destination));
}

Lufia2ExecutionResult Lufia2RestoreSavedFieldState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PreserveFieldSaveRegisters(memory, cpu);
    TransferFieldSaveBlocks(memory, cpu, 1u);
    CopyFieldSaveByte(memory, cpu, SAVE_FIELD_FIRST_STATE, WRAM_UNK_7E05BD);
    CopyFieldSaveByte(memory, cpu, SAVE_FIELD_SECOND_STATE, WRAM_UNK_7E05BE);
    CopyFieldSaveByte(memory, cpu, SAVE_FIELD_THIRD_STATE, WRAM_UNK_7E05BF);
    OpLda(memory, cpu, OpAbs(cpu, SAVE_FIELD_BACKGROUND));
    OpSta(memory, cpu, WRAM_UNK_7FF8A0);
    CopyFieldSaveByte(memory, cpu, SAVE_FIELD_MAP, WRAM_FIELD_MAP_ID);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID + 1u));
    CopyFieldSaveByte(memory, cpu, SAVE_FIELD_LAYER, WRAM_FIELD_LAYER_TABLE_OFFSET);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FIELD_LAYER_TABLE_OFFSET + 1u));
    RestoreFieldSaveRegisters(memory, cpu);
    return ExecutionReturned(0x8eba0cu);
}

Lufia2ExecutionResult Lufia2CaptureFieldSaveState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PreserveFieldSaveRegisters(memory, cpu);
    TransferFieldSaveBlocks(memory, cpu, 0u);
    CopyFieldSaveByte(memory, cpu, WRAM_UNK_7E05BD, SAVE_FIELD_FIRST_STATE);
    CopyFieldSaveByte(memory, cpu, WRAM_UNK_7E05BE, SAVE_FIELD_SECOND_STATE);
    CopyFieldSaveByte(memory, cpu, WRAM_UNK_7E05BF, SAVE_FIELD_THIRD_STATE);
    OpLda(memory, cpu, WRAM_UNK_7FF8A0);
    OpSta(memory, cpu, OpAbs(cpu, SAVE_FIELD_BACKGROUND));
    CopyFieldSaveByte(memory, cpu, WRAM_FIELD_MAP_ID, SAVE_FIELD_MAP);
    CopyFieldSaveByte(memory, cpu, WRAM_FIELD_LAYER_TABLE_OFFSET, SAVE_FIELD_LAYER);
    RestoreFieldSaveRegisters(memory, cpu);
    return ExecutionReturned(0x8eba80u);
}
