#ifndef LUFIA2_CAPSULE_PRESENTATION_INTERNAL_H
#define LUFIA2_CAPSULE_PRESENTATION_INTERNAL_H

#include "party/capsule_feed_internal.h"

enum {
    DP_FORM_RESOURCE = 0x54u,
    DP_FORM_COPY_ROWS = 0x58u,
    DP_FORM_COPY_COLUMN = 0x5au,
    DP_FORM_COPY_SOURCE = 0x5du,
    DP_FORM_COPY_DESTINATION = 0x60u,
    FORM_PALETTE_TARGET = 0x0340u,
    FORM_PALETTE_CURRENT = 0x0352u,
    FORM_PALETTE_END = 0x0358u
};

static inline bool CapsulePresentationReady(
    const Lufia2CpuState *cpu, bool byte_accumulator) {
    return cpu->program_bank == 0x82u &&
        cpu->accumulator_is_8_bit == byte_accumulator &&
        !cpu->index_is_8_bit && !cpu->decimal && cpu->direct_page == 0u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

#endif
