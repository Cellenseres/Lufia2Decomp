#include "lufia2/item.h"
#include "item/menu_application_internal.h"
#include "system/wram.h"

Lufia2ExecutionResult Lufia2MenuResolveActionActor(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    (void)context;
    if (!MenuUseReady(cpu, child, 0x81u) || !A8(cpu))
        return ExecutionHandoff(cpu, 0x81faf5u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_PARTY_STAT_MEMBER_INDEX));
    OpLdy(cpu, 0u);
    for (;;) {
        OpLsrA(cpu);
        if (cpu->carry)
            break;
        OpIny(cpu);
        OpIny(cpu);
    }
    OpLda(memory, cpu, OpAbs(cpu, WRAM_PARTY_STAT_MEMBER_INDEX));
    uint16_t records = WRAM_BATTLE_ENEMY_RECORDS;
    if (!cpu->negative) {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_SCRIPT_CONTEXT));
        records = cpu->negative ? WRAM_BATTLE_PARTY_RECORDS
                                : WRAM_MENU_PARTY_CHARACTER_OFFSET;
    }
    OpLdx(cpu, OpReadX(memory, cpu, OpAbsY(cpu, records)));
    return ExecutionReturned(0x81fb19u);
}
