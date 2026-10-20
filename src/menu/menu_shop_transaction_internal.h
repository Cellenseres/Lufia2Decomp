#ifndef LUFIA2_MENU_SHOP_TRANSACTION_INTERNAL_H
#define LUFIA2_MENU_SHOP_TRANSACTION_INTERNAL_H

#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/menu.h"
#include "menu/menu_sprite_slots.h"
#include "system/wram.h"

enum {
    SHOP_QUANTITY = WRAM_BATTLE_PORTRAIT_POSES,
    SHOP_QUANTITY_LIMIT = WRAM_BATTLE_PORTRAIT_POSES + 1u,
    SHOP_QUANTITY_STATE = WRAM_BATTLE_PORTRAIT_POSES + 2u,
    SHOP_PURCHASE_MODE = WRAM_BATTLE_CAPSULE_SLOT_FLAG,
    SHOP_FLAGS = WRAM_BATTLE_PARTY_IDS + 3u,
    SHOP_SELECTED_MEMBER = WRAM_UNK_7E14DC,
    SHOP_DIALOG_MODE = 0x14bfu,
    DP_SHOP_MEMBER = 0x22u,
    DP_SHOP_RECORD = 0x2au
};

static inline bool ShopTransactionReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x82u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && cpu->direct_page == 0u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static inline Lufia2ExecutionResult ShopTransactionUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

#define SHOP_TX_CALL(site, target, frame) \
    do { \
        if (!CallChildWithFrame(memory, cpu, child, context, \
                                site, target, frame, 0x82u)) \
            return ShopTransactionUnwound(site); \
    } while (0)

#endif
