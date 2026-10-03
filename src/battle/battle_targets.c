#include "battle/battle_internal.h"

enum {
    TARGET_DP_MODE = 0x24u,
    TARGET_DP_RECORDS_LEFT = 0x25u,
    TARGET_DP_CURSOR = 0x26u,
    TARGET_DP_SIDE_OR_MASK = 0x27u,
    TARGET_DP_BLINK_PHASE = 0x28u,
    TARGET_DP_MOVE_DELTA = 0x29u,
};

enum {
    TARGET_PARTY_UNAVAILABLE = WRAM_BATTLE_PARTY_TARGETS + 2u,
    TARGET_PARTY_SELECTED = WRAM_BATTLE_PARTY_TARGETS + 3u,
    TARGET_ENEMY_UNAVAILABLE = WRAM_BATTLE_ENEMY_TARGETS + 2u,
    TARGET_ENEMY_SELECTED = WRAM_BATTLE_ENEMY_TARGETS + 3u,
    TARGET_CAPSULE_UNAVAILABLE =
        TARGET_PARTY_UNAVAILABLE + BATTLE_PARTY_SIZE * BATTLE_TARGET_RECORD_SIZE,
    TARGET_PARTY_LAST_SELECTED =
        TARGET_PARTY_SELECTED +
        (BATTLE_PARTY_TARGET_COUNT - 1u) * BATTLE_TARGET_RECORD_SIZE,
    TARGET_ENEMY_LAST_SELECTED =
        TARGET_ENEMY_SELECTED + (BATTLE_ENEMY_COUNT - 1u) * BATTLE_TARGET_RECORD_SIZE,
    TARGET_ENEMY_SEARCH_LAST = TARGET_ENEMY_UNAVAILABLE +
                               (BATTLE_ENEMY_COUNT - 2u) * BATTLE_TARGET_RECORD_SIZE,
};

static bool TargetBuildSelectionRecords(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0u);
    OpLoadA(cpu, BATTLE_PARTY_TARGET_COUNT + 1u + BATTLE_ENEMY_COUNT);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, TARGET_PARTY_UNAVAILABLE));
        OpInx(cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpDecA(cpu);
    } while (!cpu->zero);
    OpLdx(cpu, 0u);
    OpTxy(cpu);
    do {
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpLongX(cpu, 0x97b5c9u));
        OpSta(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_PARTY_TARGETS));
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_PARTY_RECORDS));
        if (cpu->zero) {
            OpLoadA(cpu, 0xffu);
            OpSta(memory, cpu, OpAbsY(cpu, TARGET_PARTY_UNAVAILABLE));
        }
        OpSepWidths(cpu, 0x20u);
        OpIny(cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, BATTLE_PARTY_SIZE * BATTLE_POINTER_SIZE);
    } while (!cpu->zero);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, TARGET_CAPSULE_UNAVAILABLE));
    TransferDirectToA(cpu);
    OpLdx(cpu, 0u);
    OpTxy(cpu);
    do {
        PushAccumulator8(memory, cpu);
        OpPushX(memory, cpu);
        PushY(memory, cpu);
        if (!BattleCall(battle, 0xd52eu, 0x81b8b1u, 2u))
            return false;
        OpPullY(memory, cpu);
        OpPullX(memory, cpu);
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
        OpSta(memory, cpu, OpAbsX(cpu, TARGET_ENEMY_UNAVAILABLE));
        OpLda(memory, cpu, OpDp(cpu, 0x22u));
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_ENEMY_TARGETS));
        OpLda(memory, cpu, OpDp(cpu, 0x23u));
        OpSta(memory, cpu, OpAbsX(cpu, (WRAM_BATTLE_ENEMY_TARGETS + 1u)));
        LoadA8(cpu, Pull8(memory, cpu));
        OpIny(cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpIncA(cpu);
        OpCmpValue(cpu, BATTLE_ENEMY_COUNT);
    } while (!cpu->zero);
    return true;
}

static void TargetClearName(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdy(cpu, 0x7fu);
    TransferDirectToA(cpu);
    do {
        OpSta(memory, cpu, OpAbsY(cpu, 0x3800u));
        OpDey(cpu);
    } while (!cpu->negative);
}

static void TargetLoadCursorRecordOffset(const Lufia2Memory *memory,
                                         Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, TARGET_DP_CURSOR));
    OpAslA(cpu);
    OpAslA(cpu);
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
}

static void TargetFindFirstEnemy(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    /* Slot five is the original fallback. */
    for (uint16_t address = TARGET_ENEMY_UNAVAILABLE;
         address <= TARGET_ENEMY_SEARCH_LAST; address += BATTLE_TARGET_RECORD_SIZE) {
        OpLda(memory, cpu, OpAbs(cpu, address));
        if (cpu->zero)
            break;
        OpStepMem(memory, cpu, OpDp(cpu, TARGET_DP_CURSOR), 1);
    }
}

static void TargetClearParty(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    for (uint16_t address = TARGET_PARTY_SELECTED;
         address <= TARGET_PARTY_LAST_SELECTED; address += BATTLE_TARGET_RECORD_SIZE)
        OpStz(memory, cpu, OpAbs(cpu, address));
}

static void TargetClearEnemies(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    for (uint16_t address = TARGET_ENEMY_SELECTED;
         address <= TARGET_ENEMY_LAST_SELECTED; address += BATTLE_TARGET_RECORD_SIZE)
        OpStz(memory, cpu, OpAbs(cpu, address));
}

static void TargetPublishSelectionMask(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                       uint16_t unavailable_field,
                                       uint16_t last_record_offset) {
    OpStz(memory, cpu, OpDp(cpu, TARGET_DP_SIDE_OR_MASK));
    OpLdx(cpu, last_record_offset);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, unavailable_field));
        bool selected = false;
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbsX(cpu, (uint16_t)(unavailable_field + 1u)));
            selected = !cpu->zero;
        }
        cpu->carry = selected;
        OpRolMem8(memory, cpu, OpDp(cpu, TARGET_DP_SIDE_OR_MASK));
        OpDex(cpu);
        OpDex(cpu);
        OpDex(cpu);
        OpDex(cpu);
    } while (!cpu->negative);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_BATTLE_CURSOR_ENABLED));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, 0x0012f3u);
}

static bool TargetDrawSelection(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    TargetClearName(memory, cpu);
    OpLdx(cpu, 0u);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_BATTLE_CURSOR_COUNT));
    OpLda(memory, cpu, OpDp(cpu, TARGET_DP_CURSOR));
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    OpAslA(cpu);
    OpAslA(cpu);
    OpTay(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, TARGET_DP_SIDE_OR_MASK));
    if (cpu->zero) {
        OpStz(memory, cpu, OpAbs(cpu, SNES_WRIO));
        OpStz(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
        if (!BattleCall(battle, 0xd5afu, 0x81d92cu, 2u))
            return false;
    } else {
        if (!BattleCall(battle, 0xd5b4u, 0x81d920u, 2u))
            return false;
        OpPushX(memory, cpu);
        OpLda(memory, cpu, OpDp(cpu, TARGET_DP_CURSOR));
        OpAslA(cpu);
        OpTax(cpu);
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpLongX(cpu, 0x859ec8u));
        OpTay(cpu);
        OpLda(memory, cpu, OpAbs(cpu, 0x4abeu));
        OpAndValue(cpu, 0xffu);
        OpLsrA(cpu);
        OpLsrA(cpu);
        OpCmpValue(cpu, 38u);
        if (cpu->carry)
            OpLoadA(cpu, 36u);
        cpu->carry = false;
        OpAdcValue(cpu, 0x3840u);
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, 0x7eu);
        OpSta(memory, cpu, OpDp(cpu, 0x5fu));
        OpLoadA(cpu, 0x20u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
        if (!BattleCall(battle, 0xd5e3u, 0x808878u, 3u))
            return false;
        OpLda(memory, cpu, OpAbs(cpu, 0x4abeu));
        OpCmpValue(cpu, 0x98u);
        if (cpu->carry)
            OpLoadA(cpu, 0x90u);
        OpDecA(cpu);
        OpSta(memory, cpu, OpAbs(cpu, SNES_NMITIMEN));
        OpDecA(cpu);
        OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
        cpu->carry = false;
        OpAdcValue(cpu, 0x68u);
        OpSta(memory, cpu, OpAbs(cpu, SNES_WRIO));
        OpIncA(cpu);
        OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
        OpPullX(memory, cpu);
    }
    OpLda(memory, cpu, OpDp(cpu, TARGET_DP_BLINK_PHASE));
    if (cpu->zero) {
        OpLdy(cpu, 0u);
        OpLoadA(cpu, BATTLE_PARTY_TARGET_COUNT + 1u + BATTLE_ENEMY_COUNT);
        OpSta(memory, cpu, OpDp(cpu, TARGET_DP_RECORDS_LEFT));
        do {
            OpLda(memory, cpu, OpAbsY(cpu, TARGET_PARTY_UNAVAILABLE));
            if (cpu->zero) {
                OpLda(memory, cpu, OpAbsY(cpu, TARGET_PARTY_SELECTED));
                if (!cpu->zero && !BattleCall(battle, 0xd618u, 0x81d948u, 2u))
                    return false;
            }
            OpIny(cpu);
            OpIny(cpu);
            OpIny(cpu);
            OpIny(cpu);
            OpStepMem(memory, cpu, OpDp(cpu, TARGET_DP_RECORDS_LEFT), -1);
        } while (!cpu->zero);
    }
    OpLda(memory, cpu, OpDp(cpu, TARGET_DP_BLINK_PHASE));
    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
    OpSta(memory, cpu, OpDp(cpu, TARGET_DP_BLINK_PHASE));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_CURSOR_COUNT));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_CURSOR_ENABLED));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, 0x0012f3u);
    return true;
}

/* What the main loop does after a handler ran. */
typedef enum { TARGET_REDRAW, TARGET_UNWOUND, TARGET_DONE } TargetOutcome;

/* Loads the side flag: zero for the party list, $80 for the enemy list. */
static void TargetLoadSide(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, TARGET_DP_SIDE_OR_MASK));
}

/* Steps the cursor by the move delta. Running off the front wraps to the end
 * of the list, running off the end wraps to the front. */
static void TargetStepCursor(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    bool party;

    OpLda(memory, cpu, OpDp(cpu, TARGET_DP_CURSOR));
    cpu->carry = false;
    OpAdc(memory, cpu, OpDp(cpu, TARGET_DP_MOVE_DELTA));
    if (cpu->negative) {
        TargetLoadSide(memory, cpu);
        cpu->carry = false;
        RolA8(cpu);
        RolA8(cpu);
        OpAdcValue(cpu, 4u);
    }
    PushAccumulator8(memory, cpu);
    TargetLoadSide(memory, cpu);
    party = cpu->zero;
    LoadA8(cpu, Pull8(memory, cpu));
    OpCmpValue(cpu, party ? BATTLE_PARTY_TARGET_COUNT : BATTLE_ENEMY_COUNT);
    if (cpu->zero)
        OpLoadA(cpu, 0u);
}

/* Takes the candidate cursor from A, makes it the cursor, and tells whether
 * that target can be chosen. */
static bool TargetCursorAvailable(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    bool party;

    OpSta(memory, cpu, OpDp(cpu, TARGET_DP_CURSOR));
    ExchangeAccumulatorBytes(cpu);
    TargetLoadSide(memory, cpu);
    party = cpu->zero;
    ExchangeAccumulatorBytes(cpu);
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    OpAslA(cpu);
    OpAslA(cpu);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu,
          OpAbsX(cpu, party ? TARGET_PARTY_UNAVAILABLE : TARGET_ENEMY_UNAVAILABLE));
    return cpu->zero;
}

/* Keeps stepping from the candidate in A until the cursor rests on a target
 * that can be chosen. */
static void TargetSettleCursor(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    while (!TargetCursorAvailable(memory, cpu))
        TargetStepCursor(memory, cpu);
}

/* A pad direction left or right. */
static void TargetMoveAcross(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    TargetStepCursor(memory, cpu);
    TargetSettleCursor(memory, cpu);
}

/* Moving past the last party member leads to the first enemy. */
static void TargetSwitchToEnemies(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    TargetClearParty(memory, cpu);
    OpStz(memory, cpu, OpDp(cpu, TARGET_DP_CURSOR));
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, TARGET_DP_SIDE_OR_MASK));
    TargetFindFirstEnemy(memory, cpu);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpDp(cpu, TARGET_DP_MOVE_DELTA));
    OpDecA(cpu);
    TargetSettleCursor(memory, cpu);
}

/* Moving back from the enemies leads to the first party member. */
static void TargetSwitchToParty(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    TargetClearEnemies(memory, cpu);
    OpStz(memory, cpu, OpDp(cpu, TARGET_DP_CURSOR));
    OpStz(memory, cpu, OpDp(cpu, TARGET_DP_SIDE_OR_MASK));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpDp(cpu, TARGET_DP_MOVE_DELTA));
    OpDecA(cpu);
    TargetSettleCursor(memory, cpu);
}

/* A pad direction up or down, the delta already stored. On the party side
 * the cursor follows the neighbour tables until it finds a target that can be
 * chosen; running off either end switches to the enemy list. */
static void TargetMoveVertical(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    bool down;

    TargetLoadSide(memory, cpu);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, TARGET_DP_MOVE_DELTA));
        OpBitValue(cpu, 0x80u);
        if (cpu->zero)
            TargetSwitchToParty(memory, cpu);
        return;
    }
    OpLda(memory, cpu, OpDp(cpu, TARGET_DP_MOVE_DELTA));
    OpBitValue(cpu, 0x80u);
    down = cpu->zero;
    TransferDirectToA(cpu);
    if (down) {
        /* Down. */
        do {
            OpLda(memory, cpu, OpDp(cpu, TARGET_DP_CURSOR));
            OpTax(cpu);
            OpLda(memory, cpu, OpLongX(cpu, 0x97b5bau));
            if (cpu->negative) {
                TargetSwitchToEnemies(memory, cpu);
                return;
            }
            OpSta(memory, cpu, OpDp(cpu, TARGET_DP_CURSOR));
            OpAslA(cpu);
            OpAslA(cpu);
            OpTax(cpu);
            OpLda(memory, cpu, OpAbsX(cpu, TARGET_PARTY_UNAVAILABLE));
        } while (!cpu->zero);
        return;
    }
    /* Up. */
    do {
        OpLda(memory, cpu, OpDp(cpu, TARGET_DP_CURSOR));
        OpTax(cpu);
        OpLda(memory, cpu, OpDp(cpu, TARGET_DP_MODE));
        OpAndValue(cpu, 0x20u);
        OpLda(memory, cpu, OpLongX(cpu, cpu->zero ? 0x97b5bfu : 0x97b5c4u));
        if (cpu->negative) {
            TargetSwitchToEnemies(memory, cpu);
            return;
        }
        OpSta(memory, cpu, OpDp(cpu, TARGET_DP_CURSOR));
        OpAslA(cpu);
        OpAslA(cpu);
        OpTax(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, TARGET_PARTY_UNAVAILABLE));
    } while (!cpu->zero);
}

/* The "select all" button: flips every record of the cursor's side to the
 * opposite of the record under the cursor. */
static void TargetToggleAll(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpDp(cpu, TARGET_DP_CURSOR));
    OpAslA(cpu);
    OpAslA(cpu);
    OpTax(cpu);
    TargetLoadSide(memory, cpu);
    if (cpu->zero) {
        OpLda(memory, cpu, OpAbsX(cpu, TARGET_PARTY_SELECTED));
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
        for (uint16_t address = TARGET_PARTY_SELECTED;
             address <= TARGET_PARTY_LAST_SELECTED;
             address += BATTLE_TARGET_RECORD_SIZE)
            OpSta(memory, cpu, OpAbs(cpu, address));
    } else {
        OpLda(memory, cpu, OpAbsX(cpu, TARGET_ENEMY_SELECTED));
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
        for (uint16_t address = TARGET_ENEMY_SELECTED;
             address <= TARGET_ENEMY_LAST_SELECTED;
             address += BATTLE_TARGET_RECORD_SIZE)
            OpSta(memory, cpu, OpAbs(cpu, address));
    }
}

/* Leaves with the party's selection published. */
static TargetOutcome TargetFinishParty(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                       uint32_t *return_pc) {
    TargetPublishSelectionMask(memory, cpu, TARGET_PARTY_UNAVAILABLE, 16u);
    TargetLoadSide(memory, cpu);
    *return_pc = 0x81d876u;
    return TARGET_DONE;
}

/* Leaves with the enemies' selection published. */
static TargetOutcome TargetFinishEnemies(const Lufia2Memory *memory,
                                         Lufia2CpuState *cpu, uint32_t *return_pc) {
    TargetPublishSelectionMask(memory, cpu, TARGET_ENEMY_UNAVAILABLE, 20u);
    OpStz(memory, cpu, OpAbs(cpu, SNES_WRIO));
    OpStz(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
    TargetLoadSide(memory, cpu);
    OpOraValue(cpu, 0x80u);
    *return_pc = 0x81d8c1u;
    return TARGET_DONE;
}

/* The confirm button. What counts as a complete choice depends on the mode
 * (low two bits of the mode byte): a lone target of either side (2), the whole
 * party (3), or one target that is marked first and confirmed on the second
 * press. */
static TargetOutcome TargetAccept(BattleContext *battle, uint32_t *return_pc) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;

    OpLoadA(cpu, 2u);
    if (!BattleCall(battle, 0xd7bdu, 0x80953bu, 3u))
        return TARGET_UNWOUND;
    TargetClearName(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    if (!BattleCall(battle, 0xd7cdu, 0x859cc0u, 3u))
        return TARGET_UNWOUND;
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, TARGET_DP_MODE));
    OpAndValue(cpu, 3u);
    OpCmpValue(cpu, 2u);
    if (cpu->zero) {
        TargetLoadSide(memory, cpu);
        if (!cpu->zero) {
            TargetLoadCursorRecordOffset(memory, cpu);
            OpLoadA(cpu, 0xffu);
            OpSta(memory, cpu, OpAbsX(cpu, TARGET_ENEMY_SELECTED));
            return TargetFinishEnemies(memory, cpu, return_pc);
        }
        TargetLoadCursorRecordOffset(memory, cpu);
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpAbsX(cpu, TARGET_PARTY_SELECTED));
        return TargetFinishParty(memory, cpu, return_pc);
    }
    OpCmpValue(cpu, 3u);
    if (cpu->zero) {
        TargetLoadCursorRecordOffset(memory, cpu);
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpAbsX(cpu, TARGET_PARTY_SELECTED));
        TransferDirectToA(cpu);
        OpLdx(cpu, 12u);
        do {
            cpu->carry = false;
            OpAdc(memory, cpu, OpAbsX(cpu, TARGET_PARTY_SELECTED));
            OpDex(cpu);
            OpDex(cpu);
            OpDex(cpu);
            OpDex(cpu);
        } while (!cpu->negative);
        OpCmpValue(cpu, 0xfeu);
        if (!cpu->zero)
            return TARGET_REDRAW;
        return TargetFinishParty(memory, cpu, return_pc);
    }
    TargetLoadSide(memory, cpu);
    if (!cpu->zero) {
        TargetLoadCursorRecordOffset(memory, cpu);
        OpLda(memory, cpu, OpAbsX(cpu, TARGET_ENEMY_SELECTED));
        if (!cpu->zero)
            return TargetFinishEnemies(memory, cpu, return_pc);
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpAbsX(cpu, TARGET_ENEMY_SELECTED));
        return TARGET_REDRAW;
    }
    TargetLoadCursorRecordOffset(memory, cpu);
    OpLda(memory, cpu, OpAbsX(cpu, TARGET_PARTY_SELECTED));
    if (!cpu->zero)
        return TargetFinishParty(memory, cpu, return_pc);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbsX(cpu, TARGET_PARTY_SELECTED));
    return TARGET_REDRAW;
}

/* The cancel button: takes back the mark under the cursor, or when there is
 * none leaves without a choice. */
static TargetOutcome TargetCancel(BattleContext *battle, uint32_t *return_pc) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;

    OpLoadA(cpu, 1u);
    if (!BattleCall(battle, 0xd8c4u, 0x80953bu, 3u))
        return TARGET_UNWOUND;
    TargetClearName(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    if (!BattleCall(battle, 0xd8d4u, 0x859cc0u, 3u))
        return TARGET_UNWOUND;
    OpSepWidths(cpu, 0x20u);
    TargetLoadSide(memory, cpu);
    if (cpu->zero) {
        TargetLoadCursorRecordOffset(memory, cpu);
        OpLda(memory, cpu, OpAbsX(cpu, TARGET_PARTY_SELECTED));
        if (!cpu->zero) {
            OpStz(memory, cpu, OpAbsX(cpu, TARGET_PARTY_SELECTED));
            return TARGET_REDRAW;
        }
    } else {
        TargetLoadCursorRecordOffset(memory, cpu);
        OpLda(memory, cpu, OpAbsX(cpu, TARGET_ENEMY_SELECTED));
        if (!cpu->zero) {
            OpStz(memory, cpu, OpAbsX(cpu, TARGET_ENEMY_SELECTED));
            return TARGET_REDRAW;
        }
    }
    OpStz(memory, cpu, OpAbs(cpu, WRAM_BATTLE_CURSOR_ENABLED));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, 0x0012f3u);
    OpStz(memory, cpu, OpAbs(cpu, SNES_WRIO));
    OpStz(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
    OpLoadA(cpu, 0xffu);
    *return_pc = 0x81d908u;
    return TARGET_DONE;
}

/* Reads the pad: confirm, select all, cancel, or a direction. The direction
 * table at $97:B5AA has bit 0 set for left/right and is zero for no
 * direction. */
static TargetOutcome TargetHandleInput(BattleContext *battle, uint32_t *return_pc) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;

    OpLda(memory, cpu, OpDp(cpu, 0xddu));
    OpBitValue(cpu, 0xa0u);
    if (!cpu->zero)
        return TargetAccept(battle, return_pc);
    OpBitValue(cpu, 0x30u);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, TARGET_DP_MODE));
        OpAndValue(cpu, 3u);
        OpCmpValue(cpu, 1u);
        if (cpu->zero) {
            TargetToggleAll(memory, cpu);
            return TARGET_REDRAW;
        }
    }
    OpLda(memory, cpu, OpDp(cpu, 0xdeu));
    if (cpu->negative)
        return TargetCancel(battle, return_pc);
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 15u);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, 0x97b5aau));
    if (cpu->zero)
        return TARGET_REDRAW;
    OpBitValue(cpu, 1u);
    if (cpu->zero) {
        OpSta(memory, cpu, OpDp(cpu, TARGET_DP_MOVE_DELTA));
        TargetMoveVertical(memory, cpu);
    } else {
        OpSta(memory, cpu, OpDp(cpu, TARGET_DP_MOVE_DELTA));
        TargetMoveAcross(memory, cpu);
    }
    return TARGET_REDRAW;
}

Lufia2ExecutionResult Lufia2BattleChooseTargets(const Lufia2Memory *memory,
                                                Lufia2CpuState *cpu,
                                                Lufia2PushedChildCall child,
                                                void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);
    uint32_t return_pc = 0u;

    PushAccumulator8(memory, cpu);
    if (!BattleCall(&battle, 0xd4e1u, 0x85ec81u, 3u))
        return BattleChildUnwound(&battle);
    LoadA8(cpu, Pull8(memory, cpu));
    OpPushX(memory, cpu);
    OpSta(memory, cpu, OpDp(cpu, TARGET_DP_MODE));
    PushDataBank(memory, cpu);
    if (!TargetBuildSelectionRecords(&battle))
        return BattleChildUnwound(&battle);
    OpLda(memory, cpu, OpDp(cpu, TARGET_DP_MODE));
    OpAndValue(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, TARGET_DP_SIDE_OR_MASK));
    OpStz(memory, cpu, OpDp(cpu, TARGET_DP_CURSOR));
    TargetLoadSide(memory, cpu);
    if (cpu->negative)
        TargetFindFirstEnemy(memory, cpu);
    OpStz(memory, cpu, OpDp(cpu, TARGET_DP_BLINK_PHASE));
    OpLda(memory, cpu, OpAbs(cpu, 0x129eu));
    if (!BattleCall(&battle, 0xd581u, 0x81bebcu, 3u))
        return BattleChildUnwound(&battle);
    OpSetDataBank(memory, cpu, 0x7eu);
    for (;;) {
        TargetOutcome outcome;

        if (!TargetDrawSelection(&battle))
            return BattleChildUnwound(&battle);
        if (!BattleCall(&battle, 0xd635u, 0x81d9d0u, 2u))
            return BattleChildUnwound(&battle);
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, 0x0012f3u);
        OpRepWidths(cpu, 0x20u);
        if (!BattleCall(&battle, 0xd640u, 0x859cc0u, 3u) ||
            !BattleCall(&battle, 0xd644u, 0x859c64u, 3u))
            return BattleChildUnwound(&battle);
        OpSepWidths(cpu, 0x20u);
        if (!BattleCall(&battle, 0xd64au, 0x85ec81u, 3u))
            return BattleChildUnwound(&battle);
        outcome = TargetHandleInput(&battle, &return_pc);
        if (outcome == TARGET_UNWOUND)
            return BattleChildUnwound(&battle);
        if (outcome == TARGET_DONE)
            break;
    }
    PullDataBank(memory, cpu);
    OpPullX(memory, cpu);
    return ExecutionReturned(return_pc);
}
