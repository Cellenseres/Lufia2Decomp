#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    SAVED_ACTOR_ID = WRAM_UNK_7E081E,
    SAVED_ACTOR_STATE = WRAM_UNK_7E08DE,
    SAVED_ACTOR_MAP = WRAM_UNK_7E085E,
    SAVED_ACTOR_RECORD = WRAM_UNK_7E089E,
    SCENE_PARTY_IDS = WRAM_SCENE_PARTY_IDS,
    SCENE_MAP_FLAGS = WRAM_SCENE_MAP_FLAGS,
    EVENT_FLAG_NUMBER = 0x54u,
    EVENT_FLAG_MASK = 0x55u
};

static Lufia2ExecutionResult SceneResetUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static void SceneReadSavedActors(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLoadA(cpu, 0x87u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpLdy(cpu, 0u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, 0x8003u)));
    do {
        OpLda(memory, cpu, OpAbsX(cpu, 0x8000u));
        OpSta(memory, cpu, OpAbsY(cpu, SAVED_ACTOR_ID));
        OpCmpValue(cpu, 0xffu);
        if (!cpu->zero) {
            const uint16_t fields[] = {
                SAVED_ACTOR_STATE, SAVED_ACTOR_MAP, SAVED_ACTOR_RECORD
            };
            for (unsigned field = 0u; field < 3u; ++field) {
                OpLda(memory, cpu, OpAbsX(cpu, (uint16_t)(0x8001u + field)));
                OpSta(memory, cpu, OpAbsY(cpu, fields[field]));
            }
        }
        for (unsigned byte = 0u; byte < 4u; ++byte)
            OpInx(cpu);
        OpIny(cpu);
        OpCpy(cpu, 64u);
    } while (!cpu->zero);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
}

static void SceneClearActorState(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const uint16_t fields[] = {
        WRAM_EVENT_FLAGS, 0x077fu, WRAM_UNK_7E079E, 0x079fu, WRAM_UNK_7E07BE, 0x07bfu,
        WRAM_UNK_7E07DE, 0x07dfu, WRAM_UNK_7E07FE, 0x07ffu, WRAM_SCENE_MAP_FLAGS, 0x097cu
    };
    OpLdx(cpu, 30u);
    OpLoadA(cpu, 0xffu);
    do {
        for (unsigned field = 0u; field < 12u; ++field)
            OpStz(memory, cpu, OpAbsX(cpu, fields[field]));
        OpDex(cpu);
        OpDex(cpu);
    } while (!cpu->negative);
    OpLdx(cpu, 15u);
    OpLoadA(cpu, 0xffu);
    do {
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E092B));
        OpDex(cpu);
    } while (!cpu->negative);
    OpLdx(cpu, 63u);
    LoadA16(cpu, cpu->direct_page);
    do {
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E093B));
        OpDex(cpu);
    } while (!cpu->negative);
    OpLdx(cpu, 4u);
    OpLoadA(cpu, 0xffu);
    do {
        OpSta(memory, cpu, OpAbsX(cpu, SCENE_PARTY_IDS));
        OpDex(cpu);
    } while (!cpu->negative);
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_PARTY_IDS));
    OpLdx(cpu, 7u);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E091E));
        OpDex(cpu);
    } while (!cpu->negative);
}

static void SceneResetOptions(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09DF));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09E0));
    const uint16_t cleared[] = {WRAM_UNK_7E09EE, 0x09efu, WRAM_UNK_7E09E1, 0x09e2u};
    for (unsigned field = 0u; field < 4u; ++field)
        OpStz(memory, cpu, OpAbs(cpu, cleared[field]));
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09E3));
    OpLoadA(cpu, 0xccu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09E4));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FIELD_FLAGS));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_FLAGS));
    OpLoadA(cpu, 0xffu);
    LoadA16(cpu, cpu->direct_page);
    OpSta(memory, cpu, WRAM_UNK_7FD0C4);
    OpSta(memory, cpu, WRAM_UNK_7FD0C5);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_DESTINATION_PARAMETERS));
    OpSta(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
    OpSta(memory, cpu, WRAM_UNK_7FE696);
    OpSta(memory, cpu, WRAM_UNK_7FE75A);
    LoadA16(cpu, cpu->direct_page);
    OpSta(memory, cpu, WRAM_UNK_7FE75A);
}

Lufia2ExecutionResult Lufia2FieldResetSavedScene(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x83u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x83addfu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x83addfu, 0x83b062u, 2u, 0x83u))
        return SceneResetUnwound(0x83addfu);
    SceneReadSavedActors(memory, cpu);
    SceneClearActorState(memory, cpu);
    SceneResetOptions(memory, cpu);
    return ExecutionReturned(0x83aeb4u);
}

Lufia2ExecutionResult Lufia2FieldResetScene(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x83u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x83adcau);
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    const struct { uint32_t site, target; uint8_t frame; } steps[] = {
        {0x83adceu, 0x81ec35u, 3u}, {0x83add2u, 0x829aa3u, 3u},
        {0x83add6u, 0x83addfu, 2u}, {0x83add9u, 0x83a686u, 2u}
    };
    for (unsigned step = 0u; step < 4u; ++step)
        if (!CallChildWithFrame(memory, cpu, child, context,
                steps[step].site, steps[step].target, steps[step].frame, 0x83u))
            return SceneResetUnwound(steps[step].site);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x83addeu);
}

Lufia2ExecutionResult Lufia2FieldMarkCurrentMap(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x83u || !cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x83b503u);
    LoadA16(cpu, cpu->direct_page);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x83b507u, 0x80e898u, 3u, 0x83u))
        return SceneResetUnwound(0x83b507u);
    OpOra(memory, cpu, OpAbsX(cpu, SCENE_MAP_FLAGS));
    OpSta(memory, cpu, OpAbsX(cpu, SCENE_MAP_FLAGS));
    return ExecutionReturned(0x83b511u);
}

Lufia2ExecutionResult Lufia2FieldResumeSceneSong(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x83u || !cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x83b52eu);
    OpLda(memory, cpu, WRAM_UNK_7FD0FD);
    OpCmpValue(cpu, 0xffu);
    if (!cpu->zero && !CallChildWithFrame(memory, cpu, child, context,
            0x83b536u, 0x8093feu, 3u, 0x83u))
        return SceneResetUnwound(0x83b536u);
    return ExecutionReturned(0x83b53au);
}
