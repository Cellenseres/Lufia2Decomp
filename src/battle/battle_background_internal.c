#include "battle/battle_lifecycle_internal.h"
#include "core/snes_registers.h"
#include "system/dp_scratch.h"

enum {
    BATTLE_BACKGROUND_DESCRIPTOR = 0x11e2u,
    BATTLE_BACKGROUND_TABLE = 0x97fd40u,
    BATTLE_BACKGROUND_TILEMAP = 0x7e2000u,
    BATTLE_BACKGROUND_TILES = 0x7ec000u,
};

/* Copy the background's four descriptor bytes to $11E2. */
static void LoadBackgroundDescriptor(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                     uint8_t background_id) {
    OpLoadA(cpu, background_id);
    OpAslA(cpu);
    OpAslA(cpu);
    OpTax(cpu);

    for (uint16_t i = 0; i < 4u; ++i) {
        OpLda(memory, cpu, OpLongX(cpu, BATTLE_BACKGROUND_TABLE + i));
        OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(BATTLE_BACKGROUND_DESCRIPTOR + i)));
    }
}

/* Decompress the background tiles and tilemap. */
static bool DecodeBackgroundResources(BattleContext *battle) {
    Lufia2CpuState *cpu = battle->cpu;
    const Lufia2Memory *memory = battle->memory;

    OpLdx(cpu, 0xc000u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x60u), cpu->x);
    LoadA8(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, 0x62u));

    OpLda(memory, cpu, OpAbs(cpu, BATTLE_BACKGROUND_DESCRIPTOR + 1u));
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0x00ffu);
    cpu->carry = 0;
    OpAdcValue(cpu, 0x016cu);
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    if (!BattleDecompressResource(battle, 0xba08u))
        return false;

    OpLda(memory, cpu, OpAbs(cpu, BATTLE_BACKGROUND_DESCRIPTOR));
    OpAndValue(cpu, 0x00ffu);
    cpu->carry = 0;
    OpAdcValue(cpu, 0x0179u);
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    OpSepWidths(cpu, 0x20u);

    OpLdx(cpu, 0x2000u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x60u), cpu->x);
    LoadA8(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, 0x62u));
    return BattleDecompressResource(battle, 0xba23u);
}

/* Load palettes 2 and 3 from $97:CD58. */
static bool LoadBackgroundPalettes(BattleContext *battle) {
    Lufia2CpuState *cpu = battle->cpu;
    const Lufia2Memory *memory = battle->memory;

    OpLda(memory, cpu, OpAbs(cpu, BATTLE_BACKGROUND_DESCRIPTOR + 2u));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
    LoadA8(cpu, 0x40u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
    LoadA8(cpu, 0x97u);
    OpSta(memory, cpu, OpDp(cpu, 0x24u));
    OpRepWidths(cpu, 0x20u);
    cpu->carry = 0;
    LoadA16(cpu, 0xcd58u);
    OpAdc(memory, cpu, OpAbs(cpu, SNES_RDMPYL));
    OpTay(cpu);
    OpSepWidths(cpu, 0x20u);

    LoadA8(cpu, 2u);
    if (!BattleLoadPalette(battle, 0xba44u))
        return false;

    LoadA8(cpu, 3u);
    return BattleLoadPalette(battle, 0xba49u);
}

/* Undo the tilemap's difference coding at $7E:2000. */
static void DecodeBackgroundTilemap(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpRepWidths(cpu, 0x20u);

    OpLda(memory, cpu, OpAbs(cpu, 0x2000u));
    OpLdx(cpu, 2u);
    do {
        cpu->carry = 1;
        OpSbcValue(cpu, OpRead16(memory, OpAbsX(cpu, 0x2000u)));
        OpSta(memory, cpu, OpAbsX(cpu, 0x2000u));
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, 0x0800u);
    } while (!cpu->zero);

    OpSepWidths(cpu, 0x20u);
    PullDataBank(memory, cpu);
}

/* Read the background id; true when blank ($18). */
bool BattleBackgroundIsBlank(BattleContext *battle, uint8_t *background_id) {
    Lufia2CpuState *cpu = battle->cpu;

    TransferDirectToA(cpu);
    OpLda(battle->memory, cpu, OpAbs(cpu, WRAM_BATTLE_BACKGROUND_ID));
    *background_id = A8(cpu);
    OpCmpValue(cpu, BATTLE_BACKGROUND_BLANK);
    return cpu->zero;
}

/* Blank background: clear tiles and fill the tilemap. */
void BattleClearBackground(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;

    PushDataBank(memory, cpu);
    OpRepWidths(cpu, 0x20u);

    TransferDirectToA(cpu);
    OpSta(memory, cpu, BATTLE_BACKGROUND_TILES);
    OpLdx(cpu, 0xc000u);
    OpLdy(cpu, 0xc001u);
    LoadA16(cpu, 0x0ffeu);
    OpMoveNext(memory, cpu, 0x7eu, 0x7eu);

    LoadA16(cpu, 0x00ffu);
    for (uint16_t i = 0; i < 8u; ++i)
        OpSta(memory, cpu, BATTLE_BACKGROUND_TILES + 2u * i);

    LoadA16(cpu, 0x0900u);
    OpSta(memory, cpu, BATTLE_BACKGROUND_TILEMAP);
    OpLdx(cpu, 0x2000u);
    OpLdy(cpu, 0x2002u);
    LoadA16(cpu, 0x07fdu);
    OpMoveNext(memory, cpu, 0x7eu, 0x7eu);

    OpSepWidths(cpu, 0x20u);
    PullDataBank(memory, cpu);
}

/* Load descriptor, tiles, tilemap and palettes; false on unwind. */
bool BattleLoadBackground(BattleContext *battle, uint8_t background_id) {
    LoadBackgroundDescriptor(battle->memory, battle->cpu, background_id);

    if (!DecodeBackgroundResources(battle))
        return false;
    if (!LoadBackgroundPalettes(battle))
        return false;

    DecodeBackgroundTilemap(battle->memory, battle->cpu);
    return true;
}

/* Set DP $D9 bit 0; id 0 also calls $85:A701. */
bool BattleFinishBackground(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;

    LoadA8(cpu, 1u);
    OpTestBits(memory, cpu, OpDp(cpu, 0xd9u), 0u);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, 0x1b17u));

    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_BACKGROUND_ID));
    if (!cpu->zero)
        return true;

    return BattleCall(battle, 0xbac6u, 0x85a701u, 3u);
}
