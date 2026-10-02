/* Object header lookup, pending-slot search and map attribute addressing. */

#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    HEADER_RECORD_ID = 0x54,
    HEADER_RECORD_STRIDE = 0x5a,
    HEADER_SCAN_LIMIT = 65536,
};

Lufia2ExecutionResult Lufia2FieldFindHeaderRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    OpSta(memory, cpu, OpDp(cpu, HEADER_RECORD_ID));
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpDp(cpu, HEADER_RECORD_STRIDE));
    OpSetDataBank(memory, cpu, 0x7eu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, 0xf000u));
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);

    for (unsigned visited = 0; visited < HEADER_SCAN_LIMIT; ++visited) {
        OpLda(memory, cpu, OpAbsX(cpu, 0xf000u));
        OpCmp(memory, cpu, OpDp(cpu, HEADER_RECORD_ID));
        if (cpu->zero) {
            PullDataBank(memory, cpu);
            cpu->carry = 0;
            return ExecutionReturned(0x80bfd5u);
        }
        OpCmpValue(cpu, 0x00ffu);
        if (cpu->zero) {
            PullDataBank(memory, cpu);
            cpu->carry = 1;
            return ExecutionReturned(0x80bfd8u);
        }
        OpTxa(cpu);
        cpu->carry = 0;
        OpAdc(memory, cpu, OpDp(cpu, HEADER_RECORD_STRIDE));
        if (cpu->carry) {
            ExchangeAccumulatorBytes(cpu);
            OpIncA(cpu);
            ExchangeAccumulatorBytes(cpu);
        }
        OpTax(cpu);
    }
    return ExecutionHandoff(cpu, 0x80bfbcu);
}

Lufia2ExecutionResult Lufia2FieldFindPendingObject(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_PENDING_RECORD_X));
        OpCmp(memory, cpu, OpDp(cpu, DP_PROBE_X));
        if (cpu->zero) {
            OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_PENDING_RECORD_Y));
            OpCmp(memory, cpu, OpDp(cpu, DP_PROBE_Y));
            if (cpu->zero) {
                cpu->carry = 0;
                return ExecutionReturned(0x83fbbcu);
            }
        }
        OpInx(cpu);
        OpCpx(cpu, WRAM_FIELD_PENDING_RECORD_X_COUNT);
    } while (!cpu->zero);
    TransferDirectToA(cpu);
    cpu->carry = 1;
    return ExecutionReturned(0x83fbbau);
}

Lufia2ExecutionResult Lufia2FieldObjectAttributeCell(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpStz(memory, cpu, OpDp(cpu, DP_PROBE_X + 1u));
    OpStz(memory, cpu, OpDp(cpu, DP_PROBE_Y + 1u));
    OpLda(memory, cpu, OpDp(cpu, DP_PROBE_X));
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpDp(cpu, DP_PROBE_Y));
    OpSta(memory, cpu, 0x004202u);
    OpLda(memory, cpu, WRAM_FIELD_SECTION_WIDTH);
    OpSta(memory, cpu, 0x004203u);
    OpLoadA(cpu, 0u);
    ExchangeAccumulatorBytes(cpu);
    OpRepWidths(cpu, 0x20u);
    cpu->carry = 0;
    OpAdc(memory, cpu, 0x004216u);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x83f9cfu);
}
