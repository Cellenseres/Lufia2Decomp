#include "core/child_call.h"
#include "lufia2/world_map.h"
#include "world/world_scene_internal.h"

enum {
    SCRIPT_SCRATCH = 0u,
    SCRIPT_POINTER = WRAM_FIELD_LAYER_SCROLL_X + 6u,
    SCRIPT_FRAME = WRAM_FIELD_LAYER_SCROLL_Y,
    SCRIPT_CONTROL = WRAM_FIELD_STREAMED_ROW_SOURCE + 2u,
    SCRIPT_SLICE_LIMIT = 65536u,
    SCENE_FIRST_ACTOR = WRAM_WORLD_MAP_OBJECT_TABLE + 2u * 0x1du,
    SCENE_ACTOR_STRIDE = 0x1du,
    SCENE_ACTOR_COUNT = 5u,
    SCENE_FIRST_ANIMATION = 0x0du
};

typedef struct WorldFrameScript {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
} WorldFrameScript;

static bool FrameScriptEntry(const Lufia2CpuState *cpu, Lufia2PushedChildCall child) {
    return child && cpu->program_bank == 0x86u && cpu->data_bank == 0x86u &&
           cpu->accumulator_is_8_bit && !cpu->index_is_8_bit && !cpu->direct_page &&
           !cpu->decimal;
}

static bool FrameScriptChildReturned(WorldFrameScript *script, uint32_t site,
                                     bool returned) {
    if (!returned) {
        script->result = ExecutionReturned(site);
        script->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (!FrameScriptEntry(script->cpu, script->child)) {
        script->result = ExecutionHandoff(script->cpu, site + 3u);
        return false;
    }
    return true;
}

static bool FrameScriptCall(WorldFrameScript *script, uint32_t site, uint32_t target) {
    return FrameScriptChildReturned(script, site,
                                    CallChildWithFrame(script->memory, script->cpu,
                                                       script->child, script->context,
                                                       site, target, 2u, 0x86u));
}

static bool UpdateScenePlane(WorldFrameScript *script) {
    const Lufia2Memory *memory = script->memory;
    Lufia2CpuState *cpu = script->cpu;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E1711));
    OpOra(memory, cpu, OpAbs(cpu, WRAM_UNK_7E1710));
    if (!cpu->zero)
        return true;
    OpLda(memory, cpu, OpAbs(cpu, SCRIPT_CONTROL));
    OpAndValue(cpu, 4u);
    return cpu->zero || FrameScriptCall(script, 0x86c214u, 0x86a7ceu);
}

static bool RunEnabledSceneUpdate(WorldFrameScript *script, uint8_t control,
                                  uint32_t site, uint32_t target) {
    OpLda(script->memory, script->cpu, OpAbs(script->cpu, SCRIPT_CONTROL));
    OpAndValue(script->cpu, control);
    return script->cpu->zero || FrameScriptCall(script, site, target);
}

static bool AdvanceSceneFrame(WorldFrameScript *script) {
    const Lufia2Memory *memory = script->memory;
    Lufia2CpuState *cpu = script->cpu;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
    if (cpu->negative)
        return false;
    OpLdx(cpu, OpRead16(memory, OpAbs(cpu, SCRIPT_FRAME)));
    OpInx(cpu);
    OpWrite16(memory, OpAbs(cpu, SCRIPT_FRAME), cpu->x);
    return true;
}

static Lufia2ExecutionResult RunSceneFrames(WorldFrameScript *script) {
    Lufia2CpuState *cpu = script->cpu;
    for (unsigned frame = 0u; frame < SCRIPT_SLICE_LIMIT; ++frame) {
        if (!FrameScriptCall(script, 0x86c1ecu, 0x8694d4u))
            return script->result;
        if (cpu->carry)
            return ExecutionReturned(0x86c245u);
        if (!RunEnabledSceneUpdate(script, 1u, 0x86c1f8u, 0x86995bu) ||
            !RunEnabledSceneUpdate(script, 2u, 0x86c202u, 0x8699bfu) ||
            !UpdateScenePlane(script) ||
            !FrameScriptCall(script, 0x86c217u, 0x86a791u) ||
            !FrameScriptCall(script, 0x86c21au, 0x86c246u) ||
            !RunEnabledSceneUpdate(script, 8u, 0x86c224u, 0x86e11fu) ||
            !RunEnabledSceneUpdate(script, 0x10u, 0x86c22eu, 0x86e1b9u))
            return script->result;
        if (!FrameScriptCall(script, 0x86c231u, 0x86973eu) ||
            !FrameScriptCall(script, 0x86c234u, 0x86974fu))
            return script->result;
        if (!AdvanceSceneFrame(script))
            return ExecutionReturned(0x86c245u);
    }
    return ExecutionHandoff(cpu, 0x86c1ecu);
}

static Lufia2ExecutionResult StartSceneFrames(WorldFrameScript *script) {
    const Lufia2Memory *memory = script->memory;
    Lufia2CpuState *cpu = script->cpu;
    OpLdx(cpu, OpRead16(memory, OpAbs(cpu, SCRIPT_POINTER)));
    OpLda(memory, cpu, OpAbsX(cpu, 2u));
    if (!FrameScriptCall(script, 0x86c1e9u, 0x86d385u))
        return script->result;
    return RunSceneFrames(script);
}

Lufia2ExecutionResult Lufia2WorldMapRunSceneScript(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    if (!FrameScriptEntry(cpu, child))
        return ExecutionHandoff(cpu, 0x86c1d2u);
    WorldFrameScript script = {memory, cpu, child, context,
                               ExecutionReturned(0x86c245u)};
    OpWrite16(memory, OpAbs(cpu, SCRIPT_POINTER), cpu->x);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E11D8));
    if (!FrameScriptCall(&script, 0x86c1dau, 0x86e1b9u) ||
        !FrameScriptCall(&script, 0x86c1ddu, 0x86973eu) ||
        !FrameScriptCall(&script, 0x86c1e0u, 0x86974fu))
        return script.result;
    return StartSceneFrames(&script);
}

static bool RunSceneCallback(WorldFrameScript *script, uint32_t jump,
                             uint16_t return_address) {
    const Lufia2Memory *memory = script->memory;
    Lufia2CpuState *cpu = script->cpu;
    OpWrite16(memory, OpDp(cpu, SCRIPT_SCRATCH), cpu->y);
    OpLdx(cpu, return_address);
    PushIndex(memory, cpu);
    const uint32_t target = 0x860000u | Read16Bank(memory, 0u, SCRIPT_SCRATCH);
    return FrameScriptChildReturned(
        script, jump, script->child(script->context, cpu, target, jump, 2u));
}

static bool FindTimedSceneCallback(WorldFrameScript *script) {
    const Lufia2Memory *memory = script->memory;
    Lufia2CpuState *cpu = script->cpu;
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, OpRead16(memory, OpAbs(cpu, SCRIPT_POINTER)));
    OpStz(memory, cpu, OpDp(cpu, SCRIPT_SCRATCH));
    for (unsigned record = 0u; record < SCRIPT_SLICE_LIMIT; ++record) {
        OpLda(memory, cpu, OpAbsX(cpu, 3u));
        OpAndValue(cpu, 0xffu);
        if (cpu->zero)
            return true;
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpDp(cpu, SCRIPT_SCRATCH));
        OpSta(memory, cpu, OpDp(cpu, SCRIPT_SCRATCH));
        OpCmp(memory, cpu, OpAbs(cpu, SCRIPT_FRAME));
        if (cpu->zero) {
            OpSepWidths(cpu, 0x20u);
            OpLdy(cpu, OpRead16(memory, OpAbsX(cpu, 4u)));
            return RunSceneCallback(script, 0x86c26fu, 0xc271u);
        }
        OpInx(cpu);
        OpInx(cpu);
        OpInx(cpu);
    }
    script->result = ExecutionHandoff(cpu, 0x86c24du);
    return false;
}

Lufia2ExecutionResult Lufia2WorldMapStepSceneScript(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu,
                                                    Lufia2PushedChildCall child,
                                                    void *context) {
    if (!FrameScriptEntry(cpu, child))
        return ExecutionHandoff(cpu, 0x86c246u);
    WorldFrameScript script = {memory, cpu, child, context,
                               ExecutionReturned(0x86c283u)};
    if (!FindTimedSceneCallback(&script))
        return script.result;
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, OpRead16(memory, OpAbs(cpu, SCRIPT_POINTER)));
    OpLdy(cpu, OpRead16(memory, OpAbsX(cpu, 0u)));
    if (!RunSceneCallback(&script, 0x86c280u, 0xc282u))
        return script.result;
    return script.result;
}

Lufia2ExecutionResult Lufia2WorldMapStartSceneActor(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu,
                                                    Lufia2PushedChildCall child,
                                                    void *context) {
    if (!FrameScriptEntry(cpu, child))
        return ExecutionHandoff(cpu, 0x86c2adu);
    WorldFrameScript script = {memory, cpu, child, context,
                               ExecutionReturned(0x86c2b2u)};
    OpStz(memory, cpu, WorldSceneObjectField(cpu, 0x0fu));
    if (!FrameScriptCall(&script, 0x86c2afu, 0x86e0b9u))
        return script.result;
    return script.result;
}

Lufia2ExecutionResult Lufia2WorldMapStartSceneActors(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu,
                                                     Lufia2PushedChildCall child,
                                                     void *context) {
    if (!FrameScriptEntry(cpu, child))
        return ExecutionHandoff(cpu, 0x86c284u);
    WorldFrameScript script = {memory, cpu, child, context,
                               ExecutionReturned(0x86c2acu)};
    for (unsigned actor = 0u; actor < SCENE_ACTOR_COUNT; ++actor) {
        OpLdx(cpu, (uint16_t)(SCENE_FIRST_ACTOR + actor * SCENE_ACTOR_STRIDE));
        OpLoadA(cpu, (uint16_t)(SCENE_FIRST_ANIMATION + actor));
        if (!FrameScriptCall(&script, 0x86c289u + actor * 8u, 0x86c2adu))
            return script.result;
    }
    return script.result;
}

Lufia2ExecutionResult Lufia2WorldMapResumeSceneScript(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu,
                                                      Lufia2PushedChildCall child,
                                                      void *context) {
    if (!FrameScriptEntry(cpu, child))
        return ExecutionHandoff(cpu, 0x86c1e3u);
    WorldFrameScript script = {memory, cpu, child, context,
                               ExecutionReturned(0x86c245u)};
    return StartSceneFrames(&script);
}

Lufia2ExecutionResult Lufia2WorldMapResumeSceneFrame(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu,
                                                     Lufia2PushedChildCall child,
                                                     void *context) {
    if (!FrameScriptEntry(cpu, child))
        return ExecutionHandoff(cpu, 0x86c237u);
    WorldFrameScript script = {memory, cpu, child, context,
                               ExecutionReturned(0x86c245u)};
    if (!AdvanceSceneFrame(&script))
        return script.result;
    return RunSceneFrames(&script);
}
