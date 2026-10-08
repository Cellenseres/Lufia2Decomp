#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/battle.h"
#include "lufia2/battle.h"

enum {
    DP_ACTOR_COMMAND_STREAM = 0xc3u,
    ACTOR_SPAWN_SCRIPT = WRAM_BATTLE_EFFECT_SPAWN_STREAM,
    ACTOR_SPAWN_HORIZONTAL = WRAM_BATTLE_EFFECT_SPAWN_POSITION,
    ACTOR_SPAWN_VERTICAL = WRAM_BATTLE_EFFECT_SPAWN_POSITION + 1u,
    ACTOR_SPAWN_ANGLE = WRAM_BATTLE_EFFECT_SPAWN_ATTRIBUTES
};

typedef enum ActorSpawnPreset {
    ACTOR_SPAWN_DEFAULT,
    ACTOR_SPAWN_HALF_TURN,
    ACTOR_SPAWN_STREAM_ANGLE,
    ACTOR_SPAWN_DIAGONAL,
    ACTOR_SPAWN_RIGHTWARD,
    ACTOR_SPAWN_UPWARD,
    ACTOR_SPAWN_STREAM_VERTICAL
} ActorSpawnPreset;

static uint8_t ActorCommandContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x81u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && cpu->direct_page == 0u &&
        cpu->stack >= 0x1f02u && cpu->stack <= 0x1ffcu;
}

static void AdvanceActorCommand(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpStepMem(memory, cpu, OpDp(cpu, DP_ACTOR_COMMAND_STREAM), 1);
}

static void ReadActorSpawnScript(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_ACTOR_COMMAND_STREAM));
    AdvanceActorCommand(memory, cpu);
    AdvanceActorCommand(memory, cpu);
    OpSta(memory, cpu, OpAbs(cpu, ACTOR_SPAWN_SCRIPT));
}

static void SetActorSpawnPreset(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, ActorSpawnPreset preset) {
    if (preset == ACTOR_SPAWN_STREAM_ANGLE ||
        preset == ACTOR_SPAWN_STREAM_VERTICAL) {
        OpLda(memory, cpu,
            DirectLongPointer(memory, cpu, DP_ACTOR_COMMAND_STREAM));
        AdvanceActorCommand(memory, cpu);
        OpSepWidths(cpu, 0x20u);
        OpSta(memory, cpu, OpAbs(cpu, preset == ACTOR_SPAWN_STREAM_ANGLE
            ? ACTOR_SPAWN_ANGLE : ACTOR_SPAWN_VERTICAL));
        OpStz(memory, cpu, OpAbs(cpu, ACTOR_SPAWN_HORIZONTAL));
        OpStz(memory, cpu, OpAbs(cpu, preset == ACTOR_SPAWN_STREAM_ANGLE
            ? ACTOR_SPAWN_VERTICAL : ACTOR_SPAWN_ANGLE));
        return;
    }
    OpSepWidths(cpu, 0x20u);
    if (preset == ACTOR_SPAWN_DIAGONAL) {
        LoadA8(cpu, 0x32u);
        OpSta(memory, cpu, OpAbs(cpu, ACTOR_SPAWN_HORIZONTAL));
        OpSta(memory, cpu, OpAbs(cpu, ACTOR_SPAWN_VERTICAL));
        LoadA8(cpu, 0x20u);
        OpSta(memory, cpu, OpAbs(cpu, ACTOR_SPAWN_ANGLE));
    } else if (preset == ACTOR_SPAWN_RIGHTWARD) {
        LoadA8(cpu, 0x10u);
        OpSta(memory, cpu, OpAbs(cpu, ACTOR_SPAWN_HORIZONTAL));
        OpStz(memory, cpu, OpAbs(cpu, ACTOR_SPAWN_VERTICAL));
        OpStz(memory, cpu, OpAbs(cpu, ACTOR_SPAWN_ANGLE));
    } else if (preset == ACTOR_SPAWN_UPWARD) {
        OpStz(memory, cpu, OpAbs(cpu, ACTOR_SPAWN_HORIZONTAL));
        LoadA8(cpu, 0xf0u);
        OpSta(memory, cpu, OpAbs(cpu, ACTOR_SPAWN_VERTICAL));
        OpStz(memory, cpu, OpAbs(cpu, ACTOR_SPAWN_ANGLE));
    } else {
        OpStz(memory, cpu, OpAbs(cpu, ACTOR_SPAWN_HORIZONTAL));
        OpStz(memory, cpu, OpAbs(cpu, ACTOR_SPAWN_VERTICAL));
        if (preset == ACTOR_SPAWN_HALF_TURN) {
            LoadA8(cpu, 0x80u);
            OpSta(memory, cpu, OpAbs(cpu, ACTOR_SPAWN_ANGLE));
        } else
            OpStz(memory, cpu, OpAbs(cpu, ACTOR_SPAWN_ANGLE));
    }
}

static Lufia2ExecutionResult SpawnActorCommand(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint32_t entry, ActorSpawnPreset preset,
    uint8_t moving) {
    if (!ActorCommandContext(cpu))
        return ExecutionHandoff(cpu, entry);
    ReadActorSpawnScript(memory, cpu);
    SetActorSpawnPreset(memory, cpu, preset);
    return moving ? Lufia2BattleEffectSpawnMovingActor(memory, cpu)
                  : Lufia2BattleEffectSpawnActor(memory, cpu);
}

Lufia2ExecutionResult Lufia2BattleEffectSpawnStationaryActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return SpawnActorCommand(memory, cpu, 0x81a455u, ACTOR_SPAWN_DEFAULT, 0u);
}

Lufia2ExecutionResult Lufia2BattleEffectSpawnHalfTurnActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return SpawnActorCommand(memory, cpu, 0x81a46eu, ACTOR_SPAWN_HALF_TURN, 0u);
}

Lufia2ExecutionResult Lufia2BattleEffectSpawnActorWithAngle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return SpawnActorCommand(memory, cpu, 0x81a489u,
        ACTOR_SPAWN_STREAM_ANGLE, 0u);
}

Lufia2ExecutionResult Lufia2BattleEffectSpawnOffsetActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return SpawnActorCommand(memory, cpu, 0x81a4a6u, ACTOR_SPAWN_DIAGONAL, 0u);
}

Lufia2ExecutionResult Lufia2BattleEffectSpawnUnshiftedMovingActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return SpawnActorCommand(memory, cpu, 0x81a4c3u, ACTOR_SPAWN_DEFAULT, 1u);
}

Lufia2ExecutionResult Lufia2BattleEffectSpawnHalfTurnMovingActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return SpawnActorCommand(memory, cpu, 0x81a4dcu, ACTOR_SPAWN_HALF_TURN, 1u);
}

Lufia2ExecutionResult Lufia2BattleEffectSpawnRightwardActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return SpawnActorCommand(memory, cpu, 0x81a4f7u, ACTOR_SPAWN_RIGHTWARD, 1u);
}

Lufia2ExecutionResult Lufia2BattleEffectSpawnUpwardActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return SpawnActorCommand(memory, cpu, 0x81a512u, ACTOR_SPAWN_UPWARD, 1u);
}

Lufia2ExecutionResult Lufia2BattleEffectSpawnMovingActorWithAngle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return SpawnActorCommand(memory, cpu, 0x81a52du,
        ACTOR_SPAWN_STREAM_ANGLE, 1u);
}

Lufia2ExecutionResult Lufia2BattleEffectSpawnActorWithVerticalSpeed(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return SpawnActorCommand(memory, cpu, 0x81a54au,
        ACTOR_SPAWN_STREAM_VERTICAL, 1u);
}
