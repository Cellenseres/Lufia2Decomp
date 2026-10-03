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

/* Copy the scene records' offsets and headers into WRAM. */
static Lufia2ExecutionResult IndexSceneRecords(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu) {
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID)));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_FLAGS));
    OpBitValue(cpu, MAP_FLAG_CAVE);
    if (!cpu->zero) {
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, WRAM_CAVE_SCENE_RECORD_LIST);
        if (cpu->zero)
            return ExecutionReturned(0x80f028u);
    } else {
        TransferDirectToA(cpu);
        OpLda(memory, cpu, OpLongX(cpu, SCENE_LIST_INDEX_TABLE));
        if (cpu->zero)
            return ExecutionReturned(0x80f028u);
        OpDecA(cpu);
        OpAslA(cpu);
        OpRepWidths(cpu, 0x20u);
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, SCENE_LIST_TABLE));
    }
    OpSta(memory, cpu, WRAM_FIELD_SCENE_RECORD_LIST);
    OpSta(memory, cpu, OpDp(cpu, RESOURCE));
    OpTay(cpu);
    OpSepWidths(cpu, 0x20u);
    PushDataBank(memory, cpu);
    OpLoadA(cpu, SCENE_RECORD_BANK);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpLda(memory, cpu, OpAbsY(cpu, 2u));
    OpAndValue(cpu, 0x7fu);
    OpSta(memory, cpu, OpDp(cpu, RESOURCE_LENGTH));
    OpSta(memory, cpu, WRAM_FIELD_SCENE_RECORD_COUNT);
    OpIny(cpu);
    OpIny(cpu);
    OpIny(cpu);
    OpLdx(cpu, 0u);
    unsigned records = 0;
    do {
        if (records++ == SCENE_RECORD_BUDGET)
            return ExecutionHandoff(cpu, 0x80eff6u);
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbsY(cpu, 0u));
        PushY(memory, cpu);
        cpu->carry = 1u;
        OpAdc(memory, cpu, OpDp(cpu, RESOURCE));
        OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_SCENE_RECORD_OFFSETS));
        OpDecA(cpu);
        OpTay(cpu);
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbsY(cpu, 0u));
        OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_SCENE_RECORD_HEADER_WORDS));
        OpLda(memory, cpu, OpAbsY(cpu, 1u));
        OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_SCENE_RECORD_HEADER_WORDS + 1u));
        OpPullY(memory, cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, RESOURCE_LENGTH), -1);
    } while (!cpu->zero);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_SCENE_RECORD_OFFSETS));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_SCENE_RECORD_OFFSETS + 1u));
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x80f028u);
}

/* Channel 0 sends staged object tiles to $2118. */
static void StartObjectPlaneDma(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLoadA(cpu, WRAM_BANK_7E);
    OpSta(memory, cpu, SNES_A1B(0));
    OpLoadA(cpu, VRAM_DATA_PORT);
    OpSta(memory, cpu, SNES_BBAD(0));
    OpLoadA(cpu, DMA_WORD_PAIR);
    OpSta(memory, cpu, SNES_DMAP(0));
    OpLoadA(cpu, DMA_CHANNEL_0);
    OpSta(memory, cpu, SNES_MDMAEN);
}

/* Upload object tiles in two halves, a page apart. */
static void UploadObjectGraphics(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, OBJECT_SLOT)));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, OBJECT_TILE_ATTRIBUTES + 1u));
    OpAndValue(cpu, 0x1cu);
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_OBJECT_GRAPHICS_PALETTE));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_OBJECT_GRAPHICS_SHAPE));
    OpAndValue(cpu, 0xffu);
    OpAslA(cpu);
    OpTax(cpu);
    OpLoadA(cpu, OBJECT_PLANE_BUFFER);
    OpSta(memory, cpu, SNES_A1TL(0));
    OpLda(memory, cpu, OpDp(cpu, OBJECT_VRAM));
    OpSta(memory, cpu, SNES_VMADDL);
    OpLda(memory, cpu, OpLongX(cpu, OBJECT_PLANE_SIZES));
    OpSta(memory, cpu, SNES_DASL(0));
    OpSepWidths(cpu, 0x20u);
    StartObjectPlaneDma(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, OBJECT_PLANE_SIZES));
    cpu->carry = 0u;
    OpAdcValue(cpu, OBJECT_PLANE_BUFFER);
    OpSta(memory, cpu, SNES_A1TL(0));
    OpLda(memory, cpu, OpLongX(cpu, OBJECT_PLANE_SIZES));
    OpSta(memory, cpu, SNES_DASL(0));
    OpLda(memory, cpu, OpDp(cpu, OBJECT_VRAM));
    cpu->carry = 0u;
    OpAdcValue(cpu, OBJECT_SECOND_PLANE_OFFSET);
    OpSta(memory, cpu, SNES_VMADDL);
    OpSepWidths(cpu, 0x20u);
    StartObjectPlaneDma(memory, cpu);
}

/* Object record fields in the $7E:F000 work list. */
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

/* Read record Y: slot, size; A = sprite slot count. */
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

/* Locate the object's first map cell and row stride. */
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

/* Copy the object's metatiles row by row from the map. */
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

/* Build one object's tiles: allocate, assemble, upload. */
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

/* DMA the tileset staged at $7E:4000 to VRAM. */
static void StartTilesetDma(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                            uint16_t vram, uint16_t length) {
    OpLoadA(cpu, DMA_WORD_PAIR);
    OpSta(memory, cpu, OpAbs(cpu, SNES_DMAP(0)));
    OpLdx(cpu, TILESET_STAGING);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_A1TL(0)), cpu->x);
    OpLoadA(cpu, WRAM_BANK_7E);
    OpSta(memory, cpu, OpAbs(cpu, SNES_A1B(0)));
    OpLoadA(cpu, VRAM_DATA_PORT);
    OpSta(memory, cpu, OpAbs(cpu, SNES_BBAD(0)));
    OpLdx(cpu, vram);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_VMADDL), cpu->x);
    if (vram == TILESET_STAGING)
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, RESOURCE_LENGTH)));
    else
        OpLdx(cpu, length);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_DASL(0)), cpu->x);
    OpLoadA(cpu, DMA_CHANNEL_0);
    OpSta(memory, cpu, OpAbs(cpu, SNES_MDMAEN));
}

/* Does object record Y load graphics? Zero flag answers. */
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

/* Load the map's first auxiliary resource to $7E:C000. */
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

/* Load and upload the second auxiliary tile resource. */
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

/* Tileset resource in DP $54; zero flag when none. */
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

/* Palette source: cave generator's or the map's ROM entry. */
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
