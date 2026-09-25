/* Battle sprites, tilemap and frame upkeep. */

#include "core/cpu_internal.h"

/* Y += step with M=0, back to M=1. */
static void BattleNextRecord(Lufia2ActorFrontendCpu *cpu, uint16_t step) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, cpu->y);
    cpu->carry = 0;
    Add16Value(cpu, step);
    TransferAToY(cpu);
    SetAccumulatorWidth(cpu, 1);
}

/* $81:BD4B, $81:BDCC: OAM strips, five bytes per sprite. */
static void BattleOamStrips(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t mirrored) {
    BattleSetDataBank(memory, cpu, 0x7eu);
    if (mirrored) {
        LoadA8(cpu, DirectByte(memory, cpu, 0x02u));           /* BDD1 */
        DecrementA8(cpu);
        AslA8(cpu);
        AslA8(cpu);
        AslA8(cpu);
        AslA8(cpu);
        Adc8(cpu, DirectByte(memory, cpu, 0x06u));
        StoreADirect8(memory, cpu, 0x06u);
    }
    Write8(memory, DirectAddress(cpu, 0x15u), 0x00u);
    TransferDirectToA(cpu);
    cpu->carry = 1;
    Sbc8(cpu, DirectByte(memory, cpu, 0x07u));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x06u));
    TransferAToY(cpu);
    StoreYDirect16(memory, cpu, 0x17u);
    StoreYDirect16(memory, cpu, 0x1cu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x00u));
    AslA8(cpu);
    StoreADirect8(memory, cpu, 0x11u);
    And8(cpu, 0xf0u);
    Adc8(cpu, DirectByte(memory, cpu, 0x11u));
    StoreADirect8(memory, cpu, 0x11u);
    LoadA8(cpu, DirectByte(memory, cpu, 0x05u));
    DecrementA8(cpu);
    StoreADirect8(memory, cpu, 0x16u);
    LoadXDirect16(memory, cpu, 0x08u);
    do {
        LoadA8(cpu, DirectByte(memory, cpu, 0x02u));           /* BD70 */
        StoreADirect8(memory, cpu, 0x13u);
        do {
            static const uint8_t fields[4] = {0x17u, 0x16u, 0x11u, 0x04u};
            unsigned i;

            for (i = 0; i < 4u; ++i) {
                LoadA8(cpu, DirectByte(memory, cpu, fields[i]));
                StoreAAbsolute8(memory, cpu, 0x0000u, cpu->x);
                IncrementX16(cpu);
            }
            LoadA8(cpu, DirectByte(memory, cpu, 0x18u));
            And8(cpu, 0x03u);
            StoreAAbsolute8(memory, cpu, 0x0000u, cpu->x);
            IncrementX16(cpu);
            IncrementDirect8(memory, cpu, 0x15u);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16Direct(memory, cpu, 0x17u));
            if (mirrored) {
                Subtract16(cpu, 0x0010u);
            } else {
                cpu->carry = 0;
                Add16Value(cpu, 0x0010u);
            }
            Write16Direct(memory, cpu, 0x17u, cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            LoadA8(cpu, DirectByte(memory, cpu, 0x11u));
            cpu->carry = 0;
            Adc8(cpu, 0x02u);
            BitImmediate8(cpu, 0x0fu);
            if (cpu->zero)
                Adc8(cpu, 0x10u);
            StoreADirect8(memory, cpu, 0x11u);
            DecrementDirect8(memory, cpu, 0x13u);
        } while (!cpu->zero);
        LoadA8(cpu, DirectByte(memory, cpu, 0x16u));           /* BDB3 */
        cpu->carry = 0;
        Adc8(cpu, 0x10u);
        StoreADirect8(memory, cpu, 0x16u);
        LoadYDirect16(memory, cpu, 0x1cu);
        StoreYDirect16(memory, cpu, 0x17u);
        DecrementDirect8(memory, cpu, 0x03u);
    } while (!cpu->zero);
    StoreXDirect16(memory, cpu, 0x08u);
    LoadA8(cpu, DirectByte(memory, cpu, 0x15u));
    PullDataBank(memory, cpu);
}

/* JSL $81:BD47 or $81:BDC8 from bank 85. */
static void BattleCallOamStrips(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t mirrored,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x85u, return_address);
    SimulateJsrFrame(memory, cpu, mirrored ? 0xbdcau : 0xbd49u);
    BattleOamStrips(memory, cpu, mirrored);
    SimulateRtsFrame(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $81:BE58: 16x16 tilemap block, rows of $02 cells. */
static void BattleTileBlock(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    BattleSetDataBank(memory, cpu, 0x7eu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x00u));               /* BE5D */
    AslA8(cpu);
    StoreADirect8(memory, cpu, 0x11u);
    And8(cpu, 0xf0u);
    Adc8(cpu, DirectByte(memory, cpu, 0x11u));
    StoreADirect8(memory, cpu, 0x11u);
    LoadXDirect16(memory, cpu, 0x08u);
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
            Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x0000u, cpu->x),
                cpu->accumulator);
            TransferAToY(cpu);
            IncrementA16(cpu);
            Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x0002u, cpu->x),
                cpu->accumulator);
            LoadA16(cpu, cpu->y);
            cpu->carry = 0;
            Add16Value(cpu, 0x0010u);
            Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x0040u, cpu->x),
                cpu->accumulator);
            IncrementA16(cpu);
            Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x0042u, cpu->x),
                cpu->accumulator);
            LoadA16(cpu, cpu->y);
            Add16Value(cpu, 0x0002u);
            cpu->zero = (cpu->accumulator & 0x000fu) == 0;
            if (cpu->zero)
                Add16Value(cpu, 0x0010u);
            SetAccumulatorWidth(cpu, 1);
            IncrementX16(cpu);
            IncrementX16(cpu);
            IncrementX16(cpu);
            IncrementX16(cpu);
            DecrementDirect8(memory, cpu, 0x13u);
        } while (!cpu->zero);
        StoreADirect8(memory, cpu, 0x11u);                     /* BEA5 */
        DecrementDirect8(memory, cpu, 0x14u);
        if (cpu->zero)
            break;
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16Direct(memory, cpu, 0x19u));
        cpu->carry = 0;
        Add16Value(cpu, 0x0080u);
        Write16Direct(memory, cpu, 0x19u, cpu->accumulator);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
    }
    PullDataBank(memory, cpu);
}

/* $85:8B4B: $153C sprites from 13-byte records at $139A. */
static void BattleSpriteRecords(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    BattleSetDataBank(memory, cpu, 0x00u);
    LoadA8(cpu, 0xffu);                                        /* 8B50 */
    StoreAAbsolute8(memory, cpu, 0x15c7u, 0);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x15c8u, 0));
    StoreXDirect16(memory, cpu, 0x08u);
    StoreZeroAbsolute8(memory, cpu, 0x15cau, 0);
    LoadAAbsolute8(memory, cpu, 0x153cu, 0);
    if (!cpu->zero) {
        StoreADirect8(memory, cpu, 0x0du);
        LoadY16(cpu, 0x0000u);
        do {
            LoadAAbsolute8(memory, cpu, 0x139au, cpu->y);      /* 8B67 */
            if (cpu->negative) {
                PushY(memory, cpu);
                TransferDirectToA(cpu);
                LoadAAbsolute8(memory, cpu, 0x13a4u, cpu->y);
                cpu->carry = 0;
                Adc8(cpu, AbsoluteByte(memory, cpu, 0x13a0u, cpu->y));
                LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
                StoreADirect8(memory, cpu, 0x05u);
                LoadX16(cpu, 0x0202u);
                StoreXDirect16(memory, cpu, 0x02u);
                SetAccumulatorWidth(cpu, 0);
                LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x13a2u, cpu->y));
                cpu->carry = 0;
                Add16Value(cpu,
                    Read16AbsoluteIndexed(memory, cpu, 0x139eu, cpu->y));
                And16(cpu, 0x01ffu);
                Write16Direct(memory, cpu, 0x06u, cpu->accumulator);
                SetAccumulatorWidth(cpu, 1);
                LoadAAbsolute8(memory, cpu, 0x15b5u, 0);       /* 8B8D */
                cpu->carry = 0;
                Adc8(cpu, AbsoluteByte(memory, cpu, 0x139cu, cpu->y));
                And8(cpu, 0x07u);
                AslA8(cpu);
                Or8(cpu, AbsoluteByte(memory, cpu, 0x1475u, 0));
                Or8(cpu, AbsoluteByte(memory, cpu, 0x139bu, cpu->y));
                StoreADirect8(memory, cpu, 0x04u);
                LoadAAbsolute8(memory, cpu, 0x139du, cpu->y);
                StoreADirect8(memory, cpu, 0x00u);
                BattleCallOamStrips(memory, cpu, 0, 0x8ba7u);
                cpu->carry = 0;
                Adc8(cpu, AbsoluteByte(memory, cpu, 0x15cau, 0));
                StoreAAbsolute8(memory, cpu, 0x15cau, 0);
                cpu->y = PullIndexValue(memory, cpu);
            }
            BattleNextRecord(cpu, 0x000du);                    /* 8BB0 */
            DecrementDirect8(memory, cpu, 0x0du);
        } while (!cpu->zero);
    }
    PullDataBank(memory, cpu);
}

/* $85:8BC0: the single 3x3 sprite at $13CE. */
static void BattleSpriteSingle(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    BattleSetDataBank(memory, cpu, 0x00u);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x15ccu, 0));
    StoreXDirect16(memory, cpu, 0x08u);
    StoreZeroAbsolute8(memory, cpu, 0x15cbu, 0);
    LoadAAbsolute8(memory, cpu, 0x13ceu, 0);
    if (cpu->negative) {
        LoadA8(cpu, 0xffu);                                    /* 8BD2 */
        StoreAAbsolute8(memory, cpu, 0x15cbu, 0);
        TransferDirectToA(cpu);
        LoadAAbsolute8(memory, cpu, 0x13d8u, 0);
        cpu->carry = 0;
        Adc8(cpu, AbsoluteByte(memory, cpu, 0x13d4u, 0));
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        StoreADirect8(memory, cpu, 0x05u);
        LoadX16(cpu, 0x0303u);
        StoreXDirect16(memory, cpu, 0x02u);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x13d6u, 0));
        cpu->carry = 0;
        Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x13d2u, 0));
        And16(cpu, 0x01ffu);
        Write16Direct(memory, cpu, 0x06u, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        LoadAAbsolute8(memory, cpu, 0x15b6u, 0);               /* 8BF7 */
        cpu->carry = 0;
        Adc8(cpu, AbsoluteByte(memory, cpu, 0x13d0u, 0));
        And8(cpu, 0x07u);
        Or8(cpu, AbsoluteByte(memory, cpu, 0x13ceu, 0));
        cpu->carry = 1;
        RolA8(cpu);
        Or8(cpu, AbsoluteByte(memory, cpu, 0x1476u, 0));
        Or8(cpu, AbsoluteByte(memory, cpu, 0x13cfu, 0));
        StoreADirect8(memory, cpu, 0x04u);
        LoadAAbsolute8(memory, cpu, 0x13d1u, 0);
        StoreADirect8(memory, cpu, 0x00u);
        LoadA8(cpu, DirectByte(memory, cpu, 0x04u));
        BitImmediate8(cpu, 0x40u);
        if (cpu->zero)
            BattleCallOamStrips(memory, cpu, 0, 0x8c1bu);
        else
            BattleCallOamStrips(memory, cpu, 1, 0x8c21u);
        StoreAAbsolute8(memory, cpu, 0x15ceu, 0);
    }
    PullDataBank(memory, cpu);                                 /* 8C25 */
}

/* $85:8C27: five 1x1 sprites from $1435, same records as $139A. */
static void BattleSpriteMarkers(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    BattleSetDataBank(memory, cpu, 0x00u);
    LoadX16(cpu, 0x493du);                                     /* 8C2C */
    StoreXDirect16(memory, cpu, 0x08u);
    StoreZeroAbsolute8(memory, cpu, 0x15d2u, 0);
    LoadA8(cpu, 0x05u);
    StoreADirect8(memory, cpu, 0x0du);
    LoadY16(cpu, 0x0000u);
    do {
        LoadAAbsolute8(memory, cpu, 0x1435u, cpu->y);          /* 8C3B */
        if (cpu->negative) {
            PushY(memory, cpu);
            TransferDirectToA(cpu);
            LoadAAbsolute8(memory, cpu, 0x13a4u, cpu->y);
            cpu->carry = 0;
            Adc8(cpu, AbsoluteByte(memory, cpu, 0x13a0u, cpu->y));
            StoreADirect8(memory, cpu, 0x05u);
            LoadX16(cpu, 0x0101u);
            StoreXDirect16(memory, cpu, 0x02u);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x13a2u, cpu->y));
            cpu->carry = 0;
            Add16Value(cpu,
                Read16AbsoluteIndexed(memory, cpu, 0x139eu, cpu->y));
            cpu->carry = 0;
            Add16Value(cpu, 0x0010u);
            And16(cpu, 0x01ffu);
            Write16Direct(memory, cpu, 0x06u, cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            LoadAAbsolute8(memory, cpu, 0x1478u, 0);           /* 8C64 */
            cpu->carry = 0;
            Adc8(cpu, 0x08u);
            Or8(cpu, AbsoluteByte(memory, cpu, 0x1436u, cpu->y));
            Or8(cpu, 0x01u);
            StoreADirect8(memory, cpu, 0x04u);
            LoadAAbsolute8(memory, cpu, 0x1438u, cpu->y);
            StoreADirect8(memory, cpu, 0x00u);
            BattleCallOamStrips(memory, cpu, 0, 0x8c79u);
            cpu->carry = 0;
            Adc8(cpu, AbsoluteByte(memory, cpu, 0x15d2u, 0));
            StoreAAbsolute8(memory, cpu, 0x15d2u, 0);
            cpu->y = PullIndexValue(memory, cpu);
        }
        BattleNextRecord(cpu, 0x000du);                        /* 8C82 */
        DecrementDirect8(memory, cpu, 0x0du);
    } while (!cpu->zero);
    LoadAAbsolute8(memory, cpu, 0x15d2u, 0);                   /* 8C90 */
    StoreAAbsolute8(memory, cpu, 0x15cfu, 0);
    PullDataBank(memory, cpu);
}

/* $85:8C98: six sprites from 15-byte records at $13DB. */
static void BattleSpriteParty(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    BattleSetDataBank(memory, cpu, 0x00u);
    LoadA8(cpu, 0xffu);                                        /* 8C9D */
    StoreAAbsolute8(memory, cpu, 0x15d3u, 0);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x15d4u, 0));
    StoreXDirect16(memory, cpu, 0x08u);
    StoreZeroAbsolute8(memory, cpu, 0x15d6u, 0);
    LoadX16(cpu, 0x0000u);
    StoreXDirect16(memory, cpu, 0x0bu);
    StoreZeroAbsolute8(memory, cpu, 0x1577u, 0);
    LoadAAbsolute8(memory, cpu, 0x154eu, 0);
    if (!cpu->zero) {
        LoadA8(cpu, 0x06u);
        StoreADirect8(memory, cpu, 0x0du);
        LoadY16(cpu, 0x0000u);
        do {
            LoadAAbsolute8(memory, cpu, 0x13dbu, cpu->y);      /* 8CBE */
            if (cpu->negative) {
                uint8_t mirrored;

                PushY(memory, cpu);
                TransferDirectToA(cpu);
                LoadAAbsolute8(memory, cpu, 0x13e5u, cpu->y);
                cpu->carry = 0;
                Adc8(cpu, AbsoluteByte(memory, cpu, 0x13e1u, cpu->y));
                LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
                StoreADirect8(memory, cpu, 0x05u);
                SetAccumulatorWidth(cpu, 0);
                LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x13e7u, cpu->y));
                Write16Direct(memory, cpu, 0x02u, cpu->accumulator);
                LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x13e3u, cpu->y));
                cpu->carry = 0;
                Add16Value(cpu,
                    Read16AbsoluteIndexed(memory, cpu, 0x13dfu, cpu->y));
                And16(cpu, 0x01ffu);
                Write16Direct(memory, cpu, 0x06u, cpu->accumulator);
                SetAccumulatorWidth(cpu, 1);
                LoadAAbsolute8(memory, cpu, 0x15b7u, 0);       /* 8CE4 */
                cpu->carry = 0;
                Adc8(cpu, AbsoluteByte(memory, cpu, 0x13ddu, cpu->y));
                And8(cpu, 0x07u);
                Or8(cpu, AbsoluteByte(memory, cpu, 0x13dbu, cpu->y));
                cpu->carry = 1;
                RolA8(cpu);
                Or8(cpu, AbsoluteByte(memory, cpu, 0x1477u, 0));
                StoreADirect8(memory, cpu, 0x04u);
                LoadAAbsolute8(memory, cpu, 0x13deu, cpu->y);
                StoreADirect8(memory, cpu, 0x00u);
                LoadA8(cpu, DirectByte(memory, cpu, 0x04u));
                BitImmediate8(cpu, 0x40u);
                mirrored = cpu->zero ? 0u : 1u;
                BattleCallOamStrips(
                    memory, cpu, mirrored, mirrored ? 0x8d0bu : 0x8d05u);
                LoadYDirect16(memory, cpu, 0x0bu);             /* 8D0C */
                StoreAAbsolute8(memory, cpu, 0x157fu, cpu->y);
                IncrementDirect8(memory, cpu, 0x0bu);
                cpu->carry = 0;
                Adc8(cpu, AbsoluteByte(memory, cpu, 0x15d6u, 0));
                StoreAAbsolute8(memory, cpu, 0x15d6u, 0);
                StoreAAbsolute8(memory, cpu, 0x1578u, cpu->y);
                cpu->y = PullIndexValue(memory, cpu);
            }
            BattleNextRecord(cpu, 0x000fu);                    /* 8D1E */
            DecrementDirect8(memory, cpu, 0x0du);
        } while (!cpu->zero);
    }
    PullDataBank(memory, cpu);                                 /* 8D2C */
}

/* $85:8D2E: party tilemap at $7E:2800 instead of sprites. */
static void BattlePartyTilemap(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    PushDataBank(memory, cpu);
    Push8(memory, cpu, 0x85u);                                 /* PHK */
    PullDataBank(memory, cpu);
    StoreZeroAbsolute8(memory, cpu, 0x15d3u, 0);               /* 8D31 */
    LoadX16(cpu, 0x2800u);
    LoadY16(cpu, 0x0200u);
    Write16Absolute(memory, cpu, 0x2181u, cpu->x);
    StoreZeroAbsolute8(memory, cpu, 0x2183u, 0);
    LoadA8(cpu, 0x01u);
    do {
        StoreZeroAbsolute8(memory, cpu, 0x2180u, 0);           /* 8D42 */
        StoreAAbsolute8(memory, cpu, 0x2180u, 0);
        LoadY16(cpu, (uint16_t)(cpu->y - 1u));
    } while (!cpu->zero);
    LoadAAbsolute8(memory, cpu, 0x154eu, 0);                   /* 8D4B */
    if (!cpu->zero) {
        LoadA8(cpu, 0x06u);
        StoreADirect8(memory, cpu, 0x05u);
        LoadA8(cpu, 0x7eu);
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        LoadY16(cpu, 0x0000u);
        do {
            LoadAAbsolute8(memory, cpu, 0x13dbu, cpu->y);      /* 8D5B */
            if (cpu->negative) {
                PushY(memory, cpu);
                SetAccumulatorWidth(cpu, 0);
                LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x13e7u, cpu->y));
                Write16Direct(memory, cpu, 0x02u, cpu->accumulator);
                SetAccumulatorWidth(cpu, 1);
                LoadAAbsolute8(memory, cpu, 0x13e1u, cpu->y);  /* 8D6A */
                ExchangeAccumulatorBytes(cpu);
                LoadAAbsolute8(memory, cpu, 0x13dfu, cpu->y);
                SetAccumulatorWidth(cpu, 0);
                cpu->carry = 0;
                Add16Value(cpu, 0x0100u);
                LsrA16(cpu);
                LsrA16(cpu);
                LsrA16(cpu);
                And16(cpu, 0x1f1fu);
                SetAccumulatorWidth(cpu, 1);
                ExchangeAccumulatorBytes(cpu);                 /* 8D7F */
                Write8(memory, 0x004202u, A8(cpu));
                LoadA8(cpu, 0x40u);
                Write8(memory, 0x004203u, A8(cpu));
                LoadA8(cpu, 0x00u);
                ExchangeAccumulatorBytes(cpu);
                AslA8(cpu);
                SetAccumulatorWidth(cpu, 0);
                cpu->carry = 0;
                Add16Value(cpu, Read16Long(memory, 0x004216u));
                Add16Value(cpu, 0x2800u);
                Write16Direct(memory, cpu, 0x08u, cpu->accumulator);
                SetAccumulatorWidth(cpu, 1);
                LoadAAbsolute8(memory, cpu, 0x13ddu, cpu->y);  /* 8D9C */
                cpu->carry = 0;
                Adc8(cpu, AbsoluteByte(memory, cpu, 0x15b7u, 0));
                And8(cpu, 0x07u);
                AslA8(cpu);
                AslA8(cpu);
                Or8(cpu, 0x23u);
                StoreADirect8(memory, cpu, 0x04u);
                LoadAAbsolute8(memory, cpu, 0x13deu, cpu->y);
                StoreADirect8(memory, cpu, 0x00u);
                SimulateJslFrame(memory, cpu, 0x85u, 0x8db3u); /* $81:BE54 */
                SimulateJsrFrame(memory, cpu, 0xbe56u);
                BattleTileBlock(memory, cpu);
                SimulateRtsFrame(memory, cpu);
                SimulateRtlFrame(memory, cpu);
                cpu->y = PullIndexValue(memory, cpu);
            }
            BattleNextRecord(cpu, 0x000fu);                    /* 8DB5 */
            DecrementDirect8(memory, cpu, 0x05u);
        } while (!cpu->zero);
    }
    PullDataBank(memory, cpu);                                 /* 8DC3 */
}

/* $85:972E: 15 rows of 16 tile ids from $3710. */
static void BattleTileGrid(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    unsigned row;

    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, 0x3710u);
    for (row = 0; row < 15u; ++row) {
        unsigned column;

        LoadX16(cpu, (uint16_t)(0x2816u + 0x40u * row));
        SimulateJsrFrame(memory, cpu, (uint16_t)(0x9738u + 6u * row));
        for (column = 0; column < 16u; ++column) {
            Write16Long(memory, AbsoluteIndexedAddress(
                cpu, (uint16_t)(2u * column), cpu->x), cpu->accumulator);
            IncrementA16(cpu);
        }
        SimulateRtsFrame(memory, cpu);
    }
    SetAccumulatorWidth(cpu, 1);
}

/* $85:8A2F body; returns the RTL taken. */
static uint16_t BattleSprites(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadAAbsolute8(memory, cpu, 0x15abu, 0);
    DecrementA8(cpu);
    if (cpu->zero) {
        SimulateJslFrame(memory, cpu, 0x85u, 0x8a6fu);
        BattleSpriteRecords(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        SimulateJslFrame(memory, cpu, 0x85u, 0x8a73u);
        BattleSpriteSingle(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        SimulateJslFrame(memory, cpu, 0x85u, 0x8a77u);
        BattleSpriteMarkers(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        LoadAAbsolute8(memory, cpu, 0x11deu, 0);               /* 8A78 */
        if (cpu->zero) {
            SimulateJslFrame(memory, cpu, 0x85u, 0x8a80u);
            BattlePartyTilemap(memory, cpu);
            SimulateRtlFrame(memory, cpu);
            return 0x8a95u;
        }
        LoadAAbsolute8(memory, cpu, 0x125fu, 0);               /* 8A83 */
        if (!cpu->zero)
            return 0x8a95u;
        StoreZeroAbsolute8(memory, cpu, 0x15d3u, 0);
        BattleSetDataBank(memory, cpu, 0x7eu);
        SimulateJslFrame(memory, cpu, 0x85u, 0x8a93u);
        BattleTileGrid(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        PullDataBank(memory, cpu);
        return 0x8a95u;
    }
    DecrementA8(cpu);
    if (!cpu->zero)
        return 0x8a38u;
    SimulateJslFrame(memory, cpu, 0x85u, 0x8aa1u);             /* 8A9E */
    BattleSpriteRecords(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x85u, 0x8aa5u);
    BattleSpriteSingle(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x85u, 0x8aa9u);
    BattleSpriteMarkers(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x85u, 0x8aadu);
    BattleSpriteParty(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    return 0x8aaeu;
}

/* $85:91A1: refresh the five $147A state bytes. */
static void BattleSlotStates(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    PushDataBank(memory, cpu);
    Push8(memory, cpu, 0x85u);                                 /* PHK */
    PullDataBank(memory, cpu);
    LoadY16(cpu, 0x0000u);
    LoadX16(cpu, cpu->y);                                      /* TYX */
    do {
        SetAccumulatorWidth(cpu, 0);                           /* 91A8 */
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a64u, cpu->x));
        if (!cpu->zero) {
            PushIndex(memory, cpu);
            TransferAToX(cpu);
            SetAccumulatorWidth(cpu, 1);
            LoadAAbsolute8(memory, cpu, 0x147au, cpu->y);
            ExchangeAccumulatorBytes(cpu);
            LoadAAbsolute8(memory, cpu, 0x000fu, cpu->x);
            StoreAAbsolute8(memory, cpu, 0x147au, cpu->y);
            if (cpu->zero) {
                TransferDirectToA(cpu);                        /* 91CC */
                StoreAAbsolute8(memory, cpu, 0x1479u, cpu->y);
            } else {
                ExchangeAccumulatorBytes(cpu);
                if (cpu->zero) {
                    LoadA8(cpu, 0xffu);
                    StoreAAbsolute8(memory, cpu, 0x147bu, cpu->y);
                    StoreAAbsolute8(memory, cpu, 0x1479u, cpu->y);
                }
            }
            cpu->x = PullIndexValue(memory, cpu);
        }
        IncrementX16(cpu);                                     /* 91D1 */
        IncrementX16(cpu);
        IncrementY16(cpu);
        IncrementY16(cpu);
        IncrementY16(cpu);
        IncrementY16(cpu);
        Compare16(cpu, cpu->y, 0x0014u);
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);
    PullDataBank(memory, cpu);
}

static Lufia2ActorPrimaryUpdateResult BattleFrameEntry(
    Lufia2ActorFrontendCpu *cpu, uint32_t entry, uint32_t exit) {
    Lufia2ActorPrimaryUpdateResult result;

    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED;
    result.pc = exit;
    result.dispatches = 0;
    /* Only the M=1 X=0 entry is native. */
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit) {
        result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
        result.pc = cpu->resume_pc = entry;
    }
    return result;
}

/* $85:8A2F: battle sprites, JSL entry. */
Lufia2ActorPrimaryUpdateResult Lufia2BattleSprites(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    Lufia2ActorPrimaryUpdateResult result =
        BattleFrameEntry(cpu, 0x858a2fu, 0x858a38u);

    if (result.flow == LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED)
        result.pc = 0x850000u | BattleSprites(memory, cpu);
    return result;
}

/* $85:ECF0: per-frame battle upkeep from the $81:8877 loop. */
Lufia2ActorPrimaryUpdateResult Lufia2BattleFrameUpkeep(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    const Lufia2ActorPrimaryUpdateResult result =
        BattleFrameEntry(cpu, 0x85ecf0u, 0x85ed50u);
    static const uint16_t lists[2][2] = {{0x0a64u, 8u}, {0x0a6eu, 10u}};
    unsigned list;

    if (result.flow != LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED)
        return result;
    SimulateJslFrame(memory, cpu, 0x85u, 0xecf3u);
    BattleSlotStates(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x85u, 0xecf7u);
    (void)BattleSprites(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    LoadA8(cpu, 0xffu);                                        /* ECF8 */
    Write8(memory, 0x0012f3u, A8(cpu));
    SimulateJslFrame(memory, cpu, 0x85u, 0xed01u);             /* $85:9265 */
    BattleSetDataBank(memory, cpu, 0x7fu);
    LoadX16(cpu, 0x02fbu);
    do {
        StoreZeroAbsolute8(memory, cpu, 0xf44eu, cpu->x);
        LoadX16(cpu, (uint16_t)(cpu->x - 1u));
    } while (!cpu->negative);
    PullDataBank(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    StoreZeroAbsolute8(memory, cpu, 0x1b8bu, 0);               /* ED02 */
    LoadX16(cpu, 0x0023u);
    do {
        StoreZeroAbsolute8(memory, cpu, 0x1b8cu, cpu->x);
        LoadX16(cpu, (uint16_t)(cpu->x - 1u));
    } while (!cpu->negative);
    LoadAAbsolute8(memory, cpu, 0x11e8u, 0);                   /* ED0E */
    LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
    if (!cpu->zero)
        StoreAAbsolute8(memory, cpu, 0x11e8u, 0);
    StoreZeroAbsolute8(memory, cpu, 0x11eau, 0);
    for (list = 0; list < 2u; ++list) {
        LoadY16(cpu, lists[list][1]);                          /* ED1A */
        do {
            LoadX16(cpu, Read16AbsoluteIndexed(
                memory, cpu, lists[list][0], cpu->y));
            if (!cpu->zero) {
                LoadAAbsolute8(memory, cpu, 0x0010u, cpu->x);
                And8(cpu, 0xfeu);
                StoreAAbsolute8(memory, cpu, 0x0010u, cpu->x);
            }
            LoadY16(cpu, (uint16_t)(cpu->y - 1u));
            LoadY16(cpu, (uint16_t)(cpu->y - 1u));
        } while (!cpu->negative);
    }
    LoadX16(cpu, 0x0041u);                                     /* ED42 */
    do {
        LoadAAbsolute8(memory, cpu, 0x14e6u, cpu->x);
        if (!cpu->zero) {
            const uint32_t timer =
                AbsoluteIndexedAddress(cpu, 0x14e6u, cpu->x);
            const uint8_t left = (uint8_t)(Read8(memory, timer) - 1u);

            Write8(memory, timer, left);
            SetNz8(cpu, left);
        }
        LoadX16(cpu, (uint16_t)(cpu->x - 1u));
    } while (!cpu->negative);
    return result;
}

/* $85:ECDB: Y = first free slot in the $1A8F VRAM queue. */
Lufia2ActorPrimaryUpdateResult Lufia2BattleVramQueueSlot(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    Lufia2ActorPrimaryUpdateResult result;

    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED;
    result.pc = 0x85ecefu;
    result.dispatches = 0;
    /* X=1 decodes LDY #imm as two bytes. */
    if (cpu->index_is_8_bit) {
        result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
        result.pc = cpu->resume_pc = 0x85ecdbu;
        return result;
    }
    LoadY16(cpu, 0x0000u);                                     /* ECDB */
    for (;;) {
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1a8fu, cpu->y));
        if (cpu->zero)
            return result;
        LoadY16(cpu, (uint16_t)(cpu->y + 6u));
        Compare16(cpu, cpu->y, 0x0060u);
        if (cpu->zero)
            break;
    }
    /* Queue full: BRK #$6B on LLE. */
    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
    result.pc = cpu->resume_pc = 0x85eceeu;
    return result;
}
