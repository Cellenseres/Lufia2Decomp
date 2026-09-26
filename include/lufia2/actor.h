#ifndef LUFIA2_ACTOR_H
#define LUFIA2_ACTOR_H

/* Actor script VMs, slots, movement and player control. */

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

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

/*
 * JSR child at site: push the frame, run target to its RTS.
 * Returns 0 when the child unwinds instead.
 */
typedef uint8_t (*Lufia2ActorSlotChild)(
    void *context,
    Lufia2CpuState *cpu,
    uint32_t target,
    uint32_t site);

/* $83:BB93 40-slot traversal; children through the callback. */
Lufia2ExecutionResult Lufia2UpdateActorSlots(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2ActorSlotChild child,
    void *child_context);

/* $83:E03E object slots and their script VM ($83:E0FC). */
Lufia2ExecutionResult Lufia2ObjectSlotsUpdate(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:C1B4 player controller; exact LLE boundaries. */
Lufia2ExecutionResult Lufia2PlayerSlotStandardUpdate(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:D508 through its RTS, or an exact boundary. */
Lufia2ExecutionResult Lufia2ActorSecondaryUpdate(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:C7F8 through its RTS, or an exact boundary. */
/* $84:8193: VRAM DMA of the queued sprite graphics. */
Lufia2ExecutionResult Lufia2SpriteGraphicsUpload(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:A9BA: load sprite A for actor $A7; any M/X width. */
Lufia2ExecutionResult Lufia2ActorLoadSprite(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2ActorPrimaryUpdate(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* Draft $83:C7F8 front-end; stops at named continuations. */
Lufia2ActorPrimaryFlow Lufia2ActorPrimaryUpdateFrontend(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* Draft $83:D508 front-end; keeps the path-dependent X width. */
Lufia2ActorSecondaryFlow Lufia2ActorSecondaryUpdateFrontend(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:C83C-C866: opcode fetch and JMP ($D467,x) target. */
Lufia2ActorScriptDispatchResult Lufia2ActorPrimaryScriptDispatch(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:D59A: handler via $DF17 and the D/E/F tables; DB stays pushed. */
Lufia2ActorScriptDispatchResult Lufia2ActorSecondaryScriptDispatch(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* Run one primary handler to its next boundary; unknown ones untouched. */
Lufia2ActorPrimaryScriptStepResult
Lufia2ActorPrimaryScriptExecuteKnownHandler(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t handler_pc);

/* $83:D350 action and movement core, up to its RTL. */
Lufia2ActorPrimaryActionFlow Lufia2ActorPrimaryActionCore(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:FB12: step by table offset A; returns the RTL PC or 0. */
uint32_t Lufia2ActorMovementStep(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:F9D4, including its $83:F9F7 coordinate-to-cell helper. */
void Lufia2ActorResolveMapCellOffset(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:FB71 map-cell probe, including its call to $83:F9D4. */
void Lufia2ActorReadMapCellValue(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:FA3F: set occupancy bit 0 in $7E:4000 map. */
void Lufia2ActorMarkMapOccupancy(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:D416: primary script from $91:A1D4[$070A]. */
void Lufia2ActorLoadPrimaryScript(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:A746: 1/16 position from tile coordinates. */
void Lufia2ActorSyncFinePosition(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:FACB: add signed operands to $7F:DC8C/DD1C; M=0 exit. */
void Lufia2ActorAddDisplayOffset(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:FA81: move 1/16 position, update tiles; M=0 exit. */
void Lufia2ActorMoveFinePosition(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:C947: occupancy, flag reset, reload primary script. */
void Lufia2ActorPrimaryReset(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:CB65: $09A1..$09A5 = $FF. */
void Lufia2ActorClearSlotLinks(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:CA68: blocked-step event record at $7F:DEEE. */
void Lufia2ActorBlockedEvent(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
