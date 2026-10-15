#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "lufia2/system.h"
#include "system/wram.h"
#include "text/text_internal.h"

enum {
    SCENE_PAIR_ACTION = 0xafc0u,
    COPY_SCENE_TILES = 0xb62bu,
    REFRESH_ATTRIBUTES = 0xb646u,
    UPLOAD_LAYERS = 0xb659u,
    COPY_SCENE_TILES_AND_UPLOAD = 0xb6f4u,
    COPY_SCENE_TILES_AND_REDRAW = 0xb72du,
    DP_UPLOAD_REQUESTS = 0x74u,
    DP_SCENE_ACTION_X = 0x9fu,
    DP_SCENE_ACTION_Y = 0xa1u,
    DP_SCENE_ACTION_PARAMETER = 0x00u
};

typedef Lufia2ExecutionResult (*RenderStep)(const Lufia2Memory *, Lufia2CpuState *);

static uint32_t CallRenderStep(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                               RenderStep step, uint8_t bank, uint16_t back) {
    SimulateJslFrame(memory, cpu, 0x80u, back);
    cpu->program_bank = bank;
    const Lufia2ExecutionResult result = step(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result.pc;
    SimulateRtlFrame(memory, cpu);
    cpu->program_bank = 0x80u;
    return 0u;
}

static uint8_t PreserveUploadChild(void *context, Lufia2CpuState *cpu, uint32_t target,
                                   uint32_t site, uint8_t frame) {
    (void)site;
    (void)frame;
    *(uint32_t *)context = target;
    cpu->program_bank = (uint8_t)(target >> 16);
    return 0u;
}

static uint32_t ReadObjectRectangle(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                    uint16_t back) {
    SimulateJsrFrame(memory, cpu, back);
    Lufia2TextNextByte(memory, cpu, 0xb76cu);
    uint32_t handoff = Lufia2TextReadScenePair(memory, cpu, 0xb76fu);
    if (handoff)
        return handoff;
    OpLda(memory, cpu, WRAM_UNK_7E120A);
    OpSta(memory, cpu, WRAM_FIELD_PENDING_OBJECT_X);
    OpLda(memory, cpu, WRAM_UNK_7E120B);
    OpSta(memory, cpu, WRAM_FIELD_PENDING_OBJECT_Y);
    Lufia2TextNextByte(memory, cpu, 0xb782u);
    handoff = Lufia2TextReadScenePair(memory, cpu, 0xb785u);
    if (handoff)
        return handoff;
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, WRAM_UNK_7E120A);
    OpSta(memory, cpu, WRAM_FIELD_OBJECT_SOURCE_X);
    OpLda(memory, cpu, WRAM_SCENE_ACTOR_AUXILIARY);
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, WRAM_FIELD_OBJECT_SOURCE_X));
    OpSta(memory, cpu, WRAM_FIELD_OBJECT_WIDTH);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x33u);
    OpSta(memory, cpu, WRAM_FIELD_OBJECT_FLAGS);
    SimulateRtsFrame(memory, cpu);
    return 0u;
}

static uint32_t CopySceneTiles(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                               uint16_t handler) {
    uint32_t handoff = ReadObjectRectangle(memory, cpu, (uint16_t)(handler + 2u));
    if (handoff)
        return handoff;
    const bool upload = handler == COPY_SCENE_TILES_AND_UPLOAD;
    if (upload) {
        Lufia2TextNextByte(memory, cpu, 0xb6f9u);
        OpSta(memory, cpu, WRAM_FIELD_OBJECT_FLAGS);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_EVENT_REDRAW_FLAGS));
    }
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    handoff = CallRenderStep(memory, cpu, Lufia2FieldCopyObjectTiles, 0x83u,
                             handler == COPY_SCENE_TILES ? 0xb633u
                             : upload                    ? 0xb706u
                                                         : 0xb735u);
    if (handoff)
        return handoff;
    if (handler == COPY_SCENE_TILES_AND_REDRAW) {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_FADE_CONTROL));
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, TEXT_WINDOW_STATE));
            OpBitValue(cpu, 0x40u);
            if (!cpu->zero) {
                PullDataBank(memory, cpu);
                OpPullY(memory, cpu);
                return LUFIA2_TEXT_COMMAND_NEXT;
            }
            OpLda(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
            if (!cpu->negative) {
                Push8(memory, cpu, cpu->program_bank);
                PullDataBank(memory, cpu);
                OpLdx(cpu, 0u);
                handoff =
                    CallRenderStep(memory, cpu, Lufia2FieldRedrawLayer, 0x80u, 0xb74fu);
                if (handoff)
                    return handoff;
                OpLdx(cpu, 2u);
                handoff =
                    CallRenderStep(memory, cpu, Lufia2FieldRedrawLayer, 0x80u, 0xb756u);
                if (handoff)
                    return handoff;
                OpLoadA(cpu, 0xa0u);
                OpSta(memory, cpu, OpDp(cpu, DP_UPLOAD_REQUESTS));
                SimulateJsrFrame(memory, cpu, 0xb75du);
                return 0x80bf0bu;
            }
        }
        OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_LAYER_TABLE_OFFSET));
        handoff =
            CallRenderStep(memory, cpu, Lufia2FieldBuildAttributes, 0x80u, 0xb764u);
        if (handoff)
            return handoff;
    } else {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
        if (!cpu->negative) {
            handoff = CallRenderStep(memory, cpu, Lufia2FieldRenderLayerPair, 0x83u,
                                     upload ? 0xb70fu : 0xb63cu);
            if (handoff)
                return handoff;
            if (upload) {
                handoff = CallRenderStep(memory, cpu, Lufia2FieldRequestTilemapUploads,
                                         0x80u, 0xb713u);
                if (handoff)
                    return handoff;
                PullDataBank(memory, cpu);
                OpPullY(memory, cpu);
                OpWriteX(memory, cpu, OpAbs(cpu, TEXT_SCRIPT_POINTER), cpu->y);
                return LUFIA2_TEXT_COMMAND_YIELD;
            }
            OpLoadA(cpu, 0xa0u);
            OpTestBits(memory, cpu, OpDp(cpu, DP_UPLOAD_REQUESTS), 1u);
        }
    }
    PullDataBank(memory, cpu);
    OpPullY(memory, cpu);
    return LUFIA2_TEXT_COMMAND_NEXT;
}

static uint32_t RefreshAttributes(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    const uint32_t handoff =
        CallRenderStep(memory, cpu, Lufia2FieldRefreshObjectAttributes, 0x83u, 0xb64bu);
    if (handoff)
        return handoff;
    PullDataBank(memory, cpu);
    OpPullY(memory, cpu);
    SimulateJsrFrame(memory, cpu, 0xb650u);
    OpStz(memory, cpu, OpAbs(cpu, TEXT_CALLER_BANK));
    OpStz(memory, cpu, OpAbs(cpu, TEXT_RETURN_BANK));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_TEXT_STATE));
    SimulateRtsFrame(memory, cpu);
    OpLoadA(cpu, 1u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_ACTOR_STATE), 0u);
    return LUFIA2_TEXT_COMMAND_YIELD;
}

static uint32_t UploadLayers(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 0u);
    uint32_t handoff =
        CallRenderStep(memory, cpu, Lufia2FieldRedrawLayer, 0x80u, 0xb65fu);
    if (handoff)
        return handoff;
    OpLdx(cpu, 2u);
    handoff = CallRenderStep(memory, cpu, Lufia2FieldRedrawLayer, 0x80u, 0xb666u);
    if (handoff)
        return handoff;
    OpStz(memory, cpu, OpAbs(cpu, 0x4200u));
    OpLoadA(cpu, 0xa0u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_UPLOAD_REQUESTS), 1u);
    SimulateJslFrame(memory, cpu, 0x80u, 0xb671u);
    handoff = 0u;
    const Lufia2ExecutionResult result =
        Lufia2SceneUploadRequestedTilemaps(memory, cpu, PreserveUploadChild, &handoff);
    if (handoff || result.flow != LUFIA2_EXECUTION_RETURNED)
        return handoff ? handoff : result.pc;
    SimulateRtlFrame(memory, cpu);
    OpLoadA(cpu, 0x81u);
    OpSta(memory, cpu, OpAbs(cpu, 0x4200u));
    SimulateJsrFrame(memory, cpu, 0xb679u);
    return 0x80bf0bu;
}

static uint32_t ScenePairAction(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2TextNextByte(memory, cpu, 0xafc2u);
    const uint32_t handoff = Lufia2TextReadScenePair(memory, cpu, 0xafc5u);
    if (handoff)
        return handoff;
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E120A));
    OpSta(memory, cpu, OpDp(cpu, DP_SCENE_ACTION_X));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_SCENE_ACTOR_AUXILIARY));
    OpSta(memory, cpu, OpDp(cpu, DP_SCENE_ACTION_Y));
    OpSepWidths(cpu, 0x20u);
    Lufia2TextNextByte(memory, cpu, 0xafd6u);
    OpSta(memory, cpu, OpDp(cpu, DP_SCENE_ACTION_PARAMETER));
    Lufia2TextNextByte(memory, cpu, 0xafdbu);
    OpSta(memory, cpu, OpDp(cpu, DP_SCENE_ACTION_PARAMETER + 1u));
    PushY(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x80u, 0xafe2u);
    return 0x80d533u;
}

uint32_t Lufia2TextExecuteRenderCommand(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                        uint16_t handler) {
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal ||
        cpu->stack < 0x1f10u || cpu->stack > 0x1ffcu)
        return 0u;
    switch (handler) {
    case COPY_SCENE_TILES:
    case COPY_SCENE_TILES_AND_UPLOAD:
    case COPY_SCENE_TILES_AND_REDRAW:
        return CopySceneTiles(memory, cpu, handler);
    case REFRESH_ATTRIBUTES:
        return RefreshAttributes(memory, cpu);
    case UPLOAD_LAYERS:
        return UploadLayers(memory, cpu);
    case SCENE_PAIR_ACTION:
        return ScenePairAction(memory, cpu);
    default:
        return 0u;
    }
}
