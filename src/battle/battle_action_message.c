#include "battle/battle_internal.h"
#include "core/snes_registers.h"

enum {
    ACTION_RECORD_INDEX = 0x12u,
    ACTION_MESSAGE_SCROLL = 0x14u,
    ACTION_RECORD_KIND = 0x1bu,
    ACTION_MESSAGE_LENGTH = 0x22u,
    ACTION_STRING_BANK = 0x5fu,
    ACTION_MESSAGE_ID = WRAM_BATTLE_ACTION_MESSAGE_ID,
    ACTION_MESSAGE_KIND = WRAM_BATTLE_ACTION_MESSAGE_ID + 1u,
    ACTION_MESSAGE_BUFFER = WRAM_TEXT_WAIT_ACTOR,
    ACTION_MESSAGE_TILES = WRAM_BATTLE_ACTION_MESSAGE_TILES & 0xffffu,
    ACTION_SCROLL_INCREMENT = WRAM_BATTLE_MESSAGE_SCROLL_INCREMENT
};

static void ActionMessageRecordOffset(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint8_t record_size) {
    OpLda(memory, cpu, OpDp(cpu, ACTION_RECORD_INDEX));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
    OpLoadA(cpu, record_size);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
    OpPushX(memory, cpu);
    OpPullX(memory, cpu);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, SNES_RDMPYL)));
}

static int ActionMessageIpName(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    ActionMessageRecordOffset(memory, cpu, 24u);
    OpLda(memory, cpu, OpLongX(cpu, WRAM_BATTLE_EFFECT_GRAPHICS_BUFFER));
    OpCmpValue(cpu, 0xffu);
    if (cpu->zero)
        return 0;
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, WRAM_BATTLE_EFFECT_GRAPHICS_BUFFER + 1u));
    if (cpu->zero)
        return 0;
    PushAccumulator16(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    if (!BattleCall(battle, 0x9958u, 0x859578u, 3u))
        return -1;
    OpRepWidths(cpu, 0x20u);
    PullAccumulator16(memory, cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x0300u);
    OpSta(memory, cpu, OpAbs(cpu, ACTION_MESSAGE_ID));
    OpSepWidths(cpu, 0x20u);
    return 1;
}

static int ActionMessageName(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    OpLda(memory, cpu, OpDp(cpu, ACTION_RECORD_KIND));
    OpCmpValue(cpu, 2u);
    if (cpu->zero)
        return ActionMessageIpName(battle);
    ActionMessageRecordOffset(memory, cpu, 16u);
    OpLda(memory, cpu, OpLongX(cpu, WRAM_BATTLE_EFFECT_GRAPHICS_BUFFER));
    OpCmpValue(cpu, 0xffu);
    if (cpu->zero)
        return 0;
    OpLda(memory, cpu, OpDp(cpu, ACTION_RECORD_KIND));
    if (cpu->zero) {
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpLongX(cpu, WRAM_BATTLE_EFFECT_GRAPHICS_BUFFER + 1u));
        OpSta(memory, cpu, OpAbs(cpu, ACTION_MESSAGE_ID));
        OpSta(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
        OpSepWidths(cpu, 0x20u);
        if (!BattleCall(battle, 0x9999u, 0x81f1c5u, 3u) ||
            !BattleCall(battle, 0x999du, 0x859510u, 3u))
            return -1;
        return 1;
    }
    OpDecA(cpu);
    if (!cpu->zero)
        return ActionMessageIpName(battle);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, WRAM_BATTLE_EFFECT_GRAPHICS_BUFFER + 1u));
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, 0x96u));
    OpSta(memory, cpu, OpAbs(cpu, ACTION_MESSAGE_ID));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_RECORD_ID));
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpAbs(cpu, ACTION_MESSAGE_KIND));
    if (!BattleCall(battle, 0x9981u, 0x81f414u, 3u) ||
        !BattleCall(battle, 0x9985u, 0x859510u, 3u))
        return -1;
    return 1;
}

static uint8_t ActionMessageClearTiles(BattleContext *battle) {
    const uint16_t styles[] = {0x2202u, 0x2203u, 0x223cu, 0x223du, 0x2274u, 0x2275u};
    const uint16_t sites[] = {0x99b3u, 0x99b9u, 0x99bfu, 0x99c5u, 0x99cbu, 0x99d1u, 0x99d7u};
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0u);
    for (unsigned row = 0; row < 7u; ++row) {
        if (row)
            OpLoadA(cpu, styles[row - 1u]);
        if (!BattleCall(battle, sites[row], 0x859a71u, 2u))
            return 0u;
    }
    PullDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    return 1u;
}

static uint8_t ActionMessageDrawWindow(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    if (!BattleCall(battle, 0x99e2u, 0x859a7du, 3u))
        return 0u;
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpDp(cpu, ACTION_MESSAGE_LENGTH));
    OpIncA(cpu);
    OpAndValue(cpu, 0xfeu);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, (uint16_t)(cpu->accumulator ^ 0xffffu));
    OpIncA(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, ACTION_MESSAGE_TILES + 0x60u);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpDp(cpu, ACTION_STRING_BANK));
    OpLdy(cpu, ACTION_MESSAGE_BUFFER);
    if (!BattleCall(battle, 0x9a00u, 0x808878u, 3u))
        return 0u;
    OpLoadA(cpu, 0x7fu);
    OpSta(memory, cpu, OpDp(cpu, ACTION_STRING_BANK));
    OpLdx(cpu, ACTION_MESSAGE_TILES + 0xc6u);
    OpLdy(cpu, 0xf000u);
    if (!BattleCall(battle, 0x9a0eu, 0x808878u, 3u))
        return 0u;
    OpLoadA(cpu, 7u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_MESSAGE_WINDOW_ROWS));
    OpLoadA(cpu, 26u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_MESSAGE_WINDOW_COLUMNS));
    OpLdx(cpu, 0x2904u);
    if (!BattleCall(battle, 0x9a1fu, 0x81e3cdu, 3u))
        return 0u;
    OpStz(memory, cpu, OpAbs(cpu, WRAM_BATTLE_CURSOR_ENABLED));
    OpRepWidths(cpu, 0x20u);
    if (!BattleCall(battle, 0x9a28u, 0x859d61u, 3u) ||
        !BattleCall(battle, 0x9a2cu, 0x859d78u, 3u) ||
        !BattleCall(battle, 0x9a30u, 0x859cd7u, 3u))
        return 0u;
    OpSepWidths(cpu, 0x20u);
    return 1u;
}

static uint8_t ActionMessageWait(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpDp(cpu, ACTION_MESSAGE_SCROLL));
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x859e79u));
    OpSta(memory, cpu, OpAbs(cpu, ACTION_SCROLL_INCREMENT));
    for (;;) {
        if (!BattleCall(battle, 0x9a41u, 0x85894au, 3u) ||
            !BattleCall(battle, 0x9a45u, 0x859aaau, 3u) ||
            !BattleCall(battle, 0x9a49u, 0x858a2fu, 3u) ||
            !BattleCall(battle, 0x9a4du, 0x859abcu, 3u))
            return 0u;
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, BATTLE_SPRITE_REBUILD_REQUEST);
        if (!BattleCall(battle, 0x9a57u, 0x85ec81u, 3u))
            return 0u;
        OpLda(memory, cpu, OpDp(cpu, BATTLE_DP_PAD_FILTERED_HIGH));
        if (cpu->negative)
            break;
        OpLda(memory, cpu, OpDp(cpu, BATTLE_DP_PAD_FILTERED));
        OpBitValue(cpu, 0x40u);
        if (!cpu->zero)
            break;
    }
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_CURSOR_ENABLED));
    OpStz(memory, cpu, OpAbs(cpu, ACTION_SCROLL_INCREMENT));
    return 1u;
}

Lufia2ExecutionResult Lufia2BattleShowActionMessage(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page)
        return ExecutionHandoff(cpu, 0x859906u);
    BattleContext battle = BattleContextCreate(memory, cpu, child, context, 0x85u);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    const int name = ActionMessageName(&battle);
    if (name < 0)
        return BattleChildUnwound(&battle);
    if (name) {
        if (!BattleCall(&battle, 0x99a1u, 0x85c4f1u, 3u) ||
            !BattleCall(&battle, 0x99a5u, 0x81e7a5u, 3u) ||
            !ActionMessageClearTiles(&battle) ||
            !ActionMessageDrawWindow(&battle) || !ActionMessageWait(&battle))
            return BattleChildUnwound(&battle);
    }
    OpSepWidths(cpu, 0x20u);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x859a70u);
}

Lufia2ExecutionResult Lufia2BattleClearMessageRow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x85u || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859a71u);
    OpLdy(cpu, 32u);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, ACTION_MESSAGE_TILES));
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    return ExecutionReturned(0x859a7cu);
}

Lufia2ExecutionResult Lufia2BattleCopyRecordName(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859510u);
    PushAccumulator8(memory, cpu);
    OpPushX(memory, cpu);
    OpLdx(cpu, 12u);
    uint8_t all_blank = 0u;
    for (;;) {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_RECORD_BUFFER));
        if (!cpu->zero) {
            OpCmpValue(cpu, 0x10u);
            if (!cpu->zero)
                break;
        }
        OpStz(memory, cpu, OpAbsX(cpu, ACTION_MESSAGE_BUFFER));
        OpDex(cpu);
        if (cpu->negative) {
            all_blank = 1u;
            break;
        }
    }
    if (!all_blank) {
        do {
            OpSta(memory, cpu, OpAbsX(cpu, ACTION_MESSAGE_BUFFER));
            OpDex(cpu);
            if (cpu->negative)
                break;
            OpLda(memory, cpu, OpAbsX(cpu, WRAM_RECORD_BUFFER));
        } while (1);
    }
    OpPullX(memory, cpu);
    LoadA8(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x859531u);
}

Lufia2ExecutionResult Lufia2BattleLoadIpActionName(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u)
        return ExecutionHandoff(cpu, 0x859578u);
    BattleContext battle = BattleContextCreate(memory, cpu, child, context, 0x85u);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    PushAccumulator16(memory, cpu);
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    OpAndValue(cpu, 0xffu);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpSetDataBank(memory, cpu, 0x84u);
    OpRepWidths(cpu, 0x20u);
    OpTxa(cpu);
    if (!BattleCall(&battle, 0x958cu, 0x81f45eu, 3u))
        return BattleChildUnwound(&battle);
    cpu->carry = 0u;
    OpAdcValue(cpu, 6u);
    OpTay(cpu);
    OpLda(memory, cpu, OpAbsY(cpu, 0u));
    OpAndValue(cpu, 0xffu);
    if (!cpu->zero) {
        OpLdx(cpu, 0u);
        do {
            OpLda(memory, cpu, OpAbsY(cpu, 0u));
            OpSta(memory, cpu, OpAbsX(cpu, ACTION_MESSAGE_BUFFER));
            OpIny(cpu);
            OpIny(cpu);
            OpInx(cpu);
            OpInx(cpu);
            OpCpx(cpu, 0x24u);
        } while (!cpu->zero);
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbsY(cpu, 0u));
        OpSta(memory, cpu, OpAbsX(cpu, ACTION_MESSAGE_BUFFER));
        OpLoadA(cpu, 1u);
    }
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_MESSAGE_COUNT));
    OpRepWidths(cpu, 0x30u);
    cpu->y = PullIndexValue(memory, cpu);
    SetNz16(cpu, cpu->y);
    OpPullX(memory, cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x8595c5u);
}

Lufia2ExecutionResult Lufia2BattleLoadStatusMessage(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x85u)
        return ExecutionHandoff(cpu, 0x859532u);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    PushAccumulator16(memory, cpu);
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpSetDataBank(memory, cpu, 0x85u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, WRAM_BATTLE_ACTION_WORK + 0x0au);
    OpSta(memory, cpu, OpAbs(cpu, ACTION_MESSAGE_BUFFER));
    if (!cpu->zero) {
        OpDecA(cpu);
        OpAslA(cpu);
        OpTax(cpu);
        OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, 0xf2d7u)));
        OpLdx(cpu, 0u);
        OpLda(memory, cpu, OpAbsY(cpu, 0u));
        OpSta(memory, cpu, OpAbs(cpu, WRAM_PALETTE_FADE));
        do {
            OpLda(memory, cpu, OpAbsY(cpu, 1u));
            OpSta(memory, cpu, OpAbsX(cpu, ACTION_MESSAGE_BUFFER));
            OpIny(cpu);
            OpIny(cpu);
            OpInx(cpu);
            OpInx(cpu);
            OpCpx(cpu, 0x24u);
        } while (!cpu->zero);
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbsY(cpu, 1u));
        OpSta(memory, cpu, OpAbsX(cpu, ACTION_MESSAGE_BUFFER));
        OpRepWidths(cpu, 0x30u);
    }
    cpu->y = PullIndexValue(memory, cpu);
    SetNz16(cpu, cpu->y);
    OpPullX(memory, cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x859577u);
}
