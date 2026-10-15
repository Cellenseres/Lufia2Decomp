#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "system/wram.h"
#include "text/text_child_service.h"
#include "text/text_internal.h"

enum {
    UPLOAD_SCENE_GRAPHICS = 0xba97u,
    RESTORE_SCENE_GRAPHICS = 0xbaa2u,
    SET_SCENE_HDMA = 0xbaf1u,
    DP_LAYER_REQUEST = 0x74u,
    DP_HDMA_CHANNELS = 0x81u,
    ROM_BLEND_HDMA = 0xbb5fu,
    ROM_LAYER_HDMA = 0xbb67u
};

static bool UploadSceneGraphics(TextChildService *service) {
    Lufia2TextNextByte(service->memory, service->cpu, 0xba99u);
    return TextCallChild(service, 0x80ba9au, 0x8482e7u, 3u) &&
           TextCallChild(service, 0x80ba9eu, 0x80bf0bu, 2u);
}

static bool RestoreSceneGraphics(TextChildService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    Lufia2TextNextByte(memory, cpu, 0xbaa4u);
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
    Lufia2TextNextByte(memory, cpu, 0xbaf3u);
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
    default:
        return ExecutionHandoff(cpu, 0x800000u | target);
    }
    return service.result;
}
