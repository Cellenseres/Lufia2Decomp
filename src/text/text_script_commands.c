#include "core/cpu_ops.h"
#include "system/wram.h"
#include "text/text_internal.h"

enum {
    TYPING_SOUND_OR_PAUSE = 0x9dfau,
    REPEAT_SCRIPT = 0x9e83u,
    PREPARE_CHOICES = 0x9f10u,
    NEXT_VARIABLE_BRANCH = 0xa041u,
    DP_SCRIPT_OFFSET = 0x54u,
    DP_VARIABLE_INDEX = 0x56u,
    DP_BRANCH_OFFSET = 0x58u,
    TEXT_PAUSE_ELAPSED = WRAM_TEXT_PAUSE_ELAPSED,
    SCRIPT_VARIABLES = WRAM_UNK_7E079E
};

static uint32_t SetTypingSoundOrPause(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbsY(cpu, 0u));
    if (!cpu->negative) {
        Lufia2TextNextByte(memory, cpu, 0x9e01u);
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x45u);
        OpSta(memory, cpu, OpAbs(cpu, TEXT_TYPING_SOUND));
        return LUFIA2_TEXT_COMMAND_NEXT;
    }
    OpLoadA(cpu, 8u);
    OpTestBits(memory, cpu, OpAbs(cpu, TEXT_WINDOW_STATE), true);
    if (cpu->zero)
        OpStz(memory, cpu, OpAbs(cpu, TEXT_PAUSE_ELAPSED));
    OpLda(memory, cpu, OpAbsY(cpu, 0u));
    OpAndValue(cpu, 0x7fu);
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_SCRIPT_OFFSET));
    OpAslA(cpu);
    OpAslA(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, DP_SCRIPT_OFFSET));
    OpCmp(memory, cpu, OpAbs(cpu, TEXT_PAUSE_ELAPSED));
    if (!cpu->carry) {
        OpLoadA(cpu, 8u);
        OpTestBits(memory, cpu, OpAbs(cpu, TEXT_WINDOW_STATE), false);
        Lufia2TextNextByte(memory, cpu, 0x9e2eu);
        OpLda(memory, cpu, TEXT_TYPING_SOUND_PHASE);
        OpAndValue(cpu, 0xfeu);
        OpSta(memory, cpu, TEXT_TYPING_SOUND_PHASE);
        return LUFIA2_TEXT_COMMAND_NEXT;
    }
    OpStepMem(memory, cpu, OpAbs(cpu, TEXT_PAUSE_ELAPSED), 1u);
    Lufia2TextPrevByte(memory, cpu, 0x9e41u);
    return LUFIA2_TEXT_COMMAND_YIELD;
}

static uint32_t RepeatScript(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2TextNextByte(memory, cpu, 0x9e85u);
    OpSta(memory, cpu, OpDp(cpu, DP_SCRIPT_OFFSET));
    Lufia2TextNextByte(memory, cpu, 0x9e8au);
    OpSta(memory, cpu, OpDp(cpu, DP_SCRIPT_OFFSET + 1u));
    for (unsigned bit = 0u; bit < 4u; ++bit)
        OpLsrA(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, 2u);
    OpSta(memory, cpu, OpAbs(cpu, TEXT_RETURN_COUNT));
    OpWriteX(memory, cpu, OpAbs(cpu, TEXT_RETURN_POINTER), cpu->y);
    OpLoadA(cpu, 0xf0u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_SCRIPT_OFFSET + 1u), false);
    PushDataBank(memory, cpu);
    LoadA8(cpu, Pull8(memory, cpu));
    OpSta(memory, cpu, OpAbs(cpu, TEXT_RETURN_BANK));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, TEXT_RETURN_POINTER));
    cpu->carry = 1u;
    OpSbcValue(cpu, 2u);
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_SCRIPT_OFFSET)));
    OpTay(cpu);
    if (!cpu->negative) {
        OpOraValue(cpu, 0x8000u);
        OpTay(cpu);
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbs(cpu, TEXT_SCRIPT_BANK));
        OpDecA(cpu);
        OpSta(memory, cpu, OpAbs(cpu, TEXT_SCRIPT_BANK));
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
    }
    OpWriteX(memory, cpu, OpAbs(cpu, TEXT_SCRIPT_POINTER), cpu->y);
    OpSepWidths(cpu, 0x20u);
    return LUFIA2_TEXT_COMMAND_NEXT;
}

static uint32_t PrepareChoices(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2TextNextByte(memory, cpu, 0x9f12u);
    OpSta(memory, cpu, OpAbs(cpu, TEXT_CHOICE_COUNT));
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_SCRIPT_OFFSET));
    OpStz(memory, cpu, OpDp(cpu, DP_SCRIPT_OFFSET + 1u));
    OpLda(memory, cpu, OpAbs(cpu, TEXT_SCRIPT_BANK));
    OpSta(memory, cpu, OpAbs(cpu, TEXT_CHOICE_TABLE_BANK));
    OpWriteX(memory, cpu, OpAbs(cpu, TEXT_CHOICE_TABLE), cpu->y);
    OpRepWidths(cpu, 0x20u);
    OpTya(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, DP_SCRIPT_OFFSET));
    Lufia2TextSetScriptPointer(memory, cpu, 0x9f2cu);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 4u);
    OpTestBits(memory, cpu, OpAbs(cpu, TEXT_WINDOW_STATE), true);
    return 0x809d3eu;
}

static uint32_t NextVariableBranch(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2TextNextByte(memory, cpu, 0xa043u);
    OpSta(memory, cpu, OpDp(cpu, DP_VARIABLE_INDEX));
    OpStz(memory, cpu, OpDp(cpu, DP_VARIABLE_INDEX + 1u));
    Lufia2TextNextByte(memory, cpu, 0xa04au);
    OpSta(memory, cpu, OpDp(cpu, DP_SCRIPT_OFFSET));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_VARIABLE_INDEX)));
    OpLda(memory, cpu, OpAbsX(cpu, SCRIPT_VARIABLES));
    OpStepMem(memory, cpu, OpAbsX(cpu, SCRIPT_VARIABLES), 1u);
    OpCmp(memory, cpu, OpDp(cpu, DP_SCRIPT_OFFSET));
    if (cpu->carry) {
        OpLda(memory, cpu, OpDp(cpu, DP_SCRIPT_OFFSET));
        OpDecA(cpu);
        OpSta(memory, cpu, OpAbsX(cpu, SCRIPT_VARIABLES));
    }
    OpRepWidths(cpu, 0x20u);
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_BRANCH_OFFSET));
    OpTya(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, DP_BRANCH_OFFSET));
    Lufia2TextSetScriptPointer(memory, cpu, 0xa06au);
    return 0x80a3c6u;
}

uint32_t Lufia2TextExecuteScriptCommand(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                        uint16_t handler) {
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal ||
        cpu->stack < 0x1f10u || cpu->stack > 0x1ffcu)
        return 0u;
    switch (handler) {
    case TYPING_SOUND_OR_PAUSE:
        return SetTypingSoundOrPause(memory, cpu);
    case REPEAT_SCRIPT:
        return RepeatScript(memory, cpu);
    case PREPARE_CHOICES:
        return PrepareChoices(memory, cpu);
    case NEXT_VARIABLE_BRANCH:
        return NextVariableBranch(memory, cpu);
    default:
        return 0u;
    }
}
