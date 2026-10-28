#include "lufia2/menu_capsule_screen.h"
#include "menu/menu_screen_child_internal.h"
#include "menu/menu_sprite_slots.h"
#include "system/wram.h"

enum {
    CAPSULE_TEXT_BANK = 0x5fu,
    CAPSULE_CURSOR_FIRST = 0x54u,
    CAPSULE_CURSOR_COUNT = 0x56u,
    CAPSULE_PALETTE_REQUEST = 0x73u
};

static bool CapsuleChild(MenuActionCall *call, uint32_t site, uint32_t target,
                          uint8_t frame, uint8_t width) {
    return MenuScreenChild(call, site, target, frame, width, 0u);
}

static bool SelectCapsuleSprite(MenuActionCall *call, uint32_t site,
                                 uint16_t slot) {
    OpLdy(call->cpu, 0x0cu);
    OpLdx(call->cpu, slot);
    return MenuScreenChild(call, site, 0x868cdau, 3u, 1u, 1u);
}

static void OffsetCapsuleSelection(const Lufia2Memory *memory,
                                    Lufia2CpuState *cpu) {
    OpLdx(cpu, 1u);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_MENU_ITEM_COLUMN));
    OpCmpValue(cpu, 2u);
    if (cpu->zero) {
        OpLda(memory, cpu, OpAbsX(cpu, MENU_SPRITE_X_LOW + 5u));
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x10u);
        OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_X_LOW + 5u));
    }
}

static bool PrepareCapsuleSprites(MenuActionCall *call) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    OpSepWidths(cpu, 0x10u);
    if (!SelectCapsuleSprite(call, 0x82a115u, 1u) ||
        !SelectCapsuleSprite(call, 0x82a11du, 0u))
        return false;
    OpRepWidths(cpu, 0x10u);
    OpLoadA(cpu, 0x30u);
    OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_X_LOW));
    OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_X_HIGH));
    OpLoadA(cpu, 0x48u);
    OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_Y_LOW));
    OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_Y_HIGH));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ACTIVE));
    OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_FRAME_INDEX));
    OpLoadA(cpu, 0u);
    return CapsuleChild(call, 0x82a13du, 0x868cf5u, 3u, 1u);
}

static bool DrawCapsuleDetails(MenuActionCall *call) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x0602u);
    OpLdx(cpu, 0x1e03u);
    if (!CapsuleChild(call, 0x82a149u, 0x82810eu, 2u, 0u))
        return false;
    OpLoadA(cpu, 0x03c4u);
    OpLdx(cpu, 0x0808u);
    OpLdy(cpu, 0x10b0u);
    if (!CapsuleChild(call, 0x82a155u, 0x82831au, 2u, 0u))
        return false;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, CAPSULE_TEXT_BANK));
    OpLdy(cpu, 0xd2deu);
    return CapsuleChild(call, 0x82a166u, 0x808878u, 3u, 1u) &&
        CapsuleChild(call, 0x82a16au, 0x82d07bu, 2u, 1u) &&
        CapsuleChild(call, 0x82a16du, 0x82d121u, 2u, 1u);
}

static bool PrepareCapsuleSelection(MenuActionCall *call) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    OpSepWidths(cpu, 0x10u);
    if (!SelectCapsuleSprite(call, 0x82a176u, 1u))
        return false;
    OpRepWidths(cpu, 0x10u);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ACTIVE));
    OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_FRAME_INDEX));
    OpLoadA(cpu, 1u);
    if (!CapsuleChild(call, 0x82a186u, 0x868cf5u, 3u, 1u) ||
        !CapsuleChild(call, 0x82a18au, 0x82d248u, 2u, 1u))
        return false;
    OpLdx(cpu, 0x4du);
    OpWriteX(memory, cpu, OpDp(cpu, CAPSULE_CURSOR_FIRST), cpu->x);
    OpLdx(cpu, 1u);
    OpWriteX(memory, cpu, OpDp(cpu, CAPSULE_CURSOR_COUNT), cpu->x);
    OpLdx(cpu, 5u);
    OpLdy(cpu, 6u);
    if (!CapsuleChild(call, 0x82a19du, 0x8289c4u, 2u, 1u))
        return false;
    OffsetCapsuleSelection(memory, cpu);
    return true;
}

Lufia2ExecutionResult Lufia2MenuBuildCapsuleScreen(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82a0e3u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_WINDOW_REFRESH));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_CURSOR_ENABLED));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09C8));
    OpAndValue(cpu, 0xfdu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09C8));
    OpLdx(cpu, 0xffffu);
    if (!CapsuleChild(&call, 0x82a0f4u, 0x829214u, 3u, 1u) ||
        !CapsuleChild(&call, 0x82a0f8u, 0x82838fu, 2u, 1u))
        return call.result;
    OpLdx(cpu, 0x10u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTION_REFERENCE), cpu->x);
    if (!CapsuleChild(&call, 0x82a101u, 0x828069u, 2u, 1u))
        return call.result;
    OpLoadA(cpu, 4u);
    if (!CapsuleChild(&call, 0x82a106u, 0x82cf9du, 2u, 1u) ||
        !CapsuleChild(&call, 0x82a109u, 0x82cfd4u, 2u, 1u) ||
        !CapsuleChild(&call, 0x82a10cu, 0x82d270u, 2u, 1u) ||
        !PrepareCapsuleSprites(&call) || !DrawCapsuleDetails(&call) ||
        !PrepareCapsuleSelection(&call))
        return call.result;
    OpLoadA(cpu, 0x1fu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_TM));
    OpLoadA(cpu, 0x0du);
    OpSta(memory, cpu, OpAbs(cpu, SNES_TS));
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_CGWSEL));
    OpLoadA(cpu, 0x10u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_CGADSUB));
    if (!CapsuleChild(&call, 0x82a1c7u, 0x86910cu, 3u, 1u))
        return call.result;
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, CAPSULE_PALETTE_REQUEST));
    OpLoadA(cpu, 4u);
    if (!CapsuleChild(&call, 0x82a1d1u, 0x8293f6u, 2u, 1u) ||
        !CapsuleChild(&call, 0x82a1d4u, 0x828704u, 2u, 1u))
        return call.result;
    return ExecutionReturned(0x82a1d7u);
}

Lufia2ExecutionResult Lufia2MenuDrawCapsuleExperience(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82d121u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpLdx(cpu, WRAM_CAPSULE_WORK_STATS);
    OpWriteX(memory, cpu, OpDp(cpu, 0x2au), cpu->x);
    if (!CapsuleChild(&call, 0x82d126u, 0x82942fu, 2u, 1u))
        return call.result;
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, CAPSULE_TEXT_BANK));
    OpLdy(cpu, 0xc709u);
    if (!CapsuleChild(&call, 0x82d135u, 0x808878u, 3u, 1u))
        return call.result;
    return ExecutionReturned(0x82d139u);
}

Lufia2ExecutionResult Lufia2MenuRunCapsuleAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82c044u);
    const uint32_t previous_resume = cpu->resume_pc;
    SimulateJsrFrame(memory, cpu, 0xc046u);
    Lufia2ExecutionResult dispatch = Lufia2MenuDispatchInlineTable(memory, cpu);
    if (dispatch.pc != 0x82c04du && dispatch.pc != 0x82c051u &&
        dispatch.pc != 0x82c055u)
        return dispatch;
    cpu->resume_pc = previous_resume;
    MenuActionCall call = {memory, cpu, child, context, {0}};
    if (dispatch.pc == 0x82c04du) {
        if (!CapsuleChild(&call, 0x82c04du, 0x82c0acu, 2u, 1u))
            return call.result;
        return ExecutionReturned(0x82c050u);
    }
    if (dispatch.pc == 0x82c051u) {
        if (!CapsuleChild(&call, 0x82c051u, 0x82c170u, 2u, 1u))
            return call.result;
        return ExecutionReturned(0x82c054u);
    }
    if (!CapsuleChild(&call, 0x82c055u, 0x868b32u, 3u, 1u) ||
        !CapsuleChild(&call, 0x82c059u, 0x82838fu, 2u, 1u))
        return call.result;
    OpLdx(cpu, WRAM_CAPSULE_WORK_STATS);
    OpWriteX(memory, cpu, OpDp(cpu, 0x2au), cpu->x);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpDp(cpu, 0x30u));
    if (!CapsuleChild(&call, 0x82c065u, 0x82e746u, 3u, 1u) ||
        !CapsuleChild(&call, 0x82c069u, 0x82c443u, 2u, 1u) ||
        !CapsuleChild(&call, 0x82c06cu, 0x82a0e3u, 2u, 1u))
        return call.result;
    return ExecutionReturned(0x82c06fu);
}

static bool ChangeCapsuleLevel(MenuActionCall *call) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E057C));
    if (cpu->zero)
        return true;
    OpLda(memory, cpu, OpDp(cpu, 0u));
    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 1u));
    OpLsrA(cpu);
    if (cpu->carry) {
        OpStepMem(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_WORK_LEVEL), -1);
        if (cpu->zero) {
            OpLoadA(cpu, 1u);
            OpSta(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_WORK_LEVEL));
        }
    } else {
        OpStepMem(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_WORK_LEVEL), 1);
        OpLda(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_WORK_LEVEL));
        OpCmpValue(cpu, 100u);
        if (cpu->zero) {
            OpLoadA(cpu, 99u);
            OpSta(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_WORK_LEVEL));
        }
    }
    if (!CapsuleChild(call, 0x82c099u, 0x82ce23u, 2u, 1u) ||
        !CapsuleChild(call, 0x82c09cu, 0x82d270u, 2u, 1u) ||
        !CapsuleChild(call, 0x82c09fu, 0x82c443u, 2u, 1u) ||
        !CapsuleChild(call, 0x82c0a2u, 0x82d121u, 2u, 1u))
        return false;
    OpLoadA(cpu, 8u);
    TestBitsDirect(memory, cpu, 0x74u, 1u);
    return true;
}

Lufia2ExecutionResult Lufia2MenuRunCapsuleScreen(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82bfc5u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpLoadA(cpu, 5u);
    OpLdx(cpu, 5u);
    if (!CapsuleChild(&call, 0x82bfcau, 0x82895bu, 2u, 1u) ||
        !CapsuleChild(&call, 0x82bfcdu, 0x868b55u, 3u, 1u) ||
        !CapsuleChild(&call, 0x82bfd1u, 0x868b32u, 3u, 1u) ||
        !CapsuleChild(&call, 0x82bfd5u, 0x82a0e3u, 2u, 1u))
        return call.result;
    OpLoadA(cpu, 0u);
    for (;;) {
        if (!CapsuleChild(&call, 0x82bfdau, 0x828b08u, 2u, 1u))
            return call.result;
        OpCmpValue(cpu, 1u);
        if (cpu->zero) {
            OffsetCapsuleSelection(memory, cpu);
        } else {
            OpCmpValue(cpu, 2u);
            if (cpu->zero) {
                OpLoadA(cpu, 2u);
                if (!CapsuleChild(&call, 0x82c02bu, 0x80953bu, 3u, 1u))
                    return call.result;
                OpLoadA(cpu, 5u);
                OpLdx(cpu, 6u);
                if (!CapsuleChild(&call, 0x82c034u, 0x82895bu, 2u, 1u) ||
                    !CapsuleChild(&call, 0x82c037u, 0x868b55u, 3u, 1u))
                    return call.result;
                OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_INDEX));
                if (!CapsuleChild(&call, 0x82c03eu, 0x82c044u, 2u, 1u))
                    return call.result;
            } else {
                OpCmpValue(cpu, 3u);
                if (cpu->zero)
                    break;
                OpCmpValue(cpu, 6u);
                if (!cpu->zero) {
                    OpCmpValue(cpu, 7u);
                    if (!cpu->zero) {
                        OpLoadA(cpu, 0u);
                        continue;
                    }
                }
                if (!ChangeCapsuleLevel(&call))
                    return call.result;
            }
        }
        OpLoadA(cpu, 1u);
    }
    if (!CapsuleChild(&call, 0x82c011u, 0x868b32u, 3u, 1u) ||
        !CapsuleChild(&call, 0x82c015u, 0x82c261u, 3u, 1u) ||
        !CapsuleChild(&call, 0x82c019u, 0x82838fu, 2u, 1u))
        return call.result;
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTION_REFERENCE), cpu->x);
    if (!CapsuleChild(&call, 0x82c022u, 0x828069u, 2u, 1u) ||
        !CapsuleChild(&call, 0x82c025u, 0x829ae0u, 2u, 1u))
        return call.result;
    return ExecutionReturned(0x82c028u);
}
