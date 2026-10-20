#ifndef LUFIA2_CAPSULE_FEED_INTERNAL_H
#define LUFIA2_CAPSULE_FEED_INTERNAL_H

#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/party.h"
#include "menu/menu_sprite_slots.h"
#include "system/wram.h"

enum {
    CAPSULE_FOOD_TYPE = 0x154du,
    CAPSULE_FOOD_MESSAGE = 0x154eu,
    CAPSULE_FOOD_AMOUNT = 0x154fu,
    CAPSULE_FOOD_METER_LIMIT = 0x20u,
    CAPSULE_ITEM_FIELDS = WRAM_RECORD_BUFFER + 0x0du,
    CAPSULE_ITEM_VALUE = WRAM_RECORD_BUFFER + 0x12u,
    ROM_CAPSULE_FOOD_THRESHOLDS = 0x8ee466u,
    ROM_CAPSULE_FOOD_AMOUNTS = 0x8ee48au,
    ROM_CAPSULE_FORM_TYPES = 0x8ee485u,
    DP_FEED_SCRATCH = 0u,
    DP_FEED_ROWS = 2u,
    DP_FEED_COLUMNS = 4u
};

typedef struct CapsuleFeedCall {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
} CapsuleFeedCall;

static inline bool CapsuleFeedReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x82u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && cpu->direct_page == 0u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static inline bool CapsuleFeedChild(CapsuleFeedCall *call, uint32_t site,
                                    uint32_t target, uint8_t frame) {
    if (CallChildWithFrame(call->memory, call->cpu, call->child, call->context,
                           site, target, frame, 0x82u))
        return true;
    call->result = ExecutionReturned(site);
    call->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return false;
}

#endif
