#include "core/cpu_ops.h"
#include "system/wram.h"
#include "core/snes_registers.h"
#include "lufia2/battle.h"

enum {
    DP_REQUESTED_HDMA_CHANNELS = 0xdau,
    BOTTOM_LAYER_MASKS = WRAM_FIELD_STREAMED_COLUMN_VRAM + 6u,
    BOTTOM_COLOR_CONTROL = WRAM_BATTLE_HDMA_BOTTOM_COLOR_CONTROL,
    LAYER_DMA_DESCRIPTOR = WRAM_BATTLE_LAYER_HDMA_DESCRIPTOR,
    COLOR_DMA_DESCRIPTOR = WRAM_BATTLE_COLOR_HDMA_DESCRIPTOR,
    LAYER_DMA_TABLE = 0xa09eu,
    COLOR_DMA_TABLE = 0xa0aeu,
    LAYER_REGISTER = 0x2cu,
    COLOR_REGISTER = 0x30u,
    LAYER_CHANNEL = 5u,
    COLOR_CHANNEL = 2u,
    INDIRECT_REGISTER_PAIR = 0x41u,
    EFFECT_TABLE_BANK = 0x85u,
    LAYER_COLOR_CHANNEL_MASK = 0x24u
};

static void ConfigureRegisterPairDma(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t descriptor, uint8_t registers,
    uint16_t table, unsigned channel) {
    LoadA8(cpu, INDIRECT_REGISTER_PAIR);
    OpSta(memory, cpu, OpAbs(cpu, descriptor));
    LoadA8(cpu, registers);
    OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(descriptor + 1u)));
    OpLdx(cpu, table);
    OpWriteX(memory, cpu, OpAbs(cpu, (uint16_t)(descriptor + 2u)), cpu->x);
    LoadA8(cpu, EFFECT_TABLE_BANK);
    OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(descriptor + 4u)));
    LoadA8(cpu, EFFECT_TABLE_BANK);
    OpSta(memory, cpu, SNES_DASB(channel));
}

Lufia2ExecutionResult Lufia2BattleConfigureLayerColorDma(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x85ab98u);
    OpLdx(cpu, 0x0303u);
    OpWriteX(memory, cpu, OpAbs(cpu, BOTTOM_LAYER_MASKS), cpu->x);
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpAbs(cpu, BOTTOM_COLOR_CONTROL), cpu->x);
    ConfigureRegisterPairDma(memory, cpu, LAYER_DMA_DESCRIPTOR,
        LAYER_REGISTER, LAYER_DMA_TABLE, LAYER_CHANNEL);
    ConfigureRegisterPairDma(memory, cpu, COLOR_DMA_DESCRIPTOR,
        COLOR_REGISTER, COLOR_DMA_TABLE, COLOR_CHANNEL);
    LoadA8(cpu, LAYER_COLOR_CHANNEL_MASK);
    OpTestBits(memory, cpu, OpDp(cpu, DP_REQUESTED_HDMA_CHANNELS), 1u);
    return ExecutionReturned(0x85abdeu);
}
