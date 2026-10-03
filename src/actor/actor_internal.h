#ifndef LUFIA2_ACTOR_ACTOR_INTERNAL_H
#define LUFIA2_ACTOR_ACTOR_INTERNAL_H

/* Actor subsystem internals shared across modules. */

#include "lufia2/actor.h"

/* $83:C0EF: leader position to $8F/$91. */
void Lufia2ActorLeaderToProbe(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address);

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

/* $83:AA7D from bank 83: animation tables for the sprite type. */
void Lufia2ActorSpriteTables(
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

/* Secondary actor script opcode tables in bank $83. The high nibble of the
 * opcode indexes SECONDARY_NIBBLE_TABLE; the groups $Fx, $Ex and $Dx are
 * entered through the group handlers and pick their routine from a second
 * table by the low nibble. */
enum {
    SECONDARY_NIBBLE_TABLE = 0xdf17,
    SECONDARY_GROUP_F_TABLE = 0xdf37,
    SECONDARY_GROUP_E_TABLE = 0xdf57,
    SECONDARY_GROUP_D_TABLE = 0xdf77,
    SECONDARY_GROUP_F_HANDLER = 0xd5d4,
    SECONDARY_GROUP_E_HANDLER = 0xd5e0,
    SECONDARY_GROUP_D_HANDLER = 0xd5ec
};

/* Bank of the secondary actor script routines. */
#define SECONDARY_BANK_83 0x830000u

/* Per-actor words beside the display offsets (40 words each): the alternate
 * display offset pair. $F9 saves the display offsets here, $E4 sets it from
 * two signed operands and $E5 accumulates it. */
#define WRAM_ACTOR_OFFSET_ALT_X 0x7fdbecu
#define WRAM_ACTOR_OFFSET_ALT_Y 0x7fdc3cu

/* Per-actor byte (40 entries): completed quarter steps of the current walk,
 * counting 1..16. It indexes the bob table and ends the walk opcode at 16. */
#define WRAM_ACTOR_WALK_PHASE 0x7fe4b6u

/* $83:DD44: signed vertical bob offsets, indexed by the walk phase minus 1. */
#define ROM_ACTOR_WALK_BOB_TABLE 0x83dd44u

/* Second plane the occupancy of blocking actors is also recorded in. */
#define MAP_BLOCKING_ATTRIBUTES 0x7e4001u

/* Secondary script operand bytes, relative to the opcode byte. */
enum {
    SECONDARY_OPCODE_BYTE = 0x0000,
    SECONDARY_OPERAND_1 = 0x0001,
    SECONDARY_OPERAND_2 = 0x0002,
    SECONDARY_OPERAND_3 = 0x0003
};

#endif
