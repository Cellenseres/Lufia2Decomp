#include "battle/battle_internal.h"
#include "lufia2/battle_command_tiles.h"
#include "lufia2/battle_command_steps.h"

static void CopyCommandTileParameters(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, bool highlighted) {
    static const uint8_t tile_parameters[] = {0u, 8u, 9u};
    for (unsigned field = 0u; field < sizeof tile_parameters; ++field) {
        OpLda(memory, cpu, OpLongX(cpu, 0x97b55eu + field));
        if (highlighted && field == 0u) {
            cpu->carry = true;
            OpSbcValue(cpu, 8u);
        }
        OpSta(memory, cpu, OpDp(cpu, tile_parameters[field]));
    }
}

static void PrepareCommandTile(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    CopyCommandTileParameters(memory, cpu, false);
    OpLdx(cpu, (uint16_t)(cpu->x + 3u));
    OpPushX(memory, cpu);
}

static void PrepareSelectedCommandTile(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSepWidths(cpu, 0x10u);
    OpLda(memory, cpu, OpDp(cpu, 0x47u));
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpAndValue(cpu, 3u);
    OpTay(cpu);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbsY(cpu, 0xb576u)));
    OpRepWidths(cpu, 0x10u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x26u), cpu->x);
    CopyCommandTileParameters(memory, cpu, true);
}

Lufia2ExecutionResult Lufia2BattlePrepareCommandTile(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x81cb99u);
    PrepareCommandTile(memory, cpu);
    return ExecutionHandoff(cpu, 0x81cbafu);
}

Lufia2ExecutionResult Lufia2BattlePrepareSelectedCommandTile(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x81cbb8u);
    PrepareSelectedCommandTile(memory, cpu);
    return ExecutionHandoff(cpu, 0x81cbddu);
}

static bool NextCommandTile(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpPullX(memory, cpu);
    OpCpx(cpu, 9u);
    return !cpu->zero;
}

Lufia2ExecutionResult Lufia2BattleContinueCommandTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x81u || cpu->accumulator_is_8_bit > 1u ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x81cbb2u);
    return ExecutionHandoff(cpu, NextCommandTile(memory, cpu) ? 0x81cb99u : 0x81cbb8u);
}

Lufia2ExecutionResult Lufia2BattleCommandCallStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry) {
    if (cpu->program_bank != 0x81u || cpu->accumulator_is_8_bit > 1u ||
        cpu->index_is_8_bit > 1u)
        return ExecutionHandoff(cpu, entry);
    uint32_t target;
    uint8_t frame_size;
    switch (entry) {
    case 0x81cbafu: case 0x81cbddu:
        target = BATTLE_ROUTINE_TILE_BLOCK;
        frame_size = 2u;
        break;
    case 0x81cbe0u:
        target = 0x859dd4u;
        frame_size = 3u;
        break;
    case 0x81cbe4u:
        target = 0x81d9d0u;
        frame_size = 2u;
        break;
    case 0x81cbedu:
        target = BATTLE_ROUTINE_FRAME_INPUT;
        frame_size = 3u;
        break;
    default:
        return ExecutionHandoff(cpu, entry);
    }
    if (frame_size == 3u) {
        SimulateJslFrame(memory, cpu, cpu->program_bank, (uint16_t)(entry + 3u));
        cpu->program_bank = (uint8_t)(target >> 16);
    } else {
        SimulateJsrFrame(memory, cpu, (uint16_t)(entry + 2u));
    }
    return ExecutionHandoff(cpu, target);
}

typedef enum CommandInputChoice {
    COMMAND_INPUT_REPEAT,
    COMMAND_INPUT_PRIMARY,
    COMMAND_INPUT_ALTERNATE,
} CommandInputChoice;

static bool PrepareCommandSpritePreview(const Lufia2Memory *memory,
                                        Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, 0x46u));
    OpAslA(cpu);
    OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0x47u)));
    OpAndValue(cpu, 0x20u);
    OpAndValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, 0x057cu)));
    if (cpu->zero)
        return false;
    OpLoadA(cpu, 0x1bu);
    return true;
}

static CommandInputChoice PrepareCommandInput(const Lufia2Memory *memory,
                                              Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, BATTLE_DP_PAD_FILTERED));
    OpBitValue(cpu, 0xa0u);
    if (!cpu->zero) {
        OpLoadA(cpu, 2u);
        return COMMAND_INPUT_PRIMARY;
    }
    OpBitValue(cpu, 0x40u);
    if (!cpu->zero) {
        OpLoadA(cpu, 2u);
        return COMMAND_INPUT_ALTERNATE;
    }
    return COMMAND_INPUT_REPEAT;
}

static void ReadCommandSelection(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                 bool alternate) {
    OpLda(memory, cpu, OpDp(cpu, 0x26u));
    OpDecA(cpu);
    OpAndValue(cpu, 3u);
    if (alternate)
        OpOraValue(cpu, 4u);
}

Lufia2ExecutionResult Lufia2BattleCommandInputStep(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   uint32_t entry) {
    if (cpu->program_bank != 0x81u || cpu->accumulator_is_8_bit != 1u ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, entry);
    switch (entry) {
    case 0x81cbf1u:
        return ExecutionHandoff(
            cpu, PrepareCommandSpritePreview(memory, cpu) ? 0x81cbffu : 0x81cc07u);
    case 0x81cc07u: {
        const CommandInputChoice choice = PrepareCommandInput(memory, cpu);
        return ExecutionHandoff(cpu, choice == COMMAND_INPUT_PRIMARY     ? 0x81cc16u
                                     : choice == COMMAND_INPUT_ALTERNATE ? 0x81cc22u
                                                                         : 0x81cb96u);
    }
    case 0x81cc1au:
    case 0x81cc26u:
        ReadCommandSelection(memory, cpu, entry == 0x81cc26u);
        return ExecutionHandoff(cpu, entry == 0x81cc1au ? 0x81cc1fu : 0x81cc2du);
    default:
        return ExecutionHandoff(cpu, entry);
    }
}

/* $81:CB77: draw and poll the four-way Battle command selection. */
Lufia2ExecutionResult Lufia2BattleChooseCommand(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    BattleContext battle = BattleContextCreate(memory, cpu, child, child_context, 0x81u);
    if (!BattleCall(&battle, 0xcb77u, BATTLE_ROUTINE_PARTY_WINDOWS, 2u))
        return BattleChildUnwound(&battle);
    OpLoadA(cpu, 0x97u);
    OpSta(memory, cpu, OpDp(cpu, 0x24u));
    OpLdy(cpu, 0xfde6u);
    OpLoadA(cpu, 0u);
    if (!BattleCall(&battle, 0xcb83u, BATTLE_ROUTINE_LOAD_PALETTE, 2u) ||
        !BattleCall(&battle, 0xcb86u, BATTLE_ROUTINE_COMMIT_PALETTES, 3u))
        return BattleChildUnwound(&battle);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpDp(cpu, 2u));
    OpSta(memory, cpu, OpDp(cpu, 3u));
    OpSta(memory, cpu, OpDp(cpu, 1u));
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpDp(cpu, 4u));
    for (;;) {
        OpLdx(cpu, 0u);
        do {
            PrepareCommandTile(memory, cpu);
            if (!BattleCall(&battle, 0xcbafu, BATTLE_ROUTINE_TILE_BLOCK, 2u))
                return BattleChildUnwound(&battle);
        } while (NextCommandTile(memory, cpu));
        PrepareSelectedCommandTile(memory, cpu);
        if (!BattleCall(&battle, 0xcbddu, BATTLE_ROUTINE_TILE_BLOCK, 2u) ||
            !BattleCall(&battle, 0xcbe0u, 0x859dd4u, 3u) ||
            !BattleCall(&battle, 0xcbe4u, 0x81d9d0u, 2u))
            return BattleChildUnwound(&battle);
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, BATTLE_SPRITE_REBUILD_REQUEST);
        if (!BattleCall(&battle, 0xcbedu, BATTLE_ROUTINE_FRAME_INPUT, 3u))
            return BattleChildUnwound(&battle);
        if (PrepareCommandSpritePreview(memory, cpu)) {
            if (!BattleCall(&battle, 0xcbffu, 0x80953bu, 3u) ||
                !BattleCall(&battle, 0xcc03u, 0x849b3eu, 3u))
                return BattleChildUnwound(&battle);
        }
        const CommandInputChoice choice = PrepareCommandInput(memory, cpu);
        if (choice == COMMAND_INPUT_PRIMARY) {
            if (!BattleCall(&battle, 0xcc16u, 0x80953bu, 3u))
                return BattleChildUnwound(&battle);
            ReadCommandSelection(memory, cpu, false);
            return ExecutionReturned(0x81cc1fu);
        }
        if (choice == COMMAND_INPUT_ALTERNATE) {
            if (!BattleCall(&battle, 0xcc22u, 0x80953bu, 3u))
                return BattleChildUnwound(&battle);
            ReadCommandSelection(memory, cpu, true);
            return ExecutionReturned(0x81cc2du);
        }
    }
}
