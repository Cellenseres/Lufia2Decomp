#include "lufia2/menu.h"
#include "core/snes_registers.h"
#include "menu/menu_action_internal.h"
#include "system/wram.h"

enum {
    DP_DESCRIPTION_SOURCE = 0x0bu,
    DP_DESCRIPTION_SOURCE_BANK = 0x0du,
    DP_DESCRIPTION_TILE = 0u,
    DP_DESCRIPTION_TEXT_BANK = 0x5fu,
    DP_DESCRIPTION_MASK_FLAGS = 0xf2u,
    DP_DESCRIPTION_MASK_MODE = 0xf3u,
    DP_DESCRIPTION_MASK_TABLE = 0xf4u,
    DP_DESCRIPTION_MASK_BANK = 0xf6u,
    DP_DESCRIPTION_MASK_BUFFER = 0xf7u,
    DP_DESCRIPTION_MASK_BUFFER_BANK = 0xf9u,
    DP_DESCRIPTION_MASK_REGISTER = 0xfau,
    DP_DESCRIPTION_MASK_CHANNEL = 0xfbu,
    DESCRIPTION_SECONDARY_RECORD = WRAM_RECORD_BUFFER + 0x34u,
    ROM_SECONDARY_DESCRIPTION_RECORDS = 0x848f10u
};

Lufia2ExecutionResult Lufia2MenuLoadDescriptionRecord(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu,
                                                      Lufia2PushedChildCall child,
                                                      void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82fec8u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpStz(memory, cpu, OpDp(cpu, DP_DESCRIPTION_SOURCE_BANK));
    OpLdx(cpu, WRAM_RECORD_BUFFER);
    OpWriteX(memory, cpu, OpDp(cpu, DP_DESCRIPTION_SOURCE), cpu->x);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_DESCRIPTION_ID + 1u));
    OpAndValue(cpu, 2u);
    if (cpu->zero) {
        OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_DESCRIPTION_ID)));
        OpWriteX(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID), cpu->x);
        if (!MenuActionChild(&call, 0x82fedcu, 0x81f1c5u, 3u, 1u))
            return call.result;
        OpLdx(cpu, 0u);
        OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_DESCRIPTION_RECORD_KIND), cpu->x);
        OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, DESCRIPTION_SECONDARY_RECORD)));
        OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_DESCRIPTION_SECONDARY_ID), cpu->x);
        return ExecutionReturned(0x82feecu);
    }
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_DESCRIPTION_ID));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_RECORD_ID));
    if (!MenuActionChild(&call, 0x82fef3u, 0x81f414u, 3u, 1u))
        return call.result;
    OpLdx(cpu, 1u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_DESCRIPTION_RECORD_KIND), cpu->x);
    return ExecutionReturned(0x82fefdu);
}

Lufia2ExecutionResult Lufia2MenuLoadSecondaryDescription(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu,
                                                         Lufia2PushedChildCall child,
                                                         void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82fefeu);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpLoadA(cpu, 0x84u);
    OpSta(memory, cpu, OpDp(cpu, DP_DESCRIPTION_SOURCE_BANK));
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_DESCRIPTION_RECORD_KIND)));
    OpCpx(cpu, 0u);
    if (cpu->zero) {
        OpLdx(cpu,
              OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_DESCRIPTION_SECONDARY_ID)));
        if (!cpu->zero) {
            OpRepWidths(cpu, 0x20u);
            OpTxa(cpu);
            OpDecA(cpu);
            OpAslA(cpu);
            OpTax(cpu);
            OpLda(memory, cpu, OpLongX(cpu, ROM_SECONDARY_DESCRIPTION_RECORDS));
            OpTax(cpu);
            cpu->carry = 0u;
            OpAdcValue(cpu, 6u);
            OpSta(memory, cpu, OpDp(cpu, DP_DESCRIPTION_SOURCE));
            OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_DESCRIPTION_SECONDARY_ID));
            cpu->carry = 0u;
            OpAdcValue(cpu, 0x0300u);
            OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ACTION_MESSAGE_ID));
            if (!MenuActionChild(&call, 0x82ff2au, 0x85c4f1u, 3u, 0u))
                return call.result;
            OpLdx(cpu, 2u);
            OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_DESCRIPTION_RECORD_KIND),
                     cpu->x);
            OpSepWidths(cpu, 0x20u);
            cpu->carry = 0u;
            return ExecutionReturned(0x82ff37u);
        }
    }
    cpu->carry = 1u;
    return ExecutionReturned(0x82ff39u);
}

Lufia2ExecutionResult Lufia2MenuDispatchDescriptionLayout(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu,
                                                          Lufia2PushedChildCall child,
                                                          void *context) {
    (void)context;
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82fdecu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_DESCRIPTION_LAYOUT));
    SimulateJsrFrame(memory, cpu, 0xfdf1u);
    return Lufia2MenuDispatchInlineTable(memory, cpu);
}

Lufia2ExecutionResult Lufia2MenuConfigureDescriptionMask(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu,
                                                         Lufia2PushedChildCall child,
                                                         void *context) {
    (void)context;
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82fe98u);
    OpLdx(cpu, 0xe447u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_DESCRIPTION_MASK_TABLE), cpu->x);
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, DP_DESCRIPTION_MASK_BANK));
    OpLoadA(cpu, 0x20u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_DESCRIPTION_MASK_FLAGS), 1u);
    OpLoadA(cpu, 0x50u);
    OpSta(memory, cpu, OpDp(cpu, DP_DESCRIPTION_MASK_MODE));
    OpLoadA(cpu, 0x25u);
    OpSta(memory, cpu, OpDp(cpu, DP_DESCRIPTION_MASK_REGISTER));
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpDp(cpu, DP_DESCRIPTION_MASK_CHANNEL));
    OpLdx(cpu, 0x81c0u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_DESCRIPTION_MASK_BUFFER), cpu->x);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, DP_DESCRIPTION_MASK_BUFFER_BANK));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DISPLAY_REQUESTS));
    return ExecutionReturned(0x82febfu);
}

Lufia2ExecutionResult Lufia2MenuClearDescriptionMask(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu,
                                                     Lufia2PushedChildCall child,
                                                     void *context) {
    (void)context;
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82fec0u);
    OpStz(memory, cpu, OpAbs(cpu, SNES_WOBJSEL));
    OpLoadA(cpu, 0x20u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_DESCRIPTION_MASK_FLAGS), 0u);
    return ExecutionReturned(0x82fec7u);
}

Lufia2ExecutionResult
Lufia2MenuDrawSecondaryDescriptionPrompt(const Lufia2Memory *memory,
                                         Lufia2CpuState *cpu,
                                         Lufia2PushedChildCall child, void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82ff99u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_DESCRIPTION_RECORD_KIND)));
    OpCpy(cpu, 0u);
    if (cpu->zero) {
        OpLdy(cpu,
              OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_DESCRIPTION_SECONDARY_ID)));
        if (!cpu->zero) {
            OpLoadA(cpu, 0x21u);
            OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
            OpLoadA(cpu, 0x8eu);
            OpSta(memory, cpu, OpDp(cpu, DP_DESCRIPTION_TEXT_BANK));
            OpLdy(cpu, 0xd614u);
            if (!MenuActionChild(&call, 0x82ffb2u, 0x808878u, 3u, 1u))
                return call.result;
        }
    }
    return ExecutionReturned(0x82ffb6u);
}

static Lufia2ExecutionResult DrawDescriptionLayout(MenuActionCall *call, bool framed) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x0442u);
    OpLdx(cpu, framed ? 0x1e0fu : 0x1e0eu);
    if (!MenuActionChild(call, framed ? 0x82fdfeu : 0x82fe4cu,
                         framed ? 0x8283b5u : 0x8283ebu, 2u, 0u))
        return call->result;
    if (framed) {
        OpLoadA(cpu, 0x0442u);
        OpLdx(cpu, 0x1e0au);
        if (!MenuActionChild(call, 0x82fe07u, 0x82810eu, 2u, 0u))
            return call->result;
    }
    OpLoadA(cpu, framed ? 0x0502u : 0x04c2u);
    OpLdx(cpu, 0x1eu);
    if (!MenuActionChild(call, framed ? 0x82fe10u : 0x82fe55u, 0x8282e2u, 2u, 0u))
        return call->result;
    OpSepWidths(cpu, 0x20u);
    if (!MenuActionChild(call, framed ? 0x82fe15u : 0x82fe5au, 0x82fe98u, 2u, 1u) ||
        !MenuActionChild(call, framed ? 0x82fe18u : 0x82fe5du, 0x82ff3au, 2u, 1u))
        return call->result;
    OpLdx(cpu, 0x2200u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_DESCRIPTION_TILE), cpu->x);
    OpLdx(cpu, 0x1b01u);
    OpLdy(cpu, framed ? 0x0484u : 0x0444u);
    if (!MenuActionChild(call, framed ? 0x82fe26u : 0x82fe6bu, 0x829133u, 2u, 1u))
        return call->result;
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    OpLoadA(cpu, 0x7fu);
    OpSta(memory, cpu, OpDp(cpu, DP_DESCRIPTION_TEXT_BANK));
    OpLdy(cpu, 0xf000u);
    OpLdx(cpu, framed ? 0x3584u : 0x3544u);
    if (!MenuActionChild(call, framed ? 0x82fe38u : 0x82fe7du, 0x808878u, 3u, 1u))
        return call->result;
    OpLdx(cpu, framed ? 0x34b4u : 0x3474u);
    if (!MenuActionChild(call, framed ? 0x82fe3fu : 0x82fe84u, 0x82ff99u, 2u, 1u))
        return call->result;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_DESCRIPTION_TEXT_MODES));
    if (!MenuActionChild(call, 0x82fe8cu, 0x8293f6u, 2u, 1u))
        return call->result;
    OpLoadA(cpu, 0x88u);
    OpSta(memory, cpu, OpDp(cpu, DP_MENU_FRAME_FLAGS));
    if (!MenuActionChild(call, 0x82fe93u, 0x868b55u, 3u, 1u))
        return call->result;
    return ExecutionReturned(0x82fe97u);
}

Lufia2ExecutionResult Lufia2MenuDrawFramedDescription(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu,
                                                      Lufia2PushedChildCall child,
                                                      void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82fdf6u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    return DrawDescriptionLayout(&call, true);
}

Lufia2ExecutionResult Lufia2MenuDrawInlineDescription(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu,
                                                      Lufia2PushedChildCall child,
                                                      void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82fe44u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    return DrawDescriptionLayout(&call, false);
}

static bool LoadAndDrawDescription(MenuActionCall *call) {
    OpLdx(call->cpu,
          OpReadX(call->memory, call->cpu, OpAbs(call->cpu, WRAM_MENU_DESCRIPTION_ID)));
    OpWriteX(call->memory, call->cpu, OpAbs(call->cpu, WRAM_BATTLE_ACTION_MESSAGE_ID),
             call->cpu->x);
    return MenuActionChild(call, 0x82fda2u, 0x85c4f1u, 3u, 1u) &&
           MenuActionChild(call, 0x82fda6u, 0x82fec8u, 2u, 1u) &&
           MenuActionChild(call, 0x82fda9u, 0x82fdecu, 2u, 1u);
}

Lufia2ExecutionResult Lufia2MenuShowRecordDescription(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu,
                                                      Lufia2PushedChildCall child,
                                                      void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82fd81u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_DESCRIPTION_LAYOUT), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_DESCRIPTION_TEXT_MODES), cpu->y);
    OpLdx(cpu, 0x0f80u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_UNK_7E156C), cpu->x);
    OpLdx(cpu, 0x8040u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_UNK_7E156E), cpu->x);
    OpLoadA(cpu, 2u);
    if (!MenuActionChild(&call, 0x82fd95u, 0x80953bu, 3u, 1u) ||
        !MenuActionChild(&call, 0x82fd99u, 0x828345u, 2u, 1u))
        return call.result;
    bool closed = false;
    do {
        if (!LoadAndDrawDescription(&call))
            return call.result;
        for (;;) {
            if (!MenuActionChild(&call, 0x82fdacu, 0x82935du, 3u, 1u))
                return call.result;
            if (cpu->carry) {
                closed = true;
                break;
            }
            if (!MenuActionChild(&call, 0x82fdb2u, 0x82fefeu, 2u, 1u))
                return call.result;
            if (cpu->carry)
                continue;
            OpLoadA(cpu, 2u);
            if (!MenuActionChild(&call, 0x82fdb9u, 0x80953bu, 3u, 1u) ||
                !MenuActionChild(&call, 0x82fdbdu, 0x82fdecu, 2u, 1u) ||
                !MenuActionChild(&call, 0x82fdc0u, 0x82935du, 3u, 1u))
                return call.result;
            if (cpu->carry) {
                closed = true;
                break;
            }
            OpLoadA(cpu, 2u);
            if (!MenuActionChild(&call, 0x82fdc8u, 0x80953bu, 3u, 1u))
                return call.result;
            break;
        }
    } while (!closed);
    OpLoadA(cpu, 1u);
    if (!MenuActionChild(&call, 0x82fdd0u, 0x80953bu, 3u, 1u) ||
        !MenuActionChild(&call, 0x82fdd4u, 0x82836au, 2u, 1u) ||
        !MenuActionChild(&call, 0x82fdd7u, 0x82fec0u, 2u, 1u))
        return call.result;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_DESCRIPTION_TEXT_MODES + 1u));
    if (!MenuActionChild(&call, 0x82fdddu, 0x8293f6u, 2u, 1u))
        return call.result;
    OpLoadA(cpu, 0x88u);
    OpSta(memory, cpu, OpDp(cpu, DP_MENU_FRAME_FLAGS));
    if (!MenuActionChild(&call, 0x82fde4u, 0x868b55u, 3u, 1u))
        return call.result;
    OpStz(memory, cpu, OpAbs(cpu, SNES_WOBJSEL));
    return ExecutionReturned(0x82fdebu);
}
