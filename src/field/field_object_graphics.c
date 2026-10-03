/* Field-object origins, tile state and graphics setup. */

#include "actor/actor_internal.h"
#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/dp_scratch.h"
#include "system/wram.h"

enum {
    OBJECT_VERTICAL_OFFSET = 0x58,
    OBJECT_TILE_NUMBER = 0x54,
};

/* X = 0, or 2 with object flag bit 1. */
Lufia2ExecutionResult Lufia2FieldObjectLayer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 0x0000u);
    OpLda(memory, cpu, WRAM_FIELD_OBJECT_FLAGS);
    OpBitValue(cpu, 0x0002u);
    if (!cpu->zero)
        OpLdx(cpu, 0x0002u);
    return ExecutionReturned(0x83f61fu);
}

/* Copy the actor tile X and Y to the probe. */
Lufia2ExecutionResult Lufia2ActorPositionToObjectProbe(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_X));
    OpSta(memory, cpu, OpDp(cpu, DP_PROBE_X));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_Y));
    OpSta(memory, cpu, OpDp(cpu, DP_PROBE_Y));
    return ExecutionReturned(0x83d7b1u);
}

/* Object origin: probe X, probe Y minus height plus one. */
Lufia2ExecutionResult Lufia2FieldSetObjectOrigin(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, DP_PROBE_X));
    OpSta(memory, cpu, WRAM_FIELD_PENDING_OBJECT_X);
    OpLda(memory, cpu, OpDp(cpu, DP_PROBE_Y));
    cpu->carry = 1;
    OpSbcValue(cpu, OpReadM(memory, cpu, WRAM_FIELD_OBJECT_HEIGHT));
    OpIncA(cpu);
    OpSta(memory, cpu, WRAM_FIELD_PENDING_OBJECT_Y);
    return ExecutionReturned(0x83f434u);
}

/* Reset the display offsets; Y offset goes to DP $58. */
Lufia2ExecutionResult Lufia2ActorResetObjectOffsets(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SLOT_WORD_OFFSET)));
    TransferDirectToA(cpu);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_DISPLAY_OFFSET_X));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_DISPLAY_OFFSET_Y));
    OpLda(memory, cpu, OpDp(cpu, OBJECT_VERTICAL_OFFSET));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_DISPLAY_OFFSET_Y));
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x83f6c5u);
}

/* Scale X and Y by four; Y +2 when offset. */
Lufia2ExecutionResult Lufia2FieldPendingTileOffsets(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpTxa(cpu);
    OpAslA(cpu);
    OpAslA(cpu);
    OpTax(cpu);
    OpTya(cpu);
    OpAslA(cpu);
    OpAslA(cpu);
    OpTay(cpu);
    OpLda(memory, cpu, OpDp(cpu, OBJECT_VERTICAL_OFFSET));
    if (!cpu->zero) {
        OpIny(cpu);
        OpIny(cpu);
    }
    return ExecutionReturned(0x83f86au);
}

/* Store A's low 10 bits as map word X's tile. */
Lufia2ExecutionResult Lufia2FieldSetMapTileNumber(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpAndValue(cpu, 0x03ffu);
    OpSta(memory, cpu, OpDp(cpu, OBJECT_TILE_NUMBER));
    OpLda(memory, cpu, OpAbsX(cpu, 0x0000u));
    OpAndValue(cpu, 0xfc00u);
    OpOra(memory, cpu, OpDp(cpu, OBJECT_TILE_NUMBER));
    OpSta(memory, cpu, OpAbsX(cpu, 0x0000u));
    return ExecutionReturned(0x83f794u);
}

/* Marks the claimed actor id slots as empty ($FF). */
Lufia2ExecutionResult Lufia2FieldReleaseClaimedActors(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLoadA(cpu, 0x00ffu);
    OpSta(memory, cpu, WRAM_FIELD_CLAIMED_ACTOR_IDS);
    OpSta(memory, cpu, WRAM_FIELD_CLAIMED_ACTOR_IDS + 1u);
    return ExecutionReturned(0x83f7deu);
}

/* Copy the object palette to $0500 and flag the upload. */
Lufia2ExecutionResult Lufia2FieldCopyObjectPalette(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, WRAM_FIELD_OBJECT_PALETTE_POINTER);
    OpTax(cpu);
    OpLdy(cpu, 0x0500u);
    OpLoadA(cpu, 0x001fu);
    PushDataBank(memory, cpu);
    OpMoveNext(memory, cpu, 0x00u, 0x00u);
    PullDataBank(memory, cpu);
    OpLoadA(cpu, 0x0002u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_NMI_UPLOAD_FLAGS), 1u);
    return ExecutionReturned(0x83f746u);
}

/* Vertical offset $FFF0 for height 2; set the origin. */
Lufia2ExecutionResult Lufia2FieldPrepareObjectOrigin(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 0xfff0u);
    OpLda(memory, cpu, WRAM_FIELD_OBJECT_HEIGHT);
    OpCmpValue(cpu, 0x0002u);
    if (!cpu->zero)
        OpLdx(cpu, 0x0000u);
    OpWriteX(memory, cpu, OpDp(cpu, OBJECT_VERTICAL_OFFSET), cpu->x);
    SimulateJslFrame(memory, cpu, 0x83u, 0xf80bu);
    (void)Lufia2FieldSetObjectOrigin(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    return ExecutionReturned(0x83f80cu);
}

/* Probe from the actor; start a field event X=0, Y=$1C. */
Lufia2ExecutionResult Lufia2FieldStartObjectEvent(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    (void)Lufia2ActorPositionToObjectProbe(memory, cpu);
    PushY(memory, cpu);
    OpLdx(cpu, 0x0000u);
    OpLdy(cpu, 0x001cu);
    SimulateJslFrame(memory, cpu, 0x83u, 0xf7f5u);
    cpu->program_bank = 0x80u;
    Lufia2ExecutionResult result = Lufia2FieldStartEvent(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    SimulateRtlFrame(memory, cpu);
    cpu->program_bank = 0x83u;
    OpPullY(memory, cpu);
    return ExecutionReturned(0x83f7f7u);
}

/* DMAs a block to VRAM through channel 0. */
static void FieldFixedGraphicsDma(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t source_bank, uint16_t source, uint16_t length,
    uint16_t destination) {
    OpLoadA(cpu, 0x0001u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_DMAP(0)));
    OpLdx(cpu, source);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_A1TL(0)), cpu->x);
    OpLoadA(cpu, source_bank);
    OpSta(memory, cpu, OpAbs(cpu, SNES_A1B(0)));
    OpLdx(cpu, length);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_DASL(0)), cpu->x);
    OpLoadA(cpu, 0x0018u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_BBAD(0)));
    OpLdx(cpu, destination);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_VMADDL), cpu->x);
    OpLoadA(cpu, 0x0001u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_MDMAEN));
}

/* Upload the two fixed field graphics blocks to VRAM. */
Lufia2ExecutionResult Lufia2FieldUploadFixedGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit)
        PushAccumulator8(memory, cpu);
    else
        PushAccumulator16(memory, cpu);
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    FieldFixedGraphicsDma(memory, cpu, 0x9fu, 0x8500u, 0x1000u, 0x1000u);
    FieldFixedGraphicsDma(memory, cpu, 0xa5u, 0xe2fcu, 0x0300u, 0x1680u);
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    if (cpu->accumulator_is_8_bit)
        OpLoadA(cpu, Pull8(memory, cpu));
    else
        PullAccumulator16(memory, cpu);
    return ExecutionReturned(0x83b061u);
}

/* Give the claimed object's actor palette, sprites and shape. */
Lufia2ExecutionResult Lufia2FieldSetupObjectActorSprite(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpLongX(cpu, WRAM_ACTOR_CLAIMED_OBJECT_RECORD));
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_OBJECT_GRAPHICS_PALETTE));
    OpRepWidths(cpu, 0x20u);
    for (unsigned bit = 0; bit < 5u; ++bit)
        OpAslA(cpu);
    OpAdcValue(cpu, 0x0320u);
    OpSta(memory, cpu, WRAM_FIELD_OBJECT_PALETTE_POINTER);
    SimulateJsrFrame(memory, cpu, 0xf6e6u);
    (void)Lufia2FieldCopyObjectPalette(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    Lufia2ActorSpriteTables(memory, cpu, 0xf6ecu);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpLongX(cpu, WRAM_ACTOR_CLAIMED_OBJECT_RECORD));
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_OBJECT_SPRITE_ALLOCATION));
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_OBJECT_GRAPHICS_SHAPE));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7FE216));
    OpLda(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    OpSta(memory, cpu, OpLongX(cpu, 0x7fe25eu));
    TransferDirectToA(cpu);
    OpSta(memory, cpu, OpLongX(cpu, 0x7fe2a6u));
    OpLoadA(cpu, 0x000fu);
    OpSta(memory, cpu, OpLongX(cpu, 0x7fe1ceu));
    OpLoadA(cpu, 0x00feu);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E05D2));
    OpLoadA(cpu, 0x00ffu);
    OpSta(memory, cpu, OpLongX(cpu, 0x001471u));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7E066A));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_FLAGS));
    OpOraValue(cpu, 0x0002u);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_FLAGS));
    OpStz(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E1291));
    return ExecutionReturned(0x83f730u);
}
