#include "core/cpu_ops.h"
#include "lufia2/battle.h"
#include "system/wram.h"

enum {
    DP_EFFECT_SCROLL_SCRIPT = 0xc3u,
    EFFECT_SCROLL_SIGN = 0x80u,
    EFFECT_BG1_SCROLL = WRAM_NMI_SCROLL_REGISTERS,
    EFFECT_BG3_SCROLL = WRAM_NMI_SCROLL_REGISTERS + 8u
};

static void ReadSignedScrollOperand(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, unsigned axis) {
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_EFFECT_SCROLL_SCRIPT));
    if (axis == 0u)
        OpRepWidths(cpu, 0x20u);
    OpStepMem(memory, cpu, OpDp(cpu, DP_EFFECT_SCROLL_SCRIPT), 1);
    OpBitValue(cpu, EFFECT_SCROLL_SIGN);
    if (cpu->zero)
        OpAndValue(cpu, 0xffu);
    else
        OpOraValue(cpu, 0xff00u);
}

static Lufia2ExecutionResult UpdateEffectScroll(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint32_t entry, uint32_t end,
    uint16_t destination, uint8_t relative) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, entry);
    for (unsigned axis = 0; axis < 2u; ++axis) {
        ReadSignedScrollOperand(memory, cpu, axis);
        const uint32_t address = OpAbs(cpu, (uint16_t)(destination + axis * 2u));
        if (relative) {
            cpu->carry = 0u;
            OpAdc(memory, cpu, address);
        }
        OpSta(memory, cpu, address);
    }
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E15B3));
    return ExecutionReturned(end);
}

Lufia2ExecutionResult Lufia2BattleEffectAddBg3Scroll(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    return UpdateEffectScroll(memory, cpu, 0x8196feu, 0x819737u,
        EFFECT_BG3_SCROLL, 1u);
}

Lufia2ExecutionResult Lufia2BattleEffectSetBg3Scroll(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    return UpdateEffectScroll(memory, cpu, 0x819738u, 0x819769u,
        EFFECT_BG3_SCROLL, 0u);
}

Lufia2ExecutionResult Lufia2BattleEffectAddBg1Scroll(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    return UpdateEffectScroll(memory, cpu, 0x81976bu, 0x8197a4u,
        EFFECT_BG1_SCROLL, 1u);
}

Lufia2ExecutionResult Lufia2BattleEffectSetBg1Scroll(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    return UpdateEffectScroll(memory, cpu, 0x8197a5u, 0x8197d6u,
        EFFECT_BG1_SCROLL, 0u);
}
