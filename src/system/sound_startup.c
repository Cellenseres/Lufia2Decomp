#include "lufia2/system.h"
#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"

enum {
    BOOT_SOUND_SOURCE_CURSOR = 0x5d,
    BOOT_SOUND_SOURCE_BANK = 0x5f,
    BOOT_SOUND_DATA = 0x2140,
    BOOT_SOUND_PARAMETER = 0x2141,
    BOOT_SOUND_MARKER_LOW = 0x2142,
    BOOT_SOUND_MARKER_HIGH = 0x2143,
    BOOT_JOYPAD = 0x4218,
    SOUND_START_POLL_LIMIT = 65536
};

static uint8_t BootSoundPortReady(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t port) {
    for (uint32_t polls = 0u; polls < SOUND_START_POLL_LIMIT; ++polls) {
        OpCmp(memory, cpu, port);
        if (cpu->zero)
            return 1u;
    }
    return 0u;
}

static uint8_t BootSoundChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target, uint8_t frame,
    Lufia2ExecutionResult *result) {
    if (CallChildWithFrame(memory, cpu, child, context,
            site, target, frame, 0x80u))
        return 1u;
    *result = (Lufia2ExecutionResult){
        LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
    return 0u;
}

Lufia2ExecutionResult Lufia2InitializeBootSound(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x80u || !child)
        return ExecutionHandoff(cpu, 0x809383u);
    Push8(memory, cpu, PackStatus(cpu));
    Push8(memory, cpu, cpu->data_bank);
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpLdx(cpu, 0xc000u);
    OpWriteX(memory, cpu, OpDp(cpu, BOOT_SOUND_SOURCE_CURSOR), cpu->x);
    OpLoadA(cpu, 0x90u);
    OpSta(memory, cpu, OpDp(cpu, BOOT_SOUND_SOURCE_BANK));
    Lufia2ExecutionResult result;
    if (!BootSoundChild(memory, cpu, child, context,
            0x809394u, 0x809c0fu, 2u, &result))
        return result;
    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x809397u);
    OpLoadA(cpu, 0xabu);
    if (!BootSoundPortReady(memory, cpu, BOOT_SOUND_DATA))
        return ExecutionHandoff(cpu, 0x809399u);
    OpLoadA(cpu, 0xcdu);
    if (!BootSoundPortReady(memory, cpu, BOOT_SOUND_PARAMETER))
        return ExecutionHandoff(cpu, 0x8093a1u);
    OpLoadA(cpu, 0x67u);
    OpSta(memory, cpu, BOOT_SOUND_MARKER_LOW);
    OpLoadA(cpu, 0x89u);
    OpSta(memory, cpu, BOOT_SOUND_MARKER_HIGH);
    if (!BootSoundChild(memory, cpu, child, context,
            0x8093b3u, 0x809a0au, 2u, &result))
        return result;
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, BOOT_JOYPAD);
    OpCmpValue(cpu, 0x3000u);
    OpSepWidths(cpu, 0x20u);
    if (cpu->zero) {
        OpLoadA(cpu, 0u);
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 1u));
    } else {
        OpLoadA(cpu, 0u);
    }
    if (!BootSoundChild(memory, cpu, child, context,
            0x8093cbu, 0x809623u, 3u, &result) ||
        !BootSoundChild(memory, cpu, child, context,
            0x8093cfu, 0x8093d6u, 3u, &result))
        return result;
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x8093d5u);
}

Lufia2ExecutionResult Lufia2ResetSoundResources(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x80u || !child)
        return ExecutionHandoff(cpu, 0x8093d6u);
    if (cpu->accumulator_is_8_bit)
        PushAccumulator8(memory, cpu);
    else
        PushAccumulator16(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    Push8(memory, cpu, cpu->data_bank);
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_SOUND_PARAMETER_INHIBIT));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_SOUND_DEFERRED_COMMAND));
    OpLdx(cpu, 0xffffu);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_SOUND_RESOURCE_OUTPUT), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_SOUND_RESOURCE_OUTPUT + 2u), cpu->x);
    Lufia2ExecutionResult result;
    if (!BootSoundChild(memory, cpu, child, context,
            0x8093f2u, 0x809703u, 2u, &result))
        return result;
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MUSIC_SAMPLE_UPLOAD_MODE));
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    if (cpu->accumulator_is_8_bit)
        LoadA8(cpu, Pull8(memory, cpu));
    else
        PullAccumulator16(memory, cpu);
    return ExecutionReturned(0x8093fdu);
}
