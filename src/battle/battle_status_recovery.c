#include "battle/battle_internal.h"
#include "battle/status_target_internal.h"

static bool StatusMessage(BattleContext *battle, uint16_t call_site, bool recovery,
                          Lufia2ExecutionResult *stopped) {
    SimulateJsrFrame(battle->memory, battle->cpu, (uint16_t)(call_site + 2u));
    *stopped =
        recovery ? Lufia2BattleShowStatusUpdates(battle->memory, battle->cpu,
                                                 battle->child, battle->child_context)
                 : Lufia2BattleShowStatusMessage(battle->memory, battle->cpu,
                                                 battle->child, battle->child_context);
    if (stopped->flow != LUFIA2_EXECUTION_RETURNED)
        return false;
    SimulateRtsFrame(battle->memory, battle->cpu);
    return true;
}

/* Each cured status gets its own roll and message. */
static bool StatusCureRolls(BattleContext *battle, Lufia2ExecutionResult *stopped) {
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
            !StatusMessage(battle, cures[i].message_site, true, stopped))
            return false;
    }
    return true;
}

typedef struct StatusMessageChild {
    BattleContext *battle;
    Lufia2ExecutionResult stopped;
} StatusMessageChild;

static uint8_t StatusTargetChild(void *context, Lufia2CpuState *cpu, uint32_t target,
                                 uint32_t site, uint8_t frame) {
    StatusMessageChild *call = context;
    BattleContext *battle = call->battle;
    if (target != 0x859114u || site != 0x859110u || frame != 2u)
        return battle->child(battle->child_context, cpu, target, site, frame);
    call->stopped = Lufia2BattleShowStatusMessage(battle->memory, cpu, battle->child,
                                                  battle->child_context);
    if (call->stopped.flow != LUFIA2_EXECUTION_RETURNED)
        return 0u;
    SimulateRtsFrame(battle->memory, cpu);
    return 1u;
}

static bool StatusTarget(BattleContext *battle, uint16_t call_site, bool recovery,
                         Lufia2ExecutionResult *stopped) {
    SimulateJsrFrame(battle->memory, battle->cpu, (uint16_t)(call_site + 2u));
    if (!recovery) {
        StatusMessageChild call = {battle, ExecutionReturned(0u)};
        *stopped = Lufia2BattleTickStatusMessage(battle->memory, battle->cpu,
                                                 StatusTargetChild, &call);
        if (call.stopped.flow != LUFIA2_EXECUTION_RETURNED)
            *stopped = call.stopped;
        if (stopped->flow != LUFIA2_EXECUTION_RETURNED)
            return false;
    } else {
        const BattleStatusTargetState target =
            BattlePrepareStatusTarget(battle->memory, battle->cpu, battle->child,
                                      battle->child_context, 1u, stopped);
        if (target == BATTLE_STATUS_TARGET_STOPPED)
            return false;
        if (target == BATTLE_STATUS_TARGET_READY && !StatusCureRolls(battle, stopped))
            return false;
    }
    SimulateRtsFrame(battle->memory, battle->cpu);
    return true;
}

static Lufia2ExecutionResult StatusScan(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                        Lufia2PushedChildCall child,
                                        void *child_context, bool recovery) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x85u);
    Lufia2ExecutionResult stopped = ExecutionReturned(0u);
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
            if (!StatusTarget(&battle, site, recovery, &stopped))
                return stopped.flow == LUFIA2_EXECUTION_RETURNED
                           ? BattleChildUnwound(&battle)
                           : stopped;
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
