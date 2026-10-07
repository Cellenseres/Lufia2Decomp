#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "lufia2/menu.h"
#include "system/wram.h"

enum {
    MAIN_MENU_HELD_FIRST = 0x46u,
    MAIN_MENU_HELD_SECOND = 0x47u,
    MAIN_MENU_REDRAW = 0x74u,
    MAIN_MENU_LIST_POINTER = 0x08u,
    MAIN_MENU_LIST_BANK = 0x0au,
    MAIN_MENU_LIST_MEMBER = 0x0bu,
    MAIN_MENU_DISPLAY_POINTER = 0x5du,
    MAIN_MENU_DISPLAY_BANK = 0x5fu,
    MAIN_MENU_PALETTE_REQUEST = 0x73u,
    MAIN_MENU_SELECTED_ALTERNATE = 0x22u,
    MAIN_MENU_ALTERNATE_REQUEST = 0x30u,
    MAIN_MENU_INPUT_BUTTON = 0x20u,
    MAIN_MENU_SHORTCUTS = WRAM_UNK_7E057C,
    MAIN_MENU_LIST_INDEX = WRAM_MENU_LIST_INDEX,
    MAIN_MENU_SELECTION_MODE = WRAM_UNK_7E14BD,
    MAIN_MENU_ALTERNATE_MODE = WRAM_UNK_7E14BF,
    MAIN_MENU_LIST_FIRST = WRAM_MENU_LIST_MODE,
    MAIN_MENU_LIST_SECOND = WRAM_UNK_7E09D3,
    MAIN_MENU_AUXILIARY = WRAM_UNK_7E09C8,
    MAIN_MENU_SCENE_MODE = WRAM_UNK_7E0A7F,
    MAIN_MENU_ITEM_COUNT = WRAM_MENU_ITEM_QUANTITY,
    MAIN_MENU_ITEM_ID = WRAM_MENU_SELECTED_SPELL,
    MAIN_MENU_SPELL_ID = WRAM_MENU_SPELL_RECORD_ID,
    MAIN_MENU_WINDOW_REFRESH = WRAM_MENU_WINDOW_REFRESH,
    MAIN_MENU_PRICE = WRAM_MENU_PURCHASE_PRICE,
    ROM_MAIN_MENU_ITEMS = 0x82a54du,
    ROM_MAIN_MENU_SPELLS = 0xa604u,
    MAIN_MENU_CLEAR_BUFFER = WRAM_UNK_7FF080
};

static uint8_t MenuMainReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x82u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit;
}

static uint8_t MenuMainCall(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target, uint8_t frame) {
    return CallChildWithFrame(memory, cpu, child, context,
        site, target, frame, 0x82u);
}

static Lufia2ExecutionResult MenuMainUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static uint32_t MenuMainSelect(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    OpLda(memory, cpu, OpAbs(cpu, MAIN_MENU_LIST_INDEX));
    OpCmpValue(cpu, 4u);
    if (cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, MAIN_MENU_SCENE_MODE));
        OpCmpValue(cpu, 7u);
        if (!cpu->zero) {
            if (!MenuMainCall(memory, cpu, child, context,
                    0x82a47bu, 0x8297e3u, 2u))
                return 0x82a47bu;
            return 0u;
        }
    }
    OpLoadA(cpu, 2u);
    if (!MenuMainCall(memory, cpu, child, context, 0x82a482u, 0x80953bu, 3u))
        return 0x82a482u;
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, MAIN_MENU_LIST_INDEX)));
    OpWriteX(memory, cpu, OpAbs(cpu, MAIN_MENU_LIST_FIRST), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, MAIN_MENU_LIST_SECOND), cpu->x);
    OpLdy(cpu, 1u);
    if (!MenuMainCall(memory, cpu, child, context, 0x82a492u, 0x8ee751u, 3u))
        return 0x82a492u;
    OpLda(memory, cpu, OpAbs(cpu, MAIN_MENU_LIST_INDEX));
    OpCmpValue(cpu, 7u);
    uint8_t alternate = 0u;
    if (cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, MAIN_MENU_HELD_FIRST));
        OpBitValue(cpu, MAIN_MENU_INPUT_BUTTON);
        if (!cpu->zero) {
            OpLda(memory, cpu, OpDp(cpu, MAIN_MENU_HELD_SECOND));
            OpBitValue(cpu, MAIN_MENU_INPUT_BUTTON);
            alternate = !cpu->zero;
        }
    }
    if (alternate) {
        if (!MenuMainCall(memory, cpu, child, context, 0x82a4a9u, 0x82a62du, 2u))
            return 0x82a4a9u;
    } else {
        OpLda(memory, cpu, OpAbs(cpu, MAIN_MENU_LIST_INDEX));
        if (!MenuMainCall(memory, cpu, child, context, 0x82a4b1u, 0x82a4beu, 2u))
            return 0x82a4b1u;
    }
    OpLdy(cpu, 0u);
    if (!MenuMainCall(memory, cpu, child, context, 0x82a4b7u, 0x8ee710u, 3u))
        return 0x82a4b7u;
    return 0u;
}

static uint32_t MenuMainToggleAuxiliary(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    OpLda(memory, cpu, OpAbs(cpu, MAIN_MENU_SCENE_MODE));
    OpCmpValue(cpu, 7u);
    if (!cpu->zero)
        return 0u;
    OpLoadA(cpu, 2u);
    if (!MenuMainCall(memory, cpu, child, context, 0x82a4dau, 0x80953bu, 3u))
        return 0x82a4dau;
    OpLda(memory, cpu, OpAbs(cpu, MAIN_MENU_AUXILIARY));
    OpLoadA(cpu, OpA(cpu) ^ 1u);
    OpSta(memory, cpu, OpAbs(cpu, MAIN_MENU_AUXILIARY));
    if (!MenuMainCall(memory, cpu, child, context, 0x82a4e6u, 0x82c5afu, 2u))
        return 0x82a4e6u;
    OpLoadA(cpu, 0x88u);
    OpTestBits(memory, cpu, OpDp(cpu, MAIN_MENU_REDRAW), 1u);
    return 0u;
}

static uint32_t MenuMainAddItems(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    OpLoadA(cpu, 2u);
    if (!MenuMainCall(memory, cpu, child, context, 0x82a526u, 0x80953bu, 3u))
        return 0x82a526u;
    OpLdx(cpu, 1u);
    OpWriteX(memory, cpu, OpAbs(cpu, MAIN_MENU_ITEM_COUNT), cpu->x);
    OpLdx(cpu, 0u);
    do {
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpLongX(cpu, ROM_MAIN_MENU_ITEMS));
        OpSta(memory, cpu, OpAbs(cpu, MAIN_MENU_ITEM_ID));
        OpPushX(memory, cpu);
        if (!MenuMainCall(memory, cpu, child, context, 0x82a53du, 0x82f9f2u, 2u))
            return 0x82a53du;
        OpPullX(memory, cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, 10u);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    return 0u;
}

static uint32_t MenuMainAddSpells(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    static const uint8_t members[] = {0u, 4u, 1u, 3u, 6u};
    static const uint32_t sites[] = {
        0x82a568u, 0x82a56du, 0x82a572u, 0x82a577u, 0x82a57cu
    };
    OpLoadA(cpu, 2u);
    if (!MenuMainCall(memory, cpu, child, context, 0x82a559u, 0x80953bu, 3u))
        return 0x82a559u;
    OpLoadA(cpu, 0x82u);
    OpSta(memory, cpu, OpDp(cpu, MAIN_MENU_LIST_BANK));
    OpLdx(cpu, ROM_MAIN_MENU_SPELLS);
    OpWriteX(memory, cpu, OpDp(cpu, MAIN_MENU_LIST_POINTER), cpu->x);
    for (unsigned member = 0u; member < sizeof(members); ++member) {
        OpLoadA(cpu, members[member]);
        if (!MenuMainCall(memory, cpu, child, context, sites[member], 0x82a582u, 2u))
            return sites[member];
    }
    return 0u;
}

static uint32_t MenuMainChangeScene(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    OpLoadA(cpu, 2u);
    if (!MenuMainCall(memory, cpu, child, context, 0x82a59eu, 0x80953bu, 3u))
        return 0x82a59eu;
    if (!MenuMainCall(memory, cpu, child, context, 0x82a5a2u, 0x868b32u, 3u))
        return 0x82a5a2u;
    OpLda(memory, cpu, OpDp(cpu, MAIN_MENU_HELD_SECOND));
    OpBitValue(cpu, 0x20u);
    if (!cpu->zero) {
        OpLoadA(cpu, 4u);
    } else {
        OpBitValue(cpu, 0x10u);
        if (!cpu->zero) {
            OpLoadA(cpu, 12u);
        } else {
            OpLda(memory, cpu, OpAbs(cpu, MAIN_MENU_SCENE_MODE));
            OpCmpValue(cpu, 7u);
            OpLoadA(cpu, cpu->zero ? 0xffu : 7u);
            OpSta(memory, cpu, OpAbs(cpu, MAIN_MENU_SCENE_MODE));
            OpLoadA(cpu, 1u);
        }
    }
    if (!MenuMainCall(memory, cpu, child, context, 0x82a5cau, 0x82c352u, 3u))
        return 0x82a5cau;
    if (!MenuMainCall(memory, cpu, child, context, 0x82a5ceu, 0x82838fu, 2u))
        return 0x82a5ceu;
    if (!MenuMainCall(memory, cpu, child, context, 0x82a5d1u, 0x829ae0u, 2u))
        return 0x82a5d1u;
    return 0u;
}

static uint32_t MenuMainShortcut(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    OpLda(memory, cpu, OpAbs(cpu, MAIN_MENU_SHORTCUTS));
    if (cpu->zero)
        return 0u;
    OpLda(memory, cpu, OpDp(cpu, MAIN_MENU_HELD_FIRST));
    OpBitValue(cpu, MAIN_MENU_INPUT_BUTTON);
    if (cpu->zero)
        return 0u;
    OpLda(memory, cpu, OpAbs(cpu, MAIN_MENU_LIST_INDEX));
    OpCmpValue(cpu, 0u);
    if (cpu->zero)
        return MenuMainAddItems(memory, cpu, child, context);
    OpCmpValue(cpu, 2u);
    if (cpu->zero)
        return MenuMainAddSpells(memory, cpu, child, context);
    OpCmpValue(cpu, 6u);
    if (cpu->zero) {
        OpLoadA(cpu, 2u);
        if (!MenuMainCall(memory, cpu, child, context, 0x82a5d9u, 0x80953bu, 3u))
            return 0x82a5d9u;
        OpLdx(cpu, 0u);
        do {
            OpLoadA(cpu, 0xffu);
            OpSta(memory, cpu, OpLongX(cpu, MAIN_MENU_CLEAR_BUFFER));
            OpInx(cpu);
            OpCpx(cpu, 0xe9u);
        } while (!cpu->zero);
        return 0u;
    }
    OpCmpValue(cpu, 4u);
    if (cpu->zero)
        return MenuMainChangeScene(memory, cpu, child, context);
    OpCmpValue(cpu, 1u);
    if (cpu->zero) {
        OpLoadA(cpu, 2u);
        if (!MenuMainCall(memory, cpu, child, context, 0x82a5f1u, 0x80953bu, 3u))
            return 0x82a5f1u;
        OpLdx(cpu, 10000u);
        OpWriteX(memory, cpu, OpAbs(cpu, MAIN_MENU_PRICE), cpu->x);
        OpStz(memory, cpu, OpAbs(cpu, MAIN_MENU_PRICE + 2u));
        if (!MenuMainCall(memory, cpu, child, context, 0x82a5feu, 0x829808u, 2u))
            return 0x82a5feu;
    }
    return 0u;
}

Lufia2ExecutionResult Lufia2MenuRunMainInput(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!MenuMainReady(cpu))
        return ExecutionHandoff(cpu, 0x82a432u);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, MAIN_MENU_SELECTION_MODE));
    OpLdy(cpu, 0u);
    if (!MenuMainCall(memory, cpu, child, context, 0x82a43au, 0x8ee751u, 3u))
        return MenuMainUnwound(0x82a43au);
    if (!MenuMainCall(memory, cpu, child, context, 0x82a43eu, 0x829ae0u, 2u))
        return MenuMainUnwound(0x82a43eu);
    for (;;) {
        OpLoadA(cpu, 0u);
        if (!MenuMainCall(memory, cpu, child, context, 0x82a443u, 0x828b08u, 2u))
            return MenuMainUnwound(0x82a443u);
        OpCmpValue(cpu, 2u);
        uint32_t site = 0u;
        if (cpu->zero) {
            site = MenuMainSelect(memory, cpu, child, context);
        } else {
            OpCmpValue(cpu, 3u);
            if (cpu->zero) {
                OpLdy(cpu, 0u);
                if (!MenuMainCall(memory, cpu, child, context, 0x82a461u, 0x8ee710u, 3u))
                    return MenuMainUnwound(0x82a461u);
                if (!MenuMainCall(memory, cpu, child, context, 0x82a465u, 0x868b32u, 3u))
                    return MenuMainUnwound(0x82a465u);
                if (!MenuMainCall(memory, cpu, child, context, 0x82a469u, 0x82838fu, 2u))
                    return MenuMainUnwound(0x82a469u);
                return ExecutionReturned(0x82a46cu);
            }
            OpCmpValue(cpu, 9u);
            if (cpu->zero) {
                site = MenuMainToggleAuxiliary(memory, cpu, child, context);
            } else {
                OpCmpValue(cpu, 4u);
                if (cpu->zero)
                    site = MenuMainShortcut(memory, cpu, child, context);
            }
        }
        if (site)
            return MenuMainUnwound(site);
    }
}

Lufia2ExecutionResult Lufia2MenuApplySpellList(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!MenuMainReady(cpu))
        return ExecutionHandoff(cpu, 0x82a582u);
    OpSta(memory, cpu, OpDp(cpu, MAIN_MENU_LIST_MEMBER));
    OpLdy(cpu, 0u);
    for (;;) {
        OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, MAIN_MENU_LIST_POINTER));
        OpCmpValue(cpu, 0xffu);
        if (cpu->zero)
            return ExecutionReturned(0x82a59bu);
        OpSta(memory, cpu, OpAbs(cpu, MAIN_MENU_SPELL_ID));
        OpLda(memory, cpu, OpDp(cpu, MAIN_MENU_LIST_MEMBER));
        PushY(memory, cpu);
        if (!MenuMainCall(memory, cpu, child, context, 0x82a593u, 0x82fd3du, 3u))
            return MenuMainUnwound(0x82a593u);
        OpPullY(memory, cpu);
        OpIny(cpu);
    }
}

Lufia2ExecutionResult Lufia2MenuRunAlternateSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!MenuMainReady(cpu))
        return ExecutionHandoff(cpu, 0x82a62du);
    if (!MenuMainCall(memory, cpu, child, context, 0x82a62du, 0x829ef2u, 2u))
        return MenuMainUnwound(0x82a62du);
    OpLoadA(cpu, 3u);
    OpSta(memory, cpu, OpAbs(cpu, MAIN_MENU_ALTERNATE_MODE));
    if (!MenuMainCall(memory, cpu, child, context, 0x82a635u, 0x829246u, 2u))
        return MenuMainUnwound(0x82a635u);
    if (!cpu->carry) {
        OpStz(memory, cpu, OpAbs(cpu, MAIN_MENU_WINDOW_REFRESH));
        if (!MenuMainCall(memory, cpu, child, context, 0x82a63du, 0x868b32u, 3u))
            return MenuMainUnwound(0x82a63du);
        if (!MenuMainCall(memory, cpu, child, context, 0x82a641u, 0x82838fu, 2u))
            return MenuMainUnwound(0x82a641u);
        OpLda(memory, cpu, OpAbs(cpu, MAIN_MENU_LIST_INDEX));
        OpSta(memory, cpu, OpDp(cpu, MAIN_MENU_SELECTED_ALTERNATE));
        if (!MenuMainCall(memory, cpu, child, context, 0x82a649u, 0x829971u, 2u))
            return MenuMainUnwound(0x82a649u);
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, OpDp(cpu, MAIN_MENU_ALTERNATE_REQUEST));
        if (!MenuMainCall(memory, cpu, child, context, 0x82a650u, 0x82e746u, 3u))
            return MenuMainUnwound(0x82a650u);
        if (!MenuMainCall(memory, cpu, child, context, 0x82a654u, 0x829ae0u, 2u))
            return MenuMainUnwound(0x82a654u);
    }
    return ExecutionReturned(0x82a657u);
}

Lufia2ExecutionResult Lufia2MenuRefreshMainDisplay(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    static const uint32_t sites[] = {0x829af6u, 0x829afau, 0x829afeu,
        0x829b06u, 0x829b09u, 0x829b0cu};
    static const uint32_t targets[] = {0x868f6fu, 0x86911fu, 0x8690c0u,
        0x829b10u, 0x82999bu, 0x828704u};
    if (!MenuMainReady(cpu))
        return ExecutionHandoff(cpu, 0x829ae0u);
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, MAIN_MENU_DISPLAY_BANK));
    OpLdx(cpu, 0xd8e3u);
    OpWriteX(memory, cpu, OpDp(cpu, MAIN_MENU_DISPLAY_POINTER), cpu->x);
    if (!MenuMainCall(memory, cpu, child, context, 0x829ae9u, 0x8293cfu, 2u))
        return MenuMainUnwound(0x829ae9u);
    OpLoadA(cpu, 0x1fu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_TM));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_TS));
    for (unsigned part = 0u; part < sizeof(sites) / sizeof(sites[0]); ++part) {
        if (part == 3u) {
            OpLoadA(cpu, 0x80u);
            OpSta(memory, cpu, OpDp(cpu, MAIN_MENU_PALETTE_REQUEST));
        }
        if (!MenuMainCall(memory, cpu, child, context,
                sites[part], targets[part], part < 3u ? 3u : 2u))
            return MenuMainUnwound(sites[part]);
    }
    return ExecutionReturned(0x829b0fu);
}
