#include "lufia2/field.h"
#include "core/cpu_ops.h"
#include "core/child_call.h"
#include "system/wram.h"
#include "system/system_internal.h"

enum {
    DP_PALETTE_SCALE_GREEN = 0x54,
    DP_PALETTE_SCALE_SOURCE_LOW = 0x56,
    DP_PALETTE_SCALE_SOURCE_HIGH = 0x57,
    DP_PALETTE_SCALE_RESULT_LOW = 0x58,
    DP_PALETTE_SCALE_RESULT_HIGH = 0x59
};

static void ScalePaletteChannel(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSta(memory, cpu, OpAbs(cpu, 0x211cu));
    AslAbsolute8(memory, cpu, 0x2134u);
    OpLda(memory, cpu, OpAbs(cpu, 0x2135u));
    OpAdcValue(cpu, 0u);
}

static void UnpackPaletteGreen(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    for (unsigned shift = 0; shift < 2u; ++shift) {
        OpLsrA(cpu);
        const uint32_t address = OpDp(cpu, DP_PALETTE_SCALE_SOURCE_LOW);
        const uint8_t old = Read8(memory, address);
        const uint8_t value = (uint8_t)((old >> 1) | (cpu->carry ? 0x80u : 0u));
        cpu->carry = old & 1u;
        Write8(memory, address, value);
        SetNz8(cpu, value);
    }
}

static void ScalePaletteColor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpLongX(cpu, 0x9b0000u));
    OpSta(memory, cpu, OpDp(cpu, DP_PALETTE_SCALE_SOURCE_LOW));
    OpAndValue(cpu, 0x1fu);
    OpAslA(cpu);
    ScalePaletteChannel(memory, cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_PALETTE_SCALE_RESULT_LOW));
    OpLda(memory, cpu, OpLongX(cpu, 0x9b0001u));
    OpSta(memory, cpu, OpDp(cpu, DP_PALETTE_SCALE_SOURCE_HIGH));
    UnpackPaletteGreen(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, DP_PALETTE_SCALE_SOURCE_LOW));
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpAndValue(cpu, 0x3eu);
    ScalePaletteChannel(memory, cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_PALETTE_SCALE_GREEN));
    OpLda(memory, cpu, OpDp(cpu, DP_PALETTE_SCALE_SOURCE_HIGH));
    OpAndValue(cpu, 0x7cu);
    OpLsrA(cpu);
    ScalePaletteChannel(memory, cpu);
    OpAndValue(cpu, 0x1fu);
    OpAslA(cpu);
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_PALETTE_SCALE_RESULT_HIGH));
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpDp(cpu, DP_PALETTE_SCALE_GREEN));
    ExchangeAccumulatorBytes(cpu);
    OpRepWidths(cpu, 0x20u);
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpOra(memory, cpu, OpDp(cpu, DP_PALETTE_SCALE_RESULT_LOW));
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_CGRAM_BUFFER));
    OpSepWidths(cpu, 0x20u);
}

Lufia2ExecutionResult Lufia2FieldScaleScenePalette(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit || !child)
        return ExecutionHandoff(cpu, 0x848d54u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, WRAM_FIELD_PALETTE_SOURCE);
    OpTax(cpu);
    OpLda(memory, cpu, WRAM_FIELD_PALETTE_SKIP_BYTES);
    OpTay(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_RECOVERY_PALETTE_LENGTH));
    cpu->carry = 0;
    OpAdc(memory, cpu, OpAbs(cpu, WRAM_FIELD_PALETTE_SCALE_PHASE));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_PALETTE_SCALE_PHASE));
    OpSta(memory, cpu, OpDp(cpu, DP_PALETTE_SCALE_GREEN));
    OpSta(memory, cpu, OpAbs(cpu, 0x211bu));
    OpStz(memory, cpu, OpAbs(cpu, 0x211bu));
    OpCmp(memory, cpu, OpAbs(cpu, WRAM_FIELD_PALETTE_SCALE_TARGET));
    if (cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_PALETTE_SCALE_TARGET));
        OpCmpValue(cpu, 0x80u);
        const uint8_t restore_palette = cpu->zero;
        OpLda(memory, cpu, 0x0009a9u);
        OpAndValue(cpu, 0xfbu);
        OpSta(memory, cpu, 0x0009a9u);
        if (restore_palette) {
            OpStz(memory, cpu, OpAbs(cpu, WRAM_FIELD_RECOVERY_PALETTE_LENGTH));
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x848d8du, 0x80f338u, 3u, 0x84u)) {
                Lufia2ExecutionResult result = ExecutionReturned(0x848d8du);
                result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
                return result;
            }
            OpLoadA(cpu, 1u);
            OpTestBits(memory, cpu, OpDp(cpu, DP_NMI_UPLOAD_FLAGS), 1u);
            return ExecutionReturned(0x848e06u);
        }
    }
    do {
        ScalePaletteColor(memory, cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, 0x00e0u);
    } while (!cpu->carry);
    OpLoadA(cpu, 1u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_NMI_UPLOAD_FLAGS), 1u);
    return ExecutionReturned(0x848e06u);
}
