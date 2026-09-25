/* Field object slots and script VM ($83:E03E). */

#include "core/cpu_internal.h"
#include "lufia2/actor.h"
#include "actor/actor_internal.h"
#include "system/system_internal.h"
#include "system/wram.h"

/* $83:E200: sprite frame $54 for object $A7. */
static void ObjectSetFrame(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    PushY(memory, cpu);                                        /* E200 */
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    Write8(memory, LongIndexedAddress(0x7fdb2cu, cpu->x), A8(cpu));
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    And8(cpu, 0x3fu);
    Write8(memory, SNES_WRMPYA, A8(cpu));
    TransferDirectToA(cpu);                                    /* E212 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe23eu, cpu->x)));
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x83abf8u, cpu->x)));
    Write8(memory, SNES_WRMPYB, A8(cpu));
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0732u, 0));
    LoadXDirect(memory, cpu, 0xabu);                           /* E223 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe0f0u, cpu->x)));
    Write8(memory, LongIndexedAddress(0x7fd90eu, cpu->x), A8(cpu));
    LoadA8(cpu, Read8(memory, SNES_RDMPYL));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, 0x00u);
    SetAccumulatorWidth(cpu, 0);                               /* E234 */
    LsrA16(cpu);
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, LongIndexedAddress(0x7fe0eeu, cpu->x)));
    Write16Long(memory, LongIndexedAddress(0x7fd90cu, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);                   /* E242 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    AslA8(cpu);
    Adc8(cpu, 0x00u);
    AslA8(cpu);
    Adc8(cpu, 0x00u);
    And8(cpu, 0x03u);
    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x064au, cpu->x);              /* E250 */
    And8(cpu, 0xfcu);
    Or8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    Or8(cpu, 0x20u);
    StoreAAbsolute8(memory, cpu, 0x064au, cpu->x);
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
    LoadXDirect(memory, cpu, 0xabu);                           /* E143 */
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, cpu->y);
    Write16Long(memory, LongIndexedAddress(0x7fdeeeu, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    PullDataBank(memory, cpu);
    return OBJECT_FLOW_RETURN;
}

/* $83:E11C: advance by A; yield on $064A bit 4. */
static ObjectFlow ObjectAdvance(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);                               /* E11C */
    Write16Direct(memory, cpu, 0x54u, cpu->y);
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x54u));
    LoadXDirect(memory, cpu, 0xabu);
    Write16Long(memory, LongIndexedAddress(0x7fdeeeu, cpu->x), cpu->accumulator);
    TransferAToY(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);                   /* E12C */
    LoadAAbsolute8(memory, cpu, 0x064au, cpu->x);
    BitImmediate8(cpu, 0x10u);
    if (cpu->zero)
        return OBJECT_FLOW_DISPATCH;
    LoadA8(cpu, 0x01u);                                        /* E135 */
    Write8(memory, LongIndexedAddress(0x7fdfaeu, cpu->x), A8(cpu));
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
    LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);              /* EED8 */
    LsrA8(cpu);
    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
    Lufia2CallRandomScale(memory, cpu, 0xeee4u);               /* $80:8299 */
    cpu->carry = 1;
    Sbc8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
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
    LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
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
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe286u, cpu->x)));
    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe2ceu, cpu->x)));
    PushAccumulator8(memory, cpu);
    TransferXToA(cpu);
    PushAccumulator8(memory, cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe1f6u, cpu->x)));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe23eu, cpu->x)));
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    Write8(memory, LongIndexedAddress(0x7fe23eu, cpu->x), A8(cpu));
    ExchangeAccumulatorBytes(cpu);
    Write8(memory, LongIndexedAddress(0x7fe1f6u, cpu->x), A8(cpu));
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    Write8(memory, LongIndexedAddress(0x7fe286u, cpu->x), A8(cpu));
    LoadA8(cpu, Pull8(memory, cpu));
    Write8(memory, LongIndexedAddress(0x7fe3a6u, cpu->x), A8(cpu));
    LoadA8(cpu, Pull8(memory, cpu));
    Write8(memory, LongIndexedAddress(0x7fe2ceu, cpu->x), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x064au, cpu->x);
    And8(cpu, 0xfbu);
    StoreAAbsolute8(memory, cpu, 0x064au, cpu->x);
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
    LoadAAbsolute8(memory, cpu, 0x064au, cpu->x);
    And8(cpu, 0xfbu);
    StoreAAbsolute8(memory, cpu, 0x064au, cpu->x);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe23eu, cpu->x)));
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x83abf4u, cpu->x)));
    Lufia2SpriteAllocSlots(memory, cpu, 0xecf6u);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);                   /* ECF7 */
    Write8(memory, LongIndexedAddress(0x7fe286u, cpu->x), A8(cpu));
    ExchangeAccumulatorBytes(cpu);
    Write8(memory, LongIndexedAddress(0x7fe2ceu, cpu->x), A8(cpu));
    Lufia2SpriteVramBase(memory, cpu, 0xed05u);
    SetIndexWidth(cpu, 0);                                     /* ED06 */
    LoadXDirect(memory, cpu, 0xa9u);
    Write16Long(memory, LongIndexedAddress(0x7fe0aeu, cpu->x), cpu->accumulator);
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
    Write8(memory, LongIndexedAddress(0x7fe1aeu, cpu->x), A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    AslA16(cpu);
    AslA16(cpu);
    TransferAToX(cpu);
    TransferAToY(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x83f388u, cpu->x)));
    LoadXDirect(memory, cpu, 0xabu);
    Write16Long(memory, LongIndexedAddress(0x7fe0eeu, cpu->x), cpu->accumulator);
    cpu->x = cpu->y;                                           /* TYX */
    SetNz16(cpu, cpu->x);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x83f38au, cpu->x)));
    LoadXDirect(memory, cpu, 0xabu);
    Write8(memory, LongIndexedAddress(0x7fe0f0u, cpu->x), A8(cpu));
    cpu->x = cpu->y;
    SetNz16(cpu, cpu->x);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x83f38bu, cpu->x)));
    SetIndexWidth(cpu, 1);                                     /* ECC3 */
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
    And8(cpu, 0x0fu);
    Write8(memory, LongIndexedAddress(0x7fe1f6u, cpu->x), A8(cpu));
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    Write8(memory, LongIndexedAddress(0x7fe23eu, cpu->x), A8(cpu));
    ObjectSpriteSetup(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);                      /* ECDC */
    SimulateRtsFrame(memory, cpu);
}

/* $83:E8A6: spawn child object from the operand block at Y. */
static void ObjectSpawnChild(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    TransferDirectToA(cpu);                                    /* E8A6 */
    LoadAAbsolute8(memory, cpu, 0x0001u, cpu->y);
    Lufia2SignExtendA8(memory, cpu, 0xe8acu);
    Write16Direct(memory, cpu, 0x5au, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    TransferDirectToA(cpu);                                    /* E8B1 */
    LoadAAbsolute8(memory, cpu, 0x0002u, cpu->y);
    Lufia2SignExtendA8(memory, cpu, 0xe8b7u);
    Write16Direct(memory, cpu, 0x63u, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);                   /* E8BC */
    PushIndex(memory, cpu);
    LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
    SimulateJslFrame(memory, cpu, 0x83u, 0xe8c5u);
    Lufia2ActorSpawn(memory, cpu);                             /* DF87 */
    SimulateRtlFrame(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);                      /* E8C6 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_ACTOR_SLOT)));
    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
    Write8(memory, DirectAddress(cpu, 0x55u), 0x00u);
    LoadXDirect(memory, cpu, 0xa9u);
    StoreYDirect16(memory, cpu, DP_ACTOR_SLOT);
    SimulateJslFrame(memory, cpu, 0x83u, 0xe8d4u);
    Lufia2ActorRecordOffsets(memory, cpu);                     /* AB4F */
    SimulateRtlFrame(memory, cpu);
    SetAccumulatorWidth(cpu, 0);                               /* E8D5 */
    cpu->y = cpu->x;                                           /* TXY */
    SetNz16(cpu, cpu->y);
    LoadXDirect(memory, cpu, 0xa9u);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fddfeu, cpu->x)));
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x5au));
    Write16Direct(memory, cpu, 0x5au, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fde8eu, cpu->x)));
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x63u));
    Write16Direct(memory, cpu, 0x63u, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fdcdcu, cpu->x)));
    PushAccumulator16(memory, cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fdd6cu, cpu->x)));
    cpu->x = cpu->y;                                           /* TYX */
    SetNz16(cpu, cpu->x);
    Write16Long(memory, LongIndexedAddress(0x7fdd6cu, cpu->x), cpu->accumulator);
    PullAccumulator16(memory, cpu);
    Write16Long(memory, LongIndexedAddress(0x7fdcdcu, cpu->x), cpu->accumulator);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x5au));
    Write16Long(memory, LongIndexedAddress(0x7fddfeu, cpu->x), cpu->accumulator);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x63u));
    Write16Long(memory, LongIndexedAddress(0x7fde8eu, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    PushIndex(memory, cpu);                                    /* E90D */
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fda2cu, cpu->x)));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd9ccu, cpu->x)));
    LoadXDirect(memory, cpu, 0x54u);
    Write8(memory, LongIndexedAddress(0x7fd9ccu, cpu->x), A8(cpu));
    ExchangeAccumulatorBytes(cpu);
    Write8(memory, LongIndexedAddress(0x7fda2cu, cpu->x), A8(cpu));
    TransferDirectToA(cpu);                                    /* E924 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_ACTOR_SLOT)));
    if (cpu->carry) {
        TransferAToX(cpu);                                     /* E92B */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdfaeu, cpu->x)));
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        Write8(memory, LongIndexedAddress(0x7fdfaeu, cpu->x), A8(cpu));
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
    const uint8_t shape = Read8(memory, LongIndexedAddress(0x83f38bu, index));

    return Read8(memory, 0x83abf4u + (uint32_t)(shape >> 4)) == 0;
}

/* $83:F205: despawn object $A7, free its sprite. */
static void ObjectDespawn(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJslFrame(memory, cpu, 0x83u, 0xe140u);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);                   /* F205 */
    LoadA8(cpu, 0x04u);
    StoreAAbsolute8(memory, cpu, 0x064au, cpu->x);
    SetIndexWidth(cpu, 1);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe1aeu, cpu->x)));
    Compare8(cpu, A8(cpu), 0xffu);
    if (!cpu->zero) {
        TransferDirectToA(cpu);                                /* F218 */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe23eu, cpu->x)));
        Compare8(cpu, A8(cpu), 0xffu);
        if (!cpu->zero) {
            ExchangeAccumulatorBytes(cpu);
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe286u, cpu->x)));
            Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe2ceu, cpu->x)));
            Write8(memory, DirectAddress(cpu, 0x55u), A8(cpu));
            ExchangeAccumulatorBytes(cpu);
            TransferAToX(cpu);
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x83abf4u, cpu->x)));
            Lufia2SpriteFreeSlots(memory, cpu, 0xf237u);
        }
    }
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);                   /* F238 */
    LoadA8(cpu, 0xffu);
    Write8(memory, LongIndexedAddress(0x7fe1aeu, cpu->x), A8(cpu));
    Write8(memory, LongIndexedAddress(0x7fdb2cu, cpu->x), A8(cpu));
    Write8(memory, LongIndexedAddress(0x7fe3a6u, cpu->x), A8(cpu));
    StoreAAbsolute8(memory, cpu, 0x1559u, cpu->x);
    Write8(memory, LongIndexedAddress(0x7fe286u, cpu->x), A8(cpu));
    Write8(memory, LongIndexedAddress(0x7fe2ceu, cpu->x), A8(cpu));
    SetIndexWidth(cpu, 0);
    SimulateRtlFrame(memory, cpu);
}

/* Object script handlers from $83:F2C8 and its low-nibble tables. */
enum ObjectOpcodeHandler {
    OBJECT_OP_0X = 0xe13d,                /* $0x */
    OBJECT_OP_2F = 0xe150,                /* $2F */
    OBJECT_OP_F6_NIBBLE_OFFSETS = 0xe168, /* $F6 */
    OBJECT_OP_F7 = 0xe191,                /* $F7 */
    OBJECT_OP_3X_SET_FRAME = 0xe1c5,      /* $3x */
    OBJECT_OP_2C_SET_FRAME = 0xe1d3,      /* $2C */
    OBJECT_OP_25_SET_FRAME = 0xe1e6,      /* $25 */
    OBJECT_OP_FD_SET_FRAME = 0xe1f4,      /* $FD */
    OBJECT_OP_2E = 0xe270,                /* $2E */
    OBJECT_OP_19 = 0xe437,                /* $19 */
    OBJECT_OP_8E = 0xe445,                /* $8E */
    OBJECT_OP_1A_SPRITE_FRAME = 0xe589,   /* $1A */
    OBJECT_OP_8F_SET_FRAME = 0xe59f,      /* $8F */
    OBJECT_OP_1D = 0xe5b5,                /* $1D */
    OBJECT_OP_12 = 0xe700,                /* $12 */
    OBJECT_OP_13 = 0xe810,                /* $13 */
    OBJECT_OP_4X_WAIT = 0xe831,           /* $4x */
    OBJECT_OP_5X = 0xe840,                /* $5x */
    OBJECT_OP_6X = 0xe854,                /* $6x */
    OBJECT_OP_85 = 0xe860,                /* $85 */
    OBJECT_OP_8A = 0xe87c,                /* $8A */
    OBJECT_OP_EE = 0xe937,                /* $EE */
    OBJECT_OP_20 = 0xe99a,                /* $20 */
    OBJECT_OP_22 = 0xea7d,                /* $22 */
    OBJECT_OP_23 = 0xea8c,                /* $23 */
    OBJECT_OP_8D = 0xea94,                /* $8D */
    OBJECT_OP_7X_LOOP_START = 0xeab5,     /* $7x */
    OBJECT_OP_80_LOOP_END = 0xead5,       /* $80 */
    OBJECT_OP_26 = 0xeaff,                /* $26 */
    OBJECT_OP_28 = 0xeb38,                /* $28 */
    OBJECT_OP_29 = 0xeb50,                /* $29 */
    OBJECT_OP_2B = 0xec6d,                /* $2B */
    OBJECT_OP_AX = 0xec8e,                /* $Ax */
    OBJECT_OP_CX = 0xed11,                /* $Cx */
    OBJECT_OP_DX = 0xed32,                /* $Dx */
    OBJECT_OP_8X_TABLE = 0xed45,          /* $8x */
    OBJECT_OP_1X_TABLE = 0xed4b,          /* $1x */
    OBJECT_OP_2X_TABLE = 0xed51,          /* $2x */
    OBJECT_OP_EX_TABLE = 0xed57,          /* $Ex */
    OBJECT_OP_FX_TABLE = 0xed5d,          /* $Fx */
    OBJECT_OP_84 = 0xed9b,                /* $84 */
    OBJECT_OP_89 = 0xedc2,                /* $89 */
    OBJECT_OP_EA = 0xedd7,                /* $EA */
    OBJECT_OP_F1 = 0xede6,                /* $F1 */
    OBJECT_OP_FB = 0xee48,                /* $FB */
    OBJECT_OP_FC = 0xee71,                /* $FC */
    OBJECT_OP_F2_RANDOM_JITTER = 0xeeb5,  /* $F2 */
    OBJECT_OP_F8 = 0xefd1,                /* $F8 */
    OBJECT_OP_F9 = 0xefde,                /* $F9 */
    OBJECT_OP_E2 = 0xf0e8,                /* $E2 */
    OBJECT_OP_82 = 0xf0fa,                /* $82 */
    OBJECT_OP_83 = 0xf10b,                /* $83 */
    OBJECT_OP_EB = 0xf11c,                /* $EB */
    OBJECT_OP_EC = 0xf137,                /* $EC */
    OBJECT_OP_ED = 0xf155,                /* $ED */
    OBJECT_OP_E3 = 0xf19f,                /* $E3 */
};

/* $83:E831: object opcode $4x. */
static ObjectFlow ObjectOp4XWait(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
    And8(cpu, 0x0fu);
    Write8(memory, LongIndexedAddress(0x7fdfaeu, cpu->x), A8(cpu));
    IncrementY16(cpu);
    return ObjectSaveAndReturn(memory, cpu);
}

/* $83:E168: object opcode $F6. */
static ObjectFlow ObjectOpF6NibbleOffsets(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
    SimulateJsrFrame(memory, cpu, 0xe16fu);
    LoadXDirect(memory, cpu, 0xa9u);                       /* E176 */
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    ObjectSignNibble(memory, cpu, 0xe181u);
    ObjectAddPosition(memory, cpu, 0x7fdcdcu, 0xe184u);
    TransferDirectToA(cpu);                                /* E185 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    And8(cpu, 0x0fu);
    ObjectSignNibble(memory, cpu, 0xe18cu);
    ObjectAddPosition(memory, cpu, 0x7fdd6cu, 0xe18fu);
    SimulateRtsFrame(memory, cpu);
    TransferDirectToA(cpu);                                /* E170 */
    LoadA8(cpu, 0x01u);
    return ObjectAdvance(memory, cpu);
}

/* $83:EEB5: object opcode $F2. */
static ObjectFlow ObjectOpF2RandomJitter(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, 0xa9u);
    ObjectJitter(memory, cpu, 0xeeb9u);
    IncrementY16(cpu);                                     /* EEBA */
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, LongIndexedAddress(0x7fdcdcu, cpu->x)));
    Write16Long(memory, LongIndexedAddress(0x7fdcdcu, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    ObjectJitter(memory, cpu, 0xeec8u);
    IncrementY16(cpu);                                     /* EEC9 */
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, LongIndexedAddress(0x7fdd6cu, cpu->x)));
    Write16Long(memory, LongIndexedAddress(0x7fdd6cu, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    return OBJECT_FLOW_DISPATCH;
}

/* $83:E589: object opcode $1A. */
static ObjectFlow ObjectOp1ASpriteFrame(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdb0cu, cpu->x)));
    BitImmediate8(cpu, 0x20u);
    if (!cpu->zero) {
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);      /* E593 */
        Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
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
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe08eu, cpu->x)));
    DecrementA8(cpu);
    Write8(memory, LongIndexedAddress(0x7fe08eu, cpu->x), A8(cpu));
    if (cpu->negative) {
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));              /* EAE2 */
        Write8(memory, LongIndexedAddress(0x7fe08eu, cpu->x), A8(cpu));
        return OBJECT_FLOW_DISPATCH;
    }
    LoadXDirect(memory, cpu, 0xabu);                       /* EAEA */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdfcfu, cpu->x)));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdfceu, cpu->x)));
    TransferAToY(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdfd0u, cpu->x)));
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    return OBJECT_FLOW_DISPATCH;
}

/* $83:E150: object opcode $2F. */
static ObjectFlow ObjectOp2F(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, 0xa9u);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, 0x7fddaeu));
    Write16Long(memory, LongIndexedAddress(0x7fddfeu, cpu->x), cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, 0x7fde3eu));
    Write16Long(memory, LongIndexedAddress(0x7fde8eu, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    return OBJECT_FLOW_DISPATCH;
}

/* $83:E270: object opcode $2E. */
static ObjectFlow ObjectOp2E(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
    Write8(memory, LongIndexedAddress(0x7fe33eu, cpu->x), A8(cpu));
    IncrementY16(cpu);
    return OBJECT_FLOW_DISPATCH;
}

/* $83:E5B5: object opcode $1D. */
static ObjectFlow ObjectOp1D(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, 0xa9u);
    Write8(memory, AbsoluteIndexedAddress(cpu, 0x1724u, 0), (uint8_t)cpu->x);
    Write8(memory, AbsoluteIndexedAddress(cpu, 0x1725u, 0),
        (uint8_t)(cpu->x >> 8));
    return OBJECT_FLOW_DISPATCH;
}

/* $83:E810: object opcode $13. */
static ObjectFlow ObjectOp13(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, 0xa9u);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, 0x001220u));
    cpu->carry = 0;
    Add16Immediate(cpu, 0x0080u);
    Write16Long(memory, LongIndexedAddress(0x7fda6cu, cpu->x), cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, 0x001228u));
    cpu->carry = 0;
    Add16Immediate(cpu, 0x0070u);
    Write16Long(memory, LongIndexedAddress(0x7fdaacu, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    return OBJECT_FLOW_DISPATCH;
}

/* $83:EA7D: object opcode $22. */
static ObjectFlow ObjectOp22(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadAAbsolute8(memory, cpu, 0x066au, 0);
    And8(cpu, 0x06u);
    Or8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x0000u, cpu->y)));
    StoreAAbsolute8(memory, cpu, 0x066au, 0);
    IncrementY16(cpu);
    return OBJECT_FLOW_DISPATCH;
}

/* $83:EA8C: object opcode $23. */
static ObjectFlow ObjectOp23(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    TransferDirectToA(cpu);
    Write8(memory, 0x7fd0a1u, A8(cpu));
    return OBJECT_FLOW_DISPATCH;
}

/* $83:EA94: object opcode $8D. */
static ObjectFlow ObjectOp8D(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
    Or8(cpu, Read8(memory, 0x7fd0a1u));
    Write8(memory, 0x7fd0a1u, A8(cpu));
    IncrementY16(cpu);
    return OBJECT_FLOW_DISPATCH;
}

/* $83:EAB5: object opcode $7x. */
static ObjectFlow ObjectOp7XLoopStart(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadAAbsolute8(memory, cpu, 0x0001u, cpu->y);
    DecrementA8(cpu);
    Write8(memory, LongIndexedAddress(0x7fe08eu, cpu->x), A8(cpu));
    IncrementY16(cpu);
    IncrementY16(cpu);
    LoadXDirect(memory, cpu, 0xabu);                       /* EAC1 */
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, cpu->y);
    Write16Long(memory, LongIndexedAddress(0x7fdfceu, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    PushDataBank(memory, cpu);                             /* EACC */
    LoadA8(cpu, Pull8(memory, cpu));
    Write8(memory, LongIndexedAddress(0x7fdfd0u, cpu->x), A8(cpu));
    return OBJECT_FLOW_DISPATCH;
}

/* $83:EB50: object opcode $29 $2B $Cx. */
static ObjectFlow ObjectOp292BCX(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    uint8_t set;
    uint32_t target;
    uint8_t bit;

    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
    if (handler == OBJECT_OP_29) {
        set = !cpu->zero;
        target = AbsoluteIndexedAddress(cpu, 0x064au, cpu->x);
        bit = 0x04u;
    } else if (handler == OBJECT_OP_2B) {
        set = cpu->negative;
        target = LongIndexedAddress(0x7fdb0cu, cpu->x);
        bit = 0x40u;
    } else {
        And8(cpu, 0x0fu);
        set = cpu->zero;
        target = AbsoluteIndexedAddress(cpu, 0x064au, cpu->x);
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

/* $83:ED9B: object opcode $84. */
static ObjectFlow ObjectOp84(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA8(cpu, Read8(memory, 0x7fd0a1u));
    Or8(cpu, 0x10u);
    Write8(memory, 0x7fd0a1u, A8(cpu));
    return OBJECT_FLOW_DISPATCH;
}

/* $83:EDC2: object opcode $89. */
static ObjectFlow ObjectOp89(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadA8(cpu, 0xffu);
    Write8(memory, LongIndexedAddress(0x7fe23eu, cpu->x), A8(cpu));
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe33eu, cpu->x)));
    Or8(cpu, 0x80u);
    Write8(memory, LongIndexedAddress(0x7fe33eu, cpu->x), A8(cpu));
    return OBJECT_FLOW_DISPATCH;
}

/* $83:EDD7: object opcode $EA. */
static ObjectFlow ObjectOpEA(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdb0cu, cpu->x)));
    Or8(cpu, 0x80u);
    Write8(memory, LongIndexedAddress(0x7fdb0cu, cpu->x), A8(cpu));
    return OBJECT_FLOW_DISPATCH;
}

/* $83:EFD1: object opcode $F8 $F9. */
static ObjectFlow ObjectOpF8F9(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadAAbsolute8(memory, cpu, 0x064au, cpu->x);
    if (handler == OBJECT_OP_F8)
        Or8(cpu, 0x10u);
    else
        And8(cpu, 0xefu);
    StoreAAbsolute8(memory, cpu, 0x064au, cpu->x);
    return OBJECT_FLOW_DISPATCH;
}

/* $83:F0E8: object opcode $E2. */
static ObjectFlow ObjectOpE2(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, 0xa9u);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->y));
    Write16Long(memory, LongIndexedAddress(0x7fda6cu, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    IncrementY16(cpu);
    IncrementY16(cpu);
    return OBJECT_FLOW_DISPATCH;
}

/* $83:F0FA: object opcode $82 $83. */
static ObjectFlow ObjectOp8283(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    LoadXDirect(memory, cpu, 0xa9u);
    SetAccumulatorWidth(cpu, 0);
    if (handler == OBJECT_OP_82)
        CopyLong16(memory, cpu, 0x7fda6cu, 0x7fdaacu);
    else
        CopyLong16(memory, cpu, 0x7fdaacu, 0x7fda6cu);
    SetAccumulatorWidth(cpu, 1);
    return OBJECT_FLOW_DISPATCH;
}

/* $83:F11C: object opcode $EB. */
static ObjectFlow ObjectOpEB(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    unsigned i;

    LoadXDirect(memory, cpu, 0xa9u);
    /* Each byte negated on its own. */
    for (i = 0; i < 2u; ++i) {
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdaacu + i, cpu->x)));
        LoadA8(cpu, (uint8_t)(A8(cpu) ^ 0xffu));
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        Write8(memory, LongIndexedAddress(0x7fda6cu + i, cpu->x), A8(cpu));
    }
    return OBJECT_FLOW_DISPATCH;
}

/* $83:F137: object opcode $EC. */
static ObjectFlow ObjectOpEC(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
    cpu->carry = 1;
    Sbc8(cpu, Read8(memory, LongIndexedAddress(0x7fe08eu, cpu->x)));
    DecrementA8(cpu);
    Write8(memory, LongIndexedAddress(0x7fe08eu, cpu->x), A8(cpu));
    IncrementY16(cpu);
    SetAccumulatorWidth(cpu, 0);                           /* F147 */
    LoadXDirect(memory, cpu, 0xabu);
    LoadA16(cpu, cpu->y);
    Write16Long(memory, LongIndexedAddress(0x7fdfceu, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    return OBJECT_FLOW_DISPATCH;
}

/* $83:F155: object opcode $ED. */
static ObjectFlow ObjectOpED(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->y));
    cpu->carry = 0;
    Add16Immediate(cpu, 0x8ec7u);
    Write16Long(memory, 0x7fddacu, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    PushDataBank(memory, cpu);                             /* F164 */
    PushY(memory, cpu);
    LoadA8(cpu, 0x7fu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    LoadX16(cpu, 0x0000u);
    do {
        TransferDirectToA(cpu);                            /* F16D */
        LoadAAbsolute8(memory, cpu, 0xd0a6u, cpu->x);
        if (!cpu->negative) {
            Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
            AslA8(cpu);
            Adc8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
            TransferAToY(cpu);
            LoadAAbsolute8(memory, cpu, 0xddacu, 0);
            StoreAAbsolute8(memory, cpu, 0xdeeeu, cpu->y);
            LoadAAbsolute8(memory, cpu, 0xddadu, 0);
            StoreAAbsolute8(memory, cpu, 0xdeefu, cpu->y);
        }
        IncrementX16(cpu);                                 /* F185 */
        Compare16(cpu, cpu->x, 0x0008u);
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
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
        And8(cpu, 0x0fu);
        back = 0xe1ceu;
    } else if (handler == OBJECT_OP_2C_SET_FRAME) {
        LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
        cpu->carry = 0;
        Adc8(cpu, Read8(memory, LongIndexedAddress(0x7fdb2cu, cpu->x)));
        back = 0xe1e1u;
    } else if (handler == OBJECT_OP_25_SET_FRAME) {
        LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fda2cu, cpu->x)));
        back = 0xe1f0u;
        skip_cursor = 1;
    } else if (handler == OBJECT_OP_FD_SET_FRAME) {
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
        back = 0xe1fbu;
    } else {
        LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdb0cu, cpu->x)));
        BitImmediate8(cpu, 0x20u);
        if (cpu->zero)
            return OBJECT_FLOW_DISPATCH;
        LoadA8(cpu, Read8(memory, 0x7fd4f4u));
        back = 0xe5b1u;
        skip_cursor = 1;
    }
    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
    ObjectSetFrame(memory, cpu, back);
    if (!skip_cursor)
        IncrementY16(cpu);
    return OBJECT_FLOW_DISPATCH;
}

/* $83:E840: object opcode $5x $28 $Dx. */
static ObjectFlow ObjectOp5X28DX(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    if (handler == OBJECT_OP_5X) {
        LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd9ccu, cpu->x)));
        Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
        Write8(memory, DirectAddress(cpu, 0x55u), 0x00u);
        SetAccumulatorWidth(cpu, 0);                       /* E84A */
        LoadA16(cpu, cpu->y);
        cpu->carry = 0;
        Add16Value(cpu, Read16Direct(memory, cpu, 0x54u));
        TransferAToY(cpu);
    } else if (handler == OBJECT_OP_28) {
        LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fda2cu, cpu->x)));
        AslA8(cpu);
        Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
        Write8(memory, DirectAddress(cpu, 0x55u), 0x00u);
        SetAccumulatorWidth(cpu, 0);                       /* EB43 */
        LoadA16(cpu, cpu->y);
        cpu->carry = 0;
        Add16Value(cpu, Read16Direct(memory, cpu, 0x54u));
        LoadA16(cpu, (uint16_t)(cpu->accumulator - 1u));
        TransferAToY(cpu);
        SetAccumulatorWidth(cpu, 1);
    }
    SimulateJsrFrame(memory, cpu, 0xed34u);                /* ED32 */
    SetAccumulatorWidth(cpu, 0);                           /* ED38 */
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0001u, cpu->y));
    cpu->carry = 0;
    Add16Immediate(cpu, 0x8ec7u);
    TransferAToY(cpu);
    SetAccumulatorWidth(cpu, 1);
    SimulateRtsFrame(memory, cpu);
    return OBJECT_FLOW_DISPATCH;
}

/* $83:EE48: object opcode $FB. */
static ObjectFlow ObjectOpFB(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
    SimulateJsrFrame(memory, cpu, 0xee4fu);
    LoadXDirect(memory, cpu, 0xa9u);                       /* EE56 */
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    ObjectSignNibble(memory, cpu, 0xee61u);
    ObjectAddPosition(memory, cpu, 0x7fddfeu, 0xee64u);
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    And8(cpu, 0x0fu);
    ObjectSignNibble(memory, cpu, 0xee6cu);
    ObjectAddPosition(memory, cpu, 0x7fde8eu, 0xee6fu);
    SimulateRtsFrame(memory, cpu);
    TransferDirectToA(cpu);                                /* EE50 */
    LoadA8(cpu, 0x01u);
    return ObjectAdvance(memory, cpu);
}

/* $83:EE71: object opcode $FC $E3. */
static ObjectFlow ObjectOpFCE3(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    const uint8_t fixed = handler == OBJECT_OP_E3;

    if (fixed) {
        LoadXDirect(memory, cpu, 0xa9u);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fda6cu, cpu->x)));
        Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fda6du, cpu->x)));
        Write8(memory, DirectAddress(cpu, 0x55u), A8(cpu));
    } else {
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
        Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->y);
        Write8(memory, DirectAddress(cpu, 0x55u), A8(cpu));
    }
    SimulateJsrFrame(memory, cpu, fixed ? 0xf1afu : 0xee7du);
    LoadXDirect(memory, cpu, 0xa9u);                       /* EE84 */
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    Lufia2SignExtendA8(memory, cpu, 0xee8bu);
    ObjectAddPosition(memory, cpu, 0x7fddfeu, 0xee8eu);
    TransferDirectToA(cpu);                                /* EE91 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x55u)));
    Lufia2SignExtendA8(memory, cpu, 0xee96u);
    ObjectAddPosition(memory, cpu, 0x7fde8eu, 0xee99u);
    SimulateRtsFrame(memory, cpu);
    TransferDirectToA(cpu);                                /* EE7E/F1B0 */
    if (!fixed)
        LoadA8(cpu, 0x02u);
    return ObjectAdvance(memory, cpu);
}

/* $83:E191: object opcode $F7. */
static ObjectFlow ObjectOpF7(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, 0xa9u);
    TransferDirectToA(cpu);
    LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
    Lufia2SignExtendA8(memory, cpu, 0xe199u);
    ObjectAddPosition(memory, cpu, 0x7fdcdcu, 0xe19cu);
    TransferDirectToA(cpu);                                /* E19D */
    LoadAAbsolute8(memory, cpu, 0x0001u, cpu->y);
    Lufia2SignExtendA8(memory, cpu, 0xe1a3u);
    ObjectAddPosition(memory, cpu, 0x7fdd6cu, 0xe1a6u);
    TransferDirectToA(cpu);                                /* E1A7 */
    LoadA8(cpu, 0x02u);
    return ObjectAdvance(memory, cpu);
}

/* $83:E99A: object opcode $20. */
static ObjectFlow ObjectOp20(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    unsigned i;

    LoadXDirect(memory, cpu, 0xa9u);
    LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
    IncrementY16(cpu);
    TransferDirectToA(cpu);                                /* E9A2 */
    for (i = 0; i < 2u; ++i) {
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdaacu + i, cpu->x)));
        if (cpu->zero)
            continue;
        if (cpu->negative) {
            cpu->carry = 0;
            Adc8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
        } else {
            cpu->carry = 1;
            Sbc8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
        }
        LoadA8(cpu, (uint8_t)(A8(cpu) ^ 0xffu));
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        Write8(memory, LongIndexedAddress(0x7fda6cu + i, cpu->x), A8(cpu));
    }
    return OBJECT_FLOW_DISPATCH;
}

/* $83:EDE6: object opcode $F1. */
static ObjectFlow ObjectOpF1(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    uint8_t found = 0;

    LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
    LoadX16(cpu, 0x0000u);
    for (;;) {
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe1aeu, cpu->x)));
        Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x54u)));
        if (cpu->zero) {
            found = 1;
            break;
        }
        IncrementX16(cpu);                                 /* EDF6 */
        Compare16(cpu, cpu->x, 0x0020u);
        if (cpu->zero)
            break;
    }
    if (!found) {
        /* No match: the ROM clears slot 32. */
        TransferDirectToA(cpu);                            /* EDFC */
        Write8(memory, LongIndexedAddress(0x7fe286u, cpu->x), A8(cpu));
        Write8(memory, LongIndexedAddress(0x7fe2ceu, cpu->x), A8(cpu));
    } else {
        ObjectTakeAnimation(memory, cpu, 0xee09u);
    }
    IncrementY16(cpu);                                     /* EE0A */
    return OBJECT_FLOW_DISPATCH;
}

/* $83:E437: object opcode $19 $8E. */
static ObjectFlow ObjectOp198E(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    const uint8_t animation = handler == OBJECT_OP_19
        ? Read8(memory, AbsoluteIndexedAddress(cpu, 0x0000u, cpu->y))
        : Read8(memory, 0x7fd4f3u);

    if (ObjectSpriteSizeZero(memory, cpu, animation)) {
        cpu->resume_pc = 0x830000u | handler;
        return OBJECT_FLOW_BOUNDARY;
    }
    if (handler == OBJECT_OP_19) {
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
        Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
        IncrementY16(cpu);
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
        Write8(memory, DirectAddress(cpu, 0x55u), A8(cpu));
        IncrementY16(cpu);
    } else {
        LoadA8(cpu, Read8(memory, 0x7fd4f3u));
        Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
        LoadA8(cpu, Read8(memory, 0x7fd4f4u));
        Write8(memory, DirectAddress(cpu, 0x55u), A8(cpu));
    }
    LoadX16(cpu, 0x0000u);                                 /* E453 */
    for (;;) {
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe1aeu, cpu->x)));
        Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x54u)));
        if (cpu->zero) {
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdb2cu, cpu->x)));
            Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x55u)));
            if (cpu->zero) {
                ObjectTakeAnimation(memory, cpu, 0xe483u); /* E481 */
                return OBJECT_FLOW_DISPATCH;
            }
        }
        IncrementX16(cpu);                                 /* E466 */
        Compare16(cpu, cpu->x, 0x0020u);
        if (cpu->zero)
            break;
    }
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);               /* E46C */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdb0cu, cpu->x)));
    Or8(cpu, 0x20u);
    Write8(memory, LongIndexedAddress(0x7fdb0cu, cpu->x), A8(cpu));
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    ObjectAnimationSetup(memory, cpu, 0xe47du);
    return OBJECT_FLOW_DISPATCH;
}

/* $83:EAFF: object opcode $26. */
static ObjectFlow ObjectOp26(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    if (ObjectSpriteSizeZero(memory, cpu, Read8(memory,
            LongIndexedAddress(0x7fda4cu, Read16Direct(memory, cpu, DP_ACTOR_SLOT))))) {
        cpu->resume_pc = 0x830000u | handler;
        return OBJECT_FLOW_BOUNDARY;
    }
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fda4cu, cpu->x)));
    ObjectAnimationSetup(memory, cpu, 0xeb08u);
    return OBJECT_FLOW_DISPATCH;
}

/* $83:EC8E: object opcode $Ax. */
static ObjectFlow ObjectOpAX(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    if (ObjectSpriteSizeZero(memory, cpu, Read8(memory,
            AbsoluteIndexedAddress(cpu, 0x0001u, cpu->y)))) {
        cpu->resume_pc = 0x830000u | handler;
        return OBJECT_FLOW_BOUNDARY;
    }
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    TransferDirectToA(cpu);
    LoadAAbsolute8(memory, cpu, 0x0001u, cpu->y);
    ObjectAnimationSetup(memory, cpu, 0xec96u);
    IncrementY16(cpu);
    IncrementY16(cpu);
    return OBJECT_FLOW_DISPATCH;
}

/* $83:E854: object opcode $6x. */
static ObjectFlow ObjectOp6X(
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

/* $83:E860: object opcode $85. */
static ObjectFlow ObjectOp85(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    PushY(memory, cpu);
    ObjectSpawnChild(memory, cpu, 0xe863u);
    cpu->y = PullIndexValue(memory, cpu);
    TransferDirectToA(cpu);                                /* E865 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_ACTOR_SLOT)));
    Write8(memory, DirectAddress(cpu, 0x55u), A8(cpu));
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    Write8(memory, DirectAddress(cpu, DP_ACTOR_SLOT), A8(cpu));
    ObjectTakeAnimation(memory, cpu, 0xe871u);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x55u)));
    Write8(memory, DirectAddress(cpu, DP_ACTOR_SLOT), A8(cpu));
    IncrementY16(cpu);
    IncrementY16(cpu);
    IncrementY16(cpu);
    return OBJECT_FLOW_DISPATCH;
}

/* $83:E87C: object opcode $8A. */
static ObjectFlow ObjectOp8A(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    PushY(memory, cpu);
    ObjectSpawnChild(memory, cpu, 0xe87fu);
    cpu->y = PullIndexValue(memory, cpu);
    TransferDirectToA(cpu);                                /* E881 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    AslA8(cpu);
    TransferAToX(cpu);
    LoadAAbsolute8(memory, cpu, 0x0003u, cpu->y);
    Lufia2SignExtendA8(memory, cpu, 0xe88bu);
    Write16Long(memory, LongIndexedAddress(0x7fdcdcu, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    /* No TDC: B keeps the first high byte. */
    LoadAAbsolute8(memory, cpu, 0x0004u, cpu->y);
    Lufia2SignExtendA8(memory, cpu, 0xe897u);
    Write16Long(memory, LongIndexedAddress(0x7fdd6cu, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    IncrementY16(cpu);
    IncrementY16(cpu);
    IncrementY16(cpu);
    IncrementY16(cpu);
    IncrementY16(cpu);
    return OBJECT_FLOW_DISPATCH;
}

/* $83:E937: object opcode $EE. */
static ObjectFlow ObjectOpEE(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    PushY(memory, cpu);
    ObjectSpawnChild(memory, cpu, 0xe93au);
    cpu->y = PullIndexValue(memory, cpu);
    IncrementY16(cpu);
    IncrementY16(cpu);
    IncrementY16(cpu);
    TransferDirectToA(cpu);                                /* E93F */
    LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
    Write8(memory, LongIndexedAddress(0x7fda6cu, cpu->x), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x0001u, cpu->y);
    Write8(memory, LongIndexedAddress(0x7fda6du, cpu->x), A8(cpu));
    IncrementY16(cpu);
    IncrementY16(cpu);
    LoadX16(cpu, 0x0000u);                                 /* E950 */
    for (;;) {
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd0a6u, cpu->x)));
        if (cpu->negative)
            break;
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x0008u);
        if (cpu->zero) {
            LoadX16(cpu, 0x0000u);
            break;
        }
    }
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u))); /* E962 */
    Write8(memory, LongIndexedAddress(0x7fd0a6u, cpu->x), A8(cpu));
    return OBJECT_FLOW_DISPATCH;
}

/* $83:E13D: object opcode $0x. */
static ObjectFlow ObjectOp0X(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    ObjectDespawn(memory, cpu);
    PullDataBank(memory, cpu);                             /* E141 */
    return OBJECT_FLOW_RETURN;
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
    case OBJECT_OP_2F:
        return ObjectOp2F(memory, cpu);
    case OBJECT_OP_2E:
        return ObjectOp2E(memory, cpu);
    case OBJECT_OP_1D:
        return ObjectOp1D(memory, cpu);
    case OBJECT_OP_12:                                         /* no-op */
        return OBJECT_FLOW_DISPATCH;
    case OBJECT_OP_13:
        return ObjectOp13(memory, cpu);
    case OBJECT_OP_22:
        return ObjectOp22(memory, cpu);
    case OBJECT_OP_23:
        return ObjectOp23(memory, cpu);
    case OBJECT_OP_8D:
        return ObjectOp8D(memory, cpu);
    case OBJECT_OP_7X_LOOP_START:
        return ObjectOp7XLoopStart(memory, cpu);
    case OBJECT_OP_29:
    case OBJECT_OP_2B:
    case OBJECT_OP_CX:
        return ObjectOp292BCX(memory, cpu, handler);
    case OBJECT_OP_84:
        return ObjectOp84(memory, cpu);
    case OBJECT_OP_89:
        return ObjectOp89(memory, cpu);
    case OBJECT_OP_EA:
        return ObjectOpEA(memory, cpu);
    case OBJECT_OP_F8:
    case OBJECT_OP_F9:
        return ObjectOpF8F9(memory, cpu, handler);
    case OBJECT_OP_E2:
        return ObjectOpE2(memory, cpu);
    case OBJECT_OP_82:
    case OBJECT_OP_83:
        return ObjectOp8283(memory, cpu, handler);
    case OBJECT_OP_EB:
        return ObjectOpEB(memory, cpu);
    case OBJECT_OP_EC:
        return ObjectOpEC(memory, cpu);
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
        return ObjectOp5X28DX(memory, cpu, handler);
    case OBJECT_OP_FB:
        return ObjectOpFB(memory, cpu);
    case OBJECT_OP_FC:
    case OBJECT_OP_E3:
        return ObjectOpFCE3(memory, cpu, handler);
    case OBJECT_OP_F7:
        return ObjectOpF7(memory, cpu);
    case OBJECT_OP_20:
        return ObjectOp20(memory, cpu);
    case OBJECT_OP_F1:
        return ObjectOpF1(memory, cpu);
    case OBJECT_OP_19:
    case OBJECT_OP_8E:
        return ObjectOp198E(memory, cpu, handler);
    case OBJECT_OP_26:
        return ObjectOp26(memory, cpu, handler);
    case OBJECT_OP_AX:
        return ObjectOpAX(memory, cpu, handler);
    case OBJECT_OP_6X:
        return ObjectOp6X(memory, cpu);
    case OBJECT_OP_85:
        return ObjectOp85(memory, cpu);
    case OBJECT_OP_8A:
        return ObjectOp8A(memory, cpu);
    case OBJECT_OP_EE:
        return ObjectOpEE(memory, cpu);
    case OBJECT_OP_0X:
        return ObjectOp0X(memory, cpu);
    default:
        cpu->resume_pc = 0x830000u | handler;
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
    LoadXDirect(memory, cpu, 0xabu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdef0u, cpu->x)));
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdeefu, cpu->x)));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdeeeu, cpu->x)));
    TransferAToY(cpu);
    /* A script that never yields spins the ROM forever. */
    for (steps = 0; steps < 0x10000u; ++steps) {
        uint16_t handler;
        ObjectFlow flow;

        ++*dispatches;
        TransferDirectToA(cpu);                                /* E10F */
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
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
    Write8(memory, 0x7fddacu, A8(cpu));
    Write8(memory, 0x7fddadu, A8(cpu));
    Write8(memory, AbsoluteIndexedAddress(cpu, 0x09a8u, 0), 0x00u);
    LoadX16(cpu, 0x0000u);
    do {
        LoadAAbsolute8(memory, cpu, 0x064au, cpu->x);          /* E04F */
        BitImmediate8(cpu, 0x80u);
        if (!cpu->zero) {
            const uint32_t flags = LongIndexedAddress(0x7fdb0cu, cpu->x);
            const uint32_t blink = LongIndexedAddress(0x7fe386u, cpu->x);
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
                    ToggleLong8(
                        memory, cpu,
                        LongIndexedAddress(0x7fe33eu, cpu->x), 0x80u);
                }
            }
            LoadAAbsolute8(memory, cpu, 0x09a7u, 0);           /* E088 */
            BitImmediate8(cpu, 0x01u);
            if (!cpu->zero) {
                LoadA8(cpu, Read8(memory, flags));             /* E08F */
                BitImmediate8(cpu, 0x40u);
                if (!cpu->zero) {
                    LoadAAbsolute8(memory, cpu, 0x064au, cpu->x);
                    LoadA8(cpu, (uint8_t)(A8(cpu) ^ 0xffu));
                    TestBitsAbsolute8(memory, cpu, 0x09a8u, 1);
                }
                LoadAAbsolute8(memory, cpu, 0x064au, cpu->x);  /* E09F */
                BitImmediate8(cpu, 0x08u);
                if (!cpu->zero) {
                    BitImmediate8(cpu, 0x40u);
                    animate = cpu->zero;
                }
                if (!animate) {
                    And8(cpu, 0xbfu);                          /* E0AA */
                    StoreAAbsolute8(memory, cpu, 0x064au, cpu->x);
                }
            }
            if (!animate) {
                const uint32_t wait = LongIndexedAddress(0x7fdfaeu, cpu->x);

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
                    SimulateRtsFrame(memory, cpu);
                    LoadXDirect16(memory, cpu, DP_ACTOR_SLOT); /* E0C3 */
                }
            } else {
                const uint32_t step = LongIndexedAddress(0x7fdaecu, cpu->x);

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
                    ToggleLong8(
                        memory, cpu,
                        LongIndexedAddress(0x7fdb2cu, cpu->x), 0x01u);
                    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
                    ObjectSetFrame(memory, cpu, 0xe0edu);      /* E0EB */
                    LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);
                }
            }
        }
        IncrementX16(cpu);                                     /* E0F0 */
        Compare16(cpu, cpu->x, 0x0020u);
    } while (!cpu->zero);
    SetIndexWidth(cpu, 1);                                     /* E0F9 */
    return result;
}
