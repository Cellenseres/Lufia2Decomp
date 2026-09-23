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
    LUFIA2_ACTOR_PRIMARY_SCRIPT_CONTINUE_D166 = 3,
} Lufia2ActorPrimaryScriptStepFlow;

typedef struct Lufia2ActorPrimaryScriptStepResult {
    Lufia2ActorPrimaryScriptStepFlow flow;
    uint8_t opcode;
    uint32_t handler_pc;
} Lufia2ActorPrimaryScriptStepResult;

typedef enum Lufia2ActorPrimaryActionFlow {
    LUFIA2_ACTOR_PRIMARY_ACTION_RETURN_D3AE = 0,
    LUFIA2_ACTOR_PRIMARY_ACTION_CONTINUE_D389 = 1,
    LUFIA2_ACTOR_PRIMARY_ACTION_UNKNOWN_D370_TARGET = 2,
} Lufia2ActorPrimaryActionFlow;

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
 *   $83:CC85  X-coordinate range test, jump or skip
 *   $83:CCA3  Y-coordinate range test, jump or skip
 *   $83:C867/$83:C86B/$83:C86F/$83:C873 fixed actions 0..3
 *   $83:C87C/$83:C880/$83:C884/$83:C888 fixed actions $81..$84
 *   $83:C877  action byte from operand
 *   $83:D14D  conditional action handler, including its D350 call
 *   $83:D2B4  replace the script cursor with operand16 + $A1D4
 *   $83:D2BD  direct-entry cursor-low-byte alias and redispatch
 *   $83:D2C4  OR operand8 into $7F:E57E+slot
 *   $83:D2D5  AND operand8 into $7F:E57E+slot
 *   $83:C891  install secondary script operand8 + $18
 *   $83:C8EE  store operand8 to $7F:E4DE+slot
 *   $83:C8FC/$83:C90A  set/clear actor $0736 bit 1
 *   $83:CBB7  map-cell $30 test, flag $0736 bit 6 or skip
 *   $83:CBE1  leader-within-radius test, jump or skip
 *   $83:CC1B/$83:CC2E  leader X/Y equality, jump or skip
 *   $83:CC41/$83:CC63  step toward leader on X/Y via D350
 *
 * Complete handlers include their original redispatch at C85A/C85C and
 * therefore return the next selected opcode/handler. C8C7 stops immediately
 * before the common PLB/RTS exit at C8D2. The reconstructed D14D and direct
 * action handlers now include their D350 JSL/RTL stack semantics and also
 * reach C8D2. Unknown handlers are left untouched.
 */
Lufia2ActorPrimaryScriptStepResult
Lufia2ActorPrimaryScriptExecuteKnownHandler(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint32_t handler_pc);

/*
 * Draft semantic reconstruction of the common actor action core at $83:D350.
 * Local action paths, the four direction/boundary helpers at D3B7/D3C5/
 * D3D7/D3E5, and the secondary-script installer at D3F7 are reconstructed.
 *
 * RETURN_D3AE means all semantics before the original RTL are committed.
 * The movement path now includes $83:FB12, $83:F9D4/$83:F9F7 and $83:FB71,
 * so CONTINUE_D389 is retained only for source compatibility and is no longer
 * emitted by the supported ROM paths.
 */
Lufia2ActorPrimaryActionFlow Lufia2ActorPrimaryActionCore(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu);

/*
 * $83:FB12 movement-coordinate step. The input A byte is the original
 * direction-table offset (0,2,4,6). The return value is the exact RTL
 * boundary PC ($83:FB24/$FB27/$FB2A/$FB2D), or zero for an unknown table
 * target.
 */
uint32_t Lufia2ActorMovementStep(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu);

/* $83:F9D4, including its $83:F9F7 coordinate-to-cell helper. */
void Lufia2ActorResolveMapCellOffset(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu);

/* $83:FB71 map-cell probe, including its call to $83:F9D4. */
void Lufia2ActorReadMapCellValue(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu);

#ifdef __cplusplus
}
#endif

#endif
