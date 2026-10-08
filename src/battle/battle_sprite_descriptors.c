#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum {
    PARTY_DESCRIPTOR_TEMPLATE = 0x97ca1du,
    PARTY_DESCRIPTORS = WRAM_BATTLE_EFFECT_ENEMY_POSITIONS & 0xffffu,
    PARTY_DESCRIPTOR_SIZE = 13u,
    PARTY_DISPLAY_IDS = WRAM_BATTLE_PARTY_IDS & 0xffffu,
    ENEMY_RECORDS = WRAM_BATTLE_ENEMY_RECORDS & 0xffffu,
    ENEMY_DESCRIPTORS = WRAM_BATTLE_EFFECT_PARTY_POSITIONS & 0xffffu,
    RECORD_STATUS = 0x0fu,
    RECORD_GRAPHICS_ID = 0x50u
};

Lufia2ExecutionResult Lufia2BattleInitializePartySpriteDescriptors(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->stack < 0x1f00u ||
        cpu->stack > 0x1ffbu)
        return ExecutionHandoff(cpu, 0x858a03u);
    OpLdx(cpu, 64u);
    do {
        OpLda(memory, cpu, OpLongX(cpu, PARTY_DESCRIPTOR_TEMPLATE));
        OpSta(memory, cpu, OpAbsX(cpu, PARTY_DESCRIPTORS));
        OpDex(cpu);
    } while (!cpu->negative);
    OpLdx(cpu, 0u);
    OpTxy(cpu);
    do {
        LoadA16(cpu, cpu->direct_page);
        OpLda(memory, cpu, OpAbsX(cpu, PARTY_DISPLAY_IDS));
        if (cpu->negative) {
            OpIncA(cpu);
            OpSta(memory, cpu, OpAbsY(cpu, PARTY_DESCRIPTORS + 1u));
        }
        OpRepWidths(cpu, 0x20u);
        OpTya(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, PARTY_DESCRIPTOR_SIZE);
        OpTay(cpu);
        OpSepWidths(cpu, 0x20u);
        OpInx(cpu);
        OpCpx(cpu, 5u);
    } while (!cpu->zero);
    return ExecutionReturned(0x858a2eu);
}

static Lufia2ExecutionResult DescriptorChildUnwound(uint32_t site) {
    const Lufia2ExecutionResult result =
        {LUFIA2_EXECUTION_CHILD_UNWOUND, site};
    return result;
}

Lufia2ExecutionResult Lufia2BattleInitializeEnemySpriteSizes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->stack < 0x1f08u ||
        cpu->stack > 0x1ffbu || !child)
        return ExecutionHandoff(cpu, 0x858905u);
    Push8(memory, cpu, cpu->data_bank);
    Push8(memory, cpu, cpu->program_bank);
    cpu->data_bank = Pull8(memory, cpu);
    SetNz8(cpu, cpu->data_bank);
    OpLdy(cpu, 0u);
    do {
        OpLdx(cpu, OpReadX(memory, cpu, OpAbsY(cpu, ENEMY_RECORDS)));
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbsX(cpu, RECORD_STATUS));
            OpBitValue(cpu, 8u);
            if (!cpu->zero) {
                OpLoadA(cpu, 2u);
            } else {
                OpBitValue(cpu, 0x20u);
                if (!cpu->zero) {
                    OpLoadA(cpu, 1u);
                } else {
                    OpLda(memory, cpu, OpAbsX(cpu, RECORD_GRAPHICS_ID));
                    if (!CallChildWithFrame(memory, cpu, child, context,
                            0x85891eu, 0x81fbc6u, 3u, 0x85u))
                        return DescriptorChildUnwound(0x85891eu);
                    if (!CallChildWithFrame(memory, cpu, child, context,
                            0x858922u, 0x81fba2u, 3u, 0x85u))
                        return DescriptorChildUnwound(0x858922u);
                }
            }
            if (cpu->accumulator_is_8_bit)
                PushAccumulator8(memory, cpu);
            else
                PushAccumulator16(memory, cpu);
            OpTya(cpu);
            OpLsrA(cpu);
            OpSta(memory, cpu, OpAbs(cpu, 0x4202u));
            OpLoadA(cpu, 15u);
            OpSta(memory, cpu, OpAbs(cpu, 0x4203u));
            if (cpu->accumulator_is_8_bit)
                LoadA8(cpu, Pull8(memory, cpu));
            else
                PullAccumulator16(memory, cpu);
            OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, 0x4216u)));
            OpSta(memory, cpu, OpAbsX(cpu, ENEMY_DESCRIPTORS));
        }
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, 12u);
    } while (!cpu->zero);
    cpu->data_bank = Pull8(memory, cpu);
    SetNz8(cpu, cpu->data_bank);
    return ExecutionReturned(0x858949u);
}
