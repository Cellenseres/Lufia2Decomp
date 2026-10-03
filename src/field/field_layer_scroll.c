/* Field layer scroll setup ($83:8D42). */

#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    SCROLL_CELL_X = 0x56,
    SCROLL_CELL_Y = 0x58,
};

typedef Lufia2ExecutionResult (*ScrollChild)(
    const Lufia2Memory *, Lufia2CpuState *);

static uint8_t CallScrollChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t frame,
    ScrollChild child, Lufia2ExecutionResult *result) {
    uint8_t low, high;
    uint16_t actual;
    SimulateJsrFrame(memory, cpu, frame);
    *result = child(memory, cpu);
    if (result->flow != LUFIA2_EXECUTION_RETURNED)
        return 0;
    low = Pull8(memory, cpu);
    high = Pull8(memory, cpu);
    actual = (uint16_t)(low | ((uint16_t)high << 8));
    if (actual != frame) {
        *result = ExecutionHandoff(cpu, 0x830000u | (uint16_t)(actual + 1u));
        return 0;
    }
    return 1;
}

static uint8_t SetScaledLayerScroll(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint8_t right,
    Lufia2ExecutionResult *result) {
    static const uint16_t frames[2][4] = {
        {0x8da1u, 0x8da4u, 0x8da7u, 0x8daau},
        {0x8d91u, 0x8d94u, 0x8d97u, 0x8d9au},
    };
    ScrollChild scale = right ? Lufia2FieldScaleCoordinateRight
                              : Lufia2FieldScaleCoordinateLeft;
    if (!CallScrollChild(memory, cpu, frames[right][0],
                         Lufia2FieldPrepareLayerScrollX, result))
        return 0;
    if (!CallScrollChild(memory, cpu, frames[right][1], scale, result))
        return 0;
    if (!CallScrollChild(memory, cpu, frames[right][2],
                         Lufia2FieldPrepareLayerScrollY, result))
        return 0;
    if (!CallScrollChild(memory, cpu, frames[right][3], scale, result))
        return 0;
    cpu->carry = 0;
    OpAdcValue(cpu, 0x0080u);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_FIELD_LAYER_SCROLL_Y & 0xffffu));
    OpSepWidths(cpu, 0x20u);
    return 1;
}

static uint8_t ZeroLayerScroll(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpWriteM(memory, cpu, OpAbsX(cpu, WRAM_FIELD_LAYER_SCROLL_X & 0xffffu), 0u);
    OpWriteM(memory, cpu, OpAbsX(cpu, WRAM_FIELD_LAYER_SCROLL_Y & 0xffffu), 0u);
    OpSepWidths(cpu, 0x20u);
    return 1;
}

static uint8_t SetLayerScroll(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2ExecutionResult *result) {
    OpCmpValue(cpu, 0x01u);
    if (cpu->zero)
        return ZeroLayerScroll(memory, cpu);
    OpCmpValue(cpu, 0x02u);
    if (cpu->zero)
        return SetScaledLayerScroll(memory, cpu, 1u, result);
    OpCmpValue(cpu, 0x03u);
    if (cpu->zero)
        return ZeroLayerScroll(memory, cpu);
    OpCmpValue(cpu, 0x04u);
    if (cpu->zero)
        return SetScaledLayerScroll(memory, cpu, 0u, result);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, SCROLL_CELL_X));
    OpAslA(cpu);
    OpAslA(cpu);
    OpAslA(cpu);
    OpAslA(cpu);
    cpu->carry = 1;
    OpSbcValue(cpu, 0x0080u);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_FIELD_LAYER_SCROLL_X & 0xffffu));
    OpLda(memory, cpu, OpDp(cpu, SCROLL_CELL_Y));
    OpAslA(cpu);
    OpAslA(cpu);
    OpAslA(cpu);
    OpAslA(cpu);
    cpu->carry = 1;
    OpSbcValue(cpu, 0x0070u);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_FIELD_LAYER_SCROLL_Y & 0xffffu));
    OpSepWidths(cpu, 0x20u);
    return 1;
}

static void CacheLayerScroll(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_FIELD_LAYER_SCROLL_X & 0xffffu));
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_LAYER_COARSE_SCROLL_CACHE));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_FIELD_LAYER_SCROLL_Y & 0xffffu));
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpLsrA(cpu);
    /* The Y word overlaps the next layer's X word. */
    OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_LAYER_COARSE_SCROLL_CACHE + 2u));
    OpSepWidths(cpu, 0x20u);
}

Lufia2ExecutionResult Lufia2FieldSetupLayerScroll(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    OpLdx(cpu, 0x0006u);
    do {
        OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_LAYER_SECTION_WORD));
        if (!cpu->negative && !SetLayerScroll(memory, cpu, &result))
            return result;
        CacheLayerScroll(memory, cpu);
        OpDex(cpu);
        OpDex(cpu);
    } while (!cpu->negative);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x838dd9u);
}
