#ifndef LUFIA2_TEXT_CHILD_SERVICE_H
#define LUFIA2_TEXT_CHILD_SERVICE_H

#include "core/child_call.h"

typedef struct TextChildService {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
} TextChildService;

static inline bool TextCallChild(TextChildService *service, uint32_t site,
                                 uint32_t target, uint8_t frame) {
    Lufia2CpuState *cpu = service->cpu;
    if (!CallChildWithFrame(service->memory, cpu, service->child, service->context,
                            site, target, frame, 0x80u)) {
        service->result = ExecutionReturned(site);
        service->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal) {
        service->result = ExecutionHandoff(cpu, site + frame + 1u);
        return false;
    }
    return true;
}

#endif
