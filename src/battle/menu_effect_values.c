#include "lufia2/battle.h"
#include "item/menu_application_internal.h"
#include "system/wram.h"

enum {
    EFFECT_RESULT_KIND = WRAM_BATTLE_MESSAGE_WINDOW_COLUMNS,
    EFFECT_RESULT_VALUE = WRAM_BATTLE_ACTION_WORK_BYTE,
    EFFECT_MEMBER_RESULT = WRAM_MENU_MEMBER_EFFECT_RESULTS,
    EFFECT_VALUE_OFFSETS = 0xa6edu,
    EFFECT_RECORD_BANK = 0x7f0000u,
    EFFECT_RECORD_SIZE = 30u
};

Lufia2ExecutionResult Lufia2BattleReadMenuEffectValue(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu,
                                                      Lufia2PushedChildCall child,
                                                      void *context) {
    (void)context;
    if (!MenuUseReady(cpu, child, 0x85u))
        return ExecutionHandoff(cpu, 0x85cd5du);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpLongX(cpu, EFFECT_RECORD_BANK));
    OpCmpValue(cpu, 5u);
    if (cpu->zero) {
        TransferDirectToA(cpu);
    } else {
        OpLda(memory, cpu, OpLongX(cpu, EFFECT_RECORD_BANK + 2u));
        if (!cpu->zero) {
            OpSta(memory, cpu, OpAbs(cpu, EFFECT_RESULT_KIND));
            OpTay(cpu);
            OpLda(memory, cpu, OpAbsY(cpu, EFFECT_VALUE_OFFSETS));
            OpRepWidths(cpu, 0x20u);
            OpWriteX(memory, cpu, OpAbs(cpu, EFFECT_RESULT_VALUE), cpu->x);
            cpu->carry = 0u;
            OpAdc(memory, cpu, OpAbs(cpu, EFFECT_RESULT_VALUE));
            OpTax(cpu);
            OpLda(memory, cpu, OpLongX(cpu, EFFECT_RECORD_BANK));
            OpSta(memory, cpu, OpAbs(cpu, EFFECT_RESULT_VALUE));
            OpSepWidths(cpu, 0x20u);
            return ExecutionReturned(0x85cd86u);
        }
    }
    OpSta(memory, cpu, OpAbs(cpu, EFFECT_RESULT_KIND));
    return ExecutionReturned(0x85cd8bu);
}

Lufia2ExecutionResult Lufia2BattleCollectMenuEffectValues(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu,
                                                          Lufia2PushedChildCall child,
                                                          void *context) {
    if (!MenuUseReady(cpu, child, 0x85u))
        return ExecutionHandoff(cpu, 0x85cd10u);
    MenuUseCall call = {memory, cpu, child, context, 0x85u, {0}};
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    for (uint16_t member = 0u; member < 4u; ++member) {
        OpLdx(cpu, (uint16_t)(WRAM_BATTLE_ACTION_RESULT_RECORDS +
                              member * EFFECT_RECORD_SIZE));
        if (!MenuUseChild(&call, 0x85cd16u + member * 18u, 0x85cd5du, 2u, 1u))
            return call.result;
        OpLda(memory, cpu, OpAbs(cpu, EFFECT_RESULT_KIND));
        OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(EFFECT_MEMBER_RESULT + member * 3u)));
        OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, EFFECT_RESULT_VALUE)));
        OpWriteX(memory, cpu,
                 OpAbs(cpu, (uint16_t)(EFFECT_MEMBER_RESULT + member * 3u + 1u)),
                 cpu->x);
    }
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x85cd5cu);
}
