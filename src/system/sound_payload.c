#include "core/cpu_ops.h"
#include "lufia2/system.h"

enum {
    SOUND_PAYLOAD_RESOURCE = 0x56u,
    SOUND_PAYLOAD_BASE = 0x5du,
    SOUND_PAYLOAD_BANK = 0x5fu,
    SOUND_PAYLOAD_PORT = 0x2140u
};

static void SoundAdvancePayload(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpIny(cpu);
    if (cpu->zero) {
        OpLdy(cpu, 0x8000u);
        OpStepMem(memory, cpu, OpDp(cpu, SOUND_PAYLOAD_BANK), 1);
    }
}

static Lufia2ExecutionResult SoundPayloadUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2UploadSoundPayload(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->program_bank != 0x80u || cpu->decimal || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x809911u);
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 0u);
    OpLda(memory, cpu, OpDp(cpu, SOUND_PAYLOAD_RESOURCE));
    OpSta(memory, cpu, OpAbs(cpu, SOUND_PAYLOAD_PORT));
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, SOUND_PAYLOAD_BASE)));
    OpStz(memory, cpu, OpDp(cpu, SOUND_PAYLOAD_BASE));
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, SOUND_PAYLOAD_BASE));
    OpTax(cpu);
    SetAccumulatorWidth(cpu, 1u);
    SoundAdvancePayload(memory, cpu);
    SoundAdvancePayload(memory, cpu);
    OpLoadA(cpu, 1u);
    SimulateJsrFrame(memory, cpu, 0x9937u);
    Lufia2WriteSoundDriverMode(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    SimulateJsrFrame(memory, cpu, 0x993au);
    if (!child(context, cpu, 0x809a0au, 0x809938u, 2u))
        return SoundPayloadUnwound(0x809938u);
    OpLda(memory, cpu, OpDp(cpu, SOUND_PAYLOAD_BANK));
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SimulateJsrFrame(memory, cpu, 0x9941u);
    if (!child(context, cpu, 0x809945u, 0x80993fu, 2u))
        return SoundPayloadUnwound(0x80993fu);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x809944u);
}
