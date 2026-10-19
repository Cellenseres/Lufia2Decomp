#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "field/field_internal.h"
#include "lufia2/field.h"
#include "lufia2/field_phase.h"
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

static Lufia2ExecutionResult RunSessionSelection(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu,
                                                 Lufia2PushedChildCall child,
                                                 void *context) {
    SessionWidthsAndBank(memory, cpu);
    Lufia2ExecutionResult result =
        SessionChild(memory, cpu, child, context, 0x83acf5u, 0x83adcau, 3u);
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

Lufia2ExecutionResult Lufia2FieldBeginSessionSetup(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    if (!child || cpu->program_bank != 0x83u)
        return ExecutionHandoff(cpu, ((uint32_t)cpu->program_bank << 16) | 0xacb7u);
    SessionWidthsAndBank(memory, cpu);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, WRAM_UNK_7E0520);
    Lufia2ExecutionResult result =
        SessionChild(memory, cpu, child, context, 0x83acc3u, 0x83adcau, 3u);
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
    return RunSessionSelection(memory, cpu, child, context);
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

enum {
    DP_TRANSITION_TILEMAP_REQUEST = 0x74u,
    TRANSITION_POLL_LIMIT = 4096u,
    TRANSITION_ACTOR_COUNT = 40u
};

static uint8_t TransitionChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t site,
    uint32_t target, uint8_t frame, Lufia2ExecutionResult *result) {
    const uint8_t accumulator_width = cpu->accumulator_is_8_bit;
    const uint8_t index_width = cpu->index_is_8_bit;
    const uint8_t data_bank = cpu->data_bank;
    const uint16_t direct_page = cpu->direct_page;
    if (!CallChildWithFrame(memory, cpu, child, context,
            site, target, frame, (uint8_t)(site >> 16))) {
        *result = ExecutionReturned(site);
        result->flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return 0u;
    }
    if (cpu->accumulator_is_8_bit != accumulator_width ||
        cpu->index_is_8_bit != index_width || cpu->data_bank != data_bank ||
        cpu->direct_page != direct_page || cpu->decimal) {
        *result = ExecutionHandoff(cpu, site + frame + 1u);
        return 0u;
    }
    return 1u;
}

static uint8_t RunTransitionFade(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t site,
    Lufia2ExecutionResult *result) {
    for (unsigned frame = 0u; frame < TRANSITION_POLL_LIMIT; ++frame) {
        if (!TransitionChild(memory, cpu, child, context,
                site, 0x838192u, 3u, result))
            return 0u;
        OpLda(memory, cpu, OpAbs(cpu, WRAM_FADE_CONTROL));
        if (cpu->zero)
            return 1u;
    }
    *result = ExecutionHandoff(cpu, site);
    return 0u;
}

static uint8_t PollTransitionDisplay(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t site,
    uint16_t address, uint8_t wait_for_negative,
    Lufia2ExecutionResult *result) {
    for (unsigned poll = 0u; poll < TRANSITION_POLL_LIMIT; ++poll) {
        OpLda(memory, cpu, OpAbs(cpu, address));
        if (wait_for_negative ? cpu->negative : cpu->zero)
            return 1u;
    }
    *result = ExecutionHandoff(cpu, site);
    return 0u;
}

Lufia2ExecutionResult Lufia2FieldRunMapTransition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x83u || cpu->data_bank != 0x83u ||
        cpu->direct_page || cpu->decimal || !child)
        return ExecutionHandoff(cpu, 0x8384e1u);
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    OpLoadA(cpu, 1u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_ACTOR_STATE), 1u);
    OpRepWidths(cpu, 0x20u);
    OpStepMem(memory, cpu, OpAbs(cpu, WRAM_UNK_7E0B63), 1);
    if (cpu->zero)
        OpStepMem(memory, cpu, OpAbs(cpu, WRAM_UNK_7E0B63), -1);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, WRAM_FIELD_MOSAIC_STATE);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, WRAM_FIELD_MOSAIC_ACCUMULATOR);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_TRANSITION_FLAGS));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FADE_LEVEL));
    OpLoadA(cpu, 0xc8u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FADE_CONTROL));
    Lufia2ExecutionResult result;
    if (!RunTransitionFade(memory, cpu, child, context, 0x83850fu, &result))
        return result;
    TransferDirectToA(cpu);
    OpSta(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
    OpLoadA(cpu, 8u);
    if (!TransitionChild(memory, cpu, child, context,
            0x83851fu, 0x80953bu, 3u, &result) ||
        !TransitionChild(memory, cpu, child, context,
            0x838523u, 0x83c0efu, 2u, &result))
        return result;
    OpLdx(cpu, 0u);
    OpLdy(cpu, 0x20u);
    if (!TransitionChild(memory, cpu, child, context,
            0x83852cu, 0x80e722u, 3u, &result) ||
        !TransitionChild(memory, cpu, child, context,
            0x838530u, 0x80cbaeu, 3u, &result))
        return result;
    OpLoadA(cpu, 0x14u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_FIELD_RELOAD_FLAGS), 1u);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, WRAM_UNK_7FD0FD);
    static const struct { uint32_t site, target; uint8_t frame; } setup[] = {
        {0x83853fu, 0x83b5d3u, 3u},
        {0x838543u, 0x83a998u, 2u},
        {0x838546u, 0x83b512u, 2u},
        {0x838549u, 0x80beafu, 3u},
        {0x83854du, 0x80be4du, 3u},
        {0x838551u, 0x83b53bu, 3u},
        {0x838555u, 0x80be75u, 3u}
    };
    for (unsigned step = 0u; step < sizeof(setup) / sizeof(setup[0]); ++step)
        if (!TransitionChild(memory, cpu, child, context,
                setup[step].site, setup[step].target, setup[step].frame, &result))
            return result;
    OpLoadA(cpu, 0x1cu);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_FIELD_RELOAD_FLAGS), 0u);
    if (!TransitionChild(memory, cpu, child, context,
            0x83855eu, 0x83a82eu, 3u, &result) ||
        !TransitionChild(memory, cpu, child, context,
            0x838562u, 0x83b52eu, 2u, &result))
        return result;
    OpStz(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT));
    if (!TransitionChild(memory, cpu, child, context,
            0x838567u, 0x83ab4fu, 3u, &result))
        return result;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E05BD));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_ACTOR_TILE_X));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E05BE));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_ACTOR_TILE_Y));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E05BF));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_ACTOR_FACING));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E066A));
    if (!TransitionChild(memory, cpu, child, context,
            0x838580u, 0x83a746u, 3u, &result))
        return result;
    OpLdx(cpu, TRANSITION_ACTOR_COUNT - 1u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_FLAGS));
        OpAndValue(cpu, 0xdfu);
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_FLAGS));
        OpDex(cpu);
    } while (!cpu->negative);
    if (!TransitionChild(memory, cpu, child, context,
            0x838592u, 0x80f821u, 3u, &result) ||
        !TransitionChild(memory, cpu, child, context,
            0x838596u, 0x8385dcu, 3u, &result) ||
        !TransitionChild(memory, cpu, child, context,
            0x83859au, 0x83c0efu, 2u, &result))
        return result;
    OpRepWidths(cpu, 0x10u);
    OpLdx(cpu, 0u);
    OpLdy(cpu, 0u);
    if (!TransitionChild(memory, cpu, child, context,
            0x8385a5u, 0x80e722u, 3u, &result) ||
        !TransitionChild(memory, cpu, child, context,
            0x8385a9u, 0x80cbaeu, 3u, &result))
        return result;
    OpSepWidths(cpu, 0x10u);
    if (!PollTransitionDisplay(memory, cpu, 0x8385afu,
            WRAM_FADE_CONTROL, 0u, &result))
        return result;
    OpLoadA(cpu, 0xf8u);
    OpSta(memory, cpu, WRAM_FIELD_MOSAIC_STATE);
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, WRAM_FIELD_MOSAIC_ACCUMULATOR);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_TRANSITION_FLAGS));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FADE_LEVEL));
    OpLoadA(cpu, 0x88u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FADE_CONTROL));
    if (!RunTransitionFade(memory, cpu, child, context, 0x8385cdu, &result))
        return result;
    OpLoadA(cpu, 1u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_ACTOR_STATE), 0u);
    return ExecutionReturned(0x8385dbu);
}

Lufia2ExecutionResult Lufia2FieldSwitchDisplayScene(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x8eu || !cpu->accumulator_is_8_bit ||
        cpu->direct_page || cpu->decimal || !child)
        return ExecutionHandoff(cpu, 0x8eb28fu);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FADE_LEVEL));
    OpLoadA(cpu, 0xd0u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FADE_CONTROL));
    OpRepWidths(cpu, 0x10u);
    Lufia2ExecutionResult result;
    if (!TransitionChild(memory, cpu, child, context,
            0x8eb299u, 0x80f821u, 3u, &result) ||
        !TransitionChild(memory, cpu, child, context,
            0x8eb29du, 0x80be75u, 3u, &result))
        return result;
    OpLdx(cpu, 0u);
    OpLdy(cpu, 0x14u);
    if (!TransitionChild(memory, cpu, child, context,
            0x8eb2a7u, 0x80e722u, 3u, &result) ||
        !TransitionChild(memory, cpu, child, context,
            0x8eb2abu, 0x80cbaeu, 3u, &result))
        return result;
    OpLdx(cpu, 0u);
    if (!TransitionChild(memory, cpu, child, context,
            0x8eb2b2u, 0x80f47au, 3u, &result))
        return result;
    OpLdx(cpu, 2u);
    if (!TransitionChild(memory, cpu, child, context,
            0x8eb2b9u, 0x80f47au, 3u, &result) ||
        !PollTransitionDisplay(memory, cpu, 0x8eb2bdu,
            WRAM_BRIGHTNESS, 1u, &result))
        return result;
    OpStz(memory, cpu, OpAbs(cpu, SNES_NMITIMEN));
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_INIDISP));
    OpLoadA(cpu, 0xa8u);
    OpSta(memory, cpu, OpDp(cpu, DP_TRANSITION_TILEMAP_REQUEST));
    if (!TransitionChild(memory, cpu, child, context,
            0x8eb2ceu, 0x808285u, 3u, &result))
        return result;
    OpLoadA(cpu, 0x81u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_NMITIMEN));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FADE_LEVEL));
    OpLoadA(cpu, 0x90u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FADE_CONTROL));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_LAYER_TABLE_OFFSET));
    if (!TransitionChild(memory, cpu, child, context,
            0x8eb2e2u, 0x80ed9cu, 3u, &result) ||
        !PollTransitionDisplay(memory, cpu, 0x8eb2e6u,
            WRAM_FADE_CONTROL, 0u, &result))
        return result;
    OpSepWidths(cpu, 0x10u);
    return ExecutionReturned(0x8eb2edu);
}

enum {
    FIELD_LOOP_FRAME_LIMIT = 4096u,
    DP_FIELD_SECOND_PAD_HIGH = DP_BUTTONS_HELD + 3u,
};

Lufia2ExecutionResult Lufia2FieldFrameStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry) {
    static const struct {
        uint32_t site;
        uint32_t target;
        uint8_t frame_bytes;
    } services[] = {
        {0x838004u, 0x83900cu, 2u},
        {0x838031u, 0x8384e1u, 2u},
        {0x83804fu, 0x83afeau, 3u},
        {0x838069u, 0x8eb28fu, 3u},
        {0x838070u, 0x83aeb5u, 2u},
        {0x838073u, 0x809c72u, 3u},
        {0x838077u, 0x8383a0u, 2u},
        {0x83807au, 0x838682u, 2u},
        {0x83807du, 0x83bb93u, 3u},
        {0x838081u, 0x83e03eu, 2u},
        {0x838084u, 0x8ebd77u, 3u},
        {0x838088u, 0x83a21au, 3u},
        {0x83808cu, 0x8381c6u, 2u},
        {0x83808fu, 0x838103u, 2u},
        {0x838099u, 0x80953bu, 3u},
        {0x8380aau, 0x8ebb2eu, 3u},
        {0x8380b0u, 0x83867bu, 2u},
        {0x8380bbu, 0x8380cdu, 2u},
        {0x8380c6u, 0x8eb000u, 3u}
    };
    if (cpu->program_bank != 0x83u || cpu->accumulator_is_8_bit > 1u ||
        cpu->index_is_8_bit > 1u)
        return ExecutionHandoff(cpu, entry);
    for (unsigned service = 0u; service < sizeof services / sizeof services[0]; ++service) {
        if (entry != services[service].site)
            continue;
        const uint32_t target = services[service].target;
        if (services[service].frame_bytes == 3u) {
            SimulateJslFrame(memory, cpu, cpu->program_bank, (uint16_t)(entry + 3u));
            cpu->program_bank = (uint8_t)(target >> 16);
        } else {
            SimulateJsrFrame(memory, cpu, (uint16_t)(entry + 2u));
        }
        return ExecutionHandoff(cpu, target);
    }
    switch (entry) {
    case 0x838000u:
        UnpackStatus(cpu, (uint8_t)(PackStatus(cpu) | 0x30u));
        return ExecutionHandoff(cpu, entry + 2u);
    case 0x838002u:
        Push8(memory, cpu, cpu->program_bank);
        return ExecutionHandoff(cpu, entry + 1u);
    case 0x838003u:
        PullDataBank(memory, cpu);
        return ExecutionHandoff(cpu, entry + 1u);
    case 0x83803du:
        UnpackStatus(cpu, (uint8_t)(PackStatus(cpu) & ~0x20u));
        return ExecutionHandoff(cpu, entry + 2u);
    case 0x83804du:
        UnpackStatus(cpu, (uint8_t)(PackStatus(cpu) | 0x20u));
        return ExecutionHandoff(cpu, entry + 2u);
    case 0x838042u:
        if (cpu->accumulator_is_8_bit)
            PushAccumulator8(memory, cpu);
        else
            PushAccumulator16(memory, cpu);
        return ExecutionHandoff(cpu, entry + 1u);
    case 0x838049u:
        if (cpu->accumulator_is_8_bit)
            LoadA8(cpu, Pull8(memory, cpu));
        else
            PullAccumulator16(memory, cpu);
        return ExecutionHandoff(cpu, entry + 1u);
    case 0x83800fu:
        return ExecutionHandoff(cpu, cpu->zero ? 0x838017u : entry + 2u);
    case 0x838015u:
        return ExecutionHandoff(cpu, cpu->zero ? entry + 2u : 0x838004u);
    case 0x83801cu:
    case 0x838021u:
        return ExecutionHandoff(cpu, cpu->zero ? entry + 2u : 0x838062u);
    case 0x838028u:
    case 0x83803bu:
        return ExecutionHandoff(cpu, cpu->zero ? 0x838062u : entry + 2u);
    case 0x83802fu:
        return ExecutionHandoff(cpu, cpu->zero ? 0x838036u : entry + 2u);
    case 0x838034u:
    case 0x83806du:
    case 0x8380a2u:
    case 0x8380cau:
        return ExecutionHandoff(cpu, 0x838004u);
    case 0x83805fu:
        return ExecutionHandoff(cpu, 0x83b18eu);
    case 0x838067u:
        return ExecutionHandoff(cpu, cpu->zero ? 0x838070u : entry + 2u);
    case 0x838097u:
        return ExecutionHandoff(cpu, cpu->zero ? 0x83809du : entry + 2u);
    case 0x8380a0u:
        return ExecutionHandoff(cpu, cpu->negative ? entry + 2u : 0x8380a5u);
    case 0x8380a8u:
        return ExecutionHandoff(cpu, cpu->zero ? 0x8380aeu : entry + 2u);
    case 0x8380b3u:
        return ExecutionHandoff(cpu, cpu->zero ? 0x8380cau : entry + 2u);
    case 0x8380b9u:
    case 0x8380beu:
    case 0x8380c4u:
        return ExecutionHandoff(cpu, cpu->zero ? entry + 2u : 0x8380cau);
    default:
        break;
    }
    if (!cpu->accumulator_is_8_bit) {
        switch (entry) {
        case 0x83803fu:
            OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID));
            break;
        case 0x838043u:
            OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_DESTINATION_MAP));
            break;
        case 0x838046u:
            OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID));
            break;
        case 0x83804au:
            OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_DESTINATION_MAP));
            break;
        default:
            return ExecutionHandoff(cpu, entry);
        }
        return ExecutionHandoff(cpu, entry + 3u);
    }
    switch (entry) {
    case 0x838007u:
        OpLoadA(cpu, 0xffu);
        break;
    case 0x838009u:
        OpSta(memory, cpu, OpAbs(cpu, WRAM_SOUND_COMMAND));
        return ExecutionHandoff(cpu, entry + 3u);
    case 0x83800cu:
    case 0x8380a5u:
        OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E057C));
        return ExecutionHandoff(cpu, entry + 3u);
    case 0x838011u:
        OpLoadA(cpu, 0x10u);
        break;
    case 0x838013u:
        OpBit(memory, cpu, OpDp(cpu, DP_FIELD_SECOND_PAD_HIGH));
        break;
    case 0x838017u:
        OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_STATE));
        return ExecutionHandoff(cpu, entry + 3u);
    case 0x83801au:
        OpBitValue(cpu, 0x80u);
        break;
    case 0x83801eu:
        OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_REQUESTS));
        return ExecutionHandoff(cpu, entry + 3u);
    case 0x838023u:
        OpLoadA(cpu, 0x80u);
        break;
    case 0x838025u:
    case 0x838038u:
    case 0x838064u:
        OpTestBits(memory, cpu, OpAbs(cpu, WRAM_FIELD_FLAGS), 0u);
        return ExecutionHandoff(cpu, entry + 3u);
    case 0x83802au:
        OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_RELOAD_FLAGS));
        return ExecutionHandoff(cpu, entry + 3u);
    case 0x83802du:
        OpBitValue(cpu, 8u);
        break;
    case 0x838036u:
        OpLoadA(cpu, 8u);
        break;
    case 0x838053u:
        OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E05A8));
        return ExecutionHandoff(cpu, entry + 3u);
    case 0x838056u:
        OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E05A9));
        return ExecutionHandoff(cpu, entry + 3u);
    case 0x838059u:
        OpStz(memory, cpu, OpAbs(cpu, WRAM_SCREEN_EFFECTS));
        return ExecutionHandoff(cpu, entry + 3u);
    case 0x83805cu:
        OpStz(memory, cpu, OpAbs(cpu, WRAM_PALETTE_FADE));
        return ExecutionHandoff(cpu, entry + 3u);
    case 0x838062u:
        OpLoadA(cpu, 0x20u);
        break;
    case 0x838092u:
        OpLda(memory, cpu, OpAbs(cpu, WRAM_SOUND_COMMAND));
        return ExecutionHandoff(cpu, entry + 3u);
    case 0x838095u:
        OpCmpValue(cpu, 0xffu);
        break;
    case 0x83809du:
        OpLda(memory, cpu, OpAbs(cpu, WRAM_TEXT_STATE));
        return ExecutionHandoff(cpu, entry + 3u);
    case 0x8380aeu:
        OpLoadA(cpu, 0x40u);
        break;
    case 0x8380b5u:
        OpLda(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
        return ExecutionHandoff(cpu, entry + 4u);
    case 0x8380c0u:
        OpLda(memory, cpu, WRAM_UNK_7FD0FE);
        return ExecutionHandoff(cpu, entry + 4u);
    default:
        return ExecutionHandoff(cpu, entry);
    }
    return ExecutionHandoff(cpu, entry + 2u);
}

static uint8_t FieldLoopChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target, uint8_t frame,
    Lufia2ExecutionResult *result) {
    if (!CallChildWithFrame(memory, cpu, child, context,
            site, target, frame, 0x83u)) {
        *result = ExecutionReturned(site);
        result->flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return 0u;
    }
    if (!cpu->accumulator_is_8_bit || cpu->data_bank != 0x83u ||
        cpu->direct_page || cpu->decimal) {
        *result = ExecutionHandoff(cpu, site + frame + 1u);
        return 0u;
    }
    return 1u;
}

static uint8_t FieldLoopUpdateServices(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    Lufia2ExecutionResult *result) {
    static const struct { uint32_t site, target; uint8_t frame; } services[] = {
        {0x838070u, 0x83aeb5u, 2u},
        {0x838073u, 0x809c72u, 3u},
        {0x838077u, 0x8383a0u, 2u},
        {0x83807au, 0x838682u, 2u},
        {0x83807du, 0x83bb93u, 3u},
        {0x838081u, 0x83e03eu, 2u},
        {0x838084u, 0x8ebd77u, 3u},
        {0x838088u, 0x83a21au, 3u},
        {0x83808cu, 0x8381c6u, 2u},
        {0x83808fu, 0x838103u, 2u}
    };
    for (unsigned service = 0u;
         service < sizeof(services) / sizeof(services[0]); ++service)
        if (!FieldLoopChild(memory, cpu, child, context,
                services[service].site, services[service].target,
                services[service].frame, result))
            return 0u;
    return 1u;
}

static Lufia2ExecutionResult FieldLoopChangeMap(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID));
    PushAccumulator16(memory, cpu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_DESTINATION_MAP));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID));
    PullAccumulator16(memory, cpu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_DESTINATION_MAP));
    OpSepWidths(cpu, 0x20u);
    Lufia2ExecutionResult result;
    if (!FieldLoopChild(memory, cpu, child, context,
            0x83804fu, 0x83afeau, 3u, &result))
        return result;
    OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E05A8));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E05A9));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_SCREEN_EFFECTS));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_PALETTE_FADE));
    return ExecutionHandoff(cpu, 0x83b18eu);
}

static Lufia2ExecutionResult RunFieldFrames(const Lufia2Memory *memory,
                                            Lufia2CpuState *cpu,
                                            Lufia2PushedChildCall child, void *context,
                                            bool wait_first) {
    Lufia2ExecutionResult result;
    for (unsigned frame = 0u; frame < FIELD_LOOP_FRAME_LIMIT; ++frame) {
        if ((frame || wait_first) && !FieldLoopChild(memory, cpu, child, context,
                                                     0x838004u, 0x83900cu, 2u, &result))
            return result;
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_SOUND_COMMAND));
        OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E057C));
        if (!cpu->zero) {
            OpLoadA(cpu, 0x10u);
            OpBit(memory, cpu, OpDp(cpu, DP_FIELD_SECOND_PAD_HIGH));
            if (!cpu->zero)
                continue;
        }
        OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_STATE));
        OpBitValue(cpu, 0x80u);
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_REQUESTS));
            if (cpu->zero) {
                OpLoadA(cpu, 0x80u);
                OpTestBits(memory, cpu, OpAbs(cpu, WRAM_FIELD_FLAGS), 0u);
                if (!cpu->zero) {
                    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_RELOAD_FLAGS));
                    OpBitValue(cpu, 8u);
                    if (!cpu->zero) {
                        if (!FieldLoopChild(memory, cpu, child, context, 0x838031u,
                                            0x8384e1u, 2u, &result))
                            return result;
                        continue;
                    }
                    OpLoadA(cpu, 8u);
                    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_FIELD_FLAGS), 0u);
                    if (!cpu->zero)
                        return FieldLoopChangeMap(memory, cpu, child, context);
                }
            }
        }
        OpLoadA(cpu, 0x20u);
        OpTestBits(memory, cpu, OpAbs(cpu, WRAM_FIELD_FLAGS), 0u);
        if (!cpu->zero) {
            if (!FieldLoopChild(memory, cpu, child, context, 0x838069u, 0x8eb28fu, 3u,
                                &result))
                return result;
            continue;
        }
        if (!FieldLoopUpdateServices(memory, cpu, child, context, &result))
            return result;
        OpLda(memory, cpu, OpAbs(cpu, WRAM_SOUND_COMMAND));
        OpCmpValue(cpu, 0xffu);
        if (!cpu->zero && !FieldLoopChild(memory, cpu, child, context, 0x838099u,
                                          0x80953bu, 3u, &result))
            return result;
        OpLda(memory, cpu, OpAbs(cpu, WRAM_TEXT_STATE));
        if (cpu->negative)
            continue;
        OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E057C));
        if (!cpu->zero && !FieldLoopChild(memory, cpu, child, context, 0x8380aau,
                                          0x8ebb2eu, 3u, &result))
            return result;
        OpLoadA(cpu, 0x40u);
        if (!FieldLoopChild(memory, cpu, child, context, 0x8380b0u, 0x83867bu, 2u,
                            &result))
            return result;
        if (cpu->zero)
            continue;
        OpLda(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
        if (!cpu->zero)
            continue;
        if (!FieldLoopChild(memory, cpu, child, context, 0x8380bbu, 0x8380cdu, 2u,
                            &result))
            return result;
        if (!cpu->zero)
            continue;
        OpLda(memory, cpu, WRAM_UNK_7FD0FE);
        if (cpu->zero && !FieldLoopChild(memory, cpu, child, context, 0x8380c6u,
                                         0x8eb000u, 3u, &result))
            return result;
    }
    return ExecutionHandoff(cpu, 0x838004u);
}

Lufia2ExecutionResult Lufia2FieldRunLoop(const Lufia2Memory *memory,
                                         Lufia2CpuState *cpu,
                                         Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x83u || cpu->direct_page || cpu->decimal || !child)
        return ExecutionHandoff(cpu, 0x838000u);
    OpSepWidths(cpu, 0x30u);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    return RunFieldFrames(memory, cpu, child, context, true);
}

Lufia2ExecutionResult Lufia2FieldResumeFrame(const Lufia2Memory *memory,
                                             Lufia2CpuState *cpu,
                                             Lufia2PushedChildCall child,
                                             void *context) {
    if (cpu->program_bank != 0x83u || cpu->data_bank != 0x83u || cpu->direct_page ||
        cpu->decimal || !cpu->accumulator_is_8_bit || !child)
        return ExecutionHandoff(cpu, 0x838007u);
    return RunFieldFrames(memory, cpu, child, context, false);
}

Lufia2ExecutionResult Lufia2FieldSelectSessionSetup(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu,
                                                    Lufia2PushedChildCall child,
                                                    void *context) {
    if (!child || cpu->program_bank != 0x83u || cpu->direct_page || cpu->decimal ||
        cpu->stack < 0x1f10u || cpu->stack > 0x1fffu)
        return ExecutionHandoff(cpu, 0x83acefu);
    return RunSessionSelection(memory, cpu, child, context);
}
