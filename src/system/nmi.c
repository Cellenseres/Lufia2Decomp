/* Main NMI body, play clock and interrupt exit. */

#include <stddef.h>

#include "core/cpu_ops.h"
#include "lufia2/system.h"
#include "system/wram.h"

enum {
    NMI_FRAME_COUNTER_SECONDARY = 0x42u,
    NMI_HDMA_CHANNELS = 0x0081u,
    DP_NMI_PRIMARY_CALLBACK_BANK = 0x6au,
    DP_NMI_SECONDARY_CALLBACK_BANK = 0x6fu,
    DP_NMI_SCROLL_UPLOADS = 0x71u,
    NMI_RESET_CONTROL = 0x000520u
};

static uint8_t NmiChild(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                        Lufia2PushedChildCall child, void *context, uint32_t site,
                        uint32_t target) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    return child(context, cpu, target, site, 2u);
}

static void NmiTickPlayTime(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, WRAM_PLAY_TIME_FRAMES));
    if (cpu->negative)
        return;
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_PLAY_TIME_FRAMES));
    OpCmpValue(cpu, 60u);
    if (!cpu->carry)
        return;
    OpStz(memory, cpu, OpAbs(cpu, WRAM_PLAY_TIME_FRAMES));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_PLAY_TIME_SECONDS));
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_PLAY_TIME_SECONDS));
    OpCmpValue(cpu, 60u);
    if (!cpu->carry)
        return;
    OpStz(memory, cpu, OpAbs(cpu, WRAM_PLAY_TIME_SECONDS));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_PLAY_TIME_MINUTES));
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_PLAY_TIME_MINUTES));
    OpCmpValue(cpu, 60u);
    if (!cpu->carry)
        return;
    OpStz(memory, cpu, OpAbs(cpu, WRAM_PLAY_TIME_MINUTES));
    OpStepMem(memory, cpu, OpAbs(cpu, WRAM_PLAY_TIME_HOURS), 1);
}

static void PrepareNmi(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSepWidths(cpu, 0x30u);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_INIDISP));
    OpLda(memory, cpu, OpAbs(cpu, NMI_HDMA_CHANNELS));
    if (!cpu->zero)
        OpStz(memory, cpu, OpAbs(cpu, SNES_HDMAEN));
}

static void DisplayNmiAndTickPlayTime(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
    OpSta(memory, cpu, OpAbs(cpu, SNES_INIDISP));
    NmiTickPlayTime(memory, cpu);
}

static Lufia2ExecutionResult RestoreNmiFrame(const Lufia2Memory *memory,
                                             Lufia2CpuState *cpu) {
    OpStepMem(memory, cpu, OpDp(cpu, DP_NMI_FRAME_COUNTER), 1);
    OpStepMem(memory, cpu, OpDp(cpu, NMI_FRAME_COUNTER_SECONDARY), 1);
    OpLda(memory, cpu, NMI_RESET_CONTROL);
    OpAndValue(cpu, 0x00ffu);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, DP_BUTTONS_HELD));
        OpCmpValue(cpu, 0x3030u);
        OpSepWidths(cpu, 0x20u);
        if (cpu->zero)
            return ExecutionHandoff(cpu, 0x8086b1u);
    }
    OpLda(memory, cpu, OpAbs(cpu, SNES_RDNMI));
    OpRepWidths(cpu, 0x30u);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    PullAccumulator16(memory, cpu);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionHandoff(cpu, 0x8086bfu);
}

static Lufia2ExecutionResult FinishNmi(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                       Lufia2ExecutionCheckpoint checkpoint,
                                       void *context) {
    DisplayNmiAndTickPlayTime(memory, cpu);
    if (checkpoint)
        checkpoint(context, cpu, 0x808699u);
    OpRepWidths(cpu, 0x20u);
    return RestoreNmiFrame(memory, cpu);
}

typedef enum NmiPhase {
    NMI_PREPARE,
    NMI_PRIMARY_CALLBACK,
    NMI_SECONDARY_CALLBACK,
    NMI_SCROLL_UPLOADS,
    NMI_SCREEN_FADE,
    NMI_FINISH,
    NMI_INVALID_PHASE
} NmiPhase;

typedef struct NmiChildCall {
    uint32_t site;
    uint32_t target;
} NmiChildCall;

static NmiPhase NmiPhaseAt(uint32_t pc) {
    switch (pc) {
    case 0x808638u:
        return NMI_PREPARE;
    case 0x80864cu:
        return NMI_PRIMARY_CALLBACK;
    case 0x808653u:
        return NMI_SECONDARY_CALLBACK;
    case 0x80865au:
        return NMI_SCROLL_UPLOADS;
    case 0x808661u:
        return NMI_SCREEN_FADE;
    case 0x808664u:
        return NMI_FINISH;
    default:
        return NMI_INVALID_PHASE;
    }
}

static NmiChildCall SelectNmiChild(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                   NmiPhase phase) {
    switch (phase) {
    case NMI_PREPARE:
        PrepareNmi(memory, cpu);
        return (NmiChildCall){0x808649u, 0x808703u};
    case NMI_PRIMARY_CALLBACK:
        OpLda(memory, cpu, OpDp(cpu, DP_NMI_PRIMARY_CALLBACK_BANK));
        if (!cpu->zero)
            return (NmiChildCall){0x808650u, 0x800067u};
        break;
    case NMI_SECONDARY_CALLBACK:
        OpLda(memory, cpu, OpDp(cpu, DP_NMI_SECONDARY_CALLBACK_BANK));
        if (!cpu->zero)
            return (NmiChildCall){0x808657u, 0x80006cu};
        break;
    case NMI_SCROLL_UPLOADS:
        OpLda(memory, cpu, OpDp(cpu, DP_NMI_SCROLL_UPLOADS));
        if (!cpu->zero)
            return (NmiChildCall){0x80865eu, 0x8087a7u};
        break;
    case NMI_SCREEN_FADE:
        return (NmiChildCall){0x808661u, 0x8086c1u};
    default:
        break;
    }
    return (NmiChildCall){0u, 0u};
}

static Lufia2ExecutionResult RunNmiPhases(const Lufia2Memory *memory,
                                          Lufia2CpuState *cpu, NmiPhase phase,
                                          Lufia2PushedChildCall child,
                                          Lufia2ExecutionCheckpoint checkpoint,
                                          void *context) {
    for (; phase < NMI_FINISH; ++phase) {
        const NmiChildCall call = SelectNmiChild(memory, cpu, phase);
        if (!call.target)
            continue;
        if (!child)
            return ExecutionHandoff(cpu, call.site);
        if (!NmiChild(memory, cpu, child, context, call.site, call.target)) {
            Lufia2ExecutionResult result = ExecutionReturned(call.site);
            result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
            return result;
        }
        if (!cpu->accumulator_is_8_bit)
            return ExecutionHandoff(cpu, call.site + 3u);
    }
    return FinishNmi(memory, cpu, checkpoint, context);
}

Lufia2ExecutionResult Lufia2MainNmi(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                    Lufia2PushedChildCall child,
                                    Lufia2ExecutionCheckpoint checkpoint,
                                    void *context) {
    if (!child || cpu->decimal)
        return ExecutionHandoff(cpu, 0x808638u);
    return RunNmiPhases(memory, cpu, NMI_PREPARE, child, checkpoint, context);
}

Lufia2ExecutionResult Lufia2MainNmiResume(const Lufia2Memory *memory,
                                          Lufia2CpuState *cpu, uint32_t entry,
                                          Lufia2ExecutionCheckpoint checkpoint,
                                          void *context) {
    const NmiPhase phase = NmiPhaseAt(entry);
    const bool byte_mode = phase != NMI_PREPARE;
    if (phase == NMI_INVALID_PHASE || cpu->program_bank != 0x80u || cpu->decimal ||
        cpu->direct_page || cpu->accumulator_is_8_bit != byte_mode ||
        cpu->index_is_8_bit != byte_mode || cpu->stack < 0x1f00u ||
        cpu->stack > 0x1ff3u)
        return ExecutionHandoff(cpu, entry);
    return RunNmiPhases(memory, cpu, phase, NULL, checkpoint, context);
}

static bool NmiFrameSliceSupported(const Lufia2CpuState *cpu, bool byte_mode) {
    return cpu->program_bank == 0x80u && cpu->data_bank == 0x80u && !cpu->decimal &&
           !cpu->direct_page && cpu->index_is_8_bit == 1u &&
           cpu->accumulator_is_8_bit == byte_mode && cpu->stack >= 0x1f00u &&
           cpu->stack <= 0x1ff3u;
}

Lufia2ExecutionResult Lufia2NmiDisplayAndClock(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu) {
    if (!NmiFrameSliceSupported(cpu, true))
        return ExecutionHandoff(cpu, 0x808664u);
    DisplayNmiAndTickPlayTime(memory, cpu);
    return ExecutionHandoff(cpu, 0x808699u);
}

Lufia2ExecutionResult Lufia2NmiRestoreInterruptFrame(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu) {
    if (!NmiFrameSliceSupported(cpu, false))
        return ExecutionHandoff(cpu, 0x80869bu);
    return RestoreNmiFrame(memory, cpu);
}

Lufia2ExecutionResult Lufia2NmiSaveInterruptFrame(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu) {
    const uint32_t entry = ((uint32_t)cpu->program_bank << 16) | 0x862du;
    if ((cpu->program_bank != 0u && cpu->program_bank != 0x80u) ||
        cpu->accumulator_is_8_bit > 1u || cpu->index_is_8_bit > 1u ||
        cpu->stack < 0x1f08u || cpu->stack > 0x1ffbu)
        return ExecutionHandoff(cpu, entry);
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    OpRepWidths(cpu, 0x30u);
    PushAccumulator16(memory, cpu);
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    cpu->program_bank = 0x80u;
    return ExecutionHandoff(cpu, 0x808638u);
}
