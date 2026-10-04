/* Select screen setup: the video registers and the four tile map layers of
 * the menu scenes, and the sprite slot table clear. */

#include "core/cpu_internal.h"
#include "core/wram_view.h"
#include "lufia2/menu.h"

enum {
    LAYER_A = 0x7e2000u,
    LAYER_B = 0x7e2800u,
    LAYER_C = 0x7e3000u,
    LAYER_D = 0x7e3800u,
    LAYER_END = 0x0800u,
    SCROLL_WORDS = 0x0594u,
    SCROLL_END = 0x0010u,
    SLOT_FLAGS = 0x11d8u,
    SLOT_FLAGS_SIZE = 0x03e8u,
    REDRAW_FLAGS = 0x74u,
    FRAME_FLAGS = 0x72u
};

typedef struct VideoByte {
    uint16_t port;
    uint8_t value;
} VideoByte;

/* $2101 sprite size, $210B/$210C character bases, $2107-$210A tile map
 * addresses and $2105 the mode. */
static const VideoByte kVideoSetup[] = {
    {0x2101u, 0x01u}, {0x210bu, 0x44u}, {0x210cu, 0x66u}, {0x2107u, 0x00u},
    {0x2108u, 0x04u}, {0x2109u, 0x08u}, {0x210au, 0x0cu}, {0x2105u, 0x09u}
};

/* Windows, color math and layer enables. */
static const VideoByte kVideoLayers[] = {
    {0x2106u, 0x00u}, {0x2123u, 0x00u}, {0x2124u, 0x00u}, {0x2125u, 0x00u},
    {0x2126u, 0x00u}, {0x2127u, 0x00u}, {0x2128u, 0x00u}, {0x2129u, 0x00u},
    {0x212cu, 0x1fu}, {0x212du, 0x00u}, {0x2132u, 0xe0u}
};

static void WriteVideo(Lufia2Wram wram, const VideoByte *bytes, unsigned count) {
    unsigned i;

    for (i = 0; i < count; ++i)
        WramWrite(wram, bytes[i].port, bytes[i].value);
}

/* $86:8DD7: video registers and empty tile maps for the select screen, the
 * OAM cleared, then the sprite frame wait. M1, X16. Hands off at $86:8B48
 * with its return pushed. */
Lufia2ExecutionResult Lufia2MenuScreenSetup(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x868dd7u);
    WriteVideo(wram, kVideoSetup, 8u);
    SetAccumulatorWidth(cpu, 0);
    LoadX16(cpu, 0);
    do {
        WramWrite16At(wram, SCROLL_WORDS, cpu->x, 0);
        IncrementX16(cpu);
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, SCROLL_END);
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);
    WriteVideo(wram, kVideoLayers, 11u);
    SetAccumulatorWidth(cpu, 0);
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadX16(cpu, 0);
    do {
        Write16Long(memory, LongIndexedAddress(LAYER_A, cpu->x), 0);
        Write16Long(memory, LongIndexedAddress(LAYER_B, cpu->x), 0);
        Write16Long(memory, LongIndexedAddress(LAYER_C, cpu->x), 0);
        Write16Long(memory, LongIndexedAddress(LAYER_D, cpu->x), 0);
        IncrementX16(cpu);
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, LAYER_END);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0xaau);
    StoreADirect8(memory, cpu, REDRAW_FLAGS);
    SimulateJslFrame(memory, cpu, 0x86u, 0x8e62u);
    (void)Lufia2SpriteClearOam(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    LoadA8(cpu, 0x80u);
    StoreADirect8(memory, cpu, FRAME_FLAGS);
    SimulateJsrFrame(memory, cpu, 0x8e69u);
    return ExecutionHandoff(cpu, 0x868b48u);
}

/* $86:8E6B: all sprite slot flags off; M1X0. */
Lufia2ExecutionResult Lufia2SpriteClearSlots(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint16_t slot;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x868e6bu);
    for (slot = SLOT_FLAGS; slot < SLOT_FLAGS + SLOT_FLAGS_SIZE; ++slot)
        WramWriteAt(wram, SLOT_FLAGS, (uint16_t)(slot - SLOT_FLAGS), 0);
    cpu->x = SLOT_FLAGS + SLOT_FLAGS_SIZE;
    cpu->y = 0;
    cpu->zero = 1;
    cpu->negative = 0;
    return ExecutionReturned(0x868e78u);
}
