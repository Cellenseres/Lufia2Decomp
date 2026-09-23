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
    uint8_t overflow;
    uint8_t decimal;
    uint8_t irq_disable;
    /* Exact ROM PC when a run stops early. */
    uint32_t resume_pc;
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
    /* X8 at $83:D38D; F9D4 would unbalance the stack. */
    LUFIA2_ACTOR_PRIMARY_ACTION_X8_BOUNDARY_D38D = 3,
} Lufia2ActorPrimaryActionFlow;

typedef enum Lufia2ActorPrimaryUpdateFlow {
    /* State at the RTS ($83:C83B or $83:C8D3). */
    LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED = 0,
    /* State exact at resume_pc; finish in LLE. */
    LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY = 1,
    /* A child call did not return; propagate. */
    LUFIA2_ACTOR_PRIMARY_UPDATE_CHILD_UNWOUND = 2,
} Lufia2ActorPrimaryUpdateFlow;

typedef struct Lufia2ActorPrimaryUpdateResult {
    Lufia2ActorPrimaryUpdateFlow flow;
    uint32_t pc;
    uint32_t dispatches;
} Lufia2ActorPrimaryUpdateResult;

/* $83:D508 through its RTS, or an exact boundary. */
/*
 * JSR child at site: push the frame, run target to its RTS.
 * Returns 0 when the child unwinds instead.
 */
typedef uint8_t (*Lufia2ActorSlotChild)(
    void *context,
    Lufia2ActorFrontendCpu *cpu,
    uint32_t target,
    uint32_t site);

/* $83:BB93 40-slot traversal; children through the callback. */
Lufia2ActorPrimaryUpdateResult Lufia2UpdateActorSlots(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    Lufia2ActorSlotChild child,
    void *child_context);

/* $83:C1B4 player controller; exact LLE boundaries. */
Lufia2ActorPrimaryUpdateResult Lufia2PlayerSlotStandardUpdate(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu);

Lufia2ActorPrimaryUpdateResult Lufia2ActorSecondaryUpdate(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu);

/* $83:C7F8 through its RTS, or an exact boundary. */
Lufia2ActorPrimaryUpdateResult Lufia2ActorPrimaryUpdate(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu);

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
 *   $83:C8AF  timer = random(operand8) + operand8, via $80:8299
 *   $83:C8D4  timer = (random(operand8) + operand8) * 8
 *   $83:C8EE  store operand8 to $7F:E4DE+slot
 *   $83:C8FC/$83:C90A  set/clear actor $0736 bit 1
 *   $83:CBB7  map-cell $30 test, flag $0736 bit 6 or skip
 *   $83:CBE1  leader-within-radius test, jump or skip
 *   $83:CC1B/$83:CC2E  leader X/Y equality, jump or skip
 *   $83:CC41/$83:CC63  step toward leader on X/Y via D350
 *   $83:CAB9  timer + step toward $7F:E5A6/E5CE target
 *   $83:CCF0/$83:CD0D  step away from leader, random when level
 *   $83:CD2E/$83:D132  commit cursor
 *   $83:CD32  rotate facing, action via $83:C1A5
 *   $83:CD4F  jump if leader ahead in facing direction
 *   $83:CD92  action from $47 low nibble via $83:D457
 *   $83:CE73  action $60
 *   $83:D125  random direction 0..3
 *   $83:D320  jump if random byte >= operand8
 *   $83:D340  jump to operand16 + $F000
 *   $83:D176  merge operand8 into $1291+slot low bits
 *   $83:D188  timer = operand8, commit
 *   $83:D196  target + secondary script $1C
 *   $83:D1C1  action operand8 if $09A1 negative
 *   $83:D1D0  jump table indexed by leader facing
 *   $83:D1E6  facing-relative action via $83:C1A5
 *   $83:D210  compare target record, six modes
 *   $83:D293  jump unless operand8 & $7F:E57E+slot
 *   $83:D2E6/$83:D2F6/$83:D30B  set/add/sub target record
 *   $83:C98A/$83:CCD7  step toward/away from leader via C9C5
 *   $83:CA19/$83:D09A  step toward a listed point via D0AA
 *   $83:CF6E  action $5F, record position at $7F:DB9C
 *   $83:CF8C  step toward operand point or skip
 *   $83:CFB9  find actor in radius, record slot at $7F:DB4C
 *   $83:D01E  step toward the recorded actor
 *   $83:D112  action from $47 low nibble via $83:D447
 *   $83:CDA5  walk ahead of leader if the run is long enough
 *   $83:CE7D/$83:CF1A  random wander, box-checked for CF1A
 *   $83:D03F  jump if a step in operand direction is blocked
 *   $83:C918/$83:CAD3/$83:CBB1  reset via C947, exit C8D2
 *   $83:CA29  step toward target, blocked event via CA68
 *   $83:D135  set tile position, sync fine position
 *   $83:D1B5  deferred APU command via $84:8766
 *   $83:D1FE/$83:D207  signed offsets via FACB/FA81
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

/* $80:8299: A = (A.low * next random byte) >> 8. */
void Lufia2RandomScale(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu);

/* $83:FA3F: set occupancy bit 0 in $7E:4000 map. */
void Lufia2ActorMarkMapOccupancy(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu);

/* $83:D416: primary script from $91:A1D4[$070A]. */
void Lufia2ActorLoadPrimaryScript(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu);

/* $83:A746: 1/16 position from tile coordinates. */
void Lufia2ActorSyncFinePosition(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu);

/* $84:8766: defer APU command A to $17AC. */
void Lufia2QueueDeferredSound(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu);

/* $83:FACB: add signed operands to $7F:DC8C/DD1C; M=0 exit. */
void Lufia2ActorAddDisplayOffset(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu);

/* $83:FA81: move 1/16 position, update tiles; M=0 exit. */
void Lufia2ActorMoveFinePosition(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu);

/* $83:C947: occupancy, flag reset, reload primary script. */
void Lufia2ActorPrimaryReset(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu);

/* $83:CB65: $09A1..$09A5 = $FF. */
void Lufia2ActorClearSlotLinks(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu);

/* $83:CA68: blocked-step event record at $7F:DEEE. */
void Lufia2ActorBlockedEvent(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu);

/* $80:82C7: A.low = next random byte, same table and index. */
void Lufia2RandomByte(
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
