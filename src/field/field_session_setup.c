#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "field/field_internal.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    DP_SESSION_UNK_30 = 0x30u,
    DP_SESSION_UNK_31 = 0x31u,
    DP_SESSION_UNK_47 = 0x47u,
    DP_SESSION_UNK_54 = 0x54u,
    DP_SESSION_UNK_6A = 0x6au,
    DP_SESSION_UNK_72 = 0x72u,
    DP_SESSION_UNK_74 = 0x74u,
    DP_SESSION_UNK_81 = 0x81u,
    DP_SESSION_TILE_X = 0x9fu,
    DP_SESSION_TILE_Y = 0xa1u,
    DP_SESSION_UNK_F2 = 0xf2u
};

static Lufia2ExecutionResult PrepareSessionMap(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

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
    return PrepareSessionMap(memory, cpu, child, context);
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
        return PrepareSessionMap(memory, cpu, child, context);
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

static Lufia2ExecutionResult SessionMapChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target, uint8_t frame) {
    Lufia2ExecutionResult result =
        SessionChild(memory, cpu, child, context, site, target, frame);
    if (result.flow == LUFIA2_EXECUTION_RETURNED &&
        !cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, site + frame + 1u);
    return result;
}

static Lufia2ExecutionResult PrepareMapDisplay(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    SessionWidthsAndBank(memory, cpu);
    OpLdx(cpu, 0x0f30u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_UNK_7E0562), cpu->x);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
    OpLoadA(cpu, 0x81u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_NMITIMEN));
    Lufia2ExecutionResult result = SessionMapChild(memory, cpu, child, context,
        0x83b1a4u, 0x83900cu, 2u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpStz(memory, cpu, OpAbs(cpu, SNES_NMITIMEN));
    OpSta(memory, cpu, OpAbs(cpu, SNES_INIDISP));
    OpStz(memory, cpu, OpDp(cpu, DP_SESSION_UNK_81));
    OpStz(memory, cpu, OpAbs(cpu, SNES_HDMAEN));
    OpStz(memory, cpu, OpDp(cpu, DP_SESSION_UNK_72));
    OpStz(memory, cpu, OpDp(cpu, DP_NMI_UPLOAD_FLAGS));
    OpStz(memory, cpu, OpDp(cpu, DP_SESSION_UNK_74));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FIELD_FLAGS));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_FLAGS));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, WRAM_UNK_7FD0BF);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, WRAM_UNK_7FD4E6);
    OpSta(memory, cpu, WRAM_UNK_7FD0FE);
    OpSta(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_INIDISP));
    if (cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x83b1d6u);
    OpLdx(cpu, 7u);
    TransferDirectToA(cpu);
    do {
        OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7FD18C));
        OpDex(cpu);
    } while (!cpu->negative);
    result = SessionMapChild(memory, cpu, child, context,
        0x83b1e1u, 0x848328u, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpDp(cpu, DP_SESSION_UNK_74));
    return SessionMapChild(memory, cpu, child, context,
        0x83b1e9u, 0x83b503u, 2u);
}

static Lufia2ExecutionResult EnterWorldMap(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    Lufia2ExecutionResult result = SessionChild(memory, cpu, child, context,
        0x83b1fdu, 0x83b5adu, 2u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    result = SessionMapChild(memory, cpu, child, context,
        0x83b200u, 0x80be4du, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpLoadA(cpu, 0x80u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_FIELD_RELOAD_FLAGS), 0u);
    if (cpu->zero)
        OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09C9));
    PushAccumulator8(memory, cpu);
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09A9));
    OpStz(memory, cpu, OpDp(cpu, DP_SESSION_UNK_6A));
    result = SessionChild(memory, cpu, child, context,
        0x83b21cu, 0x86919eu, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    result = SessionChild(memory, cpu, child, context,
        0x83b220u, 0x83b062u, 2u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    if (cpu->accumulator_is_8_bit)
        LoadA8(cpu, Pull8(memory, cpu));
    else
        PullAccumulator16(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, WRAM_UNK_7FD100);
    OpSta(memory, cpu, WRAM_UNK_7FD102);
    OpSta(memory, cpu, WRAM_UNK_7FD0C4);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_DESTINATION_MAP));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_RELOAD_FLAGS));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FIELD_TRANSITION_SOURCE_MAP));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E085D));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E089D));
    TransferDirectToA(cpu);
    OpSta(memory, cpu, WRAM_UNK_7FE75A);
    return ExecutionReturned(0x83b251u);
}

static Lufia2ExecutionResult LoadSessionMap(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    OpCmpValue(cpu, 0xf0u);
    Lufia2ExecutionResult result;
    if (cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_TRANSITION_SOURCE_MAP));
        if (!cpu->zero) {
            OpLda(memory, cpu, WRAM_UNK_7FE696);
            if (cpu->zero) {
                OpIncA(cpu);
                OpSta(memory, cpu, WRAM_UNK_7FE696);
            }
            OpLoadA(cpu, 1u);
            OpTestBits(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_FLAGS), 1u);
            result = SessionChild(memory, cpu, child, context,
                0x83b26du, 0x839e31u, 3u);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
        }
    }
    const uint32_t sites[] = {
        0x83b271u, 0x83b275u, 0x83b278u, 0x83b27cu, 0x83b27fu
    };
    const uint32_t targets[] = {
        0x83ac7au, 0x83ab61u, 0x80e844u, 0x83b5adu, 0x83b5d3u
    };
    for (unsigned i = 0u; i < 5u; ++i) {
        result = SessionChild(memory, cpu, child, context,
            sites[i], targets[i], i == 1u || i == 3u ? 2u : 3u);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
    }
    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x83b283u);
    OpLda(memory, cpu, WRAM_FIELD_RELOAD_FLAGS);
    OpAndValue(cpu, 0xdfu);
    OpSta(memory, cpu, WRAM_FIELD_RELOAD_FLAGS);
    result = SessionMapChild(memory, cpu, child, context,
        0x83b28du, 0x80be61u, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpLda(memory, cpu, WRAM_FIELD_RELOAD_FLAGS);
    OpBitValue(cpu, 0x20u);
    if (!cpu->zero) {
        OpAndValue(cpu, 0xdfu);
        OpSta(memory, cpu, WRAM_FIELD_RELOAD_FLAGS);
        return ExecutionReturned(0x83b29fu);
    }
    result = SessionChild(memory, cpu, child, context,
        0x83b2a2u, 0x83a686u, 2u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    result = SessionMapChild(memory, cpu, child, context,
        0x83b2a5u, 0x83b512u, 2u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E079C));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E079D));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_WINDOW_MODE));
    return ExecutionReturned(0x83b2b1u);
}

static Lufia2ExecutionResult InstallSessionPosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    OpStz(memory, cpu, OpAbs(cpu, FIELD_LAYER_SCROLL_SPEED));
    OpStz(memory, cpu, OpAbs(cpu, FIELD_LAYER_SCROLL_SPEED + 1u));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_RELOAD_FLAGS));
    Lufia2ExecutionResult result;
    if (cpu->zero) {
        TransferDirectToA(cpu);
        result = SessionChild(memory, cpu, child, context,
            0x83b2d3u, 0x80c05cu, 3u);
    } else {
        OpBitValue(cpu, 0x40u);
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E05C1));
            result = SessionChild(memory, cpu, child, context,
                0x83b2dcu, 0x80c05cu, 3u);
        } else {
            OpBitValue(cpu, 1u);
            if (!cpu->zero) {
                OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_DESTINATION_X));
                result = SessionChild(memory, cpu, child, context,
                    0x83b2e5u, 0x80c05cu, 3u);
            } else {
                OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_DESTINATION_X));
                OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E120A));
                OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_DESTINATION_X + 1u));
                OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E120B));
                result = ExecutionReturned(0x83b2e9u);
            }
        }
    }
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x83b2e9u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E120A));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E05BD));
    OpSta(memory, cpu, OpDp(cpu, DP_SESSION_TILE_X));
    OpStz(memory, cpu, OpDp(cpu, DP_SESSION_TILE_X + 1u));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E120B));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E05BE));
    OpSta(memory, cpu, OpDp(cpu, DP_SESSION_TILE_Y));
    OpStz(memory, cpu, OpDp(cpu, DP_SESSION_TILE_Y + 1u));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, WRAM_UNK_7FD0FD);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_RELOAD_FLAGS));
    OpBitValue(cpu, 4u);
    return ExecutionReturned(0x83b308u);
}

static Lufia2ExecutionResult InstallSessionActors(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    OpStz(memory, cpu, OpAbs(cpu, WRAM_WINDOW_MODE));
    TransferDirectToA(cpu);
    OpSta(memory, cpu, WRAM_UNK_7FF8A0);
    OpSta(memory, cpu, WRAM_UNK_7FD0FE);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, WRAM_FIELD_UNK_05C0);
    OpSta(memory, cpu, WRAM_UNK_7E05C1);
    Lufia2ExecutionResult result = SessionMapChild(memory, cpu, child, context,
        0x83b323u, 0x80beafu, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpLda(memory, cpu, WRAM_FIELD_DESTINATION_PARAMETERS);
    OpAndValue(cpu, 0x0fu);
    OpCmpValue(cpu, 4u);
    if (cpu->zero) {
        OpLoadA(cpu, 0x71u);
        OpSta(memory, cpu, WRAM_UNK_7FD0FE);
    } else {
        OpCmpValue(cpu, 5u);
        if (cpu->zero) {
            OpLoadA(cpu, 0x72u);
            OpSta(memory, cpu, WRAM_UNK_7FD0FE);
        } else {
            OpCmpValue(cpu, 6u);
            if (cpu->zero) {
                OpLoadA(cpu, 0x73u);
                OpSta(memory, cpu, WRAM_UNK_7FD0FE);
            }
        }
    }
    result = SessionMapChild(memory, cpu, child, context,
        0x83b347u, 0x83b5adu, 2u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpRepWidths(cpu, 0x10u);
    OpLdx(cpu, 0x003eu);
    unsigned actor_passes = 0u;
    do {
        if (!cpu->accumulator_is_8_bit)
            return ExecutionHandoff(cpu, 0x83b34fu);
        if (actor_passes++ == 8192u)
            return ExecutionHandoff(cpu, 0x83b34fu);
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E081E));
        OpCmpValue(cpu, 0xffu);
        if (!cpu->zero) {
            result = SessionChild(memory, cpu, child, context,
                0x83b356u, 0x83ab4fu, 3u);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
            OpLda(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E085E));
            OpCmp(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID));
            if (cpu->zero) {
                OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
                OpLda(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E08DE));
                OpSta(memory, cpu, OpAbsY(cpu, WRAM_UNK_7E05D2));
                OpLda(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E081E));
                if (!cpu->accumulator_is_8_bit)
                    return ExecutionHandoff(cpu, 0x83b36du);
                OpAndValue(cpu, 0x7fu);
                OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E081E));
                OpSta(memory, cpu, OpAbsY(cpu, WRAM_ACTOR_ID));
                OpPushX(memory, cpu);
                OpLda(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E089E));
                OpCmpValue(cpu, 0xffu);
                if (cpu->zero) {
                    OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E120A));
                    OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E120B));
                    OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E120E));
                    OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E120F));
                    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E1210));
                    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E1211));
                    OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E1212));
                } else {
                    result = SessionChild(memory, cpu, child, context,
                        0x83b394u, 0x80c01du, 3u);
                    if (result.flow != LUFIA2_EXECUTION_RETURNED)
                        return result;
                }
                result = SessionChild(memory, cpu, child, context,
                    0x83b398u, 0x80c1a7u, 3u);
                if (result.flow != LUFIA2_EXECUTION_RETURNED)
                    return result;
                OpPullX(memory, cpu);
                OpStepMem(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT), 1);
            }
        }
        OpDex(cpu);
    } while (!cpu->negative);
    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x83b3a2u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E089D));
    OpCmp(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID));
    if (cpu->zero) {
        result = SessionMapChild(memory, cpu, child, context,
            0x83b3aau, 0x8ebc99u, 3u);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
        OpLoadA(cpu, 0xfdu);
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_ID));
        OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E091D));
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E05D2));
        OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E08DD));
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_X));
        OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E085D));
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_Y));
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
        OpAndValue(cpu, 0xfbu);
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
    }
    return ExecutionReturned(0x83b3cfu);
}

static Lufia2ExecutionResult StartSessionMap(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    OpRepWidths(cpu, 0x10u);
    OpLda(memory, cpu, OpDp(cpu, DP_SESSION_TILE_X));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_ACTOR_TILE_X));
    OpLda(memory, cpu, OpDp(cpu, DP_SESSION_TILE_Y));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_ACTOR_TILE_Y));
    OpStz(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT));
    Lufia2ExecutionResult result = SessionMapChild(memory, cpu, child, context,
        0x83b3ddu, 0x83ab4fu, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_DESTINATION_PARAMETERS));
    OpAndValue(cpu, 0xf0u);
    for (unsigned shift = 0u; shift < 4u; ++shift)
        OpLsrA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_ACTOR_FACING));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E05BF));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E066A));
    const uint32_t sites[] = {0x83b3f3u, 0x83b3f7u, 0x83b3fbu};
    const uint32_t targets[] = {0x83a746u, 0x83a82eu, 0x848204u};
    for (unsigned i = 0u; i < 3u; ++i) {
        result = SessionChild(memory, cpu, child, context,
            sites[i], targets[i], 3u);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
    }
    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x83b3ffu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_RELOAD_FLAGS));
    OpBitValue(cpu, 4u);
    if (cpu->zero) {
        result = SessionChild(memory, cpu, child, context,
            0x83b406u, 0x83b53bu, 3u);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
        result = SessionChild(memory, cpu, child, context,
            0x83b40au, 0x80be4du, 3u);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
    }
    OpSepWidths(cpu, 0x30u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_RELOAD_FLAGS));
    OpBitValue(cpu, 1u);
    if (!cpu->zero) {
        OpLoadA(cpu, 1u);
        OpTestBits(memory, cpu, OpAbs(cpu, WRAM_ACTOR_STATE), 1u);
    }
    result = SessionChild(memory, cpu, child, context,
        0x83b41cu, 0x80f821u, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    result = SessionChild(memory, cpu, child, context,
        0x83b420u, 0x80be75u, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    result = SessionChild(memory, cpu, child, context,
        0x83b424u, 0x83b52eu, 2u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    result = SessionMapChild(memory, cpu, child, context,
        0x83b427u, 0x8385dcu, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpLoadA(cpu, 0x81u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_NMITIMEN));
    TransferDirectToA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_SCREEN_EFFECTS));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_PALETTE_FADE));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09AD));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E1271));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_TEXT_WAIT_ACTOR));
    OpSta(memory, cpu, WRAM_UNK_7FD0BF);
    OpLoadA(cpu, 1u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09A9), 1u);
    OpStz(memory, cpu, OpAbs(cpu, FIELD_LAYER_SCROLL_SPEED));
    OpStz(memory, cpu, OpAbs(cpu, FIELD_LAYER_SCROLL_SPEED + 1u));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_RELOAD_FLAGS));
    OpBitValue(cpu, 1u);
    if (!cpu->zero) {
        OpRepWidths(cpu, 0x10u);
        OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E05B8));
        OpLdx(cpu, 4u);
        result = SessionChild(memory, cpu, child, context,
            0x83b460u, 0x80c12eu, 3u);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
        OpWriteX(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09B7), cpu->y);
        OpSepWidths(cpu, 0x20u);
        result = SessionMapChild(memory, cpu, child, context,
            0x83b469u, 0x83bb76u, 2u);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
        OpLoadA(cpu, 4u);
        OpTestBits(memory, cpu, OpAbs(cpu, WRAM_FIELD_RELOAD_FLAGS), 0u);
        result = SessionChild(memory, cpu, child, context,
            0x83b471u, 0x809cb8u, 3u);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
        return ExecutionHandoff(cpu, 0x838000u);
    }
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_RELOAD_FLAGS));
    OpBitValue(cpu, 4u);
    if (cpu->zero) {
        result = SessionMapChild(memory, cpu, child, context,
            0x83b47fu, 0x80c825u, 3u);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
    }
    OpLoadA(cpu, 4u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_FIELD_RELOAD_FLAGS), 0u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_DESTINATION_PARAMETERS));
    OpAndValue(cpu, 0x0fu);
    OpCmpValue(cpu, 1u);
    if (!cpu->zero) {
        OpCmpValue(cpu, 7u);
        if (!cpu->zero) {
            OpRepWidths(cpu, 0x10u);
            result = SessionChild(memory, cpu, child, context,
                0x83b4e9u, 0x83c0efu, 2u);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
            if (cpu->index_is_8_bit)
                return ExecutionHandoff(cpu, 0x83b4ecu);
            OpLdx(cpu, 0u);
            OpLdy(cpu, 0u);
            result = SessionChild(memory, cpu, child, context,
                0x83b4f2u, 0x80e722u, 3u);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
            result = SessionChild(memory, cpu, child, context,
                0x83b4f6u, 0x80cbaeu, 3u);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
            OpSepWidths(cpu, 0x30u);
            result = SessionChild(memory, cpu, child, context,
                0x83b4fcu, 0x83afcdu, 3u);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
            return ExecutionHandoff(cpu, 0x838000u);
        }
        OpRepWidths(cpu, 0x10u);
        OpLdx(cpu, 1u);
        unsigned actor_passes = 0u;
        do {
            if (!cpu->accumulator_is_8_bit)
                return ExecutionHandoff(cpu, 0x83b49au);
            if (actor_passes++ == 8192u)
                return ExecutionHandoff(cpu, 0x83b49au);
            OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
            OpBitValue(cpu, 4u);
            if (cpu->zero) {
                OpWriteX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT), cpu->x);
                result = SessionMapChild(memory, cpu, child, context,
                    0x83b4a3u, 0x83ab4fu, 3u);
                if (result.flow != LUFIA2_EXECUTION_RETURNED)
                    return result;
                OpLoadA(cpu, 0x87u);
                result = SessionChild(memory, cpu, child, context,
                    0x83b4a9u, 0x83d350u, 3u);
                if (result.flow != LUFIA2_EXECUTION_RETURNED)
                    return result;
                OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
            }
            OpInx(cpu);
            if (cpu->index_is_8_bit)
                return ExecutionHandoff(cpu, 0x83b4b0u);
            OpCpx(cpu, 8u);
        } while (!cpu->carry);
        OpSepWidths(cpu, 0x10u);
        if (!cpu->accumulator_is_8_bit)
            return ExecutionHandoff(cpu, 0x83b4b7u);
        OpLoadA(cpu, 0x86u);
    } else {
        OpLoadA(cpu, 0x1eu);
    }
    OpSta(memory, cpu, OpDp(cpu, DP_SESSION_UNK_54));
    OpStz(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT));
    result = SessionChild(memory, cpu, child, context,
        0x83b4c1u, 0x83ab4fu, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpRepWidths(cpu, 0x10u);
    OpLda(memory, cpu, OpDp(cpu, DP_SESSION_UNK_54));
    result = SessionChild(memory, cpu, child, context,
        0x83b4c9u, 0x83d350u, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpSepWidths(cpu, 0x10u);
    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x83b4cfu);
    OpLoadA(cpu, 0x40u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_ACTOR_STATE), 1u);
    result = SessionChild(memory, cpu, child, context,
        0x83b4d4u, 0x83bb93u, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    result = SessionMapChild(memory, cpu, child, context,
        0x83b4d8u, 0x83a21au, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FADE_LEVEL));
    OpLoadA(cpu, 0x88u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FADE_CONTROL));
    return ExecutionHandoff(cpu, 0x838000u);
}

static Lufia2ExecutionResult PrepareSessionMap(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    unsigned map_passes = 0u;
    for (;;) {
        if (map_passes++ == 8192u)
            return ExecutionHandoff(cpu, 0x83b18eu);
        Lufia2ExecutionResult result =
            PrepareMapDisplay(memory, cpu, child, context);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
        OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_RELOAD_FLAGS));
        OpBitValue(cpu, 4u);
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID));
            OpCmpValue(cpu, 2u);
            if (!cpu->carry) {
                result = EnterWorldMap(memory, cpu, child, context);
                if (result.flow != LUFIA2_EXECUTION_RETURNED)
                    return result;
                continue;
            }
            result = LoadSessionMap(memory, cpu, child, context);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
            if (result.pc == 0x83b29fu)
                continue;
        }
        result = InstallSessionPosition(memory, cpu, child, context);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
        if (cpu->zero) {
            result = InstallSessionActors(memory, cpu, child, context);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
        }
        return StartSessionMap(memory, cpu, child, context);
    }
}
