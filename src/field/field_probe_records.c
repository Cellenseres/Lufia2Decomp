#include "core/cpu_ops.h"
#include "lufia2/actor.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    OBJECT_RECORD_STATE = WRAM_FIELD_MAP_RECORD_BYTE7 - 16u,
    OBJECT_RECORD_PROPERTIES = WRAM_FIELD_MAP_RECORD_BYTE8 - 16u
};

static bool ProbeRecordContext(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit &&
        !cpu->decimal && !cpu->direct_page && cpu->program_bank == 0x83u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static void ProbeMapRecord(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                           uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x83u, return_address);
    Lufia2ActorReadMapCellValue(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    OpBitValue(cpu, 0xf0u);
}

static void ProbePendingRecord(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                               uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Lufia2FieldFindPendingObject(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

Lufia2ExecutionResult Lufia2FieldProbeObjectAttributes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ProbeRecordContext(cpu))
        return ExecutionHandoff(cpu, 0x83fb2eu);
    ProbeMapRecord(memory, cpu, 0xfb31u);
    if (!cpu->zero) {
        OpOraValue(cpu, 0u);
        if (cpu->zero) {
            ProbePendingRecord(memory, cpu, 0xfb3cu);
            if (cpu->carry) {
                TransferDirectToA(cpu);
                return ExecutionReturned(0x83fb50u);
            }
            TransferDirectToA(cpu);
            OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_PENDING_OBJECT_RECORD));
        }
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, OBJECT_RECORD_PROPERTIES));
        ExchangeAccumulatorBytes(cpu);
        OpLda(memory, cpu, OpLongX(cpu, OBJECT_RECORD_STATE));
        return ExecutionReturned(0x83fb4eu);
    }
    TransferDirectToA(cpu);
    return ExecutionReturned(0x83fb50u);
}

Lufia2ExecutionResult Lufia2FieldProbeObjectState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ProbeRecordContext(cpu))
        return ExecutionHandoff(cpu, 0x83fb51u);
    ProbeMapRecord(memory, cpu, 0xfb54u);
    if (!cpu->zero) {
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, OBJECT_RECORD_STATE));
        return ExecutionReturned(0x83fb5eu);
    }
    TransferDirectToA(cpu);
    return ExecutionReturned(0x83fb60u);
}

Lufia2ExecutionResult Lufia2FieldProbeObjectProperties(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ProbeRecordContext(cpu))
        return ExecutionHandoff(cpu, 0x83fb61u);
    ProbeMapRecord(memory, cpu, 0xfb64u);
    if (!cpu->zero) {
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, OBJECT_RECORD_PROPERTIES));
        return ExecutionReturned(0x83fb6eu);
    }
    TransferDirectToA(cpu);
    return ExecutionReturned(0x83fb70u);
}

Lufia2ExecutionResult Lufia2FieldPendingObjectState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ProbeRecordContext(cpu))
        return ExecutionHandoff(cpu, 0x83fb8bu);
    ProbePendingRecord(memory, cpu, 0xfb8du);
    if (!cpu->carry) {
        TransferDirectToA(cpu);
        OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_PENDING_OBJECT_RECORD));
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, OBJECT_RECORD_STATE));
    }
    return ExecutionReturned(0x83fb9au);
}

Lufia2ExecutionResult Lufia2FieldFindPendingObjectLong(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ProbeRecordContext(cpu))
        return ExecutionHandoff(cpu, 0x83fb9bu);
    ProbePendingRecord(memory, cpu, 0xfb9du);
    return ExecutionReturned(0x83fb9eu);
}
