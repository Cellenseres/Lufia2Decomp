#include "core/cpu_ops.h"
#include "field_coordinates_internal.h"
#include "lufia2/actor.h"
#include "lufia2/field.h"
#include "system/dp_scratch.h"
#include "system/wram.h"

static bool CoordinateContext(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit &&
        !cpu->decimal && !cpu->direct_page && cpu->program_bank == 0x83u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static void RoundProbeCoordinate(const Lufia2Memory *memory,
                                 Lufia2CpuState *cpu, uint8_t coordinate) {
    OpLda(memory, cpu, OpDp(cpu, coordinate));
    for (unsigned bit = 0; bit < 4; ++bit)
        OpLsrA(cpu);
    OpAdcValue(cpu, 0u);
    OpSta(memory, cpu, OpDp(cpu, coordinate));
}

static void ScaleFineCoordinate(const Lufia2Memory *memory,
                                Lufia2CpuState *cpu, uint32_t coordinate) {
    OpLda(memory, cpu, OpLongX(cpu, coordinate));
    for (unsigned bit = 0; bit < 4; ++bit)
        OpAslA(cpu);
    OpSta(memory, cpu, OpLongX(cpu, coordinate));
}

void Lufia2ObjectRoundedProbe(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SLOT_WORD_OFFSET)));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, WRAM_OBJECT_FINE_Y));
    OpSta(memory, cpu, OpDp(cpu, DP_PROBE_Y));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_OBJECT_FINE_X));
    OpSta(memory, cpu, OpDp(cpu, DP_PROBE_X));
    RoundProbeCoordinate(memory, cpu, DP_PROBE_Y);
    RoundProbeCoordinate(memory, cpu, DP_PROBE_X);
    OpSepWidths(cpu, 0x20u);
}

Lufia2ExecutionResult Lufia2ObjectFinePositionToProbe(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CoordinateContext(cpu))
        return ExecutionHandoff(cpu, 0x83fc8bu);
    Lufia2ObjectRoundedProbe(memory, cpu);
    return ExecutionReturned(0x83fcb3u);
}

Lufia2ExecutionResult Lufia2ObjectScaleFinePosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CoordinateContext(cpu))
        return ExecutionHandoff(cpu, 0x83fcb4u);
    OpRepWidths(cpu, 0x20u);
    ScaleFineCoordinate(memory, cpu, WRAM_OBJECT_FINE_X);
    ScaleFineCoordinate(memory, cpu, WRAM_OBJECT_FINE_Y);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x83fcd0u);
}

static void ProbeCellOffset(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, DP_PROBE_X));
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpDp(cpu, DP_PROBE_Y));
    Lufia2MapCellOffset(memory, cpu);
}

Lufia2ExecutionResult Lufia2MapProbeCellOffset(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CoordinateContext(cpu))
        return ExecutionHandoff(cpu, 0x83f9f2u);
    ProbeCellOffset(memory, cpu);
    return ExecutionReturned(0x83fa11u);
}

void Lufia2MapProbeHeightBody(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0xf98au);
    ProbeCellOffset(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_FIELD_LAYER_TABLE_OFFSET)));
    cpu->carry = false;
    OpAdc(memory, cpu, OpLongX(cpu, WRAM_FIELD_LAYER_CELL_BASE));
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, 0x7f0001u));
    for (unsigned bit = 0; bit < 2; ++bit) {
        OpAslA(cpu);
        OpAdcValue(cpu, 0u);
    }
    OpAndValue(cpu, 3u);
}

Lufia2ExecutionResult Lufia2MapProbeTileHeight(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CoordinateContext(cpu))
        return ExecutionHandoff(cpu, 0x83f988u);
    Lufia2MapProbeHeightBody(memory, cpu);
    return ExecutionReturned(0x83f9a4u);
}

static void PackedAttributeCell(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSta(memory, cpu, 0x004202u);
    OpLda(memory, cpu, WRAM_FIELD_SECTION_WIDTH);
    OpSta(memory, cpu, 0x004203u);
    OpLoadA(cpu, 0u);
    ExchangeAccumulatorBytes(cpu);
    OpRepWidths(cpu, 0x20u);
    cpu->carry = false;
    OpAdc(memory, cpu, 0x004216u);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
}

Lufia2ExecutionResult Lufia2MapPackedAttributeCell(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CoordinateContext(cpu))
        return ExecutionHandoff(cpu, 0x83f9b6u);
    PackedAttributeCell(memory, cpu);
    return ExecutionReturned(0x83f9cfu);
}

Lufia2ExecutionResult Lufia2MapCellOffsetLong(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CoordinateContext(cpu))
        return ExecutionHandoff(cpu, 0x83f9eeu);
    SimulateJsrFrame(memory, cpu, 0xf9f0u);
    Lufia2MapCellOffset(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    return ExecutionReturned(0x83f9f1u);
}

Lufia2ExecutionResult Lufia2FieldObjectAttributeCellLong(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CoordinateContext(cpu))
        return ExecutionHandoff(cpu, 0x83f9a5u);
    SimulateJsrFrame(memory, cpu, 0xf9a7u);
    Lufia2FieldObjectAttributeCell(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    return ExecutionReturned(0x83f9a8u);
}

Lufia2ExecutionResult Lufia2MapPackedAttributeCellLong(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CoordinateContext(cpu))
        return ExecutionHandoff(cpu, 0x83f9a9u);
    SimulateJsrFrame(memory, cpu, 0xf9abu);
    PackedAttributeCell(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    return ExecutionReturned(0x83f9acu);
}
