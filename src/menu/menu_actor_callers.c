#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "lufia2/menu.h"
#include "system/wram.h"

enum {
    MENU_ACTOR_SLOT = 0xa7u,
    MENU_ACTOR_STATE = WRAM_ACTOR_STATE,
    MENU_AUXILIARY_STATE = WRAM_UNK_7E14D4,
    MENU_AUXILIARY_FLAGS = WRAM_UNK_7E14E6
};

static Lufia2ExecutionResult MenuActorChildUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2FieldSelectMenuActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x8eu || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x8ebc99u);
    OpLdx(cpu, 7u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, MENU_ACTOR_STATE));
        OpBitValue(cpu, 4u);
        if (!cpu->zero)
            break;
        OpDex(cpu);
    } while (!cpu->zero);
    OpWriteX(memory, cpu, OpDp(cpu, MENU_ACTOR_SLOT), cpu->x);
    if (!CallChildWithFrame(memory, cpu, child, context,
        0x8ebca8u, 0x83ab4fu, 3u, 0x8eu))
        return MenuActorChildUnwound(0x8ebca8u);
    return ExecutionReturned(0x8ebcacu);
}

Lufia2ExecutionResult Lufia2MenuInitializeAuxiliarySprites(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x82u || !cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x8289a4u);
    OpSepWidths(cpu, 0x10u);
    OpLdy(cpu, 10u);
    OpLdx(cpu, 5u);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, MENU_AUXILIARY_STATE));
        OpStz(memory, cpu, OpAbsX(cpu, MENU_AUXILIARY_FLAGS));
        if (!CallChildWithFrame(memory, cpu, child, context,
            0x8289b0u, 0x868cdau, 3u, 0x82u))
            return MenuActorChildUnwound(0x8289b0u);
        OpInx(cpu);
        OpCpx(cpu, 16u);
    } while (!cpu->zero);
    OpRepWidths(cpu, 0x10u);
    return ExecutionReturned(0x8289bbu);
}
