#include "lufia2/menu.h"
#include "menu/menu_action_internal.h"
#include "system/wram.h"

enum {
    SPECIAL_TRANSITION_KIND = WRAM_UNK_7E09C9,
    SPECIAL_SPELL_COST = WRAM_RECORD_BUFFER + 0x0du,
    DP_SPECIAL_SOUND_COUNT = 4u
};

static Lufia2ExecutionResult ReturnAfterCommit(MenuActionCall *call, uint32_t site,
                                               uint32_t target, uint32_t exit) {
    if (!CallChildWithFrame(call->memory, call->cpu, call->child, call->context, site,
                            target, 3u, 0x82u))
        return (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
    return ExecutionReturned(exit);
}

Lufia2ExecutionResult Lufia2MenuConfirmSpecialItemUse(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu,
                                                      Lufia2PushedChildCall child,
                                                      void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82aef3u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpLoadA(cpu, 2u);
    if (!MenuActionChild(&call, 0x82aef5u, 0x80953bu, 3u, 1u))
        return call.result;
    return ReturnAfterCommit(&call, 0x82aef9u, 0x81f034u, 0x82aefdu);
}

Lufia2ExecutionResult Lufia2MenuConfirmSpecialSpellUse(const Lufia2Memory *memory,
                                                       Lufia2CpuState *cpu,
                                                       Lufia2PushedChildCall child,
                                                       void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82af78u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpLoadA(cpu, 2u);
    if (!MenuActionChild(&call, 0x82af7au, 0x80953bu, 3u, 1u))
        return call.result;
    OpLda(memory, cpu, OpAbs(cpu, SPECIAL_SPELL_COST));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_MP_COST));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_MP_COST + 1u));
    return ReturnAfterCommit(&call, 0x82af87u, 0x81f3bcu, 0x82af8bu);
}

static Lufia2ExecutionResult LeaveFieldMenu(MenuActionCall *call, bool spell) {
    static const uint32_t item_sites[] = {0x82aed8u, 0x82aedbu, 0x82aedfu, 0x82aee3u,
                                          0x82aee9u};
    static const uint32_t spell_sites[] = {0x82af5cu, 0x82af5fu, 0x82af63u, 0x82af67u,
                                           0x82af6du};
    const uint32_t *sites = spell ? spell_sites : item_sites;
    if (!MenuActionChild(call, sites[0], spell ? 0x82af78u : 0x82aef3u, 2u, 1u) ||
        !MenuActionChild(call, sites[1], 0x868b55u, 3u, 1u) ||
        !MenuActionChild(call, sites[2], 0x868b32u, 3u, 1u) ||
        !MenuActionChild(call, sites[3], 0x82838fu, 2u, 1u))
        return call->result;
    OpLdy(call->cpu, 0u);
    if (!MenuActionChild(call, sites[4], 0x8ee710u, 3u, 1u))
        return call->result;
    for (unsigned word = 0u; word < (spell ? 6u : 5u); ++word)
        OpPullX(call->memory, call->cpu);
    return ExecutionReturned(spell ? 0x82af77u : 0x82aef2u);
}

static Lufia2ExecutionResult PlayObjectConditionSounds(MenuActionCall *call) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    OpLoadA(cpu, 0x89u);
    if (!MenuActionChild(call, 0x82aeb4u, 0x80953bu, 3u, 1u) ||
        !MenuActionChild(call, 0x82aeb8u, 0x8eb7e2u, 3u, 1u))
        return call->result;
    OpCmpValue(cpu, 0u);
    if (!cpu->zero) {
        OpSta(memory, cpu, OpDp(cpu, DP_SPECIAL_SOUND_COUNT));
        OpLoadA(cpu, 0x5au);
        if (!MenuActionChild(call, 0x82aec4u, 0x829353u, 2u, 1u))
            return call->result;
        do {
            OpLoadA(cpu, 0x8au);
            if (!MenuActionChild(call, 0x82aec9u, 0x80953bu, 3u, 1u))
                return call->result;
            OpLoadA(cpu, 0x10u);
            if (!MenuActionChild(call, 0x82aecfu, 0x829353u, 2u, 1u))
                return call->result;
            OpStepMem(memory, cpu, OpDp(cpu, DP_SPECIAL_SOUND_COUNT), -1);
        } while (!cpu->zero);
    }
    cpu->carry = 0u;
    return ExecutionReturned(0x82aed7u);
}

Lufia2ExecutionResult Lufia2MenuHandleSpecialItemEffect(const Lufia2Memory *memory,
                                                        Lufia2CpuState *cpu,
                                                        Lufia2PushedChildCall child,
                                                        void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82ae6fu);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    if (!MenuActionChild(&call, 0x82ae6fu, 0x81efdfu, 3u, 1u))
        return call.result;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_EFFECT_KIND));
    OpCmpValue(cpu, 1u);
    if (cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_WINDOW_MODE));
        OpBitValue(cpu, 0x10u);
        if (!cpu->zero) {
            cpu->carry = 1u;
            return ExecutionReturned(0x82aeffu);
        }
        OpLoadA(cpu, 2u);
        if (!MenuActionChild(&call, 0x82ae8eu, 0x80953bu, 3u, 1u) ||
            !MenuActionChild(&call, 0x82ae92u, 0x82af8eu, 2u, 1u))
            return call.result;
        if (cpu->carry) {
            cpu->carry = 0u;
            return ExecutionReturned(0x82ae98u);
        }
    } else {
        OpCmpValue(cpu, 2u);
        if (!cpu->zero) {
            OpCmpValue(cpu, 0x26u);
            if (cpu->zero)
                return PlayObjectConditionSounds(&call);
            cpu->carry = 1u;
            return ExecutionReturned(0x82aeffu);
        }
        OpLda(memory, cpu, OpAbs(cpu, WRAM_WINDOW_MODE));
        OpBitValue(cpu, 8u);
        if (!cpu->zero) {
            cpu->carry = 1u;
            return ExecutionReturned(0x82aeffu);
        }
        OpLoadA(cpu, 2u);
        OpSta(memory, cpu, OpAbs(cpu, SPECIAL_TRANSITION_KIND));
        OpLoadA(cpu, 1u);
        OpLdx(cpu, 8u);
        OpLdy(cpu, 9u);
        if (!MenuActionChild(&call, 0x82aeadu, 0x8289bcu, 2u, 1u))
            return call.result;
    }
    return LeaveFieldMenu(&call, false);
}

Lufia2ExecutionResult Lufia2MenuHandleSpecialSpellEffect(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu,
                                                         Lufia2PushedChildCall child,
                                                         void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82af00u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    if (!MenuActionChild(&call, 0x82af00u, 0x81f34du, 3u, 1u))
        return call.result;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_EFFECT_KIND));
    OpCmpValue(cpu, 1u);
    if (cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_WINDOW_MODE));
        OpBitValue(cpu, 0x10u);
        if (!cpu->zero) {
            cpu->carry = 1u;
            return ExecutionReturned(0x82af8du);
        }
        OpLoadA(cpu, 2u);
        if (!MenuActionChild(&call, 0x82af1fu, 0x80953bu, 3u, 1u) ||
            !MenuActionChild(&call, 0x82af23u, 0x82af8eu, 2u, 1u))
            return call.result;
        if (cpu->carry) {
            cpu->carry = 0u;
            return ExecutionReturned(0x82af29u);
        }
    } else {
        OpCmpValue(cpu, 2u);
        const bool escape = cpu->zero;
        if (!escape) {
            OpCmpValue(cpu, 0x24u);
            if (!cpu->zero) {
                cpu->carry = 1u;
                return ExecutionReturned(0x82af8du);
            }
        }
        OpLda(memory, cpu, OpAbs(cpu, WRAM_WINDOW_MODE));
        OpBitValue(cpu, escape ? 8u : 0x80u);
        if (!cpu->zero) {
            cpu->carry = 1u;
            return ExecutionReturned(0x82af8du);
        }
        OpLoadA(cpu, escape ? 2u : 3u);
        OpSta(memory, cpu, OpAbs(cpu, SPECIAL_TRANSITION_KIND));
        OpLoadA(cpu, 1u);
        OpLdx(cpu, 8u);
        OpLdy(cpu, 9u);
        if (!MenuActionChild(&call, escape ? 0x82af3eu : 0x82af57u, 0x8289bcu, 2u, 1u))
            return call.result;
    }
    return LeaveFieldMenu(&call, true);
}

Lufia2ExecutionResult Lufia2MenuDispatchItemAction(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    (void)context;
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82b14bu);
    SimulateJsrFrame(memory, cpu, 0xb14du);
    return Lufia2MenuDispatchInlineTable(memory, cpu);
}
