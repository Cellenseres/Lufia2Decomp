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
 * Portable subset of the 65816 state touched by the reconstructed actor
 * front-ends and script-dispatch prefixes. The dispatcher models native mode
 * (E=0), which is the mode used by the captured game paths.
 */
typedef struct Lufia2ActorFrontendCpu {
    uint16_t accumulator;
    uint16_t x;
    uint16_t y;
    uint16_t stack;
    uint16_t direct_page;
    uint8_t data_bank;
    uint8_t program_bank;
    uint8_t carry;
    uint8_t negative;
    uint8_t zero;
    uint8_t accumulator_is_8_bit;
    uint8_t index_is_8_bit;
} Lufia2ActorFrontendCpu;

typedef enum Lufia2ActorPrimaryFlow {
    LUFIA2_ACTOR_PRIMARY_RETURN = 0,
    LUFIA2_ACTOR_PRIMARY_CONTINUE_C808 = 1,
    LUFIA2_ACTOR_PRIMARY_CONTINUE_C83C = 3,
} Lufia2ActorPrimaryFlow;

typedef enum Lufia2ActorSecondaryFlow {
    LUFIA2_ACTOR_SECONDARY_RETURN = 0,
    LUFIA2_ACTOR_SECONDARY_CONTINUE_D512 = 1,
    LUFIA2_ACTOR_SECONDARY_CONTINUE_D550 = 2,
    LUFIA2_ACTOR_SECONDARY_CONTINUE_D585 = 3,
    LUFIA2_ACTOR_SECONDARY_CONTINUE_D59A = 4,
} Lufia2ActorSecondaryFlow;

typedef struct Lufia2ActorScriptDispatchResult {
    uint8_t opcode;
    uint32_t handler_pc;
} Lufia2ActorScriptDispatchResult;

typedef enum Lufia2ActorPrimaryScriptStepFlow {
    LUFIA2_ACTOR_PRIMARY_SCRIPT_UNKNOWN_HANDLER = 0,
    LUFIA2_ACTOR_PRIMARY_SCRIPT_REDISPATCHED = 1,
    LUFIA2_ACTOR_PRIMARY_SCRIPT_CONTINUE_C8D2 = 2,
} Lufia2ActorPrimaryScriptStepFlow;

typedef struct Lufia2ActorPrimaryScriptStepResult {
    Lufia2ActorPrimaryScriptStepFlow flow;
    uint8_t opcode;
    uint32_t handler_pc;
} Lufia2ActorPrimaryScriptStepResult;

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

/*
 * Reconstruct $83:C83C..$83:C866 through the indirect script-handler jump.
 * The result is the exact 24-bit handler boundary selected by the original
 * jump table. The table itself is read through the bus callback, so the
 * standalone decomp does not embed proprietary ROM table bytes.
 */
Lufia2ActorScriptDispatchResult Lufia2ActorPrimaryScriptDispatch(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu);

/*
 * Reconstruct $83:D59A through its first-level high-nibble dispatch and the
 * D/E/F low-nibble subdispatchers. The returned PC is the leaf handler entered
 * after those table jumps. The original DB is left on the emulated stack just
 * as in the 65816 routine; handlers restore it with PLB before returning.
 */
Lufia2ActorScriptDispatchResult Lufia2ActorSecondaryScriptDispatch(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu);

/*
 * Execute one currently reconstructed primary-VM handler and stop at the next
 * semantic boundary. The initial supported cluster is:
 *   $83:C8C7  commit the current script cursor through $83:C8D2
 *   $83:D2B4  replace the script cursor with operand16 + $A1D4
 *   $83:D2C4  OR operand8 into $7F:E57E+slot
 *   $83:D2D5  AND operand8 into $7F:E57E+slot
 *
 * D2B4/D2C4/D2D5 include the original redispatch at C85A/C85C and therefore
 * return the next selected opcode/handler. C8C7 stops immediately before the
 * common PLB/RTS exit at C8D2. Unknown handlers are left untouched.
 */
Lufia2ActorPrimaryScriptStepResult
Lufia2ActorPrimaryScriptExecuteKnownHandler(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint32_t handler_pc);

#ifdef __cplusplus
}
#endif

#endif
