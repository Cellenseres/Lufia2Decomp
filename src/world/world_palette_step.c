#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/world_map.h"

enum {
    DP_COLOR_TARGET = 0x00u,
    DP_COLOR_CHANNELS = 0x04u,
    PALETTE_FIRST_COLOR = WRAM_CGRAM_BUFFER + 0x20u,
    PALETTE_END = WRAM_CGRAM_BUFFER + 0x100u,
    PALETTE_SOURCE_START = 0x20u,
    WORLD_PALETTE_TRANSFER_MODE = WRAM_WORLD_MAP_PALETTE_TRANSFER_MODE & 0xffffu,
    WORLD_PALETTE_TRANSFER_LENGTH = WRAM_WORLD_MAP_PALETTE_TRANSFER_LENGTH & 0xffffu,
    WORLD_PALETTE_TRANSFER_SOURCE = WRAM_WORLD_MAP_PALETTE_TRANSFER_SOURCE & 0xffffu,
    WORLD_PALETTE_TRANSFER_BANK = WRAM_WORLD_MAP_PALETTE_TRANSFER_BANK & 0xffffu
};

static uint8_t PaletteEntry(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x86u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal;
}

Lufia2ExecutionResult Lufia2WorldMapQueuePaletteTransfer(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!PaletteEntry(cpu))
        return ExecutionHandoff(cpu, 0x86a0a2u);
    OpLdx(cpu, WRAM_CGRAM_BUFFER);
    OpWriteX(memory, cpu, OpAbs(cpu, WORLD_PALETTE_TRANSFER_SOURCE), cpu->x);
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbs(cpu, WORLD_PALETTE_TRANSFER_BANK));
    OpLoadA(cpu, 0x10u);
    OpSta(memory, cpu, OpAbs(cpu, WORLD_PALETTE_TRANSFER_MODE));
    OpLdx(cpu, 0xe0u);
    OpWriteX(memory, cpu, OpAbs(cpu, WORLD_PALETTE_TRANSFER_LENGTH), cpu->x);
    return ExecutionReturned(0x86a0b8u);
}

static uint32_t PaletteColorAddress(const Lufia2CpuState *cpu, uint8_t byte) {
    return (uint16_t)(cpu->direct_page + cpu->x + byte) | OP_DP_WRAP;
}

static void StepRed(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbsY(cpu, PALETTE_SOURCE_START));
    OpAndValue(cpu, 0x1fu);
    OpSta(memory, cpu, OpDp(cpu, DP_COLOR_TARGET));
    OpLda(memory, cpu, PaletteColorAddress(cpu, 0u));
    OpAndValue(cpu, 0x1fu);
    OpCmpValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_COLOR_TARGET)));
    if (!cpu->zero) {
        if (cpu->carry)
            OpDecA(cpu);
        else
            OpIncA(cpu);
    }
    OpSta(memory, cpu, OpDp(cpu, DP_COLOR_CHANNELS));
}

static void StepBlue(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbsY(cpu, PALETTE_SOURCE_START + 1u));
    OpAndValue(cpu, 0x7cu);
    OpSta(memory, cpu, OpDp(cpu, DP_COLOR_TARGET));
    OpLda(memory, cpu, PaletteColorAddress(cpu, 1u));
    OpAndValue(cpu, 0x7cu);
    OpCmpValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_COLOR_TARGET)));
    if (!cpu->zero) {
        if (cpu->carry)
            OpSbcValue(cpu, 4u);
        else
            OpAdcValue(cpu, 4u);
    }
    OpSta(memory, cpu, OpDp(cpu, DP_COLOR_CHANNELS + 1u));
}

static void StepGreen(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsY(cpu, PALETTE_SOURCE_START));
    OpAndValue(cpu, 0x03e0u);
    OpSta(memory, cpu, OpDp(cpu, DP_COLOR_TARGET));
    OpLda(memory, cpu, PaletteColorAddress(cpu, 0u));
    OpAndValue(cpu, 0x03e0u);
    OpCmpValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_COLOR_TARGET)));
    if (!cpu->zero) {
        if (cpu->carry)
            OpSbcValue(cpu, 0x20u);
        else
            OpAdcValue(cpu, 0x20u);
    }
    OpOraValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_COLOR_CHANNELS)));
    OpSta(memory, cpu, PaletteColorAddress(cpu, 0u));
}

Lufia2ExecutionResult Lufia2WorldMapStepPaletteColors(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    if (!PaletteEntry(cpu) || cpu->stack < 0x1f00u ||
        cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x86a03bu);
    Push8(memory, cpu, cpu->data_bank);
    PushAccumulator8(memory, cpu);
    cpu->data_bank = Pull8(memory, cpu);
    SetNz8(cpu, cpu->data_bank);
    OpLdx(cpu, PALETTE_FIRST_COLOR);
    do {
        StepRed(memory, cpu);
        StepBlue(memory, cpu);
        StepGreen(memory, cpu);
        OpSepWidths(cpu, 0x20u);
        OpIny(cpu);
        OpIny(cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, PALETTE_END);
    } while (!cpu->carry);
    cpu->data_bank = Pull8(memory, cpu);
    SetNz8(cpu, cpu->data_bank);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x86a09eu, 0x86a0a2u, 2u, 0x86u)) {
        Lufia2ExecutionResult result = ExecutionReturned(0x86a09eu);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    return ExecutionReturned(0x86a0a1u);
}
