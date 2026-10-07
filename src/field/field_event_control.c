#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"
#include "text/text_internal.h"

enum {
    FIELD_EVENT_REQUEST = 0x120au,
    TEXT_SPEAKER_ACTOR = 0x09abu,
    TEXT_SPEAKER_REQUEST = 0x09acu
};

static bool EventControlContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x83u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && !cpu->direct_page &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

Lufia2ExecutionResult Lufia2FieldPrepareEventControl(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EventControlContext(cpu))
        return ExecutionHandoff(cpu, 0x83bb76u);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_TEXT_STATE));
    OpStz(memory, cpu, OpAbs(cpu, TEXT_CALLER_BANK));
    OpStz(memory, cpu, OpAbs(cpu, TEXT_RETURN_BANK));
    OpStz(memory, cpu, OpAbs(cpu, TEXT_WINDOW_TILE_BASE));
    OpStz(memory, cpu, OpAbs(cpu, TEXT_WINDOW_TILE_BASE + 1u));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, WRAM_FIELD_CONTROL_CHANGE_PENDING);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, TEXT_PRINT_COUNTDOWN);
    return ExecutionReturned(0x83bb92u);
}

static bool EventControlCall(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2ExecutionResult (*helper)(const Lufia2Memory *, Lufia2CpuState *),
    uint8_t bank, uint16_t back, bool far_call) {
    if (far_call)
        SimulateJslFrame(memory, cpu, 0x83u, back);
    else
        SimulateJsrFrame(memory, cpu, back);
    cpu->program_bank = bank;
    Lufia2ExecutionResult result = helper(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return false;
    uint8_t low = Pull8(memory, cpu);
    uint8_t high = Pull8(memory, cpu);
    uint16_t actual = (uint16_t)(low | ((uint16_t)high << 8));
    uint8_t actual_bank = far_call ? Pull8(memory, cpu) : 0x83u;
    cpu->program_bank = actual_bank;
    if (actual == back && actual_bank == 0x83u)
        return true;
    cpu->resume_pc = ((uint32_t)actual_bank << 16) | (uint16_t)(actual + 1u);
    return false;
}

Lufia2ExecutionResult Lufia2FieldBeginEventControl(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EventControlContext(cpu) || cpu->stack < 0x1f30u)
        return ExecutionHandoff(cpu, 0x83b727u);
    OpSta(memory, cpu, FIELD_EVENT_REQUEST);
    if (!EventControlCall(memory, cpu, Lufia2SceneScriptFindRecord,
        0x80u, 0xb72eu, true))
        return ExecutionHandoff(cpu, cpu->resume_pc);
    OpWriteX(memory, cpu, OpAbs(cpu, TEXT_SCRIPT_POINTER), cpu->y);
    if (!EventControlCall(memory, cpu, Lufia2FieldPrepareEventControl,
        0x83u, 0xb734u, false))
        return ExecutionHandoff(cpu, cpu->resume_pc);
    OpLoadA(cpu, 1u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_ACTOR_STATE), 1u);
    if (!EventControlCall(memory, cpu, Lufia2FieldMarkObjectSlots,
        0x80u, 0xb73du, true))
        return ExecutionHandoff(cpu, cpu->resume_pc);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, TEXT_SPEAKER_REQUEST));
    OpSta(memory, cpu, OpAbs(cpu, TEXT_SPEAKER_ACTOR));
    return ExecutionReturned(0x83b746u);
}
