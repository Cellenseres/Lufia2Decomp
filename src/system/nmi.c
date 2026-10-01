/* Main NMI body, play clock and interrupt exit. */

#include "core/cpu_ops.h"
#include "lufia2/system.h"
#include "system/wram.h"

enum { NMI_FRAME_COUNTER = 0x40, NMI_FRAME_COUNTER_SECONDARY = 0x42 };

static uint8_t NmiChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t site, uint32_t target) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    return child(context, cpu, target, site, 2u);
}

static void NmiTickPlayTime(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
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

Lufia2ExecutionResult Lufia2MainNmi(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, Lufia2ExecutionCheckpoint checkpoint,
    void *context) {
    if (!child || cpu->decimal)
        return ExecutionHandoff(cpu, 0x808638u);
    OpSepWidths(cpu, 0x30u);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, 0x2100u));
    OpLda(memory, cpu, OpAbs(cpu, 0x0081u));
    if (!cpu->zero)
        OpStz(memory, cpu, OpAbs(cpu, 0x420cu));
#define NMI_CALL(site, target)                                                   \
    do {                                                                        \
        if (!NmiChild(memory, cpu, child, context, site, target)) {               \
            Lufia2ExecutionResult result = {                                    \
                LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};                       \
            return result;                                                      \
        }                                                                       \
    } while (0)
    NMI_CALL(0x808649u, 0x808703u);
    OpLda(memory, cpu, OpDp(cpu, 0x6au));
    if (!cpu->zero)
        NMI_CALL(0x808650u, 0x800067u);
    OpLda(memory, cpu, OpDp(cpu, 0x6fu));
    if (!cpu->zero)
        NMI_CALL(0x808657u, 0x80006cu);
    OpLda(memory, cpu, OpDp(cpu, 0x71u));
    if (!cpu->zero)
        NMI_CALL(0x80865eu, 0x8087a7u);
    NMI_CALL(0x808661u, 0x8086c1u);
#undef NMI_CALL
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
    OpSta(memory, cpu, OpAbs(cpu, 0x2100u));
    NmiTickPlayTime(memory, cpu);
    if (checkpoint)
        checkpoint(context, cpu, 0x808699u);
    OpRepWidths(cpu, 0x20u);
    OpStepMem(memory, cpu, OpDp(cpu, NMI_FRAME_COUNTER), 1);
    OpStepMem(memory, cpu, OpDp(cpu, NMI_FRAME_COUNTER_SECONDARY), 1);
    OpLda(memory, cpu, 0x000520u);
    OpAndValue(cpu, 0x00ffu);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, 0x46u));
        OpCmpValue(cpu, 0x3030u);
        OpSepWidths(cpu, 0x20u);
        if (cpu->zero)
            return ExecutionHandoff(cpu, 0x8086b1u);
    }
    OpLda(memory, cpu, OpAbs(cpu, 0x4210u));
    OpRepWidths(cpu, 0x30u);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    PullAccumulator16(memory, cpu);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionHandoff(cpu, 0x8086bfu);
}
