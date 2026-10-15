#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "lufia2/system.h"
#include "system/wram.h"
#include "text/text_internal.h"

enum {
    RETURN_TEXT = 0x9db0u,
    RESTART_FIELD = 0xab45u,
    TOGGLE_WINDOW = 0xaddcu,
    FADE_MUSIC = 0xb871u,
    PREPARE_SONG = 0xb887u,
    RESET_SOUND = 0xb8b1u,
    IMMEDIATE_SOUND = 0xb8bdu,
    WAIT_SOUND_VALUE = 0xb8c7u,
    WAIT_SOUND_FLAG = 0xb8dcu,
    RESET_SCREEN_EFFECTS = 0xb94eu,
    LAST_TEXT_SONG = WRAM_UNK_7E099D,
    MUSIC_FADE_FRAMES = 0x7fd0fcu
};

typedef struct TextSoundService {
    const Lufia2Memory *memory;
    uint32_t handoff;
} TextSoundService;

static uint8_t RunSoundChild(void *context, Lufia2CpuState *cpu, uint32_t target,
                             uint32_t site, uint8_t frame) {
    TextSoundService *service = context;
    const Lufia2Memory *memory = service->memory;
    const uint8_t caller_bank = cpu->program_bank;
    cpu->program_bank = (uint8_t)(target >> 16);
    Lufia2ExecutionResult result;
    switch (target) {
    case 0x8096ccu:
        result = Lufia2SoundDriverWrite0A(memory, cpu, RunSoundChild, context);
        break;
    case 0x80969fu:
        result = Lufia2PrepareSongResource(memory, cpu, RunSoundChild, context);
        break;
    case 0x809528u:
        result = Lufia2SoundDriverRequest03(memory, cpu, RunSoundChild, context);
        break;
    case 0x848775u:
        result = Lufia2SendImmediateSound(memory, cpu, RunSoundChild, context);
        break;
    case 0x80953bu:
        result = Lufia2SendSoundCommand(memory, cpu, RunSoundChild, context);
        break;
    case 0x8096f2u:
        result = Lufia2SoundDriverRead08(memory, cpu, RunSoundChild, context);
        break;
    case 0x8095f0u:
        result = Lufia2SoundDriverRead0F(memory, cpu, RunSoundChild, context);
        break;
    case 0x848311u:
        result =
            Lufia2FieldRestoreBackgroundLayers(memory, cpu, RunSoundChild, context);
        break;
    default:
        service->handoff = target;
        return 0u;
    }
    if (result.flow != LUFIA2_EXECUTION_RETURNED) {
        if (!service->handoff)
            service->handoff = result.pc;
        return 0u;
    }
    uint16_t back = Pull8(memory, cpu);
    back |= (uint16_t)Pull8(memory, cpu) << 8;
    cpu->program_bank = frame == 3u ? Pull8(memory, cpu) : caller_bank;
    const uint32_t next = ((uint32_t)cpu->program_bank << 16) | (uint16_t)(back + 1u);
    if (next != site + frame + 1u) {
        service->handoff = next;
        return 0u;
    }
    return 1u;
}

static uint32_t CallSoundService(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                 uint32_t target, uint16_t back) {
    SimulateJslFrame(memory, cpu, 0x80u, back);
    TextSoundService service = {memory, 0u};
    RunSoundChild(&service, cpu, target, 0x800000u | (uint16_t)(back - 3u), 3u);
    return service.handoff;
}

static uint32_t FadeMusic(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                          uint16_t handler) {
    uint32_t handoff;
    if (handler == FADE_MUSIC) {
        OpLoadA(cpu, 0x3cu);
        OpSta(memory, cpu, MUSIC_FADE_FRAMES);
        handoff = CallSoundService(memory, cpu, 0x8096ccu, 0xb87au);
        if (handoff)
            return handoff;
        handoff = CallSoundService(memory, cpu, 0x809692u, 0xb87eu);
    } else {
        handoff = CallSoundService(memory, cpu, 0x809528u, 0xb8b4u);
    }
    if (handoff)
        return handoff;
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, LAST_TEXT_SONG));
    return LUFIA2_TEXT_COMMAND_NEXT;
}

static uint32_t PrepareSong(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2TextNextByte(memory, cpu, 0xb889u);
    OpCmp(memory, cpu, OpAbs(cpu, LAST_TEXT_SONG));
    if (!cpu->zero) {
        OpSta(memory, cpu, OpAbs(cpu, LAST_TEXT_SONG));
        uint32_t handoff = CallSoundService(memory, cpu, 0x80969fu, 0xb895u);
        if (handoff)
            return handoff;
        handoff = CallSoundService(memory, cpu, 0x848311u, 0xb899u);
        if (handoff)
            return handoff;
    }
    return LUFIA2_TEXT_COMMAND_NEXT;
}

static uint32_t ImmediateSound(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2TextNextByte(memory, cpu, 0xb8bfu);
    const uint32_t handoff = CallSoundService(memory, cpu, 0x848775u, 0xb8c3u);
    return handoff ? handoff : LUFIA2_TEXT_COMMAND_NEXT;
}

static uint32_t WaitSound(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                          uint16_t handler) {
    const bool flag = handler == WAIT_SOUND_FLAG;
    const uint32_t handoff = CallSoundService(memory, cpu, flag ? 0x8095f0u : 0x8096f2u,
                                              flag ? 0xb8dfu : 0xb8cau);
    if (handoff)
        return handoff;
    if (flag) {
        OpAndValue(cpu, 2u);
        if (cpu->zero)
            return LUFIA2_TEXT_COMMAND_NEXT;
    } else {
        OpCmp(memory, cpu, OpAbsY(cpu, 0u));
        if (cpu->carry) {
            Lufia2TextNextByte(memory, cpu, 0xb8d8u);
            return LUFIA2_TEXT_COMMAND_NEXT;
        }
    }
    Lufia2TextPrevByte(memory, cpu, flag ? 0xb8e9u : 0xb8d2u);
    return LUFIA2_TEXT_COMMAND_YIELD;
}

static uint32_t ToggleWindow(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLoadA(cpu, 0x80u);
    OpTestBits(memory, cpu, OpAbs(cpu, TEXT_WINDOW_STATE), 1u);
    if (cpu->zero)
        return LUFIA2_TEXT_COMMAND_NEXT;
    OpStz(memory, cpu, OpAbs(cpu, TEXT_WINDOW_STATE));
    Lufia2TextReleaseActors(memory, cpu, 0x9da8u);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_TEXT_STATE));
    SimulateJslFrame(memory, cpu, 0x80u, 0x9dafu);
    return 0x80ef2fu;
}

static uint32_t ResetScreenEffects(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SimulateJslFrame(memory, cpu, 0x80u, 0xb951u);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_SCREEN_EFFECTS));
    OpRepWidths(cpu, 0x20u);
    LoadA16(cpu, cpu->direct_page);
    OpSta(memory, cpu, WRAM_FIELD_SCREEN_OFFSET_X);
    OpSta(memory, cpu, WRAM_FIELD_SCREEN_OFFSET_Y);
    OpSepWidths(cpu, 0x20u);
    SimulateRtlFrame(memory, cpu);
    return LUFIA2_TEXT_COMMAND_NEXT;
}

uint32_t Lufia2TextExecuteSystemCommand(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                        uint16_t handler) {
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal ||
        cpu->stack < 0x1f10u || cpu->stack > 0x1ffcu)
        return 0u;
    switch (handler) {
    case RETURN_TEXT:
        return LUFIA2_TEXT_COMMAND_YIELD;
    case RESTART_FIELD:
        OpRepWidths(cpu, 0x20u);
        OpLoadA(cpu, 0x1fffu);
        cpu->stack = cpu->accumulator;
        OpSepWidths(cpu, 0x20u);
        cpu->program_bank = 0x83u;
        return 0x83acefu;
    case TOGGLE_WINDOW:
        return ToggleWindow(memory, cpu);
    case FADE_MUSIC:
    case RESET_SOUND:
        return FadeMusic(memory, cpu, handler);
    case PREPARE_SONG:
        return PrepareSong(memory, cpu);
    case IMMEDIATE_SOUND:
        return ImmediateSound(memory, cpu);
    case WAIT_SOUND_VALUE:
    case WAIT_SOUND_FLAG:
        return WaitSound(memory, cpu, handler);
    case RESET_SCREEN_EFFECTS:
        return ResetScreenEffects(memory, cpu);
    default:
        return 0u;
    }
}
