#ifndef LUFIA2_FIELD_EVENT_SCRIPT_INTERNAL_H
#define LUFIA2_FIELD_EVENT_SCRIPT_INTERNAL_H

/* Field event script VM, shared by its two modules. */

#include "lufia2/execution.h"

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
/* Script call frames: 13 x 10 bytes, tag = depth << 4 | slot. */
#define EVENT_CALL_FRAMES 0x7fd466u
#define EVENT_CALL_DEPTH 0x7fd4e6u
/* Slot variables saved for $FB-$FE call arguments. */
#define EVENT_SAVED_VARIABLES 0x7fd19cu
/* Condition result: bit 7 true. */
#define EVENT_CONDITION 0x7fd19au
/* Layer redraw requests from the scripts, become $74 bits. */
#define WRAM_EVENT_REDRAW 0x1273u
#define WRAM_EVENT_MAP_0692 0x0692u
/* $05B5 bit 1: a slot is still armed. */
#define FIELD_FLAG_EVENT_ARMED 0x02u
#define DP_EVENT_SLOT_RECORD 0xabu
/* List entries per $80:BFAA search before a handoff at $80:BFBC. */
#define EVENT_SEARCH_LIMIT 65536u
/* Opcodes per tick before an exact handoff at $80:CC3F. */
#define EVENT_OPCODE_LIMIT 4096u

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
    EVENT_OP_RELEASE_CAMERA = 0xdbca,                          /* $B5 */
    EVENT_OP_B8 = 0xd5e4,                                      /* $B8 */
    EVENT_OP_WAIT_FOR_LISTED_ACTOR = 0xdd8f,                   /* $5F */
    EVENT_OP_WAIT_FOR_ACTOR = 0xdd86,                          /* $68 */
    EVENT_OP_WAIT_FOR_LEADER = 0xd4ca,                         /* $6B */
    EVENT_OP_SCROLL_LAYER = 0xdb21,                            /* $58 */
    EVENT_OP_SPAWN_AT = 0xd4e0,                                /* $24 */
    EVENT_OP_SPAWN_AT_POSITION = 0xd4ec,                       /* $25 */
    EVENT_OP_STORE_CONDITION = 0xe421,                         /* $29 */
    EVENT_OP_CALL = 0xd79b,                                    /* $A9 */
    EVENT_OP_RETURN = 0xd849,                                  /* $AA */
    EVENT_OP_SET_POINT = 0xda82,                               /* $55 */
    EVENT_OP_SET_POINT_VALUE = 0xda98,                         /* $69 */
    EVENT_OP_85 = 0xdaf2,                                      /* $85 */
    EVENT_OP_PLACE_ACTOR_64 = 0xddf2,                          /* $64 */
    EVENT_OP_PLACE_ACTOR_65 = 0xddfa,                          /* $65 */
    EVENT_OP_PLACE_ACTOR_66 = 0xde02,                          /* $66 */
    EVENT_OP_PLACE_ACTOR_67 = 0xde0a,                          /* $67 */
    EVENT_OP_PLACE_ACTOR_7C = 0xded3,                          /* $7C */
    EVENT_OP_PLACE_ACTOR_7D = 0xdeda,                          /* $7D */
    EVENT_OP_PLACE_ACTOR_7E = 0xdee1,                          /* $7E */
    EVENT_OP_PLACE_ACTOR_7F = 0xdee8,                          /* $7F */
    EVENT_OP_PLACE_ACTOR_80 = 0xdeef,                          /* $80 */
    EVENT_OP_PLACE_ACTOR_81 = 0xde11,                          /* $81 */
    EVENT_OP_PLACE_ACTOR_A3 = 0xdeaa,                          /* $A3 */
    EVENT_OP_PLACE_ACTOR_A4 = 0xdeb1,                          /* $A4 */
    EVENT_OP_PLACE_ACTOR_A5 = 0xdeb8,                          /* $A5 */
    EVENT_OP_PLACE_ACTOR_A6 = 0xdebf,                          /* $A6 */
    EVENT_OP_MOVE_ACTOR_AF = 0xdf69,                           /* $AF */
    EVENT_OP_MOVE_ACTOR_B0 = 0xdf70,                           /* $B0 */
    EVENT_OP_MOVE_ACTOR_B1 = 0xdf77,                           /* $B1 */
    EVENT_OP_MOVE_ACTOR_B2 = 0xdf7e,                           /* $B2 */
    EVENT_OP_MOVE_CAMERA = 0xdb6a,                             /* $B4 */
    EVENT_OP_B6 = 0xdbdd,                                      /* $B6 */
    EVENT_OP_B7 = 0xdbf5,                                      /* $B7 */
    EVENT_OP_FILL_E33E = 0xcecf,                               /* $BD */
    EVENT_OP_POINT_FROM_OBJECT = 0xe038,                       /* $82 */
    EVENT_OP_GOTO_IF_LEADER_AT = 0xe0cd,                       /* $12 */
    EVENT_OP_GOTO_UNLESS_LEADER_AT = 0xe0d3,                   /* $6D */
    EVENT_OP_FLAG_LEADER_AT = 0xe105,                          /* $13 */
    EVENT_OP_14 = 0xe3a0,                                      /* $14 */
    EVENT_OP_FLAG_CELLS_09 = 0xe344,                           /* $15 */
    EVENT_OP_GOTO_IF_CELLS_09 = 0xe34e,                        /* $16 */
    EVENT_OP_GOTO_UNLESS_CELLS_09 = 0xe358,                    /* $6E */
    EVENT_OP_FLAG_CELLS_08 = 0xe177,                           /* $17 */
    EVENT_OP_GOTO_IF_CELLS_08 = 0xe181,                        /* $18 */
    EVENT_OP_GOTO_UNLESS_CELLS_08 = 0xe18b,                    /* $6F */
    EVENT_OP_FLAG_CELLS_01 = 0xe198,                           /* $72 */
    EVENT_OP_GOTO_IF_CELLS_01 = 0xe1a2,                        /* $73 */
    EVENT_OP_GOTO_UNLESS_CELLS_01 = 0xe1ac,                    /* $74 */
    EVENT_OP_KEEP_D0F4 = 0xe1b9,                               /* $75 */
    EVENT_OP_GOTO_IF_D0F4 = 0xe1bf,                            /* $76 */
    EVENT_OP_GOTO_UNLESS_D0F4 = 0xe1c5,                        /* $77 */
    EVENT_OP_FLAG_OBJECTS = 0xcd1d,                            /* $05 */
    EVENT_OP_GOTO_IF_OBJECTS = 0xcd23,                         /* $04 */
    EVENT_OP_GOTO_UNLESS_OBJECTS = 0xcd32                      /* $70 */
};

/* $80:E8B9: next script byte; a wrapping Y steps to the next bank. */
void Lufia2EventNextByte(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address);

/* $80:E8D0: step back one byte; below $8000 the bank steps down. */
void Lufia2EventPrevByte(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address);

/* $80:E9BC: $FB-$FF read slot variables ($C0-$DF as 0-$1F). */
void Lufia2EventVariable(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address);

/* $80:E9ED: value operand; $A0-$BF read variable n, $FB stays. */
void Lufia2EventValue(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address);

/* $80:D31C: sleep A frames, the slot resumes at Y. */
unsigned Lufia2EventSleep(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* Actor, position and point opcodes; others hand off. */
unsigned Lufia2EventActorOpcode(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler,
    uint32_t *handoff);

/* $80:E8AD: word operand, low byte first; leaves M=0. */
void Lufia2EventNextWord(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address);

/* $80:E8F4: Y = A in the base bank; below $8000 steps a bank. */
void Lufia2EventSetPointer(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address);

/* $80:D2A2: goto base + word. */
void Lufia2EventGoto(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:D2B2: skip an untaken goto target. */
void Lufia2EventSkipWord(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:E898: X = flag byte of n, A = its bit from $80:BE45. */
void Lufia2EventFlagBit(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address);

/* $80:EA09: position operand in A (x) and B (y); 0 = handoff. */
uint8_t Lufia2EventPosition(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address,
    uint32_t *handoff);

/* $80:E92A: box $9F-$A2 of an operand; 0 = handoff. */
uint8_t Lufia2EventArea(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address,
    uint32_t *handoff);

/* Condition opcodes; others hand off. */
unsigned Lufia2EventConditionOpcode(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler,
    uint32_t *handoff);

#endif
