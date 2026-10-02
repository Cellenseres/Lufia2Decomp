/* OAM entries for the battle actors that are drawn as screen sprites: one
 * four-byte-plus-size record per visible actor, grouped by sprite kind. */

#include <stdbool.h>

#include "core/cpu_internal.h"
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
 * and its offset (two adds, each without carry in). Leaves the sum in A. */
static void AddPosition(
    Lufia2Wram wram, Lufia2CpuState *cpu, uint16_t half, uint16_t record,
    uint16_t base, uint16_t offset) {
    LoadA16(cpu, half);
    cpu->carry = false;
    Add16Value(cpu, FieldWord(wram, base, record));
    cpu->carry = false;
    Add16Value(cpu, FieldWord(wram, offset, record));
}

/* $81:8EEA: adds actor Y to its kind's OAM list. Entered with M=0, X=0. A
 * sprite that falls outside the screen is not drawn. Returns the address of
 * the RTS taken. */
static uint32_t AppendActorSprite(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram) {
    const uint16_t record = cpu->y;
    uint16_t half;
    uint16_t cursor;

    /* Kind 2..5 selects the list; its pointer slot is pushed twice over. */
    LoadA16(cpu, FieldWord(wram, ACTOR_KIND, record));
    LoadA16(cpu, (uint16_t)(cpu->accumulator - 1u));
    LoadA16(cpu, (uint16_t)(cpu->accumulator - 1u));
    And16(cpu, 3u);
    AslA16(cpu);
    AslA16(cpu);
    PushAccumulator16(memory, cpu);
    LsrA16(cpu);
    TransferAToX(cpu);
    PushIndex(memory, cpu);
    LoadA16(cpu, WramRead16At(wram, LIST_CURSOR_FIRST, cpu->x));
    TransferAToX(cpu);
    cursor = cpu->x;
    LoadA16(cpu, FieldWord(wram, ACTOR_LARGE, record));
    And16(cpu, 0x00ffu);
    half = cpu->zero ? 0xfff8u : 0xfff0u;
    LoadA16(cpu, half);
    WramWrite16(wram, DP_HALF_SIZE, half);

    /* Left and top corner are stored first; the far corners must be on
     * screen for the sprite to be kept. */
    AddPosition(wram, cpu, half, record, ACTOR_BASE_X, ACTOR_OFFSET_X);
    StoreWord(wram, 0u, cursor, cpu->accumulator);
    StoreWord(wram, 3u, cursor, cpu->accumulator);
    LoadA16(cpu, WramRead16(wram, DP_HALF_SIZE));
    AddPosition(wram, cpu, (uint16_t)~cpu->accumulator, record, ACTOR_BASE_X,
        ACTOR_OFFSET_X);
    if (cpu->negative)
        goto skipped;
    Compare16(cpu, cpu->accumulator, LIST_LIMIT_X);
    if (cpu->carry)
        goto skipped;

    LoadA16(cpu, WramRead16(wram, DP_HALF_SIZE));
    Add16Value(cpu, FieldWord(wram, ACTOR_BASE_Y, record));
    cpu->carry = false;
    Add16Value(cpu, FieldWord(wram, ACTOR_OFFSET_Y, record));
    StoreWord(wram, 1u, cursor, cpu->accumulator);
    LoadA16(cpu, WramRead16(wram, DP_HALF_SIZE));
    AddPosition(wram, cpu, (uint16_t)~cpu->accumulator, record, ACTOR_BASE_Y,
        ACTOR_OFFSET_Y);
    if (cpu->negative)
        goto skipped;
    Compare16(cpu, cpu->accumulator, LIST_LIMIT_Y);
    if (cpu->carry)
        goto skipped;

    /* Tile, attributes and the size bit, with 8-bit A. */
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, FieldByte(wram, ACTOR_TILE, record));
    Write8(memory, Field(wram, 2u, cursor), A8(cpu));
    LoadA8(cpu, FieldByte(wram, ACTOR_KIND, record));
    if (cpu->zero) {
        cpu->carry = false;
        Adc8(cpu, 2u);
        Adc8(cpu, FieldByte(wram, ROTATION_PHASE, 0u));
        And8(cpu, 3u);
    }
    AslA8(cpu);
    AslA8(cpu);
    AslA8(cpu);
    AslA8(cpu);
    Or8(cpu, FieldByte(wram, ACTOR_PALETTE, record));
    Or8(cpu, FieldByte(wram, ACTOR_PRIORITY, record));
    Write8(memory, Field(wram, 3u, cursor), A8(cpu));
    LoadA8(cpu, FieldByte(wram, ACTOR_LARGE, record));
    {
        const uint32_t high = Field(wram, 4u, cursor);
        const uint8_t old = Read8(memory, high);
        const uint8_t shifted = (uint8_t)(old >> 1);

        cpu->carry = (old & 1u) != 0;
        Write8(memory, high, shifted);
        SetNz8(cpu, shifted);
    }
    RolA8(cpu);
    Write8(memory, Field(wram, 4u, cursor), A8(cpu));

    /* Advance this kind's list cursor and count the sprite. */
    SetAccumulatorWidth(cpu, 0);
    TransferXToA(cpu);
    cpu->carry = false;
    Add16Value(cpu, 5u);
    cpu->x = PullIndexValue(memory, cpu);
    WramWrite16At(wram, LIST_CURSOR_FIRST, cpu->x, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    cpu->x = PullIndexValue(memory, cpu);
    {
        const uint32_t count = Field(wram, LIST_TOTALS, cpu->x);
        const uint8_t value = (uint8_t)(Read8(memory, count) + 1u);

        Write8(memory, count, value);
        SetNz8(cpu, value);
    }
    SetAccumulatorWidth(cpu, 0);
    return 0x818f8du;

skipped:
    cpu->x = PullIndexValue(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    return 0x818f90u;
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
    cpu->y = ACTOR_FIRST;
    for (list = 0; list < LIST_CURSOR_COUNT; ++list)
        Write8(memory, Field(wram, (uint16_t)(LIST_TOTALS + 4u * list), 0u), 0u);
    SetAccumulatorWidth(cpu, 0);
    do {
        LoadA16(cpu, FieldWord(wram, ACTOR_PRESENT, cpu->y));
        And16(cpu, 0x00ffu);
        if (!cpu->zero) {
            LoadA16(cpu, FieldWord(wram, ACTOR_VISIBLE, cpu->y));
            And16(cpu, 0x00ffu);
            if (!cpu->zero) {
                PushY(memory, cpu);
                SimulateJsrFrame(memory, cpu, 0x8ec7u);
                (void)AppendActorSprite(memory, cpu, wram);
                SimulateRtsFrame(memory, cpu);
                cpu->y = PullIndexValue(memory, cpu);
            }
        }
        LoadA16(cpu, cpu->y);
        cpu->carry = false;
        Add16Value(cpu, ACTOR_SIZE);
        TransferAToY(cpu);
        Compare16(cpu, cpu->accumulator, ACTOR_END);
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);
    for (list = 0; list < LIST_CURSOR_COUNT; ++list) {
        LoadA8(cpu, FieldByte(wram, (uint16_t)(LIST_TOTALS + 4u * list), 0u));
        Write8(memory, Field(wram, (uint16_t)(PUBLISHED_TOTALS + 4u * list), 0u),
            A8(cpu));
    }
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x818ee9u);
}
