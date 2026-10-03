/* Menu screen building blocks and screens. */

#include "core/cpu_ops.h"
#include "lufia2/item.h"
#include "lufia2/menu.h"
#include "lufia2/party.h"
#include "lufia2/system.h"
#include "menu/menu_sprite_slots.h"
#include "party/party_internal.h"
#include "system/dp_scratch.h"
#include "system/system_internal.h"
#include "system/wram.h"

enum {
    /* Shop description word, in the same bytes the battle screen keeps its
     * party ids: bit 0 shows the owned items, bits 1-5 the sale lists, bit 7
     * the extra list. */
    MENU_SHOP_FLAGS = 0x1540u,
};

/* Menu cursors. Each cursor index is a six-entry byte array per field ($14C1
 * to $1508, six bytes apart). A cursor's position is origin + spacing * index
 * on each axis, written into sprite slot MENU_CURSOR_SPRITE_BASE + cursor.
 * The layout bytes are read from $A6:F518 + Y; $14D3, $14E5 and $14C7 are
 * cleared whenever a layout is placed, and the two index arrays are cleared
 * when a screen starts. */
enum {
    MENU_CURSOR_SPRITE_BASE = 5,
    MENU_SPRITE_SCROLL_THUMB = 11, /* sprite slot of the scrollbar thumb */
    /* Item ids whose use is decided by a bit of the window mode ($09A7) rather
     * than the item record: bit $10 for the first, bit $08 for the others. */
    MENU_ITEM_USE_BIT_10 = 0x2au,
    MENU_ITEM_USE_BIT_08_A = 0x29u,
    MENU_ITEM_USE_BIT_08_B = 0x2du,
    /* Byte selecting how ItemAttribute reads a row: 1 and 2 read the item
     * record byte at $8460 or $846A, 0 decides from the item id. Set by the
     * list mode in ListByMode. */
    MENU_ROW_ATTRIBUTE_KIND = 0x153eu,
    MENU_SPRITE_SLOT_SCRATCH = 0x14a9u,  /* word: slot kept across a lookup */
    MENU_CURSOR_LAYOUT_14C1 = 0x14c1u,   /* last layout byte, purpose unknown */
    MENU_CURSOR_LAYOUT_14C7 = 0x14c7u,   /* cleared with the layout */
    MENU_CURSOR_X_ORIGIN = 0x14cdu,      /* layout byte 0, low byte of x */
    MENU_CURSOR_X_ORIGIN_HIGH = 0x14d3u, /* high byte, cleared on placement */
    MENU_CURSOR_X_INDEX = 0x14d9u,       /* column the cursor is on */
    MENU_CURSOR_Y_ORIGIN = 0x14dfu,      /* layout byte 1 */
    MENU_CURSOR_Y_ORIGIN_HIGH = 0x14e5u,
    MENU_CURSOR_Y_INDEX = 0x14ebu,   /* row the cursor is on */
    MENU_CURSOR_COLUMNS = 0x14f1u,   /* layout byte 2 */
    MENU_CURSOR_ROWS = 0x14f7u,      /* layout byte 3; the shop sets it to
                                        the list length */
    MENU_CURSOR_X_SPACING = 0x14fdu, /* layout byte 4 */
    MENU_CURSOR_Y_SPACING = 0x1503u, /* layout byte 5 */
};

/* Pushes the JSR return frame for the given address. */
static void Jsr(const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t ret) {
    SimulateJsrFrame(memory, cpu, ret);
}

/* Pops the RTS frame pushed by Jsr. */
static void Rts(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SimulateRtsFrame(memory, cpu);
}

/* (dp),Y 16-bit through DB. */
static uint16_t Indirect16At(const Lufia2Memory *memory,
    const Lufia2CpuState *cpu, uint8_t offset, uint16_t index) {
    return Read16Long(memory, (((uint32_t)cpu->data_bank << 16) +
        Read16Direct(memory, cpu, offset) + index) & 0x00ffffffu);
}

/* Loads the data bank register with the given bank through A, as the ROM
 * does with LDA/PHA/PLB. */
static void SetBank(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t bank) {
    LoadA8(cpu, bank);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
}

/* Stores A at the absolute address indexed by X. */
static void StoreIndexed(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t address) {
    StoreAAbsolute8(memory, cpu, address, cpu->x);
}

/* JSL $80:8878; 1 when it handed off. */
static int DrawString(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t return_address, Lufia2ExecutionResult *out) {
    SimulateJslFrame(memory, cpu, 0x82u, return_address);
    cpu->program_bank = 0x80u;
    *out = Lufia2MenuDrawString(memory, cpu);
    if (out->flow != LUFIA2_EXECUTION_RETURNED)
        return 1;
    SimulateRtlFrame(memory, cpu);
    cpu->program_bank = 0x82u;
    return 0;
}

/* $82:83B5: clear both layers ($7E:2000/$7E:3000) in a rect; M0. */
static void MenuClearRect(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    StoreADirect16(memory, cpu, DP_SCRATCH_A);
    Write16Direct(memory, cpu, DP_SCRATCH_C, 0);
    Write16Direct(memory, cpu, DP_SCRATCH_E, 0);
    TransferXToA(cpu);
    SetAccumulatorWidth(cpu, 1);
    StoreADirect8(memory, cpu, DP_SCRATCH_E);
    ExchangeAccumulatorBytes(cpu);
    StoreADirect8(memory, cpu, DP_SCRATCH_C);
    SetAccumulatorWidth(cpu, 0);
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    SetBank(memory, cpu, 0x7eu);
    SetAccumulatorWidth(cpu, 0);
    do {
        LoadY16(cpu, Read16Direct(memory, cpu, DP_SCRATCH_C));
        LoadX16(cpu, Read16Direct(memory, cpu, DP_SCRATCH_A));
        do {
            Write16Absolute(memory, cpu, (uint16_t)(0x2000u + cpu->x), 0);
            Write16Absolute(memory, cpu, (uint16_t)(0x3000u + cpu->x), 0);
            IncrementX16(cpu);
            IncrementX16(cpu);
            cpu->y = (uint16_t)(cpu->y - 1u);
            SetNz16(cpu, cpu->y);
        } while (!cpu->zero);
        LoadA16(cpu, Read16Direct(memory, cpu, DP_SCRATCH_A));
        cpu->carry = 0;
        Add16Value(cpu, 0x0040u);
        StoreADirect16(memory, cpu, DP_SCRATCH_A);
        Decrement16Direct(memory, cpu, DP_SCRATCH_E);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
}

/* $82:88CB: cursor index X's sprite position: origin + spacing * index on
 * each axis (x, then y). */
static void MenuCursorPosition(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint16_t kIn[2][2] = {{MENU_CURSOR_X_SPACING, MENU_CURSOR_X_INDEX},
                                       {MENU_CURSOR_Y_SPACING, MENU_CURSOR_Y_INDEX}};
    static const uint16_t kBase[2][2] = {
        {MENU_CURSOR_X_ORIGIN, MENU_CURSOR_X_ORIGIN_HIGH},
        {MENU_CURSOR_Y_ORIGIN, MENU_CURSOR_Y_ORIGIN_HIGH}};
    static const uint16_t kOut[2][2] = {{MENU_SPRITE_X_LOW + MENU_CURSOR_SPRITE_BASE,
                                         MENU_SPRITE_X_HIGH + MENU_CURSOR_SPRITE_BASE},
                                        {MENU_SPRITE_Y_LOW + MENU_CURSOR_SPRITE_BASE,
                                         MENU_SPRITE_Y_HIGH + MENU_CURSOR_SPRITE_BASE}};
    static const uint16_t kReturn[2] = {0x88e0u, 0x8909u};
    unsigned i;

    for (i = 0; i < 2u; ++i) {
        LoadAAbsolute8(memory, cpu, kIn[i][0], cpu->x);
        StoreAAbsolute8(memory, cpu, WRAM_SYSTEM_MULTIPLY_A, 0);
        StoreZeroAbsolute8(memory, cpu, (WRAM_SYSTEM_MULTIPLY_A + 1u), 0);
        LoadAAbsolute8(memory, cpu, kIn[i][1], cpu->x);
        StoreAAbsolute8(memory, cpu, WRAM_SYSTEM_MULTIPLY_B, 0);
        StoreZeroAbsolute8(memory, cpu, (WRAM_SYSTEM_MULTIPLY_B + 1u), 0);
        Lufia2CallMultiply(memory, cpu, 0x82u, kReturn[i]);
        LoadAAbsolute8(memory, cpu, WRAM_SYSTEM_MULTIPLY_PRODUCT, 0);
        cpu->carry = 0;
        Adc8(cpu, AbsoluteByte(memory, cpu, kBase[i][0], cpu->x));
        StoreIndexed(memory, cpu, kOut[i][0]);
        LoadAAbsolute8(memory, cpu, (WRAM_SYSTEM_MULTIPLY_PRODUCT + 1u), 0);
        Adc8(cpu, AbsoluteByte(memory, cpu, kBase[i][1], cpu->x));
        StoreIndexed(memory, cpu, kOut[i][1]);
    }
}

/* $82:891E: cursor index X takes the seven-byte layout at $A6:F518 + Y. */
static void MenuCursorPlace(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint16_t kFields[7] = {MENU_CURSOR_X_ORIGIN,   MENU_CURSOR_Y_ORIGIN,
                                        MENU_CURSOR_COLUMNS,    MENU_CURSOR_ROWS,
                                        MENU_CURSOR_X_SPACING,  MENU_CURSOR_Y_SPACING,
                                        MENU_CURSOR_LAYOUT_14C1};
    unsigned i;

    PushDataBank(memory, cpu);
    SetBank(memory, cpu, 0xa6u);
    for (i = 0; i < 7u; ++i) {
        LoadAAbsolute8(memory, cpu, (uint16_t)(0xf518u + i), cpu->y);
        StoreIndexed(memory, cpu, kFields[i]);
        if (i == 1u) {
            StoreZeroAbsolute8(memory, cpu, MENU_CURSOR_X_ORIGIN_HIGH, cpu->x);
            StoreZeroAbsolute8(memory, cpu, MENU_CURSOR_Y_ORIGIN_HIGH, cpu->x);
            StoreZeroAbsolute8(memory, cpu, MENU_CURSOR_LAYOUT_14C7, cpu->x);
        }
    }
    PullDataBank(memory, cpu);
    Jsr(memory, cpu, 0x8959u);
    MenuCursorPosition(memory, cpu);
    Rts(memory, cpu);
}

/* $82:895B: sprite slot X shows cursor animation A. */
static void MenuCursorAnimation(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Write16Absolute(memory, cpu, MENU_SPRITE_SLOT_SCRATCH, cpu->x);
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x00ffu);
    LoadA16(cpu, (uint16_t)(cpu->accumulator - 1u));
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x82897du, cpu->x)));
    SetAccumulatorWidth(cpu, 1);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, MENU_SPRITE_SLOT_SCRATCH, 0));
    StoreIndexed(memory, cpu, MENU_SPRITE_FRAME_INDEX);
    LoadA8(cpu, 0x01u);
    StoreIndexed(memory, cpu, MENU_SPRITE_ACTIVE);
    ExchangeAccumulatorBytes(cpu);
    SimulateJslFrame(memory, cpu, 0x82u, 0x897bu);
    (void)Lufia2SpriteSetAnimation(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $82:93F6: window HDMA table A from $8E:E57E/$8E:E5A0. */
static void MenuWindowHdma(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x8ee57eu, cpu->x)));
    StoreAAbsolute16(memory, cpu, 0x0596u, 0);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x8ee5a0u, cpu->x)));
    StoreADirect16(memory, cpu, 0xf4u);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x8eu);
    StoreADirect8(memory, cpu, 0xf6u);
    LoadA8(cpu, 0x10u);
    {
        const uint8_t old = DirectByte(memory, cpu, 0xf2u);    /* TSB */

        cpu->zero = (old & 0x10u) == 0;
        Write8(memory, DirectAddress(cpu, 0xf2u), (uint8_t)(old | 0x10u));
    }
    LoadA8(cpu, 0x40u);
    StoreADirect8(memory, cpu, 0xf3u);
    LoadA8(cpu, 0x12u);
    StoreADirect8(memory, cpu, 0xfau);
    LoadA8(cpu, 0x02u);
    StoreADirect8(memory, cpu, 0xfbu);
    LoadX16(cpu, 0x80c0u);
    StoreXDirect16(memory, cpu, 0xf7u);
    LoadA8(cpu, 0x7eu);
    StoreADirect8(memory, cpu, 0xf9u);
    LoadA8(cpu, 0x03u);
    StoreAAbsolute8(memory, cpu, 0x1565u, 0);
}

/* $82:9F6F: equipment commands window. */
Lufia2ExecutionResult Lufia2MenuEquipCommands(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, 0x0442u);
    LoadX16(cpu, 0x1e0bu);
    Jsr(memory, cpu, 0x9f79u);
    MenuClearRect(memory, cpu);
    Rts(memory, cpu);
    LoadA16(cpu, 0x045eu);
    LoadX16(cpu, 0x0f0au);
    Jsr(memory, cpu, 0x9f82u);
    (void)Lufia2MenuDrawWindow(memory, cpu);
    Rts(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    StoreA8Absolute(memory, cpu, WRAM_MENU_DRAW_MODE, 0x20u);
    LoadA8(cpu, 0x8eu);
    StoreADirect8(memory, cpu, 0x5fu);
    LoadY16(cpu, 0xd2aeu);
    {
        Lufia2ExecutionResult text;

        if (DrawString(memory, cpu, 0x9f94u, &text))
            return text;
    }
    LoadA8(cpu, 0x01u);
    LoadX16(cpu, 0x0007u);
    Jsr(memory, cpu, 0x9f9cu);
    MenuCursorAnimation(memory, cpu);
    Rts(memory, cpu);
    LoadY16(cpu, 0x0015u);
    LoadX16(cpu, 0x0002u);
    Jsr(memory, cpu, 0x9fa5u);
    MenuCursorPlace(memory, cpu);
    Rts(memory, cpu);
    LoadA8(cpu, 0x02u);
    Jsr(memory, cpu, 0x9faau);
    MenuWindowHdma(memory, cpu);
    Rts(memory, cpu);
    LoadA8(cpu, 0x88u);
    {
        const uint8_t old = DirectByte(memory, cpu, 0x74u);    /* TSB */

        cpu->zero = (old & 0x88u) == 0;
        Write8(memory, DirectAddress(cpu, 0x74u), (uint8_t)(old | 0x88u));
    }
    return ExecutionReturned(0x829fafu);
}

/* Draws a window through the ROM window routine, position in A and size in X. */
static void Window(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t position, uint16_t size, uint16_t return_address) {
    LoadA16(cpu, position);
    LoadX16(cpu, size);
    Jsr(memory, cpu, return_address);
    (void)Lufia2MenuDrawWindow(memory, cpu);
    Rts(memory, cpu);
}

/* JSL $80:8878 with the string at $8E:Y, attribute $20. */
static int Text8E(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t string, uint16_t return_address, Lufia2ExecutionResult *out) {
    StoreA8Absolute(memory, cpu, WRAM_MENU_DRAW_MODE, 0x20u);
    LoadA8(cpu, 0x8eu);
    StoreADirect8(memory, cpu, 0x5fu);
    LoadY16(cpu, string);
    return DrawString(memory, cpu, return_address, out);
}

/* $82:9214 / $82:922D: clear flag bytes for set bits of X. */
static void MenuClearSlots(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t table) {
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    TransferXToA(cpu);
    LoadX16(cpu, 0x0000u);
    do {
        LsrA16(cpu);
        if (cpu->carry)
            Write8(memory, AbsoluteIndexedAddress(cpu, table, cpu->x), 0);
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x0010u);
    } while (!cpu->zero);
    UnpackStatus(cpu, Pull8(memory, cpu));
}

/* $82:D721: shop windows; the first only with $1540 bit 1. */
Lufia2ExecutionResult Lufia2MenuShopWindows(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, MENU_SHOP_FLAGS, 0));
    if (cpu->accumulator & 0x0002u)
        Window(memory, cpu, 0x0382u, 0x0f03u, 0xd733u);
    Window(memory, cpu, 0x03a0u, 0x0f03u, 0xd73cu);
    Window(memory, cpu, 0x0442u, 0x1e0au, 0xd745u);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x82d748u);
}

/* $82:E49E: shop kind title and its window. */
Lufia2ExecutionResult Lufia2MenuShopTitle(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult text;

    StoreA8Absolute(memory, cpu, WRAM_MENU_DRAW_MODE, 0x20u);
    LoadA8(cpu, 0x8eu);
    StoreADirect8(memory, cpu, 0x5fu);
    LoadY16(cpu, 0xd366u);
    LoadX16(cpu, 0x30f0u);
    if (DrawString(memory, cpu, 0xe4b0u, &text))
        return text;
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, (uint16_t)(Read16AbsoluteIndexed(memory, cpu,
                                                  WRAM_MENU_SHOP_LIST_COUNT, 0) -
                            1u));
    And16(cpu, 0x00ffu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x82e4d0u, cpu->x)));
    And16(cpu, 0x00ffu);
    cpu->carry = 0;
    Add16Value(cpu, 0x0900u);
    TransferAToX(cpu);
    LoadA16(cpu, 0x00acu);
    Jsr(memory, cpu, 0xe4ccu);
    (void)Lufia2MenuDrawWindow(memory, cpu);
    Rts(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x82e4cfu);
}

/* $82:EFC5: "Game saved." window; carry picks the layout. */
Lufia2ExecutionResult Lufia2MenuSavedWindow(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult text;

    SetAccumulatorWidth(cpu, 0);
    if (!cpu->carry) {
        Window(memory, cpu, 0x0584u, 0x1105u, 0xefd1u);
        LoadX16(cpu, 0x3648u);
    } else {
        Window(memory, cpu, 0x0090u, 0x1703u, 0xefdfu);
        LoadX16(cpu, 0x30d4u);
    }
    SetAccumulatorWidth(cpu, 1);
    if (Text8E(memory, cpu, 0xcae1u, 0xeff4u, &text))
        return text;
    return ExecutionReturned(0x82eff5u);
}

/* $82:F0A2: name entry windows and cursor. */
Lufia2ExecutionResult Lufia2MenuNameEntryWindows(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadX16(cpu, 0xffffu);
    SimulateJslFrame(memory, cpu, 0x82u, 0xf0aau);
    MenuClearSlots(memory, cpu, MENU_SPRITE_ACTIVE);
    SimulateRtlFrame(memory, cpu);
    LoadX16(cpu, 0xffffu);
    SimulateJslFrame(memory, cpu, 0x82u, 0xf0b1u);
    MenuClearSlots(memory, cpu, 0x11e8u);
    SimulateRtlFrame(memory, cpu);
    Window(memory, cpu, 0x0108u, 0x0803u, 0xf0bau);
    Window(memory, cpu, 0x0118u, 0x1003u, 0xf0c3u);
    Window(memory, cpu, 0x01c8u, 0x1811u, 0xf0ccu);
    SetAccumulatorWidth(cpu, 1);
    StoreZeroAbsolute8(memory, cpu, 0x14deu, 0);
    StoreZeroAbsolute8(memory, cpu, 0x14f0u, 0);
    LoadA8(cpu, 0x07u);
    LoadX16(cpu, 0x000au);
    Jsr(memory, cpu, 0xf0dcu);
    MenuCursorAnimation(memory, cpu);
    Rts(memory, cpu);
    LoadY16(cpu, 0x00afu);
    LoadX16(cpu, 0x0005u);
    Jsr(memory, cpu, 0xf0e5u);
    MenuCursorPlace(memory, cpu);
    Rts(memory, cpu);
    LoadX16(cpu, 0x0000u);
    Write16Absolute(memory, cpu, 0x14bbu, 0);
    Write16Absolute(memory, cpu, WRAM_MENU_LIST_COUNT, 0);
    LoadX16(cpu, 0x3162u);
    Write16Absolute(memory, cpu, 0x14adu, 0x3162u);
    return ExecutionReturned(0x82f0f5u);
}

/* $82:83EB: clear layer 2 ($7E:3000) in a rect; M0. */
static void MenuClearRect2(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    StoreADirect16(memory, cpu, DP_SCRATCH_A);
    Write16Direct(memory, cpu, DP_SCRATCH_C, 0);
    Write16Direct(memory, cpu, DP_SCRATCH_E, 0);
    TransferXToA(cpu);
    SetAccumulatorWidth(cpu, 1);
    StoreADirect8(memory, cpu, DP_SCRATCH_E);
    ExchangeAccumulatorBytes(cpu);
    StoreADirect8(memory, cpu, DP_SCRATCH_C);
    SetAccumulatorWidth(cpu, 0);
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    SetBank(memory, cpu, 0x7eu);
    SetAccumulatorWidth(cpu, 0);
    do {
        LoadY16(cpu, Read16Direct(memory, cpu, DP_SCRATCH_C));
        LoadX16(cpu, Read16Direct(memory, cpu, DP_SCRATCH_A));
        do {
            Write16Absolute(memory, cpu, (uint16_t)(0x3000u + cpu->x), 0);
            IncrementX16(cpu);
            IncrementX16(cpu);
            cpu->y = (uint16_t)(cpu->y - 1u);
            SetNz16(cpu, cpu->y);
        } while (!cpu->zero);
        LoadA16(cpu, Read16Direct(memory, cpu, DP_SCRATCH_A));
        cpu->carry = 0;
        Add16Value(cpu, 0x0040u);
        StoreADirect16(memory, cpu, DP_SCRATCH_A);
        Decrement16Direct(memory, cpu, DP_SCRATCH_E);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
}

/* $82:831A: tile block at A, X = w << 8 | h; M0. */
static void MenuTileBlock(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushAccumulator16(memory, cpu);
    Write16Direct(memory, cpu, DP_SCRATCH_C, 0);
    Write16Direct(memory, cpu, DP_SCRATCH_E, 0);
    TransferXToA(cpu);
    SetAccumulatorWidth(cpu, 1);
    StoreADirect8(memory, cpu, DP_SCRATCH_E);
    ExchangeAccumulatorBytes(cpu);
    StoreADirect8(memory, cpu, DP_SCRATCH_C);
    SetAccumulatorWidth(cpu, 0);
    cpu->x = PullIndexValue(memory, cpu);
    LoadA16(cpu, cpu->y);                                      /* TYA */
    do {
        LoadY16(cpu, Read16Direct(memory, cpu, DP_SCRATCH_C));
        PushIndex(memory, cpu);
        do {
            Write16Long(memory, LongIndexedAddress(WRAM_MUSIC_SAMPLE_CACHE, cpu->x),
                cpu->accumulator);
            IncrementX16(cpu);
            IncrementX16(cpu);
            IncrementA16(cpu);
            cpu->y = (uint16_t)(cpu->y - 1u);
            SetNz16(cpu, cpu->y);
        } while (!cpu->zero);
        TransferAToY(cpu);
        PullAccumulator16(memory, cpu);
        cpu->carry = 0;
        Add16Value(cpu, 0x0040u);
        TransferAToX(cpu);
        LoadA16(cpu, cpu->y);
        Decrement16Direct(memory, cpu, DP_SCRATCH_E);
    } while (!cpu->zero);
}

/* $82:91AB: A (max 999) into hundreds $00, tens $02, ones $04. */
static void MenuDigits3(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint8_t kSlots[3] = {0x00u, 0x02u, 0x04u};
    static const uint16_t kUnits[2] = {0x0064u, 0x000au};
    static const uint16_t kReturns[3] = {0x91c0u, 0x91c8u, 0x91d0u};
    unsigned i;

    for (i = 0; i < 3u; ++i)
        Write16Direct(memory, cpu, kSlots[i], 0);
    Compare16(cpu, cpu->accumulator, 0x03e8u);
    if (cpu->carry)
        LoadA16(cpu, 0x03e7u);
    for (i = 0; i < 2u; ++i) {
        Compare16(cpu, cpu->accumulator, kUnits[i]);
        if (!cpu->carry)
            continue;
        SimulateJsrFrame(memory, cpu, kReturns[i]);
        do {
            Increment16Direct(memory, cpu, kSlots[i]);
            Subtract16(cpu, kUnits[i]);
            Compare16(cpu, cpu->accumulator, kUnits[i]);
        } while (cpu->carry);
        SimulateRtsFrame(memory, cpu);
    }
    Compare16(cpu, cpu->accumulator, 0x0000u);
    if (!cpu->zero) {
        SimulateJsrFrame(memory, cpu, kReturns[2]);
        do {
            Increment16Direct(memory, cpu, 0x04u);
            LoadA16(cpu, (uint16_t)(cpu->accumulator - 1u));
        } while (!cpu->zero);
        SimulateRtsFrame(memory, cpu);
    }
}

/* $82:91F0: tens and ones tile at $7E:3002,X. */
static void MenuNumberTail(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadA16(cpu, Read16Direct(memory, cpu, 0x02u));
    if (!cpu->zero) {
        StoreADirect16(memory, cpu, 0x15u);
        LoadA16(cpu, 0x0060u);
        StoreADirect16(memory, cpu, 0x11u);
        LoadA16(cpu, 0x0000u);
        do {
            cpu->carry = 0;
            Add16Value(cpu, 0x0010u);
            Decrement16Direct(memory, cpu, 0x15u);
        } while (!cpu->zero);
    }
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x11u));
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x04u));
    LoadA16(cpu, (uint16_t)(cpu->accumulator |
                            Read16AbsoluteIndexed(memory, cpu,
                                                  WRAM_MENU_TILE_ATTRIBUTES, 0)));
    Write16Long(memory, LongIndexedAddress(0x7e3002u, cpu->x), cpu->accumulator);
}

/* $82:9169: small 3-digit number at $7E:3000,X. */
static void MenuSmallNumber(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Jsr(memory, cpu, 0x916bu);
    MenuDigits3(memory, cpu);
    Rts(memory, cpu);
    LoadA16(cpu, 0x0050u);
    StoreADirect16(memory, cpu, 0x11u);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x00u));
    if (!cpu->zero) {
        cpu->carry = 0;
        Add16Value(cpu, 0x0050u);
        LoadA16(cpu, (uint16_t)(cpu->accumulator |
                                Read16AbsoluteIndexed(memory, cpu,
                                                      WRAM_MENU_TILE_ATTRIBUTES, 0)));
        Write16Long(memory, LongIndexedAddress(WRAM_FIELD_LAYER2_TILEMAP, cpu->x),
            cpu->accumulator);
        LoadA16(cpu, 0x0060u);
        StoreADirect16(memory, cpu, 0x11u);
    }
    Jsr(memory, cpu, 0x9187u);
    MenuNumberTail(memory, cpu);
    Rts(memory, cpu);
}

/* $82:E4D5: seven stats of each member as small numbers; M1. */
static void MenuPartyStats(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_MENU_PARTY_MEMBER_COUNT, 0));
    And16(cpu, 0x00ffu);
    StoreADirect16(memory, cpu, 0x26u);
    LoadA16(cpu, 0x3d00u);
    StoreAAbsolute16(memory, cpu, WRAM_MENU_TILE_ATTRIBUTES, 0);
    LoadX16(cpu, 0x0000u);
    do {
        PushIndex(memory, cpu);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu,
            WRAM_MENU_PARTY_CHARACTER_OFFSET, cpu->x));
        cpu->carry = 0;
        Add16Value(cpu, 0x0029u);
        StoreADirect16(memory, cpu, 0x2au);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x82e51au, cpu->x)));
        TransferAToX(cpu);
        Jsr(memory, cpu, 0xe4f9u);                             /* E504 */
        LoadY16(cpu, 0x0000u);
        do {
            LoadA16(cpu, Read16Long(memory, (((uint32_t)cpu->data_bank << 16) +
                Read16Direct(memory, cpu, 0x2au) + cpu->y) & 0x00ffffffu));
            Jsr(memory, cpu, 0xe50bu);
            MenuSmallNumber(memory, cpu);
            Rts(memory, cpu);
            TransferXToA(cpu);
            cpu->carry = 0;
            Add16Value(cpu, 0x0040u);
            TransferAToX(cpu);
            IncrementY16(cpu);
            IncrementY16(cpu);
            Compare16(cpu, cpu->y, 0x000eu);
        } while (!cpu->zero);
        Rts(memory, cpu);
        cpu->x = PullIndexValue(memory, cpu);
        IncrementX16(cpu);
        IncrementX16(cpu);
        Decrement16Direct(memory, cpu, 0x26u);
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);
}

/* Test-and-set bits in a direct-page byte: A takes the bits, zero reports
 * whether any were already set. */
static void TsbDirect(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t offset, uint8_t bits) {
    const uint8_t old = DirectByte(memory, cpu, offset);

    LoadA8(cpu, bits);
    cpu->zero = (old & bits) == 0;
    Write8(memory, DirectAddress(cpu, offset), (uint8_t)(old | bits));
}

/* $82:D749: shop party screen with member stats. */
Lufia2ExecutionResult Lufia2MenuShopParty(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult text;

    LoadX16(cpu, 0xf7f0u);
    SimulateJslFrame(memory, cpu, 0x82u, 0xd74fu);
    MenuClearSlots(memory, cpu, MENU_SPRITE_ACTIVE);
    SimulateRtlFrame(memory, cpu);
    if (Text8E(memory, cpu, 0xccfdu, 0xd75fu, &text))
        return text;
    StoreA8Absolute(memory, cpu, WRAM_MENU_DRAW_MODE, 0x21u);
    LoadA8(cpu, 0x8eu);
    StoreADirect8(memory, cpu, 0x5fu);
    LoadY16(cpu, 0xcd27u);
    if (DrawString(memory, cpu, 0xd76fu, &text))
        return text;
    Jsr(memory, cpu, 0xd772u);
    MenuPartyStats(memory, cpu);
    Rts(memory, cpu);
    LoadA8(cpu, 0x01u);
    LoadX16(cpu, 0x0005u);
    Jsr(memory, cpu, 0xd77au);
    MenuCursorAnimation(memory, cpu);
    Rts(memory, cpu);
    LoadY16(cpu, 0x0070u);
    LoadX16(cpu, 0x0000u);
    Jsr(memory, cpu, 0xd783u);
    MenuCursorPlace(memory, cpu);
    Rts(memory, cpu);
    LoadAAbsolute8(memory, cpu, WRAM_MENU_SHOP_LIST_COUNT, 0);
    StoreAAbsolute8(memory, cpu, MENU_CURSOR_ROWS, 0);
    LoadX16(cpu, 0x0001u);
    StoreZeroAbsolute8(memory, cpu, MENU_CURSOR_X_INDEX, cpu->x);
    StoreZeroAbsolute8(memory, cpu, MENU_CURSOR_Y_INDEX, cpu->x);
    LoadA8(cpu, 0x0au);
    Jsr(memory, cpu, 0xd797u);
    MenuWindowHdma(memory, cpu);
    Rts(memory, cpu);
    TsbDirect(memory, cpu, 0x74u, 0x88u);
    return ExecutionReturned(0x82d79cu);
}

/* $82:D1A4: capsule portraits and the seven present marks. */
static void CapsulePortraits(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint16_t kBlocks[3][4] = {
        {0x0358u, 0x020au, 0x1100u, 0xd1b1u},
        {0x0378u, 0x020au, 0x1120u, 0xd1bdu},
        {0x055cu, 0x0e02u, 0x1140u, 0xd1c9u}};
    unsigned i;

    SetAccumulatorWidth(cpu, 0);
    for (i = 0; i < 3u; ++i) {
        LoadA16(cpu, kBlocks[i][0]);
        LoadX16(cpu, kBlocks[i][1]);
        LoadY16(cpu, kBlocks[i][2]);
        Jsr(memory, cpu, kBlocks[i][3]);
        MenuTileBlock(memory, cpu);
        Rts(memory, cpu);
    }
    Write16Direct(memory, cpu, 0x04u, 0);
    do {
        const uint16_t step = Read16Direct(memory, cpu, 0x04u);

        LoadA16(cpu, step);
        LsrA16(cpu);
        LsrA16(cpu);
        cpu->carry = 0;
        Add16Value(cpu, 0x11bbu);
        TransferAToX(cpu);
        LoadA16(cpu, (uint16_t)(Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x) & 0x0008u));
        AslA16(cpu);
        StoreADirect16(memory, cpu, 0x00u);
        LoadA16(cpu, (uint16_t)(Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x) & 0x0007u));
        AslA16(cpu);
        StoreADirect16(memory, cpu, 0x02u);
        LoadX16(cpu, Read16Direct(memory, cpu, 0x02u));
        LoadY16(cpu, Read16Long(memory, LongIndexedAddress(0x82d230u, cpu->x)));
        LoadA16(cpu, (uint16_t)(step + 0x049cu));
        LoadX16(cpu, 0x0203u);
        Jsr(memory, cpu, 0xd1f9u);
        MenuTileBlock(memory, cpu);
        Rts(memory, cpu);
        LoadX16(cpu, Read16Direct(memory, cpu, 0x02u));
        LoadY16(cpu, Read16Long(memory, LongIndexedAddress(0x82d23cu, cpu->x)));
        LoadA16(cpu, (uint16_t)(Read16Direct(memory, cpu, 0x04u) + 0x03dcu));
        LoadX16(cpu, 0x0203u);
        Jsr(memory, cpu, 0xd20cu);
        MenuTileBlock(memory, cpu);
        Rts(memory, cpu);
        LoadY16(cpu, (uint16_t)(Read16Direct(memory, cpu, 0x00u) + 0x116cu));
        LoadA16(cpu, (uint16_t)(Read16Direct(memory, cpu, 0x04u) + 0x035cu));
        LoadX16(cpu, 0x0202u);
        Jsr(memory, cpu, 0xd21fu);
        MenuTileBlock(memory, cpu);
        Rts(memory, cpu);
        LoadA16(cpu, (uint16_t)(Read16Direct(memory, cpu, 0x04u) + 0x0004u));
        StoreADirect16(memory, cpu, 0x04u);
        Compare16(cpu, cpu->accumulator, 0x001cu);
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);
}

/* $82:A2E3: capsule monster screen. */
Lufia2ExecutionResult Lufia2MenuCapsuleScreen(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, 0x03d8u);
    LoadX16(cpu, 0x1209u);
    Jsr(memory, cpu, 0xa2edu);
    MenuClearRect2(memory, cpu);
    Rts(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x05u);
    cpu->carry = 1;
    Sbc8(cpu, AbsoluteByte(memory, cpu, 0x11a4u, 0));
    StoreAAbsolute8(memory, cpu, 0x14f0u, 0);
    LoadAAbsolute8(memory, cpu, 0x11a3u, 0);
    StoreAAbsolute8(memory, cpu, 0x14deu, 0);
    LoadA8(cpu, 0x08u);
    LoadX16(cpu, 0x000au);
    Jsr(memory, cpu, 0xa306u);
    MenuCursorAnimation(memory, cpu);
    Rts(memory, cpu);
    LoadY16(cpu, 0x005bu);
    LoadX16(cpu, 0x0005u);
    Jsr(memory, cpu, 0xa30fu);
    MenuCursorPlace(memory, cpu);
    Rts(memory, cpu);
    Jsr(memory, cpu, 0xa312u);
    CapsulePortraits(memory, cpu);
    Rts(memory, cpu);
    TsbDirect(memory, cpu, 0x74u, 0x88u);
    return ExecutionReturned(0x82a317u);
}

/* $82:9189: small 3-digit number, hundreds tile always drawn. */
static void MenuSmallNumberFull(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Jsr(memory, cpu, 0x918bu);
    MenuDigits3(memory, cpu);
    Rts(memory, cpu);
    LoadA16(cpu, 0x0050u);
    StoreADirect16(memory, cpu, 0x11u);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x00u));
    cpu->carry = 0;
    Add16Value(cpu, 0x0040u);
    LoadA16(cpu, (uint16_t)(cpu->accumulator |
                            Read16AbsoluteIndexed(memory, cpu,
                                                  WRAM_MENU_TILE_ATTRIBUTES, 0)));
    Write16Long(memory, LongIndexedAddress(WRAM_FIELD_LAYER2_TILEMAP, cpu->x),
        cpu->accumulator);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x00u));
    if (!cpu->zero) {
        LoadA16(cpu, 0x0060u);
        StoreADirect16(memory, cpu, 0x11u);
    }
    Jsr(memory, cpu, 0x91a9u);
    MenuNumberTail(memory, cpu);
    Rts(memory, cpu);
}

/* $82:950E: level, HP and MP of member [$2A]. */
Lufia2ExecutionResult Lufia2MenuMemberStatus(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const struct {
        uint16_t field;
        int full;
        uint16_t ret;
    } kValues[4] = {
        {0x0027u, 1, 0x956fu}, {0x0013u, 0, 0x9576u},
        {0x0025u, 1, 0x957du}, {0x0011u, 0, 0x9584u}};
    unsigned i;

    OpRepWidths(cpu, 0x20u);
    OpTxa(cpu);
    cpu->carry = 0;
    OpAdcValue(cpu, 0x40u);
    PushAccumulator16(memory, cpu);
    cpu->carry = 0;
    OpAdcValue(cpu, 4u);
    PushAccumulator16(memory, cpu);
    OpTxa(cpu);
    cpu->carry = 0;
    OpAdcValue(cpu, 0x80u);
    PushAccumulator16(memory, cpu);
    cpu->carry = 0;
    OpAdcValue(cpu, 4u);
    PushAccumulator16(memory, cpu);
    OpTxa(cpu);
    cpu->carry = 0;
    OpAdcValue(cpu, 4u);
    PushAccumulator16(memory, cpu);
    LoadY16(cpu, Read16Direct(memory, cpu, 0x2au));
    LoadA16(cpu, 0x3900u);
    StoreAAbsolute16(memory, cpu, WRAM_MENU_TILE_ATTRIBUTES, 0);
    cpu->x = PullIndexValue(memory, cpu);
    LoadA16(cpu, (uint16_t)(Read16AbsoluteIndexed(memory, cpu, 0x000eu, cpu->y) & 0x00ffu));
    Jsr(memory, cpu, 0x953du);
    MenuSmallNumber(memory, cpu);
    Rts(memory, cpu);
    OpLda(memory, cpu, OpAbsY(cpu, 0x000fu));
    OpBitValue(cpu, 5u);
    if (!cpu->zero) {
        OpLoadA(cpu, 0x3500u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_TILE_ATTRIBUTES));
    } else {
        OpLda(memory, cpu, OpAbsY(cpu, 0x0025u));
        OpLsrA(cpu);
        OpLsrA(cpu);
        OpLsrA(cpu);
        OpCmp(memory, cpu, OpAbsY(cpu, 0x0011u));
        if (cpu->carry) {
            OpLoadA(cpu, 0x3500u);
            OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_TILE_ATTRIBUTES));
        } else {
            OpLda(memory, cpu, OpAbsY(cpu, 0x0025u));
            OpLsrA(cpu);
            OpLsrA(cpu);
            OpCmp(memory, cpu, OpAbsY(cpu, 0x0011u));
            if (cpu->carry) {
                OpLoadA(cpu, 0x3100u);
                OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_TILE_ATTRIBUTES));
            }
        }
    }
    for (i = 0; i < 4u; ++i) {
        cpu->x = PullIndexValue(memory, cpu);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, kValues[i].field, cpu->y));
        Jsr(memory, cpu, kValues[i].ret);
        if (kValues[i].full)
            MenuSmallNumberFull(memory, cpu);
        else
            MenuSmallNumber(memory, cpu);
        Rts(memory, cpu);
    }
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x829587u);
}

/* $82:C3B1: X = name of skill A ($A5:DF00). */
static void CapsuleSkillName(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0xa5df00u, cpu->x)));
    cpu->carry = 0;
    Add16Value(cpu, 0xdf00u);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
}

/* JSL $80:8878 with the string at $8E:Y; 1 when it handed off. */
static int String8E(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t string, uint16_t return_address, Lufia2ExecutionResult *out) {
    LoadA8(cpu, 0x8eu);
    StoreADirect8(memory, cpu, 0x5fu);
    LoadY16(cpu, string);
    return DrawString(memory, cpu, return_address, out);
}

/* $82:D07B: capsule status: stats, class and three skills. */
Lufia2ExecutionResult Lufia2MenuCapsuleStatus(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const uint16_t kSkillReturns[3] = {0xd0e8u, 0xd0f2u, 0xd0fcu};
    static const uint16_t kSkillRows[3] = {0x3364u, 0x32e4u, 0x3264u};
    Lufia2ExecutionResult text;
    unsigned i;

    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, 0x0098u);
    LoadX16(cpu, 0x140eu);
    Jsr(memory, cpu, 0xd085u);
    MenuClearRect2(memory, cpu);
    Rts(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    StoreA8Absolute(memory, cpu, WRAM_MENU_DRAW_MODE, 0x3cu);
    if (String8E(memory, cpu, 0xc6cdu, 0xd097u, &text))
        return text;
    Jsr(memory, cpu, 0xd09au);
    Lufia2CapsuleRecordPointer(memory, cpu);
    Rts(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);                     /* PHK PLA */
    LoadA8(cpu, Pull8(memory, cpu));
    StoreADirect8(memory, cpu, 0x0au);
    LoadX16(cpu, 0x10dfu);
    StoreXDirect16(memory, cpu, 0x08u);
    LoadA8(cpu, 0x8eu);
    StoreADirect8(memory, cpu, 0x5fu);
    LoadY16(cpu, 0xc8dbu);
    LoadX16(cpu, 0x3064u);
    if (DrawString(memory, cpu, 0xd0b1u, &text))
        return text;
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x09c4u, 0));
    LoadA8(cpu, 0x97u);
    StoreADirect8(memory, cpu, 0x0au);
    StoreXDirect16(memory, cpu, 0x08u);
    PushDataBank(memory, cpu);
    SetBank(memory, cpu, 0x97u);
    LoadAAbsolute8(memory, cpu, 0x000eu, cpu->x);
    StoreADirect8(memory, cpu, 0x00u);
    LoadAAbsolute8(memory, cpu, 0x000du, cpu->x);
    StoreADirect8(memory, cpu, 0x01u);
    PullDataBank(memory, cpu);
    if (String8E(memory, cpu, 0xc85eu, 0xd0d5u, &text))
        return text;
    LoadA8(cpu, 0x8eu);
    StoreADirect8(memory, cpu, 0x5fu);
    LoadY16(cpu, 0xc8dbu);
    LoadX16(cpu, 0x31e4u);
    if (DrawString(memory, cpu, 0xd0e3u, &text))
        return text;
    for (i = 0; i < 3u; ++i) {
        LoadA8(cpu, (uint8_t)(2u - i));
        Jsr(memory, cpu, kSkillReturns[i]);
        Lufia2CapsuleSkill(memory, cpu);
        Rts(memory, cpu);
        LoadX16(cpu, kSkillRows[i]);
        PushIndex(memory, cpu);
        PushAccumulator8(memory, cpu);
    }
    LoadA8(cpu, 0xa5u);
    StoreADirect8(memory, cpu, 0x0au);
    LoadA8(cpu, 0x03u);
    StoreADirect8(memory, cpu, 0x15u);
    do {
        LoadA8(cpu, Pull8(memory, cpu));
        Jsr(memory, cpu, 0xd10du);
        CapsuleSkillName(memory, cpu);
        Rts(memory, cpu);
        StoreXDirect16(memory, cpu, 0x08u);
        LoadA8(cpu, 0x8eu);
        StoreADirect8(memory, cpu, 0x5fu);
        LoadY16(cpu, 0xc8dbu);
        LoadX16(cpu, PullIndexValue(memory, cpu));             /* PLX */
        if (DrawString(memory, cpu, 0xd11bu, &text))
            return text;
        {
            const uint8_t left = (uint8_t)(DirectByte(memory, cpu, 0x15u) - 1u);

            Write8(memory, DirectAddress(cpu, 0x15u), left);
            SetNz8(cpu, left);
        }
    } while (!cpu->zero);
    return ExecutionReturned(0x82d120u);
}

/* $82:E46B / $82:E484: count words to 0 or bytes to $FF at [$08]. */
static void ShopCount(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    int bytes) {
    SetAccumulatorWidth(cpu, 0);
    LoadY16(cpu, 0xffffu);
    do {
        IncrementY16(cpu);
        LoadA16(cpu, Read16Long(memory, DirectLongPointer(memory, cpu, 0x08u)));
        if (bytes)
            And16(cpu, 0x00ffu);
        Increment16Direct(memory, cpu, 0x08u);
        if (!bytes)
            Increment16Direct(memory, cpu, 0x08u);
        Compare16(cpu, cpu->accumulator, bytes ? 0x00ffu : 0x0000u);
    } while (!cpu->zero);
    LoadA16(cpu, cpu->y);
    Write16Long(memory, LongIndexedAddress(0x7e93d0u, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
}

/* $82:E319: list X starts at [$08]. */
static void ShopList(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const uint32_t lists = AbsoluteIndexedAddress(cpu, WRAM_MENU_SHOP_LIST_COUNT, 0);

    Write8(memory, lists, (uint8_t)(Read8(memory, lists) + 1u));
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x08u));
    Write16Long(memory, LongIndexedAddress(0x7e93c0u, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
}

/* $82:FC7D: A = owned count nibble of item X ($7F:F080). */
static void ItemCount(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    TransferXToA(cpu);
    And16(cpu, 0x01ffu);
    if (cpu->zero)
        return;
    LsrA16(cpu);
    TransferAToX(cpu);
    {
        const int high = cpu->carry;

        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7ff080u, cpu->x)));
        if (!high) {
            And16(cpu, 0x000fu);
            return;
        }
        And16(cpu, 0x00f0u);
        LsrA16(cpu);
        LsrA16(cpu);
        LsrA16(cpu);
        LsrA16(cpu);
    }
}

/* $82:E433: owned items (count << 9 | id) into $7E:97E0, 0-terminated. */
static void ShopOwnedItems(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    SetBank(memory, cpu, 0x7eu);
    SetAccumulatorWidth(cpu, 0);
    Write16Direct(memory, cpu, 0x00u, 0);
    LoadY16(cpu, 0x0000u);
    do {
        LoadX16(cpu, Read16Direct(memory, cpu, 0x00u));
        Jsr(memory, cpu, 0xe443u);
        ItemCount(memory, cpu);
        Rts(memory, cpu);
        Compare16(cpu, cpu->accumulator, 0x0000u);
        if (!cpu->zero) {
            ExchangeAccumulatorBytes(cpu);
            AslA16(cpu);
            LoadA16(cpu, (uint16_t)(cpu->accumulator | Read16Direct(memory, cpu, 0x00u)));
            StoreAAbsolute16(memory, cpu, 0x97e0u, cpu->y);
            IncrementY16(cpu);
            IncrementY16(cpu);
        }
        Increment16Direct(memory, cpu, 0x00u);
        LoadA16(cpu, Read16Direct(memory, cpu, 0x00u));
        Compare16(cpu, cpu->accumulator, 0x0200u);
    } while (!cpu->zero);
    LoadA16(cpu, cpu->y);
    LsrA16(cpu);
    Write16Long(memory, 0x7e93d0u, cpu->accumulator);
    LoadA16(cpu, 0x0000u);
    StoreAAbsolute16(memory, cpu, 0x97e0u, cpu->y);
    SetAccumulatorWidth(cpu, 1);
    PullDataBank(memory, cpu);
}

/* $82:E297: shop $30: kind, flags and item lists. */
Lufia2ExecutionResult Lufia2MenuShopSetup(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const uint16_t kReturns[4][2] = {
        {0xe2cau, 0xe2cdu}, {0xe2dau, 0xe2ddu}, {0xe2eau, 0xe2edu},
        {0xe2fau, 0xe2fdu}};
    unsigned i;

    Jsr(memory, cpu, 0xe299u);                                 /* E729 */
    LoadX16(cpu, 0xee9fu);
    StoreXDirect16(memory, cpu, 0x08u);
    LoadA8(cpu, 0x97u);
    StoreADirect8(memory, cpu, 0x0au);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, (uint16_t)(Read16Direct(memory, cpu, 0x30u) & 0x00ffu));
    AslA16(cpu);
    TransferAToY(cpu);
    LoadA16(cpu, Read16IndirectLongY(memory, cpu, 0x08u));
    cpu->carry = 0;
    Add16Value(cpu, 0xee9fu);
    StoreADirect16(memory, cpu, 0x08u);
    SetAccumulatorWidth(cpu, 1);
    Rts(memory, cpu);
    LoadA8(cpu, Read8(memory, DirectLongPointer(memory, cpu, 0x08u)));
    StoreAAbsolute8(memory, cpu, 0x1543u, 0);
    SetAccumulatorWidth(cpu, 0);
    Increment16Direct(memory, cpu, 0x08u);
    LoadA16(cpu, Read16Long(memory, DirectLongPointer(memory, cpu, 0x08u)));
    StoreAAbsolute16(memory, cpu, MENU_SHOP_FLAGS, 0);
    Increment16Direct(memory, cpu, 0x08u);
    Increment16Direct(memory, cpu, 0x08u);
    SetAccumulatorWidth(cpu, 1);
    StoreZeroAbsolute8(memory, cpu, WRAM_MENU_SHOP_LIST_COUNT, 0);
    LoadAAbsolute8(memory, cpu, MENU_SHOP_FLAGS, 0);
    if (A8(cpu) & 0x01u) {
        const uint32_t lists =
            AbsoluteIndexedAddress(cpu, WRAM_MENU_SHOP_LIST_COUNT, 0);

        Write8(memory, lists, (uint8_t)(Read8(memory, lists) + 1u));
        Jsr(memory, cpu, 0xe2bdu);
        ShopOwnedItems(memory, cpu);
        Rts(memory, cpu);
    }
    for (i = 0; i < 5u; ++i) {                                 /* bits 1-5 */
        LoadAAbsolute8(memory, cpu, MENU_SHOP_FLAGS, 0);
        if (!(A8(cpu) & (0x02u << i)))
            continue;
        LoadX16(cpu, (uint16_t)(2u + 2u * i));
        Jsr(memory, cpu, i < 4u ? kReturns[i][0] : 0xe30au);
        ShopList(memory, cpu);
        Rts(memory, cpu);
        Jsr(memory, cpu, i < 4u ? kReturns[i][1] : 0xe30du);
        ShopCount(memory, cpu, i == 4u);
        Rts(memory, cpu);
    }
    LoadAAbsolute8(memory, cpu, MENU_SHOP_FLAGS, 0);
    BitImmediate8(cpu, 0x80u);
    if (!cpu->zero) {
        const uint32_t lists =
            AbsoluteIndexedAddress(cpu, WRAM_MENU_SHOP_LIST_COUNT, 0);
        const uint8_t count = (uint8_t)(Read8(memory, lists) + 1u);

        Write8(memory, lists, count);
        SetNz8(cpu, count);
    }
    return ExecutionReturned(0x82e318u);
}

/* $82:E624: stats of the new item vs now, green or red. */
static void ShopCompareStats(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadY16(cpu, 0x0000u);
    do {
        uint16_t colour = 0;

        LoadA16(cpu, Indirect16At(memory, cpu, 0xedu, 0));
        And16(cpu, 0xff00u);
        if (!cpu->zero) {
            const uint16_t now = Indirect16At(memory, cpu, 0xe7u, cpu->y);
            const uint16_t next = Indirect16At(memory, cpu, 0xeau, cpu->y);

            LoadA16(cpu, now);
            Compare16(cpu, now, next);
            if (!cpu->zero)
                colour = cpu->carry ? 0x3500u : 0x3100u;
        }
        if (!colour) {
            LoadA16(cpu, 0x0000u);                             /* E640 */
            Write16Long(memory, LongIndexedAddress(WRAM_FIELD_LAYER2_TILEMAP, cpu->x),
                0);
            Write16Long(memory, LongIndexedAddress(0x7e3002u, cpu->x), 0);
        } else {
            LoadA16(cpu, colour);
            StoreAAbsolute16(memory, cpu, WRAM_MENU_TILE_ATTRIBUTES, 0);
            LoadA16(cpu, Indirect16At(memory, cpu, 0xeau, cpu->y));
            Jsr(memory, cpu, 0xe655u);
            MenuSmallNumberFull(memory, cpu);
            Rts(memory, cpu);
        }
        TransferXToA(cpu);                                     /* E656 */
        cpu->carry = 0;
        Add16Value(cpu, 0x0040u);
        TransferAToX(cpu);
        IncrementY16(cpu);
        IncrementY16(cpu);
        Compare16(cpu, cpu->y, 0x000eu);
    } while (!cpu->zero);
}

/* $82:E664: member sprite X pose by equip flags. */
static void ShopMemberPose(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    uint8_t pose;

    LoadA16(cpu, Indirect16At(memory, cpu, 0xedu, 0));
    SetAccumulatorWidth(cpu, 1);
    if (A8(cpu) == 0) {
        pose = 0x00u;
    } else {
        ExchangeAccumulatorBytes(cpu);
        pose = A8(cpu) == 0x02u ? 0x03u : A8(cpu) == 0x01u ? 0x00u : 0x02u;
    }
    LoadA8(cpu, pose);
    StoreZeroAbsolute8(memory, cpu, MENU_SPRITE_FRAME_INDEX, cpu->x);
    SimulateJslFrame(memory, cpu, 0x82u, 0xe685u);
    (void)Lufia2SpriteSetAnimation(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
}

/* $82:E5E1: equipment comparison for each member. */
Lufia2ExecutionResult Lufia2MenuShopCompare(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const uint8_t kPointers[3] = {0xe7u, 0xeau, 0xedu};
    static const uint16_t kOffsets[3] = {0x0029u, 0x0037u, 0x0048u};
    unsigned i;

    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, (uint16_t)(Read16AbsoluteIndexed(memory, cpu,
        WRAM_MENU_PARTY_MEMBER_COUNT, 0) & 0x00ffu));
    StoreADirect16(memory, cpu, 0x26u);
    LoadX16(cpu, 0x0000u);
    do {
        PushIndex(memory, cpu);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu,
            WRAM_MENU_PARTY_CHARACTER_OFFSET, cpu->x));
        StoreADirect16(memory, cpu, 0x2au);
        for (i = 0; i < 3u; ++i) {
            LoadA16(cpu, (uint16_t)(Read16Direct(memory, cpu, 0x2au) + kOffsets[i]));
            cpu->carry = 0;
            StoreADirect16(memory, cpu, kPointers[i]);
        }
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x82e689u, cpu->x)));
        TransferAToX(cpu);
        Jsr(memory, cpu, 0xe611u);
        ShopCompareStats(memory, cpu);
        Rts(memory, cpu);
        cpu->x = PullIndexValue(memory, cpu);
        TransferXToA(cpu);
        LsrA16(cpu);
        PushIndex(memory, cpu);
        TransferAToX(cpu);
        Jsr(memory, cpu, 0xe619u);
        ShopMemberPose(memory, cpu);
        Rts(memory, cpu);
        cpu->x = PullIndexValue(memory, cpu);
        IncrementX16(cpu);
        IncrementX16(cpu);
        Decrement16Direct(memory, cpu, 0x26u);
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x82e623u);
}

/* $82:98C7: 24-bit $09BD = X * 200. */
static void ShopPriceTimes200(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint8_t kProducts[3] = {0x54u, 0x56u, 0x58u};
    unsigned i;

    Write16Absolute(memory, cpu, 0x09bdu, cpu->x);
    StoreZeroAbsolute8(memory, cpu, 0x09bfu, 0);
    StoreA8Absolute(memory, cpu, SNES_WRMPYA, 0xc8u);
    for (i = 0; i < 3u; ++i) {
        if (i < 2u)
            LoadAAbsolute8(memory, cpu, (uint16_t)(0x09bdu + i), 0);
        else
            LoadA8(cpu, 0x00u);
        StoreAAbsolute8(memory, cpu, SNES_WRMPYB, 0);
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, SNES_RDMPYL, 0));
        StoreXDirect16(memory, cpu, kProducts[i]);
    }
    LoadA8(cpu, DirectByte(memory, cpu, DP_SCRATCH_B));
    cpu->carry = 0;
    Adc8(cpu, DirectByte(memory, cpu, DP_SCRATCH_C));
    StoreADirect8(memory, cpu, DP_SCRATCH_B);
    LoadA8(cpu, DirectByte(memory, cpu, DP_SCRATCH_D));
    Adc8(cpu, DirectByte(memory, cpu, DP_SCRATCH_E));
    StoreADirect8(memory, cpu, DP_SCRATCH_C);
    for (i = 0; i < 3u; ++i) {
        LoadA8(cpu, DirectByte(memory, cpu, (uint8_t)(0x54u + i)));
        StoreAAbsolute8(memory, cpu, (uint16_t)(0x09bdu + i), 0);
    }
}

/* $82:85D2: $15 shop rows from list [$FC] at index $00: name, price. */
static int ShopRows(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2ExecutionResult *text) {
    StoreA8Absolute(memory, cpu, WRAM_MENU_DRAW_MODE, 0x20u);
    LoadA8(cpu, 0x8eu);
    StoreADirect8(memory, cpu, 0x5fu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x00u));
    AslA16(cpu);
    TransferAToX(cpu);
    do {
        StoreXDirect16(memory, cpu, 0x19u);                    /* 85E1 */
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7e93e0u, cpu->x)));
        StoreADirect16(memory, cpu, 0x02u);
        LoadY16(cpu, cpu->x);
        LoadA16(cpu, Read16IndirectLongY(memory, cpu, 0xfcu));
        And16(cpu, 0x01ffu);
        if (cpu->zero)
            break;
        StoreADirect16(memory, cpu, 0x00u);
        SetAccumulatorWidth(cpu, 1);
        LoadY16(cpu, 0xc9b1u);
        LoadX16(cpu, Read16Direct(memory, cpu, 0x11u));
        if (DrawString(memory, cpu, 0x85fdu, text))
            return 1;
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0b89u, 0));
        Jsr(memory, cpu, 0x8603u);
        (void)Lufia2AdjustPurchasePrice(memory, cpu);
        Rts(memory, cpu);
        Write16Absolute(memory, cpu, 0x0b89u, cpu->x);
        LoadAAbsolute8(memory, cpu, 0x154cu, 0);
        Compare8(cpu, A8(cpu), 0x01u);
        if (cpu->zero) {
            LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0b89u, 0));
            Jsr(memory, cpu, 0x8613u);
            ShopPriceTimes200(memory, cpu);
            Rts(memory, cpu);
        }
        LoadY16(cpu, 0xc9c2u);
        LoadX16(cpu, Read16Direct(memory, cpu, 0x11u));
        if (DrawString(memory, cpu, 0x861cu, text))
            return 1;
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16Direct(memory, cpu, 0x11u));
        cpu->carry = 0;
        Add16Value(cpu, 0x0080u);
        StoreADirect16(memory, cpu, 0x11u);
        LoadX16(cpu, Read16Direct(memory, cpu, 0x19u));
        IncrementX16(cpu);
        IncrementX16(cpu);
        Decrement16Direct(memory, cpu, 0x15u);
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);                               /* 862F */
    TsbDirect(memory, cpu, 0x74u, 0x08u);
    return 0;
}

/* $82:DCF4: one shop row at $3708; M0. */
Lufia2ExecutionResult Lufia2MenuShopRow(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult text;

    LoadX16(cpu, 0x3708u);
    StoreXDirect16(memory, cpu, 0x11u);
    LoadX16(cpu, 0x0001u);
    StoreXDirect16(memory, cpu, 0x15u);
    SetAccumulatorWidth(cpu, 1);
    Jsr(memory, cpu, 0xdd02u);
    if (ShopRows(memory, cpu, &text))
        return text;
    Rts(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    return ExecutionReturned(0x82dd05u);
}

/* $82:DCC1: five shop rows from $3488. */
Lufia2ExecutionResult Lufia2MenuShopRows(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult text;

    LoadX16(cpu, 0x3488u);
    StoreXDirect16(memory, cpu, 0x11u);
    LoadX16(cpu, 0x0005u);
    StoreXDirect16(memory, cpu, 0x15u);
    Jsr(memory, cpu, 0xdccdu);
    if (ShopRows(memory, cpu, &text))
        return text;
    Rts(memory, cpu);
    return ExecutionReturned(0x82dcceu);
}

/* $82:F703: Y = equipment slot A offset ($66 + 2A), also $1523. */
static void EquipSlot(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    cpu->carry = 0;
    Add16Value(cpu, 0x0066u);
    StoreAAbsolute16(memory, cpu, 0x1523u, 0);
    TransferAToY(cpu);
}

/* $82:F893: carry = slot A holds an item with record flag 8. */
static void EquipFlagged(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t slot_return) {
    Jsr(memory, cpu, slot_return);
    EquipSlot(memory, cpu);
    Rts(memory, cpu);
    LoadA16(cpu, (uint16_t)(Indirect16At(memory, cpu, 0x2au, cpu->y) & 0x01ffu));
    StoreAAbsolute16(memory, cpu, WRAM_ITEM_RECORD_ID, 0);
    if (cpu->zero) {
        cpu->carry = 0;
        return;
    }
    PushY(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x82u, 0xf8a4u);
    (void)Lufia2ItemRecordByte(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);
    And16(cpu, 0x0008u);
    cpu->carry = !cpu->zero;
}

/* $82:F846: equipment bonuses of member [$2A] from its six slots. */
static void EquipBonuses(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    unsigned slot;

    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    Jsr(memory, cpu, 0xf84bu);
    Lufia2BonusClear(memory, cpu, 0x0086u, 7u);
    Rts(memory, cpu);
    for (slot = 0; slot < 6u; ++slot) {
        LoadA16(cpu, (uint16_t)slot);
        PushAccumulator16(memory, cpu);
        Jsr(memory, cpu, 0xf852u);
        EquipSlot(memory, cpu);
        Rts(memory, cpu);
        Jsr(memory, cpu, 0xf855u);                             /* F710 */
        LoadA16(cpu, Indirect16At(memory, cpu, 0x2au, cpu->y));
        if (cpu->zero) {
            cpu->carry = 1;
            Rts(memory, cpu);
        } else {
            And16(cpu, 0x01ffu);
            StoreAAbsolute16(memory, cpu, WRAM_ITEM_RECORD_ID, 0);
            SimulateJslFrame(memory, cpu, 0x82u, 0xf71du);
            (void)Lufia2LoadItemRecord(memory, cpu);
            SimulateRtlFrame(memory, cpu);
            cpu->carry = 0;
            Rts(memory, cpu);
            Jsr(memory, cpu, 0xf85au);                         /* F722 */
            {
                unsigned i;

                LoadX16(cpu, Read16Direct(memory, cpu, 0x2au));
                for (i = 0; i < 7u; ++i) {
                    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu,
                        (uint16_t)(0x0086u + 2u * i), cpu->x));
                    cpu->carry = 0;
                    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu,
                        (uint16_t)(0x0b95u + 2u * i), 0));
                    StoreAAbsolute16(memory, cpu, (uint16_t)(0x0086u + 2u * i), cpu->x);
                }
            }
            Rts(memory, cpu);
        }
        PullAccumulator16(memory, cpu);
        IncrementA16(cpu);
        Compare16(cpu, cpu->accumulator, 0x0006u);
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
}

/* $82:B2C5: upgrade flagged equipment of member $14B3. */
Lufia2ExecutionResult Lufia2MenuEquipUpgrade(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const uint16_t item = Read16AbsoluteIndexed(memory, cpu, WRAM_ITEM_RECORD_ID, 0);
    int changed = 0;

    LoadX16(cpu, item);
    PushIndex(memory, cpu);
    LoadAAbsolute8(memory, cpu, 0x0a62u, 0);
    Compare8(cpu, A8(cpu), 0x25u);
    if (cpu->zero) {
        LoadAAbsolute8(memory, cpu, 0x14b3u, 0);
        StoreADirect8(memory, cpu, 0x22u);
        Jsr(memory, cpu, 0xb2d7u);                             /* 9971 */
        PushY(memory, cpu);
        Push8(memory, cpu, PackStatus(cpu));
        SetAccumulatorWidth(cpu, 0);
        SetIndexWidth(cpu, 0);
        LoadA16(cpu, (uint16_t)(Read16Direct(memory, cpu, 0x22u) & 0x00ffu));
        AslA16(cpu);
        TransferAToY(cpu);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu,
            WRAM_MENU_PARTY_CHARACTER_OFFSET, cpu->y));
        StoreADirect16(memory, cpu, 0x2au);
        UnpackStatus(cpu, Pull8(memory, cpu));
        cpu->y = PullIndexValue(memory, cpu);
        Rts(memory, cpu);
        SetAccumulatorWidth(cpu, 0);
        Jsr(memory, cpu, 0xb2dcu);                             /* F87D count */
        LoadX16(cpu, 0x0000u);
        TransferXToA(cpu);
        do {
            PushIndex(memory, cpu);
            PushAccumulator16(memory, cpu);
            TransferXToA(cpu);
            Jsr(memory, cpu, 0xf886u);
            EquipFlagged(memory, cpu, 0xf895u);
            Rts(memory, cpu);
            PullAccumulator16(memory, cpu);
            Add16Value(cpu, 0x0000u);
            cpu->x = PullIndexValue(memory, cpu);
            IncrementX16(cpu);
            Compare16(cpu, cpu->x, 0x0006u);
        } while (!cpu->zero);
        Rts(memory, cpu);
        Compare16(cpu, cpu->accumulator, 0x0000u);
        if (!cpu->zero) {
            Jsr(memory, cpu, 0xb2e4u);                         /* F8C6 */
            LoadA16(cpu, 0x0000u);
            do {
                PushAccumulator16(memory, cpu);
                Jsr(memory, cpu, 0xf8ccu);
                EquipFlagged(memory, cpu, 0xf895u);
                Rts(memory, cpu);
                if (cpu->carry) {
                    const uint32_t at = (((uint32_t)cpu->data_bank << 16) +
                        Read16Direct(memory, cpu, 0x2au) + cpu->y) & 0x00ffffffu;
                    const uint16_t next = (uint16_t)(Read16Long(memory, at) + 1u);

                    Write16Long(memory, at, next);
                    SetNz16(cpu, next);
                }
                PullAccumulator16(memory, cpu);
                IncrementA16(cpu);
                Compare16(cpu, cpu->accumulator, 0x0006u);
            } while (!cpu->zero);
            Rts(memory, cpu);
            Jsr(memory, cpu, 0xb2e7u);
            EquipBonuses(memory, cpu);
            Rts(memory, cpu);
            LoadX16(cpu, Read16Direct(memory, cpu, 0x2au));
            SimulateJslFrame(memory, cpu, 0x82u, 0xb2edu);
            (void)Lufia2PartyDerivedStats(memory, cpu);
            SimulateRtlFrame(memory, cpu);
            changed = 1;
        }
    }
    SetAccumulatorWidth(cpu, 1);
    LoadX16(cpu, PullIndexValue(memory, cpu));
    Write16Absolute(memory, cpu, WRAM_ITEM_RECORD_ID, cpu->x);
    cpu->carry = !changed;
    return ExecutionReturned(changed ? 0x82b2f5u : 0x82b2fdu);
}

/* $82:89C4: two cursor sprites: X (animation $57, or off), $5A. */
static void MenuCursorPair(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    StoreXDirect16(memory, cpu, DP_SCRATCH_E);
    StoreYDirect16(memory, cpu, 0x5au);
    LoadA8(cpu, DirectByte(memory, cpu, DP_SCRATCH_D));
    if (cpu->zero) {
        StoreZeroAbsolute8(memory, cpu, MENU_SPRITE_ACTIVE, cpu->x);
    } else {
        Jsr(memory, cpu, 0x89d3u);
        MenuCursorAnimation(memory, cpu);
        Rts(memory, cpu);
    }
    LoadX16(cpu, (uint16_t)(Read16Direct(memory, cpu, 0x5au) - 5u));
    StoreXDirect16(memory, cpu, 0x63u);
    LoadA8(cpu, DirectByte(memory, cpu, DP_SCRATCH_C));
    LoadX16(cpu, Read16Direct(memory, cpu, 0x5au));
    Jsr(memory, cpu, 0x89e3u);
    MenuCursorAnimation(memory, cpu);
    Rts(memory, cpu);
    LoadY16(cpu, Read16Direct(memory, cpu, DP_SCRATCH_A));
    LoadX16(cpu, Read16Direct(memory, cpu, 0x63u));
    Jsr(memory, cpu, 0x89eau);
    MenuCursorPlace(memory, cpu);
    Rts(memory, cpu);
}

/* $82:8C9C: scrollbar thumb sprite (slot 11) at Y. */
static void MenuScrollThumb(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, cpu->y);
    SetAccumulatorWidth(cpu, 1);
    LoadX16(cpu, MENU_SPRITE_SCROLL_THUMB);
    StoreAAbsolute8(memory, cpu, MENU_SPRITE_Y_LOW, cpu->x);
    StoreAAbsolute8(memory, cpu, (WRAM_MENU_SCROLL_THUMB_Y + 1u), 0);
    ExchangeAccumulatorBytes(cpu);
    StoreAAbsolute8(memory, cpu, MENU_SPRITE_X_LOW, cpu->x);
    StoreZeroAbsolute8(memory, cpu, MENU_SPRITE_Y_HIGH, cpu->x);
    StoreZeroAbsolute8(memory, cpu, MENU_SPRITE_X_HIGH, cpu->x);
    StoreZeroAbsolute8(memory, cpu, WRAM_MENU_SCROLL_THUMB_Y, 0);
    StoreZeroAbsolute8(memory, cpu, MENU_SPRITE_FRAME_INDEX, cpu->x);
    LoadA8(cpu, 0x04u);
    SimulateJslFrame(memory, cpu, 0x82u, 0x8cbfu);
    (void)Lufia2SpriteSetAnimation(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $82:8CC1: thumb step = $150B / rows past the window. */
static void MenuScrollStep(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadA8(cpu, 0x00u);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_MENU_SCROLL_RANGE, 0));
    Compare16(cpu, cpu->x, 0x0000u);
    if (!cpu->negative && !cpu->zero) {
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_MENU_SCROLL_TRACK, 0));
        StoreXDirect16(memory, cpu, 0x4eu);
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_MENU_SCROLL_RANGE, 0));
        StoreXDirect16(memory, cpu, 0x51u);
        SimulateJslFrame(memory, cpu, 0x82u, 0x8cdau);
        (void)Lufia2Divide16(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        LoadX16(cpu, Read16Direct(memory, cpu, 0x4eu));
        Write16Absolute(memory, cpu, WRAM_MENU_SCROLL_STEP, cpu->x);
        LoadA8(cpu, 0x01u);
    }
    LoadX16(cpu, MENU_SPRITE_SCROLL_THUMB);
    StoreAAbsolute8(memory, cpu, MENU_SPRITE_ACTIVE, cpu->x);
}

/* $82:8C43: thumb y = $1509 + row * step; frame 4 at the ends. */
static void MenuScrollPlace(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadX16(cpu, MENU_SPRITE_SCROLL_THUMB);
    LoadAAbsolute8(memory, cpu, MENU_SPRITE_ACTIVE, cpu->x);
    if (cpu->zero)
        return;
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_MENU_SCROLL_ROW, 0));
    Write16Absolute(memory, cpu, WRAM_SYSTEM_MULTIPLY_A, cpu->y);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_MENU_SCROLL_STEP, 0));
    Write16Absolute(memory, cpu, WRAM_SYSTEM_MULTIPLY_B, cpu->x);
    Lufia2CallMultiply(memory, cpu, 0x82u, 0x8c5cu);
    LoadX16(cpu, MENU_SPRITE_SCROLL_THUMB);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_MENU_SCROLL_THUMB_Y, 0));
    cpu->carry = 0;
    Add16Value(cpu,
               Read16AbsoluteIndexed(memory, cpu, WRAM_SYSTEM_MULTIPLY_PRODUCT, 0));
    SetAccumulatorWidth(cpu, 1);
    ExchangeAccumulatorBytes(cpu);
    StoreAAbsolute8(memory, cpu, MENU_SPRITE_Y_LOW, cpu->x);
    LoadA8(cpu, 0x04u);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_MENU_SCROLL_ROW, 0));
    if (!cpu->zero) {
        Compare16(cpu, cpu->y,
                  Read16AbsoluteIndexed(memory, cpu, WRAM_MENU_SCROLL_RANGE, 0));
        if (!cpu->zero)
            LoadA8(cpu, 0x05u);
    }
    Compare8(cpu, A8(cpu), AbsoluteByte(memory, cpu, 0x1208u, cpu->x));
    if (cpu->zero)
        return;
    SimulateJslFrame(memory, cpu, 0x82u, 0x8c83u);
    (void)Lufia2SpriteSetAnimation(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $82:8C85: scroll range from X rows (carry: halve), then thumb. */
static void MenuScrollbar(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    TransferXToA(cpu);
    if (cpu->carry) {
        IncrementA16(cpu);
        LsrA16(cpu);
    }
    Subtract16(cpu,
               Read16AbsoluteIndexed(memory, cpu, WRAM_MENU_SCROLL_WINDOW_ROWS, 0));
    StoreAAbsolute16(memory, cpu, WRAM_MENU_SCROLL_RANGE, 0);
    SetAccumulatorWidth(cpu, 1);
    Jsr(memory, cpu, 0x8c97u);
    MenuScrollStep(memory, cpu);
    Rts(memory, cpu);
    Jsr(memory, cpu, 0x8c9au);
    MenuScrollPlace(memory, cpu);
    Rts(memory, cpu);
}

/* $82:8680: $15 rows of $7E:943C string pointers ($8E) from index $00. */
static int MenuPointerRows(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2ExecutionResult *text) {
    StoreA8Absolute(memory, cpu, WRAM_MENU_DRAW_MODE, 0x20u);
    LoadA8(cpu, 0x8eu);
    StoreADirect8(memory, cpu, 0x5fu);
    LoadA8(cpu, 0x8eu);
    StoreADirect8(memory, cpu, 0x0au);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x00u));
    AslA16(cpu);
    TransferAToX(cpu);
    do {
        StoreXDirect16(memory, cpu, 0x19u);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7e943cu, cpu->x)));
        Compare16(cpu, cpu->accumulator, 0xffffu);
        if (cpu->zero)
            break;
        StoreADirect16(memory, cpu, 0x08u);
        SetAccumulatorWidth(cpu, 1);
        LoadY16(cpu, 0xd062u);
        LoadX16(cpu, Read16Direct(memory, cpu, 0x11u));
        if (DrawString(memory, cpu, 0x86aau, text))
            return 1;
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16Direct(memory, cpu, 0x11u));
        cpu->carry = 0;
        Add16Value(cpu, 0x0080u);
        StoreADirect16(memory, cpu, 0x11u);
        LoadX16(cpu, Read16Direct(memory, cpu, 0x19u));
        IncrementX16(cpu);
        IncrementX16(cpu);
        Decrement16Direct(memory, cpu, 0x15u);
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);
    TsbDirect(memory, cpu, 0x74u, 0x08u);
    return 0;
}

/* $82:9CB2: warp destinations list with scrollbar. */
Lufia2ExecutionResult Lufia2MenuWarpList(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult text;

    LoadX16(cpu, 0xffb0u);
    SimulateJslFrame(memory, cpu, 0x82u, 0x9cb8u);
    MenuClearSlots(memory, cpu, MENU_SPRITE_ACTIVE);
    SimulateRtlFrame(memory, cpu);
    StoreA8Absolute(memory, cpu, 0x153fu, 0x01u);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, 0x0342u);
    LoadX16(cpu, 0x1e13u);
    Jsr(memory, cpu, 0x9cc8u);
    MenuClearRect(memory, cpu);
    Rts(memory, cpu);
    Window(memory, cpu, 0x0348u, 0x1b03u, 0x9cd1u);
    Window(memory, cpu, 0x0408u, 0x1b0bu, 0x9cdau);
    SetAccumulatorWidth(cpu, 1);
    if (Text8E(memory, cpu, 0xd045u, 0x9cecu, &text))
        return text;
    LoadX16(cpu, 0x0003u);
    StoreZeroAbsolute8(memory, cpu, MENU_CURSOR_X_INDEX, cpu->x);
    StoreZeroAbsolute8(memory, cpu, MENU_CURSOR_Y_INDEX, cpu->x);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_MENU_SCROLL_ROW, 0));
    Write16Long(memory, 0x7e93c0u, cpu->accumulator);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_MENU_LIST_TOP_ROW, 0));
    Write16Long(memory, 0x7e93c2u, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadX16(cpu, 0x0000u);
    Write16Absolute(memory, cpu, WRAM_MENU_SCROLL_ROW, 0);
    Write16Absolute(memory, cpu, WRAM_MENU_LIST_TOP_ROW, 0);
    LoadX16(cpu, 0x003fu);
    StoreXDirect16(memory, cpu, DP_SCRATCH_A);
    LoadX16(cpu, 0x0001u);
    StoreXDirect16(memory, cpu, DP_SCRATCH_C);
    LoadX16(cpu, 0x0009u);
    LoadY16(cpu, 0x0008u);
    Jsr(memory, cpu, 0x9d23u);
    MenuCursorPair(memory, cpu);
    Rts(memory, cpu);
    LoadY16(cpu, 0xf08au);
    Jsr(memory, cpu, 0x9d29u);
    MenuScrollThumb(memory, cpu);
    Rts(memory, cpu);
    LoadX16(cpu, 0x4800u);
    Write16Absolute(memory, cpu, WRAM_MENU_SCROLL_TRACK, 0x4800u);
    LoadAAbsolute8(memory, cpu, WRAM_MENU_LIST_COUNT, 0);
    StoreZeroAbsolute8(memory, cpu, WRAM_MENU_LIST_OVERFLOW, 0);
    cpu->carry = 1;
    Sbc8(cpu, 0x06u);
    if (!cpu->negative) {
        StoreAAbsolute8(memory, cpu, WRAM_MENU_LIST_OVERFLOW, 0);
    } else {
        LoadAAbsolute8(memory, cpu, WRAM_MENU_LIST_COUNT, 0);
        StoreAAbsolute8(memory, cpu, 0x14fau, 0);
    }
    cpu->carry = 0;
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_MENU_LIST_COUNT, 0));
    Jsr(memory, cpu, 0x9d4cu);
    MenuScrollbar(memory, cpu);
    Rts(memory, cpu);
    Jsr(memory, cpu, 0x9d4fu);                                 /* B052 */
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, 0x040eu);
    LoadX16(cpu, 0x1410u);
    Jsr(memory, cpu, 0xb05cu);
    MenuClearRect2(memory, cpu);
    Rts(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadX16(cpu, 0x344eu);
    StoreXDirect16(memory, cpu, 0x11u);
    LoadX16(cpu, 0x0006u);
    StoreXDirect16(memory, cpu, 0x15u);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_MENU_LIST_TOP_ROW, 0));
    StoreXDirect16(memory, cpu, 0x00u);
    Jsr(memory, cpu, 0xb070u);
    if (MenuPointerRows(memory, cpu, &text))
        return text;
    Rts(memory, cpu);
    Rts(memory, cpu);
    TsbDirect(memory, cpu, 0x74u, 0x88u);
    return ExecutionReturned(0x829d54u);
}

/* Sets the text attribute (the menu draw mode) for the next string. */
static void SetAttribute(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t attribute) {
    StoreA8Absolute(memory, cpu, WRAM_MENU_DRAW_MODE, attribute);
}

/* JSL into the bank $81 reader of the first record byte of item $0A06. */
static void ItemByte(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x82u, return_address);
    cpu->program_bank = 0x81u;
    (void)Lufia2ItemRecordByte(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    cpu->program_bank = 0x82u;
}

/* $82:841E: item attribute: $20 usable, $24 not, $28 special. */
static void ItemAttribute(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    uint8_t mode;
    uint8_t attribute;

    LoadAAbsolute8(memory, cpu, MENU_ROW_ATTRIBUTE_KIND, 0);
    mode = A8(cpu);
    if (mode == 1u || mode == 2u) {
        Compare8(cpu, mode, 0x01u);
        if (mode == 2u)
            Compare8(cpu, mode, 0x02u);
        ItemByte(memory, cpu, mode == 1u ? 0x8460u : 0x846au);
        BitImmediate8(cpu, 0x20u);
        if (!cpu->zero)
            attribute = 0x28u;
        else if (mode == 1u)
            attribute = 0x20u;
        else {
            BitImmediate8(cpu, 0x01u);
            attribute = cpu->zero ? 0x24u : 0x20u;
        }
    } else {
        const uint16_t item =
            Read16AbsoluteIndexed(memory, cpu, WRAM_ITEM_RECORD_ID, 0);

        if (mode != 0u) {
            Compare8(cpu, mode, 0x01u);
            Compare8(cpu, mode, 0x02u);
        }

        LoadX16(cpu, item);
        Compare16(cpu, item, MENU_ITEM_USE_BIT_10);
        if (item == MENU_ITEM_USE_BIT_10 || item == MENU_ITEM_USE_BIT_08_A ||
            item == MENU_ITEM_USE_BIT_08_B) {
            if (item != MENU_ITEM_USE_BIT_10) {
                Compare16(cpu, item, MENU_ITEM_USE_BIT_08_A);
                if (item != MENU_ITEM_USE_BIT_08_A)
                    Compare16(cpu, item, MENU_ITEM_USE_BIT_08_B);
            }
            LoadAAbsolute8(memory, cpu, WRAM_WINDOW_MODE, 0);
            BitImmediate8(cpu, item == MENU_ITEM_USE_BIT_10 ? 0x10u : 0x08u);
            attribute = cpu->zero ? 0x20u : 0x24u;
        } else {
            Compare16(cpu, item, MENU_ITEM_USE_BIT_08_A);
            Compare16(cpu, item, MENU_ITEM_USE_BIT_08_B);
            ItemByte(memory, cpu, 0x8440u);
            BitImmediate8(cpu, 0x20u);
            if (!cpu->zero) {
                attribute = 0x28u;
            } else {
                BitImmediate8(cpu, 0x40u);
                attribute = cpu->zero ? 0x24u : 0x20u;
            }
        }
    }
    LoadA8(cpu, attribute);
    StoreAAbsolute8(memory, cpu, WRAM_MENU_DRAW_MODE, 0);
}

/* JSL into the bank $81 spell record byte reader; the C variant reads the
 * record's offset-$0C byte, the other the offset-$08 byte. */
static void SpellByte(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    int twelve, uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x82u, return_address);
    cpu->program_bank = 0x81u;
    if (twelve)
        (void)Lufia2SpellRecordByteC(memory, cpu);
    else
        (void)Lufia2SpellRecordByte8(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    cpu->program_bank = 0x82u;
}

/* $82:8483: spell attribute: $20 castable (cost <= $151D), $24 not. */
static void SpellAttribute(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    int castable = 0;
    uint8_t spell;

    LoadAAbsolute8(memory, cpu, WRAM_MENU_SPELL_RECORD_ID, 0);
    spell = A8(cpu);
    Compare8(cpu, spell, 0x24u);
    if (spell >= 0x24u && spell <= 0x26u) {
        static const uint8_t kBits[3] = {0x10u, 0x08u, 0x80u};

        if (spell != 0x24u) {
            Compare8(cpu, spell, 0x25u);
            if (spell != 0x25u)
                Compare8(cpu, spell, 0x26u);
        }
        LoadAAbsolute8(memory, cpu, WRAM_WINDOW_MODE, 0);
        BitImmediate8(cpu, kBits[spell - 0x24u]);
        castable = cpu->zero;
    } else {
        Compare8(cpu, spell, 0x25u);
        Compare8(cpu, spell, 0x26u);
        SpellByte(memory, cpu, 0, 0x8495u);
        BitImmediate8(cpu, 0x40u);
        castable = !cpu->zero;
    }
    if (castable) {
        SpellByte(memory, cpu, 1, 0x84bau);
        SetAccumulatorWidth(cpu, 0);
        And16(cpu, 0x00ffu);
        Compare16(cpu, cpu->accumulator, Read16AbsoluteIndexed(memory, cpu, 0x151du, 0));
        SetAccumulatorWidth(cpu, 1);
        castable = cpu->zero || !cpu->carry;
    }
    LoadA8(cpu, castable ? 0x20u : 0x24u);
    StoreAAbsolute8(memory, cpu, WRAM_MENU_DRAW_MODE, 0);
}

/* $82:84F7: item row at A: name and count of $0A8D,X. */
static int ItemRow(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2ExecutionResult *text) {
    StoreADirect16(memory, cpu, 0x17u);
    PushIndex(memory, cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a8du, cpu->x));
    if (!cpu->zero) {
        StoreADirect16(memory, cpu, 0x00u);
        And16(cpu, 0x01ffu);
        StoreAAbsolute16(memory, cpu, WRAM_ITEM_RECORD_ID, 0);
        SetAccumulatorWidth(cpu, 1);
        Jsr(memory, cpu, 0x850bu);
        ItemAttribute(memory, cpu);
        Rts(memory, cpu);
        SetAccumulatorWidth(cpu, 0);
        LoadY16(cpu, 0xc9b1u);
        LoadX16(cpu, Read16Direct(memory, cpu, 0x17u));
        if (DrawString(memory, cpu, 0x8516u, text))
            return 1;
        {
            const uint16_t word = Read16Direct(memory, cpu, 0x00u);   /* LSR $00 */

            cpu->carry = word & 1u;
            Write16Direct(memory, cpu, 0x00u, (uint16_t)(word >> 1));
            SetNz16(cpu, (uint16_t)(word >> 1));
        }
        LoadY16(cpu, 0xc9b7u);
        LoadX16(cpu, Read16Direct(memory, cpu, 0x17u));
        if (DrawString(memory, cpu, 0x8521u, text))
            return 1;
    }
    cpu->x = PullIndexValue(memory, cpu);
    IncrementX16(cpu);
    IncrementX16(cpu);
    return 0;
}

/* $82:854F: spell ([$2A],Y) at A: name and cost. */
static int SpellRow(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2ExecutionResult *text) {
    StoreADirect16(memory, cpu, 0x17u);
    SetAccumulatorWidth(cpu, 1);
    PushY(memory, cpu);
    LoadA8(cpu, Read8(memory, (((uint32_t)cpu->data_bank << 16) +
        Read16Direct(memory, cpu, 0x2au) + cpu->y) & 0x00ffffffu));
    Compare8(cpu, A8(cpu), 0xffu);
    if (!cpu->zero) {
        StoreADirect8(memory, cpu, 0x00u);
        StoreAAbsolute8(memory, cpu, WRAM_MENU_SPELL_RECORD_ID, 0);
        Jsr(memory, cpu, 0x8561u);
        SpellAttribute(memory, cpu);
        Rts(memory, cpu);
        LoadY16(cpu, 0xca0cu);
        LoadX16(cpu, Read16Direct(memory, cpu, 0x17u));
        if (DrawString(memory, cpu, 0x856au, text))
            return 1;
        LoadY16(cpu, 0xca34u);
        LoadX16(cpu, Read16Direct(memory, cpu, 0x17u));
        if (DrawString(memory, cpu, 0x8573u, text))
            return 1;
    }
    SetAccumulatorWidth(cpu, 0);
    LoadY16(cpu, PullIndexValue(memory, cpu));
    IncrementY16(cpu);
    return 0;
}

/* $82:FC3F: A = item of the Xth set bit of $091E-$0925; carry = none. */
static void ScenarioItem(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    IncrementX16(cpu);
    Write16Direct(memory, cpu, DP_SCRATCH_A, 0);
    Write16Direct(memory, cpu, DP_SCRATCH_C, 0);
    LoadY16(cpu, 0x0000u);
    do {
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x091eu, cpu->y));
        if (!cpu->zero) {
            StoreYDirect16(memory, cpu, DP_SCRATCH_E);
            LoadY16(cpu, 0x0010u);
            do {
                Increment16Direct(memory, cpu, DP_SCRATCH_A);
                LsrA16(cpu);
                if (cpu->carry) {
                    cpu->x = (uint16_t)(cpu->x - 1u);
                    SetNz16(cpu, cpu->x);
                    if (cpu->zero) {
                        const uint16_t n =
                            (uint16_t)(Read16Direct(memory, cpu, DP_SCRATCH_A) - 1u);

                        Write16Direct(memory, cpu, DP_SCRATCH_A, (uint16_t)(n << 1));
                        LoadX16(cpu, (uint16_t)(n << 1));
                        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x97fda0u, cpu->x)));
                        cpu->carry = 0;
                        return;
                    }
                }
                cpu->y = (uint16_t)(cpu->y - 1u);
                SetNz16(cpu, cpu->y);
            } while (!cpu->zero);
            LoadY16(cpu, Read16Direct(memory, cpu, DP_SCRATCH_E));
        }
        LoadA16(cpu, Read16Direct(memory, cpu, DP_SCRATCH_C));
        cpu->carry = 0;
        Add16Value(cpu, 0x0010u);
        StoreADirect16(memory, cpu, DP_SCRATCH_C);
        StoreADirect16(memory, cpu, DP_SCRATCH_A);
        IncrementY16(cpu);
        IncrementY16(cpu);
        Compare16(cpu, cpu->y, 0x0008u);
    } while (!cpu->zero);
    cpu->carry = 1;
}

/* $82:86E5: scenario item row at A for the Xth item. */
static int ScenarioRow(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2ExecutionResult *text) {
    PushIndex(memory, cpu);
    StoreADirect16(memory, cpu, 0x17u);
    Jsr(memory, cpu, 0x86eau);
    ScenarioItem(memory, cpu);
    Rts(memory, cpu);
    if (!cpu->carry) {
        StoreADirect16(memory, cpu, 0x00u);
        SetAccumulatorWidth(cpu, 1);
        SetAttribute(memory, cpu, 0x20u);
        SetAccumulatorWidth(cpu, 0);
        LoadY16(cpu, 0xc9b1u);
        LoadX16(cpu, Read16Direct(memory, cpu, 0x17u));
        if (DrawString(memory, cpu, 0x8700u, text))
            return 1;
    }
    cpu->x = PullIndexValue(memory, cpu);
    IncrementX16(cpu);
    return 0;
}

/* $82:84D5 / $82:8526 / $82:86C4: $15 rows from $11, index $00. */
static int ListRows(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    unsigned kind, Lufia2ExecutionResult *text) {
    LoadA8(cpu, 0x8eu);
    StoreADirect8(memory, cpu, 0x5fu);
    if (kind == 1u)
        LoadY16(cpu, Read16Direct(memory, cpu, 0x00u));
    SetAccumulatorWidth(cpu, 0);
    if (kind != 1u) {
        LoadA16(cpu, Read16Direct(memory, cpu, 0x00u));
        if (kind == 0u)
            AslA16(cpu);
        TransferAToX(cpu);
    }
    do {
        LoadA16(cpu, Read16Direct(memory, cpu, 0x11u));
        if (kind == 0u) {
            Jsr(memory, cpu, 0x84e3u);
            if (ItemRow(memory, cpu, text))
                return 1;
            Rts(memory, cpu);
        } else if (kind == 1u) {
            Jsr(memory, cpu, 0x8532u);
            if (SpellRow(memory, cpu, text))
                return 1;
            Rts(memory, cpu);
            LoadA16(cpu, Read16Direct(memory, cpu, 0x11u));
            cpu->carry = 0;
            Add16Value(cpu, 0x001cu);
            Jsr(memory, cpu, 0x853bu);
            if (SpellRow(memory, cpu, text))
                return 1;
            Rts(memory, cpu);
        } else {
            Jsr(memory, cpu, 0x86d1u);
            if (ScenarioRow(memory, cpu, text))
                return 1;
            Rts(memory, cpu);
        }
        LoadA16(cpu, Read16Direct(memory, cpu, 0x11u));
        cpu->carry = 0;
        Add16Value(cpu, 0x0080u);
        StoreADirect16(memory, cpu, 0x11u);
        Decrement16Direct(memory, cpu, 0x15u);
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);
    TsbDirect(memory, cpu, 0x74u, 0x08u);
    return 0;
}

/* List mode $09D1. */
static int ListByMode(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    const uint16_t returns[5], Lufia2ExecutionResult *text) {
    unsigned kind = 0;
    uint16_t ret;

    LoadAAbsolute8(memory, cpu, WRAM_MENU_LIST_MODE, 0);
    switch (A8(cpu)) {
    case 0x02u:
        StoreZeroAbsolute8(memory, cpu, MENU_ROW_ATTRIBUTE_KIND, 0);
        kind = 1u;
        ret = returns[1];
        break;
    case 0x04u:
        StoreA8Absolute(memory, cpu, MENU_ROW_ATTRIBUTE_KIND, 0x01u);
        ret = returns[2];
        break;
    case 0x07u:
        kind = 2u;
        ret = returns[3];
        break;
    case 0xffu:
        StoreA8Absolute(memory, cpu, MENU_ROW_ATTRIBUTE_KIND, 0x02u);
        ret = returns[4];
        break;
    default:
        StoreZeroAbsolute8(memory, cpu, MENU_ROW_ATTRIBUTE_KIND, 0);
        ret = returns[0];
        break;
    }
    Jsr(memory, cpu, ret);
    if (ListRows(memory, cpu, kind, text))
        return 1;
    Rts(memory, cpu);
    return 0;
}

/* $82:AC84: item/spell list page from index $14B5. */
static int MenuListPage(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2ExecutionResult *text) {
    static const uint16_t kReturns[5] = {0xacbcu, 0xacc3u, 0xacccu, 0xacd0u, 0xacd9u};

    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, 0x0402u);
    LoadX16(cpu, 0x1e10u);
    Jsr(memory, cpu, 0xac8eu);
    MenuClearRect2(memory, cpu);
    Rts(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadX16(cpu, 0x3448u);
    StoreXDirect16(memory, cpu, 0x11u);
    LoadX16(cpu, 0x0006u);
    StoreXDirect16(memory, cpu, 0x15u);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_MENU_LIST_TOP_ROW, 0));
    StoreXDirect16(memory, cpu, 0x00u);
    return ListByMode(memory, cpu, kReturns, text);
}

/* $82:ACDB: one list row at $3748. */
Lufia2ExecutionResult Lufia2MenuListRow(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const uint16_t kReturns[5] = {0xad01u, 0xad08u, 0xad11u, 0xad15u, 0xad1eu};
    Lufia2ExecutionResult text;

    LoadX16(cpu, 0x3748u);
    StoreXDirect16(memory, cpu, 0x11u);
    LoadX16(cpu, 0x0001u);
    StoreXDirect16(memory, cpu, 0x15u);
    if (ListByMode(memory, cpu, kReturns, &text))
        return text;
    return ExecutionReturned(0x82ad02u);
}

/* $82:A918: list cursor sprite by the selection, then the page. */
Lufia2ExecutionResult Lufia2MenuListCursor(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult text;

    LoadAAbsolute8(memory, cpu, WRAM_MENU_LIST_SELECTION, 0);
    Compare8(cpu, A8(cpu), 0xffu);
    if (!cpu->zero) {
        const int spells = AbsoluteByte(memory, cpu, WRAM_MENU_LIST_MODE, 0) == 0x02u;
        uint8_t top;
        uint8_t row;

        LoadAAbsolute8(memory, cpu, WRAM_MENU_LIST_MODE, 0);
        Compare8(cpu, A8(cpu), 0x02u);
        top = AbsoluteByte(memory, cpu, WRAM_MENU_LIST_TOP_ROW, 0);
        row = (uint8_t)(AbsoluteByte(memory, cpu, WRAM_MENU_LIST_SELECTION, 0) >> 1);
        if (spells) {
            top = (uint8_t)(top >> 1);
            row = (uint8_t)(row >> 1);
        }
        Write8(memory, DirectAddress(cpu, 0x00u), top);
        Write8(memory, DirectAddress(cpu, 0x01u), row);
        LoadA8(cpu, row);
        cpu->carry = 1;
        Sbc8(cpu, top);
        StoreAAbsolute8(memory, cpu, 0x14eeu, 0);
        StoreAAbsolute8(memory, cpu, WRAM_SYSTEM_MULTIPLY_A, 0);
        LoadA8(cpu, 0x00u);
        Sbc8(cpu, 0x00u);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_SYSTEM_MULTIPLY_A, 0));
        if (cpu->accumulator & 0x8000u) {
            LoadA16(cpu, (uint16_t)((cpu->accumulator ^ 0xffffu) + 1u));
            StoreAAbsolute16(memory, cpu, WRAM_SYSTEM_MULTIPLY_A, 0);
        }
        SetAccumulatorWidth(cpu, 1);
        LoadAAbsolute8(memory, cpu, 0x1507u, 0);
        StoreAAbsolute8(memory, cpu, WRAM_SYSTEM_MULTIPLY_B, 0);
        StoreZeroAbsolute8(memory, cpu, (WRAM_SYSTEM_MULTIPLY_B + 1u), 0);
        Lufia2CallMultiply(memory, cpu, 0x82u, 0xa971u);
        LoadAAbsolute8(memory, cpu, WRAM_SYSTEM_MULTIPLY_PRODUCT, 0);
        cpu->carry = 0;
        Adc8(cpu, AbsoluteByte(memory, cpu, 0x14e3u, 0));
        StoreAAbsolute8(memory, cpu, 0x13f0u, 0);
        StoreZeroAbsolute8(memory, cpu, 0x11e0u, 0);
        LoadAAbsolute8(memory, cpu, WRAM_MENU_LIST_MODE, 0);
        Compare8(cpu, A8(cpu), 0x02u);
        top = AbsoluteByte(memory, cpu, WRAM_MENU_LIST_TOP_ROW, 0);
        row = (uint8_t)(AbsoluteByte(memory, cpu, WRAM_MENU_LIST_SELECTION, 0) >> 1);
        if (spells) {
            top = (uint8_t)(top >> 1);
            row = (uint8_t)(row >> 1);
        }
        Write8(memory, DirectAddress(cpu, 0x02u), top);
        LoadA8(cpu, top);
        cpu->carry = 0;
        Adc8(cpu, 0x06u);
        StoreADirect8(memory, cpu, 0x03u);
        LoadA8(cpu, row);
        Compare8(cpu, row, top);
        if (cpu->carry) {
            Compare8(cpu, row, DirectByte(memory, cpu, 0x03u));
            if (!cpu->carry) {
                const uint32_t shown = AbsoluteIndexedAddress(cpu, 0x11e0u, 0);

                Write8(memory, shown, (uint8_t)(Read8(memory, shown) + 1u));
                SetNz8(cpu, Read8(memory, shown));
            }
        }
    }
    Jsr(memory, cpu, 0xa9b3u);
    if (MenuListPage(memory, cpu, &text))
        return text;
    Rts(memory, cpu);
    return ExecutionReturned(0x82a9b4u);
}
