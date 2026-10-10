#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/battle.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum {
    DP_EFFECT_STREAM = 0xc3u,
    DP_VECTOR_ANGLE = 0x54u,
    DP_VECTOR_HORIZONTAL = 0x56u,
    DP_VECTOR_VERTICAL = 0x58u,
    DP_VECTOR_SPEED = 0x5au,
    ACTOR_SCRIPT = 3u,
    ACTOR_HORIZONTAL_VELOCITY = 0x1bu,
    ACTOR_VERTICAL_VELOCITY = 0x1du,
    ACTOR_ATTRIBUTES = 0x23u,
    ACTOR_ATTRIBUTES_COPY = 0x24u,
    ACTOR_ORDER = 0x25u,
    ACTOR_DIRTY = 0x26u
};

static Lufia2ExecutionResult AllocateActor(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpSetDataBank(memory, cpu, 0x7eu);
    SimulateJsrFrame(memory, cpu, 0xa2b6u);
    const Lufia2ExecutionResult result =
        Lufia2BattleEffectFindActorSlot(memory, cpu);
    if (result.flow == LUFIA2_EXECUTION_RETURNED)
        SimulateRtsFrame(memory, cpu);
    return result;
}

static void ReadActorVector(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbsX(cpu, 0u));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_EFFECT_STREAM));
    OpStepMem(memory, cpu, OpDp(cpu, DP_EFFECT_STREAM), 1);
    OpStepMem(memory, cpu, OpDp(cpu, DP_EFFECT_STREAM), 1);
    OpSta(memory, cpu, OpAbsX(cpu, ACTOR_SCRIPT));
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_EFFECT_STREAM));
    OpStepMem(memory, cpu, OpDp(cpu, DP_EFFECT_STREAM), 1);
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, DP_VECTOR_ANGLE));
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_EFFECT_STREAM));
    OpStepMem(memory, cpu, OpDp(cpu, DP_EFFECT_STREAM), 1);
    OpStepMem(memory, cpu, OpDp(cpu, DP_EFFECT_STREAM), 1);
    OpSta(memory, cpu, OpDp(cpu, DP_VECTOR_SPEED));
    OpSepWidths(cpu, 0x20u);
}

static void InitializeActor(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const uint16_t counters[] = {0u, 1u, 2u, 7u, 10u, 13u, 16u};

    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DP_VECTOR_HORIZONTAL));
    OpSta(memory, cpu, OpAbsX(cpu, ACTOR_HORIZONTAL_VELOCITY));
    OpLda(memory, cpu, OpDp(cpu, DP_VECTOR_VERTICAL));
    OpSta(memory, cpu, OpAbsX(cpu, ACTOR_VERTICAL_VELOCITY));
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_EFFECT_STREAM));
    OpStepMem(memory, cpu, OpDp(cpu, DP_EFFECT_STREAM), 1);
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpAbsX(cpu, ACTOR_ATTRIBUTES));
    OpSta(memory, cpu, OpAbsX(cpu, ACTOR_ATTRIBUTES_COPY));
    OpLoadA(cpu, 1u);
    for (unsigned i = 0u; i < sizeof(counters) / sizeof(counters[0]); ++i)
        OpSta(memory, cpu, OpAbsX(cpu, counters[i]));
    OpLoadA(cpu, 0x40u);
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu,
        OpAbs(cpu, WRAM_BATTLE_EFFECT_SLOT_COUNTER)));
    OpSta(memory, cpu, OpAbsX(cpu, ACTOR_ORDER));
    OpStz(memory, cpu, OpAbsX(cpu, ACTOR_DIRTY));
    OpStepMem(memory, cpu, OpAbs(cpu, WRAM_BATTLE_EFFECT_ACTIVE_COUNT), 1);
}

Lufia2ExecutionResult Lufia2BattleEffectSpawnVectorActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f02u || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x81a2b0u);
    const Lufia2ExecutionResult result = AllocateActor(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    ReadActorVector(memory, cpu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x81a2dau, 0x85dd63u, 3u, 0x81u)) {
        const Lufia2ExecutionResult unwind = {LUFIA2_EXECUTION_CHILD_UNWOUND,
                                              0x81a2dau, 0u};
        return unwind;
    }
    InitializeActor(memory, cpu);
    return ExecutionReturned(0x81a31cu);
}
