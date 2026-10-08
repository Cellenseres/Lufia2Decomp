#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum {
    DP_NAME_RESOURCE = 0x54u,
    DP_NAME_BACKGROUND = 0x60u,
    DP_NAME_BACKGROUND_BANK = 0x62u,
    NAME_BACKGROUND = 0xa000u,
    NAME_OUTPUT = 0xad00u,
    NAME_LENGTH = 5u,
    NAME_MASK_LEFT = 0xcdu,
    NAME_MASK_RIGHT = 0xceu
};

static Lufia2ExecutionResult NameTilesUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static uint8_t BuildNameMasks(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context,
    uint32_t *interrupted) {
    static const uint16_t backgrounds[] = {0x0ac0u, 0x0ac0u, 0x0b20u, 0x0b20u};
    static const uint32_t sites[] = {0x81e89cu, 0x81e8a4u, 0x81e8acu, 0x81e8b4u};
    for (unsigned mask = 0u; mask < 4u; ++mask) {
        OpLoadA(cpu, (mask & 1u) ? NAME_MASK_RIGHT : NAME_MASK_LEFT);
        OpLdx(cpu, backgrounds[mask]);
        if (!CallChildWithFrame(memory, cpu, child, context,
                sites[mask], 0x81e8eeu, 2u, 0x81u)) {
            *interrupted = sites[mask];
            return 0u;
        }
    }
    return 1u;
}

static uint8_t BuildPartyNames(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context,
    uint32_t *interrupted) {
    static const uint32_t sites[NAME_LENGTH] = {
        0x81e8c0u, 0x81e8c6u, 0x81e8ccu, 0x81e8d2u, 0x81e8d8u
    };
    OpLdx(cpu, 2u * (WRAM_BATTLE_PARTY_RECORDS_COUNT - 1u));
    do {
        OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_PARTY_RECORDS)));
        for (unsigned letter = 0u; letter < NAME_LENGTH; ++letter) {
            OpLda(memory, cpu, OpAbsY(cpu, (uint16_t)letter));
            if (!CallChildWithFrame(memory, cpu, child, context,
                    sites[letter], 0x81e8e1u, 2u, 0x81u)) {
                *interrupted = sites[letter];
                return 0u;
            }
        }
        OpDex(cpu);
        OpDex(cpu);
    } while (!cpu->negative);
    return 1u;
}

static Lufia2ExecutionResult BuildNameTiles(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context,
    uint32_t entry, uint16_t resource) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, entry);
    OpLdx(cpu, resource);
    OpWriteX(memory, cpu, OpDp(cpu, DP_NAME_RESOURCE), cpu->x);
    OpLdx(cpu, NAME_BACKGROUND);
    OpWriteX(memory, cpu, OpDp(cpu, DP_NAME_BACKGROUND), cpu->x);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, DP_NAME_BACKGROUND_BANK));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x81e885u, 0x808e9du, 3u, 0x81u))
        return NameTilesUnwound(0x81e885u);
    PushDataBank(memory, cpu);
    OpLoadA(cpu, 0x9fu);
    PushAccumulator8(memory, cpu);
    cpu->data_bank = Pull8(memory, cpu);
    SetNz8(cpu, cpu->data_bank);
    OpLdx(cpu, NAME_OUTPUT);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_WMADDL), cpu->x);
    OpStz(memory, cpu, OpAbs(cpu, SNES_WMADDH));
    uint32_t interrupted;
    if (!BuildNameMasks(memory, cpu, child, context, &interrupted) ||
        !BuildPartyNames(memory, cpu, child, context, &interrupted))
        return NameTilesUnwound(interrupted);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81e8e0u);
}

Lufia2ExecutionResult Lufia2BattleBuildPartyNameTiles(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    return BuildNameTiles(memory, cpu, child, context, 0x81e872u, 0x018du);
}

Lufia2ExecutionResult Lufia2BattleBuildAlternateNameTiles(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    return BuildNameTiles(memory, cpu, child, context, 0x81e877u, 0x018eu);
}
