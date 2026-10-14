#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    FIELD_CELL_FLAGS_RETAINED = 0xcfffu,
    FIELD_PUBLISH_REQUEST = 0xa8u,
    DP_FIELD_PUBLISH_MARKER = 0x74u,
    ROM_FIELD_UNPACK_ATTRIBUTES = 0x80ed0eu,
    ROM_FIELD_REDRAW_LAYERS = 0x838e66u,
    ROM_FIELD_PUBLISH_FRAME = 0x838e4cu,
    ROM_FIELD_FRAME_WAIT = 0x83900cu
};

static uint8_t PublishChild(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                            Lufia2PushedChildCall child, void *context, uint32_t site,
                            uint32_t target, uint8_t frame,
                            Lufia2ExecutionResult *result) {
    const uint8_t bank = cpu->program_bank, data_bank = cpu->data_bank;
    if (!CallChildWithFrame(memory, cpu, child, context, site, target, frame, bank)) {
        *result = (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return 0u;
    }
    if (cpu->program_bank != bank || cpu->data_bank != data_bank ||
        !cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->direct_page ||
        cpu->decimal) {
        *result = ExecutionHandoff(cpu, site + frame + 1u);
        return 0u;
    }
    return 1u;
}

static void ClearFieldCellFlags(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLoadA(cpu, 0x7fu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, WRAM_FIELD_LAYER_TABLE_OFFSET & 0xffffu);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_FIELD_LAYER_WIDTH & 0xffffu));
    OpSta(memory, cpu, SNES_WRMPYA);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_FIELD_LAYER_HEIGHT & 0xffffu));
    OpSta(memory, cpu, SNES_WRMPYB);
    OpRepWidths(cpu, 0x20u);
    /* The original always uses the first layer. */
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_LAYER_CELL_BASE & 0xffffu));
    OpTax(cpu);
    OpLda(memory, cpu, SNES_RDMPYL);
    OpTay(cpu);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        OpAndValue(cpu, FIELD_CELL_FLAGS_RETAINED);
        OpSta(memory, cpu, OpAbsX(cpu, 0u));
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
}

Lufia2ExecutionResult Lufia2FieldClearCellFlagsAndPublish(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu,
                                                          Lufia2PushedChildCall child,
                                                          void *context) {
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal || !child)
        return ExecutionHandoff(cpu, 0x80fbceu);
    Lufia2ExecutionResult result = {0};
    PushDataBank(memory, cpu);
    if (!PublishChild(memory, cpu, child, context, 0x80fbcfu,
                      ROM_FIELD_UNPACK_ATTRIBUTES, 3u, &result))
        return result;
    ClearFieldCellFlags(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, SNES_INIDISP);
    if (!PublishChild(memory, cpu, child, context, 0x80fc0eu, ROM_FIELD_PUBLISH_FRAME,
                      3u, &result))
        return result;
    OpLoadA(cpu, 0x0fu);
    OpSta(memory, cpu, SNES_INIDISP);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x80fc19u);
}

Lufia2ExecutionResult Lufia2FieldPublishRedrawnLayers(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu,
                                                      Lufia2PushedChildCall child,
                                                      void *context) {
    if (cpu->program_bank != 0x83u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal || !child)
        return ExecutionHandoff(cpu, 0x838e4cu);
    Lufia2ExecutionResult result = {0};
    PushAccumulator8(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    if (!PublishChild(memory, cpu, child, context, 0x838e55u, ROM_FIELD_REDRAW_LAYERS,
                      3u, &result))
        return result;
    OpLoadA(cpu, FIELD_PUBLISH_REQUEST);
    OpSta(memory, cpu, OpDp(cpu, DP_FIELD_PUBLISH_MARKER));
    if (!PublishChild(memory, cpu, child, context, 0x838e5du, ROM_FIELD_FRAME_WAIT, 2u,
                      &result))
        return result;
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    LoadA8(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x838e65u);
}
