#include "battle/battle_internal.h"
#include "core/wram_view.h"

enum {
    GRAPHICS_STREAM = 0xc3u,
    GRAPHICS_RESOURCE = 0x54u,
    GRAPHICS_DESTINATION = 0x60u,
    GRAPHICS_DESTINATION_BANK = 0x62u,
    GRAPHICS_WORK_BUFFER = WRAM_BATTLE_EFFECT_GRAPHICS_BUFFER & 0xffffu,
    GRAPHICS_WORK_BANK = 0x7eu,
    GRAPHICS_QUEUE_LENGTH = WRAM_BATTLE_VRAM_QUEUE,
    GRAPHICS_QUEUE_SOURCE = WRAM_BATTLE_VRAM_QUEUE + 2u,
    GRAPHICS_QUEUE_DESTINATION = WRAM_BATTLE_VRAM_QUEUE + 4u,
    ROM_GRAPHICS_RESOURCES = 0x85ef53u,
    ROM_GRAPHICS_QUEUE_SLOT = 0x85ecdbu
};

static bool GraphicsContext(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit &&
        cpu->direct_page == 0u && cpu->program_bank == 0x81u &&
        cpu->stack >= 0x1f05u && cpu->stack <= 0x1ffcu;
}

static uint16_t GraphicsStreamWord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const uint16_t value = Read16Long(
        memory, DirectLongPointer(memory, cpu, GRAPHICS_STREAM));
    Lufia2Wram wram = WramViewOfCaller(memory, cpu);

    LoadA16(cpu, value);
    SetNz16(cpu, WramStep16(wram, GRAPHICS_STREAM, 1));
    SetNz16(cpu, WramStep16(wram, GRAPHICS_STREAM, 1));
    return value;
}

static Lufia2ExecutionResult EffectGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context, uint16_t entry) {
    BattleContext battle = BattleContextCreate(
        memory, cpu, child, child_context, 0x81u);
    Lufia2Wram wram;
    uint16_t resource;

    if (!GraphicsContext(cpu))
        return ExecutionHandoff(cpu, 0x810000u | entry);
    PushY(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    resource = GraphicsStreamWord(memory, cpu);
    cpu->carry = (resource & 0x8000u) != 0u;
    cpu->x = (uint16_t)(resource << 1);
    SetNz16(cpu, cpu->x);
    LoadA16(cpu, Read16Long(memory, ROM_GRAPHICS_RESOURCES + cpu->x));
    wram = WramViewOfCaller(memory, cpu);
    WramWrite16(wram, GRAPHICS_RESOURCE, cpu->accumulator);
    LoadX16(cpu, GRAPHICS_WORK_BUFFER);
    WramWrite16(wram, GRAPHICS_DESTINATION, cpu->x);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, GRAPHICS_WORK_BANK);
    WramWrite(wram, GRAPHICS_DESTINATION_BANK, A8(cpu));
    if (!BattleCall(&battle, (uint16_t)(entry + 0x1cu),
                    BATTLE_ROUTINE_DECOMPRESS_RESOURCE, 3u))
        return BattleChildUnwound(&battle);
    if (cpu->index_is_8_bit || cpu->direct_page != 0u)
        return ExecutionHandoff(cpu, 0x810000u | (uint16_t)(entry + 0x20u));
    SetAccumulatorWidth(cpu, 0);
    if (!BattleCall(&battle, (uint16_t)(entry + 0x22u),
                    ROM_GRAPHICS_QUEUE_SLOT, 3u))
        return BattleChildUnwound(&battle);
    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit ||
        cpu->direct_page != 0u)
        return ExecutionHandoff(cpu, 0x810000u | (uint16_t)(entry + 0x26u));
    wram = WramViewOfCaller(memory, cpu);
    LoadA16(cpu, GRAPHICS_WORK_BUFFER);
    WramWrite16At(wram, GRAPHICS_QUEUE_SOURCE, cpu->y, cpu->accumulator);
    (void)GraphicsStreamWord(memory, cpu);
    WramWrite16At(wram, GRAPHICS_QUEUE_DESTINATION, cpu->y, cpu->accumulator);
    (void)GraphicsStreamWord(memory, cpu);
    WramWrite16At(wram, GRAPHICS_QUEUE_LENGTH, cpu->y, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    OpPullY(memory, cpu);
    return ExecutionReturned(0x810000u | (uint16_t)(entry + 0x41u));
}

Lufia2ExecutionResult Lufia2BattleEffectGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    return EffectGraphics(memory, cpu, child, child_context, 0x99d0u);
}

Lufia2ExecutionResult Lufia2BattleEffectGraphicsAlternate(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    return EffectGraphics(memory, cpu, child, child_context, 0x9a12u);
}
