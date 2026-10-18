#include "lufia2/menu.h"
#include "menu/menu_action_internal.h"

enum {
    MENU_FRONT_LAYER = 0x2000u,
    MENU_BACK_LAYER = 0x3000u,
    MENU_SAVED_FRONT_LAYER = 0x83c0u,
    MENU_SAVED_BACK_LAYER = 0x8bc0u,
    MENU_LAYER_SIZE = 0x0800u
};

Lufia2ExecutionResult Lufia2MenuSaveTileLayers(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu,
                                               Lufia2PushedChildCall child,
                                               void *context) {
    (void)context;
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x828345u);
    OpRepWidths(cpu, 0x20u);
    PushDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, MENU_FRONT_LAYER));
        OpSta(memory, cpu, OpAbsX(cpu, MENU_SAVED_FRONT_LAYER));
        OpLda(memory, cpu, OpAbsX(cpu, MENU_BACK_LAYER));
        OpSta(memory, cpu, OpAbsX(cpu, MENU_SAVED_BACK_LAYER));
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, MENU_LAYER_SIZE);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x828369u);
}

Lufia2ExecutionResult Lufia2MenuRestoreTileLayers(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu,
                                                  Lufia2PushedChildCall child,
                                                  void *context) {
    (void)context;
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82836au);
    OpRepWidths(cpu, 0x20u);
    PushDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, MENU_SAVED_FRONT_LAYER));
        OpSta(memory, cpu, OpAbsX(cpu, MENU_FRONT_LAYER));
        OpLda(memory, cpu, OpAbsX(cpu, MENU_SAVED_BACK_LAYER));
        OpSta(memory, cpu, OpAbsX(cpu, MENU_BACK_LAYER));
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, MENU_LAYER_SIZE);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82838eu);
}
