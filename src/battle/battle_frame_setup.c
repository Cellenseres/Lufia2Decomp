/* Battle frame setup: the full variant of the battle sprite pass, which first
 * clears the party tilemap through the work RAM port and reloads the color
 * tables, and the row writer of the tile grid. */

#include "battle/battle_internal.h"
#include "core/child_call.h"
#include "core/snes_registers.h"
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

typedef struct BattlePartyPassStep {
    uint16_t return_address;
    uint32_t target;
    Lufia2ExecutionResult (*run)(const Lufia2Memory *, Lufia2CpuState *);
} BattlePartyPassStep;

static Lufia2ExecutionResult RenderBattlePartyPass(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    static const BattlePartyPassStep steps[] = {
        {0x8a99u, 0x858aafu, Lufia2BattleColorsInit},
        {0x8a9du, 0x858af4u, Lufia2BattleColorsParty},
        {0x8aa1u, 0x858b4bu, Lufia2BattleSpriteRecordsEntry},
        {0x8aa5u, 0x858bc0u, Lufia2BattleSpriteSingleEntry},
        {0x8aa9u, 0x858c27u, Lufia2BattleSpriteMarkersEntry},
        {0x8aadu, 0x858c98u, Lufia2BattleSpritePartyEntry}
    };
    const uint8_t caller_bank = cpu->data_bank;
    for (unsigned i = 0u; i < sizeof(steps) / sizeof(steps[0]); ++i) {
        const BattlePartyPassStep *step = &steps[i];
        if (!child) {
            Lufia2ExecutionResult result;
            if (!CallBattle(memory, cpu, step->return_address, step->run, &result))
                return result;
            continue;
        }
        const uint32_t site = 0x850000u | (uint16_t)(step->return_address - 3u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                               site, step->target, 3u, 0x85u)) {
            const Lufia2ExecutionResult result = {
                LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
            return result;
        }
        if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit ||
            cpu->program_bank != 0x85u || cpu->data_bank != caller_bank ||
            cpu->direct_page || cpu->decimal)
            return ExecutionHandoff(cpu, site + 4u);
    }
    return ExecutionReturned(0x858aaeu);
}

Lufia2ExecutionResult Lufia2BattleRenderPartyPass(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal || !child)
        return ExecutionHandoff(cpu, 0x858a96u);
    return RenderBattlePartyPass(memory, cpu, child, context);
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
    return RenderBattlePartyPass(memory, cpu, NULL, NULL);
}

enum {
    ROM_PREVIEW_SPRITES = 0x849e4du,
    PREVIEW_GRAPHICS_ID = 0x09f2u,
    PREVIEW_RECORD = 0x161bu,
    DP_PREVIEW_WIDTH_PIXELS = 0xcau,
    PREVIEW_TILEMAP_START = 0x2800u,
    PREVIEW_TILEMAP_WORDS = 0x0200u
};

typedef struct PreviewBuild {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
} PreviewBuild;

static uint8_t CallPreviewStep(PreviewBuild *preview, uint32_t site,
                               uint32_t target, uint8_t frame_size) {
    Lufia2CpuState *cpu = preview->cpu;
    const uint8_t data_bank = cpu->data_bank;
    const uint8_t accumulator_width = cpu->accumulator_is_8_bit;
    if (!CallChildWithFrame(preview->memory, cpu, preview->child,
                            preview->context, site, target, frame_size, 0x84u)) {
        const Lufia2ExecutionResult unwound = {
            LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        preview->result = unwound;
        return 0u;
    }
    if (cpu->program_bank != 0x84u || cpu->data_bank != data_bank ||
        cpu->accumulator_is_8_bit != accumulator_width || cpu->index_is_8_bit ||
        cpu->direct_page || cpu->decimal) {
        preview->result = ExecutionHandoff(cpu, site + frame_size + 1u);
        return 0u;
    }
    return 1u;
}

static void SelectPreviewSprite(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PREVIEW_SPRITE_SELECTION));
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, ROM_PREVIEW_SPRITES));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PREVIEW_SPRITE_ID));
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 10u);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_ENEMY_RECORDS));
        OpDex(cpu);
        OpDex(cpu);
    } while (!cpu->zero);
    OpLoadA(cpu, PREVIEW_RECORD);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ENEMY_RECORDS));
    OpSta(memory, cpu, OpAbs(cpu, DP_BATTLE_PREVIEW_RECORD_POINTER));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PREVIEW_SPRITE_ID));
    OpSta(memory, cpu, OpAbs(cpu, PREVIEW_GRAPHICS_ID));
}

static void PositionPreviewSprite(const Lufia2Memory *memory,
                                  Lufia2CpuState *cpu) {
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_ENEMY_ENCOUNTER_WIDTHS));
    OpAslA(cpu);
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_PREVIEW_WIDTH_PIXELS));
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_ENEMY_ENCOUNTER_HEIGHTS));
    OpLoadA(cpu, 0xa0u);
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_PREVIEW_WIDTH_PIXELS)));
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_ENEMY_SPRITE_X_POSITIONS));
    OpLoadA(cpu, 12u);
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu,
        OpAbsY(cpu, WRAM_BATTLE_ENEMY_ENCOUNTER_HEIGHTS)));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_ENEMY_GROUP_FLAGS));
    OpAslA(cpu);
    OpAslA(cpu);
    OpAslA(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_ENEMY_SPRITE_Y_POSITIONS));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_BATTLE_EFFECT_SPECIAL_PARTY));
    OpLda(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_ENEMY_ENCOUNTER_HEIGHTS));
    OpCmpValue(cpu, 16u);
    if (cpu->zero) {
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_EFFECT_SPECIAL_PARTY));
    }
}

static void FillPreviewPartyTilemap(const Lufia2Memory *memory,
                                    Lufia2CpuState *cpu) {
    OpLdx(cpu, PREVIEW_TILEMAP_START);
    OpLdy(cpu, PREVIEW_TILEMAP_WORDS);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_WMADDL), cpu->x);
    OpStz(memory, cpu, OpAbs(cpu, SNES_WMADDH));
    OpLoadA(cpu, 1u);
    do {
        OpStz(memory, cpu, OpAbs(cpu, SNES_WMDATA));
        OpSta(memory, cpu, OpAbs(cpu, SNES_WMDATA));
        OpDey(cpu);
    } while (!cpu->zero);
}

static uint8_t PublishPreviewGraphics(PreviewBuild *preview) {
    static const uint32_t uploads[][2] = {
        {0x849df9u, 0x859b67u},
        {0x849e05u, 0x859d33u},
        {0x849e11u, 0x859d4au}
    };
    Lufia2CpuState *cpu = preview->cpu;
    OpLoadA(cpu, 0xffu);
    OpSta(preview->memory, cpu, WRAM_BATTLE_EFFECT_FRAME_MARKER);
    if (!CallPreviewStep(preview, 0x849defu, 0x85ec81u, 3u) ||
        !CallPreviewStep(preview, 0x849df3u, 0x81b9afu, 3u))
        return 0u;
    for (unsigned i = 0u; i < sizeof(uploads) / sizeof(uploads[0]); ++i) {
        OpRepWidths(cpu, 0x20u);
        if (!CallPreviewStep(preview, uploads[i][0], uploads[i][1], 3u))
            return 0u;
        OpSepWidths(cpu, 0x20u);
        if (!CallPreviewStep(preview, uploads[i][0] + 6u, 0x85ec81u, 3u))
            return 0u;
    }
    return 1u;
}

Lufia2ExecutionResult Lufia2BattleBuildEnemyPreview(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x84u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal || !child)
        return ExecutionHandoff(cpu, 0x849d2cu);
    PreviewBuild preview = {memory, cpu, child, context, {0}};
    SelectPreviewSprite(memory, cpu);
    if (!CallPreviewStep(&preview, 0x849d55u, 0x81fc0bu, 3u))
        return preview.result;
    OpLdy(cpu, 0u);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_ENEMY_GROUP_FLAGS));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PREVIEW_SPRITE_ID));
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_ENEMY_SPRITE_GROUP_KEYS));
    if (!CallPreviewStep(&preview, 0x849d66u, 0x81fbc6u, 3u) ||
        !CallPreviewStep(&preview, 0x849d6au, 0x81fb8eu, 3u))
        return preview.result;
    PositionPreviewSprite(memory, cpu);
    PushDataBank(memory, cpu);
    OpLoadA(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x2100u);
    OpLdx(cpu, 0x2b96u);
    if (!CallPreviewStep(&preview, 0x849dadu, 0x849e1cu, 2u))
        return preview.result;
    OpSepWidths(cpu, 0x20u);
    PullDataBank(memory, cpu);
    if (!CallPreviewStep(&preview, 0x849db3u, 0x85dfb9u, 3u) ||
        !CallPreviewStep(&preview, 0x849db7u, 0x85df44u, 3u) ||
        !CallPreviewStep(&preview, 0x849dbbu, 0x85eb91u, 3u) ||
        !CallPreviewStep(&preview, 0x849dbfu, 0x85de9du, 3u) ||
        !CallPreviewStep(&preview, 0x849dc3u, 0x858a96u, 3u))
        return preview.result;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_EFFECT_SPECIAL_PARTY));
    if (!cpu->zero) {
        if (!CallPreviewStep(&preview, 0x849dccu, 0x8597d1u, 3u))
            return preview.result;
    } else {
        FillPreviewPartyTilemap(memory, cpu);
    }
    if (!PublishPreviewGraphics(&preview))
        return preview.result;
    return ExecutionReturned(0x849e1bu);
}

static void QueuePreviewBackgroundHalf(const Lufia2Memory *memory,
                                       Lufia2CpuState *cpu, uint16_t source,
                                       uint16_t destination) {
    OpLoadA(cpu, source);
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_VRAM_QUEUE + 2u));
    OpLoadA(cpu, destination);
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_VRAM_QUEUE + 4u));
    OpLoadA(cpu, 0x1000u);
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_VRAM_QUEUE));
}

Lufia2ExecutionResult Lufia2BattleUpdatePreviewBackground(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x84u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal || !child)
        return ExecutionHandoff(cpu, 0x849cd4u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PREVIEW_BACKGROUND_SELECTION));
    OpCmp(memory, cpu, OpAbs(cpu, WRAM_BATTLE_BACKGROUND_ID));
    if (cpu->zero)
        return ExecutionReturned(0x849d2bu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_BACKGROUND_ID));
    PreviewBuild preview = {memory, cpu, child, context, {0}};
    if (!CallPreviewStep(&preview, 0x849cdfu, 0x81b9c7u, 3u) ||
        !CallPreviewStep(&preview, 0x849ce3u, 0x81b9afu, 3u))
        return preview.result;
    OpRepWidths(cpu, 0x20u);
    if (!CallPreviewStep(&preview, 0x849ce9u, 0x85ecdbu, 3u))
        return preview.result;
    QueuePreviewBackgroundHalf(memory, cpu, 0xc000u, 0x3000u);
    OpSepWidths(cpu, 0x20u);
    if (!CallPreviewStep(&preview, 0x849d01u, 0x85ec81u, 3u))
        return preview.result;
    OpRepWidths(cpu, 0x20u);
    if (!CallPreviewStep(&preview, 0x849d07u, 0x85ecdbu, 3u))
        return preview.result;
    QueuePreviewBackgroundHalf(memory, cpu, 0xd000u, 0x3800u);
    OpSepWidths(cpu, 0x20u);
    if (!CallPreviewStep(&preview, 0x849d1fu, 0x85ec81u, 3u))
        return preview.result;
    OpRepWidths(cpu, 0x20u);
    if (!CallPreviewStep(&preview, 0x849d25u, 0x859af4u, 3u))
        return preview.result;
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x849d2bu);
}
