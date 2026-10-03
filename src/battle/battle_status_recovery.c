#include "battle/battle_internal.h"
#include "core/snes_registers.h"

/* $85:903F/$9114: present one recovered or expired status. */
static bool StatusMessage(BattleContext *battle, uint16_t call_site, bool recovery) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    SimulateJsrFrame(memory, cpu, (uint16_t)(call_site + 2u));
    OpPushX(memory, cpu);
    if (!BattleCall(battle, recovery ? 0x9040u : 0x9115u, 0x859aaau, 3u))
        return false;
    OpLdx(cpu, 0xffffu);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_BATTLE_WAIT_COUNTER), cpu->x);
    if (!BattleCall(battle, recovery ? 0x904au : 0x911fu, 0x8595feu, 3u))
        return false;
    if (recovery) {
        if (!BattleCall(battle, 0x904eu, BATTLE_ROUTINE_SYNC_STATUS_MARKERS, 3u) ||
            !BattleCall(battle, 0x9052u, 0x81bae8u, 3u))
            return false;
        OpRepWidths(cpu, 0x20u);
        if (!BattleCall(battle, 0x9058u, BATTLE_ROUTINE_QUEUE_STATUS_SPRITES, 3u))
            return false;
        OpSepWidths(cpu, 0x20u);
        if (!BattleCall(battle, 0x905eu, BATTLE_ROUTINE_SPRITES, 3u))
            return false;
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, BATTLE_SPRITE_REBUILD_REQUEST);
        if (!BattleCall(battle, 0x9068u, BATTLE_ROUTINE_FRAME_INPUT, 3u))
            return false;
    }
    OpLoadA(cpu, 0x2du);
    do {
        PushAccumulator8(memory, cpu);
        if (!BattleCall(battle, recovery ? 0x906fu : 0x9126u, BATTLE_ROUTINE_SPRITES,
                        3u))
            return false;
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, BATTLE_SPRITE_REBUILD_REQUEST);
        if (!BattleCall(battle, recovery ? 0x9079u : 0x9130u,
                        BATTLE_ROUTINE_FRAME_INPUT, 3u))
            return false;
        LoadA8(cpu, Pull8(memory, cpu));
        OpDecA(cpu);
    } while (!cpu->zero);
    if (!BattleCall(battle, recovery ? 0x9081u : 0x9138u, 0x859671u, 3u) ||
        !BattleCall(battle, recovery ? 0x9085u : 0x913cu, BATTLE_ROUTINE_SPRITES, 3u))
        return false;
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, BATTLE_SPRITE_REBUILD_REQUEST);
    if (!BattleCall(battle, recovery ? 0x908fu : 0x9146u, BATTLE_ROUTINE_FRAME_INPUT,
                    3u) ||
        !BattleCall(battle, recovery ? 0x9093u : 0x914au, 0x859abcu, 3u))
        return false;
    OpPullX(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    return true;
}

/* Each cured status bit gets its own roll, clear and message. */
static bool StatusCureRolls(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    static const struct {
        uint8_t bit, phrase;
        uint16_t slot_site, roll_site, name_site, phrase_site, message_site;
    } cures[] = {
        {0x20u, 1u, 0x8fc4u, 0x8fcbu, 0x8fdbu, 0x8fe1u, 0x8fe5u},
        {0x10u, 3u, 0x8fefu, 0x8ff6u, 0x9006u, 0x900cu, 0x9010u},
        {0x08u, 5u, 0x901au, 0x9021u, 0x9031u, 0x9037u, 0x903bu},
    };
    for (unsigned i = 0; i < 3u; ++i) {
        /* Later BIT instructions deliberately use the child-returned A. */
        OpBitValue(cpu, cures[i].bit);
        if (cpu->zero)
            continue;
        OpTxy(cpu);
        OpLda(memory, cpu, OpDp(cpu, 3u));
        if (!BattleCall(battle, cures[i].slot_site, 0x81b2dbu, 3u))
            return false;
        OpTyx(cpu);
        OpLoadA(cpu, 7u);
        if (!BattleCall(battle, cures[i].roll_site, 0x808299u, 3u))
            return false;
        OpCmpValue(cpu, 4u);
        if (!cpu->zero)
            continue;
        OpLda(memory, cpu, OpAbsX(cpu, BATTLE_BATTLER_STATUS));
        OpAndValue(cpu, (uint8_t)~cures[i].bit);
        OpSta(memory, cpu, OpAbsX(cpu, BATTLE_BATTLER_STATUS));
        if (!BattleCall(battle, cures[i].name_site, 0x859150u, 3u))
            return false;
        OpLoadA(cpu, cures[i].phrase);
        if (!BattleCall(battle, cures[i].phrase_site, 0x859173u, 3u) ||
            !StatusMessage(battle, cures[i].message_site, true))
            return false;
    }
    return true;
}

/* A timed status (bit 7) counts down and clears when it reaches zero. */
static bool StatusExpireCountdown(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    OpBitValue(cpu, 0x80u);
    if (cpu->zero)
        return true;
    OpTxy(cpu);
    OpLda(memory, cpu, OpDp(cpu, 3u));
    if (!BattleCall(battle, 0x90f0u, 0x81b2dbu, 3u))
        return false;
    OpLda(memory, cpu, OpAbsX(cpu, 6u));
    OpDecA(cpu);
    OpSta(memory, cpu, OpAbsX(cpu, 6u));
    if (!cpu->zero)
        return true;
    OpTyx(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, BATTLE_BATTLER_STATUS));
    OpAndValue(cpu, 0x7fu);
    OpSta(memory, cpu, OpAbsX(cpu, BATTLE_BATTLER_STATUS));
    if (!BattleCall(battle, 0x9106u, 0x859150u, 3u))
        return false;
    OpLoadA(cpu, 6u);
    if (!BattleCall(battle, 0x910cu, 0x859173u, 3u) ||
        !StatusMessage(battle, 0x9110u, false))
        return false;
    return true;
}

/* Each target clears its original 30-byte effect-work record first. */
static bool StatusTarget(BattleContext *battle, uint16_t call_site, bool recovery) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    SimulateJsrFrame(memory, cpu, (uint16_t)(call_site + 2u));
    OpLda(memory, cpu, OpDp(cpu, 0u));
    OpOra(memory, cpu, OpDp(cpu, 1u));
    OpSta(memory, cpu, OpDp(cpu, 3u));
    if (!BattleCall(battle, recovery ? 0x8f90u : 0x90c2u, 0x85d9c9u, 3u))
        return false;
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_WMADDL), cpu->x);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WMADDH));
    OpLdx(cpu, 0x1eu);
    do {
        OpStz(memory, cpu, OpAbs(cpu, SNES_WMDATA));
        OpDex(cpu);
    } while (!cpu->zero);
    OpLda(memory, cpu, OpDp(cpu, 3u));
    if (!BattleCall(battle, recovery ? 0x8fa7u : 0x90d9u, 0x81b2b5u, 3u))
        return false;
    OpCpx(cpu, 0u);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpAbsX(cpu, BATTLE_BATTLER_STATUS));
        OpBitValue(cpu, 4u);
        if (cpu->zero &&
            !(recovery ? StatusCureRolls(battle) : StatusExpireCountdown(battle)))
            return false;
    }
    SimulateRtsFrame(memory, cpu);
    return true;
}

static Lufia2ExecutionResult StatusScan(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                        Lufia2PushedChildCall child,
                                        void *child_context, bool recovery) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x85u);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpStz(memory, cpu, OpDp(cpu, 2u));
    OpStz(memory, cpu, OpDp(cpu, 0u));
    OpLoadA(cpu, 0x10u);
    OpSta(memory, cpu, OpDp(cpu, 1u));
    for (unsigned side = 0; side < 2u; ++side) {
        do {
            const uint16_t site =
                recovery ? (side ? 0x8f81u : 0x8f72u) : (side ? 0x90b3u : 0x90a4u);
            if (!StatusTarget(&battle, site, recovery))
                return BattleChildUnwound(&battle);
            const uint8_t mask = Read8(memory, OpDp(cpu, 1u));
            cpu->carry = (mask & 1u) != 0u;
            Write8(memory, OpDp(cpu, 1u), (uint8_t)(mask >> 1));
            SetNz8(cpu, (uint8_t)(mask >> 1));
        } while (!cpu->carry);
        if (side == 0u) {
            OpLoadA(cpu, 0x80u);
            OpSta(memory, cpu, OpDp(cpu, 0u));
            OpLoadA(cpu, 0x20u);
            OpSta(memory, cpu, OpDp(cpu, 1u));
        }
    }
    PullDataBank(memory, cpu);
    return ExecutionReturned(recovery ? 0x858f89u : 0x8590bbu);
}

Lufia2ExecutionResult Lufia2BattleRecoverStatuses(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu,
                                                  Lufia2PushedChildCall child,
                                                  void *child_context) {
    return StatusScan(memory, cpu, child, child_context, true);
}

Lufia2ExecutionResult Lufia2BattleExpireStatuses(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu,
                                                 Lufia2PushedChildCall child,
                                                 void *child_context) {
    return StatusScan(memory, cpu, child, child_context, false);
}
