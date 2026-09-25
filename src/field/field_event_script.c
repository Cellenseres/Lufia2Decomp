/* Field event script slots, their wait timers and the script VM. */

#include "core/cpu_internal.h"
#include "lufia2/field.h"
#include "actor/actor_internal.h"
#include "field/field_internal.h"
#include "system/wram.h"

/* Eight slots; bit 7 armed, bits 0-6 frames left. */
#define EVENT_SLOT_TIMERS 0x7fd18cu
#define EVENT_SLOT_COUNT 0x0008u
/* Resume point of each slot, 3 bytes per slot. */
#define EVENT_SLOT_POINTERS 0x7fd134u
/* Per-slot bits; $19/$1E/$2B test bits 7 and 0. */
#define EVENT_SLOT_BITS 0x7fd14cu
/* Script variables; operands $A0-$BF also read them. */
#define EVENT_VARIABLES 0x7fd074u
/* 64 script points; $E0-$FF operands name them. */
#define EVENT_POINT_X 0x7fd1a3u
#define EVENT_POINT_Y 0x7fd1e3u
#define EVENT_POINT_D223 0x7fd223u
#define EVENT_POINT_D263 0x7fd263u
/* Script flags, bit n & 7 of byte n >> 3. */
#define EVENT_SCRIPT_FLAGS 0x7fd100u
/* Goto targets are relative to this 24-bit base. */
#define EVENT_SCRIPT_BASE 0x7fd194u
#define EVENT_SCRIPT_BASE_BANK 0x7fd196u
/* Script start and bank of the running slot. */
#define EVENT_SCRIPT_POINTER 0x7fd197u
#define EVENT_SCRIPT_BANK 0x7fd199u
/* Layer redraw requests from the scripts, become $74 bits. */
#define WRAM_EVENT_REDRAW 0x1273u
#define WRAM_EVENT_MAP_0692 0x0692u
/* $05B5 bit 1: a slot is still armed. */
#define FIELD_FLAG_EVENT_ARMED 0x02u
#define DP_EVENT_SLOT_RECORD 0xabu
/* Opcodes per tick before an exact handoff at $80:CC3F. */
#define EVENT_OPCODE_LIMIT 4096u

/* $80:E8B9: next script byte; a wrapping Y steps to the next bank. */
static void EventNextByte(
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

/* $80:E8D0: step back one byte; below $8000 the bank steps down. */
static void EventPrevByte(
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
static void EventNextWord(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    EventNextByte(memory, cpu, 0xe8afu);                       /* E8AD */
    PushAccumulator8(memory, cpu);
    EventNextByte(memory, cpu, 0xe8b3u);
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Pull8(memory, cpu));
    SetAccumulatorWidth(cpu, 0);
    SimulateRtsFrame(memory, cpu);
}

/* $80:E8F4: Y = A in the base bank; below $8000 steps a bank. */
static void EventSetPointer(
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
static void EventGoto(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    EventNextWord(memory, cpu, 0xd2a4u);                       /* D2A2 */
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, EVENT_SCRIPT_BASE));
    EventSetPointer(memory, cpu, 0xd2acu);
    SetAccumulatorWidth(cpu, 1);
}

/* $80:D2B2: skip an untaken goto target. */
static void EventSkipWord(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    EventNextByte(memory, cpu, 0xd2b4u);                       /* D2B2 */
    EventNextByte(memory, cpu, 0xd2b7u);
}

/* $80:E9BC: $FB-$FF read slot variables ($C0-$DF as 0-$1F). */
static void EventVariable(
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
static void EventValue(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Compare8(cpu, A8(cpu), 0xfbu);                             /* E9ED */
    if (!cpu->zero) {
        EventVariable(memory, cpu, 0xe9f3u);
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
    EventNextByte(memory, cpu, 0xd9f3u);
    EventVariable(memory, cpu, 0xd9f6u);
    TransferAToX(cpu);
    EventNextByte(memory, cpu, 0xd9fau);
    SimulateRtsFrame(memory, cpu);
}

/* $80:E898: X = flag byte of n, A = its bit from $80:BE45. */
static void EventFlagBit(
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

/* What the VM does after an opcode. */
enum {
    EVENT_OPCODE_NEXT,
    EVENT_OPCODE_YIELD,
    EVENT_OPCODE_HANDOFF
};

/* Handlers in the $80:E5A4 opcode table. */
enum EventOpcodeHandler {
    EVENT_OP_END = 0xcc42,         /* $00 $07 $2C-$2E $56 $62 $93 $9B $AC $AD */
    EVENT_OP_GOTO_IF_FLAG = 0xcc4a,                            /* $01 */
    EVENT_OP_GOTO_IF_NOT_FLAG = 0xcc61,                        /* $0C */
    EVENT_OP_SET_FLAG = 0xd274,                                /* $08 */
    EVENT_OP_CLEAR_FLAG = 0xd28a,                              /* $09 */
    EVENT_OP_GOTO = 0xd2a2,                                    /* $0A */
    EVENT_OP_GOTO_IF_0692 = 0xd2c5,                            /* $0D */
    EVENT_OP_GOTO_UNLESS_0692 = 0xd2d3,                        /* $71 */
    EVENT_OP_WAIT = 0xd319,                                    /* $11 */
    EVENT_OP_19 = 0xe4eb,                                      /* $19 */
    EVENT_OP_1E = 0xe4d8,                                      /* $1E */
    EVENT_OP_2B = 0xe4f9,                                      /* $2B */
    EVENT_OP_WRITE_PPU = 0xe52a,                               /* $1B */
    EVENT_OP_START_SHAKE = 0xe538,                             /* $1C */
    EVENT_OP_NOP = 0xdb1e,                                     /* $57 */
    EVENT_OP_SET_VARIABLE_VALUE = 0xd924,                      /* $2F */
    EVENT_OP_ADD_VARIABLE = 0xd8d1,                            /* $30 */
    EVENT_OP_SUBTRACT_VARIABLE = 0xd8e0,                       /* $31 */
    EVENT_OP_INCREMENT_VARIABLE = 0xd8f2,                      /* $32 */
    EVENT_OP_DECREMENT_VARIABLE = 0xd906,                      /* $33 */
    EVENT_OP_SET_VARIABLE = 0xd91a,                            /* $34 */
    EVENT_OP_GOTO_IF_EQUAL = 0xd93b,                           /* $35 */
    EVENT_OP_GOTO_IF_NOT_EQUAL = 0xd94a,                       /* $36 */
    EVENT_OP_GOTO_IF_ABOVE = 0xd959,                           /* $37 */
    EVENT_OP_GOTO_IF_BELOW = 0xd968,                           /* $38 */
    EVENT_OP_GOTO_IF_AT_LEAST = 0xd975,                        /* $39 */
    EVENT_OP_GOTO_IF_AT_MOST = 0xd982,                         /* $3A */
    EVENT_OP_GOTO_IF_EQUAL_VALUE = 0xd98e,                     /* $3B */
    EVENT_OP_GOTO_IF_NOT_EQUAL_VALUE = 0xd99d,                 /* $3C */
    EVENT_OP_GOTO_IF_ABOVE_VALUE = 0xd9ac,                     /* $3D */
    EVENT_OP_GOTO_IF_BELOW_VALUE = 0xd9bb,                     /* $3E */
    EVENT_OP_GOTO_IF_AT_LEAST_VALUE = 0xd9cb,                  /* $3F */
    EVENT_OP_GOTO_IF_AT_MOST_VALUE = 0xd9db,                   /* $40 */
    EVENT_OP_STORE_E316 = 0xd5a5,                              /* $79 */
    EVENT_OP_OFFSET_POINT_X = 0xdac9,                          /* $83 */
    EVENT_OP_OFFSET_POINT_Y = 0xdad7,                          /* $84 */
    EVENT_OP_86 = 0xdb11,                                      /* $86 */
    EVENT_OP_POINT_X_TO_VARIABLE = 0xcfd8,                     /* $9D */
    EVENT_OP_VARIABLE_TO_POINT_X = 0xcfea,                     /* $9E */
    EVENT_OP_A2 = 0xcfb7,                                      /* $A2 */
    EVENT_OP_RESET_STAIRS = 0xd8c8,                            /* $AB */
    EVENT_OP_B5 = 0xdbca,                                      /* $B5 */
    EVENT_OP_B8 = 0xd5e4,                                      /* $B8 */
    EVENT_OP_WAIT_FOR_LISTED_ACTOR = 0xdd8f,                   /* $5F */
    EVENT_OP_WAIT_FOR_ACTOR = 0xdd86,                          /* $68 */
    EVENT_OP_WAIT_FOR_LEADER = 0xd4ca,                         /* $6B */
    EVENT_OP_SCROLL_LAYER = 0xdb21                             /* $58 */
};

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
static unsigned EventSleep(
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
    EventNextByte(memory, cpu, 0xd31bu);                       /* D319 */
    return EventSleep(memory, cpu);
}

/* $80:DD9B: actor X still moving (bit 7, not bit 2)? */
static uint8_t EventActorBusy(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    TransferAToX(cpu);                                         /* DD9B */
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->x);
    BitImmediate8(cpu, 0x04u);
    if (!cpu->zero)
        return 0;
    BitImmediate8(cpu, 0x80u);
    return !cpu->zero;
}

/* $5F/$68/$6B: wait one frame at a time while the actor moves. */
static unsigned EventOpWaitForActor(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    if (handler == EVENT_OP_WAIT_FOR_LEADER) {
        LoadAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, 0);      /* D4CA */
        BitImmediate8(cpu, 0x04u);
        if (!cpu->zero)
            return EVENT_OPCODE_NEXT;
        BitImmediate8(cpu, 0x80u);
        if (cpu->zero)
            return EVENT_OPCODE_NEXT;
        EventPrevByte(memory, cpu, 0xd4d7u);
    } else {
        if (handler == EVENT_OP_WAIT_FOR_ACTOR)
            TransferDirectToA(cpu);                            /* DD86 */
        EventNextByte(memory, cpu, (uint16_t)(handler +
            (handler == EVENT_OP_WAIT_FOR_ACTOR ? 3u : 2u)));
        EventValue(memory, cpu, (uint16_t)(handler +
            (handler == EVENT_OP_WAIT_FOR_ACTOR ? 6u : 5u)));
        if (handler == EVENT_OP_WAIT_FOR_LISTED_ACTOR) {
            /* $7F:D72C maps the operand to an actor slot. */
            TransferAToX(cpu);                                 /* DD95 */
            TransferDirectToA(cpu);
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd72cu, cpu->x)));
        }
        if (!EventActorBusy(memory, cpu))
            return EVENT_OPCODE_NEXT;
        EventPrevByte(memory, cpu, 0xdda9u);                   /* DDA7 */
        EventPrevByte(memory, cpu, 0xddacu);
    }
    LoadA8(cpu, 0x01u);
    return EventSleep(memory, cpu);
}

/* $01/$0C: goto when script flag n is set / clear. */
static unsigned EventOpGotoIfFlag(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    TransferDirectToA(cpu);
    EventNextByte(memory, cpu, (uint16_t)(handler + 3u));
    EventVariable(memory, cpu, (uint16_t)(handler + 6u));
    EventFlagBit(memory, cpu, (uint16_t)(handler + 10u));
    And8(cpu, Read8(memory, LongIndexedAddress(EVENT_SCRIPT_FLAGS, cpu->x)));
    if (cpu->zero == (handler == EVENT_OP_GOTO_IF_FLAG))
        EventSkipWord(memory, cpu);
    else
        EventGoto(memory, cpu);
    return EVENT_OPCODE_NEXT;
}

/* $08/$09: set / clear script flag n. */
static unsigned EventOpChangeFlag(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    uint32_t address;

    TransferDirectToA(cpu);
    EventNextByte(memory, cpu, (uint16_t)(handler + 3u));
    EventVariable(memory, cpu, (uint16_t)(handler + 6u));
    EventFlagBit(memory, cpu, (uint16_t)(handler + 10u));
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
    EventNextByte(memory, cpu, (uint16_t)(handler + 2u));
    Compare8(cpu, A8(cpu), AbsoluteByte(memory, cpu, WRAM_EVENT_MAP_0692, 0));
    if (cpu->zero == (handler == EVENT_OP_GOTO_IF_0692))
        EventGoto(memory, cpu);
    else
        EventSkipWord(memory, cpu);
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
        EventSkipWord(memory, cpu);
    else
        EventGoto(memory, cpu);
    return EVENT_OPCODE_NEXT;
}

/* $1E: bit 0 clear -> first target, else $19 on the second. */
static unsigned EventOp1E(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    EventLoadSlotBits(memory, cpu);                            /* E4D8 */
    BitImmediate8(cpu, 0x01u);
    if (cpu->zero) {
        EventGoto(memory, cpu);
        return EVENT_OPCODE_NEXT;
    }
    EventNextByte(memory, cpu, 0xe4e7u);                       /* E4E5 */
    EventNextByte(memory, cpu, 0xe4eau);
    return EventOp19(memory, cpu);
}

/* $2B: skip only when bit 0 is set and bit 7 clear. */
static unsigned EventOp2B(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    EventLoadSlotBits(memory, cpu);                            /* E4F9 */
    BitImmediate8(cpu, 0x01u);
    if (!cpu->zero && !cpu->negative)
        EventSkipWord(memory, cpu);
    else
        EventGoto(memory, cpu);
    return EVENT_OPCODE_NEXT;
}

/* $1B: write PPU register $2100 + n (DB-relative). */
static unsigned EventOpWritePpu(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    TransferDirectToA(cpu);                                    /* E52A */
    EventNextByte(memory, cpu, 0xe52du);
    TransferAToX(cpu);
    EventNextByte(memory, cpu, 0xe531u);
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
    EventNextByte(memory, cpu, 0xe545u);
    Write8(memory, 0x7fd07fu, A8(cpu));
    EventNextByte(memory, cpu, 0xe54cu);
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
        EventNextByte(memory, cpu, (uint16_t)(handler + 3u));
        EventVariable(memory, cpu, (uint16_t)(handler + 6u));
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
    EventNextByte(memory, cpu, 0xd927u);
    EventVariable(memory, cpu, 0xd92au);
    TransferAToX(cpu);
    PushIndex(memory, cpu);
    EventNextByte(memory, cpu, 0xd92fu);
    EventValue(memory, cpu, 0xd932u);
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
        EventValue(memory, cpu, (uint16_t)(handler + 5u));
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
        EventGoto(memory, cpu);
    else
        EventSkipWord(memory, cpu);
    return EVENT_OPCODE_NEXT;
}

/* $80:DAE5: X = point operand - $E0, A = next byte, carry clear. */
static void EventPointOperands(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    TransferDirectToA(cpu);                                    /* DAE5 */
    EventNextByte(memory, cpu, 0xdae8u);
    cpu->carry = 1;
    Sbc8(cpu, 0xe0u);
    TransferAToX(cpu);
    EventNextByte(memory, cpu, 0xdaefu);
    cpu->carry = 0;
    SimulateRtsFrame(memory, cpu);
}

/* $83/$84: point copy $7F:D223/D263 = point X/Y + n. */
static unsigned EventOpOffsetPoint(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    const uint8_t y = handler == EVENT_OP_OFFSET_POINT_Y;

    EventPointOperands(memory, cpu, (uint16_t)(handler + 2u));
    Adc8(cpu, Read8(memory, LongIndexedAddress(
        y ? EVENT_POINT_Y : EVENT_POINT_X, cpu->x)));
    Write8(memory, LongIndexedAddress(
        y ? EVENT_POINT_D263 : EVENT_POINT_D223, cpu->x), A8(cpu));
    return EVENT_OPCODE_NEXT;
}

/* $80:CFFC: $56 = variable, $54 = point operand - $E0 + next byte. */
static void EventPointVariableOperands(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    EventNextByte(memory, cpu, 0xcffeu);                       /* CFFC */
    EventVariable(memory, cpu, 0xd001u);
    StoreADirect8(memory, cpu, 0x56u);
    Write8(memory, DirectAddress(cpu, 0x57u), 0x00u);
    EventNextByte(memory, cpu, 0xd008u);
    EventValue(memory, cpu, 0xd00bu);
    cpu->carry = 1;
    Sbc8(cpu, 0xe0u);
    StoreADirect8(memory, cpu, 0x54u);
    Write8(memory, DirectAddress(cpu, 0x55u), 0x00u);
    TransferDirectToA(cpu);
    EventNextByte(memory, cpu, 0xd016u);
    cpu->carry = 0;
    Adc8(cpu, DirectByte(memory, cpu, 0x54u));
    StoreADirect8(memory, cpu, 0x54u);
    SimulateRtsFrame(memory, cpu);
}

/* $9D/$9E: copy between point X ($54) and variable ($56). */
static unsigned EventOpCopyPointX(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    const uint8_t to_variable = handler == EVENT_OP_POINT_X_TO_VARIABLE;

    EventPointVariableOperands(memory, cpu, (uint16_t)(handler + 2u));
    LoadXDirect16(memory, cpu, to_variable ? 0x54u : 0x56u);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(
        to_variable ? EVENT_POINT_X : EVENT_VARIABLES, cpu->x)));
    LoadXDirect16(memory, cpu, to_variable ? 0x56u : 0x54u);
    Write8(memory, LongIndexedAddress(
        to_variable ? EVENT_VARIABLES : EVENT_POINT_X, cpu->x), A8(cpu));
    return EVENT_OPCODE_NEXT;
}

/* $79, $86, $A2, $AB, $B5, $B8: single stores and bit changes. */
static unsigned EventOpSmall(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    switch (handler) {
    case EVENT_OP_STORE_E316:
        EventNextByte(memory, cpu, 0xd5a7u);                   /* D5A5 */
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
    case EVENT_OP_B5:
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
    EventNextByte(memory, cpu, 0xdb24u);
    AslA8(cpu);
    TransferAToX(cpu);
    TransferDirectToA(cpu);
    EventNextByte(memory, cpu, 0xdb2au);
    EventSignedByte(memory, cpu, 0xdb2du);
    target = LongIndexedAddress(0x7fd0ceu, cpu->x);
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, target));
    Write16Long(memory, target, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    TransferDirectToA(cpu);                                    /* DB39 */
    EventNextByte(memory, cpu, 0xdb3cu);
    EventSignedByte(memory, cpu, 0xdb3fu);
    target = LongIndexedAddress(0x7fd0d6u, cpu->x);
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, target));
    Write16Long(memory, target, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    EventNextByte(memory, cpu, 0xdb4du);                       /* DB4B */
    Write8(memory, LongIndexedAddress(0x7fd0deu, cpu->x), A8(cpu));
    EventNextByte(memory, cpu, 0xdb54u);
    Write8(memory, LongIndexedAddress(0x7fd0e6u, cpu->x), A8(cpu));
    /* TDC: the speed high bytes get the DP low byte. */
    TransferDirectToA(cpu);
    Write8(memory, LongIndexedAddress(0x7fd0dfu, cpu->x), A8(cpu));
    Write8(memory, LongIndexedAddress(0x7fd0e7u, cpu->x), A8(cpu));
    LoadA8(cpu, 0x40u);
    TestBitsAbsolute8(memory, cpu, WRAM_SCREEN_EFFECTS, 1);
    return EVENT_OPCODE_NEXT;
}

/* Handlers behind JMP ($E5A4,x); others hand off. */
static unsigned EventScriptOpcode(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    unsigned i;

    for (i = 0; i < 12u; ++i)
        if (handler == kEventCompares[i].handler)
            return EventOpCompareVariable(memory, cpu, i);
    switch (handler) {
    case EVENT_OP_END:
        return EventOpEnd(memory, cpu);
    case EVENT_OP_WAIT:
        return EventOpWait(memory, cpu);
    case EVENT_OP_GOTO:
        EventGoto(memory, cpu);
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
    case EVENT_OP_OFFSET_POINT_X:
    case EVENT_OP_OFFSET_POINT_Y:
        return EventOpOffsetPoint(memory, cpu, handler);
    case EVENT_OP_POINT_X_TO_VARIABLE:
    case EVENT_OP_VARIABLE_TO_POINT_X:
        return EventOpCopyPointX(memory, cpu, handler);
    case EVENT_OP_WAIT_FOR_LISTED_ACTOR:
    case EVENT_OP_WAIT_FOR_ACTOR:
    case EVENT_OP_WAIT_FOR_LEADER:
        return EventOpWaitForActor(memory, cpu, handler);
    case EVENT_OP_SCROLL_LAYER:
        return EventOpScrollLayer(memory, cpu);
    case EVENT_OP_STORE_E316:
    case EVENT_OP_86:
    case EVENT_OP_A2:
    case EVENT_OP_RESET_STAIRS:
    case EVENT_OP_B5:
    case EVENT_OP_B8:
        return EventOpSmall(memory, cpu, handler);
    default:
        return EVENT_OPCODE_HANDOFF;
    }
}

/* $80:CC35: run the script until it yields; 0 = handoff at CC3F. */
static uint8_t EventRunScript(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    unsigned *passes) {
    for (;;) {
        uint16_t handler;

        EventNextByte(memory, cpu, 0xcc37u);                   /* CC35 */
        AslA8(cpu);
        ExchangeAccumulatorBytes(cpu);
        LoadA8(cpu, 0x00u);
        RolA8(cpu);
        ExchangeAccumulatorBytes(cpu);
        TransferAToX(cpu);
        handler = Read16Bank(memory, 0x80u, (uint16_t)(0xe5a4u + cpu->x));
        switch (EventScriptOpcode(memory, cpu,
                    *passes < EVENT_OPCODE_LIMIT ? handler : 0u)) {
        case EVENT_OPCODE_NEXT:
            ++*passes;
            continue;
        case EVENT_OPCODE_YIELD:
            ++*passes;
            return 1;
        default:                                               /* CC3F */
            cpu->resume_pc = 0x80cc3fu;
            return 0;
        }
    }
}

/* $80:CBCC: resume the due slot X through the script VM. */
static uint8_t EventResumeSlot(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    unsigned *passes) {
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
    if (!EventRunScript(memory, cpu, passes))
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
                if (!EventResumeSlot(memory, cpu, passes))
                    return 0;
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
