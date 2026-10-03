#ifndef LUFIA2_SYSTEM_DP_SCRATCH_H
#define LUFIA2_SYSTEM_DP_SCRATCH_H

/* Direct page scratch bytes. Every routine that needs a temporary operand,
 * coordinate or intermediate result inside one handler uses these cells, so
 * their meaning is local to the routine and never survives a call to another
 * handler. A routine with a clearer reading names its own cells. */
enum {
    DP_SCRATCH_A = 0x54,
    DP_SCRATCH_B = 0x55,
    DP_SCRATCH_C = 0x56,
    DP_SCRATCH_D = 0x57,
    DP_SCRATCH_E = 0x58
};

#endif
