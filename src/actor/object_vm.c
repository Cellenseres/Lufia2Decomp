/* Field object slots and script VM ($83:E03E). */

#include "core/cpu_internal.h"
#include "lufia2/actor.h"
#include "actor/actor_internal.h"
#include "system/system_internal.h"

/* $83:E200: sprite frame $54 for object $A7. */
static void ObjectSetFrame(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    PushY(memory, cpu);                                        /* E200 */
    LoadXDirect(memory, cpu, 0xa7u);
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    Write8(memory, LongIndexedAddress(0x7fdb2cu, cpu->x), A8(cpu));
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    And8(cpu, 0x3fu);
    Write8(memory, 0x004202u, A8(cpu));
    TransferDirectToA(cpu);                                    /* E212 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe23eu, cpu->x)));
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x83abf8u, cpu->x)));
    Write8(memory, 0x004203u, A8(cpu));
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0732u, 0));
    LoadXDirect(memory, cpu, 0xabu);                           /* E223 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe0f0u, cpu->x)));
    Write8(memory, LongIndexedAddress(0x7fd90eu, cpu->x), A8(cpu));
    LoadA8(cpu, Read8(memory, 0x004216u));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, 0x00u);
    SetAccumulatorWidth(cpu, 0);                               /* E234 */
    LsrA16(cpu);
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, LongIndexedAddress(0x7fe0eeu, cpu->x)));
    Write16Long(memory, LongIndexedAddress(0x7fd90cu, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadXDirect(memory, cpu, 0xa7u);                           /* E242 */
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
    LoadXDirect(memory, cpu, 0xa7u);                           /* E12C */
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
    Lufia2CallRandomScale(memory, cpu, 0xeee4u);                   /* $80:8299 */
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
    LoadXDirect(memory, cpu, 0xa7u);
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
    LoadXDirect(memory, cpu, 0xa7u);
    LoadAAbsolute8(memory, cpu, 0x064au, cpu->x);
    And8(cpu, 0xfbu);
    StoreAAbsolute8(memory, cpu, 0x064au, cpu->x);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe23eu, cpu->x)));
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x83abf4u, cpu->x)));
    Lufia2SpriteAllocSlots(memory, cpu, 0xecf6u);
    LoadXDirect(memory, cpu, 0xa7u);                           /* ECF7 */
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
    LoadXDirect(memory, cpu, 0xa7u);
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
    LoadXDirect(memory, cpu, 0xa7u);                           /* E8BC */
    PushIndex(memory, cpu);
    LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
    SimulateJslFrame(memory, cpu, 0x83u, 0xe8c5u);
    Lufia2ActorSpawn(memory, cpu);                          /* DF87 */
    SimulateRtlFrame(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);                      /* E8C6 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0xa7u)));
    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
    Write8(memory, DirectAddress(cpu, 0x55u), 0x00u);
    LoadXDirect(memory, cpu, 0xa9u);
    StoreYDirect16(memory, cpu, 0xa7u);
    SimulateJslFrame(memory, cpu, 0x83u, 0xe8d4u);
    Lufia2ActorRecordOffsets(memory, cpu);                       /* AB4F */
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
    LoadXDirect(memory, cpu, 0xa7u);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fda2cu, cpu->x)));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd9ccu, cpu->x)));
    LoadXDirect(memory, cpu, 0x54u);
    Write8(memory, LongIndexedAddress(0x7fd9ccu, cpu->x), A8(cpu));
    ExchangeAccumulatorBytes(cpu);
    Write8(memory, LongIndexedAddress(0x7fda2cu, cpu->x), A8(cpu));
    TransferDirectToA(cpu);                                    /* E924 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0xa7u)));
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
    LoadXDirect(memory, cpu, 0xa7u);                           /* F205 */
    LoadA8(cpu, 0x04u);
    StoreAAbsolute8(memory, cpu, 0x064au, cpu->x);
    SetIndexWidth(cpu, 1);
    LoadXDirect(memory, cpu, 0xa7u);
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
    LoadXDirect(memory, cpu, 0xa7u);                           /* F238 */
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

static ObjectFlow ObjectExecute(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    switch (handler) {
    case 0xed45u:
        handler = ObjectSubDispatch(memory, cpu, 0xf368u, 0xed47u);
        break;
    case 0xed4bu:
        handler = ObjectSubDispatch(memory, cpu, 0xf348u, 0xed4du);
        break;
    case 0xed51u:
        handler = ObjectSubDispatch(memory, cpu, 0xf328u, 0xed53u);
        break;
    case 0xed57u:
        handler = ObjectSubDispatch(memory, cpu, 0xf308u, 0xed59u);
        break;
    case 0xed5du:
        handler = ObjectSubDispatch(memory, cpu, 0xf2e8u, 0xed5fu);
        break;
    default:
        break;
    }

    switch (handler) {
    case 0xe831u:                                              /* 4x */
        LoadXDirect(memory, cpu, 0xa7u);
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
        And8(cpu, 0x0fu);
        Write8(memory, LongIndexedAddress(0x7fdfaeu, cpu->x), A8(cpu));
        IncrementY16(cpu);
        return ObjectSaveAndReturn(memory, cpu);

    case 0xe168u:                                              /* F6 */
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

    case 0xeeb5u:                                              /* F2 */
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

    case 0xe589u:                                              /* 1A */
        LoadXDirect(memory, cpu, 0xa7u);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdb0cu, cpu->x)));
        BitImmediate8(cpu, 0x20u);
        if (!cpu->zero) {
            LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);      /* E593 */
            Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
            ObjectSetFrame(memory, cpu, 0xe59au);
        }
        IncrementY16(cpu);                                     /* E59B */
        return OBJECT_FLOW_DISPATCH;

    case 0xead5u:                                              /* 80 */
        LoadXDirect(memory, cpu, 0xa7u);
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

    case 0xe150u:                                              /* 2F */
        LoadXDirect(memory, cpu, 0xa9u);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16Long(memory, 0x7fddaeu));
        Write16Long(memory, LongIndexedAddress(0x7fddfeu, cpu->x), cpu->accumulator);
        LoadA16(cpu, Read16Long(memory, 0x7fde3eu));
        Write16Long(memory, LongIndexedAddress(0x7fde8eu, cpu->x), cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        return OBJECT_FLOW_DISPATCH;

    case 0xe270u:                                              /* 2E */
        LoadXDirect(memory, cpu, 0xa7u);
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
        Write8(memory, LongIndexedAddress(0x7fe33eu, cpu->x), A8(cpu));
        IncrementY16(cpu);
        return OBJECT_FLOW_DISPATCH;

    case 0xe5b5u:                                              /* 1D */
        LoadXDirect(memory, cpu, 0xa9u);
        Write8(memory, AbsoluteIndexedAddress(cpu, 0x1724u, 0), (uint8_t)cpu->x);
        Write8(memory, AbsoluteIndexedAddress(cpu, 0x1725u, 0),
            (uint8_t)(cpu->x >> 8));
        return OBJECT_FLOW_DISPATCH;

    case 0xe700u:                                              /* 12 */
        return OBJECT_FLOW_DISPATCH;

    case 0xe810u:                                              /* 13 */
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

    case 0xea7du:                                              /* 22 */
        LoadAAbsolute8(memory, cpu, 0x066au, 0);
        And8(cpu, 0x06u);
        Or8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x0000u, cpu->y)));
        StoreAAbsolute8(memory, cpu, 0x066au, 0);
        IncrementY16(cpu);
        return OBJECT_FLOW_DISPATCH;

    case 0xea8cu:                                              /* 23 */
        TransferDirectToA(cpu);
        Write8(memory, 0x7fd0a1u, A8(cpu));
        return OBJECT_FLOW_DISPATCH;

    case 0xea94u:                                              /* 8D */
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
        Or8(cpu, Read8(memory, 0x7fd0a1u));
        Write8(memory, 0x7fd0a1u, A8(cpu));
        IncrementY16(cpu);
        return OBJECT_FLOW_DISPATCH;

    case 0xeab5u:                                              /* 7x */
        LoadXDirect(memory, cpu, 0xa7u);
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

    case 0xeb50u:                                              /* 29 */
    case 0xec6du:                                              /* 2B */
    case 0xed11u: {                                            /* Cx */
        uint8_t set;
        uint32_t target;
        uint8_t bit;

        LoadXDirect(memory, cpu, 0xa7u);
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
        if (handler == 0xeb50u) {
            set = !cpu->zero;
            target = AbsoluteIndexedAddress(cpu, 0x064au, cpu->x);
            bit = 0x04u;
        } else if (handler == 0xec6du) {
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

    case 0xed9bu:                                              /* 84 */
        LoadA8(cpu, Read8(memory, 0x7fd0a1u));
        Or8(cpu, 0x10u);
        Write8(memory, 0x7fd0a1u, A8(cpu));
        return OBJECT_FLOW_DISPATCH;

    case 0xedc2u:                                              /* 89 */
        LoadXDirect(memory, cpu, 0xa7u);
        LoadA8(cpu, 0xffu);
        Write8(memory, LongIndexedAddress(0x7fe23eu, cpu->x), A8(cpu));
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe33eu, cpu->x)));
        Or8(cpu, 0x80u);
        Write8(memory, LongIndexedAddress(0x7fe33eu, cpu->x), A8(cpu));
        return OBJECT_FLOW_DISPATCH;

    case 0xedd7u:                                              /* EA */
        LoadXDirect(memory, cpu, 0xa7u);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdb0cu, cpu->x)));
        Or8(cpu, 0x80u);
        Write8(memory, LongIndexedAddress(0x7fdb0cu, cpu->x), A8(cpu));
        return OBJECT_FLOW_DISPATCH;

    case 0xefd1u:                                              /* F8 */
    case 0xefdeu:                                              /* F9 */
        LoadXDirect(memory, cpu, 0xa7u);
        LoadAAbsolute8(memory, cpu, 0x064au, cpu->x);
        if (handler == 0xefd1u)
            Or8(cpu, 0x10u);
        else
            And8(cpu, 0xefu);
        StoreAAbsolute8(memory, cpu, 0x064au, cpu->x);
        return OBJECT_FLOW_DISPATCH;

    case 0xf0e8u:                                              /* E2 */
        LoadXDirect(memory, cpu, 0xa9u);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->y));
        Write16Long(memory, LongIndexedAddress(0x7fda6cu, cpu->x), cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        IncrementY16(cpu);
        IncrementY16(cpu);
        return OBJECT_FLOW_DISPATCH;

    case 0xf0fau:                                              /* 82 */
    case 0xf10bu:                                              /* 83 */
        LoadXDirect(memory, cpu, 0xa9u);
        SetAccumulatorWidth(cpu, 0);
        if (handler == 0xf0fau)
            CopyLong16(memory, cpu, 0x7fda6cu, 0x7fdaacu);
        else
            CopyLong16(memory, cpu, 0x7fdaacu, 0x7fda6cu);
        SetAccumulatorWidth(cpu, 1);
        return OBJECT_FLOW_DISPATCH;

    case 0xf11cu: {                                            /* EB */
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

    case 0xf137u:                                              /* EC */
        LoadXDirect(memory, cpu, 0xa7u);
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

    case 0xf155u:                                              /* ED */
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

    case 0xe1c5u:                                              /* 3x */
    case 0xe1d3u:                                              /* 2C */
    case 0xe1e6u:                                              /* 25 */
    case 0xe1f4u:                                              /* FD */
    case 0xe59fu: {                                            /* 8F */
        uint16_t back;
        uint8_t skip_cursor = 0;

        if (handler == 0xe1c5u) {
            LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
            And8(cpu, 0x0fu);
            back = 0xe1ceu;
        } else if (handler == 0xe1d3u) {
            LoadXDirect(memory, cpu, 0xa7u);
            LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
            cpu->carry = 0;
            Adc8(cpu, Read8(memory, LongIndexedAddress(0x7fdb2cu, cpu->x)));
            back = 0xe1e1u;
        } else if (handler == 0xe1e6u) {
            LoadXDirect(memory, cpu, 0xa7u);
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fda2cu, cpu->x)));
            back = 0xe1f0u;
            skip_cursor = 1;
        } else if (handler == 0xe1f4u) {
            LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
            back = 0xe1fbu;
        } else {
            LoadXDirect(memory, cpu, 0xa7u);
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

    case 0xe840u:                                              /* 5x */
    case 0xeb38u:                                              /* 28 */
    case 0xed32u: {                                            /* Dx */
        if (handler == 0xe840u) {
            LoadXDirect(memory, cpu, 0xa7u);
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd9ccu, cpu->x)));
            Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
            Write8(memory, DirectAddress(cpu, 0x55u), 0x00u);
            SetAccumulatorWidth(cpu, 0);                       /* E84A */
            LoadA16(cpu, cpu->y);
            cpu->carry = 0;
            Add16Value(cpu, Read16Direct(memory, cpu, 0x54u));
            TransferAToY(cpu);
        } else if (handler == 0xeb38u) {
            LoadXDirect(memory, cpu, 0xa7u);
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

    case 0xee48u:                                              /* FB */
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

    case 0xee71u:                                              /* FC */
    case 0xf19fu: {                                            /* E3 */
        const uint8_t fixed = handler == 0xf19fu;

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

    case 0xe191u:                                              /* F7 */
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

    case 0xe99au: {                                            /* 20 */
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

    case 0xede6u: {                                            /* F1 */
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

    case 0xe437u:                                              /* 19 */
    case 0xe445u: {                                            /* 8E */
        const uint8_t animation = handler == 0xe437u
            ? Read8(memory, AbsoluteIndexedAddress(cpu, 0x0000u, cpu->y))
            : Read8(memory, 0x7fd4f3u);

        if (ObjectSpriteSizeZero(memory, cpu, animation)) {
            cpu->resume_pc = 0x830000u | handler;
            return OBJECT_FLOW_BOUNDARY;
        }
        if (handler == 0xe437u) {
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
        LoadXDirect(memory, cpu, 0xa7u);                       /* E46C */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdb0cu, cpu->x)));
        Or8(cpu, 0x20u);
        Write8(memory, LongIndexedAddress(0x7fdb0cu, cpu->x), A8(cpu));
        TransferDirectToA(cpu);
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
        ObjectAnimationSetup(memory, cpu, 0xe47du);
        return OBJECT_FLOW_DISPATCH;
    }

    case 0xeaffu:                                              /* 26 */
        if (ObjectSpriteSizeZero(memory, cpu, Read8(memory,
                LongIndexedAddress(0x7fda4cu, Read16Direct(memory, cpu, 0xa7u))))) {
            cpu->resume_pc = 0x830000u | handler;
            return OBJECT_FLOW_BOUNDARY;
        }
        LoadXDirect(memory, cpu, 0xa7u);
        TransferDirectToA(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fda4cu, cpu->x)));
        ObjectAnimationSetup(memory, cpu, 0xeb08u);
        return OBJECT_FLOW_DISPATCH;

    case 0xec8eu:                                              /* A0 */
        if (ObjectSpriteSizeZero(memory, cpu, Read8(memory,
                AbsoluteIndexedAddress(cpu, 0x0001u, cpu->y)))) {
            cpu->resume_pc = 0x830000u | handler;
            return OBJECT_FLOW_BOUNDARY;
        }
        LoadXDirect(memory, cpu, 0xa7u);
        TransferDirectToA(cpu);
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->y);
        ObjectAnimationSetup(memory, cpu, 0xec96u);
        IncrementY16(cpu);
        IncrementY16(cpu);
        return OBJECT_FLOW_DISPATCH;

    case 0xe854u:                                              /* 60 */
        IncrementY16(cpu);
        PushY(memory, cpu);
        ObjectSpawnChild(memory, cpu, 0xe858u);
        cpu->y = PullIndexValue(memory, cpu);
        IncrementY16(cpu);
        IncrementY16(cpu);
        IncrementY16(cpu);
        return OBJECT_FLOW_DISPATCH;

    case 0xe860u:                                              /* 85 */
        PushY(memory, cpu);
        ObjectSpawnChild(memory, cpu, 0xe863u);
        cpu->y = PullIndexValue(memory, cpu);
        TransferDirectToA(cpu);                                /* E865 */
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0xa7u)));
        Write8(memory, DirectAddress(cpu, 0x55u), A8(cpu));
        TransferAToX(cpu);
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
        Write8(memory, DirectAddress(cpu, 0xa7u), A8(cpu));
        ObjectTakeAnimation(memory, cpu, 0xe871u);
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x55u)));
        Write8(memory, DirectAddress(cpu, 0xa7u), A8(cpu));
        IncrementY16(cpu);
        IncrementY16(cpu);
        IncrementY16(cpu);
        return OBJECT_FLOW_DISPATCH;

    case 0xe87cu:                                              /* 8A */
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

    case 0xe937u:                                              /* EE */
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

    case 0xe13du:                                              /* 0x */
        ObjectDespawn(memory, cpu);
        PullDataBank(memory, cpu);                             /* E141 */
        return OBJECT_FLOW_RETURN;

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
                    StoreXDirect16(memory, cpu, 0xa7u);        /* E0BA */
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
                    LoadXDirect16(memory, cpu, 0xa7u);         /* E0C3 */
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
                    StoreXDirect16(memory, cpu, 0xa7u);
                    SimulateJslFrame(memory, cpu, 0x83u, 0xe0deu);
                    Lufia2ActorRecordOffsets(memory, cpu);
                    SimulateRtlFrame(memory, cpu);
                    ToggleLong8(
                        memory, cpu,
                        LongIndexedAddress(0x7fdb2cu, cpu->x), 0x01u);
                    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
                    ObjectSetFrame(memory, cpu, 0xe0edu);      /* E0EB */
                    LoadXDirect16(memory, cpu, 0xa7u);
                }
            }
        }
        IncrementX16(cpu);                                     /* E0F0 */
        Compare16(cpu, cpu->x, 0x0020u);
    } while (!cpu->zero);
    SetIndexWidth(cpu, 1);                                     /* E0F9 */
    return result;
}
