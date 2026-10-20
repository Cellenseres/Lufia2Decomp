#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/party.h"
#include "system/wram.h"

enum {
    CAPSULE_SPECIAL_FOOD = 0x35u,
    CAPSULE_SPECIAL_FORM = 5u,
    CAPSULE_SPECIAL_FORM_BIT = 8u,
    ROM_CAPSULE_EVOLUTION_FOOD = 0x8ee49au,
    ROM_CAPSULE_REVERSION_FOOD = 0x8ee4a8u,
    ROM_CAPSULE_REVERSION_FORM = 0x8ee4b6u,
    ROM_CAPSULE_FOOD_VALUES = 0x8ee48au,
    DP_SAVED_FORM = 0u,
    DP_CAPSULE_TABLE_INDEX = 2u,
    DP_FOOD_VALUE_INDEX = 0x54u,
    FOOD_VALUE_KIND = 0x154du,
    FOOD_MESSAGE = 0x154eu,
    FOOD_VALUE = 0x154fu
};

typedef struct CapsuleFoodCall {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
} CapsuleFoodCall;

static bool FoodContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x82u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && cpu->direct_page == 0u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static bool FoodChild(CapsuleFoodCall *call, uint32_t site, uint32_t target,
                      uint8_t frame) {
    if (CallChildWithFrame(call->memory, call->cpu, call->child, call->context,
                           site, target, frame, 0x82u))
        return true;
    call->result = ExecutionReturned(site);
    call->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return false;
}

Lufia2ExecutionResult Lufia2CapsuleClassifyFood(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!FoodContext(cpu))
        return ExecutionHandoff(cpu, 0x82c647u);
    SetAccumulatorWidth(cpu, 0u);
    LoadA16(cpu, CAPSULE_SPECIAL_FOOD);
    Compare16(cpu, cpu->accumulator,
              Read16AbsoluteIndexed(memory, cpu, WRAM_ITEM_RECORD_ID, 0u));
    if (cpu->zero) {
        SetAccumulatorWidth(cpu, 1u);
        LoadA8(cpu, 1u);
        cpu->carry = 0u;
        return ExecutionReturned(0x82c66du);
    }
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_CAPSULE_SELECTED_ID, 0u));
    And16(cpu, 0xffu);
    AslA16(cpu);
    LoadX16(cpu, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory,
                           ROM_CAPSULE_REVERSION_FOOD + cpu->x));
    Compare16(cpu, cpu->accumulator,
              Read16AbsoluteIndexed(memory, cpu, WRAM_ITEM_RECORD_ID, 0u));
    const bool special = cpu->zero != 0u;
    SetAccumulatorWidth(cpu, 1u);
    if (special)
        LoadA8(cpu, 0u);
    cpu->carry = special ? 0u : 1u;
    return ExecutionReturned(special ? 0x82c667u : 0x82c671u);
}

Lufia2ExecutionResult Lufia2CapsuleDrawFoodMessage(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!FoodContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82c672u);
    CapsuleFoodCall call = {memory, cpu, child, context, {0}};
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, 0x5fu));
    OpLdy(cpu, 0xc79fu);
    if (!FoodChild(&call, 0x82c67eu, 0x808878u, 3u))
        return call.result;
    return ExecutionReturned(0x82c682u);
}

static bool SelectFoodValue(CapsuleFoodCall *call) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_SELECTED_FORM));
    OpDecA(cpu);
    OpAslA(cpu);
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_FOOD_VALUE_INDEX));
    OpStz(memory, cpu, OpDp(cpu, DP_FOOD_VALUE_INDEX + 1u));
    OpLda(memory, cpu, OpAbs(cpu, FOOD_VALUE_KIND));
    OpLsrA(cpu);
    OpLsrA(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, DP_FOOD_VALUE_INDEX));
    OpSta(memory, cpu, OpDp(cpu, DP_FOOD_VALUE_INDEX));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_FOOD_VALUE_INDEX)));
    OpLda(memory, cpu, OpLongX(cpu, ROM_CAPSULE_FOOD_VALUES));
    OpSta(memory, cpu, OpAbs(cpu, FOOD_VALUE));
    OpSta(memory, cpu, OpAbs(cpu, FOOD_MESSAGE));
    if (!FoodChild(call, 0x82cc16u, 0x82c504u, 2u))
        return false;
    OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, 0u)));
    OpCpy(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID)));
    if (cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, FOOD_VALUE));
        OpAslA(cpu);
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpAbs(cpu, FOOD_VALUE));
        OpSta(memory, cpu, OpAbs(cpu, FOOD_VALUE));
        OpLoadA(cpu, 11u);
        OpSta(memory, cpu, OpAbs(cpu, FOOD_MESSAGE));
    }
    return true;
}

Lufia2ExecutionResult Lufia2CapsuleDescribeFood(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!FoodContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82cbd2u);
    CapsuleFoodCall call = {memory, cpu, child, context, {0}};
    if (!FoodChild(&call, 0x82cbd2u, 0x82c647u, 2u))
        return call.result;
    if (cpu->carry) {
        if (!SelectFoodValue(&call))
            return call.result;
    } else {
        OpCmpValue(cpu, 0u);
        if (cpu->zero) {
            if (!FoodChild(&call, 0x82cbdbu, 0x82c53au, 2u))
                return call.result;
            OpLoadA(cpu, 1u);
            OpSta(memory, cpu, OpAbs(cpu, FOOD_VALUE));
            OpLoadA(cpu, 9u);
        } else {
            OpLoadA(cpu, 8u);
            OpSta(memory, cpu, OpAbs(cpu, FOOD_VALUE));
            OpLoadA(cpu, 10u);
        }
        OpSta(memory, cpu, OpAbs(cpu, FOOD_MESSAGE));
    }
    if (!FoodChild(&call, 0x82cc31u, 0x82c672u, 2u))
        return call.result;
    OpLoadA(cpu, 8u);
    OpTestBits(memory, cpu, OpDp(cpu, 0x74u), 1u);
    if (!FoodChild(&call, 0x82cc38u, 0x82cb10u, 2u))
        return call.result;
    return ExecutionReturned(0x82cc3bu);
}

static bool ChangeSpecialForm(CapsuleFoodCall *call) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    if (!FoodChild(call, 0x82cc6du, 0x82c482u, 2u))
        return false;
    OpLda(memory, cpu, OpAbsX(cpu, 0u));
    OpAndValue(cpu, CAPSULE_SPECIAL_FORM_BIT);
    const bool reverting = !cpu->zero;
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_CAPSULE_TABLE_INDEX)));
    OpLda(memory, cpu, OpLongX(cpu, reverting
                             ? ROM_CAPSULE_REVERSION_FOOD
                             : ROM_CAPSULE_EVOLUTION_FOOD));
    OpCmp(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
    if (!cpu->zero)
        return false;
    if (!reverting) {
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_CAPSULE_TABLE_INDEX)));
        OpLda(memory, cpu, OpLongX(cpu, ROM_CAPSULE_REVERSION_FORM));
        OpCmp(memory, cpu, OpDp(cpu, DP_SAVED_FORM));
        if (!cpu->zero)
            return false;
    }
    OpSepWidths(cpu, 0x20u);
    if (!FoodChild(call, reverting ? 0x82ccb1u : 0x82cc90u,
                   0x82c482u, 2u))
        return false;
    OpLda(memory, cpu, OpAbsX(cpu, 0u));
    if (reverting)
        OpAndValue(cpu, 7u);
    else
        OpOraValue(cpu, CAPSULE_SPECIAL_FORM_BIT);
    OpSta(memory, cpu, OpAbsX(cpu, 0u));
    if (reverting) {
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_CAPSULE_TABLE_INDEX)));
        OpLda(memory, cpu, OpLongX(cpu, ROM_CAPSULE_REVERSION_FORM));
    } else {
        OpLoadA(cpu, CAPSULE_SPECIAL_FORM);
    }
    OpSta(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_SELECTED_FORM));
    return true;
}

Lufia2ExecutionResult Lufia2CapsuleApplySpecialFood(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!FoodContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82cc58u);
    CapsuleFoodCall call = {memory, cpu, child, context, {0}};
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_SELECTED_FORM));
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, DP_SAVED_FORM));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_SELECTED_ID));
    OpAndValue(cpu, 0xffu);
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_CAPSULE_TABLE_INDEX));
    OpSepWidths(cpu, 0x20u);
    if (!ChangeSpecialForm(&call)) {
        if (call.result.flow == LUFIA2_EXECUTION_CHILD_UNWOUND)
            return call.result;
        OpSepWidths(cpu, 0x20u);
        if (!FoodChild(&call, 0x82ccfau, 0x8297e3u, 2u))
            return call.result;
        return ExecutionReturned(0x82ccfdu);
    }
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 1u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_SELECTION)));
    if (!FoodChild(&call, 0x82cccdu, 0x82fad0u, 2u))
        return call.result;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 2u);
    if (!FoodChild(&call, 0x82ccd4u, 0x80953bu, 3u))
        return call.result;
    OpLoadA(cpu, 0x3cu);
    if (!FoodChild(&call, 0x82ccdau, 0x829353u, 2u))
        return call.result;
    OpLoadA(cpu, 0x3eu);
    if (!FoodChild(&call, 0x82ccdfu, 0x8093feu, 3u))
        return call.result;
    OpLoadA(cpu, 0x3cu);
    if (!FoodChild(&call, 0x82cce5u, 0x829353u, 2u) ||
        !FoodChild(&call, 0x82cce8u, 0x82c818u, 2u))
        return call.result;
    OpLoadA(cpu, 0x3cu);
    if (!FoodChild(&call, 0x82ccedu, 0x829353u, 2u))
        return call.result;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E099D));
    if (!FoodChild(&call, 0x82ccf3u, 0x8093feu, 3u))
        return call.result;
    return ExecutionReturned(0x82ccf7u);
}
