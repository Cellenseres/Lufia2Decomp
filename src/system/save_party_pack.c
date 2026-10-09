#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/system.h"
#include "system/wram.h"

enum {
    DP_PACKED_STAT_BITS = 0x22u,
    DP_PACKED_FIELD_BITS = 0x23u,
    DP_PACKED_STATUS = 0x11u,
    DP_PARTY_RECORD = 0x30u,
    PARTY_LEVEL = 0x0eu,
    PARTY_STATUS = 0x0fu,
    PARTY_HP = 0x11u,
    PARTY_EXPERIENCE = 0x5fu,
    PARTY_EQUIPMENT = 0x66u,
    PARTY_MODIFIERS_END = 0x86u,
    PARTY_SAVED_BYTES = 0x94u,
    PARTY_SIX_BIT_FIELDS = 0x96u,
    PARTY_SIX_BIT_FIELDS_END = 0xbau,
    PARTY_RECORD_TAIL_END = 0xbdu,
    PARTY_STAT_BONUSES = WRAM_PARTY_BASE_STAT_WORK
};

static void PackPartyByte(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, DP_PARTY_RECORD));
    OpSta(memory, cpu, OpAbsX(cpu, 0u));
    OpInx(cpu);
}

static void PackPartyVitals(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdy(cpu, PARTY_HP);
    PackPartyByte(memory, cpu);
    OpIny(cpu);
    OpIny(cpu);
    PackPartyByte(memory, cpu);
    OpLdy(cpu, PARTY_EXPERIENCE);
    do {
        PackPartyByte(memory, cpu);
        OpIny(cpu);
        OpCpy(cpu, PARTY_EXPERIENCE + 3u);
    } while (!cpu->zero);
    OpLdy(cpu, PARTY_EQUIPMENT);
    do {
        PackPartyByte(memory, cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, PARTY_MODIFIERS_END);
    } while (!cpu->zero);
    OpLdy(cpu, PARTY_SAVED_BYTES);
    PackPartyByte(memory, cpu);
    OpIny(cpu);
    PackPartyByte(memory, cpu);
}

static void PackSixBitField(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpAndValue(cpu, 0xc0u);
    OpSta(memory, cpu, OpDp(cpu, DP_PACKED_FIELD_BITS));
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, DP_PARTY_RECORD));
    OpAndValue(cpu, 0x3fu);
    OpOra(memory, cpu, OpDp(cpu, DP_PACKED_FIELD_BITS));
    OpIny(cpu);
    OpSta(memory, cpu, OpAbsX(cpu, 0u));
    OpInx(cpu);
}

static void AdvancePackedFieldBits(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const uint32_t address = OpDp(cpu, DP_PACKED_STAT_BITS);
    const uint8_t previous = Read8(memory, address);
    const uint8_t remaining = (uint8_t)(previous << 1);
    cpu->carry = (previous & 0x80u) != 0u;
    Write8(memory, address, remaining);
    SetNz8(cpu, remaining);
}

static void PackPartySixBitFields(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdy(cpu, PARTY_SIX_BIT_FIELDS);
    do {
        OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, DP_PARTY_RECORD));
        OpIny(cpu);
        OpAslA(cpu);
        OpAslA(cpu);
        OpSta(memory, cpu, OpDp(cpu, DP_PACKED_STAT_BITS));
        PackSixBitField(memory, cpu);
        AdvancePackedFieldBits(memory, cpu);
        AdvancePackedFieldBits(memory, cpu);
        OpLda(memory, cpu, OpDp(cpu, DP_PACKED_STAT_BITS));
        PackSixBitField(memory, cpu);
        OpLda(memory, cpu, OpDp(cpu, DP_PACKED_STAT_BITS));
        OpAslA(cpu);
        OpAslA(cpu);
        PackSixBitField(memory, cpu);
        OpCpy(cpu, PARTY_SIX_BIT_FIELDS_END);
    } while (!cpu->zero);
    do {
        PackPartyByte(memory, cpu);
        OpIny(cpu);
        OpCpy(cpu, PARTY_RECORD_TAIL_END);
    } while (!cpu->zero);
}

static void PackPartyHighPairs(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    const uint16_t fields[4]) {
    OpLdy(cpu, fields[0]);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, DP_PARTY_RECORD));
    for (unsigned field = 1u; field < 4u; ++field) {
        OpAslA(cpu);
        OpAslA(cpu);
        OpLdy(cpu, fields[field]);
        OpOra(memory, cpu, DirectLongIndirectY(memory, cpu, DP_PARTY_RECORD));
    }
    OpSta(memory, cpu, OpAbsX(cpu, 0u));
    OpInx(cpu);
}

static Lufia2ExecutionResult PackPartyUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2SavePackPartyRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    static const uint32_t stat_sites[8] = {
        0x85cac4u, 0x85cacau, 0x85cad0u, 0x85cad6u,
        0x85cadcu, 0x85cae2u, 0x85cae8u, 0x85caecu
    };
    static const uint16_t vitals[4] = {0x12u, 0x14u, 0x26u, 0x28u};
    static const uint16_t modifiers[4] = {0x75u, 0x77u, 0x7du, 0x7fu};

    if (!child || cpu->decimal || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x85ca22u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x85ca22u, 0x85cc69u, 2u, cpu->program_bank))
        return PackPartyUnwound(0x85ca22u);
    PackPartyVitals(memory, cpu);
    PackPartySixBitFields(memory, cpu);
    OpLdy(cpu, PARTY_LEVEL);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, DP_PARTY_RECORD));
    OpAslA(cpu);
    OpSta(memory, cpu, OpAbsX(cpu, 0u));
    OpInx(cpu);
    for (unsigned stat = 0u; stat < 8u; ++stat) {
        if (stat < 7u)
            OpLda(memory, cpu, OpAbs(cpu, (uint16_t)(PARTY_STAT_BONUSES + 2u * stat)));
        else
            TransferDirectToA(cpu);
        if (!CallChildWithFrame(memory, cpu, child, context,
                stat_sites[stat], 0x85cbd2u, 2u, cpu->program_bank))
            return PackPartyUnwound(stat_sites[stat]);
    }
    for (unsigned plane = 0u; plane < 3u; ++plane) {
        OpLda(memory, cpu, OpDp(cpu, (uint8_t)(DP_PACKED_STAT_BITS + plane)));
        OpSta(memory, cpu, OpAbsX(cpu, 0u));
        OpInx(cpu);
    }
    OpLdy(cpu, PARTY_EQUIPMENT + 1u);
    do {
        OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, DP_PARTY_RECORD));
        OpLsrA(cpu);
        OpRolMem8(memory, cpu, OpDp(cpu, DP_PACKED_STAT_BITS));
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, 0x75u);
    } while (!cpu->zero);
    OpLda(memory, cpu, OpDp(cpu, DP_PACKED_STAT_BITS));
    OpAslA(cpu);
    OpSta(memory, cpu, OpAbsX(cpu, 0u));
    OpInx(cpu);
    PackPartyHighPairs(memory, cpu, vitals);
    PackPartyHighPairs(memory, cpu, modifiers);
    OpLdy(cpu, 0x81u);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, DP_PARTY_RECORD));
    for (unsigned field = 0u; field < 2u; ++field) {
        OpAslA(cpu);
        OpAslA(cpu);
        OpLdy(cpu, (uint16_t)(0x83u + 2u * field));
        OpOra(memory, cpu, DirectLongIndirectY(memory, cpu, DP_PARTY_RECORD));
    }
    OpAslA(cpu);
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_PACKED_STATUS));
    OpLdy(cpu, PARTY_STATUS);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, DP_PARTY_RECORD));
    OpAndValue(cpu, 5u);
    OpLsrA(cpu);
    OpAdcValue(cpu, 0u);
    OpOra(memory, cpu, OpDp(cpu, DP_PACKED_STATUS));
    OpSta(memory, cpu, OpAbsX(cpu, 0u));
    OpInx(cpu);
    return ExecutionReturned(0x85cb7au);
}
