#include <stdbool.h>

#include "battle/battle_internal.h"

/* Whether the 24-bit total at address is over the cap of 9,999,999
 * ($98967F). The ROM tests the retained accumulator first, which settles every
 * case but equality; the following byte compares are kept as written. */
static bool ResultOverLimit24(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                              uint32_t address) {
    if (cpu->carry)
        return true;
    OpCmpValue(cpu, 0x98u);
    if (cpu->carry)
        return true;
    if (!cpu->zero)
        return false;
    OpLda(memory, cpu, address + 1u);
    OpCmpValue(cpu, 0x96u);
    if (cpu->carry)
        return true;
    if (!cpu->zero)
        return false;
    OpLda(memory, cpu, address);
    OpCmpValue(cpu, 0x7fu);
    return cpu->carry;
}

/* Clamp the 24-bit total at address to the cap. */
static void ResultClamp24(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                          uint32_t address) {
    if (!ResultOverLimit24(memory, cpu, address))
        return;
    OpLoadA(cpu, 0x98u);
    OpSta(memory, cpu, address + 2u);
    OpLoadA(cpu, 0x96u);
    OpSta(memory, cpu, address + 1u);
    OpLoadA(cpu, 0x7fu);
    OpSta(memory, cpu, address);
}

static void ResultShift24(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                          uint32_t address) {
    const uint8_t old = Read8(memory, address);
    const uint8_t value = (uint8_t)(old << 1);

    cpu->carry = (old & 0x80u) != 0;
    Write8(memory, address, value);
    SetNz8(cpu, value);
    OpRolMem8(memory, cpu, address + 1u);
    OpRolMem8(memory, cpu, address + 2u);
}

static void ResultScaleReward(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                              uint16_t address) {
    const uint32_t base = OpAbs(cpu, address);

    ResultShift24(memory, cpu, base);
    if (!cpu->carry)
        ResultShift24(memory, cpu, base);
    ResultClamp24(memory, cpu, base);
}

static void ResultAddReward(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                            uint32_t destination, uint16_t reward) {
    unsigned i;

    for (i = 0; i < 3u; ++i) {
        OpLda(memory, cpu, destination + i);
        if (i == 0u)
            cpu->carry = 0;
        OpAdc(memory, cpu, OpAbs(cpu, (uint16_t)(reward + i)));
        OpSta(memory, cpu, destination + i);
    }
    ResultClamp24(memory, cpu, destination);
}

static bool ResultShowLineAndWait(BattleContext *battle, uint16_t text_address,
                                  uint16_t line_call_site, uint16_t wait_call_site) {
    OpLdy(battle->cpu, text_address);
    return BattleCall(battle, line_call_site, 0x81dde7u, 2u) &&
           BattleCall(battle, wait_call_site, 0x81de9eu, 2u);
}

static bool ResultShowStatGains(BattleContext *battle, bool capsule) {
    static const uint16_t texts[] = {0xf118u, 0xf134u, 0xf150u, 0xf169u,
                                     0xf182u, 0xf19bu, 0xf1b4u};
    static const uint16_t party_sites[] = {0xdb5eu, 0xdb6cu, 0xdb7au, 0xdb88u,
                                           0xdb96u, 0xdba4u, 0xdbb2u};
    static const uint16_t capsule_sites[] = {0xdc70u, 0u,      0xdc7eu, 0xdc8cu,
                                             0xdc9au, 0xdca8u, 0xdcb6u};
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    unsigned i;

    for (i = 0; i < 7u; ++i) {
        uint16_t site;
        if (capsule && i == 1u)
            continue;
        OpLda(memory, cpu, OpAbs(cpu, (uint16_t)(0x0a38u + i)));
        if (cpu->zero)
            continue;
        site = capsule ? capsule_sites[i] : party_sites[i];
        if (!ResultShowLineAndWait(battle, texts[i], site, (uint16_t)(site + 3u)))
            return false;
    }
    return true;
}

static void ResultSubtractExperience(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, 0x2du));
    cpu->carry = 1;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0x2au)));
    OpSta(memory, cpu, OpDp(cpu, 0x2du));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, 0x2fu));
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0x2cu)));
    OpSta(memory, cpu, OpDp(cpu, 0x2fu));
}

static bool ResultAwardPartyMemberExperience(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;

    OpLda(memory, cpu, OpAbsX(cpu, BATTLE_BATTLER_STATUS));
    OpAndValue(cpu, BATTLE_STATUS_DOWNED);
    if (cpu->zero) {
        ResultAddReward(memory, cpu, OpAbsX(cpu, 0x5fu), WRAM_BATTLE_EXPERIENCE_REWARD);
        for (;;) {
            PushY(memory, cpu);
            OpRepWidths(cpu, 0x20u);
            OpTya(cpu);
            OpLsrA(cpu);
            OpTay(cpu);
            OpSepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_PARTY_IDS));
            OpPushX(memory, cpu);
            if (!BattleCall(battle, 0xdb3bu, 0x81f7cau, 2u))
                return false;
            OpPullX(memory, cpu);
            OpPullY(memory, cpu);
            if (!cpu->carry)
                break;
            OpPushX(memory, cpu);
            PushY(memory, cpu);
            OpLoadA(cpu, 0x44u);
            if (!BattleCall(battle, 0xdb49u, 0x80953bu, 3u) ||
                !ResultShowLineAndWait(battle, 0xf103u, 0xdb50u, 0xdb53u) ||
                !ResultShowStatGains(battle, false))
                return false;
            OpPullY(memory, cpu);
            OpPullX(memory, cpu);
        }
    }
    PushY(memory, cpu);
    OpLda(memory, cpu, OpAbsX(cpu, 0x61u));
    OpSta(memory, cpu, OpDp(cpu, 0x2cu));
    OpLda(memory, cpu, OpAbsX(cpu, 0x64u));
    OpSta(memory, cpu, OpDp(cpu, 0x2fu));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, 0x5fu));
    OpSta(memory, cpu, OpDp(cpu, 0x2au));
    OpLda(memory, cpu, OpAbsX(cpu, 0x62u));
    OpSta(memory, cpu, OpDp(cpu, 0x2du));
    ResultSubtractExperience(memory, cpu);
    if (!ResultShowLineAndWait(battle, 0xf0b3u, 0xdbe6u, 0xdbe9u))
        return false;
    OpPullY(memory, cpu);
    return true;
}

static bool ResultAwardCapsuleExperience(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    unsigned i;

    OpLda(memory, cpu, OpAbs(cpu, 0x0a7fu));
    if (cpu->negative)
        return true;
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, 0x0a88u)));
    OpLda(memory, cpu, OpAbsX(cpu, BATTLE_BATTLER_STATUS));
    OpAndValue(cpu, BATTLE_STATUS_DOWNED);
    if (!cpu->zero)
        return true;
    ResultAddReward(memory, cpu, OpAbsX(cpu, 0x5fu), WRAM_BATTLE_EXPERIENCE_REWARD);
    for (;;) {
        if (!BattleCall(battle, 0xdc4fu, 0x82cd83u, 3u))
            return false;
        if (cpu->carry)
            break;
        OpLoadA(cpu, 0x44u);
        if (!BattleCall(battle, 0xdc57u, 0x80953bu, 3u))
            return false;
        OpLoadA(cpu, 4u);
        OpSta(memory, cpu, OpDp(cpu, 0u));
        if (!ResultShowLineAndWait(battle, 0xf103u, 0xdc62u, 0xdc65u) ||
            !ResultShowStatGains(battle, true))
            return false;
    }
    if (!BattleCall(battle, 0xdcbeu, 0x82cd1fu, 3u))
        return false;
    if (!cpu->carry) {
        if (!BattleCall(battle, 0xdcc4u, 0x8595c6u, 3u))
            return false;
        OpLoadA(cpu, 0x7eu);
        OpSta(memory, cpu, OpDp(cpu, 0x5fu));
        if (!ResultShowLineAndWait(battle, 0xf1cdu, 0xdccfu, 0xdcd2u))
            return false;
    }
    for (i = 0; i < 6u; ++i) {
        OpLda(memory, cpu, OpAbs(cpu, (uint16_t)(0x113eu + i)));
        OpSta(memory, cpu, OpDp(cpu, (uint8_t)(0x2au + i)));
    }
    OpRepWidths(cpu, 0x20u);
    ResultSubtractExperience(memory, cpu);
    OpLoadA(cpu, 4u);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    return ResultShowLineAndWait(battle, 0xf0b3u, 0xdd0bu, 0xdd0eu);
}

/* $81:D9E1: distribute battle rewards and report progression. */
Lufia2ExecutionResult Lufia2BattleResults(const Lufia2Memory *memory,
                                          Lufia2CpuState *cpu,
                                          Lufia2PushedChildCall child,
                                          void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);

    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, 0x0562u));
    OpOraValue(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, 0x0562u));
    OpSepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpAbs(cpu, 0x11deu));
    if (!BattleCall(&battle, 0xd9f4u, 0x85ec81u, 3u) ||
        !BattleCall(&battle, 0xd9f8u, 0x8591e0u, 3u) ||
        !BattleCall(&battle, 0xd9fcu, 0x858a2fu, 3u))
        return BattleChildUnwound(&battle);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, 0x0012f3u);
    OpLda(memory, cpu, OpAbs(cpu, 0x0b51u));
    OpBitValue(cpu, 3u);
    if (!cpu->zero) {
        OpBitValue(cpu, 2u);
        ResultScaleReward(memory, cpu, WRAM_BATTLE_EXPERIENCE_REWARD);
        ResultScaleReward(memory, cpu, WRAM_BATTLE_GOLD_REWARD);
    }
    if (!BattleCall(&battle, 0xda87u, 0x81dd7fu, 2u))
        return BattleChildUnwound(&battle);
    OpLoadA(cpu, 0x85u);
    OpSta(memory, cpu, OpDp(cpu, 0x5fu));
    if (!ResultShowLineAndWait(&battle, 0xf092u, 0xda91u, 0xda94u) ||
        !ResultShowLineAndWait(&battle, 0xf0a2u, 0xda9au, 0xda9du))
        return BattleChildUnwound(&battle);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ITEM_REWARD));
    if (!cpu->zero) {
        OpSta(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
        OpSepWidths(cpu, 0x20u);
        if (!BattleCall(&battle, 0xdaacu, 0x81f085u, 3u))
            return BattleChildUnwound(&battle);
        if (!cpu->carry) {
            if (!ResultShowLineAndWait(&battle, 0xf087u, 0xdab5u, 0xdab8u))
                return BattleChildUnwound(&battle);
            OpSepWidths(cpu, 0x20u);
        }
    } else {
        OpSepWidths(cpu, 0x20u);
    }
    OpLdy(cpu, 0xf085u);
    if (!BattleCall(&battle, 0xdac0u, 0x81dde7u, 2u))
        return BattleChildUnwound(&battle);
    OpLdy(cpu, 0u);
    do {
        TransferDirectToA(cpu);
        OpTya(cpu);
        OpLsrA(cpu);
        OpTax(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_PARTY_IDS));
        OpLdx(cpu, 0u);
        for (;;) {
            OpCmp(memory, cpu, OpAbsX(cpu, WRAM_MENU_PARTY_FIRST_ID));
            if (cpu->zero)
                break;
            OpInx(cpu);
        }
        OpWriteX(memory, cpu, OpDp(cpu, 0u), cpu->x);
        OpLdx(cpu, OpReadX(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_PARTY_RECORDS)));
        if (!cpu->zero && !ResultAwardPartyMemberExperience(&battle))
            return BattleChildUnwound(&battle);
        OpIny(cpu);
        OpIny(cpu);
        Compare16(cpu, cpu->y, 8u);
    } while (!cpu->zero);
    if (!ResultAwardCapsuleExperience(&battle))
        return BattleChildUnwound(&battle);
    ResultAddReward(memory, cpu, OpAbs(cpu, WRAM_GOLD), WRAM_BATTLE_GOLD_REWARD);
    OpLdy(cpu, 0xf085u);
    if (!BattleCall(&battle, 0xdd57u, 0x81dde7u, 2u) ||
        !ResultShowLineAndWait(&battle, 0xf0ebu, 0xdd5du, 0xdd60u))
        return BattleChildUnwound(&battle);
    OpLda(memory, cpu, 0x7ff8a5u);
    cpu->carry = 0;
    OpAdc(memory, cpu, OpAbs(cpu, 0x0b74u));
    if (!cpu->carry)
        OpSta(memory, cpu, OpAbs(cpu, 0x0b74u));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, 0x0562u));
    OpAndValue(cpu, 0xff7fu);
    OpSta(memory, cpu, OpAbs(cpu, 0x0562u));
    OpSepWidths(cpu, 0x20u);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81dd7eu);
}
