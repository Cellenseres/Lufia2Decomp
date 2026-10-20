#include "core/cpu_ops.h"
#include "lufia2/system.h"
#include "system/wram.h"

enum {
    SOUND_DATA_PORT = 0x2140u,
    SOUND_SOURCE_POSITION = WRAM_SOUND_UPLOAD_SOURCE_CURSOR,
    SOUND_SOURCE_BANK = WRAM_SOUND_UPLOAD_SOURCE_BANK,
    SOUND_SOURCE_LENGTH = WRAM_SOUND_UPLOAD_SOURCE_LENGTH,
    SOUND_TRANSFER_STATE = WRAM_SOUND_PARAMETER_INHIBIT,
    SOUND_DEFERRED_COMMAND = WRAM_SOUND_DEFERRED_COMMAND,
    SOUND_RESOURCE_QUEUE = WRAM_SOUND_RESOURCE_OUTPUT,
    SOUND_QUEUE_SLOTS = WRAM_SOUND_RESOURCE_OUTPUT_COUNT
};

typedef struct QueuedSoundResource {
    uint16_t slot;
    uint8_t id;
} QueuedSoundResource;

static QueuedSoundResource FindQueuedSoundResource(
    const Lufia2Memory *memory, uint8_t data_bank) {
    const uint32_t queue = ((uint32_t)data_bank << 16) | SOUND_RESOURCE_QUEUE;
    QueuedSoundResource resource = {SOUND_QUEUE_SLOTS, 0xffu};

    for (uint16_t slot = 0u; slot < SOUND_QUEUE_SLOTS; ++slot) {
        const uint8_t id = Read8(memory, queue + slot);
        if (id != 0xffu) {
            resource.slot = slot;
            resource.id = id;
            break;
        }
    }
    return resource;
}

static uint8_t PublishQueuedSoundSource(
    const Lufia2Memory *memory, uint8_t data_bank, uint16_t direct_page,
    uint16_t source, uint16_t length) {
    const uint32_t bank = (uint32_t)data_bank << 16;

    Write16Long(memory, bank | SOUND_SOURCE_LENGTH, length);
    Write16Long(memory, bank | SOUND_SOURCE_POSITION, source);
    const uint8_t source_bank = Read8(memory, (uint16_t)(direct_page + 0x5fu));
    Write8(memory, bank | SOUND_SOURCE_BANK, source_bank);
    return source_bank;
}

static void SoundSaveAccumulator(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit)
        PushAccumulator8(memory, cpu);
    else
        PushAccumulator16(memory, cpu);
}

static void SoundRestoreAccumulator(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit)
        LoadA8(cpu, Pull8(memory, cpu));
    else
        PullAccumulator16(memory, cpu);
}

static Lufia2ExecutionResult SoundUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static uint8_t SoundCall(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t target, uint32_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    return child(context, cpu, target, site, 2u);
}

static uint8_t SoundRequest(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint8_t mode, uint32_t site) {
    OpLoadA(cpu, mode);
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    Lufia2WriteSoundDriverMode(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    return SoundCall(memory, cpu, child, context, 0x809a0au, site + 3u);
}

static void SoundWriteMarker(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t return_pc) {
    SimulateJsrFrame(memory, cpu, return_pc);
    Lufia2WriteSoundTransferMarker(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

Lufia2ExecutionResult Lufia2SoundDriverRequest03(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->decimal || !cpu->accumulator_is_8_bit ||
        cpu->program_bank != 0x80u)
        return ExecutionHandoff(cpu, 0x809528u);
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    SoundSaveAccumulator(memory, cpu);
    SetAccumulatorWidth(cpu, 1u);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    if (!SoundRequest(memory, cpu, child, context, 3u, 0x809531u))
        return SoundUnwound(0x809534u);
    SoundRestoreAccumulator(memory, cpu);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x80953au);
}

Lufia2ExecutionResult Lufia2BeginQueuedSoundResource(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->decimal || cpu->program_bank != 0x80u)
        return ExecutionHandoff(cpu, 0x809747u);
    SetAccumulatorWidth(cpu, 1u);
    SetIndexWidth(cpu, 0u);
    SoundWriteMarker(memory, cpu, 0x974du);
    OpSta(memory, cpu, OpDp(cpu, 0x54u));
    if (!SoundRequest(memory, cpu, child, context, 0x12u, 0x809752u))
        return SoundUnwound(0x809755u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, SOUND_DATA_PORT)));
    PushIndex(memory, cpu);
    if (!SoundRequest(memory, cpu, child, context, 0x1du, 0x80975eu))
        return SoundUnwound(0x809761u);
    OpLda(memory, cpu, OpDp(cpu, 0x54u));
    if (!SoundCall(memory, cpu, child, context, 0x8098a5u, 0x809766u))
        return SoundUnwound(0x809766u);
    if (cpu->accumulator_is_8_bit && !cpu->index_is_8_bit) {
        LoadA8(cpu, PublishQueuedSoundSource(memory, cpu->data_bank,
            cpu->direct_page, cpu->y, cpu->x));
    } else {
        OpWriteX(memory, cpu, OpAbs(cpu, SOUND_SOURCE_LENGTH), cpu->x);
        OpWriteX(memory, cpu, OpAbs(cpu, SOUND_SOURCE_POSITION), cpu->y);
        OpLda(memory, cpu, OpDp(cpu, 0x5fu));
        OpSta(memory, cpu, OpAbs(cpu, SOUND_SOURCE_BANK));
    }
    OpPullX(memory, cpu);
    OpWriteX(memory, cpu, OpAbs(cpu, SOUND_DATA_PORT), cpu->x);
    if (!SoundRequest(memory, cpu, child, context, 0x16u, 0x80977au))
        return SoundUnwound(0x80977du);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, SOUND_TRANSFER_STATE));
    return ExecutionReturned(0x809785u);
}

Lufia2ExecutionResult Lufia2UpdateSoundResourceQueue(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->decimal || cpu->program_bank != 0x80u)
        return ExecutionHandoff(cpu, 0x809786u);
    SoundSaveAccumulator(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1u);
    SetIndexWidth(cpu, 0u);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    SoundWriteMarker(memory, cpu, 0x9793u);
    OpLda(memory, cpu, OpAbs(cpu, SOUND_TRANSFER_STATE));
    uint8_t transfer_pending = cpu->negative;
    if (!transfer_pending) {
        const QueuedSoundResource resource =
            FindQueuedSoundResource(memory, cpu->data_bank);
        LoadY16(cpu, resource.slot);
        LoadA8(cpu, resource.id);
        if (resource.slot < SOUND_QUEUE_SLOTS) {
            Compare8(cpu, resource.id, 0xffu);
            SoundSaveAccumulator(memory, cpu);
            OpLoadA(cpu, 0xffu);
            OpSta(memory, cpu, OpAbsY(cpu, SOUND_RESOURCE_QUEUE));
            SoundRestoreAccumulator(memory, cpu);
            if (!SoundCall(memory, cpu, child, context, 0x809747u, 0x8097aau))
                return SoundUnwound(0x8097aau);
            transfer_pending = 1u;
        } else {
            Compare16(cpu, resource.slot, SOUND_QUEUE_SLOTS);
        }
        if (!transfer_pending) {
            OpLda(memory, cpu, OpAbs(cpu, SOUND_TRANSFER_STATE));
            if (!cpu->zero) {
                Write8(memory, OpAbs(cpu, SOUND_TRANSFER_STATE), 0u);
                OpLda(memory, cpu, OpAbs(cpu, SOUND_DEFERRED_COMMAND));
                OpCmpValue(cpu, 0xffu);
                if (!cpu->zero) {
                    OpSta(memory, cpu, OpAbs(cpu, SOUND_DATA_PORT));
                    if (!SoundRequest(memory, cpu, child, context, 4u, 0x8097c9u))
                        return SoundUnwound(0x8097ccu);
                }
            }
        }
    }
    if (transfer_pending && !SoundCall(
            memory, cpu, child, context, 0x8097e5u, 0x8097d1u))
        return SoundUnwound(0x8097d1u);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    SoundRestoreAccumulator(memory, cpu);
    return ExecutionReturned(0x8097d9u);
}

Lufia2ExecutionResult Lufia2UploadSoundResourceSlot(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->decimal || cpu->program_bank != 0x80u)
        return ExecutionHandoff(cpu, 0x809886u);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1u);
    SoundSaveAccumulator(memory, cpu);
    if (!SoundRequest(memory, cpu, child, context, 0x14u, 0x80988fu))
        return SoundUnwound(0x809892u);
    SoundRestoreAccumulator(memory, cpu);
    if (!SoundCall(memory, cpu, child, context, 0x8098a5u, 0x809896u))
        return SoundUnwound(0x809896u);
    OpLda(memory, cpu, OpDp(cpu, 0x5fu));
    SoundSaveAccumulator(memory, cpu);
    PullDataBank(memory, cpu);
    if (!SoundCall(memory, cpu, child, context, 0x809945u, 0x80989du))
        return SoundUnwound(0x80989du);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    return ExecutionReturned(0x8098a4u);
}
