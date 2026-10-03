/* Field actor OAM ($83:A21A). */

#include "actor/actor_internal.h"
#include "core/cpu_internal.h"
#include "field/field_internal.h"
#include "lufia2/field.h"
#include "system/dp_scratch.h"
#include "system/wram.h"

/* DP fields of the actor sprite builder. */
enum {
    OAM_DP_SIZE_MASK = 0x55u,
    OAM_DP_INDEX = 0x58u,
    OAM_DP_NEXT_ENTRY = 0x5fu,
    OAM_DP_SIZE_FLAGS = 0x90u,
    OAM_DP_ENTRY = 0x94u,
    OAM_DP_ATTRIBUTES = 0x97u,
    OAM_DP_TILE = 0x9du,
    SPRITE_DP_CAMERA_X = 0x9fu,
    SPRITE_DP_CAMERA_Y = 0xa1u,
    SPRITE_DP_WINDOW_LEFT = 0x54u,
    SPRITE_DP_WINDOW_RIGHT = 0x56u,
    SPRITE_DP_WINDOW_TOP = 0x58u,
    SPRITE_DP_WINDOW_BOTTOM = 0x5au,
    SPRITE_DP_SORT_KEY = 0x63u,
    SPRITE_DP_LIST_INDEX = 0x5du,
    SPRITE_DP_ACTOR_COUNT = 0x60u,
};

/* OAM entry field offsets from the entry in X. */
/* OAM high table: size and high-X bits, four per byte. */
#define OAM_HIGH_TABLE 0x0300u
#define OAM_ENTRY_SIZE 4u
#define OAM_X(n) ((n) * OAM_ENTRY_SIZE + 0u)
#define OAM_Y(n) ((n) * OAM_ENTRY_SIZE + 1u)
#define OAM_TILE(n) ((n) * OAM_ENTRY_SIZE + 2u)
#define OAM_ATTRIBUTES(n) ((n) * OAM_ENTRY_SIZE + 3u)

/* $83:A669: set the size bit for OAM entry $58, then $58++. */
static void FieldOamHighBit(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    TransferDirectToA(cpu);                                    /* A669 */
    LoadA8(cpu, DirectByte(memory, cpu, OAM_DP_INDEX));
    And8(cpu, 0x03u);
    TransferAToX(cpu);
    LoadAAbsolute8(memory, cpu, 0xac10u, cpu->x);
    StoreADirect8(memory, cpu, OAM_DP_SIZE_MASK);
    LoadA8(cpu, DirectByte(memory, cpu, OAM_DP_INDEX));
    IncrementDirect8(memory, cpu, OAM_DP_INDEX);
    LsrA8(cpu);
    LsrA8(cpu);
    And8(cpu, 0x1fu);
    TransferAToX(cpu);
    LoadAAbsolute8(memory, cpu, OAM_HIGH_TABLE, cpu->x);
    Or8(cpu, DirectByte(memory, cpu, OAM_DP_SIZE_MASK));
    StoreAAbsolute8(memory, cpu, OAM_HIGH_TABLE, cpu->x);
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
    LoadA8(cpu, DirectByte(memory, cpu, DP_PROBE_Y));
    StoreAAbsolute8(memory, cpu, (layout & 2u) ? OAM_Y(2) : OAM_Y(0), cpu->x);
    StoreAAbsolute8(memory, cpu, (layout & 2u) ? OAM_Y(3) : OAM_Y(1), cpu->x);
    cpu->carry = 0;
    Adc8(cpu, 0x10u);
    StoreAAbsolute8(memory, cpu, (layout & 2u) ? OAM_Y(0) : OAM_Y(2), cpu->x);
    StoreAAbsolute8(memory, cpu, (layout & 2u) ? OAM_Y(1) : OAM_Y(3), cpu->x);
    LoadA8(cpu, DirectByte(memory, cpu, DP_PROBE_X));
    StoreAAbsolute8(memory, cpu, OAM_X(0), cpu->x);
    StoreAAbsolute8(memory, cpu, OAM_X(2), cpu->x);
    cpu->carry = 0;
    Adc8(cpu, 0x10u);
    StoreAAbsolute8(memory, cpu, OAM_X(1), cpu->x);
    StoreAAbsolute8(memory, cpu, OAM_X(3), cpu->x);
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
        LoadA8(cpu, DirectByte(memory, cpu, OAM_DP_TILE)); /* A4A2 */
        StoreAAbsolute8(memory, cpu, OAM_TILE(0), cpu->x);
        LoadA8(cpu, DirectByte(memory, cpu, OAM_DP_ATTRIBUTES));
        StoreAAbsolute8(memory, cpu, OAM_ATTRIBUTES(0), cpu->x);
        LoadA8(cpu, DirectByte(memory, cpu, DP_PROBE_Y));
        StoreAAbsolute8(memory, cpu, OAM_Y(0), cpu->x);
        LoadA8(cpu, DirectByte(memory, cpu, DP_PROBE_X));
        StoreAAbsolute8(memory, cpu, OAM_X(0), cpu->x);
        LoadA8(cpu, DirectByte(memory, cpu, OAM_DP_SIZE_FLAGS));
        if (!cpu->zero)
            FieldOamHighBit(memory, cpu, 0xa4bdu);
    } else if (size == 1) {
        LoadA8(cpu, DirectByte(memory, cpu, OAM_DP_TILE)); /* A4BF */
        StoreAAbsolute8(memory, cpu, OAM_TILE(0), cpu->x);
        LoadA8(cpu, (uint8_t)(A8(cpu) + 2u));
        StoreAAbsolute8(memory, cpu, OAM_TILE(1), cpu->x);
        LoadA8(cpu, DirectByte(memory, cpu, OAM_DP_ATTRIBUTES));
        StoreAAbsolute8(memory, cpu, OAM_ATTRIBUTES(0), cpu->x);
        StoreAAbsolute8(memory, cpu, OAM_ATTRIBUTES(1), cpu->x);
        BitImmediate8(cpu, 0x80u);
        {
            const uint16_t top = cpu->zero ? OAM_Y(0) : OAM_Y(1);

            LoadA8(cpu, DirectByte(memory, cpu, DP_PROBE_Y));
            StoreAAbsolute8(memory, cpu, top, cpu->x);
            cpu->carry = 0;
            Adc8(cpu, 0x10u);
            StoreAAbsolute8(memory, cpu, top ^ OAM_X(1), cpu->x);
        }
        LoadA8(cpu, DirectByte(memory, cpu, DP_PROBE_X));           /* A4EE */
        StoreAAbsolute8(memory, cpu, OAM_X(0), cpu->x);
        StoreAAbsolute8(memory, cpu, OAM_X(1), cpu->x);
        LoadA8(cpu, DirectByte(memory, cpu, OAM_DP_SIZE_FLAGS));
        if (!cpu->zero) {
            FieldOamHighBit(memory, cpu, 0xa4fcu);
            FieldOamHighBit(memory, cpu, 0xa4ffu);
        }
    } else if (size == 2) {
        LoadA8(cpu, DirectByte(memory, cpu, OAM_DP_TILE)); /* A501 */
        StoreAAbsolute8(memory, cpu, OAM_TILE(0), cpu->x);
        LoadA8(cpu, (uint8_t)(A8(cpu) + 2u));
        StoreAAbsolute8(memory, cpu, OAM_TILE(1), cpu->x);
        LoadA8(cpu, DirectByte(memory, cpu, DP_PROBE_Y));
        StoreAAbsolute8(memory, cpu, OAM_Y(0), cpu->x);
        StoreAAbsolute8(memory, cpu, OAM_Y(1), cpu->x);
        LoadA8(cpu, DirectByte(memory, cpu, OAM_DP_ATTRIBUTES));
        StoreAAbsolute8(memory, cpu, OAM_ATTRIBUTES(0), cpu->x);
        StoreAAbsolute8(memory, cpu, OAM_ATTRIBUTES(1), cpu->x);
        LoadA8(cpu, DirectByte(memory, cpu, DP_PROBE_X));
        StoreAAbsolute8(memory, cpu, OAM_X(0), cpu->x);
        cpu->carry = 0;
        Adc8(cpu, 0x10u);
        StoreAAbsolute8(memory, cpu, OAM_X(1), cpu->x);
        if (!cpu->carry)
            LoadA8(cpu, DirectByte(memory, cpu, OAM_DP_SIZE_FLAGS));
        if (cpu->carry || !cpu->zero) {
            FieldOamHighBit(memory, cpu, 0xa52fu);             /* A52D */
            FieldOamHighBit(memory, cpu, 0xa532u);
        }
    } else {
        TransferDirectToA(cpu);                                /* A534 */
        LoadA8(cpu, DirectByte(memory, cpu, OAM_DP_ATTRIBUTES));
        StoreAAbsolute8(memory, cpu, OAM_ATTRIBUTES(0), cpu->x);
        StoreAAbsolute8(memory, cpu, OAM_ATTRIBUTES(1), cpu->x);
        StoreAAbsolute8(memory, cpu, OAM_ATTRIBUTES(2), cpu->x);
        StoreAAbsolute8(memory, cpu, OAM_ATTRIBUTES(3), cpu->x);
        SetAccumulatorWidth(cpu, 0);
        AslA16(cpu);
        AslA16(cpu);
        AslA16(cpu);
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, 0x00u);
        ExchangeAccumulatorBytes(cpu);
        And8(cpu, 0x06u);
        TransferAToX(cpu);
        LoadA8(cpu, DirectByte(memory, cpu, OAM_DP_TILE));
        FieldOamQuad(memory, cpu, (uint8_t)(cpu->x >> 1));
        LoadA8(cpu, DirectByte(memory, cpu, OAM_DP_SIZE_FLAGS)); /* A556 */
        if (!cpu->negative) {
            LoadA8(cpu, DirectByte(memory, cpu, DP_PROBE_X));
            cpu->carry = 0;
            Adc8(cpu, 0x10u);
            if (cpu->carry) {
                IncrementDirect8(memory, cpu, OAM_DP_INDEX); /* A561 */
                FieldOamHighBit(memory, cpu, 0xa565u);
                IncrementDirect8(memory, cpu, OAM_DP_INDEX);
                FieldOamHighBit(memory, cpu, 0xa56au);
            }
        } else {
            LoadA8(cpu, DirectByte(memory, cpu, DP_PROBE_X));       /* A56C */
            cpu->carry = 0;
            Adc8(cpu, 0x10u);
            if (!cpu->carry) {
                FieldOamHighBit(memory, cpu, 0xa575u);
                FieldOamHighBit(memory, cpu, 0xa578u);
                FieldOamHighBit(memory, cpu, 0xa57bu);
                FieldOamHighBit(memory, cpu, 0xa57eu);
            } else {
                FieldOamHighBit(memory, cpu, 0xa582u);         /* A580 */
                IncrementDirect8(memory, cpu, OAM_DP_INDEX);
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
    LoadXDirect8(memory, cpu, DP_ACTOR_SLOT);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FE216, cpu->x)));
    AslA8(cpu);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x83abfcu, cpu->x)));
    StoreAAbsolute16(memory, cpu, 0x11f9u, cpu->y);
    LoadXDirect8(memory, cpu, DP_SLOT_WORD_OFFSET);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fe05eu, cpu->x)));
    StoreAAbsolute16(memory, cpu, 0x11e9u, cpu->y);
    SetAccumulatorWidth(cpu, 1);
    LoadXDirect8(memory, cpu, DP_SLOT_RECORD_OFFSET);
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
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->x);     /* A492 */
    BitImmediate8(cpu, 0x20u);
    if (!cpu->zero) {
        And8(cpu, 0xdfu);
        StoreAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->x);
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
    LoadXDirect8(memory, cpu, DP_ACTOR_SLOT);
    TransferDirectToA(cpu);
    LoadAAbsolute8(memory, cpu, WRAM_UNK_7E066A, cpu->x);
    cpu->carry = 0;
    Adc8(cpu, Read8(memory, LongIndexedAddress(0x7fe2eeu, cpu->x)));
    SetAccumulatorWidth(cpu, 0);
    LoadYDirect8(memory, cpu, DP_SLOT_WORD_OFFSET);
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x13d1u, cpu->y));
    StoreADirect16(memory, cpu, DP_SCRATCH_A);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x83u);
    StoreADirect8(memory, cpu, DP_SCRATCH_C);
    LoadXDirect8(memory, cpu, DP_ACTOR_SLOT);
    pointer = Read16Direct(memory, cpu, DP_SCRATCH_A) |
              ((uint32_t)DirectByte(memory, cpu, DP_SCRATCH_C) << 16);
    LoadA8(cpu, Read8(memory, pointer));                       /* LDA [$54] */
    And8(cpu, 0x7fu);
    StoreAAbsolute8(memory, cpu, SNES_WRMPYA, 0);
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FE216, cpu->x)));
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x83abf8u, cpu->x)));
    StoreAAbsolute8(memory, cpu, SNES_WRMPYB, 0);
    LoadYDirect8(memory, cpu, DP_SLOT_RECORD_OFFSET);
    LoadAAbsolute8(memory, cpu, 0x12bbu, cpu->y);
    StoreADirect8(memory, cpu, DP_SCRATCH_A);
    LoadAAbsolute8(memory, cpu, SNES_RDMPYL, 0);
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, 0x00u);
    SetAccumulatorWidth(cpu, 0);
    LsrA16(cpu);
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x12b9u, cpu->y));
    StoreADirect16(memory, cpu, DP_SCRATCH_C);
    LoadY8(cpu, AbsoluteByte(memory, cpu, 0x0732u, 0));
    LoadXDirect8(memory, cpu, DP_SLOT_WORD_OFFSET);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1381u, cpu->x));
    StoreAAbsolute16(memory, cpu, 0x11f9u, cpu->y);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1331u, cpu->x));
    StoreAAbsolute16(memory, cpu, 0x11e9u, cpu->y);
    LoadADirect16(memory, cpu, DP_SCRATCH_A);
    StoreAAbsolute16(memory, cpu, 0x11d9u, cpu->y);
    LoadADirect16(memory, cpu, DP_SCRATCH_C);
    StoreAAbsolute16(memory, cpu, 0x05c2u, cpu->y);
    LoadY8(cpu, (uint8_t)(cpu->y + 1u));
    LoadY8(cpu, (uint8_t)(cpu->y + 1u));
    Write8(memory, AbsoluteIndexedAddress(cpu, 0x0732u, 0), (uint8_t)cpu->y);
    SimulateRtsFrame(memory, cpu);
}

/* $83:A29B: sort visible actors by Y into $E200/$E300. */
static void FieldSortVisible(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    const Lufia2FieldActorVisibility *visibility) {
    TransferDirectToA(cpu);                                    /* A295 */
    TransferAToY(cpu);
    LoadX8(cpu, 0x00u);
    SetAccumulatorWidth(cpu, 1);
    do {
        LoadAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->x); /* A29B */
        BitImmediate8(cpu, 0x04u);
        if (cpu->zero) {
            LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FE316, cpu->x)));
            BitImmediate8(cpu, 0x80u);
        }
        if (cpu->zero) {
            uint8_t visible = 0;
            uint16_t world_x;
            uint16_t world_y = 0;

            SetAccumulatorWidth(cpu, 0);
            TransferXToA(cpu);
            StoreADirect16(memory, cpu, DP_ACTOR_SLOT);
            AslA16(cpu);
            TransferAToX(cpu);
            world_x = Read16Long(memory, LongIndexedAddress(WRAM_ACTOR_FINE_X, cpu->x));
            LoadA16(cpu, world_x);
            cpu->carry = 0;
            Add16Value(cpu, 0x0030u);
            Compare16(cpu, cpu->accumulator,
                      Read16Direct(memory, cpu, SPRITE_DP_WINDOW_LEFT));
            if (!cpu->negative) {
                Compare16(cpu, cpu->accumulator,
                          Read16Direct(memory, cpu, SPRITE_DP_WINDOW_RIGHT));
                if (cpu->negative) {
                    world_y = Read16Long(
                        memory, LongIndexedAddress(WRAM_ACTOR_FINE_Y, cpu->x));
                    LoadA16(cpu, world_y);
                    cpu->carry = 0;
                    Add16Value(cpu, 0x0020u);
                    Compare16(cpu, cpu->accumulator,
                              Read16Direct(memory, cpu, SPRITE_DP_WINDOW_TOP));
                    if (!cpu->negative) {
                        Compare16(cpu, cpu->accumulator,
                                  Read16Direct(memory, cpu, SPRITE_DP_WINDOW_BOTTOM));
                        visible = cpu->negative;
                    }
                }
            }
            if (visible && visibility && visibility->accept)
                visible = visibility->accept(visibility->context, world_x, world_y);
            if (visible) {
                LoadA16(cpu, (uint16_t)(cpu->accumulator | 0x1000u));
                StoreADirect16(memory, cpu, SPRITE_DP_SORT_KEY);
                LoadXDirect8(memory, cpu, DP_ACTOR_SLOT);
                LoadA16(cpu, Read16Long(
                    memory, LongIndexedAddress(WRAM_UNK_7FE316, cpu->x)));
                cpu->zero = (cpu->accumulator & 0x0001u) == 0;
                if (!cpu->zero) {
                    LoadA16(cpu, 0x1000u);
                    TestBitsDirect(memory, cpu, SPRITE_DP_SORT_KEY, 0);
                } else {
                    cpu->zero = (cpu->accumulator & 0x0002u) == 0;
                    if (!cpu->zero) {
                        LoadA16(cpu, 0x2000u);
                        TestBitsDirect(memory, cpu, SPRITE_DP_SORT_KEY, 1);
                    }
                }
                LoadX8(cpu, (uint8_t)cpu->y);                  /* A2F2 */
                while (!cpu->zero) {
                    LoadA16(cpu, Read16AbsoluteIndexed(
                        memory, cpu, 0xe1feu, cpu->x));
                    Compare16(cpu, cpu->accumulator,
                              Read16Direct(memory, cpu, SPRITE_DP_SORT_KEY));
                    if (cpu->carry)
                        break;
                    StoreAAbsolute16(memory, cpu, 0xe200u, cpu->x);
                    LoadA16(cpu, Read16AbsoluteIndexed(
                        memory, cpu, 0xe2feu, cpu->x));
                    StoreAAbsolute16(memory, cpu, 0xe300u, cpu->x);
                    LoadX8(cpu, (uint8_t)(cpu->x - 1u));
                    LoadX8(cpu, (uint8_t)(cpu->x - 1u));
                }
                LoadADirect16(memory, cpu, SPRITE_DP_SORT_KEY); /* A309 */
                StoreAAbsolute16(memory, cpu, 0xe200u, cpu->x);
                LoadADirect16(memory, cpu, DP_ACTOR_SLOT);
                StoreAAbsolute16(memory, cpu, 0xe300u, cpu->x);
                LoadY8(cpu, (uint8_t)(cpu->y + 1u));
                LoadY8(cpu, (uint8_t)(cpu->y + 1u));
            }
            SetAccumulatorWidth(cpu, 1);                       /* A315 */
            LoadXDirect8(memory, cpu, DP_ACTOR_SLOT);
        }
        LoadX8(cpu, (uint8_t)(cpu->x + 1u));                   /* A319 */
        Compare8(cpu, (uint8_t)cpu->x, 0x48u);
    } while (!cpu->zero);
}

/* $83:A34F: draw state of a field-object actor. */
static void FieldObjectActorSetup(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadAAbsolute8(memory, cpu, 0x0732u, 0); /* A34F */
    Compare8(cpu, A8(cpu), 0x10u);
    if (!cpu->carry) {
        TransferDirectToA(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe37eu, cpu->x)));
        Compare8(cpu, A8(cpu), 0xffu);
        if (!cpu->zero) {
            PushIndex(memory, cpu);
            cpu->carry = 0;
            Adc8(cpu, 0x28u);
            StoreADirect8(memory, cpu, DP_ACTOR_SLOT);
            SimulateJslFrame(memory, cpu, 0x83u, 0xa368u);
            Lufia2ActorRecordOffsets(memory, cpu);
            SimulateRtlFrame(memory, cpu);
            LoadXDirect8(memory, cpu, DP_ACTOR_SLOT);
            FieldObjectReload(memory, cpu, 0xa36du);
            cpu->x = PullIndexValue(memory, cpu);
            Write8(memory, DirectAddress(cpu, DP_ACTOR_SLOT), (uint8_t)cpu->x);
            SimulateJslFrame(memory, cpu, 0x83u, 0xa374u);
            Lufia2ActorRecordOffsets(memory, cpu);
            SimulateRtlFrame(memory, cpu);
        } else {
            FieldObjectReload(memory, cpu, 0xa379u); /* A377 */
        }
    }
    SetAccumulatorWidth(cpu, 0); /* A37A */
    LoadXDirect8(memory, cpu, DP_SLOT_WORD_OFFSET);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_ACTOR_FINE_X, cpu->x)));
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, LongIndexedAddress(WRAM_ACTOR_DISPLAY_OFFSET_X,
                                                          cpu->x)));
    Subtract16(cpu, Read16Direct(memory, cpu, SPRITE_DP_CAMERA_X));
    Subtract16(cpu, 0x0008u);
    StoreADirect16(memory, cpu, DP_PROBE_X);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_ACTOR_FINE_Y, cpu->x)));
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, LongIndexedAddress(WRAM_ACTOR_DISPLAY_OFFSET_Y,
                                                          cpu->x)));
    cpu->carry = 0;
    Add16Value(cpu, (uint16_t)~Read16Direct(memory, cpu, SPRITE_DP_CAMERA_Y));
    StoreADirect16(memory, cpu, DP_PROBE_Y);
    SetAccumulatorWidth(cpu, 1);
    LoadXDirect8(memory, cpu, DP_ACTOR_SLOT);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe1ceu, cpu->x)));
    AslA8(cpu);
    TestBitsDirect(memory, cpu, OAM_DP_ATTRIBUTES, 1);
    TransferDirectToA(cpu);
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->x);
    And8(cpu, 0x03u);
    LsrA8(cpu);
    ExchangeAccumulatorBytes(cpu);
    RorA8(cpu);
    ExchangeAccumulatorBytes(cpu);
    LsrA8(cpu);
    ExchangeAccumulatorBytes(cpu);
    RorA8(cpu);
    TestBitsDirect(memory, cpu, OAM_DP_ATTRIBUTES, 1);
}

/* $83:A3BB: draw state of an ordinary actor. */
static void FieldActorSetup(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadAAbsolute8(memory, cpu, WRAM_UNK_7E066A, cpu->x); /* A3BB */
    Compare8(cpu, A8(cpu), AbsoluteByte(memory, cpu, 0x1471u, cpu->x));
    if (!cpu->zero) {
        LoadAAbsolute8(memory, cpu, 0x0732u, 0);
        Compare8(cpu, A8(cpu), 0x0au);
        if (!cpu->carry) {
            LoadAAbsolute8(memory, cpu, WRAM_UNK_7E066A, cpu->x);
            StoreAAbsolute8(memory, cpu, 0x1471u, cpu->x);
            FieldActorFrameUpload(memory, cpu);
        }
    }
    SetAccumulatorWidth(cpu, 0); /* A3D3 */
    LoadXDirect8(memory, cpu, DP_SLOT_WORD_OFFSET);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_ACTOR_FINE_X, cpu->x)));
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, LongIndexedAddress(WRAM_ACTOR_DISPLAY_OFFSET_X,
                                                          cpu->x)));
    Subtract16(cpu, Read16Direct(memory, cpu, SPRITE_DP_CAMERA_X));
    Subtract16(cpu, 0x0008u);
    Subtract16(cpu, Read16Long(memory, 0x7fd081u));
    StoreADirect16(memory, cpu, DP_PROBE_X);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_ACTOR_FINE_Y, cpu->x)));
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, LongIndexedAddress(WRAM_ACTOR_DISPLAY_OFFSET_Y,
                                                          cpu->x)));
    Subtract16(cpu, Read16Direct(memory, cpu, SPRITE_DP_CAMERA_Y));
    cpu->carry = 0;
    Add16Value(cpu, (uint16_t)~Read16Long(memory, 0x7fd083u));
    StoreADirect16(memory, cpu, DP_PROBE_Y);
    SetAccumulatorWidth(cpu, 1);
    LoadXDirect8(memory, cpu, DP_ACTOR_SLOT);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe1ceu, cpu->x)));
    And8(cpu, 0x0eu);
    TestBitsDirect(memory, cpu, OAM_DP_ATTRIBUTES, 1);
    LoadAAbsolute8(memory, cpu, WRAM_UNK_7E1291, cpu->x);
    And8(cpu, 0x18u);
    if (!cpu->zero) {
        LoadAAbsolute8(memory, cpu, WRAM_UNK_7E066A, cpu->x);
        And8(cpu, 0x07u);
        Compare8(cpu, A8(cpu), 0x06u);
        if (cpu->carry) {
            LoadA8(cpu, 0x40u);
            TestBitsDirect(memory, cpu, OAM_DP_ATTRIBUTES, 1);
        }
    }
}

/* $83:A33A: OAM entries for one sorted actor. */
static uint8_t FieldActorOam(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7ee300u, cpu->x)));
    StoreADirect8(memory, cpu, DP_ACTOR_SLOT);
    TransferAToX(cpu);
    AslA8(cpu);
    StoreADirect8(memory, cpu, DP_SLOT_WORD_OFFSET);
    cpu->carry = 0;
    Adc8(cpu, DirectByte(memory, cpu, DP_ACTOR_SLOT));
    StoreADirect8(memory, cpu, DP_SLOT_RECORD_OFFSET);
    Write8(memory, DirectAddress(cpu, OAM_DP_ATTRIBUTES), 0x00u);
    Compare8(cpu, (uint8_t)cpu->x, 0x28u);
    if (cpu->carry) {
        FieldObjectActorSetup(memory, cpu);
    } else {
        FieldActorSetup(memory, cpu);
    }
    LoadXDirect8(memory, cpu, DP_ACTOR_SLOT);                          /* A421 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FE316, cpu->x)));
    And8(cpu, 0x30u);
    TestBitsDirect(memory, cpu, OAM_DP_ATTRIBUTES, 1);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe2a6u, cpu->x)));
    TestBitsDirect(memory, cpu, OAM_DP_ATTRIBUTES, 1);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe25eu, cpu->x)));
    StoreADirect8(memory, cpu, OAM_DP_TILE);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FE216, cpu->x)));
    TransferAToY(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, OAM_DP_NEXT_ENTRY));
    StoreADirect8(memory, cpu, OAM_DP_ENTRY);
    cpu->carry = 0;
    Adc8(cpu, AbsoluteByte(memory, cpu, 0xabf4u, cpu->y));
    StoreADirect8(memory, cpu, OAM_DP_NEXT_ENTRY);
    TransferDirectToA(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, OAM_DP_ENTRY));
    StoreADirect8(memory, cpu, OAM_DP_INDEX);
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    AslA16(cpu);
    AslA16(cpu);
    Add16Value(cpu, 0x0100u);
    TransferAToY(cpu);
    SetAccumulatorWidth(cpu, 1);
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FE216, cpu->x)));
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
Lufia2ExecutionResult Lufia2FieldActorSpritesWithVisibility(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    const Lufia2FieldActorVisibility *visibility) {
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
    PushAndSetDataBank(memory, cpu, 0x7eu);                    /* A21A */
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
            Write16Long(memory,
                        AbsoluteIndexedAddress(
                            cpu, (uint16_t)(OAM_HIGH_TABLE + 2u + 2u * i), 0),
                        0x0000u);
    }
    SetAccumulatorWidth(cpu, 0);                               /* A253 */
    SetIndexWidth(cpu, 1);
    LoadA16(cpu, 0x0040u);
    BitAbsolute16(memory, cpu, WRAM_SCREEN_EFFECTS);
    if (!cpu->zero) {
        LoadA16(cpu, Read16Long(memory, 0x7fd0eeu));
        StoreADirect16(memory, cpu, SPRITE_DP_CAMERA_X);
        LoadA16(cpu, Read16Long(memory, 0x7fd0f0u));
        StoreADirect16(memory, cpu, SPRITE_DP_CAMERA_Y);
    } else {
        LoadA16(cpu, Read16Long(memory, 0x001220u));
        StoreADirect16(memory, cpu, SPRITE_DP_CAMERA_X);
        LoadA16(cpu, Read16Long(memory, 0x001228u));
        StoreADirect16(memory, cpu, SPRITE_DP_CAMERA_Y);
    }
    LoadADirect16(memory, cpu, SPRITE_DP_CAMERA_X); /* A279 */
    cpu->carry = 0;
    Add16Value(cpu, 0x0020u);
    StoreADirect16(memory, cpu, SPRITE_DP_WINDOW_LEFT);
    cpu->carry = 0;
    Add16Value(cpu, 0x0110u);
    StoreADirect16(memory, cpu, SPRITE_DP_WINDOW_RIGHT);
    LoadADirect16(memory, cpu, SPRITE_DP_CAMERA_Y);
    cpu->carry = 0;
    Add16Value(cpu, 0x0010u);
    StoreADirect16(memory, cpu, SPRITE_DP_WINDOW_TOP);
    cpu->carry = 0;
    Add16Value(cpu, 0x0100u);
    StoreADirect16(memory, cpu, SPRITE_DP_WINDOW_BOTTOM);
    if (visibility && visibility->horizontal_padding) {
        /* Host padding; 16-bit wrap kept. */
        const uint16_t padding = visibility->horizontal_padding;
        Write16Direct(
            memory, cpu, SPRITE_DP_WINDOW_LEFT,
            (uint16_t)(Read16Direct(memory, cpu, SPRITE_DP_WINDOW_LEFT) - padding));
        Write16Direct(
            memory, cpu, SPRITE_DP_WINDOW_RIGHT,
            (uint16_t)(Read16Direct(memory, cpu, SPRITE_DP_WINDOW_RIGHT) + padding));
    }
    FieldSortVisible(memory, cpu, visibility);
    Write8(memory, DirectAddress(cpu, SPRITE_DP_ACTOR_COUNT),
           (uint8_t)cpu->y);                                   /* A321 */
    Push8(memory, cpu, 0x83u);                                 /* PHK */
    PullDataBank(memory, cpu);
    LoadA8(cpu, DirectByte(memory, cpu, SPRITE_DP_ACTOR_COUNT));
    if (!cpu->zero) {
        Write8(memory, DirectAddress(cpu, 0xaau), 0x00u);      /* A32E */
        Write8(memory, DirectAddress(cpu, 0xacu), 0x00u);
        LoadA8(cpu, 0x10u);
        StoreADirect8(memory, cpu, OAM_DP_NEXT_ENTRY);
        LoadX8(cpu, 0x00u);
        Write8(memory, DirectAddress(cpu, SPRITE_DP_LIST_INDEX), 0x00u);
        do {
            if (!FieldActorOam(memory, cpu)) {
                result.flow = LUFIA2_EXECUTION_BOUNDARY;
                result.pc = cpu->resume_pc;
                return result;
            }
            ++result.dispatches;
            SetAccumulatorWidth(cpu, 1);                       /* A464 */
            SetIndexWidth(cpu, 1);
            LoadXDirect8(memory, cpu, SPRITE_DP_LIST_INDEX);
            LoadX8(cpu, (uint8_t)(cpu->x + 2u));
            Write8(memory, DirectAddress(cpu, SPRITE_DP_LIST_INDEX), (uint8_t)cpu->x);
            Compare8(cpu, (uint8_t)cpu->x,
                     DirectByte(memory, cpu, SPRITE_DP_ACTOR_COUNT));
        } while (!cpu->zero);
    }
    SetAccumulatorWidth(cpu, 1);                               /* A473 */
    SetIndexWidth(cpu, 1);
    LoadA8(cpu, DirectByte(memory, cpu, OAM_DP_NEXT_ENTRY));
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

Lufia2ExecutionResult Lufia2FieldActorSprites(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    return Lufia2FieldActorSpritesWithVisibility(memory, cpu, 0);
}
