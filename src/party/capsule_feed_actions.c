#include "party/capsule_feed_internal.h"

Lufia2ExecutionResult Lufia2CapsuleGetFoodMeterAddress(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CapsuleFeedReady(cpu))
        return ExecutionHandoff(cpu, 0x82c492u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_SELECTED_ID));
    OpAndValue(cpu, 0xffu);
    cpu->carry = 0u;
    OpAdcValue(cpu, WRAM_CAPSULE_FLAG_BYTES);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82c4a1u);
}

Lufia2ExecutionResult Lufia2CapsuleRefreshSelectedForm(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CapsuleFeedReady(cpu))
        return ExecutionHandoff(cpu, 0x82c53au);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_SELECTED_ID));
    OpAndValue(cpu, 0xffu);
    OpTay(cpu);
    OpLda(memory, cpu, OpAbsY(cpu, WRAM_CAPSULE_FORM_STATES));
    OpAndValue(cpu, 0xffu);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, ROM_CAPSULE_FORM_TYPES));
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_CAPSULE_FORMS));
    return ExecutionReturned(0x82c553u);
}

Lufia2ExecutionResult Lufia2CapsuleResetFoodCursor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CapsuleFeedReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82c5a2u);
    CapsuleFeedCall call = {memory, cpu, child, context, {0}};
    OpStz(memory, cpu, OpAbs(cpu, 0x1553u));
    OpStz(memory, cpu, OpAbs(cpu, 0x1556u));
    OpStz(memory, cpu, OpAbs(cpu, 0x1557u));
    if (!CapsuleFeedChild(&call, 0x82c5abu, 0x82c5f3u, 2u))
        return call.result;
    return ExecutionReturned(0x82c5aeu);
}

Lufia2ExecutionResult Lufia2CapsuleToggleFoodCache(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CapsuleFeedReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82c784u);
    CapsuleFeedCall call = {memory, cpu, child, context, {0}};
    if (!CapsuleFeedChild(&call, 0x82c784u, 0x82c4e4u, 2u))
        return call.result;
    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^
                           OpReadM(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_FLAG_ANY))));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_FLAG_ANY));
    return ExecutionReturned(0x82c78du);
}

static Lufia2ExecutionResult FinishFeedingMenu(CapsuleFeedCall *call) {
    Lufia2CpuState *cpu = call->cpu;
    if (!CapsuleFeedChild(call, 0x82c76eu, 0x868b55u, 3u))
        return call->result;
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x029eu);
    OpLdx(cpu, 0x0e02u);
    if (!CapsuleFeedChild(call, 0x82c77au, 0x8283ebu, 2u))
        return call->result;
    OpSepWidths(cpu, 0x20u);
    if (!CapsuleFeedChild(call, 0x82c77fu, 0x82ac84u, 2u))
        return call->result;
    cpu->carry = 0u;
    return ExecutionReturned(0x82c783u);
}

static bool SelectFoodTier(CapsuleFeedCall *call, bool *accepted) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    *accepted = false;
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, CAPSULE_ITEM_VALUE));
    OpCmpValue(cpu, 0x00c8u);
    if (!cpu->carry)
        return true;
    OpLdx(cpu, 0u);
    for (;;) {
        OpLda(memory, cpu, OpLongX(cpu, ROM_CAPSULE_FOOD_THRESHOLDS));
        OpCmp(memory, cpu, OpAbs(cpu, CAPSULE_ITEM_VALUE));
        if (cpu->carry)
            break;
        OpInx(cpu);
        OpInx(cpu);
    }
    OpTxa(cpu);
    OpLsrA(cpu);
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, CAPSULE_FOOD_TYPE));
    if (!CapsuleFeedChild(call, 0x82c6f4u, 0x82c4f4u, 2u))
        return false;
    OpLda(memory, cpu, OpAbs(cpu, CAPSULE_FOOD_TYPE));
    OpCmp(memory, cpu, OpAbsX(cpu, 0u));
    if (!cpu->carry)
        return true;
    if (!CapsuleFeedChild(call, 0x82c6ffu, 0x82cbd2u, 2u) ||
        !CapsuleFeedChild(call, 0x82c702u, 0x82c4f4u, 2u))
        return false;
    OpLda(memory, cpu, OpAbs(cpu, CAPSULE_FOOD_TYPE));
    OpSta(memory, cpu, OpAbsX(cpu, 0u));
    *accepted = true;
    return true;
}

static bool ReduceFoodTier(CapsuleFeedCall *call) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbs(cpu, CAPSULE_FOOD_MESSAGE));
    if (!CapsuleFeedChild(call, 0x82c714u, 0x82c4f4u, 2u))
        return false;
    OpLda(memory, cpu, OpAbsX(cpu, 0u));
    if (!cpu->zero) {
        OpLdy(cpu, cpu->x);
        OpStepMem(memory, cpu, OpAbsX(cpu, 0u), -1);
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        OpAndValue(cpu, 0xffu);
        OpLsrA(cpu);
        OpLsrA(cpu);
        OpSta(memory, cpu, OpDp(cpu, DP_FEED_SCRATCH));
        OpLda(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_SELECTED_FORM));
        OpAndValue(cpu, 0xffu);
        OpDecA(cpu);
        OpAslA(cpu);
        OpAslA(cpu);
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpDp(cpu, DP_FEED_SCRATCH));
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpLongX(cpu, ROM_CAPSULE_FOOD_AMOUNTS));
        if (cpu->zero) {
            OpLdx(cpu, cpu->y);
            OpStepMem(memory, cpu, OpAbsX(cpu, 0u), 1);
        } else if (!CapsuleFeedChild(call, 0x82c747u, 0x82c784u, 2u)) {
            return false;
        }
    }
    if (!CapsuleFeedChild(call, 0x82c74au, 0x82c672u, 2u))
        return false;
    OpLoadA(cpu, 8u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_MENU_FRAME_FLAGS), 1u);
    OpLoadA(cpu, 0x30u);
    if (!CapsuleFeedChild(call, 0x82c753u, 0x829330u, 2u))
        return false;
    return CapsuleFeedChild(call, 0x82c76bu, 0x82c5a2u, 2u);
}

static bool FinishAcceptedFood(CapsuleFeedCall *call) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    if (!CapsuleFeedChild(call, 0x82c758u, 0x82c784u, 2u) ||
        !CapsuleFeedChild(call, 0x82c75bu, 0x82c492u, 2u))
        return false;
    OpLda(memory, cpu, OpAbsX(cpu, 0u));
    OpCmpValue(cpu, CAPSULE_FOOD_METER_LIMIT);
    if (cpu->zero &&
        (!CapsuleFeedChild(call, 0x82c765u, 0x82c78eu, 2u) ||
         !CapsuleFeedChild(call, 0x82c768u, 0x82d13au, 2u)))
        return false;
    return CapsuleFeedChild(call, 0x82c76bu, 0x82c5a2u, 2u);
}

Lufia2ExecutionResult Lufia2CapsuleFeedItem(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CapsuleFeedReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82c683u);
    CapsuleFeedCall call = {memory, cpu, child, context, {0}};
    bool accepted = false;
    if (!cpu->carry) {
        OpRepWidths(cpu, 0x20u);
        if (!CapsuleFeedChild(&call, 0x82c687u, 0x82fbeeu, 2u))
            return call.result;
        OpSepWidths(cpu, 0x20u);
        if (!cpu->carry) {
            OpLda(memory, cpu, OpAbs(cpu, CAPSULE_ITEM_FIELDS));
            OpBitValue(cpu, 0x20u);
            if (!cpu->zero)
                cpu->carry = 1u;
        }
        if (cpu->carry)
            return ExecutionReturned(0x82c6a1u);
        if (!CapsuleFeedChild(&call, 0x82c695u, 0x82cc3cu, 2u))
            return call.result;
        if (cpu->carry) {
            if (!CapsuleFeedChild(&call, 0x82c69au, 0x82cc58u, 2u))
                return call.result;
            return FinishFeedingMenu(&call);
        }
        OpLoadA(cpu, 2u);
        if (!CapsuleFeedChild(&call, 0x82c6a4u, 0x80953bu, 3u))
            return call.result;
        OpRepWidths(cpu, 0x20u);
        OpLoadA(cpu, 1u);
        OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_SELECTION)));
        if (!CapsuleFeedChild(&call, 0x82c6b0u, 0x82fad0u, 2u))
            return call.result;
        OpSepWidths(cpu, 0x20u);
        if (!CapsuleFeedChild(&call, 0x82c6b5u, 0x82c647u, 2u))
            return call.result;
        if (!cpu->carry) {
            if (!CapsuleFeedChild(&call, 0x82c6bau, 0x82cbd2u, 2u))
                return call.result;
            accepted = true;
        } else {
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpAbs(cpu, CAPSULE_ITEM_FIELDS));
            OpBitValue(cpu, 0x8000u);
            if (cpu->zero) {
                OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID)));
                OpLoadA(cpu, 1u);
                if (!CapsuleFeedChild(&call, 0x82c6d0u, 0x82fc97u, 2u))
                    return call.result;
            }
        }
    }
    if (!accepted && !SelectFoodTier(&call, &accepted))
        return call.result;
    if (accepted ? !FinishAcceptedFood(&call) : !ReduceFoodTier(&call))
        return call.result;
    return FinishFeedingMenu(&call);
}

Lufia2ExecutionResult Lufia2CapsuleAdvanceForm(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CapsuleFeedReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82c78eu);
    CapsuleFeedCall call = {memory, cpu, child, context, {0}};
    if (!CapsuleFeedChild(&call, 0x82c78eu, 0x82c492u, 2u))
        return call.result;
    OpLoadA(cpu, CAPSULE_FOOD_METER_LIMIT);
    OpSta(memory, cpu, OpAbsX(cpu, 0u));
    if (!CapsuleFeedChild(&call, 0x82c796u, 0x82d150u, 2u))
        return call.result;
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x029eu);
    OpLdx(cpu, 0x0e02u);
    if (!CapsuleFeedChild(&call, 0x82c7a1u, 0x8283ebu, 2u))
        return call.result;
    OpLoadA(cpu, 0x009eu);
    OpLdx(cpu, 0x0e03u);
    if (!CapsuleFeedChild(&call, 0x82c7aau, 0x8283ebu, 2u))
        return call.result;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 8u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_MENU_FRAME_FLAGS), 1u);
    if (!CapsuleFeedChild(&call, 0x82c7b3u, 0x868b55u, 3u))
        return call.result;
    OpLoadA(cpu, 0x40u);
    if (!CapsuleFeedChild(&call, 0x82c7b9u, 0x8096ccu, 3u) ||
        !CapsuleFeedChild(&call, 0x82c7bdu, 0x809692u, 3u))
        return call.result;
    OpLoadA(cpu, 0x40u);
    do {
        PushAccumulator8(memory, cpu);
        if (!CapsuleFeedChild(&call, 0x82c7c4u, 0x868b55u, 3u))
            return call.result;
        OpLoadA(cpu, Pull8(memory, cpu));
        OpDecA(cpu);
    } while (!cpu->zero);
    if (!CapsuleFeedChild(&call, 0x82c7ccu, 0x828345u, 2u))
        return call.result;
    OpLoadA(cpu, 0x3cu);
    if (!CapsuleFeedChild(&call, 0x82c7d1u, 0x8093feu, 3u) ||
        !CapsuleFeedChild(&call, 0x82c7d5u, 0x82d4eeu, 2u) ||
        !CapsuleFeedChild(&call, 0x82c7d8u, 0x82d577u, 2u))
        return call.result;
    OpLoadA(cpu, 4u);
    if (!CapsuleFeedChild(&call, 0x82c7ddu, 0x82cf9du, 2u))
        return call.result;
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x01dau);
    OpLdx(cpu, 0x1201u);
    if (!CapsuleFeedChild(&call, 0x82c7e8u, 0x8283ebu, 2u))
        return call.result;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 8u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_MENU_FRAME_FLAGS), 1u);
    if (!CapsuleFeedChild(&call, 0x82c7f1u, 0x868b55u, 3u))
        return call.result;
    OpStepMem(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_SELECTED_FORM), 1);
    if (!CapsuleFeedChild(&call, 0x82c7f8u, 0x82c482u, 2u))
        return call.result;
    OpStepMem(memory, cpu, OpAbsX(cpu, 0u), 1);
    if (!CapsuleFeedChild(&call, 0x82c7feu, 0x82c53au, 2u) ||
        !CapsuleFeedChild(&call, 0x82c801u, 0x82c492u, 2u))
        return call.result;
    OpStz(memory, cpu, OpAbsX(cpu, 0u));
    if (!CapsuleFeedChild(&call, 0x82c807u, 0x82c818u, 2u))
        return call.result;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E099D));
    if (!CapsuleFeedChild(&call, 0x82c80du, 0x8093feu, 3u) ||
        !CapsuleFeedChild(&call, 0x82c811u, 0x82836au, 2u) ||
        !CapsuleFeedChild(&call, 0x82c814u, 0x82d270u, 2u))
        return call.result;
    return ExecutionReturned(0x82c817u);
}
