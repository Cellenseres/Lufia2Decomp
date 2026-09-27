/* Small battle helpers of bank $81. */

#include "core/cpu_internal.h"
#include "lufia2/battle.h"

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
            Write16Long(memory, at, value);
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
        ActiveBits(memory, cpu, 0x0a6eu, 0x000au);
        Or8(cpu, 0x80u);
        return ExecutionReturned(0x81b290u);
    }
    ActiveBits(memory, cpu, 0x0a64u, 0x0008u);
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
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a64u, cpu->x));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x81b2dau);
}

/* $81:B2DB: X = $1499 + 7 * target index of mask A. */
Lufia2ExecutionResult Lufia2BattleTargetSlot(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    TargetIndex(memory, cpu);
    StoreAAbsolute8(memory, cpu, 0x4202u, 0);
    LoadA8(cpu, 0x07u);
    StoreAAbsolute8(memory, cpu, 0x4203u, 0);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, 0x1499u);
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4216u, 0));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x81b307u);
}

/* Rounded high byte of the product, plus $CA. */
static void ScaleAdd(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, 0x0080u);
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4216u, 0));
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
        Sbc8(cpu, base);
        StoreAAbsolute8(memory, cpu, 0x4202u, 0);
        LoadA8(cpu, DirectByte(memory, cpu, 0x29u));
        StoreAAbsolute8(memory, cpu, 0x4203u, 0);
        ScaleAdd(memory, cpu);
        return ExecutionReturned(0x81b549u);
    }
    ExchangeAccumulatorBytes(cpu);                             /* swap with $CA */
    LoadA8(cpu, base);
    ExchangeAccumulatorBytes(cpu);
    StoreADirect8(memory, cpu, 0xcau);
    ExchangeAccumulatorBytes(cpu);
    cpu->carry = 1;
    Sbc8(cpu, DirectByte(memory, cpu, 0xcau));
    StoreAAbsolute8(memory, cpu, 0x4202u, 0);
    TransferDirectToA(cpu);
    cpu->carry = 1;
    Sbc8(cpu, DirectByte(memory, cpu, 0x29u));
    StoreAAbsolute8(memory, cpu, 0x4203u, 0);
    ScaleAdd(memory, cpu);
    return ExecutionReturned(0x81b52eu);
}

/* $81:B54A: colour $15 to its gray; $17 = level; exits M0. */
Lufia2ExecutionResult Lufia2ColorToGray(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, DirectByte(memory, cpu, 0x15u));
    And8(cpu, 0x1fu);
    StoreAAbsolute8(memory, cpu, 0x4202u, 0);
    LoadA8(cpu, 0x4du);
    StoreAAbsolute8(memory, cpu, 0x4203u, 0);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, (uint16_t)(Read16Direct(memory, cpu, 0x15u) >> 5));
    cpu->carry = (Read16Direct(memory, cpu, 0x15u) >> 4) & 1u;
    SetAccumulatorWidth(cpu, 1);
    And8(cpu, 0x1fu);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4216u, 0));
    StoreAAbsolute8(memory, cpu, 0x4202u, 0);
    LoadA8(cpu, 0x97u);
    StoreAAbsolute8(memory, cpu, 0x4203u, 0);
    Write16Direct(memory, cpu, 0x17u, cpu->y);
    LoadA8(cpu, DirectByte(memory, cpu, 0x16u));
    cpu->carry = (A8(cpu) >> 1) & 1u;
    LoadA8(cpu, (uint8_t)(A8(cpu) >> 2));
    And8(cpu, 0x1fu);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4216u, 0));
    StoreAAbsolute8(memory, cpu, 0x4202u, 0);
    LoadA8(cpu, 0x1cu);
    StoreAAbsolute8(memory, cpu, 0x4203u, 0);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, cpu->y);
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x17u));
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4216u, 0));
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x00u);
    ExchangeAccumulatorBytes(cpu);
    StoreADirect8(memory, cpu, 0x17u);
    StoreADirect8(memory, cpu, 0x15u);
    AslA8(cpu);
    AslA8(cpu);
    StoreADirect8(memory, cpu, 0x16u);
    SetAccumulatorWidth(cpu, 0);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    {
        const uint16_t old = Read16Direct(memory, cpu, 0x15u);

        cpu->zero = (old & cpu->accumulator) == 0;
        Write16Direct(memory, cpu, 0x15u, (uint16_t)(old | cpu->accumulator));
    }
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
        Write8(memory, LongIndexedAddress(0x7ff1dbu, cpu->x), A8(cpu));
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
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7ff1dbu, cpu->x)));
        StoreAAbsolute8(memory, cpu, 0x0320u, cpu->x);
        LoadX16(cpu, (uint16_t)(cpu->x - 1u));
    } while (!cpu->negative);
    LoadA8(cpu, 0x80u);
    StoreADirect8(memory, cpu, 0x73u);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81b9c6u);
}

/* $81:C2C0: 16 bytes $B401 to $123C. */
Lufia2ExecutionResult Lufia2BattleCopyC2C0(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadX16(cpu, 0x0000u);
    do {
        LoadAAbsolute8(memory, cpu, 0xb401u, cpu->x);
        StoreAAbsolute8(memory, cpu, 0x123cu, cpu->x);
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x0010u);
    } while (!cpu->zero);
    return ExecutionReturned(0x81c2cfu);
}

/* WMDATA fill from address, count words or bytes. */
static void PortFill(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t address, uint16_t count, int pairs) {
    LoadX16(cpu, address);
    StoreWordAbsolute(memory, cpu, 0x2181u, cpu->x);
    StoreZeroAbsolute8(memory, cpu, 0x2183u, 0);
    LoadX16(cpu, count);
    if (pairs)
        LoadA8(cpu, 0x21u);
    do {
        StoreZeroAbsolute8(memory, cpu, 0x2180u, 0);
        if (pairs)
            StoreAAbsolute8(memory, cpu, 0x2180u, 0);
        LoadX16(cpu, (uint16_t)(cpu->x - 1u));
    } while (!cpu->zero);
}

/* $81:C2D0: clear $7E:2000-$27FF. */
Lufia2ExecutionResult Lufia2BattleClear2000(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    PortFill(memory, cpu, 0x2000u, 0x0800u, 0);
    return ExecutionReturned(0x81c2e2u);
}

/* $81:C2E3: fill $7E:2800-$2FFF with tile $2100. */
Lufia2ExecutionResult Lufia2BattleFill2800(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    PortFill(memory, cpu, 0x2800u, 0x0400u, 1);
    return ExecutionReturned(0x81c2fau);
}

/* $81:C2FB: clear $7E:3000-$37FF. */
Lufia2ExecutionResult Lufia2BattleClear3000(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    PortFill(memory, cpu, 0x3000u, 0x0800u, 0);
    return ExecutionReturned(0x81c30du);
}

/* $81:C30E: clear $7E:3800-$3FFF. */
Lufia2ExecutionResult Lufia2BattleClear3800(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    PortFill(memory, cpu, 0x3800u, 0x0800u, 0);
    return ExecutionReturned(0x81c320u);
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

/* Next 2x2 tile: +2, skipping to the next row pair. */
static void NextTile16(Lufia2CpuState *cpu) {
    Add16Value(cpu, 0x0002u);
    if ((cpu->accumulator & 0x000fu) == 0) {
        cpu->zero = 1;
        Add16Value(cpu, 0x0010u);
    } else {
        cpu->zero = 0;
    }
}

/* Tile $00 as a 2x2 sheet index. */
static void SheetTile(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadA8(cpu, DirectByte(memory, cpu, 0x00u));
    AslA8(cpu);
    StoreADirect8(memory, cpu, 0x11u);
    And8(cpu, 0xf0u);
    Adc8(cpu, DirectByte(memory, cpu, 0x11u));
    StoreADirect8(memory, cpu, 0x11u);
}

static void Bank7E(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    LoadA8(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
}

static uint8_t DecDirect(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t offset) {
    const uint8_t value = (uint8_t)(DirectByte(memory, cpu, offset) - 1u);

    Write8(memory, (uint16_t)(cpu->direct_page + offset), value);
    SetNz8(cpu, value);
    return value;
}

/* $81:BE58: $02 x $03 2x2 tiles from $00, attribute $04, at $7E:$08. */
Lufia2ExecutionResult Lufia2BattleTileBlock(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Bank7E(memory, cpu);
    SheetTile(memory, cpu);
    LoadX16(cpu, Read16Direct(memory, cpu, 0x08u));
    StoreXDirect16(memory, cpu, 0x19u);
    LoadA8(cpu, DirectByte(memory, cpu, 0x03u));
    StoreADirect8(memory, cpu, 0x14u);
    for (;;) {
        LoadA8(cpu, DirectByte(memory, cpu, 0x02u));           /* BE70 */
        StoreADirect8(memory, cpu, 0x13u);
        LoadA8(cpu, DirectByte(memory, cpu, 0x04u));
        ExchangeAccumulatorBytes(cpu);
        LoadA8(cpu, DirectByte(memory, cpu, 0x11u));
        do {
            SetAccumulatorWidth(cpu, 0);                       /* BE79 */
            StoreAAbsolute16(memory, cpu, 0x0000u, cpu->x);
            TransferAToY(cpu);
            LoadA16(cpu, (uint16_t)(cpu->accumulator + 1u));
            StoreAAbsolute16(memory, cpu, 0x0002u, cpu->x);
            LoadA16(cpu, cpu->y);
            cpu->carry = 0;
            Add16Value(cpu, 0x0010u);
            StoreAAbsolute16(memory, cpu, 0x0040u, cpu->x);
            LoadA16(cpu, (uint16_t)(cpu->accumulator + 1u));
            StoreAAbsolute16(memory, cpu, 0x0042u, cpu->x);
            LoadA16(cpu, cpu->y);
            NextTile16(cpu);
            SetAccumulatorWidth(cpu, 1);
            LoadX16(cpu, (uint16_t)(cpu->x + 4u));
        } while (DecDirect(memory, cpu, 0x13u));
        StoreADirect8(memory, cpu, 0x11u);
        if (!DecDirect(memory, cpu, 0x14u))
            break;
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16Direct(memory, cpu, 0x19u));
        cpu->carry = 0;
        Add16Value(cpu, 0x0080u);
        StoreADirect16(memory, cpu, 0x19u);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
    }
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81bebbu);
}

/* $81:BD4B: $02 x $03 sprites from $00 at $06/$05, $7E:$08; A = count. */
Lufia2ExecutionResult Lufia2BattleSpriteBlock(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Bank7E(memory, cpu);
    Write8(memory, (uint16_t)(cpu->direct_page + 0x15u), 0x00u);
    TransferDirectToA(cpu);
    cpu->carry = 1;
    Sbc8(cpu, DirectByte(memory, cpu, 0x07u));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x06u));
    TransferAToY(cpu);
    Write16Direct(memory, cpu, 0x17u, cpu->y);
    Write16Direct(memory, cpu, 0x1cu, cpu->y);
    SheetTile(memory, cpu);
    LoadA8(cpu, (uint8_t)(DirectByte(memory, cpu, 0x05u) - 1u));
    StoreADirect8(memory, cpu, 0x16u);
    LoadX16(cpu, Read16Direct(memory, cpu, 0x08u));
    do {
        LoadA8(cpu, DirectByte(memory, cpu, 0x02u));           /* BD70 */
        StoreADirect8(memory, cpu, 0x13u);
        do {
            static const uint8_t kFields[4] = {0x17u, 0x16u, 0x11u, 0x04u};
            unsigned i;

            for (i = 0; i < 4u; ++i) {
                LoadA8(cpu, DirectByte(memory, cpu, kFields[i]));
                StoreAAbsolute8(memory, cpu, 0x0000u, cpu->x);
                IncrementX16(cpu);
            }
            LoadA8(cpu, (uint8_t)(DirectByte(memory, cpu, 0x18u) & 0x03u));
            StoreAAbsolute8(memory, cpu, 0x0000u, cpu->x);
            IncrementX16(cpu);
            Write8(memory, (uint16_t)(cpu->direct_page + 0x15u),
                (uint8_t)(DirectByte(memory, cpu, 0x15u) + 1u));
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16Direct(memory, cpu, 0x17u));
            cpu->carry = 0;
            Add16Value(cpu, 0x0010u);
            StoreADirect16(memory, cpu, 0x17u);
            SetAccumulatorWidth(cpu, 1);
            LoadA8(cpu, DirectByte(memory, cpu, 0x11u));
            cpu->carry = 0;
            Adc8(cpu, 0x02u);
            if ((A8(cpu) & 0x0fu) == 0)
                Adc8(cpu, 0x10u);
            StoreADirect8(memory, cpu, 0x11u);
        } while (DecDirect(memory, cpu, 0x13u));
        LoadA8(cpu, DirectByte(memory, cpu, 0x16u));
        cpu->carry = 0;
        Adc8(cpu, 0x10u);
        StoreADirect8(memory, cpu, 0x16u);
        LoadY16(cpu, Read16Direct(memory, cpu, 0x1cu));
        Write16Direct(memory, cpu, 0x17u, cpu->y);
    } while (DecDirect(memory, cpu, 0x03u));
    StoreXDirect16(memory, cpu, 0x08u);
    LoadA8(cpu, DirectByte(memory, cpu, 0x15u));
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81bdc7u);
}
