#ifndef LUFIA2_CARD_TABLE_SERVICES_H
#define LUFIA2_CARD_TABLE_SERVICES_H

#include "core/child_call.h"
#include "core/cpu_ops.h"

typedef struct TableService {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
    bool binary;
} TableService;

static inline bool TableServiceReady(const Lufia2CpuState *cpu, bool binary) {
    return cpu->program_bank == 0x84u && !cpu->direct_page &&
        (!binary || !cpu->decimal);
}

static inline bool TableServiceCall(TableService *service, uint32_t site,
    uint32_t target, bool normalizes_accumulator) {
    Lufia2CpuState *cpu = service->cpu;
    const bool byte_mode = cpu->accumulator_is_8_bit != 0u;
    const uint8_t frame = target >> 16 == 0x84u ? 2u : 3u;
    if (!CallChildWithFrame(service->memory, cpu, service->child, service->context,
                            site, target, frame, 0x84u)) {
        service->result = ExecutionReturned(site);
        service->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (!TableServiceReady(cpu, service->binary) || cpu->index_is_8_bit ||
        (!normalizes_accumulator && cpu->accumulator_is_8_bit != byte_mode)) {
        service->result = ExecutionHandoff(cpu, (uint32_t)cpu->program_bank << 16 |
            (uint16_t)(site + frame + 1u));
        return false;
    }
    return true;
}

#endif
