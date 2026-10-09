#include "battle/battle_internal.h"
#include "lufia2/battle.h"

enum {
    ROM_PARTY_STATUS_TILE_CURSORS = 0x818811u,
    ROM_PARTY_EFFECT_SLOTS = 0x859eecu,
    ROM_STATUS_ICON_TILES = 0x859e7fu,
    EFFECT_STATUS_ICON = 0x7f0006u,
    BATTLE_UNK_1BE0 = 0x1be0u,
    PARTY_LEVEL = 0x0eu,
    PARTY_CURRENT_HP = 0x11u,
    PARTY_CURRENT_MP = 0x13u,
    PARTY_MAXIMUM_HP = 0x25u,
    PARTY_MAXIMUM_MP = 0x27u,
    PARTY_CURRENT_IP = 0xbcu,
    DP_GAUGE_VALUE = 0x11u,
    DP_GAUGE_MAXIMUM = 0x13u,
    DP_GAUGE_TILE_BASE = 0x15u,
    DP_GAUGE_LABEL = 0x16u,
    DP_SYMBOL_COUNT = 0x1bu,
    DP_DIGIT_ONES = 0xb2u,
    DP_DIGIT_TENS = 0xb3u,
    STATUS_ROW_BYTES = 0x40u,
    STATUS_SYMBOLS = 5u,
    STATUS_TILE_ATTRIBUTE = 0x21u,
    STATUS_DIGIT_TILE_BASE = 0x40u
};

static void DrawStatusSymbolRow(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpStack(cpu, 5u));
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, BATTLE_UNK_1BE0));
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, STATUS_SYMBOLS);
    OpSta(memory, cpu, OpDp(cpu, DP_SYMBOL_COUNT));
    do {
        OpTxa(cpu);
        OpSta(memory, cpu, OpAbsY(cpu, 0u));
        OpLoadA(cpu, STATUS_TILE_ATTRIBUTE);
        OpSta(memory, cpu, OpAbsY(cpu, 1u));
        OpInx(cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, DP_SYMBOL_COUNT), -1);
    } while (!cpu->zero);
}

static bool DrawPartyLevel(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    OpLda(memory, cpu, OpAbsX(cpu, PARTY_LEVEL));
    OpPushX(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    if (!BattleCall(battle, 0xe1f8u, 0x81e808u, 2u)) return false;
    OpLoadA(cpu, 0x60u);
    OpSta(memory, cpu, OpAbsY(cpu, 0x0au));
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, 0x0cu));
    OpLda(memory, cpu, OpDp(cpu, DP_DIGIT_TENS));
    if (!cpu->zero) {
        cpu->carry = 0u;
        OpAdcValue(cpu, STATUS_DIGIT_TILE_BASE);
        OpSta(memory, cpu, OpAbsY(cpu, 0x0eu));
    }
    OpLda(memory, cpu, OpDp(cpu, DP_DIGIT_ONES));
    cpu->carry = 0u;
    OpAdcValue(cpu, STATUS_DIGIT_TILE_BASE);
    OpSta(memory, cpu, OpAbsY(cpu, 0x10u));
    OpPullX(memory, cpu);
    return true;
}

static void AdvancePartyStatusRow(Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpTya(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, STATUS_ROW_BYTES);
    OpTay(cpu);
}

static bool DrawPartyResource(BattleContext *battle, uint16_t current,
    uint16_t maximum, uint8_t label, uint8_t tile,
    uint16_t digits_site, uint16_t gauge_site) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    AdvancePartyStatusRow(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, current));
    OpSta(memory, cpu, OpDp(cpu, DP_GAUGE_VALUE));
    OpLda(memory, cpu, OpAbsX(cpu, maximum));
    OpSta(memory, cpu, OpDp(cpu, DP_GAUGE_MAXIMUM));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, label);
    OpSta(memory, cpu, OpDp(cpu, DP_GAUGE_LABEL));
    OpLoadA(cpu, tile);
    OpSta(memory, cpu, OpDp(cpu, DP_GAUGE_TILE_BASE));
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    if (!BattleCall(battle, digits_site, 0x81e2c8u, 2u) ||
        !BattleCall(battle, gauge_site, 0x81e308u, 2u)) return false;
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    return true;
}

static bool DrawPartyIp(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    AdvancePartyStatusRow(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x62u);
    OpSta(memory, cpu, OpAbsY(cpu, 0u));
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, 2u));
    OpLoadA(cpu, 0x4eu);
    OpSta(memory, cpu, OpAbsY(cpu, 6u));
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, 8u));
    OpLda(memory, cpu, OpAbsX(cpu, PARTY_CURRENT_IP));
    OpSta(memory, cpu, OpDp(cpu, DP_GAUGE_VALUE));
    OpStz(memory, cpu, OpDp(cpu, DP_GAUGE_VALUE + 1u));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, DP_GAUGE_MAXIMUM));
    OpStz(memory, cpu, OpDp(cpu, DP_GAUGE_MAXIMUM + 1u));
    OpLoadA(cpu, 0x2bu);
    OpSta(memory, cpu, OpDp(cpu, DP_GAUGE_TILE_BASE));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpStack(cpu, 1u));
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, ROM_PARTY_EFFECT_SLOTS));
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpLongX(cpu, EFFECT_STATUS_ICON));
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, ROM_STATUS_ICON_TILES));
    OpSta(memory, cpu, OpAbsY(cpu, 4u));
    return BattleCall(battle, 0xe2a9u, 0x81e308u, 2u);
}

Lufia2ExecutionResult Lufia2BattleDrawPartyStatusDetail(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x81e1b5u);
    BattleContext battle = BattleContextCreate(memory, cpu, child, context, 0x81u);
    PushDataBank(memory, cpu);
    OpPushX(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, ROM_PARTY_STATUS_TILE_CURSORS));
    PushAccumulator16(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_PARTY_RECORDS)));
    OpTyx(cpu);
    OpPullY(memory, cpu);
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    OpLoadA(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    DrawStatusSymbolRow(&battle);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    if (!DrawPartyLevel(&battle) ||
        !DrawPartyResource(&battle, PARTY_CURRENT_HP, PARTY_MAXIMUM_HP,
            0x4au, 1u, 0xe235u, 0xe238u) ||
        !DrawPartyResource(&battle, PARTY_CURRENT_MP, PARTY_MAXIMUM_MP,
            0x4cu, 0x16u, 0xe25bu, 0xe25eu) || !DrawPartyIp(&battle))
        return BattleChildUnwound(&battle);
    OpPullX(memory, cpu);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81e2aeu);
}
