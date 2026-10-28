#ifndef LUFIA2_MENU_SCREEN_CHILD_INTERNAL_H
#define LUFIA2_MENU_SCREEN_CHILD_INTERNAL_H

#include "menu/menu_action_internal.h"

static inline bool MenuScreenChild(MenuActionCall *call, uint32_t site,
                                   uint32_t target, uint8_t frame,
                                   uint8_t accumulator_width, uint8_t index_width) {
    if (!CallChildWithFrame(call->memory, call->cpu, call->child, call->context,
                            site, target, frame, 0x82u)) {
        call->result = (Lufia2ExecutionResult){
            LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return false;
    }
    Lufia2CpuState *cpu = call->cpu;
    if (cpu->program_bank != 0x82u ||
        cpu->accumulator_is_8_bit != accumulator_width ||
        cpu->index_is_8_bit != index_width || cpu->direct_page || cpu->decimal) {
        call->result = ExecutionHandoff(cpu,
            (uint32_t)cpu->program_bank << 16 | (uint16_t)(site + frame + 1u));
        return false;
    }
    return true;
}

#endif
