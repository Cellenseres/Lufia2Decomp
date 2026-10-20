#include "party/capsule_feed_internal.h"

enum {
    FORM_TILE_BYTES = 0x0800u,
    FORM_PANEL_COLUMNS = 8u,
    FORM_PANEL_ROWS = 8u,
    FORM_PANEL_START = 0x0106u,
    FORM_TILE_ROW_REMAINDER = 0x30u,
    FORM_PANEL_STATE = 0x1551u,
    ROM_FORM_PANEL_TILES = 0xa6fa8du,
    ROM_FORM_UPLOAD_RECORD = 0x8366u,
    ROM_FORM_TILE_COEFFICIENTS = 0x82c9d0u,
    DP_FORM_ROW_COUNT = 0x13u,
    DP_FORM_COLUMN_COUNT = 0x15u,
    DP_FORM_TILE_VALUE = 6u,
    DP_FORM_TILE_SOURCE = 8u
};

static void UseFormTileBank(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpRepWidths(cpu, 0x20u);
}

Lufia2ExecutionResult Lufia2CapsuleLoadFormPanel(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CapsuleFeedReady(cpu))
        return ExecutionHandoff(cpu, 0x82c85au);
    OpRepWidths(cpu, 0x20u);
    UseFormTileBank(memory, cpu);
    OpLdx(cpu, 0u);
    OpLdy(cpu, FORM_PANEL_START);
    OpLoadA(cpu, FORM_PANEL_ROWS);
    OpSta(memory, cpu, OpDp(cpu, DP_FEED_ROWS));
    do {
        OpLoadA(cpu, FORM_PANEL_COLUMNS);
        OpSta(memory, cpu, OpDp(cpu, DP_FEED_COLUMNS));
        do {
            OpLda(memory, cpu, OpLongX(cpu, ROM_FORM_PANEL_TILES));
            OpSta(memory, cpu, OpAbsY(cpu, 0x3000u));
            OpIny(cpu);
            OpIny(cpu);
            OpInx(cpu);
            OpInx(cpu);
            OpStepMem(memory, cpu, OpDp(cpu, DP_FEED_COLUMNS), -1);
        } while (!cpu->zero);
        OpTya(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, FORM_TILE_ROW_REMAINDER);
        OpTay(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, DP_FEED_ROWS), -1);
    } while (!cpu->zero);
    OpLdx(cpu, ROM_FORM_UPLOAD_RECORD);
    OpLdy(cpu, 0x0340u);
    OpLoadA(cpu, 5u);
    OpMoveNext(memory, cpu, 0u, 0x9fu);
    OpLdx(cpu, 0x0352u);
    OpStz(memory, cpu, OpAbsX(cpu, 0u));
    OpStz(memory, cpu, OpAbsX(cpu, 2u));
    OpStz(memory, cpu, OpAbsX(cpu, 4u));
    PullDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 6u);
    OpSta(memory, cpu, OpAbs(cpu, FORM_PANEL_STATE));
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, 0x73u));
    OpLoadA(cpu, 8u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_MENU_FRAME_FLAGS), 1u);
    return ExecutionReturned(0x82c8b6u);
}

Lufia2ExecutionResult Lufia2CapsuleClearFormPanel(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CapsuleFeedReady(cpu))
        return ExecutionHandoff(cpu, 0x82c8b7u);
    OpRepWidths(cpu, 0x20u);
    UseFormTileBank(memory, cpu);
    OpLdy(cpu, FORM_PANEL_START);
    OpLoadA(cpu, FORM_PANEL_ROWS);
    OpSta(memory, cpu, OpDp(cpu, DP_FEED_ROWS));
    do {
        OpLoadA(cpu, FORM_PANEL_COLUMNS);
        OpSta(memory, cpu, OpDp(cpu, DP_FEED_COLUMNS));
        do {
            OpLoadA(cpu, 0u);
            OpSta(memory, cpu, OpAbsY(cpu, 0x3000u));
            OpIny(cpu);
            OpIny(cpu);
            OpStepMem(memory, cpu, OpDp(cpu, DP_FEED_COLUMNS), -1);
        } while (!cpu->zero);
        OpTya(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, FORM_TILE_ROW_REMAINDER);
        OpTay(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, DP_FEED_ROWS), -1);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 8u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_MENU_FRAME_FLAGS), 1u);
    return ExecutionReturned(0x82c8ecu);
}

Lufia2ExecutionResult Lufia2CapsuleSaveFormTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CapsuleFeedReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82c8edu);
    CapsuleFeedCall call = {memory, cpu, child, context, {0}};
    OpRepWidths(cpu, 0x20u);
    PushDataBank(memory, cpu);
    OpLdx(cpu, 0x6000u);
    OpLdy(cpu, 0x7000u);
    OpLoadA(cpu, FORM_TILE_BYTES - 1u);
    OpMoveNext(memory, cpu, 0x7eu, 0x7eu);
    OpLdx(cpu, 0x04a0u);
    OpLdy(cpu, 0x04c0u);
    OpLoadA(cpu, 0x001fu);
    OpMoveNext(memory, cpu, 0u, 0u);
    PullDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, 0x73u));
    OpLdx(cpu, 1u);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ACTIVE));
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_FRAME_INDEX));
    OpLoadA(cpu, 0u);
    if (!CapsuleFeedChild(&call, 0x82c91eu, 0x868cf5u, 3u))
        return call.result;
    OpLdx(cpu, 0u);
    OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ACTIVE));
    return ExecutionReturned(0x82c928u);
}

Lufia2ExecutionResult Lufia2CapsuleCopyFormTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CapsuleFeedReady(cpu))
        return ExecutionHandoff(cpu, 0x82c929u);
    OpRepWidths(cpu, 0x20u);
    PushDataBank(memory, cpu);
    OpLdx(cpu, 0x6000u);
    OpLdy(cpu, 0x4000u);
    OpLoadA(cpu, FORM_TILE_BYTES - 1u);
    OpMoveNext(memory, cpu, 0x7eu, 0x7eu);
    PullDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82c93bu);
}

Lufia2ExecutionResult Lufia2CapsuleClearFormTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CapsuleFeedReady(cpu))
        return ExecutionHandoff(cpu, 0x82c93cu);
    OpRepWidths(cpu, 0x20u);
    UseFormTileBank(memory, cpu);
    OpLdx(cpu, 0u);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, 0x6000u));
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, FORM_TILE_BYTES);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82c957u);
}

static bool WaitForFormTiles(CapsuleFeedCall *call, bool finishing) {
    do {
        if (!CapsuleFeedChild(call, finishing ? 0x82c9c3u : 0x82c96cu,
                              0x868b55u, 3u) ||
            !CapsuleFeedChild(call, finishing ? 0x82c9c7u : 0x82c970u,
                              0x82d378u, 2u) ||
            !CapsuleFeedChild(call, finishing ? 0x82c9cau : 0x82c973u,
                              finishing ? 0x82d443u : 0x82d3acu, 2u))
            return false;
    } while (call->cpu->carry);
    return true;
}

static bool TransformFormTiles(CapsuleFeedCall *call) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    UseFormTileBank(memory, cpu);
    OpStz(memory, cpu, OpDp(cpu, DP_FEED_COLUMNS));
    OpLoadA(cpu, 6u);
    OpSta(memory, cpu, OpDp(cpu, DP_FORM_ROW_COUNT));
    do {
        OpStz(memory, cpu, OpDp(cpu, DP_FEED_ROWS));
        OpLoadA(cpu, 6u);
        OpSta(memory, cpu, OpDp(cpu, DP_FORM_COLUMN_COUNT));
        do {
            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_FEED_COLUMNS)));
            OpLda(memory, cpu, OpLongX(cpu, ROM_FORM_TILE_COEFFICIENTS));
            OpSta(memory, cpu, OpDp(cpu, DP_FORM_TILE_VALUE));
            if (!CapsuleFeedChild(call, 0x82c998u, 0x82ca18u, 2u))
                return false;
            OpStepMem(memory, cpu, OpDp(cpu, DP_FEED_COLUMNS), 1);
            OpStepMem(memory, cpu, OpDp(cpu, DP_FEED_COLUMNS), 1);
            OpStepMem(memory, cpu, OpDp(cpu, DP_FORM_COLUMN_COUNT), -1);
        } while (!cpu->zero);
        OpStepMem(memory, cpu, OpDp(cpu, DP_FORM_ROW_COUNT), -1);
    } while (!cpu->zero);
    OpLda(memory, cpu, OpDp(cpu, DP_FORM_TILE_SOURCE));
    cpu->carry = 0u;
    OpAdcValue(cpu, FORM_TILE_ROW_REMAINDER);
    OpSta(memory, cpu, OpDp(cpu, DP_FORM_TILE_SOURCE));
    OpSepWidths(cpu, 0x20u);
    PullDataBank(memory, cpu);
    return true;
}

Lufia2ExecutionResult Lufia2CapsuleAnimateFormTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CapsuleFeedReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82c958u);
    CapsuleFeedCall call = {memory, cpu, child, context, {0}};
    OpLdx(cpu, 0x9d20u);
    OpWriteX(memory, cpu, OpDp(cpu, 8u), cpu->x);
    OpLdx(cpu, 0x7000u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x0bu), cpu->x);
    OpLdx(cpu, 0x6000u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x19u), cpu->x);
    OpLdx(cpu, 0x4000u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x1cu), cpu->x);
    if (!WaitForFormTiles(&call, false))
        return call.result;
    OpLoadA(cpu, 0x40u);
    do {
        PushAccumulator8(memory, cpu);
        if (!TransformFormTiles(&call))
            return call.result;
        if (!CapsuleFeedChild(&call, 0x82c9b2u, 0x82cfd4u, 2u) ||
            !CapsuleFeedChild(&call, 0x82c9b5u, 0x82cffdu, 2u) ||
            !CapsuleFeedChild(&call, 0x82c9b8u, 0x82d378u, 2u) ||
            !CapsuleFeedChild(&call, 0x82c9bbu, 0x868b55u, 3u))
            return call.result;
        OpLoadA(cpu, Pull8(memory, cpu));
        OpDecA(cpu);
    } while (!cpu->zero);
    if (!WaitForFormTiles(&call, true))
        return call.result;
    return ExecutionReturned(0x82c9cfu);
}

Lufia2ExecutionResult Lufia2CapsuleRefreshFormScene(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CapsuleFeedReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82c818u);
    CapsuleFeedCall call = {memory, cpu, child, context, {0}};
    if (!CapsuleFeedChild(&call, 0x82c818u, 0x868b55u, 3u) ||
        !CapsuleFeedChild(&call, 0x82c81cu, 0x82c85au, 2u) ||
        !CapsuleFeedChild(&call, 0x82c81fu, 0x82d4a8u, 2u) ||
        !CapsuleFeedChild(&call, 0x82c822u, 0x82c8edu, 2u) ||
        !CapsuleFeedChild(&call, 0x82c825u, 0x82cffdu, 2u) ||
        !CapsuleFeedChild(&call, 0x82c828u, 0x868b55u, 3u))
        return call.result;
    OpLoadA(cpu, 4u);
    if (!CapsuleFeedChild(&call, 0x82c82eu, 0x82cf9du, 2u) ||
        !CapsuleFeedChild(&call, 0x82c831u, 0x82c929u, 2u) ||
        !CapsuleFeedChild(&call, 0x82c834u, 0x82c93cu, 2u) ||
        !CapsuleFeedChild(&call, 0x82c837u, 0x82cfd4u, 2u) ||
        !CapsuleFeedChild(&call, 0x82c83au, 0x868b55u, 3u))
        return call.result;
    OpLdx(cpu, 0u);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ACTIVE));
    if (!CapsuleFeedChild(&call, 0x82c846u, 0x82c958u, 2u) ||
        !CapsuleFeedChild(&call, 0x82c849u, 0x82c8b7u, 2u) ||
        !CapsuleFeedChild(&call, 0x82c84cu, 0x868b55u, 3u) ||
        !CapsuleFeedChild(&call, 0x82c850u, 0x82d4d0u, 2u))
        return call.result;
    OpLdx(cpu, 1u);
    OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ACTIVE));
    return ExecutionReturned(0x82c859u);
}
