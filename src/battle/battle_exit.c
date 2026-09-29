/* Battle-side exit wrapper at $81:876B. */

#include "battle/battle_internal.h"
#include "core/snes_registers.h"
#include "system/scene_nmi_internal.h"
#include "system/wram.h"

static uint8_t UseRegularBattleFade(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, 0x7ff8a2u);
    OpCmpValue(cpu, 1u);
    if (!cpu->zero)
        return 1;

    OpLda(memory, cpu, 0x7ff8a3u);
    if (!cpu->negative)
        return 0;

    OpLda(memory, cpu, 0x7ff8a4u);
    OpCmpValue(cpu, 11u);
    if (cpu->zero)
        return 1;
    OpCmpValue(cpu, 37u);
    return cpu->zero;
}

Lufia2ExecutionResult Lufia2BattleExit(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context) {
    Lufia2BattleChildCalls calls = {
        memory, cpu, child, child_context, 0x81u, 0u};

    if (!Lufia2BattleCallChild(&calls, 0x876bu, 0x85ee3eu, 3u))
        return Lufia2BattleChildUnwound(&calls);
    if (!Lufia2BattleCallChild(&calls, 0x876fu, 0x85eddbu, 3u))
        return Lufia2BattleChildUnwound(&calls);

    if (UseRegularBattleFade(memory, cpu)) {
        if (!Lufia2BattleCallChild(&calls, 0x8793u, 0x81c321u, 2u))
            return Lufia2BattleChildUnwound(&calls);
    } else if (!Lufia2BattleCallChild(&calls, 0x878du, 0x85eaefu, 3u)) {
        return Lufia2BattleChildUnwound(&calls);
    }

    if (!Lufia2BattleCallChild(&calls, 0x8796u, 0x81c2fbu, 2u))
        return Lufia2BattleChildUnwound(&calls);
    if (!Lufia2BattleCallChild(&calls, 0x8799u, 0x81c30eu, 2u))
        return Lufia2BattleChildUnwound(&calls);

    OpRepWidths(cpu, 0x20u);
    if (!Lufia2BattleCallChild(&calls, 0x879eu, 0x859bc3u, 3u))
        return Lufia2BattleChildUnwound(&calls);
    OpSepWidths(cpu, 0x20u);

    if (!Lufia2BattleCallChild(&calls, 0x87a4u, 0x85ec81u, 3u))
        return Lufia2BattleChildUnwound(&calls);

    Lufia2DisableSceneNmi(memory, cpu);
    OpStz(memory, cpu, OpAbs(cpu, SNES_HDMAEN));
    LoadA8(cpu, BRIGHTNESS_FORCED_BLANK);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));

    if (!Lufia2BattleCallChild(&calls, 0x87b2u, 0x85ec81u, 3u))
        return Lufia2BattleChildUnwound(&calls);

    return ExecutionReturned(0x8187b6u);
}
