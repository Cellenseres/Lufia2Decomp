#ifndef LUFIA2_ACTOR_FRONTEND_H
#define LUFIA2_ACTOR_FRONTEND_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t (*Lufia2ActorBusReadByte)(void *context, uint32_t address);
typedef void (*Lufia2ActorBusWriteByte)(
    void *context, uint32_t address, uint8_t value);

typedef struct Lufia2ActorFrontendMemory {
    Lufia2ActorBusReadByte read_byte;
    Lufia2ActorBusWriteByte write_byte;
    void *context;
} Lufia2ActorFrontendMemory;

/*
 * CPU subset touched by the reconstructed prefixes. Both routines are only
 * modeled for M=1 (8-bit memory/accumulator), matching every captured entry.
 * Fields not present here are unchanged by the reconstructed prefix.
 */
typedef struct Lufia2ActorFrontendCpu {
    uint16_t accumulator;
    uint16_t x;
    uint16_t direct_page;
    uint8_t data_bank;
    uint8_t carry;
    uint8_t negative;
    uint8_t zero;
    uint8_t index_is_8_bit;
} Lufia2ActorFrontendCpu;

typedef enum Lufia2ActorPrimaryFlow {
    LUFIA2_ACTOR_PRIMARY_RETURN = 0,
    LUFIA2_ACTOR_PRIMARY_CONTINUE_C808 = 1,
    LUFIA2_ACTOR_PRIMARY_CONTINUE_C829 = 2,
    LUFIA2_ACTOR_PRIMARY_CONTINUE_C83C = 3,
} Lufia2ActorPrimaryFlow;

typedef enum Lufia2ActorSecondaryFlow {
    LUFIA2_ACTOR_SECONDARY_RETURN = 0,
    LUFIA2_ACTOR_SECONDARY_CONTINUE_D512 = 1,
    LUFIA2_ACTOR_SECONDARY_CONTINUE_D550 = 2,
    LUFIA2_ACTOR_SECONDARY_CONTINUE_D585 = 3,
    LUFIA2_ACTOR_SECONDARY_CONTINUE_D59A = 4,
} Lufia2ActorSecondaryFlow;

/*
 * Draft semantic front-end of $83:C7F8. Known instructions are executed until
 * the original routine either reaches its RTS at $83:C83B or crosses into an
 * as-yet unreconstructed continuation block. The returned flow names that
 * exact boundary; CPU and bus side effects up to it are committed.
 */
Lufia2ActorPrimaryFlow Lufia2ActorPrimaryUpdateFrontend(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu);

/*
 * Draft semantic front-end of $83:D508. The same boundary contract applies;
 * this routine additionally preserves the observed path-dependent X width.
 */
Lufia2ActorSecondaryFlow Lufia2ActorSecondaryUpdateFrontend(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu);

#ifdef __cplusplus
}
#endif

#endif
