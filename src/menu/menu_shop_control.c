#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/menu.h"
#include "system/wram.h"

enum {
    SHOP_FLAGS = WRAM_BATTLE_PARTY_IDS + 3u,
    SHOP_LIST_MODE = WRAM_BATTLE_CAPSULE_SLOT_FLAG,
    SHOP_LIST_LIMIT = WRAM_BATTLE_PARTY_IDS + 2u,
    SHOP_SOURCE = 0xfcu,
    SHOP_DESTINATION = 8u,
    SHOP_LIST_POINTERS = 0x7e93c0u,
    SHOP_LIST_COUNTS = 0x7e93d0u,
    SHOP_CURSOR_STATE = 0x14c1u,
    SHOP_SAVED_SELECTION = 0x14afu,
    SHOP_VISIBLE_ROWS = 0x14f8u
};

static bool ShopControlReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x82u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && cpu->direct_page == 0u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static Lufia2ExecutionResult ShopControlUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

#define SHOP_CALL(site, target, frame) \
    do { \
        if (!CallChildWithFrame(memory, cpu, child, context, \
                                site, target, frame, 0x82u)) \
            return ShopControlUnwound(site); \
    } while (0)

Lufia2ExecutionResult Lufia2MenuRunShop(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopControlReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82da8fu);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E14BD));
    SHOP_CALL(0x82da94u, 0x8ee700u, 3u);
    SHOP_CALL(0x82da98u, 0x82e297u, 2u);
    SHOP_CALL(0x82da9bu, 0x82e49eu, 2u);
    SHOP_CALL(0x82da9eu, 0x82d702u, 2u);
    SHOP_CALL(0x82daa1u, 0x82d721u, 2u);
    SHOP_CALL(0x82daa4u, 0x82d749u, 2u);
    SHOP_CALL(0x82daa7u, 0x82dad8u, 2u);
    SHOP_CALL(0x82daaau, 0x828704u, 2u);
    uint8_t refresh = 1u;
    for (;;) {
        OpLoadA(cpu, refresh);
        SHOP_CALL(0x82daafu, 0x828b08u, 2u);
        OpCmpValue(cpu, 1u);
        if (cpu->zero) {
            OpLda(memory, cpu, OpDp(cpu, 0x47u));
            OpAndValue(cpu, 0x0cu);
            if (!cpu->zero) {
                OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_SHOP_LIST_COUNT));
                OpCmpValue(cpu, 1u);
                if (!cpu->zero)
                    SHOP_CALL(0x82dad3u, 0x82dad8u, 2u);
            }
            refresh = 1u;
            continue;
        }
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            OpLoadA(cpu, 2u);
            SHOP_CALL(0x82daf7u, 0x80953bu, 3u);
            OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_COUNT));
            if (!cpu->zero) {
                SHOP_CALL(0x82db00u, 0x82db15u, 2u);
                OpLdy(cpu, 0u);
                SHOP_CALL(0x82db06u, 0x8ee710u, 3u);
            }
            refresh = 0u;
            continue;
        }
        OpCmpValue(cpu, 3u);
        if (cpu->zero)
            break;
        refresh = 0u;
    }
    SHOP_CALL(0x82db0du, 0x868b32u, 3u);
    SHOP_CALL(0x82db11u, 0x82838fu, 2u);
    return ExecutionReturned(0x82db14u);
}

static Lufia2ExecutionResult CloseShopSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    OpLoadA(cpu, 0u);
    SHOP_CALL(0x82db77u, 0x8299f3u, 2u);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x018au);
    OpLdx(cpu, 0x1007u);
    SHOP_CALL(0x82db82u, 0x8283ebu, 2u);
    OpSepWidths(cpu, 0x20u);
    SHOP_CALL(0x82db87u, 0x82994eu, 3u);
    SHOP_CALL(0x82db8bu, 0x82e4d5u, 2u);
    OpLoadA(cpu, 8u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_MENU_FRAME_FLAGS), 1u);
    OpLoadA(cpu, 0u);
    OpLdx(cpu, 6u);
    OpLdy(cpu, 5u);
    SHOP_CALL(0x82db9au, 0x8289fau, 2u);
    OpLoadA(cpu, 3u);
    OpLdx(cpu, 6u);
    OpLdy(cpu, 5u);
    SHOP_CALL(0x82dba5u, 0x8289bcu, 2u);
    return ExecutionReturned(0x82dba8u);
}

static void RememberShopSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, 0x14b3u));
    cpu->carry = 0u;
    OpAdcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW)));
    OpSta(memory, cpu, OpAbs(cpu, SHOP_SAVED_SELECTION));
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, 1u);
    OpLda(memory, cpu, OpAbsX(cpu, SHOP_CURSOR_STATE));
    OpAndValue(cpu, 1u);
    OpSta(memory, cpu, OpAbsX(cpu, SHOP_CURSOR_STATE));
    OpLoadA(cpu, 8u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_MENU_FRAME_FLAGS), 1u);
}

Lufia2ExecutionResult Lufia2MenuSelectShopItem(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopControlReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82db15u);
    SHOP_CALL(0x82db15u, 0x82d79du, 2u);
    OpLdx(cpu, 1u);
    SHOP_CALL(0x82db1bu, 0x8288a0u, 2u);
    OpLda(memory, cpu, OpAbs(cpu, 0x11e3u));
    if (cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_COUNT));
        OpSta(memory, cpu, OpAbs(cpu, SHOP_VISIBLE_ROWS));
    }
    SHOP_CALL(0x82db29u, 0x82dc10u, 2u);
    OpLoadA(cpu, 8u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_MENU_FRAME_FLAGS), 1u);
    uint8_t refresh = 0u;
    for (;;) {
        OpLoadA(cpu, refresh);
        SHOP_CALL(0x82db32u, 0x828b08u, 2u);
        const uint8_t action = (uint8_t)cpu->accumulator;
        switch (action) {
        case 2u:
            OpCmpValue(cpu, 2u);
            OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PRICE_ADJUST_MODE));
            SHOP_CALL(0x82dbacu, 0x82dbc9u, 2u);
            if (cpu->carry) {
                refresh = 1u;
                continue;
            }
            SHOP_CALL(0x82dbb1u, 0x82e327u, 2u);
            SHOP_CALL(0x82dbb4u, 0x8293c2u, 2u);
            SHOP_CALL(0x82dbb7u, 0x82d721u, 2u);
            SHOP_CALL(0x82dbbau, 0x82dc69u, 2u);
            OpLoadA(cpu, 10u);
            SHOP_CALL(0x82dbbfu, 0x8293f6u, 2u);
            OpLoadA(cpu, 0x88u);
            OpSta(memory, cpu, OpDp(cpu, DP_MENU_FRAME_FLAGS));
            refresh = 0u;
            continue;
        case 3u:
            OpCmpValue(cpu, 3u);
            return CloseShopSelection(memory, cpu, child, context);
        case 4u:
            OpCmpValue(cpu, 4u);
            OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PRICE_ADJUST_MODE));
            SHOP_CALL(0x82dc33u, 0x82dc49u, 2u);
            OpSta(memory, cpu, OpAbs(cpu, 0x1513u));
            OpSepWidths(cpu, 0x20u);
            if (!cpu->carry) {
                OpLdx(cpu, 0u);
                OpLdy(cpu, 0x0a0fu);
                SHOP_CALL(0x82dc43u, 0x82fd81u, 2u);
            }
            refresh = 0u;
            continue;
        case 6u:
        case 7u:
            OpCmpValue(cpu, action);
            OpLoadA(cpu, OpA(cpu) ^ 1u);
            OpLsrA(cpu);
            SHOP_CALL(0x82dbd7u, 0x828ce9u, 2u);
            if (!cpu->carry) {
                SHOP_CALL(0x82dbdcu, 0x82dc8au, 2u);
                SHOP_CALL(0x82dbdfu, 0x82dc10u, 2u);
            }
            break;
        case 10u:
            OpCmpValue(cpu, 10u);
            SHOP_CALL(0x82dbe4u, 0x82dd18u, 2u);
            break;
        case 11u:
            OpCmpValue(cpu, 11u);
            SHOP_CALL(0x82dbeau, 0x82ddaeu, 2u);
            break;
        case 1u:
            OpCmpValue(cpu, 1u);
            SHOP_CALL(0x82db6fu, 0x82dc10u, 2u);
            break;
        default:
            OpCmpValue(cpu, 1u);
            refresh = 0u;
            continue;
        }
        SHOP_CALL(0x82dbedu, 0x828c43u, 2u);
        RememberShopSelection(memory, cpu);
        refresh = 1u;
    }
}

Lufia2ExecutionResult Lufia2MenuShopSelectListMode(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ShopControlReady(cpu))
        return ExecutionHandoff(cpu, 0x82e012u);
    OpLda(memory, cpu, OpAbs(cpu, SHOP_FLAGS));
    OpBitValue(cpu, 2u);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_ITEM_ROW));
        OpLoadA(cpu, cpu->zero ? 2u : 1u);
    } else {
        OpLoadA(cpu, 0u);
    }
    OpSta(memory, cpu, OpAbs(cpu, SHOP_LIST_MODE));
    return ExecutionReturned(0x82e02bu);
}

Lufia2ExecutionResult Lufia2MenuShopDrawListPanel(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopControlReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82dc69u);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x0444u);
    OpLdx(cpu, 0x1c0fu);
    SHOP_CALL(0x82dc71u, 0x8283ebu, 2u);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, 0x5fu));
    OpLdy(cpu, 0xccfdu);
    SHOP_CALL(0x82dc82u, 0x808878u, 3u);
    SHOP_CALL(0x82dc86u, 0x82dc8au, 2u);
    return ExecutionReturned(0x82dc89u);
}

Lufia2ExecutionResult Lufia2MenuShopDrawVisibleRows(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopControlReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82dc8au);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x0486u);
    OpLdx(cpu, 0x1b0cu);
    SHOP_CALL(0x82dc92u, 0x8283ebu, 2u);
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW)));
    OpWriteX(memory, cpu, OpDp(cpu, 0u), cpu->x);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PRICE_ADJUST_MODE));
    SHOP_CALL(0x82dc9fu, 0x82dca3u, 2u);
    return ExecutionReturned(0x82dca2u);
}

static void LoadShopListGeometry(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, SHOP_LIST_COUNTS + cpu->x);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_COUNT));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_OVERFLOW));
    cpu->carry = 1u;
    OpSbcValue(cpu, 5u);
    if (cpu->carry)
        OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_OVERFLOW));
}

Lufia2ExecutionResult Lufia2MenuShopBuildSelectedList(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopControlReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82e327u);
    OpRepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_COUNT));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_ITEM_ROW));
    OpAndValue(cpu, 0xffu);
    OpTax(cpu);
    OpInx(cpu);
    OpLoadA(cpu, 0u);
    cpu->carry = 1u;
    for (;;) {
        RolA16(cpu);
        OpBit(memory, cpu, OpAbs(cpu, SHOP_FLAGS));
        if (!cpu->zero) {
            OpDex(cpu);
            if (cpu->zero)
                break;
        }
    }
    OpSepWidths(cpu, 0x20u);
    unsigned list_kind = 6u;
    for (unsigned bit = 0; bit < 6u; ++bit) {
        OpBitValue(cpu, (uint16_t)(1u << bit));
        if (!cpu->zero) {
            list_kind = bit;
            OpLdx(cpu, (uint16_t)(2u * bit));
            break;
        }
    }
    if (list_kind < 5u) {
        OpLoadA(cpu, list_kind ? 1u : 3u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_PRICE_ADJUST_MODE));
        OpLoadA(cpu, list_kind ? 0x97u : 0x7eu);
        OpSta(memory, cpu, OpDp(cpu, SHOP_SOURCE + 2u));
        if (list_kind) {
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, SHOP_LIST_POINTERS + cpu->x);
            OpSta(memory, cpu, OpDp(cpu, SHOP_SOURCE));
        } else {
            OpLdy(cpu, 0x97e0u);
            OpWriteX(memory, cpu, OpDp(cpu, SHOP_SOURCE), cpu->y);
        }
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, 0x7eu);
        OpSta(memory, cpu, OpDp(cpu, SHOP_DESTINATION + 2u));
        OpLdy(cpu, 0x93e0u);
        OpWriteX(memory, cpu, OpDp(cpu, SHOP_DESTINATION), cpu->y);
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, OpAbs(cpu, SHOP_LIST_LIMIT));
        OpRepWidths(cpu, 0x20u);
        LoadShopListGeometry(memory, cpu);
        OpLdy(cpu, 0u);
        for (;;) {
            OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, SHOP_SOURCE));
            OpAndValue(cpu, 0x01ffu);
            if (cpu->zero)
                break;
            SHOP_CALL(0x82e3ccu, 0x82fb1fu, 3u);
            if (cpu->carry)
                OpLoadA(cpu, 0u);
            else
                SHOP_CALL(0x82e3d2u, 0x82faf4u, 2u);
            OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, SHOP_DESTINATION));
            OpIny(cpu);
            OpIny(cpu);
        }
        cpu->carry = 0u;
    } else if (list_kind == 5u) {
        OpLoadA(cpu, 2u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_PRICE_ADJUST_MODE));
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, OpAbs(cpu, SHOP_LIST_LIMIT));
        OpRepWidths(cpu, 0x20u);
        LoadShopListGeometry(memory, cpu);
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, 0x97u);
        OpSta(memory, cpu, OpDp(cpu, SHOP_SOURCE + 2u));
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, SHOP_LIST_POINTERS + cpu->x);
        OpSta(memory, cpu, OpDp(cpu, SHOP_SOURCE));
        cpu->carry = 0u;
    } else {
        OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_PRICE_ADJUST_MODE));
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, OpAbs(cpu, SHOP_LIST_LIMIT));
        OpLdx(cpu, 0x60u);
        OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_COUNT), cpu->x);
        OpLdx(cpu, 0x5bu);
        OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_OVERFLOW), cpu->x);
        cpu->carry = 0u;
    }
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_COUNT)));
    SHOP_CALL(0x82e42fu, 0x828c85u, 2u);
    return ExecutionReturned(0x82e432u);
}

#undef SHOP_CALL
