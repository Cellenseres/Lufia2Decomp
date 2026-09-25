/* Actor sprite slot allocation. */

#include "core/cpu_internal.h"
#include "lufia2/actor.h"
#include "actor/actor_internal.h"

/* $83:ABE9: sprite VRAM base (A.high << 4) + $2000; M=0. */
void Lufia2SpriteVramBase(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x83u, return_address);
    ExchangeAccumulatorBytes(cpu);                             /* ABE9 */
    SetAccumulatorWidth(cpu, 0);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    Add16Immediate(cpu, 0x2000u);
    SimulateRtlFrame(memory, cpu);
}

/* $83:AAAF: free the actor's sprite, clear occupancy. */
static void ActorReleaseSprite(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x83u, return_address);
    SetAccumulatorWidth(cpu, 1);                               /* AAAF */
    SetIndexWidth(cpu, 1);
    LoadXDirect(memory, cpu, 0xa7u);
    LoadAAbsolute8(memory, cpu, 0x0622u, cpu->x);
    Or8(cpu, 0x04u);
    StoreAAbsolute8(memory, cpu, 0x0622u, cpu->x);
    LoadA8(cpu, 0xffu);
    StoreAAbsolute8(memory, cpu, 0x05d2u, cpu->x);
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe25eu, cpu->x)));
    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe2a6u, cpu->x)));
    Write8(memory, DirectAddress(cpu, 0x55u), A8(cpu));
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x83abf4u, cpu->x)));
    Lufia2SpriteFreeSlots(memory, cpu, 0xaad9u);
    SetIndexWidth(cpu, 0);                                     /* AADA */
    SimulateJslFrame(memory, cpu, 0x83u, 0xaadfu);
    Lufia2ActorClearMapOccupancy(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $83:A9E5: sprite descriptor A from $CF:F000. */
static void ActorSpriteDescriptor(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    uint32_t pointer;

    SimulateJslFrame(memory, cpu, 0x83u, return_address);
    PushDataBank(memory, cpu);                                 /* A9E5 */
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 1);
    LoadXDirect(memory, cpu, 0xa7u);
    StoreAAbsolute8(memory, cpu, 0x05d2u, cpu->x);
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, 0x00u);
    ExchangeAccumulatorBytes(cpu);
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0xcff000u, cpu->x)));
    Write16Direct(memory, cpu, 0x5du, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);                               /* A9FB */
    SetIndexWidth(cpu, 1);
    LoadA8(cpu, 0xcfu);
    Write8(memory, DirectAddress(cpu, 0x5fu), A8(cpu));
    LoadXDirect(memory, cpu, 0xa7u);
    pointer = DirectLongPointer(memory, cpu, 0x5du);
    LoadA8(cpu, Read8(memory, pointer));
    And8(cpu, 0x07u);
    Write8(memory, LongIndexedAddress(0x7fe216u, cpu->x), A8(cpu));
    LoadA8(cpu, Read8(memory, pointer));
    And8(cpu, 0xf8u);
    StoreAAbsolute8(memory, cpu, 0x1291u, cpu->x);
    LoadY8(cpu, 0x01u);
    LoadA8(cpu, Read8(memory, (pointer + cpu->y) & 0x00ffffffu));
    AslA8(cpu);
    Write8(memory, LongIndexedAddress(0x7fe1ceu, cpu->x), A8(cpu));
    LoadY8(cpu, (uint8_t)(cpu->y + 1u));
    SetAccumulatorWidth(cpu, 0);                               /* AA1C */
    LoadXDirect(memory, cpu, 0xabu);
    LoadA16(cpu, Read16Long(memory, (pointer + cpu->y) & 0x00ffffffu));
    Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x12b9u, cpu->x),
        cpu->accumulator);
    LoadY8(cpu, (uint8_t)(cpu->y + 1u));
    LoadY8(cpu, (uint8_t)(cpu->y + 1u));
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, (pointer + cpu->y) & 0x00ffffffu));
    StoreAAbsolute8(memory, cpu, 0x12bbu, cpu->x);
    PullDataBank(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $83:AA7D: animation tables for the actor's sprite type. */
static void ActorSpriteTables(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x83u, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* AA7D */
    SetAccumulatorWidth(cpu, 1);
    LoadXDirect(memory, cpu, 0xa7u);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
    AslA8(cpu);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x83abfcu, cpu->x)));
    Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x1381u, cpu->y),
        cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);                               /* AA91 */
    LoadY8(cpu, Read8(memory, DirectAddress(cpu, 0xa7u)));
    LoadA8(cpu, 0xffu);
    StoreAAbsolute8(memory, cpu, 0x1471u, cpu->y);
    LoadAAbsolute8(memory, cpu, 0x1291u, cpu->y);
    And8(cpu, 0x18u);
    LsrA8(cpu);
    LsrA8(cpu);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x83ac14u, cpu->x)));
    LoadY8(cpu, Read8(memory, DirectAddress(cpu, 0xa9u)));
    Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x13d1u, cpu->y),
        cpu->accumulator);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtlFrame(memory, cpu);
}

/* $83:AA50: allocate sprite slots for the actor. */
static void ActorAllocSprite(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* AA50 */
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 1);
    LoadXDirect(memory, cpu, 0xa7u);
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x83abf4u, cpu->x)));
    Lufia2SpriteAllocSlots(memory, cpu, 0xaa62u);
    LoadXDirect(memory, cpu, 0xa7u);                           /* AA63 */
    Write8(memory, LongIndexedAddress(0x7fe25eu, cpu->x), A8(cpu));
    ExchangeAccumulatorBytes(cpu);
    Write8(memory, LongIndexedAddress(0x7fe2a6u, cpu->x), A8(cpu));
    Lufia2SpriteVramBase(memory, cpu, 0xaa71u);
    LoadY8(cpu, Read8(memory, DirectAddress(cpu, 0xa9u)));    /* AA72 */
    Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x1331u, cpu->y),
        cpu->accumulator);
    ActorSpriteTables(memory, cpu, 0xaa7au);
    UnpackStatus(cpu, Pull8(memory, cpu));                     /* AA7B */
    SimulateRtsFrame(memory, cpu);
}

/* $83:A9BA: load sprite A for actor $A7. */
static void ActorLoadSprite(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    const uint8_t wide = !cpu->accumulator_is_8_bit;

    SimulateJslFrame(memory, cpu, 0x83u, return_address);
    if (wide)                                                  /* A9BA */
        PushAccumulator16(memory, cpu);
    else
        PushAccumulator8(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 1);
    Push8(memory, cpu, 0x83u);
    PullDataBank(memory, cpu);
    ActorSpriteDescriptor(memory, cpu, 0xa9c6u);
    ActorAllocSprite(memory, cpu, 0xa9c9u);
    UnpackStatus(cpu, Pull8(memory, cpu));                     /* A9CA */
    PullDataBank(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    if (wide)
        PullAccumulator16(memory, cpu);
    else
        LoadA8(cpu, Pull8(memory, cpu));
    SimulateRtlFrame(memory, cpu);
}

/* $83:AA30: sprite height offset, -16 for odd types. */
static void ActorSpriteOffset(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x83u, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* AA30 */
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    LoadXDirect(memory, cpu, 0xa7u);
    TransferDirectToA(cpu);
    LoadA8(cpu, 0xf0u);
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
    BitImmediate8(cpu, 0x01u);
    if (cpu->zero)
        TransferDirectToA(cpu);                                /* AA43 */
    ExchangeAccumulatorBytes(cpu);
    Lufia2SignExtendA8(memory, cpu, 0xaa47u);
    LoadXDirect(memory, cpu, 0xa9u);
    Write16Long(memory, LongIndexedAddress(0x7fdd1cu, cpu->x), cpu->accumulator);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtlFrame(memory, cpu);
}

/* $83:DAE9: reload the actor's sprite, keep its frame. */
void Lufia2ActorSpriteReload(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, 0xabu);                           /* DAE9 */
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fe506u, cpu->x)));
    PushAccumulator16(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe508u, cpu->x)));
    PushAccumulator8(memory, cpu);
    LoadXDirect(memory, cpu, 0xa7u);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe3c6u, cpu->x)));
    PushAccumulator8(memory, cpu);
    ActorReleaseSprite(memory, cpu, 0xdb03u);
    LoadXDirect(memory, cpu, 0xa7u);                           /* DB04 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe5a6u, cpu->x)));
    StoreAAbsolute8(memory, cpu, 0x05d2u, cpu->x);
    ActorLoadSprite(memory, cpu, 0xdb10u);
    ActorSpriteOffset(memory, cpu, 0xdb14u);
    LoadA8(cpu, Pull8(memory, cpu));                           /* DB15 */
    LoadXDirect(memory, cpu, 0xa7u);
    Write8(memory, LongIndexedAddress(0x7fe3c6u, cpu->x), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x0622u, cpu->x);
    And8(cpu, 0xfbu);
    StoreAAbsolute8(memory, cpu, 0x0622u, cpu->x);
    LoadA8(cpu, Pull8(memory, cpu));
    LoadXDirect(memory, cpu, 0xabu);
    Write8(memory, LongIndexedAddress(0x7fe508u, cpu->x), A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    PullAccumulator16(memory, cpu);
    Write16Long(memory, LongIndexedAddress(0x7fe506u, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
}

/* $83:AB7C: claim A free sprite slots in $7E:E100. */
void Lufia2SpriteAllocSlots(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x83u, return_address);
    PushDataBank(memory, cpu);                                 /* AB7C */
    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
    LoadA8(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    LoadX8(cpu, 0x00u);
    LoadY8(cpu, 0x00u);
    Write8(memory, DirectAddress(cpu, 0x55u), 0x00u);
    Write8(memory, DirectAddress(cpu, 0x56u), 0x00u);
    for (;;) {
        LoadAAbsolute8(memory, cpu, 0xe100u, cpu->x);          /* AB8A */
        if (cpu->zero) {
            /* Y keeps counting across used slots. */
            LoadY8(cpu, (uint8_t)(cpu->y + 1u));
            Compare8(cpu, (uint8_t)cpu->y,
                Read8(memory, DirectAddress(cpu, 0x54u)));
            if (cpu->zero)
                break;
            LoadX8(cpu, (uint8_t)(cpu->x + 1u));
            continue;
        }
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x56u))); /* AB97 */
        cpu->carry = 0;
        Adc8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
        Write8(memory, DirectAddress(cpu, 0x56u), A8(cpu));
        Write8(memory, DirectAddress(cpu, 0x55u), A8(cpu));
        TransferAToX(cpu);
        Compare8(cpu, A8(cpu), 0x80u);
        if (cpu->zero) {
            PullDataBank(memory, cpu);                         /* ABA5 */
            cpu->carry = 1;
            SimulateRtlFrame(memory, cpu);
            return;
        }
    }
    LoadX8(cpu, Read8(memory, DirectAddress(cpu, 0x55u)));     /* ABA8 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0xa7u)));
    Or8(cpu, 0x80u);
    do {
        StoreAAbsolute8(memory, cpu, 0xe100u, cpu->x);
        LoadX8(cpu, (uint8_t)(cpu->x + 1u));
        DecrementDirect8(memory, cpu, 0x54u);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);                                 /* ABB6 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x55u)));
    AslA8(cpu);
    Write8(memory, DirectAddress(cpu, 0x55u), A8(cpu));
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x55u)));
    And8(cpu, 0xf0u);
    TrbDirect8(memory, cpu, 0x55u);
    AslA8(cpu);
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, 0x00u);
    {
        const uint8_t carry = cpu->carry;                      /* ROL */

        cpu->carry = 0;
        LoadA8(cpu, (uint8_t)((A8(cpu) << 1) | carry));
    }
    ExchangeAccumulatorBytes(cpu);
    Adc8(cpu, Read8(memory, DirectAddress(cpu, 0x55u)));
    cpu->carry = 0;
    SimulateRtlFrame(memory, cpu);
}

/* $83:ABCC: clear A sprite slots from $54/$55. */
void Lufia2SpriteFreeSlots(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x83u, return_address);
    ExchangeAccumulatorBytes(cpu);                             /* ABCC */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x55u)));
    LsrA8(cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    {
        const uint8_t carry = cpu->carry;                      /* ROR */

        cpu->carry = (A8(cpu) & 0x01u) != 0;
        LoadA8(cpu, (uint8_t)((A8(cpu) >> 1) | (carry ? 0x80u : 0u)));
    }
    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
    And8(cpu, 0xf0u);
    TrbDirect8(memory, cpu, 0x54u);
    LsrA8(cpu);
    Adc8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    TransferAToX(cpu);
    ExchangeAccumulatorBytes(cpu);
    TransferAToY(cpu);
    TransferDirectToA(cpu);
    do {
        Write8(memory, LongIndexedAddress(0x7ee100u, cpu->x), A8(cpu));
        LoadX8(cpu, (uint8_t)(cpu->x + 1u));
        LoadY8(cpu, (uint8_t)(cpu->y - 1u));
    } while (!cpu->zero);
    SimulateRtlFrame(memory, cpu);
}
