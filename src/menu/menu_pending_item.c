#include "lufia2/menu.h"
#include "menu/menu_action_internal.h"
#include "system/wram.h"

enum {
    PENDING_ITEM_FLAGS = WRAM_RECORD_BUFFER + 0x0du,
    PENDING_ITEM_EFFECT = WRAM_RECORD_BUFFER + 0x0fu,
    PENDING_ITEM_CONTEXT = WRAM_BATTLE_PARTY_IDS + 1u,
    DP_PENDING_TARGET = 0u,
    DP_PENDING_TEXT_BANK = 0x5fu
};

static bool SelectPendingItemTarget(MenuActionCall *call, uint32_t site) {
    OpStz(call->memory, call->cpu, OpAbs(call->cpu, WRAM_MENU_EFFECT_TARGET_MASK));
    OpLdx(call->cpu,
          OpReadX(call->memory, call->cpu, OpAbs(call->cpu, WRAM_MENU_LIST_INDEX)));
    return MenuActionChild(call, site, 0x82ae64u, 2u, 1u);
}

static Lufia2ExecutionResult FinishPendingItemUse(MenuActionCall *call) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    OpLda(memory, cpu, OpAbs(cpu, PENDING_ITEM_EFFECT));
    OpCmpValue(cpu, 0u);
    if (cpu->zero) {
        if (!MenuActionChild(call, 0x82b223u, 0x8297e3u, 2u, 1u))
            return call->result;
    } else {
        OpLoadA(cpu, 2u);
        if (!MenuActionChild(call, 0x82b232u, 0x80953bu, 3u, 1u) ||
            !MenuActionChild(call, 0x82b236u, 0x829e07u, 2u, 1u))
            return call->result;
        for (;;) {
            OpLoadA(cpu, 7u);
            OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E14BF));
            if (!MenuActionChild(call, 0x82b23eu, 0x829246u, 2u, 1u))
                return call->result;
            if (cpu->carry)
                break;
            if (!SelectPendingItemTarget(call, 0x82b249u) ||
                !MenuActionChild(call, 0x82b24cu, 0x81efdfu, 3u, 1u))
                return call->result;
            if (cpu->carry) {
                if (!MenuActionChild(call, 0x82b252u, 0x82b2c5u, 2u, 1u))
                    return call->result;
                if (cpu->carry) {
                    if (!MenuActionChild(call, 0x82b257u, 0x8297e3u, 2u, 1u))
                        return call->result;
                    continue;
                }
            }
            OpLoadA(cpu, 2u);
            if (!MenuActionChild(call, 0x82b25eu, 0x80953bu, 3u, 1u) ||
                !SelectPendingItemTarget(call, 0x82b268u) ||
                !MenuActionChild(call, 0x82b26bu, 0x81ef93u, 3u, 1u) ||
                !SelectPendingItemTarget(call, 0x82b275u) ||
                !MenuActionChild(call, 0x82b278u, 0x829588u, 2u, 1u) ||
                !MenuActionChild(call, 0x82b27bu, 0x8295bfu, 2u, 1u) ||
                !MenuActionChild(call, 0x82b27eu, 0x82999bu, 2u, 1u))
                return call->result;
            OpLoadA(cpu, 4u);
            if (!MenuActionChild(call, 0x82b283u, 0x82a318u, 2u, 1u) ||
                !MenuActionChild(call, 0x82b286u, 0x868b55u, 3u, 1u))
                return call->result;
            OpLdx(cpu, 0u);
            OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTED_SPELL), cpu->x);
            OpWriteX(memory, cpu, OpDp(cpu, DP_PENDING_TARGET), cpu->x);
            OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E1562));
            OpLoadA(cpu, 0x20u);
            OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
            OpLoadA(cpu, 0x8eu);
            OpSta(memory, cpu, OpDp(cpu, DP_PENDING_TEXT_BANK));
            OpLdy(cpu, 0xc9a4u);
            OpLdx(cpu, 0x33a0u);
            if (!MenuActionChild(call, 0x82b2a4u, 0x808878u, 3u, 1u))
                return call->result;
            OpLdy(cpu, 0xc9b1u);
            OpLdx(cpu, 0x33a0u);
            if (!MenuActionChild(call, 0x82b2aeu, 0x808878u, 3u, 1u))
                return call->result;
            OpLoadA(cpu, 1u);
            OpLdx(cpu, 10u);
            OpLdy(cpu, 9u);
            if (!MenuActionChild(call, 0x82b2bau, 0x8289bcu, 2u, 1u))
                return call->result;
            OpLoadA(cpu, 8u);
            OpTestBits(memory, cpu, OpDp(cpu, DP_MENU_FRAME_FLAGS), 1u);
            break;
        }
    }
    OpStz(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_RESOURCE_DIRTY));
    return ExecutionReturned(0x82b2c4u);
}

Lufia2ExecutionResult Lufia2MenuUsePendingItem(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu,
                                               Lufia2PushedChildCall child,
                                               void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82b1feu);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTED_SPELL)));
    if (!cpu->zero) {
        OpWriteX(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID), cpu->x);
        if (!MenuActionChild(&call, 0x82b206u, 0x81f1c5u, 3u, 1u))
            return call.result;
        OpLda(memory, cpu, OpAbs(cpu, PENDING_ITEM_CONTEXT));
        OpCmpValue(cpu, 2u);
        const bool alternate = cpu->zero;
        OpLda(memory, cpu, OpAbs(cpu, PENDING_ITEM_FLAGS));
        OpBitValue(cpu, alternate ? 1u : 0x40u);
        if (!cpu->zero)
            return FinishPendingItemUse(&call);
    }
    if (!MenuActionChild(&call, 0x82b223u, 0x8297e3u, 2u, 1u))
        return call.result;
    OpStz(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_RESOURCE_DIRTY));
    return ExecutionReturned(0x82b2c4u);
}

Lufia2ExecutionResult Lufia2MenuCloseItemSelection(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82b1ceu);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTED_SPELL)));
    if (!cpu->zero) {
        OpRepWidths(cpu, 0x20u);
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_ITEM_QUANTITY));
        if (!MenuActionChild(&call, 0x82b1dbu, 0x82f9f2u, 2u, 0u))
            return call.result;
        OpSepWidths(cpu, 0x20u);
        if (cpu->carry) {
            if (!MenuActionChild(&call, 0x82b1fau, 0x8297e3u, 2u, 1u))
                return call.result;
            return ExecutionReturned(0x82b1fdu);
        }
    }
    OpLoadA(cpu, 2u);
    if (!MenuActionChild(&call, 0x82b1e4u, 0x80953bu, 3u, 1u))
        return call.result;
    OpLoadA(cpu, 5u);
    OpLdx(cpu, 7u);
    if (!MenuActionChild(&call, 0x82b1edu, 0x82895bu, 2u, 1u) ||
        !MenuActionChild(&call, 0x82b1f0u, 0x868b55u, 3u, 1u))
        return call.result;
    OpPullX(memory, cpu);
    if (!MenuActionChild(&call, 0x82b1f5u, 0x868b32u, 3u, 1u))
        return call.result;
    return ExecutionReturned(0x82b1f9u);
}

Lufia2ExecutionResult Lufia2MenuRunPendingItemActions(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu,
                                                      Lufia2PushedChildCall child,
                                                      void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82b183u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    if (!MenuActionChild(&call, 0x82b183u, 0x829e1bu, 2u, 1u))
        return call.result;
    for (;;) {
        OpLoadA(cpu, 0u);
        if (!MenuActionChild(&call, 0x82b188u, 0x828b08u, 2u, 1u))
            return call.result;
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            if (!MenuActionChild(&call, 0x82b19fu, 0x82b1feu, 2u, 1u))
                return call.result;
            continue;
        }
        OpCmpValue(cpu, 3u);
        if (cpu->zero)
            break;
        OpCmpValue(cpu, 4u);
        if (cpu->zero) {
            OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTED_SPELL)));
            if (!cpu->zero) {
                OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_DESCRIPTION_ID), cpu->x);
                OpLdy(cpu, 0x100eu);
                OpLdx(cpu, 1u);
                if (!MenuActionChild(&call, 0x82b1c9u, 0x82fd81u, 2u, 1u))
                    return call.result;
            }
        }
    }
    OpLoadA(cpu, 0u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_CURSOR_SLOT)));
    OpLdy(cpu, 7u);
    if (!MenuActionChild(&call, 0x82b1acu, 0x8289fau, 2u, 1u))
        return call.result;
    OpLoadA(cpu, 3u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_CURSOR_SLOT)));
    OpLdy(cpu, 7u);
    if (!MenuActionChild(&call, 0x82b1b7u, 0x8289bcu, 2u, 1u))
        return call.result;
    return ExecutionReturned(0x82b1bau);
}
