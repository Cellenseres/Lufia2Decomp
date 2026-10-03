#include "battle/battle_internal.h"
#include "core/snes_registers.h"

enum {
    COMMAND_DP_MAX_PRIORITY_OR_RANDOM = 0x22u,
    COMMAND_DP_MIN_PRIORITY = 0x24u,
    COMMAND_DP_PALETTE_BANK = 0x24u,
};

/* The two drawing loops use different tables and exact child callsites. */
static bool BattleDrawCommands(BattleContext *battle, bool after_selection) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    const uint32_t command_layout = after_selection ? 0x97b567u : 0x97b55eu;
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpLongX(cpu, command_layout));
        OpSta(memory, cpu, OpDp(cpu, 0u));
        OpLda(memory, cpu, OpLongX(cpu, command_layout + 1u));
        OpSta(memory, cpu, OpDp(cpu, 8u));
        OpLda(memory, cpu, OpLongX(cpu, command_layout + 2u));
        OpSta(memory, cpu, OpDp(cpu, 9u));
        OpInx(cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpPushX(memory, cpu);
        if (!BattleCall(battle, after_selection ? 0xcb0fu : 0xc9ddu, 0x81be58u, 2u))
            return false;
        OpPullX(memory, cpu);
        OpCpx(cpu, after_selection ? 15u : 9u);
    } while (!cpu->zero);
    return true;
}

static void BattleCollectPartyPriorityBounds(const Lufia2Memory *memory,
                                             Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpDp(cpu, COMMAND_DP_MAX_PRIORITY_OR_RANDOM));
    OpLoadA(cpu, 0xffffu);
    OpSta(memory, cpu, OpDp(cpu, COMMAND_DP_MIN_PRIORITY));
    OpLdy(cpu, 6u);
    do {
        OpLdx(cpu, OpReadX(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_PARTY_RECORDS)));
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbsX(cpu, BATTLE_BATTLER_STATUS));
            OpBitValue(cpu, BATTLE_STATUS_NO_COMMAND_MASK);
            if (cpu->zero) {
                OpLda(memory, cpu, OpAbsX(cpu, BATTLE_BATTLER_BASE_PRIORITY));
                cpu->carry = false;
                OpAdc(memory, cpu, OpAbsX(cpu, BATTLE_BATTLER_PRIORITY_BONUS));
                OpCmp(memory, cpu, OpDp(cpu, COMMAND_DP_MAX_PRIORITY_OR_RANDOM));
                if (cpu->carry)
                    OpSta(memory, cpu, OpDp(cpu, COMMAND_DP_MAX_PRIORITY_OR_RANDOM));
                OpCmp(memory, cpu, OpDp(cpu, COMMAND_DP_MIN_PRIORITY));
                if (!cpu->carry)
                    OpSta(memory, cpu, OpDp(cpu, COMMAND_DP_MIN_PRIORITY));
            }
        }
        OpDey(cpu);
        OpDey(cpu);
    } while (!cpu->negative);
}

/* $C826: priority bounds and two random fractions for the collective action. */
static bool BattleQueueCollectiveCommand(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    BattleCollectPartyPriorityBounds(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, COMMAND_DP_MAX_PRIORITY_OR_RANDOM));
    cpu->carry = true;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, COMMAND_DP_MIN_PRIORITY)));
    OpLsrA(cpu);
    PushAccumulator16(memory, cpu);
    if (!BattleCall(battle, 0xc85du, 0x85dceau, 3u))
        return false;
    OpSta(memory, cpu, OpDp(cpu, COMMAND_DP_MAX_PRIORITY_OR_RANDOM));
    PullAccumulator16(memory, cpu);
    if (!BattleCall(battle, 0xc864u, 0x85dceau, 3u))
        return false;
    cpu->carry = false;
    OpAdc(memory, cpu, OpDp(cpu, COMMAND_DP_MAX_PRIORITY_OR_RANDOM));
    /* Second ADC keeps the previous carry. */
    OpAdc(memory, cpu, OpDp(cpu, COMMAND_DP_MIN_PRIORITY));
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_ACTION_PRIORITY));
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_STAGED_ACTION));
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_ACTION_TARGET_MASK));
    OpLoadA(cpu, 6u);
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_ACTION_TYPE));
    if (!BattleCall(battle, 0xc87fu, 0x8592ceu, 3u) ||
        !BattleCall(battle, 0xc883u, 0x8592ffu, 3u))
        return false;
    OpSepWidths(cpu, 0x20u);
    return true;
}

/* Formation copies use multiplier reads and keep the original swap order. */
static void BattleSwapFormationBlocks(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const uint16_t bases[] = {0x1499u, 0x14e6u};
    const uint8_t lengths[] = {7u, 6u};
    for (unsigned block = 0; block < 2u; ++block) {
        OpLoadA(cpu, lengths[block]);
        OpSta(memory, cpu, OpAbs(cpu, 0x09f2u));
        OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
        OpLda(memory, cpu, OpAbs(cpu, 0x09f4u));
        OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
        OpLda(memory, cpu, OpAbs(cpu, 0x09f5u));
        OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, SNES_RDMPYL)));
        OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
        PushAccumulator8(memory, cpu);
        LoadA8(cpu, Pull8(memory, cpu));
        OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, SNES_RDMPYL)));
        do {
            OpLda(memory, cpu, OpAbsX(cpu, bases[block]));
            ExchangeAccumulatorBytes(cpu);
            OpLda(memory, cpu, OpAbsY(cpu, bases[block]));
            OpSta(memory, cpu, OpAbsX(cpu, bases[block]));
            ExchangeAccumulatorBytes(cpu);
            OpSta(memory, cpu, OpAbsY(cpu, bases[block]));
            OpInx(cpu);
            OpIny(cpu);
            OpStepMem(memory, cpu, OpAbs(cpu, 0x09f2u), -1);
        } while (!cpu->zero);
    }
    OpLoadA(cpu, 13u);
    OpSta(memory, cpu, OpAbs(cpu, 0x09f2u));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
    OpLda(memory, cpu, OpAbs(cpu, 0x09f4u));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpAbs(cpu, SNES_RDMPYL));
    OpTax(cpu);
    OpLda(memory, cpu, OpAbs(cpu, 0x09f5u));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
    OpLda(memory, cpu, OpAbs(cpu, SNES_RDMPYL));
    OpTay(cpu);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, 0x1434u));
        PushAccumulator8(memory, cpu);
        OpLda(memory, cpu, OpAbsY(cpu, 0x1434u));
        OpSta(memory, cpu, OpAbsX(cpu, 0x1434u));
        LoadA8(cpu, Pull8(memory, cpu));
        OpSta(memory, cpu, OpAbsY(cpu, 0x1434u));
        OpInx(cpu);
        OpIny(cpu);
        OpStepMem(memory, cpu, OpAbs(cpu, 0x09f2u), -1);
    } while (!cpu->zero);
}

static bool BattleRedrawFormation(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    if (!BattleCall(battle, 0xc9bbu, 0x81e872u, 2u) ||
        !BattleCall(battle, 0xc9beu, 0x81dea9u, 2u) ||
        !BattleCall(battle, 0xc9c1u, 0x81df0au, 2u) ||
        !BattleDrawCommands(battle, false))
        return false;
    OpSepWidths(cpu, 0x10u);
    OpLda(memory, cpu, OpDp(cpu, 0x47u));
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpAndValue(cpu, 3u);
    OpTay(cpu);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbsY(cpu, 0xb576u)));
    OpRepWidths(cpu, 0x10u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x26u), cpu->x);
    OpLda(memory, cpu, OpLongX(cpu, 0x97b55eu));
    cpu->carry = true;
    OpSbcValue(cpu, 8u);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLda(memory, cpu, OpLongX(cpu, 0x97b55fu));
    OpSta(memory, cpu, OpDp(cpu, 8u));
    OpLda(memory, cpu, OpLongX(cpu, 0x97b560u));
    OpSta(memory, cpu, OpDp(cpu, 9u));
    if (!BattleCall(battle, 0xca0bu, 0x81be58u, 2u))
        return false;
    OpRepWidths(cpu, 0x20u);
    if (!BattleCall(battle, 0xca10u, 0x859b22u, 3u) ||
        !BattleCall(battle, 0xca14u, 0x859b67u, 3u))
        return false;
    OpSepWidths(cpu, 0x20u);
    if (!BattleCall(battle, 0xca1au, 0x85ec81u, 3u))
        return false;
    TransferDirectToA(cpu);
    OpTax(cpu);
    OpTxy(cpu);
    do {
        OpLda(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_PARTY_IDS));
        PushY(memory, cpu);
        OpTay(cpu);
        OpLda(memory, cpu, OpAbsY(cpu, 0xb411u));
        OpSta(memory, cpu, OpLongX(cpu, 0x00139cu));
        OpRepWidths(cpu, 0x20u);
        OpTxa(cpu);
        cpu->carry = false;
        OpAdcValue(cpu, 13u);
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
        OpPullY(memory, cpu);
        OpIny(cpu);
        OpCpy(cpu, 4u);
    } while (!cpu->zero);
    if (!BattleCall(battle, 0xca3eu, 0x8591a1u, 3u) ||
        !BattleCall(battle, 0xca42u, 0x858a2fu, 3u))
        return false;
    TransferDirectToA(cpu);
    do {
        PushAccumulator8(memory, cpu);
        if (!BattleCall(battle, 0xca48u, 0x81bbe0u, 2u))
            return false;
        LoadA8(cpu, Pull8(memory, cpu));
        OpIncA(cpu);
        OpCmpValue(cpu, 4u);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, 0x0012f3u);
    OpRepWidths(cpu, 0x20u);
    if (!BattleCall(battle, 0xca5bu, 0x859bdau, 3u))
        return false;
    OpSepWidths(cpu, 0x20u);
    return BattleCall(battle, 0xca61u, 0x85ec81u, 3u);
}

/* $81:C739: complete caller body; exceptional exits remain exact handoffs. */
/* What the command loop does after a step. */
typedef enum {
    COMMAND_SELECT,
    COMMAND_RESTART,
    COMMAND_FINISH,
    COMMAND_UNWOUND,
    COMMAND_HANDOFF
} CommandStep;

/* Clears the staging area, draws the battle screen and the command window, and
 * starts the menu. */
static bool BattleCommandsSetup(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;

    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, 0x125fu));
    OpLdx(cpu, 0xbfu);
    TransferDirectToA(cpu);
    do {
        OpSta(memory, cpu, OpLongX(cpu, WRAM_SAVE_FILE_BUFFER));
        OpDex(cpu);
    } while (!cpu->negative);
    if (!BattleCall(battle, 0xc749u, 0x81c2e3u, 2u) ||
        !BattleCall(battle, 0xc74cu, 0x8597d1u, 3u) ||
        !BattleCall(battle, 0xc750u, 0x81b9afu, 3u) ||
        !BattleCall(battle, 0xc754u, 0x8591a1u, 3u) ||
        !BattleCall(battle, 0xc758u, 0x858905u, 3u) ||
        !BattleCall(battle, 0xc75cu, 0x85ec81u, 3u))
        return false;
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, 0x0012f3u);
    OpRepWidths(cpu, 0x20u);
    if (!BattleCall(battle, 0xc768u, 0x859b67u, 3u) ||
        !BattleCall(battle, 0xc76cu, 0x859c7bu, 3u))
        return false;
    OpSepWidths(cpu, 0x20u);
    if (!BattleCall(battle, 0xc772u, 0x85ec81u, 3u))
        return false;
    OpLoadA(cpu, 10u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_BG3SC));
    OpLoadA(cpu, 0x97u);
    OpSta(memory, cpu, OpDp(cpu, COMMAND_DP_PALETTE_BANK));
    OpLdy(cpu, 0xfe46u);
    OpLoadA(cpu, 1u);
    if (!BattleCall(battle, 0xc784u, 0x81b974u, 2u) ||
        !BattleCall(battle, 0xc787u, 0x81b9afu, 3u))
        return false;
    OpLdx(cpu, 0x151fu);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x1245u), cpu->x);
    OpLdx(cpu, 0x4202u);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x1249u), cpu->x);
    OpLoadA(cpu, 0x90u);
    OpTestBits(memory, cpu, OpDp(cpu, 0xd9u), 0u);
    OpTestBits(memory, cpu, OpDp(cpu, 0xdbu), 0u);
    OpSta(memory, cpu, OpAbs(cpu, 0x15b3u));
    if (!BattleCall(battle, 0xc7a0u, 0x85a804u, 3u) ||
        !BattleCall(battle, 0xc7a4u, 0x81e872u, 2u))
        return false;
    OpRepWidths(cpu, 0x20u);
    if (!BattleCall(battle, 0xc7a9u, 0x859b0bu, 3u))
        return false;
    OpSepWidths(cpu, 0x20u);
    if (!BattleCall(battle, 0xc7afu, 0x85ec81u, 3u) ||
        !BattleCall(battle, 0xc7b3u, 0x81eb93u, 3u) ||
        !BattleCall(battle, 0xc7b7u, 0x858a2fu, 3u) ||
        !BattleCall(battle, 0xc7bbu, 0x85ec81u, 3u) ||
        !BattleCall(battle, 0xc7bfu, 0x81dea9u, 2u))
        return false;
    OpRepWidths(cpu, 0x20u);
    if (!BattleCall(battle, 0xc7c4u, 0x859b67u, 3u) ||
        !BattleCall(battle, 0xc7c8u, 0x859c08u, 3u))
        return false;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, 0x0012f3u);
    if (!BattleCall(battle, 0xc7d4u, 0x85ec81u, 3u))
        return false;
    OpLdx(cpu, 0xdf00u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x60u), cpu->x);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, 0x62u));
    OpLdx(cpu, 0x18fu);
    OpWriteX(memory, cpu, OpDp(cpu, 0x54u), cpu->x);
    if (!BattleCall(battle, 0xc7e6u, 0x808e9du, 3u))
        return false;
    OpRepWidths(cpu, 0x20u);
    if (!BattleCall(battle, 0xc7ecu, 0x859c92u, 3u))
        return false;
    OpSepWidths(cpu, 0x20u);
    return true;
}

/* Marks the seven battle slots empty and sets the field battle result to
 * zero, for the menu entries that leave the battle. */
static void BattleLeave(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;

    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, 0x0c69u));
    OpSta(memory, cpu, OpAbs(cpu, 0x0d27u));
    OpSta(memory, cpu, OpAbs(cpu, 0x0de5u));
    OpSta(memory, cpu, OpAbs(cpu, 0x0ea3u));
    OpSta(memory, cpu, OpAbs(cpu, 0x0f61u));
    OpSta(memory, cpu, OpAbs(cpu, 0x101fu));
    OpSta(memory, cpu, OpAbs(cpu, 0x10ddu));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, WRAM_FIELD_BATTLE_RESULT);
}

/* Swaps two party members' places: the menu asks for the other member, then
 * their ids, records and status icons trade slots and the formation is drawn
 * again. */
static CommandStep BattleSwapPartyOrder(BattleContext *battle, uint32_t *handoff) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;

    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_COUNT));
    OpCmpValue(cpu, 1u);
    if (cpu->zero)
        return COMMAND_SELECT;
    OpLoadA(cpu, 4u);
    OpSta(memory, cpu, OpAbs(cpu, 0x129eu));
    TransferDirectToA(cpu);
    OpLoadA(cpu, 0x23u);
    if (!BattleCall(battle, 0xc89eu, 0x81d4e0u, 2u))
        return COMMAND_UNWOUND;
    OpCmpValue(cpu, 0xffu);
    if (cpu->zero)
        return COMMAND_SELECT;
    OpSta(memory, cpu, OpAbs(cpu, 0x09f2u));
    OpLdy(cpu, 0u);
    OpLdx(cpu, 4u);
    OpLoadA(cpu, 0u);
    do {
        for (;;) {
            const uint32_t address = OpAbs(cpu, 0x09f2u);
            const uint8_t value = Read8(memory, address);
            cpu->carry = (value & 1u) != 0;
            Write8(memory, address, (uint8_t)(value >> 1));
            SetNz8(cpu, (uint8_t)(value >> 1));
            if (cpu->carry)
                break;
            OpIncA(cpu);
            OpDex(cpu);
            if (cpu->zero) {
                *handoff = 0x81c8bcu;
                return COMMAND_HANDOFF;
            }
        }
        OpSta(memory, cpu, OpAbsY(cpu, 0x09f4u));
        OpIncA(cpu);
        OpIny(cpu);
        OpCpy(cpu, 2u);
    } while (!cpu->zero);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpAbs(cpu, 0x09f4u));
    OpTax(cpu);
    OpLda(memory, cpu, OpAbs(cpu, 0x09f5u));
    OpTay(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_PARTY_IDS));
    PushAccumulator8(memory, cpu);
    OpLda(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_PARTY_IDS));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_PARTY_IDS));
    LoadA8(cpu, Pull8(memory, cpu));
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_PARTY_IDS));
    OpLda(memory, cpu, OpAbs(cpu, 0x09f4u));
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbs(cpu, 0x09f5u));
    OpAslA(cpu);
    OpTay(cpu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_PARTY_RECORDS));
    PushAccumulator16(memory, cpu);
    OpLda(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_PARTY_RECORDS));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_PARTY_RECORDS));
    PullAccumulator16(memory, cpu);
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_PARTY_RECORDS));
    OpTxa(cpu);
    OpAslA(cpu);
    OpTax(cpu);
    OpTya(cpu);
    OpAslA(cpu);
    OpTay(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_STATUS_ICON_RECORDS));
    PushAccumulator16(memory, cpu);
    OpLda(memory, cpu, OpAbsX(cpu, BATTLE_ICON_RECORD_TIMER));
    PushAccumulator16(memory, cpu);
    OpLda(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_STATUS_ICON_RECORDS));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_STATUS_ICON_RECORDS));
    OpLda(memory, cpu, OpAbsY(cpu, BATTLE_ICON_RECORD_TIMER));
    OpSta(memory, cpu, OpAbsX(cpu, BATTLE_ICON_RECORD_TIMER));
    PullAccumulator16(memory, cpu);
    OpSta(memory, cpu, OpAbsY(cpu, BATTLE_ICON_RECORD_TIMER));
    PullAccumulator16(memory, cpu);
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_STATUS_ICON_RECORDS));
    OpSepWidths(cpu, 0x20u);
    BattleSwapFormationBlocks(memory, cpu);
    if (!BattleRedrawFormation(battle))
        return COMMAND_UNWOUND;
    return COMMAND_SELECT;
    return COMMAND_SELECT;
}

/* Asks every party member in turn for a command. Backing out of a member's
 * menu returns to the previous member that can still act; backing out of the
 * first returns to the command menu. */
static CommandStep BattlePartyCommands(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;

    OpLdx(cpu, 0u);
    for (;;) {
        bool back;

        OpPushX(memory, cpu);
        OpRepWidths(cpu, 0x20u);
        OpTxa(cpu);
        OpSta(memory, cpu, WRAM_BATTLE_PARTY_SLOT);
        OpAslA(cpu);
        OpTay(cpu);
        OpLda(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_PARTY_RECORDS));
        OpSta(memory, cpu, OpDp(cpu, 0xd5u));
        OpTay(cpu);
        OpLda(memory, cpu, OpAbsY(cpu, BATTLE_BATTLER_STATUS));
        OpBitValue(cpu, BATTLE_STATUS_NO_COMMAND_MASK);
        OpSepWidths(cpu, 0x20u);
        if (!cpu->zero) {
            /* This member cannot act. */
            OpPullX(memory, cpu);
            back = false;
        } else {
            TransferDirectToA(cpu);
            OpLda(memory, cpu, WRAM_BATTLE_PARTY_SLOT);
            OpTax(cpu);
            OpLda(memory, cpu, OpLongX(cpu, 0x96ffecu));
            OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_STAGED_ACTION));
            OpSta(memory, cpu, OpDp(cpu, 0x54u));
            OpPushX(memory, cpu);
            if (!BattleCall(battle, 0xcab8u, 0x85937du, 3u))
                return COMMAND_UNWOUND;
            OpPullX(memory, cpu);
            if (!BattleCall(battle, 0xcabdu, 0x81cc2eu, 2u))
                return COMMAND_UNWOUND;
            OpPullX(memory, cpu);
            OpCmpValue(cpu, 0u);
            back = !cpu->zero;
        }
        if (back) {
            for (;;) {
                OpDex(cpu);
                if (cpu->negative)
                    return COMMAND_RESTART;
                OpRepWidths(cpu, 0x20u);
                OpTxa(cpu);
                OpSta(memory, cpu, WRAM_BATTLE_PARTY_SLOT);
                OpAslA(cpu);
                OpTay(cpu);
                OpLda(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_PARTY_RECORDS));
                OpTay(cpu);
                OpLda(memory, cpu, OpAbsY(cpu, BATTLE_BATTLER_STATUS));
                OpBitValue(cpu, BATTLE_STATUS_NO_COMMAND_MASK);
                OpSepWidths(cpu, 0x20u);
                if (cpu->zero)
                    break;
            }
            if (!BattleCall(battle, 0xcae2u, 0x8596beu, 3u))
                return COMMAND_UNWOUND;
            continue;
        }
        OpInx(cpu);
        OpTxa(cpu);
        OpCmp(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_COUNT));
        if (cpu->zero)
            return COMMAND_FINISH;
    }
}

/* Runs the command menu until it ends the collection or sends it back to the
 * start. The menu answer is 1 for a command for the whole party, 2 for
 * member-by-member commands, 3 for swapping places and 5 to 7 for leaving the
 * battle (when that is allowed). */
static CommandStep BattleChooseCommand(BattleContext *battle, uint32_t *handoff) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;

    for (;;) {
        CommandStep step;

        if (!BattleCall(battle, 0xc7f8u, 0x81cb77u, 2u))
            return COMMAND_UNWOUND;
        OpDecA(cpu);
        if (cpu->zero) {
            if (!BattleQueueCollectiveCommand(battle))
                return COMMAND_UNWOUND;
            return COMMAND_FINISH;
        }
        OpDecA(cpu);
        if (cpu->zero)
            return BattlePartyCommands(battle);
        OpDecA(cpu);
        if (cpu->zero) {
            step = BattleSwapPartyOrder(battle, handoff);
            if (step != COMMAND_SELECT)
                return step;
            continue;
        }
        ExchangeAccumulatorBytes(cpu);
        OpLda(memory, cpu, OpAbs(cpu, 0x057cu));
        if (cpu->zero)
            continue;
        ExchangeAccumulatorBytes(cpu);
        OpDecA(cpu);
        OpDecA(cpu);
        if (!cpu->zero) {
            OpDecA(cpu);
            if (!cpu->zero) {
                OpDecA(cpu);
                if (!cpu->zero)
                    continue;
            }
        }
        BattleLeave(battle);
        *handoff = 0x818855u;
        return COMMAND_HANDOFF;
    }
}

/* Draws the finished commands and the battle screen again. */
static bool BattleCommandsFinish(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;

    if (!BattleCall(battle, 0xcaf3u, 0x81df0au, 2u) ||
        !BattleDrawCommands(battle, true) ||
        !BattleCall(battle, 0xcb18u, 0x81c2e3u, 2u) ||
        !BattleCall(battle, 0xcb1bu, 0x81c2fbu, 2u))
        return false;
    OpStz(memory, cpu, OpAbs(cpu, 0x125fu));
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpAbs(cpu, 0x15abu));
    if (!BattleCall(battle, 0xcb26u, 0x858a39u, 3u))
        return false;
    OpRepWidths(cpu, 0x20u);
    if (!BattleCall(battle, 0xcb2cu, 0x859b67u, 3u) ||
        !BattleCall(battle, 0xcb30u, 0x859c08u, 3u))
        return false;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 2u);
    OpTestBits(memory, cpu, OpDp(cpu, 0xdbu), 0u);
    OpLoadA(cpu, 0xb6u);
    OpTestBits(memory, cpu, OpDp(cpu, 0xd9u), 0u);
    OpStz(memory, cpu, OpAbs(cpu, 0x1b1fu));
    OpStz(memory, cpu, OpAbs(cpu, 0x124au));
    if (!BattleCall(battle, 0xcb44u, 0x85ec81u, 3u) ||
        !BattleCall(battle, 0xcb48u, 0x81e877u, 2u))
        return false;
    OpRepWidths(cpu, 0x20u);
    if (!BattleCall(battle, 0xcb4du, 0x859b0bu, 3u))
        return false;
    OpSepWidths(cpu, 0x20u);
    if (!BattleCall(battle, 0xcb53u, 0x85ec81u, 3u) ||
        !BattleCall(battle, 0xcb57u, 0x81ebf4u, 3u) ||
        !BattleCall(battle, 0xcb5bu, 0x858a2fu, 3u))
        return false;
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpAbs(cpu, 0x15abu));
    if (!BattleCall(battle, 0xcb64u, 0x858a39u, 3u) ||
        !BattleCall(battle, 0xcb68u, 0x8589e5u, 3u))
        return false;
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, 0x0012f3u);
    if (!BattleCall(battle, 0xcb72u, 0x85ec81u, 3u))
        return false;
    return true;
}

Lufia2ExecutionResult Lufia2BattleCollectCommands(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu,
                                                  Lufia2PushedChildCall child,
                                                  void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);
    uint32_t handoff = 0u;
    bool restart = true;

    if (!BattleCommandsSetup(&battle))
        return BattleChildUnwound(&battle);
    for (;;) {
        CommandStep step;

        if (restart) {
            OpLoadA(cpu, 0u);
            if (!BattleCall(&battle, 0xc7f4u, 0x859326u, 3u))
                return BattleChildUnwound(&battle);
            restart = false;
        }
        step = BattleChooseCommand(&battle, &handoff);
        if (step == COMMAND_UNWOUND)
            return BattleChildUnwound(&battle);
        if (step == COMMAND_HANDOFF)
            return ExecutionHandoff(cpu, handoff);
        if (step == COMMAND_FINISH)
            break;
        restart = true;
    }
    if (!BattleCommandsFinish(&battle))
        return BattleChildUnwound(&battle);
    return ExecutionReturned(0x81cb76u);
}
