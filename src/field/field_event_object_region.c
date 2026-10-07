#include "core/cpu_ops.h"
#include "field/event_script_internal.h"
#include "lufia2/field.h"
#include "lufia2/actor.h"
#include "system/wram.h"

typedef Lufia2ExecutionResult (*ObjectRegionStep)(
    const Lufia2Memory *, Lufia2CpuState *);

typedef struct ObjectRegionHeader {
    const Lufia2Memory *memory;
    Lufia2ExecutionResult result;
} ObjectRegionHeader;

static bool ObjectRegionContext(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit &&
        !cpu->decimal && !cpu->direct_page && cpu->program_bank == 0x80u &&
        cpu->stack >= 0x1f20u && cpu->stack <= 0x1ffcu;
}

static Lufia2ExecutionResult ObjectRegionReturn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t bank, uint16_t back, uint8_t frame_size) {
    uint8_t low = Pull8(memory, cpu);
    uint8_t high = Pull8(memory, cpu);
    uint8_t caller = frame_size == 3u ? Pull8(memory, cpu) : bank;
    uint16_t actual = (uint16_t)(low | ((uint16_t)high << 8));
    cpu->program_bank = caller;
    if (actual != back || caller != bank)
        return ExecutionHandoff(cpu,
            ((uint32_t)caller << 16) | (uint16_t)(actual + 1u));
    return ExecutionReturned(0);
}

static uint8_t ObjectRegionFindHeader(
    void *context, Lufia2CpuState *cpu,
    uint32_t target, uint32_t site, uint8_t frame_size) {
    ObjectRegionHeader *header = context;
    (void)target;
    (void)site;
    (void)frame_size;
    cpu->program_bank = 0x80u;
    header->result = Lufia2FieldFindHeaderRecord(header->memory, cpu);
    if (header->result.flow != LUFIA2_EXECUTION_RETURNED)
        return 0;
    header->result = ObjectRegionReturn(header->memory, cpu, 0x83u, 0x8b4au, 3u);
    return header->result.flow == LUFIA2_EXECUTION_RETURNED;
}

static Lufia2ExecutionResult ObjectRegionLoadHeader(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    ObjectRegionHeader header = {memory, {LUFIA2_EXECUTION_RETURNED, 0, 0}};
    Lufia2ExecutionResult result = Lufia2FieldLoadObjectRecord(
        memory, cpu, ObjectRegionFindHeader, &header);
    return result.flow == LUFIA2_EXECUTION_CHILD_UNWOUND ? header.result : result;
}

static Lufia2ExecutionResult ObjectRegionCall(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    ObjectRegionStep step, uint8_t bank, uint16_t back, uint8_t frame_size) {
    Lufia2ExecutionResult result;
    if (frame_size == 3u)
        SimulateJslFrame(memory, cpu, 0x80u, back);
    else
        SimulateJsrFrame(memory, cpu, back);
    cpu->program_bank = bank;
    result = step(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    return ObjectRegionReturn(memory, cpu, 0x80u, back, frame_size);
}

Lufia2ExecutionResult Lufia2FieldPrepareObjectRegion(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;
    if (!ObjectRegionContext(cpu))
        return ExecutionHandoff(cpu, 0x80d0acu);
    PushY(memory, cpu);
    OpSta(memory, cpu, WRAM_FIELD_REGION_RECORD_ID);
    result = ObjectRegionCall(memory, cpu, ObjectRegionLoadHeader,
        0x83u, 0xd0b4u, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    result = ObjectRegionCall(memory, cpu, Lufia2FieldNormalizeObjectOrigin,
        0x80u, 0xd0b7u, 2u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpPullY(memory, cpu);
    return ExecutionReturned(0x80d0b9u);
}

Lufia2ExecutionResult Lufia2FieldCopyEventObjectRegion(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;
    if (!ObjectRegionContext(cpu))
        return ExecutionHandoff(cpu, 0x80d112u);
    PushY(memory, cpu);
    OpLda(memory, cpu, WRAM_FIELD_OBJECT_FLAGS);
    OpTestBits(memory, cpu, OpAbs(cpu, 0x1273u), 1u);
    result = ObjectRegionCall(memory, cpu, Lufia2FieldNormalizeObjectOrigin,
        0x80u, 0xd11cu, 2u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    result = ObjectRegionCall(memory, cpu, Lufia2FieldCopyObjectTiles,
        0x83u, 0xd120u, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpLda(memory, cpu, WRAM_FIELD_OBJECT_FLAGS);
    OpTestBits(memory, cpu, OpAbs(cpu, 0x1273u), 1u);
    OpBitValue(cpu, 0x22u);
    if (!cpu->zero) {
        OpLda(memory, cpu, 0x0005aau);
        result = ObjectRegionCall(memory, cpu, Lufia2FieldRefreshObjectAttributes,
            0x83u, 0xd133u, 3u);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
    }
    OpPullY(memory, cpu);
    return ExecutionReturned(0x80d135u);
}

typedef struct PendingObjectWork {
    const Lufia2Memory *memory;
    Lufia2ExecutionResult result;
} PendingObjectWork;

static uint8_t PendingObjectChild(
    void *context, Lufia2CpuState *cpu,
    uint32_t target, uint32_t site, uint8_t frame_size) {
    PendingObjectWork *work = context;
    const Lufia2Memory *memory = work->memory;
    cpu->program_bank = (uint8_t)(target >> 16);
    switch (target) {
    case 0x83f80du:
        work->result = Lufia2FieldMarkObjectAttributes(
            memory, cpu, PendingObjectChild, work);
        if (work->result.flow == LUFIA2_EXECUTION_CHILD_UNWOUND)
            return 0;
        break;
    case 0x83f9adu:
        work->result = Lufia2FieldObjectAttributeCell(memory, cpu);
        break;
    case 0x83fb9fu:
        work->result = Lufia2FieldFindPendingObject(memory, cpu);
        break;
    case 0x83f85au:
        work->result = Lufia2FieldPendingTileOffsets(memory, cpu);
        break;
    case 0x83f9d4u:
        Lufia2ActorResolveMapCellOffset(memory, cpu);
        work->result = ExecutionReturned(0);
        break;
    case 0x83f9d9u:
        work->result = Lufia2LayerCellOffset(memory, cpu);
        break;
    case 0x83f91fu:
        work->result = Lufia2FieldCopyCellTile(memory, cpu);
        break;
    default:
        work->result = ExecutionHandoff(cpu, target);
        return 0;
    }
    if (work->result.flow != LUFIA2_EXECUTION_RETURNED)
        return 0;
    work->result = ObjectRegionReturn(memory, cpu, 0x83u,
        (uint16_t)(site + frame_size), frame_size);
    return work->result.flow == LUFIA2_EXECUTION_RETURNED;
}

static Lufia2ExecutionResult PendingObjectPlace(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PendingObjectWork work = {memory, {LUFIA2_EXECUTION_RETURNED, 0, 0}};
    Lufia2ExecutionResult result = Lufia2FieldPlacePendingObject(
        memory, cpu, PendingObjectChild, &work);
    return result.flow == LUFIA2_EXECUTION_CHILD_UNWOUND ? work.result : result;
}

static bool PendingObjectSelected(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_FIELD_PENDING_RECORD_FLAGS & 0xffffu));
    if (!cpu->negative)
        return false;
    OpBitValue(cpu, 1u);
    return !cpu->zero;
}

static void PendingObjectLoadId(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpPushX(memory, cpu);
    OpWriteX(memory, cpu, OpDp(cpu, 0x56u), cpu->x);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_FIELD_PENDING_OBJECT_RECORD & 0xffffu));
    cpu->carry = 1;
    OpSbcValue(cpu, 0x10u);
}

Lufia2ExecutionResult Lufia2FieldRemoveRegionObjects(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;
    if (!ObjectRegionContext(cpu))
        return ExecutionHandoff(cpu, 0x80d1e1u);
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7fu);
    OpLdx(cpu, 47u);
    do {
        if (PendingObjectSelected(memory, cpu)) {
            PendingObjectLoadId(memory, cpu);
            result = ObjectRegionCall(memory, cpu, ObjectRegionLoadHeader,
                0x83u, 0xd1feu, 3u);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x56u)));
            OpLda(memory, cpu, OpAbsX(cpu, WRAM_FIELD_PENDING_RECORD_X & 0xffffu));
            OpSta(memory, cpu, OpDp(cpu, 0x8fu));
            OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_PENDING_OBJECT_X & 0xffffu));
            OpLda(memory, cpu, OpAbsX(cpu, WRAM_FIELD_PENDING_RECORD_Y & 0xffffu));
            cpu->carry = 1;
            OpSbcValue(cpu, OpReadM(memory, cpu,
                OpAbs(cpu, WRAM_FIELD_OBJECT_HEIGHT & 0xffffu)));
            OpIncA(cpu);
            OpSta(memory, cpu, OpDp(cpu, 0x91u));
            OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_PENDING_OBJECT_Y & 0xffffu));
            TransferDirectToA(cpu);
            OpLda(memory, cpu, OpDp(cpu, 0x56u));
            result = ObjectRegionCall(memory, cpu, Lufia2FieldSetObjectTiles,
                0x83u, 0xd21cu, 3u);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
            result = ObjectRegionCall(memory, cpu, Lufia2FieldClearObjectAttributes,
                0x83u, 0xd220u, 3u);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
            OpPullX(memory, cpu);
        }
        OpDex(cpu);
    } while (!cpu->negative);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x80d226u);
}

Lufia2ExecutionResult Lufia2FieldRestoreRegionObjects(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;
    if (!ObjectRegionContext(cpu))
        return ExecutionHandoff(cpu, 0x80d19fu);
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7fu);
    OpLdx(cpu, 47u);
    do {
        if (PendingObjectSelected(memory, cpu)) {
            OpAndValue(cpu, 0xfeu);
            OpSta(memory, cpu, OpAbsX(cpu, WRAM_FIELD_PENDING_RECORD_FLAGS & 0xffffu));
            PendingObjectLoadId(memory, cpu);
            result = ObjectRegionCall(memory, cpu, ObjectRegionLoadHeader,
                0x83u, 0xd1c1u, 3u);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x56u)));
            OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_PENDING_RECORD_X));
            OpSta(memory, cpu, OpDp(cpu, 0x8fu));
            OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_PENDING_RECORD_Y));
            OpSta(memory, cpu, OpDp(cpu, 0x91u));
            result = ObjectRegionCall(memory, cpu, Lufia2FieldSetObjectOrigin,
                0x83u, 0xd1d3u, 3u);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
            TransferDirectToA(cpu);
            OpLda(memory, cpu, OpDp(cpu, 0x56u));
            result = ObjectRegionCall(memory, cpu, PendingObjectPlace,
                0x83u, 0xd1dau, 3u);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
            OpPullX(memory, cpu);
        }
        OpDex(cpu);
    } while (!cpu->negative);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x80d1e0u);
}
Lufia2ExecutionResult Lufia2FieldUpdateEventObjectRegion(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;
    if (!ObjectRegionContext(cpu) || cpu->stack < 0x1f40u)
        return ExecutionHandoff(cpu, 0x80d0bau);
    PushY(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpSepWidths(cpu, 0x20u);
    result = ObjectRegionCall(memory, cpu, Lufia2FieldSaveObjectRegion,
        0x80u, 0xd0c1u, 2u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpLda(memory, cpu, WRAM_FIELD_OBJECT_FLAGS);
    OpTestBits(memory, cpu, OpAbs(cpu, 0x1273u), 1u);
    OpBitValue(cpu, 2u);
    if (!cpu->zero) {
        result = ObjectRegionCall(memory, cpu, Lufia2FieldMarkRegionObjects,
            0x80u, 0xd0d0u, 3u);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
        OpSta(memory, cpu, WRAM_FIELD_REGION_CHANGED_FLAGS);
        OpOraValue(cpu, 0u);
        if (cpu->negative) {
            result = ObjectRegionCall(memory, cpu, Lufia2FieldRemoveRegionObjects,
                0x80u, 0xd0dcu, 3u);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
            result = ObjectRegionCall(memory, cpu, Lufia2FieldRestoreObjectRegion,
                0x80u, 0xd0dfu, 2u);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
        }
    }
    result = ObjectRegionCall(memory, cpu, Lufia2FieldCopyObjectTiles,
        0x83u, 0xd0e3u, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpLda(memory, cpu, WRAM_FIELD_OBJECT_FLAGS);
    OpTestBits(memory, cpu, OpAbs(cpu, 0x1273u), 1u);
    OpBitValue(cpu, 0x22u);
    if (!cpu->zero) {
        OpLda(memory, cpu, 0x0005aau);
        result = ObjectRegionCall(memory, cpu, Lufia2FieldRefreshObjectAttributes,
            0x83u, 0xd0f6u, 3u);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
    }
    OpLda(memory, cpu, WRAM_FIELD_OBJECT_FLAGS);
    OpBitValue(cpu, 2u);
    if (!cpu->zero) {
        OpLda(memory, cpu, WRAM_FIELD_REGION_CHANGED_FLAGS);
        if (cpu->negative) {
            result = ObjectRegionCall(memory, cpu, Lufia2FieldRestoreRegionObjects,
                0x80u, 0xd108u, 3u);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
            result = ObjectRegionCall(memory, cpu, Lufia2FieldRestoreObjectRegion,
                0x80u, 0xd10bu, 2u);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
        }
    }
    result = ObjectRegionCall(memory, cpu, Lufia2FieldRenderLayerPair,
        0x83u, 0xd10fu, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpPullY(memory, cpu);
    return ExecutionReturned(0x80d111u);
}
Lufia2ExecutionResult Lufia2FieldReadObjectRegionPosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    uint32_t handoff = 0;
    if (!ObjectRegionContext(cpu))
        return ExecutionHandoff(cpu, 0x80d077u);
    Lufia2EventNextByte(memory, cpu, 0xd079u);
    if (!Lufia2EventPosition(memory, cpu, 0xd07cu, &handoff))
        return ExecutionHandoff(cpu, handoff);
    OpSta(memory, cpu, WRAM_FIELD_PENDING_OBJECT_X);
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, WRAM_FIELD_PENDING_OBJECT_Y);
    return ExecutionReturned(0x80d086u);
}

Lufia2ExecutionResult Lufia2FieldReadObjectRegionDestination(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    uint32_t handoff = 0;
    if (!ObjectRegionContext(cpu))
        return ExecutionHandoff(cpu, 0x80ce5cu);
    Write8(memory, OpDp(cpu, 0xaeu), 0u);
    Lufia2EventNextByte(memory, cpu, 0xce60u);
    OpSta(memory, cpu, WRAM_FIELD_OBJECT_FLAGS);
    Lufia2EventNextByte(memory, cpu, 0xce67u);
    Lufia2EventValue(memory, cpu, 0xce6au);
    if (!Lufia2EventPosition(memory, cpu, 0xce6du, &handoff))
        return ExecutionHandoff(cpu, handoff);
    OpSta(memory, cpu, WRAM_FIELD_PENDING_OBJECT_X);
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, WRAM_FIELD_PENDING_OBJECT_Y);
    Lufia2EventNextByte(memory, cpu, 0xce79u);
    Lufia2EventValue(memory, cpu, 0xce7cu);
    return ExecutionReturned(0x80ce7du);
}

Lufia2ExecutionResult Lufia2FieldReadObjectRegionArea(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    uint32_t handoff = 0;
    if (!ObjectRegionContext(cpu))
        return ExecutionHandoff(cpu, 0x80ce7eu);
    if (!Lufia2EventArea(memory, cpu, 0xce80u, &handoff))
        return ExecutionHandoff(cpu, handoff);
    OpLda(memory, cpu, OpDp(cpu, 0x9fu));
    OpSta(memory, cpu, WRAM_FIELD_OBJECT_SOURCE_X);
    OpLda(memory, cpu, OpDp(cpu, 0xa0u));
    OpSta(memory, cpu, WRAM_FIELD_OBJECT_SOURCE_Y);
    OpLda(memory, cpu, OpDp(cpu, 0xa1u));
    cpu->carry = 1;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0x9fu)));
    OpSta(memory, cpu, WRAM_FIELD_OBJECT_WIDTH);
    OpLda(memory, cpu, OpDp(cpu, 0xa2u));
    cpu->carry = 1;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0xa0u)));
    OpSta(memory, cpu, WRAM_FIELD_OBJECT_HEIGHT);
    return ExecutionReturned(0x80ce9fu);
}

unsigned Lufia2EventCopyObjectRegion(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t *handoff) {
    Lufia2ExecutionResult result;
    Write8(memory, OpDp(cpu, 0xaeu), 0u);
    Lufia2EventNextByte(memory, cpu, 0xcdddu);
    if (!Lufia2EventPosition(memory, cpu, 0xcde0u, handoff))
        return EVENT_OPCODE_HANDOFF;
    OpSta(memory, cpu, WRAM_FIELD_PENDING_OBJECT_X);
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, WRAM_FIELD_PENDING_OBJECT_Y);
    Lufia2EventNextByte(memory, cpu, 0xcdecu);
    Lufia2EventVariable(memory, cpu, 0xcdefu);
    PushY(memory, cpu);
    OpSta(memory, cpu, WRAM_FIELD_REGION_RECORD_ID);
    result = ObjectRegionCall(memory, cpu, ObjectRegionLoadHeader,
        0x83u, 0xcdf8u, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        goto child_handoff;
    result = ObjectRegionCall(memory, cpu, Lufia2FieldNormalizeObjectOrigin,
        0x80u, 0xcdfbu, 2u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        goto child_handoff;
    result = ObjectRegionCall(memory, cpu, Lufia2FieldCopyObjectTiles,
        0x83u, 0xcdffu, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        goto child_handoff;
    OpLda(memory, cpu, WRAM_FIELD_OBJECT_FLAGS);
    OpTestBits(memory, cpu, OpAbs(cpu, 0x1273u), 1u);
    OpBitValue(cpu, 0x22u);
    if (!cpu->zero) {
        OpLda(memory, cpu, 0x0005aau);
        result = ObjectRegionCall(memory, cpu, Lufia2FieldRefreshObjectAttributes,
            0x83u, 0xce12u, 3u);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            goto child_handoff;
    }
    result = ObjectRegionCall(memory, cpu, Lufia2FieldRenderLayerPair,
        0x83u, 0xce16u, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        goto child_handoff;
    OpPullY(memory, cpu);
    return EVENT_OPCODE_NEXT;

child_handoff:
    *handoff = result.pc;
    return EVENT_OPCODE_HANDOFF;
}
