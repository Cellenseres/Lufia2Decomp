#ifndef LUFIA2_MENU_CAPSULE_SELECTION_H
#define LUFIA2_MENU_CAPSULE_SELECTION_H

#include "lufia2/menu.h"

Lufia2ExecutionResult Lufia2MenuBackupInventory(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2MenuBuildCapsuleFeedingPanels(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);
Lufia2ExecutionResult Lufia2MenuBuildCapsuleFeedingScreen(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);
Lufia2ExecutionResult Lufia2MenuChooseCapsule(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);
Lufia2ExecutionResult Lufia2MenuRunCapsuleFeeding(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

#endif
