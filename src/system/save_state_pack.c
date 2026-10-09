#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/system.h"
#include "system/wram.h"

enum {
    DP_CHECKSUM = 0x22u,
    DP_CHECKSUM_SIZE = 0x26u,
    DP_CHECKSUM_RECORD = 0x2au,
    DP_PARTY_RECORD = 0x30u,
    SAVE_RECORD_DATA = 0x3bc8u,
    SAVE_RECORD_SIZE = 0x0435u,
    SAVE_RECORD_CHECKSUM = 0x3ffdu,
    SAVE_PARTY_MEMBER_INDEX = WRAM_PARTY_STAT_MEMBER_INDEX,
    SAVE_PARTY_HEADER = 0x0badu,
    SAVE_CAPSULE_RECORD = 0x119du,
    SAVE_FIELD_HEADER = 0x0a7bu,
    SAVE_FIELD_STATE = 0x0a8au
};

static void StoreSaveByte(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSta(memory, cpu, OpAbsX(cpu, 0u));
    OpInx(cpu);
    OpDey(cpu);
}

static void PackSaveFieldState(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdy(cpu, 4u);
    do {
        OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, DP_PARTY_RECORD));
        StoreSaveByte(memory, cpu);
    } while (!cpu->negative);
    OpLdy(cpu, 4u);
    do {
        OpLda(memory, cpu, OpAbsY(cpu, SAVE_FIELD_HEADER));
        StoreSaveByte(memory, cpu);
    } while (!cpu->negative);
    OpLdy(cpu, 0xe9u);
    do {
        OpLda(memory, cpu, OpAbsY(cpu, SAVE_FIELD_STATE));
        StoreSaveByte(memory, cpu);
    } while (!cpu->negative);
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7fu);
    OpLdy(cpu, 0xffu);
    do {
        OpLda(memory, cpu, OpAbsY(cpu, 0xf080u));
        OpSta(memory, cpu, LongIndexedAddress(0x7e0000u, cpu->x));
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->negative);
    PullDataBank(memory, cpu);
}

static void PackSaveItemTail(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint32_t source, uint16_t last) {
    OpLdx(cpu, last);
    do {
        OpLda(memory, cpu, LongIndexedAddress(source, cpu->x));
        OpSta(memory, cpu, OpAbsY(cpu, 0u));
        OpIny(cpu);
        OpDex(cpu);
    } while (!cpu->negative);
}

static Lufia2ExecutionResult SavePackUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2SavePackState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    static const uint16_t members[7] = {
        0x0badu, 0x0c6bu, 0x0d29u, 0x0de7u, 0x0ea5u, 0x0f63u, 0x1021u
    };
    static const uint32_t member_sites[7] = {
        0x85c9afu, 0x85c9b7u, 0x85c9bfu, 0x85c9c7u,
        0x85c9cfu, 0x85c9d7u, 0x85c9dfu
    };

    if (!child || cpu->decimal || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x85c954u);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    OpLoadA(cpu, 0u);
    OpLdx(cpu, SAVE_PARTY_HEADER);
    OpWriteX(memory, cpu, OpDp(cpu, DP_PARTY_RECORD), cpu->x);
    OpSta(memory, cpu, OpDp(cpu, DP_PARTY_RECORD + 2u));
    OpSetDataBank(memory, cpu, 0x7eu);
    OpLdx(cpu, SAVE_RECORD_DATA);
    PackSaveFieldState(memory, cpu);
    OpLdy(cpu, 0u);
    OpWriteX(memory, cpu, OpAbs(cpu, SAVE_PARTY_MEMBER_INDEX), cpu->y);
    for (unsigned member = 0u; member < 7u; ++member) {
        OpLdy(cpu, members[member]);
        OpWriteX(memory, cpu, OpDp(cpu, DP_PARTY_RECORD), cpu->y);
        if (!CallChildWithFrame(memory, cpu, child, context,
                member_sites[member], 0x85ca22u, 2u, cpu->program_bank))
            return SavePackUnwound(member_sites[member]);
    }
    OpLdy(cpu, SAVE_CAPSULE_RECORD);
    OpWriteX(memory, cpu, OpDp(cpu, DP_PARTY_RECORD), cpu->y);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x85c9e7u, 0x85cb7bu, 2u, cpu->program_bank))
        return SavePackUnwound(0x85c9e7u);
    OpTxy(cpu);
    PackSaveItemTail(memory, cpu, 0x7ff180u, 0x3eu);
    PackSaveItemTail(memory, cpu, 0x7ff1bfu, 0x1bu);
    OpTyx(cpu);
    OpLdy(cpu, SAVE_RECORD_DATA);
    OpWriteX(memory, cpu, OpDp(cpu, DP_CHECKSUM_RECORD), cpu->y);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, DP_CHECKSUM_RECORD + 2u));
    OpLdy(cpu, SAVE_RECORD_SIZE);
    OpWriteX(memory, cpu, OpDp(cpu, DP_CHECKSUM_SIZE), cpu->y);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x85ca16u, 0x85de6cu, 3u, cpu->program_bank))
        return SavePackUnwound(0x85ca16u);
    OpLdy(cpu, OpRead16(memory, OpDp(cpu, DP_CHECKSUM)));
    OpWriteX(memory, cpu, OpAbs(cpu, SAVE_RECORD_CHECKSUM), cpu->y);
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x85ca21u);
}
