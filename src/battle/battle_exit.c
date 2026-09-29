/* Battle exit. */

#include "battle/battle_internal.h"
#include "core/snes_registers.h"
#include "system/scene_nmi_internal.h"
#include "system/wram.h"

static bool UseRegularBattleFade(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, 0x7ff8a2u);
    OpCmpValue(cpu, 1u);
    if (!cpu->zero)
        return true;

    OpLda(memory, cpu, 0x7ff8a3u);
    if (!cpu->negative)
        return false;

    OpLda(memory, cpu, 0x7ff8a4u);
    OpCmpValue(cpu, 11u);
    if (cpu->zero)
        return 1;
    OpCmpValue(cpu, 37u);
    return cpu->zero;
}

Lufia2ExecutionResult Lufia2BattleExit(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                       Lufia2PushedChildCall child,
                                       void *child_context) {
    BattleContext battle = {memory, cpu, child, child_context, 0x81u, 0u};

    if (!BattleCall(&battle, 0x876bu, 0x85ee3eu, 3u))
        return BattleChildUnwound(&battle);
    if (!BattleCall(&battle, 0x876fu, 0x85eddbu, 3u))
        return BattleChildUnwound(&battle);

    if (UseRegularBattleFade(memory, cpu)) {
        if (!BattleCall(&battle, 0x8793u, 0x81c321u, 2u))
            return BattleChildUnwound(&battle);
    } else if (!BattleCall(&battle, 0x878du, 0x85eaefu, 3u)) {
        return BattleChildUnwound(&battle);
    }

    if (!BattleClear3000(&battle, 0x8796u))
        return BattleChildUnwound(&battle);
    if (!BattleClear3800(&battle, 0x8799u))
        return BattleChildUnwound(&battle);

    OpRepWidths(cpu, 0x20u);
    if (!BattleCall(&battle, 0x879eu, 0x859bc3u, 3u))
        return BattleChildUnwound(&battle);
    OpSepWidths(cpu, 0x20u);

    if (!BattleCall(&battle, 0x87a4u, 0x85ec81u, 3u))
        return BattleChildUnwound(&battle);

    Lufia2DisableSceneNmi(memory, cpu);
    OpStz(memory, cpu, OpAbs(cpu, SNES_HDMAEN));
    LoadA8(cpu, BRIGHTNESS_FORCED_BLANK);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));

    if (!BattleCall(&battle, 0x87b2u, 0x85ec81u, 3u))
        return BattleChildUnwound(&battle);

    return ExecutionReturned(0x8187b6u);
}
