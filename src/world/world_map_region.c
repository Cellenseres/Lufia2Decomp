#include "lufia2/world_map.h"
#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"

enum {
    REGION_POSITION_REQUEST = WRAM_UNK_7E09EE,
    REGION_PLAYER_POSITION_X = WRAM_UNK_7E09E3,
    REGION_PLAYER_POSITION_Y = WRAM_UNK_7E09E4,
    DP_REGION_ID = 0x00,
    DP_REGION_POSITION = 0x06,
    REGION_POSITION_LIST = 0x0e,
    REGION_POSITION_RECORD_SIZE = 3,
    REGION_SCAN_BOUNDARY = 4096
};

static uint8_t RegionContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x86u && cpu->data_bank == 0x86u &&
        !cpu->direct_page && !cpu->decimal &&
        cpu->accumulator_is_8_bit && !cpu->index_is_8_bit;
}

Lufia2ExecutionResult Lufia2WorldMapFindRegionPosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!RegionContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x869f13u);
    PushDataBank(memory, cpu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x869f14u, 0x869f35u, 2u, 0x86u))
        return (Lufia2ExecutionResult){
            LUFIA2_EXECUTION_CHILD_UNWOUND, 0x869f14u, 0u};
    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x869f17u);
    OpAdc(memory, cpu, OpAbsX(cpu, REGION_POSITION_LIST));
    OpTax(cpu);
    for (unsigned scanned = 0u; scanned < REGION_SCAN_BOUNDARY; ++scanned) {
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        OpCmp(memory, cpu, OpDp(cpu, DP_REGION_ID));
        if (cpu->zero) {
            OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, 1u)));
            OpWriteX(memory, cpu, OpDp(cpu, DP_REGION_POSITION), cpu->y);
            PullDataBank(memory, cpu);
            return ExecutionReturned(0x869f34u);
        }
        OpRepWidths(cpu, 0x20u);
        OpTxa(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, REGION_POSITION_RECORD_SIZE);
        OpTax(cpu);
    }
    return ExecutionHandoff(cpu, 0x869f1bu);
}

Lufia2ExecutionResult Lufia2WorldMapConsumeRegionPosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!RegionContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x86a2aeu);
    OpLda(memory, cpu, OpAbs(cpu, REGION_POSITION_REQUEST));
    if (!cpu->zero) {
        OpSta(memory, cpu, OpDp(cpu, DP_REGION_ID));
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x86a2b5u, 0x869f13u, 2u, 0x86u))
            return (Lufia2ExecutionResult){
                LUFIA2_EXECUTION_CHILD_UNWOUND, 0x86a2b5u, 0u};
        if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
            return ExecutionHandoff(cpu, 0x86a2b8u);
        OpLda(memory, cpu, OpDp(cpu, DP_REGION_POSITION));
        OpSta(memory, cpu, OpAbs(cpu, REGION_PLAYER_POSITION_X));
        OpLda(memory, cpu, OpDp(cpu, DP_REGION_POSITION + 1u));
        OpSta(memory, cpu, OpAbs(cpu, REGION_PLAYER_POSITION_Y));
        OpStz(memory, cpu, OpAbs(cpu, REGION_POSITION_REQUEST));
    }
    return ExecutionReturned(0x86a2c5u);
}
