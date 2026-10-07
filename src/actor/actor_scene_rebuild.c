#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/actor.h"
#include "system/wram.h"

enum {
    SCENE_ACTOR_SLOT = 0xa7u,
    SCENE_ACTOR_WORD = 0xa9u,
    SCENE_ACTOR_PRESENTATION = WRAM_UNK_7E070A,
    SCENE_ACTOR_ATTRIBUTES = WRAM_UNK_7E120E,
    SCENE_ACTOR_DELAY = WRAM_ACTOR_PRIMARY_TIMER,
    SCENE_ACTOR_ORDER = WRAM_ACTOR_CLAIMED_OBJECT_RECORD,
    SCENE_OPTIONS = WRAM_WINDOW_MODE,
    SCENE_HIDDEN_PARTY = WRAM_UNK_7FD0FE,
    SCENE_ACTOR_OFFSET_X = WRAM_ACTOR_DISPLAY_OFFSET_X,
    SCENE_ACTOR_OFFSET_Y = WRAM_ACTOR_DISPLAY_OFFSET_Y,
    SCENE_ACTOR_SPRITE = WRAM_UNK_7E05D2,
    SCENE_ACTOR_RECORD_ID = WRAM_ACTOR_ID,
    SCENE_SAVED_MAP_IDS = WRAM_UNK_7E085E,
    SCENE_SAVED_RECORD_IDS = WRAM_UNK_7E089E,
    SCENE_SPRITE_ATTRIBUTES = 0xcff6b5u
};

static bool ActorSceneOwnerContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x83u && cpu->data_bank == 0x83u &&
        !cpu->direct_page && !cpu->decimal;
}

static Lufia2ExecutionResult ActorSceneOwnerUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static void ActorSceneSetPartyOffsets(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    bool hidden;
    if (cpu->carry) {
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, SCENE_ACTOR_WORD)));
        hidden = true;
    } else {
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, SCENE_ACTOR_SLOT)));
        OpCpx(cpu, 4u);
        if (cpu->carry)
            return;
        OpTxa(cpu);
        OpSta(memory, cpu, OpLongX(cpu, SCENE_ACTOR_ORDER));
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, SCENE_ACTOR_WORD)));
        OpLda(memory, cpu, SCENE_HIDDEN_PARTY);
        hidden = !cpu->zero;
    }
    if (hidden) {
        OpLoadA(cpu, 0xf8u);
        OpSta(memory, cpu, OpLongX(cpu, SCENE_ACTOR_OFFSET_X));
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpLongX(cpu, SCENE_ACTOR_OFFSET_X + 1u));
    } else {
        LoadA16(cpu, cpu->direct_page);
        OpSta(memory, cpu, OpLongX(cpu, SCENE_ACTOR_OFFSET_X));
        OpSta(memory, cpu, OpLongX(cpu, SCENE_ACTOR_OFFSET_X + 1u));
    }
    OpLoadA(cpu, 0xf0u);
    OpSta(memory, cpu, OpLongX(cpu, SCENE_ACTOR_OFFSET_Y));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpLongX(cpu, SCENE_ACTOR_OFFSET_Y + 1u));
}

static void ActorSceneCopyAttributes(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const uint32_t destinations[] = {WRAM_ACTOR_BOX_MIN_X, WRAM_ACTOR_BOX_MIN_Y, WRAM_ACTOR_BOX_MAX_X, WRAM_ACTOR_BOX_MAX_Y};
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, SCENE_ACTOR_SLOT)));
    for (unsigned field = 0u; field < 4u; ++field) {
        OpLda(memory, cpu, SCENE_ACTOR_ATTRIBUTES + field);
        OpSta(memory, cpu, OpLongX(cpu, destinations[field]));
    }
}

Lufia2ExecutionResult Lufia2ActorRebuildSceneState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ActorSceneOwnerContext(cpu))
        return ExecutionHandoff(cpu, 0x83a82eu);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1u);
    SetIndexWidth(cpu, 1u);
    if (!CallChildWithFrame(memory, cpu, child, context, 0x83a831u, 0x83ac7au, 3u, 0x83u))
        return ActorSceneOwnerUnwound(0x83a831u);
    if (!CallChildWithFrame(memory, cpu, child, context, 0x83a835u, 0x83f7d4u, 3u, 0x83u))
        return ActorSceneOwnerUnwound(0x83a835u);
    OpLdx(cpu, 39u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, SCENE_ACTOR_SPRITE));
        OpCmpValue(cpu, 0xfeu);
        bool active = !cpu->zero;
        if (active) {
            OpCmpValue(cpu, 0xffu);
            active = !cpu->zero;
        }
        if (active) {
            OpWriteX(memory, cpu, OpDp(cpu, SCENE_ACTOR_SLOT), cpu->x);
            if (!CallChildWithFrame(memory, cpu, child, context, 0x83a84eu, 0x83ab4fu, 3u, 0x83u))
                return ActorSceneOwnerUnwound(0x83a84eu);
            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, SCENE_ACTOR_SLOT)));
            OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
            OpAndValue(cpu, 0x7fu);
            OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
            OpLda(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E066A));
            OpAndValue(cpu, 6u);
            OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E066A));
            OpLda(memory, cpu, OpAbsX(cpu, SCENE_ACTOR_SPRITE));
            if (!CallChildWithFrame(memory, cpu, child, context, 0x83a867u, 0x83a9d0u, 3u, 0x83u))
                return ActorSceneOwnerUnwound(0x83a867u);
            OpLoadA(cpu, 0x20u);
            OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7FE316));
            if (!CallChildWithFrame(memory, cpu, child, context, 0x83a871u, 0x83a6dfu, 2u, 0x83u))
                return ActorSceneOwnerUnwound(0x83a871u);
            if (!CallChildWithFrame(memory, cpu, child, context, 0x83a874u, 0x83a746u, 3u, 0x83u))
                return ActorSceneOwnerUnwound(0x83a874u);
            if (!CallChildWithFrame(memory, cpu, child, context, 0x83a878u, 0x83aa30u, 3u, 0x83u))
                return ActorSceneOwnerUnwound(0x83a878u);
            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, SCENE_ACTOR_SLOT)));
            OpCpx(cpu, 5u);
            if (!cpu->carry) {
                OpTxa(cpu);
                OpSta(memory, cpu, OpLongX(cpu, SCENE_ACTOR_ORDER));
                LoadA16(cpu, cpu->direct_page);
            } else {
                OpLda(memory, cpu, OpAbs(cpu, SCENE_OPTIONS));
                OpBitValue(cpu, 1u);
                if (!cpu->zero) {
                    LoadA16(cpu, cpu->direct_page);
                } else {
                    OpLoadA(cpu, 60u);
                    if (!CallChildWithFrame(memory, cpu, child, context, 0x83a896u, 0x808299u, 3u, 0x83u))
                        return ActorSceneOwnerUnwound(0x83a896u);
                }
            }
            OpIncA(cpu);
            OpSta(memory, cpu, OpLongX(cpu, SCENE_ACTOR_DELAY));
            if (!CallChildWithFrame(memory, cpu, child, context, 0x83a89fu, 0x83a97eu, 2u, 0x83u))
                return ActorSceneOwnerUnwound(0x83a89fu);
            ActorSceneSetPartyOffsets(memory, cpu);
            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, SCENE_ACTOR_SLOT)));
            LoadA16(cpu, cpu->direct_page);
            OpLda(memory, cpu, OpAbsX(cpu, SCENE_ACTOR_RECORD_ID));
            OpCmpValue(cpu, 0x70u);
            bool apply_presentation = cpu->carry;
            if (!apply_presentation) {
                OpCmpValue(cpu, 0x10u);
                if (cpu->carry) {
                    OpCmpValue(cpu, 0x50u);
                    if (!cpu->carry) {
                        cpu->carry = 1u;
                        OpSbcValue(cpu, 0x10u);
                        OpTax(cpu);
                        OpLda(memory, cpu, OpAbsX(cpu, SCENE_SAVED_MAP_IDS));
                        OpCmpValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID)));
                        apply_presentation = cpu->zero;
                        if (apply_presentation)
                            OpLda(memory, cpu, OpAbsX(cpu, SCENE_SAVED_RECORD_IDS));
                    } else {
                        cpu->carry = 1u;
                        OpSbcValue(cpu, 0x4fu);
                        apply_presentation = true;
                    }
                    if (apply_presentation) {
                        SetIndexWidth(cpu, 0u);
                        if (!CallChildWithFrame(memory, cpu, child, context, 0x83a906u, 0x80c01du, 3u, 0x83u))
                            return ActorSceneOwnerUnwound(0x83a906u);
                        ActorSceneCopyAttributes(memory, cpu);
                    }
                }
            }
            if (apply_presentation) {
                OpLda(memory, cpu, OpAbs(cpu, SCENE_OPTIONS));
                OpBitValue(cpu, 1u);
                bool store = true;
                bool forced = false;
                if (!cpu->zero) {
                    OpLda(memory, cpu, OpAbsX(cpu, SCENE_ACTOR_SPRITE));
                    OpCmpValue(cpu, 0xfdu);
                    store = !cpu->zero;
                    if (store) {
                        OpCmpValue(cpu, 0x80u);
                        if (!cpu->carry) {
                            OpCmpValue(cpu, 0x0au);
                            forced = cpu->carry;
                        }
                    }
                }
                if (store) {
                    if (forced) {
                        OpLoadA(cpu, 9u);
                    } else {
                        OpLda(memory, cpu, SCENE_ACTOR_ATTRIBUTES + 4u);
                        if (cpu->zero) {
                            LoadA16(cpu, cpu->direct_page);
                            OpLda(memory, cpu, OpAbsX(cpu, SCENE_ACTOR_SPRITE));
                            cpu->carry = 1u;
                            OpSbcValue(cpu, 0x80u);
                            if (cpu->carry) {
                                OpTax(cpu);
                                OpLda(memory, cpu, OpLongX(cpu, SCENE_SPRITE_ATTRIBUTES));
                                OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, SCENE_ACTOR_SLOT)));
                            } else {
                                if (!CallChildWithFrame(memory, cpu, child, context, 0x83a95eu, 0x83a97eu, 2u, 0x83u))
                                    return ActorSceneOwnerUnwound(0x83a95eu);
                                if (cpu->carry)
                                    OpLoadA(cpu, 9u);
                                else
                                    LoadA16(cpu, cpu->direct_page);
                            }
                        }
                    }
                    OpSta(memory, cpu, OpAbsX(cpu, SCENE_ACTOR_PRESENTATION));
                }
            }
            if (!CallChildWithFrame(memory, cpu, child, context, 0x83a96bu, 0x83d416u, 3u, 0x83u))
                return ActorSceneOwnerUnwound(0x83a96bu);
            SetIndexWidth(cpu, 1u);
            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, SCENE_ACTOR_SLOT)));
        }
        OpDex(cpu);
    } while (!cpu->negative);
    if (!CallChildWithFrame(memory, cpu, child, context, 0x83a979u, 0x83a998u, 2u, 0x83u))
        return ActorSceneOwnerUnwound(0x83a979u);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x83a97du);
}
