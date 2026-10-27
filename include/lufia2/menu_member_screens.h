#ifndef LUFIA2_MENU_MEMBER_SCREENS_H
#define LUFIA2_MENU_MEMBER_SCREENS_H

#include "lufia2/menu.h"

Lufia2ExecutionResult Lufia2MenuChooseSpellMember(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);
Lufia2ExecutionResult Lufia2MenuRunMemberStatus(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);
Lufia2ExecutionResult Lufia2MenuBuildMemberStatus(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuRunMemberScreenPhase(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

#endif
