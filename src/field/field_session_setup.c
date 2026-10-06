#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "field/field_internal.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    DP_SESSION_UNK_30 = 0x30u,
    DP_SESSION_UNK_31 = 0x31u,
    DP_SESSION_UNK_47 = 0x47u,
    DP_SESSION_UNK_6A = 0x6au,
    DP_SESSION_UNK_74 = 0x74u,
    DP_SESSION_UNK_81 = 0x81u,
    DP_SESSION_UNK_F2 = 0xf2u
};

static Lufia2ExecutionResult SessionChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target, uint8_t frame) {
    if (!CallChildWithFrame(memory, cpu, child, context,
            site, target, frame, 0x83u)) {
        Lufia2ExecutionResult result = ExecutionReturned(site);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    return ExecutionReturned(site + frame + 1u);
}

static void SessionWidthsAndBank(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
}

static void SessionDestination(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t map, uint8_t parameter, uint8_t position) {
    OpLoadA(cpu, map);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FIELD_DESTINATION_PARAMETERS));
    OpLoadA(cpu, parameter);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E05B8));
    OpLoadA(cpu, position);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_DESTINATION_X));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_RELOAD_FLAGS));
}

Lufia2ExecutionResult Lufia2FieldResumeSessionSetup(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->program_bank != 0x83u ||
        !cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu,
            ((uint32_t)cpu->program_bank << 16) | 0xad23u);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, WRAM_FIELD_UNK_05C0);
    OpSta(memory, cpu, WRAM_UNK_7E05C1);
    OpSta(memory, cpu, WRAM_UNK_7FD0BF);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, WRAM_UNK_7FD0FE);
    OpSta(memory, cpu, WRAM_UNK_7FE75A);
    const uint32_t sites[] = {
        0x83ad3au, 0x83ad3eu, 0x83ad41u, 0x83ad45u, 0x83ad48u
    };
    const uint32_t targets[] = {
        0x80e844u, 0x83b5adu, 0x83b5d3u, 0x83ab61u, 0x83a82eu
    };
    Lufia2ExecutionResult result;
    for (unsigned i = 0u; i < 5u; ++i) {
        result = SessionChild(memory, cpu, child, context,
            sites[i], targets[i], i == 1u || i == 3u ? 2u : 3u);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
    }
    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x83ad4cu);
    OpLoadA(cpu, 0x10u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_FIELD_RELOAD_FLAGS), 1u);
    result = SessionChild(memory, cpu, child, context,
        0x83ad51u, 0x80beafu, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x83ad55u);
    OpLoadA(cpu, 0x10u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_FIELD_RELOAD_FLAGS), 0u);
    result = SessionChild(memory, cpu, child, context,
        0x83ad5au, 0x83b53bu, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    result = SessionChild(memory, cpu, child, context,
        0x83ad5eu, 0x80f821u, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E099D));
    result = SessionChild(memory, cpu, child, context,
        0x83ad65u, 0x8093feu, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpStz(memory, cpu, OpAbs(cpu, FIELD_LAYER_SCROLL_SPEED));
    OpStz(memory, cpu, OpAbs(cpu, FIELD_LAYER_SCROLL_SPEED + 1u));
    result = SessionChild(memory, cpu, child, context,
        0x83ad6fu, 0x8385dcu, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    result = SessionChild(memory, cpu, child, context,
        0x83ad73u, 0x83afcdu, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    return ExecutionHandoff(cpu, 0x838000u);
}

static Lufia2ExecutionResult SelectSessionDestination(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    Lufia2ExecutionResult result = SessionChild(memory, cpu, child, context,
        0x83ad7au, 0x83addfu, 2u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    result = SessionChild(memory, cpu, child, context,
        0x83ad7du, 0x83a686u, 2u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x83ad80u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E0B51));
    OpBitValue(cpu, 2u);
    if (!cpu->zero) {
        OpLoadA(cpu, 7u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E0A7F));
        OpLoadA(cpu, 1u);
        result = SessionChild(memory, cpu, child, context,
            0x83ad8eu, 0x82c352u, 3u);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
        if (!cpu->accumulator_is_8_bit)
            return ExecutionHandoff(cpu, 0x83ad92u);
        SessionDestination(memory, cpu, 0x68u, 2u, 3u);
    } else {
        SessionDestination(memory, cpu, 3u, 2u, 3u);
    }
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E099D));
    return ExecutionHandoff(cpu, 0x83b18eu);
}

Lufia2ExecutionResult Lufia2FieldBeginSessionSetup(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->program_bank != 0x83u)
        return ExecutionHandoff(cpu,
            ((uint32_t)cpu->program_bank << 16) | 0xacb7u);
    SessionWidthsAndBank(memory, cpu);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, WRAM_UNK_7E0520);
    Lufia2ExecutionResult result = SessionChild(memory, cpu, child, context,
        0x83acc3u, 0x83adcau, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x83acc7u);
    OpLda(memory, cpu, OpDp(cpu, DP_SESSION_UNK_47));
    OpBitValue(cpu, 0x80u);
    if (cpu->zero) {
        OpLoadA(cpu, 2u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID));
        OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_DESTINATION_MAP));
        OpStz(memory, cpu, OpAbs(cpu, WRAM_FIELD_DESTINATION_PARAMETERS));
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E05B8));
        OpLoadA(cpu, 6u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_DESTINATION_X));
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_RELOAD_FLAGS));
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E099D));
        return ExecutionHandoff(cpu, 0x83b18eu);
    }
    /* Selection repeats the original initialization. */
    SessionWidthsAndBank(memory, cpu);
    result = SessionChild(memory, cpu, child, context,
        0x83acf5u, 0x83adcau, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    result = SessionChild(memory, cpu, child, context,
        0x83acf9u, 0x83b062u, 2u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x83acfcu);
    OpStz(memory, cpu, OpDp(cpu, DP_SESSION_UNK_74));
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
    OpStz(memory, cpu, OpDp(cpu, DP_SESSION_UNK_6A));
    OpStz(memory, cpu, OpDp(cpu, DP_SESSION_UNK_81));
    OpStz(memory, cpu, OpAbs(cpu, SNES_HDMAEN));
    OpStz(memory, cpu, OpDp(cpu, DP_SESSION_UNK_F2));
    OpLoadA(cpu, 0x81u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_NMITIMEN));
    result = SessionChild(memory, cpu, child, context,
        0x83ad11u, 0x83900cu, 2u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpStz(memory, cpu, OpDp(cpu, DP_SESSION_UNK_30));
    OpStz(memory, cpu, OpDp(cpu, DP_SESSION_UNK_31));
    result = SessionChild(memory, cpu, child, context,
        0x83ad18u, 0x82e746u, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x83ad1cu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E0B51));
    OpBitValue(cpu, 0x80u);
    if (cpu->zero)
        return SelectSessionDestination(memory, cpu, child, context);
    if (cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x83ad23u);
    return Lufia2FieldResumeSessionSetup(memory, cpu, child, context);
}
