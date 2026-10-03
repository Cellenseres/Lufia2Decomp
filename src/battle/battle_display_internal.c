/* Battle display setup. */

#include "battle/battle_lifecycle_internal.h"
#include "core/snes_registers.h"
#include "core/wram_view.h"
#include "system/scene_nmi_internal.h"
#include "system/wram.h"

/* Direct-page bytes of the scene NMI that setup clears first. */
enum {
    DISPLAY_PPU_TABLE = 0xb5d3u, /* ROM pairs of PPU register and value */
    DISPLAY_DP_CLEAR_COUNT = 4,
    DISPLAY_QUEUE = 0x0594u, /* 16 bytes cleared, then three words set to $FFFF */
    DISPLAY_QUEUE_BYTES = 16,
    DISPLAY_QUEUE_WORD_A = 0x0596u,
    DISPLAY_QUEUE_WORD_B = 0x059au,
    DISPLAY_QUEUE_WORD_C = 0x059eu,
    DISPLAY_LAST_FLAG = 0x15b3u,
};

/* Clears the scene NMI's upload bytes and queue, and blanks the screen. */
void BattleResetDisplayWork(BattleContext *battle) {
    static const uint8_t cleared_dp[DISPLAY_DP_CLEAR_COUNT] = {0x74u, 0x72u, 0x73u,
                                                               0x71u};
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    unsigned i;

    for (i = 0; i < DISPLAY_DP_CLEAR_COUNT; ++i)
        WramWrite(wram, cleared_dp[i], 0u);
    WramWrite(wram, WRAM_BRIGHTNESS, BRIGHTNESS_FORCED_BLANK);
    for (i = DISPLAY_QUEUE_BYTES; i-- > 0u;)
        WramWriteAt(wram, DISPLAY_QUEUE, (uint16_t)i, 0u);
    cpu->x = 0xffffu;
    WramWrite16(wram, DISPLAY_QUEUE_WORD_A, cpu->x);
    WramWrite16(wram, DISPLAY_QUEUE_WORD_B, cpu->x);
    WramWrite16(wram, DISPLAY_QUEUE_WORD_C, cpu->x);
    OpTxa(cpu);
    WramWrite(wram, DISPLAY_LAST_FLAG, A8(cpu));
}

enum {
    DISPLAY_RECORD_FIRST = 0x1a8fu, /* 15 records of 6 bytes, first word cleared */
    DISPLAY_RECORD_STRIDE = 6,
    DISPLAY_RECORD_END = 90,
};

/* Clears the four tilemaps and the display records. */
bool BattleClearDisplayBuffers(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    unsigned offset;

    if (!BattleClearBackgroundTilemap(battle))
        return false;
    if (!BattleResetPartyTilemap(battle))
        return false;
    if (!BattleClearWindowTilemapForSetup(battle))
        return false;
    if (!BattleClearTilemap3800ForSetup(battle))
        return false;

    WramWrite(wram, 0xd8u, 0u);
    WramWrite(wram, 0xd9u, 0u);
    WramWrite(wram, BATTLE_FRAME_STATE, 0u);

    OpRepWidths(cpu, 0x20u);
    for (offset = DISPLAY_RECORD_END; offset > 0u; offset -= DISPLAY_RECORD_STRIDE)
        WramWrite16At(wram, DISPLAY_RECORD_FIRST, (uint16_t)offset, 0u);
    OpLdx(cpu, 0u);
    OpSepWidths(cpu, 0x20u);

    return true;
}

enum {
    DISPLAY_PARTY_SLOTS = 4,
    DISPLAY_PARTY_PORTRAIT_FLAGS = 0xb411u, /* per party id, ROM */
    DISPLAY_PARTY_STATE = 0x00139cu,        /* 13 bytes per slot */
    DISPLAY_PARTY_STATE_STRIDE = 13,
};

/* ORs each party member's portrait flags into that slot's display state. */
void BattleBuildPartyDisplayState(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const Lufia2Wram state = WramViewLong(memory);
    /* The first id is widened with the direct page's high byte left in A. */
    uint16_t high = 0u;
    unsigned slot;

    TransferDirectToA(cpu);
    high = (uint16_t)(cpu->accumulator & 0xff00u);
    for (slot = 0; slot < DISPLAY_PARTY_SLOTS; ++slot) {
        const uint16_t id =
            (uint16_t)(high | WramReadAt(wram, WRAM_BATTLE_PARTY_IDS, (uint16_t)slot));
        const uint16_t offset = (uint16_t)(slot * DISPLAY_PARTY_STATE_STRIDE);
        uint8_t flags;

        /* The slot counter waits on the stack while Y holds the id. */
        cpu->y = (uint16_t)slot;
        PushY(memory, cpu);
        flags = WramReadAt(wram, DISPLAY_PARTY_PORTRAIT_FLAGS, id);
        WramWriteAt(state, DISPLAY_PARTY_STATE, offset,
                    (uint8_t)(flags | WramReadAt(state, DISPLAY_PARTY_STATE, offset)));
        OpPullY(memory, cpu);
        high = 0u;
    }
    cpu->accumulator = (uint16_t)(DISPLAY_PARTY_SLOTS * DISPLAY_PARTY_STATE_STRIDE);
    cpu->x = cpu->accumulator;
    cpu->y = DISPLAY_PARTY_SLOTS;
    cpu->overflow = 0;
    OpCpy(cpu, DISPLAY_PARTY_SLOTS);
}

enum {
    DISPLAY_NAME_ROWS = 11,
    DISPLAY_CLEAR_AREA = 0x1b17u,
    DISPLAY_CLEAR_AREA_BYTES = 96,
    DISPLAY_ROW_COPY_ROM = 0xb2fcu,
    DISPLAY_FILL_AREA = 0x12e3u,
    DISPLAY_FILL_BYTES = 16,
    DISPLAY_ICON_SLOTS_LONG = 0x001be0u, /* four bytes, last slot first */
};

/* Resets the display record words and flags to their opening values. */
void BattleInitializeDisplayRecords(BattleContext *battle) {
    static const uint16_t clear_bytes[] = {0x15a8u, 0x15a9u, 0x15c7u, 0x15cbu,
                                           0x15cfu, 0x15d3u, 0x15d7u, 0x15dbu,
                                           0x15dfu, 0x15e3u, 0x15e7u};
    static const uint16_t slot_words[] = {0x48c0u, 0x4910u, 0x4956u, 0x4abeu, 0x4b36u};
    static const uint16_t slot_destinations[] = {0x15c8u, 0x15ccu, 0x15d4u, 0x15d8u,
                                                 0x15e8u};
    static const uint16_t no_entry_words[] = {0x1321u, 0x1323u, 0x131eu, 0x1320u};
    static const uint16_t gauge_bytes[] = {0x1475u, 0x1476u, 0x1478u};
    static const uint8_t icon_slots[] = {0x6cu, 0x71u, 0x76u, 0x7bu};
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const Lufia2Wram bank0 = WramViewLong(memory);
    unsigned i;

    for (i = 0; i < sizeof(clear_bytes) / sizeof(clear_bytes[0]); ++i)
        WramWrite(wram, clear_bytes[i], 0u);
    for (i = 0; i < sizeof(slot_words) / sizeof(slot_words[0]); ++i)
        WramWrite16(wram, slot_destinations[i], slot_words[i]);
    for (i = 0; i < sizeof(no_entry_words) / sizeof(no_entry_words[0]); ++i)
        WramWrite16(wram, no_entry_words[i], 0xffffu);
    for (i = 0; i < sizeof(gauge_bytes) / sizeof(gauge_bytes[0]); ++i)
        WramWrite(wram, gauge_bytes[i], 0x30u);
    WramWrite(wram, 0x1477u, 0x20u);

    for (i = DISPLAY_CLEAR_AREA_BYTES; i-- > 0u;)
        WramWriteAt(wram, DISPLAY_CLEAR_AREA, (uint16_t)i, 0u);
    WramWrite(wram, 0xdbu, 0u);
    for (i = DISPLAY_NAME_ROWS; i-- > 0u;)
        WramWriteAt(wram, WRAM_FIELD_STREAMED_ROW_SOURCE + 1u, (uint16_t)i,
                    WramReadAt(wram, DISPLAY_ROW_COPY_ROM, (uint16_t)i));
    for (i = DISPLAY_FILL_BYTES; i-- > 0u;)
        WramWriteAt(wram, DISPLAY_FILL_AREA, (uint16_t)i, 0xffu);
    for (i = 0; i < sizeof(icon_slots); ++i)
        WramWriteAt(bank0, DISPLAY_ICON_SLOTS_LONG, (uint16_t)(2u * (3u - i)),
                    icon_slots[i]);

    cpu->x = 0xffffu;
    LoadA8(cpu, icon_slots[sizeof(icon_slots) - 1u]);
}

/* Loads the display defaults, then runs the two record-preparing children. */
bool BattlePrepareDisplayRecords(BattleContext *battle) {
    if (!BattleLoadDisplayDefaults(battle))
        return false;
    if (!BattleCall(battle, 0x8627u, 0x81e877u, 2u))
        return false;
    return BattleCall(battle, 0x862au, BATTLE_ROUTINE_FRAME_INPUT, 3u);
}

/* Writes the (register, value) pairs of the PPU table, ending at a register
 * number with bit 7 set. */
void BattleApplyPpuTable(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    bool loaded = false;
    uint8_t value = 0u;
    uint8_t index = 0u;
    uint8_t reg;

    OpSepWidths(cpu, 0x30u);
    for (;;) {
        reg = WramReadAt(wram, DISPLAY_PPU_TABLE, index);
        if (reg & 0x80u)
            break;
        ++index;
        value = WramReadAt(wram, DISPLAY_PPU_TABLE, index);
        ++index;
        WramWriteAt(wram, SNES_INIDISP, reg, value);
        loaded = true;
    }
    cpu->y = index;
    if (loaded)
        LoadA8(cpu, value);
    OpLdx(cpu, reg);
}

/* Installs the scene NMI, decompresses the base resource and loads portraits and the
 * base tile children. */
bool BattleLoadBaseGraphics(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;

    OpRepWidths(cpu, 0x10u);
    Lufia2InstallSceneNmi(memory, cpu, 0x8dc5u, 0x85u);

    OpLdx(cpu, 0x8800u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x60u), cpu->x);
    LoadA8(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, 0x62u));
    OpLdx(cpu, 0x0190u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x54u), cpu->x);

    if (!BattleDecompressResource(battle, 0x865du))
        return false;
    if (!BattleLoadPortraits(battle))
        return false;
    if (!BattleCall(battle, 0x8665u, 0x81bc55u, 2u))
        return false;
    if (!BattleCall(battle, 0x8668u, 0x85df44u, 3u))
        return false;
    if (!BattleCall(battle, 0x866cu, 0x85eb91u, 3u))
        return false;
    return BattleCall(battle, 0x8670u, 0x85de9du, 3u);
}

typedef struct BattleVramTransfer {
    uint16_t vram_address;
    uint32_t source;
    uint16_t size;
} BattleVramTransfer;

/* DMAs one block of tile data to VRAM on channel 0. */
static void UploadVramBlock(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                            const BattleVramTransfer *transfer) {
    OpLdx(cpu, transfer->vram_address);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_VMADDL), cpu->x);
    LoadA8(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_DMAP(0)));
    OpLdx(cpu, (uint16_t)transfer->source);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_A1TL(0)), cpu->x);
    LoadA8(cpu, (uint8_t)(transfer->source >> 16));
    OpSta(memory, cpu, OpAbs(cpu, SNES_A1B(0)));
    OpLdx(cpu, transfer->size);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_DASL(0)), cpu->x);
    LoadA8(cpu, 0x18u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_BBAD(0)));
    LoadA8(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_MDMAEN));
}

/* Uploads the two base tile blocks to VRAM. */
void BattleUploadBaseTiles(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    static const BattleVramTransfer transfers[] = {
        {0x1000u, 0x9f8500u, 0x0f00u},
        {0x1780u, 0x96ff3cu, 0x00b0u},
    };

    for (unsigned i = 0; i < sizeof(transfers) / sizeof(transfers[0]); ++i)
        UploadVramBlock(memory, cpu, &transfers[i]);
}

/* Loads the battle background, palettes and resource blocks used by the presentation.
 */
bool BattleLoadPresentationAssets(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    static const uint32_t resource_children[9] = {0x859addu, 0x859af4u, 0x859b0bu,
                                                  0x859b39u, 0x859b50u, 0x859b67u,
                                                  0x859b7eu, 0x859b95u, 0x859bacu};

    if (!BattlePrepareBackground(battle))
        return false;

    LoadA8(cpu, 0x97u);
    OpSta(memory, cpu, OpDp(cpu, 0x24u));
    OpLdy(cpu, 0xfde6u);
    TransferDirectToA(cpu);
    if (!BattleLoadPalette(battle, 0x86ccu))
        return false;

    LoadA8(cpu, 0x97u);
    OpSta(memory, cpu, OpDp(cpu, 0x24u));
    OpLdy(cpu, 0xcd38u);
    LoadA8(cpu, 8u);
    if (!BattleLoadPalette(battle, 0x86d8u))
        return false;

    LoadA8(cpu, 2u);
    OpSta(memory, cpu, OpAbs(cpu, 0x15abu));
    if (!BattleCall(battle, 0x86e0u, 0x858a39u, 3u))
        return false;
    if (!BattleCommitPalettes(battle))
        return false;

    LoadA8(cpu, 0xffu);
    OpSta(memory, cpu, BATTLE_SPRITE_REBUILD_REQUEST);

    OpRepWidths(cpu, 0x20u);
    for (unsigned i = 0; i < 9u; ++i) {
        if (!BattleCall(battle, (uint16_t)(0x86f0u + 4u * i), resource_children[i], 3u))
            return false;
    }
    OpSepWidths(cpu, 0x20u);
    return true;
}

enum {
    DISPLAY_ICON_ROWS = 10,
    DISPLAY_ICON_SOURCE = 0x161bu,
    DISPLAY_ICON_BLANK = 16,
    DISPLAY_ICON_COPY_LONG = 0x7ee700u,
    DISPLAY_SPRITE_STATE_FLAG = 0x40u,
    DISPLAY_ROW_COUNTER = 0x1bu,
};

/* Copies the ten icon ids to $7E:E700; an id of 16 is replaced by the direct
 * page's low byte. */
void BattlePrepareSpriteState(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    const Lufia2Wram dp = WramViewOfCaller(memory, cpu);
    Lufia2Wram work;
    uint16_t high = (uint16_t)(cpu->accumulator & 0xff00u);
    uint8_t loaded = 0u;
    uint8_t stored = 0u;
    unsigned row;

    WramWrite(dp, 0x72u, DISPLAY_SPRITE_STATE_FLAG);
    WramWrite(dp, BATTLE_FRAME_STATE, (uint8_t)(DISPLAY_SPRITE_STATE_FLAG << 1));

    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7eu);
    work = WramViewOfCaller(memory, cpu);
    WramWrite(dp, DISPLAY_ROW_COUNTER, DISPLAY_ICON_ROWS);
    for (row = 0; row < DISPLAY_ICON_ROWS; ++row) {
        loaded = WramReadAt(work, DISPLAY_ICON_SOURCE, (uint16_t)row);
        stored = loaded;
        if (loaded == DISPLAY_ICON_BLANK) {
            stored = (uint8_t)cpu->direct_page;
            high = (uint16_t)(cpu->direct_page & 0xff00u);
        }
        WramWriteAt(WramViewLong(memory), DISPLAY_ICON_COPY_LONG, (uint16_t)row,
                    stored);
        WramStep(dp, DISPLAY_ROW_COUNTER, -1);
    }
    cpu->accumulator = (uint16_t)(high | stored);
    cpu->carry = loaded >= DISPLAY_ICON_BLANK;
    cpu->x = DISPLAY_ICON_ROWS;
    cpu->y = DISPLAY_ICON_ROWS;

    PullDataBank(memory, cpu);
}

/* Runs the closing display children, then arms a 30-frame wait; control modes 2 and 1
 * together skip that tail. */
bool BattleFinishDisplay(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;

    if (!BattleCall(battle, 0x873eu, 0x85ab5bu, 3u))
        return false;
    if (!BattleCall(battle, 0x8742u, BATTLE_ROUTINE_FRAME_INPUT, 3u))
        return false;

    OpSta(memory, cpu, OpAbs(cpu, 0x123au));
    if (BattleControlHas(battle, BATTLE_CONTROL_MODE_2)) {
        OpBitValue(cpu, BATTLE_CONTROL_MODE_1);
        if (!cpu->zero)
            return true;
        OpLdx(cpu, 0xf291u);
    } else {
        OpLdx(cpu, 0xf283u);
    }

    if (!BattleCall(battle, 0x875cu, 0x8594e7u, 3u))
        return false;

    OpLdx(cpu, 30u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_BATTLE_WAIT_COUNTER), cpu->x);

    return BattleCall(battle, 0x8766u, 0x8595feu, 3u);
}
