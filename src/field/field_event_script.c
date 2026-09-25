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
    EVENT_OP_NOP = 0xdb1e                                      /* $57 */
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

/* $11: sleep n frames, resume after the operand. */
static unsigned EventOpWait(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    EventNextByte(memory, cpu, 0xd31bu);                       /* D319 */
    LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);
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

/* Handlers behind JMP ($E5A4,x); others hand off. */
static unsigned EventScriptOpcode(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
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
