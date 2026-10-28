#ifndef LUFIA2_MENU_CAPSULE_SCREEN_H
#define LUFIA2_MENU_CAPSULE_SCREEN_H

#include "lufia2/menu.h"

Lufia2ExecutionResult Lufia2MenuRunCapsuleScreen(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);
Lufia2ExecutionResult Lufia2MenuRunCapsuleAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);
Lufia2ExecutionResult Lufia2MenuBuildCapsuleScreen(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuDrawCapsuleExperience(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

#endif
