#include "lufia2/menu.h"
#include "menu/menu_action_internal.h"
#include "system/wram.h"

enum {
    DP_WARP_FLAG_BIT = 0x11u,
    DP_WARP_FLAG_WORD = 0x13u,
    DP_WARP_OUTPUT_OFFSET = 0x15u,
    DP_WARP_MAP_ID = 0x17u,
    DP_WARP_TABLE_OFFSET = 0x19u,
    DP_WARP_RECORD_WORD = 0x1cu,
    ROM_WARP_RECORD_TABLE = 0xd06eu,
    WARP_SAVED_SCROLL = WRAM_UNK_7E93C0,
    WARP_SAVED_TOP_ROW = WRAM_UNK_7E93C0 + 2u,
    WARP_MAP_IDS = WRAM_UNK_7E93C0 + 4u,
    WARP_RECORD_WORDS = WRAM_UNK_7E93C0 + 0x40u,
    WARP_NAME_POINTERS = WRAM_UNK_7E93C0 + 0x7cu,
    WARP_SECONDARY_LIMIT = WRAM_BATTLE_PARTY_IDS + 2u,
    DP_WARP_DRAW_INDEX = 0x00u,
    DP_WARP_DRAW_TILEMAP = 0x11u,
    DP_WARP_DRAW_ROWS = 0x15u,
    WARP_INCOMING_ROW_TILEMAP = 0x374eu,
    WARP_VISIBLE_ROWS = 6u,
    WARP_SCROLL_REQUEST = WRAM_MENU_DISPLAY_REQUESTS + 1u
};

static void SelectWarpFlag(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 1u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_WARP_FLAG_BIT), cpu->x);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, 1u));
    OpSta(memory, cpu, OpDp(cpu, DP_WARP_RECORD_WORD));
    OpLda(memory, cpu, OpAbsX(cpu, 0u));
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, DP_WARP_MAP_ID));
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_WARP_FLAG_WORD));
    OpLda(memory, cpu, OpAbsX(cpu, 0u));
    OpAndValue(cpu, 0x0fu);
    while (!cpu->zero) {
        const uint32_t address = OpDp(cpu, DP_WARP_FLAG_BIT);
        const uint16_t old = OpRead16(memory, address);
        const uint16_t bit = (uint16_t)(old << 1);
        cpu->carry = (old & 0x8000u) != 0u;
        Write8(memory, OpNextByte(address), (uint8_t)(bit >> 8));
        Write8(memory, address, (uint8_t)bit);
        SetNz16(cpu, bit);
        OpDecA(cpu);
    }
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_WARP_FLAG_WORD)));
    OpLda(memory, cpu, OpAbsY(cpu, WRAM_SCENE_MAP_FLAGS));
    OpBit(memory, cpu, OpDp(cpu, DP_WARP_FLAG_BIT));
}

static void AppendWarpDestination(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpInx(cpu);
    OpInx(cpu);
    OpInx(cpu);
    OpTxa(cpu);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_WARP_OUTPUT_OFFSET)));
    OpSta(memory, cpu, OpLongX(cpu, WARP_NAME_POINTERS));
    OpLda(memory, cpu, OpDp(cpu, DP_WARP_MAP_ID));
    OpSta(memory, cpu, OpLongX(cpu, WARP_MAP_IDS));
    OpLda(memory, cpu, OpDp(cpu, DP_WARP_RECORD_WORD));
    OpSta(memory, cpu, OpLongX(cpu, WARP_RECORD_WORDS));
    OpStepMem(memory, cpu, OpDp(cpu, DP_WARP_OUTPUT_OFFSET), 1);
    OpStepMem(memory, cpu, OpDp(cpu, DP_WARP_OUTPUT_OFFSET), 1);
}

Lufia2ExecutionResult Lufia2MenuCollectWarpDestinations(const Lufia2Memory *memory,
                                                        Lufia2CpuState *cpu,
                                                        Lufia2PushedChildCall child,
                                                        void *context) {
    (void)context;
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x829449u);
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x8eu);
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_WARP_OUTPUT_OFFSET), cpu->x);
    for (;;) {
        OpWriteX(memory, cpu, OpDp(cpu, DP_WARP_TABLE_OFFSET), cpu->x);
        OpLda(memory, cpu, OpAbsX(cpu, ROM_WARP_RECORD_TABLE));
        OpCmpValue(cpu, 0xffffu);
        if (cpu->zero)
            break;
        SelectWarpFlag(memory, cpu);
        if (!cpu->zero)
            AppendWarpDestination(memory, cpu);
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_WARP_TABLE_OFFSET)));
        OpInx(cpu);
        OpInx(cpu);
    }
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_WARP_OUTPUT_OFFSET)));
    OpLoadA(cpu, 0xffffu);
    OpSta(memory, cpu, OpLongX(cpu, WARP_NAME_POINTERS));
    OpLsrMem(memory, cpu, OpDp(cpu, DP_WARP_OUTPUT_OFFSET));
    OpSepWidths(cpu, 0x20u);
    PullDataBank(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, DP_WARP_OUTPUT_OFFSET));
    return ExecutionReturned(0x8294bfu);
}

Lufia2ExecutionResult Lufia2MenuRestoreWarpScroll(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu,
                                                  Lufia2PushedChildCall child,
                                                  void *context) {
    (void)context;
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82b03fu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, WARP_SAVED_SCROLL);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_SCROLL_ROW));
    OpLda(memory, cpu, WARP_SAVED_TOP_ROW);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW));
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82b051u);
}

static Lufia2ExecutionResult ConfirmWarpDestination(MenuActionCall *call) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_INDEX));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW));
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, WARP_RECORD_WORDS));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DESTINATION_WORD));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, WARP_MAP_IDS));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_RETURN_MAP));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09C9));
    OpLoadA(cpu, 5u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_CURSOR_SLOT)));
    if (!MenuActionChild(call, 0x82aff3u, 0x82895bu, 2u, 1u) ||
        !MenuActionChild(call, 0x82aff6u, 0x82b03fu, 2u, 1u))
        return call->result;
    cpu->carry = 0u;
    return ExecutionReturned(0x82affau);
}

static Lufia2ExecutionResult CancelWarpSelection(MenuActionCall *call) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    if (!MenuActionChild(call, 0x82affbu, 0x82b03fu, 2u, 1u))
        return call->result;
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WARP_SECONDARY_LIMIT));
    if (!MenuActionChild(call, 0x82b003u, 0x829c10u, 2u, 1u))
        return call->result;
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E11DF));
    OpLoadA(cpu, 1u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_CURSOR_SLOT)));
    OpLdy(cpu, 9u);
    if (!MenuActionChild(call, 0x82b013u, 0x8289bcu, 2u, 1u) ||
        !MenuActionChild(call, 0x82b016u, 0x82836au, 2u, 1u))
        return call->result;
    OpLoadA(cpu, 0x88u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_MENU_FRAME_FLAGS), 1u);
    cpu->carry = 1u;
    return ExecutionReturned(0x82b01eu);
}

static bool MoveWarpSelection(MenuActionCall *call, uint8_t action) {
    Lufia2CpuState *cpu = call->cpu;
    if (action == 6u || action == 7u) {
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 1u));
        OpLsrA(cpu);
        if (!MenuActionChild(call, 0x82b022u, 0x828ce9u, 2u, 1u))
            return false;
        if (!cpu->carry && !MenuActionChild(call, 0x82b027u, 0x82b052u, 2u, 1u))
            return false;
    } else if (!MenuActionChild(call, action == 10u ? 0x82b02cu : 0x82b032u,
                                action == 10u ? 0x82b080u : 0x82b0beu, 2u, 1u)) {
        return false;
    }
    if (!MenuActionChild(call, 0x82b035u, 0x828c43u, 2u, 1u))
        return false;
    OpLoadA(cpu, 8u);
    OpTestBits(call->memory, cpu, OpDp(cpu, DP_MENU_FRAME_FLAGS), 1u);
    return true;
}

Lufia2ExecutionResult Lufia2MenuSelectWarpDestination(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu,
                                                      Lufia2PushedChildCall child,
                                                      void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82af8eu);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    if (!MenuActionChild(&call, 0x82af8eu, 0x829449u, 2u, 1u))
        return call.result;
    OpCmpValue(cpu, 0u);
    if (cpu->zero) {
        cpu->carry = 1u;
        return ExecutionReturned(0x82af96u);
    }
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_COUNT));
    if (!MenuActionChild(&call, 0x82af9au, 0x828345u, 2u, 1u) ||
        !MenuActionChild(&call, 0x82af9du, 0x829cb2u, 2u, 1u))
        return call.result;
    bool moved = false;
    for (;;) {
        OpLoadA(cpu, moved ? 1u : 0u);
        if (!MenuActionChild(&call, 0x82afa2u, 0x828b08u, 2u, 1u))
            return call.result;
        OpCmpValue(cpu, 2u);
        if (cpu->zero)
            return ConfirmWarpDestination(&call);
        OpCmpValue(cpu, 3u);
        if (cpu->zero)
            return CancelWarpSelection(&call);
        OpCmpValue(cpu, 6u);
        if (cpu->zero) {
            moved = true;
        } else {
            OpCmpValue(cpu, 7u);
            if (cpu->zero) {
                moved = true;
            } else {
                OpCmpValue(cpu, 10u);
                if (cpu->zero) {
                    moved = true;
                } else {
                    OpCmpValue(cpu, 11u);
                    moved = cpu->zero;
                }
            }
        }
        if (moved && !MoveWarpSelection(&call, A8(cpu)))
            return call.result;
    }
}

Lufia2ExecutionResult Lufia2MenuDrawWarpScrollRow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82b072u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpLdx(cpu, WARP_INCOMING_ROW_TILEMAP);
    OpWriteX(memory, cpu, OpDp(cpu, DP_WARP_DRAW_TILEMAP), cpu->x);
    OpLdx(cpu, 1u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_WARP_DRAW_ROWS), cpu->x);
    if (!MenuActionChild(&call, 0x82b07cu, 0x828680u, 2u, 1u))
        return call.result;
    return ExecutionReturned(0x82b07fu);
}

static bool AnimateWarpScroll(MenuActionCall *call, bool down) {
    static const uint32_t up_sites[] = {0x82b098u, 0x82b0a1u, 0x82b0aau};
    static const uint32_t down_sites[] = {0x82b0deu, 0x82b0e7u, 0x82b0f0u};
    for (unsigned frame = 0u; frame < 3u; ++frame) {
        const uint16_t request = (uint16_t)((down ? 8u : 2u) << (frame != 0u));
        OpLoadA(call->cpu, request);
        OpSta(call->memory, call->cpu, OpAbs(call->cpu, WARP_SCROLL_REQUEST));
        if (!MenuActionChild(call, down ? down_sites[frame] : up_sites[frame],
                             0x868b55u, 3u, 1u))
            return false;
    }
    return true;
}

Lufia2ExecutionResult Lufia2MenuScrollWarpDestinationsUp(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82b080u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW));
    if (!cpu->zero) {
        OpStepMem(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW), -1);
        OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW)));
        OpWriteX(memory, cpu, OpDp(cpu, DP_WARP_DRAW_INDEX), cpu->x);
        if (!MenuActionChild(&call, 0x82b090u, 0x82b072u, 2u, 1u) ||
            !AnimateWarpScroll(&call, false) ||
            !MenuActionChild(&call, 0x82b0aeu, 0x828f5eu, 2u, 1u))
            return call.result;
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, OpAbs(cpu, WARP_SCROLL_REQUEST));
        OpRepWidths(cpu, 0x20u);
        OpStepMem(memory, cpu, OpAbs(cpu, WRAM_MENU_SCROLL_ROW), -1);
    }
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82b0bdu);
}

Lufia2ExecutionResult Lufia2MenuScrollWarpDestinationsDown(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82b0beu);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW));
    OpCmp(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_OVERFLOW));
    if (!cpu->carry) {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW));
        cpu->carry = 0u;
        OpAdcValue(cpu, WARP_VISIBLE_ROWS);
        OpSta(memory, cpu, OpDp(cpu, DP_WARP_DRAW_INDEX));
        OpStz(memory, cpu, OpDp(cpu, DP_WARP_DRAW_INDEX + 1u));
        if (!MenuActionChild(&call, 0x82b0d3u, 0x82b072u, 2u, 1u))
            return call.result;
        OpStepMem(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW), 1);
        if (!AnimateWarpScroll(&call, true) ||
            !MenuActionChild(&call, 0x82b0f4u, 0x829009u, 2u, 1u))
            return call.result;
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, OpAbs(cpu, WARP_SCROLL_REQUEST));
        OpRepWidths(cpu, 0x20u);
        OpStepMem(memory, cpu, OpAbs(cpu, WRAM_MENU_SCROLL_ROW), 1);
    }
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82b103u);
}
