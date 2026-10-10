#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/field.h"

enum {
    PAGE = 0x00u,
    VALUE_COLUMN = 0x02u,
    ROW = 0x03u,
    FIELD_VALUES = 0x0du,
    INDEX = 0x22u,
    VALUE = 0x23u,
    VALUE_INDEX = 0x24u,
    ROWS_LEFT = 0x26u,
    TEXT_POSITION = 0x2du,
    CHANGE = 0x54u,
    TEXT_BANK = 0x5fu,
    BIT_MASKS = 0x830au
};

static bool DiagnosticsSupported(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit && !cpu->decimal;
}

static Lufia2ExecutionResult DiagnosticsUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static bool DiagnosticsChild(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t site,
    uint32_t target, uint8_t frame) {
    return CallChildWithFrame(memory, cpu, child, context, site, target, frame, 0x85u);
}

Lufia2ExecutionResult Lufia2FieldDiagnosticsResolveFlag(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!DiagnosticsSupported(cpu)) return ExecutionHandoff(cpu, 0x858312u);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpDp(cpu, INDEX));
    OpAndValue(cpu, 7u);
    OpTax(cpu);
    OpLda(memory, cpu, OpDp(cpu, INDEX));
    for (unsigned bit = 0; bit < 3u; ++bit) OpLsrA(cpu);
    OpTay(cpu);
    return ExecutionReturned(0x85831eu);
}

Lufia2ExecutionResult Lufia2FieldDiagnosticsEditValue(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!DiagnosticsSupported(cpu) || !child) return ExecutionHandoff(cpu, 0x85821cu);
    OpSta(memory, cpu, OpDp(cpu, CHANGE));
    OpLda(memory, cpu, OpDp(cpu, ROW));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, PAGE));
    OpSta(memory, cpu, OpDp(cpu, INDEX));
    OpLda(memory, cpu, OpDp(cpu, VALUE_COLUMN));
    if (!cpu->zero) {
        TransferDirectToA(cpu);
        OpLda(memory, cpu, OpDp(cpu, INDEX));
        OpTax(cpu);
        OpLda(memory, cpu, OpDp(cpu, FIELD_VALUES));
        const bool field = !cpu->zero;
        const uint32_t variable = field ? OpLongX(cpu, WRAM_FIELD_EVENT_VALUE_VARIABLES) : OpAbsX(cpu, WRAM_UNK_7E079E);
        OpLda(memory, cpu, variable);
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpDp(cpu, CHANGE));
        OpSta(memory, cpu, variable);
        return ExecutionReturned(field ? 0x85823cu : 0x858246u);
    }
    if (!DiagnosticsChild(memory, cpu, child, context, 0x858247u, 0x858312u, 2u))
        return DiagnosticsUnwound(0x858247u);
    OpLda(memory, cpu, OpDp(cpu, FIELD_VALUES));
    if (!cpu->zero) {
        OpTxa(cpu);
        OpTyx(cpu);
        OpTay(cpu);
        OpLda(memory, cpu, OpLongX(cpu, WRAM_UNK_7FD100));
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ OpReadM(memory, cpu, OpAbsY(cpu, BIT_MASKS))));
        OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7FD100));
        return ExecutionReturned(0x85825cu);
    }
    OpLda(memory, cpu, OpAbsY(cpu, WRAM_EVENT_FLAGS));
    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ OpReadM(memory, cpu, OpAbsX(cpu, BIT_MASKS))));
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_EVENT_FLAGS));
    return ExecutionReturned(0x858266u);
}

Lufia2ExecutionResult Lufia2FieldDiagnosticsCursorAddress(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!DiagnosticsSupported(cpu)) return ExecutionHandoff(cpu, 0x858281u);
    OpLda(memory, cpu, OpDp(cpu, ROW));
    ExchangeAccumulatorBytes(cpu);
    OpLoadA(cpu, 0u);
    OpRepWidths(cpu, 0x20u);
    OpLsrA(cpu);
    OpLsrA(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x104u);
    OpTax(cpu);
    OpLda(memory, cpu, OpDp(cpu, VALUE_COLUMN));
    OpAndValue(cpu, 0xffu);
    if (!cpu->zero) {
        OpTxa(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x1cu);
        OpTax(cpu);
    }
    return ExecutionReturned(0x85829cu);
}

static Lufia2ExecutionResult DiagnosticsCursor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, bool visible) {
    const uint32_t entry = visible ? 0x858274u : 0x858267u;
    if (!DiagnosticsSupported(cpu) || !child) return ExecutionHandoff(cpu, entry);
    if (!DiagnosticsChild(memory, cpu, child, context, entry, 0x858281u, 2u))
        return DiagnosticsUnwound(entry);
    OpLoadA(cpu, visible ? 0x203eu : 0u);
    OpSta(memory, cpu, OpLongX(cpu, 0x7e3000u));
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(visible ? 0x858280u : 0x858273u);
}

Lufia2ExecutionResult Lufia2FieldDiagnosticsHideCursor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return DiagnosticsCursor(memory, cpu, child, context, false);
}

Lufia2ExecutionResult Lufia2FieldDiagnosticsShowCursor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return DiagnosticsCursor(memory, cpu, child, context, true);
}

Lufia2ExecutionResult Lufia2FieldDiagnosticsDrawValues(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!DiagnosticsSupported(cpu) || !child) return ExecutionHandoff(cpu, 0x85829du);
    OpLda(memory, cpu, OpDp(cpu, PAGE));
    OpSta(memory, cpu, OpDp(cpu, INDEX));
    OpLoadA(cpu, 16u);
    OpSta(memory, cpu, OpDp(cpu, ROWS_LEFT));
    OpLdx(cpu, 0x3106u);
    Write16Direct(memory, cpu, TEXT_POSITION, cpu->x);
    do {
        if (!DiagnosticsChild(memory, cpu, child, context, 0x8582aau, 0x858312u, 2u))
            return DiagnosticsUnwound(0x8582aau);
        OpLda(memory, cpu, OpDp(cpu, FIELD_VALUES));
        if (!cpu->zero) {
            OpPushX(memory, cpu);
            OpTyx(cpu);
            OpLda(memory, cpu, OpLongX(cpu, WRAM_UNK_7FD100));
            OpPullX(memory, cpu);
        } else OpLda(memory, cpu, OpAbsY(cpu, WRAM_EVENT_FLAGS));
        OpAndValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, BIT_MASKS)));
        OpSta(memory, cpu, OpDp(cpu, VALUE));
        OpLdx(cpu, Read16Direct(memory, cpu, TEXT_POSITION));
        OpLoadA(cpu, 0x85u);
        OpSta(memory, cpu, OpDp(cpu, TEXT_BANK));
        OpLdy(cpu, 0x85eau);
        if (!DiagnosticsChild(memory, cpu, child, context, 0x8582cbu, 0x808878u, 3u))
            return DiagnosticsUnwound(0x8582cbu);
        TransferDirectToA(cpu);
        OpLda(memory, cpu, OpDp(cpu, INDEX));
        OpAndValue(cpu, 0x7fu);
        OpSta(memory, cpu, OpDp(cpu, VALUE_INDEX));
        OpTax(cpu);
        OpLda(memory, cpu, OpDp(cpu, FIELD_VALUES));
        OpLda(memory, cpu, cpu->zero ? OpAbsX(cpu, WRAM_UNK_7E079E) : OpLongX(cpu, WRAM_FIELD_EVENT_VALUE_VARIABLES));
        OpSta(memory, cpu, OpDp(cpu, VALUE));
        OpLdx(cpu, Read16Direct(memory, cpu, TEXT_POSITION));
        OpLoadA(cpu, 0x85u);
        OpSta(memory, cpu, OpDp(cpu, TEXT_BANK));
        OpLdy(cpu, 0x8600u);
        if (!DiagnosticsChild(memory, cpu, child, context, 0x8582efu, 0x808878u, 3u))
            return DiagnosticsUnwound(0x8582efu);
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpDp(cpu, TEXT_POSITION));
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x40u);
        OpSta(memory, cpu, OpDp(cpu, TEXT_POSITION));
        OpSepWidths(cpu, 0x20u);
        OpStepMem(memory, cpu, OpDp(cpu, INDEX), 1u);
        OpStepMem(memory, cpu, OpDp(cpu, ROWS_LEFT), -1);
    } while (!cpu->zero);
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpDp(cpu, DP_MENU_FRAME_FLAGS));
    return ExecutionReturned(0x858309u);
}

Lufia2ExecutionResult Lufia2FieldDiagnosticsClearTilemap(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->decimal) return ExecutionHandoff(cpu, 0x8584a9u);
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpRepWidths(cpu, 0x30u);
    OpLdy(cpu, 0x7fcu);
    TransferDirectToA(cpu);
    do {
        OpSta(memory, cpu, OpAbsY(cpu, 0x3000u));
        OpSta(memory, cpu, OpAbsY(cpu, 0x3002u));
        for (unsigned word = 0; word < 4u; ++word) OpDey(cpu);
    } while (!cpu->negative);
    OpSepWidths(cpu, 0x20u);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x8584c5u);
}

static Lufia2ExecutionResult DiagnosticsConsumeButtons(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, bool high) {
    const uint32_t entry = high ? 0x85850eu : 0x858515u;
    if (!DiagnosticsSupported(cpu)) return ExecutionHandoff(cpu, entry);
    OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_BUTTONS_HELD + (high ? 1u : 0u))));
    if (!cpu->zero) OpTestBits(memory, cpu, OpDp(cpu, DP_BUTTONS_PRESSED + (high ? 1u : 0u)), 0u);
    return ExecutionReturned(high ? 0x858514u : 0x85851bu);
}

Lufia2ExecutionResult Lufia2FieldDiagnosticsConsumeHighButtons(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return DiagnosticsConsumeButtons(memory, cpu, true);
}

Lufia2ExecutionResult Lufia2FieldDiagnosticsConsumeLowButtons(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return DiagnosticsConsumeButtons(memory, cpu, false);
}
