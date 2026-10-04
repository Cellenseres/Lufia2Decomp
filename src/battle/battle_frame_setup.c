/* Battle frame setup: the full variant of the battle sprite pass, which first
 * clears the party tilemap through the work RAM port and reloads the color
 * tables, and the row writer of the tile grid. */

#include "battle/battle_internal.h"
#include "lufia2/battle.h"

enum {
    WMDATA = 0x2180u,
    WMADDL = 0x2181u,
    WMADDH = 0x2183u,
    CLEAR_WORDS = 0x02a0u,
    MODE = 0x15abu,
    MODE_OVERRIDE = 0x11deu,
    TILE_GRID_HOLD = 0x125fu,
    PARTY_SPRITES = 0x15d3u,
    COLOR_BANK = 0x97u,
    COLOR_SOURCE = 0xccf8u,
    COLOR_COUNT = 0x0040u,
    COLOR_TARGET = 0xf35bu,
    COLOR_TARGET_PARTY = 0xf3bbu,
    COLOR_PARTY_COUNT = 0x0020u,
    COLOR_BUFFER = 0x4800u,
    COLOR_ROWS = 0x4820u,
    COLOR_ROW_COUNT = 0x0060u,
    BANK_TARGET = 0x7fu,
    BANK_WORK = 0x7eu
};

static Lufia2ExecutionResult BattleColorEntry(Lufia2CpuState *cpu, uint32_t entry,
    uint32_t exit) {
    Lufia2ExecutionResult result = ExecutionReturned(exit);

    result.dispatches = 0;
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit) {
        result.flow = LUFIA2_EXECUTION_BOUNDARY;
        result.pc = cpu->resume_pc = entry;
    }
    return result;
}

/* STX $2181 and the bank byte, which select the work RAM port address. */
static void SetWramPortAddress(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t data_bank, uint16_t address, uint8_t bank) {
    const uint32_t base = (uint32_t)data_bank << 16;

    LoadX16(cpu, address);
    Write8(memory, base | WMADDL, (uint8_t)cpu->x);
    Write8(memory, (base | WMADDL) + 1u, (uint8_t)(cpu->x >> 8));
    LoadA8(cpu, bank);
    Write8(memory, base | WMADDH, A8(cpu));
}

/* Streams bytes at bank:source to the port, X counting up. */
static void StreamColorBytes(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint32_t source, uint32_t port, uint16_t count) {
    LoadX16(cpu, 0);
    do {
        LoadA8(cpu, Read8(memory, (source + cpu->x) & 0x00ffffffu));
        Write8(memory, port, A8(cpu));
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, count);
    } while (!cpu->zero);
}

/* The work RAM view of the current data bank for a byte store. */
static void StoreWork(const Lufia2Memory *memory, const Lufia2CpuState *cpu,
    uint16_t address) {
    Write8(memory, ((uint32_t)cpu->data_bank << 16) | address, A8(cpu));
}

/* $85:8AAF: the color tables, 64 bytes from ROM and 32 from the buffer, are
 * streamed to the work RAM port; the counters are set. M1X0, JSL. */
Lufia2ExecutionResult Lufia2BattleColorsInit(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result = BattleColorEntry(cpu, 0x858aafu, 0x858af3u);

    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    PushDataBank(memory, cpu);
    SetWramPortAddress(memory, cpu, cpu->data_bank, COLOR_TARGET, BANK_TARGET);
    LoadA8(cpu, COLOR_BANK);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    StreamColorBytes(memory, cpu, ((uint32_t)COLOR_BANK << 16) | COLOR_SOURCE,
        ((uint32_t)COLOR_BANK << 16) | WMDATA, COLOR_COUNT);
    LoadA8(cpu, 0x0cu);
    StoreWork(memory, cpu, 0x15b5u);
    LoadX16(cpu, COLOR_TARGET_PARTY);
    Write8(memory, ((uint32_t)cpu->data_bank << 16) | WMADDL, (uint8_t)cpu->x);
    Write8(memory, ((uint32_t)cpu->data_bank << 16) | (WMADDL + 1u),
        (uint8_t)(cpu->x >> 8));
    LoadA8(cpu, BANK_WORK);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    StreamColorBytes(memory, cpu, ((uint32_t)BANK_WORK << 16) | COLOR_BUFFER,
        WMDATA, COLOR_PARTY_COUNT);
    LoadA8(cpu, 0x07u);
    StoreWork(memory, cpu, 0x15b6u);
    PullDataBank(memory, cpu);
    return result;
}

/* The bank $85 variants stream 96 bytes of the buffer to a color table. */
static void RowsToTable(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t target) {
    SetWramPortAddress(memory, cpu, cpu->data_bank, target, BANK_TARGET);
    LoadA8(cpu, BANK_WORK);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    StreamColorBytes(memory, cpu, ((uint32_t)BANK_WORK << 16) | COLOR_ROWS, WMDATA,
        COLOR_ROW_COUNT);
}

/* $85:8AF4: the party rows to the table at $7F:F2FB, with $15D3 = $FF. */
Lufia2ExecutionResult Lufia2BattleColorsParty(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result = BattleColorEntry(cpu, 0x858af4u, 0x858b21u);

    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    PushDataBank(memory, cpu);
    Push8(memory, cpu, 0x85u);
    PullDataBank(memory, cpu);
    LoadA8(cpu, 0x09u);
    StoreWork(memory, cpu, 0x15b7u);
    LoadX16(cpu, 0xf2fbu);
    LoadA8(cpu, 0xffu);
    StoreWork(memory, cpu, PARTY_SPRITES);
    Write8(memory, ((uint32_t)cpu->data_bank << 16) | WMADDL, (uint8_t)cpu->x);
    Write8(memory, ((uint32_t)cpu->data_bank << 16) | (WMADDL + 1u),
        (uint8_t)(cpu->x >> 8));
    LoadA8(cpu, BANK_TARGET);
    StoreWork(memory, cpu, WMADDH);
    LoadA8(cpu, BANK_WORK);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    StreamColorBytes(memory, cpu, ((uint32_t)BANK_WORK << 16) | COLOR_ROWS, WMDATA,
        COLOR_ROW_COUNT);
    PullDataBank(memory, cpu);
    return result;
}

/* $85:8B22: the monster rows to the table at $7F:F27B, with $15B7 = 5. */
Lufia2ExecutionResult Lufia2BattleColorsMonster(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result = BattleColorEntry(cpu, 0x858b22u, 0x858b4au);

    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    PushDataBank(memory, cpu);
    Push8(memory, cpu, 0x85u);
    PullDataBank(memory, cpu);
    RowsToTable(memory, cpu, 0xf27bu);
    LoadA8(cpu, 0x05u);
    StoreWork(memory, cpu, 0x15b7u);
    PullDataBank(memory, cpu);
    return result;
}

/* $85:9790: A counts up through 16 words at X (data bank), M0X0. */
Lufia2ExecutionResult Lufia2BattleTileRow(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    unsigned column;

    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859790u);
    for (column = 0; column < 16u; ++column) {
        Write16Long(memory, AbsoluteIndexedAddress(
            cpu, (uint16_t)(2u * column), cpu->x), cpu->accumulator);
        IncrementA16(cpu);
    }
    return ExecutionReturned(0x8597d0u);
}

/* One JSL into bank $85; false when it handed off (frame stays pushed). */
static int CallBattle(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t return_address,
    Lufia2ExecutionResult (*routine)(const Lufia2Memory *, Lufia2CpuState *),
    Lufia2ExecutionResult *result) {
    SimulateJslFrame(memory, cpu, 0x85u, return_address);
    *result = routine(memory, cpu);
    if (result->flow != LUFIA2_EXECUTION_RETURNED)
        return 0;
    SimulateRtlFrame(memory, cpu);
    return 1;
}

/* The setup clears the tilemap before it calls any renderer. Check their
 * output spans now, so unsupported entries resume before that first clear. */
static bool BattleFrameSetupInputsFit(
    const Lufia2Memory *memory, const Lufia2CpuState *cpu) {
    const uint8_t bank = cpu->data_bank;
    const uint32_t base = (uint32_t)bank << 16;
    bool active;
    uint8_t mode;
    uint32_t cursor;
    uint32_t bytes;

    if (cpu->decimal || cpu->direct_page != 0u || cpu->stack < 0x1f10u ||
        cpu->stack > 0x1ffcu || !(bank < 0x40u || (bank >= 0x80u && bank < 0xc0u)))
        return false;
    active = Read8(memory, base | MODE_OVERRIDE) != 0;
    mode = active ? 1u : Read8(memory, base | MODE);
    if (mode != 1u && mode != 2u)
        return true;
    cursor = Read16Long(memory, 0x15c8u);
    bytes = 20u * Read8(memory, 0x153cu);
    if (bytes != 0u && (cursor < 0x2000u || cursor + bytes > 0x10000u))
        return false;
    cursor = Read16Long(memory, 0x15ccu);
    bytes = (Read8(memory, 0x13ceu) & 0x80u) != 0u ? 45u : 0u;
    if (bytes != 0u && (cursor < 0x2000u || cursor + bytes > 0x10000u))
        return false;
    return active || BattlePartyRenderInputsFit(memory, mode == 2u);
}

/* $85:8A39: the battle sprite pass with its setup. */
Lufia2ExecutionResult Lufia2BattleFrameSetup(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result = BattleColorEntry(cpu, 0x858a39u, 0x858a95u);
    const uint32_t base = (uint32_t)cpu->data_bank << 16;
    uint16_t count;

    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    if (!BattleFrameSetupInputsFit(memory, cpu))
        return ExecutionHandoff(cpu, 0x858a39u);
    LoadAAbsolute8(memory, cpu, MODE_OVERRIDE, 0);
    if (!cpu->zero) {
        LoadA8(cpu, 0x01u);
        StoreAAbsolute8(memory, cpu, MODE, 0);
    }
    LoadX16(cpu, 0x2800u);
    LoadY16(cpu, CLEAR_WORDS);
    Write8(memory, base | WMADDL, (uint8_t)cpu->x);
    Write8(memory, (base | WMADDL) + 1u, (uint8_t)(cpu->x >> 8));
    Write8(memory, base | WMADDH, 0);
    LoadA8(cpu, 0x01u);
    for (count = CLEAR_WORDS; count != 0; --count) {
        Write8(memory, base | WMDATA, 0);
        Write8(memory, base | WMDATA, A8(cpu));
    }
    cpu->y = 0;
    SetNz16(cpu, 0);
    LoadAAbsolute8(memory, cpu, MODE, 0);
    DecrementA8(cpu);
    if (cpu->zero) {
        if (!CallBattle(memory, cpu, 0x8a67u, Lufia2BattleColorsInit, &result) ||
            !CallBattle(memory, cpu, 0x8a6bu, Lufia2BattleColorsMonster, &result) ||
            !CallBattle(memory, cpu, 0x8a6fu, Lufia2BattleSpriteRecordsEntry, &result) ||
            !CallBattle(memory, cpu, 0x8a73u, Lufia2BattleSpriteSingleEntry, &result) ||
            !CallBattle(memory, cpu, 0x8a77u, Lufia2BattleSpriteMarkersEntry, &result))
            return result;
        LoadAAbsolute8(memory, cpu, MODE_OVERRIDE, 0);
        if (cpu->zero) {
            if (!CallBattle(memory, cpu, 0x8a80u, Lufia2BattlePartyTilemapEntry,
                    &result))
                return result;
            return ExecutionReturned(0x858a95u);
        }
        LoadAAbsolute8(memory, cpu, TILE_GRID_HOLD, 0);
        if (!cpu->zero)
            return ExecutionReturned(0x858a95u);
        StoreZeroAbsolute8(memory, cpu, PARTY_SPRITES, 0);
        PushDataBank(memory, cpu);
        LoadA8(cpu, 0x7eu);
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        if (!CallBattle(memory, cpu, 0x8a93u, Lufia2BattleTileGridEntry, &result))
            return result;
        PullDataBank(memory, cpu);
        return ExecutionReturned(0x858a95u);
    }
    DecrementA8(cpu);
    if (!cpu->zero)
        return ExecutionReturned(0x858a63u);
    if (!CallBattle(memory, cpu, 0x8a99u, Lufia2BattleColorsInit, &result) ||
        !CallBattle(memory, cpu, 0x8a9du, Lufia2BattleColorsParty, &result) ||
        !CallBattle(memory, cpu, 0x8aa1u, Lufia2BattleSpriteRecordsEntry, &result) ||
        !CallBattle(memory, cpu, 0x8aa5u, Lufia2BattleSpriteSingleEntry, &result) ||
        !CallBattle(memory, cpu, 0x8aa9u, Lufia2BattleSpriteMarkersEntry, &result) ||
        !CallBattle(memory, cpu, 0x8aadu, Lufia2BattleSpritePartyEntry, &result))
        return result;
    return ExecutionReturned(0x858aaeu);
}
