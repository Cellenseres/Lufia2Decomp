#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/text.h"
#include "system/wram.h"
#include "text/text_internal.h"

enum {
    SCENE_SETUP_ACTOR_SLOT = 0xa7u,
    SCENE_SETUP_UPLOAD = 0x73u,
    SCENE_SETUP_PALETTE = 0x0500u,
    ROM_SCENE_SETUP_PALETTE = 0xbfe0u
};

static bool SceneSetupContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x80u && cpu->accumulator_is_8_bit &&
           !cpu->index_is_8_bit && !cpu->direct_page && !cpu->decimal &&
           cpu->stack >= 0x1f10u && cpu->stack <= 0x1ffcu;
}

static bool SceneSetupChild(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                            Lufia2PushedChildCall child, void *context,
                            uint32_t site, uint32_t target, uint8_t frame,
                            Lufia2ExecutionResult *result) {
    if (!CallChildWithFrame(memory, cpu, child, context, site, target, frame, 0x80u)) {
        *result = ExecutionReturned(site);
        result->flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal) {
        *result = ExecutionHandoff(cpu, site + frame + 1u);
        return false;
    }
    return true;
}

Lufia2ExecutionResult Lufia2TextPlaceActorFromScript(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SceneSetupContext(cpu))
        return ExecutionHandoff(cpu, 0x80bc98u);
    Lufia2ExecutionResult result;
    if (!SceneSetupChild(memory, cpu, child, context, 0x80bc98u, 0x8482d5u, 3u,
                         &result))
        return result;
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, SCENE_SETUP_ACTOR_SLOT)));
    if (!SceneSetupChild(memory, cpu, child, context, 0x80bc9eu, 0x80c0b7u, 2u,
                         &result))
        return result;
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_ID));
    if (!SceneSetupChild(memory, cpu, child, context, 0x80bca5u, 0x80c0b7u, 2u,
                         &result))
        return result;
    OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7E05D2));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_ACTOR_ID));
    cpu->carry = 1u;
    OpSbcValue(cpu, 0x4fu);
    if (!SceneSetupChild(memory, cpu, child, context, 0x80bcb3u, 0x80c01du, 3u,
                         &result) ||
        !SceneSetupChild(memory, cpu, child, context, 0x80bcb7u, 0x80c1a7u, 3u,
                         &result))
        return result;
    return ExecutionReturned(0x80bcbbu);
}

Lufia2ExecutionResult Lufia2TextPlaceNextSceneActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SceneSetupContext(cpu))
        return ExecutionHandoff(cpu, 0x80bc3du);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_RELOAD_FLAGS));
    OpBitValue(cpu, 0x10u);
    Lufia2ExecutionResult result;
    if (cpu->zero) {
        if (!SceneSetupChild(memory, cpu, child, context, 0x80bc44u, 0x80bc98u, 2u,
                             &result))
            return result;
        OpStepMem(memory, cpu, OpDp(cpu, SCENE_SETUP_ACTOR_SLOT), 1);
    } else {
        if (!SceneSetupChild(memory, cpu, child, context, 0x80bc4cu, 0x80c0b7u, 2u,
                             &result) ||
            !SceneSetupChild(memory, cpu, child, context, 0x80bc4fu, 0x80c0b7u, 2u,
                             &result))
            return result;
    }
    return ExecutionReturned(LUFIA2_TEXT_COMMAND_NEXT);
}

Lufia2ExecutionResult Lufia2TextSetWindowMode(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SceneSetupContext(cpu))
        return ExecutionHandoff(cpu, 0x80bcbcu);
    Lufia2ExecutionResult result;
    if (!SceneSetupChild(memory, cpu, child, context, 0x80bcbcu, 0x80c0b7u, 2u,
                         &result))
        return result;
    OpSta(memory, cpu, OpAbs(cpu, WRAM_WINDOW_MODE));
    OpBitValue(cpu, 2u);
    if (!cpu->zero) {
        OpRepWidths(cpu, 0x30u);
        PushY(memory, cpu);
        PushDataBank(memory, cpu);
        OpLdx(cpu, ROM_SCENE_SETUP_PALETTE);
        OpLdy(cpu, SCENE_SETUP_PALETTE);
        OpLoadA(cpu, 0x1fu);
        OpMoveNext(memory, cpu, 0u, 0xa6u);
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, 0x80u);
        OpSta(memory, cpu, OpDp(cpu, SCENE_SETUP_UPLOAD));
        if (!SceneSetupChild(memory, cpu, child, context, 0x80bcdcu, 0x80bf0bu, 2u,
                             &result))
            return result;
        PullDataBank(memory, cpu);
        OpPullY(memory, cpu);
    }
    return ExecutionReturned(LUFIA2_TEXT_COMMAND_NEXT);
}
