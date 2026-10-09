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
    SAVE_RECORD_CHECKSUM = 0x7e3ffdu,
    SAVE_RECORD_DIFFERENCE = WRAM_SAVE_RECORD_CHECKSUM_DIFFERENCE,
    SAVE_PARTY_HEADER = 0x0badu,
    SAVE_CAPSULE_RECORD = 0x119du,
    SAVE_PARTY_COUNT = 0x0a7au,
    SAVE_PARTY_MEMBERS = 0x0a7bu,
    SAVE_FIELD_STATE = 0x0a8au,
    SAVE_ACTIVE_PARTY = 0x0a80u
};

static void RestoreSaveFieldState(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdy(cpu, 4u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, DP_PARTY_RECORD));
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->negative);
    OpLdy(cpu, 4u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        OpSta(memory, cpu, OpAbsY(cpu, SAVE_PARTY_MEMBERS));
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->negative);
    OpLdy(cpu, 0xe9u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        OpSta(memory, cpu, OpAbsY(cpu, SAVE_FIELD_STATE));
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->negative);
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7fu);
    OpLdy(cpu, 0xffu);
    do {
        OpLda(memory, cpu, LongIndexedAddress(0x7e0000u, cpu->x));
        OpSta(memory, cpu, OpAbsY(cpu, 0xf080u));
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->negative);
    PullDataBank(memory, cpu);
}

static void RestoreSaveItemTail(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint32_t destination, uint16_t last) {
    OpLdx(cpu, last);
    do {
        OpLda(memory, cpu, OpAbsY(cpu, 0u));
        OpSta(memory, cpu, LongIndexedAddress(destination, cpu->x));
        OpIny(cpu);
        OpDex(cpu);
    } while (!cpu->negative);
}

static void CountSavedPartyMembers(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 3u);
    OpLdy(cpu, 0u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, SAVE_PARTY_MEMBERS));
        if (!cpu->negative)
            OpIny(cpu);
        OpDex(cpu);
    } while (!cpu->negative);
    OpTya(cpu);
    OpSta(memory, cpu, OpAbs(cpu, SAVE_PARTY_COUNT));
}

static uint8_t RestoreSaveChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target, uint8_t frame) {
    return CallChildWithFrame(memory, cpu, child, context,
        site, target, frame, cpu->program_bank);
}

static Lufia2ExecutionResult RestoreSaveUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2SaveRestoreState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    static const uint16_t members[7] = {
        0x0badu, 0x0c6bu, 0x0d29u, 0x0de7u, 0x0ea5u, 0x0f63u, 0x1021u
    };
    static const uint32_t unpack_sites[7] = {
        0x85c68cu, 0x85c694u, 0x85c69cu, 0x85c6a4u,
        0x85c6acu, 0x85c6b4u, 0x85c6bcu
    };
    static const uint32_t check_sites[7] = {
        0x85c722u, 0x85c728u, 0x85c72eu, 0x85c734u,
        0x85c73au, 0x85c740u, 0x85c746u
    };
    static const uint32_t prepare_sites[5] = {
        0x85c6e5u, 0x85c6e8u, 0x85c6ffu, 0x85c703u, 0x85c707u
    };
    static const uint32_t prepare_targets[5] = {
        0x85cbdcu, 0x81f789u, 0x82994eu, 0x82c515u, 0x82c261u
    };

    if (!child || cpu->decimal || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x85c60eu);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    if (!RestoreSaveChild(memory, cpu, child, context, 0x85c614u, 0x81ec56u, 3u))
        return RestoreSaveUnwound(0x85c614u);
    OpLdy(cpu, SAVE_RECORD_DATA);
    OpWriteX(memory, cpu, OpDp(cpu, DP_CHECKSUM_RECORD), cpu->y);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, DP_CHECKSUM_RECORD + 2u));
    OpLdy(cpu, SAVE_RECORD_SIZE);
    OpWriteX(memory, cpu, OpDp(cpu, DP_CHECKSUM_SIZE), cpu->y);
    if (!RestoreSaveChild(memory, cpu, child, context, 0x85c626u, 0x85de6cu, 3u))
        return RestoreSaveUnwound(0x85c626u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DP_CHECKSUM));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, SAVE_RECORD_CHECKSUM));
    OpSta(memory, cpu, OpAbs(cpu, SAVE_RECORD_DIFFERENCE));
    OpSepWidths(cpu, 0x20u);
    if (!cpu->zero) {
        UnpackStatus(cpu, Pull8(memory, cpu));
        PullDataBank(memory, cpu);
        return ExecutionReturned(0x85c753u);
    }
    OpLoadA(cpu, 0u);
    OpLdx(cpu, SAVE_PARTY_HEADER);
    OpWriteX(memory, cpu, OpDp(cpu, DP_PARTY_RECORD), cpu->x);
    OpSta(memory, cpu, OpDp(cpu, DP_PARTY_RECORD + 2u));
    OpSetDataBank(memory, cpu, 0x7eu);
    OpLdx(cpu, SAVE_RECORD_DATA);
    RestoreSaveFieldState(memory, cpu);
    for (unsigned member = 0u; member < 7u; ++member) {
        OpLdy(cpu, members[member]);
        OpWriteX(memory, cpu, OpDp(cpu, DP_PARTY_RECORD), cpu->y);
        if (!RestoreSaveChild(memory, cpu, child, context,
                unpack_sites[member], 0x85c754u, 2u))
            return RestoreSaveUnwound(unpack_sites[member]);
    }
    OpLdy(cpu, SAVE_CAPSULE_RECORD);
    OpWriteX(memory, cpu, OpDp(cpu, DP_PARTY_RECORD), cpu->y);
    if (!RestoreSaveChild(memory, cpu, child, context, 0x85c6c4u, 0x85c8cfu, 2u))
        return RestoreSaveUnwound(0x85c6c4u);
    OpTxy(cpu);
    RestoreSaveItemTail(memory, cpu, 0x7ff180u, 0x3eu);
    RestoreSaveItemTail(memory, cpu, 0x7ff1bfu, 0x1bu);
    OpTyx(cpu);
    for (unsigned service = 0u; service < 5u; ++service) {
        if (service == 2u)
            CountSavedPartyMembers(memory, cpu);
        if (!RestoreSaveChild(memory, cpu, child, context,
                prepare_sites[service], prepare_targets[service], service == 0u ? 2u : 3u))
            return RestoreSaveUnwound(prepare_sites[service]);
    }
    OpLdx(cpu, 6u);
    do {
        OpLdy(cpu, OpRead16(memory, OpAbsX(cpu, SAVE_ACTIVE_PARTY)));
        if (!cpu->zero) {
            PushIndex(memory, cpu);
            if (!RestoreSaveChild(memory, cpu, child, context, 0x85c714u, 0x81f4e9u, 3u))
                return RestoreSaveUnwound(0x85c714u);
            OpPullX(memory, cpu);
        }
        OpDex(cpu);
        OpDex(cpu);
    } while (!cpu->negative);
    OpStz(memory, cpu, OpDp(cpu, DP_CHECKSUM));
    for (unsigned member = 0u; member < 7u; ++member) {
        OpLdy(cpu, members[member]);
        if (!RestoreSaveChild(memory, cpu, child, context,
                check_sites[member], 0x85c932u, 2u))
            return RestoreSaveUnwound(check_sites[member]);
    }
    OpLda(memory, cpu, OpDp(cpu, DP_CHECKSUM));
    if (!cpu->zero &&
            !RestoreSaveChild(memory, cpu, child, context, 0x85c74du, 0x81ec56u, 3u))
        return RestoreSaveUnwound(0x85c74du);
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x85c753u);
}
