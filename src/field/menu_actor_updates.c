#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    MENU_UPDATE_SLOT = 0xa7u,
    MENU_UPDATE_RECORD = 0xabu,
    MENU_UPDATE_X = 0x8fu,
    MENU_UPDATE_Y = 0x91u,
    MENU_UPDATE_ARGUMENT = 0x97u,
    MENU_UPDATE_ACTOR_STATE = WRAM_ACTOR_STATE,
    MENU_UPDATE_ACTOR_ID = WRAM_ACTOR_ID,
    MENU_UPDATE_TEXT_WAIT = WRAM_TEXT_WAIT_ACTOR,
    MENU_UPDATE_BUFFER_OFFSET = WRAM_MENU_ACTOR_UPDATE_BUFFER & 0xffffu
};

static Lufia2ExecutionResult MenuActorUpdateUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static void MenuActorWriteUpdate(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLoadA(cpu, 0x12u);
    OpSta(memory, cpu, OpLongX(cpu, 0x7e0000u));
    OpLda(memory, cpu, OpDp(cpu, MENU_UPDATE_X));
    OpSta(memory, cpu, OpLongX(cpu, 0x7e0001u));
    OpLda(memory, cpu, OpDp(cpu, MENU_UPDATE_Y));
    OpSta(memory, cpu, OpLongX(cpu, 0x7e0002u));
    OpLda(memory, cpu, OpDp(cpu, MENU_UPDATE_ARGUMENT));
    OpSta(memory, cpu, OpLongX(cpu, 0x7e0003u));
}

static void MenuActorAttachUpdate(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpPushX(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpTxa(cpu);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, MENU_UPDATE_RECORD)));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_PRIMARY_SCRIPT));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_PRIMARY_SCRIPT + 2u));
    TransferDirectToA(cpu);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, MENU_UPDATE_SLOT)));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_PRIMARY_TIMER));
    OpLda(memory, cpu, OpAbsX(cpu, MENU_UPDATE_ACTOR_STATE));
    OpAndValue(cpu, 0xfeu);
    OpOraValue(cpu, 8u);
    OpSta(memory, cpu, OpAbsX(cpu, MENU_UPDATE_ACTOR_STATE));
    OpPullX(memory, cpu);
    OpStz(memory, cpu, OpAbs(cpu, MENU_UPDATE_TEXT_WAIT));
    for (unsigned byte = 0u; byte < 4u; ++byte)
        OpInx(cpu);
}

Lufia2ExecutionResult Lufia2FieldQueueMenuActorUpdates(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->y >= 5u)
        return ExecutionHandoff(cpu, 0x80b404u);
    OpLdx(cpu, MENU_UPDATE_BUFFER_OFFSET);
    do {
        OpLda(memory, cpu, OpAbsY(cpu, MENU_UPDATE_ACTOR_STATE));
        OpBitValue(cpu, 4u);
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbsY(cpu, MENU_UPDATE_ACTOR_ID));
            OpCmpValue(cpu, 16u);
            if (!cpu->carry) {
                OpWriteX(memory, cpu, OpDp(cpu, MENU_UPDATE_SLOT), cpu->y);
                if (!CallChildWithFrame(memory, cpu, child, context,
                        0x80b417u, 0x8482d5u, 3u, 0x80u))
                    return MenuActorUpdateUnwound(0x80b417u);
                OpPushX(memory, cpu);
                if (!CallChildWithFrame(memory, cpu, child, context,
                        0x80b41cu, 0x83fa12u, 3u, 0x80u))
                    return MenuActorUpdateUnwound(0x80b41cu);
                OpPullX(memory, cpu);
                MenuActorWriteUpdate(memory, cpu);
                MenuActorAttachUpdate(memory, cpu);
            }
        }
        OpIny(cpu);
        OpCpy(cpu, 5u);
    } while (!cpu->zero);
    return ExecutionReturned(0x80b46au);
}
