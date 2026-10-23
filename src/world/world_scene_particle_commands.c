#include "lufia2/world_scene_particle_commands.h"
#include "core/child_call.h"
#include "system/wram.h"
#include "world/world_scene_internal.h"
#include "world/world_scene_resources_internal.h"

enum {
    PARTICLE_FIRST_SLOT = WRAM_WORLD_MAP_OBJECT_TABLE + 8u * 0x1du,
    PARTICLE_SLOT_COUNT = 13u,
    ORBIT_PARTICLE_SLOT_COUNT = 19u,
    PARTICLE_SLOT_SIZE = 0x1du,
    PARTICLE_POSITION_X = 2u,
    PARTICLE_POSITION_Y = 5u,
    PARTICLE_ANIMATION_PHASE = 15u,
    DP_PARTICLE_X = 4u,
    DP_PARTICLE_Y = 6u
};

Lufia2ExecutionResult Lufia2WorldMapSpawnTrailingSceneParticle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86bf3bu);
    OpWrite16(memory, OpDp(cpu, DP_PARTICLE_X), cpu->x);
    OpWrite16(memory, OpDp(cpu, DP_PARTICLE_Y), cpu->y);
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, PARTICLE_FIRST_SLOT);
    OpLdy(cpu, PARTICLE_SLOT_COUNT);
    do {
        OpLda(memory, cpu, WorldSceneObjectField(cpu, PARTICLE_POSITION_Y));
        if (cpu->negative) {
            OpLda(memory, cpu, OpDp(cpu, DP_PARTICLE_X));
            OpSta(memory, cpu, WorldSceneObjectField(cpu, PARTICLE_POSITION_X));
            OpLda(memory, cpu, OpDp(cpu, DP_PARTICLE_Y));
            OpDecA(cpu);
            OpSta(memory, cpu, WorldSceneObjectField(cpu, PARTICLE_POSITION_Y));
            OpSepWidths(cpu, 0x20u);
            OpStz(memory, cpu, WorldSceneObjectField(cpu, PARTICLE_ANIMATION_PHASE));
            OpLoadA(cpu, 0x21u);
            if (!CallChildWithFrame(memory, cpu, child, context,
                                    0x86bf66u, 0x86e0b9u, 2u, 0x86u)) {
                Lufia2ExecutionResult result = ExecutionReturned(0x86bf66u);
                result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
                return result;
            }
            if (!WorldSceneResourceEntry(cpu))
                return ExecutionHandoff(cpu, 0x86bf69u);
            return ExecutionReturned(0x86bf69u);
        }
        OpTxa(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, PARTICLE_SLOT_SIZE);
        OpTax(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x86bf56u);
}

Lufia2ExecutionResult Lufia2WorldMapFindOrbitSceneParticleSlot(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86c1b4u);
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, WRAM_WORLD_MAP_OBJECT_TABLE + PARTICLE_SLOT_SIZE);
    OpLdy(cpu, ORBIT_PARTICLE_SLOT_COUNT);
    cpu->carry = 0u;
    do {
        OpLda(memory, cpu, WorldSceneObjectField(cpu, PARTICLE_POSITION_Y));
        if (cpu->negative)
            break;
        OpTxa(cpu);
        OpAdcValue(cpu, PARTICLE_SLOT_SIZE);
        OpTax(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x86c1cbu);
}
