#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "system/wram.h"
#include "text/text_child_service.h"
#include "text/text_internal.h"

enum {
    RUN_SCENE_SEQUENCE = 0xba69u,
    UPLOAD_SCENE_GRAPHICS = 0xba97u,
    RESTORE_SCENE_GRAPHICS = 0xbaa2u,
    RUN_ALTERNATE_SCENE = 0xbac9u,
    SET_SCENE_HDMA = 0xbaf1u,
    RUN_WORLD_SCENE = 0xbb94u,
    DP_SCENE_SELECTOR = 0x30u,
    DP_SCENE_ARGUMENT = 0x54u,
    DP_NMI_BANK = 0x6au,
    DP_LAYER_REQUEST = 0x74u,
    DP_HDMA_CHANNELS = 0x81u,
    ROM_BLEND_HDMA = 0xbb5fu,
    ROM_LAYER_HDMA = 0xbb67u
};

static void InvalidateSceneReload(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, WRAM_SCENE_RELOAD_UNK_D0C2);
    OpSta(memory, cpu, WRAM_SCENE_RELOAD_UNK_D0C2 + 1u);
}

static bool RunSceneSequence(TextChildService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    if (!TextCallChild(service, 0x80ba69u, 0x80c0b7u, 2u))
        return false;
    PushDataBank(memory, cpu);
    PushY(memory, cpu);
    PushAccumulator8(memory, cpu);
    if (!TextCallChild(service, 0x80ba6fu, 0x83afeau, 3u))
        return false;
    OpStz(memory, cpu, OpDp(cpu, DP_NMI_BANK));
    OpLoadA(cpu, Pull8(memory, cpu));
    OpSta(memory, cpu, OpDp(cpu, DP_SCENE_ARGUMENT));
    if (!TextCallChild(service, 0x80ba78u, 0x86aeb1u, 3u) ||
        !TextCallChild(service, 0x80ba7cu, 0x83b5d3u, 3u) ||
        !TextCallChild(service, 0x80ba80u, 0x83b53bu, 3u) ||
        !TextCallChild(service, 0x80ba84u, 0x83a82eu, 3u) ||
        !TextCallChild(service, 0x80ba88u, 0x8385dcu, 3u) ||
        !TextCallChild(service, 0x80ba8cu, 0x80a368u, 2u))
        return false;
    OpLoadA(cpu, 0x80u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_TEXT_STATE), true);
    OpPullY(memory, cpu);
    PullDataBank(memory, cpu);
    service->result = ExecutionReturned(0x80ba96u);
    return true;
}

static bool RunAlternateScene(TextChildService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    if (!TextCallChild(service, 0x80bac9u, 0x80c0b7u, 2u))
        return false;
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    if (!TextCallChild(service, 0x80baceu, 0x8eb2eeu, 3u))
        return false;
    InvalidateSceneReload(memory, cpu);
    if (!TextCallChild(service, 0x80badcu, 0x8385dcu, 3u))
        return false;
    OpLoadA(cpu, 0x40u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_FIELD_FLAGS), false);
    PullDataBank(memory, cpu);
    OpPullY(memory, cpu);
    OpLoadA(cpu, 0x80u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_TEXT_STATE), true);
    service->result = ExecutionReturned(0x80baecu);
    return true;
}

static bool RunWorldScene(TextChildService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    if (!TextCallChild(service, 0x80bb96u, 0x83afeau, 3u))
        return false;
    PullDataBank(memory, cpu);
    OpPullY(memory, cpu);
    if (!TextCallChild(service, 0x80bb9cu, 0x80c0b7u, 2u))
        return false;
    OpSta(memory, cpu, OpDp(cpu, DP_SCENE_SELECTOR));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09A9));
    OpStz(memory, cpu, OpDp(cpu, DP_HDMA_CHANNELS));
    PushAccumulator8(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpCmpValue(cpu, 2u);
    if (cpu->zero) {
        if (!TextCallChild(service, 0x80bbafu, 0x8692dfu, 3u))
            return false;
    } else if (!TextCallChild(service, 0x80bbb5u, 0x868000u, 3u)) {
        return false;
    }
    if (cpu->carry) {
        OpLoadA(cpu, 0x40u);
        OpTestBits(memory, cpu, OpAbs(cpu, WRAM_FIELD_FLAGS), true);
    }
    Push8(memory, cpu, 0x80u);
    PullDataBank(memory, cpu);
    OpStz(memory, cpu, OpDp(cpu, DP_HDMA_CHANNELS));
    InvalidateSceneReload(memory, cpu);
    if (!TextCallChild(service, 0x80bbceu, 0x8385dcu, 3u) ||
        !TextCallChild(service, 0x80bbd2u, 0x80bf0bu, 2u) ||
        !TextCallChild(service, 0x80bbd5u, 0x848328u, 3u))
        return false;
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpDp(cpu, DP_LAYER_REQUEST));
    if (!TextCallChild(service, 0x80bbddu, 0x80bf0bu, 2u))
        return false;
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    OpLoadA(cpu, Pull8(memory, cpu));
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_TEXT_STATE));
    service->result = ExecutionReturned(0x80bbeau);
    return true;
}

static bool UploadSceneGraphics(TextChildService *service) {
    if (!TextCallChild(service, 0x80ba97u, 0x80c0b7u, 2u))
        return false;
    return TextCallChild(service, 0x80ba9au, 0x8482e7u, 3u) &&
           TextCallChild(service, 0x80ba9eu, 0x80bf0bu, 2u);
}

static bool RestoreSceneGraphics(TextChildService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    if (!TextCallChild(service, 0x80baa2u, 0x80c0b7u, 2u))
        return false;
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
    if (!TextCallChild(service, 0x80baaau, 0x80bf0bu, 2u))
        return false;
    OpStz(memory, cpu, OpAbs(cpu, SNES_NMITIMEN));
    if (!TextCallChild(service, 0x80bab0u, 0x848328u, 3u))
        return false;
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpDp(cpu, DP_LAYER_REQUEST));
    if (!TextCallChild(service, 0x80bab8u, 0x808285u, 3u) ||
        !TextCallChild(service, 0x80babcu, 0x83b007u, 3u))
        return false;
    OpLoadA(cpu, 0x81u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_NMITIMEN));
    return TextCallChild(service, 0x80bac5u, 0x80bf0bu, 2u);
}

static bool SetSceneHdma(TextChildService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    if (!TextCallChild(service, 0x80baf1u, 0x80c0b7u, 2u))
        return false;
    OpOraValue(cpu, 0u);
    if (cpu->zero) {
        OpLoadA(cpu, 0x30u);
        OpTestBits(memory, cpu, OpDp(cpu, DP_HDMA_CHANNELS), false);
        OpLoadA(cpu, 0x80u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
        OpStz(memory, cpu, OpAbs(cpu, SNES_CGADSUB));
        OpStz(memory, cpu, OpAbs(cpu, SNES_CGWSEL));
        if (!TextCallChild(service, 0x80bb5bu, 0x80bf0bu, 2u))
            return false;
        service->result = ExecutionReturned(0x80bb5eu);
        return true;
    }
    if (!TextCallChild(service, 0x80baf8u, 0x80bf0bu, 2u))
        return false;
    OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E1271));
    OpLdx(cpu, ROM_BLEND_HDMA);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_A1TL(4)), cpu->x);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_A1B(4)));
    OpSta(memory, cpu, OpAbs(cpu, 0x4347u));
    OpLoadA(cpu, 0x32u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_BBAD(4)));
    OpLoadA(cpu, 0x40u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_DMAP(4)));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_CGWSEL));
    OpLoadA(cpu, 0xbfu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_CGADSUB));
    if (!TextCallChild(service, 0x80bb20u, 0x8482e7u, 3u))
        return false;
    OpLdx(cpu, ROM_LAYER_HDMA);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_A1TL(5)), cpu->x);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_A1B(5)));
    OpLoadA(cpu, 0x2cu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_BBAD(5)));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_DMAP(5)));
    OpLoadA(cpu, 0x30u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_HDMA_CHANNELS), true);
    if (!TextCallChild(service, 0x80bb3du, 0x80bf0bu, 2u))
        return false;
    OpLdx(cpu, 4u);
    OpWriteX(memory, cpu, OpAbs(cpu, TEXT_BG3_VOFS_SHADOW), cpu->x);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_INTERACTION_FACING_ACTOR_ID));
    service->result = ExecutionReturned(0x80bb4bu);
    return true;
}

Lufia2ExecutionResult Lufia2TextSceneDisplayCallback(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu,
                                                     uint16_t target,
                                                     Lufia2PushedChildCall child,
                                                     void *context) {
    TextChildService service = {memory, cpu, child, context,
                                ExecutionReturned(0x800000u | target)};
    switch (target) {
    case RUN_SCENE_SEQUENCE:
        RunSceneSequence(&service);
        break;
    case UPLOAD_SCENE_GRAPHICS:
        if (UploadSceneGraphics(&service))
            service.result = ExecutionReturned(0x80baa1u);
        break;
    case RESTORE_SCENE_GRAPHICS:
        if (RestoreSceneGraphics(&service))
            service.result = ExecutionReturned(0x80bac8u);
        break;
    case SET_SCENE_HDMA:
        SetSceneHdma(&service);
        break;
    case RUN_ALTERNATE_SCENE:
        RunAlternateScene(&service);
        break;
    case RUN_WORLD_SCENE:
        RunWorldScene(&service);
        break;
    default:
        return ExecutionHandoff(cpu, 0x800000u | target);
    }
    return service.result;
}
