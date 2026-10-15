#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "lufia2/battle.h"
#include "system/wram.h"

enum {
    DP_ENCOUNTER_SLOT = 0u,
    ROM_MAP_ENCOUNTERS = 0xb9acu,
    CAVE_ENCOUNTERS = 0x7ff966u,
    ANCIENT_CAVE_MAP = 0xf0u,
    ENCOUNTER_SLOT_COUNT = 32u,
    ENCOUNTER_RECORD_BYTES = 10u
};

static void SelectFieldEncounter(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSta(memory, cpu, OpDp(cpu, DP_ENCOUNTER_SLOT));
    OpStz(memory, cpu, OpDp(cpu, DP_ENCOUNTER_SLOT + 1u));
    OpLoadA(cpu, 0x97u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, DP_ENCOUNTER_SLOT));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
    OpLoadA(cpu, ENCOUNTER_RECORD_BYTES);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
    LoadA16(cpu, cpu->direct_page);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID));
    OpCmpValue(cpu, ANCIENT_CAVE_MAP);
    if (cpu->zero) {
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ENCOUNTER_SLOT)));
        OpLda(memory, cpu, OpLongX(cpu, CAVE_ENCOUNTERS));
    } else {
        OpRepWidths(cpu, 0x20u);
        OpAslA(cpu);
        OpTax(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, ROM_MAP_ENCOUNTERS));
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpDp(cpu, DP_ENCOUNTER_SLOT));
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbsX(cpu, ROM_MAP_ENCOUNTERS));
    }
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, SNES_RDMPYL)));
}

Lufia2ExecutionResult Lufia2FieldGenerateEncounter(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page ||
        cpu->stack < 0x1f20u || cpu->stack > 0x1ffcu ||
        A8(cpu) >= ENCOUNTER_SLOT_COUNT || !child)
        return ExecutionHandoff(cpu, 0x818351u);
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    OpRepWidths(cpu, 0x30u);
    PushAccumulator16(memory, cpu);
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpCmpValue(cpu, ENCOUNTER_SLOT_COUNT);
    SelectFieldEncounter(memory, cpu);
    if (!CallChildWithFrame(memory, cpu, child, context, 0x818394u, 0x81839fu, 2u,
                            0x81u)) {
        const Lufia2ExecutionResult result = {LUFIA2_EXECUTION_CHILD_UNWOUND, 0x818394u,
                                              0u};
        return result;
    }
    OpRepWidths(cpu, 0x30u);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    PullAccumulator16(memory, cpu);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x81839eu);
}
