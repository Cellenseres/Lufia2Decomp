#include "core/cpu_ops.h"
#include "field/event_script_internal.h"
#include "actor/actor_internal.h"
#include "lufia2/actor.h"
#include "lufia2/field.h"
#include "system/dp_scratch.h"
typedef Lufia2ExecutionResult (*EventRefreshStep)(
    const Lufia2Memory *, Lufia2CpuState *);

typedef struct EventRefreshActors {
    const Lufia2Memory *memory;
    uint32_t handoff;
} EventRefreshActors;

static uint8_t EventRefreshActorChild(
    void *context, Lufia2CpuState *cpu, uint32_t target, uint32_t site) {
    EventRefreshActors *actors = context;
    SimulateJsrFrame(actors->memory, cpu, (uint16_t)(site + 2u));
    actors->handoff = target;
    cpu->resume_pc = target;
    return 0;
}

static Lufia2ExecutionResult EventRefreshActorSlots(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    EventRefreshActors actors = {memory, 0};
    Lufia2ExecutionResult result = Lufia2UpdateActorSlots(
        memory, cpu, EventRefreshActorChild, &actors);
    if (actors.handoff)
        return ExecutionHandoff(cpu, actors.handoff);
    return result;
}

static uint8_t EventRefreshCall(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    EventRefreshStep step, uint16_t back, uint32_t *handoff) {
    SimulateJslFrame(memory, cpu, 0x80u, back);
    cpu->program_bank = 0x83u;
    Lufia2ExecutionResult result = step(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED) {
        *handoff = result.pc;
        return 0;
    }
    uint8_t low = Pull8(memory, cpu);
    uint8_t high = Pull8(memory, cpu);
    uint8_t bank = Pull8(memory, cpu);
    uint16_t actual = (uint16_t)(low | ((uint16_t)high << 8));
    cpu->program_bank = bank;
    if (actual != back || bank != 0x80u) {
        *handoff = ((uint32_t)bank << 16) | (uint16_t)(actual + 1u);
        cpu->resume_pc = *handoff;
        return 0;
    }
    return 1;
}

static unsigned EventRefreshParty(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t *handoff) {
    PushY(memory, cpu);
    OpSepWidths(cpu, 0x30u);
    OpLda(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT));
    PushAccumulator8(memory, cpu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_STATE));
    PushAccumulator8(memory, cpu);
    OpLoadA(cpu, 1u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_ACTOR_STATE), 1);
    if (!EventRefreshCall(memory, cpu, EventRefreshActorSlots, 0xd5ceu, handoff) ||
        !EventRefreshCall(memory, cpu, Lufia2FieldActorSprites, 0xd5d2u, handoff))
        return EVENT_OPCODE_HANDOFF;
    LoadA8(cpu, Pull8(memory, cpu));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_ACTOR_STATE));
    LoadA8(cpu, Pull8(memory, cpu));
    OpSta(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT));
    if (!EventRefreshCall(memory, cpu, Lufia2ActorSetRecordOffsets,
            0xd5ddu, handoff))
        return EVENT_OPCODE_HANDOFF;
    OpRepWidths(cpu, 0x10u);
    OpPullY(memory, cpu);
    return EVENT_OPCODE_NEXT;
}

static unsigned EventCenterLayers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t *handoff) {
    TransferDirectToA(cpu);
    Lufia2EventNextByte(memory, cpu, 0xdba0u);
    cpu->carry = 0;
    OpAdc(memory, cpu, OpAbs(cpu, WRAM_ACTOR_TILE_X));
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_C));
    OpStz(memory, cpu, OpDp(cpu, DP_SCRATCH_C + 1u));
    Lufia2EventNextByte(memory, cpu, 0xdbabu);
    cpu->carry = 0;
    OpAdc(memory, cpu, OpAbs(cpu, WRAM_ACTOR_TILE_Y));
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_E));
    OpStz(memory, cpu, OpDp(cpu, DP_SCRATCH_E + 1u));
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    if (!EventRefreshCall(memory, cpu, Lufia2FieldSetupLayerScroll,
            0xdbb9u, handoff) ||
        !EventRefreshCall(memory, cpu, Lufia2FieldActorSprites,
            0xdbbdu, handoff))
        return EVENT_OPCODE_HANDOFF;
    OpRepWidths(cpu, 0x10u);
    OpLoadA(cpu, 8u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_SCREEN_EFFECTS), 1);
    PullDataBank(memory, cpu);
    OpPullY(memory, cpu);
    return EVENT_OPCODE_NEXT;
}

unsigned Lufia2EventRefreshOpcode(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t handler, uint32_t *handoff) {
    if (handler == EVENT_OP_REFRESH_PARTY_CLEAR_MODE) {
        OpLoadA(cpu, 1u);
        OpTestBits(memory, cpu, OpDp(cpu, 0x72u), 0);
        return EventRefreshParty(memory, cpu, handoff);
    }
    if (handler == EVENT_OP_REFRESH_PARTY)
        return EventRefreshParty(memory, cpu, handoff);
    if (handler == EVENT_OP_CENTER_LAYERS)
        return EventCenterLayers(memory, cpu, handoff);
    *handoff = 0x800000u | handler;
    return EVENT_OPCODE_HANDOFF;
}
