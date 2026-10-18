#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/battle.h"
#include "system/wram.h"

enum {
    ENEMY_RESOURCE_TABLE = 0x9685c0u,
    ENEMY_EXTENSION_OFFSET = 33u,
    ITEM_DROP_FIELD = 3u,
    ENEMY_ID = WRAM_PARTY_STAT_MEMBER_INDEX,
    DROP_CHANCE = WRAM_PARTY_STAT_MEMBER_INDEX + 1u,
    DROP_RECORD = WRAM_BATTLE_ACTION_BIAS,
    DROPPED_ITEM = WRAM_PARTY_STAT_LEVEL,
    SCRIPT_CONTEXT = WRAM_BATTLE_SCRIPT_CONTEXT,
    SAVED_SCRIPT_STATE = 0x0a62u,
    SCRIPT_ITEM = WRAM_ITEM_RECORD_ID,
    SCRIPT_ITEM_RECORD = WRAM_ITEM_RECORD_POINTER,
    EQUIPMENT_SCRIPT_OFFSET = WRAM_RECORD_BUFFER + 28u,
    EQUIPMENT_SCRIPT_BASE = WRAM_BATTLE_MESSAGE_NAME_POINTER,
    EQUIPMENT_SCRIPT_BANK = WRAM_BATTLE_MESSAGE_NAME_BANK,
    ACTOR_PARTY_SLOT_TABLE = 0x97fe26u,
    PARTY_CHARACTER_IDS = WRAM_BATTLE_PARTY_IDS
};

static uint8_t ActionSupportContext(const Lufia2CpuState *cpu, uint8_t bank) {
    return cpu->program_bank == bank && !cpu->direct_page && !cpu->decimal &&
           cpu->accumulator_is_8_bit && !cpu->index_is_8_bit && cpu->stack >= 0x1f20u &&
           cpu->stack <= 0x1ffcu;
}

static uint8_t ActionSupportCall(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                 Lufia2PushedChildCall child, void *context,
                                 uint32_t site, uint32_t target, uint8_t frame) {
    return CallChildWithFrame(memory, cpu, child, context, site, target, frame,
                              (uint8_t)(site >> 16));
}

static Lufia2ExecutionResult ActionSupportUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2BattleRollEnemyItemDrop(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu,
                                                    Lufia2PushedChildCall child,
                                                    void *context) {
    if (!child || !ActionSupportContext(cpu, 0x81u))
        return ExecutionHandoff(cpu, 0x81fb1au);
    OpRepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpAbs(cpu, DROPPED_ITEM));
    OpLda(memory, cpu, OpAbs(cpu, ENEMY_ID));
    OpAndValue(cpu, 0xffu);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, ENEMY_RESOURCE_TABLE));
    cpu->carry = 0u;
    OpAdcValue(cpu, ENEMY_EXTENSION_OFFSET);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    for (;;) {
        OpLda(memory, cpu, OpLongX(cpu, ENEMY_RESOURCE_TABLE));
        if (cpu->zero)
            return ExecutionReturned(0x81fb78u);
        OpInx(cpu);
        OpCmpValue(cpu, ITEM_DROP_FIELD);
        if (cpu->zero) {
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpLongX(cpu, ENEMY_RESOURCE_TABLE));
            if (!cpu->zero) {
                OpSta(memory, cpu, OpAbs(cpu, DROP_RECORD));
                OpSepWidths(cpu, 0x20u);
                ExchangeAccumulatorBytes(cpu);
                OpLsrA(cpu);
                OpSta(memory, cpu, OpAbs(cpu, DROP_CHANCE));
                OpLoadA(cpu, 100u);
                if (!ActionSupportCall(memory, cpu, child, context, 0x81fb51u,
                                       0x808299u, 3u))
                    return ActionSupportUnwound(0x81fb51u);
                OpCmpValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, DROP_CHANCE)));
                if (!cpu->carry) {
                    OpLoadA(cpu, 4u);
                    if (!ActionSupportCall(memory, cpu, child, context, 0x81fb5cu,
                                           0x808299u, 3u))
                        return ActionSupportUnwound(0x81fb5cu);
                    OpCmpValue(cpu, 1u);
                    if (cpu->zero) {
                        OpRepWidths(cpu, 0x20u);
                        OpLda(memory, cpu, OpAbs(cpu, DROP_RECORD));
                        OpAndValue(cpu, 0x01ffu);
                        OpOraValue(cpu, 0x0200u);
                        OpSta(memory, cpu, OpAbs(cpu, DROPPED_ITEM));
                        OpSepWidths(cpu, 0x20u);
                    }
                }
            } else {
                OpSepWidths(cpu, 0x20u);
            }
        }
        OpInx(cpu);
        OpInx(cpu);
    }
}

Lufia2ExecutionResult Lufia2BattleRunEquipmentItemScript(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu,
                                                         Lufia2PushedChildCall child,
                                                         void *context) {
    if (!child || !ActionSupportContext(cpu, 0x85u))
        return ExecutionHandoff(cpu, 0x85da43u);
    OpWriteX(memory, cpu, OpAbs(cpu, SCRIPT_ITEM), cpu->y);
    OpPushX(memory, cpu);
    if (!ActionSupportCall(memory, cpu, child, context, 0x85da47u, 0x81f1c5u, 3u))
        return ActionSupportUnwound(0x85da47u);
    OpPullX(memory, cpu);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, SCRIPT_ITEM_RECORD)));
    OpWriteX(memory, cpu, OpAbs(cpu, EQUIPMENT_SCRIPT_BASE), cpu->y);
    OpLoadA(cpu, 0x96u);
    OpSta(memory, cpu, OpAbs(cpu, EQUIPMENT_SCRIPT_BANK));
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, EQUIPMENT_SCRIPT_OFFSET)));
    if (!ActionSupportCall(memory, cpu, child, context, 0x85da5au, 0x81fac9u, 3u))
        return ActionSupportUnwound(0x85da5au);
    return ExecutionReturned(0x85da5eu);
}

Lufia2ExecutionResult Lufia2BattleRunEquipmentScripts(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu,
                                                      Lufia2PushedChildCall child,
                                                      void *context) {
    static const struct {
        uint16_t offset;
        uint32_t site;
    } equipment[] = {{0x6eu, 0x85da1du}, {0x70u, 0x85da23u}, {0x68u, 0x85da29u},
                     {0x6au, 0x85da2fu}, {0x6cu, 0x85da35u}, {0x66u, 0x85da3bu}};
    if (!child || !ActionSupportContext(cpu, 0x85u))
        return ExecutionHandoff(cpu, 0x85da11u);
    OpLda(memory, cpu, OpAbs(cpu, SCRIPT_CONTEXT));
    if (cpu->zero)
        return ExecutionReturned(0x85da42u);
    OpLda(memory, cpu, OpAbs(cpu, SAVED_SCRIPT_STATE));
    PushAccumulator8(memory, cpu);
    for (unsigned slot = 0u; slot < 6u; ++slot) {
        OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, equipment[slot].offset)));
        if (!ActionSupportCall(memory, cpu, child, context, equipment[slot].site,
                               0x85da43u, 2u))
            return ActionSupportUnwound(equipment[slot].site);
    }
    if (cpu->accumulator_is_8_bit)
        LoadA8(cpu, Pull8(memory, cpu));
    else
        PullAccumulator16(memory, cpu);
    OpSta(memory, cpu, OpAbs(cpu, SAVED_SCRIPT_STATE));
    return ExecutionReturned(0x85da42u);
}

Lufia2ExecutionResult Lufia2BattleCharacterFromActor(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu) {
    if (!ActionSupportContext(cpu, 0x85u))
        return ExecutionHandoff(cpu, 0x85da5fu);
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    OpSepWidths(cpu, 0x10u);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, ACTOR_PARTY_SLOT_TABLE));
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, PARTY_CHARACTER_IDS));
    OpRepWidths(cpu, 0x10u);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    return ExecutionReturned(0x85da70u);
}
