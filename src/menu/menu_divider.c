#include "lufia2/menu.h"
#include "menu/menu_action_internal.h"

enum {
    DP_DIVIDER_CELLS = 0x56u,
    ROM_DIVIDER_LEFT = 0x82dau,
    ROM_DIVIDER_PATTERN = 0x82dcu,
    ROM_DIVIDER_RIGHT = 0x82e0u,
    DIVIDER_TILEMAP = 0x7e2000u
};

Lufia2ExecutionResult Lufia2MenuDrawWindowDivider(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu,
                                                  Lufia2PushedChildCall child,
                                                  void *context) {
    (void)context;
    if (!MenuActionReady(cpu, child, 0u))
        return ExecutionHandoff(cpu, 0x8282e2u);
    OpDex(cpu);
    OpDex(cpu);
    OpWriteX(memory, cpu, OpDp(cpu, DP_DIVIDER_CELLS), cpu->x);
    OpTax(cpu);
    PushDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpSetDataBank(memory, cpu, 0x82u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, ROM_DIVIDER_LEFT));
    OpSta(memory, cpu, OpLongX(cpu, DIVIDER_TILEMAP));
    OpLdy(cpu, 0u);
    do {
        OpLda(memory, cpu, OpAbsY(cpu, ROM_DIVIDER_PATTERN));
        OpSta(memory, cpu, OpLongX(cpu, DIVIDER_TILEMAP + 2u));
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, 4u);
        if (!cpu->zero)
            OpLdy(cpu, 0u);
        OpInx(cpu);
        OpInx(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, DP_DIVIDER_CELLS), -1);
    } while (!cpu->zero);
    OpLda(memory, cpu, OpAbs(cpu, ROM_DIVIDER_RIGHT));
    OpSta(memory, cpu, OpLongX(cpu, DIVIDER_TILEMAP + 2u));
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x828319u);
}
