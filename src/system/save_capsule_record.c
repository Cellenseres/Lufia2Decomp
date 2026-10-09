#include "core/cpu_ops.h"
#include "lufia2/system.h"

enum {
    DP_CAPSULE_PACKED_BITS = 0x23u,
    DP_CAPSULE_PACKED_HIGH = 0x22u,
    DP_CAPSULE_HIGH_FIELDS = 0x2au,
    DP_CAPSULE_RECORD = 0x30u,
    CAPSULE_HIGH_FIELDS_OFFSET = 7u,
    CAPSULE_HEADER_OFFSET = 6u,
    CAPSULE_FORM_FIELDS_START = 0x1eu,
    CAPSULE_FORM_FIELDS_END = 0x25u,
    CAPSULE_WORD_FIELDS_START = 0x2du,
    CAPSULE_WORD_FIELDS_END = 0x3bu
};

static void LocateCapsuleHighFields(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_CAPSULE_RECORD)));
    for (unsigned byte = 0u; byte < CAPSULE_HIGH_FIELDS_OFFSET; ++byte)
        OpIny(cpu);
    OpWriteX(memory, cpu, OpDp(cpu, DP_CAPSULE_HIGH_FIELDS), cpu->y);
    OpLda(memory, cpu, OpDp(cpu, DP_CAPSULE_RECORD + 2u));
    OpSta(memory, cpu, OpDp(cpu, DP_CAPSULE_HIGH_FIELDS + 2u));
}

static void UnpackCapsuleByte(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbsX(cpu, 0u));
    OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, DP_CAPSULE_RECORD));
    OpInx(cpu);
    OpIny(cpu);
}

static void PackCapsuleByte(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, DP_CAPSULE_RECORD));
    OpSta(memory, cpu, OpAbsX(cpu, 0u));
    OpInx(cpu);
    OpIny(cpu);
}

static void CollectCapsuleLowBit(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const uint32_t address = OpDp(cpu, DP_CAPSULE_PACKED_BITS);
    const uint8_t previous = Read8(memory, address);
    const uint8_t bits = (uint8_t)((previous >> 1) | (cpu->carry ? 0x80u : 0u));
    cpu->carry = previous & 1u;
    Write8(memory, address, bits);
    SetNz8(cpu, bits);
}

static void TakeCapsuleLowBit(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const uint32_t address = OpDp(cpu, DP_CAPSULE_PACKED_BITS);
    const uint8_t previous = Read8(memory, address);
    const uint8_t bits = (uint8_t)(previous << 1);
    cpu->carry = (previous & 0x80u) != 0u;
    Write8(memory, address, bits);
    SetNz8(cpu, bits);
}

Lufia2ExecutionResult Lufia2SaveUnpackCapsuleRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LocateCapsuleHighFields(memory, cpu);
    OpLdy(cpu, CAPSULE_HEADER_OFFSET);
    UnpackCapsuleByte(memory, cpu);
    UnpackCapsuleByte(memory, cpu);
    OpLdy(cpu, CAPSULE_FORM_FIELDS_START);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        OpAndValue(cpu, 7u);
        OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, DP_CAPSULE_RECORD));
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        for (unsigned bit = 0u; bit < 3u; ++bit)
            OpLsrA(cpu);
        OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, DP_CAPSULE_HIGH_FIELDS));
        OpInx(cpu);
        OpIny(cpu);
        OpCpy(cpu, CAPSULE_FORM_FIELDS_END);
    } while (!cpu->zero);
    OpLdy(cpu, CAPSULE_WORD_FIELDS_START);
    do {
        UnpackCapsuleByte(memory, cpu);
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        OpLsrA(cpu);
        CollectCapsuleLowBit(memory, cpu);
        OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, DP_CAPSULE_RECORD));
        OpInx(cpu);
        OpIny(cpu);
        OpCpy(cpu, CAPSULE_WORD_FIELDS_END);
    } while (!cpu->zero);
    OpLdy(cpu, CAPSULE_FORM_FIELDS_START);
    do {
        OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, DP_CAPSULE_RECORD));
        TakeCapsuleLowBit(memory, cpu);
        RolA8(cpu);
        OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, DP_CAPSULE_RECORD));
        OpIny(cpu);
        OpCpy(cpu, CAPSULE_FORM_FIELDS_END);
    } while (!cpu->zero);
    return ExecutionReturned(0x85c931u);
}

Lufia2ExecutionResult Lufia2SavePackCapsuleRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LocateCapsuleHighFields(memory, cpu);
    OpLdy(cpu, CAPSULE_HEADER_OFFSET);
    PackCapsuleByte(memory, cpu);
    PackCapsuleByte(memory, cpu);
    OpLdy(cpu, CAPSULE_FORM_FIELDS_START);
    do {
        OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, DP_CAPSULE_HIGH_FIELDS));
        for (unsigned bit = 0u; bit < 3u; ++bit)
            OpAslA(cpu);
        OpSta(memory, cpu, OpDp(cpu, DP_CAPSULE_PACKED_HIGH));
        OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, DP_CAPSULE_RECORD));
        OpLsrA(cpu);
        CollectCapsuleLowBit(memory, cpu);
        OpAndValue(cpu, 7u);
        OpOra(memory, cpu, OpDp(cpu, DP_CAPSULE_PACKED_HIGH));
        OpSta(memory, cpu, OpAbsX(cpu, 0u));
        OpInx(cpu);
        OpIny(cpu);
        OpCpy(cpu, CAPSULE_FORM_FIELDS_END);
    } while (!cpu->zero);
    OpLdy(cpu, CAPSULE_WORD_FIELDS_START);
    do {
        PackCapsuleByte(memory, cpu);
        OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, DP_CAPSULE_RECORD));
        TakeCapsuleLowBit(memory, cpu);
        RolA8(cpu);
        OpSta(memory, cpu, OpAbsX(cpu, 0u));
        OpInx(cpu);
        OpIny(cpu);
        OpCpy(cpu, CAPSULE_WORD_FIELDS_END);
    } while (!cpu->zero);
    return ExecutionReturned(0x85cbd1u);
}
