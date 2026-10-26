#include "menu/card_table_services.h"
#include "lufia2/card_table.h"
#include "system/wram.h"

enum {
    GRAPHICS_RESOURCE = 0x54u,
    GRAPHICS_DESTINATION = 0x60u,
    GRAPHICS_DESTINATION_BANK = 0x62u,
    SYMBOL_COUNT = 0u,
    SYMBOL_OPTION = 0x30u,
    SYMBOL_TILES = WRAM_CARD_GAME_TILE_BUFFER & 0xffffu,
    SYMBOL_FIRST_LAYER = WRAM_MUSIC_SAMPLE_CACHE & 0xffffu,
    SYMBOL_SECOND_LAYER = WRAM_FIELD_LAYER0_TILEMAP & 0xffffu
};

static void CopyTableLayer(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                            uint16_t source, uint16_t destination) {
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0u);
    OpLdy(cpu, 0x0400u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, source));
        OpSta(memory, cpu, OpAbsX(cpu, destination));
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
}

Lufia2ExecutionResult Lufia2CardTableCopyPanelFirstLayer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!TableServiceReady(cpu, false) || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x84ef60u);
    CopyTableLayer(memory, cpu, 0xa000u, SYMBOL_FIRST_LAYER);
    return ExecutionReturned(0x84ef73u);
}

Lufia2ExecutionResult Lufia2CardTableCopyPanelSecondLayer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!TableServiceReady(cpu, false) || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x84ef74u);
    CopyTableLayer(memory, cpu, 0xa000u, SYMBOL_SECOND_LAYER);
    return ExecutionReturned(0x84ef87u);
}

static Lufia2ExecutionResult LoadTableLayers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, bool alternate) {
    const uint32_t entry = alternate ? 0x84f7d3u : 0x84edfau;
    if (!child || !TableServiceReady(cpu, false) || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, entry);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(alternate ? 0x84f823u : 0x84ee4au), false};
    Push8(memory, cpu, PackStatus(cpu));
    for (unsigned layer = 0u; layer < 2u; ++layer) {
        OpSepWidths(cpu, 0x20u);
        OpLdx(cpu, alternate ? (layer ? 0x02a6u : 0x02a7u) :
            (layer ? 0x02a2u : 0x02a1u));
        OpWriteX(memory, cpu, OpDp(cpu, GRAPHICS_RESOURCE), cpu->x);
        OpLdx(cpu, layer ? 0x4000u : 0xa000u);
        OpWriteX(memory, cpu, OpDp(cpu, GRAPHICS_DESTINATION), cpu->x);
        OpLoadA(cpu, 0x7eu);
        OpSta(memory, cpu, OpDp(cpu, GRAPHICS_DESTINATION_BANK));
        const uint32_t site = alternate ? (layer ? 0x84f80bu : 0x84f7e4u) :
            (layer ? 0x84ee32u : 0x84ee0bu);
        if (!TableServiceCall(&service, site, 0x808e9du, true))
            return service.result;
        CopyTableLayer(memory, cpu, layer ? 0x4000u : 0xa000u,
            layer ? SYMBOL_SECOND_LAYER : SYMBOL_FIRST_LAYER);
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
    return service.result;
}

Lufia2ExecutionResult Lufia2CardTableLoadGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return LoadTableLayers(memory, cpu, child, context, false);
}

Lufia2ExecutionResult Lufia2CardTableLoadAlternateGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return LoadTableLayers(memory, cpu, child, context, true);
}

Lufia2ExecutionResult Lufia2CardTableLoadAlternateTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!TableServiceReady(cpu, false) || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x84f78fu);
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0u);
    OpTxy(cpu);
    OpLda(memory, cpu, OpDp(cpu, SYMBOL_OPTION));
    OpAndValue(cpu, 1u);
    if (cpu->zero) OpLdx(cpu, 0x57u);
    OpLoadA(cpu, 0x57u);
    OpSta(memory, cpu, OpDp(cpu, SYMBOL_COUNT));
    do {
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpLongX(cpu, 0x84f824u));
        OpRepWidths(cpu, 0x20u);
        OpAndValue(cpu, 0xffu);
        OpSta(memory, cpu, OpAbsY(cpu, SYMBOL_TILES));
        OpInx(cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, SYMBOL_COUNT), -1);
    } while (!cpu->zero);
    OpLdx(cpu, 0u);
    OpLoadA(cpu, 0x28u);
    OpSta(memory, cpu, OpDp(cpu, SYMBOL_COUNT));
    do {
        OpLda(memory, cpu, OpLongX(cpu, 0x84f8d2u));
        OpSta(memory, cpu, OpAbsY(cpu, SYMBOL_TILES));
        OpInx(cpu);
        OpInx(cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, SYMBOL_COUNT), -1);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x84f7d2u);
}
