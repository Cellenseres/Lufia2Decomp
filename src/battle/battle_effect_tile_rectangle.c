#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum {
    DP_RECTANGLE_SIZE = 0u,
    DP_COLUMNS_LEFT = 2u,
    DP_ROW_START = 4u,
    DP_TILE_SOURCE = 8u,
    DP_TILE_DESTINATION = 11u,
    DP_TILE_STREAM = 0xc3u,
    TILE_ROW_BYTES = 64u,
    TILE_WORK_BANK = 0x95u,
    TILE_COPY_BANK = 0x7eu
};

static void AdvanceTileStream(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpStepMem(memory, cpu, OpDp(cpu, DP_TILE_STREAM), 1);
    OpStepMem(memory, cpu, OpDp(cpu, DP_TILE_STREAM), 1);
}

static void ReadTileAddress(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint32_t base, uint8_t pointer) {
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_TILE_STREAM));
    AdvanceTileStream(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
    LoadA8(cpu, 0u);
    ExchangeAccumulatorBytes(cpu);
    OpRepWidths(cpu, 0x20u);
    OpAslA(cpu);
    OpAdc(memory, cpu, OpAbs(cpu, SNES_RDMPYL));
    OpAdcValue(cpu, (uint16_t)base);
    OpSta(memory, cpu, OpDp(cpu, pointer));
}

static void CopyTileByte(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const uint16_t source = Read16Direct(memory, cpu, DP_TILE_SOURCE);
    OpLda(memory, cpu, AbsoluteIndexedAddress(cpu, source, cpu->y));
    const uint16_t destination = Read16Direct(memory, cpu, DP_TILE_DESTINATION);
    OpSta(memory, cpu, AbsoluteIndexedAddress(cpu, destination, cpu->y));
    OpIny(cpu);
}

static void CopyTileRectangle(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLdy(cpu, 0u);
    do {
        OpLda(memory, cpu, OpDp(cpu, DP_RECTANGLE_SIZE));
        OpSta(memory, cpu, OpDp(cpu, DP_COLUMNS_LEFT));
        OpWriteX(memory, cpu, OpDp(cpu, DP_ROW_START), cpu->y);
        do {
            CopyTileByte(memory, cpu);
            CopyTileByte(memory, cpu);
            OpStepMem(memory, cpu, OpDp(cpu, DP_COLUMNS_LEFT), -1);
        } while (!cpu->zero);
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpDp(cpu, DP_ROW_START));
        cpu->carry = 0u;
        OpAdcValue(cpu, TILE_ROW_BYTES);
        OpTay(cpu);
        OpSepWidths(cpu, 0x20u);
        OpStepMem(memory, cpu, OpDp(cpu, DP_RECTANGLE_SIZE + 1u), -1);
    } while (!cpu->zero);
}

Lufia2ExecutionResult Lufia2BattleEffectCopyTileRectangle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x8197d8u);
    OpSetDataBank(memory, cpu, TILE_WORK_BANK);
    LoadA8(cpu, TILE_ROW_BYTES);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
    OpRepWidths(cpu, 0x20u);
    ReadTileAddress(memory, cpu, WRAM_FIELD_LAYER2_TILEMAP, DP_TILE_DESTINATION);
    ReadTileAddress(memory, cpu, WRAM_SAVE_FILE_BUFFER, DP_TILE_SOURCE);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_TILE_STREAM));
    AdvanceTileStream(memory, cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_RECTANGLE_SIZE));
    OpSepWidths(cpu, 0x20u);
    OpSetDataBank(memory, cpu, TILE_COPY_BANK);
    PushY(memory, cpu);
    CopyTileRectangle(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);
    LoadA8(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_EFFECT_BG3_REQUEST));
    return ExecutionReturned(0x819852u);
}
