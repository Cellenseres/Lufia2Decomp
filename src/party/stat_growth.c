#include "core/cpu_ops.h"
#include "lufia2/party.h"
#include <stdbool.h>

typedef struct GrowthContext {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    uint32_t unwind_site;
} GrowthContext;

static bool GrowthCall(GrowthContext *growth, uint16_t site, uint32_t target,
                       uint8_t frame) {
    if (frame == 2u)
        SimulateJsrFrame(growth->memory, growth->cpu, (uint16_t)(site + 2u));
    else
        SimulateJslFrame(growth->memory, growth->cpu, 0x81u, (uint16_t)(site + 3u));
    if (growth->child(growth->context, growth->cpu, target, 0x810000u | site, frame))
        return true;
    growth->unwind_site = 0x810000u | site;
    return false;
}

static Lufia2ExecutionResult GrowthUnwound(const GrowthContext *growth) {
    Lufia2ExecutionResult result = ExecutionReturned(growth->unwind_site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static uint32_t GrowthStat(const Lufia2Memory *memory, const Lufia2CpuState *cpu) {
    return (OpAbs(cpu, Read16Direct(memory, cpu, 0xb2u)) + cpu->y) & 0xffffffu;
}

/* $81:F7CA: apply one party level gain and refresh the derived record. */
Lufia2ExecutionResult Lufia2PartyApplyLevel(const Lufia2Memory *memory,
                                            Lufia2CpuState *cpu,
                                            Lufia2PushedChildCall child,
                                            void *context) {
    GrowthContext growth = {memory, cpu, child, context, 0u};
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, 0x09fau));
    OpSepWidths(cpu, 0x20u);
    if (!GrowthCall(&growth, 0xf7d4u, 0x81f979u, 2u))
        return GrowthUnwound(&growth);
    OpCmpValue(cpu, 1u);
    if (!cpu->zero) {
        cpu->carry = 0;
        return ExecutionReturned(0x81f7ecu);
    }
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, 0x09fau)));
    if (!GrowthCall(&growth, 0xf7deu, 0x81f7bdu, 2u))
        return GrowthUnwound(&growth);
    OpWriteX(memory, cpu, OpDp(cpu, 0xc1u), cpu->x);
    if (!GrowthCall(&growth, 0xf7e3u, 0x81f7edu, 2u) ||
        !GrowthCall(&growth, 0xf7e6u, 0x81f4edu, 2u))
        return GrowthUnwound(&growth);
    cpu->carry = 1;
    return ExecutionReturned(0x81f7eau);
}

/* $81:F7ED: seven stat gains, preserving the original calibration BRK. */
Lufia2ExecutionResult Lufia2PartyStatGrowth(const Lufia2Memory *memory,
                                            Lufia2CpuState *cpu,
                                            Lufia2PushedChildCall child,
                                            void *context) {
    GrowthContext growth = {memory, cpu, child, context, 0u};
    OpPushX(memory, cpu);
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x97u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, 0x0eu));
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, 0x09feu));
    OpSepWidths(cpu, 0x20u);
    if (!GrowthCall(&growth, 0xf800u, 0x81f87fu, 3u))
        return GrowthUnwound(&growth);
    OpLdy(cpu, 6u);
    do {
        OpLoadA(cpu, 5u);
        if (!GrowthCall(&growth, 0xf809u, 0x808299u, 3u))
            return GrowthUnwound(&growth);
        OpSta(memory, cpu, OpAbsY(cpu, 0x0a38u));
        OpDey(cpu);
    } while (!cpu->negative);
    OpStz(memory, cpu, OpAbs(cpu, 0x0a3du));
    OpRepWidths(cpu, 0x20u);
    OpTxa(cpu);
    cpu->carry = 0;
    OpAdcValue(cpu, 0x51u);
    OpSta(memory, cpu, OpDp(cpu, 0xb2u));
    OpLdy(cpu, 0u);
    OpTyx(cpu);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, 0x0a38u));
        OpAndValue(cpu, 0xffu);
        cpu->carry = 0;
        OpAdc(memory, cpu, OpAbsY(cpu, 0x0a1cu));
        cpu->carry = 1;
        OpSbcValue(cpu, OpReadM(memory, cpu, GrowthStat(memory, cpu)));
        if (!cpu->carry) {
            OpSepWidths(cpu, 0x20u);
            OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
            OpIncA(cpu);
            cpu->carry = 0;
            OpAdc(memory, cpu, OpAbsX(cpu, 0x0a38u));
            OpCmpValue(cpu, 5u);
            if (cpu->carry) {
                cpu->resume_pc = 0x81f87eu;
                return (Lufia2ExecutionResult){LUFIA2_EXECUTION_BOUNDARY, 0x81f87eu,
                                               0u};
            }
            OpSta(memory, cpu, OpAbsX(cpu, 0x0a38u));
            OpRepWidths(cpu, 0x20u);
        }
        OpLda(memory, cpu, OpAbsY(cpu, 0x0a1cu));
        if (cpu->zero)
            OpStz(memory, cpu, OpAbsX(cpu, 0x0a38u));
        OpInx(cpu);
        OpIny(cpu);
        OpIny(cpu);
        Compare16(cpu, cpu->x, 7u);
    } while (!cpu->zero);
    OpLdy(cpu, 0u);
    OpTyx(cpu);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, 0x0a38u));
        OpAndValue(cpu, 0xffu);
        cpu->carry = 0;
        OpAdc(memory, cpu, OpAbsY(cpu, 0x0a1cu));
        cpu->carry = 1;
        OpSbcValue(cpu, OpReadM(memory, cpu, GrowthStat(memory, cpu)));
        /* SEP/REP preserves the high byte used by the following ADC. */
        OpSepWidths(cpu, 0x20u);
        OpSta(memory, cpu, OpAbsX(cpu, 0x0a38u));
        OpRepWidths(cpu, 0x20u);
        cpu->carry = 0;
        OpAdcValue(cpu, OpReadM(memory, cpu, GrowthStat(memory, cpu)));
        OpSta(memory, cpu, GrowthStat(memory, cpu));
        OpInx(cpu);
        OpIny(cpu);
        OpIny(cpu);
        Compare16(cpu, cpu->x, 7u);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    PullDataBank(memory, cpu);
    OpPullX(memory, cpu);
    return ExecutionReturned(0x81f87du);
}
