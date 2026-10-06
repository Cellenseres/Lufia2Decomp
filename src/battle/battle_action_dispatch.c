#include "battle/battle_internal.h"

enum {
    DISPATCH_ACTOR = WRAM_BATTLE_ACTION_WORK,
    DISPATCH_TARGET_MASK = WRAM_BATTLE_ACTION_WORK + 2u,
    DISPATCH_TYPE = WRAM_BATTLE_ACTION_WORK + 6u,
    DISPATCH_REQUEST = WRAM_BATTLE_ACTION_TARGET_REQUEST & 0xffffu,
    DISPATCH_TABLE = 0x81a867u
};

static bool ActionDispatchContext(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit &&
        !cpu->decimal && cpu->direct_page == 0u &&
        cpu->program_bank == 0x81u && cpu->data_bank == 0x97u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static bool ActionDispatchCall(BattleContext *battle, uint16_t site,
                               uint32_t target, uint8_t frame,
                               Lufia2ExecutionResult *result) {
    Lufia2CpuState *cpu = battle->cpu;

    if (!BattleCall(battle, site, target, frame)) {
        *result = BattleChildUnwound(battle);
        return false;
    }
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit ||
        cpu->decimal || cpu->direct_page != 0u) {
        *result = ExecutionHandoff(cpu,
            0x810000u | (uint16_t)(site + (frame == 2u ? 3u : 4u)));
        return false;
    }
    return true;
}

Lufia2ExecutionResult Lufia2BattleDispatchAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    BattleContext battle = BattleContextCreate(memory, cpu, child, child_context, 0x81u);
    Lufia2ExecutionResult result = {0};

    if (!ActionDispatchContext(cpu))
        return ExecutionHandoff(cpu, 0x81a832u);
    if (!ActionDispatchCall(&battle, 0xa832u, 0x85cd8cu, 3u, &result))
        return result;
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, DISPATCH_TARGET_MASK));
    Write16Absolute(memory, cpu, DISPATCH_REQUEST, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    if (!ActionDispatchCall(&battle, 0xa841u, 0x81b228u, 2u, &result))
        return result;
    Write8(memory, DISPATCH_TARGET_MASK, A8(cpu));
    LoadA8(cpu, Read8(memory, DISPATCH_ACTOR));
    Compare8(cpu, A8(cpu), 0x20u);
    if (!cpu->zero) {
        if (!ActionDispatchCall(&battle, 0xa850u, 0x81b2b5u, 3u, &result))
            return result;
        LoadAAbsolute8(memory, cpu, BATTLE_BATTLER_STATUS, cpu->x);
        BitImmediate8(cpu, BATTLE_STATUS_NO_TURN_MASK);
        if (!cpu->zero)
            return Lufia2BattleSkipAction(memory, cpu, child, child_context);
    }
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, DISPATCH_TYPE));
    And8(cpu, 0x1fu);
    AslA8(cpu);
    TransferAToX(cpu);
    const uint16_t handler = Read16Long(memory, DISPATCH_TABLE + cpu->x);

    switch (handler) {
    case 0xa8a7u: return Lufia2BattleSkipAction(memory, cpu, child, child_context);
    case 0xa8a8u: return Lufia2BattleAttackAction(memory, cpu, child, child_context);
    case 0xa968u: return Lufia2BattleBeginSpellAction(memory, cpu, child, child_context);
    case 0xa977u: return Lufia2BattleSpellAction(memory, cpu, child, child_context);
    case 0xaa1bu: return Lufia2BattleBeginItemAction(memory, cpu, child, child_context);
    case 0xaa2eu: return Lufia2BattleItemAction(memory, cpu, child, child_context);
    case 0xaad0u: return Lufia2BattleDefendAction(memory, cpu, child, child_context);
    case 0xab07u: return Lufia2BattleContinueAction(memory, cpu, child, child_context);
    case 0xab94u: return Lufia2BattleCollectiveAction(memory, cpu, child, child_context);
    case 0xac72u: return Lufia2BattleAction07(memory, cpu, child, child_context);
    case 0xab0cu: return Lufia2BattleIpAction(memory, cpu, child, child_context);
    case 0xaf7au: return Lufia2BattleBeginUncostedSpellAction(memory, cpu, child, child_context);
    case 0xaf89u: return Lufia2BattleUncostedSpellAction(memory, cpu, child, child_context);
    case 0xad3bu: return Lufia2BattleCapsuleAction(memory, cpu, child, child_context);
    case 0xad8du: return Lufia2BattleWaitAction(memory, cpu, child, child_context);
    case 0xae0fu: return Lufia2BattleAlternateAttackAction(memory, cpu, child, child_context);
    case 0xaee1u: return Lufia2BattleWaitLongAction(memory, cpu, child, child_context);
    case 0xaf37u: return Lufia2BattleFollowupAction(memory, cpu, child, child_context);
    default: return ExecutionHandoff(cpu, 0x810000u | handler);
    }
}
