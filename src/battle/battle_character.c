/* Character sprite record lookups used by battle rendering. */

#include "core/cpu_internal.h"
#include "lufia2/battle.h"

/* Item, party and sprite helpers of bank $81. */

#include "core/cpu_internal.h"
#include "lufia2/battle.h"

static void SetBank(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t bank) {
    LoadA8(cpu, bank);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
}

/* $81:FB79: A = sprite byte of character A; X kept. */
Lufia2ExecutionResult Lufia2CharacterSpriteByte(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    PushIndex(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x9685c0u, cpu->x)));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x9685ceu, cpu->x)));
    cpu->x = PullIndexValue(memory, cpu);
    return ExecutionReturned(0x81fb8du);
}

/* $81:FBA2: packed sprite size; P and X kept, X16 required. */
Lufia2ExecutionResult Lufia2SpriteSizePacked(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));
    PushIndex(memory, cpu);
    if (cpu->accumulator_is_8_bit)
        DecrementA8(cpu);
    else
        LoadA16(cpu, (uint16_t)(cpu->accumulator - 1u));
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, (uint8_t)(Read8(memory, LongIndexedAddress(0x97ca64u, cpu->x)) >> 5));
    StoreADirect8(memory, cpu, 0xcau);
    LoadA8(cpu, (uint8_t)((Read8(memory, LongIndexedAddress(0x97ca65u, cpu->x)) & 0xe0u) >> 2));
    Or8(cpu, DirectByte(memory, cpu, 0xcau));
    cpu->x = PullIndexValue(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x81fbc5u);
}

/* $81:FBDB: $09FC-$09FF = sprite box of character $09FA. */
Lufia2ExecutionResult Lufia2CharacterSpriteBox(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    unsigned i;

    PushIndex(memory, cpu);
    LoadAAbsolute8(memory, cpu, 0x09fau, 0);
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x9685c0u, cpu->x)));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    for (i = 0; i < 4u; ++i) {
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x9685ddu + i, cpu->x)));
        StoreAAbsolute8(memory, cpu, (uint16_t)(0x09fcu + i), 0);
    }
    cpu->x = PullIndexValue(memory, cpu);
    return ExecutionReturned(0x81fc0au);
}

/* $81:FCE2: X = $96 record of character A; Y kept. */
Lufia2ExecutionResult Lufia2CharacterSpritePointer(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    PushY(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToY(cpu);
    SetAccumulatorWidth(cpu, 1);
    SetBank(memory, cpu, 0x96u);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x85c0u, cpu->y));
    cpu->carry = 0;
    Add16Value(cpu, 0x85c0u);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    cpu->y = PullIndexValue(memory, cpu);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81fcffu);
}
