/* Actor positions, map cells and collision. */

#include "actor/actor_internal.h"
#include "core/cpu_internal.h"
#include "core/hardware_math.h"
#include "core/wram_view.h"
#include "lufia2/actor.h"
#include "system/wram.h"

/* The probe column and row are words whose tile number sits in the low byte;
 * the cell loader clears the high bytes. */
enum {
    PROBE_X_HIGH = DP_PROBE_X + 1,
    PROBE_Y_HIGH = DP_PROBE_Y + 1,
    MAP_CELL_SIZE = 2, /* bytes per cell in a layer */
    CELL_HEIGHT_SHIFT = 6,
    CELL_HEIGHT_MASK = 3,
    CELL_TILE_MASK = 0x03ff, /* low ten bits: metatile number */
};

#define MAP_LAYER_CELLS 0x7f0000u /* cell words of every layer */

/* Cell number of a tile in the section: the map is stored row by row. The row
 * offset comes from the hardware multiplier. */
static uint16_t MapCellIndex(const Lufia2Memory *memory, uint8_t tile_x,
                             uint8_t tile_y) {
    const uint8_t width = WramRead(WramViewLong(memory), WRAM_FIELD_SECTION_WIDTH);

    return (uint16_t)(tile_x + HardwareMultiply8(memory, tile_y, width));
}

/* The original adds the tile column to the row offset in A; the sum and its
 * flags are what the callers see. */
static uint16_t AddRowOffset(Lufia2CpuState *cpu, uint8_t tile_x, uint16_t index) {
    cpu->accumulator = tile_x;
    cpu->carry = 0;
    Add16Value(cpu, (uint16_t)(index - tile_x));
    return cpu->accumulator;
}

/* $83:F9AD / $83:F9B6: X = $8F + $91 * width. */
void Lufia2MapCellIndex(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                        uint16_t return_address, uint8_t from_probe) {
    const Lufia2Wram dp = WramViewOfCaller(memory, cpu);
    uint8_t tile_x = (uint8_t)(cpu->accumulator >> 8);
    uint8_t tile_y = A8(cpu);
    uint16_t index;

    SimulateJsrFrame(memory, cpu, return_address);
    if (from_probe) {
        WramWrite(dp, PROBE_X_HIGH, 0);
        WramWrite(dp, PROBE_Y_HIGH, 0);
        tile_x = WramRead(dp, DP_PROBE_X);
        tile_y = WramRead(dp, DP_PROBE_Y);
    }
    index = MapCellIndex(memory, tile_x, tile_y);
    cpu->accumulator = AddRowOffset(cpu, tile_x, index);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    SimulateRtsFrame(memory, cpu);
}

/* Terrain height class of a layer cell: the top two bits of its high byte. */
static uint8_t CellHeight(const Lufia2Memory *memory, uint16_t cell_offset) {
    const uint8_t high_byte =
        Read8(memory, LongIndexedAddress(MAP_LAYER_CELLS + 1u, cell_offset));

    return (uint8_t)((high_byte >> CELL_HEIGHT_SHIFT) & CELL_HEIGHT_MASK);
}

/* Start of the selected layer's cell data; the layer is a byte index into
 * the table of layer bases. */
static uint16_t ReadLayerCellBase(Lufia2Wram wram, uint16_t layer) {
    return Read16Long(wram.memory,
                      LongIndexedAddress(WRAM_FIELD_LAYER_CELL_BASE, layer));
}

/* $83:F988: height bits 7-6 of the map cell at $8F/$91. */
void Lufia2MapTileHeight(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                         uint16_t return_address) {
    const Lufia2Wram caller = WramViewOfCaller(memory, cpu);
    uint16_t cell_offset;
    uint16_t cell;
    uint8_t tile_x;
    uint8_t tile_y;
    uint8_t height;

    SimulateJsrFrame(memory, cpu, return_address);
    SimulateJsrFrame(memory, cpu, 0xf98au);                    /* F988 */
    tile_x = WramRead(caller, DP_PROBE_X);
    tile_y = WramRead(caller, DP_PROBE_Y);
    cell_offset = (uint16_t)(MapCellIndex(memory, tile_x, tile_y) * MAP_CELL_SIZE);
    SimulateRtsFrame(memory, cpu);
    cell = (uint16_t)(cell_offset +
                      ReadLayerCellBase(
                          caller, WramRead16(caller, WRAM_FIELD_LAYER_TABLE_OFFSET)));
    height = CellHeight(memory, cell);
    /* Exit: X is the cell, A keeps its high byte, C and V clear, N clear. */
    cpu->x = cell;
    cpu->accumulator = (uint16_t)((cell & 0xff00u) | height);
    cpu->accumulator_is_8_bit = 1;
    cpu->carry = 0;
    cpu->overflow = 0;
    SetNz8(cpu, height);
    SimulateRtsFrame(memory, cpu);
}

/* Collision byte test: A = mask & $7E:4000+offset,X. */
static void PrimaryCollisionTest(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t mask,
    uint8_t second_column) {
    LoadA8(cpu, mask);
    And8(
        cpu, Read8(
            memory,
            LongIndexedAddress(
                second_column ? 0x7e4001u : 0x7e4000u, cpu->x)));
}

/* $9A >= 2 means a two-column actor. */
static uint8_t PrimaryWideActor(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x9au)));
    Compare8(cpu, A8(cpu), 0x02u);
    return cpu->carry;
}

/* $83:D89E body: Z clear = blocked, 0 = unknown target. */
uint8_t Lufia2ActorStepBlockedBody(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    uint16_t target;

    TransferAToX(cpu);                                         /* D89E */
    target = Read16ProgramIndexed(memory, cpu, 0xd8a3u, cpu->x);
    SimulateJsrFrame(memory, cpu, 0xd8a1u);                    /* D89F */

    switch (target) {
    case 0xd8abu:
        IncrementDirect8(memory, cpu, DP_PROBE_Y);             /* D8AB */
        Lufia2MapCellIndex(memory, cpu, 0xd8afu, 1);
        PrimaryCollisionTest(memory, cpu, 0x9bu, 0);
        if (!cpu->zero)
            break;
        if (!PrimaryWideActor(memory, cpu)) {
            TransferDirectToA(cpu);                            /* D8BE */
            break;
        }
        PrimaryCollisionTest(memory, cpu, 0x9bu, 1);           /* D8C0 */
        break;

    case 0xd8c7u:
        Lufia2MapCellIndex(memory, cpu, 0xd8c9u, 1);           /* D8C7 */
        PrimaryCollisionTest(memory, cpu, 0x20u, 0);
        if (!cpu->zero)
            break;
        DecrementDirect8(memory, cpu, DP_PROBE_X);             /* D8D2 */
        Lufia2MapCellIndex(memory, cpu, 0xd8d6u, 1);
        PrimaryCollisionTest(memory, cpu, 0x8bu, 0);
        break;

    case 0xd8deu:
        Lufia2MapCellIndex(memory, cpu, 0xd8e0u, 1);           /* D8DE */
        PrimaryCollisionTest(memory, cpu, 0x10u, 0);
        if (!cpu->zero)
            break;
        if (PrimaryWideActor(memory, cpu)) {
            PrimaryCollisionTest(memory, cpu, 0x10u, 1);       /* D8EF */
            if (!cpu->zero)
                break;
        }
        DecrementDirect8(memory, cpu, DP_PROBE_Y);             /* D8F7 */
        Lufia2MapCellIndex(memory, cpu, 0xd8fbu, 1);
        PrimaryCollisionTest(memory, cpu, 0x8bu, 0);
        if (!cpu->zero)
            break;
        if (!PrimaryWideActor(memory, cpu)) {
            TransferDirectToA(cpu);                            /* D90A */
            break;
        }
        PrimaryCollisionTest(memory, cpu, 0x8bu, 1);           /* D90C */
        break;

    case 0xd913u:
        if (PrimaryWideActor(memory, cpu))                     /* D913 */
            IncrementDirect8(memory, cpu, DP_PROBE_X);
        IncrementDirect8(memory, cpu, DP_PROBE_X);             /* D91B */
        Lufia2MapCellIndex(memory, cpu, 0xd91fu, 1);
        PrimaryCollisionTest(memory, cpu, 0xabu, 0);
        break;

    default:
        cpu->resume_pc = 0x830000u | target;
        return 0;
    }

    SimulateRtsFrame(memory, cpu);
    return 1;
}

void Lufia2ActorMarkMapOccupancy(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);                   /* FA3F */
    Write8(memory, DirectAddress(cpu, 0x9eu), 0x00u);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FE216, cpu->x)));
    Compare8(cpu, A8(cpu), 0x02u);                             /* FA47 */
    if (cpu->carry) {
        LoadAAbsolute8(memory, cpu, WRAM_UNK_7E05D2, cpu->x);          /* FA4B */
        Compare8(cpu, A8(cpu), 0x71u);
        if (!cpu->zero) {
            Compare8(cpu, A8(cpu), 0x72u);
            if (!cpu->zero) {
                Compare8(cpu, A8(cpu), 0x73u);
                if (!cpu->zero) {
                    LoadA8(cpu, 0xffu);                        /* FA5A */
                    Write8(memory, DirectAddress(cpu, 0x9eu), A8(cpu));
                }
            }
        }
    }
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_X, cpu->x);              /* FA5E */
    ExchangeAccumulatorBytes(cpu);
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_Y, cpu->x);
    Lufia2MapCellIndex(memory, cpu, 0xfa67u, 0);               /* FA65 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_FIELD_MAP_ATTRIBUTES, cpu->x)));
    Or8(cpu, 0x01u);
    Write8(memory, LongIndexedAddress(WRAM_FIELD_MAP_ATTRIBUTES, cpu->x), A8(cpu));
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x9eu)));     /* FA72 */
    if (!cpu->zero) {
        LoadA8(
            cpu, Read8(memory, LongIndexedAddress(0x7e4001u, cpu->x)));
        Or8(cpu, 0x01u);
        Write8(memory, LongIndexedAddress(0x7e4001u, cpu->x), A8(cpu));
    }
}

void Lufia2ActorSyncFinePosition(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);                   /* A746 */
    TransferDirectToA(cpu);
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_X, cpu->x);
    SetAccumulatorWidth(cpu, 0);                               /* A74C */
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    PushAccumulator16(memory, cpu);                            /* A752 */
    SetAccumulatorWidth(cpu, 1);
    TransferDirectToA(cpu);                                    /* A755 */
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_Y, cpu->x);
    SetAccumulatorWidth(cpu, 0);                               /* A759 */
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);                           /* A75F */
    Write16Long(
        memory, LongIndexedAddress(WRAM_ACTOR_FINE_Y, cpu->x), cpu->accumulator);
    PullAccumulator16(memory, cpu);                            /* A765 */
    Write16Long(
        memory, LongIndexedAddress(WRAM_ACTOR_FINE_X, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);                               /* A76A */
}

/* $83:FAFA: sign-extend A.low, leave M=0. */
void Lufia2SignExtendA8(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Or8(cpu, 0x00u);                                           /* FAFA */
    if (cpu->negative) {
        ExchangeAccumulatorBytes(cpu);                         /* FAFE */
        LoadA8(cpu, 0xffu);
        ExchangeAccumulatorBytes(cpu);
    }
    SetAccumulatorWidth(cpu, 0);                               /* FB02 */
    SimulateRtsFrame(memory, cpu);
}

/* Signed operand bytes added to a 16-bit X/Y pair. */
void Lufia2ActorAddSignedPair(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t pair,
    uint16_t operand,
    uint16_t return_address) {
    TransferDirectToA(cpu);
    LoadAAbsolute8(memory, cpu, operand, cpu->y);
    Lufia2SignExtendA8(memory, cpu, return_address);
    cpu->carry = 0;
    Add16Value(
        cpu, Read16Long(memory, LongIndexedAddress(pair, cpu->x)));
    Write16Long(memory, LongIndexedAddress(pair, cpu->x), cpu->accumulator);
}

void Lufia2ActorAddDisplayOffset(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);                           /* FACB */
    Lufia2ActorAddSignedPair(memory, cpu, WRAM_ACTOR_DISPLAY_OFFSET_X, 0x0001u,
        0xfad3u);
    SetAccumulatorWidth(cpu, 1);                               /* FADD */
    Lufia2ActorAddSignedPair(memory, cpu, WRAM_ACTOR_DISPLAY_OFFSET_Y, 0x0002u,
        0xfae5u);
    LoadA16(cpu, cpu->y);                                      /* FAEF */
    IncrementA16(cpu);
    IncrementA16(cpu);
    IncrementA16(cpu);
}

/* ADC #0 after LSR x4 rounds the 1/16 position. */
static void PrimaryFineToTile(Lufia2CpuState *cpu) {
    LsrA16(cpu);
    LsrA16(cpu);
    LsrA16(cpu);
    LsrA16(cpu);
    Add16Value(cpu, 0x0000u);
    SetAccumulatorWidth(cpu, 1);
}

void Lufia2ActorMoveFinePosition(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadYDirect16(memory, cpu, 0x2au);                         /* FA81 */
    TransferDirectToA(cpu);
    LoadAAbsolute8(memory, cpu, 0x0001u, cpu->y);
    Lufia2SignExtendA8(memory, cpu, 0xfa89u);
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);                           /* FA8A */
    cpu->carry = 0;
    Add16Value(
        cpu, Read16Long(memory, LongIndexedAddress(WRAM_ACTOR_FINE_X, cpu->x)));
    Write16Long(
        memory, LongIndexedAddress(WRAM_ACTOR_FINE_X, cpu->x), cpu->accumulator);
    PrimaryFineToTile(cpu);                                    /* FA95 */
    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
    Lufia2ActorAddSignedPair(memory, cpu, WRAM_ACTOR_FINE_Y, 0x0002u, 0xfaa6u);
    PrimaryFineToTile(cpu);                                    /* FAB0 */
    LoadYDirect16(memory, cpu, DP_ACTOR_SLOT);                 /* FAB9 */
    StoreAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_Y, cpu->y);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    StoreAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_X, cpu->y);
    SetAccumulatorWidth(cpu, 0);                               /* FAC3 */
    LoadA16(cpu, Read16Direct(memory, cpu, 0x2au));
    IncrementA16(cpu);
    IncrementA16(cpu);
    IncrementA16(cpu);
}

/* $83:F9F7: A = column:row, X = byte offset of the cell in a layer. */
Lufia2ExecutionResult Lufia2MapCellOffset(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const uint8_t tile_x = (uint8_t)(cpu->accumulator >> 8);
    const uint16_t index = MapCellIndex(memory, tile_x, A8(cpu));

    AddRowOffset(cpu, tile_x, index);
    AslA16(cpu);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x83fa11u);
}

uint32_t Lufia2ActorMovementStep(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    uint16_t target;
    uint32_t address;
    uint8_t value;

    ExchangeAccumulatorBytes(cpu);                      /* $83:FB12 */
    LoadA8(cpu, 0x00u);                                 /* $83:FB13 */
    ExchangeAccumulatorBytes(cpu);                      /* $83:FB15 */
    TransferAToX(cpu);                                  /* $83:FB16 */
    target = Read16ProgramIndexed(memory, cpu, 0xfb1au, cpu->x);
                                                               /* $83:FB17 */

    switch (target) {
    case 0xfb22u:
        address = DirectAddress(cpu, DP_PROBE_Y);
        value = (uint8_t)(Read8(memory, address) + 1u);
        Write8(memory, address, value);
        SetNz8(cpu, value);
        return 0x83fb24u;
    case 0xfb25u:
        address = DirectAddress(cpu, DP_PROBE_X);
        value = (uint8_t)(Read8(memory, address) - 1u);
        Write8(memory, address, value);
        SetNz8(cpu, value);
        return 0x83fb27u;
    case 0xfb28u:
        address = DirectAddress(cpu, DP_PROBE_Y);
        value = (uint8_t)(Read8(memory, address) - 1u);
        Write8(memory, address, value);
        SetNz8(cpu, value);
        return 0x83fb2au;
    case 0xfb2bu:
        address = DirectAddress(cpu, DP_PROBE_X);
        value = (uint8_t)(Read8(memory, address) + 1u);
        Write8(memory, address, value);
        SetNz8(cpu, value);
        return 0x83fb2du;
    default:
        cpu->resume_pc = 0x83fb17u;
        return 0;
    }
}

/* $83:F9D9: the cell offset of A in the selected layer's cell data. */
Lufia2ExecutionResult Lufia2LayerCellOffset(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewLong(memory);

    SimulateJsrFrame(memory, cpu, 0xf9dbu);                    /* F9D9 */
    (void)Lufia2MapCellOffset(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    SetAccumulatorWidth(cpu, 0);                               /* F9DC */
    PushIndex(memory, cpu);                                    /* F9DE */
    PullAccumulator16(memory, cpu);
    cpu->carry = 0;
    Add16Value(
        cpu, ReadLayerCellBase(wram, WramRead16(wram, WRAM_FIELD_LAYER_TABLE_OFFSET)));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);                               /* F9EB */
    return ExecutionReturned(0x83f9edu);
}

/* A = column:row of the probe position. */
static void LoadProbeTile(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram dp = WramViewOfCaller(memory, cpu);

    LoadA8(cpu, WramRead(dp, DP_PROBE_X));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, WramRead(dp, DP_PROBE_Y));
}

void Lufia2ActorResolveMapCellOffset(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadProbeTile(memory, cpu); /* F9D4 */
    (void)Lufia2LayerCellOffset(memory, cpu);
}

/* $83:FB71: attribute byte of the metatile in the probe cell. */
void Lufia2ActorReadMapCellValue(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewLong(memory);
    uint16_t metatile;

    SimulateJsrFrame(memory, cpu, 0xfb73u);                    /* FB71 */
    Lufia2ActorResolveMapCellOffset(memory, cpu);              /* F9D4 */
    SimulateRtsFrame(memory, cpu);
    SetAccumulatorWidth(cpu, 0);                               /* FB74 */
    metatile = Read16Long(memory, LongIndexedAddress(MAP_LAYER_CELLS, cpu->x)) &
               CELL_TILE_MASK;
    cpu->accumulator = metatile;
    cpu->carry = 0;
    Add16Value(cpu, WramRead16(wram, WRAM_FIELD_METATILE_ATTRIBUTE_BASE));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    TransferDirectToA(cpu);                                    /* FB85 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(MAP_LAYER_CELLS, cpu->x)));
}

static void ClearCellBit0(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t base) {
    const uint32_t address = LongIndexedAddress(base, cpu->x);
    LoadA8(cpu, Read8(memory, address));
    And8(cpu, 0xfeu);
    Write8(memory, address, A8(cpu));
}

/* $83:FA12: clear occupancy bit 0 under the actor. */
void Lufia2ActorClearMapOccupancy(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);                   /* FA12 */
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_X, cpu->x);
    ExchangeAccumulatorBytes(cpu);
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_Y, cpu->x);
    Lufia2MapCellIndex(memory, cpu, 0xfa1du, 0);               /* F9B6 */
    ClearCellBit0(memory, cpu, WRAM_FIELD_MAP_ATTRIBUTES); /* FA1E */
    PushIndex(memory, cpu);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FE216, cpu->x)));
    Compare8(cpu, A8(cpu), 0x02u);
    cpu->x = PullIndexValue(memory, cpu);                      /* FA31 */
    if (cpu->carry)
        ClearCellBit0(memory, cpu, 0x7e4001u);                 /* FA34 */
}
