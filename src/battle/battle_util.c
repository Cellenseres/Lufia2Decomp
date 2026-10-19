/* Small battle helpers of bank $81. */

#include "core/cpu_internal.h"
#include "core/cpu_ops.h"
#include "lufia2/battle.h"
#include "battle/sprite_render_internal.h"
#include "system/wram.h"

/* Mask bit per record without status bit 2. */
static void ActiveBits(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t table, uint16_t last) {
    SetAccumulatorWidth(cpu, 0);
    LoadX16(cpu, last);
    do {
        int active = 0;

        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, table, cpu->x));
        if (!cpu->zero) {
            TransferAToY(cpu);
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x000fu, cpu->y));
            cpu->zero = (cpu->accumulator & 0x0004u) == 0;
            active = cpu->zero;
        }
        {
            const uint32_t at = AbsoluteIndexedAddress(cpu, 0x0022u, 0);
            const uint16_t old = Read16Long(memory, at);
            const uint16_t value = (uint16_t)((old << 1) | (active ? 1u : 0u));

            cpu->carry = (old & 0x8000u) != 0;
            /* Original word ROL writes the high byte before the low byte. */
            Write8(memory, (at + 1u) & 0x00ffffffu, (uint8_t)(value >> 8));
            Write8(memory, at, (uint8_t)value);
            SetNz16(cpu, value);
        }
        LoadX16(cpu, (uint16_t)(cpu->x - 2u));
    } while (!cpu->negative);
    SetAccumulatorWidth(cpu, 1);
    LoadAAbsolute8(memory, cpu, 0x0022u, 0);
}

/* $81:B264: active mask of the enemies (A bit 7) or party. */
Lufia2ExecutionResult Lufia2BattleActiveMask(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    StoreZeroAbsolute8(memory, cpu, 0x0022u, 0);
    BitImmediate8(cpu, 0x80u);
    if (!cpu->zero) {
        ActiveBits(memory, cpu, WRAM_BATTLE_ENEMY_RECORDS, 0x000au);
        Or8(cpu, 0x80u);
        return ExecutionReturned(0x81b290u);
    }
    ActiveBits(memory, cpu, WRAM_BATTLE_PARTY_RECORDS, 0x0008u);
    return ExecutionReturned(0x81b2b4u);
}

/* Target index of mask A: lowest bit, +5 for enemies. */
static void TargetIndex(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    StoreADirect8(memory, cpu, 0x23u);
    BitImmediate8(cpu, 0x3fu);
    if (cpu->zero)
        return;
    And8(cpu, 0x80u);
    if (!cpu->zero)
        LoadA8(cpu, 0x05u);
    StoreADirect8(memory, cpu, 0x22u);
    LoadA8(cpu, 0xffu);
    do {
        uint8_t bits;

        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        bits = DirectByte(memory, cpu, 0x23u);
        cpu->carry = bits & 1u;
        bits >>= 1;
        Write8(memory, (uint16_t)(cpu->direct_page + 0x23u), bits);
        SetNz8(cpu, bits);
    } while (!cpu->carry);
    cpu->carry = 0;
    Adc8(cpu, DirectByte(memory, cpu, 0x22u));
}

/* $81:B2B5: X = battler record of mask A ($0A64 table). */
Lufia2ExecutionResult Lufia2BattleTargetRecord(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    TargetIndex(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_BATTLE_PARTY_RECORDS, cpu->x));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x81b2dau);
}

/* $81:B2DB: X = $1499 + 7 * target index of mask A. */
Lufia2ExecutionResult Lufia2BattleTargetSlot(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    TargetIndex(memory, cpu);
    StoreAAbsolute8(memory, cpu, SNES_WRMPYA, 0);
    LoadA8(cpu, 0x07u);
    StoreAAbsolute8(memory, cpu, SNES_WRMPYB, 0);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, 0x1499u);
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, SNES_RDMPYL, 0));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x81b307u);
}

/* Rounded high byte of the product, plus $CA. */
static void ScaleAdd(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, 0x0080u);
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, SNES_RDMPYL, 0));
    SetAccumulatorWidth(cpu, 1);
    ExchangeAccumulatorBytes(cpu);
    cpu->carry = 0;
    Adc8(cpu, DirectByte(memory, cpu, 0xcau));
}

/* $81:B505: A = $CA + (A - $CA) * $29 / 256, rounded. */
Lufia2ExecutionResult Lufia2BattleBlend(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const uint8_t base = DirectByte(memory, cpu, 0xcau);

    Compare8(cpu, A8(cpu), base);
    if (cpu->zero)
        return ExecutionReturned(0x81b52eu);
    if (cpu->carry) {
        cpu->carry = 1;                                        /* B52F */
        Sbc8(cpu, DirectByte(memory, cpu, 0xcau));
        StoreAAbsolute8(memory, cpu, SNES_WRMPYA, 0);
        LoadA8(cpu, DirectByte(memory, cpu, 0x29u));
        StoreAAbsolute8(memory, cpu, SNES_WRMPYB, 0);
        ScaleAdd(memory, cpu);
        return ExecutionReturned(0x81b549u);
    }
    ExchangeAccumulatorBytes(cpu);                             /* swap with $CA */
    LoadA8(cpu, DirectByte(memory, cpu, 0xcau));
    ExchangeAccumulatorBytes(cpu);
    StoreADirect8(memory, cpu, 0xcau);
    ExchangeAccumulatorBytes(cpu);
    cpu->carry = 1;
    Sbc8(cpu, DirectByte(memory, cpu, 0xcau));
    StoreAAbsolute8(memory, cpu, SNES_WRMPYA, 0);
    TransferDirectToA(cpu);
    cpu->carry = 1;
    Sbc8(cpu, DirectByte(memory, cpu, 0x29u));
    StoreAAbsolute8(memory, cpu, SNES_WRMPYB, 0);
    ScaleAdd(memory, cpu);
    return ExecutionReturned(0x81b52eu);
}

/* Gray weights of the colour channels, out of 256. */
enum { GRAY_WEIGHT_RED = 0x4d, GRAY_WEIGHT_GREEN = 0x97, GRAY_WEIGHT_BLUE = 0x1c };

/* Colour work: the BGR555 colour and a scratch word. */
enum {
    COLOR_WORK_COLOR = 0x15,
    COLOR_WORK_COLOR_HIGH = 0x16,
    COLOR_WORK_SCRATCH = 0x17
};

enum { COLOR_CHANNEL_MASK = 0x1f };

/* $81:B54A: replace the colour at $15 by its gray. */
Lufia2ExecutionResult Lufia2ColorToGray(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, DirectByte(memory, cpu, COLOR_WORK_COLOR));
    And8(cpu, COLOR_CHANNEL_MASK);
    StoreAAbsolute8(memory, cpu, SNES_WRMPYA, 0);
    LoadA8(cpu, GRAY_WEIGHT_RED);
    StoreAAbsolute8(memory, cpu, SNES_WRMPYB, 0);
    SetAccumulatorWidth(cpu, 0);
    const uint16_t color = Read16Direct(memory, cpu, COLOR_WORK_COLOR);
    LoadA16(cpu, (uint16_t)(color >> 5));
    cpu->carry = (color >> 4) & 1u;
    SetAccumulatorWidth(cpu, 1);
    And8(cpu, COLOR_CHANNEL_MASK);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, SNES_RDMPYL)));
    StoreAAbsolute8(memory, cpu, SNES_WRMPYA, 0);
    LoadA8(cpu, GRAY_WEIGHT_GREEN);
    StoreAAbsolute8(memory, cpu, SNES_WRMPYB, 0);
    OpWriteX(memory, cpu, OpDp(cpu, COLOR_WORK_SCRATCH), cpu->y);
    LoadA8(cpu, DirectByte(memory, cpu, COLOR_WORK_COLOR_HIGH));
    cpu->carry = (A8(cpu) >> 1) & 1u;
    LoadA8(cpu, (uint8_t)(A8(cpu) >> 2));
    And8(cpu, COLOR_CHANNEL_MASK);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, SNES_RDMPYL)));
    StoreAAbsolute8(memory, cpu, SNES_WRMPYA, 0);
    LoadA8(cpu, GRAY_WEIGHT_BLUE);
    StoreAAbsolute8(memory, cpu, SNES_WRMPYB, 0);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, cpu->y);
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, COLOR_WORK_SCRATCH));
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, SNES_RDMPYL, 0));
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x00u);
    ExchangeAccumulatorBytes(cpu);
    StoreADirect8(memory, cpu, COLOR_WORK_SCRATCH);
    StoreADirect8(memory, cpu, COLOR_WORK_COLOR);
    AslA8(cpu);
    AslA8(cpu);
    StoreADirect8(memory, cpu, COLOR_WORK_COLOR_HIGH);
    SetAccumulatorWidth(cpu, 0);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    OpTestBits(memory, cpu, OpDp(cpu, COLOR_WORK_COLOR), 1u);
    return ExecutionReturned(0x81b5a2u);
}

/* $81:B5A3: hide all sprites in the OAM buffer. */
Lufia2ExecutionResult Lufia2BattleHideOam(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadX16(cpu, 0x001eu);
    LoadA16(cpu, 0x5555u);
    do {
        StoreAAbsolute16(memory, cpu, 0x0300u, cpu->x);
        LoadX16(cpu, (uint16_t)(cpu->x - 2u));
    } while (!cpu->negative);
    LoadX16(cpu, 0x01fcu);
    LoadA16(cpu, 0xe001u);
    do {
        StoreAAbsolute16(memory, cpu, 0x0100u, cpu->x);
        LoadX16(cpu, (uint16_t)(cpu->x - 4u));
    } while (!cpu->negative);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x81b5c3u);
}

/* $81:B974: palette A from $24:Y into $7F:F1DB, noted at $12B3. */
Lufia2ExecutionResult Lufia2BattleLoadPalette(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    uint8_t count;

    PushDataBank(memory, cpu);
    PushAccumulator8(memory, cpu);
    AslA8(cpu);
    Adc8(cpu, Read8(memory, (uint16_t)(cpu->stack + 1u)));
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x00ffu);
    TransferAToX(cpu);
    LoadA16(cpu, cpu->y);
    Write16Long(memory, LongIndexedAddress(0x0012b3u, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Pull8(memory, cpu));
    Write8(memory, LongIndexedAddress(0x0012b5u, cpu->x), A8(cpu));
    AslA8(cpu);
    AslA8(cpu);
    AslA8(cpu);
    AslA8(cpu);
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, DirectByte(memory, cpu, 0x24u));
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    LoadA8(cpu, 0x20u);
    StoreADirect8(memory, cpu, 0x25u);
    do {
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
        Write8(memory, LongIndexedAddress(WRAM_UNK_7FF1DB, cpu->x), A8(cpu));
        IncrementX16(cpu);
        IncrementY16(cpu);
        count = (uint8_t)(DirectByte(memory, cpu, 0x25u) - 1u);
        Write8(memory, (uint16_t)(cpu->direct_page + 0x25u), count);
        SetNz8(cpu, count);
    } while (count);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81b9aeu);
}

/* $81:B9AF: palettes $7F:F1DB to the CGRAM buffer, upload flag. */
Lufia2ExecutionResult Lufia2BattleCommitPalettes(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    LoadA8(cpu, 0x00u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    LoadX16(cpu, 0x01ffu);
    do {
        LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FF1DB, cpu->x)));
        StoreAAbsolute8(memory, cpu, WRAM_CGRAM_BUFFER, cpu->x);
        LoadX16(cpu, (uint16_t)(cpu->x - 1u));
    } while (!cpu->negative);
    LoadA8(cpu, 0x80u);
    StoreADirect8(memory, cpu, 0x73u);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81b9c6u);
}

/* $81:C5CF: X = record of target mask A (bit 7: enemy). */
Lufia2ExecutionResult Lufia2BattleTargetPointer(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    uint8_t bits;
    int enemy;

    StoreADirect8(memory, cpu, 0x31u);
    And8(cpu, 0x80u);
    StoreADirect8(memory, cpu, 0x30u);
    LoadA8(cpu, 0xffu);
    do {
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        bits = DirectByte(memory, cpu, 0x31u);
        cpu->carry = bits & 1u;
        bits >>= 1;
        Write8(memory, (uint16_t)(cpu->direct_page + 0x31u), bits);
        SetNz8(cpu, bits);
    } while (!cpu->carry);
    Or8(cpu, DirectByte(memory, cpu, 0x30u));
    BitImmediate8(cpu, 0x80u);
    enemy = !cpu->zero;
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x007fu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(enemy ? 0x859ec8u : 0x000a64u, cpu->x)));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(enemy ? 0x81c5ffu : 0x81c5f0u);
}

/* Draw two-row tiles from the shared grid. */
Lufia2ExecutionResult Lufia2BattleDrawTileGrid(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    PushAndSetDataBank(memory, cpu, 0x7eu);
    LoadA8(cpu, DirectByte(memory, cpu, BATTLE_GRID_TILE));
    AslA8(cpu);
    StoreADirect8(memory, cpu, BATTLE_GRID_ROW_TILE);
    And8(cpu, 0xf0u);
    OpAdcValue(cpu, DirectByte(memory, cpu, BATTLE_GRID_ROW_TILE));
    StoreADirect8(memory, cpu, BATTLE_GRID_ROW_TILE);
    LoadXDirect16(memory, cpu, BATTLE_GRID_WRITE_OFFSET);
    StoreXDirect16(memory, cpu, BATTLE_GRID_TILE_ROW_BASE);
    LoadA8(cpu, DirectByte(memory, cpu, BATTLE_GRID_HEIGHT));
    StoreADirect8(memory, cpu, BATTLE_GRID_ROWS_LEFT);
    for (;;) {
        LoadA8(cpu, DirectByte(memory, cpu, BATTLE_GRID_WIDTH));
        StoreADirect8(memory, cpu, BATTLE_GRID_COLUMNS_LEFT);
        LoadA8(cpu, DirectByte(memory, cpu, BATTLE_GRID_ATTRIBUTES));
        ExchangeAccumulatorBytes(cpu);
        LoadA8(cpu, DirectByte(memory, cpu, BATTLE_GRID_ROW_TILE));
        do {
            SetAccumulatorWidth(cpu, 0);
            Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x0000u, cpu->x),
                cpu->accumulator);
            TransferAToY(cpu);
            IncrementA16(cpu);
            Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x0002u, cpu->x),
                cpu->accumulator);
            LoadA16(cpu, cpu->y);
            cpu->carry = 0;
            OpAdcValue(cpu, 0x0010u);
            Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x0040u, cpu->x),
                cpu->accumulator);
            IncrementA16(cpu);
            Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x0042u, cpu->x),
                cpu->accumulator);
            LoadA16(cpu, cpu->y);
            OpAdcValue(cpu, 0x0002u);
            cpu->zero = (cpu->accumulator & 0x000fu) == 0;
            if (cpu->zero)
                OpAdcValue(cpu, 0x0010u);
            SetAccumulatorWidth(cpu, 1);
            IncrementX16(cpu);
            IncrementX16(cpu);
            IncrementX16(cpu);
            IncrementX16(cpu);
            DecrementDirect8(memory, cpu, BATTLE_GRID_COLUMNS_LEFT);
        } while (!cpu->zero);
        StoreADirect8(memory, cpu, BATTLE_GRID_ROW_TILE);
        DecrementDirect8(memory, cpu, BATTLE_GRID_ROWS_LEFT);
        if (cpu->zero)
            break;
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16Direct(memory, cpu, 0x19u));
        cpu->carry = 0;
        OpAdcValue(cpu, 0x0080u);
        Write16Direct(memory, cpu, BATTLE_GRID_TILE_ROW_BASE, cpu->accumulator);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
    }
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81bebbu);
}

Lufia2ExecutionResult Lufia2BattleTileBlock(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return Lufia2BattleDrawTileGrid(memory, cpu);
}

/* $81:BD4B: $02 x $03 sprites; A = count. */
Lufia2ExecutionResult Lufia2BattleSpriteBlock(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return Lufia2BattleDrawSpriteGrid(memory, cpu, false);
}

/* Blend A toward $CA through $81:B505, called at pc. */
static void BlendAt(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t ret) {
    SimulateJsrFrame(memory, cpu, ret);
    (void)Lufia2BattleBlend(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $81:B48B: $22 = gray $22 blended to colour $24 by $13 (0-$40). */
Lufia2ExecutionResult Lufia2BattleFadeColor(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    uint8_t level;

    LoadA8(cpu, DirectByte(memory, cpu, 0x13u));
    level = A8(cpu);
    if (cpu->zero)
        return ExecutionReturned(0x81b504u);
    Compare8(cpu, level, 0x40u);
    if (cpu->zero) {
        LoadX16(cpu, Read16Direct(memory, cpu, 0x24u));
        StoreXDirect16(memory, cpu, 0x22u);
        return ExecutionReturned(0x81b504u);
    }
    AslA8(cpu);
    AslA8(cpu);
    StoreADirect8(memory, cpu, 0x29u);
    LoadA8(cpu, (uint8_t)((DirectByte(memory, cpu, 0x13u) >> 4) & 0x03u));
    OpTestBits(memory, cpu, OpDp(cpu, 0x29u), 1u);
    LoadA8(cpu, (uint8_t)(DirectByte(memory, cpu, 0x22u) & 0x1fu));
    StoreADirect8(memory, cpu, 0x26u);
    LoadA8(cpu, (uint8_t)((DirectByte(memory, cpu, 0x23u) >> 2) & 0x1fu));
    StoreADirect8(memory, cpu, 0x27u);
    TransferDirectToA(cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x22u));
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    ExchangeAccumulatorBytes(cpu);
    SetAccumulatorWidth(cpu, 1);
    And8(cpu, 0x1fu);
    StoreADirect8(memory, cpu, 0x28u);
    LoadA8(cpu, DirectByte(memory, cpu, 0x26u));              /* red */
    StoreADirect8(memory, cpu, 0xcau);
    LoadA8(cpu, (uint8_t)(DirectByte(memory, cpu, 0x24u) & 0x1fu));
    BlendAt(memory, cpu, 0xb4ceu);
    StoreADirect8(memory, cpu, 0x26u);
    LoadA8(cpu, DirectByte(memory, cpu, 0x27u));              /* blue */
    StoreADirect8(memory, cpu, 0xcau);
    LoadA8(cpu, DirectByte(memory, cpu, 0x25u));
    cpu->carry = (A8(cpu) >> 1) & 1u;
    LoadA8(cpu, (uint8_t)(A8(cpu) >> 2));
    And8(cpu, 0x1fu);
    BlendAt(memory, cpu, 0xb4ddu);
    AslA8(cpu);
    AslA8(cpu);
    StoreADirect8(memory, cpu, 0x27u);
    LoadA8(cpu, DirectByte(memory, cpu, 0x28u));              /* green */
    StoreADirect8(memory, cpu, 0xcau);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x24u));
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    SetAccumulatorWidth(cpu, 1);
    ExchangeAccumulatorBytes(cpu);
    And8(cpu, 0x1fu);
    BlendAt(memory, cpu, 0xb4f4u);
    ExchangeAccumulatorBytes(cpu);
    SetAccumulatorWidth(cpu, 0);
    cpu->carry = (cpu->accumulator >> 2) & 1u;
    LoadA16(cpu, (uint16_t)(cpu->accumulator >> 3));
    And16(cpu, 0x03e0u);
    Or16(cpu, Read16Direct(memory, cpu, 0x26u));
    StoreADirect16(memory, cpu, 0x22u);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x81b504u);
}

/* $81:B444: palette $11 colours 1-15 grayed, faded by $13, to CGRAM buffer. */
Lufia2ExecutionResult Lufia2BattlePaletteFade(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    TransferDirectToA(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x11u));
    SetAccumulatorWidth(cpu, 0);
    ExchangeAccumulatorBytes(cpu);
    cpu->carry = (cpu->accumulator >> 2) & 1u;
    LoadA16(cpu, (uint16_t)(cpu->accumulator >> 3));
    TransferAToX(cpu);
    LoadA16(cpu, 0x000fu);
    StoreADirect16(memory, cpu, 0x19u);
    SetAccumulatorWidth(cpu, 1);
    do {
        uint16_t gray;

        IncrementX16(cpu);
        IncrementX16(cpu);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_UNK_7FF1DB, cpu->x)));
        PushIndex(memory, cpu);
        StoreADirect16(memory, cpu, 0x24u);
        StoreADirect16(memory, cpu, 0x15u);
        SimulateJsrFrame(memory, cpu, 0xb464u);
        (void)Lufia2ColorToGray(memory, cpu);
        SimulateRtsFrame(memory, cpu);
        gray = Read16Direct(memory, cpu, 0x15u);
        cpu->carry = (gray >> 1) & 1u;
        LoadA16(cpu, (uint16_t)((((gray >> 2) & 0x1c00u) ^ 0xffffu) + 1u));
        cpu->carry = 0;
        Add16Value(cpu, Read16Direct(memory, cpu, COLOR_WORK_COLOR));
        StoreADirect16(memory, cpu, 0x15u);
        StoreADirect16(memory, cpu, 0x22u);
        SetAccumulatorWidth(cpu, 1);
        SimulateJsrFrame(memory, cpu, 0xb47bu);
        (void)Lufia2BattleFadeColor(memory, cpu);
        SimulateRtsFrame(memory, cpu);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16Direct(memory, cpu, 0x22u));
        cpu->x = PullIndexValue(memory, cpu);
        StoreAAbsolute16(memory, cpu, WRAM_CGRAM_BUFFER, cpu->x);
        SetAccumulatorWidth(cpu, 1);
        DecrementDirect8(memory, cpu, 0x19u);
    } while (!cpu->zero);
    return ExecutionReturned(0x81b48au);
}

/* Item, party and sprite helpers of bank $81. */

#include "core/cpu_internal.h"
#include "core/cpu_ops.h"
#include "lufia2/battle.h"
#include "battle/sprite_render_internal.h"

static void SetPaletteBank(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t bank) {
    LoadA8(cpu, bank);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
}

/* $81:EC41: clear $7F:F000-$FFFF through WMDATA; M1X0. */
Lufia2ExecutionResult Lufia2BattleClearF000(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadX16(cpu, 0xf000u);
    StoreWordAbsolute(memory, cpu, SNES_WMADDL, cpu->x);
    LoadA8(cpu, 0x01u);
    StoreAAbsolute8(memory, cpu, SNES_WMADDH, 0);
    LoadX16(cpu, 0x1000u);
    do {
        StoreZeroAbsolute8(memory, cpu, SNES_WMDATA, 0);
        LoadX16(cpu, (uint16_t)(cpu->x - 1u));
    } while (!cpu->zero);
    return ExecutionReturned(0x81ec55u);
}

/* $81:EB34: palette $24 ($9A:F970) to $120F, then 16 zeros. */
Lufia2ExecutionResult Lufia2BattlePaletteCopy(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    SetPaletteBank(memory, cpu, 0x9au);
    LoadX16(cpu, 0x0000u);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, (uint16_t)(Read16Direct(memory, cpu, 0x24u) << 4));
    cpu->carry = (Read16Direct(memory, cpu, 0x24u) >> 12) & 1u;
    TransferAToY(cpu);
    SetAccumulatorWidth(cpu, 1);
    do {
        LoadAAbsolute8(memory, cpu, 0xf970u, cpu->y);
        StoreAAbsolute8(memory, cpu, 0x120fu, cpu->x);
        IncrementY16(cpu);
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x0010u);
    } while (!cpu->zero);
    TransferDirectToA(cpu);
    do {
        StoreAAbsolute8(memory, cpu, 0x120fu, cpu->x);
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x0020u);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);
    return ExecutionReturned(0x81eb61u);
}

/* $81:EB62: palette $24 split: low nibbles << 4 at $121F, high at $120F. */
Lufia2ExecutionResult Lufia2BattlePaletteSplit(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    unsigned i;

    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    SetPaletteBank(memory, cpu, 0x9au);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, (uint16_t)(Read16Direct(memory, cpu, 0x24u) << 4));
    cpu->carry = (Read16Direct(memory, cpu, 0x24u) >> 12) & 1u;
    TransferAToY(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadX16(cpu, 0x0000u);
    do {
        TransferDirectToA(cpu);
        LoadAAbsolute8(memory, cpu, 0xf970u, cpu->y);
        SetAccumulatorWidth(cpu, 0);
        for (i = 0; i < 4u; ++i)
            AslA16(cpu);
        SetAccumulatorWidth(cpu, 1);
        StoreAAbsolute8(memory, cpu, (WRAM_FIELD_LAYER_SCROLL_X + 1u), cpu->x);
        ExchangeAccumulatorBytes(cpu);
        StoreAAbsolute8(memory, cpu, 0x120fu, cpu->x);
        IncrementY16(cpu);
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x0010u);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);
    return ExecutionReturned(0x81eb92u);
}

/* $81:BD47: far entry of Lufia2BattleSpriteBlock. */
Lufia2ExecutionResult Lufia2BattleSpriteBlockFar(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0xbd49u);
    (void)Lufia2BattleSpriteBlock(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    return ExecutionReturned(0x81bd4au);
}

/* $81:BE54: far entry of Lufia2BattleTileBlock. */
Lufia2ExecutionResult Lufia2BattleTileBlockFar(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0xbe56u);
    (void)Lufia2BattleTileBlock(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    return ExecutionReturned(0x81be57u);
}
