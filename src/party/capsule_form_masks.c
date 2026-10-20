#include "party/capsule_feed_internal.h"

enum {
    FORM_MASK_BASE = (WRAM_UNK_7E93C0 & 0xffffu) + 0x930u,
    FORM_MASK_SEQUENCE = (WRAM_UNK_7E93C0 & 0xffffu) + 0x960u,
    FORM_MASK_ROWS = 8u,
    FORM_MASK_COLUMNS = 6u,
    FORM_MASK_FRAMES = 64u,
    FORM_MASK_BYTES = 48u,
    FORM_MASK_SEQUENCE_BYTES = 0x0c00u,
    ROM_FORM_MASK_BITS = 0x8ed8bbu,
    DP_FORM_INVERSE_MASK = 0u,
    DP_FORM_MASK_ROW = 2u,
    DP_FORM_MASK_BITS_LEFT = 4u,
    DP_FORM_TILE_OFFSET = 6u,
    DP_FORM_MASK_SOURCE = 8u,
    DP_FORM_MASK_NEXT = 0x0bu,
    DP_FORM_PLANE_MASK = 0x11u,
    DP_FORM_ROWS_LEFT = 0x17u,
    DP_FORM_MERGED_MASK = 0x19u,
    DP_FORM_NEW_TILES = 0x1cu
};

static bool FormMaskReady(const Lufia2CpuState *cpu, bool byte_accumulator) {
    return cpu->program_bank == 0x82u &&
        cpu->accumulator_is_8_bit == byte_accumulator &&
        !cpu->index_is_8_bit && !cpu->decimal && cpu->direct_page == 0u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static uint32_t FormTileAddress(
    const Lufia2Memory *memory, const Lufia2CpuState *cpu, uint8_t pointer) {
    return AbsoluteIndexedAddress(
        cpu, Read16Direct(memory, cpu, pointer), cpu->y);
}

static void ApplyFormPlaneMask(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, FormTileAddress(memory, cpu, DP_FORM_MASK_NEXT));
    OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_FORM_INVERSE_MASK)));
    OpSta(memory, cpu, FormTileAddress(memory, cpu, DP_FORM_MASK_NEXT));
    OpLda(memory, cpu, FormTileAddress(memory, cpu, DP_FORM_NEW_TILES));
    OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_FORM_PLANE_MASK)));
    OpOra(memory, cpu, FormTileAddress(memory, cpu, DP_FORM_MERGED_MASK));
    OpSta(memory, cpu, FormTileAddress(memory, cpu, DP_FORM_MERGED_MASK));
}

Lufia2ExecutionResult Lufia2CapsuleApplyFormMask(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!FormMaskReady(cpu, false))
        return ExecutionHandoff(cpu, 0x82ca18u);
    OpLoadA(cpu, FORM_MASK_ROWS);
    OpSta(memory, cpu, OpDp(cpu, DP_FORM_ROWS_LEFT));
    do {
        OpSepWidths(cpu, 0x20u);
        OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_FORM_MASK_ROW)));
        OpLda(memory, cpu, FormTileAddress(memory, cpu, DP_FORM_MASK_SOURCE));
        if (!cpu->zero) {
            OpSta(memory, cpu, OpDp(cpu, DP_FORM_PLANE_MASK));
            OpSta(memory, cpu, OpDp(cpu, DP_FORM_PLANE_MASK + 1u));
            OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
            OpSta(memory, cpu, OpDp(cpu, DP_FORM_INVERSE_MASK));
            OpSta(memory, cpu, OpDp(cpu, DP_FORM_INVERSE_MASK + 1u));
            OpRepWidths(cpu, 0x20u);
            OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_FORM_TILE_OFFSET)));
            ApplyFormPlaneMask(memory, cpu);
            OpTya(cpu);
            cpu->carry = 0u;
            OpAdcValue(cpu, 0x10u);
            OpTay(cpu);
            ApplyFormPlaneMask(memory, cpu);
        }
        OpRepWidths(cpu, 0x20u);
        OpStepMem(memory, cpu, OpDp(cpu, DP_FORM_TILE_OFFSET), 1);
        OpStepMem(memory, cpu, OpDp(cpu, DP_FORM_TILE_OFFSET), 1);
        OpStepMem(memory, cpu, OpDp(cpu, DP_FORM_MASK_ROW), 1);
        OpStepMem(memory, cpu, OpDp(cpu, DP_FORM_ROWS_LEFT), -1);
    } while (!cpu->zero);
    return ExecutionReturned(0x82ca61u);
}

Lufia2ExecutionResult Lufia2CapsuleChooseFormMaskBit(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!FormMaskReady(cpu, true) || !child)
        return ExecutionHandoff(cpu, 0x82cab1u);
    CapsuleFeedCall call = {memory, cpu, child, context, {0}};
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpDp(cpu, DP_FORM_MASK_BITS_LEFT));
    do {
        OpRepWidths(cpu, 0x20u);
        OpLoadA(cpu, FORM_MASK_ROWS);
        if (!CapsuleFeedChild(&call, 0x82cabau, 0x808299u, 3u))
            return call.result;
        OpAndValue(cpu, 0xffu);
        OpTay(cpu);
        OpLoadA(cpu, FORM_MASK_ROWS);
        if (!CapsuleFeedChild(&call, 0x82cac5u, 0x808299u, 3u))
            return call.result;
        OpAndValue(cpu, 0xffu);
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpLongX(cpu, ROM_FORM_MASK_BITS));
        OpSta(memory, cpu, OpDp(cpu, DP_FORM_INVERSE_MASK));
        OpLda(memory, cpu,
              DirectLongIndirectY(memory, cpu, DP_FORM_MASK_SOURCE));
        OpBit(memory, cpu, OpDp(cpu, DP_FORM_INVERSE_MASK));
        if (!cpu->zero)
            continue;
        OpLda(memory, cpu,
              DirectLongIndirectY(memory, cpu, DP_FORM_MASK_NEXT));
        OpBit(memory, cpu, OpDp(cpu, DP_FORM_INVERSE_MASK));
        if (!cpu->zero)
            continue;
        OpLda(memory, cpu,
              DirectLongIndirectY(memory, cpu, DP_FORM_MASK_NEXT));
        OpOra(memory, cpu, OpDp(cpu, DP_FORM_INVERSE_MASK));
        OpSta(memory, cpu,
              DirectLongIndirectY(memory, cpu, DP_FORM_MASK_NEXT));
        OpStepMem(memory, cpu, OpDp(cpu, DP_FORM_MASK_BITS_LEFT), -1);
        if (cpu->zero)
            break;
    } while (true);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DP_FORM_MASK_SOURCE));
    cpu->carry = 0u;
    OpAdcValue(cpu, FORM_MASK_ROWS);
    OpSta(memory, cpu, OpDp(cpu, DP_FORM_MASK_SOURCE));
    OpLda(memory, cpu, OpDp(cpu, DP_FORM_MASK_NEXT));
    cpu->carry = 0u;
    OpAdcValue(cpu, FORM_MASK_ROWS);
    OpSta(memory, cpu, OpDp(cpu, DP_FORM_MASK_NEXT));
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82caffu);
}

Lufia2ExecutionResult Lufia2CapsuleMergeFormMask(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!FormMaskReady(cpu, true))
        return ExecutionHandoff(cpu, 0x82cb00u);
    OpLdy(cpu, 0u);
    do {
        OpLda(memory, cpu,
              DirectLongIndirectY(memory, cpu, DP_FORM_MERGED_MASK));
        OpOra(memory, cpu,
              DirectLongIndirectY(memory, cpu, DP_FORM_NEW_TILES));
        OpSta(memory, cpu,
              DirectLongIndirectY(memory, cpu, DP_FORM_MERGED_MASK));
        OpIny(cpu);
        OpCpy(cpu, FORM_MASK_BYTES);
    } while (!cpu->zero);
    return ExecutionReturned(0x82cb0fu);
}

static void ClearFormMasks(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    OpLoadA(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpLdx(cpu, 0u);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, FORM_MASK_BASE));
        OpInx(cpu);
        OpCpx(cpu, FORM_MASK_BYTES);
    } while (!cpu->zero);
    OpLdx(cpu, 0u);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, FORM_MASK_SEQUENCE));
        OpInx(cpu);
        OpCpx(cpu, FORM_MASK_SEQUENCE_BYTES);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
}

Lufia2ExecutionResult Lufia2CapsuleBuildFormMasks(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!FormMaskReady(cpu, true) || !child)
        return ExecutionHandoff(cpu, 0x82ca62u);
    CapsuleFeedCall call = {memory, cpu, child, context, {0}};
    ClearFormMasks(memory, cpu);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, DP_FORM_MASK_SOURCE + 2u));
    OpSta(memory, cpu, OpDp(cpu, DP_FORM_MERGED_MASK + 2u));
    OpSta(memory, cpu, OpDp(cpu, DP_FORM_MASK_NEXT + 2u));
    OpSta(memory, cpu, OpDp(cpu, DP_FORM_NEW_TILES + 2u));
    OpLdx(cpu, FORM_MASK_SEQUENCE);
    OpWriteX(memory, cpu, OpDp(cpu, DP_FORM_MASK_NEXT), cpu->x);
    OpLdx(cpu, FORM_MASK_FRAMES);
    do {
        OpPushX(memory, cpu);
        OpLdx(cpu, FORM_MASK_BASE);
        OpWriteX(memory, cpu, OpDp(cpu, DP_FORM_MASK_SOURCE), cpu->x);
        OpWriteX(memory, cpu, OpDp(cpu, DP_FORM_MERGED_MASK), cpu->x);
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_FORM_MASK_NEXT)));
        OpWriteX(memory, cpu, OpDp(cpu, DP_FORM_NEW_TILES), cpu->x);
        OpLdx(cpu, FORM_MASK_COLUMNS);
        do {
            OpPushX(memory, cpu);
            if (!CapsuleFeedChild(&call, 0x82caa2u, 0x82cab1u, 2u))
                return call.result;
            OpLdx(cpu, PullIndexValue(memory, cpu));
            OpDex(cpu);
        } while (!cpu->zero);
        if (!CapsuleFeedChild(&call, 0x82caa9u, 0x82cb00u, 2u))
            return call.result;
        OpLdx(cpu, PullIndexValue(memory, cpu));
        OpDex(cpu);
    } while (!cpu->zero);
    return ExecutionReturned(0x82cab0u);
}
