#include "battle/battle_internal.h"
#include "core/snes_registers.h"

enum {
    PARTY_DP_PICKED_ENTRY = 0x12u,
    PARTY_DP_COMMAND = 0x26u,
    PARTY_DP_BATTLER_OFFSET = 0xd5u,
    PARTY_SUBMENU_ENTRY_PARAMETER = 0xdf01u,
};

/* $81:CC2E descendants: redraw a member label through the original children. */
static bool BattlePartyCommandLabel(BattleContext *battle, uint16_t record_site,
                                    uint16_t text_site, uint16_t label,
                                    bool leave_label_index_on_stack) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, WRAM_BATTLE_PARTY_SLOT);
    OpAslA(cpu);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    if (!BattleCall(battle, record_site, 0x81e1b5u, 2u))
        return false;
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, 0x818809u));
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpPushX(memory, cpu);
    OpLoadA(cpu, 13u);
    OpSta(memory, cpu, OpAbs(cpu, 0x09f2u));
    OpLoadA(cpu, 4u);
    OpSta(memory, cpu, OpAbs(cpu, 0x09f3u));
    OpLdy(cpu, label);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x09f4u), cpu->y);
    if (!BattleCall(battle, text_site, 0x81e503u, 2u))
        return false;
    if (!leave_label_index_on_stack)
        OpPullX(memory, cpu);
    return true;
}

static void BattlePartyCommandTextDimensions(const Lufia2Memory *memory,
                                             Lufia2CpuState *cpu) {
    OpLoadA(cpu, 16u);
    OpSta(memory, cpu, OpAbs(cpu, 0x09f2u));
    OpLoadA(cpu, 6u);
    OpSta(memory, cpu, OpAbs(cpu, 0x09f3u));
    OpLoadA(cpu, 0x21u);
    OpSta(memory, cpu, OpAbs(cpu, 0x09f6u));
}

static bool BattlePartyCommandDraw(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpLongX(cpu, 0x97b567u));
        OpSta(memory, cpu, OpDp(cpu, 0u));
        OpLda(memory, cpu, OpLongX(cpu, 0x97b568u));
        OpSta(memory, cpu, OpDp(cpu, 8u));
        OpLda(memory, cpu, OpLongX(cpu, 0x97b569u));
        OpSta(memory, cpu, OpDp(cpu, 9u));
        OpInx(cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpPushX(memory, cpu);
        if (!BattleCall(battle, 0xccacu, BATTLE_ROUTINE_TILE_BLOCK, 2u))
            return false;
        OpPullX(memory, cpu);
        OpCpx(cpu, 15u);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x10u);
    OpLda(memory, cpu, OpDp(cpu, 0x47u));
    OpAndValue(cpu, 15u);
    OpTay(cpu);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbsY(cpu, 0xb57au)));
    OpRepWidths(cpu, 0x10u);
    OpWriteX(memory, cpu, OpDp(cpu, PARTY_DP_COMMAND), cpu->x);
    OpLda(memory, cpu, OpLongX(cpu, 0x97b567u));
    cpu->carry = true;
    OpSbcValue(cpu, 8u);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLda(memory, cpu, OpLongX(cpu, 0x97b568u));
    OpSta(memory, cpu, OpDp(cpu, 8u));
    OpLda(memory, cpu, OpLongX(cpu, 0x97b569u));
    OpSta(memory, cpu, OpDp(cpu, 9u));
    return BattleCall(battle, 0xccd8u, BATTLE_ROUTINE_TILE_BLOCK, 2u) &&
           BattleCall(battle, 0xccdbu, 0x859dd4u, 3u) &&
           BattleCall(battle, 0xccdfu, 0x81d9d0u, 2u);
}

static void BattlePartyCommandPriority(const Lufia2Memory *memory,
                                       Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbsX(cpu, BATTLE_BATTLER_BASE_PRIORITY));
    cpu->carry = false;
    OpAdc(memory, cpu, OpAbsX(cpu, BATTLE_BATTLER_PRIORITY_BONUS));
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_ACTION_PRIORITY));
}

static bool BattleCommitPartyCommand(BattleContext *battle, uint16_t record_site,
                                     uint16_t queue_site, uint16_t label_record_site,
                                     uint16_t label_text_site,
                                     bool restore_party_index) {
    if (!BattleCall(battle, record_site, 0x8592ceu, 3u) ||
        !BattleCall(battle, queue_site, 0x8592ffu, 3u) ||
        !BattlePartyCommandLabel(battle, label_record_site, label_text_site, 0x87b7u,
                                 false))
        return false;
    BattlePartyCommandTextDimensions(battle->memory, battle->cpu);
    if (restore_party_index)
        OpPullX(battle->memory, battle->cpu);
    TransferDirectToA(battle->cpu);
    return true;
}

/* What the party action loop does after a command was confirmed. */
typedef enum { PARTY_POLL, PARTY_DONE, PARTY_HANDOFF, PARTY_UNWOUND } PartyStep;

/* Attack: with a weapon that needs a target the target menu runs first. */
static PartyStep PartyAttack(BattleContext *battle, uint32_t *pc) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    bool targeted = true;

    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, 0x129eu));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, PARTY_DP_BATTLER_OFFSET)));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, 0x66u));
    OpAndValue(cpu, 0x01ffu);
    if (!cpu->zero) {
        OpSta(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
        OpSepWidths(cpu, 0x20u);
        if (!BattleCall(battle, 0xcd5eu, 0x81f291u, 3u))
            return PARTY_UNWOUND;
        OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_POINTER)));
        PushDataBank(memory, cpu);
        OpSetDataBank(memory, cpu, 0x96u);
        OpLda(memory, cpu, OpAbsX(cpu, 2u));
        PullDataBank(memory, cpu);
        OpCmpValue(cpu, 0u);
        targeted = !cpu->zero;
    } else {
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, 0x82u);
    }
    if (targeted) {
        ExchangeAccumulatorBytes(cpu);
        OpLda(memory, cpu, WRAM_BATTLE_PARTY_SLOT);
        OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
        OpLoadA(cpu, 7u);
        OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
        OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, SNES_RDMPYL)));
        OpLda(memory, cpu, OpAbsX(cpu, 0x1369u));
        ExchangeAccumulatorBytes(cpu);
        OpPushX(memory, cpu);
        if (!BattleCall(battle, 0xcd90u, BATTLE_ROUTINE_CHOOSE_TARGETS, 2u))
            return PARTY_UNWOUND;
        OpPullX(memory, cpu);
        OpCmpValue(cpu, 0xffu);
        if (cpu->zero)
            return PARTY_POLL;
        PushAccumulator8(memory, cpu);
        ExchangeAccumulatorBytes(cpu);
        OpLda(memory, cpu, OpDp(cpu, 0x24u));
        OpCmpValue(cpu, 0x82u);
        if (cpu->zero) {
            ExchangeAccumulatorBytes(cpu);
            OpBitValue(cpu, 0x80u);
            if (!cpu->zero)
                OpSta(memory, cpu, OpAbsX(cpu, 0x1369u));
        }
    } else {
        PushAccumulator8(memory, cpu);
    }
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, PARTY_DP_BATTLER_OFFSET)));
    OpLoadA(cpu, BATTLE_ACTION_ATTACK);
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_ACTION_TYPE));
    LoadA8(cpu, Pull8(memory, cpu));
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_ACTION_TARGET_MASK));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, 0x66u));
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_ACTION_PARAMETER));
    BattlePartyCommandPriority(memory, cpu);
    if (!BattleCommitPartyCommand(battle, 0xcdcbu, 0xcdcfu, 0xcddbu, 0xcdf8u, false))
        return PARTY_UNWOUND;
    *pc = 0x81ce0cu;
    return PARTY_DONE;
}

/* The entry/target choice shared by spells, items and IP attacks: where the
 * child calls sit, how big one list record is, and which table says whether an
 * entry needs a target. */
typedef struct PartyPickSpec {
    uint16_t cancel_site;
    uint16_t window_site;
    uint16_t target_site;
    uint16_t retry_site;
    uint8_t record_size;
    uint8_t window_kind;       /* written to $129E, unless it comes from a table */
    uint32_t window_kind_long; /* table byte for the kind, 0 if constant */
    uint32_t needs_target_long;
} PartyPickSpec;

static const PartyPickSpec PARTY_PICK_SPELL = {0xce35u, 0xce53u, 0xce5eu, 0xce2eu,
                                               16u,     2u,      0u,      0x7edf0fu};
static const PartyPickSpec PARTY_PICK_ITEM = {0xceffu, 0xcf1du, 0xcf28u, 0xcef8u,
                                              16u,     3u,      0u,      0x7edf13u};
static const PartyPickSpec PARTY_PICK_IP = {0xcfd6u, 0xcff6u, 0xd001u,   0xcfcfu,
                                            24u,     0u,      0x7edf03u, 0x7edf04u};

typedef enum { PICK_CANCELLED, PICK_UNWOUND, PICK_CHOSEN } PartyPick;

/* Loops until the player picks an entry (and, if it needs one, a target) or
 * backs out. The selection comes back in DP $12 and A. */
static PartyPick PartyPickEntryAndTarget(BattleContext *battle,
                                         const PartyPickSpec *spec) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    for (;;) {
        OpCmpValue(cpu, 0u);
        if (!cpu->zero) {
            if (!BattleCall(battle, spec->cancel_site, 0x81e16fu, 2u))
                return PICK_UNWOUND;
            OpPullX(memory, cpu);
            return PICK_CANCELLED;
        }
        OpLda(memory, cpu, OpDp(cpu, PARTY_DP_PICKED_ENTRY));
        OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
        OpLoadA(cpu, spec->record_size);
        OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
        OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, SNES_RDMPYL)));
        OpPushX(memory, cpu);
        if (spec->window_kind_long != 0u)
            OpLda(memory, cpu, OpLongX(cpu, spec->window_kind_long));
        else
            OpLoadA(cpu, spec->window_kind);
        OpSta(memory, cpu, OpAbs(cpu, 0x129eu));
        if (!BattleCall(battle, spec->window_site, 0x81e16fu, 2u))
            return PICK_UNWOUND;
        OpPullX(memory, cpu);
        TransferDirectToA(cpu);
        OpLda(memory, cpu, OpLongX(cpu, spec->needs_target_long));
        if (!cpu->zero) {
            if (!BattleCall(battle, spec->target_site, BATTLE_ROUTINE_CHOOSE_TARGETS,
                            2u))
                return PICK_UNWOUND;
            OpCmpValue(cpu, 0xffu);
            if (cpu->zero) {
                if (!BattleCall(battle, spec->retry_site, 0x81d19au, 2u))
                    return PICK_UNWOUND;
                continue;
            }
        }
        return PICK_CHOSEN;
    }
}

/* Takes the chosen entry's value from the submenu table (16 bytes per entry),
 * maps it through the byte table at $7E:0096 and stores it as the action
 * parameter. */
static void PartyStoreSpellParameter(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpLda(memory, cpu, OpDp(cpu, PARTY_DP_PICKED_ENTRY));
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    OpAslA(cpu);
    OpAslA(cpu);
    OpAslA(cpu);
    OpAslA(cpu);
    OpTay(cpu);
    OpLda(memory, cpu, OpAbsY(cpu, PARTY_SUBMENU_ENTRY_PARAMETER));
    OpTay(cpu);
    OpLda(memory, cpu, OpAbsY(cpu, 0x96u));
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_ACTION_PARAMETER));
    OpStz(memory, cpu, OpAbs(cpu, (BATTLE_ACTION_PARAMETER + 1u)));
    PullDataBank(memory, cpu);
}

/* Spell: asks for the spell, then for its target. */
static PartyStep PartySpell(BattleContext *battle, uint32_t *pc) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    OpPushX(memory, cpu);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, PARTY_DP_BATTLER_OFFSET)));
    OpLda(memory, cpu, OpAbsX(cpu, BATTLE_BATTLER_STATUS));
    OpBitValue(cpu, 2u);
    if (!cpu->zero) {
        if (!BattleCall(battle, 0xce20u, 0x81d975u, 2u))
            return PARTY_UNWOUND;
        OpPullX(memory, cpu);
        return PARTY_POLL;
    }
    OpLoadA(cpu, 1u);
    if (!BattleCall(battle, 0xce29u, 0x81d12fu, 2u))
        return PARTY_UNWOUND;
    switch (PartyPickEntryAndTarget(battle, &PARTY_PICK_SPELL)) {
    case PICK_CANCELLED:
        return PARTY_POLL;
    case PICK_UNWOUND:
        return PARTY_UNWOUND;
    case PICK_CHOSEN:
        break;
    }
    PushAccumulator8(memory, cpu);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, PARTY_DP_BATTLER_OFFSET)));
    OpLoadA(cpu, BATTLE_ACTION_SPELL);
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_ACTION_TYPE));
    LoadA8(cpu, Pull8(memory, cpu));
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_ACTION_TARGET_MASK));
    OpRepWidths(cpu, 0x20u);
    BattlePartyCommandPriority(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    PartyStoreSpellParameter(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    if (!BattleCommitPartyCommand(battle, 0xcea5u, 0xcea9u, 0xceb5u, 0xced2u, true))
        return PARTY_UNWOUND;
    *pc = 0x81cee7u;
    return PARTY_DONE;
}

/* Item: asks for the item, then for its target. */
static PartyStep PartyItem(BattleContext *battle, uint32_t *pc) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    OpPushX(memory, cpu);
    TransferDirectToA(cpu);
    if (!BattleCall(battle, 0xcef3u, 0x81d12fu, 2u))
        return PARTY_UNWOUND;
    switch (PartyPickEntryAndTarget(battle, &PARTY_PICK_ITEM)) {
    case PICK_CANCELLED:
        return PARTY_POLL;
    case PICK_UNWOUND:
        return PARTY_UNWOUND;
    case PICK_CHOSEN:
        break;
    }
    PushAccumulator8(memory, cpu);
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, PARTY_DP_BATTLER_OFFSET)));
    OpLoadA(cpu, BATTLE_ACTION_ITEM);
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_ACTION_TYPE));
    LoadA8(cpu, Pull8(memory, cpu));
    if (cpu->zero)
        OpLoadA(cpu, 0x81u);
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_ACTION_TARGET_MASK));
    OpLda(memory, cpu, OpDp(cpu, PARTY_DP_PICKED_ENTRY));
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, 0x0a8du));
    OpAndValue(cpu, 0x1ffu);
    OpOraValue(cpu, 0x200u);
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_ACTION_PARAMETER));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
    OpSepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_POINTER));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, PARTY_DP_BATTLER_OFFSET)));
    OpRepWidths(cpu, 0x20u);
    BattlePartyCommandPriority(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 1u);
    if (!BattleCall(battle, 0xcf70u, 0x81f15bu, 3u))
        return PARTY_UNWOUND;
    OpCmpValue(cpu, 0u);
    if (cpu->zero) {
        *pc = 0x81cf78u;
        return PARTY_HANDOFF;
    }
    if (!BattleCommitPartyCommand(battle, 0xcf79u, 0xcf7du, 0xcf8bu, 0xcfa8u, true))
        return PARTY_UNWOUND;
    *pc = 0x81cfbdu;
    return PARTY_DONE;
}

/* Copies the chosen IP skill's word from the submenu table (24 bytes per
 * entry) into the action parameter. */
static void PartyStoreIpParameter(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpLda(memory, cpu, OpDp(cpu, PARTY_DP_PICKED_ENTRY));
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    OpAslA(cpu);
    OpAslA(cpu);
    OpAslA(cpu);
    PushAccumulator16(memory, cpu);
    OpAslA(cpu);
    OpAdc(memory, cpu, OpStack(cpu, 1u));
    OpTay(cpu);
    PullAccumulator16(memory, cpu);
    OpLda(memory, cpu, OpAbsY(cpu, PARTY_SUBMENU_ENTRY_PARAMETER));
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_ACTION_PARAMETER));
    PullDataBank(memory, cpu);
}

/* IP attack: asks for the skill, then for its target. */
static PartyStep PartyIp(BattleContext *battle, uint32_t *pc) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    OpPushX(memory, cpu);
    OpLoadA(cpu, 2u);
    if (!BattleCall(battle, 0xcfcau, 0x81d12fu, 2u))
        return PARTY_UNWOUND;
    switch (PartyPickEntryAndTarget(battle, &PARTY_PICK_IP)) {
    case PICK_CANCELLED:
        return PARTY_POLL;
    case PICK_UNWOUND:
        return PARTY_UNWOUND;
    case PICK_CHOSEN:
        break;
    }
    PushAccumulator8(memory, cpu);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, PARTY_DP_BATTLER_OFFSET)));
    OpLoadA(cpu, BATTLE_ACTION_IP);
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_ACTION_TYPE));
    LoadA8(cpu, Pull8(memory, cpu));
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_ACTION_TARGET_MASK));
    OpRepWidths(cpu, 0x20u);
    BattlePartyCommandPriority(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    PartyStoreIpParameter(memory, cpu);
    if (!BattleCommitPartyCommand(battle, 0xd03eu, 0xd042u, 0xd04eu, 0xd06bu, true))
        return PARTY_UNWOUND;
    *pc = 0x81d080u;
    return PARTY_DONE;
}

/* Defend. */
static PartyStep PartyDefend(BattleContext *battle, uint32_t *pc) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, PARTY_DP_BATTLER_OFFSET)));
    OpLoadA(cpu, BATTLE_ACTION_DEFEND);
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_ACTION_TYPE));
    OpLoadA(cpu, 0x81u);
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_ACTION_TARGET_MASK));
    OpRepWidths(cpu, 0x20u);
    BattlePartyCommandPriority(memory, cpu);
    if (!BattleCommitPartyCommand(battle, 0xd0e7u, 0xd0ebu, 0xd0f7u, 0xd114u, false))
        return PARTY_UNWOUND;
    *pc = 0x81d128u;
    return PARTY_DONE;
}

/* A command was confirmed: runs the part that belongs to the command picked in
 * the menu. */
static PartyStep PartyAccepted(BattleContext *battle, uint32_t *pc) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;

    OpLoadA(cpu, 2u);
    if (!BattleCall(battle, 0xcd3bu, 0x80953bu, 3u))
        return PARTY_UNWOUND;
    OpLda(memory, cpu, OpDp(cpu, PARTY_DP_COMMAND));
    OpCmpValue(cpu, BATTLE_PARTY_COMMAND_ATTACK);
    if (cpu->zero)
        return PartyAttack(battle, pc);
    OpLda(memory, cpu, OpDp(cpu, PARTY_DP_COMMAND));
    OpCmpValue(cpu, BATTLE_PARTY_COMMAND_SPELL);
    if (cpu->zero)
        return PartySpell(battle, pc);
    OpLda(memory, cpu, OpDp(cpu, PARTY_DP_COMMAND));
    OpCmpValue(cpu, BATTLE_PARTY_COMMAND_ITEM);
    if (cpu->zero)
        return PartyItem(battle, pc);
    OpLda(memory, cpu, OpDp(cpu, PARTY_DP_COMMAND));
    OpCmpValue(cpu, BATTLE_PARTY_COMMAND_IP);
    if (cpu->zero)
        return PartyIp(battle, pc);
    OpLda(memory, cpu, OpDp(cpu, PARTY_DP_COMMAND));
    OpCmpValue(cpu, BATTLE_PARTY_COMMAND_DEFEND);
    if (!cpu->zero) {
        *pc = 0x81d12cu;
        return PARTY_HANDOFF;
    }
    return PartyDefend(battle, pc);
}

/* $81:CC2E: individual action selection with exact cancellation boundaries. */
Lufia2ExecutionResult Lufia2BattleChoosePartyAction(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu,
                                                    Lufia2PushedChildCall child,
                                                    void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);
    uint32_t pc = 0u;
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, PARTY_DP_BATTLER_OFFSET)));
    OpLoadA(cpu, BATTLE_ACTION_NONE);
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_ACTION_TYPE));
    if (!BattleCall(&battle, 0xcc35u, 0x8592ceu, 3u) ||
        !BattleCall(&battle, 0xcc39u, BATTLE_ROUTINE_PARTY_WINDOWS, 2u) ||
        !BattlePartyCommandLabel(&battle, 0xcc46u, 0xcc63u, 0x87c7u, true))
        return BattleChildUnwound(&battle);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, WRAM_BATTLE_PARTY_SLOT);
    OpAslA(cpu);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x65u);
    OpSta(memory, cpu, OpDp(cpu, 0x11u));
    if (!BattleCall(&battle, 0xcc74u, 0x81e4d1u, 2u))
        return BattleChildUnwound(&battle);
    OpPullX(memory, cpu);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpDp(cpu, 2u));
    OpSta(memory, cpu, OpDp(cpu, 3u));
    OpSta(memory, cpu, OpDp(cpu, 1u));
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpDp(cpu, 4u));
    OpLoadA(cpu, 0x97u);
    OpSta(memory, cpu, OpDp(cpu, 0x24u));
    OpLdy(cpu, 0xfe06u);
    TransferDirectToA(cpu);
    if (!BattleCall(&battle, 0xcc8cu, BATTLE_ROUTINE_LOAD_PALETTE, 2u) ||
        !BattleCall(&battle, 0xcc8fu, BATTLE_ROUTINE_COMMIT_PALETTES, 3u))
        return BattleChildUnwound(&battle);
    for (;;) {
        PartyStep step;

        if (!BattlePartyCommandDraw(&battle))
            return BattleChildUnwound(&battle);
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, BATTLE_SPRITE_REBUILD_REQUEST);
        if (!BattleCall(&battle, 0xcce8u, BATTLE_ROUTINE_FRAME_INPUT, 3u))
            return BattleChildUnwound(&battle);
        OpLda(memory, cpu, OpDp(cpu, BATTLE_DP_PAD_FILTERED));
        OpBitValue(cpu, 0xa0u);
        if (cpu->zero) {
            OpLda(memory, cpu, OpDp(cpu, BATTLE_DP_PAD_FILTERED_HIGH));
            if (!cpu->negative)
                continue;
            if (!BattlePartyCommandLabel(&battle, 0xcd00u, 0xcd1du, 0x87b7u, false))
                return BattleChildUnwound(&battle);
            BattlePartyCommandTextDimensions(memory, cpu);
            OpLoadA(cpu, 1u);
            if (!BattleCall(&battle, 0xcd32u, 0x80953bu, 3u))
                return BattleChildUnwound(&battle);
            OpLoadA(cpu, 0xffu);
            return ExecutionReturned(0x81cd38u);
        }
        step = PartyAccepted(&battle, &pc);
        if (step == PARTY_UNWOUND)
            return BattleChildUnwound(&battle);
        if (step == PARTY_DONE)
            return ExecutionReturned(pc);
        if (step == PARTY_HANDOFF)
            return ExecutionHandoff(cpu, pc);
    }
}
