#include "lufia2/world_map.h"
#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"

enum {
    WORLD_SONG = WRAM_UNK_7E099D,
    WORLD_VEHICLE_KIND = WRAM_UNK_7E09E0,
    WORLD_VEHICLE_ACTIVE = WRAM_UNK_7E09E1,
    WORLD_PLAYER_OBJECT = WRAM_WORLD_MAP_PLAYER_OBJECT,
    WORLD_ACTIVE_OBJECT = WRAM_BATTLE_CONTROL_FLAGS,
    WORLD_VEHICLE_PHASE = WRAM_FIELD_STREAMED_ROW_VRAM + 1u,
    WORLD_MOVE_PHASE = WRAM_WORLD_MAP_MOVE_PHASE,
    WORLD_RETURN_OBJECT = WRAM_WORLD_MAP_OBJECT_TABLE + 0x1d,
    OBJECT_X = 0x02,
    OBJECT_Y = 0x05,
    OBJECT_POSE = 0x0f,
    OBJECT_ANIMATION = 0x10,
    OBJECT_MOVING = 0x15,
    ROM_VEHICLE_SONGS = 0x86a410,
    DP_BUTTON_HIGH = 0x47,
    DP_TILE_CLASS = 0x54,
    DP_TARGET_X = 0x58,
    DP_TARGET_Y = 0x5a
};

static uint8_t PlayerContext(const Lufia2CpuState *cpu,
    Lufia2PushedChildCall child) {
    return cpu->program_bank == 0x86u && cpu->data_bank == 0x86u &&
        !cpu->direct_page && !cpu->decimal &&
        cpu->accumulator_is_8_bit && !cpu->index_is_8_bit && child;
}

static uint32_t PlayerObjectField(const Lufia2CpuState *cpu, uint8_t field) {
    return DirectIndexedAddress(cpu, field, cpu->x) | OP_DP_WRAP;
}

static uint8_t PlayerChild(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target, uint8_t frame,
    Lufia2ExecutionResult *result) {
    if (!CallChildWithFrame(memory, cpu, child, context,
            site, target, frame, 0x86u)) {
        *result = (Lufia2ExecutionResult){
            LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return 0u;
    }
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit) {
        *result = ExecutionHandoff(cpu, site + frame + 1u);
        return 0u;
    }
    return 1u;
}

Lufia2ExecutionResult Lufia2WorldMapSetPlayerSong(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    Lufia2ExecutionResult result;
    if (!PlayerContext(cpu, child))
        return ExecutionHandoff(cpu, 0x86d35du);
    OpPushX(memory, cpu);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_TARGET_X)));
    OpPushX(memory, cpu);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_TARGET_Y)));
    OpPushX(memory, cpu);
    OpCmp(memory, cpu, OpAbs(cpu, WORLD_SONG));
    if (!cpu->zero) {
        OpSta(memory, cpu, OpAbs(cpu, WORLD_SONG));
        if (!PlayerChild(memory, cpu, child, context,
                0x86d36cu, 0x8093feu, 3u, &result))
            return result;
    }
    OpPullX(memory, cpu);
    OpWriteX(memory, cpu, OpDp(cpu, DP_TARGET_Y), cpu->x);
    OpPullX(memory, cpu);
    OpWriteX(memory, cpu, OpDp(cpu, DP_TARGET_X), cpu->x);
    OpPullX(memory, cpu);
    return ExecutionReturned(0x86d377u);
}

Lufia2ExecutionResult Lufia2WorldMapActivateVehicle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    Lufia2ExecutionResult result;
    if (!PlayerContext(cpu, child))
        return ExecutionHandoff(cpu, 0x86a1c5u);
    OpLda(memory, cpu, OpAbs(cpu, WORLD_ACTIVE_OBJECT));
    if (!PlayerChild(memory, cpu, child, context,
            0x86a1c8u, 0x86e709u, 2u, &result))
        return result;
    OpLdy(cpu, 0xffffu);
    OpWriteX(memory, cpu, PlayerObjectField(cpu, OBJECT_Y), cpu->y);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WORLD_ACTIVE_OBJECT));
    OpStz(memory, cpu, OpAbs(cpu, WORLD_VEHICLE_PHASE));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WORLD_VEHICLE_ACTIVE));
    OpSepWidths(cpu, 0x10u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WORLD_VEHICLE_KIND)));
    OpLda(memory, cpu, OpLongX(cpu, ROM_VEHICLE_SONGS));
    OpRepWidths(cpu, 0x10u);
    if (!PlayerChild(memory, cpu, child, context,
            0x86a1e8u, 0x86d35du, 2u, &result))
        return result;
    return ExecutionReturned(0x86a1ebu);
}

Lufia2ExecutionResult Lufia2WorldMapRestorePlayerObject(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    Lufia2ExecutionResult result;
    if (!PlayerContext(cpu, child))
        return ExecutionHandoff(cpu, 0x86a1ecu);
    OpLda(memory, cpu, OpAbs(cpu, WORLD_PLAYER_OBJECT));
    OpSta(memory, cpu, OpAbs(cpu, WORLD_ACTIVE_OBJECT));
    if (!PlayerChild(memory, cpu, child, context,
            0x86a1f2u, 0x86e709u, 2u, &result))
        return result;
    OpLdy(cpu, OpReadX(memory, cpu,
        OpAbs(cpu, WORLD_RETURN_OBJECT + OBJECT_X)));
    OpWriteX(memory, cpu, PlayerObjectField(cpu, OBJECT_X), cpu->y);
    OpLdy(cpu, OpReadX(memory, cpu,
        OpAbs(cpu, WORLD_RETURN_OBJECT + OBJECT_Y)));
    OpWriteX(memory, cpu, PlayerObjectField(cpu, OBJECT_Y), cpu->y);
    OpLda(memory, cpu, OpAbs(cpu, WORLD_RETURN_OBJECT + OBJECT_POSE));
    OpSta(memory, cpu, PlayerObjectField(cpu, OBJECT_POSE));
    OpLda(memory, cpu, PlayerObjectField(cpu, OBJECT_ANIMATION));
    if (!PlayerChild(memory, cpu, child, context,
            0x86a206u, 0x86e0b9u, 2u, &result))
        return result;
    if (!PlayerChild(memory, cpu, child, context,
            0x86a209u, 0x86a2c6u, 2u, &result))
        return result;
    OpStz(memory, cpu, OpAbs(cpu, WORLD_VEHICLE_ACTIVE));
    OpStz(memory, cpu, OpAbs(cpu, WORLD_MOVE_PHASE));
    OpLoadA(cpu, 0u);
    if (!PlayerChild(memory, cpu, child, context,
            0x86a214u, 0x86d35du, 2u, &result))
        return result;
    return ExecutionReturned(0x86a217u);
}

static Lufia2ExecutionResult AcceptPlayerMove(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    Lufia2ExecutionResult result;
    if (!PlayerChild(memory, cpu, child, context,
            0x8698f7u, 0x86992bu, 2u, &result))
        return result;
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, PlayerObjectField(cpu, OBJECT_MOVING));
    cpu->carry = 0u;
    return ExecutionReturned(0x8698ffu);
}

static Lufia2ExecutionResult CheckPlayerMove(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    Lufia2ExecutionResult result;
    if (!PlayerChild(memory, cpu, child, context,
            0x8698e0u, 0x869ab8u, 2u, &result))
        return result;
    if (!cpu->carry) {
        PushY(memory, cpu);
        if (!PlayerChild(memory, cpu, child, context,
                0x8698e6u, 0x869a71u, 2u, &result))
            return result;
        OpSta(memory, cpu, OpDp(cpu, DP_TILE_CLASS));
        OpPullY(memory, cpu);
    }
    OpLda(memory, cpu, OpAbs(cpu, WORLD_VEHICLE_ACTIVE));
    if (cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, DP_TILE_CLASS));
        OpCmpValue(cpu, 0x30u);
        if (!cpu->carry)
            return AcceptPlayerMove(memory, cpu, child, context);
    } else {
        OpLda(memory, cpu, OpDp(cpu, DP_TILE_CLASS));
        OpCmpValue(cpu, 0x40u);
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, WORLD_VEHICLE_KIND));
            if (!cpu->zero)
                return AcceptPlayerMove(memory, cpu, child, context);
            OpLda(memory, cpu, OpDp(cpu, DP_TILE_CLASS));
            OpCmpValue(cpu, 0x30u);
            if (!cpu->zero) {
                PushY(memory, cpu);
                if (!PlayerChild(memory, cpu, child, context,
                        0x869912u, 0x86992bu, 2u, &result))
                    return result;
                if (!PlayerChild(memory, cpu, child, context,
                        0x869915u, 0x86a1ecu, 2u, &result))
                    return result;
                OpPullY(memory, cpu);
            }
            return AcceptPlayerMove(memory, cpu, child, context);
        }
    }
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E057C));
    if (!cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, DP_BUTTON_HIGH));
        OpBitValue(cpu, 0x80u);
        if (!cpu->zero)
            return AcceptPlayerMove(memory, cpu, child, context);
    }
    if (!PlayerChild(memory, cpu, child, context,
            0x869926u, 0x86992bu, 2u, &result))
        return result;
    cpu->carry = 1u;
    return ExecutionReturned(0x86992au);
}

static Lufia2ExecutionResult MovePlayerTile(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context,
    uint32_t entry, uint16_t heading, uint8_t coordinate, int direction) {
    if (!PlayerContext(cpu, child))
        return ExecutionHandoff(cpu, entry);
    OpLdy(cpu, heading);
    OpStepMem(memory, cpu, OpDp(cpu, coordinate), direction);
    return CheckPlayerMove(memory, cpu, child, context);
}

Lufia2ExecutionResult Lufia2WorldMapMovePlayerNorth(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return MovePlayerTile(memory, cpu, child, context,
        0x8698c6u, 0x0000u, DP_TARGET_Y, -1);
}

Lufia2ExecutionResult Lufia2WorldMapMovePlayerWest(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return MovePlayerTile(memory, cpu, child, context,
        0x8698cdu, 0x4000u, DP_TARGET_X, -1);
}

Lufia2ExecutionResult Lufia2WorldMapMovePlayerSouth(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return MovePlayerTile(memory, cpu, child, context,
        0x8698d4u, 0x8000u, DP_TARGET_Y, 1);
}

Lufia2ExecutionResult Lufia2WorldMapMovePlayerEast(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return MovePlayerTile(memory, cpu, child, context,
        0x8698dbu, 0xc000u, DP_TARGET_X, 1);
}
