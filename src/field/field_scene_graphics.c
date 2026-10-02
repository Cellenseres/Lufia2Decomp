#include "core/cpu_ops.h"
#include "core/snes_registers.h"
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

static Lufia2ExecutionResult IndexSceneRecords(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu) {
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID)));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_FLAGS));
    OpBitValue(cpu, 1u);
    if (!cpu->zero) {
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, WRAM_CAVE_SCENE_RECORD_LIST);
        if (cpu->zero)
            return ExecutionReturned(0x80f028u);
    } else {
        TransferDirectToA(cpu);
        OpLda(memory, cpu, OpLongX(cpu, 0xcffba6u));
        if (cpu->zero)
            return ExecutionReturned(0x80f028u);
        OpDecA(cpu);
        OpAslA(cpu);
        OpRepWidths(cpu, 0x20u);
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, 0xcffc98u));
    }
    OpSta(memory, cpu, WRAM_FIELD_SCENE_RECORD_LIST);
    OpSta(memory, cpu, OpDp(cpu, RESOURCE));
    OpTay(cpu);
    OpSepWidths(cpu, 0x20u);
    PushDataBank(memory, cpu);
    OpLoadA(cpu, 0xa1u);
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
        OpSta(memory, cpu, OpLongX(cpu, 0x7fed01u));
        OpPullY(memory, cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, RESOURCE_LENGTH), -1);
    } while (!cpu->zero);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_SCENE_RECORD_OFFSETS));
    OpSta(memory, cpu, OpLongX(cpu, 0x7fec01u));
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x80f028u);
}

static void StartObjectPlaneDma(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, SNES_A1B(0));
    OpLoadA(cpu, 0x18u);
    OpSta(memory, cpu, SNES_BBAD(0));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, SNES_DMAP(0));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, SNES_MDMAEN);
}

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
    OpLoadA(cpu, 0xd000u);
    OpSta(memory, cpu, SNES_A1TL(0));
    OpLda(memory, cpu, OpDp(cpu, OBJECT_VRAM));
    OpSta(memory, cpu, SNES_VMADDL);
    OpLda(memory, cpu, OpLongX(cpu, 0x83abfcu));
    OpSta(memory, cpu, SNES_DASL(0));
    OpSepWidths(cpu, 0x20u);
    StartObjectPlaneDma(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, 0x83abfcu));
    cpu->carry = 0u;
    OpAdcValue(cpu, 0xd000u);
    OpSta(memory, cpu, SNES_A1TL(0));
    OpLda(memory, cpu, OpLongX(cpu, 0x83abfcu));
    OpSta(memory, cpu, SNES_DASL(0));
    OpLda(memory, cpu, OpDp(cpu, OBJECT_VRAM));
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x100u);
    OpSta(memory, cpu, SNES_VMADDL);
    OpSepWidths(cpu, 0x20u);
    StartObjectPlaneDma(memory, cpu);
}

static Lufia2ExecutionResult AssembleObjectGraphics(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu,
                                                    Lufia2PushedChildCall child,
                                                    void *context,
                                                    unsigned *metatiles_copied) {
    PushY(memory, cpu);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpAbsY(cpu, 0xf000u));
    OpTax(cpu);
    OpWriteX(memory, cpu, OpDp(cpu, OBJECT_SLOT), cpu->x);
    OpLda(memory, cpu, OpAbsY(cpu, 0xf004u));
    OpSta(memory, cpu, OpDp(cpu, OBJECT_WIDTH));
    OpStz(memory, cpu, OpDp(cpu, OBJECT_WIDTH + 1u));
    OpDecA(cpu);
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, RESOURCE));
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpAbsY(cpu, 0xf005u));
    OpSta(memory, cpu, OpDp(cpu, OBJECT_ROWS_REMAINING));
    OpStz(memory, cpu, OpDp(cpu, OBJECT_ROWS_REMAINING + 1u));
    OpDecA(cpu);
    OpOra(memory, cpu, OpDp(cpu, RESOURCE));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_OBJECT_GRAPHICS_SHAPE));
    OpWriteX(memory, cpu, OpDp(cpu, OBJECT_RECORD), cpu->y);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x83abf4u));
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
    OpLda(memory, cpu, OpLongX(cpu, 0x80f42au));
    OpSta(memory, cpu, OpDp(cpu, RESOURCE));
    OpSepWidths(cpu, 0x20u);
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, OBJECT_RECORD)));
    OpLda(memory, cpu, OpAbsY(cpu, 0xf002u));
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpAbsY(cpu, 0xf003u));
    result =
        CallGraphicsChild(memory, cpu, child, context, 0x80f105u, 0x83f9eeu, 3u, 1u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpLda(memory, cpu, OpAbsY(cpu, 0xf001u));
    OpTxy(cpu);
    OpLdx(cpu, 0u);
    OpBitValue(cpu, 1u);
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
    do {
        OpLda(memory, cpu, OpDp(cpu, OBJECT_WIDTH));
        OpSta(memory, cpu, OpDp(cpu, OBJECT_COLUMNS_REMAINING));
        do {
            if ((*metatiles_copied)++ == SCENE_METATILE_BUDGET)
                return ExecutionHandoff(cpu, 0x80f130u);
            OpLda(memory, cpu, OpLongX(cpu, 0x7f0000u));
            OpPushX(memory, cpu);
            OpAndValue(cpu, 0x3ffu);
            for (unsigned shift = 0; shift < 3u; ++shift)
                OpAslA(cpu);
            OpAdc(memory, cpu, WRAM_FIELD_METATILE_BASE);
            OpTax(cpu);
            OpLda(memory, cpu, OpDp(cpu, OBJECT_TILE_ATTRIBUTES));
            if (cpu->zero) {
                OpLda(memory, cpu, OpLongX(cpu, 0x7f0000u));
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
    UploadObjectGraphics(memory, cpu);
    OpPullY(memory, cpu);
    return ExecutionReturned(0x80f1e0u);
}

static void StartTilesetDma(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                            uint16_t vram, uint16_t length) {
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_DMAP(0)));
    OpLdx(cpu, 0x4000u);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_A1TL(0)), cpu->x);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_A1B(0)));
    OpLoadA(cpu, 0x18u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_BBAD(0)));
    OpLdx(cpu, vram);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_VMADDL), cpu->x);
    if (vram == 0x4000u)
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, RESOURCE_LENGTH)));
    else
        OpLdx(cpu, length);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_DASL(0)), cpu->x);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_MDMAEN));
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
        OpLda(memory, cpu, OpAbsY(cpu, 0xf000u));
        OpCmpValue(cpu, 0xffu);
        if (cpu->zero) {
            PullDataBank(memory, cpu);
            return ExecutionReturned(0x80f1eeu);
        }
        OpLda(memory, cpu, OpAbsY(cpu, 0xf007u));
        OpBitValue(cpu, 0x0cu);
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbsY(cpu, 0xf008u));
            OpBitValue(cpu, 0x08u);
        }
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
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0xcff90eu));
    if (!cpu->zero) {
        OpRepWidths(cpu, 0x20u);
        OpAslA(cpu);
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
        TransferDirectToA(cpu);
        OpLda(memory, cpu, OpLongX(cpu, 0xcff9feu));
        OpSta(memory, cpu, OpDp(cpu, AUXILIARY_RESOURCES));
        OpLda(memory, cpu, OpLongX(cpu, 0xcff9ffu));
        OpSta(memory, cpu, OpDp(cpu, AUXILIARY_RESOURCES + 1u));
        OpLda(memory, cpu, OpDp(cpu, AUXILIARY_RESOURCES));
        for (unsigned shift = 0; shift < 4u; ++shift)
            OpLsrA(cpu);
        if (!cpu->zero) {
            OpRepWidths(cpu, 0x20u);
            cpu->carry = 0u;
            OpAdcValue(cpu, 0x15fu);
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
    OpLda(memory, cpu, OpDp(cpu, AUXILIARY_RESOURCES + 1u));
    for (unsigned shift = 0; shift < 4u; ++shift)
        OpLsrA(cpu);
    if (!cpu->zero) {
        OpRepWidths(cpu, 0x20u);
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x164u);
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
    return ExecutionReturned(0x80f2b6u);
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
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID)));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_FLAGS));
    OpBitValue(cpu, 1u);
    if (!cpu->zero)
        OpLda(memory, cpu, WRAM_CAVE_TILESET_RESOURCE);
    else {
        OpLda(memory, cpu, OpLongX(cpu, 0xcff81eu));
        OpAndValue(cpu, 0xffu);
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x14bu);
    }
    OpSta(memory, cpu, OpDp(cpu, RESOURCE));
    OpSepWidths(cpu, 0x20u);
    if (!cpu->zero) {
        result = LoadTilesetAndObjects(memory, cpu, child, context);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
        result = LoadAuxiliaryGraphics(memory, cpu, child, context);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
    }
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
        OpLda(memory, cpu, OpLongX(cpu, 0xcffa08u));
        OpRepWidths(cpu, 0x20u);
        OpDecA(cpu);
        OpAslA(cpu);
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, 0xcffafau));
    }
    OpSta(memory, cpu, WRAM_FIELD_PALETTE_SOURCE);
    OpSepWidths(cpu, 0x20u);
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
