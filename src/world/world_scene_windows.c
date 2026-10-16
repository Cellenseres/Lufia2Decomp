#include "core/child_call.h"
#include "core/snes_registers.h"
#include "lufia2/world_map.h"
#include "system/wram.h"
#include "world/world_scene_internal.h"
#include "world/world_scene_resources_internal.h"

enum {
    WINDOW_RECORD_BASE = WRAM_WORLD_SCENE_WINDOW_RECORD_STORAGE,
    WINDOW_RECORD_SIZE = 6u,
    WINDOW_FIRST_BUFFER = WRAM_BATTLE_PREVIEW_BACKGROUND_SELECTION,
    WINDOW_SECOND_BUFFER = WRAM_BATTLE_PREVIEW_SPRITE_SELECTION,
    WINDOW_LAYER = 0u,
    WINDOW_CENTER_X = 0x58u,
    WINDOW_CENTER_Y = 0x5au,
    WINDOW_RADIUS = 0x54u,
    WINDOW_PARAMETER_56 = 0x56u,
    WINDOW_DESTINATION = 0x60u,
    WINDOW_DESTINATION_BANK = 0x62u
};

typedef struct SceneWindow {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
} SceneWindow;

static bool CallWindowChild(SceneWindow *window, uint32_t site, uint32_t target,
                            uint8_t frame_size) {
    if (!CallChildWithFrame(window->memory, window->cpu, window->child, window->context,
                            site, target, frame_size, 0x86u)) {
        window->result = ExecutionReturned(site);
        window->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (!WorldSceneResourceEntry(window->cpu)) {
        window->result = ExecutionHandoff(window->cpu, site + frame_size + 1u);
        return false;
    }
    return true;
}

static void SelectSceneRecord(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                              uint8_t size, uint16_t base) {
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
    OpLoadA(cpu, size);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, base);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, SNES_RDMPYL));
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
}

Lufia2ExecutionResult Lufia2WorldMapSelectSceneWindowRecord(const Lufia2Memory *memory,
                                                            Lufia2CpuState *cpu) {
    if (!WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86c321u);
    SelectSceneRecord(memory, cpu, WINDOW_RECORD_SIZE, WINDOW_RECORD_BASE);
    return ExecutionReturned(0x86c335u);
}

Lufia2ExecutionResult Lufia2WorldMapLoadSceneWindowRecord(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu,
                                                          Lufia2PushedChildCall child,
                                                          void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86c336u);
    SceneWindow window = {memory, cpu, child, context, ExecutionReturned(0x86c357u)};
    if (!CallWindowChild(&window, 0x86c336u, 0x86c321u, 2u))
        return window.result;
    for (uint8_t field = 0u; field < WINDOW_RECORD_SIZE; ++field) {
        OpLda(memory, cpu, OpAbsY(cpu, field));
        OpSta(memory, cpu, WorldSceneObjectField(cpu, field));
    }
    return window.result;
}

static void ReadWindowShape(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint8_t parameters[] = {WINDOW_CENTER_X, WINDOW_CENTER_Y,
                                         WINDOW_RADIUS, WINDOW_PARAMETER_56};
    for (uint8_t field = 0u; field < 4u; ++field) {
        OpLda(memory, cpu, WorldSceneObjectField(cpu, field));
        OpSta(memory, cpu, OpDp(cpu, parameters[field]));
    }
}

static void SelectWindowBuffer(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                               bool second_layer) {
    OpLdy(cpu, second_layer ? 0xd800u : 0xd400u);
    OpLda(memory, cpu,
          OpAbs(cpu, second_layer ? WINDOW_SECOND_BUFFER : WINDOW_FIRST_BUFFER));
    OpLsrA(cpu);
    if (!cpu->carry)
        OpLdy(cpu, second_layer ? 0xda00u : 0xd600u);
    OpWrite16(memory, OpDp(cpu, WINDOW_DESTINATION), cpu->y);
    OpLoadA(cpu, 0x7fu);
    OpSta(memory, cpu, OpDp(cpu, WINDOW_DESTINATION_BANK));
}

Lufia2ExecutionResult Lufia2WorldMapSubmitSceneWindowRecord(const Lufia2Memory *memory,
                                                            Lufia2CpuState *cpu,
                                                            Lufia2PushedChildCall child,
                                                            void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86c2d9u);
    SceneWindow window = {memory, cpu, child, context, ExecutionReturned(0x86c320u)};
    if (!CallWindowChild(&window, 0x86c2d9u, 0x86c321u, 2u))
        return window.result;
    ReadWindowShape(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, WINDOW_LAYER));
    PushAccumulator8(memory, cpu);
    SelectWindowBuffer(memory, cpu, !cpu->zero);
    if (!CallWindowChild(&window, 0x86c311u, 0x86c358u, 3u))
        return window.result;
    OpLoadA(cpu, Pull8(memory, cpu));
    OpStepMem(memory, cpu,
              OpAbs(cpu, cpu->zero ? WINDOW_FIRST_BUFFER : WINDOW_SECOND_BUFFER), true);
    return window.result;
}

Lufia2ExecutionResult Lufia2WorldMapSelectSlopedWindowRecord(const Lufia2Memory *memory,
                                                             Lufia2CpuState *cpu) {
    if (!WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86c536u);
    SelectSceneRecord(memory, cpu, 15u, WINDOW_RECORD_BASE + 0x18u);
    return ExecutionReturned(0x86c54au);
}

Lufia2ExecutionResult Lufia2WorldMapLoadSlopedWindowRecord(const Lufia2Memory *memory,
                                                           Lufia2CpuState *cpu,
                                                           Lufia2PushedChildCall child,
                                                           void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86c54bu);
    SceneWindow window = {memory, cpu, child, context, ExecutionReturned(0x86c581u)};
    if (!CallWindowChild(&window, 0x86c54bu, 0x86c536u, 2u))
        return window.result;
    static const uint8_t words[] = {0u, 2u, 6u, 8u, 10u};
    static const uint8_t bytes[] = {4u, 5u, 12u, 13u};
    OpRepWidths(cpu, 0x20u);
    for (unsigned field = 0u; field < 5u; ++field) {
        OpLda(memory, cpu, OpAbsY(cpu, words[field]));
        OpSta(memory, cpu, WorldSceneObjectField(cpu, words[field]));
    }
    OpSepWidths(cpu, 0x20u);
    for (unsigned field = 0u; field < 4u; ++field) {
        OpLda(memory, cpu, OpAbsY(cpu, bytes[field]));
        OpSta(memory, cpu, WorldSceneObjectField(cpu, bytes[field]));
    }
    OpStz(memory, cpu, WorldSceneObjectField(cpu, 14u));
    return window.result;
}

static void ReadSlopedWindowShape(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdy(cpu, OpRead16(memory, WorldSceneObjectField(cpu, 0u)));
    OpWrite16(memory, OpDp(cpu, WINDOW_CENTER_X), cpu->y);
    OpLdy(cpu, OpRead16(memory, WorldSceneObjectField(cpu, 2u)));
    OpWrite16(memory, OpDp(cpu, WINDOW_CENTER_Y), cpu->y);
    OpLda(memory, cpu, WorldSceneObjectField(cpu, 4u));
    OpSta(memory, cpu, OpDp(cpu, 0x22u));
    OpLda(memory, cpu, WorldSceneObjectField(cpu, 5u));
    OpSta(memory, cpu, OpDp(cpu, 0x23u));
    static const uint8_t source[] = {6u, 8u, 10u};
    for (unsigned field = 0u; field < 3u; ++field) {
        OpLdy(cpu, OpRead16(memory, WorldSceneObjectField(cpu, source[field])));
        OpWrite16(memory, OpDp(cpu, (uint8_t)(0x24u + field * 2u)), cpu->y);
    }
}

Lufia2ExecutionResult
Lufia2WorldMapSubmitSlopedWindowRecord(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                       Lufia2PushedChildCall child, void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86c4e0u);
    SceneWindow window = {memory, cpu, child, context, ExecutionReturned(0x86c535u)};
    if (!CallWindowChild(&window, 0x86c4e0u, 0x86c536u, 2u))
        return window.result;
    ReadSlopedWindowShape(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, WINDOW_LAYER));
    OpAndValue(cpu, 1u);
    PushAccumulator8(memory, cpu);
    SelectWindowBuffer(memory, cpu, !cpu->zero);
    if (!CallWindowChild(&window, 0x86c526u, 0x86c5f8u, 3u))
        return window.result;
    OpLoadA(cpu, Pull8(memory, cpu));
    OpStepMem(memory, cpu,
              OpAbs(cpu, cpu->zero ? WINDOW_FIRST_BUFFER : WINDOW_SECOND_BUFFER), 1);
    return window.result;
}
