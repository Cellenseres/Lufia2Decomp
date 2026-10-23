/* Battle script VM ($85:B452). */

#include <stddef.h>

#include "battle/battle_internal.h"
#include "lufia2/system.h"
#include "lufia2/item.h"
#include "core/child_call.h"
#include "system/dp_scratch.h"
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
#define BATTLE_SCRIPT_RESULT_FLAGS (WRAM_BATTLE_ACTION_WORK + 0x82u)
#define BATTLE_ACTION_CODE 0x7ff454u
/* Script battler records: two battlers, then action arguments. */
#define BATTLE_SCRIPT_RECORD 0x7ff44eu
#define BATTLE_BATTLER_A BATTLE_SCRIPT_RECORD
#define BATTLE_BATTLER_B 0x7ff450u
#define BATTLE_ACTION_ARGUMENT 0x7ff456u
#define BATTLE_RECORD_F45A 0x7ff45au
#define BATTLE_RECORD_F45C 0x7ff45cu
#define BATTLE_RECORD_F45E 0x7ff45eu
#define BATTLE_RECORD_F460 0x7ff460u
#define BATTLE_RECORD_F462 0x7ff462u
#define BATTLE_RECORD_F464 0x7ff464u
#define BATTLE_RECORD_WORD_OFFSETS 0x859f04u
#define BATTLE_RECORD_BYTE_OFFSETS 0x859f0fu
#define BATTLE_BATTLER_MASKS 0x0a5du /* per side; negative: none */
#define BATTLE_SCRIPT_UNK_0A62 0x0a62u
#define BATTLE_BASES_CONTEXT_SET 0x0a64u  /* variable bases per battler */
#define BATTLE_BASES_CONTEXT_ZERO 0x0a80u /* when the script context is 0 */
#define BATTLE_SLOT_BITS 0x09fbu          /* scratch of the battler lookup */
#define BATTLE_STAT_TRANSFER_VALUE WRAM_BATTLE_STAT_TRANSFER
#define BATTLE_STAT_TRANSFER_OFFSET (WRAM_BATTLE_STAT_TRANSFER + 2u)
#define BATTLE_STAT_TRANSFER_MASK (WRAM_BATTLE_STAT_TRANSFER + 4u)
#define BATTLE_SLOT_BASE 0x09fau

/* $85:C168: battler byte A to its variable base in X. */
static uint8_t SelectActorRecord(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    StoreAAbsolute8(memory, cpu, BATTLE_SLOT_BITS, 0); /* C168 */
    BitImmediate8(cpu, 0x3fu);
    if (!cpu->zero) {
        And8(cpu, 0x80u);
        if (!cpu->zero)
            LoadA8(cpu, 0x05u);
        StoreAAbsolute8(memory, cpu, BATTLE_SLOT_BASE, 0);
        LoadA8(cpu, 0xffu);
        do {
            const uint32_t bits = AbsoluteIndexedAddress(cpu, BATTLE_SLOT_BITS, 0);
            const uint8_t value = Read8(memory, bits);

            LoadA8(cpu, (uint8_t)(A8(cpu) + 1u)); /* C17A */
            cpu->carry = value & 1u;
            Write8(memory, bits, (uint8_t)(value >> 1));
            SetNz8(cpu, (uint8_t)(value >> 1));
        } while (!cpu->carry);
        cpu->carry = 0;
        Adc8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, BATTLE_SLOT_BASE, 0)));
    }
    SetAccumulatorWidth(cpu, 0); /* C184 */
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_BATTLE_SCRIPT_CONTEXT, 0));
    And16(cpu, 0x00ffu);
    const uint8_t zero_context = cpu->zero;
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu,
                                       zero_context ? BATTLE_BASES_CONTEXT_ZERO
                                                    : BATTLE_BASES_CONTEXT_SET,
                                       cpu->x));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    return zero_context;
}

static void BattleScriptSlot(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                             uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    (void)SelectActorRecord(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $85:C4DE: variable bases $C1 and $BE. */
static void BattleScriptBases(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0xb462u);
    LoadA8(cpu, Read8(memory, BATTLE_BATTLER_B)); /* C4DE */
    BattleScriptSlot(memory, cpu, 0xc4e4u);
    Write16Direct(memory, cpu, BATTLE_DP_LOCALS_F450, cpu->x);
    LoadA8(cpu, Read8(memory, BATTLE_BATTLER_A));
    BattleScriptSlot(memory, cpu, 0xc4edu);
    Write16Direct(memory, cpu, BATTLE_DP_LOCALS_F44E, cpu->x);
    SimulateRtsFrame(memory, cpu);
}

/* INC $BB, 16-bit. */
static void BattleScriptAdvance(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const uint16_t pointer = (uint16_t)(Read16Direct(memory, cpu, BATTLE_DP_SCRIPT_POINTER) + 1u);

    const uint16_t address = (uint16_t)DirectAddress(cpu, BATTLE_DP_SCRIPT_POINTER);
    Write8(memory, (uint16_t)(address + 1u), (uint8_t)(pointer >> 8));
    Write8(memory, address, (uint8_t)pointer);
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
    Write16Direct(memory, cpu, DP_SCRATCH_A, cpu->x);
    BattleScriptValue(memory, cpu, (uint16_t)(opcode + 10u));
}

/* Signed $54 - X, 16-bit, overflow kept. */
static void BattleScriptCompare(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Direct(memory, cpu, DP_SCRATCH_A));
    Write16Direct(memory, cpu, DP_SCRATCH_A, cpu->x);
    cpu->carry = 1;
    Add16Value(cpu, (uint16_t)~Read16Direct(memory, cpu, DP_SCRATCH_A));
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

/* $85:DCA3: $63-$66 = $54 * $56, 16x16 via $4202. */
static void BattleMultiplyBody(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));                       /* DCA3 */
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    LoadA8(cpu, DirectByte(memory, cpu, DP_SCRATCH_C));
    StoreAAbsolute8(memory, cpu, SNES_WRMPYA, 0);
    LoadA8(cpu, DirectByte(memory, cpu, DP_SCRATCH_A));
    StoreAAbsolute8(memory, cpu, SNES_WRMPYB, 0);
    LoadA8(cpu, DirectByte(memory, cpu, DP_SCRATCH_D));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, DP_SCRATCH_B));
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, SNES_RDMPYL, 0));
    SetAccumulatorWidth(cpu, 0);
    StoreWordAbsolute(memory, cpu, SNES_WRMPYA, cpu->accumulator);
    Write16Direct(memory, cpu, 0x63u, cpu->x);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, DirectByte(memory, cpu, DP_SCRATCH_A));
    SetAccumulatorWidth(cpu, 0);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, SNES_RDMPYL, 0));
    StoreWordAbsolute(memory, cpu, SNES_WRMPYA, cpu->accumulator);
    Write16Direct(memory, cpu, 0x65u, cpu->x);
    LoadX16(cpu, Read16Direct(memory, cpu, DP_SCRATCH_B));
    cpu->carry = 0;
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, SNES_RDMPYL, 0));
    StoreWordAbsolute(memory, cpu, SNES_WRMPYA, cpu->x);
    Add16Value(cpu, Read16Direct(memory, cpu, 0x64u));
    if (cpu->carry) {
        OpStepMem(memory, cpu, OpDp(cpu, 0x66u), 1);
        cpu->carry = 0;
    }
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, SNES_RDMPYL, 0));
    if (cpu->carry)
        OpStepMem(memory, cpu, OpDp(cpu, 0x66u), 1);
    Write16Direct(memory, cpu, 0x64u, cpu->accumulator);       /* DCE6 */
    UnpackStatus(cpu, Pull8(memory, cpu));
}

/* $85:DCA3 multiply as a JSL to return_address. */
static void BattleMultiply(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x85u, return_address);
    BattleMultiplyBody(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $85:DCA3: native entry for the multiply, returns to $85:DCE9. */
Lufia2ExecutionResult Lufia2BattleMultiply(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    BattleMultiplyBody(memory, cpu);
    return ExecutionReturned(0x85dce9u);
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
    LoadA8(cpu, DirectByte(memory, cpu, DP_SCRATCH_A));
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
    LoadA8(cpu, DirectByte(memory, cpu, DP_SCRATCH_A));
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
static void BattleRandomFractionBody(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const struct {
        uint8_t limit;
        uint16_t call;
    } rolls[3] = {{0x80u, 0xdcf7u}, {0x80u, 0xdcffu}, {0x04u, 0xdd07u}};
    unsigned i;

    Push8(memory, cpu, PackStatus(cpu));                       /* DCEA */
    PushDataBank(memory, cpu);
    Push8(memory, cpu, 0x85u);
    PullDataBank(memory, cpu);
    if (cpu->accumulator_is_8_bit)
        StoreADirect8(memory, cpu, DP_SCRATCH_A);
    else
        Write16Direct(memory, cpu, DP_SCRATCH_A, cpu->accumulator);
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
}

/* $85:DCEA random fraction as a JSL to return_address. */
void BattleCallRandomFraction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x85u, return_address);
    BattleRandomFractionBody(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $85:DCEA: native entry for the random fraction, returns to $85:DD18. */
Lufia2ExecutionResult Lufia2BattleRandomFraction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    BattleRandomFractionBody(memory, cpu);
    return ExecutionReturned(0x85dd18u);
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
        const uint32_t low_address = DirectAddress(cpu, 0x63u);
        const uint16_t low = Read16Direct(memory, cpu, 0x63u);
        const uint16_t low_shifted = (uint16_t)(low << 1);
        Write8(memory, (uint16_t)(low_address + 1u), (uint8_t)(low_shifted >> 8));
        Write8(memory, low_address, (uint8_t)low_shifted);
        const uint32_t high_address = DirectAddress(cpu, 0x65u);
        const uint16_t high = Read16Direct(memory, cpu, 0x65u);
        const uint16_t high_shifted = (uint16_t)((high << 1) | (low >> 15));
        Write8(memory, (uint16_t)(high_address + 1u), (uint8_t)(high_shifted >> 8));
        Write8(memory, high_address, (uint8_t)high_shifted);
        const uint16_t a = cpu->accumulator;
        LoadA16(cpu, (uint16_t)((a << 1) | (high >> 15)));
        cpu->carry = a >> 15;
        if (!cpu->carry) {
            Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, DP_SCRATCH_E));
            if (!cpu->carry)
                continue;
        }
        Add16Value(cpu, (uint16_t)~Read16Direct(memory, cpu, DP_SCRATCH_E));
        const uint16_t incremented = (uint16_t)(Read16Direct(memory, cpu, 0x63u) + 1u);
        Write8(memory, (uint16_t)(low_address + 1u), (uint8_t)(incremented >> 8));
        Write8(memory, low_address, (uint8_t)incremented);
        SetNz16(cpu, incremented);
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
static void BattleActiveMaskBody(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const uint8_t enemies = (uint8_t)(A8(cpu) & 0x80u);

    StoreZeroAbsolute8(memory, cpu, BATTLE_SLOT_BASE, 0); /* C117 */
    cpu->zero = enemies == 0;
    SetAccumulatorWidth(cpu, 0);
    LoadX16(cpu, enemies ? 0x000au : 0x0008u);
    do {
        const uint32_t mask = AbsoluteIndexedAddress(cpu, BATTLE_SLOT_BASE, 0);
        uint16_t bits;

        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu,
                                           enemies ? 0x0a6eu : BATTLE_BASES_CONTEXT_SET,
                                           cpu->x));
        cpu->carry = 0;
        if (!cpu->zero) {
            LoadY16(cpu, cpu->accumulator);
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x000fu, cpu->y));
            cpu->zero = (cpu->accumulator & 0x0004u) == 0;
            cpu->carry = cpu->zero;
        }
        bits =
            (uint16_t)(Read8(memory, mask) |
                       (Read8(memory, AbsoluteIndexedAddress(cpu, BATTLE_SLOT_BITS, 0))
                        << 8));
        {
            const uint8_t out = (uint8_t)(bits >> 15);

            bits = (uint16_t)((bits << 1) | cpu->carry);
            cpu->carry = out;
        }
        Write8(memory, AbsoluteIndexedAddress(cpu, BATTLE_SLOT_BITS, 0),
               (uint8_t)(bits >> 8));
        Write8(memory, mask, (uint8_t)bits);
        SetNz16(cpu, bits);
        cpu->x = (uint16_t)(cpu->x - 2u);
        SetNz16(cpu, cpu->x);
    } while (!cpu->negative);
    SetAccumulatorWidth(cpu, 1);
    LoadAAbsolute8(memory, cpu, BATTLE_SLOT_BASE, 0);
    if (enemies)
        Or8(cpu, 0x80u);
}

static void BattleActiveMask(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    BattleActiveMaskBody(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* Handlers in the $85:B483 opcode table. */
enum BattleOpcodeHandler {
    BATTLE_OP_STATUS_MESSAGE = 0xb90b,
    BATTLE_OP_DISPLAY_MESSAGE = 0xbe06,
    BATTLE_OP_PLAY_EFFECT = 0xbd54,
    BATTLE_OP_REPEAT_FRAMES = 0xbe2b,                         /* $4A */
    BATTLE_OP_SHOW_POPUPS = 0xbe43,                           /* $4B */
    BATTLE_OP_SHOW_POPUPS_MODE = 0xbe4c,                      /* $4C */
    BATTLE_OP_WAIT_COUNTER = 0xbe1a,                          /* $49 */

    BATTLE_OP_DISPATCH_ACTION = 0xb53d,                         /* $01 */
    BATTLE_OP_SPELL_ACTION = 0xbaa6,
    BATTLE_OP_SUMMON_ENEMY = 0xbb15,

    BATTLE_OP_SELECT_BATTLERS = 0xbba2,                        /* $32 */
    BATTLE_OP_CLEAR_ACTION_WORK = 0xb904,                      /* $1D */
    BATTLE_OP_CLEAR_MESSAGE = 0xbe13,                          /* $48 */
    BATTLE_OP_CLEAR_PARTY_MODIFIERS = 0xbf97,                   /* $58 */
    BATTLE_OP_CLEAR_ENEMY_MODIFIERS = 0xbf9e,                   /* $59 */
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
    BATTLE_OP_UNSIGNED_DIVIDE = 0xb71d,                        /* $55 */
    BATTLE_OP_SCALE_ACTION = 0xbef5,                           /* $57 */
    BATTLE_OP_DIVIDE = 0xb6d2,                                 /* $10 */
    BATTLE_OP_RANDOM_SCALE = 0xb73b,                           /* $11 */
    BATTLE_OP_LONG_DIVIDE = 0xb753,                            /* $12 */
    BATTLE_OP_AVERAGE_TARGET_STAT = 0xb77b,                    /* $13 */
    BATTLE_OP_WRITE_TARGET_STAT = 0xb7a8,                      /* $14 */
    BATTLE_OP_AVERAGE_LOCAL_STAT = 0xb7cf,                     /* $15 */
    BATTLE_OP_AVERAGE_ACTOR_STAT = 0xb81c,                     /* $4E */
    BATTLE_OP_AND = 0xb849,                                    /* $16 */
    BATTLE_OP_OR = 0xb864,                                     /* $17 */
    BATTLE_OP_XOR = 0xb87f,                                    /* $18 */
    BATTLE_OP_ABS = 0xb89a,                                    /* $19 */
    BATTLE_OP_NEGATE = 0xb8b4,                                 /* $1A */
    BATTLE_OP_SIGN = 0xb8cc,                                   /* $1B */
    BATTLE_OP_JUMP_IF_CAPSULE_ABILITY = 0xb652,                 /* $3F */
    BATTLE_OP_CAPSULE_ABILITIES = 0xb8f7,                      /* $40 */
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
    BATTLE_OP_WRITE_PARTY_BYTE = 0xbdcb,                        /* $44 */
    BATTLE_OP_READ_PARTY_BYTE = 0xbdda,                         /* $45 */
    BATTLE_OP_JUMP_IF_PARTY_BYTE_ZERO = 0xbdf3,                  /* $46 */
    BATTLE_OP_SET_TIMER = 0xbe22,                              /* $56 */
    BATTLE_OP_LIVING_MASK = 0xbe56,                            /* $4D */
    BATTLE_OP_F450_MASK = 0xbe68,                              /* $50 */
    BATTLE_OP_NORMAL_RESULT_FLAG = 0xbe7d,                     /* $51 */
    BATTLE_OP_RESULT_FLAG = 0xbe96,                            /* $52 */
    BATTLE_OP_ACTION_9 = 0xbebf,                               /* $53 */
    BATTLE_OP_ACTION_25 = 0xbeda,                              /* $54 */
    BATTLE_OP_FADE_VALUE = 0xbfa5,                             /* $5A */
    BATTLE_OP_CLEAR_F8A5 = 0xbfae,                             /* $5B */
    BATTLE_OP_ALTERNATE_ACTION = 0xbfb6,                       /* $5C */
};

enum BattleGroupScratch {
    BATTLE_GROUP_SUM = 0,
    BATTLE_GROUP_SUM_HIGH = 2,
    BATTLE_GROUP_MASK = 4,
    BATTLE_GROUP_SIDE = 5,
    BATTLE_GROUP_BIT = 6,
    BATTLE_GROUP_COUNT = 7,
    BATTLE_GROUP_DIVISOR = 0x54,
    BATTLE_GROUP_DIVIDEND = 0x5d,
    BATTLE_GROUP_DIVIDEND_HIGH = 0x5f
};

static bool BattleScriptGroupStat(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t return_address, bool write) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadA8(cpu, Read8(memory, BATTLE_STAT_TRANSFER_OFFSET));
    BattleStatOffset(memory, cpu, write ? 0xc245u : 0xc1cdu);
    SetAccumulatorWidth(cpu, 0);
    TransferXToA(cpu);
    Write16Long(memory, BATTLE_STAT_TRANSFER_OFFSET, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, BATTLE_STAT_TRANSFER_MASK));
    StoreADirect8(memory, cpu, BATTLE_GROUP_MASK);
    And8(cpu, 0x80u);
    StoreADirect8(memory, cpu, BATTLE_GROUP_SIDE);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, 0x20u);
    Write16Direct(memory, cpu, BATTLE_GROUP_BIT, cpu->accumulator);
    Write16Direct(memory, cpu, BATTLE_GROUP_SUM, 0u);
    Write16Direct(memory, cpu, BATTLE_GROUP_SUM_HIGH, 0u);
    SetAccumulatorWidth(cpu, 1);
    do {
        LoadA8(cpu, DirectByte(memory, cpu, BATTLE_GROUP_BIT));
        And8(cpu, DirectByte(memory, cpu, BATTLE_GROUP_MASK));
        if (!cpu->zero) {
            Or8(cpu, DirectByte(memory, cpu, BATTLE_GROUP_SIDE));
            BattleScriptSlot(memory, cpu, write ? 0xc270u : 0xc1f8u);
            SetAccumulatorWidth(cpu, 0);
            TransferXToA(cpu);
            cpu->carry = 0u;
            Add16Value(cpu, Read16Long(memory, BATTLE_STAT_TRANSFER_OFFSET));
            TransferAToX(cpu);
            if (write) {
                LoadA16(cpu, Read16Long(memory, BATTLE_STAT_TRANSFER_VALUE));
                Write16Long(memory, AbsoluteIndexedAddress(cpu, 0u, cpu->x), cpu->accumulator);
            } else {
                LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0u, cpu->x));
                cpu->carry = 0u;
                Add16Value(cpu, Read16Direct(memory, cpu, BATTLE_GROUP_SUM));
                Write16Direct(memory, cpu, BATTLE_GROUP_SUM, cpu->accumulator);
                TransferDirectToA(cpu);
                Add16Value(cpu, Read16Direct(memory, cpu, BATTLE_GROUP_SUM_HIGH));
                Write16Direct(memory, cpu, BATTLE_GROUP_SUM_HIGH, cpu->accumulator);
            }
            SetAccumulatorWidth(cpu, 1);
            IncrementDirect8(memory, cpu, BATTLE_GROUP_COUNT);
        }
        const uint8_t bit = DirectByte(memory, cpu, BATTLE_GROUP_BIT);
        cpu->carry = bit & 1u;
        Write8(memory, DirectAddress(cpu, BATTLE_GROUP_BIT), (uint8_t)(bit >> 1));
        SetNz8(cpu, (uint8_t)(bit >> 1));
    } while (!cpu->carry);
    LoadA8(cpu, DirectByte(memory, cpu, BATTLE_GROUP_COUNT));
    if (cpu->zero) {
        cpu->resume_pc = write ? 0x85c28eu : 0x85c23eu;
        return false;
    }
    if (!write) {
        Compare8(cpu, A8(cpu), 1u);
        if (!cpu->zero) {
            StoreADirect8(memory, cpu, BATTLE_GROUP_DIVISOR);
            LoadA8(cpu, DirectByte(memory, cpu, BATTLE_GROUP_SUM_HIGH));
            StoreADirect8(memory, cpu, BATTLE_GROUP_DIVIDEND_HIGH);
            LoadX16(cpu, Read16Direct(memory, cpu, BATTLE_GROUP_SUM));
            Write16Direct(memory, cpu, BATTLE_GROUP_DIVIDEND, cpu->x);
            BattleDivide(memory, cpu, 0xc22cu);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16Direct(memory, cpu, BATTLE_GROUP_DIVIDEND));
        } else {
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16Direct(memory, cpu, BATTLE_GROUP_SUM));
        }
        Write16Long(memory, BATTLE_STAT_TRANSFER_VALUE, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
    }
    SimulateRtsFrame(memory, cpu);
    return true;
}

static bool BattleOpGroupStat(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t handler) {
    BattleScriptByte(memory, cpu, (uint16_t)(handler + 2u));
    if (handler == BATTLE_OP_WRITE_TARGET_STAT) {
        BattleScriptRead(memory, cpu, 0xb7adu);
        SetAccumulatorWidth(cpu, 0);
        TransferXToA(cpu);
        Write16Long(memory, BATTLE_STAT_TRANSFER_VALUE, cpu->accumulator);
        LoadA16(cpu, Read16Long(memory, BATTLE_BATTLER_B));
        Write16Long(memory, BATTLE_STAT_TRANSFER_MASK, cpu->accumulator);
        BattleScriptByte(memory, cpu, 0xb7bfu);
        And16(cpu, 0xffu);
        Write16Long(memory, BATTLE_STAT_TRANSFER_OFFSET, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        return BattleScriptGroupStat(memory, cpu, 0xb7cbu, true);
    }
    PushAccumulator8(memory, cpu);
    uint16_t operand_return, group_return, result_return;
    uint32_t battler;
    if (handler == BATTLE_OP_AVERAGE_LOCAL_STAT) {
        LoadAAbsolute8(memory, cpu, BATTLE_LOCAL_SELECT, 0u);
        const bool local_target = !cpu->zero;
        operand_return = local_target ? 0xb7f5u : 0xb7dau;
        group_return = local_target ? 0xb80bu : 0xb7f0u;
        result_return = 0xb818u;
        battler = local_target ? BATTLE_BATTLER_B : BATTLE_BATTLER_A;
    } else {
        const bool target = handler == BATTLE_OP_AVERAGE_TARGET_STAT;
        if (target)
            SetAccumulatorWidth(cpu, 0);
        operand_return = target ? 0xb783u : 0xb822u;
        group_return = target ? 0xb797u : 0xb838u;
        result_return = target ? 0xb7a4u : 0xb845u;
        battler = target ? BATTLE_BATTLER_B : BATTLE_BATTLER_A;
    }
    BattleScriptByte(memory, cpu, operand_return);
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0xffu);
    Write16Long(memory, BATTLE_STAT_TRANSFER_OFFSET, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, battler));
    Write16Long(memory, BATTLE_STAT_TRANSFER_MASK, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    if (!BattleScriptGroupStat(memory, cpu, group_return, false))
        return false;
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, BATTLE_STAT_TRANSFER_VALUE));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Pull8(memory, cpu));
    BattleScriptWrite(memory, cpu, result_return);
    return true;
}

enum BattleSelectionRow {
    BATTLE_SELECTION_ROW_BYTES = 3,
    BATTLE_SELECTION_ROW_SCORE = 1,
    BATTLE_SELECTION_ROW_COUNT = 12
};

enum BattleSelectionScratch {
    BATTLE_SELECTION_PASSES = 0,
    BATTLE_SELECTION_START = 2,
    BATTLE_SELECTION_LENGTH = 3,
    BATTLE_SELECTION_STAT = 0x22,
    BATTLE_SELECTION_COUNT = 0x23,
    BATTLE_SELECTION_SIDE = 0x24,
    BATTLE_SELECTION_BIT = 0x25,
    BATTLE_SELECTION_OUTPUT = 0x26,
    BATTLE_SELECTION_OFFSET = 0x2a,
    BATTLE_SELECTION_MASK = 0x54
};
#define BATTLE_SELECTION_ROWS 0x11ebu

static bool BattleSelectionStat(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    TransferDirectToA(cpu);
    LoadAAbsolute8(memory, cpu, 0x09f2u, 0u);
    Compare8(cpu, A8(cpu), 0x80u);
    if (!cpu->carry) {
        Write8(memory, BATTLE_STAT_TRANSFER_OFFSET, A8(cpu));
        if (!BattleScriptGroupStat(memory, cpu, 0xc3c0u, false))
            return false;
    } else {
        cpu->carry = 1u;
        Sbc8(cpu, 0x80u);
        AslA8(cpu);
        TransferAToX(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x859e71u, cpu->x)));
        Write8(memory, BATTLE_STAT_TRANSFER_OFFSET, A8(cpu));
        PushIndex(memory, cpu);
        if (!BattleScriptGroupStat(memory, cpu, 0xc3d2u, false))
            return false;
        cpu->x = PullIndexValue(memory, cpu);
        SetAccumulatorWidth(cpu, 0u);
        LoadA16(cpu, Read16Long(memory, BATTLE_STAT_TRANSFER_VALUE));
        Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x09f4u, 0u), cpu->accumulator);
        SetAccumulatorWidth(cpu, 1u);
        cpu->x = (uint16_t)(cpu->x + 1u);
        SetNz16(cpu, cpu->x);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x859e71u, cpu->x)));
        StoreAAbsolute8(memory, cpu, 0x09f3u, 0u);
        And8(cpu, 0x3fu);
        Write8(memory, BATTLE_STAT_TRANSFER_OFFSET, A8(cpu));
        if (!BattleScriptGroupStat(memory, cpu, 0xc3efu, false))
            return false;
        LoadAAbsolute8(memory, cpu, 0x09f3u, 0u);
        if (cpu->negative) {
            SetAccumulatorWidth(cpu, 0u);
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x09f4u, 0u));
            cpu->carry = 1u;
            Add16Value(cpu, (uint16_t)~Read16Long(memory, BATTLE_STAT_TRANSFER_VALUE));
            Write16Long(memory, BATTLE_STAT_TRANSFER_VALUE, cpu->accumulator);
            SetAccumulatorWidth(cpu, 1u);
            LoadAAbsolute8(memory, cpu, 0x09f3u, 0u);
        }
        BitImmediate8(cpu, 0x40u);
        if (!cpu->zero) {
            SetAccumulatorWidth(cpu, 0u);
            LoadA16(cpu, Read16Long(memory, BATTLE_STAT_TRANSFER_VALUE));
            Write16Direct(memory, cpu, 0x65u, cpu->accumulator);
            Write16Direct(memory, cpu, 0x63u, 0u);
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x09f4u, 0u));
            Write16Direct(memory, cpu, 0x58u, cpu->accumulator);
            BattleLongDivide(memory, cpu, 0xc41eu);
            LoadA16(cpu, Read16Direct(memory, cpu, 0x63u));
            Write16Long(memory, BATTLE_STAT_TRANSFER_VALUE, cpu->accumulator);
            SetAccumulatorWidth(cpu, 1u);
        }
    }
    SimulateRtsFrame(memory, cpu);
    return true;
}

static bool BattleBuildSelectionRows(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    TransferDirectToA(cpu);
    BattleScriptByte(memory, cpu, 0xc309u);
    StoreADirect8(memory, cpu, BATTLE_SELECTION_STAT);
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x859e47u, cpu->x)));
    TransferAToX(cpu);
    Write16Direct(memory, cpu, BATTLE_SELECTION_OFFSET, cpu->x);
    LoadX16(cpu, BATTLE_SELECTION_ROW_BYTES * BATTLE_SELECTION_ROW_COUNT - 1u);
    do {
        StoreZeroAbsolute8(memory, cpu, BATTLE_SELECTION_ROWS, cpu->x);
        cpu->x = (uint16_t)(cpu->x - 1u);
        SetNz16(cpu, cpu->x);
    } while (!cpu->negative);
    Write8(memory, DirectAddress(cpu, BATTLE_SELECTION_COUNT), 0u);
    LoadA8(cpu, 1u);
    StoreADirect8(memory, cpu, BATTLE_SELECTION_BIT);
    LoadA8(cpu, Read8(memory, BATTLE_BATTLER_B));
    And8(cpu, 0x80u);
    StoreADirect8(memory, cpu, BATTLE_SELECTION_SIDE);
    bool enemy_first = !cpu->zero;
    LoadX16(cpu, 0u);
    Write16Direct(memory, cpu, BATTLE_SELECTION_OUTPUT, cpu->x);
    for (;;) {
        LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu,
            enemy_first ? 0x0a6eu : BATTLE_BASES_CONTEXT_SET, cpu->x));
        bool include = !cpu->zero;
        if (enemy_first && include) {
            LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x0fu, cpu->y)));
            BitImmediate8(cpu, 4u);
            include = cpu->zero;
        }
        if (include) {
            PushIndex(memory, cpu);
            PushY(memory, cpu);
            LoadX16(cpu, Read16Direct(memory, cpu, BATTLE_SELECTION_OUTPUT));
            LoadA8(cpu, DirectByte(memory, cpu, BATTLE_SELECTION_STAT));
            StoreAAbsolute8(memory, cpu, 0x09f2u, 0u);
            LoadA8(cpu, DirectByte(memory, cpu, BATTLE_SELECTION_BIT));
            Write8(memory, BATTLE_STAT_TRANSFER_MASK, A8(cpu));
            StoreAAbsolute8(memory, cpu, BATTLE_SELECTION_ROWS, cpu->x);
            cpu->x = (uint16_t)(cpu->x + 1u);
            SetNz16(cpu, cpu->x);
            PushIndex(memory, cpu);
            if (!BattleSelectionStat(memory, cpu, enemy_first ? 0xc393u : 0xc34du))
                return false;
            cpu->x = PullIndexValue(memory, cpu);
            SetAccumulatorWidth(cpu, 0u);
            LoadA16(cpu, Read16Long(memory, BATTLE_STAT_TRANSFER_VALUE));
            Write16Long(memory, AbsoluteIndexedAddress(cpu, BATTLE_SELECTION_ROWS, cpu->x), cpu->accumulator);
            cpu->x = (uint16_t)(cpu->x + 2u);
            SetNz16(cpu, cpu->x);
            SetAccumulatorWidth(cpu, 1u);
            Write16Direct(memory, cpu, BATTLE_SELECTION_OUTPUT, cpu->x);
            IncrementDirect8(memory, cpu, BATTLE_SELECTION_COUNT);
            cpu->y = PullIndexValue(memory, cpu);
            cpu->x = PullIndexValue(memory, cpu);
        }
        const uint8_t bit = DirectByte(memory, cpu, BATTLE_SELECTION_BIT);
        cpu->carry = bit >> 7;
        Write8(memory, DirectAddress(cpu, BATTLE_SELECTION_BIT), (uint8_t)(bit << 1));
        SetNz8(cpu, (uint8_t)(bit << 1));
        cpu->x = (uint16_t)(cpu->x + 2u);
        SetNz16(cpu, cpu->x);
        Compare16(cpu, cpu->x, enemy_first ? 0x0cu : 0x0au);
        if (cpu->zero)
            break;
        enemy_first = false;
    }
    SimulateRtsFrame(memory, cpu);
    return true;
}

static void BattleSwapSelectionRows(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, BATTLE_SELECTION_ROWS + BATTLE_SELECTION_ROW_BYTES + BATTLE_SELECTION_ROW_SCORE, cpu->x));
    Write16Long(memory, AbsoluteIndexedAddress(cpu, BATTLE_SELECTION_ROWS + BATTLE_SELECTION_ROW_BYTES + BATTLE_SELECTION_ROW_SCORE, cpu->x), cpu->accumulator);
    LoadA16(cpu, cpu->y);
    Write16Long(memory, AbsoluteIndexedAddress(cpu, BATTLE_SELECTION_ROWS + BATTLE_SELECTION_ROW_SCORE, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1u);
    LoadAAbsolute8(memory, cpu, BATTLE_SELECTION_ROWS, cpu->x);
    ExchangeAccumulatorBytes(cpu);
    LoadAAbsolute8(memory, cpu, BATTLE_SELECTION_ROWS + BATTLE_SELECTION_ROW_BYTES, cpu->x);
    StoreAAbsolute8(memory, cpu, BATTLE_SELECTION_ROWS, cpu->x);
    ExchangeAccumulatorBytes(cpu);
    StoreAAbsolute8(memory, cpu, BATTLE_SELECTION_ROWS + BATTLE_SELECTION_ROW_BYTES, cpu->x);
}

static void BattleSortSelectionRows(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t return_address, bool shuffle) {
    SimulateJsrFrame(memory, cpu, return_address);
    if (shuffle)
        LoadA8(cpu, 24u);
    else
        LoadA8(cpu, DirectByte(memory, cpu, BATTLE_SELECTION_STAT));
    StoreADirect8(memory, cpu, BATTLE_SELECTION_PASSES);
    do {
        LoadX16(cpu, 0u);
        for (;;) {
            LoadAAbsolute8(memory, cpu, BATTLE_SELECTION_ROWS, cpu->x);
            if (cpu->zero)
                break;
            if (shuffle) {
                LoadAAbsolute8(memory, cpu, BATTLE_SELECTION_ROWS + BATTLE_SELECTION_ROW_BYTES, cpu->x);
                if (cpu->zero)
                    break;
                LoadA8(cpu, 2u);
                SimulateJslFrame(memory, cpu, 0x85u, 0xc4b0u);
                Lufia2RandomScale(memory, cpu);
                SimulateRtlFrame(memory, cpu);
                Compare8(cpu, A8(cpu), 0u);
                if (!cpu->zero) {
                    SetAccumulatorWidth(cpu, 0u);
                    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, BATTLE_SELECTION_ROWS + BATTLE_SELECTION_ROW_SCORE, cpu->x));
                    BattleSwapSelectionRows(memory, cpu);
                }
            } else {
                SetAccumulatorWidth(cpu, 0u);
                LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, BATTLE_SELECTION_ROWS + BATTLE_SELECTION_ROW_SCORE, cpu->x));
                Compare16(cpu, cpu->accumulator,
                    Read16AbsoluteIndexed(memory, cpu, BATTLE_SELECTION_ROWS + BATTLE_SELECTION_ROW_BYTES + BATTLE_SELECTION_ROW_SCORE, cpu->x));
                if (!cpu->carry)
                    BattleSwapSelectionRows(memory, cpu);
                SetAccumulatorWidth(cpu, 1u);
            }
            cpu->x = (uint16_t)(cpu->x + BATTLE_SELECTION_ROW_BYTES);
            SetNz16(cpu, cpu->x);
        }
        DecrementDirect8(memory, cpu, BATTLE_SELECTION_PASSES);
    } while (!cpu->zero);
    SimulateRtsFrame(memory, cpu);
}

static void BattleSelectionRange(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    BattleScriptByte(memory, cpu, 0xc42au);
    StoreADirect8(memory, cpu, BATTLE_SELECTION_START);
    BattleScriptByte(memory, cpu, 0xc42fu);
    StoreADirect8(memory, cpu, BATTLE_SELECTION_LENGTH);
    SetIndexWidth(cpu, 1u);
    LoadA8(cpu, DirectByte(memory, cpu, BATTLE_SELECTION_PASSES));
    Compare8(cpu, A8(cpu), DirectByte(memory, cpu, BATTLE_SELECTION_START));
    if (cpu->carry)
        StoreADirect8(memory, cpu, BATTLE_SELECTION_START);
    LoadA8(cpu, BATTLE_SELECTION_ROW_BYTES);
    StoreAAbsolute8(memory, cpu, SNES_WRMPYA, 0u);
    LoadA8(cpu, DirectByte(memory, cpu, BATTLE_SELECTION_START));
    StoreAAbsolute8(memory, cpu, SNES_WRMPYB, 0u);
    TransferDirectToA(cpu);
    LoadY8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, SNES_RDMPYL, 0u)));
    do {
        LoadX8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, BATTLE_SELECTION_ROWS, cpu->y)));
        if (cpu->zero)
            break;
        Or8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, BATTLE_SELECTION_ROWS, cpu->y)));
        cpu->y = (uint8_t)(cpu->y + BATTLE_SELECTION_ROW_BYTES);
        SetNz8(cpu, (uint8_t)cpu->y);
        DecrementDirect8(memory, cpu, BATTLE_SELECTION_LENGTH);
    } while (!cpu->zero);
    SetIndexWidth(cpu, 0u);
    SimulateRtsFrame(memory, cpu);
}

static void BattlePresentMask(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    BitImmediate8(cpu, 0x80u);
    if (!cpu->zero) {
        BattleActiveMaskBody(memory, cpu);
        SimulateRtsFrame(memory, cpu);
        return;
    }
    SetAccumulatorWidth(cpu, 0u);
    LoadX16(cpu, 8u);
    do {
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, BATTLE_BASES_CONTEXT_SET, cpu->x));
        cpu->carry = !cpu->zero;
        const uint32_t low = AbsoluteIndexedAddress(cpu, BATTLE_SLOT_BASE, 0u);
        const uint32_t high = AbsoluteIndexedAddress(cpu, BATTLE_SLOT_BITS, 0u);
        const uint16_t value = (uint16_t)(Read8(memory, low) | (Read8(memory, high) << 8));
        const uint16_t shifted = (uint16_t)((value << 1) | cpu->carry);
        cpu->carry = value >> 15;
        Write8(memory, high, (uint8_t)(shifted >> 8));
        Write8(memory, low, (uint8_t)shifted);
        SetNz16(cpu, shifted);
        cpu->x = (uint16_t)(cpu->x - 2u);
        SetNz16(cpu, cpu->x);
    } while (!cpu->negative);
    SetAccumulatorWidth(cpu, 1u);
    LoadAAbsolute8(memory, cpu, BATTLE_SLOT_BASE, 0u);
    SimulateRtsFrame(memory, cpu);
}

static bool BattleOpSelectBattlers(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    BattleScriptByte(memory, cpu, 0xbba4u);
    SetAccumulatorWidth(cpu, 0u);
    And16(cpu, 0xffu);
    AslA16(cpu);
    TransferAToX(cpu);
    TransferDirectToA(cpu);
    Write16Long(memory, BATTLE_CONDITION, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1u);
    const uint16_t choice = Read16Bank(memory, 0x85u, (uint16_t)(0xbbb6u + cpu->x));
    switch (choice) {
    case 0xbbcau:
        return true;
    case 0xbbf8u:
    case 0xbc26u:
        if (!BattleBuildSelectionRows(memory, cpu, choice == 0xbbf8u ? 0xbbfau : 0xbc28u))
            return false;
        if (choice == 0xbbf8u) {
            LoadX16(cpu, 0u);
            for (;;) {
                LoadAAbsolute8(memory, cpu, BATTLE_SELECTION_ROWS, cpu->x);
                if (cpu->zero)
                    break;
                SetAccumulatorWidth(cpu, 0u);
                LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, BATTLE_SELECTION_ROWS + BATTLE_SELECTION_ROW_SCORE, cpu->x));
                LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffffu));
                LoadA16(cpu, (uint16_t)(cpu->accumulator + 1u));
                Write16Long(memory, AbsoluteIndexedAddress(cpu, BATTLE_SELECTION_ROWS + BATTLE_SELECTION_ROW_SCORE, cpu->x), cpu->accumulator);
                SetAccumulatorWidth(cpu, 1u);
                cpu->x = (uint16_t)(cpu->x + BATTLE_SELECTION_ROW_BYTES);
                SetNz16(cpu, cpu->x);
            }
        }
        BattleSortSelectionRows(memory, cpu, choice == 0xbbf8u ? 0xbc18u : 0xbc2bu, true);
        BattleSortSelectionRows(memory, cpu, choice == 0xbbf8u ? 0xbc1bu : 0xbc2eu, false);
        BattleSelectionRange(memory, cpu, choice == 0xbbf8u ? 0xbc1eu : 0xbc31u);
        Write8(memory, BATTLE_BATTLER_B, A8(cpu));
        return true;
    case 0xbc39u:
        LoadA8(cpu, Read8(memory, BATTLE_BATTLER_B));
        And8(cpu, 0x80u);
        BattlePresentMask(memory, cpu, 0xbc41u);
        And8(cpu, 0x0fu);
        StoreAAbsolute8(memory, cpu, 0x09fcu, 0u);
        BattleActiveMask(memory, cpu, 0xbc49u);
        And8(cpu, 0x10u);
        Or8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x09fcu, 0u)));
        Write8(memory, BATTLE_BATTLER_B, A8(cpu));
        return true;
    case 0xbc56u:
        LoadA8(cpu, Read8(memory, BATTLE_BATTLER_B));
        And8(cpu, 0x80u);
        BattleActiveMask(memory, cpu, 0xbc5eu);
        Write8(memory, BATTLE_BATTLER_B, A8(cpu));
        return true;
    case 0xbc66u:
        SetAccumulatorWidth(cpu, 0u);
        LoadA16(cpu, Read16Long(memory, BATTLE_BATTLER_A));
        Write16Long(memory, BATTLE_BATTLER_B, cpu->accumulator);
        return true;
    case 0xbbcdu:
    case 0xbc73u:
    case 0xbc9cu: {
        if (choice == 0xbc9cu)
            TransferDirectToA(cpu);
        else {
            LoadA8(cpu, Read8(memory, BATTLE_BATTLER_A));
            And8(cpu, 0x80u);
            if (choice == 0xbbcdu)
                LoadA8(cpu, (uint8_t)(A8(cpu) ^ 0x80u));
        }
        BattleActiveMask(memory, cpu, choice == 0xbbcdu ? 0xbbd7u : choice == 0xbc73u ? 0xbc7bu : 0xbc9fu);
        if (choice == 0xbc9cu) {
            And8(cpu, 0x0fu);
            if (cpu->zero) {
                LoadA8(cpu, 0xffu);
                Write8(memory, BATTLE_CONDITION, A8(cpu));
                return true;
            }
        }
        StoreADirect8(memory, cpu, BATTLE_SELECTION_MASK);
        SetIndexWidth(cpu, 1u);
        for (unsigned attempt = 0u;; ++attempt) {
            LoadA8(cpu, 6u);
            SimulateJslFrame(memory, cpu, 0x85u, choice == 0xbbcdu ? 0xbbe1u : choice == 0xbc73u ? 0xbc85u : 0xbcadu);
            Lufia2RandomScale(memory, cpu);
            SimulateRtlFrame(memory, cpu);
            TransferAToX(cpu);
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x96ffecu, cpu->x)));
            const uint8_t mask = DirectByte(memory, cpu, BATTLE_SELECTION_MASK);
            cpu->zero = (A8(cpu) & mask) == 0u;
            cpu->negative = (mask >> 7) & 1u;
            cpu->overflow = (mask >> 6) & 1u;
            if (!cpu->zero)
                break;
            if (attempt == 4095u) {
                cpu->resume_pc = choice == 0xbbcdu ? 0x85bbdcu : choice == 0xbc73u ? 0x85bc80u : 0x85bca8u;
                return false;
            }
        }
        SetIndexWidth(cpu, 0u);
        Or8(cpu, 0x80u);
        And8(cpu, DirectByte(memory, cpu, BATTLE_SELECTION_MASK));
        Write8(memory, BATTLE_BATTLER_B, A8(cpu));
        return true;
    }
    default:
        cpu->resume_pc = 0x850000u | choice;
        return false;
    }
}

static bool BattleScriptPartyByteOffset(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 0);
    PushAccumulator16(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    TransferDirectToA(cpu);
    LoadAAbsolute8(memory, cpu, BATTLE_LOCAL_SELECT, 0u);
    const bool first_battler = !cpu->zero;
    LoadA8(cpu, Read8(memory, first_battler ? BATTLE_BATTLER_A : BATTLE_BATTLER_B));
    BitImmediate8(cpu, 0x80u);
    if (!cpu->zero) {
        cpu->resume_pc = 0x85c0d8u;
        return false;
    }
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x97fe26u, cpu->x)));
    Write8(memory, SNES_WRMPYA, A8(cpu));
    LoadA8(cpu, 6u);
    Write8(memory, SNES_WRMPYB, A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Bank(memory, 0u, (uint16_t)(cpu->stack + 1u)));
    And16(cpu, 0xffu);
    cpu->carry = 0u;
    Add16Value(cpu, Read16Long(memory, SNES_RDMPYL));
    TransferAToX(cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtsFrame(memory, cpu);
    return true;
}

static bool BattleScriptPartyByte(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t return_address, bool write) {
    SimulateJsrFrame(memory, cpu, return_address);
    PushIndex(memory, cpu);
    if (!BattleScriptPartyByteOffset(memory, cpu, write ? 0xc0b6u : 0xc0adu))
        return false;
    if (write) {
        LoadA8(cpu, Read8(memory, (uint16_t)(cpu->stack + 1u)));
        StoreAAbsolute8(memory, cpu, WRAM_UNK_7E14E6, cpu->x);
    } else
        LoadAAbsolute8(memory, cpu, WRAM_UNK_7E14E6, cpu->x);
    cpu->x = PullIndexValue(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    return true;
}

static bool BattleOpPartyByte(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t handler) {
    BattleScriptByte(memory, cpu, (uint16_t)(handler + 2u));
    if (handler != BATTLE_OP_JUMP_IF_PARTY_BYTE_ZERO) {
        PushAccumulator8(memory, cpu);
        BattleScriptByte(memory, cpu, (uint16_t)(handler + 6u));
    }
    if (handler == BATTLE_OP_WRITE_PARTY_BYTE) {
        TransferAToX(cpu);
        LoadA8(cpu, Pull8(memory, cpu));
        return BattleScriptPartyByte(memory, cpu, 0xbdd6u, true);
    }
    if (!BattleScriptPartyByte(memory, cpu,
                               handler == BATTLE_OP_READ_PARTY_BYTE ? 0xbde3u : 0xbdf8u, false))
        return false;
    if (handler == BATTLE_OP_READ_PARTY_BYTE) {
        SetAccumulatorWidth(cpu, 0);
        And16(cpu, 0xffu);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, Pull8(memory, cpu));
        BattleScriptWrite(memory, cpu, 0xbdefu);
    } else {
        BitImmediate8(cpu, 0xffu);
        if (cpu->zero)
            BattleScriptJump(memory, cpu);
        else
            BattleScriptWord(memory, cpu, 0xbe02u);
    }
    return true;
}

static bool BattleOpClearWork(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t handler) {
    Lufia2ExecutionResult result;
    uint16_t return_address;
    switch (handler) {
    case BATTLE_OP_CLEAR_ACTION_WORK: return_address = 0xb907u; break;
    case BATTLE_OP_CLEAR_MESSAGE: return_address = 0xbe16u; break;
    case BATTLE_OP_CLEAR_PARTY_MODIFIERS: return_address = 0xbf9au; break;
    default: return_address = 0xbfa1u; break;
    }
    SimulateJslFrame(memory, cpu, 0x85u, return_address);
    switch (handler) {
    case BATTLE_OP_CLEAR_ACTION_WORK: result = Lufia2BattleClearActionWork(memory, cpu); break;
    case BATTLE_OP_CLEAR_MESSAGE: result = Lufia2BattleClearMessage(memory, cpu); break;
    case BATTLE_OP_CLEAR_PARTY_MODIFIERS: result = Lufia2BattleClearPartyModifiers(memory, cpu); break;
    default: result = Lufia2BattleClearEnemyModifiers(memory, cpu); break;
    }
    if (result.flow != LUFIA2_EXECUTION_RETURNED) {
        cpu->resume_pc = result.pc;
        return false;
    }
    SimulateRtlFrame(memory, cpu);
    return true;
}

static void BattleSaveContextValue(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t return_address, bool word) {
    SimulateJsrFrame(memory, cpu, return_address);
    if (word)
        LoadX16(cpu, (uint16_t)(cpu->x - 1u));
    LoadX16(cpu, (uint16_t)(cpu->x - 1u));
    const uint32_t address = LongIndexedAddress(WRAM_BATTLE_SCRIPT_CONTEXT_STACK, cpu->x);
    if (cpu->accumulator_is_8_bit)
        Write8(memory, address, A8(cpu));
    else
        Write16Long(memory, address, cpu->accumulator);
    SimulateRtsFrame(memory, cpu);
}

static void BattleScriptSaveContext(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    PushAccumulator16(memory, cpu);
    PushIndex(memory, cpu);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_BATTLE_SCRIPT_CONTEXT_STACK_INDEX, 0u));
    LoadA16(cpu, Read16Direct(memory, cpu, BATTLE_DP_LOCALS_F44E));
    BattleSaveContextValue(memory, cpu, 0xc29bu, true);
    LoadA16(cpu, Read16Direct(memory, cpu, BATTLE_DP_LOCALS_F450));
    BattleSaveContextValue(memory, cpu, 0xc2a0u, true);
    LoadA16(cpu, Read16Direct(memory, cpu, BATTLE_DP_SCRIPT_POINTER));
    BattleSaveContextValue(memory, cpu, 0xc2a5u, true);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, BATTLE_SCRIPT_BASE, 0u));
    BattleSaveContextValue(memory, cpu, 0xc2abu, true);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, DirectByte(memory, cpu, BATTLE_DP_SCRIPT_BANK));
    BattleSaveContextValue(memory, cpu, 0xc2b2u, false);
    LoadAAbsolute8(memory, cpu, BATTLE_SCRIPT_BANK, 0u);
    BattleSaveContextValue(memory, cpu, 0xc2b8u, false);
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    Write16Absolute(memory, cpu, WRAM_BATTLE_SCRIPT_CONTEXT_STACK_INDEX, cpu->x);
    cpu->x = PullIndexValue(memory, cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}

static void BattleRestoreContextValue(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    const uint32_t address = LongIndexedAddress(WRAM_BATTLE_SCRIPT_CONTEXT_STACK, cpu->x);
    if (cpu->accumulator_is_8_bit)
        LoadA8(cpu, Read8(memory, address));
    else
        LoadA16(cpu, Read16Long(memory, address));
    LoadX16(cpu, (uint16_t)(cpu->x + 1u));
    SimulateRtsFrame(memory, cpu);
}

static void BattleScriptRestoreContext(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    PushAccumulator16(memory, cpu);
    PushIndex(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_BATTLE_SCRIPT_CONTEXT_STACK_INDEX, 0u));
    BattleRestoreContextValue(memory, cpu, 0xc2d5u);
    StoreAAbsolute8(memory, cpu, BATTLE_SCRIPT_BANK, 0u);
    BattleRestoreContextValue(memory, cpu, 0xc2dbu);
    StoreADirect8(memory, cpu, BATTLE_DP_SCRIPT_BANK);
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    BattleRestoreContextValue(memory, cpu, 0xc2e2u);
    LoadX16(cpu, (uint16_t)(cpu->x + 1u));
    Write16Absolute(memory, cpu, BATTLE_SCRIPT_BASE, cpu->accumulator);
    BattleRestoreContextValue(memory, cpu, 0xc2e9u);
    LoadX16(cpu, (uint16_t)(cpu->x + 1u));
    Write16Direct(memory, cpu, BATTLE_DP_SCRIPT_POINTER, cpu->accumulator);
    BattleRestoreContextValue(memory, cpu, 0xc2efu);
    LoadX16(cpu, (uint16_t)(cpu->x + 1u));
    Write16Direct(memory, cpu, BATTLE_DP_LOCALS_F450, cpu->accumulator);
    BattleRestoreContextValue(memory, cpu, 0xc2f5u);
    LoadX16(cpu, (uint16_t)(cpu->x + 1u));
    Write16Direct(memory, cpu, BATTLE_DP_LOCALS_F44E, cpu->accumulator);
    Write16Absolute(memory, cpu, WRAM_BATTLE_SCRIPT_CONTEXT_STACK_INDEX, cpu->x);
    cpu->x = PullIndexValue(memory, cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}
static Lufia2ExecutionResult BattleOpDispatchAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    BattleScriptSaveContext(memory, cpu, 0xb53fu);
    if (!CallChildWithFrame(memory, cpu, child, child_context,
            0x85b540u, 0x81a832u, 3u, 0x85u)) {
        const Lufia2ExecutionResult result = {
            LUFIA2_EXECUTION_CHILD_UNWOUND, 0x85b540u, 0u};
        return result;
    }
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x85b544u);
    SimulateJslFrame(memory, cpu, 0x85u, 0xb547u);
    const Lufia2ExecutionResult clear = Lufia2BattleClearActionWork(memory, cpu);
    if (clear.flow != LUFIA2_EXECUTION_RETURNED)
        return clear;
    SimulateRtlFrame(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    BattleScriptRestoreContext(memory, cpu, 0xb54cu);
    return ExecutionReturned(0x85b54du);
}
static void BattleOpSpellAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    enum { DP_SPELL_COST = 0u, DP_SPELL_COST_HIGH = 1u };
    BattleScriptByte(memory, cpu, 0xbaa8u);
    StoreAAbsolute8(memory, cpu, WRAM_MENU_SPELL_RECORD_ID, 0u);
    SimulateJslFrame(memory, cpu, 0x85u, 0xbaafu);
    cpu->program_bank = 0x81u;
    (void)Lufia2SpellRecordByteC(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    cpu->program_bank = 0x85u;
    StoreADirect8(memory, cpu, DP_SPELL_COST);
    Write8(memory, DirectAddress(cpu, DP_SPELL_COST_HIGH), 0u);
    LoadA8(cpu, 1u);
    BattleStat(memory, cpu, 0xbab8u);
    Compare16(cpu, cpu->x, Read16Direct(memory, cpu, DP_SPELL_COST));
    bool usable = cpu->carry != 0;
    if (usable) {
        LoadA8(cpu, 20u);
        BattleStat(memory, cpu, 0xbac1u);
        TransferXToA(cpu);
        BitImmediate8(cpu, 2u);
        usable = cpu->zero != 0;
    }
    TransferDirectToA(cpu);
    if (!usable)
        StoreZeroAbsolute8(memory, cpu, WRAM_TEXT_WAIT_ACTOR, 0u);
    LoadAAbsolute8(memory, cpu, WRAM_MENU_SPELL_RECORD_ID, 0u);
    SetAccumulatorWidth(cpu, 0);
    Write16Long(memory, BATTLE_ACTION_ARGUMENT, cpu->accumulator);
    LoadA16(cpu, usable ? 18u : 2u);
    Write16Long(memory, BATTLE_ACTION_CODE, cpu->accumulator);
    if (!usable) {
        LoadA16(cpu, 2u);
        Write16Long(memory, BATTLE_CONDITION, cpu->accumulator);
        return;
    }
    TransferDirectToA(cpu);
    Write16Long(memory, BATTLE_CONDITION, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, BATTLE_ACTION_ARGUMENT));
    Write16Absolute(memory, cpu, WRAM_MENU_SPELL_RECORD_ID, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    SimulateJslFrame(memory, cpu, 0x85u, 0xbae9u);
    cpu->program_bank = 0x81u;
    (void)Lufia2LoadSpellRecord(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    cpu->program_bank = 0x85u;
    SimulateJslFrame(memory, cpu, 0x85u, 0xbaedu);
    (void)Lufia2BattleCopyRecordName(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    LoadX16(cpu, 120u);
    Write16Absolute(memory, cpu, WRAM_BATTLE_WAIT_COUNTER, cpu->x);
}
static bool BattleOpSummonEnemy(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    BattleScriptByte(memory, cpu, 0xbb17u);
    StoreADirect8(memory, cpu, DP_SCRATCH_A);
    if (!CallChildWithFrame(memory, cpu, child, child_context,
                            0x85bb1au, 0x85e0e4u, 3u, 0x85u))
        return false;
    const bool unavailable = cpu->carry != 0;
    SetAccumulatorWidth(cpu, 0);
    if (!unavailable) {
        LoadA16(cpu, 7u);
        Write16Long(memory, BATTLE_ACTION_CODE, cpu->accumulator);
        LoadA16(cpu, Read16Direct(memory, cpu, DP_SCRATCH_A));
        And16(cpu, 0x00ffu);
        Write16Long(memory, BATTLE_ACTION_ARGUMENT, cpu->accumulator);
        TransferDirectToA(cpu);
    } else {
        LoadA16(cpu, 2u);
    }
    Write16Long(memory, BATTLE_CONDITION, cpu->accumulator);
    return true;
}

static void BattleOpStatusMessage(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    BattleScriptByte(memory, cpu, 0xb90du);
    Write8(memory, BATTLE_ACTION_ARGUMENT + 2u, A8(cpu));
    SimulateJslFrame(memory, cpu, 0x85u, 0xb915u);
    (void)Lufia2BattleLoadStatusMessage(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    LoadX16(cpu, 120u);
    Write16Absolute(memory, cpu, WRAM_BATTLE_WAIT_COUNTER, cpu->x);
}

static bool BattleOpDisplayMessage(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    BattleScriptValue(memory, cpu, 0xbe08u);
    Write16Absolute(memory, cpu, WRAM_BATTLE_WAIT_COUNTER, cpu->x);
    return CallChildWithFrame(memory, cpu, child, child_context,
                              0x85be0cu, 0x8595feu, 3u, 0x85u) != 0;
}

static bool BattleOpPlayEffect(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    BattleScriptByte(memory, cpu, 0xbd56u);
    return CallChildWithFrame(memory, cpu, child, child_context,
                              0x85bd57u, 0x81895eu, 3u, 0x85u) != 0;
}
static Lufia2ExecutionResult BattleOpRepeatFrames(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    BattleScriptByte(memory, cpu, 0xbe2du);
    do {
        PushAccumulator8(memory, cpu);
        if (!CallChildWithFrame(memory, cpu, child, child_context,
                0x85be2fu, 0x858a2fu, 3u, 0x85u)) {
            const Lufia2ExecutionResult result = {
                LUFIA2_EXECUTION_CHILD_UNWOUND, 0x85be2fu, 0u};
            return result;
        }
        if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
            return ExecutionHandoff(cpu, 0x85be33u);
        LoadA8(cpu, 0xffu);
        StoreAAbsolute8(memory, cpu, WRAM_BATTLE_EFFECT_FRAME_MARKER, 0u);
        if (!CallChildWithFrame(memory, cpu, child, child_context,
                0x85be38u, 0x85ec81u, 3u, 0x85u)) {
            const Lufia2ExecutionResult result = {
                LUFIA2_EXECUTION_CHILD_UNWOUND, 0x85be38u, 0u};
            return result;
        }
        if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
            return ExecutionHandoff(cpu, 0x85be3cu);
        LoadA8(cpu, Pull8(memory, cpu));
        DecrementA8(cpu);
    } while (!cpu->zero);
    return ExecutionReturned(0x85be40u);
}

static void BattleOpShowPopups(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, bool explicit_mode) {
    if (explicit_mode)
        BattleScriptByte(memory, cpu, 0xbe4eu);
    else
        LoadA8(cpu, 4u);
    SimulateJslFrame(memory, cpu, 0x85u, explicit_mode ? 0xbe52u : 0xbe48u);
    const uint8_t caller_bank = cpu->program_bank;
    cpu->program_bank = 0x81u;
    (void)Lufia2BattlePopups(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    cpu->program_bank = caller_bank;
}

static void BattleOpSetResultFlag(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, bool only_normal) {
    SetAccumulatorWidth(cpu, 0);
    if (only_normal) {
        LoadA16(cpu, Read16Long(memory, BATTLE_RECORD_F45A));
        cpu->zero = (cpu->accumulator & 1u) == 0u;
        if (!cpu->zero)
            return;
    }
    LoadA16(cpu, only_normal ? 2u : 4u);
    Or16(cpu, Read16Long(memory, BATTLE_SCRIPT_RESULT_FLAGS));
    Write16Long(memory, BATTLE_SCRIPT_RESULT_FLAGS, cpu->accumulator);
}

static void BattleOpQueueAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t action) {
    BattleScriptByte(memory, cpu, action == 9u ? 0xbec1u : 0xbedcu);
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0xffu);
    Write16Long(memory, BATTLE_ACTION_ARGUMENT, cpu->accumulator);
    LoadA16(cpu, action);
    Write16Long(memory, BATTLE_ACTION_CODE, cpu->accumulator);
    TransferDirectToA(cpu);
    Write16Long(memory, BATTLE_CONDITION, cpu->accumulator);
}

/* $05: jump when a random byte is below n. */
static void BattleOpRandomJump(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    BattleScriptByte(memory, cpu, 0xb584u);
    StoreADirect8(memory, cpu, DP_SCRATCH_A);
    LoadA8(cpu, 0xffu);
    SimulateJslFrame(memory, cpu, 0x85u, 0xb58cu);
    Lufia2RandomScale(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    cpu->carry = 1;
    Sbc8(cpu, DirectByte(memory, cpu, DP_SCRATCH_A));
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
    Compare16(cpu, cpu->x, Read16Direct(memory, cpu, DP_SCRATCH_A));
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
        Add16Value(cpu, Read16Direct(memory, cpu, DP_SCRATCH_A));
    } else {
        LoadA16(cpu, Read16Direct(memory, cpu, DP_SCRATCH_A));
        Write16Direct(memory, cpu, DP_SCRATCH_A, cpu->x);
        cpu->carry = 1;
        Add16Value(cpu, (uint16_t)~Read16Direct(memory, cpu, DP_SCRATCH_A));
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
    Write16Direct(memory, cpu, DP_SCRATCH_A, cpu->x);
    BattleScriptValue(memory, cpu, (uint16_t)(handler + 11u));
    SetAccumulatorWidth(cpu, 0);
    value = Read16Direct(memory, cpu, DP_SCRATCH_A);
    value = handler == BATTLE_OP_AND ? (uint16_t)(cpu->x & value)
        : handler == BATTLE_OP_OR ? (uint16_t)(cpu->x | value)
        : (uint16_t)(cpu->x ^ value);
    BattleScriptStoreBinary(memory, cpu, value,
        (uint16_t)(handler + 0x17u));
}

static void BattleScriptCapsuleAbilities(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    PushIndex(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1);
    TransferDirectToA(cpu);
    LoadAAbsolute8(memory, cpu, WRAM_CAPSULE_SELECTED_ID, 0u);
    AslA8(cpu);
    SetAccumulatorWidth(cpu, 0);
    TransferAToX(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu,
                                      WRAM_CAPSULE_FLAG_BYTES + 8u, cpu->x));
    PushAccumulator16(memory, cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_CAPSULE_SELECTED_FORM, 0u));
    And16(cpu, 0xffu);
    TransferAToX(cpu);
    PullAccumulator16(memory, cpu);
    for (;;) {
        LoadX16(cpu, (uint16_t)(cpu->x - 1u));
        if (cpu->zero)
            break;
        LsrA16(cpu);
        LsrA16(cpu);
        LsrA16(cpu);
    }
    And16(cpu, 7u);
    UnpackStatus(cpu, Pull8(memory, cpu));
    cpu->x = PullIndexValue(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

static void BattleOpCapsuleAbilities(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, bool jump) {
    if (jump) {
        LoadA8(cpu, 1u);
        StoreADirect8(memory, cpu, DP_SCRATCH_B);
        BattleScriptByte(memory, cpu, 0xb658u);
        for (;;) {
            LoadA8(cpu, (uint8_t)(A8(cpu) - 1u));
            if (cpu->zero)
                break;
            AslDirect8(memory, cpu, DP_SCRATCH_B);
        }
        BattleScriptCapsuleAbilities(memory, cpu, 0xb662u);
        And8(cpu, DirectByte(memory, cpu, DP_SCRATCH_B));
        if (!cpu->zero)
            BattleScriptJump(memory, cpu);
        else
            BattleScriptWord(memory, cpu, 0xb66cu);
    } else {
        BattleScriptCapsuleAbilities(memory, cpu, 0xb8f9u);
        TransferAToX(cpu);
        BattleScriptByte(memory, cpu, 0xb8fdu);
        BattleScriptWrite(memory, cpu, 0xb900u);
    }
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
    Write16Long(memory, BATTLE_RECORD_F45C, cpu->accumulator);
}

/* $20: bytes into $7F:F45E/F460. */
static void BattleOpSetF45E(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    BattleScriptByte(memory, cpu, 0xb92eu);
    Write8(memory, BATTLE_RECORD_F45E, A8(cpu));
    BattleScriptByte(memory, cpu, 0xb935u);
    Write8(memory, BATTLE_RECORD_F460, A8(cpu));
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
    Write16Long(memory, BATTLE_RECORD_F45C, cpu->accumulator);
    LoadA16(cpu, 0x0020u);
    Write16Long(memory, BATTLE_RECORD_F45A, cpu->accumulator);
    LoadA16(cpu, 0x0005u);
    Write16Long(memory, BATTLE_ACTION_CODE, cpu->accumulator);
    BattleScriptValue(memory, cpu, 0xb957u);
    LoadA16(cpu, (uint16_t)(0u - cpu->x));
    Write16Long(memory, BATTLE_RECORD_F462, cpu->accumulator);
    LoadA16(cpu, 0x0020u);
    Write16Long(memory, BATTLE_RECORD_F464, cpu->accumulator);
    BattleScriptByte(memory, cpu, 0xb96au);
}

/* $23: step setup. */
static void BattleOpStepSetup(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    BattleScriptWord(memory, cpu, 0xb9bau);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, (uint16_t)(0u - cpu->x));
    Write16Long(memory, BATTLE_RECORD_F462, cpu->accumulator);
    LoadA16(cpu, 0x0020u);
    Write16Long(memory, BATTLE_RECORD_F464, cpu->accumulator);
    Write16Long(memory, BATTLE_RECORD_F45A, cpu->accumulator);
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
    LoadA16(cpu,
            Read16Long(memory, LongIndexedAddress(BATTLE_RECORD_WORD_OFFSETS, cpu->x)));
    And16(cpu, 0x00ffu);
    PushAccumulator16(memory, cpu);
    BattleScriptValue(memory, cpu, 0xb9edu);
    LoadA16(cpu, cpu->x);
    cpu->x = PullIndexValue(memory, cpu);
    Write16Long(memory, LongIndexedAddress(BATTLE_SCRIPT_RECORD, cpu->x),
                cpu->accumulator);
    IncrementX16(cpu);
    IncrementX16(cpu);
    SetAccumulatorWidth(cpu, 1);
    BattleScriptByte(memory, cpu, 0xb9fau);
    Write8(memory, LongIndexedAddress(BATTLE_SCRIPT_RECORD, cpu->x), A8(cpu));
}

/* $25: battler record byte, cleared. */
static void BattleOpRecordByteClear(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    BattleScriptByte(memory, cpu, 0xba04u);
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x00ffu);
    TransferAToX(cpu);
    LoadA16(cpu,
            Read16Long(memory, LongIndexedAddress(BATTLE_RECORD_WORD_OFFSETS, cpu->x)));
    And16(cpu, 0x00ffu);
    cpu->carry = 0;
    Add16Value(cpu, 0x0004u);
    TransferAToX(cpu);
    BattleScriptByte(memory, cpu, 0xba19u);
    Write16Long(memory, LongIndexedAddress(BATTLE_SCRIPT_RECORD, cpu->x),
                cpu->accumulator);
    cpu->x = (uint16_t)(cpu->x - 2u);
    SetNz16(cpu, cpu->x);
    TransferDirectToA(cpu);
    Write16Long(memory, LongIndexedAddress(BATTLE_SCRIPT_RECORD, cpu->x),
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
    LoadA16(cpu,
            Read16Long(memory, LongIndexedAddress(BATTLE_RECORD_BYTE_OFFSETS, cpu->x)));
    And16(cpu, 0x00ffu);
    if (handler == BATTLE_OP_RECORD_BYTE_26)
        LoadA16(cpu, (uint16_t)(cpu->accumulator + 2u));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    BattleScriptByte(memory, cpu,
        handler == BATTLE_OP_RECORD_BYTE_26 ? 0xba3fu : 0xba5cu);
    Write8(memory, LongIndexedAddress(BATTLE_SCRIPT_RECORD, cpu->x), A8(cpu));
}

/* $28: action code 1. */
static void BattleOpAction1(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, 0x0001u);
    Write16Long(memory, BATTLE_ACTION_CODE, cpu->accumulator);
    LoadA16(cpu, 0x0000u);
    Write16Long(memory, BATTLE_ACTION_ARGUMENT, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    StoreAImmediate8(memory, cpu, 0xffu, WRAM_PALETTE_FADE);
    StoreZeroAbsolute8(memory, cpu, WRAM_TEXT_WAIT_ACTOR, 0);
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
    Write16Long(memory, BATTLE_ACTION_ARGUMENT, cpu->accumulator);
}

/* $3E: action code 11. */
static void BattleOpAction11(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA8(cpu, 0x0bu);
    Write8(memory, BATTLE_ACTION_CODE, A8(cpu));
    BattleScriptByte(memory, cpu, 0xbd66u);
    Write8(memory, BATTLE_ACTION_ARGUMENT, A8(cpu));
}

/* $2F: count battlers with bit 2 set. */
static void BattleOpPartyFlagCount(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA8(cpu, Read8(memory, BATTLE_BATTLER_B));
    if (cpu->negative) {
        LoadX16(cpu, 0x0000u);
    } else {
        Write8(memory, DirectAddress(cpu, DP_SCRATCH_A), 0x00u);
        LoadY16(cpu, 0x0008u);
        do {
            LoadX16(cpu, Read16AbsoluteIndexed(
                memory, cpu, WRAM_BATTLE_PARTY_RECORDS, cpu->y));        /* BB5F */
            if (!cpu->zero) {
                LoadAAbsolute8(memory, cpu, 0x000fu, cpu->x);
                BitImmediate8(cpu, 0x04u);
                if (!cpu->zero)
                    IncrementDirect8(memory, cpu, DP_SCRATCH_A);
            }
            cpu->y = (uint16_t)(cpu->y - 2u);
            SetNz16(cpu, cpu->y);
        } while (!cpu->negative);
        TransferDirectToA(cpu);
        LoadA8(cpu, DirectByte(memory, cpu, DP_SCRATCH_A));
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
    LoadA8(cpu, Read8(memory, BATTLE_BATTLER_A));
    if (cpu->negative) {
        LoadAAbsolute8(memory, cpu, 0x15feu, 0);
    } else {
        LoadAAbsolute8(memory, cpu, WRAM_BATTLE_SCRIPT_CONTEXT, 0);
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
    StoreAAbsolute8(memory, cpu, BATTLE_SCRIPT_UNK_0A62, 0);
}

/* $36: byte into $0A62 when $7F:F42E. */
static void BattleOpSet0A62IfF42E(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    BattleScriptByte(memory, cpu, 0xbcdau);
    StoreADirect8(memory, cpu, DP_SCRATCH_A);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, BATTLE_CONDITION));
    if (!cpu->zero) {
        LoadA16(cpu, Read16Direct(memory, cpu, DP_SCRATCH_A));
        Write16Absolute(memory, cpu, BATTLE_SCRIPT_UNK_0A62, cpu->accumulator);
    }
}

/* $56: timer $1264. */
static void BattleOpSetTimer(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    BattleScriptValue(memory, cpu, 0xbe24u);
    Write16Absolute(memory, cpu, WRAM_BATTLE_WAIT_COUNTER, cpu->x);
}

/* $50: battler mask $7F:F450. */
static void BattleOpF450Mask(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadAAbsolute8(memory, cpu, BATTLE_BATTLER_MASKS, 0);
    if (!cpu->negative) {
        LoadA8(cpu, (uint8_t)(Read8(memory, BATTLE_BATTLER_B) | 0xefu));
        LoadA8(cpu,
               (uint8_t)(A8(cpu) & Read8(memory, AbsoluteIndexedAddress(
                                                     cpu, BATTLE_BATTLER_MASKS, 0))));
        Write8(memory, BATTLE_BATTLER_B, A8(cpu));
    }
}

/* $0F: multiply. */
static void BattleOpMultiply(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    BattleScriptOperands(memory, cpu, handler);
    Write16Direct(memory, cpu, DP_SCRATCH_C, cpu->x);
    PushAccumulator8(memory, cpu);
    BattleMultiply(memory, cpu, 0xb6c8u);
    LoadA8(cpu, Pull8(memory, cpu));
    LoadX16(cpu, Read16Direct(memory, cpu, 0x63u));
    BattleScriptWrite(memory, cpu, 0xb6ceu);
}

static void BattleOpUnsignedDivide(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    BattleScriptByte(memory, cpu, 0xb71fu);
    BattleScriptRead(memory, cpu, 0xb722u);
    PushAccumulator8(memory, cpu);
    Write16Direct(memory, cpu, 0x5du, cpu->x);
    BattleScriptValue(memory, cpu, 0xb728u);
    TransferXToA(cpu);
    StoreADirect8(memory, cpu, DP_SCRATCH_A);
    Write8(memory, DirectAddress(cpu, 0x5fu), 0u);
    BattleDivide(memory, cpu, 0xb731u);
    LoadX16(cpu, Read16Direct(memory, cpu, 0x5du));
    LoadA8(cpu, Pull8(memory, cpu));
    BattleScriptWrite(memory, cpu, 0xb737u);
}

enum BattleScaleScratch {
    BATTLE_SCALE_MULTIPLICAND = 0x4e,
    BATTLE_SCALE_MULTIPLIER = 0x50,
    BATTLE_SCALE_PRODUCT = 0x51,
    BATTLE_SCALE_PRODUCT_MIDDLE = 0x52,
    BATTLE_SCALE_PRODUCT_HIGH = 0x53,
    BATTLE_SCALE_SIGNED_FIELD = 0x14,
    BATTLE_SCALE_FIRST_LIMITED_FIELD = 0x30,
    BATTLE_SCALE_SECOND_LIMITED_FIELD = 0x42
};

static void BattleScaleActionValue(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Write16Direct(memory, cpu, BATTLE_SCALE_MULTIPLICAND, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadAAbsolute8(memory, cpu, WRAM_PARTY_STAT_MEMBER_INDEX, 0u);
    StoreADirect8(memory, cpu, BATTLE_SCALE_MULTIPLIER);
    SimulateJslFrame(memory, cpu, 0x85u, 0xbf7fu);
    const uint8_t caller_bank = cpu->program_bank;
    cpu->program_bank = 0x80u;
    (void)Lufia2Multiply16By8(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    cpu->program_bank = caller_bank;
    LoadA8(cpu, DirectByte(memory, cpu, BATTLE_SCALE_PRODUCT));
    ExchangeAccumulatorBytes(cpu);
    SetAccumulatorWidth(cpu, 0);
    for (unsigned shift = 0; shift < 2u; ++shift) {
        AslA16(cpu);
        const uint16_t middle = Read16Direct(memory, cpu, BATTLE_SCALE_PRODUCT_MIDDLE);
        const uint16_t shifted = (uint16_t)((middle << 1) | cpu->carry);
        cpu->carry = (middle & 0x8000u) != 0u;
        Write8(memory, DirectAddress(cpu, BATTLE_SCALE_PRODUCT_HIGH), (uint8_t)(shifted >> 8));
        Write8(memory, DirectAddress(cpu, BATTLE_SCALE_PRODUCT_MIDDLE), (uint8_t)shifted);
        SetNz16(cpu, shifted);
        if (cpu->carry)
            break;
    }
    if (!cpu->carry) {
        LoadA16(cpu, Read16Direct(memory, cpu, BATTLE_SCALE_PRODUCT_MIDDLE));
        if (cpu->negative)
            LoadA16(cpu, 0x7fffu);
    } else
        LoadA16(cpu, 0x7fffu);
    SimulateRtsFrame(memory, cpu);
}

static void BattleOpScaleAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    BattleScriptWord(memory, cpu, 0xbef7u);
    Write16Absolute(memory, cpu, WRAM_BATTLE_ACTION_BIAS, cpu->x);
    LoadA8(cpu, Read8(memory, BATTLE_BATTLER_A));
    if (!cpu->negative) {
        BattleScriptSlot(memory, cpu, 0xbf03u);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x6eu, cpu->x));
        And16(cpu, 0x1ffu);
        Compare16(cpu, cpu->accumulator,
                  Read16AbsoluteIndexed(memory, cpu, WRAM_BATTLE_ACTION_BIAS, 0u));
        if (cpu->zero) {
            BattleScriptByte(memory, cpu, 0xbf1bu);
            Write16Absolute(memory, cpu, WRAM_PARTY_STAT_MEMBER_INDEX, cpu->accumulator);
            SetAccumulatorWidth(cpu, 0);
            LoadX16(cpu, BATTLE_SCALE_SIGNED_FIELD);
            LoadA16(cpu, Read16Long(memory, LongIndexedAddress(BATTLE_SCRIPT_RECORD, cpu->x)));
            if (!cpu->zero) {
                const bool negative = cpu->negative;
                if (negative) {
                    LoadA16(cpu, (uint16_t)~cpu->accumulator);
                    LoadA16(cpu, (uint16_t)(cpu->accumulator + 1u));
                }
                BattleScaleActionValue(memory, cpu, negative ? 0xbf37u : 0xbf2eu);
                if (negative) {
                    LoadA16(cpu, (uint16_t)~cpu->accumulator);
                    LoadA16(cpu, (uint16_t)(cpu->accumulator + 1u));
                }
                Write16Long(memory, LongIndexedAddress(BATTLE_SCRIPT_RECORD, cpu->x), cpu->accumulator);
            }
            for (unsigned field = 0; field < 2u; ++field) {
                LoadX16(cpu, field ? BATTLE_SCALE_SECOND_LIMITED_FIELD : BATTLE_SCALE_FIRST_LIMITED_FIELD);
                LoadA16(cpu, Read16Long(memory, LongIndexedAddress(BATTLE_SCRIPT_RECORD, cpu->x)));
                if (!cpu->zero) {
                    BattleScaleActionValue(memory, cpu, field ? 0xbf63u : 0xbf4bu);
                    Compare16(cpu, cpu->accumulator, 100u);
                    if (cpu->carry)
                        LoadA16(cpu, 100u);
                    Write16Long(memory, LongIndexedAddress(BATTLE_SCRIPT_RECORD, cpu->x), cpu->accumulator);
                }
            }
            return;
        }
    }
    SetAccumulatorWidth(cpu, 1);
    BattleScriptByte(memory, cpu, 0xbf15u);
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
    StoreADirect8(memory, cpu, DP_SCRATCH_A);
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
    BattleCallRandomFraction(memory, cpu, 0xb748u);
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
    Write16Direct(memory, cpu, DP_SCRATCH_E, cpu->x);
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
    Write16Long(memory, BATTLE_RECORD_F45C, cpu->accumulator);
    LoadA16(cpu, 0x01dcu);
    Write16Long(memory, BATTLE_RECORD_F45A, cpu->accumulator);
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
    Write16Long(memory, BATTLE_RECORD_F462, cpu->accumulator);
    LoadA16(cpu, 0x0020u);
    Write16Long(memory, BATTLE_RECORD_F464, cpu->accumulator);
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
    Write16Long(memory, BATTLE_ACTION_ARGUMENT, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, BATTLE_RECORD_F462));
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
        Write16Long(memory, BATTLE_RECORD_F462, cpu->accumulator);
        PullAccumulator16(memory, cpu);
    }
    LoadA16(cpu, 0x0040u);                             /* BD21 */
    Write16Long(memory, BATTLE_RECORD_F464, cpu->accumulator);
}

/* $3C: extended jump. */
static void BattleOpExtendJump(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA8(cpu, 0x08u);
    BattleStat(memory, cpu, 0xbd30u);
    Write16Direct(memory, cpu, 0xcau, cpu->x);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, BATTLE_RECORD_F462));
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
        Write16Long(memory, BATTLE_RECORD_F462, cpu->accumulator);
    }
}

/* $4D: mask of living battlers. */
static void BattleOpLivingMask(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA8(cpu, Read8(memory, BATTLE_BATTLER_B));
    BattleActiveMask(memory, cpu, 0xbe5cu);
    LoadA8(cpu, (uint8_t)(A8(cpu) & Read8(memory, BATTLE_BATTLER_B)));
    Write8(memory, BATTLE_BATTLER_B, A8(cpu));
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
Lufia2ExecutionResult Lufia2BattleScriptWithChildren(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
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
        case BATTLE_OP_SELECT_BATTLERS:
            if (BattleOpSelectBattlers(memory, cpu))
                break;
            result = ExecutionHandoff(cpu, cpu->resume_pc);
            result.dispatches = opcodes;
            return result;
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
        case BATTLE_OP_JUMP_IF_CAPSULE_ABILITY:
        case BATTLE_OP_CAPSULE_ABILITIES:
            BattleOpCapsuleAbilities(memory, cpu, handler == BATTLE_OP_JUMP_IF_CAPSULE_ABILITY);
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
        case BATTLE_OP_UNSIGNED_DIVIDE:
            BattleOpUnsignedDivide(memory, cpu);
            break;
        case BATTLE_OP_SCALE_ACTION:
            BattleOpScaleAction(memory, cpu);
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
        case BATTLE_OP_AVERAGE_TARGET_STAT:
        case BATTLE_OP_WRITE_TARGET_STAT:
        case BATTLE_OP_AVERAGE_LOCAL_STAT:
        case BATTLE_OP_AVERAGE_ACTOR_STAT:
            if (BattleOpGroupStat(memory, cpu, handler))
                break;
            result = ExecutionHandoff(cpu, cpu->resume_pc);
            result.dispatches = opcodes;
            return result;
        case BATTLE_OP_WRITE_PARTY_BYTE:
        case BATTLE_OP_READ_PARTY_BYTE:
        case BATTLE_OP_JUMP_IF_PARTY_BYTE_ZERO:
            if (BattleOpPartyByte(memory, cpu, handler))
                break;
            result = ExecutionHandoff(cpu, cpu->resume_pc);
            result.dispatches = opcodes;
            return result;
        case BATTLE_OP_CLEAR_ACTION_WORK:
        case BATTLE_OP_CLEAR_MESSAGE:
        case BATTLE_OP_CLEAR_PARTY_MODIFIERS:
        case BATTLE_OP_CLEAR_ENEMY_MODIFIERS:
            if (BattleOpClearWork(memory, cpu, handler))
                break;
            result = ExecutionHandoff(cpu, cpu->resume_pc);
            result.dispatches = opcodes;
            return result;
        case BATTLE_OP_DISPATCH_ACTION:
            if (!child) {
                result = ExecutionHandoff(cpu, 0x85b470u);
                result.dispatches = opcodes;
                return result;
            }
            result = BattleOpDispatchAction(memory, cpu, child, child_context);
            if (result.flow == LUFIA2_EXECUTION_RETURNED)
                break;
            result.dispatches = opcodes;
            return result;
        case BATTLE_OP_SUMMON_ENEMY:
            if (!child) {
                result = ExecutionHandoff(cpu, 0x85b470u);
                result.dispatches = opcodes;
                return result;
            }
            if (!BattleOpSummonEnemy(memory, cpu, child, child_context)) {
                result = ExecutionReturned(0x85bb1au);
                result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
                result.dispatches = opcodes;
                return result;
            }
            break;
        case BATTLE_OP_SPELL_ACTION:
            BattleOpSpellAction(memory, cpu);
            break;
        case BATTLE_OP_STATUS_MESSAGE:
            BattleOpStatusMessage(memory, cpu);
            break;
        case BATTLE_OP_DISPLAY_MESSAGE:
        case BATTLE_OP_PLAY_EFFECT:
            if (!child) {
                result = ExecutionHandoff(cpu, 0x85b470u);
                result.dispatches = opcodes;
                return result;
            }
            if (!(handler == BATTLE_OP_DISPLAY_MESSAGE
                    ? BattleOpDisplayMessage(memory, cpu, child, child_context)
                    : BattleOpPlayEffect(memory, cpu, child, child_context))) {
                result = ExecutionReturned(handler == BATTLE_OP_DISPLAY_MESSAGE
                    ? 0x85be0cu : 0x85bd57u);
                result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
                result.dispatches = opcodes;
                return result;
            }
            if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit) {
                result = ExecutionHandoff(cpu, handler == BATTLE_OP_DISPLAY_MESSAGE
                    ? 0x85be10u : 0x85bd5bu);
                result.dispatches = opcodes;
                return result;
            }
            break;
        case BATTLE_OP_REPEAT_FRAMES:
            if (!child) {
                result = ExecutionHandoff(cpu, 0x85b470u);
                result.dispatches = opcodes;
                return result;
            }
            result = BattleOpRepeatFrames(memory, cpu, child, child_context);
            if (result.flow == LUFIA2_EXECUTION_RETURNED)
                break;
            result.dispatches = opcodes;
            return result;
        case BATTLE_OP_SHOW_POPUPS:
        case BATTLE_OP_SHOW_POPUPS_MODE:
            BattleOpShowPopups(memory, cpu, handler == BATTLE_OP_SHOW_POPUPS_MODE);
            break;
        case BATTLE_OP_WAIT_COUNTER:
            LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu,
                WRAM_BATTLE_WAIT_COUNTER, 0u));
            if (!cpu->zero) {
                result = ExecutionHandoff(cpu, 0x85be1au);
                result.dispatches = opcodes;
                return result;
            }
            break;
        case BATTLE_OP_NORMAL_RESULT_FLAG:
        case BATTLE_OP_RESULT_FLAG:
            BattleOpSetResultFlag(memory, cpu, handler == BATTLE_OP_NORMAL_RESULT_FLAG);
            break;
        case BATTLE_OP_ACTION_9:
        case BATTLE_OP_ACTION_25:
            BattleOpQueueAction(memory, cpu, handler == BATTLE_OP_ACTION_9 ? 9u : 25u);
            break;
        case BATTLE_OP_FADE_VALUE:
            BattleScriptByte(memory, cpu, 0xbfa7u);
            StoreAAbsolute8(memory, cpu, WRAM_PALETTE_FADE, 0u);
            break;
        case BATTLE_OP_CLEAR_F8A5:
            TransferDirectToA(cpu);
            Write8(memory, WRAM_BATTLE_SCRIPT_UNK_F8A5, A8(cpu));
            break;
        case BATTLE_OP_ALTERNATE_ACTION:
            LoadA8(cpu, 0xffu);
            Write8(memory, WRAM_BATTLE_ACTION_ALTERNATE, A8(cpu));
            break;
        default:
            result = ExecutionHandoff(cpu, 0x85b470u);
            result.dispatches = opcodes;
            return result;
        }
    }
}

Lufia2ExecutionResult Lufia2BattleResolveActorRecord(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu) {
    uint8_t bank = cpu->data_bank;
    if (cpu->program_bank != 0x85u || cpu->direct_page || cpu->decimal ||
        !cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->stack < 0x1f20u ||
        cpu->stack > 0x1ffcu || !(bank < 0x40u || (bank >= 0x7eu && bank < 0xc0u)))
        return ExecutionHandoff(cpu, 0x85c168u);
    return ExecutionReturned(SelectActorRecord(memory, cpu) ? 0x85c1a0u : 0x85c199u);
}

Lufia2ExecutionResult Lufia2BattleScript(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return Lufia2BattleScriptWithChildren(memory, cpu, NULL, NULL);
}
