/* OAM entries for the battle actors that are drawn as screen sprites: one
 * four-byte-plus-size record per visible actor, grouped by sprite kind. */

#include <stdbool.h>

#include "core/cpu_internal.h"
#include "core/plain_ops.h"
#include "core/wram_view.h"
#include "lufia2/battle.h"

/* Actor record table: 64 records of $2D bytes starting at $54B7. */
enum {
    ACTOR_FIRST = 0x54b7u,
    ACTOR_END = 0x5ff7u,
    ACTOR_SIZE = 0x2du,
    ACTOR_PRESENT = 0x00u,
    ACTOR_VISIBLE = 0x26u,
    ACTOR_TILE = 0x27u,
    ACTOR_PALETTE = 0x28u,
    ACTOR_KIND = 0x29u,
    ACTOR_LARGE = 0x2au,
    ACTOR_PRIORITY = 0x2bu,
    ACTOR_BASE_X = 0x1bu,
    ACTOR_OFFSET_X = 0x1fu,
    ACTOR_BASE_Y = 0x1du,
    ACTOR_OFFSET_Y = 0x21u
};

/* Direct page: write cursor of each OAM list, one word per kind. */
enum {
    LIST_CURSOR_FIRST = 0x00u,
    LIST_CURSOR_COUNT = 3u,
    DP_HALF_SIZE = 0x0eu
};

/* Work RAM: sprites written to each list ($15DE + 4 * kind), the totals
 * published for the OAM merge, and the frame counter used for rotation. */
enum {
    LIST_TOTALS = 0x15deu,
    PUBLISHED_TOTALS = 0x15dbu,
    LIST_LIMIT_X = 0x0110u,
    LIST_LIMIT_Y = 0x00e0u,
    ROTATION_PHASE = 0x15abu,
    FIRST_CURSORS_A = 0x4b4au,
    FIRST_CURSORS_B = 0x4c8au,
    FIRST_CURSORS_C = 0x4dcau,
    BATTLE_DATA_BANK = 0x7eu
};

/* Absolute address in the data bank, indexed. */
static uint32_t Field(const Lufia2Wram wram, uint16_t offset, uint16_t index) {
    return (((uint32_t)wram.data_bank << 16) + offset + index) & 0x00ffffffu;
}

static uint8_t FieldByte(const Lufia2Wram wram, uint16_t offset, uint16_t index) {
    return Read8(wram.memory, Field(wram, offset, index));
}

static uint16_t FieldWord(const Lufia2Wram wram, uint16_t offset, uint16_t index) {
    const uint32_t low = Field(wram, offset, index);

    return (uint16_t)(Read8(wram.memory, low) |
                      ((uint16_t)Read8(wram.memory, (low + 1u) & 0x00ffffffu) << 8));
}

static void StoreWord(
    const Lufia2Wram wram, uint16_t offset, uint16_t index, uint16_t value) {
    const uint32_t low = Field(wram, offset, index);

    Write8(wram.memory, low, (uint8_t)value);
    Write8(wram.memory, (low + 1u) & 0x00ffffffu, (uint8_t)(value >> 8));
}

/* One screen coordinate: the half size folded with the actor's base position
 * and its offset (two adds, each without carry in). The flags of the last
 * add are the ones a skipped sprite leaves behind. */
static Word16Result ScreenCoordinate(
    Lufia2Wram wram, uint16_t half, uint16_t record, uint16_t base,
    uint16_t offset) {
    const Word16Result folded =
        Sum16(half, FieldWord(wram, base, record), false);

    return Sum16(folded.value, FieldWord(wram, offset, record), false);
}

/* A sprite outside the screen is dropped: both saved words leave the stack,
 * the second one into X. */
static uint32_t SkipActorSprite(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Word16Result last,
    bool carry) {
    cpu->accumulator = last.value;
    cpu->carry = carry;
    cpu->overflow = last.overflow;
    (void)PullIndexValue(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    return 0x818f90u;
}

/* $81:8EEA: adds actor Y to its kind's OAM list. Entered with M=0, X=0. A
 * sprite that falls outside the screen is not drawn. Returns the address of
 * the RTS taken. */
static uint32_t AppendActorSprite(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram) {
    const uint16_t record = cpu->y;
    /* Kind 2..5 selects the list: its word offset, four bytes a list. */
    const uint16_t list_offset = (uint16_t)(
        ((FieldWord(wram, ACTOR_KIND, record) - 2u) & 3u) << 2);
    const uint16_t list_slot = (uint16_t)(list_offset >> 1);
    const uint16_t cursor = WramRead16At(wram, LIST_CURSOR_FIRST, list_slot);
    uint16_t half;
    Word16Result left;
    Word16Result right;
    Word16Result top;
    Word16Result bottom;
    Word16Result next;
    uint8_t selector;
    uint8_t attributes;
    uint8_t count;

    PushStackWord(memory, cpu, list_offset);
    PushStackWord(memory, cpu, list_slot);
    half = (FieldWord(wram, ACTOR_LARGE, record) & 0x00ffu) == 0 ? 0xfff8u
                                                                  : 0xfff0u;
    WramWrite16(wram, DP_HALF_SIZE, half);

    /* Left and top corner are stored first; the far corners must be on
     * screen for the sprite to be kept. */
    left = ScreenCoordinate(wram, half, record, ACTOR_BASE_X, ACTOR_OFFSET_X);
    StoreWord(wram, 0u, cursor, left.value);
    StoreWord(wram, 3u, cursor, left.value);
    right = ScreenCoordinate(
        wram, (uint16_t)~half, record, ACTOR_BASE_X, ACTOR_OFFSET_X);
    if ((right.value & 0x8000u) != 0)
        return SkipActorSprite(memory, cpu, right, right.carry);
    if (right.value >= LIST_LIMIT_X)
        return SkipActorSprite(memory, cpu, right, true);

    top = ScreenCoordinate(wram, half, record, ACTOR_BASE_Y, ACTOR_OFFSET_Y);
    StoreWord(wram, 1u, cursor, top.value);
    bottom = ScreenCoordinate(
        wram, (uint16_t)~half, record, ACTOR_BASE_Y, ACTOR_OFFSET_Y);
    if ((bottom.value & 0x8000u) != 0)
        return SkipActorSprite(memory, cpu, bottom, bottom.carry);
    if (bottom.value >= LIST_LIMIT_Y)
        return SkipActorSprite(memory, cpu, bottom, true);

    /* Tile, attributes and the size bit. A kind of 0 cycles through the four
     * palettes with the frame counter. */
    Write8(memory, Field(wram, 2u, cursor), FieldByte(wram, ACTOR_TILE, record));
    selector = FieldByte(wram, ACTOR_KIND, record);
    if (selector == 0)
        selector = (uint8_t)((2u + FieldByte(wram, ROTATION_PHASE, 0u)) & 3u);
    attributes = (uint8_t)(selector << 4);
    attributes |= FieldByte(wram, ACTOR_PALETTE, record);
    attributes |= FieldByte(wram, ACTOR_PRIORITY, record);
    Write8(memory, Field(wram, 3u, cursor), attributes);
    {
        const uint8_t large = FieldByte(wram, ACTOR_LARGE, record);
        const uint32_t high = Field(wram, 4u, cursor);
        const uint8_t old = Read8(memory, high);

        Write8(memory, high, (uint8_t)(old >> 1));
        Write8(memory, Field(wram, 4u, cursor),
            (uint8_t)((large << 1) | (old & 1u)));
    }

    /* Advance this kind's list cursor and count the sprite. */
    next = Sum16(cursor, 5u, false);
    WramWrite16At(wram, LIST_CURSOR_FIRST, PullIndexValue(memory, cpu), next.value);
    cpu->x = PullIndexValue(memory, cpu);
    count = (uint8_t)(FieldByte(wram, LIST_TOTALS, cpu->x) + 1u);
    Write8(memory, Field(wram, LIST_TOTALS, cpu->x), count);
    cpu->accumulator = next.value;
    SetSumFlags(cpu, next);
    SetNz8(cpu, count);
    return 0x818f8du;
}

Lufia2ExecutionResult Lufia2BattleActorSprite(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x818eeau);
    return ExecutionReturned(
        AppendActorSprite(memory, cpu, WramViewOfCaller(memory, cpu)));
}

/* $81:8E92: rebuilds the three sprite lists from all actor records, then
 * publishes the per-list totals. */
Lufia2ExecutionResult Lufia2BattleActorSprites(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;
    uint16_t record;
    unsigned list;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x818e92u);
    PushDataBank(memory, cpu);
    SelectDataBank(memory, cpu, BATTLE_DATA_BANK);
    wram = WramViewOfCaller(memory, cpu);
    WramWrite16(wram, LIST_CURSOR_FIRST, FIRST_CURSORS_A);
    WramWrite16(wram, LIST_CURSOR_FIRST + 2u, FIRST_CURSORS_B);
    WramWrite16(wram, LIST_CURSOR_FIRST + 4u, FIRST_CURSORS_C);
    cpu->x = FIRST_CURSORS_C;
    for (list = 0; list < LIST_CURSOR_COUNT; ++list)
        Write8(memory, Field(wram, (uint16_t)(LIST_TOTALS + 4u * list), 0u), 0u);
    SetAccumulatorWidth(cpu, 0);
    for (record = ACTOR_FIRST; record != ACTOR_END;
         record = (uint16_t)(record + ACTOR_SIZE)) {
        cpu->y = record;
        if ((FieldWord(wram, ACTOR_PRESENT, record) & 0x00ffu) == 0)
            continue;
        if ((FieldWord(wram, ACTOR_VISIBLE, record) & 0x00ffu) == 0)
            continue;
        PushY(memory, cpu);
        SimulateJsrFrame(memory, cpu, 0x8ec7u);
        (void)AppendActorSprite(memory, cpu, wram);
        SimulateRtsFrame(memory, cpu);
        cpu->y = PullIndexValue(memory, cpu);
    }
    /* The last step to the end of the table neither carries nor overflows;
     * the compare with the end leaves carry set. */
    cpu->y = ACTOR_END;
    cpu->accumulator = ACTOR_END;
    cpu->carry = true;
    cpu->overflow = false;
    SetAccumulatorWidth(cpu, 1);
    for (list = 0; list < LIST_CURSOR_COUNT; ++list) {
        LoadA8(cpu, FieldByte(wram, (uint16_t)(LIST_TOTALS + 4u * list), 0u));
        Write8(memory, Field(wram, (uint16_t)(PUBLISHED_TOTALS + 4u * list), 0u),
            A8(cpu));
    }
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x818ee9u);
}
