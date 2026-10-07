/* Field object slots and script VM ($83:E03E). */

#include <stdbool.h>

#include "core/cpu_internal.h"
#include "core/cpu_ops.h"
#include "field/field_coordinates_internal.h"
#include "lufia2/actor.h"
#include "lufia2/system.h"
#include "actor/actor_internal.h"
#include "system/system_internal.h"
#include "system/wram.h"

/* ROM tables: four-byte animation records, sine and cosine. */
#define OBJECT_PROGRAM_BANK 0x830000u

enum {
    OBJECT_SPRITE_SLOT_COUNTS = 0x83abf4u,
    OBJECT_FRAME_DATA_SIZES = 0x83abf8u,
    OBJECT_ANIMATION_RECORDS = 0x83f388u,
    OBJECT_SCRIPT_TABLE = 0x8ec7u, /* bank $91, operands are offsets from it */
    SINE_TABLE = 0x8084edu,
    COSINE_TABLE = 0x80852du,
};

/* Opcode $14 spin and zoom state per object slot. */
enum {
    OBJECT_SPIN_ANGLE_A = 0x14d9u,
    OBJECT_SPIN_ANGLE_B = 0x14f9u,
    OBJECT_PROJECTED_Y = 0x1519u,
    OBJECT_PROJECTED_X = 0x1539u,
    OBJECT_ZOOM = 0x1579u,
    OBJECT_OFFSET_ANGLE = 0x1599u,
    OBJECT_SPIN_UPDATE = 0x15b9u, /* bits request the recalculations */
    OBJECT_ZOOM_RATE = 0x15d9u,
    OBJECT_ZOOM_COUNTER = 0x15f9u,
    OBJECT_ZOOM_LIMIT = 0x1619u,
    OBJECT_SPIN_RATE_A = 0x1639u,
    OBJECT_SPIN_COUNTER_A = 0x1659u,
    OBJECT_SPIN_LIMIT_A = 0x1679u,
    OBJECT_SPIN_RATE_B = 0x1699u,
    OBJECT_SPIN_COUNTER_B = 0x16b9u,
    OBJECT_SPIN_LIMIT_B = 0x16d9u,
};

/* $83:E200: sprite frame $54 for object $A7. */
static void ObjectSetFrame(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    PushY(memory, cpu);                                        /* E200 */
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_A)));
    Write8(memory, LongIndexedAddress(WRAM_OBJECT_FRAME, cpu->x), A8(cpu));
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_A)));
    And8(cpu, 0x3fu);
    Write8(memory, SNES_WRMPYA, A8(cpu));
    TransferDirectToA(cpu);                                    /* E212 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_OBJECT_SPRITE_SIZE, cpu->x)));
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(OBJECT_FRAME_DATA_SIZES, cpu->x)));
    Write8(memory, SNES_WRMPYB, A8(cpu));
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0732u, 0));
    LoadXDirect(memory, cpu, DP_SLOT_RECORD_OFFSET); /* E223 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress((WRAM_OBJECT_ANIMATION_POINTER + 2u),
                                                 cpu->x)));
    Write8(memory, LongIndexedAddress((WRAM_OBJECT_FRAME_POINTER + 2u), cpu->x),
           A8(cpu));
    LoadA8(cpu, Read8(memory, SNES_RDMPYL));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, 0x00u);
    SetAccumulatorWidth(cpu, 0);                               /* E234 */
    LsrA16(cpu);
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, LongIndexedAddress(WRAM_OBJECT_ANIMATION_POINTER,
                                                          cpu->x)));
    Write16Long(memory, LongIndexedAddress(WRAM_OBJECT_FRAME_POINTER, cpu->x),
                cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);                   /* E242 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_A)));
    AslA8(cpu);
    Adc8(cpu, 0x00u);
    AslA8(cpu);
    Adc8(cpu, 0x00u);
    And8(cpu, 0x03u);
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_A), A8(cpu));
    LoadAAbsolute8(memory, cpu, WRAM_OBJECT_STATE, cpu->x);              /* E250 */
    And8(cpu, 0xfcu);
    Or8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_A)));
    Or8(cpu, 0x20u);
    StoreAAbsolute8(memory, cpu, WRAM_OBJECT_STATE, cpu->x);
    cpu->y = PullIndexValue(memory, cpu);                      /* E25C */
    SimulateRtsFrame(memory, cpu);
}

typedef enum ObjectFlow {
    OBJECT_FLOW_DISPATCH = 0,
    OBJECT_FLOW_RETURN = 1,
    OBJECT_FLOW_BOUNDARY = 2,
} ObjectFlow;

/* $83:E143: store the cursor, PLB, RTS. */
static ObjectFlow ObjectSaveAndReturn(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_SLOT_RECORD_OFFSET); /* E143 */
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, cpu->y);
    Write16Long(memory, LongIndexedAddress(WRAM_OBJECT_SCRIPT, cpu->x),
        cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    PullDataBank(memory, cpu);
    return OBJECT_FLOW_RETURN;
}

/* $83:E11C: advance by A; yield on $064A bit 4. */
static ObjectFlow ObjectAdvance(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);                               /* E11C */
    Write16Direct(memory, cpu, DP_SCRATCH_A, cpu->y);
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, DP_SCRATCH_A));
    LoadXDirect(memory, cpu, DP_SLOT_RECORD_OFFSET);
    Write16Long(memory, LongIndexedAddress(WRAM_OBJECT_SCRIPT, cpu->x),
        cpu->accumulator);
    TransferAToY(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);                   /* E12C */
    LoadAAbsolute8(memory, cpu, WRAM_OBJECT_STATE, cpu->x);
    BitImmediate8(cpu, 0x10u);
    if (cpu->zero)
        return OBJECT_FLOW_DISPATCH;
    LoadA8(cpu, 0x01u);                                        /* E135 */
    Write8(memory, LongIndexedAddress(WRAM_UNK_7FDFAE, cpu->x), A8(cpu));
    PullDataBank(memory, cpu);
    return OBJECT_FLOW_RETURN;
}

/* $83:FB05: sign-extend nibble A; M=0 exit. */
static void ObjectSignNibble(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    BitImmediate8(cpu, 0x08u);                                 /* FB05 */
    if (!cpu->zero) {
        Or8(cpu, 0xf0u);
        ExchangeAccumulatorBytes(cpu);
        LoadA8(cpu, 0xffu);
        ExchangeAccumulatorBytes(cpu);
    }
    SetAccumulatorWidth(cpu, 0);
    SimulateRtsFrame(memory, cpu);
}

/* $83:E1AD / $83:E1B9: add A16 to a position word. */
static void ObjectAddPosition(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t base,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, LongIndexedAddress(base, cpu->x)));
    Write16Long(memory, LongIndexedAddress(base, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    SimulateRtsFrame(memory, cpu);
}

/* $83:EED8: random offset around 0 of width operand. */
static void ObjectJitter(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadAAbsolute8(memory, cpu, 0x00u, cpu->y); /* EED8 */
    LsrA8(cpu);
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_A), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x00u, cpu->y);
    Lufia2CallRandomScale(memory, cpu, 0xeee4u);               /* $80:8299 */
    cpu->carry = 1;
    Sbc8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_A)));
    Lufia2SignExtendA8(memory, cpu, 0xeeeau);
    SimulateRtsFrame(memory, cpu);
}

/* $83:ED63: low nibble into a handler table. */
static uint16_t ObjectSubDispatch(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t table,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    TransferDirectToA(cpu);                                    /* ED63 */
    LoadAAbsolute8(memory, cpu, 0x00u, cpu->y);
    IncrementY16(cpu);
    And8(cpu, 0x0fu);
    AslA8(cpu);
    TransferAToX(cpu);
    SimulateRtsFrame(memory, cpu);
    return Read16ProgramIndexed(memory, cpu, table, cpu->x);
}

/* $83:EE0E: copy animation state of slot X to $A7. */
static void ObjectTakeAnimation(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_OBJECT_SPRITE_SLOT_A, cpu->x)));
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_A), A8(cpu));
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_OBJECT_SPRITE_SLOT_B, cpu->x)));
    PushAccumulator8(memory, cpu);
    TransferXToA(cpu);
    PushAccumulator8(memory, cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_OBJECT_ANIMATION_FLAGS, cpu->x)));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_OBJECT_SPRITE_SIZE, cpu->x)));
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    Write8(memory, LongIndexedAddress(WRAM_OBJECT_SPRITE_SIZE, cpu->x), A8(cpu));
    ExchangeAccumulatorBytes(cpu);
    Write8(memory, LongIndexedAddress(WRAM_OBJECT_ANIMATION_FLAGS, cpu->x), A8(cpu));
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_A)));
    Write8(memory, LongIndexedAddress(WRAM_OBJECT_SPRITE_SLOT_A, cpu->x), A8(cpu));
    LoadA8(cpu, Pull8(memory, cpu));
    Write8(memory, LongIndexedAddress(WRAM_UNK_7FE3A6, cpu->x), A8(cpu));
    LoadA8(cpu, Pull8(memory, cpu));
    Write8(memory, LongIndexedAddress(WRAM_OBJECT_SPRITE_SLOT_B, cpu->x), A8(cpu));
    LoadAAbsolute8(memory, cpu, WRAM_OBJECT_STATE, cpu->x);
    And8(cpu, 0xfbu);
    StoreAAbsolute8(memory, cpu, WRAM_OBJECT_STATE, cpu->x);
    SimulateRtsFrame(memory, cpu);
}

/* $83:ECDE: sprite slots and VRAM base for object $A7. */
static void ObjectSpriteSetup(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0xecdbu);
    SetAccumulatorWidth(cpu, 1);                               /* ECDE */
    SetIndexWidth(cpu, 1);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadAAbsolute8(memory, cpu, WRAM_OBJECT_STATE, cpu->x);
    And8(cpu, 0xfbu);
    StoreAAbsolute8(memory, cpu, WRAM_OBJECT_STATE, cpu->x);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_OBJECT_SPRITE_SIZE, cpu->x)));
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(OBJECT_SPRITE_SLOT_COUNTS, cpu->x)));
    Lufia2SpriteAllocSlots(memory, cpu, 0xecf6u);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);                   /* ECF7 */
    Write8(memory, LongIndexedAddress(WRAM_OBJECT_SPRITE_SLOT_A, cpu->x), A8(cpu));
    ExchangeAccumulatorBytes(cpu);
    Write8(memory, LongIndexedAddress(WRAM_OBJECT_SPRITE_SLOT_B, cpu->x), A8(cpu));
    Lufia2SpriteVramBase(memory, cpu, 0xed05u);
    SetIndexWidth(cpu, 0);                                     /* ED06 */
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
    Write16Long(memory, LongIndexedAddress(WRAM_OBJECT_SPRITE_VRAM_BASE, cpu->x),
                cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    SimulateRtsFrame(memory, cpu);
}

/* $83:EC9C: animation A for object X. */
static void ObjectAnimationSetup(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    PushY(memory, cpu);                                        /* EC9C */
    Write8(memory, LongIndexedAddress(WRAM_OBJECT_ANIMATION_ID, cpu->x), A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    AslA16(cpu);
    AslA16(cpu);
    TransferAToX(cpu);
    TransferAToY(cpu);
    LoadA16(cpu,
            Read16Long(memory, LongIndexedAddress(OBJECT_ANIMATION_RECORDS, cpu->x)));
    LoadXDirect(memory, cpu, DP_SLOT_RECORD_OFFSET);
    Write16Long(memory, LongIndexedAddress(WRAM_OBJECT_ANIMATION_POINTER, cpu->x),
                cpu->accumulator);
    cpu->x = cpu->y;                                           /* TYX */
    SetNz16(cpu, cpu->x);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu,
           Read8(memory, LongIndexedAddress((OBJECT_ANIMATION_RECORDS + 2u), cpu->x)));
    LoadXDirect(memory, cpu, DP_SLOT_RECORD_OFFSET);
    Write8(memory, LongIndexedAddress((WRAM_OBJECT_ANIMATION_POINTER + 2u), cpu->x),
           A8(cpu));
    cpu->x = cpu->y;
    SetNz16(cpu, cpu->x);
    LoadA8(cpu,
           Read8(memory, LongIndexedAddress((OBJECT_ANIMATION_RECORDS + 3u), cpu->x)));
    SetIndexWidth(cpu, 1);                                     /* ECC3 */
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_A), A8(cpu));
    And8(cpu, 0x0fu);
    Write8(memory, LongIndexedAddress(WRAM_OBJECT_ANIMATION_FLAGS, cpu->x), A8(cpu));
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_A)));
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    Write8(memory, LongIndexedAddress(WRAM_OBJECT_SPRITE_SIZE, cpu->x), A8(cpu));
    ObjectSpriteSetup(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);                      /* ECDC */
    SimulateRtsFrame(memory, cpu);
}

/* DP words: child offset, then its fine position. */
enum { SPAWN_OFFSET_X = 0x5a, SPAWN_OFFSET_Y = 0x63 };

/* $83:E8A6: spawn child object from the operand block at Y. */
static void ObjectSpawnChild(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    TransferDirectToA(cpu);                                    /* E8A6 */
    LoadAAbsolute8(memory, cpu, 0x01u, cpu->y);
    Lufia2SignExtendA8(memory, cpu, 0xe8acu);
    Write16Direct(memory, cpu, SPAWN_OFFSET_X, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    TransferDirectToA(cpu);                                    /* E8B1 */
    LoadAAbsolute8(memory, cpu, 0x02u, cpu->y);
    Lufia2SignExtendA8(memory, cpu, 0xe8b7u);
    Write16Direct(memory, cpu, SPAWN_OFFSET_Y, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);                   /* E8BC */
    PushIndex(memory, cpu);
    LoadAAbsolute8(memory, cpu, 0x00u, cpu->y);
    SimulateJslFrame(memory, cpu, 0x83u, 0xe8c5u);
    Lufia2ActorSpawn(memory, cpu);                             /* DF87 */
    SimulateRtlFrame(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);                      /* E8C6 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_ACTOR_SLOT)));
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_A), A8(cpu));
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_B), 0x00u);
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
    StoreYDirect16(memory, cpu, DP_ACTOR_SLOT);
    SimulateJslFrame(memory, cpu, 0x83u, 0xe8d4u);
    Lufia2ActorRecordOffsets(memory, cpu);                     /* AB4F */
    SimulateRtlFrame(memory, cpu);
    SetAccumulatorWidth(cpu, 0);                               /* E8D5 */
    cpu->y = cpu->x;                                           /* TXY */
    SetNz16(cpu, cpu->y);
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_OBJECT_FINE_X, cpu->x)));
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, SPAWN_OFFSET_X));
    Write16Direct(memory, cpu, SPAWN_OFFSET_X, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_OBJECT_FINE_Y, cpu->x)));
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, SPAWN_OFFSET_Y));
    Write16Direct(memory, cpu, SPAWN_OFFSET_Y, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory,
                            LongIndexedAddress(WRAM_OBJECT_DISPLAY_OFFSET_X, cpu->x)));
    PushAccumulator16(memory, cpu);
    LoadA16(cpu, Read16Long(memory,
                            LongIndexedAddress(WRAM_OBJECT_DISPLAY_OFFSET_Y, cpu->x)));
    cpu->x = cpu->y;                                           /* TYX */
    SetNz16(cpu, cpu->x);
    Write16Long(memory, LongIndexedAddress(WRAM_OBJECT_DISPLAY_OFFSET_Y, cpu->x),
                cpu->accumulator);
    PullAccumulator16(memory, cpu);
    Write16Long(memory, LongIndexedAddress(WRAM_OBJECT_DISPLAY_OFFSET_X, cpu->x),
                cpu->accumulator);
    LoadA16(cpu, Read16Direct(memory, cpu, SPAWN_OFFSET_X));
    Write16Long(memory, LongIndexedAddress(WRAM_OBJECT_FINE_X, cpu->x),
                cpu->accumulator);
    LoadA16(cpu, Read16Direct(memory, cpu, SPAWN_OFFSET_Y));
    Write16Long(memory, LongIndexedAddress(WRAM_OBJECT_FINE_Y, cpu->x),
                cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    PushIndex(memory, cpu);                                    /* E90D */
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FDA2C, cpu->x)));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FD9CC, cpu->x)));
    LoadXDirect(memory, cpu, DP_SCRATCH_A);
    Write8(memory, LongIndexedAddress(WRAM_UNK_7FD9CC, cpu->x), A8(cpu));
    ExchangeAccumulatorBytes(cpu);
    Write8(memory, LongIndexedAddress(WRAM_UNK_7FDA2C, cpu->x), A8(cpu));
    TransferDirectToA(cpu);                                    /* E924 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_A)));
    Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_ACTOR_SLOT)));
    if (cpu->carry) {
        TransferAToX(cpu);                                     /* E92B */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FDFAE, cpu->x)));
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        Write8(memory, LongIndexedAddress(WRAM_UNK_7FDFAE, cpu->x), A8(cpu));
    }
    cpu->x = PullIndexValue(memory, cpu);                      /* E935 */
    SimulateRtsFrame(memory, cpu);
}

/* Size-0 sprites can spin $83:AB7C forever. */
static uint8_t ObjectSpriteSizeZero(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu,
    uint8_t animation) {
    const uint16_t index =
        (uint16_t)(((cpu->direct_page & 0xff00u) | animation) << 2);
    const uint8_t shape =
        Read8(memory, LongIndexedAddress((OBJECT_ANIMATION_RECORDS + 3u), index));

    return Read8(memory, OBJECT_SPRITE_SLOT_COUNTS + (uint32_t)(shape >> 4)) == 0;
}

static bool ObjectEventReturn(
    const Lufia2Memory *, Lufia2CpuState *, uint16_t, uint8_t);

static bool ObjectDespawn(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SimulateJslFrame(memory, cpu, 0x83u, 0xe140u);
    Lufia2ExecutionResult result = Lufia2ObjectRemoveSlot(memory, cpu);
    return result.flow == LUFIA2_EXECUTION_RETURNED &&
        ObjectEventReturn(memory, cpu, 0xe140u, 3u);
}

/* Object script handlers from $83:F2C8 and its low-nibble tables. */
enum ObjectOpcodeHandler {
    OBJECT_OP_0X_DESPAWN = 0xe13d,                 /* $0x */
    OBJECT_OP_2F_COPY_ACTOR0_POSITION = 0xe150,    /* $2F */
    OBJECT_OP_F6_NIBBLE_OFFSETS = 0xe168,          /* $F6 */
    OBJECT_OP_F7_BYTE_OFFSETS = 0xe191,            /* $F7 */
    OBJECT_OP_3X_SET_FRAME = 0xe1c5,               /* $3x */
    OBJECT_OP_2C_SET_FRAME = 0xe1d3,               /* $2C */
    OBJECT_OP_25_SET_FRAME = 0xe1e6,               /* $25 */
    OBJECT_OP_FD_SET_FRAME = 0xe1f4,               /* $FD */
    OBJECT_OP_SET_DRAW_FLAGS = 0xe270,             /* $2E */
    OBJECT_OP_19 = 0xe437,                         /* $19 */
    OBJECT_OP_8E = 0xe445,                         /* $8E */
    OBJECT_OP_1A_SPRITE_FRAME = 0xe589,            /* $1A */
    OBJECT_OP_8F_SET_FRAME = 0xe59f,               /* $8F */
    OBJECT_OP_MARK_BLOCKED_EVENT_OBJECT = 0xe5b5,  /* $1D */
    OBJECT_OP_12_NOP = 0xe700,                     /* $12 */
    OBJECT_OP_CENTER_ON_SCREEN = 0xe810,           /* $13 */
    OBJECT_OP_SPIN_ZOOM = 0xe27d,                  /* $14 */
    OBJECT_OP_4X_WAIT = 0xe831,                    /* $4x */
    OBJECT_OP_5X = 0xe840,                         /* $5x */
    OBJECT_OP_6X_SPAWN_CHILD = 0xe854,             /* $6x */
    OBJECT_OP_SPAWN_CHILD_WITH_ANIMATION = 0xe860, /* $85 */
    OBJECT_OP_SPAWN_CHILD_AT_OFFSET = 0xe87c,      /* $8A */
    OBJECT_OP_SPAWN_REGISTERED_CHILD = 0xe937,     /* $EE */
    OBJECT_OP_20 = 0xe99a,                         /* $20 */
    OBJECT_OP_SET_SLOT0_FLAGS = 0xea7d,            /* $22 */
    OBJECT_OP_SET_FIELD_CONTROL = 0xea8c,          /* $23 */
    OBJECT_OP_OR_FIELD_CONTROL = 0xea94,           /* $8D */
    OBJECT_OP_7X_LOOP_START = 0xeab5,              /* $7x */
    OBJECT_OP_80_LOOP_END = 0xead5,                /* $80 */
    OBJECT_OP_SET_ANIMATION = 0xeaff,              /* $26 */
    OBJECT_OP_28 = 0xeb38,                         /* $28 */
    OBJECT_OP_29 = 0xeb50,                         /* $29 */
    OBJECT_OP_2B = 0xec6d,                         /* $2B */
    OBJECT_OP_ANIMATION_FROM_OPERAND = 0xec8e,     /* $Ax */
    OBJECT_OP_CX = 0xed11,                         /* $Cx */
    OBJECT_OP_START_LEADER_EVENT = 0xe25e,
    OBJECT_OP_START_POSITION_EVENT = 0xeaa3,
    OBJECT_OP_START_MODE_EVENT = 0xeda8,
    OBJECT_OP_CLEAR_FOLLOWERS = 0xe96b,
    OBJECT_OP_APPLY_PROBED_RECORD = 0xeb0c,
    OBJECT_OP_WAKE_OR_APPLY_RECORD = 0xef22,
    OBJECT_OP_REACT_TO_MAP_RECORD = 0xe9d4,
    OBJECT_OP_REACT_TO_PROBED_ACTOR = 0xf256,
    OBJECT_OP_INTERACT_WITH_RECORD = 0xeb6d,
    OBJECT_OP_SET_CUSTOM_GRAPHICS = 0xe487,
    OBJECT_OP_REACT_TO_MOVEMENT = 0xf005,
    OBJECT_OP_EVENT_PROBE_JUMP = 0xf1b4,
    OBJECT_OP_EVENT_PROBE_ALT_JUMP = 0xf1bc,
    OBJECT_OP_PROBE_DIRECTION_JUMP = 0xeeec,
    OBJECT_OP_DX = 0xed32,                         /* $Dx */
    OBJECT_OP_8X_TABLE = 0xed45,                   /* $8x */
    OBJECT_OP_1X_TABLE = 0xed4b,                   /* $1x */
    OBJECT_OP_2X_TABLE = 0xed51,                   /* $2x */
    OBJECT_OP_EX_TABLE = 0xed57,                   /* $Ex */
    OBJECT_OP_FX_TABLE = 0xed5d,                   /* $Fx */
    OBJECT_OP_SET_FIELD_CONTROL_BIT4 = 0xed9b,     /* $84 */
    OBJECT_OP_89 = 0xedc2,                         /* $89 */
    OBJECT_OP_SET_FLAG_BIT7 = 0xedd7,              /* $EA */
    OBJECT_OP_TAKE_ANIMATION = 0xede6,             /* $F1 */
    OBJECT_OP_FB = 0xee48,                         /* $FB */
    OBJECT_OP_FC = 0xee71,                         /* $FC */
    OBJECT_OP_F2_RANDOM_JITTER = 0xeeb5,           /* $F2 */
    OBJECT_OP_F8_SET_STATE_BIT4 = 0xefd1,          /* $F8 */
    OBJECT_OP_F9_CLEAR_STATE_BIT4 = 0xefde,        /* $F9 */
    OBJECT_OP_SET_OFFSET_WORD = 0xf0e8,            /* $E2 */
    OBJECT_OP_COPY_OFFSETS = 0xf0fa,               /* $82 */
    OBJECT_OP_COPY_OFFSETS_REVERSED = 0xf10b,      /* $83 */
    OBJECT_OP_NEGATE_OFFSETS = 0xf11c,             /* $EB */
    OBJECT_OP_EC = 0xf137,                         /* $EC */
    OBJECT_OP_ED = 0xf155,                         /* $ED */
    OBJECT_OP_COPY_FRAME_GRAPHICS = 0xe505,
    OBJECT_OP_QUEUE_SOUND = 0xefeb,
    OBJECT_OP_E3 = 0xf19f,                         /* $E3 */
    OBJECT_OP_REFRESH_MAP_HEIGHT = 0xeff6,
    OBJECT_OP_UPDATE_HEIGHT_DRAW_FLAG = 0xe6d3,
    OBJECT_OP_CLEAR_MAP_OCCUPANCY = 0xed6d,
    OBJECT_OP_INTERPOLATE_POSITION = 0xe5bd,
    OBJECT_OP_APPROACH_POSITION = 0xe651,
};

/* $83:E831: object opcode $4x. */
static ObjectFlow ObjectOp4XWait(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadAAbsolute8(memory, cpu, 0x00u, cpu->y);
    And8(cpu, 0x0fu);
    Write8(memory, LongIndexedAddress(WRAM_UNK_7FDFAE, cpu->x), A8(cpu));
    IncrementY16(cpu);
    return ObjectSaveAndReturn(memory, cpu);
}

/* $83:E168: object opcode $F6. */
static ObjectFlow ObjectOpF6NibbleOffsets(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadAAbsolute8(memory, cpu, 0x00u, cpu->y);
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_A), A8(cpu));
    SimulateJsrFrame(memory, cpu, 0xe16fu);
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);                       /* E176 */
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_A)));
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    ObjectSignNibble(memory, cpu, 0xe181u);
    ObjectAddPosition(memory, cpu, WRAM_OBJECT_DISPLAY_OFFSET_X, 0xe184u);
    TransferDirectToA(cpu);                                /* E185 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_A)));
    And8(cpu, 0x0fu);
    ObjectSignNibble(memory, cpu, 0xe18cu);
    ObjectAddPosition(memory, cpu, WRAM_OBJECT_DISPLAY_OFFSET_Y, 0xe18fu);
    SimulateRtsFrame(memory, cpu);
    TransferDirectToA(cpu);                                /* E170 */
    LoadA8(cpu, 0x01u);
    return ObjectAdvance(memory, cpu);
}

/* $83:EEB5: object opcode $F2. */
static ObjectFlow ObjectOpF2RandomJitter(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
    ObjectJitter(memory, cpu, 0xeeb9u);
    IncrementY16(cpu);                                     /* EEBA */
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, LongIndexedAddress(WRAM_OBJECT_DISPLAY_OFFSET_X,
                                                          cpu->x)));
    Write16Long(memory, LongIndexedAddress(WRAM_OBJECT_DISPLAY_OFFSET_X, cpu->x),
                cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    ObjectJitter(memory, cpu, 0xeec8u);
    IncrementY16(cpu);                                     /* EEC9 */
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, LongIndexedAddress(WRAM_OBJECT_DISPLAY_OFFSET_Y,
                                                          cpu->x)));
    Write16Long(memory, LongIndexedAddress(WRAM_OBJECT_DISPLAY_OFFSET_Y, cpu->x),
                cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    return OBJECT_FLOW_DISPATCH;
}

/* $83:E589: object opcode $1A. */
static ObjectFlow ObjectOp1ASpriteFrame(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_OBJECT_FLAGS, cpu->x)));
    BitImmediate8(cpu, 0x20u);
    if (!cpu->zero) {
        LoadAAbsolute8(memory, cpu, 0x00u, cpu->y); /* E593 */
        Write8(memory, DirectAddress(cpu, DP_SCRATCH_A), A8(cpu));
        ObjectSetFrame(memory, cpu, 0xe59au);
    }
    IncrementY16(cpu);                                     /* E59B */
    return OBJECT_FLOW_DISPATCH;
}

/* $83:EAD5: object opcode $80. */
static ObjectFlow ObjectOp80LoopEnd(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_OBJECT_LOOP_COUNTER, cpu->x)));
    DecrementA8(cpu);
    Write8(memory, LongIndexedAddress(WRAM_OBJECT_LOOP_COUNTER, cpu->x), A8(cpu));
    if (cpu->negative) {
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));              /* EAE2 */
        Write8(memory, LongIndexedAddress(WRAM_OBJECT_LOOP_COUNTER, cpu->x), A8(cpu));
        return OBJECT_FLOW_DISPATCH;
    }
    LoadXDirect(memory, cpu, DP_SLOT_RECORD_OFFSET);                       /* EAEA */
    LoadA8(cpu,
           Read8(memory, LongIndexedAddress((WRAM_OBJECT_LOOP_POINTER + 1u), cpu->x)));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_OBJECT_LOOP_POINTER, cpu->x)));
    TransferAToY(cpu);
    LoadA8(cpu,
           Read8(memory, LongIndexedAddress((WRAM_OBJECT_LOOP_POINTER + 2u), cpu->x)));
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    return OBJECT_FLOW_DISPATCH;
}

/* Object opcode $2F: actor 0 fine position to $7F:DDFE/DE8E. */
static ObjectFlow ObjectOp2FCopyActor0Position(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, WRAM_ACTOR_FINE_X));
    Write16Long(memory, LongIndexedAddress(WRAM_OBJECT_FINE_X, cpu->x),
                cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, WRAM_ACTOR_FINE_Y));
    Write16Long(memory, LongIndexedAddress(WRAM_OBJECT_FINE_Y, cpu->x),
                cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    return OBJECT_FLOW_DISPATCH;
}

/* Object opcode $2E: next script byte to $7F:E33E[slot]. */
static ObjectFlow ObjectSetDrawFlags(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadAAbsolute8(memory, cpu, 0x00u, cpu->y);
    Write8(memory, LongIndexedAddress(WRAM_OBJECT_DRAW_FLAGS, cpu->x), A8(cpu));
    IncrementY16(cpu);
    return OBJECT_FLOW_DISPATCH;
}

/* Object opcode $1D: slot word offset to $1724/$1725. */
static ObjectFlow ObjectMarkBlockedEventObject(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
    Write8(memory, AbsoluteIndexedAddress(cpu, WRAM_BLOCKED_EVENT_OBJECT, 0),
           (uint8_t)cpu->x);
    Write8(memory, AbsoluteIndexedAddress(cpu, (WRAM_BLOCKED_EVENT_OBJECT + 1u), 0),
           (uint8_t)(cpu->x >> 8));
    return OBJECT_FLOW_DISPATCH;
}

/* Object opcode $13: position = scroll $1220/$1228 + (128, 112). */
static ObjectFlow ObjectCenterOnScreen(const Lufia2Memory *memory,
                                       Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, 0x001220u));
    cpu->carry = 0;
    Add16Immediate(cpu, 0x80u);
    Write16Long(memory, LongIndexedAddress(WRAM_OBJECT_SCREEN_X, cpu->x),
                cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, 0x001228u));
    cpu->carry = 0;
    Add16Immediate(cpu, 0x70u);
    Write16Long(memory, LongIndexedAddress(WRAM_OBJECT_SCREEN_Y, cpu->x),
                cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    return OBJECT_FLOW_DISPATCH;
}

/* Object opcode $22: $066A[0] = ($066A[0] & $06) | script byte. */
static ObjectFlow ObjectSetSlot0Flags(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadAAbsolute8(memory, cpu, WRAM_UNK_7E066A, 0);
    And8(cpu, 0x06u);
    Or8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x00u, cpu->y)));
    StoreAAbsolute8(memory, cpu, WRAM_UNK_7E066A, 0);
    IncrementY16(cpu);
    return OBJECT_FLOW_DISPATCH;
}

/* Object opcode $23: $7F:D0A1 = low byte of D. */
static ObjectFlow ObjectSetFieldControl(const Lufia2Memory *memory,
                                        Lufia2CpuState *cpu) {
    TransferDirectToA(cpu);
    Write8(memory, WRAM_FIELD_CONTROL_FLAGS, A8(cpu));
    return OBJECT_FLOW_DISPATCH;
}

/* Object opcode $8D: OR the next script byte into $7F:D0A1. */
static ObjectFlow ObjectOrFieldControl(const Lufia2Memory *memory,
                                       Lufia2CpuState *cpu) {
    LoadAAbsolute8(memory, cpu, 0x00u, cpu->y);
    Or8(cpu, Read8(memory, WRAM_FIELD_CONTROL_FLAGS));
    Write8(memory, WRAM_FIELD_CONTROL_FLAGS, A8(cpu));
    IncrementY16(cpu);
    return OBJECT_FLOW_DISPATCH;
}

/* $83:EAB5: object opcode $7x. */
static ObjectFlow ObjectOp7XLoopStart(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadAAbsolute8(memory, cpu, 0x01u, cpu->y);
    DecrementA8(cpu);
    Write8(memory, LongIndexedAddress(WRAM_OBJECT_LOOP_COUNTER, cpu->x), A8(cpu));
    IncrementY16(cpu);
    IncrementY16(cpu);
    LoadXDirect(memory, cpu, DP_SLOT_RECORD_OFFSET);                       /* EAC1 */
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, cpu->y);
    Write16Long(memory, LongIndexedAddress(WRAM_OBJECT_LOOP_POINTER, cpu->x),
                cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    PushDataBank(memory, cpu);                             /* EACC */
    LoadA8(cpu, Pull8(memory, cpu));
    Write8(memory, LongIndexedAddress((WRAM_OBJECT_LOOP_POINTER + 2u), cpu->x),
           A8(cpu));
    return OBJECT_FLOW_DISPATCH;
}

/* Object opcode $29 $2B $Cx: set or clear one flag bit.
 * $29: state bit 2 if the byte is non-zero; $2B: bit 6 of $7F:DB0C if the byte
 * is negative; $Cx: state bit 3 if the low nibble is zero. */
static ObjectFlow ObjectOp292BCX(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    uint8_t set;
    uint32_t target;
    uint8_t bit;

    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadAAbsolute8(memory, cpu, 0x00u, cpu->y);
    if (handler == OBJECT_OP_29) {
        set = !cpu->zero;
        target = AbsoluteIndexedAddress(cpu, WRAM_OBJECT_STATE, cpu->x);
        bit = 0x04u;
    } else if (handler == OBJECT_OP_2B) {
        set = cpu->negative;
        target = LongIndexedAddress(WRAM_OBJECT_FLAGS, cpu->x);
        bit = 0x40u;
    } else {
        And8(cpu, 0x0fu);
        set = cpu->zero;
        target = AbsoluteIndexedAddress(cpu, WRAM_OBJECT_STATE, cpu->x);
        bit = 0x08u;
    }
    LoadA8(cpu, Read8(memory, target));
    if (set)
        Or8(cpu, bit);
    else
        And8(cpu, (uint8_t)~bit);
    Write8(memory, target, A8(cpu));
    IncrementY16(cpu);
    return OBJECT_FLOW_DISPATCH;
}

/* Object opcode $84: set bit 4 of $7F:D0A1. */
static ObjectFlow ObjectSetFieldControlBit4(const Lufia2Memory *memory,
                                            Lufia2CpuState *cpu) {
    LoadA8(cpu, Read8(memory, WRAM_FIELD_CONTROL_FLAGS));
    Or8(cpu, 0x10u);
    Write8(memory, WRAM_FIELD_CONTROL_FLAGS, A8(cpu));
    return OBJECT_FLOW_DISPATCH;
}

/* Object opcode $89: $7F:E23E = $FF, set bit 7 of $7F:E33E. */
static ObjectFlow ObjectOp89(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadA8(cpu, 0xffu);
    Write8(memory, LongIndexedAddress(WRAM_OBJECT_SPRITE_SIZE, cpu->x), A8(cpu));
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_OBJECT_DRAW_FLAGS, cpu->x)));
    Or8(cpu, 0x80u);
    Write8(memory, LongIndexedAddress(WRAM_OBJECT_DRAW_FLAGS, cpu->x), A8(cpu));
    return OBJECT_FLOW_DISPATCH;
}

/* Object opcode $EA: set bit 7 of $7F:DB0C[slot]. */
static ObjectFlow ObjectSetFlagBit7(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_OBJECT_FLAGS, cpu->x)));
    Or8(cpu, 0x80u);
    Write8(memory, LongIndexedAddress(WRAM_OBJECT_FLAGS, cpu->x), A8(cpu));
    return OBJECT_FLOW_DISPATCH;
}

/* Object opcode $F8 $F9: set or clear bit 4 of the object state. */
static ObjectFlow ObjectOpF8F9StateBit4(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadAAbsolute8(memory, cpu, WRAM_OBJECT_STATE, cpu->x);
    if (handler == OBJECT_OP_F8_SET_STATE_BIT4)
        Or8(cpu, 0x10u);
    else
        And8(cpu, 0xefu);
    StoreAAbsolute8(memory, cpu, WRAM_OBJECT_STATE, cpu->x);
    return OBJECT_FLOW_DISPATCH;
}

/* Object opcode $E2: word operand to $7F:DA6C[slot]. */
static ObjectFlow ObjectSetOffsetWord(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x00u, cpu->y));
    Write16Long(memory, LongIndexedAddress(WRAM_OBJECT_SCREEN_X, cpu->x),
                cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    IncrementY16(cpu);
    IncrementY16(cpu);
    return OBJECT_FLOW_DISPATCH;
}

/* Object opcode $82 $83: copy $7F:DA6C to DAAC ($83: reverse). */
static ObjectFlow ObjectCopyOffsets(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                    uint16_t handler) {
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
    SetAccumulatorWidth(cpu, 0);
    if (handler == OBJECT_OP_COPY_OFFSETS)
        CopyLong16(memory, cpu, WRAM_OBJECT_SCREEN_X, WRAM_OBJECT_SCREEN_Y);
    else
        CopyLong16(memory, cpu, WRAM_OBJECT_SCREEN_Y, WRAM_OBJECT_SCREEN_X);
    SetAccumulatorWidth(cpu, 1);
    return OBJECT_FLOW_DISPATCH;
}

/* Object opcode $EB: $7F:DA6C[slot] = -$7F:DAAC[slot], per byte. */
static ObjectFlow ObjectNegateOffsets(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    unsigned i;

    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
    /* Each byte negated on its own. */
    for (i = 0; i < 2u; ++i) {
        LoadA8(cpu,
               Read8(memory, LongIndexedAddress(WRAM_OBJECT_SCREEN_Y + i, cpu->x)));
        LoadA8(cpu, (uint8_t)(A8(cpu) ^ 0xffu));
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        Write8(memory, LongIndexedAddress(WRAM_OBJECT_SCREEN_X + i, cpu->x), A8(cpu));
    }
    return OBJECT_FLOW_DISPATCH;
}

/* $83:F137: $EC starts a loop with the operand count. */
static ObjectFlow ObjectOpBeginLoop(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadAAbsolute8(memory, cpu, 0x00u, cpu->y);
    cpu->carry = 1;
    Sbc8(cpu, Read8(memory, LongIndexedAddress(WRAM_OBJECT_LOOP_COUNTER, cpu->x)));
    DecrementA8(cpu);
    Write8(memory, LongIndexedAddress(WRAM_OBJECT_LOOP_COUNTER, cpu->x), A8(cpu));
    IncrementY16(cpu);
    SetAccumulatorWidth(cpu, 0);                           /* F147 */
    LoadXDirect(memory, cpu, DP_SLOT_RECORD_OFFSET);
    LoadA16(cpu, cpu->y);
    Write16Long(memory, LongIndexedAddress(WRAM_OBJECT_LOOP_POINTER, cpu->x),
                cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    return OBJECT_FLOW_DISPATCH;
}

/* $83:F155: object opcode $ED. */
static ObjectFlow ObjectOpED(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x00u, cpu->y));
    cpu->carry = 0;
    Add16Immediate(cpu, OBJECT_SCRIPT_TABLE);
    Write16Long(memory, WRAM_UNK_7FDDAC, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    PushDataBank(memory, cpu);                             /* F164 */
    PushY(memory, cpu);
    LoadA8(cpu, 0x7fu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    LoadX16(cpu, 0x00u);
    do {
        TransferDirectToA(cpu);                            /* F16D */
        LoadAAbsolute8(memory, cpu, 0xd0a6u, cpu->x);
        if (!cpu->negative) {
            Write8(memory, DirectAddress(cpu, DP_SCRATCH_A), A8(cpu));
            AslA8(cpu);
            Adc8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_A)));
            TransferAToY(cpu);
            LoadAAbsolute8(memory, cpu, 0xddacu, 0);
            StoreAAbsolute8(memory, cpu, 0xdeeeu, cpu->y);
            LoadAAbsolute8(memory, cpu, 0xddadu, 0);
            StoreAAbsolute8(memory, cpu, 0xdeefu, cpu->y);
        }
        IncrementX16(cpu);                                 /* F185 */
        Compare16(cpu, cpu->x, 0x08u);
    } while (!cpu->zero);
    cpu->y = PullIndexValue(memory, cpu);                  /* F18B */
    PullDataBank(memory, cpu);
    IncrementY16(cpu);
    IncrementY16(cpu);
    return OBJECT_FLOW_DISPATCH;
}

/* $83:E1C5: object opcode $3x $2C $25 $FD $8F. */
static ObjectFlow ObjectOp3X2C25FD8FSetFrame(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    uint16_t back;
    uint8_t skip_cursor = 0;

    if (handler == OBJECT_OP_3X_SET_FRAME) {
        LoadAAbsolute8(memory, cpu, 0x00u, cpu->y);
        And8(cpu, 0x0fu);
        back = 0xe1ceu;
    } else if (handler == OBJECT_OP_2C_SET_FRAME) {
        LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
        LoadAAbsolute8(memory, cpu, 0x00u, cpu->y);
        cpu->carry = 0;
        Adc8(cpu, Read8(memory, LongIndexedAddress(WRAM_OBJECT_FRAME, cpu->x)));
        back = 0xe1e1u;
    } else if (handler == OBJECT_OP_25_SET_FRAME) {
        LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FDA2C, cpu->x)));
        back = 0xe1f0u;
        skip_cursor = 1;
    } else if (handler == OBJECT_OP_FD_SET_FRAME) {
        LoadAAbsolute8(memory, cpu, 0x00u, cpu->y);
        back = 0xe1fbu;
    } else {
        LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_OBJECT_FLAGS, cpu->x)));
        BitImmediate8(cpu, 0x20u);
        if (cpu->zero)
            return OBJECT_FLOW_DISPATCH;
        LoadA8(cpu, Read8(memory, (WRAM_OBJECT_SPAWN_IDS + 1u)));
        back = 0xe5b1u;
        skip_cursor = 1;
    }
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_A), A8(cpu));
    ObjectSetFrame(memory, cpu, back);
    if (!skip_cursor)
        IncrementY16(cpu);
    return OBJECT_FLOW_DISPATCH;
}

/* Object opcode $5x $28 $Dx: script jump to the operand + $8EC7. */
static ObjectFlow ObjectJump(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                             uint16_t handler) {
    if (handler == OBJECT_OP_5X) {
        LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FD9CC, cpu->x)));
        Write8(memory, DirectAddress(cpu, DP_SCRATCH_A), A8(cpu));
        Write8(memory, DirectAddress(cpu, DP_SCRATCH_B), 0x00u);
        SetAccumulatorWidth(cpu, 0);                       /* E84A */
        LoadA16(cpu, cpu->y);
        cpu->carry = 0;
        Add16Value(cpu, Read16Direct(memory, cpu, DP_SCRATCH_A));
        TransferAToY(cpu);
    } else if (handler == OBJECT_OP_28) {
        LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FDA2C, cpu->x)));
        AslA8(cpu);
        Write8(memory, DirectAddress(cpu, DP_SCRATCH_A), A8(cpu));
        Write8(memory, DirectAddress(cpu, DP_SCRATCH_B), 0x00u);
        SetAccumulatorWidth(cpu, 0);                       /* EB43 */
        LoadA16(cpu, cpu->y);
        cpu->carry = 0;
        Add16Value(cpu, Read16Direct(memory, cpu, DP_SCRATCH_A));
        LoadA16(cpu, (uint16_t)(cpu->accumulator - 1u));
        TransferAToY(cpu);
        SetAccumulatorWidth(cpu, 1);
    }
    SimulateJsrFrame(memory, cpu, 0xed34u);                /* ED32 */
    SetAccumulatorWidth(cpu, 0);                           /* ED38 */
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x01u, cpu->y));
    cpu->carry = 0;
    Add16Immediate(cpu, OBJECT_SCRIPT_TABLE);
    TransferAToY(cpu);
    SetAccumulatorWidth(cpu, 1);
    SimulateRtsFrame(memory, cpu);
    return OBJECT_FLOW_DISPATCH;
}

/* Object opcode $FB: signed nibbles to $7F:DDFE/DE8E positions. */
static ObjectFlow ObjectOpFB(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadAAbsolute8(memory, cpu, 0x00u, cpu->y);
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_A), A8(cpu));
    SimulateJsrFrame(memory, cpu, 0xee4fu);
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);                       /* EE56 */
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_A)));
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    ObjectSignNibble(memory, cpu, 0xee61u);
    ObjectAddPosition(memory, cpu, WRAM_OBJECT_FINE_X, 0xee64u);
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_A)));
    And8(cpu, 0x0fu);
    ObjectSignNibble(memory, cpu, 0xee6cu);
    ObjectAddPosition(memory, cpu, WRAM_OBJECT_FINE_Y, 0xee6fu);
    SimulateRtsFrame(memory, cpu);
    TransferDirectToA(cpu);                                /* EE50 */
    LoadA8(cpu, 0x01u);
    return ObjectAdvance(memory, cpu);
}

/* Object opcode $FC $E3: signed bytes to $7F:DDFE/DE8E positions. */
static ObjectFlow ObjectOpFCE3(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    const uint8_t fixed = handler == OBJECT_OP_E3;

    if (fixed) {
        LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_OBJECT_SCREEN_X, cpu->x)));
        Write8(memory, DirectAddress(cpu, DP_SCRATCH_A), A8(cpu));
        LoadA8(cpu,
               Read8(memory, LongIndexedAddress((WRAM_OBJECT_SCREEN_X + 1u), cpu->x)));
        Write8(memory, DirectAddress(cpu, DP_SCRATCH_B), A8(cpu));
    } else {
        LoadAAbsolute8(memory, cpu, 0x00u, cpu->y);
        Write8(memory, DirectAddress(cpu, DP_SCRATCH_A), A8(cpu));
        LoadAAbsolute8(memory, cpu, 0x01u, cpu->y);
        Write8(memory, DirectAddress(cpu, DP_SCRATCH_B), A8(cpu));
    }
    SimulateJsrFrame(memory, cpu, fixed ? 0xf1afu : 0xee7du);
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);                       /* EE84 */
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_A)));
    Lufia2SignExtendA8(memory, cpu, 0xee8bu);
    ObjectAddPosition(memory, cpu, WRAM_OBJECT_FINE_X, 0xee8eu);
    TransferDirectToA(cpu);                                /* EE91 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_B)));
    Lufia2SignExtendA8(memory, cpu, 0xee96u);
    ObjectAddPosition(memory, cpu, WRAM_OBJECT_FINE_Y, 0xee99u);
    SimulateRtsFrame(memory, cpu);
    TransferDirectToA(cpu);                                /* EE7E/F1B0 */
    if (!fixed)
        LoadA8(cpu, 0x02u);
    return ObjectAdvance(memory, cpu);
}

/* Object opcode $F7: signed bytes to $7F:DCDC/DD6C positions. */
static ObjectFlow ObjectOpF7ByteOffsets(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
    TransferDirectToA(cpu);
    LoadAAbsolute8(memory, cpu, 0x00u, cpu->y);
    Lufia2SignExtendA8(memory, cpu, 0xe199u);
    ObjectAddPosition(memory, cpu, WRAM_OBJECT_DISPLAY_OFFSET_X, 0xe19cu);
    TransferDirectToA(cpu);                                /* E19D */
    LoadAAbsolute8(memory, cpu, 0x01u, cpu->y);
    Lufia2SignExtendA8(memory, cpu, 0xe1a3u);
    ObjectAddPosition(memory, cpu, WRAM_OBJECT_DISPLAY_OFFSET_Y, 0xe1a6u);
    TransferDirectToA(cpu);                                /* E1A7 */
    LoadA8(cpu, 0x02u);
    return ObjectAdvance(memory, cpu);
}

/* $83:E99A: object opcode $20. */
static ObjectFlow ObjectOp20(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    unsigned i;

    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
    LoadAAbsolute8(memory, cpu, 0x00u, cpu->y);
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_A), A8(cpu));
    IncrementY16(cpu);
    TransferDirectToA(cpu);                                /* E9A2 */
    for (i = 0; i < 2u; ++i) {
        LoadA8(cpu,
               Read8(memory, LongIndexedAddress(WRAM_OBJECT_SCREEN_Y + i, cpu->x)));
        if (cpu->zero)
            continue;
        if (cpu->negative) {
            cpu->carry = 0;
            Adc8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_A)));
        } else {
            cpu->carry = 1;
            Sbc8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_A)));
        }
        LoadA8(cpu, (uint8_t)(A8(cpu) ^ 0xffu));
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        Write8(memory, LongIndexedAddress(WRAM_OBJECT_SCREEN_X + i, cpu->x), A8(cpu));
    }
    return OBJECT_FLOW_DISPATCH;
}

/* Object opcode $F1: take the animation of the slot with this id. */
static ObjectFlow ObjectTakeAnimationBySlotId(const Lufia2Memory *memory,
                                              Lufia2CpuState *cpu) {
    uint8_t found = 0;

    LoadAAbsolute8(memory, cpu, 0x00u, cpu->y);
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_A), A8(cpu));
    LoadX16(cpu, 0x00u);
    for (;;) {
        LoadA8(cpu,
               Read8(memory, LongIndexedAddress(WRAM_OBJECT_ANIMATION_ID, cpu->x)));
        Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_SCRATCH_A)));
        if (cpu->zero) {
            found = 1;
            break;
        }
        IncrementX16(cpu);                                 /* EDF6 */
        Compare16(cpu, cpu->x, 0x20u);
        if (cpu->zero)
            break;
    }
    if (!found) {
        /* No match: the ROM clears slot 32. */
        TransferDirectToA(cpu);                            /* EDFC */
        Write8(memory, LongIndexedAddress(WRAM_OBJECT_SPRITE_SLOT_A, cpu->x), A8(cpu));
        Write8(memory, LongIndexedAddress(WRAM_OBJECT_SPRITE_SLOT_B, cpu->x), A8(cpu));
    } else {
        ObjectTakeAnimation(memory, cpu, 0xee09u);
    }
    IncrementY16(cpu);                                     /* EE0A */
    return OBJECT_FLOW_DISPATCH;
}

/* Object opcode $19 $8E: share a slot with the same animation, else
 * set one up; $8E takes its ids from $7F:D4F3/D4F4. */
static ObjectFlow ObjectOp198E(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    const uint8_t animation =
        handler == OBJECT_OP_19
            ? Read8(memory, AbsoluteIndexedAddress(cpu, 0x00u, cpu->y))
            : Read8(memory, WRAM_OBJECT_SPAWN_IDS);

    if (ObjectSpriteSizeZero(memory, cpu, animation)) {
        cpu->resume_pc = OBJECT_PROGRAM_BANK | handler;
        return OBJECT_FLOW_BOUNDARY;
    }
    if (handler == OBJECT_OP_19) {
        LoadAAbsolute8(memory, cpu, 0x00u, cpu->y);
        Write8(memory, DirectAddress(cpu, DP_SCRATCH_A), A8(cpu));
        IncrementY16(cpu);
        LoadAAbsolute8(memory, cpu, 0x00u, cpu->y);
        Write8(memory, DirectAddress(cpu, DP_SCRATCH_B), A8(cpu));
        IncrementY16(cpu);
    } else {
        LoadA8(cpu, Read8(memory, WRAM_OBJECT_SPAWN_IDS));
        Write8(memory, DirectAddress(cpu, DP_SCRATCH_A), A8(cpu));
        LoadA8(cpu, Read8(memory, (WRAM_OBJECT_SPAWN_IDS + 1u)));
        Write8(memory, DirectAddress(cpu, DP_SCRATCH_B), A8(cpu));
    }
    LoadX16(cpu, 0x00u); /* E453 */
    for (;;) {
        LoadA8(cpu,
               Read8(memory, LongIndexedAddress(WRAM_OBJECT_ANIMATION_ID, cpu->x)));
        Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_SCRATCH_A)));
        if (cpu->zero) {
            LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_OBJECT_FRAME, cpu->x)));
            Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_SCRATCH_B)));
            if (cpu->zero) {
                ObjectTakeAnimation(memory, cpu, 0xe483u); /* E481 */
                return OBJECT_FLOW_DISPATCH;
            }
        }
        IncrementX16(cpu);                                 /* E466 */
        Compare16(cpu, cpu->x, 0x20u);
        if (cpu->zero)
            break;
    }
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);               /* E46C */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_OBJECT_FLAGS, cpu->x)));
    Or8(cpu, 0x20u);
    Write8(memory, LongIndexedAddress(WRAM_OBJECT_FLAGS, cpu->x), A8(cpu));
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_A)));
    ObjectAnimationSetup(memory, cpu, 0xe47du);
    return OBJECT_FLOW_DISPATCH;
}

/* Object opcode $26: set up the animation id of $7F:DA4C[slot]. */
static ObjectFlow ObjectSetAnimation(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                     uint16_t handler) {
    if (ObjectSpriteSizeZero(
            memory, cpu,
            Read8(memory,
                  LongIndexedAddress(WRAM_OBJECT_ANIMATION_REQUEST,
                                     Read16Direct(memory, cpu, DP_ACTOR_SLOT))))) {
        cpu->resume_pc = OBJECT_PROGRAM_BANK | handler;
        return OBJECT_FLOW_BOUNDARY;
    }
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    TransferDirectToA(cpu);
    LoadA8(cpu,
           Read8(memory, LongIndexedAddress(WRAM_OBJECT_ANIMATION_REQUEST, cpu->x)));
    ObjectAnimationSetup(memory, cpu, 0xeb08u);
    return OBJECT_FLOW_DISPATCH;
}

/* Object opcode $Ax: set up the animation named by the operand. */
static ObjectFlow ObjectSetAnimationFromOperand(const Lufia2Memory *memory,
                                                Lufia2CpuState *cpu, uint16_t handler) {
    if (ObjectSpriteSizeZero(
            memory, cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x01u, cpu->y)))) {
        cpu->resume_pc = OBJECT_PROGRAM_BANK | handler;
        return OBJECT_FLOW_BOUNDARY;
    }
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    TransferDirectToA(cpu);
    LoadAAbsolute8(memory, cpu, 0x01u, cpu->y);
    ObjectAnimationSetup(memory, cpu, 0xec96u);
    IncrementY16(cpu);
    IncrementY16(cpu);
    return OBJECT_FLOW_DISPATCH;
}

/* Object opcode $6x: spawn a child object. */
static ObjectFlow ObjectOp6XSpawnChild(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    IncrementY16(cpu);
    PushY(memory, cpu);
    ObjectSpawnChild(memory, cpu, 0xe858u);
    cpu->y = PullIndexValue(memory, cpu);
    IncrementY16(cpu);
    IncrementY16(cpu);
    IncrementY16(cpu);
    return OBJECT_FLOW_DISPATCH;
}

/* Object opcode $85: spawn a child and take its animation. */
static ObjectFlow ObjectSpawnChildWithAnimation(const Lufia2Memory *memory,
                                                Lufia2CpuState *cpu) {
    PushY(memory, cpu);
    ObjectSpawnChild(memory, cpu, 0xe863u);
    cpu->y = PullIndexValue(memory, cpu);
    TransferDirectToA(cpu);                                /* E865 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_ACTOR_SLOT)));
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_B), A8(cpu));
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_A)));
    Write8(memory, DirectAddress(cpu, DP_ACTOR_SLOT), A8(cpu));
    ObjectTakeAnimation(memory, cpu, 0xe871u);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_B)));
    Write8(memory, DirectAddress(cpu, DP_ACTOR_SLOT), A8(cpu));
    IncrementY16(cpu);
    IncrementY16(cpu);
    IncrementY16(cpu);
    return OBJECT_FLOW_DISPATCH;
}

/* Object opcode $8A: spawn a child at an offset from the operands. */
static ObjectFlow ObjectSpawnChildAtOffset(const Lufia2Memory *memory,
                                           Lufia2CpuState *cpu) {
    PushY(memory, cpu);
    ObjectSpawnChild(memory, cpu, 0xe87fu);
    cpu->y = PullIndexValue(memory, cpu);
    TransferDirectToA(cpu);                                /* E881 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_A)));
    AslA8(cpu);
    TransferAToX(cpu);
    LoadAAbsolute8(memory, cpu, 0x03u, cpu->y);
    Lufia2SignExtendA8(memory, cpu, 0xe88bu);
    Write16Long(memory, LongIndexedAddress(WRAM_OBJECT_DISPLAY_OFFSET_X, cpu->x),
                cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    /* No TDC: B keeps the first high byte. */
    LoadAAbsolute8(memory, cpu, 0x04u, cpu->y);
    Lufia2SignExtendA8(memory, cpu, 0xe897u);
    Write16Long(memory, LongIndexedAddress(WRAM_OBJECT_DISPLAY_OFFSET_Y, cpu->x),
                cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    IncrementY16(cpu);
    IncrementY16(cpu);
    IncrementY16(cpu);
    IncrementY16(cpu);
    IncrementY16(cpu);
    return OBJECT_FLOW_DISPATCH;
}

/* Object opcode $EE: spawn a child and register it in $7F:D0A6. */
static ObjectFlow ObjectSpawnRegisteredChild(const Lufia2Memory *memory,
                                             Lufia2CpuState *cpu) {
    PushY(memory, cpu);
    ObjectSpawnChild(memory, cpu, 0xe93au);
    cpu->y = PullIndexValue(memory, cpu);
    IncrementY16(cpu);
    IncrementY16(cpu);
    IncrementY16(cpu);
    TransferDirectToA(cpu);                                /* E93F */
    LoadAAbsolute8(memory, cpu, 0x00u, cpu->y);
    Write8(memory, LongIndexedAddress(WRAM_OBJECT_SCREEN_X, cpu->x), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x01u, cpu->y);
    Write8(memory, LongIndexedAddress((WRAM_OBJECT_SCREEN_X + 1u), cpu->x), A8(cpu));
    IncrementY16(cpu);
    IncrementY16(cpu);
    LoadX16(cpu, 0x00u); /* E950 */
    for (;;) {
        LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FD0A6, cpu->x)));
        if (cpu->negative)
            break;
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x08u);
        if (cpu->zero) {
            LoadX16(cpu, 0x00u);
            break;
        }
    }
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_A))); /* E962 */
    Write8(memory, LongIndexedAddress(WRAM_UNK_7FD0A6, cpu->x), A8(cpu));
    return OBJECT_FLOW_DISPATCH;
}

/* Object opcode $0x: despawn the object and leave the script. */
static ObjectFlow ObjectOpDespawn(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!ObjectDespawn(memory, cpu))
        return OBJECT_FLOW_BOUNDARY;
    PullDataBank(memory, cpu);                             /* E141 */
    return OBJECT_FLOW_RETURN;
}

/* $83:E42D: high nibble of A as a signed step. */
static void ObjectHighNibbleStep(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LsrA8(cpu);                                                /* E42D */
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    ObjectSignNibble(memory, cpu, 0xe433u);
    SetAccumulatorWidth(cpu, 1);
    SimulateRtsFrame(memory, cpu);
}

/* $83:E2E0/$83:E331: step an angle every (rate & 15) ticks. */
static void ObjectAngleStep(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t rate,
    uint16_t counter,
    uint16_t angle,
    uint16_t limit,
    uint16_t return_address) {
    LoadAAbsolute8(memory, cpu, rate, cpu->x);
    if (cpu->zero)
        return;
    StepMemory8(memory, cpu, AbsoluteIndexedAddress(cpu, counter, cpu->x), 1);
    And8(cpu, 0x0fu);
    Compare8(cpu, A8(cpu), AbsoluteByte(memory, cpu, counter, cpu->x));
    if (!cpu->zero)
        return;
    StoreZeroAbsolute8(memory, cpu, counter, cpu->x);
    LoadAAbsolute8(memory, cpu, rate, cpu->x);
    ObjectHighNibbleStep(memory, cpu, return_address);
    StoreADirect8(memory, cpu, DP_SCRATCH_B);
    cpu->carry = 0;
    Adc8(cpu, AbsoluteByte(memory, cpu, angle, cpu->x));
    StoreAAbsolute8(memory, cpu, angle, cpu->x);
    LoadAAbsolute8(memory, cpu, limit, cpu->x);
    if (cpu->negative) {
        AslA8(cpu);
        StoreADirect8(memory, cpu, DP_SCRATCH_A);
        LoadA8(cpu, DirectByte(memory, cpu, DP_SCRATCH_B));
        if (cpu->negative) {
            LoadA8(cpu, (uint8_t)~A8(cpu));
            LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
            StoreADirect8(memory, cpu, DP_SCRATCH_B);
        }
        LoadAAbsolute8(memory, cpu, angle, cpu->x);            /* distance */
        cpu->carry = 1;
        Sbc8(cpu, DirectByte(memory, cpu, DP_SCRATCH_A));
        if (cpu->negative) {
            LoadA8(cpu, (uint8_t)~A8(cpu));
            LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        }
        Compare8(cpu, A8(cpu), DirectByte(memory, cpu, DP_SCRATCH_B));
        if (!cpu->carry) {
            LoadA8(cpu, DirectByte(memory, cpu, DP_SCRATCH_A));
            StoreAAbsolute8(memory, cpu, angle, cpu->x);
            StoreZeroAbsolute8(memory, cpu, rate, cpu->x);
        }
    }
    LoadA8(cpu, 0x01u);
    Or8(cpu, AbsoluteByte(memory, cpu, OBJECT_SPIN_UPDATE, cpu->x));
    StoreAAbsolute8(memory, cpu, OBJECT_SPIN_UPDATE, cpu->x);
}

/* STA $211B twice: A sign-extended into the multiplicand. */
static void ObjectMultiplicand(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t value) {
    LoadA8(cpu, value);
    StoreAAbsolute8(memory, cpu, SNES_M7A, 0);
    if (cpu->negative) {
        LoadA8(cpu, 0xffu);
        StoreAAbsolute8(memory, cpu, SNES_M7A, 0);
    } else {
        StoreZeroAbsolute8(memory, cpu, SNES_M7A, 0);
    }
}

/* $83:E73A: product * 2 >> 8, rounded, from $2134/$2135. */
static void ObjectProduct(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, SNES_MPYL, 0));
    AslA16(cpu);
    SetAccumulatorWidth(cpu, 1);
    RolA8(cpu);
    ExchangeAccumulatorBytes(cpu);
    Adc8(cpu, 0x00u);
}

/* $83:E7C2: (|v| << 8) >> 1 in A16, v a signed byte. */
static void ObjectHalfMagnitude(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t offset) {
    TransferDirectToA(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, offset));
    if (cpu->negative) {
        LoadA8(cpu, (uint8_t)~A8(cpu));
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
    }
    ExchangeAccumulatorBytes(cpu);
    SetAccumulatorWidth(cpu, 0);
    LsrA16(cpu);
}

/* DP cells of the spin and zoom projection. */
enum {
    PROJ_DEPTH = 0x51,
    PROJ_COS_B = 0x54,
    PROJ_SIN_B = 0x55,
    PROJ_SIN_A = 0x56,
    PROJ_COS_A = 0x57,
    PROJ_TERM_1 = 0x58,
    PROJ_TERM_2 = 0x59,
    PROJ_TERM_3 = 0x5a,
    PROJ_SCREEN_X = 0x54, /* result words overwrite the cosine and sine */
    PROJ_SCREEN_Y = 0x56
};

/* $83:E703: rotate and project the object's offset. */
static void ObjectProject(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadX8(cpu, AbsoluteByte(memory, cpu, OBJECT_SPIN_ANGLE_B, cpu->y)); /* E703 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(COSINE_TABLE, cpu->x)));
    StoreADirect8(memory, cpu, PROJ_COS_B);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(SINE_TABLE, cpu->x)));
    StoreADirect8(memory, cpu, PROJ_SIN_B);
    LoadX8(cpu, AbsoluteByte(memory, cpu, OBJECT_SPIN_ANGLE_A, cpu->y));
    LoadA8(cpu, Read8(memory, LongIndexedAddress(SINE_TABLE, cpu->x)));
    StoreADirect8(memory, cpu, PROJ_SIN_A);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(COSINE_TABLE, cpu->x)));
    StoreADirect8(memory, cpu, PROJ_COS_A);
    ObjectMultiplicand(memory, cpu,
                       AbsoluteByte(memory, cpu, OBJECT_PROJECTED_X, cpu->y));
    LoadA8(cpu, DirectByte(memory, cpu, PROJ_SIN_B)); /* E733 */
    StoreAAbsolute8(memory, cpu, SNES_M7B, 0);
    ObjectProduct(memory, cpu);
    StoreADirect8(memory, cpu, PROJ_TERM_1);
    LoadA8(cpu, DirectByte(memory, cpu, PROJ_COS_B));
    StoreAAbsolute8(memory, cpu, SNES_M7B, 0);
    ObjectProduct(memory, cpu);
    StoreADirect8(memory, cpu, PROJ_TERM_2);
    ObjectMultiplicand(memory, cpu,
                       AbsoluteByte(memory, cpu, OBJECT_PROJECTED_Y, cpu->y));
    ObjectProduct(memory, cpu);                                /* E76B */
    cpu->carry = 1;
    Sbc8(cpu, DirectByte(memory, cpu, PROJ_TERM_1));
    StoreADirect8(memory, cpu, PROJ_TERM_1);
    LoadA8(cpu, DirectByte(memory, cpu, PROJ_SIN_B));
    StoreAAbsolute8(memory, cpu, SNES_M7B, 0);
    ObjectProduct(memory, cpu);
    Adc8(cpu, DirectByte(memory, cpu, PROJ_TERM_2));
    StoreADirect8(memory, cpu, PROJ_TERM_2);
    ObjectMultiplicand(memory, cpu, DirectByte(memory, cpu, PROJ_TERM_2));
    LoadA8(cpu, DirectByte(memory, cpu, PROJ_SIN_A)); /* E7A2 */
    StoreAAbsolute8(memory, cpu, SNES_M7B, 0);
    ObjectProduct(memory, cpu);
    StoreADirect8(memory, cpu, PROJ_TERM_3);
    LoadA8(cpu, DirectByte(memory, cpu, PROJ_COS_A));
    StoreAAbsolute8(memory, cpu, SNES_M7B, 0);
    LoadAAbsolute8(memory, cpu, SNES_MPYM, 0);
    cpu->carry = 0;
    Adc8(cpu, 0x80u);
    StoreADirect8(memory, cpu, PROJ_DEPTH);        /* depth */
    ObjectHalfMagnitude(memory, cpu, PROJ_TERM_1); /* E7C2 */
    SetIndexWidth(cpu, 0);
    StoreAAbsolute16(memory, cpu, SNES_WRDIVL, 0);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, DirectByte(memory, cpu, PROJ_DEPTH));
    StoreAAbsolute8(memory, cpu, SNES_WRDIVB, 0);
    ObjectHalfMagnitude(memory, cpu, PROJ_TERM_3); /* E7D8 */
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, SNES_RDDIVL, 0));
    StoreAAbsolute16(memory, cpu, SNES_WRDIVL, 0);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, DirectByte(memory, cpu, PROJ_DEPTH));
    StoreAAbsolute8(memory, cpu, SNES_WRDIVB, 0);
    LoadA8(cpu, DirectByte(memory, cpu, PROJ_TERM_1)); /* E7F1 */
    AslA8(cpu);
    SetAccumulatorWidth(cpu, 0);
    TransferXToA(cpu);
    if (cpu->carry) {
        LoadA16(cpu, (uint16_t)~cpu->accumulator);
        IncrementA16(cpu);
    }
    StoreADirect16(memory, cpu, PROJ_SCREEN_X); /* screen x */
    LoadADirect16(memory, cpu, PROJ_TERM_2);
    AslA16(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, SNES_RDDIVL, 0));
    if (cpu->carry) {
        LoadA16(cpu, (uint16_t)~cpu->accumulator);
        IncrementA16(cpu);
    }
    StoreADirect16(memory, cpu, PROJ_SCREEN_Y); /* screen y */
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 1);
    SimulateRtsFrame(memory, cpu);
}

/* $83:E382: rebuild the spin offset when flagged. */
static void ObjectSpinOffset(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadAAbsolute8(memory, cpu, OBJECT_SPIN_UPDATE, cpu->x); /* E382 */
    BitImmediate8(cpu, 0x02u);
    if (!cpu->zero) {
        And8(cpu, 0xfdu);
        StoreAAbsolute8(memory, cpu, OBJECT_SPIN_UPDATE, cpu->x);
        SetIndexWidth(cpu, 1); /* E38E */
        LoadYDirect8(memory, cpu, DP_ACTOR_SLOT);
        LoadAAbsolute8(memory, cpu, OBJECT_ZOOM, cpu->y);
        StoreAAbsolute8(memory, cpu, SNES_M7A, 0);
        StoreZeroAbsolute8(memory, cpu, SNES_M7A, 0);
        LoadX8(cpu, AbsoluteByte(memory, cpu, OBJECT_OFFSET_ANGLE, cpu->y));
        LoadA8(cpu, Read8(memory, LongIndexedAddress(SINE_TABLE, cpu->x)));
        StoreAAbsolute8(memory, cpu, SNES_M7B, 0);
        AslAbsolute8(memory, cpu, SNES_MPYL);
        LoadAAbsolute8(memory, cpu, SNES_MPYM, 0);
        RolA8(cpu);
        StoreAAbsolute8(memory, cpu, OBJECT_PROJECTED_X, cpu->y); /* offset x */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(COSINE_TABLE, cpu->x)));
        StoreAAbsolute8(memory, cpu, SNES_M7B, 0);
        AslAbsolute8(memory, cpu, SNES_MPYL);
        LoadAAbsolute8(memory, cpu, SNES_MPYM, 0);
        RolA8(cpu);
        StoreAAbsolute8(memory, cpu, OBJECT_PROJECTED_Y, cpu->y); /* offset y */
        SetIndexWidth(cpu, 0);
    }
}

/* Per-frame work of opcode $14; true when despawned. */
static bool ObjectSpinZoomStep(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadAAbsolute8(memory, cpu, OBJECT_SPIN_UPDATE, cpu->x);
    BitImmediate8(cpu, 0x08u);
    if (!cpu->zero) {
        LoadAAbsolute8(memory, cpu, OBJECT_ZOOM, cpu->x);
        And8(cpu, 0xfeu);
        if (cpu->zero)
            return true;
    }
    LoadAAbsolute8(memory, cpu, OBJECT_ZOOM_RATE, cpu->x); /* E291 zoom */
    if (!cpu->zero) {
        StepMemory8(memory, cpu,
                    AbsoluteIndexedAddress(cpu, OBJECT_ZOOM_COUNTER, cpu->x), 1);
        And8(cpu, 0x0fu);
        Compare8(cpu, A8(cpu), AbsoluteByte(memory, cpu, OBJECT_ZOOM_COUNTER, cpu->x));
        if (cpu->zero) {
            StoreZeroAbsolute8(memory, cpu, OBJECT_ZOOM_COUNTER, cpu->x);
            LoadAAbsolute8(memory, cpu, OBJECT_ZOOM_RATE, cpu->x);
            ObjectHighNibbleStep(memory, cpu, 0xe2a8u);
            cpu->carry = 0;
            Adc8(cpu, AbsoluteByte(memory, cpu, OBJECT_ZOOM, cpu->x));
            StoreAAbsolute8(memory, cpu, OBJECT_ZOOM, cpu->x);
            LsrA8(cpu);
            StoreADirect8(memory, cpu, DP_SCRATCH_B);
            LoadAAbsolute8(memory, cpu, OBJECT_ZOOM_LIMIT, cpu->x);
            if (cpu->negative) {
                uint8_t clamp;

                And8(cpu, 0x7fu);
                StoreADirect8(memory, cpu, DP_SCRATCH_A);
                LoadAAbsolute8(memory, cpu, OBJECT_ZOOM_RATE, cpu->x);
                if (cpu->negative) {
                    LoadA8(cpu, DirectByte(memory, cpu, DP_SCRATCH_B));
                    Compare8(cpu, A8(cpu), DirectByte(memory, cpu, DP_SCRATCH_A));
                    clamp = cpu->negative;
                } else {
                    LoadA8(cpu, DirectByte(memory, cpu, DP_SCRATCH_B));
                    Compare8(cpu, A8(cpu), DirectByte(memory, cpu, DP_SCRATCH_A));
                    clamp = !cpu->negative;
                }
                if (clamp) {
                    StoreZeroAbsolute8(memory, cpu, OBJECT_ZOOM_RATE,
                                       cpu->x); /* E2CF */
                    LoadA8(cpu, DirectByte(memory, cpu, DP_SCRATCH_A));
                    AslA8(cpu);
                    StoreAAbsolute8(memory, cpu, OBJECT_ZOOM, cpu->x);
                }
            }
            LoadA8(cpu, 0x03u);                                /* E2D8 */
            Or8(cpu, AbsoluteByte(memory, cpu, OBJECT_SPIN_UPDATE, cpu->x));
            StoreAAbsolute8(memory, cpu, OBJECT_SPIN_UPDATE, cpu->x);
        }
    }
    ObjectAngleStep(memory, cpu, OBJECT_SPIN_RATE_A, OBJECT_SPIN_COUNTER_A,
                    OBJECT_SPIN_ANGLE_A, OBJECT_SPIN_LIMIT_A, 0xe2f7u);
    ObjectAngleStep(memory, cpu, OBJECT_SPIN_RATE_B, OBJECT_SPIN_COUNTER_B,
                    OBJECT_SPIN_ANGLE_B, OBJECT_SPIN_LIMIT_B, 0xe348u);
    ObjectSpinOffset(memory, cpu);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);                   /* E3C2 */
    LoadAAbsolute8(memory, cpu, OBJECT_SPIN_UPDATE, cpu->x);
    BitImmediate8(cpu, 0x01u);
    if (cpu->zero)
        return false;
    And8(cpu, 0xfeu);
    StoreAAbsolute8(memory, cpu, OBJECT_SPIN_UPDATE, cpu->x);
    SetIndexWidth(cpu, 1);                                     /* E3D3 */
    LoadYDirect8(memory, cpu, DP_ACTOR_SLOT);
    ObjectProject(memory, cpu, 0xe3d9u);
    LoadAAbsolute8(memory, cpu, OBJECT_SPIN_UPDATE, cpu->y);
    BitImmediate8(cpu, 0x04u);
    if (!cpu->zero) {
        LoadAAbsolute8(memory, cpu, OBJECT_ZOOM_RATE, cpu->y);
        SetIndexWidth(cpu, 0);
        if (cpu->zero) {
            SetAccumulatorWidth(cpu, 0);
            return true;
        }
    }
    SetAccumulatorWidth(cpu, 0);                               /* E3EA */
    SetIndexWidth(cpu, 0);
    LoadXDirect16(memory, cpu, DP_SLOT_WORD_OFFSET);
    LoadADirect16(memory, cpu, DP_SCRATCH_A);
    cpu->carry = 0;
    Add16Value(cpu,
               Read16Long(memory, LongIndexedAddress(WRAM_OBJECT_SCREEN_X, cpu->x)));
    Write16Long(memory, LongIndexedAddress(WRAM_OBJECT_FINE_X, cpu->x),
                cpu->accumulator);
    LoadADirect16(memory, cpu, DP_SCRATCH_C);
    cpu->carry = 0;
    Add16Value(cpu,
               Read16Long(memory, LongIndexedAddress(WRAM_OBJECT_SCREEN_Y, cpu->x)));
    Write16Long(memory, LongIndexedAddress(WRAM_OBJECT_FINE_Y, cpu->x),
                cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);                   /* E406 */
    LoadA8(cpu, DirectByte(memory, cpu, 0x51u));
    And8(cpu, 0xffu);
    Compare8(cpu, A8(cpu), 0x80u);
    LoadA8(cpu, 0x02u);                                        /* behind */
    if (cpu->carry)
        TransferDirectToA(cpu);
    StoreADirect8(memory, cpu, DP_SCRATCH_A);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_OBJECT_DRAW_FLAGS, cpu->x)));
    And8(cpu, 0xfdu);
    Or8(cpu, DirectByte(memory, cpu, DP_SCRATCH_A));
    Write8(memory, LongIndexedAddress(WRAM_OBJECT_DRAW_FLAGS, cpu->x), A8(cpu));
    return false;
}

/* $83:E27D: object opcode $14, spin and zoom. */
static ObjectFlow ObjectSpinZoom(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    bool despawn;

    LoadXDirect(memory, cpu, DP_ACTOR_SLOT); /* E27D */
    PushY(memory, cpu);
    despawn = ObjectSpinZoomStep(memory, cpu);
    SetAccumulatorWidth(cpu, 1);                               /* E421 */
    cpu->y = PullIndexValue(memory, cpu);
    return despawn ? ObjectOpDespawn(memory, cpu) : OBJECT_FLOW_DISPATCH;
}

typedef void (*ObjectCoordinateHelper)(const Lufia2Memory *, Lufia2CpuState *);

enum {
    OBJECT_MOTION_CURRENT_ADDRESS = 0x5d,
    OBJECT_MOTION_CURRENT_BANK = 0x5f,
    OBJECT_MOTION_TARGET_BANK = 0x62
};

static bool ObjectCoordinateCall(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                 uint16_t return_address, ObjectCoordinateHelper helper) {
    uint16_t returned;
    SimulateJsrFrame(memory, cpu, return_address);
    helper(memory, cpu);
    returned = Pull8(memory, cpu);
    returned |= (uint16_t)Pull8(memory, cpu) << 8;
    if (returned == return_address)
        return true;
    cpu->resume_pc = OBJECT_PROGRAM_BANK | (uint16_t)(returned + 1u);
    return false;
}

static ObjectFlow ObjectRefreshMapHeight(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ObjectCoordinateCall(memory, cpu, 0xeff8u, Lufia2ObjectRoundedProbe) ||
        !ObjectCoordinateCall(memory, cpu, 0xeffbu, Lufia2MapProbeHeightBody))
        return OBJECT_FLOW_BOUNDARY;
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    Write8(memory, LongIndexedAddress(WRAM_UNK_7FDA2C, cpu->x), A8(cpu));
    return OBJECT_FLOW_DISPATCH;
}

static ObjectFlow ObjectUpdateHeightDrawFlag(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ObjectCoordinateCall(memory, cpu, 0xe6d5u, Lufia2ObjectRoundedProbe))
        return OBJECT_FLOW_BOUNDARY;
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FD9CC, cpu->x)));
    Compare8(cpu, A8(cpu), 4u);
    if (cpu->zero) {
        uint32_t row = DirectAddress(cpu, DP_PROBE_Y);
        uint8_t next = (uint8_t)(Read8(memory, row) + 1u);
        Write8(memory, row, next);
        SetNz8(cpu, next);
    }
    if (!ObjectCoordinateCall(memory, cpu, 0xe6e4u, Lufia2MapProbeHeightBody))
        return OBJECT_FLOW_BOUNDARY;
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    Compare8(cpu, A8(cpu), Read8(memory, LongIndexedAddress(WRAM_UNK_7FDA2C, cpu->x)));
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_OBJECT_DRAW_FLAGS, cpu->x)));
    And8(cpu, 0xfdu);
    if (!cpu->carry)
        Or8(cpu, 2u);
    Write8(memory, LongIndexedAddress(WRAM_OBJECT_DRAW_FLAGS, cpu->x), A8(cpu));
    return OBJECT_FLOW_DISPATCH;
}

static ObjectFlow ObjectClearMapOccupancy(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_OBJECT_FLAGS, cpu->x)));
    And8(cpu, 0x7fu);
    Write8(memory, LongIndexedAddress(WRAM_OBJECT_FLAGS, cpu->x), A8(cpu));
    if (!ObjectCoordinateCall(memory, cpu, 0xed7bu, Lufia2ObjectRoundedProbe))
        return OBJECT_FLOW_BOUNDARY;
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_PROBE_X)));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_PROBE_Y)));
    Lufia2MapCellIndex(memory, cpu, 0xed83u, 0u);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_FIELD_MAP_ATTRIBUTES, cpu->x)));
    And8(cpu, 0xfdu);
    Write8(memory, LongIndexedAddress(WRAM_FIELD_MAP_ATTRIBUTES, cpu->x), A8(cpu));
    LoadA8(cpu, Read8(memory, WRAM_FIELD_CONTROL_FLAGS));
    And8(cpu, 0xefu);
    Write8(memory, WRAM_FIELD_CONTROL_FLAGS, A8(cpu));
    return OBJECT_FLOW_DISPATCH;
}

static bool ObjectInterpolateAxis(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                  uint32_t position, uint32_t origin,
                                  uint16_t return_address) {
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_BLOCKED_EVENT_OBJECT, 0u));
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(position, cpu->x)));
    Write16Direct(memory, cpu, DP_SCRATCH_C + 2u, cpu->accumulator);
    cpu->carry = true;
    Subtract16(cpu, Read16Long(memory, origin));
    if (cpu->zero) {
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(position, cpu->x)));
        LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
    } else if (!ObjectCoordinateCall(memory, cpu, return_address,
                                     Lufia2ObjectInterpolateCoordinateBody)) {
        return false;
    }
    Write16Long(memory, LongIndexedAddress(position, cpu->x), cpu->accumulator);
    return true;
}

static ObjectFlow ObjectInterpolatePosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_OBJECT_SCREEN_X, cpu->x)));
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_C), A8(cpu));
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_C + 1u), 0u);
    SetAccumulatorWidth(cpu, 0);
    if (!ObjectInterpolateAxis(memory, cpu, WRAM_OBJECT_FINE_X, WRAM_ACTOR_FINE_X, 0xe5dbu) ||
        !ObjectInterpolateAxis(memory, cpu, WRAM_OBJECT_FINE_Y, WRAM_ACTOR_FINE_Y, 0xe5fau))
        return OBJECT_FLOW_BOUNDARY;
    SetAccumulatorWidth(cpu, 1);
    IncrementY16(cpu);
    IncrementY16(cpu);
    return OBJECT_FLOW_DISPATCH;
}

static bool ObjectApproachAxis(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                               uint32_t position, uint32_t target,
                               uint16_t return_address) {
    LoadA16(cpu, cpu->x);
    cpu->carry = false;
    Add16Value(cpu, (uint16_t)position);
    Write16Direct(memory, cpu, OBJECT_MOTION_CURRENT_ADDRESS, cpu->accumulator);
    LoadA16(cpu, (uint16_t)target);
    return ObjectCoordinateCall(memory, cpu, return_address,
                                Lufia2ObjectApproachCoordinateBody);
}

static ObjectFlow ObjectApproachPosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
    LoadA8(cpu, 0x7fu);
    Write8(memory, DirectAddress(cpu, OBJECT_MOTION_CURRENT_BANK), A8(cpu));
    Write8(memory, DirectAddress(cpu, OBJECT_MOTION_TARGET_BANK), A8(cpu));
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_OBJECT_SCREEN_X, cpu->x)));
    Or8(cpu, Read8(memory, LongIndexedAddress(WRAM_OBJECT_SCREEN_X + 1u, cpu->x)));
    SetAccumulatorWidth(cpu, 0);
    cpu->zero = (cpu->accumulator & 0x80u) == 0;
    if (!cpu->zero) {
        LoadA16(cpu, cpu->accumulator | 0xff00u);
        LoadA16(cpu, cpu->accumulator ^ 0xffffu);
        LoadA16(cpu, (uint16_t)(cpu->accumulator + 1u));
    }
    Write16Direct(memory, cpu, DP_SCRATCH_A, cpu->accumulator);
    if (!ObjectApproachAxis(memory, cpu, WRAM_OBJECT_FINE_X, WRAM_ACTOR_FINE_X, 0xe67du) ||
        !ObjectApproachAxis(memory, cpu, WRAM_OBJECT_FINE_Y, WRAM_ACTOR_FINE_Y, 0xe68au))
        return OBJECT_FLOW_BOUNDARY;
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_OBJECT_FINE_X, cpu->x)));
    Compare16(cpu, cpu->accumulator, Read16Long(memory, WRAM_ACTOR_FINE_X));
    if (cpu->zero) {
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_OBJECT_FINE_Y, cpu->x)));
        Compare16(cpu, cpu->accumulator, Read16Long(memory, WRAM_ACTOR_FINE_Y));
        if (cpu->zero) {
            LoadY16(cpu, (uint16_t)(cpu->y - 1u));
            return ObjectJump(memory, cpu, OBJECT_OP_DX);
        }
    }
    IncrementY16(cpu);
    IncrementY16(cpu);
    SetAccumulatorWidth(cpu, 1);
    return OBJECT_FLOW_DISPATCH;
}


static void ObjectCopyGraphicsRow(const Lufia2Memory *memory,
                                  Lufia2CpuState *cpu, uint16_t last_byte) {
    OpLoadA(cpu, last_byte);
    OpMoveNext(memory, cpu, 0x7eu, 0x7eu);
}

static ObjectFlow ObjectCopyFrameGraphics(const Lufia2Memory *memory,
                                          Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SLOT_RECORD_OFFSET)));
    OpLda(memory, cpu, OpAbsY(cpu, 2u));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_FRAME_POINTER));
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_C));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_FRAME_POINTER + 2u));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_OBJECT_STATE));
    OpAndValue(cpu, 0xfcu);
    OpOraValue(cpu, 0x20u);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_OBJECT_STATE));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_OBJECT_SPRITE_SIZE));
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    OpStz(memory, cpu, OpDp(cpu, DP_SCRATCH_A + 1u));
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0xa000u);
    cpu->carry = false;
    OpAdc(memory, cpu, OpAbsY(cpu, 0u));
    OpTax(cpu);
    PushDataBank(memory, cpu);
    PushY(memory, cpu);
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SCRATCH_C)));
    OpLda(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    if (cpu->zero) {
        ObjectCopyGraphicsRow(memory, cpu, 0x3fu);
        OpTxa(cpu);
        cpu->carry = false;
        OpAdcValue(cpu, 0x1c0u);
        OpTax(cpu);
        ObjectCopyGraphicsRow(memory, cpu, 0x3fu);
    } else {
        for (unsigned row = 0; row < 4; ++row) {
            ObjectCopyGraphicsRow(memory, cpu, 0x7fu);
            if (row == 3)
                break;
            OpTxa(cpu);
            if (row == 1) {
                cpu->carry = true;
                OpSbcValue(cpu, 0x280u);
            } else {
                cpu->carry = false;
                OpAdcValue(cpu, 0x380u);
            }
            OpTax(cpu);
        }
    }
    cpu->y = PullIndexValue(memory, cpu);
    PullDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    for (unsigned operand = 0; operand < 4; ++operand)
        OpIny(cpu);
    return OBJECT_FLOW_DISPATCH;
}

static ObjectFlow ObjectQueueSound(const Lufia2Memory *memory,
                                   Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbsY(cpu, 0u));
    SimulateJslFrame(memory, cpu, 0x83u, 0xeff1u);
    Lufia2QueueDeferredSound(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    OpIny(cpu);
    return OBJECT_FLOW_DISPATCH;
}


static bool ObjectProbeLongCall(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t back, Lufia2ExecutionResult (*helper)(const Lufia2Memory *, Lufia2CpuState *)) {
    SimulateJslFrame(memory, cpu, 0x83u, back);
    Lufia2ExecutionResult result = helper(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return false;
    SimulateRtlFrame(memory, cpu);
    return true;
}

static ObjectFlow ObjectProbeDirectionJump(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ObjectCoordinateCall(memory, cpu, 0xeeeeu, Lufia2ObjectRoundedProbe))
        return OBJECT_FLOW_BOUNDARY;
    LoadAAbsolute8(memory, cpu, 0u, cpu->y);
    SimulateJslFrame(memory, cpu, 0x83u, 0xeef5u);
    uint32_t movement = Lufia2ActorMovementStep(memory, cpu);
    if (!movement) {
        return OBJECT_FLOW_BOUNDARY;
    }
    SimulateRtlFrame(memory, cpu);
    PushY(memory, cpu);
    if (!ObjectProbeLongCall(memory, cpu, 0xeefau, Lufia2FieldProbeObjectState))
        return OBJECT_FLOW_BOUNDARY;
    cpu->y = PullIndexValue(memory, cpu);
    BitImmediate8(cpu, 2u);
    if (cpu->zero) {
        PushY(memory, cpu);
        LoadX16(cpu, 0x1au);
        LoadY16(cpu, 3u);
        if (!ObjectProbeLongCall(memory, cpu, 0xef0au, Lufia2FieldFindPointRecord))
            return OBJECT_FLOW_BOUNDARY;
        cpu->y = PullIndexValue(memory, cpu);
        if (!cpu->carry) {
            if (!ObjectCoordinateCall(memory, cpu, 0xef10u, Lufia2ObjectRoundedProbe))
                return OBJECT_FLOW_BOUNDARY;
            LoadAAbsolute8(memory, cpu, 0u, cpu->y);
            SimulateJsrFrame(memory, cpu, 0xef16u);
            Lufia2ExecutionResult result = Lufia2FieldProbeDirectionBlocked(memory, cpu);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return OBJECT_FLOW_BOUNDARY;
            SimulateRtsFrame(memory, cpu);
            if (!cpu->zero)
                return ObjectJump(memory, cpu, OBJECT_OP_DX);
        }
    }
    IncrementY16(cpu);
    IncrementY16(cpu);
    IncrementY16(cpu);
    return OBJECT_FLOW_DISPATCH;
}

static bool ObjectStartProbeEvent(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SimulateJslFrame(memory, cpu, 0x83u, 0xf1f1u);
    cpu->program_bank = 0x80u;
    Lufia2ExecutionResult result = Lufia2FieldStartEventAtProbe(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return false;
    SimulateRtlFrame(memory, cpu);
    cpu->program_bank = 0x83u;
    return true;
}

static ObjectFlow ObjectEventProbeJump(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint8_t mode) {
    LoadA8(cpu, mode);
    Write8(memory, WRAM_UNK_7FD133, A8(cpu));
    if (!ObjectCoordinateCall(memory, cpu, 0xf1c4u, Lufia2ObjectRoundedProbe))
        return OBJECT_FLOW_BOUNDARY;
    LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FD9CC, cpu->x)));
    SimulateJslFrame(memory, cpu, 0x83u, 0xf1ceu);
    if (!Lufia2ActorMovementStep(memory, cpu))
        return OBJECT_FLOW_BOUNDARY;
    SimulateRtlFrame(memory, cpu);
    PushY(memory, cpu);
    LoadX16(cpu, 0x1eu);
    LoadY16(cpu, 3u);
    if (!ObjectProbeLongCall(memory, cpu, 0xf1d9u, Lufia2FieldFindPointRecord))
        return OBJECT_FLOW_BOUNDARY;
    if (cpu->carry) {
        if (!ObjectCoordinateCall(memory, cpu, 0xf1deu, Lufia2MapProbeHeightBody))
            return OBJECT_FLOW_BOUNDARY;
        LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);
        Compare8(cpu, A8(cpu), Read8(memory,
            LongIndexedAddress(WRAM_UNK_7FDA2C, cpu->x)));
        cpu->carry = 0;
        if (cpu->zero) {
            LoadX16(cpu, 0x1eu);
            LoadY16(cpu, 6u);
            if (!ObjectStartProbeEvent(memory, cpu))
                return OBJECT_FLOW_BOUNDARY;
            cpu->carry = !cpu->carry;
        }
    }
    cpu->y = PullIndexValue(memory, cpu);
    LoadY16(cpu, (uint16_t)(cpu->y - 1u));
    if (cpu->carry)
        return ObjectJump(memory, cpu, OBJECT_OP_DX);
    IncrementY16(cpu);
    IncrementY16(cpu);
    IncrementY16(cpu);
    return OBJECT_FLOW_DISPATCH;
}

static bool ObjectEventReturn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t back, uint8_t frame_size) {
    uint8_t low = Pull8(memory, cpu);
    uint8_t high = Pull8(memory, cpu);
    uint8_t bank = frame_size == 3u ? Pull8(memory, cpu) : 0x83u;
    uint16_t actual = (uint16_t)(low | ((uint16_t)high << 8));
    cpu->program_bank = bank;
    if (actual == back && bank == 0x83u)
        return true;
    cpu->resume_pc = ((uint32_t)bank << 16) | (uint16_t)(actual + 1u);
    return false;
}

static bool ObjectStartHeaderEvent(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t record_offset, uint16_t back) {
    OpLdx(cpu, 0u);
    OpLdy(cpu, record_offset);
    SimulateJslFrame(memory, cpu, 0x83u, back);
    cpu->program_bank = 0x80u;
    Lufia2ExecutionResult result = Lufia2FieldStartEvent(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return false;
    return ObjectEventReturn(memory, cpu, back, 3u);
}

static ObjectFlow ObjectStartLeaderEvent(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushY(memory, cpu);
    SimulateJsrFrame(memory, cpu, 0xe261u);
    Lufia2ExecutionResult result = Lufia2FieldProbeLeaderPosition(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED ||
        !ObjectEventReturn(memory, cpu, 0xe261u, 2u) ||
        !ObjectStartHeaderEvent(memory, cpu, 0x12u, 0xe26bu))
        return OBJECT_FLOW_BOUNDARY;
    OpPullY(memory, cpu);
    return OBJECT_FLOW_DISPATCH;
}

static ObjectFlow ObjectStartPositionEvent(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushY(memory, cpu);
    if (!ObjectCoordinateCall(memory, cpu, 0xeaa6u, Lufia2ObjectRoundedProbe) ||
        !ObjectStartHeaderEvent(memory, cpu, 0x0au, 0xeab0u))
        return OBJECT_FLOW_BOUNDARY;
    OpPullY(memory, cpu);
    return OBJECT_FLOW_DISPATCH;
}

static ObjectFlow ObjectStartModeEvent(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ObjectCoordinateCall(memory, cpu, 0xedaau, Lufia2ObjectRoundedProbe))
        return OBJECT_FLOW_BOUNDARY;
    OpLda(memory, cpu, OpAbsY(cpu, 0u));
    OpSta(memory, cpu, WRAM_UNK_7FD133);
    PushY(memory, cpu);
    if (!ObjectStartHeaderEvent(memory, cpu, 0x1au, 0xedbcu))
        return OBJECT_FLOW_BOUNDARY;
    OpPullY(memory, cpu);
    OpIny(cpu);
    return OBJECT_FLOW_DISPATCH;
}

static bool ObjectFollowerOffsets(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t back) {
    SimulateJslFrame(memory, cpu, 0x83u, back);
    Lufia2ActorRecordOffsets(memory, cpu);
    return ObjectEventReturn(memory, cpu, back, 3u);
}

static ObjectFlow ObjectClearFollowers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushY(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT));
    PushAccumulator8(memory, cpu);
    OpLdx(cpu, 7u);
    do {
        OpLda(memory, cpu, OpLongX(cpu, WRAM_UNK_7FD0A6));
        OpCmpValue(cpu, 0xffu);
        if (!cpu->zero) {
            OpSta(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT));
            OpLoadA(cpu, 0xffu);
            OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7FD0A6));
            OpPushX(memory, cpu);
            if (!ObjectFollowerOffsets(memory, cpu, 0xe986u))
                return OBJECT_FLOW_BOUNDARY;
            SimulateJslFrame(memory, cpu, 0x83u, 0xe98au);
            Lufia2ExecutionResult result = Lufia2ObjectRemoveSlot(memory, cpu);
            if (result.flow != LUFIA2_EXECUTION_RETURNED ||
                !ObjectEventReturn(memory, cpu, 0xe98au, 3u))
                return OBJECT_FLOW_BOUNDARY;
            OpPullX(memory, cpu);
        }
        OpDex(cpu);
    } while (!cpu->negative);
    LoadA8(cpu, Pull8(memory, cpu));
    OpSta(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT));
    if (!ObjectFollowerOffsets(memory, cpu, 0xe995u))
        return OBJECT_FLOW_BOUNDARY;
    OpPullY(memory, cpu);
    return OBJECT_FLOW_DISPATCH;
}

typedef Lufia2ExecutionResult (*ObjectCollisionStep)(
    const Lufia2Memory *, Lufia2CpuState *);

static bool ObjectCollisionReturn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t back, uint8_t frame_size) {
    uint8_t low = Pull8(memory, cpu);
    uint8_t high = Pull8(memory, cpu);
    uint8_t bank = frame_size == 3u ? Pull8(memory, cpu) : 0x83u;
    uint16_t actual = (uint16_t)(low | ((uint16_t)high << 8));
    cpu->program_bank = bank;
    if (actual == back && bank == 0x83u)
        return true;
    cpu->resume_pc = ((uint32_t)bank << 16) | (uint16_t)(actual + 1u);
    return false;
}

static bool ObjectCollisionCall(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    ObjectCollisionStep step, uint8_t bank, uint16_t back, uint8_t frame_size) {
    if (frame_size == 3u)
        SimulateJslFrame(memory, cpu, 0x83u, back);
    else
        SimulateJsrFrame(memory, cpu, back);
    cpu->program_bank = bank;
    Lufia2ExecutionResult result = step(memory, cpu);
    return result.flow == LUFIA2_EXECUTION_RETURNED &&
        ObjectCollisionReturn(memory, cpu, back, frame_size);
}

static void ObjectCollisionSetPendingPosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, DP_PROBE_X));
    OpSta(memory, cpu, WRAM_FIELD_PENDING_OBJECT_X);
    OpLda(memory, cpu, OpDp(cpu, DP_PROBE_Y));
    OpSta(memory, cpu, WRAM_FIELD_PENDING_OBJECT_Y);
}

static ObjectFlow ObjectApplyProbedRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ObjectCoordinateCall(memory, cpu, 0xeb0eu, Lufia2ObjectRoundedProbe))
        return OBJECT_FLOW_BOUNDARY;
    PushY(memory, cpu);
    if (!ObjectCoordinateCall(memory, cpu, 0xeb12u, Lufia2MapProbeHeightBody))
        return OBJECT_FLOW_BOUNDARY;
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpCmp(memory, cpu, OpLongX(cpu, WRAM_UNK_7FDA2C));
    if (cpu->zero) {
        ObjectCollisionSetPendingPosition(memory, cpu);
        if (!ObjectCollisionCall(memory, cpu, Lufia2FieldProbeObjectState,
            0x83u, 0xeb2au, 3u))
            return OBJECT_FLOW_BOUNDARY;
        OpBitValue(cpu, 0x20u);
        if (!cpu->zero) {
            OpLoadA(cpu, 1u);
            if (!ObjectCollisionCall(memory, cpu, Lufia2FieldApplyObjectRecord,
                0x83u, 0xeb33u, 2u))
                return OBJECT_FLOW_BOUNDARY;
        }
    }
    OpPullY(memory, cpu);
    return OBJECT_FLOW_DISPATCH;
}

static bool ObjectCollisionProbeEvent(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t header_offset, uint16_t back) {
    OpLdx(cpu, 0u);
    OpLdy(cpu, header_offset);
    return ObjectCollisionCall(memory, cpu, Lufia2FieldStartEvent,
        0x80u, back, 3u);
}

static ObjectFlow ObjectWakeOrApplyRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ObjectCoordinateCall(memory, cpu, 0xef24u, Lufia2ObjectRoundedProbe))
        return OBJECT_FLOW_BOUNDARY;
    PushY(memory, cpu);
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, WRAM_UNK_7FD133);
    OpLdx(cpu, 0x1au);
    OpLdy(cpu, 8u);
    if (!ObjectCollisionCall(memory, cpu, Lufia2FieldStartEventAtProbe,
            0x80u, 0xef35u, 3u) ||
        !ObjectCollisionProbeEvent(memory, cpu, 0x10u, 0xef3fu))
        return OBJECT_FLOW_BOUNDARY;
    OpPullY(memory, cpu);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_OBJECT_FLAGS));
    OpBitValue(cpu, 0x80u);
    if (cpu->zero) {
        if (!ObjectCollisionCall(memory, cpu, Lufia2ObjectWakeMatchingPosition,
            0x83u, 0xef4du, 2u))
            return OBJECT_FLOW_BOUNDARY;
        if (cpu->carry)
            return OBJECT_FLOW_DISPATCH;
    }
    if (!ObjectCollisionCall(memory, cpu, Lufia2FieldProbeObjectState,
        0x83u, 0xef53u, 3u))
        return OBJECT_FLOW_BOUNDARY;
    OpBitValue(cpu, 2u);
    if (!cpu->zero) {
        PushY(memory, cpu);
        ObjectCollisionSetPendingPosition(memory, cpu);
        OpLoadA(cpu, 2u);
        if (!ObjectCollisionCall(memory, cpu, Lufia2FieldApplyObjectRecord,
            0x83u, 0xef69u, 2u))
            return OBJECT_FLOW_BOUNDARY;
        OpPullY(memory, cpu);
    }
    return OBJECT_FLOW_DISPATCH;
}

static bool ObjectCollisionMoveProbe(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t back) {
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_UNK_7FD9CC));
    SimulateJslFrame(memory, cpu, 0x83u, back);
    if (!Lufia2ActorMovementStep(memory, cpu))
        return false;
    return ObjectCollisionReturn(memory, cpu, back, 3u);
}

static ObjectFlow ObjectCollisionSkipJump(Lufia2CpuState *cpu) {
    OpIny(cpu);
    OpIny(cpu);
    return OBJECT_FLOW_DISPATCH;
}

static bool ObjectCollisionActorCall(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    void (*step)(const Lufia2Memory *, Lufia2CpuState *), uint16_t back) {
    SimulateJslFrame(memory, cpu, 0x83u, back);
    step(memory, cpu);
    return ObjectCollisionReturn(memory, cpu, back, 3u);
}

static ObjectFlow ObjectTriggerLeaderReaction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ObjectCoordinateCall(memory, cpu, 0xea39u, Lufia2ObjectRoundedProbe))
        return OBJECT_FLOW_BOUNDARY;
    OpLda(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT));
    PushAccumulator8(memory, cpu);
    OpStz(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT));
    if (!ObjectCollisionActorCall(memory, cpu, Lufia2ActorRecordOffsets, 0xea42u) ||
        !ObjectCollisionActorCall(memory, cpu, Lufia2ActorClearMapOccupancy, 0xea46u))
        return OBJECT_FLOW_BOUNDARY;
    OpLoadA(cpu, 0x12u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E070A));
    OpLda(memory, cpu, OpDp(cpu, DP_PROBE_X));
    OpSta(memory, cpu, WRAM_ACTOR_CLAIMED_OBJECT_RECORD);
    OpLda(memory, cpu, OpDp(cpu, DP_PROBE_Y));
    OpSta(memory, cpu, WRAM_ACTOR_CLAIMED_PENDING_OBJECT);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_STATE));
    OpOraValue(cpu, 0x0au);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_ACTOR_STATE));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_FLAGS));
    OpOraValue(cpu, 2u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_ACTOR_FLAGS));
    OpLoadA(cpu, 0x10u);
    OpSta(memory, cpu, WRAM_UNK_7FE4DE);
    if (!ObjectCollisionActorCall(memory, cpu, Lufia2ActorLoadPrimaryScript, 0xea71u))
        return OBJECT_FLOW_BOUNDARY;
    LoadA8(cpu, Pull8(memory, cpu));
    OpSta(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT));
    if (!ObjectCollisionActorCall(memory, cpu, Lufia2ActorRecordOffsets, 0xea78u))
        return OBJECT_FLOW_BOUNDARY;
    OpDey(cpu);
    return ObjectJump(memory, cpu, OBJECT_OP_DX);
}

static ObjectFlow ObjectReactToMapRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ObjectCoordinateCall(memory, cpu, 0xe9d6u, Lufia2ObjectRoundedProbe) ||
        !ObjectCollisionMoveProbe(memory, cpu, 0xe9e0u) ||
        !ObjectCoordinateCall(memory, cpu, 0xe9e3u, Lufia2MapProbeHeightBody))
        return OBJECT_FLOW_BOUNDARY;
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpCmp(memory, cpu, OpLongX(cpu, WRAM_UNK_7FDA2C));
    if (!cpu->zero)
        return ObjectCollisionSkipJump(cpu);
    Lufia2MapCellIndex(memory, cpu, 0xe9eeu, 1u);
    OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_MAP_ATTRIBUTES));
    OpBitValue(cpu, 0x40u);
    if (!cpu->zero) {
        IncrementDirect8(memory, cpu, DP_PROBE_Y);
        if (!ObjectCollisionCall(memory, cpu, Lufia2FieldFindPendingObject,
            0x83u, 0xe9fbu, 2u))
            return OBJECT_FLOW_BOUNDARY;
        OpRepWidths(cpu, 0x20u);
        OpTxa(cpu);
        OpAslA(cpu);
        OpAslA(cpu);
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_PENDING_OBJECT_TILES));
    } else {
        OpRepWidths(cpu, 0x20u);
        OpTxa(cpu);
        OpAslA(cpu);
        OpAdc(memory, cpu, WRAM_FIELD_LAYER_CELL_BASE + 2u);
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, 0x7f0000u));
    }
    OpAndValue(cpu, 0x03ffu);
    cpu->carry = 0;
    OpAdc(memory, cpu, WRAM_FIELD_METATILE_ATTRIBUTE_BASE);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x7f0000u));
    OpBitValue(cpu, 0xf0u);
    if (cpu->zero)
        return ObjectCollisionSkipJump(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_MAP_RECORD_BYTE8 - 16u));
    OpBitValue(cpu, 4u);
    if (cpu->zero)
        return ObjectCollisionSkipJump(cpu);
    return ObjectTriggerLeaderReaction(memory, cpu);
}

static ObjectFlow ObjectReactToProbedActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ObjectCoordinateCall(memory, cpu, 0xf258u, Lufia2ObjectRoundedProbe) ||
        !ObjectCollisionMoveProbe(memory, cpu, 0xf262u))
        return OBJECT_FLOW_BOUNDARY;
    Lufia2MapCellIndex(memory, cpu, 0xf265u, 1u);
    OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_MAP_ATTRIBUTES));
    OpBitValue(cpu, 1u);
    if (cpu->zero)
        return ObjectCollisionSkipJump(cpu);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpWriteX(memory, cpu, OpDp(cpu, DP_SCRATCH_C), cpu->x);
    OpLda(memory, cpu, OpLongX(cpu, WRAM_UNK_7FDA2C));
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    if (!ObjectCollisionCall(memory, cpu, Lufia2FieldFindActorAtProbe,
        0x83u, 0xf27au, 2u))
        return OBJECT_FLOW_BOUNDARY;
    if (!cpu->carry)
        return ObjectCollisionSkipJump(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E05D2));
    OpCmpValue(cpu, 0x70u);
    if (cpu->zero)
        return ObjectCollisionSkipJump(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_X));
    OpSta(memory, cpu, OpDp(cpu, DP_PROBE_X));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_Y));
    OpSta(memory, cpu, OpDp(cpu, DP_PROBE_Y));
    OpPushX(memory, cpu);
    if (!ObjectCoordinateCall(memory, cpu, 0xf291u, Lufia2MapProbeHeightBody))
        return OBJECT_FLOW_BOUNDARY;
    OpPullX(memory, cpu);
    OpCmp(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    if (!cpu->zero)
        return ObjectCollisionSkipJump(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_ID));
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    PushY(memory, cpu);
    OpPushX(memory, cpu);
    if (!ObjectCollisionCall(memory, cpu, Lufia2SceneScriptSelectRecord,
        0x80u, 0xf2a1u, 3u))
        return OBJECT_FLOW_BOUNDARY;
    OpPullX(memory, cpu);
    OpPullY(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09B7));
    OpCmpValue(cpu, 0xffffu);
    OpSepWidths(cpu, 0x20u);
    if (cpu->zero) {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_FLAGS));
        OpOraValue(cpu, 4u);
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_FLAGS));
        OpLoadA(cpu, 9u);
        OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_PRIMARY_TIMER));
    }
    OpDey(cpu);
    return ObjectJump(memory, cpu, OBJECT_OP_DX);
}

static bool ObjectProbeMovementBoundary(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpLongX(cpu, WRAM_UNK_7FD9CC));
    OpTax(cpu);
    uint16_t target = Read16ProgramIndexed(memory, cpu, 0xf09bu, cpu->x);
    SimulateJsrFrame(memory, cpu, 0xf012u);
    switch (target) {
    case 0xf0a3u:
        OpLda(memory, cpu, OpDp(cpu, DP_PROBE_Y));
        OpCmp(memory, cpu, OpAbs(cpu, 0x05bbu));
        break;
    case 0xf0aau:
        OpLda(memory, cpu, OpDp(cpu, DP_PROBE_X));
        OpCmp(memory, cpu, OpAbs(cpu, 0x05b9u));
        break;
    case 0xf0b1u:
        OpLda(memory, cpu, OpDp(cpu, DP_PROBE_X));
        break;
    case 0xf0b5u:
        OpLda(memory, cpu, OpDp(cpu, DP_PROBE_Y));
        break;
    default:
        cpu->resume_pc = OBJECT_PROGRAM_BANK | target;
        return false;
    }
    cpu->carry = 0;
    if (cpu->zero)
        cpu->carry = 1;
    return ObjectCollisionReturn(memory, cpu, 0xf012u, 2u);
}

static ObjectFlow ObjectMovementScriptJump(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpDey(cpu);
    return ObjectJump(memory, cpu, OBJECT_OP_DX);
}

static ObjectFlow ObjectMovementMapCheck(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2MapCellIndex(memory, cpu, 0xf06bu, 1u);
    OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_MAP_ATTRIBUTES));
    OpBitValue(cpu, 4u);
    if (!cpu->zero)
        return ObjectMovementScriptJump(memory, cpu);
    return ObjectCollisionSkipJump(cpu);
}

static ObjectFlow ObjectMovementStartEvent(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushY(memory, cpu);
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, WRAM_UNK_7FD133);
    OpLda(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
    OpBitValue(cpu, 8u);
    if (cpu->zero) {
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, WRAM_UNK_7FD133);
    }
    if (!ObjectCollisionProbeEvent(memory, cpu, 8u, 0xf097u))
        return OBJECT_FLOW_BOUNDARY;
    OpPullY(memory, cpu);
    return ObjectMovementScriptJump(memory, cpu);
}

static ObjectFlow ObjectReactToMovement(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ObjectCoordinateCall(memory, cpu, 0xf007u, Lufia2ObjectRoundedProbe) ||
        !ObjectProbeMovementBoundary(memory, cpu))
        return OBJECT_FLOW_BOUNDARY;
    if (cpu->carry)
        return ObjectMovementMapCheck(memory, cpu);
    OpLda(memory, cpu, 0x0009a7u);
    OpBitValue(cpu, 0x20u);
    if (!cpu->zero) {
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
        OpLda(memory, cpu, OpLongX(cpu, WRAM_UNK_7FD9CC));
        OpCmpValue(cpu, 0u);
        if (cpu->zero) {
            OpLoadA(cpu, 0x30u);
            if (!ObjectCollisionCall(memory, cpu, Lufia2FieldSetFollowingObjectDrawFlags,
                0x83u, 0xf02bu, 2u))
                return OBJECT_FLOW_BOUNDARY;
            return ObjectMovementMapCheck(memory, cpu);
        }
    }
    if (!ObjectCollisionMoveProbe(memory, cpu, 0xf038u) ||
        !ObjectCollisionCall(memory, cpu, Lufia2FieldProbeObjectAttributes,
            0x83u, 0xf03cu, 3u))
        return OBJECT_FLOW_BOUNDARY;
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    OpLda(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
    OpBitValue(cpu, 4u);
    bool compare_height = false;
    if (!cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
        OpBitValue(cpu, 4u);
        compare_height = !cpu->zero;
    }
    if (!compare_height) {
        OpLda(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
        OpBitValue(cpu, 2u);
        if (!cpu->zero)
            return ObjectMovementStartEvent(memory, cpu);
    }
    if (!ObjectCoordinateCall(memory, cpu, 0xf056u, Lufia2MapProbeHeightBody))
        return OBJECT_FLOW_BOUNDARY;
    OpCmpValue(cpu, 3u);
    if (cpu->zero)
        return ObjectMovementScriptJump(memory, cpu);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpCmp(memory, cpu, OpLongX(cpu, WRAM_UNK_7FDA2C));
    if (cpu->zero)
        return ObjectMovementMapCheck(memory, cpu);
    if (!cpu->carry)
        return ObjectCollisionSkipJump(cpu);
    return ObjectMovementScriptJump(memory, cpu);
}

static void ObjectCopyCustomPalette(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpLongX(cpu, 0x83f408u));
    OpTax(cpu);
    OpLdy(cpu, 0x0500u);
    OpLoadA(cpu, 31u);
    PushDataBank(memory, cpu);
    do {
        uint8_t color_byte = Read8(memory, 0x960000u | cpu->x);
        Write8(memory, 0x960000u | cpu->y, color_byte);
        cpu->x = (uint16_t)(cpu->x + 1u);
        cpu->y = (uint16_t)(cpu->y + 1u);
        cpu->accumulator = (uint16_t)(cpu->accumulator - 1u);
        cpu->data_bank = 0x96u;
    } while (cpu->accumulator != 0xffffu);
    PullDataBank(memory, cpu);
}

static bool ObjectLoadCustomGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSta(memory, cpu, 0x7fd4f5u);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpAbsY(cpu, 0u));
    SetAccumulatorWidth(cpu, 0u);
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    OpTax(cpu);
    ObjectCopyCustomPalette(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x83f400u));
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    OpLoadA(cpu, 0xa000u);
    OpSta(memory, cpu, OpDp(cpu, 0x60u));
    SetAccumulatorWidth(cpu, 1u);
    OpLoadA(cpu, 2u);
    OpTestBits(memory, cpu, OpDp(cpu, 0x73u), 1u);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, 0x62u));
    return ObjectCollisionCall(memory, cpu, Lufia2DecompressResource,
        0x80u, 0xe4e0u, 3u);
}

static ObjectFlow ObjectSetCustomGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!(cpu->data_bank & 0x40u) && cpu->y >= 0x1fffu &&
        cpu->y < 0x8000u) {
        cpu->resume_pc = 0x83e487u;
        return OBJECT_FLOW_BOUNDARY;
    }
    if (Read8(memory, OpAbsY(cpu, 0u)) >= 4u ||
        Read8(memory, OpAbsY(cpu, 1u)) >= 4u) {
        cpu->resume_pc = 0x83e487u;
        return OBJECT_FLOW_BOUNDARY;
    }
    TransferDirectToA(cpu);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLda(memory, cpu, OpAbsY(cpu, 0u));
    OpOraValue(cpu, 0x80u);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_ANIMATION_ID));
    OpLda(memory, cpu, OpAbsY(cpu, 1u));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_SPRITE_SIZE));
    PushY(memory, cpu);
    OpLda(memory, cpu, OpLongX(cpu, WRAM_OBJECT_ANIMATION_ID));
    OpAndValue(cpu, 0x7fu);
    OpCmp(memory, cpu, 0x7fd4f5u);
    if (!cpu->zero && !ObjectLoadCustomGraphics(memory, cpu))
        return OBJECT_FLOW_BOUNDARY;
    SetAccumulatorWidth(cpu, 0u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 0xabu)));
    OpLoadA(cpu, 0xa000u);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_ANIMATION_POINTER));
    SetAccumulatorWidth(cpu, 1u);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_ANIMATION_POINTER + 2u));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLoadA(cpu, 7u);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_ANIMATION_FLAGS));
    if (!ObjectCollisionCall(memory, cpu, Lufia2ObjectAllocateSpriteResources,
        0x83u, 0xe4feu, 2u))
        return OBJECT_FLOW_BOUNDARY;
    OpPullY(memory, cpu);
    OpIny(cpu);
    OpIny(cpu);
    return OBJECT_FLOW_DISPATCH;
}

static ObjectFlow ObjectFinishInteraction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, bool jump) {
    if (!ObjectCollisionCall(memory, cpu, Lufia2ObjectStartInteractionEvent,
        0x83u, jump ? 0xec4au : 0xebfdu, 2u))
        return OBJECT_FLOW_BOUNDARY;
    if (jump)
        return ObjectMovementScriptJump(memory, cpu);
    return ObjectCollisionSkipJump(cpu);
}

static ObjectFlow ObjectActivateInteractionRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushY(memory, cpu);
    OpLoadA(cpu, 3u);
    if (!ObjectCollisionCall(memory, cpu, Lufia2FieldApplyObjectRecord,
            0x83u, 0xec08u, 2u) ||
        !ObjectCoordinateCall(memory, cpu, 0xec0bu, Lufia2ObjectRoundedProbe) ||
        !ObjectCollisionMoveProbe(memory, cpu, 0xec15u) ||
        !ObjectCollisionProbeEvent(memory, cpu, 0x18u, 0xec1fu))
        return OBJECT_FLOW_BOUNDARY;
    OpPullY(memory, cpu);
    return ObjectFinishInteraction(memory, cpu, true);
}

static ObjectFlow ObjectPushInteractionRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ObjectCollisionCall(memory, cpu, Lufia2FieldFindPendingObject,
        0x83u, 0xec25u, 2u))
        return OBJECT_FLOW_BOUNDARY;
    OpTxa(cpu);
    OpSta(memory, cpu, 0x7fd0beu);
    PushY(memory, cpu);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_UNK_7FD9CC));
    OpSta(memory, cpu, OpDp(cpu, 0x23u));
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpDp(cpu, 0x22u));
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT));
    PushAccumulator8(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x83u, 0xec3fu);
    cpu->program_bank = 0x80u;
    cpu->resume_pc = 0x80dcdau;
    return OBJECT_FLOW_BOUNDARY;
}

static ObjectFlow ObjectChooseInteractionRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSta(memory, cpu, OpDp(cpu, 0x8bu));
    OpStz(memory, cpu, OpDp(cpu, 0x8cu));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x8bu)));
    OpLda(memory, cpu, OpLongX(cpu, 0x7fd296u));
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    OpLda(memory, cpu, OpLongX(cpu, 0x7fd376u));
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_A + 1u));
    OpOra(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    if (cpu->zero)
        return ObjectFinishInteraction(memory, cpu, false);
    OpLda(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    OpBitValue(cpu, 0x10u);
    if (!cpu->zero)
        return ObjectActivateInteractionRecord(memory, cpu);
    OpBitValue(cpu, 8u);
    if (cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, DP_SCRATCH_A + 1u));
        OpBitValue(cpu, 2u);
        return ObjectFinishInteraction(memory, cpu, !cpu->zero);
    }
    OpLda(memory, cpu, OpDp(cpu, DP_SCRATCH_A + 1u));
    OpBitValue(cpu, 2u);
    if (cpu->zero)
        return ObjectFinishInteraction(memory, cpu, false);
    OpLoadA(cpu, 0x0au);
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpDp(cpu, 0x8bu));
    cpu->carry = 1u;
    OpSbcValue(cpu, 0x10u);
    OpLdx(cpu, 0x16u);
    if (!ObjectCollisionCall(memory, cpu, Lufia2FieldFindHeaderRecord,
        0x80u, 0xebeau, 3u))
        return OBJECT_FLOW_BOUNDARY;
    OpLda(memory, cpu, OpLongX(cpu, 0x7ef005u));
    OpCmpValue(cpu, 2u);
    if (cpu->carry)
        return ObjectPushInteractionRecord(memory, cpu);
    return ObjectFinishInteraction(memory, cpu, false);
}

static ObjectFlow ObjectInteractWithRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ObjectCollisionCall(memory, cpu, Lufia2ObjectProbeNextTile,
        0x83u, 0xeb6fu, 2u))
        return OBJECT_FLOW_BOUNDARY;
    Lufia2MapTileHeight(memory, cpu, 0xeb72u);
    OpSta(memory, cpu, 0x7fd070u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpCmp(memory, cpu, OpLongX(cpu, WRAM_UNK_7FDA2C));
    if (!cpu->zero) {
        if (cpu->carry) {
            OpLda(memory, cpu, OpLongX(cpu, WRAM_UNK_7FD9CC));
            OpCmpValue(cpu, 4u);
            if (!cpu->zero)
                return ObjectFinishInteraction(memory, cpu, false);
        } else {
            OpLda(memory, cpu, 0x0009a7u);
            OpBitValue(cpu, 0x20u);
            if (!cpu->zero)
                return ObjectCollisionSkipJump(cpu);
            return ObjectFinishInteraction(memory, cpu, false);
        }
    }
    if (!ObjectCollisionCall(memory, cpu, Lufia2FieldSetPendingProbePosition,
        0x83u, 0xeb9bu, 2u))
        return OBJECT_FLOW_BOUNDARY;
    SimulateJslFrame(memory, cpu, 0x83u, 0xeb9fu);
    Lufia2ActorReadMapCellValue(memory, cpu);
    if (!ObjectCollisionReturn(memory, cpu, 0xeb9fu, 3u))
        return OBJECT_FLOW_BOUNDARY;
    OpOraValue(cpu, 0u);
    if (!cpu->zero) {
        OpBitValue(cpu, 0xf0u);
        if (cpu->zero)
            return ObjectFinishInteraction(memory, cpu, false);
    } else {
        if (!ObjectCollisionCall(memory, cpu, Lufia2FieldFindPendingObject,
            0x83u, 0xebacu, 2u))
            return OBJECT_FLOW_BOUNDARY;
        if (cpu->carry)
            return ObjectFinishInteraction(memory, cpu, false);
        TransferDirectToA(cpu);
        OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_PENDING_OBJECT_RECORD));
    }
    return ObjectChooseInteractionRecord(memory, cpu);
}

static ObjectFlow ObjectExecute(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    switch (handler) {
    case OBJECT_OP_8X_TABLE:
        handler = ObjectSubDispatch(memory, cpu, 0xf368u, 0xed47u);
        break;
    case OBJECT_OP_1X_TABLE:
        handler = ObjectSubDispatch(memory, cpu, 0xf348u, 0xed4du);
        break;
    case OBJECT_OP_2X_TABLE:
        handler = ObjectSubDispatch(memory, cpu, 0xf328u, 0xed53u);
        break;
    case OBJECT_OP_EX_TABLE:
        handler = ObjectSubDispatch(memory, cpu, 0xf308u, 0xed59u);
        break;
    case OBJECT_OP_FX_TABLE:
        handler = ObjectSubDispatch(memory, cpu, 0xf2e8u, 0xed5fu);
        break;
    default:
        break;
    }

    switch (handler) {
    case OBJECT_OP_REFRESH_MAP_HEIGHT:
        return ObjectRefreshMapHeight(memory, cpu);
    case OBJECT_OP_UPDATE_HEIGHT_DRAW_FLAG:
        return ObjectUpdateHeightDrawFlag(memory, cpu);
    case OBJECT_OP_CLEAR_MAP_OCCUPANCY:
        return ObjectClearMapOccupancy(memory, cpu);
    case OBJECT_OP_INTERPOLATE_POSITION:
        return ObjectInterpolatePosition(memory, cpu);
    case OBJECT_OP_APPROACH_POSITION:
        return ObjectApproachPosition(memory, cpu);
    case OBJECT_OP_COPY_FRAME_GRAPHICS:
        return ObjectCopyFrameGraphics(memory, cpu);
    case OBJECT_OP_QUEUE_SOUND:
        return ObjectQueueSound(memory, cpu);
    case OBJECT_OP_START_LEADER_EVENT:
        return ObjectStartLeaderEvent(memory, cpu);
    case OBJECT_OP_START_POSITION_EVENT:
        return ObjectStartPositionEvent(memory, cpu);
    case OBJECT_OP_START_MODE_EVENT:
        return ObjectStartModeEvent(memory, cpu);
    case OBJECT_OP_CLEAR_FOLLOWERS:
        return ObjectClearFollowers(memory, cpu);
    case OBJECT_OP_APPLY_PROBED_RECORD:
        return ObjectApplyProbedRecord(memory, cpu);
    case OBJECT_OP_WAKE_OR_APPLY_RECORD:
        return ObjectWakeOrApplyRecord(memory, cpu);
    case OBJECT_OP_REACT_TO_MAP_RECORD:
        return ObjectReactToMapRecord(memory, cpu);
    case OBJECT_OP_REACT_TO_PROBED_ACTOR:
        return ObjectReactToProbedActor(memory, cpu);
    case OBJECT_OP_INTERACT_WITH_RECORD:
        return ObjectInteractWithRecord(memory, cpu);
    case OBJECT_OP_SET_CUSTOM_GRAPHICS:
        return ObjectSetCustomGraphics(memory, cpu);
    case OBJECT_OP_REACT_TO_MOVEMENT:
        return ObjectReactToMovement(memory, cpu);
    case OBJECT_OP_EVENT_PROBE_JUMP:
        return ObjectEventProbeJump(memory, cpu, 0u);
    case OBJECT_OP_EVENT_PROBE_ALT_JUMP:
        return ObjectEventProbeJump(memory, cpu, 1u);
    case OBJECT_OP_PROBE_DIRECTION_JUMP:
        return ObjectProbeDirectionJump(memory, cpu);
    case OBJECT_OP_4X_WAIT:
        return ObjectOp4XWait(memory, cpu);
    case OBJECT_OP_F6_NIBBLE_OFFSETS:
        return ObjectOpF6NibbleOffsets(memory, cpu);
    case OBJECT_OP_F2_RANDOM_JITTER:
        return ObjectOpF2RandomJitter(memory, cpu);
    case OBJECT_OP_1A_SPRITE_FRAME:
        return ObjectOp1ASpriteFrame(memory, cpu);
    case OBJECT_OP_80_LOOP_END:
        return ObjectOp80LoopEnd(memory, cpu);
    case OBJECT_OP_2F_COPY_ACTOR0_POSITION:
        return ObjectOp2FCopyActor0Position(memory, cpu);
    case OBJECT_OP_SET_DRAW_FLAGS:
        return ObjectSetDrawFlags(memory, cpu);
    case OBJECT_OP_MARK_BLOCKED_EVENT_OBJECT:
        return ObjectMarkBlockedEventObject(memory, cpu);
    case OBJECT_OP_12_NOP:                                         /* no-op */
        return OBJECT_FLOW_DISPATCH;
    case OBJECT_OP_CENTER_ON_SCREEN:
        return ObjectCenterOnScreen(memory, cpu);
    case OBJECT_OP_SET_SLOT0_FLAGS:
        return ObjectSetSlot0Flags(memory, cpu);
    case OBJECT_OP_SET_FIELD_CONTROL:
        return ObjectSetFieldControl(memory, cpu);
    case OBJECT_OP_OR_FIELD_CONTROL:
        return ObjectOrFieldControl(memory, cpu);
    case OBJECT_OP_7X_LOOP_START:
        return ObjectOp7XLoopStart(memory, cpu);
    case OBJECT_OP_29:
    case OBJECT_OP_2B:
    case OBJECT_OP_CX:
        return ObjectOp292BCX(memory, cpu, handler);
    case OBJECT_OP_SET_FIELD_CONTROL_BIT4:
        return ObjectSetFieldControlBit4(memory, cpu);
    case OBJECT_OP_89:
        return ObjectOp89(memory, cpu);
    case OBJECT_OP_SET_FLAG_BIT7:
        return ObjectSetFlagBit7(memory, cpu);
    case OBJECT_OP_F8_SET_STATE_BIT4:
    case OBJECT_OP_F9_CLEAR_STATE_BIT4:
        return ObjectOpF8F9StateBit4(memory, cpu, handler);
    case OBJECT_OP_SET_OFFSET_WORD:
        return ObjectSetOffsetWord(memory, cpu);
    case OBJECT_OP_COPY_OFFSETS:
    case OBJECT_OP_COPY_OFFSETS_REVERSED:
        return ObjectCopyOffsets(memory, cpu, handler);
    case OBJECT_OP_NEGATE_OFFSETS:
        return ObjectNegateOffsets(memory, cpu);
    case OBJECT_OP_EC:
        return ObjectOpBeginLoop(memory, cpu);
    case OBJECT_OP_ED:
        return ObjectOpED(memory, cpu);
    case OBJECT_OP_3X_SET_FRAME:
    case OBJECT_OP_2C_SET_FRAME:
    case OBJECT_OP_25_SET_FRAME:
    case OBJECT_OP_FD_SET_FRAME:
    case OBJECT_OP_8F_SET_FRAME:
        return ObjectOp3X2C25FD8FSetFrame(memory, cpu, handler);
    case OBJECT_OP_5X:
    case OBJECT_OP_28:
    case OBJECT_OP_DX:
        return ObjectJump(memory, cpu, handler);
    case OBJECT_OP_FB:
        return ObjectOpFB(memory, cpu);
    case OBJECT_OP_FC:
    case OBJECT_OP_E3:
        return ObjectOpFCE3(memory, cpu, handler);
    case OBJECT_OP_F7_BYTE_OFFSETS:
        return ObjectOpF7ByteOffsets(memory, cpu);
    case OBJECT_OP_20:
        return ObjectOp20(memory, cpu);
    case OBJECT_OP_TAKE_ANIMATION:
        return ObjectTakeAnimationBySlotId(memory, cpu);
    case OBJECT_OP_19:
    case OBJECT_OP_8E:
        return ObjectOp198E(memory, cpu, handler);
    case OBJECT_OP_SET_ANIMATION:
        return ObjectSetAnimation(memory, cpu, handler);
    case OBJECT_OP_ANIMATION_FROM_OPERAND:
        return ObjectSetAnimationFromOperand(memory, cpu, handler);
    case OBJECT_OP_6X_SPAWN_CHILD:
        return ObjectOp6XSpawnChild(memory, cpu);
    case OBJECT_OP_SPAWN_CHILD_WITH_ANIMATION:
        return ObjectSpawnChildWithAnimation(memory, cpu);
    case OBJECT_OP_SPAWN_CHILD_AT_OFFSET:
        return ObjectSpawnChildAtOffset(memory, cpu);
    case OBJECT_OP_SPAWN_REGISTERED_CHILD:
        return ObjectSpawnRegisteredChild(memory, cpu);
    case OBJECT_OP_0X_DESPAWN:
        return ObjectOpDespawn(memory, cpu);
    case OBJECT_OP_SPIN_ZOOM:
        return ObjectSpinZoom(memory, cpu);
    default:
        cpu->resume_pc = OBJECT_PROGRAM_BANK | handler;
        return OBJECT_FLOW_BOUNDARY;
    }
}

/* $83:E0FC: run object $A7's script until it yields. */
static uint8_t ObjectRunScript(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t *dispatches) {
    unsigned steps;

    PushDataBank(memory, cpu);                                 /* E0FC */
    LoadXDirect(memory, cpu, DP_SLOT_RECORD_OFFSET);
    LoadA8(cpu, Read8(memory, LongIndexedAddress((WRAM_OBJECT_SCRIPT + 2u), cpu->x)));
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress((WRAM_OBJECT_SCRIPT + 1u), cpu->x)));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_OBJECT_SCRIPT, cpu->x)));
    TransferAToY(cpu);
    /* A script that never yields spins the ROM forever. */
    for (steps = 0; steps < 0x10000u; ++steps) {
        uint16_t handler;
        ObjectFlow flow;

        ++*dispatches;
        TransferDirectToA(cpu);                                /* E10F */
        LoadAAbsolute8(memory, cpu, 0x00u, cpu->y);
        LsrA8(cpu);
        LsrA8(cpu);
        LsrA8(cpu);
        And8(cpu, 0x1eu);
        TransferAToX(cpu);
        handler = Read16ProgramIndexed(memory, cpu, 0xf2c8u, cpu->x);
        flow = ObjectExecute(memory, cpu, handler);
        if (flow == OBJECT_FLOW_RETURN)
            return 1;
        if (flow == OBJECT_FLOW_BOUNDARY)
            return 0;
    }
    cpu->resume_pc = 0x83e10fu;
    return 0;
}

Lufia2ExecutionResult Lufia2ObjectSlotsUpdate(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;

    result.flow = LUFIA2_EXECUTION_RETURNED;
    result.pc = 0x83e0fbu;
    result.dispatches = 0;
    SetIndexWidth(cpu, 0);                                     /* E03E */
    TransferDirectToA(cpu);
    Write8(memory, WRAM_UNK_7FDDAC, A8(cpu));
    Write8(memory, (WRAM_UNK_7FDDAC + 1u), A8(cpu));
    Write8(memory, AbsoluteIndexedAddress(cpu, WRAM_UNK_7E09A8, 0), 0x00u);
    LoadX16(cpu, 0x00u);
    do {
        LoadAAbsolute8(memory, cpu, WRAM_OBJECT_STATE, cpu->x);          /* E04F */
        BitImmediate8(cpu, 0x80u);
        if (!cpu->zero) {
            const uint32_t flags = LongIndexedAddress(WRAM_OBJECT_FLAGS, cpu->x);
            const uint32_t blink =
                LongIndexedAddress(WRAM_OBJECT_BLINK_COUNTER, cpu->x);
            uint8_t animate = 0;

            LoadA8(cpu, Read8(memory, flags));                 /* E059 */
            BitImmediate8(cpu, 0x10u);
            if (!cpu->zero) {
                LoadA8(cpu, Read8(memory, blink));
                DecrementA8(cpu);
                Write8(memory, blink, A8(cpu));
                And8(cpu, 0x0fu);
                if (cpu->zero) {
                    LoadA8(cpu, Read8(memory, blink));         /* E06E */
                    LsrA8(cpu);
                    LsrA8(cpu);
                    LsrA8(cpu);
                    LsrA8(cpu);
                    Or8(cpu, Read8(memory, blink));
                    Write8(memory, blink, A8(cpu));
                    ToggleLong8(memory, cpu,
                                LongIndexedAddress(WRAM_OBJECT_DRAW_FLAGS, cpu->x),
                                0x80u);
                }
            }
            LoadAAbsolute8(memory, cpu, WRAM_WINDOW_MODE, 0);           /* E088 */
            BitImmediate8(cpu, 0x01u);
            if (!cpu->zero) {
                LoadA8(cpu, Read8(memory, flags));             /* E08F */
                BitImmediate8(cpu, 0x40u);
                if (!cpu->zero) {
                    LoadAAbsolute8(memory, cpu, WRAM_OBJECT_STATE, cpu->x);
                    LoadA8(cpu, (uint8_t)(A8(cpu) ^ 0xffu));
                    TestBitsAbsolute8(memory, cpu, WRAM_UNK_7E09A8, 1);
                }
                LoadAAbsolute8(memory, cpu, WRAM_OBJECT_STATE, cpu->x);  /* E09F */
                BitImmediate8(cpu, 0x08u);
                if (!cpu->zero) {
                    BitImmediate8(cpu, 0x40u);
                    animate = cpu->zero;
                }
                if (!animate) {
                    And8(cpu, 0xbfu);                          /* E0AA */
                    StoreAAbsolute8(memory, cpu, WRAM_OBJECT_STATE, cpu->x);
                }
            }
            if (!animate) {
                const uint32_t wait = LongIndexedAddress(WRAM_UNK_7FDFAE, cpu->x);

                LoadA8(cpu, Read8(memory, wait));              /* E0AF */
                DecrementA8(cpu);
                Write8(memory, wait, A8(cpu));
                if (cpu->zero) {
                    StoreXDirect16(memory, cpu, DP_ACTOR_SLOT); /* E0BA */
                    SimulateJslFrame(memory, cpu, 0x83u, 0xe0bfu);
                    Lufia2ActorRecordOffsets(memory, cpu);
                    SimulateRtlFrame(memory, cpu);
                    SimulateJsrFrame(memory, cpu, 0xe0c2u);    /* E0C0 */
                    if (!ObjectRunScript(
                            memory, cpu, &result.dispatches)) {
                        result.flow = LUFIA2_EXECUTION_BOUNDARY;
                        result.pc = cpu->resume_pc;
                        return result;
                    }
                    if (!ObjectEventReturn(memory, cpu, 0xe0c2u, 2u)) {
                        result.flow = LUFIA2_EXECUTION_BOUNDARY;
                        result.pc = cpu->resume_pc;
                        return result;
                    }
                    LoadXDirect16(memory, cpu, DP_ACTOR_SLOT); /* E0C3 */
                }
            } else {
                const uint32_t step =
                    LongIndexedAddress(WRAM_OBJECT_STEP_COUNTER, cpu->x);

                LoadA8(cpu, Read8(memory, step));              /* E0C7 */
                LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
                Write8(memory, step, A8(cpu));
                Compare8(cpu, A8(cpu), 0x20u);
                if (cpu->zero) {
                    TransferDirectToA(cpu);                    /* E0D4 */
                    Write8(memory, step, A8(cpu));
                    StoreXDirect16(memory, cpu, DP_ACTOR_SLOT);
                    SimulateJslFrame(memory, cpu, 0x83u, 0xe0deu);
                    Lufia2ActorRecordOffsets(memory, cpu);
                    SimulateRtlFrame(memory, cpu);
                    ToggleLong8(memory, cpu,
                                LongIndexedAddress(WRAM_OBJECT_FRAME, cpu->x), 0x01u);
                    Write8(memory, DirectAddress(cpu, DP_SCRATCH_A), A8(cpu));
                    ObjectSetFrame(memory, cpu, 0xe0edu);      /* E0EB */
                    LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);
                }
            }
        }
        IncrementX16(cpu);                                     /* E0F0 */
        Compare16(cpu, cpu->x, 0x20u);
    } while (!cpu->zero);
    SetIndexWidth(cpu, 1);                                     /* E0F9 */
    return result;
}
