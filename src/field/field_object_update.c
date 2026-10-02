#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"

typedef struct ObjectHeaderLookup {
    const Lufia2Memory *memory;
    Lufia2ExecutionResult result;
} ObjectHeaderLookup;

static uint8_t FindObjectHeader(
    void *context, Lufia2CpuState *cpu,
    uint32_t target, uint32_t site, uint8_t frame_size) {
    ObjectHeaderLookup *lookup = context;
    uint8_t low, high, bank;
    uint16_t frame;

    (void)target;
    (void)site;
    (void)frame_size;
    cpu->program_bank = 0x80u;
    lookup->result = Lufia2FieldFindHeaderRecord(lookup->memory, cpu);
    if (lookup->result.flow != LUFIA2_EXECUTION_RETURNED)
        return 0;
    low = Pull8(lookup->memory, cpu);
    high = Pull8(lookup->memory, cpu);
    bank = Pull8(lookup->memory, cpu);
    frame = (uint16_t)(low | ((uint16_t)high << 8));
    cpu->program_bank = bank;
    if (frame != 0x8b4au || bank != 0x83u) {
        lookup->result = ExecutionHandoff(cpu,
            ((uint32_t)bank << 16) | (uint16_t)(frame + 1u));
        return 0;
    }
    return 1;
}

static Lufia2ExecutionResult LoadPlacedObjectRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    ObjectHeaderLookup lookup = {memory, {LUFIA2_EXECUTION_RETURNED, 0, 0}};
    Lufia2ExecutionResult result = Lufia2FieldLoadObjectRecord(
        memory, cpu, FindObjectHeader, &lookup);
    if (result.flow == LUFIA2_EXECUTION_CHILD_UNWOUND)
        return lookup.result;
    return result;
}

typedef Lufia2ExecutionResult (*ObjectUpdateStep)(
    const Lufia2Memory *, Lufia2CpuState *);

static Lufia2ExecutionResult RunObjectUpdateStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    ObjectUpdateStep step, uint16_t frame, uint8_t frame_size) {
    Lufia2ExecutionResult result;
    uint8_t low, high, bank;
    uint16_t actual_frame;

    if (frame_size == 3u)
        SimulateJslFrame(memory, cpu, 0x83u, frame);
    else
        SimulateJsrFrame(memory, cpu, frame);
    result = step(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    low = Pull8(memory, cpu);
    high = Pull8(memory, cpu);
    bank = frame_size == 3u ? Pull8(memory, cpu) : cpu->program_bank;
    actual_frame = (uint16_t)(low | ((uint16_t)high << 8));
    cpu->program_bank = bank;
    if (actual_frame != frame || bank != 0x83u)
        return ExecutionHandoff(cpu,
            ((uint32_t)bank << 16) | (uint16_t)(actual_frame + 1u));
    return result;
}

Lufia2ExecutionResult Lufia2FieldUpdatePlacedObject(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;

    PushDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    result = RunObjectUpdateStep(memory, cpu,
        LoadPlacedObjectRecord, 0x8b14u, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpLda(memory, cpu, WRAM_FIELD_PENDING_OBJECT_Y);
    cpu->carry = 1;
    OpSbcValue(cpu, OpReadM(memory, cpu, WRAM_FIELD_OBJECT_HEIGHT));
    OpIncA(cpu);
    OpSta(memory, cpu, WRAM_FIELD_PENDING_OBJECT_Y);
    result = RunObjectUpdateStep(memory, cpu,
        Lufia2FieldCopyObjectTiles, 0x8b26u, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    result = RunObjectUpdateStep(memory, cpu,
        Lufia2FieldRenderObjectLayers, 0x8b29u, 2u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    result = RunObjectUpdateStep(memory, cpu,
        Lufia2FieldClearObjectTileBit, 0x8b2cu, 2u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpLda(memory, cpu, WRAM_FIELD_OBJECT_HEIGHT);
    OpIncA(cpu);
    OpSta(memory, cpu, WRAM_FIELD_OBJECT_HEIGHT);
    OpLda(memory, cpu, WRAM_FIELD_LAYER_TABLE_OFFSET);
    result = RunObjectUpdateStep(memory, cpu,
        Lufia2FieldRefreshObjectAttributes, 0x8b3du, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x838b3fu);
}
