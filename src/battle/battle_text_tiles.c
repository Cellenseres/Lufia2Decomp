#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "lufia2/battle.h"

enum {
    TEXT_SPECIAL_GLYPH_BASE = 0xcdu,
    TEXT_MASK_LEFT = 0xcdu,
    TEXT_MASK_RIGHT = 0xceu,
    TEXT_GLYPH_CODES = 0x97b2c9u,
    TEXT_GLYPH_PLANES = 0x8500u,
    TEXT_BACKGROUND_TILE = 0x7ea000u,
    TEXT_GLYPH_PLANE_BYTES = 16u,
    TEXT_TILE_ROWS = 8u
};

static void WriteTileByte(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSta(memory, cpu, OpAbs(cpu, SNES_WMDATA));
}

Lufia2ExecutionResult Lufia2BattleMergeGlyphTile(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x81u || cpu->decimal)
        return ExecutionHandoff(cpu, 0x81e8eeu);
    PushY(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    for (unsigned shift = 0u; shift < 4u; ++shift)
        OpAslA(cpu);
    OpTay(cpu);
    OpSepWidths(cpu, 0x20u);
    for (unsigned byte = 0u; byte < TEXT_GLYPH_PLANE_BYTES; ++byte) {
        OpLda(memory, cpu, OpAbsY(cpu, (uint16_t)(TEXT_GLYPH_PLANES + byte)));
        OpOraValue(cpu, Read8(memory, OpLongX(cpu, TEXT_BACKGROUND_TILE + byte)));
        WriteTileByte(memory, cpu);
    }
    for (unsigned row = 0u; row < TEXT_TILE_ROWS; ++row) {
        const unsigned byte = TEXT_GLYPH_PLANE_BYTES + row * 2u;
        OpLda(memory, cpu, OpLongX(cpu, TEXT_BACKGROUND_TILE + byte));
        WriteTileByte(memory, cpu);
        OpLda(memory, cpu,
            OpAbsY(cpu, (uint16_t)(TEXT_GLYPH_PLANES + row * 2u)));
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
        OpAndValue(cpu, Read8(memory,
            OpLongX(cpu, TEXT_BACKGROUND_TILE + byte + 1u)));
        WriteTileByte(memory, cpu);
    }
    cpu->y = PullIndexValue(memory, cpu);
    return ExecutionReturned(0x81ea34u);
}
