#include "battle/battle_internal.h"
#include "core/snes_registers.h"
#include "core/wram_view.h"

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

/* A word of the battler record that X points to, in the data bank. */
static uint16_t BattlerWord(const Lufia2Memory *memory, const Lufia2CpuState *cpu,
                            uint16_t field) {
    return Read16Long(memory, OpAbsX(cpu, field));
}

/* Finds the lowest and highest priority among the party members that can still
 * take a command, into the two words at $22 and $24. Leaves the last member
 * record in X and Y below zero, as the loop did. */
static void BattleCollectPartyPriorityBounds(const Lufia2Memory *memory,
                                             Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    unsigned slot;

    OpRepWidths(cpu, 0x20u);
    WramWrite16(wram, COMMAND_DP_MAX_PRIORITY_OR_RANDOM, 0u);
    WramWrite16(wram, COMMAND_DP_MIN_PRIORITY, 0xffffu);
    for (slot = BATTLE_PARTY_SIZE; slot-- > 0u;) {
        const uint16_t record =
            WramRead16At(wram, WRAM_BATTLE_PARTY_RECORDS, (uint16_t)(2u * slot));

        cpu->x = record;
        if (record == 0u)
            continue;
        if (BattlerWord(memory, cpu, BATTLE_BATTLER_STATUS) &
            BATTLE_STATUS_NO_COMMAND_MASK)
            continue;
        {
            const uint16_t priority =
                (uint16_t)(BattlerWord(memory, cpu, BATTLE_BATTLER_BASE_PRIORITY) +
                           BattlerWord(memory, cpu, BATTLE_BATTLER_PRIORITY_BONUS));

            if (priority >= WramRead16(wram, COMMAND_DP_MAX_PRIORITY_OR_RANDOM))
                WramWrite16(wram, COMMAND_DP_MAX_PRIORITY_OR_RANDOM, priority);
            if (priority < WramRead16(wram, COMMAND_DP_MIN_PRIORITY))
                WramWrite16(wram, COMMAND_DP_MIN_PRIORITY, priority);
        }
    }
    cpu->y = 0xfffeu;
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

enum {
    FORMATION_COUNTER = 0x09f2u, /* bytes left to swap */
    FORMATION_PICK_A = 0x09f4u,  /* the two members being swapped */
    FORMATION_PICK_B = 0x09f5u,
    FORMATION_BLOCK_A = 0x1499u, /* 7 bytes per member */
    FORMATION_BLOCK_A_BYTES = 7,
    FORMATION_BLOCK_B = 0x14e6u, /* 6 bytes per member */
    FORMATION_BLOCK_B_BYTES = 6,
    FORMATION_STATE = 0x1434u, /* 13 bytes per member */
    FORMATION_STATE_BYTES = 13,
};

/* Swaps the two picked members' rows of a table. The row offsets come from
 * the hardware multiplier, so its registers are written and read in the
 * original order. */
static void SwapFormationRows(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                              Lufia2Wram wram, uint16_t table, uint8_t row_bytes) {
    uint8_t pick_b;
    uint16_t from;
    uint16_t to;

    LoadA8(cpu, row_bytes);
    WramWrite(wram, FORMATION_COUNTER, row_bytes);
    WramWrite(wram, SNES_WRMPYA, row_bytes);
    WramWrite(wram, SNES_WRMPYB, WramRead(wram, FORMATION_PICK_A));
    pick_b = WramRead(wram, FORMATION_PICK_B);
    from = WramRead16(wram, SNES_RDMPYL);
    WramWrite(wram, SNES_WRMPYB, pick_b);
    LoadA8(cpu, pick_b);
    PushAccumulator8(memory, cpu);
    LoadA8(cpu, Pull8(memory, cpu));
    to = WramRead16(wram, SNES_RDMPYL);
    do {
        const uint8_t first = WramReadAt(wram, table, from);
        const uint8_t second = WramReadAt(wram, table, to);

        WramWriteAt(wram, table, from, second);
        WramWriteAt(wram, table, to, first);
        ++from;
        ++to;
        OpStepMem(memory, cpu, OpAbs(cpu, FORMATION_COUNTER), -1);
    } while (!cpu->zero);
}

/* Swaps the two picked members' rows in both formation tables and their
 * 13-byte state records. */
static void BattleSwapFormationBlocks(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint16_t high;
    uint16_t from;
    uint16_t to;

    SwapFormationRows(memory, cpu, wram, FORMATION_BLOCK_A, FORMATION_BLOCK_A_BYTES);
    SwapFormationRows(memory, cpu, wram, FORMATION_BLOCK_B, FORMATION_BLOCK_B_BYTES);

    LoadA8(cpu, FORMATION_STATE_BYTES);
    WramWrite(wram, FORMATION_COUNTER, FORMATION_STATE_BYTES);
    WramWrite(wram, SNES_WRMPYA, FORMATION_STATE_BYTES);
    WramWrite(wram, SNES_WRMPYB, WramRead(wram, FORMATION_PICK_A));
    /* The direct page's high byte rides along in the offsets. */
    TransferDirectToA(cpu);
    high = (uint16_t)(cpu->accumulator & 0xff00u);
    from = (uint16_t)(high | WramRead(wram, SNES_RDMPYL));
    WramWrite(wram, SNES_WRMPYB, WramRead(wram, FORMATION_PICK_B));
    to = (uint16_t)(high | WramRead(wram, SNES_RDMPYL));
    do {
        LoadA8(cpu, WramReadAt(wram, FORMATION_STATE, from));
        PushAccumulator8(memory, cpu);
        WramWriteAt(wram, FORMATION_STATE, from, WramReadAt(wram, FORMATION_STATE, to));
        LoadA8(cpu, Pull8(memory, cpu));
        WramWriteAt(wram, FORMATION_STATE, to, A8(cpu));
        ++from;
        ++to;
        OpStepMem(memory, cpu, OpAbs(cpu, FORMATION_COUNTER), -1);
    } while (!cpu->zero);
    cpu->x = from;
    cpu->y = to;
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

/* Reads the two picked members out of the bitmask in $09F2: the positions of its
 * two lowest set bits among the four slots, into $09F4/$09F5. When it holds
 * fewer, the original goes on elsewhere, so the registers are left as the
 * search left them and the caller hands off. */
static bool BattlePickSwapMembers(Lufia2CpuState *cpu, Lufia2Wram wram,
                                  uint32_t *handoff) {
    unsigned picked = 0u;
    unsigned remaining = BATTLE_PARTY_SIZE;
    uint8_t position = 0u;

    while (picked < 2u) {
        const uint8_t mask = WramRead(wram, FORMATION_COUNTER);
        const bool set = (mask & 1u) != 0;

        WramWrite(wram, FORMATION_COUNTER, (uint8_t)(mask >> 1));
        if (set) {
            WramWriteAt(wram, FORMATION_PICK_A, (uint16_t)picked, position);
            ++position;
            ++picked;
            continue;
        }
        ++position;
        if (--remaining == 0u) {
            cpu->carry = 0;
            cpu->y = (uint16_t)picked;
            LoadA8(cpu, position);
            OpLdx(cpu, 0u);
            *handoff = 0x81c8bcu;
            return false;
        }
    }
    return true;
}

/* Swaps the two picked members' ids, member records and status-icon records,
 * trading through the stack as the original did. The offsets carry the direct
 * page's high byte, which rides along in A. */
static void BattleSwapPartyEntries(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                   Lufia2Wram wram) {
    uint16_t from;
    uint16_t to;

    TransferDirectToA(cpu);
    {
        const uint16_t high = (uint16_t)(cpu->accumulator & 0xff00u);
        const uint8_t pick_a = WramRead(wram, FORMATION_PICK_A);
        const uint8_t pick_b = WramRead(wram, FORMATION_PICK_B);

        from = (uint16_t)(high | pick_a);
        to = (uint16_t)(high | pick_b);
        LoadA8(cpu, WramReadAt(wram, WRAM_BATTLE_PARTY_IDS, from));
        PushAccumulator8(memory, cpu);
        WramWriteAt(wram, WRAM_BATTLE_PARTY_IDS, from,
                    WramReadAt(wram, WRAM_BATTLE_PARTY_IDS, to));
        LoadA8(cpu, Pull8(memory, cpu));
        WramWriteAt(wram, WRAM_BATTLE_PARTY_IDS, to, A8(cpu));
        from = (uint16_t)(high | (uint8_t)(WramRead(wram, FORMATION_PICK_A) << 1));
        to = (uint16_t)(high | (uint8_t)(WramRead(wram, FORMATION_PICK_B) << 1));
    }
    OpRepWidths(cpu, 0x20u);
    LoadA16(cpu, WramRead16At(wram, WRAM_BATTLE_PARTY_RECORDS, from));
    PushAccumulator16(memory, cpu);
    WramWrite16At(wram, WRAM_BATTLE_PARTY_RECORDS, from,
                  WramRead16At(wram, WRAM_BATTLE_PARTY_RECORDS, to));
    PullAccumulator16(memory, cpu);
    WramWrite16At(wram, WRAM_BATTLE_PARTY_RECORDS, to, cpu->accumulator);
    from = (uint16_t)(from << 1);
    to = (uint16_t)(to << 1);
    LoadA16(cpu, WramRead16At(wram, WRAM_BATTLE_STATUS_ICON_RECORDS, from));
    PushAccumulator16(memory, cpu);
    LoadA16(cpu, WramRead16At(wram, BATTLE_ICON_RECORD_TIMER, from));
    PushAccumulator16(memory, cpu);
    WramWrite16At(wram, WRAM_BATTLE_STATUS_ICON_RECORDS, from,
                  WramRead16At(wram, WRAM_BATTLE_STATUS_ICON_RECORDS, to));
    WramWrite16At(wram, BATTLE_ICON_RECORD_TIMER, from,
                  WramRead16At(wram, BATTLE_ICON_RECORD_TIMER, to));
    PullAccumulator16(memory, cpu);
    WramWrite16At(wram, BATTLE_ICON_RECORD_TIMER, to, cpu->accumulator);
    PullAccumulator16(memory, cpu);
    WramWrite16At(wram, WRAM_BATTLE_STATUS_ICON_RECORDS, to, cpu->accumulator);
}

/* Swaps two party members' places: the menu asks for the other member, then
 * their ids, records and status icons trade slots and the formation is drawn
 * again. */
static CommandStep BattleSwapPartyOrder(BattleContext *battle, uint32_t *handoff) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);

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
    WramWrite(wram, FORMATION_COUNTER, A8(cpu));
    if (!BattlePickSwapMembers(cpu, wram, handoff))
        return COMMAND_HANDOFF;
    BattleSwapPartyEntries(memory, cpu, wram);
    OpSepWidths(cpu, 0x20u);
    BattleSwapFormationBlocks(memory, cpu);
    if (!BattleRedrawFormation(battle))
        return COMMAND_UNWOUND;
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
