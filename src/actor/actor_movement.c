/* Actor positions, map cells and collision. */

#include "actor/actor_internal.h"
#include "actor/actor_slot_view.h"
#include "core/cpu_internal.h"
#include "core/wram_view.h"
#include "lufia2/actor.h"
#include "system/wram.h"

/* Probe column and row: tile in the low byte. */
enum {
    PROBE_X_HIGH = DP_PROBE_X + 1,
    PROBE_Y_HIGH = DP_PROBE_Y + 1,
    CELL_TILE_MASK = 0x03ff /* low ten bits: metatile number */
};

#define MAP_LAYER_CELLS 0x7f0000u /* cell words of every layer */

/* $83:F9AD / $83:F9B6: X = $8F + $91 * width. */
void Lufia2MapCellIndex(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address,
    uint8_t from_probe) {
    SimulateJsrFrame(memory, cpu, return_address);
    if (from_probe) {
        Write8(memory, DirectAddress(cpu, PROBE_X_HIGH), 0x00u);      /* F9AD */
        Write8(memory, DirectAddress(cpu, PROBE_Y_HIGH), 0x00u);
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_PROBE_X)));
        ExchangeAccumulatorBytes(cpu);
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_PROBE_Y)));
    }
    Write8(memory, SNES_WRMPYA, A8(cpu));                      /* F9B6 */
    LoadA8(cpu, Read8(memory, WRAM_FIELD_SECTION_WIDTH));
    Write8(memory, SNES_WRMPYB, A8(cpu));
    LoadA8(cpu, 0x00u);
    ExchangeAccumulatorBytes(cpu);
    SetAccumulatorWidth(cpu, 0);
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, SNES_RDMPYL));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    SimulateRtsFrame(memory, cpu);
}

/* $83:F988: height bits 7-6 of the map cell at $8F/$91. */
void Lufia2MapTileHeight(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    SimulateJsrFrame(memory, cpu, 0xf98au);                    /* F988 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_PROBE_X))); /* F9F2 */
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_PROBE_Y)));
    Lufia2MapCellOffset(memory, cpu);             /* F9F7 */
    SimulateRtsFrame(memory, cpu);
    SetAccumulatorWidth(cpu, 0);                               /* F98B */
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_FIELD_LAYER_TABLE_OFFSET, 0));
    cpu->carry = 0;
    Add16Value(
        cpu, Read16Long(memory, LongIndexedAddress(WRAM_FIELD_LAYER_CELL_BASE,
            cpu->x)));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(MAP_LAYER_CELLS + 1u, cpu->x)));
    AslA8(cpu);                                                /* F99C */
    Adc8(cpu, 0x00u);
    AslA8(cpu);
    Adc8(cpu, 0x00u);
    And8(cpu, 0x03u);                                          /* F9A2 */
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

/* Scratch byte: non-zero when the actor blocks its cell. */
enum { DP_ACTOR_BLOCKS = 0x9e };

/* Cell attribute bit: occupied by an actor. */
enum { CELL_OCCUPIED = 0x01 };

/* Actors with this state or higher can be solid. */
enum { ACTOR_SOLID_FROM_STATE = 2 };

/* Sprite ids that never block their cell. */
enum { NON_BLOCKING_FIRST = 0x71, NON_BLOCKING_LAST = 0x73 };

/* OR the occupied bit into the cell; flags from OR. */
static void SetCellOccupied(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                            uint32_t attributes, uint16_t cell) {
    const Lufia2Wram wram = WramViewLong(memory);

    LoadA8(cpu, WramReadAt(wram, attributes, cell));
    Or8(cpu, CELL_OCCUPIED);
    WramWriteAt(wram, attributes, cell, A8(cpu));
}

/* $83:FA3F: mark the cell under actor [$A7] occupied. */
void Lufia2ActorMarkMapOccupancy(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    Lufia2ActorSlotView slot;
    uint8_t blocks;

    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    slot = Lufia2ActorSlotAt(memory, cpu, cpu->x);
    WramWrite(wram, DP_ACTOR_BLOCKS, 0);
    if (Lufia2ActorSlotReadLong(&slot, WRAM_UNK_7FE216) >= ACTOR_SOLID_FROM_STATE) {
        const uint8_t sprite = Lufia2ActorSlotReadMirrored(&slot, WRAM_UNK_7E05D2);

        if (sprite < NON_BLOCKING_FIRST || sprite > NON_BLOCKING_LAST)
            WramWrite(wram, DP_ACTOR_BLOCKS, 0xff);
    }
    LoadA8(cpu, Lufia2ActorSlotTileX(&slot));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Lufia2ActorSlotTileY(&slot));
    Lufia2MapCellIndex(memory, cpu, 0xfa67u, 0);
    SetCellOccupied(memory, cpu, WRAM_FIELD_MAP_ATTRIBUTES, cpu->x);
    blocks = WramRead(wram, DP_ACTOR_BLOCKS);
    LoadA8(cpu, blocks);
    if (blocks != 0)
        SetCellOccupied(memory, cpu, MAP_BLOCKING_ATTRIBUTES, cpu->x);
}

/* Tile to 1/16 units; A keeps the shifts' flags. */
static uint16_t TileToFine(Lufia2CpuState *cpu, uint8_t tile) {
    unsigned shifts;

    cpu->accumulator = (uint16_t)((cpu->direct_page & 0xff00u) | tile);
    for (shifts = 0; shifts < 4; ++shifts)
        AslA16(cpu);
    return cpu->accumulator;
}

/* $83:A746: fine position = tile position * 16. */
void Lufia2ActorSyncFinePosition(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewLong(memory);
    Lufia2ActorSlotView slot;
    uint16_t fine_y;

    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    slot = Lufia2ActorSlotAt(memory, cpu, cpu->x);
    TileToFine(cpu, Lufia2ActorSlotTileX(&slot));
    PushAccumulator16(memory, cpu); /* x waits on the stack */
    fine_y = TileToFine(cpu, Lufia2ActorSlotTileY(&slot));
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
    WramWrite16At(wram, WRAM_ACTOR_FINE_Y, cpu->x, fine_y);
    PullAccumulator16(memory, cpu);
    WramWrite16At(wram, WRAM_ACTOR_FINE_X, cpu->x, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
}

/* $83:FAFA: sign-extend A's low byte; positive keeps high byte. */
void Lufia2SignExtendA8(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    const uint8_t value = A8(cpu);

    SimulateJsrFrame(memory, cpu, return_address);
    if (value & 0x80u)
        cpu->accumulator = (uint16_t)(0xff00u | value);
    SetNz8(cpu, value);
    SetAccumulatorWidth(cpu, 0);
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
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_A), A8(cpu));
    Lufia2ActorAddSignedPair(memory, cpu, WRAM_ACTOR_FINE_Y, 0x0002u, 0xfaa6u);
    PrimaryFineToTile(cpu);                                    /* FAB0 */
    LoadYDirect16(memory, cpu, DP_ACTOR_SLOT);                 /* FAB9 */
    StoreAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_Y, cpu->y);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_A)));
    StoreAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_X, cpu->y);
    SetAccumulatorWidth(cpu, 0);                               /* FAC3 */
    LoadA16(cpu, Read16Direct(memory, cpu, 0x2au));
    IncrementA16(cpu);
    IncrementA16(cpu);
    IncrementA16(cpu);
}

/* $83:F9F7: A = column:row, X = cell byte offset. */
Lufia2ExecutionResult Lufia2MapCellOffset(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Write8(memory, SNES_WRMPYA, A8(cpu));                      /* $83:F9F7 */
    LoadA8(cpu, Read8(memory, WRAM_FIELD_SECTION_WIDTH));       /* F9FB */
    Write8(memory, SNES_WRMPYB, A8(cpu));                      /* $83:F9FF */
    LoadA8(cpu, 0x00u);                                 /* $83:FA03 */
    ExchangeAccumulatorBytes(cpu);                      /* $83:FA05 */
    SetAccumulatorWidth(cpu, 0);                        /* $83:FA06 */
    cpu->carry = 0;                                     /* $83:FA08 */
    Add16Value(cpu, Read16Long(memory, SNES_RDMPYL));          /* $83:FA09 */
    AslA16(cpu);                                        /* $83:FA0D */
    TransferAToX(cpu);                                  /* $83:FA0E */
    SetAccumulatorWidth(cpu, 1);                        /* $83:FA0F */
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

/* $83:F9D9: cell offset of A in the selected layer. */
Lufia2ExecutionResult Lufia2LayerCellOffset(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0xf9dbu);                    /* F9D9 */
    (void)Lufia2MapCellOffset(memory, cpu);
    {
        const uint8_t low = Pull8(memory, cpu);
        const uint8_t high = Pull8(memory, cpu);
        const uint16_t frame = (uint16_t)(low | ((uint16_t)high << 8));
        if (frame != 0xf9dbu)
            return ExecutionHandoff(cpu, 0x830000u | (uint16_t)(frame + 1u));
    }

    SetAccumulatorWidth(cpu, 0);                               /* F9DC */
    PushIndex(memory, cpu);                                    /* F9DE */
    LoadA16(cpu, Read16Long(memory, WRAM_FIELD_LAYER_TABLE_OFFSET));
    TransferAToX(cpu);                                         /* F9E3 */
    PullAccumulator16(memory, cpu);                            /* F9E4 */
    cpu->carry = 0;                                            /* F9E5 */
    Add16Value(
        cpu, Read16Long(
            memory, LongIndexedAddress(WRAM_FIELD_LAYER_CELL_BASE, cpu->x)));
    TransferAToX(cpu);                                         /* F9EA */
    SetAccumulatorWidth(cpu, 1);                               /* F9EB */
    return ExecutionReturned(0x83f9edu);
}

void Lufia2ActorResolveMapCellOffset(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_PROBE_X))); /* F9D4 */
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_PROBE_Y)));
    (void)Lufia2LayerCellOffset(memory, cpu);
}

/* $83:FB71: attribute byte of the metatile in the probe cell. */
void Lufia2ActorReadMapCellValue(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0xfb73u);                    /* FB71 */
    Lufia2ActorResolveMapCellOffset(memory, cpu);              /* F9D4 */
    SimulateRtsFrame(memory, cpu);

    SetAccumulatorWidth(cpu, 0);                               /* FB74 */
    LoadA16(
        cpu, Read16Long(
            memory, LongIndexedAddress(MAP_LAYER_CELLS, cpu->x)));   /* FB76 */
    And16(cpu, CELL_TILE_MASK);                                /* FB7A */
    cpu->carry = 0;                                            /* FB7D */
    Add16Value(cpu, Read16Long(memory, WRAM_FIELD_METATILE_ATTRIBUTE_BASE)); /* FB7E */
    TransferAToX(cpu);                                         /* FB82 */
    SetAccumulatorWidth(cpu, 1);                               /* FB83 */
    TransferDirectToA(cpu);                                    /* FB85 */
    LoadA8(
        cpu, Read8(
            memory, LongIndexedAddress(MAP_LAYER_CELLS, cpu->x)));   /* FB86 */
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
