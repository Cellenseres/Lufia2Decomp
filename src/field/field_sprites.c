/* Field actor OAM ($83:A21A). */

#include "core/cpu_internal.h"
#include "lufia2/field.h"
#include "actor/actor_internal.h"
#include "field/field_internal.h"

/* $83:A669: set the size bit for OAM entry $58, then $58++. */
static void FieldOamHighBit(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    TransferDirectToA(cpu);                                    /* A669 */
    LoadA8(cpu, DirectByte(memory, cpu, 0x58u));
    And8(cpu, 0x03u);
    TransferAToX(cpu);
    LoadAAbsolute8(memory, cpu, 0xac10u, cpu->x);
    StoreADirect8(memory, cpu, 0x55u);
    LoadA8(cpu, DirectByte(memory, cpu, 0x58u));
    IncrementDirect8(memory, cpu, 0x58u);
    LsrA8(cpu);
    LsrA8(cpu);
    And8(cpu, 0x1fu);
    TransferAToX(cpu);
    LoadAAbsolute8(memory, cpu, 0x0300u, cpu->x);
    Or8(cpu, DirectByte(memory, cpu, 0x55u));
    StoreAAbsolute8(memory, cpu, 0x0300u, cpu->x);
    SimulateRtsFrame(memory, cpu);
}

/* $83:A591-$83:A633: four 16x16 tiles; bit 1 flips Y, bit 0 X. */
static void FieldOamQuad(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t layout) {
    static const uint8_t tiles[2][4] = {
        {0x02u, 0x06u, 0x0au, 0x0eu}, {0x06u, 0x02u, 0x0eu, 0x0au}};
    const uint8_t *order = tiles[layout & 1u];
    unsigned i;

    SimulateJsrFrame(memory, cpu, 0xa555u);
    LoadX16(cpu, cpu->y);                                      /* TYX */
    for (i = 0; i < 4u; ++i) {
        if (i)
            LoadA8(cpu, (uint8_t)(A8(cpu) + 2u));
        StoreAAbsolute8(memory, cpu, order[i], cpu->x);
    }
    LoadA8(cpu, DirectByte(memory, cpu, 0x91u));
    StoreAAbsolute8(memory, cpu, (layout & 2u) ? 0x0009u : 0x0001u, cpu->x);
    StoreAAbsolute8(memory, cpu, (layout & 2u) ? 0x000du : 0x0005u, cpu->x);
    cpu->carry = 0;
    Adc8(cpu, 0x10u);
    StoreAAbsolute8(memory, cpu, (layout & 2u) ? 0x0001u : 0x0009u, cpu->x);
    StoreAAbsolute8(memory, cpu, (layout & 2u) ? 0x0005u : 0x000du, cpu->x);
    LoadA8(cpu, DirectByte(memory, cpu, 0x8fu));
    StoreAAbsolute8(memory, cpu, 0x0000u, cpu->x);
    StoreAAbsolute8(memory, cpu, 0x0008u, cpu->x);
    cpu->carry = 0;
    Adc8(cpu, 0x10u);
    StoreAAbsolute8(memory, cpu, 0x0004u, cpu->x);
    StoreAAbsolute8(memory, cpu, 0x000cu, cpu->x);
    SimulateRtsFrame(memory, cpu);
}

/* $83:A48A handlers: OAM entries by sprite size. */
static void FieldOamEntries(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t size) {
    SimulateJsrFrame(memory, cpu, 0xa463u);
    LoadX16(cpu, cpu->y);                                      /* TYX */
    if (size == 0) {
        LoadA8(cpu, DirectByte(memory, cpu, 0x9du));           /* A4A2 */
        StoreAAbsolute8(memory, cpu, 0x0002u, cpu->x);
        LoadA8(cpu, DirectByte(memory, cpu, 0x97u));
        StoreAAbsolute8(memory, cpu, 0x0003u, cpu->x);
        LoadA8(cpu, DirectByte(memory, cpu, 0x91u));
        StoreAAbsolute8(memory, cpu, 0x0001u, cpu->x);
        LoadA8(cpu, DirectByte(memory, cpu, 0x8fu));
        StoreAAbsolute8(memory, cpu, 0x0000u, cpu->x);
        LoadA8(cpu, DirectByte(memory, cpu, 0x90u));
        if (!cpu->zero)
            FieldOamHighBit(memory, cpu, 0xa4bdu);
    } else if (size == 1) {
        LoadA8(cpu, DirectByte(memory, cpu, 0x9du));           /* A4BF */
        StoreAAbsolute8(memory, cpu, 0x0002u, cpu->x);
        LoadA8(cpu, (uint8_t)(A8(cpu) + 2u));
        StoreAAbsolute8(memory, cpu, 0x0006u, cpu->x);
        LoadA8(cpu, DirectByte(memory, cpu, 0x97u));
        StoreAAbsolute8(memory, cpu, 0x0003u, cpu->x);
        StoreAAbsolute8(memory, cpu, 0x0007u, cpu->x);
        BitImmediate8(cpu, 0x80u);
        {
            const uint16_t top = cpu->zero ? 0x0001u : 0x0005u;

            LoadA8(cpu, DirectByte(memory, cpu, 0x91u));
            StoreAAbsolute8(memory, cpu, top, cpu->x);
            cpu->carry = 0;
            Adc8(cpu, 0x10u);
            StoreAAbsolute8(memory, cpu, top ^ 0x0004u, cpu->x);
        }
        LoadA8(cpu, DirectByte(memory, cpu, 0x8fu));           /* A4EE */
        StoreAAbsolute8(memory, cpu, 0x0000u, cpu->x);
        StoreAAbsolute8(memory, cpu, 0x0004u, cpu->x);
        LoadA8(cpu, DirectByte(memory, cpu, 0x90u));
        if (!cpu->zero) {
            FieldOamHighBit(memory, cpu, 0xa4fcu);
            FieldOamHighBit(memory, cpu, 0xa4ffu);
        }
    } else if (size == 2) {
        LoadA8(cpu, DirectByte(memory, cpu, 0x9du));           /* A501 */
        StoreAAbsolute8(memory, cpu, 0x0002u, cpu->x);
        LoadA8(cpu, (uint8_t)(A8(cpu) + 2u));
        StoreAAbsolute8(memory, cpu, 0x0006u, cpu->x);
        LoadA8(cpu, DirectByte(memory, cpu, 0x91u));
        StoreAAbsolute8(memory, cpu, 0x0001u, cpu->x);
        StoreAAbsolute8(memory, cpu, 0x0005u, cpu->x);
        LoadA8(cpu, DirectByte(memory, cpu, 0x97u));
        StoreAAbsolute8(memory, cpu, 0x0003u, cpu->x);
        StoreAAbsolute8(memory, cpu, 0x0007u, cpu->x);
        LoadA8(cpu, DirectByte(memory, cpu, 0x8fu));
        StoreAAbsolute8(memory, cpu, 0x0000u, cpu->x);
        cpu->carry = 0;
        Adc8(cpu, 0x10u);
        StoreAAbsolute8(memory, cpu, 0x0004u, cpu->x);
        if (!cpu->carry)
            LoadA8(cpu, DirectByte(memory, cpu, 0x90u));
        if (cpu->carry || !cpu->zero) {
            FieldOamHighBit(memory, cpu, 0xa52fu);             /* A52D */
            FieldOamHighBit(memory, cpu, 0xa532u);
        }
    } else {
        TransferDirectToA(cpu);                                /* A534 */
        LoadA8(cpu, DirectByte(memory, cpu, 0x97u));
        StoreAAbsolute8(memory, cpu, 0x0003u, cpu->x);
        StoreAAbsolute8(memory, cpu, 0x0007u, cpu->x);
        StoreAAbsolute8(memory, cpu, 0x000bu, cpu->x);
        StoreAAbsolute8(memory, cpu, 0x000fu, cpu->x);
        SetAccumulatorWidth(cpu, 0);
        AslA16(cpu);
        AslA16(cpu);
        AslA16(cpu);
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, 0x00u);
        ExchangeAccumulatorBytes(cpu);
        And8(cpu, 0x06u);
        TransferAToX(cpu);
        LoadA8(cpu, DirectByte(memory, cpu, 0x9du));
        FieldOamQuad(memory, cpu, (uint8_t)(cpu->x >> 1));
        LoadA8(cpu, DirectByte(memory, cpu, 0x90u));           /* A556 */
        if (!cpu->negative) {
            LoadA8(cpu, DirectByte(memory, cpu, 0x8fu));
            cpu->carry = 0;
            Adc8(cpu, 0x10u);
            if (cpu->carry) {
                IncrementDirect8(memory, cpu, 0x58u);          /* A561 */
                FieldOamHighBit(memory, cpu, 0xa565u);
                IncrementDirect8(memory, cpu, 0x58u);
                FieldOamHighBit(memory, cpu, 0xa56au);
            }
        } else {
            LoadA8(cpu, DirectByte(memory, cpu, 0x8fu));       /* A56C */
            cpu->carry = 0;
            Adc8(cpu, 0x10u);
            if (!cpu->carry) {
                FieldOamHighBit(memory, cpu, 0xa575u);
                FieldOamHighBit(memory, cpu, 0xa578u);
                FieldOamHighBit(memory, cpu, 0xa57bu);
                FieldOamHighBit(memory, cpu, 0xa57eu);
            } else {
                FieldOamHighBit(memory, cpu, 0xa582u);         /* A580 */
                IncrementDirect8(memory, cpu, 0x58u);
                FieldOamHighBit(memory, cpu, 0xa587u);
            }
        }
    }
    SimulateRtsFrame(memory, cpu);
}

/* $83:FCD1: queue a sprite upload from the object tables. */
static void FieldObjectSpriteUpload(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0xa4a0u);
    LoadY8(cpu, AbsoluteByte(memory, cpu, 0x0732u, 0));        /* FCD1 */
    TransferDirectToA(cpu);
    LoadXDirect8(memory, cpu, 0xa7u);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
    AslA8(cpu);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x83abfcu, cpu->x)));
    StoreAAbsolute16(memory, cpu, 0x11f9u, cpu->y);
    LoadXDirect8(memory, cpu, 0xa9u);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fe05eu, cpu->x)));
    StoreAAbsolute16(memory, cpu, 0x11e9u, cpu->y);
    SetAccumulatorWidth(cpu, 1);
    LoadXDirect8(memory, cpu, 0xabu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd896u, cpu->x)));
    StoreAAbsolute8(memory, cpu, 0x11d9u, cpu->y);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fd894u, cpu->x)));
    StoreAAbsolute16(memory, cpu, 0x05c2u, cpu->y);
    SetAccumulatorWidth(cpu, 1);
    LoadY8(cpu, (uint8_t)(cpu->y + 1u));
    LoadY8(cpu, (uint8_t)(cpu->y + 1u));
    Write8(memory, AbsoluteIndexedAddress(cpu, 0x0732u, 0), (uint8_t)cpu->y);
    SimulateRtsFrame(memory, cpu);
}

/* $83:A492: pending reload flag $20 in $0622,x. */
static void FieldObjectReload(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadAAbsolute8(memory, cpu, 0x0622u, cpu->x);              /* A492 */
    BitImmediate8(cpu, 0x20u);
    if (!cpu->zero) {
        And8(cpu, 0xdfu);
        StoreAAbsolute8(memory, cpu, 0x0622u, cpu->x);
        FieldObjectSpriteUpload(memory, cpu);
    }
    SimulateRtsFrame(memory, cpu);
}

/* $83:AAE5: queue the actor's new animation frame upload. */
static void FieldActorFrameUpload(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    uint32_t pointer;

    SimulateJsrFrame(memory, cpu, 0xa3d2u);
    SetAccumulatorWidth(cpu, 1);                               /* AAE5 */
    SetIndexWidth(cpu, 1);
    LoadXDirect8(memory, cpu, 0xa7u);
    TransferDirectToA(cpu);
    LoadAAbsolute8(memory, cpu, 0x066au, cpu->x);
    cpu->carry = 0;
    Adc8(cpu, Read8(memory, LongIndexedAddress(0x7fe2eeu, cpu->x)));
    SetAccumulatorWidth(cpu, 0);
    LoadYDirect8(memory, cpu, 0xa9u);
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x13d1u, cpu->y));
    StoreADirect16(memory, cpu, 0x54u);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x83u);
    StoreADirect8(memory, cpu, 0x56u);
    LoadXDirect8(memory, cpu, 0xa7u);
    pointer = Read16Direct(memory, cpu, 0x54u) |
        ((uint32_t)DirectByte(memory, cpu, 0x56u) << 16);
    LoadA8(cpu, Read8(memory, pointer));                       /* LDA [$54] */
    And8(cpu, 0x7fu);
    StoreAAbsolute8(memory, cpu, 0x4202u, 0);
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x83abf8u, cpu->x)));
    StoreAAbsolute8(memory, cpu, 0x4203u, 0);
    LoadYDirect8(memory, cpu, 0xabu);
    LoadAAbsolute8(memory, cpu, 0x12bbu, cpu->y);
    StoreADirect8(memory, cpu, 0x54u);
    LoadAAbsolute8(memory, cpu, 0x4216u, 0);
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, 0x00u);
    SetAccumulatorWidth(cpu, 0);
    LsrA16(cpu);
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x12b9u, cpu->y));
    StoreADirect16(memory, cpu, 0x56u);
    LoadY8(cpu, AbsoluteByte(memory, cpu, 0x0732u, 0));
    LoadXDirect8(memory, cpu, 0xa9u);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1381u, cpu->x));
    StoreAAbsolute16(memory, cpu, 0x11f9u, cpu->y);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1331u, cpu->x));
    StoreAAbsolute16(memory, cpu, 0x11e9u, cpu->y);
    LoadADirect16(memory, cpu, 0x54u);
    StoreAAbsolute16(memory, cpu, 0x11d9u, cpu->y);
    LoadADirect16(memory, cpu, 0x56u);
    StoreAAbsolute16(memory, cpu, 0x05c2u, cpu->y);
    LoadY8(cpu, (uint8_t)(cpu->y + 1u));
    LoadY8(cpu, (uint8_t)(cpu->y + 1u));
    Write8(memory, AbsoluteIndexedAddress(cpu, 0x0732u, 0), (uint8_t)cpu->y);
    SimulateRtsFrame(memory, cpu);
}

/* $83:A29B: sort visible actors by Y into $E200/$E300. */
static void FieldSortVisible(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    TransferDirectToA(cpu);                                    /* A295 */
    TransferAToY(cpu);
    LoadX8(cpu, 0x00u);
    SetAccumulatorWidth(cpu, 1);
    do {
        LoadAAbsolute8(memory, cpu, 0x0622u, cpu->x);          /* A29B */
        BitImmediate8(cpu, 0x04u);
        if (cpu->zero) {
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe316u, cpu->x)));
            BitImmediate8(cpu, 0x80u);
        }
        if (cpu->zero) {
            uint8_t visible = 0;

            SetAccumulatorWidth(cpu, 0);
            TransferXToA(cpu);
            StoreADirect16(memory, cpu, 0xa7u);
            AslA16(cpu);
            TransferAToX(cpu);
            LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fddaeu, cpu->x)));
            cpu->carry = 0;
            Add16Value(cpu, 0x0030u);
            Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, 0x54u));
            if (!cpu->negative) {
                Compare16(cpu, cpu->accumulator,
                    Read16Direct(memory, cpu, 0x56u));
                if (cpu->negative) {
                    LoadA16(cpu, Read16Long(
                        memory, LongIndexedAddress(0x7fde3eu, cpu->x)));
                    cpu->carry = 0;
                    Add16Value(cpu, 0x0020u);
                    Compare16(cpu, cpu->accumulator,
                        Read16Direct(memory, cpu, 0x58u));
                    if (!cpu->negative) {
                        Compare16(cpu, cpu->accumulator,
                            Read16Direct(memory, cpu, 0x5au));
                        visible = cpu->negative;
                    }
                }
            }
            if (visible) {
                LoadA16(cpu, (uint16_t)(cpu->accumulator | 0x1000u));
                StoreADirect16(memory, cpu, 0x63u);
                LoadXDirect8(memory, cpu, 0xa7u);
                LoadA16(cpu, Read16Long(
                    memory, LongIndexedAddress(0x7fe316u, cpu->x)));
                cpu->zero = (cpu->accumulator & 0x0001u) == 0;
                if (!cpu->zero) {
                    LoadA16(cpu, 0x1000u);
                    TestBitsDirect(memory, cpu, 0x63u, 0);
                } else {
                    cpu->zero = (cpu->accumulator & 0x0002u) == 0;
                    if (!cpu->zero) {
                        LoadA16(cpu, 0x2000u);
                        TestBitsDirect(memory, cpu, 0x63u, 1);
                    }
                }
                LoadX8(cpu, (uint8_t)cpu->y);                  /* A2F2 */
                while (!cpu->zero) {
                    LoadA16(cpu, Read16AbsoluteIndexed(
                        memory, cpu, 0xe1feu, cpu->x));
                    Compare16(cpu, cpu->accumulator,
                        Read16Direct(memory, cpu, 0x63u));
                    if (cpu->carry)
                        break;
                    StoreAAbsolute16(memory, cpu, 0xe200u, cpu->x);
                    LoadA16(cpu, Read16AbsoluteIndexed(
                        memory, cpu, 0xe2feu, cpu->x));
                    StoreAAbsolute16(memory, cpu, 0xe300u, cpu->x);
                    LoadX8(cpu, (uint8_t)(cpu->x - 1u));
                    LoadX8(cpu, (uint8_t)(cpu->x - 1u));
                }
                LoadADirect16(memory, cpu, 0x63u);             /* A309 */
                StoreAAbsolute16(memory, cpu, 0xe200u, cpu->x);
                LoadADirect16(memory, cpu, 0xa7u);
                StoreAAbsolute16(memory, cpu, 0xe300u, cpu->x);
                LoadY8(cpu, (uint8_t)(cpu->y + 1u));
                LoadY8(cpu, (uint8_t)(cpu->y + 1u));
            }
            SetAccumulatorWidth(cpu, 1);                       /* A315 */
            LoadXDirect8(memory, cpu, 0xa7u);
        }
        LoadX8(cpu, (uint8_t)(cpu->x + 1u));                   /* A319 */
        Compare8(cpu, (uint8_t)cpu->x, 0x48u);
    } while (!cpu->zero);
}

/* $83:A33A: OAM entries for one sorted actor. */
static uint8_t FieldActorOam(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7ee300u, cpu->x)));
    StoreADirect8(memory, cpu, 0xa7u);
    TransferAToX(cpu);
    AslA8(cpu);
    StoreADirect8(memory, cpu, 0xa9u);
    cpu->carry = 0;
    Adc8(cpu, DirectByte(memory, cpu, 0xa7u));
    StoreADirect8(memory, cpu, 0xabu);
    Write8(memory, DirectAddress(cpu, 0x97u), 0x00u);
    Compare8(cpu, (uint8_t)cpu->x, 0x28u);
    if (cpu->carry) {
        LoadAAbsolute8(memory, cpu, 0x0732u, 0);               /* A34F */
        Compare8(cpu, A8(cpu), 0x10u);
        if (!cpu->carry) {
            TransferDirectToA(cpu);
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe37eu, cpu->x)));
            Compare8(cpu, A8(cpu), 0xffu);
            if (!cpu->zero) {
                PushIndex(memory, cpu);
                cpu->carry = 0;
                Adc8(cpu, 0x28u);
                StoreADirect8(memory, cpu, 0xa7u);
                SimulateJslFrame(memory, cpu, 0x83u, 0xa368u);
                Lufia2ActorRecordOffsets(memory, cpu);
                SimulateRtlFrame(memory, cpu);
                LoadXDirect8(memory, cpu, 0xa7u);
                FieldObjectReload(memory, cpu, 0xa36du);
                cpu->x = PullIndexValue(memory, cpu);
                Write8(memory, DirectAddress(cpu, 0xa7u), (uint8_t)cpu->x);
                SimulateJslFrame(memory, cpu, 0x83u, 0xa374u);
                Lufia2ActorRecordOffsets(memory, cpu);
                SimulateRtlFrame(memory, cpu);
            } else {
                FieldObjectReload(memory, cpu, 0xa379u);       /* A377 */
            }
        }
        SetAccumulatorWidth(cpu, 0);                           /* A37A */
        LoadXDirect8(memory, cpu, 0xa9u);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fddaeu, cpu->x)));
        cpu->carry = 0;
        Add16Value(cpu, Read16Long(memory, LongIndexedAddress(0x7fdc8cu, cpu->x)));
        Subtract16(cpu, Read16Direct(memory, cpu, 0x9fu));
        Subtract16(cpu, 0x0008u);
        StoreADirect16(memory, cpu, 0x8fu);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fde3eu, cpu->x)));
        cpu->carry = 0;
        Add16Value(cpu, Read16Long(memory, LongIndexedAddress(0x7fdd1cu, cpu->x)));
        cpu->carry = 0;
        Add16Value(cpu, (uint16_t)~Read16Direct(memory, cpu, 0xa1u));
        StoreADirect16(memory, cpu, 0x91u);
        SetAccumulatorWidth(cpu, 1);
        LoadXDirect8(memory, cpu, 0xa7u);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe1ceu, cpu->x)));
        AslA8(cpu);
        TestBitsDirect(memory, cpu, 0x97u, 1);
        TransferDirectToA(cpu);
        LoadAAbsolute8(memory, cpu, 0x0622u, cpu->x);
        And8(cpu, 0x03u);
        LsrA8(cpu);
        ExchangeAccumulatorBytes(cpu);
        RorA8(cpu);
        ExchangeAccumulatorBytes(cpu);
        LsrA8(cpu);
        ExchangeAccumulatorBytes(cpu);
        RorA8(cpu);
        TestBitsDirect(memory, cpu, 0x97u, 1);
    } else {
        LoadAAbsolute8(memory, cpu, 0x066au, cpu->x);          /* A3BB */
        Compare8(cpu, A8(cpu), AbsoluteByte(memory, cpu, 0x1471u, cpu->x));
        if (!cpu->zero) {
            LoadAAbsolute8(memory, cpu, 0x0732u, 0);
            Compare8(cpu, A8(cpu), 0x0au);
            if (!cpu->carry) {
                LoadAAbsolute8(memory, cpu, 0x066au, cpu->x);
                StoreAAbsolute8(memory, cpu, 0x1471u, cpu->x);
                FieldActorFrameUpload(memory, cpu);
            }
        }
        SetAccumulatorWidth(cpu, 0);                           /* A3D3 */
        LoadXDirect8(memory, cpu, 0xa9u);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fddaeu, cpu->x)));
        cpu->carry = 0;
        Add16Value(cpu, Read16Long(memory, LongIndexedAddress(0x7fdc8cu, cpu->x)));
        Subtract16(cpu, Read16Direct(memory, cpu, 0x9fu));
        Subtract16(cpu, 0x0008u);
        Subtract16(cpu, Read16Long(memory, 0x7fd081u));
        StoreADirect16(memory, cpu, 0x8fu);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fde3eu, cpu->x)));
        cpu->carry = 0;
        Add16Value(cpu, Read16Long(memory, LongIndexedAddress(0x7fdd1cu, cpu->x)));
        Subtract16(cpu, Read16Direct(memory, cpu, 0xa1u));
        cpu->carry = 0;
        Add16Value(cpu, (uint16_t)~Read16Long(memory, 0x7fd083u));
        StoreADirect16(memory, cpu, 0x91u);
        SetAccumulatorWidth(cpu, 1);
        LoadXDirect8(memory, cpu, 0xa7u);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe1ceu, cpu->x)));
        And8(cpu, 0x0eu);
        TestBitsDirect(memory, cpu, 0x97u, 1);
        LoadAAbsolute8(memory, cpu, 0x1291u, cpu->x);
        And8(cpu, 0x18u);
        if (!cpu->zero) {
            LoadAAbsolute8(memory, cpu, 0x066au, cpu->x);
            And8(cpu, 0x07u);
            Compare8(cpu, A8(cpu), 0x06u);
            if (cpu->carry) {
                LoadA8(cpu, 0x40u);
                TestBitsDirect(memory, cpu, 0x97u, 1);
            }
        }
    }
    LoadXDirect8(memory, cpu, 0xa7u);                          /* A421 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe316u, cpu->x)));
    And8(cpu, 0x30u);
    TestBitsDirect(memory, cpu, 0x97u, 1);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe2a6u, cpu->x)));
    TestBitsDirect(memory, cpu, 0x97u, 1);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe25eu, cpu->x)));
    StoreADirect8(memory, cpu, 0x9du);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
    TransferAToY(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x5fu));
    StoreADirect8(memory, cpu, 0x94u);
    cpu->carry = 0;
    Adc8(cpu, AbsoluteByte(memory, cpu, 0xabf4u, cpu->y));
    StoreADirect8(memory, cpu, 0x5fu);
    TransferDirectToA(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x94u));
    StoreADirect8(memory, cpu, 0x58u);
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    AslA16(cpu);
    AslA16(cpu);
    Add16Value(cpu, 0x0100u);
    TransferAToY(cpu);
    SetAccumulatorWidth(cpu, 1);
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
    Compare8(cpu, A8(cpu), 0x04u);
    if (cpu->carry)
        TransferDirectToA(cpu);
    AslA8(cpu);
    TransferAToX(cpu);
    if (cpu->x > 6u) {
        /* D high byte set: LLE takes the JSR. */
        cpu->resume_pc = 0x83a461u;
        return 0;
    }
    FieldOamEntries(memory, cpu, (uint8_t)(cpu->x >> 1));
    return 1;
}

/* $83:A21A: field OAM from the visible, Y-sorted actors. */
Lufia2ExecutionResult Lufia2FieldActorSprites(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;

    result.flow = LUFIA2_EXECUTION_RETURNED;
    result.pc = 0x83a489u;
    result.dispatches = 0;
    /* X is set before any index use; M=1 only. */
    if (!cpu->accumulator_is_8_bit) {
        result.flow = LUFIA2_EXECUTION_BOUNDARY;
        result.pc = cpu->resume_pc = 0x83a21au;
        return result;
    }
    PushAndSetDataBank(memory, cpu, 0x7eu);                     /* A21A */
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, 0x7fd0b0u));
    Compare8(cpu, A8(cpu), 0xffu);
    if (!cpu->zero) {
        unsigned i;

        SetAccumulatorWidth(cpu, 0);                           /* A228 */
        SetIndexWidth(cpu, 0);
        AslA16(cpu);
        AslA16(cpu);
        TransferAToX(cpu);
        LoadA16(cpu, 0xf000u);
        do {
            StoreAAbsolute16(memory, cpu, 0x0140u, cpu->x);
            LoadX16(cpu, (uint16_t)(cpu->x - 4u));
        } while (!cpu->negative);
        SetIndexWidth(cpu, 1);
        for (i = 0; i < 8u; ++i)
            Write16Long(memory, AbsoluteIndexedAddress(
                cpu, (uint16_t)(0x0302u + 2u * i), 0), 0x0000u);
    }
    SetAccumulatorWidth(cpu, 0);                               /* A253 */
    SetIndexWidth(cpu, 1);
    LoadA16(cpu, 0x0040u);
    BitAbsolute16(memory, cpu, 0x1261u);
    if (!cpu->zero) {
        LoadA16(cpu, Read16Long(memory, 0x7fd0eeu));
        StoreADirect16(memory, cpu, 0x9fu);
        LoadA16(cpu, Read16Long(memory, 0x7fd0f0u));
        StoreADirect16(memory, cpu, 0xa1u);
    } else {
        LoadA16(cpu, Read16Long(memory, 0x001220u));
        StoreADirect16(memory, cpu, 0x9fu);
        LoadA16(cpu, Read16Long(memory, 0x001228u));
        StoreADirect16(memory, cpu, 0xa1u);
    }
    LoadADirect16(memory, cpu, 0x9fu);                         /* A279 */
    cpu->carry = 0;
    Add16Value(cpu, 0x0020u);
    StoreADirect16(memory, cpu, 0x54u);
    cpu->carry = 0;
    Add16Value(cpu, 0x0110u);
    StoreADirect16(memory, cpu, 0x56u);
    LoadADirect16(memory, cpu, 0xa1u);
    cpu->carry = 0;
    Add16Value(cpu, 0x0010u);
    StoreADirect16(memory, cpu, 0x58u);
    cpu->carry = 0;
    Add16Value(cpu, 0x0100u);
    StoreADirect16(memory, cpu, 0x5au);
    FieldSortVisible(memory, cpu);
    Write8(memory, DirectAddress(cpu, 0x60u), (uint8_t)cpu->y); /* A321 */
    Push8(memory, cpu, 0x83u);                                 /* PHK */
    PullDataBank(memory, cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x60u));
    if (!cpu->zero) {
        Write8(memory, DirectAddress(cpu, 0xaau), 0x00u);      /* A32E */
        Write8(memory, DirectAddress(cpu, 0xacu), 0x00u);
        LoadA8(cpu, 0x10u);
        StoreADirect8(memory, cpu, 0x5fu);
        LoadX8(cpu, 0x00u);
        Write8(memory, DirectAddress(cpu, 0x5du), 0x00u);
        do {
            if (!FieldActorOam(memory, cpu)) {
                result.flow = LUFIA2_EXECUTION_BOUNDARY;
                result.pc = cpu->resume_pc;
                return result;
            }
            ++result.dispatches;
            SetAccumulatorWidth(cpu, 1);                       /* A464 */
            SetIndexWidth(cpu, 1);
            LoadXDirect8(memory, cpu, 0x5du);
            LoadX8(cpu, (uint8_t)(cpu->x + 2u));
            Write8(memory, DirectAddress(cpu, 0x5du), (uint8_t)cpu->x);
            Compare8(cpu, (uint8_t)cpu->x, DirectByte(memory, cpu, 0x60u));
        } while (!cpu->zero);
    }
    SetAccumulatorWidth(cpu, 1);                               /* A473 */
    SetIndexWidth(cpu, 1);
    LoadA8(cpu, DirectByte(memory, cpu, 0x5fu));
    cpu->carry = 1;
    Sbc8(cpu, 0x11u);
    Write8(memory, 0x7fd0b0u, A8(cpu));
    LoadA8(cpu, DirectByte(memory, cpu, 0x72u));
    BitImmediate8(cpu, 0x01u);
    if (cpu->zero) {
        LoadA8(cpu, 0x80u);
        StoreADirect8(memory, cpu, 0x72u);
    }
    PullDataBank(memory, cpu);                                 /* A488 */
    return result;
}
