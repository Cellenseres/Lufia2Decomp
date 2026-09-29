/* Battle display setup. */

#include "battle/battle_lifecycle_internal.h"
#include "core/snes_registers.h"
#include "system/scene_nmi_internal.h"
#include "system/wram.h"

void BattleResetDisplayWork(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    OpStz(memory, cpu, OpDp(cpu, 0x74u));
    OpStz(memory, cpu, OpDp(cpu, 0x72u));
    OpStz(memory, cpu, OpDp(cpu, 0x73u));
    OpStz(memory, cpu, OpDp(cpu, 0x71u));

    LoadA8(cpu, BRIGHTNESS_FORCED_BLANK);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));

    OpLdx(cpu, 15u);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, 0x0594u));
        OpDex(cpu);
    } while (!cpu->negative);

    OpWriteX(memory, cpu, OpAbs(cpu, 0x0596u), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x059au), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x059eu), cpu->x);
    OpTxa(cpu);
    OpSta(memory, cpu, OpAbs(cpu, 0x15b3u));
}

bool BattleClearDisplayBuffers(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;

    if (!BattleClearBackgroundTilemap(battle))
        return false;
    if (!BattleResetPartyTilemap(battle))
        return false;
    if (!BattleClearWindowTilemapForSetup(battle))
        return false;
    if (!BattleClearTilemap3800ForSetup(battle))
        return false;

    OpStz(memory, cpu, OpDp(cpu, 0xd8u));
    OpStz(memory, cpu, OpDp(cpu, 0xd9u));
    OpStz(memory, cpu, OpAbs(cpu, BATTLE_FRAME_STATE));

    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 90u);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, 0x1a8fu));
        for (unsigned i = 0; i < 6u; ++i)
            OpDex(cpu);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);

    return true;
}

void BattleBuildPartyDisplayState(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    TransferDirectToA(cpu);
    OpLdx(cpu, 0u);
    OpTxy(cpu);

    do {
        OpLda(memory, cpu, OpAbsY(cpu, 0x153du));
        PushY(memory, cpu);
        OpTay(cpu);

        OpLda(memory, cpu, OpAbsY(cpu, 0xb411u));
        OpOra(memory, cpu, OpLongX(cpu, 0x00139cu));
        OpSta(memory, cpu, OpLongX(cpu, 0x00139cu));

        OpRepWidths(cpu, 0x20u);
        OpTxa(cpu);
        cpu->carry = 0;
        OpAdcValue(cpu, 13u);
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);

        OpPullY(memory, cpu);
        OpIny(cpu);
        OpCpy(cpu, 4u);
    } while (!cpu->zero);
}

void BattleInitializeDisplayRecords(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    static const uint16_t clear_bytes[] = {0x15a8u, 0x15a9u, 0x15c7u, 0x15cbu,
                                           0x15cfu, 0x15d3u, 0x15d7u, 0x15dbu,
                                           0x15dfu, 0x15e3u, 0x15e7u};
    static const uint16_t slot_words[] = {0x48c0u, 0x4910u, 0x4956u, 0x4abeu, 0x4b36u};
    static const uint16_t slot_destinations[] = {0x15c8u, 0x15ccu, 0x15d4u, 0x15d8u,
                                                 0x15e8u};

    for (unsigned i = 0; i < sizeof(clear_bytes) / sizeof(clear_bytes[0]); ++i)
        OpStz(memory, cpu, OpAbs(cpu, clear_bytes[i]));

    for (unsigned i = 0; i < sizeof(slot_words) / sizeof(slot_words[0]); ++i) {
        OpLdx(cpu, slot_words[i]);
        OpWriteX(memory, cpu, OpAbs(cpu, slot_destinations[i]), cpu->x);
    }

    OpLdx(cpu, 0xffffu);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x1321u), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x1323u), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x131eu), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x1320u), cpu->x);

    LoadA8(cpu, 0x30u);
    OpSta(memory, cpu, OpAbs(cpu, 0x1475u));
    OpSta(memory, cpu, OpAbs(cpu, 0x1476u));
    OpSta(memory, cpu, OpAbs(cpu, 0x1478u));
    LoadA8(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, 0x1477u));

    OpLdx(cpu, 95u);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, 0x1b17u));
        OpDex(cpu);
    } while (!cpu->negative);

    OpStz(memory, cpu, OpDp(cpu, 0xdbu));

    OpLdx(cpu, 10u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, 0xb2fcu));
        OpSta(memory, cpu, OpAbsX(cpu, 0x122fu));
        OpDex(cpu);
    } while (!cpu->negative);

    OpLdx(cpu, 15u);
    LoadA8(cpu, 0xffu);
    do {
        OpSta(memory, cpu, OpAbsX(cpu, 0x12e3u));
        OpDex(cpu);
    } while (!cpu->negative);

    LoadA8(cpu, 0x6cu);
    OpSta(memory, cpu, 0x001be6u);
    LoadA8(cpu, 0x71u);
    OpSta(memory, cpu, 0x001be4u);
    LoadA8(cpu, 0x76u);
    OpSta(memory, cpu, 0x001be2u);
    LoadA8(cpu, 0x7bu);
    OpSta(memory, cpu, 0x001be0u);
}

bool BattlePrepareDisplayRecords(BattleContext *battle) {
    if (!BattleLoadDisplayDefaults(battle))
        return false;
    if (!BattleCall(battle, 0x8627u, 0x81e877u, 2u))
        return false;
    return BattleCall(battle, 0x862au, 0x85ec81u, 3u);
}

void BattleApplyPpuTable(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    OpSepWidths(cpu, 0x30u);
    OpLdy(cpu, 0u);
    OpTyx(cpu);

    for (;;) {
        OpLdx(cpu, OpReadX(memory, cpu, OpAbsY(cpu, 0xb5d3u)));
        if (cpu->negative)
            return;

        OpIny(cpu);
        OpLda(memory, cpu, OpAbsY(cpu, 0xb5d3u));
        OpIny(cpu);
        OpSta(memory, cpu, OpAbsX(cpu, SNES_INIDISP));
    }
}

bool BattleLoadBaseGraphics(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;

    OpRepWidths(cpu, 0x10u);
    Lufia2InstallSceneNmi(memory, cpu, 0x8dc5u, 0x85u);

    OpLdx(cpu, 0x8800u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x60u), cpu->x);
    LoadA8(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, 0x62u));
    OpLdx(cpu, 0x0190u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x54u), cpu->x);

    if (!BattleDecompressResource(battle, 0x865du))
        return false;
    if (!BattleLoadPortraits(battle))
        return false;
    if (!BattleCall(battle, 0x8665u, 0x81bc55u, 2u))
        return false;
    if (!BattleCall(battle, 0x8668u, 0x85df44u, 3u))
        return false;
    if (!BattleCall(battle, 0x866cu, 0x85eb91u, 3u))
        return false;
    return BattleCall(battle, 0x8670u, 0x85de9du, 3u);
}

typedef struct BattleVramTransfer {
    uint16_t vram_address;
    uint32_t source;
    uint16_t size;
} BattleVramTransfer;

static void UploadVramBlock(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                            const BattleVramTransfer *transfer) {
    OpLdx(cpu, transfer->vram_address);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_VMADDL), cpu->x);
    LoadA8(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_DMAP(0)));
    OpLdx(cpu, (uint16_t)transfer->source);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_A1TL(0)), cpu->x);
    LoadA8(cpu, (uint8_t)(transfer->source >> 16));
    OpSta(memory, cpu, OpAbs(cpu, SNES_A1B(0)));
    OpLdx(cpu, transfer->size);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_DASL(0)), cpu->x);
    LoadA8(cpu, 0x18u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_BBAD(0)));
    LoadA8(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_MDMAEN));
}

void BattleUploadBaseTiles(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    static const BattleVramTransfer transfers[] = {
        {0x1000u, 0x9f8500u, 0x0f00u},
        {0x1780u, 0x96ff3cu, 0x00b0u},
    };

    for (unsigned i = 0; i < sizeof(transfers) / sizeof(transfers[0]); ++i)
        UploadVramBlock(memory, cpu, &transfers[i]);
}

bool BattleLoadPresentationAssets(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    static const uint32_t resource_children[9] = {0x859addu, 0x859af4u, 0x859b0bu,
                                                  0x859b39u, 0x859b50u, 0x859b67u,
                                                  0x859b7eu, 0x859b95u, 0x859bacu};

    if (!BattlePrepareBackground(battle))
        return false;

    LoadA8(cpu, 0x97u);
    OpSta(memory, cpu, OpDp(cpu, 0x24u));
    OpLdy(cpu, 0xfde6u);
    TransferDirectToA(cpu);
    if (!BattleLoadPalette(battle, 0x86ccu))
        return false;

    LoadA8(cpu, 0x97u);
    OpSta(memory, cpu, OpDp(cpu, 0x24u));
    OpLdy(cpu, 0xcd38u);
    LoadA8(cpu, 8u);
    if (!BattleLoadPalette(battle, 0x86d8u))
        return false;

    LoadA8(cpu, 2u);
    OpSta(memory, cpu, OpAbs(cpu, 0x15abu));
    if (!BattleCall(battle, 0x86e0u, 0x858a39u, 3u))
        return false;
    if (!BattleCommitPalettes(battle))
        return false;

    LoadA8(cpu, 0xffu);
    OpSta(memory, cpu, 0x0012f3u);

    OpRepWidths(cpu, 0x20u);
    for (unsigned i = 0; i < 9u; ++i) {
        if (!BattleCall(battle, (uint16_t)(0x86f0u + 4u * i), resource_children[i], 3u))
            return false;
    }
    OpSepWidths(cpu, 0x20u);
    return true;
}

void BattlePrepareSpriteState(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    LoadA8(cpu, 0x40u);
    OpSta(memory, cpu, OpDp(cpu, 0x72u));
    OpAslA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_FRAME_STATE));

    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7eu);

    OpLdy(cpu, 0u);
    OpTyx(cpu);
    LoadA8(cpu, 10u);
    OpSta(memory, cpu, OpDp(cpu, 0x1bu));
    do {
        OpLda(memory, cpu, OpAbsY(cpu, 0x161bu));
        OpCmpValue(cpu, 16u);
        if (cpu->zero)
            TransferDirectToA(cpu);
        OpSta(memory, cpu, OpLongX(cpu, 0x7ee700u));
        OpIny(cpu);
        OpInx(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, 0x1bu), -1);
    } while (!cpu->zero);

    PullDataBank(memory, cpu);
}

bool BattleFinishDisplay(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;

    if (!BattleCall(battle, 0x873eu, 0x85ab5bu, 3u))
        return false;
    if (!BattleCall(battle, 0x8742u, 0x85ec81u, 3u))
        return false;

    OpSta(memory, cpu, OpAbs(cpu, 0x123au));
    if (BattleControlHas(battle, BATTLE_CONTROL_MODE_2)) {
        OpBitValue(cpu, BATTLE_CONTROL_MODE_1);
        if (!cpu->zero)
            return true;
        OpLdx(cpu, 0xf291u);
    } else {
        OpLdx(cpu, 0xf283u);
    }

    if (!BattleCall(battle, 0x875cu, 0x8594e7u, 3u))
        return false;

    OpLdx(cpu, 30u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_BATTLE_WAIT_COUNTER), cpu->x);

    return BattleCall(battle, 0x8766u, 0x8595feu, 3u);
}
