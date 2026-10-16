#include <stddef.h>
#include "core/cpu_ops.h"
#include "field/field_internal.h"
#include "lufia2/field.h"
#include "system/wram.h"
#include "text/text_internal.h"
#include "text/text_child_service.h"

enum {
    REBUILD_ATTRIBUTES = 0xa2b0u,
    CENTER_LAYERS_AT_CELL = 0xb341u,
    CENTER_LAYERS_AT_SCENE = 0xb34fu,
    PUBLISH_LAYERS = 0xb494u,
    CENTER_CAMERA_AT_LEADER = 0xb49bu,
    CENTER_CAMERA_AT_CELL = 0xb4adu,
    REBUILD_ATTRIBUTES_SAVED = 0xb721u,
    RESTORE_SCENE_DISPLAY = 0xaf6bu,
    SCENE_FINE_POSITION = WRAM_FIELD_LAST_PENDING_OBJECT
};

typedef Lufia2ExecutionResult (*CameraStep)(const Lufia2Memory *, Lufia2CpuState *);

static uint32_t CallCameraStep(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                               CameraStep step, uint32_t target, uint16_t back,
                               TextChildService *service) {
    if (service) {
        const uint32_t site = 0x800000u | (uint16_t)(back - 3u);
        const uint16_t caller_stack = cpu->stack;
        if (!TextCallChild(service, site, target, 3u)) {
            if (service->result.flow == LUFIA2_EXECUTION_BOUNDARY)
                service->result = ExecutionHandoff(
                    cpu, ((uint32_t)cpu->program_bank << 16) | (uint16_t)(back + 1u));
            return service->result.pc;
        }
        if (cpu->stack != caller_stack) {
            service->result = ExecutionHandoff(cpu, site + 4u);
            return service->result.pc;
        }
        return 0u;
    }
    SimulateJslFrame(memory, cpu, 0x80u, back);
    cpu->program_bank = (uint8_t)(target >> 16);
    const Lufia2ExecutionResult result = step(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result.pc;
    SimulateRtlFrame(memory, cpu);
    cpu->program_bank = 0x80u;
    return 0u;
}

static uint8_t PreservePublishChild(void *context, Lufia2CpuState *cpu, uint32_t target,
                                    uint32_t site, uint8_t frame) {
    (void)site;
    (void)frame;
    *(uint32_t *)context = target;
    cpu->program_bank = (uint8_t)(target >> 16);
    return 0u;
}

static uint32_t PublishLayers(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                              uint16_t back, TextChildService *service) {
    if (service)
        return CallCameraStep(memory, cpu, NULL, 0x838e4cu, back, service);
    SimulateJslFrame(memory, cpu, 0x80u, back);
    cpu->program_bank = 0x83u;
    uint32_t handoff = 0u;
    const Lufia2ExecutionResult result =
        Lufia2FieldPublishRedrawnLayers(memory, cpu, PreservePublishChild, &handoff);
    if (handoff)
        return handoff;
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result.pc;
    SimulateRtlFrame(memory, cpu);
    cpu->program_bank = 0x80u;
    return 0u;
}

static void CenterCamera(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                         uint16_t back) {
    SimulateJsrFrame(memory, cpu, back);
    Lufia2TextScaleSceneCoordinates(memory, cpu, 0xb376u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, SCENE_FINE_POSITION));
    cpu->carry = 1u;
    OpSbcValue(cpu, FIELD_SCREEN_HALF_WIDTH);
    OpSta(memory, cpu, FIELD_SCRIPTED_CAMERA_X);
    OpLda(memory, cpu, OpAbs(cpu, SCENE_FINE_POSITION + 2u));
    cpu->carry = 1u;
    OpSbcValue(cpu, FIELD_SCREEN_HALF_HEIGHT);
    OpSta(memory, cpu, FIELD_SCRIPTED_CAMERA_Y);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 8u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_SCREEN_EFFECTS), 1);
    SimulateRtsFrame(memory, cpu);
}

static void ReadCameraCell(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                           uint16_t handler) {
    Lufia2TextNextByte(memory, cpu, (uint16_t)(handler + 2u));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E120A));
    Lufia2TextNextByte(memory, cpu, (uint16_t)(handler + 8u));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E120B));
}

static uint32_t CenterLayers(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                             uint16_t handler, TextChildService *service) {
    if (handler == CENTER_LAYERS_AT_CELL) {
        ReadCameraCell(memory, cpu, handler);
    } else {
        Lufia2TextNextByte(memory, cpu, 0xb351u);
        const uint32_t handoff = Lufia2TextReadSceneCell(memory, cpu, 0xb355u);
        if (handoff)
            return handoff;
    }
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E120A));
    OpSta(memory, cpu, OpDp(cpu, 0x56u));
    OpStz(memory, cpu, OpDp(cpu, 0x57u));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E120B));
    OpSta(memory, cpu, OpDp(cpu, 0x58u));
    OpStz(memory, cpu, OpDp(cpu, 0x59u));
    PushY(memory, cpu);
    uint32_t handoff = CallCameraStep(memory, cpu, Lufia2FieldSetupLayerScroll,
                                      0x838d42u, 0xb368u, service);
    if (handoff)
        return handoff;
    CenterCamera(memory, cpu, 0xb36bu);
    handoff = PublishLayers(memory, cpu, 0xb36fu, service);
    if (handoff)
        return handoff;
    OpPullY(memory, cpu);
    return LUFIA2_TEXT_COMMAND_NEXT;
}

static uint32_t RunCameraCommand(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                 uint16_t handler, TextChildService *service) {
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal ||
        cpu->stack < 0x1f10u || cpu->stack > 0x1ffcu)
        return 0u;
    switch (handler) {
    case REBUILD_ATTRIBUTES:
    case REBUILD_ATTRIBUTES_SAVED: {
        if (handler == REBUILD_ATTRIBUTES_SAVED)
            PushY(memory, cpu);
        OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_LAYER_TABLE_OFFSET));
        const uint32_t handoff =
            CallCameraStep(memory, cpu, Lufia2FieldBuildAttributes, 0x80ed9cu,
                           handler == REBUILD_ATTRIBUTES ? 0xa2b6u : 0xb728u, service);
        if (handoff)
            return handoff;
        if (handler == REBUILD_ATTRIBUTES_SAVED)
            OpPullY(memory, cpu);
        return LUFIA2_TEXT_COMMAND_NEXT;
    }
    case CENTER_LAYERS_AT_CELL:
    case CENTER_LAYERS_AT_SCENE:
        return CenterLayers(memory, cpu, handler, service);
    case PUBLISH_LAYERS: {
        const uint32_t handoff = PublishLayers(memory, cpu, 0xb497u, service);
        return handoff ? handoff : LUFIA2_TEXT_COMMAND_NEXT;
    }
    case CENTER_CAMERA_AT_LEADER:
        OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_TILE_X));
        OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E120A));
        OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_TILE_Y));
        OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E120B));
        CenterCamera(memory, cpu, 0xb4a9u);
        return LUFIA2_TEXT_COMMAND_NEXT;
    case CENTER_CAMERA_AT_CELL:
        ReadCameraCell(memory, cpu, handler);
        CenterCamera(memory, cpu, 0xb4bbu);
        OpLoadA(cpu, 2u);
        OpSta(memory, cpu, OpAbs(cpu, FIELD_LAYER_SCROLL_SPEED));
        return LUFIA2_TEXT_COMMAND_NEXT;
    case RESTORE_SCENE_DISPLAY: {
        const uint32_t handoff = CallCameraStep(memory, cpu, Lufia2FieldSetSceneDisplay,
                                                0x80f2f3u, 0xaf6eu, service);
        if (handoff)
            return handoff;
        OpLoadA(cpu, 0x80u);
        OpTestBits(memory, cpu, OpAbs(cpu, WRAM_SCREEN_EFFECTS), 0);
        return LUFIA2_TEXT_COMMAND_NEXT;
    }
    default:
        return 0u;
    }
}

Lufia2ExecutionResult Lufia2TextRunCameraCommand(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu, uint16_t handler,
                                                 Lufia2PushedChildCall child,
                                                 void *context) {
    if (handler != REBUILD_ATTRIBUTES && handler != REBUILD_ATTRIBUTES_SAVED &&
        handler != CENTER_LAYERS_AT_CELL && handler != CENTER_LAYERS_AT_SCENE &&
        handler != PUBLISH_LAYERS && handler != RESTORE_SCENE_DISPLAY)
        return ExecutionReturned(0u);
    if (!child || cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal ||
        cpu->stack < 0x1f10u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x800000u | handler);
    TextChildService service = {memory, cpu, child, context, ExecutionReturned(0u)};
    const uint32_t flow = RunCameraCommand(memory, cpu, handler, &service);
    if (service.result.flow != LUFIA2_EXECUTION_RETURNED)
        return service.result;
    if (flow != LUFIA2_TEXT_COMMAND_NEXT && flow != LUFIA2_TEXT_COMMAND_YIELD)
        return ExecutionHandoff(cpu, flow);
    return ExecutionReturned(flow);
}

uint32_t Lufia2TextExecuteCameraCommand(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                        uint16_t handler) {
    return RunCameraCommand(memory, cpu, handler, NULL);
}
