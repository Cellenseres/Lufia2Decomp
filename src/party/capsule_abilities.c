#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/party.h"

enum {
    DP_ABILITY_MASK = 0x11u,
    DP_ABILITY_ID = 0x13u,
    DP_ABILITY_OFFSET = 0x19u,
    DP_ABILITY_BANK = 0x1bu,
    DP_SPELL_MEMBER = 0x22u,
    DP_SPELL_LIST = 0x2au,
    CAPSULE_ABILITY_FLAGS = WRAM_CAPSULE_FLAG_BYTES + 8u,
    ROM_CAPSULE_ABILITY_MASKS = 0x8ed8c3u,
    ROM_MEMBER_RECORDS = 0x859ebau,
    ROM_MEMBER_SPELL_MASKS = 0x8ed8bbu,
    SPELL_APPLICABILITY_OFFSET = 11u,
    MEMBER_SPELL_LIST_OFFSET = 0x96u,
    MEMBER_SPELL_CAPACITY = 36u
};

static bool CapsuleSpellReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x82u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal;
}

static Lufia2ExecutionResult CapsuleSpellUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2CapsuleStoreSavedStats(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !CapsuleSpellReady(cpu))
        return ExecutionHandoff(cpu, 0x82c443u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82c443u, 0x82c3d3u, 2u, 0x82u))
        return CapsuleSpellUnwound(0x82c443u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_SAVED_INDEX)));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_WORK_LEVEL));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_CAPSULE_SAVED_LEVELS));
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, OpReadX(memory, cpu,
        OpAbs(cpu, WRAM_CAPSULE_SAVED_EXPERIENCE_OFFSET)));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_LEVEL_START_EXPERIENCE));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_CAPSULE_SAVED_EXPERIENCE));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_LEVEL_START_EXPERIENCE + 2u));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_CAPSULE_SAVED_EXPERIENCE + 2u));
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, OpReadX(memory, cpu,
        OpAbs(cpu, WRAM_CAPSULE_SAVED_STATS_OFFSET)));
    for (unsigned offset = 0u; offset < 4u; offset += 2u) {
        OpLda(memory, cpu,
            OpAbs(cpu, (uint16_t)(WRAM_CAPSULE_WORK_STATS + offset)));
        OpSta(memory, cpu, OpLongX(cpu, WRAM_CAPSULE_SAVED_STATS + offset));
    }
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_WORK_STATS + 4u));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_CAPSULE_SAVED_STATS + 4u));
    return ExecutionReturned(0x82c481u);
}

Lufia2ExecutionResult Lufia2CapsuleSelectAbilityFlags(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CapsuleSpellReady(cpu))
        return ExecutionHandoff(cpu, 0x82c4a2u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_SELECTED_ID));
    OpAndValue(cpu, 0xffu);
    OpAslA(cpu);
    cpu->carry = false;
    OpAdcValue(cpu, (uint16_t)CAPSULE_ABILITY_FLAGS);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82c4b2u);
}

static void CapsuleAdvanceAbilityMask(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    for (unsigned bit = 0u; bit < 3u; ++bit) {
        const uint16_t mask = Read16Direct(memory, cpu, DP_ABILITY_MASK);
        const uint16_t shifted = (uint16_t)(mask << 1);
        cpu->carry = (mask & 0x8000u) != 0u;
        Write8(memory, DirectAddress(cpu, DP_ABILITY_MASK + 1u),
            (uint8_t)(shifted >> 8));
        Write8(memory, DirectAddress(cpu, DP_ABILITY_MASK), (uint8_t)shifted);
        SetNz16(cpu, shifted);
    }
}

Lufia2ExecutionResult Lufia2CapsuleBuildAbilityMask(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CapsuleSpellReady(cpu))
        return ExecutionHandoff(cpu, 0x82ccfeu);
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, ROM_CAPSULE_ABILITY_MASKS));
    OpSta(memory, cpu, OpDp(cpu, DP_ABILITY_MASK));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_SELECTED_FORM));
    OpAndValue(cpu, 0xffu);
    OpDecA(cpu);
    while (!cpu->zero) {
        CapsuleAdvanceAbilityMask(memory, cpu);
        OpDecA(cpu);
    }
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82cd1eu);
}

Lufia2ExecutionResult Lufia2CapsuleReadAbilityByte(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !CapsuleSpellReady(cpu))
        return ExecutionHandoff(cpu, 0x82c4b3u);
    OpSta(memory, cpu, OpDp(cpu, DP_ABILITY_OFFSET));
    OpWriteM(memory, cpu, OpDp(cpu, DP_ABILITY_OFFSET + 1u), 0u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82c4b7u, 0x82ccfeu, 2u, 0x82u))
        return CapsuleSpellUnwound(0x82c4b7u);
    OpSepWidths(cpu, 0x20u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82c4bcu, 0x82c38bu, 2u, 0x82u))
        return CapsuleSpellUnwound(0x82c4bcu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82c4bfu, 0x82c4a2u, 2u, 0x82u))
        return CapsuleSpellUnwound(0x82c4bfu);
    OpLoadA(cpu, 0x97u);
    OpSta(memory, cpu, OpDp(cpu, DP_ABILITY_BANK));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_RECORD_POINTER));
    cpu->carry = false;
    OpAdc(memory, cpu, OpDp(cpu, DP_ABILITY_OFFSET));
    OpSta(memory, cpu, OpDp(cpu, DP_ABILITY_OFFSET));
    OpLda(memory, cpu, OpAbsX(cpu, 0u));
    OpBit(memory, cpu, OpDp(cpu, DP_ABILITY_MASK));
    OpLdy(cpu, cpu->zero ? 15u : 18u);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, DP_ABILITY_OFFSET));
    return ExecutionReturned(0x82c4e3u);
}

Lufia2ExecutionResult Lufia2CapsuleUnlockAbility(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !CapsuleSpellReady(cpu))
        return ExecutionHandoff(cpu, 0x82cd41u);
    OpSta(memory, cpu, OpDp(cpu, DP_ABILITY_ID));
    OpWriteM(memory, cpu, OpDp(cpu, DP_ABILITY_ID + 1u), 0u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82cd45u, 0x82ccfeu, 2u, 0x82u))
        return CapsuleSpellUnwound(0x82cd45u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82cd48u, 0x82c4a2u, 2u, 0x82u))
        return CapsuleSpellUnwound(0x82cd48u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, 0u));
    OpBit(memory, cpu, OpDp(cpu, DP_ABILITY_MASK));
    if (cpu->zero) {
        OpSepWidths(cpu, 0x20u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82cd56u, 0x82c38bu, 2u, 0x82u))
            return CapsuleSpellUnwound(0x82cd56u);
        OpLoadA(cpu, 0x97u);
        OpSta(memory, cpu, OpDp(cpu, DP_ABILITY_BANK));
        OpRepWidths(cpu, 0x20u);
        OpTxa(cpu);
        cpu->carry = false;
        OpAdc(memory, cpu, OpDp(cpu, DP_ABILITY_ID));
        OpSta(memory, cpu, OpDp(cpu, DP_ABILITY_OFFSET));
        OpSepWidths(cpu, 0x20u);
        OpLdy(cpu, 18u);
        OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, DP_ABILITY_OFFSET));
        if (!cpu->zero) {
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82cd6eu, 0x82c4a2u, 2u, 0x82u))
                return CapsuleSpellUnwound(0x82cd6eu);
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpAbsX(cpu, 0u));
            OpOra(memory, cpu, OpDp(cpu, DP_ABILITY_MASK));
            OpSta(memory, cpu, OpAbsX(cpu, 0u));
            OpSepWidths(cpu, 0x20u);
            cpu->carry = false;
            return ExecutionReturned(0x82cd7eu);
        }
    }
    OpSepWidths(cpu, 0x20u);
    cpu->carry = true;
    return ExecutionReturned(0x82cd82u);
}

static uint32_t PartySelectedSpellAddress(
    const Lufia2Memory *memory, const Lufia2CpuState *cpu) {
    const uint16_t list = Read16Direct(memory, cpu, DP_SPELL_LIST);
    return AbsoluteIndexedAddress(cpu, list, cpu->y);
}

Lufia2ExecutionResult Lufia2PartyLearnSelectedSpell(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !CapsuleSpellReady(cpu))
        return ExecutionHandoff(cpu, 0x82fd3du);
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, DP_SPELL_MEMBER));
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, ROM_MEMBER_RECORDS));
    cpu->carry = false;
    OpAdcValue(cpu, MEMBER_SPELL_LIST_OFFSET);
    OpSta(memory, cpu, OpDp(cpu, DP_SPELL_LIST));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_RECORD_ID));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82fd55u, 0x81f414u, 3u, 0x82u))
        return CapsuleSpellUnwound(0x82fd55u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SPELL_MEMBER)));
    OpLda(memory, cpu, OpLongX(cpu, ROM_MEMBER_SPELL_MASKS));
    OpBit(memory, cpu, OpAbs(cpu,
        WRAM_RECORD_BUFFER + SPELL_APPLICABILITY_OFFSET));
    if (!cpu->zero) {
        OpLdy(cpu, 0u);
        do {
            OpLda(memory, cpu, PartySelectedSpellAddress(memory, cpu));
            OpCmpValue(cpu, 0xffu);
            if (cpu->zero) {
                OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_RECORD_ID));
                OpSta(memory, cpu, PartySelectedSpellAddress(memory, cpu));
                cpu->carry = false;
                return ExecutionReturned(0x82fd80u);
            }
            OpCmp(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_RECORD_ID));
            if (cpu->zero)
                break;
            OpIny(cpu);
            OpCpy(cpu, MEMBER_SPELL_CAPACITY);
        } while (!cpu->zero);
    }
    cpu->carry = true;
    return ExecutionReturned(0x82fd79u);
}
