#ifndef LUFIA2_FIELD_FIELD_INTERNAL_H
#define LUFIA2_FIELD_FIELD_INTERNAL_H

/* Field subsystem internals shared across modules. */

#include "lufia2/execution.h"

/* $83:80CD: field idle test; zero = no event running. */
void Lufia2FieldIdleBody(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:CBAE: event slot timers; 0 = LLE at resume_pc. */
/* $80:BFAA: find key A in a list; carry clear = found. */
uint8_t Lufia2FieldListSearch(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t return_bank,
    uint16_t return_address);

/* Read-only: cells $83:8E85 draws for layer X; ~0 for no layer. */
uint32_t Lufia2FieldRegionCells(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu);

/* $83:8E85: redraw region $7F:D046/D04C in layer X. */
void Lufia2FieldRedrawRegion(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t return_bank,
    uint16_t return_address);

/* $83:8E66: redraw all four BG layers. */
void Lufia2FieldRedrawLayers(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address);

uint8_t Lufia2FieldEventTimerBody(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    unsigned *passes);

/* $84:8000: screen effects of $1261 and the $1262 palette fade. */
void Lufia2FieldScreenEffects(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:EBAA: section table at $7F:D000; M1X0. */
Lufia2ExecutionResult Lufia2FieldReadSections(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:EC18: pack section attribute bits; M1X0. */
Lufia2ExecutionResult Lufia2FieldPackSectionAttributes(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:EC78 size of section $05AA into $05B9/$05BB (DB = $7F). */
Lufia2ExecutionResult Lufia2FieldSectionSize(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:EC98 (M0X0): decompress map data; returns M1. */
Lufia2ExecutionResult Lufia2FieldDecompressMapData(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* Object animation slots, eight entries in bank $7F ($83:8682 ticks them).
 * The state byte's bit 7 marks a slot in use; the object byte is the object
 * the slot animates. */
#define EVENT_ANIMATION_SLOT_STATE 0x7fd057u
#define EVENT_ANIMATION_SLOT_OBJECT 0x7fd04fu
/* The object index the running object opcode or animation slot works on. */
#define EVENT_OBJECT_OPERAND 0x7fd04eu

/* Scripted camera target (x, y words) used while SCREEN_EFFECT_SCRIPTED_CAMERA
 * is set. */
#define FIELD_SCRIPTED_CAMERA_X 0x7fd08bu
#define FIELD_SCRIPTED_CAMERA_Y 0x7fd08du
/* Screen shake offsets added to the published scroll. */
#define FIELD_SCREEN_OFFSET_X 0x7fd081u
#define FIELD_SCREEN_OFFSET_Y 0x7fd083u
/* Screen shake parameters; the first is only set, the second is the
 * amplitude and the third the chance threshold compared with a random byte. */
#define FIELD_SHAKE_PARAM_7E 0x7fd07eu
#define FIELD_SHAKE_AMPLITUDE 0x7fd07fu
#define FIELD_SHAKE_CHANCE 0x7fd080u
/* Per-layer scroll follow state: target words and speed words. */
#define FIELD_FOLLOW_TARGET_X 0x7fd0ceu
#define FIELD_FOLLOW_TARGET_Y 0x7fd0d6u
#define FIELD_FOLLOW_SPEED_X 0x7fd0deu
#define FIELD_FOLLOW_SPEED_Y 0x7fd0e6u
/* Per-layer scroll speed; zero snaps the layer to the camera. */
#define FIELD_LAYER_SCROLL_SPEED 0x05a8u
/* Half of the screen size: a centred camera sits this far from its origin. */
#define FIELD_SCREEN_HALF_WIDTH 0x0080u
#define FIELD_SCREEN_HALF_HEIGHT 0x0070u

/* Pending object records: 48 entries (WRAM_FIELD_PENDING_RECORD_X/Y). */
#define EVENT_OBJECT_RECORD_COUNT 0x0030u
#define EVENT_ANIMATION_SLOT_COUNT 0x0008u
/* A map tile word: the low ten bits pick the tile, the rest are its flags. */
#define FIELD_TILE_INDEX_MASK 0x03ffu
#define FIELD_TILE_FLAGS_MASK 0xfc00u
/* Bank $7F base for offsets into the tile and attribute planes. */
#define FIELD_BANK_7F 0x7f0000u

#endif
