#include "menu/menu_shop_transaction_internal.h"

Lufia2ExecutionResult Lufia2MenuResetPartyAnimations(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopTransactionReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x8299f3u);
    OpLdx(cpu, 0u);
    do {
        PushAccumulator8(memory, cpu);
        PushAccumulator8(memory, cpu);
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ACTIVE));
        OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ANIMATION));
        OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_FRAME_INDEX));
        LoadA8(cpu, Pull8(memory, cpu));
        SHOP_TX_CALL(0x829a04u, 0x868cf5u, 3u);
        LoadA8(cpu, Pull8(memory, cpu));
        OpSepWidths(cpu, 0x10u);
        OpInx(cpu);
        OpCpx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_PARTY_MEMBER_COUNT)));
        OpRepWidths(cpu, 0x10u);
    } while (!cpu->zero);
    return ExecutionReturned(0x829a13u);
}
