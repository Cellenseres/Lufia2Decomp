/* Battle transition. */

#include "battle/battle_internal.h"
#include "core/snes_registers.h"
#include "core/wram_view.h"

enum {
    TRANSITION_ANGLE = 0x00u,
    TRANSITION_ANGLE_HI = 0x01u,
    TRANSITION_RADIUS = 0x04u,
    TRANSITION_RADIUS_HI = 0x05u,
    TRANSITION_CONTROL = 0x74u,
    TRANSITION_HDMA_FLAGS = 0x81u,
    SWIRL_RIM_TABLE = 0x3040u,
    SWIRL_RIM_BYTES = 0x40u,
    SWIRL_HDMA_TABLE = 0x3100u,
    SWIRL_HALF_BYTES = 0xe0u,
    TRANSITION_CLEAR_START = 0x07fcu,
    SHADOW_BG3HOFS = 0x059cu,
    SHADOW_BG3VOFS = 0x059eu,
    FADE_CONTROL_MOSAIC = 0xc4u,
    SWIRL_HDMA_CHANNELS = 0x03u,
    TRANSITION_RUNNING = 0x08u,
    SWIRL_HDMA_END = 2u * SWIRL_HALF_BYTES + 1u,
    SWIRL_HALF_LINES = 0xf0u,
    SWIRL_RIM_CENTER = 0x1228u, /* work RAM words added to the rings */
    SWIRL_HDMA_CENTER = 0x1220u,
    SWIRL_SINE_TABLE = 0x8084edu,
};

static bool WaitForTransitionFrame(BattleContext *battle, uint16_t site) {
    return BattleCall(battle, site, 0x848d4du, 2u);
}

/* Fills three $800-byte areas of work RAM, each as two interleaved word
 * sequences, with the direct page address. */
static void ClearTransitionPlanes(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint32_t kPlanes[6] = {WRAM_MUSIC_SAMPLE_CACHE,   0x7e2002u,
                                        WRAM_FIELD_LAYER0_TILEMAP, 0x7e2802u,
                                        WRAM_FIELD_LAYER2_TILEMAP, 0x7e3002u};
    const Lufia2Wram wram = WramViewLong(memory);
    const uint16_t blank = cpu->direct_page;
    uint16_t offset;
    unsigned plane;

    for (offset = TRANSITION_CLEAR_START; offset < 0x8000u;
         offset = (uint16_t)(offset - 4u)) {
        for (plane = 0; plane < 6u; ++plane)
            WramWrite16At(wram, kPlanes[plane], offset, blank);
    }
    TransferDirectToA(cpu);
    LoadX16(cpu, 0xfffcu);
    SetAccumulatorWidth(cpu, 1);
}

static void SeedMosaicTransition(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const Lufia2Wram io = WramViewLong(memory);
    const uint8_t scroll = (uint8_t)cpu->direct_page;

    WramWrite(io, SNES_BGMODE, 0x02u);
    /* Both background 3 scroll registers take the direct page's low byte, and
     * so do their shadows. */
    WramWrite(io, SNES_BG3HOFS, scroll);
    WramWrite(io, SNES_BG3HOFS, scroll);
    WramWrite(io, SHADOW_BG3HOFS, scroll);
    WramWrite(io, SHADOW_BG3HOFS + 1u, scroll);
    WramWrite(io, SNES_BG3VOFS, scroll);
    WramWrite(io, SNES_BG3VOFS, scroll);
    WramWrite(io, SHADOW_BG3VOFS, scroll);
    WramWrite(io, SHADOW_BG3VOFS + 1u, scroll);

    WramWrite(io, WRAM_FIELD_MOSAIC_STATE, 0x01u);
    WramWrite(io, WRAM_FIELD_MOSAIC_ACCUMULATOR, 0x80u);
    WramWrite(wram, WRAM_FIELD_TRANSITION_FLAGS, 0x01u);

    WramWrite(wram, TRANSITION_ANGLE, 0);
    WramWrite(wram, TRANSITION_ANGLE_HI, 0);
    WramWrite(wram, TRANSITION_RADIUS, 0);
    WramWrite(wram, TRANSITION_RADIUS_HI, 0);
    WramWrite(wram, WRAM_FADE_LEVEL, 0);
    WramWrite(wram, WRAM_FADE_CONTROL, FADE_CONTROL_MOSAIC);
    TransferDirectToA(cpu);
    LoadA8(cpu, FADE_CONTROL_MOSAIC);
}

/* Fills the two scanline tables of the swirl. The rim table at $3040 holds one
 * background scroll word per ring; the HDMA table at $3100 holds two runs of
 * 112 words for the upper and the lower half of the screen. Every entry is
 * the hardware product of a sine table value and the current radius, offset by
 * a centre word kept in work RAM. */
static void BuildSwirlTilemaps(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const Lufia2Wram ppu = WramViewLong(memory);
    uint16_t angle = (uint16_t)((WramRead16(wram, TRANSITION_ANGLE) + 2u) & 0x00ffu);
    uint16_t radius;
    uint16_t phase;
    uint16_t y;

    WramWrite16(wram, TRANSITION_ANGLE, angle);
    phase = (uint16_t)((angle + 0x50u) & 0x00ffu);
    radius = (uint16_t)((WramRead16(wram, TRANSITION_RADIUS) + 8u) & 0x00ffu);
    WramWrite16(wram, TRANSITION_RADIUS, radius);
    WramWrite(ppu, SNES_M7A, (uint8_t)radius);
    WramWrite(ppu, SNES_M7A, 0);

    for (y = 0; y != SWIRL_RIM_BYTES; y = (uint16_t)(y + 2u)) {
        WramWrite(ppu, SNES_M7B,
                  Read8(memory, LongIndexedAddress(SWIRL_SINE_TABLE, phase)));
        WramWrite16At(wram, SWIRL_RIM_TABLE, y,
                      (uint16_t)(((WramRead16(ppu, SNES_MPYM) +
                                   WramRead16(ppu, SWIRL_RIM_CENTER)) &
                                  0x03ffu) |
                                 0x6000u));
        phase = (uint16_t)((phase + 8u) & 0x00ffu);
    }

    WramWrite(wram, SWIRL_HDMA_TABLE, SWIRL_HALF_LINES);
    WramWrite(wram, SWIRL_HDMA_TABLE + SWIRL_HALF_BYTES + 1u, SWIRL_HALF_LINES);
    WramWrite(wram, SWIRL_HDMA_TABLE + SWIRL_HDMA_END, 0);

    WramWrite(ppu, SNES_M7A, WramRead(wram, TRANSITION_RADIUS));
    WramWrite(ppu, SNES_M7A, WramRead(wram, TRANSITION_RADIUS_HI));
    /* The phase starts from the direct page's high byte and the angle. */
    phase = (uint16_t)((cpu->direct_page & 0xff00u) | WramRead(wram, TRANSITION_ANGLE));
    y = 0;
    do {
        WramWrite(ppu, SNES_M7B,
                  Read8(memory, LongIndexedAddress(SWIRL_SINE_TABLE, phase)));
        LoadA16(cpu, WramRead16(ppu, SNES_MPYM));
        cpu->carry = 0;
        Add16Value(cpu, WramRead16(ppu, SWIRL_HDMA_CENTER));
        WramWrite16At(wram, SWIRL_HDMA_TABLE + 1u, y, cpu->accumulator);
        phase = (uint16_t)((phase + 1u) & 0x00ffu);
        y = (uint16_t)(y + 2u);
        if (y == SWIRL_HALF_BYTES)
            ++y;
    } while (y != SWIRL_HDMA_END);

    /* Registers as the last iteration left them. */
    cpu->accumulator = phase;
    cpu->x = phase;
    cpu->y = y;
    Compare16(cpu, cpu->y, SWIRL_HDMA_END);
    SetAccumulatorWidth(cpu, 1);
}

/* Points HDMA channels 0 and 1 at the swirl table in work RAM; they write the
 * horizontal scroll registers of backgrounds 1 and 2 ($210D and $210F). */
static void ConfigureSwirlHdma(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const Lufia2Wram io = WramViewLong(memory);
    const uint8_t flags = WramRead(wram, TRANSITION_HDMA_FLAGS);
    const uint8_t control = WramRead(wram, TRANSITION_CONTROL);
    unsigned channel;

    for (channel = 0; channel < 2u; ++channel)
        WramWrite16(io, SNES_A1TL(channel), SWIRL_HDMA_TABLE);
    WramWrite(io, SNES_DMAP(0), 0x02u);
    WramWrite(io, SNES_DMAP(1), 0x02u);
    WramWrite(io, SNES_BBAD(0), 0x0du);
    WramWrite(io, SNES_BBAD(1), 0x0fu);
    WramWrite(io, SNES_A1B(0), 0x7eu);
    WramWrite(io, SNES_A1B(1), 0x7eu);

    WramWrite(wram, TRANSITION_HDMA_FLAGS, (uint8_t)(flags | SWIRL_HDMA_CHANNELS));
    WramWrite(wram, TRANSITION_CONTROL, (uint8_t)(control | TRANSITION_RUNNING));
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, SWIRL_HDMA_TABLE);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, TRANSITION_RUNNING);
    cpu->zero = (control & TRANSITION_RUNNING) == 0;
}

Lufia2ExecutionResult Lufia2BattleVisualTransition(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x84u);

    PushDataBank(memory, cpu);
    ClearTransitionPlanes(memory, cpu);

    LoadA8(cpu, 0x08u);
    OpSta(memory, cpu, OpDp(cpu, TRANSITION_CONTROL));
    if (!WaitForTransitionFrame(&battle, 0x8bf2u))
        return BattleChildUnwound(&battle);

    OpLda(memory, cpu, 0x7f0001u);
    if (cpu->zero)
        LoadA8(cpu, 0x0fu);
    else
        OpAndValue(cpu, 0x0fu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_TM));

    OpLdx(cpu, 0x0000u);
    if (!BattleCall(&battle, 0x8c07u, 0x80f47au, 3u))
        return BattleChildUnwound(&battle);
    OpLdx(cpu, 0x0002u);
    if (!BattleCall(&battle, 0x8c0eu, 0x80f47au, 3u))
        return BattleChildUnwound(&battle);

    LoadA8(cpu, 0xa0u);
    OpSta(memory, cpu, OpDp(cpu, TRANSITION_CONTROL));
    if (!WaitForTransitionFrame(&battle, 0x8c16u))
        return BattleChildUnwound(&battle);

    SeedMosaicTransition(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7eu);

    do {
        BuildSwirlTilemaps(memory, cpu);
        ConfigureSwirlHdma(memory, cpu);

        if (!WaitForTransitionFrame(&battle, 0x8d3fu))
            return BattleChildUnwound(&battle);

        OpLda(memory, cpu, OpDp(cpu, TRANSITION_ANGLE));
        OpCmpValue(cpu, 0x40u);
    } while (!cpu->zero);

    PullDataBank(memory, cpu);
    return ExecutionReturned(0x848d4cu);
}
