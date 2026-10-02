#include "core/cpu_ops.h"
#include "lufia2/actor.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    OBJECT_UPLOAD_SOURCE_OFFSET = 0x00,
    OBJECT_UPLOAD_DESTINATION = 0x54,
};

Lufia2ExecutionResult Lufia2FieldClearObjectTileBit(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpLoadA(cpu, 0x20u);
    OpAndValue(cpu, OpReadM(memory, cpu, WRAM_FIELD_OBJECT_FLAGS));
    if (!cpu->zero) {
        Lufia2ExecutionResult result;
        uint8_t low, high;
        uint16_t frame;

        OpLda(memory, cpu, WRAM_FIELD_PENDING_OBJECT_X);
        ExchangeAccumulatorBytes(cpu);
        OpLda(memory, cpu, WRAM_FIELD_OBJECT_HEIGHT);
        OpCmpValue(cpu, 4u);
        if (cpu->carry)
            OpDecA(cpu);
        cpu->carry = 0;
        OpAdc(memory, cpu, WRAM_FIELD_PENDING_OBJECT_Y);
        SimulateJsrFrame(memory, cpu, 0x89eeu);
        result = Lufia2LayerCellOffset(memory, cpu);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
        low = Pull8(memory, cpu);
        high = Pull8(memory, cpu);
        frame = (uint16_t)(low | ((uint16_t)high << 8));
        if (frame != 0x89eeu)
            return ExecutionHandoff(cpu, 0x830000u | (uint16_t)(frame + 1u));
        OpLda(memory, cpu, WRAM_FIELD_OBJECT_WIDTH);
        OpCmpValue(cpu, 2u);
        if (cpu->carry) {
            OpInx(cpu);
            OpInx(cpu);
        }
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpLongX(cpu, 0x7f0000u));
        OpAndValue(cpu, 0xfbffu);
        OpSta(memory, cpu, OpLongX(cpu, 0x7f0000u));
        OpSepWidths(cpu, 0x20u);
    }
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x838a09u);
}

static Lufia2ExecutionResult RenderObjectLayer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;
    uint8_t low, high, bank;
    uint16_t frame;

    SimulateJslFrame(memory, cpu, 0x83u, 0x8a38u);
    result = Lufia2FieldRenderRegion(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    low = Pull8(memory, cpu);
    high = Pull8(memory, cpu);
    bank = Pull8(memory, cpu);
    frame = (uint16_t)(low | ((uint16_t)high << 8));
    cpu->program_bank = bank;
    if (frame != 0x8a38u || bank != 0x83u)
        return ExecutionHandoff(cpu,
            ((uint32_t)bank << 16) | (uint16_t)(frame + 1u));
    return ExecutionReturned(0x838a39u);
}

static void QueueObjectLayerUpload(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    PushIndex(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, OBJECT_UPLOAD_SOURCE_OFFSET));
    OpLsrA(cpu);
    OpAdc(memory, cpu, OpAbsX(cpu, 0x8ff8u));
    OpSta(memory, cpu, OpDp(cpu, OBJECT_UPLOAD_DESTINATION));
    OpLda(memory, cpu, OpDp(cpu, OBJECT_UPLOAD_SOURCE_OFFSET));
    OpAdc(memory, cpu, OpAbsX(cpu, 0x8ff0u));
    OpTyx(cpu);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_COLUMN_UPLOAD_SOURCE));
    OpLda(memory, cpu, OpDp(cpu, OBJECT_UPLOAD_DESTINATION));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_COLUMN_UPLOAD_DESTINATION));
    OpLda(memory, cpu, WRAM_FIELD_OBJECT_HEIGHT);
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_COLUMN_UPLOAD_ROWS));
    OpIny(cpu);
    OpIny(cpu);
    OpPullX(memory, cpu);
    OpSepWidths(cpu, 0x20u);
}

Lufia2ExecutionResult Lufia2FieldRenderObjectLayers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    unsigned layers = 0;

    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, WRAM_FIELD_PENDING_OBJECT_Y);
    OpAndValue(cpu, 0x0fu);
    ExchangeAccumulatorBytes(cpu);
    OpRepWidths(cpu, 0x20u);
    OpLsrA(cpu);
    OpSta(memory, cpu, OpDp(cpu, OBJECT_UPLOAD_SOURCE_OFFSET));
    OpLda(memory, cpu, WRAM_FIELD_COLUMN_UPLOAD_COUNT_BYTES);
    OpTay(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, 6u);
    for (;;) {
        if (layers == 4096u)
            return ExecutionHandoff(cpu, 0x838a24u);
        ++layers;
        OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_LAYER_SECTION_WORD));
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbsX(cpu, 0x8c82u));
            OpAndValue(cpu, OpReadM(memory, cpu, WRAM_FIELD_OBJECT_FLAGS));
            OpBitValue(cpu, 0x0fu);
            if (!cpu->zero) {
                Lufia2ExecutionResult result = RenderObjectLayer(memory, cpu);
                if (result.flow != LUFIA2_EXECUTION_RETURNED)
                    return result;
                QueueObjectLayerUpload(memory, cpu);
            }
        }
        OpDex(cpu);
        OpDex(cpu);
        if (cpu->negative)
            break;
    }
    OpTya(cpu);
    OpSta(memory, cpu, WRAM_FIELD_COLUMN_UPLOAD_COUNT_BYTES);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x838a6eu);
}
