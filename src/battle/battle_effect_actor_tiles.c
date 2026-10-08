#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum {
    DP_SOURCE_ORIGIN = 0u,
    DP_ACTOR_ORIGIN = 2u,
    DP_TILE_RADIUS = 4u,
    DP_TILE_SIZE = 6u,
    DP_TILE_SOURCE = 8u,
    DP_TILE_DESTINATION = 11u,
    DP_EFFECT_STREAM = 0xc3u,
    ACTOR_HORIZONTAL_MOTION = 0x1bu,
    ACTOR_VERTICAL_MOTION = 0x1du,
    ACTOR_HORIZONTAL = 0x1fu,
    ACTOR_VERTICAL = 0x21u,
    ACTOR_TILE_SIZE = 0x23u,
    TILE_ROW_BYTES = 64u
};

static uint32_t ActorField(const Lufia2CpuState *cpu, uint16_t field) {
    return LongIndexedAddress(0x7e0000u | field, cpu->x);
}

static void ReadSourceOrigin(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_EFFECT_STREAM));
    OpStepMem(memory, cpu, OpDp(cpu, DP_EFFECT_STREAM), 1);
    OpStepMem(memory, cpu, OpDp(cpu, DP_EFFECT_STREAM), 1);
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
    OpLoadA(cpu, TILE_ROW_BYTES);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
    OpLoadA(cpu, 0u);
    ExchangeAccumulatorBytes(cpu);
    OpRepWidths(cpu, 0x20u);
    OpAslA(cpu);
    OpAdc(memory, cpu, OpAbs(cpu, SNES_RDMPYL));
    OpSta(memory, cpu, OpDp(cpu, DP_SOURCE_ORIGIN));
}

static void ReadActorOrigin(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpTyx(cpu);
    OpLda(memory, cpu, ActorField(cpu, ACTOR_HORIZONTAL));
    cpu->carry = 0u;
    OpAdc(memory, cpu, ActorField(cpu, ACTOR_HORIZONTAL_MOTION));
    OpAndValue(cpu, 0xffu);
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_ACTOR_ORIGIN));
    OpLda(memory, cpu, ActorField(cpu, ACTOR_VERTICAL));
    cpu->carry = 0u;
    OpAdc(memory, cpu, ActorField(cpu, ACTOR_VERTICAL_MOTION));
    OpAndValue(cpu, 0xffu);
    OpIncA(cpu);
    for (unsigned shift = 0u; shift < 3u; ++shift)
        OpLsrA(cpu);
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
    OpLoadA(cpu, TILE_ROW_BYTES);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DP_ACTOR_ORIGIN));
    OpAdc(memory, cpu, OpAbs(cpu, SNES_RDMPYL));
    OpSta(memory, cpu, OpDp(cpu, DP_ACTOR_ORIGIN));
}

static void ReadTileRadius(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLda(memory, cpu, ActorField(cpu, ACTOR_TILE_SIZE));
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x0101u);
    OpAndValue(cpu, 0xfefeu);
    OpSta(memory, cpu, OpDp(cpu, DP_TILE_SIZE));
    OpLsrA(cpu);
    OpSepWidths(cpu, 0x20u);
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
    OpLoadA(cpu, TILE_ROW_BYTES);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
    OpLoadA(cpu, 0u);
    ExchangeAccumulatorBytes(cpu);
    OpRepWidths(cpu, 0x20u);
    OpAslA(cpu);
    OpAdc(memory, cpu, OpAbs(cpu, SNES_RDMPYL));
    OpSta(memory, cpu, OpDp(cpu, DP_TILE_RADIUS));
}

static void CenterTilePointers(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, DP_SOURCE_ORIGIN));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_TILE_RADIUS)));
    cpu->carry = 0u;
    OpAdcValue(cpu, WRAM_SAVE_FILE_BUFFER & 0xffffu);
    OpSta(memory, cpu, OpDp(cpu, DP_TILE_SOURCE));
    OpLda(memory, cpu, OpDp(cpu, DP_ACTOR_ORIGIN));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_TILE_RADIUS)));
    cpu->carry = 0u;
    OpAdcValue(cpu, WRAM_FIELD_LAYER2_TILEMAP & 0xffffu);
    OpBitValue(cpu, 1u);
    if (!cpu->zero)
        OpDecA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_TILE_DESTINATION));
}

static void CopyTileByte(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    uint16_t pointer = Read16Direct(memory, cpu, DP_TILE_SOURCE);
    OpLda(memory, cpu, AbsoluteIndexedAddress(cpu, pointer, cpu->y));
    pointer = Read16Direct(memory, cpu, DP_TILE_DESTINATION);
    OpSta(memory, cpu, AbsoluteIndexedAddress(cpu, pointer, cpu->y));
    OpIny(cpu);
}

static void CopyActorTiles(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdy(cpu, 0u);
    do {
        OpLda(memory, cpu, OpDp(cpu, DP_TILE_SIZE));
        OpSta(memory, cpu, OpDp(cpu, DP_ACTOR_ORIGIN));
        OpWriteX(memory, cpu, OpDp(cpu, DP_TILE_RADIUS), cpu->y);
        do {
            CopyTileByte(memory, cpu);
            CopyTileByte(memory, cpu);
            OpStepMem(memory, cpu, OpDp(cpu, DP_ACTOR_ORIGIN), -1);
        } while (!cpu->zero);
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpDp(cpu, DP_TILE_RADIUS));
        cpu->carry = 0u;
        OpAdcValue(cpu, TILE_ROW_BYTES);
        OpTay(cpu);
        OpSepWidths(cpu, 0x20u);
        OpStepMem(memory, cpu, OpDp(cpu, DP_TILE_SIZE + 1u), -1);
    } while (!cpu->zero);
}

Lufia2ExecutionResult Lufia2BattleEffectCopyActorTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x81986eu);
    ReadSourceOrigin(memory, cpu);
    ReadActorOrigin(memory, cpu);
    ReadTileRadius(memory, cpu);
    CenterTilePointers(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpSetDataBank(memory, cpu, 0x7eu);
    PushY(memory, cpu);
    CopyActorTiles(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_EFFECT_BG3_REQUEST));
    return ExecutionReturned(0x819933u);
}
