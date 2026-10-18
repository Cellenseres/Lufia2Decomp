#include "lufia2/item.h"
#include "item/menu_application_internal.h"
#include "system/wram.h"

enum {
    USE_TARGET_RESULT = 0x54u,
    USE_MP_COST = 0x00u,
    USE_CHARACTER_MP = 0x13u,
    USE_RECORD_SPELL_COST = WRAM_RECORD_BUFFER + 0x0du,
    USE_RECORD_SPELL_SCRIPT = WRAM_RECORD_BUFFER + 0x12u,
    USE_RECORD_ITEM_SCRIPT = WRAM_RECORD_BUFFER + 0x16u,
    USE_ACTION_TARGET = WRAM_BATTLE_ACTION_WORK + 2u
};

static void SaveUseRegisters(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                             bool accumulator) {
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    if (accumulator)
        PushAccumulator16(memory, cpu);
    OpPushX(memory, cpu);
    PushY(memory, cpu);
}

static void RestoreUseRegisters(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                bool accumulator) {
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    if (accumulator)
        PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
}

static void FinishTargetCheck(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    cpu->carry = 1u;
    OpLda(memory, cpu, OpDp(cpu, USE_TARGET_RESULT));
    if (!cpu->zero)
        cpu->carry = 0u;
}

Lufia2ExecutionResult Lufia2ApplyMenuItem(const Lufia2Memory *memory,
                                          Lufia2CpuState *cpu,
                                          Lufia2PushedChildCall child, void *context) {
    if (!MenuUseReady(cpu, child, 0x81u))
        return ExecutionHandoff(cpu, 0x81ef93u);
    MenuUseCall call = {memory, cpu, child, context, 0x81u, {0}};
    PushDataBank(memory, cpu);
    SaveUseRegisters(memory, cpu, true);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID)));
    OpPushX(memory, cpu);
    OpLdx(cpu, 0u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_EFFECT_TARGET_MASK));
    OpSta(memory, cpu, USE_ACTION_TARGET);
    if (!MenuUseChild(&call, 0x81efa8u, 0x85ccceu, 3u, 0u) ||
        !MenuUseChild(&call, 0x81efacu, 0x81f1c5u, 3u, 0u))
        return call.result;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_POINTER));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_MESSAGE_NAME_POINTER));
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, USE_RECORD_ITEM_SCRIPT)));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x96u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_MESSAGE_NAME_BANK));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_EFFECT_KIND));
    if (!MenuUseChild(&call, 0x81efc3u, 0x81fac9u, 3u, 1u) ||
        !MenuUseChild(&call, 0x81efc7u, 0x85cd8cu, 3u, 1u) ||
        !MenuUseChild(&call, 0x81efcbu, 0x85ce21u, 3u, 1u) ||
        !MenuUseChild(&call, 0x81efcfu, 0x85cd10u, 3u, 1u))
        return call.result;
    OpPullX(memory, cpu);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID), cpu->x);
    OpRepWidths(cpu, 0x30u);
    RestoreUseRegisters(memory, cpu, true);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81efdeu);
}

Lufia2ExecutionResult Lufia2CheckMenuItemTargets(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu,
                                                 Lufia2PushedChildCall child,
                                                 void *context) {
    if (!MenuUseReady(cpu, child, 0x81u))
        return ExecutionHandoff(cpu, 0x81efdfu);
    MenuUseCall call = {memory, cpu, child, context, 0x81u, {0}};
    PushDataBank(memory, cpu);
    SaveUseRegisters(memory, cpu, false);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID)));
    OpPushX(memory, cpu);
    OpLdx(cpu, 0u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_EFFECT_TARGET_MASK));
    OpSta(memory, cpu, USE_ACTION_TARGET);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ACTION_TARGET_REQUEST));
    if (!MenuUseChild(&call, 0x81eff6u, 0x85ccceu, 3u, 0u) ||
        !MenuUseChild(&call, 0x81effau, 0x81f1c5u, 3u, 0u))
        return call.result;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_POINTER));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_MESSAGE_NAME_POINTER));
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, USE_RECORD_ITEM_SCRIPT)));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x96u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_MESSAGE_NAME_BANK));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_EFFECT_KIND));
    if (!MenuUseChild(&call, 0x81f011u, 0x81fac9u, 3u, 1u) ||
        !MenuUseChild(&call, 0x81f015u, 0x85cd8cu, 3u, 1u) ||
        !MenuUseChild(&call, 0x81f019u, 0x85d711u, 3u, 1u))
        return call.result;
    OpStz(memory, cpu, OpDp(cpu, USE_TARGET_RESULT));
    if (cpu->carry)
        OpStepMem(memory, cpu, OpDp(cpu, USE_TARGET_RESULT), 1u);
    OpPullX(memory, cpu);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID), cpu->x);
    OpRepWidths(cpu, 0x30u);
    RestoreUseRegisters(memory, cpu, false);
    PullDataBank(memory, cpu);
    FinishTargetCheck(memory, cpu);
    return ExecutionReturned(0x81f033u);
}

Lufia2ExecutionResult Lufia2ApplyMenuSpell(const Lufia2Memory *memory,
                                           Lufia2CpuState *cpu,
                                           Lufia2PushedChildCall child, void *context) {
    if (!MenuUseReady(cpu, child, 0x81u))
        return ExecutionHandoff(cpu, 0x81f2e7u);
    MenuUseCall call = {memory, cpu, child, context, 0x81u, {0}};
    PushDataBank(memory, cpu);
    SaveUseRegisters(memory, cpu, true);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_RECORD_ID));
    PushAccumulator8(memory, cpu);
    OpLdx(cpu, 0u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_LIST_ID));
    OpSta(memory, cpu, WRAM_BATTLE_ACTION_WORK);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_EFFECT_TARGET_MASK));
    OpSta(memory, cpu, USE_ACTION_TARGET);
    if (!MenuUseChild(&call, 0x81f305u, 0x85ccceu, 3u, 1u) ||
        !MenuUseChild(&call, 0x81f309u, 0x81f414u, 3u, 1u))
        return call.result;
    OpLda(memory, cpu, OpAbs(cpu, USE_RECORD_SPELL_COST));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_MP_COST));
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_SPELL_RECORD_POINTER)));
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_BATTLE_MESSAGE_NAME_POINTER), cpu->y);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, USE_RECORD_SPELL_SCRIPT)));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x95u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_MESSAGE_NAME_BANK));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_EFFECT_KIND));
    if (!MenuUseChild(&call, 0x81f326u, 0x81fac9u, 3u, 1u) ||
        !MenuUseChild(&call, 0x81f32au, 0x85cd8cu, 3u, 1u))
        return call.result;
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x01dcu);
    OpSta(memory, cpu, WRAM_BATTLE_ACTION_RESULT_FLAGS);
    OpSepWidths(cpu, 0x20u);
    if (!MenuUseChild(&call, 0x81f339u, 0x85ce21u, 3u, 1u) ||
        !MenuUseChild(&call, 0x81f33du, 0x85cd10u, 3u, 1u))
        return call.result;
    LoadA8(cpu, Pull8(memory, cpu));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_RECORD_ID));
    OpRepWidths(cpu, 0x30u);
    RestoreUseRegisters(memory, cpu, true);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81f34cu);
}

Lufia2ExecutionResult Lufia2CheckMenuSpellTargets(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu,
                                                  Lufia2PushedChildCall child,
                                                  void *context) {
    if (!MenuUseReady(cpu, child, 0x81u))
        return ExecutionHandoff(cpu, 0x81f34du);
    MenuUseCall call = {memory, cpu, child, context, 0x81u, {0}};
    PushDataBank(memory, cpu);
    SaveUseRegisters(memory, cpu, true);
    if (!MenuUseChild(&call, 0x81f354u, 0x85ccceu, 3u, 0u))
        return call.result;
    OpStz(memory, cpu, OpDp(cpu, USE_TARGET_RESULT));
    OpSepWidths(cpu, 0x20u);
    if (!MenuUseChild(&call, 0x81f35cu, 0x81f414u, 3u, 1u))
        return call.result;
    OpLda(memory, cpu, OpAbs(cpu, USE_RECORD_SPELL_COST));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_MP_COST));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_MP_COST + 1u));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_LIST_ID));
    if (!MenuUseChild(&call, 0x81f36cu, 0x81faf5u, 2u, 1u))
        return call.result;
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, USE_CHARACTER_MP));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_MP_COST)));
    if (cpu->carry) {
        OpStepMem(memory, cpu, OpDp(cpu, USE_TARGET_RESULT), 1u);
        OpLdx(cpu, 0u);
        OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_EFFECT_TARGET_MASK));
        OpSta(memory, cpu, USE_ACTION_TARGET);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ACTION_TARGET_REQUEST));
        OpLda(memory, cpu, OpAbs(cpu, WRAM_SPELL_RECORD_POINTER));
        OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_MESSAGE_NAME_POINTER));
        OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, USE_RECORD_SPELL_SCRIPT)));
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, 0x95u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_MESSAGE_NAME_BANK));
        OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_EFFECT_KIND));
        if (!MenuUseChild(&call, 0x81f39cu, 0x81fac9u, 3u, 1u) ||
            !MenuUseChild(&call, 0x81f3a0u, 0x85cd8cu, 3u, 1u) ||
            !MenuUseChild(&call, 0x81f3a4u, 0x85d711u, 3u, 1u))
            return call.result;
        OpStz(memory, cpu, OpDp(cpu, USE_TARGET_RESULT));
        if (cpu->carry)
            OpStepMem(memory, cpu, OpDp(cpu, USE_TARGET_RESULT), 1u);
        OpRepWidths(cpu, 0x30u);
    }
    RestoreUseRegisters(memory, cpu, true);
    PullDataBank(memory, cpu);
    FinishTargetCheck(memory, cpu);
    return ExecutionReturned(0x81f3bbu);
}

Lufia2ExecutionResult Lufia2ConsumeMenuSpellMp(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu,
                                               Lufia2PushedChildCall child,
                                               void *context) {
    if (!MenuUseReady(cpu, child, 0x81u))
        return ExecutionHandoff(cpu, 0x81f3bcu);
    MenuUseCall call = {memory, cpu, child, context, 0x81u, {0}};
    SaveUseRegisters(memory, cpu, true);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_LIST_ID));
    if (!MenuUseChild(&call, 0x81f3c7u, 0x81faf5u, 2u, 1u))
        return call.result;
    OpRepWidths(cpu, 0x30u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_MP_COST));
    OpAndValue(cpu, 0x00ffu);
    OpSta(memory, cpu, OpDp(cpu, USE_MP_COST));
    OpLda(memory, cpu, OpAbsX(cpu, USE_CHARACTER_MP));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, USE_MP_COST)));
    OpSta(memory, cpu, OpAbsX(cpu, USE_CHARACTER_MP));
    OpRepWidths(cpu, 0x30u);
    RestoreUseRegisters(memory, cpu, true);
    return ExecutionReturned(0x81f3e3u);
}
