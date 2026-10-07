#include "battle/battle_internal.h"

/* $81:E4D1: five-character name above a party window. */
Lufia2ExecutionResult Lufia2BattlePartyName(const Lufia2Memory *memory,
                                            Lufia2CpuState *cpu,
                                            Lufia2PushedChildCall child,
                                            void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);

    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, 0x818811u));
    OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_PARTY_RECORDS)));
    cpu->carry = 1;
    OpSbcValue(cpu, 0x40u);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpLoadA(cpu, 5u);
    OpSta(memory, cpu, OpDp(cpu, 0x12u));
    do {
        OpLda(memory, cpu, OpAbsY(cpu, 0u));
        if (!BattleCall(&battle, 0xe4edu, 0x81e835u, 2u))
            return BattleChildUnwound(&battle);
        OpBitValue(cpu, 0xffu);
        if (!cpu->zero) {
            cpu->carry = 1;
            OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0x11u)));
            OpSta(memory, cpu, OpAbsX(cpu, 0u));
        }
        OpInx(cpu);
        OpInx(cpu);
        OpIny(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, 0x12u), -1);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81e502u);
}

static Lufia2ExecutionResult QueueBattleDisplayBlock(BattleContext *battle, uint16_t site,
                                                uint16_t source, uint16_t destination,
                                                uint16_t size, uint32_t exit) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;

    if (!BattleCall(battle, site, 0x85ecdbu, 3u))
        return BattleChildUnwound(battle);
    OpLoadA(cpu, source);
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_VRAM_QUEUE + 2u));
    OpLoadA(cpu, destination);
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_VRAM_QUEUE + 4u));
    OpLoadA(cpu, size);
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_VRAM_QUEUE));
    return ExecutionReturned(exit);
}

/* $85:9CD7: queue the action-window tilemap. */
Lufia2ExecutionResult Lufia2BattleQueueActionWindow(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu,
                                                    Lufia2PushedChildCall child,
                                                    void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x85u);

    return QueueBattleDisplayBlock(&battle, 0x9cd7u, 0x2800u, 0x5c00u, 0x0400u, 0x859cedu);
}

/* $85:9CEE: queue the command-list tilemap. */
Lufia2ExecutionResult Lufia2BattleQueueListWindow(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu,
                                                  Lufia2PushedChildCall child,
                                                  void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x85u);

    return QueueBattleDisplayBlock(&battle, 0x9ceeu, 0x3000u, 0x0800u, 0x0600u, 0x859d04u);
}

Lufia2ExecutionResult Lufia2BattleQueueBackgroundTilemap(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859af4u);
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, context, 0x85u);
    return QueueBattleDisplayBlock(&battle, 0x9af4u,
        0x2000u, 0x0000u, 0x0800u, 0x859b0au);
}

Lufia2ExecutionResult Lufia2BattleQueueBackgroundTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859b0bu);
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, context, 0x85u);
    return QueueBattleDisplayBlock(&battle, 0x9b0bu,
        0xa000u, 0x5000u, 0x1000u, 0x859b21u);
}

Lufia2ExecutionResult Lufia2BattleQueueExtraBackgroundTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859b22u);
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, context, 0x85u);
    return QueueBattleDisplayBlock(&battle, 0x9b22u,
        0xa800u, 0x5400u, 0x0800u, 0x859b38u);
}

Lufia2ExecutionResult Lufia2BattleQueueSpriteTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859b39u);
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, context, 0x85u);
    return QueueBattleDisplayBlock(&battle, 0x9b39u,
        0x8000u, 0x6000u, 0x2000u, 0x859b4fu);
}

Lufia2ExecutionResult Lufia2BattleQueueSecondarySpriteTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859b50u);
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, context, 0x85u);
    return QueueBattleDisplayBlock(&battle, 0x9b50u,
        0x6000u, 0x7000u, 0x2000u, 0x859b66u);
}

Lufia2ExecutionResult Lufia2BattleQueueWindowGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859b67u);
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, context, 0x85u);
    return QueueBattleDisplayBlock(&battle, 0x9b67u,
        0x2800u, 0x5c00u, 0x0800u, 0x859b7du);
}

Lufia2ExecutionResult Lufia2BattleQueueWindowTilemapHalf(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859b7eu);
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, context, 0x85u);
    return QueueBattleDisplayBlock(&battle, 0x9b7eu,
        0x3000u, 0x0800u, 0x0400u, 0x859b94u);
}

Lufia2ExecutionResult Lufia2BattleQueueAuxiliaryTilemap(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859b95u);
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, context, 0x85u);
    return QueueBattleDisplayBlock(&battle, 0x9b95u,
        0x3400u, 0x0c00u, 0x0400u, 0x859babu);
}

Lufia2ExecutionResult Lufia2BattleQueueOverlayTilemap(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859bacu);
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, context, 0x85u);
    return QueueBattleDisplayBlock(&battle, 0x9bacu,
        0x3800u, 0x1800u, 0x0400u, 0x859bc2u);
}

Lufia2ExecutionResult Lufia2BattleQueueWindowTilemaps(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859bc3u);
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, context, 0x85u);
    return QueueBattleDisplayBlock(&battle, 0x9bc3u,
        0x3000u, 0x0800u, 0x1000u, 0x859bd9u);
}

Lufia2ExecutionResult Lufia2BattleQueueShortWindowGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859bf1u);
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, context, 0x85u);
    return QueueBattleDisplayBlock(&battle, 0x9bf1u,
        0x2800u, 0x5c00u, 0x0700u, 0x859c07u);
}

Lufia2ExecutionResult Lufia2BattleQueueWindowTilemap(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859c08u);
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, context, 0x85u);
    return QueueBattleDisplayBlock(&battle, 0x9c08u,
        0x3000u, 0x0800u, 0x0800u, 0x859c1eu);
}

Lufia2ExecutionResult Lufia2BattleQueueTilesAt3000(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859addu);
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, context, 0x85u);
    return QueueBattleDisplayBlock(&battle, 0x9addu,
        0xc000u, 0x3000u, 0x2000u, 0x859af3u);
}

Lufia2ExecutionResult Lufia2BattleQueueTilesAt4000(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859c1fu);
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, context, 0x85u);
    return QueueBattleDisplayBlock(&battle, 0x9c1fu,
        0xa000u, 0x4000u, 0x1000u, 0x859c35u);
}

Lufia2ExecutionResult Lufia2BattleQueueTilesAt4800(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859c36u);
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, context, 0x85u);
    return QueueBattleDisplayBlock(&battle, 0x9c36u,
        0xb000u, 0x4800u, 0x1000u, 0x859c4cu);
}

Lufia2ExecutionResult Lufia2BattleQueueTilesAt1800(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859c4du);
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, context, 0x85u);
    return QueueBattleDisplayBlock(&battle, 0x9c4du,
        0xdf00u, 0x1800u, 0x1000u, 0x859c63u);
}

Lufia2ExecutionResult Lufia2BattleQueueTilePatchAt4400(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859c64u);
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, context, 0x85u);
    return QueueBattleDisplayBlock(&battle, 0x9c64u,
        0x8c00u, 0x4400u, 0x0400u, 0x859c7au);
}

Lufia2ExecutionResult Lufia2BattleQueueShortAuxiliaryTilemap(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859c7bu);
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, context, 0x85u);
    return QueueBattleDisplayBlock(&battle, 0x9c7bu,
        0x3800u, 0x0c00u, 0x00c0u, 0x859c91u);
}

Lufia2ExecutionResult Lufia2BattleQueueAlternateTilesAt4000(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859c92u);
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, context, 0x85u);
    return QueueBattleDisplayBlock(&battle, 0x9c92u,
        0xdf00u, 0x4000u, 0x0800u, 0x859ca8u);
}

Lufia2ExecutionResult Lufia2BattleQueueWindowTilemapPair(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859ca9u);
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, context, 0x85u);
    return QueueBattleDisplayBlock(&battle, 0x9ca9u,
        0x3070u, 0x0838u, 0x0004u, 0x859cbfu);
}

Lufia2ExecutionResult Lufia2BattleQueueMinimalAuxiliaryTilemap(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859cc0u);
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, context, 0x85u);
    return QueueBattleDisplayBlock(&battle, 0x9cc0u,
        0x3800u, 0x0c00u, 0x0080u, 0x859cd6u);
}

Lufia2ExecutionResult Lufia2BattleQueueSmallSpriteTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859d05u);
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, context, 0x85u);
    return QueueBattleDisplayBlock(&battle, 0x9d05u,
        0x8000u, 0x6000u, 0x0800u, 0x859d1bu);
}

Lufia2ExecutionResult Lufia2BattleQueueAlternateSpriteTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859d1cu);
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, context, 0x85u);
    return QueueBattleDisplayBlock(&battle, 0x9d1cu,
        0xdf00u, 0x6000u, 0x0800u, 0x859d32u);
}

Lufia2ExecutionResult Lufia2BattleQueueMidSpriteTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859d33u);
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, context, 0x85u);
    return QueueBattleDisplayBlock(&battle, 0x9d33u,
        0x6000u, 0x7000u, 0x1000u, 0x859d49u);
}

Lufia2ExecutionResult Lufia2BattleQueueUpperSpriteTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859d4au);
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, context, 0x85u);
    return QueueBattleDisplayBlock(&battle, 0x9d4au,
        0x7000u, 0x7800u, 0x1000u, 0x859d60u);
}

Lufia2ExecutionResult Lufia2BattleQueueTilesAt2000(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859d61u);
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, context, 0x85u);
    return QueueBattleDisplayBlock(&battle, 0x9d61u,
        0xc000u, 0x2000u, 0x0ac0u, 0x859d77u);
}

Lufia2ExecutionResult Lufia2BattleQueueTilemapAt0E00(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859d78u);
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, context, 0x85u);
    return QueueBattleDisplayBlock(&battle, 0x9d78u,
        0xee00u, 0x0e00u, 0x01c0u, 0x859d8eu);
}

Lufia2ExecutionResult Lufia2BattleQueueSmallTilesAt4000(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859d8fu);
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, context, 0x85u);
    return QueueBattleDisplayBlock(&battle, 0x9d8fu,
        0x6000u, 0x4000u, 0x0800u, 0x859da5u);
}

Lufia2ExecutionResult Lufia2BattleQueueTilesAt4400(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859da6u);
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, context, 0x85u);
    return QueueBattleDisplayBlock(&battle, 0x9da6u,
        0x6800u, 0x4400u, 0x0c00u, 0x859dbcu);
}

Lufia2ExecutionResult Lufia2BattleQueueTilesAt4A00(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859dbdu);
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, context, 0x85u);
    return QueueBattleDisplayBlock(&battle, 0x9dbdu,
        0x7400u, 0x4a00u, 0x0c00u, 0x859dd3u);
}
