#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum {
    DP_PARTY_SLOT = 0u,
    DP_PORTRAIT_SLOT = 0x11u,
    DP_PORTRAIT_POSE = 0x12u,
    PORTRAIT_TILE_TARGETS = 0xb52cu,
    BATTLER_STATUS = 15u,
    POSE_DEFAULT = 1u,
    POSE_STATUS = 3u,
    POSE_DOWNED = 5u,
    EMPTY_SLOT = 0xffu,
    PORTRAIT_BLOCK_BYTES = 256u
};

static void ClearSlotPortrait(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, DP_PARTY_SLOT));
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, PORTRAIT_TILE_TARGETS));
    OpTax(cpu);
    PushDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpRepWidths(cpu, 0x20u);
    OpLdy(cpu, PORTRAIT_BLOCK_BYTES);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, 0u));
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    OpRepWidths(cpu, 0x20u);
    OpTxa(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, PORTRAIT_BLOCK_BYTES);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLdy(cpu, PORTRAIT_BLOCK_BYTES);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, 0u));
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
}

static void SelectSlotPose(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpTxa(cpu);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_PARTY_RECORDS));
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, BATTLER_STATUS));
    OpBitValue(cpu, 4u);
    if (!cpu->zero) {
        OpLoadA(cpu, POSE_DOWNED);
    } else {
        OpBitValue(cpu, 0x29u);
        OpLoadA(cpu, cpu->zero ? POSE_DEFAULT : POSE_STATUS);
    }
    OpSta(memory, cpu, OpDp(cpu, DP_PORTRAIT_POSE));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DP_PARTY_SLOT));
    OpSta(memory, cpu, OpDp(cpu, DP_PORTRAIT_SLOT));
}

Lufia2ExecutionResult Lufia2BattleUpdateSlotPortraitStatus(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f06u || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x81bbe0u);
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, DP_PARTY_SLOT));
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_PARTY_IDS));
    OpAndValue(cpu, 0xffu);
    OpCmpValue(cpu, EMPTY_SLOT);
    if (cpu->zero) {
        ClearSlotPortrait(memory, cpu);
        return ExecutionReturned(0x81bc50u);
    }
    SelectSlotPose(memory, cpu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x81bc1cu, 0x81bb75u, 2u, 0x81u)) {
        const Lufia2ExecutionResult result = {LUFIA2_EXECUTION_CHILD_UNWOUND, 0x81bc1cu,
                                              0u};
        return result;
    }
    return ExecutionReturned(0x81bc1fu);
}
