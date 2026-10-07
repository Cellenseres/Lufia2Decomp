#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "lufia2/party.h"
#include "system/wram.h"

enum {
    EXPERIENCE_STEP = WRAM_CAPSULE_EXPERIENCE_STEP,
    LEVEL_EXPERIENCE = WRAM_CAPSULE_LEVEL_EXPERIENCE,
    EXPERIENCE_MAXIMUM = 9999999u,
    EXPERIENCE_CAPPED_LEVEL = 98u,
    EXPERIENCE_INITIAL_STEP = 20u,
    EXPERIENCE_LEVEL_OFFSET = 10u,
    ROM_GROWTH_FACTORS = 0x8ee4cbu,
    DP_GROWTH_PRODUCTS = 0x11u,
    DP_GROWTH_GAIN = 0x12u
};

static uint8_t CapsuleExperienceReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x82u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal;
}

static void CapLevelExperience(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    for (unsigned byte = 0u; byte < 3u; ++byte) {
        LoadA8(cpu, (uint8_t)(EXPERIENCE_MAXIMUM >> (8u * byte)));
        OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(LEVEL_EXPERIENCE + byte)));
    }
}

static void AddExperienceStep(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    for (unsigned byte = 0u; byte < 3u; ++byte) {
        OpLda(memory, cpu, OpAbs(cpu, (uint16_t)(LEVEL_EXPERIENCE + byte)));
        if (!byte)
            cpu->carry = 0u;
        OpAdc(memory, cpu, OpAbs(cpu, (uint16_t)(EXPERIENCE_STEP + 1u + byte)));
        OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(LEVEL_EXPERIENCE + byte)));
    }
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, LEVEL_EXPERIENCE));
    cpu->carry = 1u;
    OpSbcValue(cpu, EXPERIENCE_MAXIMUM & 0xffffu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, LEVEL_EXPERIENCE + 2u));
    OpSbcValue(cpu, EXPERIENCE_MAXIMUM >> 16);
    if (cpu->carry)
        CapLevelExperience(memory, cpu);
}

static void MultiplyStepBytes(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_WORK_LEVEL));
    OpAndValue(cpu, 0xffu);
    OpIncA(cpu);
    for (unsigned bit = 0u; bit < 3u; ++bit)
        LsrA16(cpu);
    TransferAToX(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, ROM_GROWTH_FACTORS));
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
    for (unsigned byte = 0u; byte < 4u; ++byte) {
        OpLda(memory, cpu, OpAbs(cpu, (uint16_t)(EXPERIENCE_STEP + byte)));
        OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, SNES_RDMPYL, 0u));
        Write16Direct(memory, cpu, (uint8_t)(DP_GROWTH_PRODUCTS + 2u * byte), cpu->x);
    }
}

static void CombineGrowthProducts(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, DP_GROWTH_PRODUCTS + 1u));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, DP_GROWTH_PRODUCTS + 2u));
    OpSta(memory, cpu, OpDp(cpu, DP_GROWTH_GAIN));
    OpLda(memory, cpu, OpDp(cpu, DP_GROWTH_PRODUCTS + 3u));
    OpAdc(memory, cpu, OpDp(cpu, DP_GROWTH_PRODUCTS + 4u));
    OpSta(memory, cpu, OpDp(cpu, DP_GROWTH_GAIN + 1u));
    OpLda(memory, cpu, OpDp(cpu, DP_GROWTH_PRODUCTS + 6u));
    OpAdc(memory, cpu, OpDp(cpu, DP_GROWTH_PRODUCTS + 5u));
    OpSta(memory, cpu, OpDp(cpu, DP_GROWTH_GAIN + 2u));
    LoadA8(cpu, 0u);
    OpAdc(memory, cpu, OpDp(cpu, DP_GROWTH_PRODUCTS + 7u));
    OpSta(memory, cpu, OpDp(cpu, DP_GROWTH_GAIN + 3u));
}

static void GrowExperienceStep(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    for (unsigned byte = 0u; byte < 4u; ++byte) {
        OpLda(memory, cpu, OpAbs(cpu, (uint16_t)(EXPERIENCE_STEP + byte)));
        if (!byte)
            cpu->carry = 0u;
        OpAdc(memory, cpu, OpDp(cpu, (uint8_t)(DP_GROWTH_GAIN + byte)));
        OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(EXPERIENCE_STEP + byte)));
    }
}

Lufia2ExecutionResult Lufia2CapsuleAdvanceExperienceStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CapsuleExperienceReady(cpu))
        return ExecutionHandoff(cpu, 0x82ceabu);
    AddExperienceStep(memory, cpu);
    MultiplyStepBytes(memory, cpu);
    CombineGrowthProducts(memory, cpu);
    GrowExperienceStep(memory, cpu);
    return ExecutionReturned(0x82cf71u);
}

Lufia2ExecutionResult Lufia2CapsuleBuildLevelExperience(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CapsuleExperienceReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82ce52u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_WORK_LEVEL));
    OpCmpValue(cpu, EXPERIENCE_CAPPED_LEVEL);
    if (cpu->carry) {
        CapLevelExperience(memory, cpu);
        return ExecutionReturned(0x82ce68u);
    }
    LoadA8(cpu, EXPERIENCE_INITIAL_STEP);
    OpStz(memory, cpu, OpAbs(cpu, EXPERIENCE_STEP));
    OpSta(memory, cpu, OpAbs(cpu, EXPERIENCE_STEP + 1u));
    for (unsigned byte = 2u; byte < 4u; ++byte)
        OpStz(memory, cpu, OpAbs(cpu, (uint16_t)(EXPERIENCE_STEP + byte)));
    for (unsigned byte = 0u; byte < 3u; ++byte)
        OpStz(memory, cpu, OpAbs(cpu, (uint16_t)(LEVEL_EXPERIENCE + byte)));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_WORK_LEVEL));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_WORK_LEVEL));
    do {
        PushAccumulator8(memory, cpu);
        OpStepMem(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_WORK_LEVEL), 1);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82ce8au, 0x82ceabu, 2u, 0x82u)) {
            Lufia2ExecutionResult result = ExecutionReturned(0x82ce8au);
            result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
            return result;
        }
        LoadA8(cpu, Pull8(memory, cpu));
        OpDecA(cpu);
    } while (!cpu->zero);
    for (unsigned byte = 0u; byte < 3u; ++byte) {
        OpLda(memory, cpu, OpAbs(cpu, (uint16_t)(LEVEL_EXPERIENCE + byte)));
        if (!byte)
            cpu->carry = 1u;
        OpSbcValue(cpu, byte ? 0u : EXPERIENCE_LEVEL_OFFSET);
        OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(LEVEL_EXPERIENCE + byte)));
    }
    return ExecutionReturned(0x82ceaau);
}
