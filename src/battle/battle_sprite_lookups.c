#include "core/cpu_ops.h"
#include "lufia2/battle.h"

enum {
    CHARACTER_SPRITE_TABLE = 0x9685c0u,
    CHARACTER_SPRITE_WORD = 0x9685cfu,
    SPRITE_COORDINATES_TABLE = 0x97ca64u,
    SPRITE_COORDINATES_MASK = 0x1f1fu
};

static bool LookupContext(const Lufia2CpuState *cpu) {
    return !cpu->index_is_8_bit && cpu->program_bank == 0x81u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static void SaveLookupRegisters(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));
    PushIndex(memory, cpu);
}

static void RestoreLookupRegisters(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    cpu->x = PullIndexValue(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
}

static void SelectLookupEntry(Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    LoadA16(cpu, cpu->accumulator & 0xffu);
    AslA16(cpu);
    LoadX16(cpu, cpu->accumulator);
}

Lufia2ExecutionResult Lufia2CharacterSpriteWord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!LookupContext(cpu))
        return ExecutionHandoff(cpu, 0x81fbc6u);
    SaveLookupRegisters(memory, cpu);
    SelectLookupEntry(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(CHARACTER_SPRITE_TABLE, cpu->x)));
    LoadX16(cpu, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(CHARACTER_SPRITE_WORD, cpu->x)));
    RestoreLookupRegisters(memory, cpu);
    return ExecutionReturned(0x81fbdau);
}

Lufia2ExecutionResult Lufia2SpriteCoordinatesPacked(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!LookupContext(cpu))
        return ExecutionHandoff(cpu, 0x81fb8eu);
    SaveLookupRegisters(memory, cpu);
    if (cpu->accumulator_is_8_bit)
        DecrementA8(cpu);
    else
        LoadA16(cpu, (uint16_t)(cpu->accumulator - 1u));
    SelectLookupEntry(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(SPRITE_COORDINATES_TABLE, cpu->x)));
    LoadA16(cpu, cpu->accumulator & SPRITE_COORDINATES_MASK);
    RestoreLookupRegisters(memory, cpu);
    return ExecutionReturned(0x81fba1u);
}
