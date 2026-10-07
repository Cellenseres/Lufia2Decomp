#include "core/cpu_ops.h"
#include "lufia2/system.h"

enum {
    SOUND_DATA_PORT = 0x2140u,
    SOUND_SOURCE_POSITION = 0x0589u,
    SOUND_SOURCE_BANK = 0x058bu,
    SOUND_SOURCE_LENGTH = 0x058cu,
    SOUND_TRANSFER_STATE = 0x058eu,
    SOUND_DEFERRED_COMMAND = 0x058fu,
    SOUND_RESOURCE_QUEUE = 0x0590u,
    SOUND_QUEUE_SLOTS = 4u
};

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
    OpWriteX(memory, cpu, OpAbs(cpu, SOUND_SOURCE_LENGTH), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, SOUND_SOURCE_POSITION), cpu->y);
    OpLda(memory, cpu, OpDp(cpu, 0x5fu));
    OpSta(memory, cpu, OpAbs(cpu, SOUND_SOURCE_BANK));
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
        OpLdy(cpu, 0u);
        do {
            OpLda(memory, cpu, OpAbsY(cpu, SOUND_RESOURCE_QUEUE));
            OpCmpValue(cpu, 0xffu);
            if (!cpu->zero) {
                SoundSaveAccumulator(memory, cpu);
                OpLoadA(cpu, 0xffu);
                OpSta(memory, cpu, OpAbsY(cpu, SOUND_RESOURCE_QUEUE));
                SoundRestoreAccumulator(memory, cpu);
                if (!SoundCall(memory, cpu, child, context, 0x809747u, 0x8097aau))
                    return SoundUnwound(0x8097aau);
                transfer_pending = 1u;
                break;
            }
            OpIny(cpu);
            OpCpy(cpu, SOUND_QUEUE_SLOTS);
        } while (!cpu->zero);
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
