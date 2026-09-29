#include "battle/battle_lifecycle_detail.h"
#include "core/snes_registers.h"
#include "system/scene_nmi_internal.h"

static bool UseRegularBattleFade(BattleContext *battle) {
    Lufia2CpuState *cpu = battle->cpu;

    OpLda(battle->memory, cpu, 0x7ff8a2u);
    OpCmpValue(cpu, 1u);
    if (!cpu->zero)
        return true;

    OpLda(battle->memory, cpu, 0x7ff8a3u);
    if (!cpu->negative)
        return false;

    OpLda(battle->memory, cpu, 0x7ff8a4u);
    OpCmpValue(cpu, 11u);
    if (cpu->zero)
        return true;

    OpCmpValue(cpu, 37u);
    return cpu->zero;
}

bool BattlePrepareExit(BattleContext *battle) {
    if (!BattleCall(battle, 0x876bu, 0x85ee3eu, 3u))
        return false;

    return BattleCall(battle, 0x876fu, 0x85eddbu, 3u);
}

bool BattleFadeOut(BattleContext *battle) {
    if (UseRegularBattleFade(battle))
        return BattleCall(battle, 0x8793u, 0x81c321u, 2u);

    return BattleCall(battle, 0x878du, 0x85eaefu, 3u);
}

bool BattleTearDownDisplay(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;

    if (!BattleClear3000(battle, 0x8796u))
        return false;
    if (!BattleClear3800(battle, 0x8799u))
        return false;

    OpRepWidths(cpu, 0x20u);
    if (!BattleCall(battle, 0x879eu, 0x859bc3u, 3u))
        return false;
    OpSepWidths(cpu, 0x20u);

    if (!BattleCall(battle, 0x87a4u, 0x85ec81u, 3u))
        return false;

    Lufia2DisableSceneNmi(memory, cpu);
    OpStz(memory, cpu, OpAbs(cpu, SNES_HDMAEN));
    LoadA8(cpu, BRIGHTNESS_FORCED_BLANK);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));

    return BattleCall(battle, 0x87b2u, 0x85ec81u, 3u);
}
