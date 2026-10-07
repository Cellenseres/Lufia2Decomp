#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/dp_scratch.h"
#include "system/wram.h"


typedef Lufia2ExecutionResult (*SceneRecordStep)(
    const Lufia2Memory *, Lufia2CpuState *);

static bool SceneRecordContext(const Lufia2CpuState *cpu, bool byte_input) {
    return cpu->program_bank == 0x80u &&
        cpu->accumulator_is_8_bit == byte_input && !cpu->index_is_8_bit &&
        !cpu->decimal && !cpu->direct_page &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static bool SceneRecordCall(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    SceneRecordStep step, uint16_t back) {
    SimulateJsrFrame(memory, cpu, back);
    Lufia2ExecutionResult result = step(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return false;
    uint8_t low = Pull8(memory, cpu);
    uint8_t high = Pull8(memory, cpu);
    uint16_t actual = (uint16_t)(low | ((uint16_t)high << 8));
    if (actual == back)
        return true;
    cpu->resume_pc = 0x800000u | (uint16_t)(actual + 1u);
    return false;
}

Lufia2ExecutionResult Lufia2SceneScriptReadWord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!SceneRecordContext(cpu, true))
        return ExecutionHandoff(cpu, 0x80c0d0u);
    if (!SceneRecordCall(memory, cpu, Lufia2SceneScriptReadOperand, 0xc0d2u))
        return ExecutionHandoff(cpu, cpu->resume_pc);
    PushAccumulator8(memory, cpu);
    if (!SceneRecordCall(memory, cpu, Lufia2SceneScriptReadOperand, 0xc0d6u))
        return ExecutionHandoff(cpu, cpu->resume_pc);
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x80c0d9u);
}

Lufia2ExecutionResult Lufia2SceneScriptSeekRelative(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!SceneRecordContext(cpu, false))
        return ExecutionHandoff(cpu, 0x80c102u);
    OpOraValue(cpu, 0u);
    if (cpu->negative) {
        OpTay(cpu);
        return ExecutionReturned(0x80c108u);
    }
    Push8(memory, cpu, PackStatus(cpu));
    cpu->carry = 0;
    OpAdcValue(cpu, 0x8000u);
    OpTay(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_SCENE_SCRIPT_BANK));
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_SCENE_SCRIPT_BANK));
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x80c11bu);
}

static bool SceneRecordReadOffset(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!SceneRecordCall(memory, cpu, Lufia2SceneScriptReadWord, 0xc16cu))
        return false;
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_C));
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_D));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_SCENE_RECORD_BANK));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_SCENE_SCRIPT_BANK));
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DP_SCRATCH_C));
    cpu->carry = 0;
    OpAdc(memory, cpu, OpAbs(cpu, WRAM_SCENE_RECORD_BASE));
    if (!SceneRecordCall(memory, cpu, Lufia2SceneScriptSeekRelative, 0xc184u))
        return false;
    OpSepWidths(cpu, 0x20u);
    cpu->carry = 0;
    return true;
}

Lufia2ExecutionResult Lufia2SceneScriptFindRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!SceneRecordContext(cpu, true) || cpu->stack < 0x1f10u)
        return ExecutionHandoff(cpu, 0x80c12eu);
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    PushDataBank(memory, cpu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_SCENE_RECORD_BANK));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_SCENE_SCRIPT_BANK));
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpTxa(cpu);
    cpu->carry = 0;
    OpAdc(memory, cpu, OpAbs(cpu, WRAM_SCENE_RECORD_BASE));
    if (!SceneRecordCall(memory, cpu, Lufia2SceneScriptSeekRelative, 0xc142u))
        return ExecutionHandoff(cpu, cpu->resume_pc);
    OpLda(memory, cpu, WRAM_SCENE_CURSOR_STATE);
    PushAccumulator16(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    if (!SceneRecordCall(memory, cpu, Lufia2SceneScriptReadWord, 0xc14cu))
        return ExecutionHandoff(cpu, cpu->resume_pc);
    OpRepWidths(cpu, 0x20u);
    cpu->carry = 0;
    OpAdc(memory, cpu, OpAbs(cpu, WRAM_SCENE_RECORD_BASE));
    OpTay(cpu);
    PullAccumulator16(memory, cpu);
    OpSta(memory, cpu, WRAM_SCENE_CURSOR_STATE);
    OpTya(cpu);
    if (!SceneRecordCall(memory, cpu, Lufia2SceneScriptSeekRelative, 0xc15cu))
        return ExecutionHandoff(cpu, cpu->resume_pc);
    OpSepWidths(cpu, 0x20u);
    for (;;) {
        if (!SceneRecordCall(memory, cpu, Lufia2SceneScriptReadOperand, 0xc161u))
            return ExecutionHandoff(cpu, cpu->resume_pc);
        OpCmpValue(cpu, 0xffu);
        if (cpu->zero) {
            OpLdy(cpu, 0xffffu);
            cpu->carry = 1;
            break;
        }
        OpCmp(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
        if (cpu->zero) {
            if (!SceneRecordReadOffset(memory, cpu))
                return ExecutionHandoff(cpu, cpu->resume_pc);
            break;
        }
        if (!SceneRecordCall(memory, cpu, Lufia2SceneScriptReadWord, 0xc18cu))
            return ExecutionHandoff(cpu, cpu->resume_pc);
    }
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x80c194u);
}
