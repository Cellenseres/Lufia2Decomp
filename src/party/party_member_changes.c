#include "core/cpu_ops.h"
#include "lufia2/party.h"
#include "system/wram.h"

enum {
    MEMBER_STATUS = 0x0fu,
    MEMBER_HP = 0x11u,
    MEMBER_MP = 0x13u,
    MEMBER_MAX_HP = 0x25u,
    MEMBER_MAX_MP = 0x27u,
    MEMBER_STATUS_COUNTER = 0xbau,
    MEMBER_DISABLED = 4u,
    MEMBER_WIDE_STATUS_TEST = 0xf004u
};

static bool MemberChangeReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x81u && cpu->accumulator_is_8_bit &&
           !cpu->index_is_8_bit && cpu->stack >= 0x1f20u &&
           cpu->stack <= 0x1ffcu;
}

static void SelectMemberRecord(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t return_address, bool keep_status_mask) {
    OpSepWidths(cpu, 0x10u);
    if (keep_status_mask)
        OpTxy(cpu);
    OpTax(cpu);
    OpRepWidths(cpu, 0x10u);
    SimulateJsrFrame(memory, cpu, return_address);
    (void)Lufia2BattleTable9EBA(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

static uint32_t AddRecordStatus(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const bool byte_status = cpu->accumulator_is_8_bit != 0;
    OpOra(memory, cpu, OpAbsX(cpu, MEMBER_STATUS));
    OpSta(memory, cpu, OpAbsX(cpu, MEMBER_STATUS));
    /* Word entry consumes the following branch bytes. */
    OpBitValue(cpu, byte_status ? MEMBER_DISABLED : MEMBER_WIDE_STATUS_TEST);
    if (byte_status && cpu->zero)
        return 0x81f669u;
    if (!byte_status)
        OpAslA(cpu);
    OpRepWidths(cpu, 0x20u);
    OpStepMem(memory, cpu, OpAbsX(cpu, MEMBER_STATUS_COUNTER), 1);
    OpStz(memory, cpu, OpAbsX(cpu, MEMBER_HP));
    OpSepWidths(cpu, 0x20u);
    return 0x81f669u;
}

Lufia2ExecutionResult Lufia2PartyAddRecordStatus(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!MemberChangeReady(cpu))
        return ExecutionHandoff(cpu, 0x81f655u);
    return ExecutionReturned(AddRecordStatus(memory, cpu));
}

static uint32_t ClearRecordStatus(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
    OpAndValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, MEMBER_STATUS)));
    OpSta(memory, cpu, OpAbsX(cpu, MEMBER_STATUS));
    OpBitValue(cpu, MEMBER_DISABLED);
    if (cpu->zero) {
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbsX(cpu, MEMBER_HP));
        if (cpu->zero)
            OpStepMem(memory, cpu, OpAbsX(cpu, MEMBER_HP), 1);
        OpSepWidths(cpu, 0x20u);
    }
    return 0x81f682u;
}

Lufia2ExecutionResult Lufia2PartyClearRecordStatus(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!MemberChangeReady(cpu))
        return ExecutionHandoff(cpu, 0x81f66au);
    return ExecutionReturned(ClearRecordStatus(memory, cpu));
}

Lufia2ExecutionResult Lufia2PartyAddMemberStatus(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!MemberChangeReady(cpu))
        return ExecutionHandoff(cpu, 0x81f639u);
    SelectMemberRecord(memory, cpu, 0xf641u, true);
    OpTya(cpu);
    SimulateJsrFrame(memory, cpu, 0xf645u);
    (void)AddRecordStatus(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    return ExecutionReturned(0x81f646u);
}

Lufia2ExecutionResult Lufia2PartyClearMemberStatus(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!MemberChangeReady(cpu))
        return ExecutionHandoff(cpu, 0x81f647u);
    SelectMemberRecord(memory, cpu, 0xf64fu, true);
    OpTya(cpu);
    SimulateJsrFrame(memory, cpu, 0xf653u);
    (void)ClearRecordStatus(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    return ExecutionReturned(0x81f654u);
}

static Lufia2ExecutionResult AddMemberVital(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, bool health) {
    const uint16_t current = health ? MEMBER_HP : MEMBER_MP;
    const uint16_t maximum = health ? MEMBER_MAX_HP : MEMBER_MAX_MP;
    SelectMemberRecord(memory, cpu, health ? 0xf68au : 0xf6deu, false);
    OpRepWidths(cpu, 0x20u);
    OpTya(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbsX(cpu, current));
    OpCmp(memory, cpu, OpAbsX(cpu, maximum));
    if (!cpu->carry) {
        cpu->carry = 1u;
        OpLda(memory, cpu, OpAbsX(cpu, maximum));
        OpSbcValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, current)));
        OpTay(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, maximum));
    }
    OpSta(memory, cpu, OpAbsX(cpu, current));
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(health ? 0x81f6a7u : 0x81f6fbu);
}

static Lufia2ExecutionResult SubtractMemberVital(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, bool health) {
    const uint16_t current = health ? MEMBER_HP : MEMBER_MP;
    SelectMemberRecord(memory, cpu, health ? 0xf6afu : 0xf703u, false);
    OpRepWidths(cpu, 0x20u);
    OpTya(cpu);
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, current)));
    if (!cpu->carry) {
        OpWriteX(memory, cpu, OpAbs(cpu, WRAM_PARTY_STAT_MEMBER_INDEX), cpu->y);
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, current)));
        OpSta(memory, cpu, OpAbsX(cpu, current));
        OpSepWidths(cpu, 0x20u);
        if (health)
            cpu->carry = 0u;
        return ExecutionReturned(health ? 0x81f6c6u : 0x81f719u);
    }
    OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, current)));
    OpStz(memory, cpu, OpAbsX(cpu, current));
    if (health) {
        OpLoadA(cpu, MEMBER_DISABLED);
        SimulateJsrFrame(memory, cpu, 0xf6d2u);
        (void)AddRecordStatus(memory, cpu);
        SimulateRtsFrame(memory, cpu);
    }
    OpSepWidths(cpu, 0x20u);
    if (health)
        cpu->carry = 1u;
    return ExecutionReturned(health ? 0x81f6d6u : 0x81f722u);
}

Lufia2ExecutionResult Lufia2PartyAddMemberHp(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!MemberChangeReady(cpu))
        return ExecutionHandoff(cpu, 0x81f683u);
    return AddMemberVital(memory, cpu, true);
}

Lufia2ExecutionResult Lufia2PartyAddMemberMp(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!MemberChangeReady(cpu))
        return ExecutionHandoff(cpu, 0x81f6d7u);
    return AddMemberVital(memory, cpu, false);
}

Lufia2ExecutionResult Lufia2PartySubtractMemberHp(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!MemberChangeReady(cpu))
        return ExecutionHandoff(cpu, 0x81f6a8u);
    return SubtractMemberVital(memory, cpu, true);
}

Lufia2ExecutionResult Lufia2PartySubtractMemberMp(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!MemberChangeReady(cpu))
        return ExecutionHandoff(cpu, 0x81f6fcu);
    return SubtractMemberVital(memory, cpu, false);
}
