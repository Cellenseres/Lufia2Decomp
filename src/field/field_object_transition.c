#include "core/cpu_ops.h"
#include "actor/actor_internal.h"
#include "lufia2/actor.h"
#include "lufia2/field.h"
#include "system/dp_scratch.h"
#include "system/wram.h"

enum {
    OBJECT_PROBE_X = 0x8fu,
    OBJECT_PROBE_Y = 0x91u,
    OBJECT_PROBE_ACTION = 0x94u,
    OBJECT_HEADER_BASE = 0x7ef000u,
};

typedef Lufia2ExecutionResult (*ObjectTransitionStep)(
    const Lufia2Memory *, Lufia2CpuState *);

static bool ObjectTransitionContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x83u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && !cpu->direct_page &&
        cpu->stack >= 0x1f10u && cpu->stack <= 0x1ffcu;
}

Lufia2ExecutionResult Lufia2ActorSpawnFromId(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ObjectTransitionContext(cpu))
        return ExecutionHandoff(cpu, 0x83df87u);
    Lufia2ActorSpawn(memory, cpu);
    return ExecutionReturned(0x83dfa4u);
}

static bool ObjectTransitionCall(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    ObjectTransitionStep step, uint8_t bank, uint16_t back, uint8_t frame) {
    if (frame == 3u)
        SimulateJslFrame(memory, cpu, 0x83u, back);
    else
        SimulateJsrFrame(memory, cpu, back);
    cpu->program_bank = bank;
    Lufia2ExecutionResult result = step(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return false;
    uint8_t low = Pull8(memory, cpu);
    uint8_t high = Pull8(memory, cpu);
    uint8_t caller = frame == 3u ? Pull8(memory, cpu) : 0x83u;
    uint16_t actual = (uint16_t)(low | ((uint16_t)high << 8));
    cpu->program_bank = caller;
    if (actual == back && caller == 0x83u)
        return true;
    cpu->resume_pc = ((uint32_t)caller << 16) | (uint16_t)(actual + 1u);
    return false;
}

static Lufia2ExecutionResult ObjectTransitionRecordOffsets(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ActorRecordOffsets(memory, cpu);
    return ExecutionReturned(0x83ab70u);
}

static bool ObjectTransitionSpawnActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpLongX(cpu, OBJECT_HEADER_BASE + 6u));
    OpCmpValue(cpu, 0xffu);
    if (cpu->zero)
        return true;
    OpPushX(memory, cpu);
    if (!ObjectTransitionCall(memory, cpu, Lufia2ActorSpawnFromId,
        0x83u, 0xf4d4u, 3u))
        return false;
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLda(memory, cpu, OpDp(cpu, OBJECT_PROBE_ACTION));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7FDA2C));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SLOT_WORD_OFFSET)));
    OpLda(memory, cpu, OpDp(cpu, OBJECT_PROBE_X));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_FINE_X));
    OpLda(memory, cpu, OpDp(cpu, OBJECT_PROBE_Y));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_FINE_Y));
    TransferDirectToA(cpu);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_FINE_X + 1u));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_FINE_Y + 1u));
    if (!ObjectTransitionCall(memory, cpu, Lufia2ObjectScaleFinePosition,
        0x83u, 0xf4f6u, 2u))
        return false;
    OpPullX(memory, cpu);
    return true;
}

static bool ObjectTransitionPreparePending(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, WRAM_FIELD_PENDING_UPDATE_RECORD_ID);
    cpu->carry = 0;
    OpAdcValue(cpu, 0x10u);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_PENDING_OBJECT_RECORD));
    OpLoadA(cpu, 0x0au);
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, WRAM_FIELD_PENDING_UPDATE_RECORD_ID);
    OpLdx(cpu, 0x16u);
    if (!ObjectTransitionCall(memory, cpu, Lufia2FieldFindHeaderRecord,
        0x80u, 0xf527u, 3u))
        return false;
    OpLda(memory, cpu, OpLongX(cpu, OBJECT_HEADER_BASE + 7u));
    OpOraValue(cpu, OpReadM(memory, cpu, OpLongX(cpu, OBJECT_HEADER_BASE + 8u)));
    if (!cpu->zero)
        return true;
    TransferDirectToA(cpu);
    OpLda(memory, cpu, WRAM_FIELD_PENDING_UPDATE_RECORD_INDEX);
    OpTax(cpu);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_PENDING_RECORD_X));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_PENDING_RECORD_Y));
    TransferDirectToA(cpu);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_PENDING_RECORD_FLAGS));
    return true;
}

static bool ObjectTransitionUpdatePending(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpLongX(cpu, OBJECT_HEADER_BASE + 9u));
    OpCmpValue(cpu, 0xffu);
    if (cpu->zero)
        return true;
    OpSta(memory, cpu, WRAM_FIELD_PENDING_UPDATE_RECORD_ID);
    if (!ObjectTransitionCall(memory, cpu, Lufia2FieldFindPendingObject,
        0x83u, 0xf507u, 2u))
        return false;
    OpTxa(cpu);
    OpSta(memory, cpu, WRAM_FIELD_PENDING_UPDATE_RECORD_INDEX);
    if (!cpu->carry && !ObjectTransitionPreparePending(memory, cpu))
        return false;
    OpLda(memory, cpu, WRAM_FIELD_PENDING_UPDATE_RECORD_ID);
    return ObjectTransitionCall(memory, cpu, Lufia2FieldUpdatePlacedObject,
        0x83u, 0xf54du, 2u) &&
        ObjectTransitionCall(memory, cpu, Lufia2FieldClearPendingOccupancy,
            0x83u, 0xf550u, 2u);
}

Lufia2ExecutionResult Lufia2FieldApplyObjectRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ObjectTransitionContext(cpu) || cpu->stack < 0x1f40u)
        return ExecutionHandoff(cpu, 0x83f4b4u);
    OpSta(memory, cpu, OpDp(cpu, OBJECT_PROBE_ACTION));
    OpLda(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT));
    PushAccumulator8(memory, cpu);
    OpTxa(cpu);
    ExchangeAccumulatorBytes(cpu);
    OpLoadA(cpu, 0x0au);
    ExchangeAccumulatorBytes(cpu);
    cpu->carry = 1;
    OpSbcValue(cpu, 0x10u);
    OpLdx(cpu, 0x16u);
    if (!ObjectTransitionCall(memory, cpu, Lufia2FieldFindHeaderRecord,
            0x80u, 0xf4c7u, 3u) ||
        !ObjectTransitionSpawnActor(memory, cpu) ||
        !ObjectTransitionUpdatePending(memory, cpu))
        return ExecutionHandoff(cpu, cpu->resume_pc);
    LoadA8(cpu, Pull8(memory, cpu));
    OpSta(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT));
    if (!ObjectTransitionCall(memory, cpu, ObjectTransitionRecordOffsets,
        0x83u, 0xf557u, 3u))
        return ExecutionHandoff(cpu, cpu->resume_pc);
    return ExecutionReturned(0x83f558u);
}
