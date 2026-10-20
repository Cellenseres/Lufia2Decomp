#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/menu.h"
#include "system/wram.h"

enum {
    SHOP_FLAGS = WRAM_BATTLE_PARTY_IDS + 3u,
    SHOP_TEXT_SOURCE = 0x5du,
    SHOP_TEXT_BANK = 0x5fu
};

static bool ShopSessionReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x82u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && cpu->direct_page == 0u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static Lufia2ExecutionResult ShopChildUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

#define SHOP_CALL(site, target, frame) \
    do { \
        if (!CallChildWithFrame(memory, cpu, child, context, \
                                site, target, frame, 0x82u)) \
            return ShopChildUnwound(site); \
    } while (0)

Lufia2ExecutionResult Lufia2MenuShopSession(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopSessionReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82d69cu);
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    SHOP_CALL(0x82d6a0u, 0x868dbbu, 3u);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, 0x71u));
    OpLda(memory, cpu, OpDp(cpu, 0x30u));
    OpCmpValue(cpu, 0xf0u);
    if (cpu->carry) {
        SHOP_CALL(0x82d6aeu, 0x868dbbu, 3u);
        cpu->program_bank = 0x84u;
        return ExecutionHandoff(cpu, 0x84a689u);
    }
    SHOP_CALL(0x82d6b6u, 0x868dd7u, 3u);
    SHOP_CALL(0x82d6bau, 0x868e6bu, 3u);
    SHOP_CALL(0x82d6beu, 0x868eb0u, 3u);
    SHOP_CALL(0x82d6c2u, 0x868f1bu, 3u);
    SHOP_CALL(0x82d6c6u, 0x868f45u, 3u);
    SHOP_CALL(0x82d6cau, 0x868f6fu, 3u);
    SHOP_CALL(0x82d6ceu, 0x8690c0u, 3u);
    SHOP_CALL(0x82d6d2u, 0x8690e6u, 3u);
    SHOP_CALL(0x82d6d6u, 0x8690f9u, 3u);
    SHOP_CALL(0x82d6dau, 0x86911fu, 3u);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, DP_NMI_UPLOAD_FLAGS));
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, SHOP_TEXT_BANK));
    OpLdx(cpu, 0xd8ebu);
    OpWriteX(memory, cpu, OpDp(cpu, SHOP_TEXT_SOURCE), cpu->x);
    SHOP_CALL(0x82d6ebu, 0x8293cfu, 2u);
    SHOP_CALL(0x82d6eeu, 0x8289a4u, 2u);
    SHOP_CALL(0x82d6f1u, 0x868e79u, 3u);
    SHOP_CALL(0x82d6f5u, 0x82da8fu, 2u);
    OpLoadA(cpu, 0x80u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_PLAY_TIME_FRAMES), 0u);
    SHOP_CALL(0x82d6fdu, 0x868dbbu, 3u);
    return ExecutionReturned(0x82d701u);
}

Lufia2ExecutionResult Lufia2MenuShopInitializeSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopSessionReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82d702u);
    OpLdx(cpu, 0x20u);
    SHOP_CALL(0x82d705u, 0x868d47u, 3u);
    OpLdy(cpu, 0xf090u);
    SHOP_CALL(0x82d70cu, 0x828c9cu, 2u);
    OpLdx(cpu, 0x4000u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_SCROLL_TRACK), cpu->x);
    SHOP_CALL(0x82d715u, 0x82da5cu, 2u);
    SHOP_CALL(0x82d718u, 0x828069u, 2u);
    OpLoadA(cpu, 0u);
    SHOP_CALL(0x82d71du, 0x8299f3u, 2u);
    return ExecutionReturned(0x82d720u);
}

Lufia2ExecutionResult Lufia2MenuShopSelectListWindow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopSessionReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82da5cu);
    OpLda(memory, cpu, OpAbs(cpu, SHOP_FLAGS));
    OpBitValue(cpu, 0x20u);
    if (!cpu->zero) {
        OpLdx(cpu, 0x30u);
    } else {
        OpBitValue(cpu, 4u);
        if (!cpu->zero) {
            OpBitValue(cpu, 0x1bu);
            OpLdx(cpu, cpu->zero ? 0x40u : 0x20u);
        } else {
            OpLdx(cpu, 0x20u);
        }
    }
    OpRepWidths(cpu, 0x20u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTION_REFERENCE), cpu->x);
    OpTxa(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x1040u);
    OpTay(cpu);
    OpLoadA(cpu, 0x42u);
    OpLdx(cpu, 0x0404u);
    SHOP_CALL(0x82da89u, 0x82831au, 2u);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82da8eu);
}

Lufia2ExecutionResult Lufia2MenuShopRefreshSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopSessionReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82dad8u);
    SHOP_CALL(0x82dad8u, 0x82e012u, 2u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_ITEM_ROW));
    cpu->carry = 0u;
    OpAdcValue(cpu, 8u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09D3));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09D3 + 1u));
    OpLdy(cpu, 1u);
    SHOP_CALL(0x82daeau, 0x8ee751u, 3u);
    SHOP_CALL(0x82daeeu, 0x82e327u, 2u);
    SHOP_CALL(0x82daf1u, 0x82dc69u, 2u);
    return ExecutionReturned(0x82daf4u);
}

#undef SHOP_CALL
