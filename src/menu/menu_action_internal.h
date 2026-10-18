#ifndef LUFIA2_MENU_ACTION_INTERNAL_H
#define LUFIA2_MENU_ACTION_INTERNAL_H

#include "core/child_call.h"
#include "core/cpu_ops.h"

typedef struct MenuActionCall {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
} MenuActionCall;

static inline bool MenuActionReady(const Lufia2CpuState *cpu,
                                   Lufia2PushedChildCall child, uint8_t width) {
    return child && cpu->program_bank == 0x82u && cpu->accumulator_is_8_bit == width &&
           !cpu->index_is_8_bit && !cpu->direct_page && !cpu->decimal &&
           cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static inline bool MenuActionChild(MenuActionCall *call, uint32_t site, uint32_t target,
                                   uint8_t frame, uint8_t width) {
    Lufia2CpuState *cpu = call->cpu;
    if (!CallChildWithFrame(call->memory, cpu, call->child, call->context, site, target,
                            frame, 0x82u)) {
        call->result =
            (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return false;
    }
    if (cpu->program_bank != 0x82u || cpu->accumulator_is_8_bit != width ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal) {
        call->result = ExecutionHandoff(cpu, site + frame + 1u);
        return false;
    }
    return true;
}

#endif
