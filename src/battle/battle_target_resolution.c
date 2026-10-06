#include "core/cpu_ops.h"
#include "lufia2/battle.h"
#include "lufia2/system.h"
#include "system/wram.h"

enum {
    TARGET_REQUEST = 0x24u,
    TARGET_AVAILABLE = 0x25u,
    TARGET_SELECTION_LIMIT = 65536u,
    ROM_TARGET_BITS = 0x96ffecu,
    EFFECT_HORIZONTAL = 0u,
    EFFECT_VERTICAL = 1u,
    EFFECT_ENEMY_POSITIONS = WRAM_BATTLE_EFFECT_ENEMY_POSITIONS & 0xffffu,
    EFFECT_PARTY_POSITIONS = WRAM_BATTLE_EFFECT_PARTY_POSITIONS & 0xffffu,
    EFFECT_SPECIAL_HORIZONTAL = WRAM_BATTLE_EFFECT_SPECIAL_HORIZONTAL & 0xffffu,
    EFFECT_SPECIAL_VERTICAL = WRAM_BATTLE_EFFECT_SPECIAL_VERTICAL & 0xffffu,
    EFFECT_SPECIAL_PARTY = WRAM_BATTLE_EFFECT_SPECIAL_PARTY & 0xffffu,
    TARGET_SPRITE_FIELD = 0x50u,
    POSITION_HORIZONTAL_FIELD = 5u,
    POSITION_VERTICAL_FIELD = 7u
};

static bool TargetContext(const Lufia2CpuState *cpu, uint16_t minimum) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit &&
        !cpu->decimal && cpu->direct_page == 0u &&
        cpu->program_bank == 0x81u && cpu->data_bank == 0x97u &&
        cpu->stack >= minimum && cpu->stack <= 0x1ffcu;
}

static void ReadActiveTargets(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                              uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    (void)Lufia2BattleActiveMask(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

Lufia2ExecutionResult Lufia2BattleResolveTargetMask(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!TargetContext(cpu, 0x1f00u))
        return ExecutionHandoff(cpu, 0x81b228u);
    StoreAAbsolute8(memory, cpu, TARGET_REQUEST, 0u);
    ReadActiveTargets(memory, cpu, 0xb22bu);
    StoreAAbsolute8(memory, cpu, TARGET_AVAILABLE, 0u);
    And8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, TARGET_REQUEST, 0u)));
    And8(cpu, 0x7fu);
    if (!cpu->zero) {
        LoadAAbsolute8(memory, cpu, TARGET_REQUEST, 0u);
        And8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, TARGET_AVAILABLE, 0u)));
        return ExecutionReturned(0x81b23eu);
    }
    LoadAAbsolute8(memory, cpu, TARGET_REQUEST, 0u);
    And8(cpu, 0x80u);
    ReadActiveTargets(memory, cpu, 0xb244u);
    StoreAAbsolute8(memory, cpu, TARGET_REQUEST, 0u);
    SetIndexWidth(cpu, 1);
    if (!(A8(cpu) & 0x7fu))
        return ExecutionHandoff(cpu, 0x81b24cu);
    for (unsigned attempt = 0u; attempt < TARGET_SELECTION_LIMIT; ++attempt) {
        LoadA8(cpu, 6u);
        SimulateJslFrame(memory, cpu, 0x81u, 0xb251u);
        Lufia2RandomScale(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        LoadX8(cpu, A8(cpu));
        LoadA8(cpu, Read8(memory, LongIndexedAddress(ROM_TARGET_BITS, cpu->x)));
        OpBit(memory, cpu, OpAbs(cpu, TARGET_REQUEST));
        if (!cpu->zero) {
            SetIndexWidth(cpu, 0);
            Or8(cpu, 0x80u);
            And8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, TARGET_REQUEST, 0u)));
            return ExecutionReturned(0x81b263u);
        }
    }
    return ExecutionHandoff(cpu, 0x81b24cu);
}

static void ReadSpriteCoordinates(const Lufia2Memory *memory,
                                  Lufia2CpuState *cpu, uint16_t site) {
    SimulateJslFrame(memory, cpu, 0x81u, (uint16_t)(site + 3u));
    (void)Lufia2CharacterSpriteWord(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x81u, (uint16_t)(site + 7u));
    (void)Lufia2SpriteCoordinatesPacked(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

static void SelectTargetSprite(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                               uint16_t mask) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, cpu->accumulator & mask);
    AslA16(cpu);
    LoadX16(cpu, cpu->accumulator);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_BATTLE_ENEMY_RECORDS, cpu->x));
    LoadX16(cpu, cpu->accumulator);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, TARGET_SPRITE_FIELD, cpu->x));
}

Lufia2ExecutionResult Lufia2BattleTargetSpriteCoordinates(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!TargetContext(cpu, 0x1f03u))
        return ExecutionHandoff(cpu, 0x81b7efu);
    SelectTargetSprite(memory, cpu, 0xffu);
    ReadSpriteCoordinates(memory, cpu, 0xb7fdu);
    Write16Direct(memory, cpu, EFFECT_HORIZONTAL, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x81b809u);
}

static void SelectEffectPosition(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                 bool party) {
    Write8(memory, SNES_WRMPYA, A8(cpu));
    LoadA8(cpu, party ? 15u : 13u);
    Write8(memory, SNES_WRMPYB, A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, party ? EFFECT_PARTY_POSITIONS : EFFECT_ENEMY_POSITIONS);
    cpu->carry = false;
    Add16Value(cpu, Read16Long(memory, SNES_RDMPYL));
    LoadX16(cpu, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0u);
}

static void StoreEffectAxis(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                            uint16_t field, uint8_t offset) {
    LoadA8(cpu, offset);
    cpu->carry = false;
    Adc8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, field, cpu->x)));
    StoreADirect8(memory, cpu,
        field == POSITION_HORIZONTAL_FIELD ? EFFECT_HORIZONTAL : EFFECT_VERTICAL);
}

Lufia2ExecutionResult Lufia2BattleEffectTargetCoordinates(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!TargetContext(cpu, 0x1f05u))
        return ExecutionHandoff(cpu, 0x81b80au);
    BitImmediate8(cpu, 0x80u);
    if (!cpu->zero) {
        And8(cpu, 0x7fu);
        Compare8(cpu, A8(cpu), 4u);
        if (cpu->zero) {
            LoadA8(cpu, 24u);
            cpu->carry = false;
            Adc8(cpu, Read8(memory, EFFECT_SPECIAL_HORIZONTAL));
            StoreADirect8(memory, cpu, EFFECT_HORIZONTAL);
            LoadA8(cpu, 24u);
            cpu->carry = false;
            Adc8(cpu, Read8(memory, EFFECT_SPECIAL_VERTICAL));
            StoreADirect8(memory, cpu, EFFECT_VERTICAL);
            return ExecutionReturned(0x81b854u);
        }
        SelectEffectPosition(memory, cpu, false);
        StoreEffectAxis(memory, cpu, POSITION_HORIZONTAL_FIELD, 16u);
        StoreEffectAxis(memory, cpu, POSITION_VERTICAL_FIELD, 16u);
        PullDataBank(memory, cpu);
        return ExecutionReturned(0x81b841u);
    }
    LoadX16(cpu, cpu->accumulator);
    LoadAAbsolute8(memory, cpu, EFFECT_SPECIAL_PARTY, 0u);
    if (!cpu->zero) {
        LoadX16(cpu, 0x3c98u);
        Write16Direct(memory, cpu, EFFECT_HORIZONTAL, cpu->x);
        return ExecutionReturned(0x81b860u);
    }
    LoadA8(cpu, (uint8_t)cpu->x);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, cpu->accumulator & 0x7fu);
    PushAccumulator16(memory, cpu);
    SelectTargetSprite(memory, cpu, 0x7fu);
    ReadSpriteCoordinates(memory, cpu, 0xb871u);
    AslA16(cpu);
    AslA16(cpu);
    LoadA16(cpu, cpu->accumulator & 0xfcfcu);
    Write16Direct(memory, cpu, EFFECT_HORIZONTAL, cpu->accumulator);
    PullAccumulator16(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    SelectEffectPosition(memory, cpu, true);
    cpu->carry = false;
    LoadAAbsolute8(memory, cpu, POSITION_HORIZONTAL_FIELD, cpu->x);
    Adc8(cpu, DirectByte(memory, cpu, EFFECT_HORIZONTAL));
    StoreADirect8(memory, cpu, EFFECT_HORIZONTAL);
    cpu->carry = false;
    LoadAAbsolute8(memory, cpu, POSITION_VERTICAL_FIELD, cpu->x);
    Adc8(cpu, DirectByte(memory, cpu, EFFECT_VERTICAL));
    StoreADirect8(memory, cpu, EFFECT_VERTICAL);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81b8b0u);
}
