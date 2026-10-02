#ifndef LUFIA2_CORE_CHILD_CALL_H
#define LUFIA2_CORE_CHILD_CALL_H

/* Pushed-frame call of a child routine through the consumer. */

#include "core/cpu_internal.h"

/* Push the JSR (frame_size 2) or JSL (3) return frame of the call at `site`,
 * then run `target` through the consumer. Returns 0 when the child unwinds
 * instead of returning. The return bank only matters for a JSL frame. */
static inline uint8_t CallChildWithFrame(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *context,
    uint32_t site,
    uint32_t target,
    uint8_t frame_size,
    uint8_t return_bank) {
    if (frame_size == 3u)
        SimulateJslFrame(memory, cpu, return_bank, (uint16_t)(site + 3u));
    else
        SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    return child(context, cpu, target, site, frame_size);
}

#endif
