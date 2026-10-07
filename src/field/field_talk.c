#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    DP_TALK_X = 0x8fu,
    DP_TALK_Y = 0x91u,
    DP_TALK_HEIGHT = 0x54u,
    TALK_BLOCKED = 0x01u,
    TALK_VERTICAL_STEP = 0x10u,
    TALK_HORIZONTAL_STEP = 0x20u,
    ROM_TALK_DIRECTIONS = 0xba54u
};

static uint8_t TalkContext(const Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, uint8_t any_index_width) {
    return cpu->program_bank == 0x83u && cpu->accumulator_is_8_bit &&
        (any_index_width || !cpu->index_is_8_bit) && !cpu->decimal && child;
}

static Lufia2ExecutionResult TalkUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static uint8_t TalkCall(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t site, uint32_t target) {
    return CallChildWithFrame(memory, cpu, child, context,
        site, target, 2u, 0x83u);
}

static uint8_t TalkAttributes(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t site) {
    if (!TalkCall(memory, cpu, child, context, site, 0x83f9adu))
        return 0u;
    OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_MAP_ATTRIBUTES));
    return 1u;
}

static Lufia2ExecutionResult FinishTalkProbe(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    if (!TalkAttributes(memory, cpu, child, context, 0x83ba76u))
        return TalkUnwound(0x83ba76u);
    OpAndValue(cpu, TALK_BLOCKED);
    return ExecutionReturned(0x83ba7fu);
}

Lufia2ExecutionResult Lufia2FieldProbeTalkBlocked(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    if (!TalkContext(cpu, child, 0u))
        return ExecutionHandoff(cpu, 0x83ba76u);
    return FinishTalkProbe(memory, cpu, child, context);
}

Lufia2ExecutionResult Lufia2FieldProbeTalkDown(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    if (!TalkContext(cpu, child, 0u))
        return ExecutionHandoff(cpu, 0x83ba5cu);
    OpStepMem(memory, cpu, OpDp(cpu, DP_TALK_Y), 1);
    if (!TalkAttributes(memory, cpu, child, context, 0x83ba5eu))
        return TalkUnwound(0x83ba5eu);
    OpBitValue(cpu, TALK_BLOCKED);
    if (!cpu->zero)
        return ExecutionReturned(0x83ba7fu);
    OpBitValue(cpu, TALK_VERTICAL_STEP);
    if (!cpu->zero) {
        OpStepMem(memory, cpu, OpDp(cpu, DP_TALK_Y), 1);
        if (!TalkCall(memory, cpu, child, context, 0x83ba6fu, 0x83ba76u))
            return TalkUnwound(0x83ba6fu);
        if (!cpu->zero)
            return ExecutionReturned(0x83bac1u);
        OpStepMem(memory, cpu, OpDp(cpu, DP_TALK_Y), 1);
    }
    return FinishTalkProbe(memory, cpu, child, context);
}

static Lufia2ExecutionResult ProbeTalkBackward(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context,
    uint32_t start, uint8_t coordinate, uint8_t step_mask) {
    if (!TalkContext(cpu, child, 0u))
        return ExecutionHandoff(cpu, start);
    if (!TalkAttributes(memory, cpu, child, context, start))
        return TalkUnwound(start);
    OpAndValue(cpu, step_mask);
    if (!cpu->zero) {
        OpStepMem(memory, cpu, OpDp(cpu, coordinate), -1);
        const uint32_t site = start + 13u;
        if (!TalkCall(memory, cpu, child, context, site, 0x83ba76u))
            return TalkUnwound(site);
        if (!cpu->zero)
            return ExecutionReturned(0x83bac1u);
    }
    OpStepMem(memory, cpu, OpDp(cpu, coordinate), -1);
    return FinishTalkProbe(memory, cpu, child, context);
}

Lufia2ExecutionResult Lufia2FieldProbeTalkLeft(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    return ProbeTalkBackward(memory, cpu, child, context,
        0x83ba80u, DP_TALK_X, TALK_HORIZONTAL_STEP);
}

Lufia2ExecutionResult Lufia2FieldProbeTalkUp(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    return ProbeTalkBackward(memory, cpu, child, context,
        0x83ba96u, DP_TALK_Y, TALK_VERTICAL_STEP);
}

Lufia2ExecutionResult Lufia2FieldProbeTalkRight(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    if (!TalkContext(cpu, child, 0u))
        return ExecutionHandoff(cpu, 0x83baacu);
    OpStepMem(memory, cpu, OpDp(cpu, DP_TALK_X), 1);
    if (!TalkAttributes(memory, cpu, child, context, 0x83baaeu))
        return TalkUnwound(0x83baaeu);
    OpBitValue(cpu, TALK_BLOCKED);
    if (!cpu->zero)
        return ExecutionReturned(0x83bac1u);
    OpBitValue(cpu, TALK_HORIZONTAL_STEP);
    if (cpu->zero)
        return ExecutionReturned(0x83bac1u);
    OpStepMem(memory, cpu, OpDp(cpu, DP_TALK_X), 1);
    return FinishTalkProbe(memory, cpu, child, context);
}

Lufia2ExecutionResult Lufia2FieldProbeTalkTarget(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    if (!TalkContext(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x83ba06u);
    OpRepWidths(cpu, 0x10u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_TILE_X));
    OpSta(memory, cpu, OpDp(cpu, DP_TALK_X));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_TILE_Y));
    OpSta(memory, cpu, OpDp(cpu, DP_TALK_Y));
    if (!TalkCall(memory, cpu, child, context, 0x83ba12u, 0x83f988u))
        return TalkUnwound(0x83ba12u);
    OpSta(memory, cpu, OpDp(cpu, DP_TALK_HEIGHT));
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_FACING));
    OpTax(cpu);
    const uint32_t direction = JumpProgramTable(memory, cpu, ROM_TALK_DIRECTIONS);
    if (!TalkCall(memory, cpu, child, context, 0x83ba1cu, direction))
        return TalkUnwound(0x83ba1cu);
    cpu->carry = 0u;
    if (cpu->zero)
        return ExecutionReturned(0x83ba53u);
    if (!TalkCall(memory, cpu, child, context, 0x83ba22u, 0x83bac2u))
        return TalkUnwound(0x83ba22u);
    if (!cpu->carry)
        return ExecutionReturned(0x83ba53u);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E05D2));
    OpCmpValue(cpu, 0x70u);
    if (cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_WINDOW_MODE));
        OpBitValue(cpu, 1u);
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_FACING));
            OpCmpValue(cpu, 4u);
            cpu->carry = cpu->zero;
            return ExecutionReturned(0x83ba53u);
        }
    }
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_X));
    OpSta(memory, cpu, OpDp(cpu, DP_TALK_X));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_Y));
    OpSta(memory, cpu, OpDp(cpu, DP_TALK_Y));
    if (!TalkCall(memory, cpu, child, context, 0x83ba4au, 0x83f988u))
        return TalkUnwound(0x83ba4au);
    OpCmp(memory, cpu, OpDp(cpu, DP_TALK_HEIGHT));
    cpu->carry = cpu->zero;
    return ExecutionReturned(0x83ba53u);
}
