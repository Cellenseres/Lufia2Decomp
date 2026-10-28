#ifndef LUFIA2_MENU_SCENARIO_ITEMS_H
#define LUFIA2_MENU_SCENARIO_ITEMS_H

#include "lufia2/menu.h"

Lufia2ExecutionResult Lufia2MenuFindScenarioItem(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2MenuRunScenarioItems(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

#endif
