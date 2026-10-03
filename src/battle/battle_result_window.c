#include "battle/battle_internal.h"

/* $81:DD7F: prepare the battle-result window. */
Lufia2ExecutionResult Lufia2BattleResultWindowPrepare(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu,
                                                      Lufia2PushedChildCall child,
                                                      void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);

    if (!BattleCall(&battle, 0xdd7fu, 0x85ec81u, 3u))
        return BattleChildUnwound(&battle);
    OpStz(memory, cpu, OpDp(cpu, 0x1eu));
    OpLdx(cpu, 0x151fu);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x1245u), cpu->x);
    OpLdx(cpu, 0x4202u);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x1249u), cpu->x);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, 0x15b3u));
    if (!BattleCall(&battle, 0xdd96u, 0x81c2fbu, 2u))
        return BattleChildUnwound(&battle);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpLoadA(cpu, 11u);
    OpSta(memory, cpu, OpAbs(cpu, 0x09f3u));
    OpLoadA(cpu, 26u);
    OpSta(memory, cpu, OpAbs(cpu, 0x09f2u));
    OpLdx(cpu, 0x2844u);
    if (!BattleCall(&battle, 0xdda9u, 0x81e3aeu, 3u))
        return BattleChildUnwound(&battle);
    PullDataBank(memory, cpu);
    OpLdx(cpu, 0x3108u);
    OpWriteX(memory, cpu, OpAbs(cpu, BATTLE_RESULT_WINDOW_BASE), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, BATTLE_RESULT_WRITE_POSITION), cpu->x);
    OpLoadA(cpu, 3u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_RESULT_WINDOW));
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_RESULT_WINDOW_Y));
    OpLoadA(cpu, 26u);
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_RESULT_WINDOW_WIDTH));
    OpLoadA(cpu, 6u);
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_RESULT_WINDOW_HEIGHT));
    OpStz(memory, cpu, OpAbs(cpu, BATTLE_RESULT_COLUMN));
    OpStz(memory, cpu, OpAbs(cpu, BATTLE_RESULT_ROW));
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_RESULT_TILE_ATTRIBUTES));
    if (!BattleCall(&battle, 0xddd6u, 0x85a972u, 3u))
        return BattleChildUnwound(&battle);
    OpRepWidths(cpu, 0x20u);
    if (!BattleCall(&battle, 0xdddcu, 0x859b67u, 3u))
        return BattleChildUnwound(&battle);
    OpSepWidths(cpu, 0x20u);
    if (!BattleCall(&battle, 0xdde2u, 0x85ec81u, 3u))
        return BattleChildUnwound(&battle);
    return ExecutionReturned(0x81dde6u);
}

/* Original ADC chain keeps carry between the row offset and window base. */
static void ResultWindowNextPosition(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, BATTLE_RESULT_ROW));
    ExchangeAccumulatorBytes(cpu);
    OpRepWidths(cpu, 0x20u);
    OpLsrA(cpu);
    OpSta(memory, cpu, OpDp(cpu, 0x63u));
    OpSepWidths(cpu, 0x20u);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpAbs(cpu, BATTLE_RESULT_COLUMN));
    OpAslA(cpu);
    OpRepWidths(cpu, 0x20u);
    OpAdc(memory, cpu, OpDp(cpu, 0x63u));
    OpAdc(memory, cpu, OpAbs(cpu, BATTLE_RESULT_WINDOW_BASE));
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_RESULT_WRITE_POSITION));
}

/* $81:DDE7: draw the next result line, scrolling when full. */
Lufia2ExecutionResult Lufia2BattleResultWindowLine(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);

    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpWriteX(memory, cpu, OpDp(cpu, 0x5du), cpu->y);
    OpLda(memory, cpu, OpAbs(cpu, BATTLE_RESULT_ROW));
    OpCmp(memory, cpu, OpAbs(cpu, BATTLE_RESULT_WINDOW_HEIGHT));
    if (cpu->carry && !cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, BATTLE_RESULT_TILE_ATTRIBUTES));
        OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
        OpLoadA(cpu, 0x85u);
        OpSta(memory, cpu, OpDp(cpu, 0x5fu));
        OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, BATTLE_RESULT_WRITE_POSITION)));
        OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x5du)));
        if (!BattleCall(&battle, 0xde07u, 0x808878u, 3u) ||
            !BattleCall(&battle, 0xde0bu, 0x81de55u, 2u))
            return BattleChildUnwound(&battle);
    } else {
        OpLda(memory, cpu, OpAbs(cpu, BATTLE_RESULT_TILE_ATTRIBUTES));
        OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
        OpLoadA(cpu, 0x85u);
        OpSta(memory, cpu, OpDp(cpu, 0x5fu));
        OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, BATTLE_RESULT_WRITE_POSITION)));
        OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x5du)));
        if (!BattleCall(&battle, 0xde1fu, 0x808878u, 3u))
            return BattleChildUnwound(&battle);
    }
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, 0x7e3213u);
    OpStz(memory, cpu, OpAbs(cpu, BATTLE_RESULT_COLUMN));
    OpStepMem(memory, cpu, OpAbs(cpu, BATTLE_RESULT_ROW), 1);
    ResultWindowNextPosition(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x20u);
    if (!BattleCall(&battle, 0xde4du, 0x859c08u, 3u))
        return BattleChildUnwound(&battle);
    OpSepWidths(cpu, 0x20u);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81de54u);
}

/* $81:DE55: move result rows up and wait three frames. */
Lufia2ExecutionResult Lufia2BattleResultWindowScroll(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu,
                                                     Lufia2PushedChildCall child,
                                                     void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);

    TransferDirectToA(cpu);
    OpStepMem(memory, cpu, OpAbs(cpu, BATTLE_RESULT_ROW), -1);
    OpStz(memory, cpu, OpAbs(cpu, BATTLE_RESULT_COLUMN));
    ResultWindowNextPosition(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLdy(cpu, 0x3000u);
    OpLdx(cpu, 0x3080u);
    OpLoadA(cpu, 0x077fu);
    OpMoveNext(memory, cpu, 0x7eu, 0x7eu);
    if (!BattleCall(&battle, 0xde84u, 0x859c08u, 3u))
        return BattleChildUnwound(&battle);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0xfdu);
    for (;;) {
        OpStz(memory, cpu, OpAbs(cpu, 0x1b23u));
        OpSta(memory, cpu, OpAbs(cpu, 0x1b22u));
        if (cpu->zero)
            break;
        PushAccumulator8(memory, cpu);
        if (!BattleCall(&battle, 0xde8fu, 0x85ec81u, 3u))
            return BattleChildUnwound(&battle);
        LoadA8(cpu, Pull8(memory, cpu));
        OpIncA(cpu);
    }
    return ExecutionReturned(0x81de9du);
}

/* $81:DE9E: wait for result confirmation. */
Lufia2ExecutionResult Lufia2BattleResultWindowWait(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);

    do {
        if (!BattleCall(&battle, 0xde9eu, 0x85ec81u, 3u))
            return BattleChildUnwound(&battle);
        OpLda(memory, cpu, OpDp(cpu, BATTLE_DP_PAD_FILTERED));
        OpBitValue(cpu, 0xa0u);
    } while (cpu->zero);
    return ExecutionReturned(0x81dea8u);
}
