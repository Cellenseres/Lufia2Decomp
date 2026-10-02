#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"

static void BeginFieldReload(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    Push8(memory, cpu, 0x83u);
    PullDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    OpStz(memory, cpu, OpAbs(cpu, SNES_NMITIMEN));
    TransferDirectToA(cpu);
    OpSta(memory, cpu, 0x7fd0ffu);
    OpSta(memory, cpu, OpAbs(cpu, 0x0562u));
    OpSta(memory, cpu, OpAbs(cpu, 0x0563u));
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
    OpSta(memory, cpu, OpAbs(cpu, SNES_INIDISP));
    TransferDirectToA(cpu);
    OpSta(memory, cpu, OpDp(cpu, 0x6au));
    OpSta(memory, cpu, OpDp(cpu, 0x6fu));
    OpStz(memory, cpu, OpDp(cpu, 0x72u));
    OpStz(memory, cpu, OpDp(cpu, 0x74u));
    OpStz(memory, cpu, OpDp(cpu, DP_NMI_UPLOAD_FLAGS));
    OpSta(memory, cpu, OpAbs(cpu, 0x1255u));
    OpSta(memory, cpu, OpAbs(cpu, 0x1256u));
    OpSta(memory, cpu, OpDp(cpu, 0x81u));
    OpSta(memory, cpu, OpAbs(cpu, SNES_HDMAEN));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, 0x7fd0b0u);
    OpSta(memory, cpu, 0x7fd4f5u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_SOUND_COMMAND));
    const uint16_t cleared[] = {WRAM_TEXT_STATE, 0x099cu, WRAM_SCREEN_EFFECTS,
        WRAM_PALETTE_FADE, 0x1254u, 0x09a6u, 0x09adu};
    for (unsigned i = 0u; i < 4u; ++i)
        OpStz(memory, cpu, OpAbs(cpu, cleared[i]));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_TEXT_WAIT_ACTOR));
    for (unsigned i = 4u; i < 7u; ++i)
        OpStz(memory, cpu, OpAbs(cpu, cleared[i]));
}

Lufia2ExecutionResult Lufia2FieldReloadSetup(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    BeginFieldReload(memory, cpu);
    return ExecutionHandoff(cpu, 0x838637u);
}

static Lufia2ExecutionResult ReloadChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target, uint8_t frame) {
    if (frame == 3u)
        SimulateJslFrame(memory, cpu, 0x83u, (uint16_t)(site + 3u));
    else
        SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    if (!child(context, cpu, target, site, frame)) {
        Lufia2ExecutionResult result = ExecutionReturned(site);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    return ExecutionReturned(site + frame + 1u);
}

Lufia2ExecutionResult Lufia2FieldReloadMap(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->program_bank != 0x83u)
        return ExecutionHandoff(cpu, ((uint32_t)cpu->program_bank << 16) | 0x85dcu);
    BeginFieldReload(memory, cpu);
    const uint32_t sites[] = {0x838637u, 0x83863au, 0x83863eu,
        0x838642u, 0x838645u, 0x838649u};
    const uint32_t targets[] = {0x83b062u, 0x83b5d3u, 0x8eba81u,
        0x83ab61u, 0x80ef8eu, 0x83a76du};
    const uint8_t frames[] = {2u, 3u, 3u, 2u, 3u, 2u};
    Lufia2ExecutionResult result;
    for (unsigned i = 0u; i < 6u; ++i) {
        result = ReloadChild(memory, cpu, child, context, sites[i], targets[i], frames[i]);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
    }
    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x83864cu);
    OpLoadA(cpu, 1u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09A9), 1u);
    result = ReloadChild(memory, cpu, child, context, 0x838651u, 0x8eb09cu, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    result = ReloadChild(memory, cpu, child, context, 0x838655u, 0x838e66u, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x838659u);
    OpLoadA(cpu, 0xa8u);
    OpSta(memory, cpu, OpDp(cpu, 0x74u));
    result = ReloadChild(memory, cpu, child, context, 0x83865du, 0x808285u, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_LAYER_TABLE_OFFSET));
    result = ReloadChild(memory, cpu, child, context, 0x838664u, 0x80ed9cu, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x838668u);
    OpLoadA(cpu, 0x81u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_NMITIMEN));
    result = ReloadChild(memory, cpu, child, context, 0x83866du, 0x83a21au, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x838673u);
}
