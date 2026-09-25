#ifndef LUFIA2_ACTOR_ACTOR_INTERNAL_H
#define LUFIA2_ACTOR_ACTOR_INTERNAL_H

/* Actor subsystem internals shared across modules. */

#include "lufia2/execution.h"

/* $83:C0EF: leader position to $8F/$91. */
void Lufia2ActorLeaderToProbe(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address);

/* $83:F9F7: X = 2 * (B + A * width); no frame. */
void Lufia2MapCellOffset(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:F9AD / $83:F9B6: X = $8F + $91 * width. */
void Lufia2MapCellIndex(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address,
    uint8_t from_probe);

/* $83:F988: height bits 7-6 of the map cell at $8F/$91. */
void Lufia2MapTileHeight(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address);

/* $83:D89E body: Z clear = blocked, 0 = unknown target. */
uint8_t Lufia2ActorStepBlockedBody(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:FAFA: sign-extend A.low, leave M=0. */
void Lufia2SignExtendA8(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address);

/* Signed operand bytes added to a 16-bit X/Y pair. */
void Lufia2ActorAddSignedPair(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t pair,
    uint16_t operand,
    uint16_t return_address);

/* $83:CA93: step toward $7F:E5A6/E5CE target. */
void Lufia2ActorTargetDirection(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:CD6E..$83:CD91 facing comparators. */
uint8_t Lufia2ActorFacingCompare(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t helper_pc);

void Lufia2ActorInstallSecondaryScript(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:FA12: clear occupancy bit 0 under the actor. */
void Lufia2ActorClearMapOccupancy(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:AB4F: record offsets for actor $A7. */
void Lufia2ActorRecordOffsets(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:DF87: spawn id A into the first free slot. */
void Lufia2ActorSpawn(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:ABE9: sprite VRAM base (A.high << 4) + $2000; M=0. */
void Lufia2SpriteVramBase(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address);

/* $83:DAE9: reload the actor's sprite, keep its frame. */
void Lufia2ActorSpriteReload(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:AB7C: claim A free sprite slots in $7E:E100. */
void Lufia2SpriteAllocSlots(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address);

/* $83:ABCC: clear A sprite slots from $54/$55. */
void Lufia2SpriteFreeSlots(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address);

#endif
