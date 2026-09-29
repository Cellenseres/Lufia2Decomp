/* Battle display and resource setup at $81:851E. */

#include "battle/battle_internal.h"
#include "core/snes_registers.h"
#include "system/scene_nmi_internal.h"
#include "system/wram.h"

static void ResetBattleDisplayWork(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpStz(memory, cpu, OpDp(cpu, 0x74u));                    /* $81:851E */
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

static uint8_t ClearBattleDisplayBuffers(
    Lufia2BattleChildCalls *calls) {
    const Lufia2Memory *memory = calls->memory;
    Lufia2CpuState *cpu = calls->cpu;

    if (!Lufia2BattleCallChild(calls, 0x8541u, 0x81c2d0u, 2u))
        return 0;
    if (!Lufia2BattleCallChild(calls, 0x8544u, 0x81c2e3u, 2u))
        return 0;
    if (!Lufia2BattleCallChild(calls, 0x8547u, 0x81c2fbu, 2u))
        return 0;
    if (!Lufia2BattleCallChild(calls, 0x854au, 0x81c30eu, 2u))
        return 0;

    OpStz(memory, cpu, OpDp(cpu, 0xd8u));
    OpStz(memory, cpu, OpDp(cpu, 0xd9u));
    OpStz(memory, cpu, OpAbs(cpu, BATTLE_WRAM_FRAME_STATE));

    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 90u);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, 0x1a8fu));
        for (unsigned i = 0; i < 6u; ++i)
            OpDex(cpu);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);

    return 1;
}

static void BuildPartyDisplayState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
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

static void InitializeBattleDisplayRecords(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint16_t clear_bytes[] = {
        0x15a8u, 0x15a9u, 0x15c7u, 0x15cbu, 0x15cfu,
        0x15d3u, 0x15d7u, 0x15dbu, 0x15dfu, 0x15e3u, 0x15e7u};
    static const uint16_t slot_words[] = {
        0x48c0u, 0x4910u, 0x4956u, 0x4abeu, 0x4b36u};
    static const uint16_t slot_destinations[] = {
        0x15c8u, 0x15ccu, 0x15d4u, 0x15d8u, 0x15e8u};

    for (unsigned i = 0;
         i < sizeof(clear_bytes) / sizeof(clear_bytes[0]); ++i)
        OpStz(memory, cpu, OpAbs(cpu, clear_bytes[i]));

    for (unsigned i = 0;
         i < sizeof(slot_words) / sizeof(slot_words[0]); ++i) {
        OpLdx(cpu, slot_words[i]);
        OpWriteX(
            memory, cpu, OpAbs(cpu, slot_destinations[i]), cpu->x);
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

static uint8_t PrepareBattleDisplayRecords(
    Lufia2BattleChildCalls *calls) {
    if (!Lufia2BattleCallChild(calls, 0x8624u, 0x81c2c0u, 2u))
        return 0;
    if (!Lufia2BattleCallChild(calls, 0x8627u, 0x81e877u, 2u))
        return 0;
    return Lufia2BattleCallChild(calls, 0x862au, 0x85ec81u, 3u);
}

static void ApplyBattlePpuTable(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
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

static uint8_t LoadBattleBaseGraphics(
    Lufia2BattleChildCalls *calls) {
    const Lufia2Memory *memory = calls->memory;
    Lufia2CpuState *cpu = calls->cpu;

    OpRepWidths(cpu, 0x10u);
    Lufia2InstallSceneNmi(memory, cpu, 0x8dc5u, 0x85u);

    OpLdx(cpu, 0x8800u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x60u), cpu->x);
    LoadA8(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, 0x62u));
    OpLdx(cpu, 0x0190u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x54u), cpu->x);

    if (!Lufia2BattleCallChild(calls, 0x865du, 0x808e9du, 3u))
        return 0;
    if (!Lufia2BattleCallChild(calls, 0x8661u, 0x81bae8u, 3u))
        return 0;
    if (!Lufia2BattleCallChild(calls, 0x8665u, 0x81bc55u, 2u))
        return 0;
    if (!Lufia2BattleCallChild(calls, 0x8668u, 0x85df44u, 3u))
        return 0;
    if (!Lufia2BattleCallChild(calls, 0x866cu, 0x85eb91u, 3u))
        return 0;
    return Lufia2BattleCallChild(calls, 0x8670u, 0x85de9du, 3u);
}

static void UploadBattleBaseTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 0x1000u);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_VMADDL), cpu->x);
    LoadA8(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_DMAP(0)));
    OpLdx(cpu, 0x8500u);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_A1TL(0)), cpu->x);
    LoadA8(cpu, 0x9fu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_A1B(0)));
    OpLdx(cpu, 0x0f00u);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_DASL(0)), cpu->x);
    LoadA8(cpu, 0x18u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_BBAD(0)));
    LoadA8(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_MDMAEN));

    OpLdx(cpu, 0x1780u);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_VMADDL), cpu->x);
    LoadA8(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_DMAP(0)));
    OpLdx(cpu, 0xff3cu);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_A1TL(0)), cpu->x);
    LoadA8(cpu, 0x96u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_A1B(0)));
    OpLdx(cpu, 0x00b0u);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_DASL(0)), cpu->x);
    LoadA8(cpu, 0x18u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_BBAD(0)));
    LoadA8(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_MDMAEN));
}

static uint8_t LoadBattlePresentationAssets(
    Lufia2BattleChildCalls *calls) {
    const Lufia2Memory *memory = calls->memory;
    Lufia2CpuState *cpu = calls->cpu;
    static const uint32_t resource_children[9] = {
        0x859addu, 0x859af4u, 0x859b0bu, 0x859b39u, 0x859b50u,
        0x859b67u, 0x859b7eu, 0x859b95u, 0x859bacu};

    if (!Lufia2BattleCallChild(calls, 0x86c0u, 0x81b9c7u, 3u))
        return 0;

    LoadA8(cpu, 0x97u);
    OpSta(memory, cpu, OpDp(cpu, 0x24u));
    OpLdy(cpu, 0xfde6u);
    TransferDirectToA(cpu);
    if (!Lufia2BattleCallChild(calls, 0x86ccu, 0x81b974u, 2u))
        return 0;

    LoadA8(cpu, 0x97u);
    OpSta(memory, cpu, OpDp(cpu, 0x24u));
    OpLdy(cpu, 0xcd38u);
    LoadA8(cpu, 8u);
    if (!Lufia2BattleCallChild(calls, 0x86d8u, 0x81b974u, 2u))
        return 0;

    LoadA8(cpu, 2u);
    OpSta(memory, cpu, OpAbs(cpu, 0x15abu));
    if (!Lufia2BattleCallChild(calls, 0x86e0u, 0x858a39u, 3u))
        return 0;
    if (!Lufia2BattleCallChild(calls, 0x86e4u, 0x81b9afu, 3u))
        return 0;

    LoadA8(cpu, 0xffu);
    OpSta(memory, cpu, 0x0012f3u);

    OpRepWidths(cpu, 0x20u);
    for (unsigned i = 0; i < 9u; ++i) {
        if (!Lufia2BattleCallChild(
                calls, (uint16_t)(0x86f0u + 4u * i),
                resource_children[i], 3u))
            return 0;
    }
    OpSepWidths(cpu, 0x20u);
    return 1;
}

static void PrepareBattleSpriteState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadA8(cpu, 0x40u);
    OpSta(memory, cpu, OpDp(cpu, 0x72u));
    OpAslA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_WRAM_FRAME_STATE));

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

static uint8_t FinishBattleDisplay(
    Lufia2BattleChildCalls *calls) {
    const Lufia2Memory *memory = calls->memory;
    Lufia2CpuState *cpu = calls->cpu;

    if (!Lufia2BattleCallChild(calls, 0x873eu, 0x85ab5bu, 3u))
        return 0;
    if (!Lufia2BattleCallChild(calls, 0x8742u, 0x85ec81u, 3u))
        return 0;

    OpSta(memory, cpu, OpAbs(cpu, 0x123au));
    Lufia2BattleLoadControlFlags(memory, cpu);
    OpBitValue(cpu, BATTLE_CONTROL_MODE_2);
    if (!cpu->zero) {
        OpBitValue(cpu, BATTLE_CONTROL_MODE_1);
        if (!cpu->zero)
            return 1;
        OpLdx(cpu, 0xf291u);
    } else {
        OpLdx(cpu, 0xf283u);
    }

    if (!Lufia2BattleCallChild(calls, 0x875cu, 0x8594e7u, 3u))
        return 0;

    OpLdx(cpu, 30u);
    OpWriteX(
        memory, cpu, OpAbs(cpu, BATTLE_WRAM_STARTUP_COUNTER), cpu->x);

    return Lufia2BattleCallChild(calls, 0x8766u, 0x8595feu, 3u);
}

Lufia2ExecutionResult Lufia2BattleDisplaySetup(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context) {
    Lufia2BattleChildCalls calls = {
        memory, cpu, child, child_context, 0x81u, 0u};

    ResetBattleDisplayWork(memory, cpu);

    if (!ClearBattleDisplayBuffers(&calls))
        return Lufia2BattleChildUnwound(&calls);

    BuildPartyDisplayState(memory, cpu);
    InitializeBattleDisplayRecords(memory, cpu);

    if (!PrepareBattleDisplayRecords(&calls))
        return Lufia2BattleChildUnwound(&calls);

    ApplyBattlePpuTable(memory, cpu);

    if (!LoadBattleBaseGraphics(&calls))
        return Lufia2BattleChildUnwound(&calls);

    UploadBattleBaseTiles(memory, cpu);

    if (!LoadBattlePresentationAssets(&calls))
        return Lufia2BattleChildUnwound(&calls);

    PrepareBattleSpriteState(memory, cpu);

    if (!FinishBattleDisplay(&calls))
        return Lufia2BattleChildUnwound(&calls);

    return ExecutionReturned(0x81876au);
}
