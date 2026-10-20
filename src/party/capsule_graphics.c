#include "party/capsule_presentation_internal.h"

static void QueueFormTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, bool saved) {
    const uint16_t channel = saved ? 0x4310u : 0x4300u;
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, DP_NMI_UPLOAD_FLAGS));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, channel));
    OpLdx(cpu, saved ? 0x7000u : 0x6000u);
    OpWriteX(memory, cpu, OpAbs(cpu, channel + 2u), cpu->x);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpAbs(cpu, channel + 4u));
    OpLdx(cpu, 0x0800u);
    OpWriteX(memory, cpu, OpAbs(cpu, channel + 5u), cpu->x);
    OpLoadA(cpu, 0x18u);
    OpSta(memory, cpu, OpAbs(cpu, channel + 1u));
    OpLdx(cpu, saved ? 0x3400u : 0x3000u);
    OpWriteX(memory, cpu, OpDp(cpu, saved ? 0x7bu : 0x79u), cpu->x);
    OpLoadA(cpu, saved ? 0x42u : 0x41u);
    OpSta(memory, cpu, OpDp(cpu, saved ? 0x76u : 0x75u));
}

Lufia2ExecutionResult Lufia2CapsuleQueueFormTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CapsulePresentationReady(cpu, true))
        return ExecutionHandoff(cpu, 0x82cfd4u);
    QueueFormTiles(memory, cpu, false);
    return ExecutionReturned(0x82cffcu);
}

Lufia2ExecutionResult Lufia2CapsuleQueueSavedFormTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CapsulePresentationReady(cpu, true))
        return ExecutionHandoff(cpu, 0x82cffdu);
    QueueFormTiles(memory, cpu, true);
    return ExecutionReturned(0x82d025u);
}

static void CopyFormTileRow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, bool second_half) {
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_FORM_COPY_SOURCE)));
    if (second_half) {
        OpLda(memory, cpu, OpDp(cpu, DP_FORM_COPY_DESTINATION));
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x0200u);
        OpTay(cpu);
    } else {
        OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_FORM_COPY_DESTINATION)));
    }
    OpLoadA(cpu, 0x003fu);
    OpMoveNext(memory, cpu, 0x7eu, 0x7eu);
    OpLda(memory, cpu, OpDp(cpu, DP_FORM_COPY_SOURCE));
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x0040u);
    OpSta(memory, cpu, OpDp(cpu, DP_FORM_COPY_SOURCE));
}

Lufia2ExecutionResult Lufia2CapsuleArrangeFormTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CapsulePresentationReady(cpu, false))
        return ExecutionHandoff(cpu, 0x82d026u);
    PushDataBank(memory, cpu);
    OpLdx(cpu, 0x4000u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_FORM_COPY_SOURCE), cpu->x);
    OpLdx(cpu, 9u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_FORM_COPY_ROWS), cpu->x);
    do {
        CopyFormTileRow(memory, cpu, false);
        CopyFormTileRow(memory, cpu, true);
        OpLda(memory, cpu, OpDp(cpu, DP_FORM_COPY_DESTINATION));
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x0040u);
        OpSta(memory, cpu, OpDp(cpu, DP_FORM_COPY_DESTINATION));
        OpStepMem(memory, cpu, OpDp(cpu, DP_FORM_COPY_COLUMN), 1);
        OpLda(memory, cpu, OpDp(cpu, DP_FORM_COPY_COLUMN));
        OpCmpValue(cpu, 8u);
        if (cpu->zero) {
            OpStz(memory, cpu, OpDp(cpu, DP_FORM_COPY_COLUMN));
            OpLda(memory, cpu, OpDp(cpu, DP_FORM_COPY_DESTINATION));
            cpu->carry = 0u;
            OpAdcValue(cpu, 0x0200u);
            OpSta(memory, cpu, OpDp(cpu, DP_FORM_COPY_DESTINATION));
        }
        OpStepMem(memory, cpu, OpDp(cpu, DP_FORM_COPY_ROWS), -1);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x82d07au);
}

Lufia2ExecutionResult Lufia2CapsuleLoadFormGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CapsulePresentationReady(cpu, true) || !child)
        return ExecutionHandoff(cpu, 0x82cf9du);
    CapsuleFeedCall call = {memory, cpu, child, context, {0}};
    PushAccumulator8(memory, cpu);
    OpLdx(cpu, 0x4000u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_FORM_COPY_DESTINATION), cpu->x);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, DP_FORM_COPY_DESTINATION + 2u));
    if (!CapsuleFeedChild(&call, 0x82cfa7u, 0x82c3c4u, 2u))
        return call.result;
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x0194u);
    OpSta(memory, cpu, OpDp(cpu, DP_FORM_RESOURCE));
    if (!CapsuleFeedChild(&call, 0x82cfb5u, 0x808e9du, 3u))
        return call.result;
    OpLdx(cpu, 0x6000u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_FORM_COPY_DESTINATION), cpu->x);
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_FORM_COPY_COLUMN), cpu->x);
    if (!CapsuleFeedChild(&call, 0x82cfc3u, 0x82d026u, 2u))
        return call.result;
    OpSepWidths(cpu, 0x20u);
    if (!CapsuleFeedChild(&call, 0x82cfc8u, 0x82c3c4u, 2u))
        return call.result;
    OpSta(memory, cpu, OpDp(cpu, 2u));
    OpLoadA(cpu, Pull8(memory, cpu));
    OpSta(memory, cpu, OpDp(cpu, 0u));
    if (!CapsuleFeedChild(&call, 0x82cfd0u, 0x82cf72u, 2u))
        return call.result;
    return ExecutionReturned(0x82cfd3u);
}

static void ClearFormEffectRecords(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 0u);
    do {
        OpLoadA(cpu, 0u);
        OpSta(memory, cpu, OpLongX(cpu, 0x7ea920u));
        OpSta(memory, cpu, OpLongX(cpu, 0x7ea921u));
        OpLoadA(cpu, 0u);
        OpSta(memory, cpu, OpLongX(cpu, 0x7ea922u));
        OpLoadA(cpu, 0x18u);
        OpSta(memory, cpu, OpLongX(cpu, 0x7ea923u));
        OpInx(cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, 0x20u);
    } while (!cpu->zero);
}

Lufia2ExecutionResult Lufia2CapsuleLoadFormEffects(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CapsulePresentationReady(cpu, true) || !child)
        return ExecutionHandoff(cpu, 0x82d4eeu);
    CapsuleFeedCall call = {memory, cpu, child, context, {0}};
    OpSepWidths(cpu, 0x10u);
    OpLdx(cpu, 0x10u);
    OpLdy(cpu, 0x0cu);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_FRAME_INDEX));
        OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ACTIVE));
        OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_X_HIGH));
        OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_Y_HIGH));
        if (!CapsuleFeedChild(&call, 0x82d500u, 0x868cdau, 3u))
            return call.result;
        OpRepWidths(cpu, 0x10u);
        OpLoadA(cpu, 2u);
        if (!CapsuleFeedChild(&call, 0x82d508u, 0x868cf5u, 3u))
            return call.result;
        OpSepWidths(cpu, 0x10u);
        OpInx(cpu);
        OpCpx(cpu, 0x14u);
    } while (!cpu->zero);
    OpRepWidths(cpu, 0x10u);
    ClearFormEffectRecords(memory, cpu);
    OpRepWidths(cpu, 0x30u);
    PushDataBank(memory, cpu);
    OpLdx(cpu, 0x8380u);
    OpLdy(cpu, 0x04e0u);
    OpLoadA(cpu, 0x001fu);
    OpMoveNext(memory, cpu, 0u, 0x9fu);
    PullDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, DP_NMI_UPLOAD_FLAGS));
    OpLdx(cpu, 0x024au);
    OpWriteX(memory, cpu, OpDp(cpu, DP_FORM_RESOURCE), cpu->x);
    OpLdx(cpu, 0x4000u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_FORM_COPY_DESTINATION), cpu->x);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, DP_FORM_COPY_DESTINATION + 2u));
    if (!CapsuleFeedChild(&call, 0x82d55bu, 0x808e9du, 3u))
        return call.result;
    OpLdx(cpu, 0x4800u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_FORM_COPY_SOURCE), cpu->x);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, DP_FORM_COPY_SOURCE + 2u));
    OpLdx(cpu, 0x3800u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_FORM_COPY_DESTINATION), cpu->x);
    OpLdx(cpu, 0x0400u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_FORM_COPY_ROWS), cpu->x);
    if (!CapsuleFeedChild(&call, 0x82d572u, 0x828044u, 3u))
        return call.result;
    return ExecutionReturned(0x82d576u);
}
