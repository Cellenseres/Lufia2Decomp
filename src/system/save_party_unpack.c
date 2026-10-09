#include "core/cpu_ops.h"
#include "lufia2/system.h"

enum {
    DP_SAVE_STAT_BITS = 0x22u,
    DP_SAVE_PARTY_RECORD = 0x30u,
    PARTY_LEVEL = 0x0eu,
    PARTY_STATUS = 0x0fu,
    PARTY_HP = 0x11u,
    PARTY_MP = 0x13u,
    PARTY_BASE_STATS = 0x51u,
    PARTY_EXPERIENCE = 0x5fu,
    PARTY_EXPERIENCE_END = 0x62u,
    PARTY_EQUIPMENT = 0x66u,
    PARTY_EQUIPMENT_END = 0x86u,
    PARTY_SAVED_BYTES = 0x94u,
    PARTY_SIX_BIT_FIELDS = 0x96u,
    PARTY_SIX_BIT_FIELDS_END = 0xbau,
    PARTY_RECORD_TAIL_END = 0xbdu
};

static void RestorePartyByte(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbsX(cpu, 0u));
    OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, DP_SAVE_PARTY_RECORD));
    OpInx(cpu);
}

static void RestorePartyVitals(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdy(cpu, PARTY_HP);
    RestorePartyByte(memory, cpu);
    OpIny(cpu);
    OpIny(cpu);
    RestorePartyByte(memory, cpu);
    OpLdy(cpu, PARTY_EXPERIENCE);
    do {
        RestorePartyByte(memory, cpu);
        OpIny(cpu);
        OpCpy(cpu, PARTY_EXPERIENCE_END);
    } while (!cpu->zero);
    OpLdy(cpu, PARTY_EQUIPMENT);
    do {
        RestorePartyByte(memory, cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, PARTY_EQUIPMENT_END);
    } while (!cpu->zero);
    OpLdy(cpu, PARTY_SAVED_BYTES);
    RestorePartyByte(memory, cpu);
    OpIny(cpu);
    RestorePartyByte(memory, cpu);
}

static void RestoreSixBitValue(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpCmpValue(cpu, 0x3fu);
    if (cpu->zero)
        OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, DP_SAVE_PARTY_RECORD));
    OpIny(cpu);
}

static void RestorePartySixBitFields(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdy(cpu, PARTY_SIX_BIT_FIELDS);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        OpAndValue(cpu, 0xc0u);
        OpSta(memory, cpu, OpDp(cpu, DP_SAVE_STAT_BITS));
        OpLda(memory, cpu, OpAbsX(cpu, 1u));
        OpAndValue(cpu, 0xc0u);
        OpLsrA(cpu);
        OpLsrA(cpu);
        OpTestBits(memory, cpu, OpDp(cpu, DP_SAVE_STAT_BITS), 1u);
        OpLda(memory, cpu, OpAbsX(cpu, 2u));
        OpAndValue(cpu, 0xc0u);
        for (unsigned bit = 0u; bit < 4u; ++bit)
            OpLsrA(cpu);
        OpOra(memory, cpu, OpDp(cpu, DP_SAVE_STAT_BITS));
        OpLsrA(cpu);
        OpLsrA(cpu);
        RestoreSixBitValue(memory, cpu);
        for (unsigned field = 0u; field < 3u; ++field) {
            OpLda(memory, cpu, OpAbsX(cpu, 0u));
            OpInx(cpu);
            OpAndValue(cpu, 0x3fu);
            RestoreSixBitValue(memory, cpu);
        }
        OpCpy(cpu, PARTY_SIX_BIT_FIELDS_END);
    } while (!cpu->zero);
    do {
        RestorePartyByte(memory, cpu);
        OpIny(cpu);
        OpCpy(cpu, PARTY_RECORD_TAIL_END);
    } while (!cpu->zero);
}

static void ConsumeStatHighBit(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, unsigned plane) {
    const uint32_t address = OpDp(cpu, (uint8_t)(DP_SAVE_STAT_BITS + plane));
    const uint8_t previous = Read8(memory, address);
    const uint8_t remaining = (uint8_t)(previous << 1);
    cpu->carry = (previous & 0x80u) != 0u;
    Write8(memory, address, remaining);
    SetNz8(cpu, remaining);
}

static void RestorePartyStatBonuses(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdy(cpu, PARTY_LEVEL);
    OpLda(memory, cpu, OpAbsX(cpu, 0u));
    OpLsrA(cpu);
    OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, DP_SAVE_PARTY_RECORD));
    OpInx(cpu);
    for (unsigned plane = 0u; plane < 3u; ++plane) {
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        OpSta(memory, cpu, OpDp(cpu, (uint8_t)(DP_SAVE_STAT_BITS + plane)));
        OpInx(cpu);
    }
    OpLdy(cpu, PARTY_BASE_STATS);
    do {
        TransferDirectToA(cpu);
        for (unsigned plane = 3u; plane > 0u; --plane) {
            ConsumeStatHighBit(memory, cpu, plane - 1u);
            RolA8(cpu);
        }
        OpRepWidths(cpu, 0x20u);
        OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, DP_SAVE_PARTY_RECORD));
        OpSepWidths(cpu, 0x20u);
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, PARTY_EXPERIENCE);
    } while (!cpu->zero);
    OpLda(memory, cpu, OpAbsX(cpu, 0u));
    OpInx(cpu);
    OpLsrA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_SAVE_STAT_BITS));
    OpLdy(cpu, 0x73u);
    do {
        OpLsrMem(memory, cpu, OpDp(cpu, DP_SAVE_STAT_BITS));
        TransferDirectToA(cpu);
        RolA8(cpu);
        OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, DP_SAVE_PARTY_RECORD));
        OpDey(cpu);
        OpDey(cpu);
        OpCpy(cpu, PARTY_EQUIPMENT - 1u);
    } while (!cpu->zero);
}

static void RestorePartyHighPairs(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    const uint16_t fields[4], uint8_t status) {
    OpLdy(cpu, fields[0]);
    OpLda(memory, cpu, OpAbsX(cpu, 0u));
    OpAndValue(cpu, 3u);
    if (status) {
        cpu->carry = 0u;
        OpAdcValue(cpu, 2u);
        OpAndValue(cpu, 5u);
    }
    OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, DP_SAVE_PARTY_RECORD));
    OpLdy(cpu, fields[1]);
    OpLda(memory, cpu, OpAbsX(cpu, 0u));
    for (unsigned field = 1u; field < 4u; ++field) {
        if (field > 1u) {
            OpLdy(cpu, fields[field]);
            LoadA8(cpu, Pull8(memory, cpu));
        }
        OpLsrA(cpu);
        OpLsrA(cpu);
        if (field < 3u) {
            PushAccumulator8(memory, cpu);
            OpAndValue(cpu, 3u);
        }
        OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, DP_SAVE_PARTY_RECORD));
    }
    OpInx(cpu);
}

Lufia2ExecutionResult Lufia2SaveUnpackPartyRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint16_t vitals[4] = {0x28u, 0x26u, 0x14u, 0x12u};
    static const uint16_t modifiers[4] = {0x7fu, 0x7du, 0x77u, 0x75u};
    static const uint16_t status[4] = {PARTY_STATUS, 0x85u, 0x83u, 0x81u};

    if (cpu->decimal)
        return ExecutionHandoff(cpu, 0x85c754u);
    RestorePartyVitals(memory, cpu);
    RestorePartySixBitFields(memory, cpu);
    RestorePartyStatBonuses(memory, cpu);
    RestorePartyHighPairs(memory, cpu, vitals, 0u);
    RestorePartyHighPairs(memory, cpu, modifiers, 0u);
    RestorePartyHighPairs(memory, cpu, status, 1u);
    return ExecutionReturned(0x85c8ceu);
}

Lufia2ExecutionResult Lufia2SavePackStatBits(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    for (unsigned plane = 0u; plane < 3u; ++plane) {
        OpLsrA(cpu);
        OpRolMem8(memory, cpu, OpDp(cpu, (uint8_t)(DP_SAVE_STAT_BITS + plane)));
    }
    return ExecutionReturned(0x85cbdbu);
}
