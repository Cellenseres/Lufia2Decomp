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

enum {
    ENTRY_MODE = WRAM_UNK_7E09C9,
    ENTRY_ID = 0x09cbu,
    ENTRY_REGION = 0x09ccu,
    REGION_CELL_X = 0x09e6u,
    REGION_CELL_Y = 0x09e7u,
    TARGET_REGION = WRAM_UNK_7E09EE,
    TARGET_STATE = 0x09e0u,
    TARGET_PENDING = 0x09e1u,
    TARGET_POSE = 0x09e2u,
    TARGET_PIXEL_X = 0x11e8u,
    TARGET_PIXEL_Y = 0x11eau,
    DP_ENTRY_ID = 0u,
    DP_ENTRY_X = 6u,
    DP_ENTRY_Y = 7u,
    RESOURCE_ENTRY_LIST = 6u,
    ENTRY_RECORD_BYTES = 9u,
    ENTRY_SCAN_BOUNDARY = 4096u,
    ROM_RESOURCE_POINTER = 0x869f35u,
    ROM_CELL_CENTER = 0x8691feu,
    ROM_REGION_POSITION = 0x869f13u,
    ROM_TEST_EVENT_FLAG = 0x80be1au,
    MINIMUM_EVENT_REGION = 0xd6u,
    ENTRY_REGION_EVENT = 0x95u
};

static uint8_t EntryPositionContext(const Lufia2CpuState *cpu, uint8_t m) {
    return cpu->program_bank == 0x86u &&
        cpu->accumulator_is_8_bit == m && !cpu->index_is_8_bit &&
        !cpu->decimal;
}

static uint8_t EntryPositionCall(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t site, uint32_t target,
    uint8_t frame, uint8_t m, Lufia2ExecutionResult *result) {
    if (!CallChildWithFrame(memory, cpu, child, context, site, target, frame, 0x86u)) {
        *result = (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return 0u;
    }
    if (!EntryPositionContext(cpu, m)) {
        *result = ExecutionHandoff(cpu, site + frame + 1u);
        return 0u;
    }
    return 1u;
}

static uint8_t CenterEntryCoordinate(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t site, uint16_t output,
    Lufia2ExecutionResult *result) {
    if (!EntryPositionCall(memory, cpu, child, context, site,
            ROM_CELL_CENTER, 2u, 1u, result))
        return 0u;
    OpWriteX(memory, cpu, OpAbs(cpu, output), cpu->y);
    return 1u;
}

Lufia2ExecutionResult Lufia2WorldMapApplyEntryPosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!RegionContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x869bcau);
    Lufia2ExecutionResult result;
    OpLda(memory, cpu, OpAbs(cpu, ENTRY_MODE));
    OpCmpValue(cpu, 1u);
    if (cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, ENTRY_ID));
        OpSta(memory, cpu, OpDp(cpu, DP_ENTRY_ID));
        PushDataBank(memory, cpu);
        if (!EntryPositionCall(memory, cpu, child, context, 0x869bd7u,
                ROM_RESOURCE_POINTER, 2u, 0u, &result))
            return result;
        OpAdc(memory, cpu, OpAbsX(cpu, RESOURCE_ENTRY_LIST));
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
        unsigned scanned;
        for (scanned = 0u; scanned < ENTRY_SCAN_BOUNDARY; ++scanned) {
            OpLda(memory, cpu, OpAbsX(cpu, 0u));
            if (cpu->negative) {
                PullDataBank(memory, cpu);
                cpu->carry = 0u;
                return ExecutionReturned(0x869bf7u);
            }
            OpCmp(memory, cpu, OpDp(cpu, DP_ENTRY_ID));
            if (cpu->zero)
                break;
            OpRepWidths(cpu, 0x20u);
            OpTxa(cpu);
            cpu->carry = 0u;
            OpAdcValue(cpu, ENTRY_RECORD_BYTES);
            OpTax(cpu);
            OpSepWidths(cpu, 0x20u);
        }
        if (scanned == ENTRY_SCAN_BOUNDARY)
            return ExecutionHandoff(cpu, 0x869be0u);
        OpLda(memory, cpu, OpAbsX(cpu, 1u));
        OpDecA(cpu);
        OpSta(memory, cpu, OpDp(cpu, DP_ENTRY_X));
        OpLda(memory, cpu, OpAbsX(cpu, 4u));
        OpDecA(cpu);
        OpSta(memory, cpu, OpDp(cpu, DP_ENTRY_Y));
        PullDataBank(memory, cpu);
        OpLda(memory, cpu, OpDp(cpu, DP_ENTRY_X));
        if (!CenterEntryCoordinate(memory, cpu, child, context, 0x869c07u,
                TARGET_PIXEL_X, &result))
            return result;
        OpLda(memory, cpu, OpDp(cpu, DP_ENTRY_Y));
        if (!CenterEntryCoordinate(memory, cpu, child, context, 0x869c0fu,
                TARGET_PIXEL_Y, &result))
            return result;
        OpLda(memory, cpu, OpAbs(cpu, ENTRY_REGION));
        OpSta(memory, cpu, OpAbs(cpu, TARGET_REGION));
        OpStz(memory, cpu, OpAbs(cpu, TARGET_PENDING));
        OpLoadA(cpu, 2u);
        OpSta(memory, cpu, OpAbs(cpu, TARGET_POSE));
        OpLda(memory, cpu, OpAbs(cpu, TARGET_STATE));
        if (!cpu->negative)
            OpStz(memory, cpu, OpAbs(cpu, TARGET_STATE));
        OpLoadA(cpu, ENTRY_REGION_EVENT);
        if (!EntryPositionCall(memory, cpu, child, context, 0x869c2du,
                ROM_TEST_EVENT_FLAG, 3u, 1u, &result))
            return result;
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, TARGET_REGION));
            OpCmpValue(cpu, MINIMUM_EVENT_REGION);
            if (!cpu->carry) {
                OpLoadA(cpu, MINIMUM_EVENT_REGION);
                OpSta(memory, cpu, OpAbs(cpu, TARGET_REGION));
            }
        }
    } else {
        OpLda(memory, cpu, OpAbs(cpu, ENTRY_MODE));
        OpCmpValue(cpu, 2u);
        if (!cpu->zero) {
            cpu->carry = 0u;
            return ExecutionReturned(0x869c6eu);
        }
        OpLda(memory, cpu, OpAbs(cpu, REGION_CELL_Y));
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, REGION_CELL_X));
            OpSta(memory, cpu, OpDp(cpu, DP_ENTRY_ID));
            if (!EntryPositionCall(memory, cpu, child, context, 0x869c52u,
                    ROM_REGION_POSITION, 2u, 1u, &result))
                return result;
            OpLda(memory, cpu, OpDp(cpu, DP_ENTRY_X));
            OpSta(memory, cpu, OpAbs(cpu, REGION_CELL_X));
            OpLda(memory, cpu, OpDp(cpu, DP_ENTRY_Y));
        }
        if (!CenterEntryCoordinate(memory, cpu, child, context, 0x869c5cu,
                TARGET_PIXEL_Y, &result))
            return result;
        OpLda(memory, cpu, OpAbs(cpu, REGION_CELL_X));
        if (!CenterEntryCoordinate(memory, cpu, child, context, 0x869c65u,
                TARGET_PIXEL_X, &result))
            return result;
    }
    cpu->carry = 1u;
    return ExecutionReturned(0x869c70u);
}
