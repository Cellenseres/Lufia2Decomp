#ifndef LUFIA2_MENU_OPTIONS_SCREEN_H
#define LUFIA2_MENU_OPTIONS_SCREEN_H

#include "lufia2/menu.h"

Lufia2ExecutionResult Lufia2MenuSetOptionIndicators(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2MenuRefreshOptions(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);
Lufia2ExecutionResult Lufia2MenuBuildOptionsScreen(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);
Lufia2ExecutionResult Lufia2MenuRunOptionsScreen(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);


#endif
