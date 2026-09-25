/* Battle script VM ($85:B452). */

#include "core/cpu_internal.h"
#include "system/system_internal.h"

/* $85:C168: battler byte A to its variable base in X. */
static void BattleScriptSlot(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    StoreAAbsolute8(memory, cpu, 0x09fbu, 0);                  /* C168 */
    BitImmediate8(cpu, 0x3fu);
    if (!cpu->zero) {
        And8(cpu, 0x80u);
        if (!cpu->zero)
            LoadA8(cpu, 0x05u);
        StoreAAbsolute8(memory, cpu, 0x09fau, 0);
        LoadA8(cpu, 0xffu);
        do {
            const uint32_t bits = AbsoluteIndexedAddress(cpu, 0x09fbu, 0);
            const uint8_t value = Read8(memory, bits);

            LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));              /* C17A */
            cpu->carry = value & 1u;
            Write8(memory, bits, (uint8_t)(value >> 1));
            SetNz8(cpu, (uint8_t)(value >> 1));
        } while (!cpu->carry);
        cpu->carry = 0;
        Adc8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x09fau, 0)));
    }
    SetAccumulatorWidth(cpu, 0);                               /* C184 */
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a13u, 0));
    And16(cpu, 0x00ffu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu,
        cpu->zero ? 0x0a80u : 0x0a64u, cpu->x));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    SimulateRtsFrame(memory, cpu);
}

/* $85:C4DE: variable bases $C1 and $BE. */
static void BattleScriptBases(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0xb462u);
    LoadA8(cpu, Read8(memory, 0x7ff450u));                     /* C4DE */
    BattleScriptSlot(memory, cpu, 0xc4e4u);
    Write16Direct(memory, cpu, 0xc1u, cpu->x);
    LoadA8(cpu, Read8(memory, 0x7ff44eu));
    BattleScriptSlot(memory, cpu, 0xc4edu);
    Write16Direct(memory, cpu, 0xbeu, cpu->x);
    SimulateRtsFrame(memory, cpu);
}

/* INC $BB, 16-bit. */
static void BattleScriptAdvance(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    const uint16_t pointer = (uint16_t)(Read16Direct(memory, cpu, 0xbbu) + 1u);

    Write16Direct(memory, cpu, 0xbbu, pointer);
    SetNz16(cpu, pointer);
}

/* $85:BFBF: next script byte into A; flags kept. */
static void BattleScriptByte(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* BFBF */
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, DirectLongPointer(memory, cpu, 0xbbu)));
    SetAccumulatorWidth(cpu, 0);
    BattleScriptAdvance(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $85:BFCA: next script word into X. */
static void BattleScriptWord(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* BFCA */
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    PushAccumulator16(memory, cpu);
    LoadA16(cpu, Read16Long(memory, DirectLongPointer(memory, cpu, 0xbbu)));
    BattleScriptAdvance(memory, cpu);
    BattleScriptAdvance(memory, cpu);
    TransferAToX(cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}

/* Selector bit 7: local slot at base $C1 or $BE. */
static uint16_t BattleScriptLocal(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a52u, 0));
    And16(cpu, 0x00ffu);
    {
        const uint8_t base = cpu->zero ? 0xbeu : 0xc1u;

        LoadA16(cpu, (uint16_t)(
            Read8(memory, (uint16_t)(cpu->stack + 1u)) |
            (Read8(memory, (uint16_t)(cpu->stack + 2u)) << 8)));
        And16(cpu, 0x007fu);
        AslA16(cpu);
        Add16Value(cpu, Read16Direct(memory, cpu, base));
    }
    return cpu->accumulator;
}

/* $85:BFED: read variable A (bit 7 local) into X. */
static void BattleScriptRead(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* BFED */
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    PushAccumulator16(memory, cpu);
    cpu->zero = (cpu->accumulator & 0x0080u) == 0;
    if (cpu->zero) {
        And16(cpu, 0x00ffu);
        AslA16(cpu);
        TransferAToX(cpu);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7ff40eu, cpu->x)));
    } else {
        BattleScriptLocal(memory, cpu);                        /* C001 */
        TransferAToX(cpu);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0015u, cpu->x));
    }
    TransferAToX(cpu);                                         /* C01F */
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $85:C023: write X to variable A. */
static void BattleScriptWrite(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* C023 */
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    PushY(memory, cpu);
    PushAccumulator16(memory, cpu);
    cpu->zero = (cpu->accumulator & 0x0080u) == 0;
    if (cpu->zero) {
        And16(cpu, 0x00ffu);
        AslA16(cpu);
        LoadY16(cpu, cpu->accumulator);
        LoadA16(cpu, cpu->x);
        LoadX16(cpu, cpu->y);
        Write16Long(memory, LongIndexedAddress(0x7ff40eu, cpu->x),
            cpu->accumulator);
    } else {
        BattleScriptLocal(memory, cpu);                        /* C03A */
        LoadY16(cpu, cpu->accumulator);
        LoadA16(cpu, cpu->x);
        LoadX16(cpu, cpu->y);
        Write8(memory, AbsoluteIndexedAddress(cpu, 0x0015u, cpu->x),
            (uint8_t)cpu->accumulator);
        Write8(memory, AbsoluteIndexedAddress(cpu, 0x0016u, cpu->x),
            (uint8_t)(cpu->accumulator >> 8));
    }
    TransferAToX(cpu);                                         /* C05A */
    PullAccumulator16(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $85:BFD8: next word, bit 15 = variable, into X. */
static void BattleScriptValue(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* BFD8 */
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    PushAccumulator16(memory, cpu);
    LoadA16(cpu, Read16Long(memory, DirectLongPointer(memory, cpu, 0xbbu)));
    if (cpu->negative)
        BattleScriptRead(memory, cpu, 0xbfe2u);
    else
        TransferAToX(cpu);
    BattleScriptAdvance(memory, cpu);                          /* BFE6 */
    BattleScriptAdvance(memory, cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $85:B551: $BB = $0A42 + next word. */
static void BattleScriptJump(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SetAccumulatorWidth(cpu, 0);                               /* B551 */
    BattleScriptWord(memory, cpu, 0xb555u);
    LoadA16(cpu, cpu->x);
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a42u, 0));
    Write16Direct(memory, cpu, 0xbbu, cpu->accumulator);
}

/* Operand fetch shared by the compare and math opcodes. */
static void BattleScriptOperands(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t opcode) {
    BattleScriptByte(memory, cpu, (uint16_t)(opcode + 2u));
    BattleScriptRead(memory, cpu, (uint16_t)(opcode + 5u));
    Write16Direct(memory, cpu, 0x54u, cpu->x);
    BattleScriptValue(memory, cpu, (uint16_t)(opcode + 10u));
}

/* Signed $54 - X, 16-bit, overflow kept. */
static void BattleScriptCompare(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x54u));
    Write16Direct(memory, cpu, 0x54u, cpu->x);
    cpu->carry = 1;
    Add16Value(cpu, (uint16_t)~Read16Direct(memory, cpu, 0x54u));
}

/* Store binary result X into the destination variable. */
static void BattleScriptStoreBinary(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t value,
    uint16_t return_address) {
    LoadA16(cpu, value);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Pull8(memory, cpu));
    BattleScriptWrite(memory, cpu, return_address);
}

/* Unary opcodes: destination byte, source variable, result X. */
static void BattleScriptUnary(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t opcode,
    uint8_t kind) {
    BattleScriptByte(memory, cpu, (uint16_t)(opcode + 2u));
    PushAccumulator8(memory, cpu);
    BattleScriptRead(memory, cpu, (uint16_t)(opcode + 6u));
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, cpu->x);
    if (kind == 0) {                                           /* abs */
        if (cpu->negative) {
            LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffffu));
            LoadA16(cpu, (uint16_t)(cpu->accumulator + 1u));
        }
        TransferAToX(cpu);
    } else if (kind == 1) {                                    /* negate */
        LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffffu));
        LoadA16(cpu, (uint16_t)(cpu->accumulator + 1u));
        TransferAToX(cpu);
    } else if (!cpu->zero) {                                   /* sign */
        LoadX16(cpu, cpu->negative ? 0xffffu : 0x0001u);
    }
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Pull8(memory, cpu));
    BattleScriptWrite(memory, cpu,
        (uint16_t)(opcode + (kind == 0 ? 22u : kind == 1 ? 20u : 27u)));
}

/* INC $66, 16-bit. */
static void BattleIncrement66(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    const uint16_t value = (uint16_t)(Read16Direct(memory, cpu, 0x66u) + 1u);

    Write16Direct(memory, cpu, 0x66u, value);
    SetNz16(cpu, value);
}

/* $85:DCA3: $63-$66 = $54 * $56, 16x16 via $4202. */
static void BattleMultiply(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x85u, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* DCA3 */
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    LoadA8(cpu, DirectByte(memory, cpu, 0x56u));
    StoreAAbsolute8(memory, cpu, 0x4202u, 0);
    LoadA8(cpu, DirectByte(memory, cpu, 0x54u));
    StoreAAbsolute8(memory, cpu, 0x4203u, 0);
    LoadA8(cpu, DirectByte(memory, cpu, 0x57u));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x55u));
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4216u, 0));
    SetAccumulatorWidth(cpu, 0);
    StoreWordAbsolute(memory, cpu, 0x4202u, cpu->accumulator);
    Write16Direct(memory, cpu, 0x63u, cpu->x);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, DirectByte(memory, cpu, 0x54u));
    SetAccumulatorWidth(cpu, 0);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4216u, 0));
    StoreWordAbsolute(memory, cpu, 0x4202u, cpu->accumulator);
    Write16Direct(memory, cpu, 0x65u, cpu->x);
    LoadX16(cpu, Read16Direct(memory, cpu, 0x55u));
    cpu->carry = 0;
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4216u, 0));
    StoreWordAbsolute(memory, cpu, 0x4202u, cpu->x);
    Add16Value(cpu, Read16Direct(memory, cpu, 0x64u));
    if (cpu->carry) {
        BattleIncrement66(memory, cpu);
        cpu->carry = 0;
    }
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4216u, 0));
    if (cpu->carry)
        BattleIncrement66(memory, cpu);
    Write16Direct(memory, cpu, 0x64u, cpu->accumulator);       /* DCE6 */
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtlFrame(memory, cpu);
}

/* $85:DC6F: $5D-$5F /= $54, 24/8 via $4204. */
static void BattleDivide(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x85u, return_address);
    PushDataBank(memory, cpu);                                 /* DC6F */
    Push8(memory, cpu, 0x85u);
    PullDataBank(memory, cpu);
    LoadX16(cpu, Read16Direct(memory, cpu, 0x5eu));
    StoreWordAbsolute(memory, cpu, 0x4204u, cpu->x);
    LoadA8(cpu, DirectByte(memory, cpu, 0x54u));
    StoreAAbsolute8(memory, cpu, 0x4206u, 0);
    PushIndex(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x5du));
    ExchangeAccumulatorBytes(cpu);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4214u, 0));
    LoadAAbsolute8(memory, cpu, 0x4216u, 0);
    ExchangeAccumulatorBytes(cpu);
    LoadY16(cpu, cpu->accumulator);
    StoreWordAbsolute(memory, cpu, 0x4204u, cpu->y);
    LoadA8(cpu, DirectByte(memory, cpu, 0x54u));
    StoreAAbsolute8(memory, cpu, 0x4206u, 0);
    LoadA8(cpu, (uint8_t)cpu->x);
    ExchangeAccumulatorBytes(cpu);
    StoreADirect8(memory, cpu, 0x5fu);
    LoadA8(cpu, 0x00u);
    SetAccumulatorWidth(cpu, 0);
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4214u, 0));
    Write16Direct(memory, cpu, 0x5du, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    PullDataBank(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $85:DCEA: A times a random 16-bit fraction. */
static void BattleRandomScale(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    static const struct {
        uint8_t limit;
        uint16_t call;
    } rolls[3] = {{0x80u, 0xdcf7u}, {0x80u, 0xdcffu}, {0x04u, 0xdd07u}};
    unsigned i;

    SimulateJslFrame(memory, cpu, 0x85u, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* DCEA */
    PushDataBank(memory, cpu);
    Push8(memory, cpu, 0x85u);
    PullDataBank(memory, cpu);
    if (cpu->accumulator_is_8_bit)
        StoreADirect8(memory, cpu, 0x54u);
    else
        Write16Direct(memory, cpu, 0x54u, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    for (i = 0; i < 3u; ++i) {
        LoadA8(cpu, rolls[i].limit);
        SimulateJslFrame(memory, cpu, 0x85u, rolls[i].call);
        Lufia2RandomScale(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        if (i < 2u)
            StoreADirect8(memory, cpu, i == 0 ? 0x56u : 0x57u);
    }
    for (i = 0; i < 2u; ++i) {
        const uint32_t address = DirectAddress(cpu, (uint8_t)(0x56u + i));
        const uint8_t value = Read8(memory, address);
        const uint8_t in = (uint8_t)(A8(cpu) & 1u);

        LsrA8(cpu);                                            /* DD08 */
        cpu->carry = value >> 7;
        Write8(memory, address, (uint8_t)((value << 1) | in));
        SetNz8(cpu, (uint8_t)((value << 1) | in));
    }
    BattleMultiply(memory, cpu, 0xdd11u);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x65u));
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtlFrame(memory, cpu);
}

/* $85:DB6D: 32/16 shift-subtract divide of $63-$66 by $58. */
static void BattleLongDivide(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    unsigned i;

    SimulateJslFrame(memory, cpu, 0x85u, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* DB6D */
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x66u));
    And16(cpu, 0x00ffu);
    LoadX16(cpu, Read16Direct(memory, cpu, 0x64u));
    Write16Direct(memory, cpu, 0x65u, cpu->x);
    LoadX16(cpu, Read16Direct(memory, cpu, 0x63u));
    Write16Direct(memory, cpu, 0x64u, cpu->x);
    for (i = 0; i < 16u; ++i) {
        const uint16_t low = Read16Direct(memory, cpu, 0x63u);
        const uint16_t high = Read16Direct(memory, cpu, 0x65u);
        const uint16_t a = cpu->accumulator;

        Write16Direct(memory, cpu, 0x63u, (uint16_t)(low << 1));
        Write16Direct(memory, cpu, 0x65u,
            (uint16_t)((high << 1) | (low >> 15)));
        LoadA16(cpu, (uint16_t)((a << 1) | (high >> 15)));
        cpu->carry = a >> 15;
        if (!cpu->carry) {
            Compare16(cpu, cpu->accumulator,
                Read16Direct(memory, cpu, 0x58u));
            if (!cpu->carry)
                continue;
        }
        Add16Value(cpu, (uint16_t)~Read16Direct(memory, cpu, 0x58u));
        Write16Direct(memory, cpu, 0x63u,
            (uint16_t)(Read16Direct(memory, cpu, 0x63u) + 1u));
    }
    UnpackStatus(cpu, Pull8(memory, cpu));                     /* DC6D */
    SimulateRtlFrame(memory, cpu);
}

/* $85:C099: X = $85:9E47 word for index A. */
static void BattleStatOffset(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* C099 */
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    PushAccumulator16(memory, cpu);
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadX16(cpu, Read16Long(memory, LongIndexedAddress(0x859e47u, cpu->x)));
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $85:C05F: X = stat A of battler ($BE); bytes at $0E/$BC. */
static void BattleStat(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* C05F */
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    PushAccumulator16(memory, cpu);
    PushY(memory, cpu);
    BattleStatOffset(memory, cpu, 0xc066u);
    LoadY16(cpu, cpu->x);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu,
        Read16Direct(memory, cpu, 0xbeu), cpu->y));
    if (cpu->y == 0x00bcu || cpu->y == 0x000eu)
        And16(cpu, 0x00ffu);
    else
        Compare16(cpu, cpu->y, 0x000eu);
    TransferAToX(cpu);
    cpu->y = PullIndexValue(memory, cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $85:C117: mask of battlers without status bit 2. */
static void BattleActiveMask(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    const uint8_t enemies = (uint8_t)(A8(cpu) & 0x80u);

    SimulateJsrFrame(memory, cpu, return_address);
    StoreZeroAbsolute8(memory, cpu, 0x09fau, 0);               /* C117 */
    cpu->zero = enemies == 0;
    SetAccumulatorWidth(cpu, 0);
    LoadX16(cpu, enemies ? 0x000au : 0x0008u);
    do {
        const uint32_t mask = AbsoluteIndexedAddress(cpu, 0x09fau, 0);
        uint16_t bits;

        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu,
            enemies ? 0x0a6eu : 0x0a64u, cpu->x));
        cpu->carry = 0;
        if (!cpu->zero) {
            LoadY16(cpu, cpu->accumulator);
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x000fu, cpu->y));
            cpu->zero = (cpu->accumulator & 0x0004u) == 0;
            cpu->carry = cpu->zero;
        }
        bits = (uint16_t)(Read8(memory, mask) |
            (Read8(memory, AbsoluteIndexedAddress(cpu, 0x09fbu, 0)) << 8));
        {
            const uint8_t out = (uint8_t)(bits >> 15);

            bits = (uint16_t)((bits << 1) | cpu->carry);
            cpu->carry = out;
        }
        Write8(memory, mask, (uint8_t)bits);
        Write8(memory, AbsoluteIndexedAddress(cpu, 0x09fbu, 0),
            (uint8_t)(bits >> 8));
        SetNz16(cpu, bits);
        cpu->x = (uint16_t)(cpu->x - 2u);
        SetNz16(cpu, cpu->x);
    } while (!cpu->negative);
    SetAccumulatorWidth(cpu, 1);
    LoadAAbsolute8(memory, cpu, 0x09fau, 0);
    if (enemies)
        Or8(cpu, 0x80u);
    SimulateRtsFrame(memory, cpu);
}

/* $85:B452: battle script VM; other opcodes run on LLE. */
Lufia2ActorPrimaryUpdateResult Lufia2BattleScript(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    Lufia2ActorPrimaryUpdateResult result;
    unsigned opcodes;

    PushDataBank(memory, cpu);                                 /* B452 */
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    PushAccumulator16(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    StoreZeroAbsolute8(memory, cpu, 0x0a60u, 0);
    for (opcodes = 0;; ++opcodes) {
        uint16_t handler;

        SetAccumulatorWidth(cpu, 1);                           /* B45E */
        BattleScriptBases(memory, cpu);
        TransferDirectToA(cpu);
        LoadA8(cpu, Read8(memory, DirectLongPointer(memory, cpu, 0xbbu)));
        SetAccumulatorWidth(cpu, 0);
        AslA16(cpu);
        TransferAToX(cpu);
        BattleScriptAdvance(memory, cpu);
        SetAccumulatorWidth(cpu, 1);
        Push8(memory, cpu, 0x85u);
        PullDataBank(memory, cpu);
        handler = (uint16_t)(
            Read8(memory, 0x850000u | (uint16_t)(0xb483u + cpu->x)) |
            (Read8(memory, 0x850000u | (uint16_t)(0xb484u + cpu->x)) << 8));
        switch (opcodes < 4096u ? handler : 0u) {
        case 0xb473u:                                          /* end */
        case 0xb476u:                                          /* end, $FF */
            if (handler == 0xb473u)
                TransferDirectToA(cpu);
            else
                LoadA8(cpu, 0xffu);
            StoreAAbsolute8(memory, cpu, 0x0a5bu, 0);          /* B478 */
            SetAccumulatorWidth(cpu, 0);
            SetIndexWidth(cpu, 0);
            cpu->y = PullIndexValue(memory, cpu);
            cpu->x = PullIndexValue(memory, cpu);
            PullAccumulator16(memory, cpu);
            UnpackStatus(cpu, Pull8(memory, cpu));
            PullDataBank(memory, cpu);
            return FieldLoopResult(0x85b482u);
        case 0xb551u:                                          /* jump */
            BattleScriptJump(memory, cpu);
            break;
        case 0xb582u:                                          /* random */
            BattleScriptByte(memory, cpu, 0xb584u);
            StoreADirect8(memory, cpu, 0x54u);
            LoadA8(cpu, 0xffu);
            SimulateJslFrame(memory, cpu, 0x85u, 0xb58cu);
            Lufia2RandomScale(memory, cpu);
            SimulateRtlFrame(memory, cpu);
            cpu->carry = 1;
            Sbc8(cpu, DirectByte(memory, cpu, 0x54u));
            if (!cpu->carry)
                BattleScriptJump(memory, cpu);
            else
                BattleScriptWord(memory, cpu, 0xb594u);
            break;
        case 0xb598u:                                          /* equal */
        case 0xb5adu:                                          /* not equal */
            BattleScriptOperands(memory, cpu, handler);
            Compare16(cpu, cpu->x, Read16Direct(memory, cpu, 0x54u));
            if (cpu->zero == (handler == 0xb598u))
                BattleScriptJump(memory, cpu);
            else
                BattleScriptWord(memory, cpu, (uint16_t)(handler + 17u));
            break;
        case 0xb60au:                                          /* >= */
            BattleScriptOperands(memory, cpu, handler);
            BattleScriptCompare(memory, cpu);
            if (cpu->negative == cpu->overflow)
                BattleScriptJump(memory, cpu);
            else
                BattleScriptWord(memory, cpu, 0xb629u);
            break;
        case 0xb62du:                                          /* <= */
            BattleScriptOperands(memory, cpu, handler);
            BattleScriptCompare(memory, cpu);
            if (cpu->zero || cpu->negative != cpu->overflow)
                BattleScriptJump(memory, cpu);
            else
                BattleScriptWord(memory, cpu, 0xb64eu);
            break;
        case 0xb670u:                                          /* set */
            BattleScriptByte(memory, cpu, 0xb672u);
            BattleScriptValue(memory, cpu, 0xb675u);
            BattleScriptWrite(memory, cpu, 0xb678u);
            break;
        case 0xb67cu:                                          /* add */
        case 0xb698u:                                          /* subtract */
            BattleScriptOperands(memory, cpu, handler);
            PushAccumulator8(memory, cpu);
            SetAccumulatorWidth(cpu, 0);
            if (handler == 0xb67cu) {
                LoadA16(cpu, cpu->x);
                cpu->carry = 0;
                Add16Value(cpu, Read16Direct(memory, cpu, 0x54u));
            } else {
                LoadA16(cpu, Read16Direct(memory, cpu, 0x54u));
                Write16Direct(memory, cpu, 0x54u, cpu->x);
                cpu->carry = 1;
                Add16Value(cpu, (uint16_t)~Read16Direct(memory, cpu, 0x54u));
            }
            BattleScriptStoreBinary(memory, cpu, cpu->accumulator,
                handler == 0xb67cu ? 0xb694u : 0xb6b3u);
            break;
        case 0xb849u:                                          /* and */
        case 0xb864u:                                          /* or */
        case 0xb87fu: {                                        /* xor */
            uint16_t value;

            BattleScriptByte(memory, cpu, (uint16_t)(handler + 2u));
            PushAccumulator8(memory, cpu);
            BattleScriptRead(memory, cpu, (uint16_t)(handler + 6u));
            Write16Direct(memory, cpu, 0x54u, cpu->x);
            BattleScriptValue(memory, cpu, (uint16_t)(handler + 11u));
            SetAccumulatorWidth(cpu, 0);
            value = Read16Direct(memory, cpu, 0x54u);
            value = handler == 0xb849u ? (uint16_t)(cpu->x & value)
                : handler == 0xb864u ? (uint16_t)(cpu->x | value)
                : (uint16_t)(cpu->x ^ value);
            BattleScriptStoreBinary(memory, cpu, value,
                (uint16_t)(handler + 0x17u));
            break;
        }
        case 0xb89au:
            BattleScriptUnary(memory, cpu, handler, 0);
            break;
        case 0xb8b4u:
            BattleScriptUnary(memory, cpu, handler, 1);
            break;
        case 0xb8ccu:
            BattleScriptUnary(memory, cpu, handler, 2);
            break;
        case 0xb8ebu:                                          /* leader id */
            LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x11e8u, 0));
            BattleScriptByte(memory, cpu, 0xb8f0u);
            BattleScriptWrite(memory, cpu, 0xb8f3u);
            break;
        case 0xb91fu:                                          /* $7F:F45C */
            BattleScriptWord(memory, cpu, 0xb921u);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, cpu->x);
            Write16Long(memory, 0x7ff45cu, cpu->accumulator);
            break;
        case 0xb92cu:                                          /* $7F:F45E */
            BattleScriptByte(memory, cpu, 0xb92eu);
            Write8(memory, 0x7ff45eu, A8(cpu));
            BattleScriptByte(memory, cpu, 0xb935u);
            Write8(memory, 0x7ff460u, A8(cpu));
            break;
        case 0xb560u:                                          /* jump if F42E */
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16Long(memory, 0x7ff42eu));
            if (!cpu->zero)
                BattleScriptJump(memory, cpu);
            else
                BattleScriptWord(memory, cpu, 0xb56au);
            break;
        case 0xb5c2u:                                          /* > */
            BattleScriptOperands(memory, cpu, handler);
            BattleScriptCompare(memory, cpu);
            if (!cpu->zero && cpu->negative == cpu->overflow)
                BattleScriptJump(memory, cpu);
            else
                BattleScriptWord(memory, cpu, 0xb5e3u);
            break;
        case 0xb5e7u:                                          /* < */
            BattleScriptOperands(memory, cpu, handler);
            BattleScriptCompare(memory, cpu);
            if (cpu->negative != cpu->overflow)
                BattleScriptJump(memory, cpu);
            else
                BattleScriptWord(memory, cpu, 0xb606u);
            break;
        case 0xb93du:                                          /* move setup */
            BattleScriptWord(memory, cpu, 0xb93fu);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, cpu->x);
            Write16Long(memory, 0x7ff45cu, cpu->accumulator);
            LoadA16(cpu, 0x0020u);
            Write16Long(memory, 0x7ff45au, cpu->accumulator);
            LoadA16(cpu, 0x0005u);
            Write16Long(memory, 0x7ff454u, cpu->accumulator);
            BattleScriptValue(memory, cpu, 0xb957u);
            LoadA16(cpu, (uint16_t)(0u - cpu->x));
            Write16Long(memory, 0x7ff462u, cpu->accumulator);
            LoadA16(cpu, 0x0020u);
            Write16Long(memory, 0x7ff464u, cpu->accumulator);
            BattleScriptByte(memory, cpu, 0xb96au);
            break;
        case 0xb9b8u:                                          /* step setup */
            BattleScriptWord(memory, cpu, 0xb9bau);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, (uint16_t)(0u - cpu->x));
            Write16Long(memory, 0x7ff462u, cpu->accumulator);
            LoadA16(cpu, 0x0020u);
            Write16Long(memory, 0x7ff464u, cpu->accumulator);
            Write16Long(memory, 0x7ff45au, cpu->accumulator);
            LoadA16(cpu, 0x0005u);
            break;
        case 0xb9dau:                                          /* F44E word+byte */
            BattleScriptByte(memory, cpu, 0xb9dcu);
            SetAccumulatorWidth(cpu, 0);
            And16(cpu, 0x00ffu);
            TransferAToX(cpu);
            LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x859f04u, cpu->x)));
            And16(cpu, 0x00ffu);
            PushAccumulator16(memory, cpu);
            BattleScriptValue(memory, cpu, 0xb9edu);
            LoadA16(cpu, cpu->x);
            cpu->x = PullIndexValue(memory, cpu);
            Write16Long(memory, LongIndexedAddress(0x7ff44eu, cpu->x),
                cpu->accumulator);
            IncrementX16(cpu);
            IncrementX16(cpu);
            SetAccumulatorWidth(cpu, 1);
            BattleScriptByte(memory, cpu, 0xb9fau);
            Write8(memory, LongIndexedAddress(0x7ff44eu, cpu->x), A8(cpu));
            break;
        case 0xba02u:                                          /* F44E byte, clear */
            BattleScriptByte(memory, cpu, 0xba04u);
            SetAccumulatorWidth(cpu, 0);
            And16(cpu, 0x00ffu);
            TransferAToX(cpu);
            LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x859f04u, cpu->x)));
            And16(cpu, 0x00ffu);
            cpu->carry = 0;
            Add16Value(cpu, 0x0004u);
            TransferAToX(cpu);
            BattleScriptByte(memory, cpu, 0xba19u);
            Write16Long(memory, LongIndexedAddress(0x7ff44eu, cpu->x),
                cpu->accumulator);
            cpu->x = (uint16_t)(cpu->x - 2u);
            SetNz16(cpu, cpu->x);
            TransferDirectToA(cpu);
            Write16Long(memory, LongIndexedAddress(0x7ff44eu, cpu->x),
                cpu->accumulator);
            break;
        case 0xba28u:                                          /* F44E byte */
        case 0xba47u:
            BattleScriptByte(memory, cpu, (uint16_t)(handler + 2u));
            SetAccumulatorWidth(cpu, 0);
            And16(cpu, 0x00ffu);
            TransferAToX(cpu);
            LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x859f0fu, cpu->x)));
            And16(cpu, 0x00ffu);
            if (handler == 0xba28u)
                LoadA16(cpu, (uint16_t)(cpu->accumulator + 2u));
            TransferAToX(cpu);
            SetAccumulatorWidth(cpu, 1);
            BattleScriptByte(memory, cpu,
                handler == 0xba28u ? 0xba3fu : 0xba5cu);
            Write8(memory, LongIndexedAddress(0x7ff44eu, cpu->x), A8(cpu));
            break;
        case 0xba64u:                                          /* action 1 */
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, 0x0001u);
            Write16Long(memory, 0x7ff454u, cpu->accumulator);
            LoadA16(cpu, 0x0000u);
            Write16Long(memory, 0x7ff456u, cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            StoreAImmediate8(memory, cpu, 0xffu, 0x1262u);
            StoreZeroAbsolute8(memory, cpu, 0x1269u, 0);
            break;
        case 0xba81u:                                          /* action 4 */
        case 0xba8au:                                          /* action 6 */
        case 0xbb46u:                                          /* action 13 */
        case 0xbd6eu:                                          /* action 12 */
            LoadA8(cpu, handler == 0xba81u ? 0x04u : handler == 0xba8au
                ? 0x06u : handler == 0xbb46u ? 0x0du : 0x0cu);
            Write8(memory, 0x7ff454u, A8(cpu));
            break;
        case 0xba93u:                                          /* action 3 */
            LoadA8(cpu, 0x03u);
            Write8(memory, 0x7ff454u, A8(cpu));
            BattleScriptWord(memory, cpu, 0xba9bu);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, cpu->x);
            Write16Long(memory, 0x7ff456u, cpu->accumulator);
            break;
        case 0xbd5eu:                                          /* action 11 */
            LoadA8(cpu, 0x0bu);
            Write8(memory, 0x7ff454u, A8(cpu));
            BattleScriptByte(memory, cpu, 0xbd66u);
            Write8(memory, 0x7ff456u, A8(cpu));
            break;
        case 0xbb4fu:                                          /* party flag count */
            LoadA8(cpu, Read8(memory, 0x7ff450u));
            if (cpu->negative) {
                LoadX16(cpu, 0x0000u);
            } else {
                Write8(memory, DirectAddress(cpu, 0x54u), 0x00u);
                LoadY16(cpu, 0x0008u);
                do {
                    LoadX16(cpu, Read16AbsoluteIndexed(
                        memory, cpu, 0x0a64u, cpu->y));        /* BB5F */
                    if (!cpu->zero) {
                        LoadAAbsolute8(memory, cpu, 0x000fu, cpu->x);
                        BitImmediate8(cpu, 0x04u);
                        if (!cpu->zero)
                            IncrementDirect8(memory, cpu, 0x54u);
                    }
                    cpu->y = (uint16_t)(cpu->y - 2u);
                    SetNz16(cpu, cpu->y);
                } while (!cpu->negative);
                TransferDirectToA(cpu);
                LoadA8(cpu, DirectByte(memory, cpu, 0x54u));
                TransferAToX(cpu);
            }
            BattleScriptByte(memory, cpu, 0xbb77u);
            BattleScriptWrite(memory, cpu, 0xbb7au);
            break;
        case 0xbb7eu:                                          /* side leader */
            TransferDirectToA(cpu);
            LoadA8(cpu, Read8(memory, 0x7ff44eu));
            if (cpu->negative) {
                LoadAAbsolute8(memory, cpu, 0x15feu, 0);
            } else {
                LoadAAbsolute8(memory, cpu, 0x0a13u, 0);
                LoadAAbsolute8(memory, cpu,
                    cpu->zero ? 0x0a7au : 0x153cu, 0);
            }
            TransferAToX(cpu);
            BattleScriptByte(memory, cpu, 0xbb9au);
            BattleScriptWrite(memory, cpu, 0xbb9du);
            break;
        case 0xbccfu:                                          /* $0A62 byte */
            BattleScriptByte(memory, cpu, 0xbcd1u);
            StoreAAbsolute8(memory, cpu, 0x0a62u, 0);
            break;
        case 0xbcd8u:                                          /* $0A62 if F42E */
            BattleScriptByte(memory, cpu, 0xbcdau);
            StoreADirect8(memory, cpu, 0x54u);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16Long(memory, 0x7ff42eu));
            if (!cpu->zero) {
                LoadA16(cpu, Read16Direct(memory, cpu, 0x54u));
                Write16Absolute(memory, cpu, 0x0a62u, cpu->accumulator);
            }
            break;
        case 0xbe22u:                                          /* timer $1264 */
            BattleScriptValue(memory, cpu, 0xbe24u);
            Write16Absolute(memory, cpu, 0x1264u, cpu->x);
            break;
        case 0xbe68u:                                          /* F450 mask */
            LoadAAbsolute8(memory, cpu, 0x0a5du, 0);
            if (!cpu->negative) {
                LoadA8(cpu, (uint8_t)(Read8(memory, 0x7ff450u) | 0xefu));
                LoadA8(cpu, (uint8_t)(A8(cpu) & Read8(memory,
                    AbsoluteIndexedAddress(cpu, 0x0a5du, 0))));
                Write8(memory, 0x7ff450u, A8(cpu));
            }
            break;
        case 0xb6b7u:                                          /* multiply */
            BattleScriptOperands(memory, cpu, handler);
            Write16Direct(memory, cpu, 0x56u, cpu->x);
            PushAccumulator8(memory, cpu);
            BattleMultiply(memory, cpu, 0xb6c8u);
            LoadA8(cpu, Pull8(memory, cpu));
            LoadX16(cpu, Read16Direct(memory, cpu, 0x63u));
            BattleScriptWrite(memory, cpu, 0xb6ceu);
            break;
        case 0xb6d2u: {                                        /* divide */
            uint8_t negative;

            BattleScriptByte(memory, cpu, 0xb6d4u);
            BattleScriptRead(memory, cpu, 0xb6d7u);
            PushAccumulator8(memory, cpu);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, cpu->x);
            negative = cpu->negative;
            if (negative)
                LoadA16(cpu, (uint16_t)(0u - cpu->accumulator));
            Write16Direct(memory, cpu, 0x5du, cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            BattleScriptValue(memory, cpu, negative ? 0xb701u : 0xb6e4u);
            LoadA8(cpu, (uint8_t)cpu->x);
            StoreADirect8(memory, cpu, 0x54u);
            Write8(memory, DirectAddress(cpu, 0x5fu), 0x00u);
            BattleDivide(memory, cpu, negative ? 0xb70au : 0xb6edu);
            if (negative) {
                SetAccumulatorWidth(cpu, 0);
                LoadA16(cpu, (uint16_t)(0u - Read16Direct(memory, cpu, 0x5du)));
                TransferAToX(cpu);
                SetAccumulatorWidth(cpu, 1);
            } else {
                LoadX16(cpu, Read16Direct(memory, cpu, 0x5du));
            }
            LoadA8(cpu, Pull8(memory, cpu));
            BattleScriptWrite(memory, cpu, negative ? 0xb719u : 0xb6f3u);
            break;
        }
        case 0xb73bu:                                          /* random scale */
            BattleScriptByte(memory, cpu, 0xb73du);
            BattleScriptRead(memory, cpu, 0xb740u);
            PushAccumulator8(memory, cpu);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, cpu->x);
            BattleRandomScale(memory, cpu, 0xb748u);
            TransferAToX(cpu);
            SetAccumulatorWidth(cpu, 1);
            LoadA8(cpu, Pull8(memory, cpu));
            BattleScriptWrite(memory, cpu, 0xb74fu);
            break;
        case 0xb753u:                                          /* long divide */
            BattleScriptByte(memory, cpu, 0xb755u);
            BattleScriptRead(memory, cpu, 0xb758u);
            PushAccumulator8(memory, cpu);
            Write16Direct(memory, cpu, 0x58u, cpu->x);
            BattleScriptByte(memory, cpu, 0xb75eu);
            BattleScriptRead(memory, cpu, 0xb761u);
            Write16Direct(memory, cpu, 0x65u, cpu->x);
            SetAccumulatorWidth(cpu, 0);
            Write16Direct(memory, cpu, 0x63u, 0x0000u);
            BattleLongDivide(memory, cpu, 0xb76bu);
            LoadA16(cpu, Read16Direct(memory, cpu, 0x64u));
            And16(cpu, 0x00ffu);
            TransferAToX(cpu);
            SetAccumulatorWidth(cpu, 1);
            LoadA8(cpu, Pull8(memory, cpu));
            BattleScriptWrite(memory, cpu, 0xb777u);
            break;
        case 0xb96eu:                                          /* move by stats */
            BattleScriptWord(memory, cpu, 0xb970u);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, cpu->x);
            Write16Long(memory, 0x7ff45cu, cpu->accumulator);
            LoadA16(cpu, 0x01dcu);
            Write16Long(memory, 0x7ff45au, cpu->accumulator);
            LoadA16(cpu, 0x000au);
            Write16Long(memory, 0x7ff454u, cpu->accumulator);
            BattleScriptValue(memory, cpu, 0xb988u);
            Write16Direct(memory, cpu, 0xcau, cpu->x);
            LoadA16(cpu, 0x0008u);
            BattleStat(memory, cpu, 0xb990u);
            LoadA16(cpu, cpu->x);
            cpu->carry = 0;
            Add16Value(cpu, Read16Direct(memory, cpu, 0xcau));
            Write16Direct(memory, cpu, 0xcau, cpu->accumulator);
            LoadA16(cpu, 0x0011u);
            BattleStat(memory, cpu, 0xb99cu);
            LoadA16(cpu, cpu->x);
            cpu->carry = 0;
            Add16Value(cpu, Read16Direct(memory, cpu, 0xcau));
            Write16Direct(memory, cpu, 0xcau, cpu->accumulator);
            LoadA16(cpu, (uint16_t)(0u - cpu->accumulator));
            Write16Long(memory, 0x7ff462u, cpu->accumulator);
            LoadA16(cpu, 0x0020u);
            Write16Long(memory, 0x7ff464u, cpu->accumulator);
            BattleScriptByte(memory, cpu, 0xb9b4u);
            break;
        case 0xbcedu:                                          /* jump back */
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, 0x0001u);
            Write16Long(memory, 0x7ff454u, cpu->accumulator);
            TransferDirectToA(cpu);
            Write16Long(memory, 0x7ff456u, cpu->accumulator);
            LoadA16(cpu, Read16Long(memory, 0x7ff462u));
            if (cpu->zero) {
                LoadA16(cpu, 0x0004u);
                BattleStat(memory, cpu, 0xbd06u);
                LoadA16(cpu, cpu->x);
                cpu->carry = 0;
                Add16Value(cpu, Read16Long(memory, 0x7ff4cau));
                PushAccumulator16(memory, cpu);
                LoadA16(cpu, 0x000du);
                BattleStat(memory, cpu, 0xbd13u);
                LoadA16(cpu, cpu->x);
                cpu->carry = 0;
                Add16Value(cpu, (uint16_t)(
                    Read8(memory, (uint16_t)(cpu->stack + 1u)) |
                    (Read8(memory, (uint16_t)(cpu->stack + 2u)) << 8)));
                LoadA16(cpu, (uint16_t)(0u - cpu->accumulator));
                Write16Long(memory, 0x7ff462u, cpu->accumulator);
                PullAccumulator16(memory, cpu);
            }
            LoadA16(cpu, 0x0040u);                             /* BD21 */
            Write16Long(memory, 0x7ff464u, cpu->accumulator);
            break;
        case 0xbd2cu:                                          /* extend jump */
            LoadA8(cpu, 0x08u);
            BattleStat(memory, cpu, 0xbd30u);
            Write16Direct(memory, cpu, 0xcau, cpu->x);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16Long(memory, 0x7ff462u));
            if (!cpu->zero) {
                if (cpu->negative) {
                    LoadA16(cpu, (uint16_t)(0u - cpu->accumulator));
                    cpu->carry = 0;
                    Add16Value(cpu, Read16Direct(memory, cpu, 0xcau));
                    LoadA16(cpu, (uint16_t)(0u - cpu->accumulator));
                } else {
                    cpu->carry = 0;
                    Add16Value(cpu, Read16Direct(memory, cpu, 0xcau));
                }
                Write16Long(memory, 0x7ff462u, cpu->accumulator);
            }
            break;
        case 0xbe56u:                                          /* living mask */
            LoadA8(cpu, Read8(memory, 0x7ff450u));
            BattleActiveMask(memory, cpu, 0xbe5cu);
            LoadA8(cpu, (uint8_t)(A8(cpu) & Read8(memory, 0x7ff450u)));
            Write8(memory, 0x7ff450u, A8(cpu));
            break;
        case 0xbd77u:                                          /* call */
            BattleScriptWord(memory, cpu, 0xbd79u);
            Write16Direct(memory, cpu, 0xcau, cpu->x);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, cpu->x);
            AslA16(cpu);
            TransferAToX(cpu);
            SetAccumulatorWidth(cpu, 1);
            LoadY16(cpu, Read16Direct(memory, cpu, 0xbbu));
            Write16Absolute(memory, cpu, 0x0a48u, cpu->y);
            LoadA8(cpu, DirectByte(memory, cpu, 0xbdu));
            StoreAAbsolute8(memory, cpu, 0x0a4au, 0);
            LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a42u, 0));
            Write16Absolute(memory, cpu, 0x0a45u, cpu->y);
            LoadAAbsolute8(memory, cpu, 0x0a44u, 0);
            StoreAAbsolute8(memory, cpu, 0x0a47u, 0);
            LoadA8(cpu, 0x96u);
            StoreADirect8(memory, cpu, 0xbdu);
            StoreAAbsolute8(memory, cpu, 0x0a44u, 0);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16Long(memory,
                LongIndexedAddress(0x96faddu, cpu->x)));
            cpu->carry = 0;
            Add16Value(cpu, 0xfaddu);
            Write16Absolute(memory, cpu, 0x0a42u, cpu->accumulator);
            Write16Direct(memory, cpu, 0xbbu, cpu->accumulator);
            break;
        case 0xbdb2u:                                          /* return */
            LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a48u, 0));
            Write16Direct(memory, cpu, 0xbbu, cpu->y);
            LoadAAbsolute8(memory, cpu, 0x0a4au, 0);
            StoreADirect8(memory, cpu, 0xbdu);
            LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a45u, 0));
            Write16Absolute(memory, cpu, 0x0a42u, cpu->y);
            LoadAAbsolute8(memory, cpu, 0x0a47u, 0);
            StoreAAbsolute8(memory, cpu, 0x0a44u, 0);
            break;
        default:                                               /* B470 */
            result = FieldLoopHandoff(cpu, 0x85b470u);
            result.dispatches = opcodes;
            return result;
        }
    }
}
