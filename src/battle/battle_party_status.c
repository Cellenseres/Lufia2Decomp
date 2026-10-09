#include "core/cpu_ops.h"
#include "core/child_call.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum {
    ROM_PARTY_STATUS_TILE_CURSORS = 0x8809u,
    DP_STATUS_TILE_CURSOR = 0x11u,
    DP_PARTY_SLOT = 0x19u,
    DP_PARTY_SLOT_BANK = 0x1bu,
    PARTY_RECORD_BYTES = 8u,
    PARTY_MEMBERS = 4u,
    ROM_STATUS_SPRITE_POSITIONS = 0xebe0u,
    STATUS_POSITION_BYTES = 20u,
    STATUS_SPRITE_RECORD_BYTES = 13u,
    STATUS_SPRITE_X = WRAM_BATTLE_EFFECT_TARGET_HORIZONTAL_MOTION,
    STATUS_SPRITE_Y = WRAM_BATTLE_EFFECT_TARGET_VERTICAL_MOTION
};

static Lufia2ExecutionResult PartyStatusChildUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2BattleDrawPartyStatusWindows(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x81dea9u);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpLdx(cpu, 0u);
    do {
        OpLdy(cpu, OpReadX(memory, cpu,
            OpAbsX(cpu, ROM_PARTY_STATUS_TILE_CURSORS)));
        OpWriteX(memory, cpu, OpDp(cpu, DP_STATUS_TILE_CURSOR), cpu->y);
        OpPushX(memory, cpu);
        OpLdy(cpu, OpReadX(memory, cpu,
            OpAbsX(cpu, WRAM_BATTLE_PARTY_RECORDS)));
        if (!cpu->zero) {
            OpLoadA(cpu, 13u);
            OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_MESSAGE_WINDOW_COLUMNS));
            OpLoadA(cpu, 4u);
            OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_MESSAGE_WINDOW_ROWS));
            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_STATUS_TILE_CURSOR)));
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x81dec6u, 0x81e3ecu, 2u, cpu->program_bank))
                return PartyStatusChildUnwound(0x81dec6u);
            OpPullY(memory, cpu);
            PushY(memory, cpu);
            OpWriteX(memory, cpu, OpDp(cpu, DP_PARTY_SLOT), cpu->y);
            OpLoadA(cpu, 0x7eu);
            OpSta(memory, cpu, OpDp(cpu, DP_PARTY_SLOT_BANK));
            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_PARTY_SLOT)));
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x81ded3u, 0x81e1b5u, 2u, cpu->program_bank))
                return PartyStatusChildUnwound(0x81ded3u);
            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_PARTY_SLOT)));
            OpLoadA(cpu, 0x63u);
            OpSta(memory, cpu, OpDp(cpu, DP_STATUS_TILE_CURSOR));
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x81dedcu, 0x81e4d1u, 2u, cpu->program_bank))
                return PartyStatusChildUnwound(0x81dedcu);
        }
        OpPullX(memory, cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, PARTY_RECORD_BYTES);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81dee8u);
}

Lufia2ExecutionResult Lufia2BattleRefreshPartyStatusSprites(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child) return ExecutionHandoff(cpu, 0x81eb93u);
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    OpRepWidths(cpu, 0x30u);
    PushAccumulator16(memory, cpu);
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    TransferDirectToA(cpu);
    do {
        PushAccumulator8(memory, cpu);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x81eb9eu, 0x81bbe0u, 2u, cpu->program_bank))
            return PartyStatusChildUnwound(0x81eb9eu);
        LoadA8(cpu, Pull8(memory, cpu));
        OpIncA(cpu);
        OpCmpValue(cpu, PARTY_MEMBERS);
    } while (!cpu->zero);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLdy(cpu, 0u);
    OpTyx(cpu);
    do {
        OpLda(memory, cpu, OpAbsY(cpu, ROM_STATUS_SPRITE_POSITIONS));
        OpSta(memory, cpu, OpAbsX(cpu, STATUS_SPRITE_X));
        OpLda(memory, cpu, OpAbsY(cpu, ROM_STATUS_SPRITE_POSITIONS + 2u));
        OpSta(memory, cpu, OpAbsX(cpu, STATUS_SPRITE_Y));
        for (unsigned byte = 0u; byte < 4u; ++byte) OpIny(cpu);
        OpTxa(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, STATUS_SPRITE_RECORD_BYTES);
        OpTax(cpu);
        OpCpy(cpu, STATUS_POSITION_BYTES);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, WRAM_BATTLE_EFFECT_FRAME_MARKER);
    OpRepWidths(cpu, 0x20u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x81ebd4u, 0x859bdau, 3u, cpu->program_bank))
        return PartyStatusChildUnwound(0x81ebd4u);
    OpRepWidths(cpu, 0x30u);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    PullAccumulator16(memory, cpu);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x81ebdfu);
}
