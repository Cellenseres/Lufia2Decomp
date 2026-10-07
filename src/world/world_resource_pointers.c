#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "lufia2/world_map.h"
#include "system/wram.h"

enum {
    RESOURCE_ID = WRAM_WORLD_MAP_RESOURCE_ID,
    RESOURCE_TABLE_OFFSET = WRAM_WORLD_MAP_RESOURCE_TABLE_OFFSET,
    RESOURCE_ROW_BYTES = 18u,
    DP_METATILE_SOURCE_POINTER = 0xdfu,
    DP_METATILE_SOURCE_BANK = 0xe1u,
    DP_METATILE_NEXT_POINTER = 0xe3u,
    DP_METATILE_NEXT_HIGH = 0xe4u,
    DP_METATILE_NEXT_BANK = 0xe5u,
    DP_METATILE_FIRST_WORD = 0xe7u,
    DP_METATILE_SECOND_WORD = 0xeau,
    DP_METATILE_SECOND_HIGH = 0xedu,
    DP_METATILE_FIRST_BANK = 0xe9u,
    DP_METATILE_SECOND_BANK = 0xecu,
    DP_METATILE_HIGH_BANK = 0xefu,
    ROM_RESOURCE_SOURCES = 0xce3bu,
    ROM_RESOURCE_SOURCE_BANKS = 0xce3du,
    ROM_RESOURCE_TILE_WORDS = 0xce3eu,
    ROM_RESOURCE_TILE_BANKS = 0xce40u
};

Lufia2ExecutionResult Lufia2WorldMapBindResourcePointers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x86u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x86cdf5u);
    OpLda(memory, cpu, OpAbs(cpu, RESOURCE_ID));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
    LoadA8(cpu, RESOURCE_ROW_BYTES);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, SNES_RDMPYL, 0u));
    Write16Absolute(memory, cpu, RESOURCE_TABLE_OFFSET, cpu->x);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, ROM_RESOURCE_SOURCES, cpu->x));
    Write16Direct(memory, cpu, DP_METATILE_SOURCE_POINTER, cpu->y);
    Write16Direct(memory, cpu, DP_METATILE_NEXT_POINTER, cpu->y);
    OpLda(memory, cpu, OpAbsX(cpu, ROM_RESOURCE_SOURCE_BANKS));
    OpSta(memory, cpu, OpDp(cpu, DP_METATILE_SOURCE_BANK));
    OpIncA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_METATILE_NEXT_BANK));
    LoadA8(cpu, 0x80u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_METATILE_NEXT_HIGH), 0u);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, ROM_RESOURCE_TILE_WORDS, cpu->x));
    Write16Direct(memory, cpu, DP_METATILE_FIRST_WORD, cpu->y);
    IncrementY16(cpu);
    IncrementY16(cpu);
    Write16Direct(memory, cpu, DP_METATILE_SECOND_WORD, cpu->y);
    IncrementY16(cpu);
    Write16Direct(memory, cpu, DP_METATILE_SECOND_HIGH, cpu->y);
    OpLda(memory, cpu, OpAbsX(cpu, ROM_RESOURCE_TILE_BANKS));
    OpSta(memory, cpu, OpDp(cpu, DP_METATILE_FIRST_BANK));
    OpSta(memory, cpu, OpDp(cpu, DP_METATILE_SECOND_BANK));
    OpSta(memory, cpu, OpDp(cpu, DP_METATILE_HIGH_BANK));
    return ExecutionReturned(0x86ce31u);
}
