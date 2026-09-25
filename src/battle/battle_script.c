/* Battle script VM ($85:B452). */

#include "core/cpu_internal.h"
#include "lufia2/battle.h"
#include "lufia2/system.h"
#include "system/system_internal.h"
#include "system/wram.h"

/* Battle script WRAM. */
#define BATTLE_DP_SCRIPT_POINTER 0xbbu        /* 24-bit cursor */
#define BATTLE_DP_SCRIPT_BANK 0xbdu
#define BATTLE_DP_LOCALS_F44E 0xbeu           /* local base of battler $F44E */
#define BATTLE_DP_LOCALS_F450 0xc1u           /* local base of battler $F450 */
#define BATTLE_SCRIPT_BASE 0x0a42u            /* jump origin */
#define BATTLE_SCRIPT_BANK 0x0a44u
#define BATTLE_CALLER_BASE 0x0a45u            /* saved by $42 */
#define BATTLE_CALLER_BANK 0x0a47u
#define BATTLE_CALLER_POINTER 0x0a48u
#define BATTLE_CALLER_POINTER_BANK 0x0a4au
#define BATTLE_LOCAL_SELECT 0x0a52u           /* zero: $BE locals, else $C1 */
#define BATTLE_SCRIPT_RESULT 0x0a5bu          /* 0 or $FF at the end */
#define BATTLE_LEADER_ID 0x11e8u
#define BATTLE_GLOBAL_VARIABLES 0x7ff40eu
#define BATTLE_CONDITION 0x7ff42eu
#define BATTLE_ACTION_CODE 0x7ff454u

/* $85:C168: battler byte A to its variable base in X. */
static void BattleScriptSlot(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
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
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0xb462u);
    LoadA8(cpu, Read8(memory, 0x7ff450u));                     /* C4DE */
    BattleScriptSlot(memory, cpu, 0xc4e4u);
    Write16Direct(memory, cpu, BATTLE_DP_LOCALS_F450, cpu->x);
    LoadA8(cpu, Read8(memory, 0x7ff44eu));
    BattleScriptSlot(memory, cpu, 0xc4edu);
    Write16Direct(memory, cpu, BATTLE_DP_LOCALS_F44E, cpu->x);
    SimulateRtsFrame(memory, cpu);
}

/* INC $BB, 16-bit. */
static void BattleScriptAdvance(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const uint16_t pointer = (uint16_t)(Read16Direct(memory, cpu, BATTLE_DP_SCRIPT_POINTER) + 1u);

    Write16Direct(memory, cpu, BATTLE_DP_SCRIPT_POINTER, pointer);
    SetNz16(cpu, pointer);
}

/* $85:BFBF: next script byte into A; flags kept. */
static void BattleScriptByte(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* BFBF */
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, DirectLongPointer(memory, cpu, BATTLE_DP_SCRIPT_POINTER)));
    SetAccumulatorWidth(cpu, 0);
    BattleScriptAdvance(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $85:BFCA: next script word into X. */
static void BattleScriptWord(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* BFCA */
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    PushAccumulator16(memory, cpu);
    LoadA16(cpu, Read16Long(memory, DirectLongPointer(memory, cpu, BATTLE_DP_SCRIPT_POINTER)));
    BattleScriptAdvance(memory, cpu);
    BattleScriptAdvance(memory, cpu);
    TransferAToX(cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}

/* Selector bit 7: local slot at base $C1 or $BE. */
static uint16_t BattleScriptLocal(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, BATTLE_LOCAL_SELECT, 0));
    And16(cpu, 0x00ffu);
    {
        const uint8_t base = cpu->zero ? BATTLE_DP_LOCALS_F44E : BATTLE_DP_LOCALS_F450;

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
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
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
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(BATTLE_GLOBAL_VARIABLES, cpu->x)));
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
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
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
        Write16Long(memory, LongIndexedAddress(BATTLE_GLOBAL_VARIABLES, cpu->x),
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
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* BFD8 */
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    PushAccumulator16(memory, cpu);
    LoadA16(cpu, Read16Long(memory, DirectLongPointer(memory, cpu, BATTLE_DP_SCRIPT_POINTER)));
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
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);                               /* B551 */
    BattleScriptWord(memory, cpu, 0xb555u);
    LoadA16(cpu, cpu->x);
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, BATTLE_SCRIPT_BASE, 0));
    Write16Direct(memory, cpu, BATTLE_DP_SCRIPT_POINTER, cpu->accumulator);
}

/* Operand fetch shared by the compare and math opcodes. */
static void BattleScriptOperands(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t opcode) {
    BattleScriptByte(memory, cpu, (uint16_t)(opcode + 2u));
    BattleScriptRead(memory, cpu, (uint16_t)(opcode + 5u));
    Write16Direct(memory, cpu, 0x54u, cpu->x);
    BattleScriptValue(memory, cpu, (uint16_t)(opcode + 10u));
}

/* Signed $54 - X, 16-bit, overflow kept. */
static void BattleScriptCompare(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x54u));
    Write16Direct(memory, cpu, 0x54u, cpu->x);
    cpu->carry = 1;
    Add16Value(cpu, (uint16_t)~Read16Direct(memory, cpu, 0x54u));
}

/* Store binary result X into the destination variable. */
static void BattleScriptStoreBinary(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
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
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
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
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const uint16_t value = (uint16_t)(Read16Direct(memory, cpu, 0x66u) + 1u);

    Write16Direct(memory, cpu, 0x66u, value);
    SetNz16(cpu, value);
}

/* $85:DCA3: $63-$66 = $54 * $56, 16x16 via $4202. */
static void BattleMultiply(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x85u, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* DCA3 */
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    LoadA8(cpu, DirectByte(memory, cpu, 0x56u));
    StoreAAbsolute8(memory, cpu, SNES_WRMPYA, 0);
    LoadA8(cpu, DirectByte(memory, cpu, 0x54u));
    StoreAAbsolute8(memory, cpu, SNES_WRMPYB, 0);
    LoadA8(cpu, DirectByte(memory, cpu, 0x57u));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x55u));
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, SNES_RDMPYL, 0));
    SetAccumulatorWidth(cpu, 0);
    StoreWordAbsolute(memory, cpu, SNES_WRMPYA, cpu->accumulator);
    Write16Direct(memory, cpu, 0x63u, cpu->x);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, DirectByte(memory, cpu, 0x54u));
    SetAccumulatorWidth(cpu, 0);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, SNES_RDMPYL, 0));
    StoreWordAbsolute(memory, cpu, SNES_WRMPYA, cpu->accumulator);
    Write16Direct(memory, cpu, 0x65u, cpu->x);
    LoadX16(cpu, Read16Direct(memory, cpu, 0x55u));
    cpu->carry = 0;
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, SNES_RDMPYL, 0));
    StoreWordAbsolute(memory, cpu, SNES_WRMPYA, cpu->x);
    Add16Value(cpu, Read16Direct(memory, cpu, 0x64u));
    if (cpu->carry) {
        BattleIncrement66(memory, cpu);
        cpu->carry = 0;
    }
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, SNES_RDMPYL, 0));
    if (cpu->carry)
        BattleIncrement66(memory, cpu);
    Write16Direct(memory, cpu, 0x64u, cpu->accumulator);       /* DCE6 */
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtlFrame(memory, cpu);
}

/* $85:DC6F: $5D-$5F /= $54, 24/8 via $4204. */
static void BattleDivide(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x85u, return_address);
    PushDataBank(memory, cpu);                                 /* DC6F */
    Push8(memory, cpu, 0x85u);
    PullDataBank(memory, cpu);
    LoadX16(cpu, Read16Direct(memory, cpu, 0x5eu));
    StoreWordAbsolute(memory, cpu, SNES_WRDIVL, cpu->x);
    LoadA8(cpu, DirectByte(memory, cpu, 0x54u));
    StoreAAbsolute8(memory, cpu, SNES_WRDIVB, 0);
    PushIndex(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x5du));
    ExchangeAccumulatorBytes(cpu);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, SNES_RDDIVL, 0));
    LoadAAbsolute8(memory, cpu, SNES_RDMPYL, 0);
    ExchangeAccumulatorBytes(cpu);
    LoadY16(cpu, cpu->accumulator);
    StoreWordAbsolute(memory, cpu, SNES_WRDIVL, cpu->y);
    LoadA8(cpu, DirectByte(memory, cpu, 0x54u));
    StoreAAbsolute8(memory, cpu, SNES_WRDIVB, 0);
    LoadA8(cpu, (uint8_t)cpu->x);
    ExchangeAccumulatorBytes(cpu);
    StoreADirect8(memory, cpu, 0x5fu);
    LoadA8(cpu, 0x00u);
    SetAccumulatorWidth(cpu, 0);
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, SNES_RDDIVL, 0));
    Write16Direct(memory, cpu, 0x5du, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    PullDataBank(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $85:DCEA: A times a random 16-bit fraction. */
static void BattleRandomScale(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
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
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
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
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
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
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
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
        Read16Direct(memory, cpu, BATTLE_DP_LOCALS_F44E), cpu->y));
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
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
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

/* Handlers in the $85:B483 opcode table. */
enum BattleOpcodeHandler {
    BATTLE_OP_END = 0xb473,                                    /* $00 */
    BATTLE_OP_END_FF = 0xb476,                                 /* $4F */
    BATTLE_OP_JUMP = 0xb551,                                   /* $03 */
    BATTLE_OP_JUMP_IF_F42E = 0xb560,                           /* $04 */
    BATTLE_OP_RANDOM_JUMP = 0xb582,                            /* $05 */
    BATTLE_OP_JUMP_IF_EQUAL = 0xb598,                          /* $06 */
    BATTLE_OP_JUMP_IF_NOT_EQUAL = 0xb5ad,                      /* $07 */
    BATTLE_OP_JUMP_IF_GREATER = 0xb5c2,                        /* $08 */
    BATTLE_OP_JUMP_IF_LESS = 0xb5e7,                           /* $09 */
    BATTLE_OP_JUMP_IF_GREATER_EQUAL = 0xb60a,                  /* $0A */
    BATTLE_OP_JUMP_IF_LESS_EQUAL = 0xb62d,                     /* $0B */
    BATTLE_OP_SET = 0xb670,                                    /* $0C */
    BATTLE_OP_ADD = 0xb67c,                                    /* $0D */
    BATTLE_OP_SUBTRACT = 0xb698,                               /* $0E */
    BATTLE_OP_MULTIPLY = 0xb6b7,                               /* $0F */
    BATTLE_OP_DIVIDE = 0xb6d2,                                 /* $10 */
    BATTLE_OP_RANDOM_SCALE = 0xb73b,                           /* $11 */
    BATTLE_OP_LONG_DIVIDE = 0xb753,                            /* $12 */
    BATTLE_OP_AND = 0xb849,                                    /* $16 */
    BATTLE_OP_OR = 0xb864,                                     /* $17 */
    BATTLE_OP_XOR = 0xb87f,                                    /* $18 */
    BATTLE_OP_ABS = 0xb89a,                                    /* $19 */
    BATTLE_OP_NEGATE = 0xb8b4,                                 /* $1A */
    BATTLE_OP_SIGN = 0xb8cc,                                   /* $1B */
    BATTLE_OP_LEADER_ID = 0xb8eb,                              /* $1C */
    BATTLE_OP_SET_F45C = 0xb91f,                               /* $1F */
    BATTLE_OP_SET_F45E = 0xb92c,                               /* $20 */
    BATTLE_OP_MOVE_SETUP = 0xb93d,                             /* $21 */
    BATTLE_OP_MOVE_BY_STATS = 0xb96e,                          /* $22 */
    BATTLE_OP_STEP_SETUP = 0xb9b8,                             /* $23 */
    BATTLE_OP_RECORD_WORD_BYTE = 0xb9da,                       /* $24 */
    BATTLE_OP_RECORD_BYTE_CLEAR = 0xba02,                      /* $25 */
    BATTLE_OP_RECORD_BYTE_26 = 0xba28,                         /* $26 */
    BATTLE_OP_RECORD_BYTE_27 = 0xba47,                         /* $27 */
    BATTLE_OP_ACTION_1 = 0xba64,                               /* $28 */
    BATTLE_OP_ACTION_4 = 0xba81,                               /* $29 */
    BATTLE_OP_ACTION_6 = 0xba8a,                               /* $2A */
    BATTLE_OP_ACTION_3 = 0xba93,                               /* $2B */
    BATTLE_OP_ACTION_13 = 0xbb46,                              /* $2E */
    BATTLE_OP_PARTY_FLAG_COUNT = 0xbb4f,                       /* $2F */
    BATTLE_OP_SIDE_LEADER = 0xbb7e,                            /* $30 */
    BATTLE_OP_SET_0A62 = 0xbccf,                               /* $35 */
    BATTLE_OP_SET_0A62_IF_F42E = 0xbcd8,                       /* $36 */
    BATTLE_OP_JUMP_BACK = 0xbced,                              /* $37 */
    BATTLE_OP_EXTEND_JUMP = 0xbd2c,                            /* $3C */
    BATTLE_OP_ACTION_11 = 0xbd5e,                              /* $3E */
    BATTLE_OP_ACTION_12 = 0xbd6e,                              /* $41 */
    BATTLE_OP_CALL = 0xbd77,                                   /* $42 */
    BATTLE_OP_RETURN = 0xbdb2,                                 /* $43 */
    BATTLE_OP_SET_TIMER = 0xbe22,                              /* $56 */
    BATTLE_OP_LIVING_MASK = 0xbe56,                            /* $4D */
    BATTLE_OP_F450_MASK = 0xbe68,                              /* $50 */
};

/* $05: jump when a random byte is below n. */
static void BattleOpRandomJump(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
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
}

/* $06/$07: jump if equal or not equal. */
static void BattleOpJumpIfEqual(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    BattleScriptOperands(memory, cpu, handler);
    Compare16(cpu, cpu->x, Read16Direct(memory, cpu, 0x54u));
    if (cpu->zero == (handler == BATTLE_OP_JUMP_IF_EQUAL))
        BattleScriptJump(memory, cpu);
    else
        BattleScriptWord(memory, cpu, (uint16_t)(handler + 17u));
}

/* $0A: signed jump if greater or equal. */
static void BattleOpJumpIfGreaterEqual(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    BattleScriptOperands(memory, cpu, handler);
    BattleScriptCompare(memory, cpu);
    if (cpu->negative == cpu->overflow)
        BattleScriptJump(memory, cpu);
    else
        BattleScriptWord(memory, cpu, 0xb629u);
}

/* $0B: signed jump if less or equal. */
static void BattleOpJumpIfLessEqual(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    BattleScriptOperands(memory, cpu, handler);
    BattleScriptCompare(memory, cpu);
    if (cpu->zero || cpu->negative != cpu->overflow)
        BattleScriptJump(memory, cpu);
    else
        BattleScriptWord(memory, cpu, 0xb64eu);
}

/* $0C: variable = value. */
static void BattleOpSet(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    BattleScriptByte(memory, cpu, 0xb672u);
    BattleScriptValue(memory, cpu, 0xb675u);
    BattleScriptWrite(memory, cpu, 0xb678u);
}

/* $0D/$0E: variable = a + b or a - b. */
static void BattleOpAddSubtract(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    BattleScriptOperands(memory, cpu, handler);
    PushAccumulator8(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    if (handler == BATTLE_OP_ADD) {
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
        handler == BATTLE_OP_ADD ? 0xb694u : 0xb6b3u);
}

/* $16-$18: and, or, xor. */
static void BattleOpBitwise(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    uint16_t value;

    BattleScriptByte(memory, cpu, (uint16_t)(handler + 2u));
    PushAccumulator8(memory, cpu);
    BattleScriptRead(memory, cpu, (uint16_t)(handler + 6u));
    Write16Direct(memory, cpu, 0x54u, cpu->x);
    BattleScriptValue(memory, cpu, (uint16_t)(handler + 11u));
    SetAccumulatorWidth(cpu, 0);
    value = Read16Direct(memory, cpu, 0x54u);
    value = handler == BATTLE_OP_AND ? (uint16_t)(cpu->x & value)
        : handler == BATTLE_OP_OR ? (uint16_t)(cpu->x | value)
        : (uint16_t)(cpu->x ^ value);
    BattleScriptStoreBinary(memory, cpu, value,
        (uint16_t)(handler + 0x17u));
}

/* $1C: variable = leader id $11E8. */
static void BattleOpLeaderId(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, BATTLE_LEADER_ID, 0));
    BattleScriptByte(memory, cpu, 0xb8f0u);
    BattleScriptWrite(memory, cpu, 0xb8f3u);
}

/* $1F: word into $7F:F45C. */
static void BattleOpSetF45C(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    BattleScriptWord(memory, cpu, 0xb921u);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, cpu->x);
    Write16Long(memory, 0x7ff45cu, cpu->accumulator);
}

/* $20: bytes into $7F:F45E/F460. */
static void BattleOpSetF45E(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    BattleScriptByte(memory, cpu, 0xb92eu);
    Write8(memory, 0x7ff45eu, A8(cpu));
    BattleScriptByte(memory, cpu, 0xb935u);
    Write8(memory, 0x7ff460u, A8(cpu));
}

/* $04: jump when $7F:F42E is set. */
static void BattleOpJumpIfF42E(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, BATTLE_CONDITION));
    if (!cpu->zero)
        BattleScriptJump(memory, cpu);
    else
        BattleScriptWord(memory, cpu, 0xb56au);
}

/* $08: signed jump if greater. */
static void BattleOpJumpIfGreater(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    BattleScriptOperands(memory, cpu, handler);
    BattleScriptCompare(memory, cpu);
    if (!cpu->zero && cpu->negative == cpu->overflow)
        BattleScriptJump(memory, cpu);
    else
        BattleScriptWord(memory, cpu, 0xb5e3u);
}

/* $09: signed jump if less. */
static void BattleOpJumpIfLess(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    BattleScriptOperands(memory, cpu, handler);
    BattleScriptCompare(memory, cpu);
    if (cpu->negative != cpu->overflow)
        BattleScriptJump(memory, cpu);
    else
        BattleScriptWord(memory, cpu, 0xb606u);
}

/* $21: movement setup. */
static void BattleOpMoveSetup(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    BattleScriptWord(memory, cpu, 0xb93fu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, cpu->x);
    Write16Long(memory, 0x7ff45cu, cpu->accumulator);
    LoadA16(cpu, 0x0020u);
    Write16Long(memory, 0x7ff45au, cpu->accumulator);
    LoadA16(cpu, 0x0005u);
    Write16Long(memory, BATTLE_ACTION_CODE, cpu->accumulator);
    BattleScriptValue(memory, cpu, 0xb957u);
    LoadA16(cpu, (uint16_t)(0u - cpu->x));
    Write16Long(memory, 0x7ff462u, cpu->accumulator);
    LoadA16(cpu, 0x0020u);
    Write16Long(memory, 0x7ff464u, cpu->accumulator);
    BattleScriptByte(memory, cpu, 0xb96au);
}

/* $23: step setup. */
static void BattleOpStepSetup(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    BattleScriptWord(memory, cpu, 0xb9bau);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, (uint16_t)(0u - cpu->x));
    Write16Long(memory, 0x7ff462u, cpu->accumulator);
    LoadA16(cpu, 0x0020u);
    Write16Long(memory, 0x7ff464u, cpu->accumulator);
    Write16Long(memory, 0x7ff45au, cpu->accumulator);
    LoadA16(cpu, 0x0005u);
}

/* $24: battler record word and byte. */
static void BattleOpRecordWordByte(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
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
}

/* $25: battler record byte, cleared. */
static void BattleOpRecordByteClear(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
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
}

/* $26/$27: battler record byte. */
static void BattleOpRecordByte(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    BattleScriptByte(memory, cpu, (uint16_t)(handler + 2u));
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x00ffu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x859f0fu, cpu->x)));
    And16(cpu, 0x00ffu);
    if (handler == BATTLE_OP_RECORD_BYTE_26)
        LoadA16(cpu, (uint16_t)(cpu->accumulator + 2u));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    BattleScriptByte(memory, cpu,
        handler == BATTLE_OP_RECORD_BYTE_26 ? 0xba3fu : 0xba5cu);
    Write8(memory, LongIndexedAddress(0x7ff44eu, cpu->x), A8(cpu));
}

/* $28: action code 1. */
static void BattleOpAction1(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, 0x0001u);
    Write16Long(memory, BATTLE_ACTION_CODE, cpu->accumulator);
    LoadA16(cpu, 0x0000u);
    Write16Long(memory, 0x7ff456u, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    StoreAImmediate8(memory, cpu, 0xffu, WRAM_PALETTE_FADE);
    StoreZeroAbsolute8(memory, cpu, 0x1269u, 0);
}

/* $29/$2A/$2E/$41: action code 4, 6, 13 or 12. */
static void BattleOpActionFromHandler(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    LoadA8(cpu, handler == BATTLE_OP_ACTION_4 ? 0x04u : handler == BATTLE_OP_ACTION_6
        ? 0x06u : handler == BATTLE_OP_ACTION_13 ? 0x0du : 0x0cu);
    Write8(memory, BATTLE_ACTION_CODE, A8(cpu));
}

/* $2B: action code 3. */
static void BattleOpAction3(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA8(cpu, 0x03u);
    Write8(memory, BATTLE_ACTION_CODE, A8(cpu));
    BattleScriptWord(memory, cpu, 0xba9bu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, cpu->x);
    Write16Long(memory, 0x7ff456u, cpu->accumulator);
}

/* $3E: action code 11. */
static void BattleOpAction11(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA8(cpu, 0x0bu);
    Write8(memory, BATTLE_ACTION_CODE, A8(cpu));
    BattleScriptByte(memory, cpu, 0xbd66u);
    Write8(memory, 0x7ff456u, A8(cpu));
}

/* $2F: count battlers with bit 2 set. */
static void BattleOpPartyFlagCount(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
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
}

/* $30: leader of a side. */
static void BattleOpSideLeader(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
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
}

/* $35: byte into $0A62. */
static void BattleOpSet0A62(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    BattleScriptByte(memory, cpu, 0xbcd1u);
    StoreAAbsolute8(memory, cpu, 0x0a62u, 0);
}

/* $36: byte into $0A62 when $7F:F42E. */
static void BattleOpSet0A62IfF42E(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    BattleScriptByte(memory, cpu, 0xbcdau);
    StoreADirect8(memory, cpu, 0x54u);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, BATTLE_CONDITION));
    if (!cpu->zero) {
        LoadA16(cpu, Read16Direct(memory, cpu, 0x54u));
        Write16Absolute(memory, cpu, 0x0a62u, cpu->accumulator);
    }
}

/* $56: timer $1264. */
static void BattleOpSetTimer(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    BattleScriptValue(memory, cpu, 0xbe24u);
    Write16Absolute(memory, cpu, 0x1264u, cpu->x);
}

/* $50: battler mask $7F:F450. */
static void BattleOpF450Mask(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadAAbsolute8(memory, cpu, 0x0a5du, 0);
    if (!cpu->negative) {
        LoadA8(cpu, (uint8_t)(Read8(memory, 0x7ff450u) | 0xefu));
        LoadA8(cpu, (uint8_t)(A8(cpu) & Read8(memory,
            AbsoluteIndexedAddress(cpu, 0x0a5du, 0))));
        Write8(memory, 0x7ff450u, A8(cpu));
    }
}

/* $0F: multiply. */
static void BattleOpMultiply(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    BattleScriptOperands(memory, cpu, handler);
    Write16Direct(memory, cpu, 0x56u, cpu->x);
    PushAccumulator8(memory, cpu);
    BattleMultiply(memory, cpu, 0xb6c8u);
    LoadA8(cpu, Pull8(memory, cpu));
    LoadX16(cpu, Read16Direct(memory, cpu, 0x63u));
    BattleScriptWrite(memory, cpu, 0xb6ceu);
}

/* $10: signed divide. */
static void BattleOpDivide(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
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
}

/* $11: random value scaled to n. */
static void BattleOpRandomScale(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
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
}

/* $12: 32-bit by 16-bit divide. */
static void BattleOpLongDivide(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
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
}

/* $22: movement setup from battler stats. */
static void BattleOpMoveByStats(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    BattleScriptWord(memory, cpu, 0xb970u);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, cpu->x);
    Write16Long(memory, 0x7ff45cu, cpu->accumulator);
    LoadA16(cpu, 0x01dcu);
    Write16Long(memory, 0x7ff45au, cpu->accumulator);
    LoadA16(cpu, 0x000au);
    Write16Long(memory, BATTLE_ACTION_CODE, cpu->accumulator);
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
}

/* $37: jump backwards. */
static void BattleOpJumpBack(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, 0x0001u);
    Write16Long(memory, BATTLE_ACTION_CODE, cpu->accumulator);
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
}

/* $3C: extended jump. */
static void BattleOpExtendJump(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
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
}

/* $4D: mask of living battlers. */
static void BattleOpLivingMask(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA8(cpu, Read8(memory, 0x7ff450u));
    BattleActiveMask(memory, cpu, 0xbe5cu);
    LoadA8(cpu, (uint8_t)(A8(cpu) & Read8(memory, 0x7ff450u)));
    Write8(memory, 0x7ff450u, A8(cpu));
}

/* $42: call a sub-script from $96:FADD. */
static void BattleOpCall(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    BattleScriptWord(memory, cpu, 0xbd79u);
    Write16Direct(memory, cpu, 0xcau, cpu->x);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, cpu->x);
    AslA16(cpu);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadY16(cpu, Read16Direct(memory, cpu, BATTLE_DP_SCRIPT_POINTER));
    Write16Absolute(memory, cpu, BATTLE_CALLER_POINTER, cpu->y);
    LoadA8(cpu, DirectByte(memory, cpu, BATTLE_DP_SCRIPT_BANK));
    StoreAAbsolute8(memory, cpu, BATTLE_CALLER_POINTER_BANK, 0);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, BATTLE_SCRIPT_BASE, 0));
    Write16Absolute(memory, cpu, BATTLE_CALLER_BASE, cpu->y);
    LoadAAbsolute8(memory, cpu, BATTLE_SCRIPT_BANK, 0);
    StoreAAbsolute8(memory, cpu, BATTLE_CALLER_BANK, 0);
    LoadA8(cpu, 0x96u);
    StoreADirect8(memory, cpu, BATTLE_DP_SCRIPT_BANK);
    StoreAAbsolute8(memory, cpu, BATTLE_SCRIPT_BANK, 0);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory,
        LongIndexedAddress(0x96faddu, cpu->x)));
    cpu->carry = 0;
    Add16Value(cpu, 0xfaddu);
    Write16Absolute(memory, cpu, BATTLE_SCRIPT_BASE, cpu->accumulator);
    Write16Direct(memory, cpu, BATTLE_DP_SCRIPT_POINTER, cpu->accumulator);
}

/* $43: return from a sub-script. */
static void BattleOpReturn(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, BATTLE_CALLER_POINTER, 0));
    Write16Direct(memory, cpu, BATTLE_DP_SCRIPT_POINTER, cpu->y);
    LoadAAbsolute8(memory, cpu, BATTLE_CALLER_POINTER_BANK, 0);
    StoreADirect8(memory, cpu, BATTLE_DP_SCRIPT_BANK);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, BATTLE_CALLER_BASE, 0));
    Write16Absolute(memory, cpu, BATTLE_SCRIPT_BASE, cpu->y);
    LoadAAbsolute8(memory, cpu, BATTLE_CALLER_BANK, 0);
    StoreAAbsolute8(memory, cpu, BATTLE_SCRIPT_BANK, 0);
}

/* $85:B452: battle script VM; other opcodes run on LLE. */
Lufia2ExecutionResult Lufia2BattleScript(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;
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
        LoadA8(cpu, Read8(memory, DirectLongPointer(memory, cpu, BATTLE_DP_SCRIPT_POINTER)));
        SetAccumulatorWidth(cpu, 0);
        AslA16(cpu);
        TransferAToX(cpu);
        BattleScriptAdvance(memory, cpu);
        SetAccumulatorWidth(cpu, 1);
        Push8(memory, cpu, 0x85u);
        PullDataBank(memory, cpu);
        handler = Read16Bank(memory, 0x85u, (uint16_t)(0xb483u + cpu->x));
        switch (opcodes < 4096u ? handler : 0u) {
        case BATTLE_OP_END:
        case BATTLE_OP_END_FF:
            if (handler == BATTLE_OP_END)
                TransferDirectToA(cpu);
            else
                LoadA8(cpu, 0xffu);
            StoreAAbsolute8(memory, cpu, BATTLE_SCRIPT_RESULT, 0); /* B478 */
            SetAccumulatorWidth(cpu, 0);
            SetIndexWidth(cpu, 0);
            cpu->y = PullIndexValue(memory, cpu);
            cpu->x = PullIndexValue(memory, cpu);
            PullAccumulator16(memory, cpu);
            UnpackStatus(cpu, Pull8(memory, cpu));
            PullDataBank(memory, cpu);
            return ExecutionReturned(0x85b482u);
        case BATTLE_OP_JUMP:
            BattleScriptJump(memory, cpu);
            break;
        case BATTLE_OP_RANDOM_JUMP:
            BattleOpRandomJump(memory, cpu);
            break;
        case BATTLE_OP_JUMP_IF_EQUAL:
        case BATTLE_OP_JUMP_IF_NOT_EQUAL:
            BattleOpJumpIfEqual(memory, cpu, handler);
            break;
        case BATTLE_OP_JUMP_IF_GREATER_EQUAL:
            BattleOpJumpIfGreaterEqual(memory, cpu, handler);
            break;
        case BATTLE_OP_JUMP_IF_LESS_EQUAL:
            BattleOpJumpIfLessEqual(memory, cpu, handler);
            break;
        case BATTLE_OP_SET:
            BattleOpSet(memory, cpu);
            break;
        case BATTLE_OP_ADD:
        case BATTLE_OP_SUBTRACT:
            BattleOpAddSubtract(memory, cpu, handler);
            break;
        case BATTLE_OP_AND:
        case BATTLE_OP_OR:
        case BATTLE_OP_XOR:
            BattleOpBitwise(memory, cpu, handler);
            break;
        case BATTLE_OP_ABS:
            BattleScriptUnary(memory, cpu, handler, 0);
            break;
        case BATTLE_OP_NEGATE:
            BattleScriptUnary(memory, cpu, handler, 1);
            break;
        case BATTLE_OP_SIGN:
            BattleScriptUnary(memory, cpu, handler, 2);
            break;
        case BATTLE_OP_LEADER_ID:
            BattleOpLeaderId(memory, cpu);
            break;
        case BATTLE_OP_SET_F45C:
            BattleOpSetF45C(memory, cpu);
            break;
        case BATTLE_OP_SET_F45E:
            BattleOpSetF45E(memory, cpu);
            break;
        case BATTLE_OP_JUMP_IF_F42E:
            BattleOpJumpIfF42E(memory, cpu);
            break;
        case BATTLE_OP_JUMP_IF_GREATER:
            BattleOpJumpIfGreater(memory, cpu, handler);
            break;
        case BATTLE_OP_JUMP_IF_LESS:
            BattleOpJumpIfLess(memory, cpu, handler);
            break;
        case BATTLE_OP_MOVE_SETUP:
            BattleOpMoveSetup(memory, cpu);
            break;
        case BATTLE_OP_STEP_SETUP:
            BattleOpStepSetup(memory, cpu);
            break;
        case BATTLE_OP_RECORD_WORD_BYTE:
            BattleOpRecordWordByte(memory, cpu);
            break;
        case BATTLE_OP_RECORD_BYTE_CLEAR:
            BattleOpRecordByteClear(memory, cpu);
            break;
        case BATTLE_OP_RECORD_BYTE_26:
        case BATTLE_OP_RECORD_BYTE_27:
            BattleOpRecordByte(memory, cpu, handler);
            break;
        case BATTLE_OP_ACTION_1:
            BattleOpAction1(memory, cpu);
            break;
        case BATTLE_OP_ACTION_4:
        case BATTLE_OP_ACTION_6:
        case BATTLE_OP_ACTION_13:
        case BATTLE_OP_ACTION_12:
            BattleOpActionFromHandler(memory, cpu, handler);
            break;
        case BATTLE_OP_ACTION_3:
            BattleOpAction3(memory, cpu);
            break;
        case BATTLE_OP_ACTION_11:
            BattleOpAction11(memory, cpu);
            break;
        case BATTLE_OP_PARTY_FLAG_COUNT:
            BattleOpPartyFlagCount(memory, cpu);
            break;
        case BATTLE_OP_SIDE_LEADER:
            BattleOpSideLeader(memory, cpu);
            break;
        case BATTLE_OP_SET_0A62:
            BattleOpSet0A62(memory, cpu);
            break;
        case BATTLE_OP_SET_0A62_IF_F42E:
            BattleOpSet0A62IfF42E(memory, cpu);
            break;
        case BATTLE_OP_SET_TIMER:
            BattleOpSetTimer(memory, cpu);
            break;
        case BATTLE_OP_F450_MASK:
            BattleOpF450Mask(memory, cpu);
            break;
        case BATTLE_OP_MULTIPLY:
            BattleOpMultiply(memory, cpu, handler);
            break;
        case BATTLE_OP_DIVIDE:
            BattleOpDivide(memory, cpu);
            break;
        case BATTLE_OP_RANDOM_SCALE:
            BattleOpRandomScale(memory, cpu);
            break;
        case BATTLE_OP_LONG_DIVIDE:
            BattleOpLongDivide(memory, cpu);
            break;
        case BATTLE_OP_MOVE_BY_STATS:
            BattleOpMoveByStats(memory, cpu);
            break;
        case BATTLE_OP_JUMP_BACK:
            BattleOpJumpBack(memory, cpu);
            break;
        case BATTLE_OP_EXTEND_JUMP:
            BattleOpExtendJump(memory, cpu);
            break;
        case BATTLE_OP_LIVING_MASK:
            BattleOpLivingMask(memory, cpu);
            break;
        case BATTLE_OP_CALL:
            BattleOpCall(memory, cpu);
            break;
        case BATTLE_OP_RETURN:
            BattleOpReturn(memory, cpu);
            break;
        default:
            result = ExecutionHandoff(cpu, 0x85b470u);
            result.dispatches = opcodes;
            return result;
        }
    }
}
