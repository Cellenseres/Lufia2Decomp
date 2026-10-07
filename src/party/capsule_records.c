#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/party.h"
#include "system/wram.h"

enum {
    CAPSULE_ID = WRAM_CAPSULE_SELECTED_ID,
    CAPSULE_FORM = WRAM_CAPSULE_SELECTED_FORM,
    CAPSULE_FLAG_ANY = WRAM_CAPSULE_FLAG_ANY,
    CAPSULE_FLAGS = WRAM_CAPSULE_FLAG_BYTES,
    CAPSULE_FLAG_COUNT = 20u,
    CAPSULE_RECORD = WRAM_CAPSULE_RECORD_POINTER,
    CAPSULE_INDEX = WRAM_CAPSULE_SAVED_INDEX,
    CAPSULE_EXPERIENCE_OFFSET = WRAM_CAPSULE_SAVED_EXPERIENCE_OFFSET,
    CAPSULE_STATS_OFFSET = WRAM_CAPSULE_SAVED_STATS_OFFSET,
    CAPSULE_WORK_STATS = WRAM_CAPSULE_WORK_STATS,
    CAPSULE_WORK_STATS_SIZE = 10u,
    CAPSULE_LEVEL = WRAM_CAPSULE_WORK_LEVEL,
    CAPSULE_LEVEL_EXPERIENCE = WRAM_CAPSULE_LEVEL_START_EXPERIENCE,
    CAPSULE_RECORD_TABLE = 0x97dcb8u,
    CAPSULE_RECORD_BASE = 0xdcb8u,
    CAPSULE_SAVED_LEVELS = WRAM_CAPSULE_SAVED_LEVELS,
    CAPSULE_SAVED_EXPERIENCE = WRAM_CAPSULE_SAVED_EXPERIENCE,
    CAPSULE_SAVED_STATS = WRAM_CAPSULE_SAVED_STATS
};

static uint8_t CapsuleRecordReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x82u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal;
}

Lufia2ExecutionResult Lufia2CapsuleClearFlags(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    if (!CapsuleRecordReady(cpu))
        return ExecutionHandoff(cpu, 0x82c37bu);
    OpStz(memory, cpu, OpAbs(cpu, CAPSULE_FLAG_ANY));
    OpLdx(cpu, 0u);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, CAPSULE_FLAGS));
        OpInx(cpu);
        OpCpx(cpu, CAPSULE_FLAG_COUNT);
    } while (!cpu->zero);
    return ExecutionReturned(0x82c38au);
}

Lufia2ExecutionResult Lufia2CapsuleResolveRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    if (!CapsuleRecordReady(cpu))
        return ExecutionHandoff(cpu, 0x82c38bu);
    OpLda(memory, cpu, OpAbs(cpu, CAPSULE_ID));
    OpAndValue(cpu, 7u);
    OpAslA(cpu);
    OpAslA(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, CAPSULE_ID));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, CAPSULE_FORM));
    OpDecA(cpu);
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0x00ffu);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, CAPSULE_RECORD_TABLE));
    cpu->carry = 0u;
    OpAdcValue(cpu, CAPSULE_RECORD_BASE);
    OpTax(cpu);
    OpWrite16(memory, OpAbs(cpu, CAPSULE_RECORD), cpu->x);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82c3b0u);
}

Lufia2ExecutionResult Lufia2CapsuleFormIndex(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    if (!CapsuleRecordReady(cpu))
        return ExecutionHandoff(cpu, 0x82c3c4u);
    OpLda(memory, cpu, OpAbs(cpu, CAPSULE_ID));
    OpAslA(cpu);
    OpAslA(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, CAPSULE_ID));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, CAPSULE_FORM));
    OpDecA(cpu);
    return ExecutionReturned(0x82c3d2u);
}

Lufia2ExecutionResult Lufia2CapsuleSavedOffsets(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    if (!CapsuleRecordReady(cpu))
        return ExecutionHandoff(cpu, 0x82c3d3u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, CAPSULE_ID));
    OpAndValue(cpu, 0x00ffu);
    OpSta(memory, cpu, OpAbs(cpu, CAPSULE_INDEX));
    OpLda(memory, cpu, OpAbs(cpu, CAPSULE_INDEX));
    OpAslA(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, CAPSULE_INDEX));
    OpSta(memory, cpu, OpAbs(cpu, CAPSULE_EXPERIENCE_OFFSET));
    OpLda(memory, cpu, OpAbs(cpu, CAPSULE_INDEX));
    OpAslA(cpu);
    OpAslA(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, CAPSULE_INDEX));
    OpSta(memory, cpu, OpAbs(cpu, CAPSULE_STATS_OFFSET));
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82c3f7u);
}

Lufia2ExecutionResult Lufia2CapsuleLoadSavedStats(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CapsuleRecordReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82c3f8u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82c3f8u, 0x82c3d3u, 2u, 0x82u)) {
        Lufia2ExecutionResult result = ExecutionReturned(0x82c3f8u);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    OpLdx(cpu, 0u);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, CAPSULE_WORK_STATS));
        OpInx(cpu);
        OpCpx(cpu, CAPSULE_WORK_STATS_SIZE);
    } while (!cpu->zero);
    OpLdx(cpu, OpRead16(memory, OpAbs(cpu, CAPSULE_INDEX)));
    OpLda(memory, cpu, OpLongX(cpu, CAPSULE_SAVED_LEVELS));
    OpSta(memory, cpu, OpAbs(cpu, CAPSULE_LEVEL));
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, OpRead16(memory, OpAbs(cpu, CAPSULE_EXPERIENCE_OFFSET)));
    OpLda(memory, cpu, OpLongX(cpu, CAPSULE_SAVED_EXPERIENCE));
    OpSta(memory, cpu, OpAbs(cpu, CAPSULE_LEVEL_EXPERIENCE));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, CAPSULE_SAVED_EXPERIENCE + 2u));
    OpSta(memory, cpu, OpAbs(cpu, CAPSULE_LEVEL_EXPERIENCE + 2u));
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, OpRead16(memory, OpAbs(cpu, CAPSULE_STATS_OFFSET)));
    for (unsigned offset = 0u; offset < 4u; offset += 2u) {
        OpLda(memory, cpu, OpLongX(cpu, CAPSULE_SAVED_STATS + offset));
        OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(CAPSULE_WORK_STATS + offset)));
    }
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, CAPSULE_SAVED_STATS + 4u));
    OpSta(memory, cpu, OpAbs(cpu, CAPSULE_WORK_STATS + 4u));
    return ExecutionReturned(0x82c442u);
}
