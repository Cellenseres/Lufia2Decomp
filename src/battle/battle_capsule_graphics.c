#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum {
    CAPSULE_SLOT_FLAG = WRAM_BATTLE_CAPSULE_SLOT_FLAG & 0xffffu,
    CAPSULE_GRAPHICS_ID = WRAM_BATTLE_CAPSULE_GRAPHICS_ID & 0xffffu,
    DP_BLIT_POSITION = 0u,
    DP_BLIT_FACTORS = 2u,
    DP_BLIT_SOURCE = 8u,
    DP_BLIT_TARGET = 11u,
    DP_RESOURCE_ID = 0x54u,
    DP_RESOURCE_TARGET = 0x60u,
    DP_RESOURCE_BANK = 0x62u,
    CAPSULE_RESOURCES = 0x194u,
    CAPSULE_SOURCE = 0xdf00u,
    CAPSULE_TARGET = 0x8000u,
    CAPSULE_BYTES = 0x480u,
    CAPSULE_PALETTE = 0x7e4800u,
    PALETTE_TABLE = 0xd258u,
    PALETTE_BYTES = 32u,
    WM_DATA = 0x2180u,
    WM_ADDRESS = 0x2181u,
    WM_BANK = 0x2183u,
    MULTIPLIER_A = 0x4202u,
    MULTIPLIER_B = 0x4203u,
    MULTIPLIER_RESULT = 0x4216u
};

static void ClearCapsuleSource(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLdx(cpu, CAPSULE_SOURCE);
    OpWriteX(memory, cpu, OpAbs(cpu, WM_ADDRESS), cpu->x);
    OpStz(memory, cpu, OpAbs(cpu, WM_BANK));
    OpLdx(cpu, CAPSULE_BYTES);
    do {
        OpStz(memory, cpu, OpAbs(cpu, WM_DATA));
        OpDex(cpu);
    } while (!cpu->zero);
}

static void PrepareCapsuleResource(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, CAPSULE_GRAPHICS_ID));
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    cpu->carry = 0u;
    OpAdcValue(cpu, CAPSULE_RESOURCES);
    OpSta(memory, cpu, OpDp(cpu, DP_RESOURCE_ID));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, MULTIPLIER_RESULT));
    OpLdx(cpu, CAPSULE_SOURCE);
    OpWriteX(memory, cpu, OpDp(cpu, DP_RESOURCE_TARGET), cpu->x);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, DP_RESOURCE_BANK));
}

static void PrepareCapsuleTiles(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpDp(cpu, DP_BLIT_POSITION));
    OpLoadA(cpu, 3u);
    OpSta(memory, cpu, OpDp(cpu, DP_BLIT_FACTORS));
    OpSta(memory, cpu, OpDp(cpu, DP_BLIT_FACTORS + 1u));
    OpLdx(cpu, CAPSULE_TARGET);
    OpWriteX(memory, cpu, OpDp(cpu, DP_BLIT_TARGET), cpu->x);
    OpLdx(cpu, CAPSULE_SOURCE);
    OpWriteX(memory, cpu, OpDp(cpu, DP_BLIT_SOURCE), cpu->x);
}

static void LoadCapsulePalette(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, CAPSULE_GRAPHICS_ID));
    OpSta(memory, cpu, OpAbs(cpu, MULTIPLIER_A));
    OpLoadA(cpu, PALETTE_BYTES);
    OpSta(memory, cpu, OpAbs(cpu, MULTIPLIER_B));
    OpSetDataBank(memory, cpu, 0x97u);
    OpLdx(cpu, 0u);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, MULTIPLIER_RESULT)));
    do {
        OpLda(memory, cpu, OpAbsY(cpu, PALETTE_TABLE));
        OpSta(memory, cpu, OpLongX(cpu, CAPSULE_PALETTE));
        OpInx(cpu);
        OpIny(cpu);
        OpCpx(cpu, PALETTE_BYTES);
    } while (!cpu->zero);
}

Lufia2ExecutionResult Lufia2BattleLoadCapsuleGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f0au || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x81bc55u);
    PushDataBank(memory, cpu);
    OpLda(memory, cpu, OpAbs(cpu, CAPSULE_SLOT_FLAG));
    if (cpu->zero) {
        ClearCapsuleSource(memory, cpu);
    } else {
        PrepareCapsuleResource(memory, cpu);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x81bc8bu, 0x808e9du, 3u, 0x81u)) {
            const Lufia2ExecutionResult result =
                {LUFIA2_EXECUTION_CHILD_UNWOUND, 0x81bc8bu};
            return result;
        }
    }
    PrepareCapsuleTiles(memory, cpu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x81bca3u, 0x81bcccu, 3u, 0x81u)) {
        const Lufia2ExecutionResult result =
            {LUFIA2_EXECUTION_CHILD_UNWOUND, 0x81bca3u};
        return result;
    }
    LoadCapsulePalette(memory, cpu);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81bccbu);
}
