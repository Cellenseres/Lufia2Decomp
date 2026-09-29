/* Battle background preparation at $81:B9C7. */

#include "battle/battle_internal.h"
#include "core/snes_registers.h"

enum {
    BATTLE_BACKGROUND_DESCRIPTOR = 0x11e2u,
    BATTLE_BACKGROUND_TABLE = 0x97fd40u,
    BATTLE_BACKGROUND_TILEMAP = 0x7e2000u,
    BATTLE_BACKGROUND_TILES = 0x7ec000u,
};

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

static uint8_t DecodeBackgroundResources(Lufia2BattleChildCalls *calls) {
    Lufia2CpuState *cpu = calls->cpu;
    const Lufia2Memory *memory = calls->memory;

    OpLdx(cpu, 0xc000u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x60u), cpu->x);
    LoadA8(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, 0x62u));

    OpLda(memory, cpu, OpAbs(cpu, BATTLE_BACKGROUND_DESCRIPTOR + 1u));
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0x00ffu);
    cpu->carry = 0;
    OpAdcValue(cpu, 0x016cu);
    OpSta(memory, cpu, OpDp(cpu, 0x54u));
    if (!Lufia2BattleCallChild(calls, 0xba08u, 0x808e9du, 3u))
        return 0;

    OpLda(memory, cpu, OpAbs(cpu, BATTLE_BACKGROUND_DESCRIPTOR));
    OpAndValue(cpu, 0x00ffu);
    cpu->carry = 0;
    OpAdcValue(cpu, 0x0179u);
    OpSta(memory, cpu, OpDp(cpu, 0x54u));
    OpSepWidths(cpu, 0x20u);

    OpLdx(cpu, 0x2000u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x60u), cpu->x);
    LoadA8(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, 0x62u));
    return Lufia2BattleCallChild(calls, 0xba23u, 0x808e9du, 3u);
}

static uint8_t LoadBackgroundPalettes(Lufia2BattleChildCalls *calls) {
    Lufia2CpuState *cpu = calls->cpu;
    const Lufia2Memory *memory = calls->memory;

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
    if (!Lufia2BattleCallChild(calls, 0xba44u, 0x81b974u, 2u))
        return 0;
    LoadA8(cpu, 3u);
    return Lufia2BattleCallChild(calls, 0xba49u, 0x81b974u, 2u);
}

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

static void ClearBackgroundBuffers(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
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

Lufia2ExecutionResult Lufia2BattleBackgroundPrepare(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu,
                                                    Lufia2PushedChildCall child,
                                                    void *child_context) {
    Lufia2BattleChildCalls calls = {memory, cpu, child, child_context, 0x81u, 0u};

    TransferDirectToA(cpu); /* $81:B9C7 */
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_BACKGROUND_ID));
    const uint8_t background_id = A8(cpu);
    OpCmpValue(cpu, BATTLE_BACKGROUND_BLANK);

    if (cpu->zero) {
        ClearBackgroundBuffers(memory, cpu);
    } else {
        LoadBackgroundDescriptor(memory, cpu, background_id);
        if (!DecodeBackgroundResources(&calls))
            return Lufia2BattleChildUnwound(&calls);
        if (!LoadBackgroundPalettes(&calls))
            return Lufia2BattleChildUnwound(&calls);
        DecodeBackgroundTilemap(memory, cpu);
    }

    LoadA8(cpu, 1u);
    OpTestBits(memory, cpu, OpDp(cpu, 0xd9u), 0u);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, 0x1b17u));

    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_BACKGROUND_ID));
    if (cpu->zero && !Lufia2BattleCallChild(&calls, 0xbac6u, 0x85a701u, 3u))
        return Lufia2BattleChildUnwound(&calls);

    return ExecutionReturned(0x81bacau);
}
