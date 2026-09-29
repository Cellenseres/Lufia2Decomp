/* Battle display buffers. */

#include "core/cpu_internal.h"
#include "core/snes_registers.h"
#include "lufia2/battle.h"

enum {
    BATTLE_BACKGROUND_TILEMAP = 0x2000u,
    BATTLE_PARTY_TILEMAP = 0x2800u,
    BATTLE_WINDOW_TILEMAP = 0x3000u,
    BATTLE_TILEMAP_3800 = 0x3800u,
};

static void FillWramPort(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                         uint16_t address, uint16_t count, int pairs) {
    LoadX16(cpu, address);
    StoreWordAbsolute(memory, cpu, SNES_WMADDL, cpu->x);
    StoreZeroAbsolute8(memory, cpu, SNES_WMADDH, 0);
    LoadX16(cpu, count);
    if (pairs)
        LoadA8(cpu, 0x21u);
    do {
        StoreZeroAbsolute8(memory, cpu, SNES_WMDATA, 0);
        if (pairs)
            StoreAAbsolute8(memory, cpu, SNES_WMDATA, 0);
        LoadX16(cpu, (uint16_t)(cpu->x - 1u));
    } while (!cpu->zero);
}

Lufia2ExecutionResult Lufia2BattleLoadDisplayDefaults(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadX16(cpu, 0x0000u);
    do {
        LoadAAbsolute8(memory, cpu, 0xb401u, cpu->x);
        StoreAAbsolute8(memory, cpu, 0x123cu, cpu->x);
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x0010u);
    } while (!cpu->zero);
    return ExecutionReturned(0x81c2cfu);
}

Lufia2ExecutionResult Lufia2BattleClearBackgroundTilemap(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    FillWramPort(memory, cpu, BATTLE_BACKGROUND_TILEMAP, 0x0800u, 0);
    return ExecutionReturned(0x81c2e2u);
}

Lufia2ExecutionResult Lufia2BattleResetPartyTilemap(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    FillWramPort(memory, cpu, BATTLE_PARTY_TILEMAP, 0x0400u, 1);
    return ExecutionReturned(0x81c2fau);
}

Lufia2ExecutionResult Lufia2BattleClearWindowTilemap(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    FillWramPort(memory, cpu, BATTLE_WINDOW_TILEMAP, 0x0800u, 0);
    return ExecutionReturned(0x81c30du);
}

Lufia2ExecutionResult Lufia2BattleClearTilemap3800(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    FillWramPort(memory, cpu, BATTLE_TILEMAP_3800, 0x0800u, 0);
    return ExecutionReturned(0x81c320u);
}
