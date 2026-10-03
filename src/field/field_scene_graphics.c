#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "core/wram_view.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    AUXILIARY_RESOURCES = 0x28,
    MAP_TABLE_OFFSET = 0x2a,
    RESOURCE = 0x54,
    RESOURCE_LENGTH = 0x58,
    OBJECT_COLUMNS_REMAINING = 0x5a,
    RESOURCE_DESTINATION = 0x60,
    RESOURCE_DESTINATION_BANK = 0x62,
    OBJECT_ROW_STRIDE = 0x63,
    OBJECT_RECORD = 0x8b,
    OBJECT_VRAM = 0x8d,
    OBJECT_TILE_ATTRIBUTES = 0x98,
    OBJECT_WIDTH = 0xa3,
    OBJECT_ROWS_REMAINING = 0xa5,
    OBJECT_SLOT = 0xa7,
    SCENE_RECORD_BUDGET = 65536,
    WRAM_BANK_7E = 0x7e,
    VRAM_DATA_PORT = 0x18,
    DMA_WORD_PAIR = 0x01,
    DMA_CHANNEL_0 = 0x01,
    OBJECT_PLANE_SIZES = 0x83abfc,
    OBJECT_PLANE_BUFFER = 0xd000,
    OBJECT_SECOND_PLANE_OFFSET = 0x100,
    TILESET_STAGING = 0x4000,
    MAP_FLAG_CAVE = 0x01,
    SCENE_RECORD_BANK = 0xa1,
    SCENE_LIST_INDEX_TABLE = 0xcffba6,
    SCENE_LIST_TABLE = 0xcffc98,
    SCENE_OBJECT_BUDGET = 4096,
    SCENE_METATILE_BUDGET = 4096,
    OBJECT_RECORD_BYTES = 10,
    OBJECT_RECORDS_OFFSET = WRAM_FIELD_HEADER_OBJECT_RECORDS_OFFSET & 0xffffu,
};

static Lufia2ExecutionResult
CallGraphicsChild(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                  Lufia2PushedChildCall child, void *context, uint32_t site,
                  uint32_t target, uint8_t frame, uint8_t accumulator_8) {
    if (frame == 3u)
        SimulateJslFrame(memory, cpu, 0x80u, (uint16_t)(site + 3u));
    else
        SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    if (!child(context, cpu, target, site, frame)) {
        Lufia2ExecutionResult result = ExecutionReturned(site);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    const uint32_t next = site + frame + 1u;
    if (cpu->accumulator_is_8_bit != accumulator_8 ||
        (cpu->index_is_8_bit && site != 0x80f0d4u) || cpu->decimal)
        return ExecutionHandoff(cpu, next);
    return ExecutionReturned(next);
}

/* Copies the offsets and header words of the scene's records into work RAM.
 * The map's record list is a pointer into bank $A1: a count byte at +2 (low
 * seven bits) and then one word per record, which points at the record body.
 * Each record's offset is stored relative to the list, and the first two bytes
 * of the body are kept as its header. The tables end with the direct page's
 * low byte as a marker. */
static Lufia2ExecutionResult IndexSceneRecords(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const Lufia2Wram work = WramViewLong(memory);
    unsigned records = 0;
    uint16_t list;
    uint8_t remaining;

    LoadX16(cpu, WramRead16(wram, WRAM_FIELD_MAP_ID));
    LoadA8(cpu, WramRead(wram, WRAM_FIELD_MAP_FLAGS));
    cpu->zero = (A8(cpu) & MAP_FLAG_CAVE) == 0;
    if (!cpu->zero) {
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, WramRead16(work, WRAM_CAVE_SCENE_RECORD_LIST));
        if (cpu->zero)
            return ExecutionReturned(0x80f028u);
    } else {
        TransferDirectToA(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(SCENE_LIST_INDEX_TABLE, cpu->x)));
        if (cpu->zero)
            return ExecutionReturned(0x80f028u);
        DecrementA8(cpu);
        AslA8(cpu);
        SetAccumulatorWidth(cpu, 0);
        TransferAToX(cpu);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(SCENE_LIST_TABLE, cpu->x)));
    }
    list = cpu->accumulator;
    WramWrite16(work, WRAM_FIELD_SCENE_RECORD_LIST, list);
    WramWrite16(wram, RESOURCE, list);
    cpu->y = list;
    SetAccumulatorWidth(cpu, 1);
    PushDataBank(memory, cpu);
    LoadA8(cpu, SCENE_RECORD_BANK);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);

    remaining =
        (uint8_t)(Read8(memory, AbsoluteIndexedAddress(cpu, 2u, cpu->y)) & 0x7fu);
    WramWrite(wram, RESOURCE_LENGTH, remaining);
    WramWrite(work, WRAM_FIELD_SCENE_RECORD_COUNT, remaining);
    cpu->y = (uint16_t)(cpu->y + 3u);
    cpu->x = 0;
    do {
        uint16_t body;

        /* A direct page that overlaps the stack can keep the count from ever
         * reaching zero; the budget turns that into a handoff. */
        if (records++ == SCENE_RECORD_BUDGET)
            return ExecutionHandoff(cpu, 0x80eff6u);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0u, cpu->y));
        PushY(memory, cpu);
        cpu->carry = 1u;
        Add16Value(cpu, WramRead16(wram, RESOURCE));
        WramWrite16At(work, WRAM_FIELD_SCENE_RECORD_OFFSETS, cpu->x, cpu->accumulator);
        body = (uint16_t)(cpu->accumulator - 1u);
        cpu->y = body;
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0u, body)));
        WramWriteAt(work, WRAM_FIELD_SCENE_RECORD_HEADER_WORDS, cpu->x, A8(cpu));
        LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 1u, body)));
        WramWriteAt(work, WRAM_FIELD_SCENE_RECORD_HEADER_WORDS + 1u, cpu->x, A8(cpu));
        OpPullY(memory, cpu);
        cpu->x = (uint16_t)(cpu->x + 2u);
        cpu->y = (uint16_t)(cpu->y + 2u);
        remaining = (uint8_t)(WramRead(wram, RESOURCE_LENGTH) - 1u);
        WramWrite(wram, RESOURCE_LENGTH, remaining);
        SetNz8(cpu, remaining);
    } while (remaining != 0);
    TransferDirectToA(cpu);
    WramWriteAt(work, WRAM_FIELD_SCENE_RECORD_OFFSETS, cpu->x, A8(cpu));
    WramWriteAt(work, WRAM_FIELD_SCENE_RECORD_OFFSETS + 1u, cpu->x, A8(cpu));
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x80f028u);
}

/* Channel 0 sends the staged object tiles from work RAM to the PPU data port
 * ($2118). */
static void StartObjectPlaneDma(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram io = WramViewLong(memory);

    WramWrite(io, SNES_A1B(0), WRAM_BANK_7E);
    WramWrite(io, SNES_BBAD(0), VRAM_DATA_PORT);
    WramWrite(io, SNES_DMAP(0), DMA_WORD_PAIR);
    LoadA8(cpu, DMA_CHANNEL_0);
    WramWrite(io, SNES_MDMAEN, DMA_CHANNEL_0);
}

/* Sends the assembled object tiles to video memory in two halves: the first
 * plane at the object's address and the second a page further on. */
static void UploadObjectGraphics(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const Lufia2Wram io = WramViewLong(memory);
    uint16_t size;

    LoadX16(cpu, WramRead16(wram, OBJECT_SLOT));
    SetAccumulatorWidth(cpu, 1);
    WramWriteAt(io, WRAM_FIELD_OBJECT_GRAPHICS_PALETTE, cpu->x,
                (uint8_t)((WramRead(wram, OBJECT_TILE_ATTRIBUTES + 1u) & 0x1cu) >> 2));
    cpu->x = (uint16_t)((WramRead16At(io, WRAM_FIELD_OBJECT_GRAPHICS_SHAPE, cpu->x) &
                         0x00ffu)
                        << 1);
    size = WramRead16At(io, OBJECT_PLANE_SIZES, cpu->x);
    WramWrite16(io, SNES_A1TL(0), OBJECT_PLANE_BUFFER);
    WramWrite16(io, SNES_VMADDL, WramRead16(wram, OBJECT_VRAM));
    WramWrite16(io, SNES_DASL(0), size);
    StartObjectPlaneDma(memory, cpu);

    WramWrite16(io, SNES_A1TL(0), (uint16_t)(size + OBJECT_PLANE_BUFFER));
    WramWrite16(io, SNES_DASL(0), size);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, WramRead16(wram, OBJECT_VRAM));
    cpu->carry = 0;
    Add16Value(cpu, OBJECT_SECOND_PLANE_OFFSET);
    WramWrite16(io, SNES_VMADDL, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    StartObjectPlaneDma(memory, cpu);
}

/* Fields of an object record in the work list at $7E:F000 (DB-relative). */
enum {
    OBJECT_FIELD_SLOT = 0xf000u,
    OBJECT_FIELD_FLAGS = 0xf001u,
    OBJECT_FIELD_SOURCE_X = 0xf002u,
    OBJECT_FIELD_SOURCE_Y = 0xf003u,
    OBJECT_FIELD_WIDTH = 0xf004u,
    OBJECT_FIELD_HEIGHT = 0xf005u,
    OBJECT_FIELD_FLAGS_A = 0xf007u,
    OBJECT_FIELD_FLAGS_B = 0xf008u,
    OBJECT_USES_TILES_A = 0x0cu,
    OBJECT_USES_TILES_B = 0x08u,
    SCENE_TILESET_TABLE = 0xcff81eu,
    SCENE_TILESET_RESOURCE_BASE = 0x14bu,
    SCENE_PALETTE_INDEX_TABLE = 0xcffa08u,
    SCENE_PALETTE_TABLE = 0xcffafau,
    AUXILIARY_MAP_TABLE = 0xcff90eu,
    AUXILIARY_RESOURCE_TABLE = 0xcff9feu,
    AUXILIARY_FIRST_RESOURCE_BASE = 0x15fu,
    AUXILIARY_TILES_RESOURCE_BASE = 0x164u,
    OBJECT_FLAG_SECOND_LAYER = 0x01u,
    OBJECT_SPRITE_SLOT_COUNTS = 0x83abf4u,
    OBJECT_TILE_SHAPE_WORDS = 0x80f42au,
    OBJECT_MAP_CELLS = 0x7f0000u,
};

/* Reads the record at Y: slot, width and height. Stores the shape code (height
 * - 1 with width - 1 in the next bit up) for the slot and leaves A at the
 * sprite slot count for that shape, ready for the allocation child. */
static void AssembleReadRecord(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushY(memory, cpu);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpAbsY(cpu, OBJECT_FIELD_SLOT));
    OpTax(cpu);
    OpWriteX(memory, cpu, OpDp(cpu, OBJECT_SLOT), cpu->x);
    OpLda(memory, cpu, OpAbsY(cpu, OBJECT_FIELD_WIDTH));
    OpSta(memory, cpu, OpDp(cpu, OBJECT_WIDTH));
    OpStz(memory, cpu, OpDp(cpu, OBJECT_WIDTH + 1u));
    OpDecA(cpu);
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, RESOURCE));
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpAbsY(cpu, OBJECT_FIELD_HEIGHT));
    OpSta(memory, cpu, OpDp(cpu, OBJECT_ROWS_REMAINING));
    OpStz(memory, cpu, OpDp(cpu, OBJECT_ROWS_REMAINING + 1u));
    OpDecA(cpu);
    OpOra(memory, cpu, OpDp(cpu, RESOURCE));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_OBJECT_GRAPHICS_SHAPE));
    OpWriteX(memory, cpu, OpDp(cpu, OBJECT_RECORD), cpu->y);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, OBJECT_SPRITE_SLOT_COUNTS));
}

/* With the source cell index in Y, picks the layer named by the record flags
 * and leaves X at the object's first map cell and the row stride (section
 * width minus object width, in bytes) in DP $63. */
static void AssembleLocateSource(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbsY(cpu, OBJECT_FIELD_FLAGS));
    OpTxy(cpu);
    OpLdx(cpu, 0u);
    OpBitValue(cpu, OBJECT_FLAG_SECOND_LAYER);
    OpRepWidths(cpu, 0x20u);
    if (cpu->zero)
        OpLdx(cpu, 2u);
    OpTya(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpLongX(cpu, WRAM_FIELD_LAYER_CELL_BASE));
    OpTax(cpu);
    OpLda(memory, cpu, WRAM_FIELD_SECTION_WIDTH);
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, OBJECT_WIDTH)));
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, OBJECT_ROW_STRIDE));
    OpStz(memory, cpu, OpDp(cpu, OBJECT_TILE_ATTRIBUTES));
}

/* Copies the object's metatiles row by row from the map: each cell's metatile
 * is handed to the tile child, and the first cell's attribute word is kept in
 * DP $98. The metatile budget turns a runaway count into a handoff. */
static Lufia2ExecutionResult AssembleCopyMetatiles(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context,
                                                   unsigned *metatiles_copied) {
    Lufia2ExecutionResult result;

    do {
        OpLda(memory, cpu, OpDp(cpu, OBJECT_WIDTH));
        OpSta(memory, cpu, OpDp(cpu, OBJECT_COLUMNS_REMAINING));
        do {
            if ((*metatiles_copied)++ == SCENE_METATILE_BUDGET)
                return ExecutionHandoff(cpu, 0x80f130u);
            OpLda(memory, cpu, OpLongX(cpu, OBJECT_MAP_CELLS));
            OpPushX(memory, cpu);
            OpAndValue(cpu, 0x3ffu);
            for (unsigned shift = 0; shift < 3u; ++shift)
                OpAslA(cpu);
            OpAdc(memory, cpu, WRAM_FIELD_METATILE_BASE);
            OpTax(cpu);
            OpLda(memory, cpu, OpDp(cpu, OBJECT_TILE_ATTRIBUTES));
            if (cpu->zero) {
                OpLda(memory, cpu, OpLongX(cpu, OBJECT_MAP_CELLS));
                OpSta(memory, cpu, OpDp(cpu, OBJECT_TILE_ATTRIBUTES));
            }
            result = CallGraphicsChild(memory, cpu, child, context, 0x80f14au,
                                       0x80f35bu, 2u, 0u);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
            OpPullX(memory, cpu);
            OpInx(cpu);
            OpInx(cpu);
            OpStepMem(memory, cpu, OpDp(cpu, OBJECT_COLUMNS_REMAINING), -1);
        } while (!cpu->zero);
        OpTxa(cpu);
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpDp(cpu, OBJECT_ROW_STRIDE));
        OpTax(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, OBJECT_ROWS_REMAINING), -1);
    } while (!cpu->zero);
    return ExecutionReturned(0u);
}

/* Builds one object's tile graphics: allocates its sprite slots, assembles the
 * metatiles from the map and uploads the result to video memory. */
static Lufia2ExecutionResult AssembleObjectGraphics(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu,
                                                    Lufia2PushedChildCall child,
                                                    void *context,
                                                    unsigned *metatiles_copied) {
    AssembleReadRecord(memory, cpu);
    OpSepWidths(cpu, 0x10u);
    Lufia2ExecutionResult result =
        CallGraphicsChild(memory, cpu, child, context, 0x80f0d4u, 0x83ab7cu, 3u, 1u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpRepWidths(cpu, 0x10u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, OBJECT_SLOT)));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_OBJECT_SPRITE_ALLOCATION));
    ExchangeAccumulatorBytes(cpu);
    OpLoadA(cpu, 0u);
    result =
        CallGraphicsChild(memory, cpu, child, context, 0x80f0e3u, 0x83abe9u, 3u, 0u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpSta(memory, cpu, OpDp(cpu, OBJECT_VRAM));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, OBJECT_SLOT)));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_OBJECT_GRAPHICS_SHAPE));
    OpAndValue(cpu, 0xffu);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, OBJECT_TILE_SHAPE_WORDS));
    OpSta(memory, cpu, OpDp(cpu, RESOURCE));
    OpSepWidths(cpu, 0x20u);
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, OBJECT_RECORD)));
    OpLda(memory, cpu, OpAbsY(cpu, OBJECT_FIELD_SOURCE_X));
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpAbsY(cpu, OBJECT_FIELD_SOURCE_Y));
    result =
        CallGraphicsChild(memory, cpu, child, context, 0x80f105u, 0x83f9eeu, 3u, 1u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    AssembleLocateSource(memory, cpu);
    result = AssembleCopyMetatiles(memory, cpu, child, context, metatiles_copied);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    UploadObjectGraphics(memory, cpu);
    OpPullY(memory, cpu);
    return ExecutionReturned(0x80f1e0u);
}

/* Sends the tileset staged at $7E:4000 to video memory. The length comes from
 * the resource length word unless the caller names one. */
static void StartTilesetDma(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                            uint16_t vram, uint16_t length) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);

    WramWrite(wram, SNES_DMAP(0), DMA_CHANNEL_0);
    WramWrite16(wram, SNES_A1TL(0), TILESET_STAGING);
    WramWrite(wram, SNES_A1B(0), WRAM_BANK_7E);
    WramWrite(wram, SNES_BBAD(0), VRAM_DATA_PORT);
    WramWrite16(wram, SNES_VMADDL, vram);
    LoadX16(cpu, vram == TILESET_STAGING ? WramRead16(wram, RESOURCE_LENGTH) : length);
    WramWrite16(wram, SNES_DASL(0), cpu->x);
    LoadA8(cpu, DMA_CHANNEL_0);
    WramWrite(wram, SNES_MDMAEN, DMA_CHANNEL_0);
}

/* Tests whether the object record at Y has graphics to load: one of the
 * flag bits $0C in its byte 7, or bit 3 of byte 8. Leaves the result in the
 * zero flag (clear when it does). */
static void SceneTestObjectUsesTiles(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbsY(cpu, OBJECT_FIELD_FLAGS_A));
    OpBitValue(cpu, OBJECT_USES_TILES_A);
    if (cpu->zero) {
        OpLda(memory, cpu, OpAbsY(cpu, OBJECT_FIELD_FLAGS_B));
        OpBitValue(cpu, OBJECT_USES_TILES_B);
    }
}

static Lufia2ExecutionResult LoadTilesetAndObjects(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    OpLdx(cpu, 0x4000u);
    OpWriteX(memory, cpu, OpDp(cpu, RESOURCE_DESTINATION), cpu->x);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, RESOURCE_DESTINATION_BANK));
    Lufia2ExecutionResult result =
        CallGraphicsChild(memory, cpu, child, context, 0x80f058u, 0x808e9du, 3u, 1u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    StartTilesetDma(memory, cpu, 0x4000u, 0u);
    PushDataBank(memory, cpu);
    OpLoadA(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpLda(memory, cpu, OpAbs(cpu, OBJECT_RECORDS_OFFSET + 1u));
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpAbs(cpu, OBJECT_RECORDS_OFFSET));
    OpTay(cpu);
    unsigned metatiles_copied = 0;
    for (unsigned records = 0; records < SCENE_OBJECT_BUDGET; ++records) {
        OpLda(memory, cpu, OpAbsY(cpu, OBJECT_FIELD_SLOT));
        OpCmpValue(cpu, 0xffu);
        if (cpu->zero) {
            PullDataBank(memory, cpu);
            return ExecutionReturned(0x80f1eeu);
        }
        SceneTestObjectUsesTiles(memory, cpu);
        if (!cpu->zero) {
            result =
                AssembleObjectGraphics(memory, cpu, child, context, &metatiles_copied);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
        }
        OpRepWidths(cpu, 0x20u);
        OpTya(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, OBJECT_RECORD_BYTES);
        OpTay(cpu);
        OpSepWidths(cpu, 0x20u);
    }
    return ExecutionHandoff(cpu, 0x80f08eu);
}

/* Loads the map's first auxiliary resource (when its table entry names one)
 * into $7E:C000 and records where the resource and its table start. */
static Lufia2ExecutionResult LoadMapAuxiliaryResource(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu,
                                                      Lufia2PushedChildCall child,
                                                      void *context) {
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpLongX(cpu, AUXILIARY_MAP_TABLE));
    if (!cpu->zero) {
        OpRepWidths(cpu, 0x20u);
        OpAslA(cpu);
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
        TransferDirectToA(cpu);
        OpLda(memory, cpu, OpLongX(cpu, AUXILIARY_RESOURCE_TABLE));
        OpSta(memory, cpu, OpDp(cpu, AUXILIARY_RESOURCES));
        OpLda(memory, cpu, OpLongX(cpu, AUXILIARY_RESOURCE_TABLE + 1u));
        OpSta(memory, cpu, OpDp(cpu, AUXILIARY_RESOURCES + 1u));
        OpLda(memory, cpu, OpDp(cpu, AUXILIARY_RESOURCES));
        for (unsigned shift = 0; shift < 4u; ++shift)
            OpLsrA(cpu);
        if (!cpu->zero) {
            OpRepWidths(cpu, 0x20u);
            cpu->carry = 0u;
            OpAdcValue(cpu, AUXILIARY_FIRST_RESOURCE_BASE);
            OpSta(memory, cpu, OpDp(cpu, RESOURCE));
            OpSepWidths(cpu, 0x20u);
            OpLdx(cpu, 0xc000u);
            OpWriteX(memory, cpu, OpDp(cpu, RESOURCE_DESTINATION), cpu->x);
            OpLoadA(cpu, 0x7eu);
            OpSta(memory, cpu, OpDp(cpu, RESOURCE_DESTINATION_BANK));
            Lufia2ExecutionResult result = CallGraphicsChild(
                memory, cpu, child, context, 0x80f24bu, 0x808e9du, 3u, 1u);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, 0x7ec000u);
            cpu->carry = 0u;
            OpAdcValue(cpu, 0xc000u);
            OpSta(memory, cpu, WRAM_FIELD_AUXILIARY_RESOURCE_OFFSET);
            OpLoadA(cpu, 0xc002u);
            OpSta(memory, cpu, WRAM_FIELD_AUXILIARY_TABLE_OFFSET);
            OpSepWidths(cpu, 0x20u);
            TransferDirectToA(cpu);
            OpSta(memory, cpu, WRAM_UNK_7FD0C8);
        }
    }
    return ExecutionReturned(0u);
}

/* Loads the second auxiliary resource, named by the high byte of the pair,
 * into the tileset staging area and sends it to video memory. */
static Lufia2ExecutionResult LoadAuxiliaryTiles(const Lufia2Memory *memory,
                                                Lufia2CpuState *cpu,
                                                Lufia2PushedChildCall child,
                                                void *context) {
    OpLda(memory, cpu, OpDp(cpu, AUXILIARY_RESOURCES + 1u));
    for (unsigned shift = 0; shift < 4u; ++shift)
        OpLsrA(cpu);
    if (!cpu->zero) {
        OpRepWidths(cpu, 0x20u);
        cpu->carry = 0u;
        OpAdcValue(cpu, AUXILIARY_TILES_RESOURCE_BASE);
        OpSta(memory, cpu, OpDp(cpu, RESOURCE));
        OpLoadA(cpu, 0x4000u);
        OpSta(memory, cpu, OpDp(cpu, RESOURCE_DESTINATION));
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, 0x7eu);
        OpSta(memory, cpu, OpDp(cpu, RESOURCE_DESTINATION_BANK));
        Lufia2ExecutionResult result = CallGraphicsChild(memory, cpu, child, context,
                                                         0x80f286u, 0x808e9du, 3u, 1u);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
        StartTilesetDma(memory, cpu, 0x1000u, 0x2000u);
        OpLoadA(cpu, 0x20u);
        OpSta(memory, cpu, WRAM_FIELD_PALETTE_SKIP_BYTES);
    }
    return ExecutionReturned(0u);
}

/* Resets the auxiliary tables, then loads the map's auxiliary resources. */
static Lufia2ExecutionResult LoadAuxiliaryGraphics(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, WRAM_FIELD_AUXILIARY_TABLE_OFFSET);
    OpSta(memory, cpu, (WRAM_FIELD_AUXILIARY_TABLE_OFFSET + 1u));
    OpLoadA(cpu, 0x30u);
    OpSta(memory, cpu, WRAM_FIELD_PALETTE_SKIP_BYTES);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, (WRAM_FIELD_PALETTE_SKIP_BYTES + 1u));
    OpStz(memory, cpu, OpDp(cpu, AUXILIARY_RESOURCES));
    OpStz(memory, cpu, OpDp(cpu, AUXILIARY_RESOURCES + 1u));
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID)));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_FLAGS));
    OpBitValue(cpu, 1u);
    if (!cpu->zero)
        return ExecutionReturned(0x80f2b6u);
    Lufia2ExecutionResult result =
        LoadMapAuxiliaryResource(memory, cpu, child, context);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    result = LoadAuxiliaryTiles(memory, cpu, child, context);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    return ExecutionReturned(0x80f2b6u);
}

/* Names the map's tileset resource in DP $54: the cave generator's choice in
 * a generated cave, otherwise the map's entry in the ROM table plus the
 * resource base. Leaves the zero flag set when there is none. */
static void SceneSelectTileset(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID)));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_FLAGS));
    OpBitValue(cpu, 1u);
    if (!cpu->zero)
        OpLda(memory, cpu, WRAM_CAVE_TILESET_RESOURCE);
    else {
        OpLda(memory, cpu, OpLongX(cpu, SCENE_TILESET_TABLE));
        OpAndValue(cpu, 0xffu);
        cpu->carry = 0u;
        OpAdcValue(cpu, SCENE_TILESET_RESOURCE_BASE);
    }
    OpSta(memory, cpu, OpDp(cpu, RESOURCE));
    OpSepWidths(cpu, 0x20u);
}

/* Picks the palette source: the cave generator's in a generated cave,
 * otherwise the map's entry in the ROM palette table. */
static void ScenePickPalette(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID)));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_FLAGS));
    OpBitValue(cpu, 1u);
    if (!cpu->zero) {
        OpRepWidths(cpu, 0x20u);
        OpLoadA(cpu, 0x30u);
        OpSta(memory, cpu, WRAM_FIELD_PALETTE_SKIP_BYTES);
        OpLda(memory, cpu, WRAM_CAVE_PALETTE_SOURCE);
    } else {
        TransferDirectToA(cpu);
        OpLda(memory, cpu, OpLongX(cpu, SCENE_PALETTE_INDEX_TABLE));
        OpRepWidths(cpu, 0x20u);
        OpDecA(cpu);
        OpAslA(cpu);
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, SCENE_PALETTE_TABLE));
    }
    OpSta(memory, cpu, WRAM_FIELD_PALETTE_SOURCE);
    OpSepWidths(cpu, 0x20u);
}

Lufia2ExecutionResult Lufia2FieldLoadSceneGraphics(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    if (!child || cpu->program_bank != 0x80u || cpu->decimal)
        return ExecutionHandoff(cpu, ((uint32_t)cpu->program_bank << 16) | 0xef8eu);
    if (cpu->accumulator_is_8_bit)
        PushAccumulator8(memory, cpu);
    else
        PushAccumulator16(memory, cpu);
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    Push8(memory, cpu, 0x80u);
    PullDataBank(memory, cpu);
    OpRepWidths(cpu, 0x30u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID));
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, MAP_TABLE_OFFSET));
    OpLoadA(cpu, 0xffffu);
    OpSta(memory, cpu, WRAM_FIELD_AUXILIARY_TABLE_OFFSET);
    OpSepWidths(cpu, 0x20u);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, WRAM_FIELD_SCENE_RECORD_COUNT);
    OpSta(memory, cpu, WRAM_UNK_7E09A9);
    Lufia2ExecutionResult result =
        CallGraphicsChild(memory, cpu, child, context, 0x80efafu, 0x80f2f3u, 3u, 1u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    result = IndexSceneRecords(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    SceneSelectTileset(memory, cpu);
    if (!cpu->zero) {
        result = LoadTilesetAndObjects(memory, cpu, child, context);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
        result = LoadAuxiliaryGraphics(memory, cpu, child, context);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
    }
    ScenePickPalette(memory, cpu);
    result =
        CallGraphicsChild(memory, cpu, child, context, 0x80f2e5u, 0x80f338u, 3u, 1u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, DP_NMI_UPLOAD_FLAGS));
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    if (cpu->accumulator_is_8_bit)
        LoadA8(cpu, Pull8(memory, cpu));
    else
        PullAccumulator16(memory, cpu);
    return ExecutionReturned(0x80f2f2u);
}
