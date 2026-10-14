#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    SELECTED_RECORD = 0x8b,
    RECORD_X = 0x8f,
    RECORD_Y = 0x91,
    RECORD_ID = 0x58,
    CHOICE_ORDINAL = 0x02,
    CHOICE_COUNT = 0x04,
    CHOICE_BYTES = 0x06,
    OBJECT_RECORDS = 0x7ef000,
    CONDITION_RECORDS = 0x91b8b5,
    CONDITION_LIST = 0x7fd1a0,
    REWARD_OBJECT = 0x7fd4ee,
    REWARD_VALUE = 0x7fd4ef,
    COLLECTED_FLAGS = 0x7fe733,
    REWARD_VALUES = 0x7fe746,
    SPECIAL_REWARD = 0x7fe75b,
    CHOICE_VALUES = 0x7ed000,
    CHOICE_FLAGS = 0x093b,
    CHOSEN_VALUES = 0x092b,
    FIELD_INPUT_LOCK = 0x17aa,
    REWARD_CONTEXT = 0x05b6,
    CLAIMED_COUNT = 0x0b5f,
    REWARD_COUNT_A = 0x0b6e,
    REWARD_COUNT_B = 0x0b70,
    REWARD_REMAINING = 0x09cf,
    SPELL_RECORD_ID = 0x0a0b
};

static uint8_t RewardWidths(const Lufia2CpuState *cpu, uint8_t wide) {
    return !cpu->index_is_8_bit && cpu->accumulator_is_8_bit != wide;
}

static uint8_t RewardChild(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                           Lufia2PushedChildCall child, void *context, uint32_t site,
                           uint32_t target, uint8_t frame, uint8_t wide,
                           Lufia2ExecutionResult *result) {
    if (!CallChildWithFrame(memory, cpu, child, context, site, target, frame, 0x8eu)) {
        *result = ExecutionReturned(site);
        result->flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return 0u;
    }
    if (!RewardWidths(cpu, wide)) {
        *result = ExecutionHandoff(cpu, site + frame + 1u);
        return 0u;
    }
    return 1u;
}

static uint8_t RewardConditionFound(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpLongX(cpu, OBJECT_RECORDS + 1u));
    OpSta(memory, cpu, OpDp(cpu, RECORD_X));
    OpLda(memory, cpu, OpLongX(cpu, OBJECT_RECORDS + 2u));
    OpSta(memory, cpu, OpDp(cpu, RECORD_Y));
    OpLda(memory, cpu, OpLongX(cpu, OBJECT_RECORDS));
    OpSta(memory, cpu, OpDp(cpu, RECORD_ID));
    OpLda(memory, cpu, CONDITION_LIST + 1u);
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, CONDITION_LIST);
    OpTax(cpu);
    for (;;) {
        OpLda(memory, cpu, OpLongX(cpu, CONDITION_RECORDS));
        OpCmpValue(cpu, 0xffu);
        if (cpu->zero) {
            cpu->carry = 1u;
            return 0u;
        }
        OpAndValue(cpu, 0x1fu);
        OpCmpValue(cpu, OpReadM(memory, cpu, OpDp(cpu, RECORD_ID)));
        if (cpu->zero)
            return 1u;
        OpInx(cpu);
        OpInx(cpu);
        OpInx(cpu);
    }
}

static void IncrementRewardCount(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                 uint16_t count) {
    OpLda(memory, cpu, OpAbs(cpu, count));
    OpIncA(cpu);
    if (cpu->zero)
        OpDecA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, count));
}

static Lufia2ExecutionResult ApplyContextReward(const Lufia2Memory *memory,
                                                Lufia2CpuState *cpu,
                                                Lufia2PushedChildCall child,
                                                void *context) {
    Lufia2ExecutionResult result;
    OpWriteX(memory, cpu, OpDp(cpu, SELECTED_RECORD), cpu->x);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpLongX(cpu, OBJECT_RECORDS));
    OpSta(memory, cpu, REWARD_OBJECT);
    if (!RewardChild(memory, cpu, child, context, 0x8ec0b8u, 0x80e898u, 3u, 0u,
                     &result))
        return result;
    OpAndValue(cpu, OpReadM(memory, cpu, COLLECTED_FLAGS));
    if (!cpu->zero) {
        result = ExecutionReturned(0x8ec152u);
        return result;
    }
    TransferDirectToA(cpu);
    OpLda(memory, cpu, REWARD_OBJECT);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, REWARD_VALUES));
    OpSta(memory, cpu, REWARD_VALUE);
    OpLda(memory, cpu, OpLongX(cpu, REWARD_VALUES + 1u));
    OpSta(memory, cpu, REWARD_VALUE + 1u);
    if (cpu->negative) {
        OpLdx(cpu, 0u);
        do {
            OpLda(memory, cpu, REWARD_VALUE);
            OpSta(memory, cpu, OpAbs(cpu, SPELL_RECORD_ID));
            OpTxa(cpu);
            PushIndex(memory, cpu);
            if (!RewardChild(memory, cpu, child, context, 0x8ec0eau, 0x82fd3du, 3u, 0u,
                             &result))
                return result;
            OpPullX(memory, cpu);
            OpInx(cpu);
            OpCpx(cpu, 7u);
        } while (!cpu->carry);
    }
    if (!RewardChild(memory, cpu, child, context, 0x8ec0f5u, 0x8ec1deu, 3u, 0u,
                     &result))
        return result;
    OpLda(memory, cpu, OpAbs(cpu, REWARD_REMAINING + 1u));
    OpAndValue(cpu, 0x7fu);
    OpOraValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, REWARD_REMAINING)));
    if (cpu->zero) {
        TransferDirectToA(cpu);
        OpLda(memory, cpu, REWARD_OBJECT);
        OpAslA(cpu);
        OpTax(cpu);
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpLongX(cpu, REWARD_VALUES));
        OpAndValue(cpu, 0x1ffu);
        OpCmpValue(cpu, 0x2du);
        if (cpu->zero) {
            OpLoadA(cpu, 0xffffu);
            OpSta(memory, cpu, SPECIAL_REWARD);
        }
        OpLda(memory, cpu, OpLongX(cpu, REWARD_VALUES));
        OpBitValue(cpu, 0x4000u);
        IncrementRewardCount(memory, cpu, cpu->zero ? REWARD_COUNT_B : REWARD_COUNT_A);
        OpSepWidths(cpu, 0x20u);
        TransferDirectToA(cpu);
        OpLda(memory, cpu, REWARD_OBJECT);
        if (!RewardChild(memory, cpu, child, context, 0x8ec145u, 0x80e898u, 3u, 0u,
                         &result))
            return result;
        OpOraValue(cpu, OpReadM(memory, cpu, COLLECTED_FLAGS));
        OpSta(memory, cpu, COLLECTED_FLAGS);
    }
    cpu->carry = 1u;
    result = ExecutionReturned(0x8ec152u);
    return result;
}

static Lufia2ExecutionResult ApplyConditionReward(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu,
                                                  Lufia2PushedChildCall child,
                                                  void *context) {
    Lufia2ExecutionResult result;
    OpWriteX(memory, cpu, OpDp(cpu, SELECTED_RECORD), cpu->x);
    if (!RewardChild(memory, cpu, child, context, 0x8ec155u, 0x8ec338u, 3u, 0u,
                     &result))
        return result;
    if (!RewardChild(memory, cpu, child, context, 0x8ec159u, 0x8ec34fu, 3u, 0u,
                     &result))
        return result;
    OpBit(memory, cpu, OpAbsX(cpu, CHOICE_FLAGS));
    if (!cpu->zero)
        return ExecutionReturned(0x8ec1c4u);
    OpLda(memory, cpu, OpDp(cpu, RECORD_ID));
    OpSta(memory, cpu, REWARD_OBJECT);
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 14u);
    OpLda(memory, cpu, REWARD_VALUE + 2u);
    uint8_t chosen = 0u;
    for (;;) {
        OpCmpValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, CHOSEN_VALUES)));
        if (cpu->zero) {
            OpLoadA(cpu, 0x2bu);
            OpSta(memory, cpu, REWARD_VALUE);
            OpSepWidths(cpu, 0x20u);
            chosen = 1u;
            break;
        }
        OpDex(cpu);
        OpDex(cpu);
        if (cpu->negative)
            break;
    }
    if (!chosen) {
        OpSepWidths(cpu, 0x20u);
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, SELECTED_RECORD)));
        if (!RewardChild(memory, cpu, child, context, 0x8ec189u, 0x8ec36du, 2u, 0u,
                         &result))
            return result;
        OpSta(memory, cpu, REWARD_VALUE);
        ExchangeAccumulatorBytes(cpu);
        OpSta(memory, cpu, REWARD_VALUE + 1u);
    }
    if (!RewardChild(memory, cpu, child, context, 0x8ec195u, 0x8ec1deu, 3u, 0u,
                     &result))
        return result;
    OpLda(memory, cpu, OpAbs(cpu, REWARD_REMAINING + 1u));
    OpAndValue(cpu, 0x7fu);
    OpOraValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, REWARD_REMAINING)));
    if (cpu->zero) {
        if (!RewardChild(memory, cpu, child, context, 0x8ec1a3u, 0x8ec34fu, 3u, 0u,
                         &result))
            return result;
        OpOraValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, CHOICE_FLAGS)));
        OpSta(memory, cpu, OpAbsX(cpu, CHOICE_FLAGS));
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, REWARD_VALUE);
        if (!RewardChild(memory, cpu, child, context, 0x8ec1b3u, 0x8ec1c5u, 3u, 1u,
                         &result))
            return result;
        if (!cpu->carry) {
            OpStepMem(memory, cpu, OpAbs(cpu, CLAIMED_COUNT), 1u);
            if (cpu->zero)
                OpStepMem(memory, cpu, OpAbs(cpu, CLAIMED_COUNT), -1);
        }
        OpSepWidths(cpu, 0x20u);
    }
    cpu->carry = 1u;
    return ExecutionReturned(0x8ec1c4u);
}

Lufia2ExecutionResult Lufia2FieldTryObjectReward(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu,
                                                 Lufia2PushedChildCall child,
                                                 void *context) {
    if (!RewardWidths(cpu, 0u) || !child || cpu->decimal || cpu->program_bank != 0x8eu)
        return ExecutionHandoff(cpu, 0x8ec05fu);
    OpLda(memory, cpu, OpAbs(cpu, FIELD_INPUT_LOCK));
    cpu->carry = 0u;
    if (!cpu->zero)
        return ExecutionReturned(0x8ec0acu);
    OpLdx(cpu, 0x26u);
    OpLdy(cpu, 4u);
    Lufia2ExecutionResult result;
    if (!RewardChild(memory, cpu, child, context, 0x8ec06bu, 0x83b851u, 3u, 0u,
                     &result))
        return result;
    if (!cpu->carry)
        return ExecutionReturned(0x8ec0acu);
    OpLda(memory, cpu, OpAbs(cpu, REWARD_CONTEXT));
    OpBitValue(cpu, 1u);
    if (!cpu->zero) {
        return ApplyContextReward(memory, cpu, child, context);
    }
    if (!RewardConditionFound(memory, cpu))
        return ExecutionReturned(0x8ec0acu);
    return ApplyConditionReward(memory, cpu, child, context);
}

Lufia2ExecutionResult Lufia2FieldFindAvailableChoice(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu) {
    if (!RewardWidths(cpu, 1u) || cpu->program_bank != 0x8eu)
        return ExecutionHandoff(cpu, 0x8ebd5eu);
    OpLdx(cpu, 0u);
    for (;;) {
        OpLda(memory, cpu, OpLongX(cpu, CHOICE_VALUES));
        if (!cpu->negative) {
            OpStepMem(memory, cpu, OpDp(cpu, CHOICE_ORDINAL), -1);
            cpu->carry = 1u;
            if (cpu->zero)
                return ExecutionReturned(0x8ebd76u);
        }
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, OpReadX(memory, cpu, OpDp(cpu, CHOICE_BYTES)));
        if (!cpu->carry)
            continue;
        OpLdx(cpu, 0u);
        cpu->carry = 0u;
        return ExecutionReturned(0x8ebd76u);
    }
}

static uint8_t GatherCollectedChoices(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                      Lufia2PushedChildCall child, void *context,
                                      Lufia2ExecutionResult *result) {
    OpLdy(cpu, 0u);
    OpLdx(cpu, 2u);
    do {
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpLongX(cpu, CONDITION_RECORDS));
        OpCmpValue(cpu, 0xffffu);
        OpSepWidths(cpu, 0x20u);
        if (cpu->zero)
            break;
        PushIndex(memory, cpu);
        OpTax(cpu);
        for (;;) {
            OpLda(memory, cpu, OpLongX(cpu, CONDITION_RECORDS));
            OpCmpValue(cpu, 0xffu);
            if (cpu->zero)
                break;
            PushIndex(memory, cpu);
            OpBitValue(cpu, 0x20u);
            if (cpu->zero) {
                OpAndValue(cpu, 0x80u);
                if (!cpu->zero)
                    OpLoadA(cpu, 1u);
                OpSta(memory, cpu, REWARD_VALUE + 3u);
                OpLda(memory, cpu, OpLongX(cpu, CONDITION_RECORDS + 1u));
                OpSta(memory, cpu, REWARD_VALUE + 2u);
                if (!RewardChild(memory, cpu, child, context, 0x8ebce1u, 0x8ec34fu, 3u,
                                 0u, result))
                    return 0u;
                OpBit(memory, cpu, OpAbsX(cpu, CHOICE_FLAGS));
                if (!cpu->zero) {
                    OpTyx(cpu);
                    OpRepWidths(cpu, 0x20u);
                    OpLda(memory, cpu, REWARD_VALUE + 2u);
                    OpSta(memory, cpu, OpLongX(cpu, CHOICE_VALUES));
                    OpSepWidths(cpu, 0x20u);
                    OpIny(cpu);
                    OpIny(cpu);
                }
            }
            OpPullX(memory, cpu);
            OpInx(cpu);
            OpInx(cpu);
            OpInx(cpu);
        }
        OpPullX(memory, cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, 0x1e2u);
    } while (!cpu->carry);
    OpWriteX(memory, cpu, OpDp(cpu, CHOICE_BYTES), cpu->y);
    return 1u;
}

Lufia2ExecutionResult Lufia2FieldChooseCollectedObjects(const Lufia2Memory *memory,
                                                        Lufia2CpuState *cpu,
                                                        Lufia2PushedChildCall child,
                                                        void *context) {
    if (!RewardWidths(cpu, 0u) || !child || cpu->decimal || cpu->program_bank != 0x8eu)
        return ExecutionHandoff(cpu, 0x8ebcadu);
    Lufia2ExecutionResult result;
    if (!GatherCollectedChoices(memory, cpu, child, context, &result))
        return result;
    OpRepWidths(cpu, 0x20u);
    OpLdy(cpu, 0u);
    do {
        OpLda(memory, cpu, OpDp(cpu, CHOICE_BYTES));
        OpLsrA(cpu);
        OpSta(memory, cpu, OpDp(cpu, CHOICE_COUNT));
        if (!RewardChild(memory, cpu, child, context, 0x8ebd13u, 0x869e3bu, 3u, 1u,
                         &result))
            return result;
        OpStepMem(memory, cpu, OpDp(cpu, CHOICE_ORDINAL), 1u);
        if (!RewardChild(memory, cpu, child, context, 0x8ebd19u, 0x8ebd5eu, 2u, 1u,
                         &result))
            return result;
        if (!cpu->carry && !RewardChild(memory, cpu, child, context, 0x8ebd1eu,
                                        0x8ebd5eu, 2u, 1u, &result))
            return result;
        OpLda(memory, cpu, OpLongX(cpu, CHOICE_VALUES));
        OpSta(memory, cpu, REWARD_VALUE + 2u);
        OpSta(memory, cpu, OpAbsY(cpu, CHOSEN_VALUES));
        OpOraValue(cpu, 0x8000u);
        OpSta(memory, cpu, OpLongX(cpu, CHOICE_VALUES));
        OpSepWidths(cpu, 0x20u);
        if (!RewardChild(memory, cpu, child, context, 0x8ebd35u, 0x8ec34fu, 3u, 0u,
                         &result))
            return result;
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
        OpAndValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, CHOICE_FLAGS)));
        OpSta(memory, cpu, OpAbsX(cpu, CHOICE_FLAGS));
        OpRepWidths(cpu, 0x20u);
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, 16u);
    } while (!cpu->carry);
    OpLoadA(cpu, 0x2bu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
    OpSepWidths(cpu, 0x20u);
    if (!RewardChild(memory, cpu, child, context, 0x8ebd52u, 0x81f057u, 3u, 0u,
                     &result))
        return result;
    TransferDirectToA(cpu);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS + 1u));
    return ExecutionReturned(0x8ebd5du);
}

enum {
    REWARD_ACTOR_REQUEST = WRAM_FIELD_SELECTED_OBJECT_RECORD_ID,
    REWARD_ANIMATION = WRAM_ACTOR_CLAIMED_OBJECT_RECORD,
    REWARD_ANIMATION_TIME = WRAM_ACTOR_CLAIMED_PENDING_OBJECT,
    REWARD_DESCRIPTION = WRAM_RECORD_BUFFER,
    REWARD_DESCRIPTION_KIND = 0x0b88u,
    REWARD_DESCRIPTION_FLAGS = 0x0b84u,
    REWARD_MENU_ORDINAL = WRAM_MENU_ITEM_QUANTITY,
    REWARD_MESSAGE_POINTER = WRAM_UNK_7E09B7,
    REWARD_MESSAGE_BANK = WRAM_SCENE_SCRIPT_BANK,
    REWARD_MESSAGE_POSITION = WRAM_SCENE_LABEL_ROW_POSITION,
    REWARD_MESSAGE_TIME = WRAM_FIELD_CONTROL_CHANGE_PENDING,
    REWARD_MESSAGE_FIRST = WRAM_UNK_7E0562,
    REWARD_MESSAGE_SECOND = WRAM_UNK_7E0563,
    REWARD_ACTOR_STATE = WRAM_ACTOR_STATE,
    DP_REWARD_SELECTED_RECORD = 0x8bu,
    DP_REWARD_DESCRIPTION_FLAGS = 0x60u,
    DP_REWARD_MENU_OPERATION = 0x30u,
    DP_REWARD_MENU_PARAMETER = 0x31u,
    ROM_REWARD_FACING_SCRIPT = 0x83c1a5u,
    REWARD_FRAME_LIMIT = 4096u,
    REWARD_POLL_LIMIT = 65536u
};

static uint8_t ApplyRewardChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t site,
    uint32_t target, uint8_t saved_bank, Lufia2ExecutionResult *result) {
    if (!CallChildWithFrame(memory, cpu, child, context, site, target, 3u, 0x8eu)) {
        *result = ExecutionReturned(site);
        result->flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return 0u;
    }
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit ||
        cpu->direct_page || cpu->decimal || (!saved_bank && cpu->data_bank!=0x83u)) {
        *result = ExecutionHandoff(cpu, site+4u);
        return 0u;
    }
    return 1u;
}

static uint8_t DescribeObjectReward(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, Lufia2ExecutionResult *result) {
    OpLda(memory, cpu, REWARD_OBJECT);
    OpSta(memory, cpu, REWARD_ACTOR_REQUEST);
    OpOraValue(cpu, 0x80u);
    ExchangeAccumulatorBytes(cpu);
    OpLoadA(cpu, 0u);
    if (!ApplyRewardChild(memory, cpu, child, context,
            0x8ec1ebu, 0x83f559u, 0u, result))
        return 0u;
    OpLdx(cpu, 0x8000u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_REWARD_DESCRIPTION_FLAGS), cpu->x);
    OpLda(memory, cpu, REWARD_VALUE+1u);
    if (cpu->negative) {
        OpLda(memory, cpu, REWARD_VALUE);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_RECORD_ID));
        if (!ApplyRewardChild(memory, cpu, child, context,
                0x8ec201u, 0x81f414u, 0u, result))
            return 0u;
    } else {
        OpLda(memory, cpu, REWARD_VALUE);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
        OpLda(memory, cpu, REWARD_VALUE+1u);
        OpAndValue(cpu, 1u);
        OpSta(memory, cpu, REWARD_VALUE+1u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID + 1u));
        if (!ApplyRewardChild(memory, cpu, child, context,
                0x8ec21eu, 0x81f1c5u, 0u, result))
            return 0u;
    }
    OpLdx(cpu, REWARD_DESCRIPTION);
    return ApplyRewardChild(memory, cpu, child, context,
        0x8ec225u, 0x8eb5fbu, 0u, result);
}

static void ChooseRewardAnnouncement(const Lufia2Memory *memory,
                                    Lufia2CpuState *cpu) {
    OpLda(memory, cpu, REWARD_VALUE+1u);
    if (cpu->negative) {
        OpLoadA(cpu, 0x1cu);
        OpSta(memory, cpu, REWARD_ANIMATION);
        OpLoadA(cpu, 0x0au);
        OpSta(memory, cpu, REWARD_ANIMATION_TIME);
        OpLoadA(cpu, 0x35u);
        return;
    }
    OpLda(memory, cpu, OpAbs(cpu, REWARD_DESCRIPTION_KIND));
    OpCmpValue(cpu, 0x40u);
    if (cpu->carry) {
        cpu->carry=1u;
        OpSbcValue(cpu, 0x40u);
        OpSta(memory, cpu, REWARD_ANIMATION);
        OpLoadA(cpu, 0x18u);
        OpSta(memory, cpu, REWARD_ANIMATION_TIME);
        OpLoadA(cpu, 0x35u);
        return;
    }
    OpSta(memory, cpu, REWARD_ANIMATION);
    OpLoadA(cpu, 0x0au);
    OpSta(memory, cpu, REWARD_ANIMATION_TIME);
    OpLda(memory, cpu, OpAbs(cpu, REWARD_DESCRIPTION_FLAGS));
    OpBitValue(cpu, 0x20u);
    OpLoadA(cpu, cpu->zero ? 0x36u : 0x33u);
}

static uint8_t WaitForRewardAnnouncement(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, Lufia2ExecutionResult *result) {
    for (unsigned frame=0u; frame<REWARD_FRAME_LIMIT; ++frame) {
        if (!ApplyRewardChild(memory, cpu, child, context,
                0x8ec2aau, 0x838192u, 0u, result))
            return 0u;
        OpLda(memory, cpu, OpDp(cpu, DP_FRAME_COUNTER));
        OpCmpValue(cpu, 0xc8u);
        if (cpu->carry)
            return 1u;
    }
    *result=ExecutionHandoff(cpu, 0x8ec2aau);
    return 0u;
}

static uint8_t WaitForRewardNextFrame(const Lufia2Memory *memory,
                                    Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, DP_NMI_FRAME_COUNTER));
    for (unsigned poll=0u; poll<REWARD_POLL_LIMIT; ++poll) {
        OpCmp(memory, cpu, OpDp(cpu, DP_NMI_FRAME_COUNTER));
        if (!cpu->zero)
            return 1u;
    }
    return 0u;
}

Lufia2ExecutionResult Lufia2FieldApplyObjectReward(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank!=0x8eu || cpu->data_bank!=0x83u ||
        !cpu->accumulator_is_8_bit || cpu->index_is_8_bit ||
        cpu->direct_page || cpu->decimal || !child)
        return ExecutionHandoff(cpu, 0x8ec1deu);
    Lufia2ExecutionResult result;
    if (!DescribeObjectReward(memory, cpu, child, context, &result))
        return result;
    ChooseRewardAnnouncement(memory, cpu);
    if (!ApplyRewardChild(memory, cpu, child, context,
            0x8ec26eu, 0x8093feu, 0u, &result) ||
        !ApplyRewardChild(memory, cpu, child, context,
            0x8ec272u, 0x838e66u, 0u, &result))
        return result;
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_FACING));
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, ROM_REWARD_FACING_SCRIPT));
    cpu->carry=0u;
    OpAdcValue(cpu, 0x40u);
    if (!ApplyRewardChild(memory, cpu, child, context,
            0x8ec282u, 0x83d350u, 0u, &result))
        return result;
    OpLdx(cpu, 0xe000u);
    OpWriteX(memory, cpu, OpAbs(cpu, REWARD_MESSAGE_POINTER), cpu->x);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpAbs(cpu, REWARD_MESSAGE_BANK));
    OpLdx(cpu, 0x0440u);
    OpWriteX(memory, cpu, OpAbs(cpu, REWARD_MESSAGE_POSITION), cpu->x);
    PushDataBank(memory, cpu);
    if (!ApplyRewardChild(memory, cpu, child, context,
            0x8ec298u, 0x80c8d5u, 1u, &result))
        return result;
    OpLoadA(cpu, 0xc4u);
    OpSta(memory, cpu, REWARD_MESSAGE_TIME);
    PullDataBank(memory, cpu);
    OpLoadA(cpu, 1u);
    OpTestBits(memory, cpu, OpAbs(cpu, REWARD_ACTOR_STATE), 1u);
    OpStz(memory, cpu, OpDp(cpu, DP_FRAME_COUNTER));
    if (!WaitForRewardAnnouncement(memory, cpu, child, context, &result))
        return result;
    OpLoadA(cpu, 1u);
    OpTestBits(memory, cpu, OpAbs(cpu, REWARD_ACTOR_STATE), 0u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E099D));
    if (!ApplyRewardChild(memory, cpu, child, context,
            0x8ec2bcu, 0x80969fu, 0u, &result) ||
        !ApplyRewardChild(memory, cpu, child, context,
            0x8ec2c0u, 0x838e66u, 0u, &result))
        return result;
    if (!WaitForRewardNextFrame(memory, cpu))
        return ExecutionHandoff(cpu, 0x8ec2c6u);
    OpRepWidths(cpu, 0x10u);
    OpLda(memory, cpu, REWARD_VALUE);
    OpSta(memory, cpu, OpAbs(cpu, REWARD_REMAINING));
    OpLda(memory, cpu, REWARD_VALUE+1u);
    OpSta(memory, cpu, OpAbs(cpu, REWARD_REMAINING+1u));
    if (cpu->negative) {
        OpStz(memory, cpu, OpAbs(cpu, REWARD_REMAINING));
        OpStz(memory, cpu, OpAbs(cpu, REWARD_REMAINING+1u));
    } else {
        OpLdy(cpu, 1u);
        OpWriteX(memory, cpu, OpAbs(cpu, REWARD_MENU_ORDINAL), cpu->y);
        OpStz(memory, cpu, OpDp(cpu, DP_REWARD_MENU_PARAMETER));
        OpLoadA(cpu, 4u);
        OpSta(memory, cpu, OpDp(cpu, DP_REWARD_MENU_OPERATION));
        if (!ApplyRewardChild(memory, cpu, child, context,
                0x8ec2f0u, 0x82e746u, 0u, &result))
            return result;
        TransferDirectToA(cpu);
        OpSta(memory, cpu, OpAbs(cpu, REWARD_MESSAGE_FIRST));
        OpLoadA(cpu, 0x0fu);
        OpSta(memory, cpu, OpAbs(cpu, REWARD_MESSAGE_SECOND));
        OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, REWARD_REMAINING)));
        if (cpu->negative) {
            OpLoadA(cpu, 0xffu);
            OpSta(memory, cpu, REWARD_MESSAGE_TIME+1u);
            OpSta(memory, cpu, REWARD_MESSAGE_TIME+2u);
            if (!ApplyRewardChild(memory, cpu, child, context,
                    0x8ec30cu, 0x8385dcu, 0u, &result) ||
                !ApplyRewardChild(memory, cpu, child, context,
                    0x8ec310u, 0x83afcdu, 0u, &result))
                return result;
        }
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_REWARD_SELECTED_RECORD)));
        OpLda(memory, cpu, OpAbs(cpu, REWARD_REMAINING+1u));
        OpAndValue(cpu, 0x7fu);
        OpOra(memory, cpu, OpAbs(cpu, REWARD_REMAINING));
        if (!cpu->zero) {
            OpLda(memory, cpu, REWARD_OBJECT);
            OpSta(memory, cpu, REWARD_ACTOR_REQUEST);
            OpOraValue(cpu, 0x80u);
            ExchangeAccumulatorBytes(cpu);
            OpLoadA(cpu, 0x40u);
            if (!ApplyRewardChild(memory, cpu, child, context,
                    0x8ec32du, 0x83f559u, 0u, &result))
                return result;
        }
    }
    cpu->carry=1u;
    TransferDirectToA(cpu);
    OpSta(memory, cpu, WRAM_UNK_7FD0AE);
    return ExecutionReturned(0x8ec337u);
}
