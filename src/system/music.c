/* Original song/sample upload and music-driver command wrappers. */
#include <stdbool.h>

#include "core/cpu_ops.h"
#include "lufia2/system.h"
#include "system/wram.h"

enum {
    MUSIC_RESOURCE_ID = 0x54u,
    MUSIC_SAMPLE_ID = 0x55u,
    MUSIC_UPLOAD_DESTINATION = 0x56u,
    MUSIC_SAMPLE_COUNT = 0x58u,
    MUSIC_UPLOAD_SOURCE = 0x5du,
    MUSIC_UPLOAD_SOURCE_BANK = 0x5fu,
    MUSIC_RESOURCE_DESTINATION = 0x60u,
    MUSIC_RESOURCE_DESTINATION_BANK = 0x62u,
    MUSIC_SAMPLE_LOOKUP = 0x9a34u,
    MUSIC_SAMPLE_VALUES = 0x9a59u,
    MUSIC_APU_PORT = 0x2140u
};

static void MusicPushA(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit)
        PushAccumulator8(memory, cpu);
    else
        PushAccumulator16(memory, cpu);
}

static void MusicPullA(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit)
        LoadA8(cpu, Pull8(memory, cpu));
    else
        PullAccumulator16(memory, cpu);
}

static Lufia2ExecutionResult MusicUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static uint8_t MusicChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target, uint8_t frame) {
    if (frame == 3u)
        SimulateJslFrame(memory, cpu, cpu->program_bank, (uint16_t)(site + 3u));
    else
        SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    return child(context, cpu, target, site, frame);
}

#define MUSIC_CALL(site, target, frame) \
    do { \
        if (!MusicChild(memory, cpu, child, context, site, target, frame)) \
            return MusicUnwound(site); \
    } while (0)

/* Sends one sample to the sound driver: command $13 with the sample value in
 * A, the two handshake children, then the upload child. Returns 0, or the
 * call site of the child that unwound. */
static uint32_t MusicSendSample(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                Lufia2PushedChildCall child, void *context,
                                uint32_t site, bool load_resource_id) {
    MusicPushA(memory, cpu);
    if (load_resource_id)
        OpLda(memory, cpu, OpDp(cpu, MUSIC_RESOURCE_ID));
    OpSta(memory, cpu, OpAbs(cpu, MUSIC_APU_PORT));
    OpLoadA(cpu, 0x13u);
    if (!MusicChild(memory, cpu, child, context, site, 0x8099fdu, 2u))
        return site;
    if (!MusicChild(memory, cpu, child, context, site + 3u, 0x809a0au, 2u))
        return site + 3u;
    MusicPullA(memory, cpu);
    if (!MusicChild(memory, cpu, child, context, site + 7u, 0x809886u, 2u))
        return site + 7u;
    return 0u;
}

/* Marks every cache slot empty ($FF) and restarts the sample count. */
static void MusicClearSampleCache(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushIndex(memory, cpu);
    OpLdx(cpu, 31u);
    OpLoadA(cpu, 0xffu);
    do {
        OpSta(memory, cpu, OpLongX(cpu, WRAM_MUSIC_SAMPLE_CACHE));
        OpDex(cpu);
    } while (!cpu->negative);
    OpPullX(memory, cpu);
    OpStz(memory, cpu, OpDp(cpu, MUSIC_SAMPLE_COUNT));
    OpStz(memory, cpu, OpDp(cpu, MUSIC_SAMPLE_COUNT + 1u));
}

/* Uploads the song's sample list: first the fixed resources that the list
 * flags with bit 7, then each distinct sample value that is not cached yet. */
static Lufia2ExecutionResult MusicLoadSamples(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpDp(cpu, MUSIC_RESOURCE_ID));
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpLongX(cpu, WRAM_MUSIC_SAMPLE_LIST));
        OpBitValue(cpu, 0x80u);
        if (!cpu->zero)
            break;
        const uint32_t unwound =
            MusicSendSample(memory, cpu, child, context, 0x809496u, true);
        if (unwound)
            return MusicUnwound(unwound);
        OpStepMem(memory, cpu, OpDp(cpu, MUSIC_RESOURCE_ID), 1);
        OpInx(cpu);
        OpCpx(cpu, 32u);
    } while (!cpu->zero);
    if (cpu->zero)
        return ExecutionReturned(0x809506u);

    MusicClearSampleCache(memory, cpu);
    do {
        OpLda(memory, cpu, OpLongX(cpu, WRAM_MUSIC_SAMPLE_LIST));
        OpCmpValue(cpu, 0xffu);
        if (cpu->zero)
            break;
        OpAndValue(cpu, 0x7fu);
        OpSta(memory, cpu, OpDp(cpu, MUSIC_SAMPLE_ID));
        PushIndex(memory, cpu);
        OpLdy(cpu, 0u);
        for (;;) {
            OpLda(memory, cpu, OpAbsY(cpu, MUSIC_SAMPLE_LOOKUP));
            if (cpu->zero)
                break;
            OpCmp(memory, cpu, OpDp(cpu, MUSIC_SAMPLE_ID));
            if (cpu->zero) {
                OpLda(memory, cpu, OpAbsY(cpu, MUSIC_SAMPLE_VALUES));
                OpLdx(cpu, 31u);
                do {
                    OpCmp(memory, cpu, OpLongX(cpu, WRAM_MUSIC_SAMPLE_CACHE));
                    if (cpu->zero)
                        break;
                    OpDex(cpu);
                } while (!cpu->negative);
                if (cpu->negative) {
                    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, MUSIC_SAMPLE_COUNT)));
                    OpSta(memory, cpu, OpLongX(cpu, WRAM_MUSIC_SAMPLE_CACHE));
                    OpStepMem(memory, cpu, OpDp(cpu, MUSIC_SAMPLE_COUNT), 1);
                    const uint32_t unwound =
                        MusicSendSample(memory, cpu, child, context, 0x8094f5u, false);
                    if (unwound)
                        return MusicUnwound(unwound);
                }
                break;
            }
            OpIny(cpu);
        }
        OpPullX(memory, cpu);
        OpInx(cpu);
        OpCpx(cpu, 32u);
    } while (!cpu->zero);
    return ExecutionReturned(0x809506u);
}

Lufia2ExecutionResult Lufia2LoadSong(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, Lufia2MusicCheckpoint checkpoint, void *context) {
    Lufia2ExecutionResult result;
    if (!child || cpu->decimal)
        return ExecutionHandoff(cpu, 0x80941au);
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    MusicPushA(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    MUSIC_CALL(0x809421u, 0x809528u, 3u);
    MUSIC_CALL(0x809425u, 0x8095d2u, 3u);
    MusicPullA(memory, cpu);
    OpCmpValue(cpu, 100u);
    if (cpu->carry) {
        cpu->carry = 1;
        return ExecutionReturned(0x80944du);
    }
    if (checkpoint && !checkpoint(context, cpu, 0x80942eu))
        return MusicUnwound(0x80942eu);
    OpSta(memory, cpu, OpDp(cpu, MUSIC_RESOURCE_ID));
    OpStz(memory, cpu, OpDp(cpu, MUSIC_SAMPLE_ID));
    OpLdx(cpu, 0x2020u);
    OpWriteX(memory, cpu, OpDp(cpu, MUSIC_RESOURCE_DESTINATION), cpu->x);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, MUSIC_RESOURCE_DESTINATION_BANK));
    MUSIC_CALL(0x80943bu, 0x808e9du, 3u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, WRAM_MUSIC_SONG_HEADER + 2u);
    OpCmpValue(cpu, 0x3253u);
    OpSepWidths(cpu, 0x20u);
    if (!cpu->zero) {
        cpu->carry = 1;
        return ExecutionReturned(0x80944du);
    }
    OpLda(memory, cpu, WRAM_MUSIC_SONG_HEADER + 0x1au);
    if (!cpu->zero) {
        OpSta(memory, cpu, OpAbs(cpu, WRAM_MUSIC_SAMPLE_UPLOAD_MODE));
        OpLdx(cpu, 0x5800u);
        OpWriteX(memory, cpu, OpAbs(cpu, MUSIC_APU_PORT), cpu->x);
        OpLoadA(cpu, 0x11u);
        MUSIC_CALL(0x80945fu, 0x8099fdu, 2u);
        MUSIC_CALL(0x809462u, 0x809a0au, 2u);
    } else {
        OpLoadA(cpu, 0xffu);
        OpTestBits(memory, cpu, OpAbs(cpu, WRAM_MUSIC_SAMPLE_UPLOAD_MODE), 0u);
        if (!cpu->zero) {
            MUSIC_CALL(0x80946eu, 0x809703u, 2u);
        }
        OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MUSIC_APU_UPLOAD_ADDRESS)));
        OpWriteX(memory, cpu, OpAbs(cpu, MUSIC_APU_PORT), cpu->x);
        OpLoadA(cpu, 0x11u);
        MUSIC_CALL(0x809479u, 0x8099fdu, 2u);
        MUSIC_CALL(0x80947cu, 0x809a0au, 2u);
    }
    result = MusicLoadSamples(memory, cpu, child, context);
    if (result.flow == LUFIA2_EXECUTION_CHILD_UNWOUND)
        return result;
    OpLdx(cpu, 0x2020u);
    OpWriteX(memory, cpu, OpDp(cpu, MUSIC_UPLOAD_SOURCE), cpu->x);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, MUSIC_UPLOAD_SOURCE_BANK));
    OpLdx(cpu, 0x4400u);
    OpWriteX(memory, cpu, OpDp(cpu, MUSIC_UPLOAD_DESTINATION), cpu->x);
    MUSIC_CALL(0x809514u, 0x809911u, 3u);
    OpLoadA(cpu, 0x12u);
    MUSIC_CALL(0x80951au, 0x8099fdu, 2u);
    MUSIC_CALL(0x80951du, 0x809a0au, 2u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, MUSIC_APU_PORT)));
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MUSIC_APU_REPLY), cpu->x);
    cpu->carry = 0;
    return ExecutionReturned(0x809527u);
}

Lufia2ExecutionResult Lufia2PlaySong(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->decimal)
        return ExecutionHandoff(cpu, 0x8093feu);
    MusicPushA(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    MUSIC_CALL(0x809407u, 0x80941au, 2u);
    if (!cpu->carry) {
        OpLoadA(cpu, 2u);
        MUSIC_CALL(0x80940eu, 0x8099fdu, 2u);
        MUSIC_CALL(0x809411u, 0x809a0au, 2u);
    }
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    MusicPullA(memory, cpu);
    return ExecutionReturned(0x809419u);
}

Lufia2ExecutionResult Lufia2FadeOutMusic(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, Lufia2MusicCheckpoint checkpoint, void *context) {
    if (!child || cpu->decimal)
        return ExecutionHandoff(cpu, 0x809692u);
    if (checkpoint && !checkpoint(context, cpu, 0x809692u))
        return MusicUnwound(0x809692u);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 6u);
    MUSIC_CALL(0x809697u, 0x8099fdu, 2u);
    MUSIC_CALL(0x80969au, 0x809a0au, 2u);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x80969eu);
}

Lufia2ExecutionResult Lufia2SetMusicVolume(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->decimal)
        return ExecutionHandoff(cpu, 0x809601u);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, MUSIC_APU_PORT);
    OpLoadA(cpu, 0x0bu);
    MUSIC_CALL(0x80960au, 0x8099fdu, 2u);
    MUSIC_CALL(0x80960du, 0x809a0au, 2u);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x809611u);
}
