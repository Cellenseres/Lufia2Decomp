/* Field event script slots, their wait timers and the script VM. */

#include "core/cpu_internal.h"
#include "lufia2/field.h"
#include "actor/actor_internal.h"
#include "field/event_script_internal.h"
#include "field/field_internal.h"
#include "system/wram.h"

/* $80:E8B9: next script byte; a wrapping Y steps to the next bank. */
void Lufia2EventNextByte(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadScriptByteY(memory, cpu);                              /* E8B9 */
    IncrementY16(cpu);
    if (!cpu->negative) {
        PushAccumulator8(memory, cpu);                         /* E8BF */
        LoadA8(cpu, (uint8_t)(Read8(memory, EVENT_SCRIPT_BANK) + 1u));
        Write8(memory, EVENT_SCRIPT_BANK, A8(cpu));
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        LoadA8(cpu, Pull8(memory, cpu));
        LoadY16(cpu, 0x8000u);
    }
    SimulateRtsFrame(memory, cpu);
}

/* Read-only: a $FF within limit script bytes from Y. */
uint8_t Lufia2EventListEnds(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu,
    unsigned limit) {
    uint8_t bank = cpu->data_bank;
    uint8_t script_bank = Read8(memory, EVENT_SCRIPT_BANK);
    uint16_t y = cpu->y;

    while (limit--) {
        const uint8_t value = Read8(memory, ((uint32_t)bank << 16) | y);

        if (++y < 0x8000u) {
            bank = ++script_bank;
            y = 0x8000u;
        }
        if (value == 0xffu)
            return 1;
    }
    return 0;
}

/* $80:E8D0: step back one byte; below $8000 the bank steps down. */
void Lufia2EventPrevByte(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    cpu->y = (uint16_t)(cpu->y - 1u);                          /* E8D0 */
    SetNz16(cpu, cpu->y);
    if (!cpu->negative) {
        LoadA8(cpu, (uint8_t)(Read8(memory, EVENT_SCRIPT_BANK) - 1u));
        Write8(memory, EVENT_SCRIPT_BANK, A8(cpu));
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        LoadY16(cpu, 0xffffu);
    }
    SimulateRtsFrame(memory, cpu);
}

/* $80:E8AD: word operand, low byte first; leaves M=0. */
void Lufia2EventNextWord(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Lufia2EventNextByte(memory, cpu, 0xe8afu);                       /* E8AD */
    PushAccumulator8(memory, cpu);
    Lufia2EventNextByte(memory, cpu, 0xe8b3u);
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Pull8(memory, cpu));
    SetAccumulatorWidth(cpu, 0);
    SimulateRtsFrame(memory, cpu);
}

/* $80:E8F4: Y = A in the base bank; below $8000 steps a bank. */
void Lufia2EventSetPointer(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    TransferAToY(cpu);                                         /* E8F4 */
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, EVENT_SCRIPT_BASE_BANK));
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, cpu->y);
    if (!cpu->negative) {
        LoadA16(cpu, (uint16_t)(cpu->accumulator | 0x8000u));  /* E902 */
        TransferAToY(cpu);
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu,
            (uint8_t)(Read8(memory, EVENT_SCRIPT_BASE_BANK) + 1u));
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        SetAccumulatorWidth(cpu, 0);
    }
    SimulateRtsFrame(memory, cpu);
}

/* $80:D2A2: goto base + word. */
void Lufia2EventGoto(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2EventNextWord(memory, cpu, 0xd2a4u);                       /* D2A2 */
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, EVENT_SCRIPT_BASE));
    Lufia2EventSetPointer(memory, cpu, 0xd2acu);
    SetAccumulatorWidth(cpu, 1);
}

/* $80:D2B2: skip an untaken goto target. */
void Lufia2EventSkipWord(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2EventNextByte(memory, cpu, 0xd2b4u);                       /* D2B2 */
    Lufia2EventNextByte(memory, cpu, 0xd2b7u);
}

/* $80:E9BC: $FB-$FF read slot variables ($C0-$DF as 0-$1F). */
void Lufia2EventVariable(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Compare8(cpu, A8(cpu), 0xfbu);                             /* E9BC */
    if (cpu->carry) {
        cpu->carry = 1;
        Sbc8(cpu, 0xfbu);
        /* DB-relative: only the script bank reaches the multiplier. */
        StoreAAbsolute8(memory, cpu, SNES_WRMPYA, 0);
        StoreAImmediate8(memory, cpu, 0x08u, SNES_WRMPYB);
        LoadA8(cpu, 0x7fu);
        StoreADirect8(memory, cpu, 0x5fu);
        SetAccumulatorWidth(cpu, 0);                           /* E9CF */
        cpu->carry = 0;
        LoadADirect16(memory, cpu, DP_ACTOR_SLOT);
        Add16Value(cpu, 0xd15cu);
        Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, SNES_RDMPYL, 0));
        StoreADirect16(memory, cpu, 0x5du);
        SetAccumulatorWidth(cpu, 1);
        TransferDirectToA(cpu);                                /* E9DE */
        LoadA8(cpu, Read8(memory, DirectLongPointer(memory, cpu, 0x5du)));
        Compare8(cpu, A8(cpu), 0xe0u);
        if (!cpu->carry) {
            Compare8(cpu, A8(cpu), 0xc0u);
            if (cpu->carry) {
                cpu->carry = 1;
                Sbc8(cpu, 0xc0u);
            }
        }
    }
    SimulateRtsFrame(memory, cpu);
}

/* $80:E9ED: value operand; $A0-$BF read variable n, $FB stays. */
void Lufia2EventValue(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Compare8(cpu, A8(cpu), 0xfbu);                             /* E9ED */
    if (!cpu->zero) {
        Lufia2EventVariable(memory, cpu, 0xe9f3u);
        Compare8(cpu, A8(cpu), 0xc0u);
        if (!cpu->carry) {
            Compare8(cpu, A8(cpu), 0xa0u);
            if (cpu->carry) {
                PushIndex(memory, cpu);                        /* E9FC */
                ExchangeAccumulatorBytes(cpu);
                LoadA8(cpu, 0x00u);
                ExchangeAccumulatorBytes(cpu);
                TransferAToX(cpu);
                TransferDirectToA(cpu);
                LoadA8(cpu, Read8(memory,
                    LongIndexedAddress(EVENT_VARIABLES, cpu->x)));
                cpu->x = PullIndexValue(memory, cpu);
            }
        }
    }
    SimulateRtsFrame(memory, cpu);
}

/* $80:D9F0: X = variable operand, A = next byte. */
static void EventVariableOperands(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    TransferDirectToA(cpu);                                    /* D9F0 */
    Lufia2EventNextByte(memory, cpu, 0xd9f3u);
    Lufia2EventVariable(memory, cpu, 0xd9f6u);
    TransferAToX(cpu);
    Lufia2EventNextByte(memory, cpu, 0xd9fau);
    SimulateRtsFrame(memory, cpu);
}

/* $80:E898: X = flag byte of n, A = its bit from $80:BE45. */
void Lufia2EventFlagBit(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x80u, return_address);
    StoreADirect8(memory, cpu, 0x54u);                         /* E898 */
    And8(cpu, 0x07u);
    /* TAX keeps B, so DP high enters both indexes. */
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x80be45u, cpu->x)));
    StoreADirect8(memory, cpu, 0x55u);
    TransferDirectToA(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x54u));
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    TransferAToX(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x55u));
    SimulateRtlFrame(memory, cpu);
}

/* $00 and aliases: disarm the slot (stores the DP low byte). */
static unsigned EventOpEnd(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);                 /* CC42 */
    TransferDirectToA(cpu);
    Write8(memory, LongIndexedAddress(EVENT_SLOT_TIMERS, cpu->x), A8(cpu));
    return EVENT_OPCODE_YIELD;
}

/* $80:D31C: sleep A frames, the slot resumes at Y. */
unsigned Lufia2EventSleep(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);                 /* D31C */
    Or8(cpu, 0x80u);
    Write8(memory, LongIndexedAddress(EVENT_SLOT_TIMERS, cpu->x), A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    LoadXDirect16(memory, cpu, DP_EVENT_SLOT_RECORD);
    LoadA16(cpu, cpu->y);
    Write16Long(memory, LongIndexedAddress(EVENT_SLOT_POINTERS, cpu->x),
        cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    PushDataBank(memory, cpu);
    LoadA8(cpu, Pull8(memory, cpu));
    Write8(memory, LongIndexedAddress(EVENT_SLOT_POINTERS + 2u, cpu->x),
        A8(cpu));
    return EVENT_OPCODE_YIELD;
}

/* $11: sleep n frames, resume after the operand. */
static unsigned EventOpWait(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2EventNextByte(memory, cpu, 0xd31bu);                       /* D319 */
    return Lufia2EventSleep(memory, cpu);
}

/* $01/$0C: goto when script flag n is set / clear. */
static unsigned EventOpGotoIfFlag(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    TransferDirectToA(cpu);
    Lufia2EventNextByte(memory, cpu, (uint16_t)(handler + 3u));
    Lufia2EventVariable(memory, cpu, (uint16_t)(handler + 6u));
    Lufia2EventFlagBit(memory, cpu, (uint16_t)(handler + 10u));
    And8(cpu, Read8(memory, LongIndexedAddress(EVENT_SCRIPT_FLAGS, cpu->x)));
    if (cpu->zero == (handler == EVENT_OP_GOTO_IF_FLAG))
        Lufia2EventSkipWord(memory, cpu);
    else
        Lufia2EventGoto(memory, cpu);
    return EVENT_OPCODE_NEXT;
}

/* $08/$09: set / clear script flag n. */
static unsigned EventOpChangeFlag(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    uint32_t address;

    TransferDirectToA(cpu);
    Lufia2EventNextByte(memory, cpu, (uint16_t)(handler + 3u));
    Lufia2EventVariable(memory, cpu, (uint16_t)(handler + 6u));
    Lufia2EventFlagBit(memory, cpu, (uint16_t)(handler + 10u));
    address = LongIndexedAddress(EVENT_SCRIPT_FLAGS, cpu->x);
    if (handler == EVENT_OP_SET_FLAG) {
        Or8(cpu, Read8(memory, address));
    } else {
        LoadA8(cpu, (uint8_t)(A8(cpu) ^ 0xffu));
        And8(cpu, Read8(memory, address));
    }
    Write8(memory, address, A8(cpu));
    return EVENT_OPCODE_NEXT;
}

/* $0D/$71: goto when $0692 equals / differs from n. */
static unsigned EventOpGotoIf0692(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    Lufia2EventNextByte(memory, cpu, (uint16_t)(handler + 2u));
    Compare8(cpu, A8(cpu), AbsoluteByte(memory, cpu, WRAM_EVENT_MAP_0692, 0));
    if (cpu->zero == (handler == EVENT_OP_GOTO_IF_0692))
        Lufia2EventGoto(memory, cpu);
    else
        Lufia2EventSkipWord(memory, cpu);
    return EVENT_OPCODE_NEXT;
}

static void EventLoadSlotBits(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(EVENT_SLOT_BITS, cpu->x)));
}

/* $19: goto unless slot bit 7. */
static unsigned EventOp19(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    EventLoadSlotBits(memory, cpu);                            /* E4EB */
    if (cpu->negative)
        Lufia2EventSkipWord(memory, cpu);
    else
        Lufia2EventGoto(memory, cpu);
    return EVENT_OPCODE_NEXT;
}

/* $1E: bit 0 clear -> first target, else $19 on the second. */
static unsigned EventOp1E(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    EventLoadSlotBits(memory, cpu);                            /* E4D8 */
    BitImmediate8(cpu, 0x01u);
    if (cpu->zero) {
        Lufia2EventGoto(memory, cpu);
        return EVENT_OPCODE_NEXT;
    }
    Lufia2EventNextByte(memory, cpu, 0xe4e7u);                       /* E4E5 */
    Lufia2EventNextByte(memory, cpu, 0xe4eau);
    return EventOp19(memory, cpu);
}

/* $2B: skip only when bit 0 is set and bit 7 clear. */
static unsigned EventOp2B(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    EventLoadSlotBits(memory, cpu);                            /* E4F9 */
    BitImmediate8(cpu, 0x01u);
    if (!cpu->zero && !cpu->negative)
        Lufia2EventSkipWord(memory, cpu);
    else
        Lufia2EventGoto(memory, cpu);
    return EVENT_OPCODE_NEXT;
}

/* $1B: write PPU register $2100 + n (DB-relative). */
static unsigned EventOpWritePpu(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    TransferDirectToA(cpu);                                    /* E52A */
    Lufia2EventNextByte(memory, cpu, 0xe52du);
    TransferAToX(cpu);
    Lufia2EventNextByte(memory, cpu, 0xe531u);
    StoreAAbsolute8(memory, cpu, SNES_INIDISP, cpu->x);
    return EVENT_OPCODE_NEXT;
}

/* $1C: start the screen shake with two operand bytes. */
static unsigned EventOpStartShake(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA8(cpu, 0x04u);                                        /* E538 */
    TestBitsAbsolute8(memory, cpu, WRAM_SCREEN_EFFECTS, 1);
    LoadA8(cpu, 0x05u);
    Write8(memory, 0x7fd07eu, A8(cpu));
    Lufia2EventNextByte(memory, cpu, 0xe545u);
    Write8(memory, 0x7fd07fu, A8(cpu));
    Lufia2EventNextByte(memory, cpu, 0xe54cu);
    Write8(memory, 0x7fd080u, A8(cpu));
    return EVENT_OPCODE_NEXT;
}

/* $30-$34: variable n plus, minus or set to a byte; $32/$33 +-1. */
static unsigned EventOpChangeVariable(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    uint32_t variable;

    if (handler == EVENT_OP_INCREMENT_VARIABLE ||
        handler == EVENT_OP_DECREMENT_VARIABLE) {
        TransferDirectToA(cpu);
        Lufia2EventNextByte(memory, cpu, (uint16_t)(handler + 3u));
        Lufia2EventVariable(memory, cpu, (uint16_t)(handler + 6u));
        TransferAToX(cpu);
        variable = LongIndexedAddress(EVENT_VARIABLES, cpu->x);
        LoadA8(cpu, (uint8_t)(Read8(memory, variable) +
            (handler == EVENT_OP_INCREMENT_VARIABLE ? 1u : 0xffu)));
        Write8(memory, variable, A8(cpu));
        return EVENT_OPCODE_NEXT;
    }
    EventVariableOperands(memory, cpu, (uint16_t)(handler + 2u));
    variable = LongIndexedAddress(EVENT_VARIABLES, cpu->x);
    if (handler == EVENT_OP_SUBTRACT_VARIABLE)
        LoadA8(cpu, (uint8_t)((A8(cpu) ^ 0xffu) + 1u));        /* EOR; INC */
    if (handler != EVENT_OP_SET_VARIABLE) {
        cpu->carry = 0;
        Adc8(cpu, Read8(memory, variable));
    }
    Write8(memory, variable, A8(cpu));
    return EVENT_OPCODE_NEXT;
}

/* $2F: variable n = value operand. */
static unsigned EventOpSetVariableValue(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    TransferDirectToA(cpu);                                    /* D924 */
    Lufia2EventNextByte(memory, cpu, 0xd927u);
    Lufia2EventVariable(memory, cpu, 0xd92au);
    TransferAToX(cpu);
    PushIndex(memory, cpu);
    Lufia2EventNextByte(memory, cpu, 0xd92fu);
    Lufia2EventValue(memory, cpu, 0xd932u);
    cpu->x = PullIndexValue(memory, cpu);
    Write8(memory, LongIndexedAddress(EVENT_VARIABLES, cpu->x), A8(cpu));
    return EVENT_OPCODE_NEXT;
}

enum {
    EVENT_GOTO_IF_EQUAL,
    EVENT_GOTO_IF_NOT_EQUAL,
    EVENT_GOTO_IF_CARRY_CLEAR,
    EVENT_GOTO_IF_CARRY_SET
};

/* $35-$40: CMP operand (minus 1) with variable n, goto or skip. */
static const struct {
    uint16_t handler;
    uint8_t value;                  /* operand through $80:E9ED */
    uint8_t decrement;
    uint8_t condition;
} kEventCompares[12] = {
    {EVENT_OP_GOTO_IF_EQUAL, 0, 0, EVENT_GOTO_IF_EQUAL},
    {EVENT_OP_GOTO_IF_NOT_EQUAL, 0, 0, EVENT_GOTO_IF_NOT_EQUAL},
    {EVENT_OP_GOTO_IF_ABOVE, 0, 0, EVENT_GOTO_IF_CARRY_CLEAR},
    {EVENT_OP_GOTO_IF_BELOW, 0, 1, EVENT_GOTO_IF_CARRY_SET},
    {EVENT_OP_GOTO_IF_AT_LEAST, 0, 1, EVENT_GOTO_IF_CARRY_CLEAR},
    {EVENT_OP_GOTO_IF_AT_MOST, 0, 0, EVENT_GOTO_IF_CARRY_SET},
    {EVENT_OP_GOTO_IF_EQUAL_VALUE, 1, 0, EVENT_GOTO_IF_EQUAL},
    {EVENT_OP_GOTO_IF_NOT_EQUAL_VALUE, 1, 0, EVENT_GOTO_IF_NOT_EQUAL},
    {EVENT_OP_GOTO_IF_ABOVE_VALUE, 1, 0, EVENT_GOTO_IF_CARRY_CLEAR},
    {EVENT_OP_GOTO_IF_BELOW_VALUE, 1, 1, EVENT_GOTO_IF_CARRY_SET},
    {EVENT_OP_GOTO_IF_AT_LEAST_VALUE, 1, 1, EVENT_GOTO_IF_CARRY_CLEAR},
    {EVENT_OP_GOTO_IF_AT_MOST_VALUE, 1, 0, EVENT_GOTO_IF_CARRY_SET},
};

static unsigned EventOpCompareVariable(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    unsigned index) {
    const uint16_t handler = kEventCompares[index].handler;
    uint8_t taken;

    EventVariableOperands(memory, cpu, (uint16_t)(handler + 2u));
    if (kEventCompares[index].value)
        Lufia2EventValue(memory, cpu, (uint16_t)(handler + 5u));
    if (kEventCompares[index].decrement)
        DecrementA8(cpu);
    Compare8(cpu, A8(cpu),
        Read8(memory, LongIndexedAddress(EVENT_VARIABLES, cpu->x)));
    switch (kEventCompares[index].condition) {
    case EVENT_GOTO_IF_EQUAL:
        taken = cpu->zero;
        break;
    case EVENT_GOTO_IF_NOT_EQUAL:
        taken = !cpu->zero;
        break;
    case EVENT_GOTO_IF_CARRY_CLEAR:
        taken = !cpu->carry;
        break;
    default:
        taken = cpu->carry;
        break;
    }
    if (taken)
        Lufia2EventGoto(memory, cpu);
    else
        Lufia2EventSkipWord(memory, cpu);
    return EVENT_OPCODE_NEXT;
}

/* $79, $86, $A2, $AB, $B5, $B8: single stores and bit changes. */
static unsigned EventOpSmall(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    switch (handler) {
    case EVENT_OP_STORE_E316:
        Lufia2EventNextByte(memory, cpu, 0xd5a7u);                   /* D5A5 */
        Write8(memory, 0x7fe316u, A8(cpu));
        break;
    case EVENT_OP_86:
        LoadA8(cpu, 0x80u);                                    /* DB11 */
        TestBitsAbsolute8(memory, cpu, WRAM_FIELD_FLAGS, 1);
        LoadA8(cpu, 0x08u);
        TestBitsAbsolute8(memory, cpu, 0x05b3u, 1);
        break;
    case EVENT_OP_A2:
        LoadA8(cpu, 0x20u);                                    /* CFB7 */
        TestBitsAbsolute8(memory, cpu, WRAM_FIELD_FLAGS, 1);
        break;
    case EVENT_OP_RESET_STAIRS:
        LoadA8(cpu, 0xffu);                                    /* D8C8 */
        Write8(memory, 0x7fd0bfu, A8(cpu));
        break;
    case EVENT_OP_RELEASE_CAMERA:
        /* $1261 bit 3: camera from $7F:D08B. */
        LoadA8(cpu, 0x08u);                                    /* DBCA */
        TestBitsAbsolute8(memory, cpu, WRAM_SCREEN_EFFECTS, 0);
        break;
    default:
        LoadA8(cpu, 0x01u);                                    /* D5E4 */
        TestBitsDirect(memory, cpu, 0x72u, 1);
        break;
    }
    return EVENT_OPCODE_NEXT;
}

/* $80:DBD2: widen A to 16 bits; only negative bytes get $FF. */
static void EventSignedByte(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Or8(cpu, 0x00u);                                           /* DBD2 */
    if (cpu->negative) {
        ExchangeAccumulatorBytes(cpu);
        LoadA8(cpu, 0xffu);
        ExchangeAccumulatorBytes(cpu);
    }
    SetAccumulatorWidth(cpu, 0);
    SimulateRtsFrame(memory, cpu);
}

/* $58: move scroll target of layer n by (dx, dy) at two speeds. */
static unsigned EventOpScrollLayer(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    uint32_t target;

    TransferDirectToA(cpu);                                    /* DB21 */
    Lufia2EventNextByte(memory, cpu, 0xdb24u);
    AslA8(cpu);
    TransferAToX(cpu);
    TransferDirectToA(cpu);
    Lufia2EventNextByte(memory, cpu, 0xdb2au);
    EventSignedByte(memory, cpu, 0xdb2du);
    target = LongIndexedAddress(0x7fd0ceu, cpu->x);
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, target));
    Write16Long(memory, target, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    TransferDirectToA(cpu);                                    /* DB39 */
    Lufia2EventNextByte(memory, cpu, 0xdb3cu);
    EventSignedByte(memory, cpu, 0xdb3fu);
    target = LongIndexedAddress(0x7fd0d6u, cpu->x);
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, target));
    Write16Long(memory, target, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    Lufia2EventNextByte(memory, cpu, 0xdb4du);                       /* DB4B */
    Write8(memory, LongIndexedAddress(0x7fd0deu, cpu->x), A8(cpu));
    Lufia2EventNextByte(memory, cpu, 0xdb54u);
    Write8(memory, LongIndexedAddress(0x7fd0e6u, cpu->x), A8(cpu));
    /* TDC: the speed high bytes get the DP low byte. */
    TransferDirectToA(cpu);
    Write8(memory, LongIndexedAddress(0x7fd0dfu, cpu->x), A8(cpu));
    Write8(memory, LongIndexedAddress(0x7fd0e7u, cpu->x), A8(cpu));
    LoadA8(cpu, 0x40u);
    TestBitsAbsolute8(memory, cpu, WRAM_SCREEN_EFFECTS, 1);
    return EVENT_OPCODE_NEXT;
}

/* $29: script flag (slot variable) = slot bit 7 (last condition). */
static unsigned EventOpStoreCondition(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    uint32_t flags;
    uint8_t bit;

    LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);                 /* E421 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(EVENT_SLOT_BITS, cpu->x)));
    Write8(memory, EVENT_CONDITION, A8(cpu));
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd15cu, cpu->x)));
    Lufia2EventFlagBit(memory, cpu, 0xe433u);
    StoreADirect8(memory, cpu, 0x54u);
    LoadA8(cpu, Read8(memory, EVENT_CONDITION));
    flags = LongIndexedAddress(EVENT_SCRIPT_FLAGS, cpu->x);
    bit = DirectByte(memory, cpu, 0x54u);
    if (!cpu->negative) {
        LoadA8(cpu, (uint8_t)(bit ^ 0xffu));
        And8(cpu, Read8(memory, flags));
    } else {
        LoadA8(cpu, bit);                                      /* E44B */
        Or8(cpu, Read8(memory, flags));
    }
    Write8(memory, flags, A8(cpu));
    return EVENT_OPCODE_NEXT;
}

/* $80:D73A: slot X takes the argument list (to $FF) as variables. */
static void EventArguments(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    static const uint32_t kVariables[4] = {
        0x7fd15cu, 0x7fd164u, 0x7fd16cu, 0x7fd174u};
    unsigned i;

    SimulateJsrFrame(memory, cpu, return_address);
    PushIndex(memory, cpu);                                    /* D73A */
    LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);
    for (i = 0; i < 4u; ++i) {
        LoadA8(cpu, Read8(memory, LongIndexedAddress(kVariables[i], cpu->x)));
        Write8(memory, EVENT_SAVED_VARIABLES + i, A8(cpu));
    }
    cpu->x = PullIndexValue(memory, cpu);
    StoreXDirect16(memory, cpu, DP_ACTOR_SLOT);
    SimulateJslFrame(memory, cpu, 0x80u, 0xd763u);
    Lufia2ActorRecordOffsets(memory, cpu);                     /* $83:AB4F */
    SimulateRtlFrame(memory, cpu);
    LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);                 /* D764 */
    LoadA8(cpu, Read8(memory, EVENT_CONDITION));
    Write8(memory, LongIndexedAddress(EVENT_SLOT_BITS, cpu->x), A8(cpu));
    LoadA8(cpu, 0x7fu);
    Write8(memory, LongIndexedAddress(0x7fd154u, cpu->x), A8(cpu));
    for (;;) {
        TransferDirectToA(cpu);                                /* D774 */
        Lufia2EventNextByte(memory, cpu, 0xd777u);
        Compare8(cpu, A8(cpu), 0xffu);
        if (cpu->zero)
            break;
        Compare8(cpu, A8(cpu), 0xfbu);
        if (cpu->carry) {
            /* $FB-$FF: the caller's own variables. */
            PushIndex(memory, cpu);                            /* D780 */
            cpu->carry = 1;
            Sbc8(cpu, 0xfbu);
            TransferAToX(cpu);
            LoadA8(cpu, Read8(memory,
                LongIndexedAddress(EVENT_SAVED_VARIABLES, cpu->x)));
            cpu->x = PullIndexValue(memory, cpu);
        }
        Write8(memory, LongIndexedAddress(0x7fd164u, cpu->x), A8(cpu));
        SetAccumulatorWidth(cpu, 0);                           /* D78E */
        TransferXToA(cpu);
        cpu->carry = 0;
        Add16Value(cpu, 0x0008u);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
    }
    SimulateRtsFrame(memory, cpu);
}

/* Call-frame tag of this slot at its current depth, into $54. */
static void EventCallTag(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    AslA8(cpu);
    AslA8(cpu);
    AslA8(cpu);
    AslA8(cpu);
    Or8(cpu, DirectByte(memory, cpu, DP_ACTOR_SLOT));
    StoreADirect8(memory, cpu, 0x54u);
}

/* Next call frame: X += 10 while below $80. */
static uint8_t EventNextFrame(Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    TransferXToA(cpu);
    cpu->carry = 0;
    Add16Value(cpu, 0x000au);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    Compare16(cpu, cpu->x, 0x0080u);
    return !cpu->carry;
}

/* Copy the four slot variables to (1) or from (0) frame X. */
static void EventFrameVariables(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t save) {
    do {
        const uint32_t variable = DirectLongIndirectY(memory, cpu, 0x5du);
        const uint32_t saved = LongIndexedAddress(0x7fd46au, cpu->x);

        if (save) {
            LoadA8(cpu, Read8(memory, variable));              /* D7F6 */
            Write8(memory, saved, A8(cpu));
        } else {
            LoadA8(cpu, Read8(memory, saved));                 /* D8AE */
            Write8(memory, variable, A8(cpu));
        }
        IncrementX16(cpu);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, cpu->y);
        cpu->carry = 0;
        Add16Value(cpu, 0x0008u);
        TransferAToY(cpu);
        Compare16(cpu, cpu->accumulator, 0x0020u);
        SetAccumulatorWidth(cpu, 1);
    } while (!cpu->carry);
}

/* $5D/$5F: long pointer to this slot's variables. */
static void EventSlotVariablePointer(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA8(cpu, 0x7fu);
    StoreADirect8(memory, cpu, 0x5fu);
    SetAccumulatorWidth(cpu, 0);
    LoadADirect16(memory, cpu, DP_ACTOR_SLOT);
    cpu->carry = 0;
    Add16Value(cpu, 0xd15cu);
    StoreADirect16(memory, cpu, 0x5du);
}

/* $A9: call base + word with arguments; saves the caller's state. */
static unsigned EventOpCall(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    uint32_t depth;

    LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);                 /* D79B */
    depth = LongIndexedAddress(EVENT_CALL_DEPTH, cpu->x);
    LoadA8(cpu, (uint8_t)(Read8(memory, depth) + 1u));
    Write8(memory, depth, A8(cpu));
    EventCallTag(memory, cpu);
    LoadX16(cpu, 0x0000u);
    for (;;) {
        LoadA8(cpu, Read8(memory, LongIndexedAddress(EVENT_CALL_FRAMES, cpu->x)));
        if (cpu->zero)
            break;
        if (!EventNextFrame(cpu)) {
            /* No free frame: frame 0 is overwritten. */
            TransferDirectToA(cpu);                            /* D7C6 */
            LoadX16(cpu, 0x0000u);
            break;
        }
    }
    LoadA8(cpu, DirectByte(memory, cpu, 0x54u));               /* D7CA */
    Write8(memory, LongIndexedAddress(EVENT_CALL_FRAMES, cpu->x), A8(cpu));
    LoadA8(cpu, Read8(memory, EVENT_CONDITION));
    Write8(memory, LongIndexedAddress(0x7fd46fu, cpu->x), A8(cpu));
    LoadA8(cpu, Read8(memory, EVENT_CONDITION + 1u));
    Write8(memory, LongIndexedAddress(0x7fd46eu, cpu->x), A8(cpu));
    StoreXDirect16(memory, cpu, 0x56u);
    EventSlotVariablePointer(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    PushY(memory, cpu);                                        /* D7F2 */
    LoadY16(cpu, 0x0000u);
    EventFrameVariables(memory, cpu, 1);
    cpu->y = PullIndexValue(memory, cpu);
    LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);                 /* D80D */
    EventArguments(memory, cpu, 0xd811u);
    Lufia2EventNextWord(memory, cpu, 0xd814u);
    PushAccumulator16(memory, cpu);                            /* M=0 */
    LoadXDirect16(memory, cpu, 0x56u);
    LoadA16(cpu, cpu->y);
    Write16Long(memory, LongIndexedAddress(0x7fd467u, cpu->x),
        cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    PushDataBank(memory, cpu);
    LoadA8(cpu, Pull8(memory, cpu));
    Write8(memory, LongIndexedAddress(0x7fd469u, cpu->x), A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    PullAccumulator16(memory, cpu);
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, EVENT_SCRIPT_BASE));
    Lufia2EventSetPointer(memory, cpu, 0xd82fu);
    SetAccumulatorWidth(cpu, 1);
    return EVENT_OPCODE_NEXT;
}

/* $AA: return to the caller's frame and variables. */
static unsigned EventOpReturn(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    uint32_t depth;

    LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);                 /* D849 */
    depth = LongIndexedAddress(EVENT_CALL_DEPTH, cpu->x);
    LoadA8(cpu, Read8(memory, depth));
    EventCallTag(memory, cpu);
    LoadA8(cpu, (uint8_t)(Read8(memory, depth) - 1u));
    Write8(memory, depth, A8(cpu));
    LoadX16(cpu, 0x0000u);
    for (;;) {
        /* Without a match X ends at $80, past the frames. */
        LoadA8(cpu, Read8(memory,
            LongIndexedAddress(EVENT_CALL_FRAMES, cpu->x)));   /* D863 */
        Compare8(cpu, A8(cpu), DirectByte(memory, cpu, 0x54u));
        if (cpu->zero || !EventNextFrame(cpu))
            break;
    }
    TransferDirectToA(cpu);                                    /* D87A */
    Write8(memory, LongIndexedAddress(EVENT_CALL_FRAMES, cpu->x), A8(cpu));
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd46fu, cpu->x)));
    Write8(memory, EVENT_CONDITION, A8(cpu));
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd46eu, cpu->x)));
    Write8(memory, EVENT_CONDITION + 1u, A8(cpu));
    EventSlotVariablePointer(memory, cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fd467u, cpu->x)));
    TransferAToY(cpu);
    SetAccumulatorWidth(cpu, 1);
    PushY(memory, cpu);                                        /* D8A4 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd469u, cpu->x)));
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    LoadY16(cpu, 0x0000u);
    EventFrameVariables(memory, cpu, 0);
    cpu->y = PullIndexValue(memory, cpu);
    return EVENT_OPCODE_NEXT;
}

/* $B4: camera target = scroll of layer $05AA + (dx, dy), speed n;
   $1261 bit 3 hands the camera to $7F:D08B/D08D. */
static unsigned EventOpMoveCamera(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const struct {
        uint16_t scroll;
        uint32_t target;
        uint16_t fetch;
        uint16_t widen;
    } kAxes[2] = {
        {0x121eu, 0x7fd08bu, 0xdb70u, 0xdb73u},
        {0x1226u, 0x7fd08du, 0xdb81u, 0xdb84u},
    };
    unsigned i;

    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x05aau, 0)); /* DB6A */
    for (i = 0; i < 2u; ++i) {
        TransferDirectToA(cpu);
        Lufia2EventNextByte(memory, cpu, kAxes[i].fetch);
        EventSignedByte(memory, cpu, kAxes[i].widen);
        cpu->carry = 0;
        Add16Value(cpu, Read16AbsoluteIndexed(
            memory, cpu, kAxes[i].scroll, cpu->x));
        Write16Long(memory, kAxes[i].target, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
    }
    Lufia2EventNextByte(memory, cpu, 0xdb91u);                 /* DB8F */
    StoreAAbsolute8(memory, cpu, 0x05a8u, 0);
    LoadA8(cpu, 0x08u);
    TestBitsAbsolute8(memory, cpu, WRAM_SCREEN_EFFECTS, 1);
    return EVENT_OPCODE_NEXT;
}

/* Passes through $80:CC3F per nesting level: $26/$27 run a second
   slot inside the first, and the verifier counts visits per stack
   depth. */
#define EVENT_NEST_LIMIT 8u

typedef struct EventRun {
    unsigned total;
    unsigned depth;
    unsigned passes[EVENT_NEST_LIMIT];
} EventRun;

static uint8_t EventRunScript(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    EventRun *run);

/* $27: fork a free slot ($80:E99D, slot 0 when none) with arguments
   and a target, run it until it yields, then go on; $26 only forks
   when slot bit 0 is set and else skips the arguments and target. */
static unsigned EventOpFork(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler,
    EventRun *run,
    uint32_t *handoff) {
    if (handler == EVENT_OP_FORK_IF) {
        unsigned skipped = 0;

        LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);             /* D6CA */
        TransferDirectToA(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(EVENT_SLOT_BITS, cpu->x)));
        Write8(memory, EVENT_CONDITION, A8(cpu));
        BitImmediate8(cpu, 0x01u);
        if (cpu->zero) {
            do {
                /* No $FF in any bank loops forever. */
                if (skipped++ >= 0x01000000u) {
                    *handoff = 0x80d6d9u;
                    return EVENT_OPCODE_HANDOFF;
                }
                Lufia2EventNextByte(memory, cpu, 0xd6dbu);     /* D6D9 */
                Compare8(cpu, A8(cpu), 0xffu);
            } while (!cpu->zero);
            Lufia2EventNextByte(memory, cpu, 0xd6e2u);         /* D6E0 */
            Lufia2EventNextByte(memory, cpu, 0xd6e5u);
            return EVENT_OPCODE_NEXT;
        }
    }
    LoadA8(cpu, DirectByte(memory, cpu, DP_ACTOR_SLOT));       /* D6E9 */
    PushAccumulator8(memory, cpu);
    LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(EVENT_SLOT_BITS, cpu->x)));
    Write8(memory, EVENT_CONDITION, A8(cpu));
    SimulateJsrFrame(memory, cpu, 0xd6f8u);
    LoadX16(cpu, 0x0000u);                                     /* E99D */
    for (;;) {
        LoadA8(cpu, Read8(memory, LongIndexedAddress(EVENT_SLOT_TIMERS, cpu->x)));
        if (!cpu->negative)
            break;
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, EVENT_SLOT_COUNT);
        if (cpu->zero) {
            LoadX16(cpu, 0x0000u);
            break;
        }
    }
    SimulateRtsFrame(memory, cpu);
    EventArguments(memory, cpu, 0xd6fbu);
    LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);                 /* D6FC */
    LoadA8(cpu, 0x80u);
    Write8(memory, LongIndexedAddress(EVENT_SLOT_TIMERS, cpu->x), A8(cpu));
    Lufia2EventNextWord(memory, cpu, 0xd706u);
    PushDataBank(memory, cpu);
    PushY(memory, cpu);
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, EVENT_SCRIPT_BASE));
    Lufia2EventSetPointer(memory, cpu, 0xd710u);
    LoadXDirect16(memory, cpu, DP_EVENT_SLOT_RECORD);
    LoadA16(cpu, cpu->y);
    Write16Long(memory, LongIndexedAddress(EVENT_SLOT_POINTERS, cpu->x),
        cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    PushDataBank(memory, cpu);
    LoadA8(cpu, Pull8(memory, cpu));
    Write8(memory, LongIndexedAddress(EVENT_SLOT_POINTERS + 2u, cpu->x),
        A8(cpu));
    if (run->depth + 1u >= EVENT_NEST_LIMIT) {
        *handoff = 0x80d720u;
        return EVENT_OPCODE_HANDOFF;
    }
    SimulateJsrFrame(memory, cpu, 0xd722u);                    /* D720 */
    ++run->depth;
    if (!EventRunScript(memory, cpu, run)) {
        *handoff = cpu->resume_pc;
        return EVENT_OPCODE_HANDOFF;
    }
    --run->depth;
    SimulateRtsFrame(memory, cpu);
    LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);                 /* D723 */
    LoadA8(cpu, (uint8_t)(Read8(memory,
        LongIndexedAddress(EVENT_SLOT_TIMERS, cpu->x)) + 1u));
    Write8(memory, LongIndexedAddress(EVENT_SLOT_TIMERS, cpu->x), A8(cpu));
    cpu->y = PullIndexValue(memory, cpu);
    PullDataBank(memory, cpu);
    LoadA8(cpu, Pull8(memory, cpu));
    StoreADirect8(memory, cpu, DP_ACTOR_SLOT);
    SimulateJslFrame(memory, cpu, 0x80u, 0xd736u);
    Lufia2ActorRecordOffsets(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    return EVENT_OPCODE_NEXT;
}

/* $1D ($80:B97E): clear $1261 (DB-relative) and $7F:D081/D083. */
static unsigned EventOpClearD081(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJslFrame(memory, cpu, 0x80u, 0xe557u);
    Write8(memory, AbsoluteIndexedAddress(cpu, 0x1261u, 0), 0x00u);  /* B97E */
    SetAccumulatorWidth(cpu, 0);
    TransferDirectToA(cpu);
    Write16Long(memory, 0x7fd081u, cpu->accumulator);
    Write16Long(memory, 0x7fd083u, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    SimulateRtlFrame(memory, cpu);
    return EVENT_OPCODE_NEXT;
}

/* $80:E722: start list entry Y/2 at [base + X] in the first free
   slot (slot 0 when none), due, at $8F/$91. 0 = handoff. */
static uint8_t EventStart(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x80u, return_address);
    LoadA8(cpu, Read8(memory, EVENT_SCRIPT_BASE_BANK));        /* E722 */
    Compare8(cpu, A8(cpu), 0xffu);
    if (cpu->zero) {
        SimulateRtlFrame(memory, cpu);
        return 1;
    }
    Write16Direct(memory, cpu, 0x5du, cpu->x);
    if (!Lufia2EventFindList(memory, cpu, 0xe730u))
        return 0;
    if (!cpu->carry) {
        for (LoadX16(cpu, 0x0000u);;) {                        /* E733 */
            LoadA8(cpu, Read8(memory, LongIndexedAddress(EVENT_SLOT_TIMERS, cpu->x)));
            if (!cpu->negative)
                break;
            IncrementX16(cpu);
            Compare16(cpu, cpu->x, EVENT_SLOT_COUNT);
            if (cpu->zero) {
                LoadX16(cpu, 0x0000u);
                break;
            }
        }
        LoadA8(cpu, 0x81u);                                    /* E745 */
        Write8(memory, LongIndexedAddress(EVENT_SLOT_TIMERS, cpu->x), A8(cpu));
        TransferDirectToA(cpu);
        Write8(memory, LongIndexedAddress(0x7fd4e6u, cpu->x), A8(cpu));
        Write16Direct(memory, cpu, 0x54u, cpu->x);
        LoadA8(cpu, DirectByte(memory, cpu, 0x5du));
        Write8(memory, LongIndexedAddress(0x7fd154u, cpu->x), A8(cpu));
        LoadA8(cpu, DirectByte(memory, cpu, 0x56u));
        Write8(memory, LongIndexedAddress(0x7fd15cu, cpu->x), A8(cpu));
        TransferDirectToA(cpu);
        Write8(memory, LongIndexedAddress(EVENT_SLOT_BITS, cpu->x), A8(cpu));
        SetAccumulatorWidth(cpu, 0);                           /* E763 */
        LoadA16(cpu, cpu->x);
        AslA16(cpu);
        Add16Value(cpu, Read16Direct(memory, cpu, 0x54u));
        TransferAToX(cpu);
        LoadA16(cpu, Read16Long(memory, EVENT_SCRIPT_POINTER));
        Write16Long(memory, LongIndexedAddress(EVENT_SLOT_POINTERS, cpu->x),
            cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, Read8(memory, EVENT_SCRIPT_BANK));
        Write8(memory, LongIndexedAddress(EVENT_SLOT_POINTERS + 2u, cpu->x),
            A8(cpu));
        LoadXDirect16(memory, cpu, 0x54u);
        LoadA8(cpu, DirectByte(memory, cpu, 0x8fu));
        Write8(memory, LongIndexedAddress(0x7fd17cu, cpu->x), A8(cpu));
        LoadA8(cpu, DirectByte(memory, cpu, 0x91u));
        Write8(memory, LongIndexedAddress(0x7fd184u, cpu->x), A8(cpu));
    }
    SetAccumulatorWidth(cpu, 1);                               /* E78A */
    SimulateRtlFrame(memory, cpu);
    return 1;
}

/* $63: start event 0 of the header list, then end the slot. */
static unsigned EventOpStartEventAndEnd(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t *handoff) {
    PushY(memory, cpu);                                        /* DDE3 */
    LoadX16(cpu, 0x0000u);
    LoadY16(cpu, 0x0000u);
    if (!EventStart(memory, cpu, 0xddedu)) {
        *handoff = cpu->resume_pc;
        return EVENT_OPCODE_HANDOFF;
    }
    cpu->y = PullIndexValue(memory, cpu);
    return EventOpEnd(memory, cpu);
}

/* Handlers behind JMP ($E5A4,x); others hand off. */
static unsigned EventScriptOpcode(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler,
    EventRun *run,
    uint32_t *handoff) {
    unsigned i;

    if (handler == EVENT_OP_FORK || handler == EVENT_OP_FORK_IF)
        return EventOpFork(memory, cpu, handler, run, handoff);
    if (handler == EVENT_OP_CLEAR_D081)
        return EventOpClearD081(memory, cpu);
    if (handler == EVENT_OP_START_EVENT_AND_END)
        return EventOpStartEventAndEnd(memory, cpu, handoff);
    for (i = 0; i < 12u; ++i)
        if (handler == kEventCompares[i].handler)
            return EventOpCompareVariable(memory, cpu, i);
    switch (handler) {
    case EVENT_OP_END:
        return EventOpEnd(memory, cpu);
    case EVENT_OP_WAIT:
        return EventOpWait(memory, cpu);
    case EVENT_OP_GOTO:
        Lufia2EventGoto(memory, cpu);
        return EVENT_OPCODE_NEXT;
    case EVENT_OP_GOTO_IF_FLAG:
    case EVENT_OP_GOTO_IF_NOT_FLAG:
        return EventOpGotoIfFlag(memory, cpu, handler);
    case EVENT_OP_SET_FLAG:
    case EVENT_OP_CLEAR_FLAG:
        return EventOpChangeFlag(memory, cpu, handler);
    case EVENT_OP_GOTO_IF_0692:
    case EVENT_OP_GOTO_UNLESS_0692:
        return EventOpGotoIf0692(memory, cpu, handler);
    case EVENT_OP_19:
        return EventOp19(memory, cpu);
    case EVENT_OP_1E:
        return EventOp1E(memory, cpu);
    case EVENT_OP_2B:
        return EventOp2B(memory, cpu);
    case EVENT_OP_WRITE_PPU:
        return EventOpWritePpu(memory, cpu);
    case EVENT_OP_START_SHAKE:
        return EventOpStartShake(memory, cpu);
    case EVENT_OP_NOP:
        return EVENT_OPCODE_NEXT;
    case EVENT_OP_ADD_VARIABLE:
    case EVENT_OP_SUBTRACT_VARIABLE:
    case EVENT_OP_INCREMENT_VARIABLE:
    case EVENT_OP_DECREMENT_VARIABLE:
    case EVENT_OP_SET_VARIABLE:
        return EventOpChangeVariable(memory, cpu, handler);
    case EVENT_OP_SET_VARIABLE_VALUE:
        return EventOpSetVariableValue(memory, cpu);
    case EVENT_OP_SCROLL_LAYER:
        return EventOpScrollLayer(memory, cpu);
    case EVENT_OP_MOVE_CAMERA:
        return EventOpMoveCamera(memory, cpu);
    case EVENT_OP_STORE_CONDITION:
        return EventOpStoreCondition(memory, cpu);
    case EVENT_OP_CALL:
        return EventOpCall(memory, cpu);
    case EVENT_OP_RETURN:
        return EventOpReturn(memory, cpu);
    case EVENT_OP_STORE_E316:
    case EVENT_OP_86:
    case EVENT_OP_A2:
    case EVENT_OP_RESET_STAIRS:
    case EVENT_OP_RELEASE_CAMERA:
    case EVENT_OP_B8:
        return EventOpSmall(memory, cpu, handler);
    default:
        return Lufia2EventActorOpcode(memory, cpu, handler, handoff);
    }
}

/* $80:CC35: run the script until it yields; 0 = handoff at CC3F. */
static uint8_t EventRunScript(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    EventRun *run) {
    for (;;) {
        uint16_t handler;
        uint32_t handoff = 0x80cc3fu;

        Lufia2EventNextByte(memory, cpu, 0xcc37u);                   /* CC35 */
        AslA8(cpu);
        ExchangeAccumulatorBytes(cpu);
        LoadA8(cpu, 0x00u);
        RolA8(cpu);
        ExchangeAccumulatorBytes(cpu);
        TransferAToX(cpu);
        handler = Read16Bank(memory, 0x80u, (uint16_t)(0xe5a4u + cpu->x));
        switch (EventScriptOpcode(memory, cpu,
                    run->total < EVENT_OPCODE_LIMIT ? handler : 0u, run,
                    &handoff)) {
        case EVENT_OPCODE_NEXT:
            ++run->total;
            ++run->passes[run->depth];
            continue;
        case EVENT_OPCODE_YIELD:
            ++run->total;
            ++run->passes[run->depth];
            return 1;
        default:                                               /* CC3F */
            cpu->resume_pc = handoff;
            return 0;
        }
    }
}

/* $80:CBCC: resume the due slot X through the script VM. */
static uint8_t EventResumeSlot(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    EventRun *run) {
    PushIndex(memory, cpu);                                    /* CBCC */
    StoreXDirect16(memory, cpu, DP_ACTOR_SLOT);
    SimulateJslFrame(memory, cpu, 0x80u, 0xcbd2u);
    Lufia2ActorRecordOffsets(memory, cpu);                     /* $83:AB4F */
    SimulateRtlFrame(memory, cpu);
    SetAccumulatorWidth(cpu, 0);                               /* CBD3 */
    LoadXDirect16(memory, cpu, DP_EVENT_SLOT_RECORD);
    LoadA16(cpu, Read16Long(memory,
        LongIndexedAddress(EVENT_SLOT_POINTERS, cpu->x)));
    Write16Long(memory, EVENT_SCRIPT_POINTER, cpu->accumulator);
    TransferAToY(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory,
        LongIndexedAddress(EVENT_SLOT_POINTERS + 2u, cpu->x)));
    Write8(memory, EVENT_SCRIPT_BANK, A8(cpu));
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SimulateJsrFrame(memory, cpu, 0xcbeeu);                    /* CBEC */
    if (!EventRunScript(memory, cpu, run))
        return 0;
    SimulateRtsFrame(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);                      /* CBEF */
    return 1;
}

/* $80:CBAE: count down the slot timers; 0 = handoff. */
uint8_t Lufia2FieldEventTimerBody(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    unsigned *passes) {
    EventRun run = {0, 0, {0}};

    *passes = 0;
    cpu->program_bank = 0x80u;
    PushDataBank(memory, cpu);                                 /* CBAE */
    Push8(memory, cpu, PackStatus(cpu));
    SetIndexWidth(cpu, 0);
    Write8(memory, AbsoluteIndexedAddress(cpu, WRAM_EVENT_REDRAW, 0), 0x00u);
    LoadA8(cpu, FIELD_FLAG_EVENT_ARMED);
    TestBitsAbsolute8(memory, cpu, WRAM_FIELD_FLAGS, 0);
    LoadX16(cpu, 0x0000u);
    do {
        const uint32_t timer = LongIndexedAddress(EVENT_SLOT_TIMERS, cpu->x);

        LoadA8(cpu, Read8(memory, timer));                     /* CBBD */
        if (cpu->negative) {
            /* An armed $80 wraps to $7F and disarms. */
            DecrementA8(cpu);
            Write8(memory, timer, A8(cpu));
            And8(cpu, 0x7fu);
            if (cpu->zero) {
                /* The VM's ADCs are binary only. */
                if (cpu->decimal) {
                    cpu->resume_pc = 0x80cbccu;
                    return 0;
                }
                if (!EventResumeSlot(memory, cpu, &run)) {
                    *passes = run.passes[run.depth];
                    return 0;
                }
            }
        }
        IncrementX16(cpu);                                     /* CBF0 */
        Compare16(cpu, cpu->x, EVENT_SLOT_COUNT);
    } while (!cpu->carry);
    LoadX16(cpu, 0x0007u);                                     /* CBF6 */
    for (;;) {
        LoadA8(cpu, Read8(memory,
            LongIndexedAddress(EVENT_SLOT_TIMERS, cpu->x)));
        if (cpu->negative) {
            LoadA8(cpu, FIELD_FLAG_EVENT_ARMED);               /* CBFF */
            TestBitsAbsolute8(memory, cpu, WRAM_FIELD_FLAGS, 1);
            break;
        }
        LoadX16(cpu, (uint16_t)(cpu->x - 1u));                 /* CC06 */
        if (cpu->negative)
            break;
    }
    /* Read with the last script bank as DB. */
    LoadAAbsolute8(memory, cpu, WRAM_EVENT_REDRAW, 0);         /* CC09 */
    if (!cpu->zero) {
        cpu->resume_pc = 0x80cc0eu;
        return 0;
    }
    UnpackStatus(cpu, Pull8(memory, cpu));                     /* CC23 */
    PullDataBank(memory, cpu);
    return 1;
}

/* $80:CBAE from a JSL; M=0 decodes differently, LLE. */
Lufia2ExecutionResult Lufia2FieldEventTimerTick(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;
    unsigned passes;

    result.flow = LUFIA2_EXECUTION_RETURNED;
    result.pc = 0x80cc25u;
    result.dispatches = 0;
    if (!cpu->accumulator_is_8_bit) {
        result.flow = LUFIA2_EXECUTION_BOUNDARY;
        result.pc = cpu->resume_pc = 0x80cbaeu;
        return result;
    }
    if (!Lufia2FieldEventTimerBody(memory, cpu, &passes)) {
        result.flow = LUFIA2_EXECUTION_BOUNDARY;
        result.pc = cpu->resume_pc;
        if (result.pc == 0x80cc3fu)
            result.dispatches = passes;
    }
    return result;
}
