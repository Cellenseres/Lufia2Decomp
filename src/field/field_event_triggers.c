#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    EVENT_RECORD_STORAGE = 0x7ef000,
    EVENT_SLOT_PROBE_X = 0x7fd17c,
    EVENT_SLOT_PROBE_Y = 0x7fd184,
    EVENT_REDRAW_REQUEST = 0x05b5,
    EVENT_SELECTED_SLOT = 0x54,
    EVENT_PROBE_X = 0x8f,
    EVENT_PROBE_Y = 0x91
};

static bool EventTriggerContext(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit &&
        !cpu->decimal && !cpu->direct_page && cpu->program_bank == 0x80u &&
        cpu->stack >= 0x1f10u && cpu->stack <= 0x1ffcu;
}

static Lufia2ExecutionResult EventTriggerCall(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t bank, uint16_t back,
    Lufia2ExecutionResult (*helper)(const Lufia2Memory *, Lufia2CpuState *)) {
    SimulateJslFrame(memory, cpu, 0x80u, back);
    cpu->program_bank = bank;
    Lufia2ExecutionResult result = helper(memory, cpu);
    if (result.flow == LUFIA2_EXECUTION_RETURNED) {
        SimulateRtlFrame(memory, cpu);
        cpu->program_bank = 0x80u;
    }
    return result;
}

Lufia2ExecutionResult Lufia2FieldStartEventAtProbe(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;

    if (!EventTriggerContext(cpu))
        return ExecutionHandoff(cpu, 0x80e7fau);
    PushY(memory, cpu);
    OpLdy(cpu, 3u);
    result = EventTriggerCall(memory, cpu, 0x83u, 0xe801u,
        Lufia2FieldFindPointRecord);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpPullY(memory, cpu);
    if (cpu->carry) {
        TransferDirectToA(cpu);
        OpLda(memory, cpu, OpLongX(cpu, EVENT_RECORD_STORAGE));
        OpTyx(cpu);
        OpAslA(cpu);
        OpTay(cpu);
        result = EventTriggerCall(memory, cpu, 0x80u, 0xe810u,
            Lufia2FieldStartEvent);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
        OpLoadA(cpu, 0x14u);
        OpTestBits(memory, cpu, OpAbs(cpu, EVENT_REDRAW_REQUEST), 1u);
    }
    return ExecutionReturned(0x80e816u);
}

Lufia2ExecutionResult Lufia2FieldStartPositionEvent(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;

    if (!EventTriggerContext(cpu))
        return ExecutionHandoff(cpu, 0x80e7dfu);
    PushDataBank(memory, cpu);
    OpLdx(cpu, 0u);
    OpLdy(cpu, 6u);
    result = EventTriggerCall(memory, cpu, 0x80u, 0xe7e9u,
        Lufia2FieldStartEvent);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, EVENT_SELECTED_SLOT)));
    OpLda(memory, cpu, OpDp(cpu, EVENT_PROBE_X));
    OpSta(memory, cpu, OpLongX(cpu, EVENT_SLOT_PROBE_X));
    OpLda(memory, cpu, OpDp(cpu, EVENT_PROBE_Y));
    OpSta(memory, cpu, OpLongX(cpu, EVENT_SLOT_PROBE_Y));
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x80e7f9u);
}
