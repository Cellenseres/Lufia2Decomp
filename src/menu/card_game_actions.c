#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/card_game.h"
#include "system/wram.h"

enum {
    CARD_STAKE = WRAM_CARD_GAME_STAKE & 0xffffu,
    CARD_STAKE_LIMIT = WRAM_CARD_GAME_STAKE_LIMIT & 0xffffu,
    CARD_RESERVED_STAKE = WRAM_CARD_GAME_RESERVED_STAKE & 0xffffu,
    CARD_BALANCE = WRAM_MENU_EXCHANGE_BALANCE & 0xffffu,
    CARD_HANDS = WRAM_CARD_GAME_HANDS & 0xffffu,
    CARD_COUNTS = WRAM_CARD_GAME_HAND_COUNTS & 0xffffu,
    CARD_WINDOW = WRAM_SPECIAL_MENU_WINDOW_PARAMETERS & 0xffffu,
    CARD_ACCEPTED_STAKE = WRAM_CARD_GAME_ACCEPTED_STAKE & 0xffffu,
    CARD_ACTION_LIMIT = 65536u
};

typedef struct CardAction {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
    unsigned calls;
} CardAction;

static bool CardActionReady(const Lufia2CpuState *cpu, bool byte_accumulator) {
    return cpu->program_bank == 0x84u &&
           cpu->accumulator_is_8_bit == byte_accumulator &&
           !cpu->index_is_8_bit && !cpu->direct_page && !cpu->decimal;
}

static bool ActionCall(CardAction *action, uint32_t site, uint32_t target) {
    Lufia2CpuState *cpu = action->cpu;
    const bool byte_accumulator = cpu->accumulator_is_8_bit != 0u;
    if (action->calls++ == CARD_ACTION_LIMIT) {
        action->result = ExecutionHandoff(cpu, site);
        return false;
    }
    const uint8_t frame = (target >> 16) == 0x84u ? 2u : 3u;
    if (!CallChildWithFrame(action->memory, cpu, action->child, action->context,
                            site, target, frame, 0x84u)) {
        action->result = ExecutionReturned(site);
        action->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (!CardActionReady(cpu, byte_accumulator)) {
        action->result = ExecutionHandoff(cpu, site + frame + 1u);
        return false;
    }
    return true;
}

Lufia2ExecutionResult Lufia2CardGameInitializeScene(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !CardActionReady(cpu, true))
        return ExecutionHandoff(cpu, 0x84b57fu);
    CardAction action = {memory, cpu, child, context,
                         ExecutionReturned(0x84b602u), 0u};
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x20u);
    PushDataBank(memory, cpu);
    for (unsigned palette = 0u; palette < 2u; ++palette) {
        OpLdx(cpu, 0xed20u);
        OpLdy(cpu, palette ? 0x0420u : 0x0320u);
        OpLoadA(cpu, 255u);
        OpMoveNext(memory, cpu, 0x00u, 0xa6u);
    }
    PullDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, 0x73u));
    if (!ActionCall(&action, 0x84b5a2u, 0x84a5a8u))
        return action.result;
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0x0296u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x54u), cpu->x);
    OpLoadA(cpu, 0x2000u);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLoadA(cpu, 0x2000u);
    OpSta(memory, cpu, OpDp(cpu, 2u));
    if (!ActionCall(&action, 0x84b5b6u, 0x84a569u))
        return action.result;
    OpLdx(cpu, 0x0297u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x54u), cpu->x);
    OpLoadA(cpu, 0x2000u);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLoadA(cpu, 0x4000u);
    OpSta(memory, cpu, OpDp(cpu, 2u));
    if (!ActionCall(&action, 0x84b5c8u, 0x84a569u) ||
        !ActionCall(&action, 0x84b5cbu, 0x84a4ddu) ||
        !ActionCall(&action, 0x84b5ceu, 0x84a4fdu) ||
        !ActionCall(&action, 0x84b5d1u, 0x84b523u) ||
        !ActionCall(&action, 0x84b5d4u, 0x84b4a7u))
        return action.result;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x24u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    OpLoadA(cpu, 0x84u);
    OpSta(memory, cpu, OpDp(cpu, 0x5fu));
    if (!ActionCall(&action, 0x84b5e2u, 0x84af11u))
        return action.result;
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, 0x74u));
    OpSta(memory, cpu, OpDp(cpu, 0x72u));
    if (!ActionCall(&action, 0x84b5ebu, 0x84a5a8u))
        return action.result;
    OpLoadA(cpu, 40u);
    OpSta(memory, cpu, OpDp(cpu, 0x74u));
    if (!ActionCall(&action, 0x84b5f2u, 0x84a5a8u) ||
        !ActionCall(&action, 0x84b5f5u, 0x84a5b5u))
        return action.result;
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, WRAM_SPECIAL_MENU_SCENE_STATE);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x84b602u);
}

Lufia2ExecutionResult Lufia2CardGameDealOpeningHands(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !CardActionReady(cpu, false))
        return ExecutionHandoff(cpu, 0x84af4bu);
    CardAction action = {memory, cpu, child, context,
                         ExecutionReturned(0x84afb7u), 0u};
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x84u);
    OpSta(memory, cpu, OpDp(cpu, 0x1bu));
    OpLdx(cpu, 0xb6edu);
    OpWriteX(memory, cpu, OpDp(cpu, 0x19u), cpu->x);
    OpRepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpDp(cpu, 0x13u));
    OpStz(memory, cpu, OpDp(cpu, 0x15u));
    OpStz(memory, cpu, OpDp(cpu, 0x28u));
    OpStz(memory, cpu, OpDp(cpu, 2u));
    OpStepMem(memory, cpu, OpDp(cpu, 2u), 1);
    static const uint32_t animations[] = {0x84af62u, 0x84af65u, 0x84af68u};
    for (unsigned hand = 0u; hand < 3u; ++hand)
        if (!ActionCall(&action, animations[hand], 0x84afb8u))
            return action.result;
    OpStz(memory, cpu, OpDp(cpu, 2u));
    OpStepMem(memory, cpu, OpDp(cpu, 2u), 1);
    OpLdx(cpu, 0x5828u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x24u), cpu->x);
    static const uint32_t deals[] = {0x84af74u, 0x84af77u, 0x84af7au};
    for (unsigned hand = 0u; hand < 3u; ++hand)
        if (!ActionCall(&action, deals[hand], 0x84affcu))
            return action.result;
    if (!ActionCall(&action, 0x84af7du, 0x84b027u) ||
        !ActionCall(&action, 0x84af80u, 0x84adc0u))
        return action.result;
    OpStz(memory, cpu, OpDp(cpu, 2u));
    if (!ActionCall(&action, 0x84af85u, 0x84adf3u) ||
        !ActionCall(&action, 0x84af88u, 0x84b54fu))
        return action.result;
    OpLoadA(cpu, 0x0084u);
    OpSta(memory, cpu, OpDp(cpu, 0x1bu));
    OpLdx(cpu, 0xb6fcu);
    OpWriteX(memory, cpu, OpDp(cpu, 0x19u), cpu->x);
    OpStz(memory, cpu, OpDp(cpu, 2u));
    OpStz(memory, cpu, OpDp(cpu, 0x13u));
    if (!ActionCall(&action, 0x84af99u, 0x84afb8u))
        return action.result;
    OpStepMem(memory, cpu, OpDp(cpu, 2u), -1);
    OpLdx(cpu, 0x3060u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x24u), cpu->x);
    if (!ActionCall(&action, 0x84afa3u, 0x84affcu))
        return action.result;
    OpStepMem(memory, cpu, OpDp(cpu, 2u), -1);
    OpStepMem(memory, cpu, OpAbs(cpu, CARD_COUNTS + 6u), 1);
    OpLda(memory, cpu, OpDp(cpu, 0x22u));
    OpSta(memory, cpu, OpAbs(cpu, CARD_HANDS + 30u));
    OpSta(memory, cpu, OpDp(cpu, 0x22u));
    if (!ActionCall(&action, 0x84afb2u, 0x84b11eu))
        return action.result;
    OpRepWidths(cpu, 0x20u);
    return ExecutionReturned(0x84afb7u);
}

static void LoadCardDealPath(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, 0x17u));
    cpu->carry = 0u;
    RolA16(cpu);
    OpTax(cpu);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    RolA16(cpu);
    OpAdc(memory, cpu, OpDp(cpu, 0u));
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLda(memory, cpu, OpAbsX(cpu, CARD_COUNTS));
    OpDecA(cpu);
    OpAndValue(cpu, 0x00ffu);
    cpu->carry = 0u;
    RolA16(cpu);
    RolA16(cpu);
    RolA16(cpu);
    OpSta(memory, cpu, OpDp(cpu, 2u));
    RolA16(cpu);
    OpAdc(memory, cpu, OpDp(cpu, 2u));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, 0u));
    OpTay(cpu);
    OpStz(memory, cpu, OpDp(cpu, 2u));
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, 0x19u));
    OpSta(memory, cpu, OpDp(cpu, 0x24u));
    OpIny(cpu);
    OpIny(cpu);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, 0x19u));
    OpSta(memory, cpu, OpDp(cpu, 0x11u));
    OpIny(cpu);
    OpIny(cpu);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, 0x19u));
    OpSta(memory, cpu, OpDp(cpu, 4u));
    OpStz(memory, cpu, OpDp(cpu, 0x28u));
}

Lufia2ExecutionResult Lufia2CardGameAnimateDeal(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !CardActionReady(cpu, false))
        return ExecutionHandoff(cpu, 0x84b18eu);
    CardAction action = {memory, cpu, child, context,
                         ExecutionReturned(0x84b22au), 0u};
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x84u);
    OpSta(memory, cpu, OpDp(cpu, 0x1bu));
    OpLdx(cpu, 0xb719u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x19u), cpu->x);
    OpRepWidths(cpu, 0x20u);
    LoadCardDealPath(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 32u);
    if (!ActionCall(&action, 0x84b1d3u, 0x84a607u) ||
        !ActionCall(&action, 0x84b1d6u, 0x84b2bcu) ||
        !ActionCall(&action, 0x84b1d9u, 0x84b3c9u))
        return action.result;
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, 0x72u));
    if (!ActionCall(&action, 0x84b1e0u, 0x84a5a8u))
        return action.result;
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x11u)));
    OpWriteX(memory, cpu, OpDp(cpu, 0u), cpu->x);
    do {
        if (!ActionCall(&action, 0x84b1e7u, 0x84b43bu))
            return action.result;
        OpLoadA(cpu, 0x80u);
        OpSta(memory, cpu, OpDp(cpu, 0x72u));
        if (!ActionCall(&action, 0x84b1eeu, 0x84a5a8u))
            return action.result;
        OpStepMem(memory, cpu, OpDp(cpu, 4u), -1);
    } while (!cpu->zero);
    OpRepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpDp(cpu, 0x28u));
    if (!ActionCall(&action, 0x84b1f9u, 0x84b295u))
        return action.result;
    OpLda(memory, cpu, OpDp(cpu, 0x24u));
    OpSta(memory, cpu, OpDp(cpu, 0x0bu));
    OpLda(memory, cpu, OpDp(cpu, 0x17u));
    OpDecA(cpu);
    if (cpu->negative) {
        OpStepMem(memory, cpu, OpDp(cpu, 0x28u), 1);
        if (!ActionCall(&action, 0x84b20au, 0x84b22bu))
            return action.result;
    }
    if (!ActionCall(&action, 0x84b20du, 0x84b274u))
        return action.result;
    OpLda(memory, cpu, OpDp(cpu, 0x0bu));
    OpSta(memory, cpu, OpDp(cpu, 0x24u));
    if (!ActionCall(&action, 0x84b214u, 0x84af1cu) ||
        !ActionCall(&action, 0x84b217u, 0x84b2bcu) ||
        !ActionCall(&action, 0x84b21au, 0x84b387u))
        return action.result;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, 0x72u));
    OpSta(memory, cpu, OpDp(cpu, 0x74u));
    if (!ActionCall(&action, 0x84b225u, 0x84a5a8u))
        return action.result;
    OpRepWidths(cpu, 0x20u);
    return ExecutionReturned(0x84b22au);
}

static void LimitStakeToBalance(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, CARD_BALANCE + 2u));
    if (!cpu->zero)
        return;
    OpLda(memory, cpu, OpAbs(cpu, CARD_BALANCE));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, CARD_RESERVED_STAKE));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0x0bu)));
    if (cpu->carry)
        return;
    OpLda(memory, cpu, OpAbs(cpu, CARD_BALANCE));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, CARD_RESERVED_STAKE));
    OpSta(memory, cpu, OpAbs(cpu, CARD_BALANCE));
    OpSta(memory, cpu, OpDp(cpu, 0x0bu));
    OpStz(memory, cpu, OpAbs(cpu, CARD_RESERVED_STAKE));
}

static bool ShowPendingStake(CardAction *action) {
    const Lufia2Memory *memory = action->memory;
    Lufia2CpuState *cpu = action->cpu;
    OpLda(memory, cpu, OpDp(cpu, 0x0bu));
    if (!ActionCall(action, 0x84acd7u, 0x84c31bu))
        return false;
    OpLda(memory, cpu, OpDp(cpu, 0x0bu));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 2u)));
    OpSta(memory, cpu, OpDp(cpu, 2u));
    OpStz(memory, cpu, OpDp(cpu, 0x2du));
    OpLda(memory, cpu, OpDp(cpu, 0u));
    OpLdy(cpu, 0x8870u);
    OpLoadA(cpu, 0x3600u);
    if (!ActionCall(action, 0x84acebu, 0x84c2ceu))
        return false;
    OpLda(memory, cpu, OpDp(cpu, 2u));
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLdy(cpu, 0x8880u);
    OpLoadA(cpu, 0x3400u);
    if (!ActionCall(action, 0x84acf8u, 0x84c2ceu))
        return false;
    OpLdx(cpu, 0x3416u);
    OpLdy(cpu, 0xf954u);
    return ActionCall(action, 0x84ad01u, 0x808878u);
}

static bool ReservePendingStake(CardAction *action) {
    const Lufia2Memory *memory = action->memory;
    Lufia2CpuState *cpu = action->cpu;
    OpLda(memory, cpu, OpAbs(cpu, CARD_RESERVED_STAKE));
    OpCmp(memory, cpu, OpDp(cpu, 0x0bu));
    if (cpu->zero)
        return true;
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 33u);
    if (!ActionCall(action, 0x84ad11u, 0x84a607u))
        return false;
    UnpackStatus(cpu, Pull8(memory, cpu));
    OpLda(memory, cpu, OpAbs(cpu, CARD_RESERVED_STAKE));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0x0bu)));
    if (!cpu->negative) {
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpAbs(cpu, CARD_BALANCE));
        OpSta(memory, cpu, OpAbs(cpu, CARD_BALANCE));
        if (cpu->carry)
            OpStepMem(memory, cpu, OpAbs(cpu, CARD_BALANCE + 2u), 1);
    } else {
        OpLda(memory, cpu, OpDp(cpu, 0x0bu));
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, CARD_RESERVED_STAKE)));
        OpSta(memory, cpu, OpDp(cpu, 0u));
        OpLda(memory, cpu, OpAbs(cpu, CARD_BALANCE));
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0u)));
        OpSta(memory, cpu, OpAbs(cpu, CARD_BALANCE));
        if (!cpu->carry)
            OpStepMem(memory, cpu, OpAbs(cpu, CARD_BALANCE + 2u), -1);
    }
    return true;
}

Lufia2ExecutionResult Lufia2CardGameChooseStake(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !CardActionReady(cpu, false))
        return ExecutionHandoff(cpu, 0x84ac66u);
    CardAction action = {memory, cpu, child, context,
                         ExecutionReturned(0x84ad9cu), 0u};
    OpLoadA(cpu, 99u);
    OpSta(memory, cpu, OpAbs(cpu, CARD_STAKE_LIMIT));
    OpLda(memory, cpu, OpAbs(cpu, CARD_STAKE));
    OpStz(memory, cpu, OpAbs(cpu, CARD_RESERVED_STAKE));
    OpSta(memory, cpu, OpDp(cpu, 0x0bu));
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLdx(cpu, 0x3416u);
    OpLdy(cpu, 0xf954u);
    if (!ActionCall(&action, 0x84ac7cu, 0x808878u))
        return action.result;
    OpLoadA(cpu, 10u);
    OpSta(memory, cpu, OpAbs(cpu, CARD_WINDOW));
    OpLoadA(cpu, 15u);
    OpSta(memory, cpu, OpAbs(cpu, CARD_WINDOW + 2u));
    OpLoadA(cpu, 12u);
    OpSta(memory, cpu, OpAbs(cpu, CARD_WINDOW + 4u));
    OpLoadA(cpu, 6u);
    OpSta(memory, cpu, OpAbs(cpu, CARD_WINDOW + 6u));
    if (!ActionCall(&action, 0x84ac98u, 0x84a2b3u))
        return action.result;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x88u);
    OpSta(memory, cpu, OpDp(cpu, 0x74u));
    if (!ActionCall(&action, 0x84aca1u, 0x84a5a8u))
        return action.result;
    do {
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpDp(cpu, 0x0bu));
        if (cpu->zero)
            OpStepMem(memory, cpu, OpDp(cpu, 0x0bu), 1);
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, 33u);
        if (!ActionCall(&action, 0x84acb0u, 0x84a607u))
            return action.result;
        OpRepWidths(cpu, 0x20u);
        LimitStakeToBalance(memory, cpu);
        if (!ShowPendingStake(&action) || !ReservePendingStake(&action) ||
            !ActionCall(&action, 0x84ad41u, 0x84af11u))
            return action.result;
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, 0x80u);
        OpSta(memory, cpu, OpDp(cpu, 0x72u));
        OpLoadA(cpu, 8u);
        OpSta(memory, cpu, OpDp(cpu, 0x74u));
        if (!ActionCall(&action, 0x84ad4eu, 0x84a5a8u))
            return action.result;
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpDp(cpu, 0x0bu));
        OpSta(memory, cpu, OpAbs(cpu, CARD_RESERVED_STAKE));
        if (!ActionCall(&action, 0x84ad58u, 0x84c350u))
            return action.result;
        OpCmpValue(cpu, 1u);
    } while (!cpu->zero);
    OpLda(memory, cpu, OpDp(cpu, 0x0bu));
    OpSta(memory, cpu, OpAbs(cpu, CARD_STAKE));
    OpSta(memory, cpu, OpAbs(cpu, CARD_ACCEPTED_STAKE));
    if (!ActionCall(&action, 0x84ad6bu, 0x84c243u) ||
        !ActionCall(&action, 0x84ad6eu, 0x84c26fu))
        return action.result;
    OpLdx(cpu, 0u);
    OpLdy(cpu, 12u);
    OpLoadA(cpu, 0xf000u);
    do {
        OpSta(memory, cpu, OpAbsX(cpu, 0x0100u));
        OpInx(cpu);
        OpInx(cpu);
        OpStz(memory, cpu, OpAbsX(cpu, 0x0100u));
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    if (!ActionCall(&action, 0x84ad87u, 0x84a39bu) ||
        !ActionCall(&action, 0x84ad8au, 0x84af11u))
        return action.result;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x88u);
    OpSta(memory, cpu, OpDp(cpu, 0x74u));
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, 0x72u));
    if (!ActionCall(&action, 0x84ad97u, 0x84a5a8u))
        return action.result;
    OpRepWidths(cpu, 0x20u);
    return ExecutionReturned(0x84ad9cu);
}

static void StoreStakeCommand(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSta(memory, cpu, OpDp(cpu, 0x0bu));
    OpLoadA(cpu, 3u);
}

static void ClampStakeCommand(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpCmp(memory, cpu, OpAbs(cpu, CARD_STAKE_LIMIT));
    if (!cpu->negative)
        OpLda(memory, cpu, OpAbs(cpu, CARD_STAKE_LIMIT));
    StoreStakeCommand(memory, cpu);
}

static bool ApplyStakeDirection(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    ExchangeAccumulatorBytes(cpu);
    RorA8(cpu);
    if (cpu->carry) {
        OpLda(memory, cpu, OpDp(cpu, 0x0bu));
        cpu->carry = 0u;
        OpAdcValue(cpu, 10u);
        ClampStakeCommand(memory, cpu);
        return true;
    }
    RorA8(cpu);
    if (cpu->carry) {
        OpLda(memory, cpu, OpDp(cpu, 0x0bu));
        cpu->carry = 1u;
        OpSbcValue(cpu, 10u);
        if (cpu->negative)
            OpLoadA(cpu, 0u);
        StoreStakeCommand(memory, cpu);
        return true;
    }
    RorA8(cpu);
    if (cpu->carry) {
        OpLda(memory, cpu, OpDp(cpu, 0x0bu));
        OpIncA(cpu);
        ClampStakeCommand(memory, cpu);
        return true;
    }
    RorA8(cpu);
    if (cpu->carry) {
        OpLda(memory, cpu, OpDp(cpu, 0x0bu));
        OpDecA(cpu);
        if (cpu->negative)
            OpIncA(cpu);
        StoreStakeCommand(memory, cpu);
        return true;
    }
    return false;
}

Lufia2ExecutionResult Lufia2CardGameReadStakeCommand(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !CardActionReady(cpu, false))
        return ExecutionHandoff(cpu, 0x84c350u);
    CardAction action = {memory, cpu, child, context,
                         ExecutionReturned(0x84c3b3u), 0u};
    for (;;) {
        if (!ActionCall(&action, 0x84c350u, 0x849f1au))
            return action.result;
        OpCmpValue(cpu, 0u);
        if (cpu->zero)
            continue;
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
        OpCmpValue(cpu, 6u);
        if (cpu->zero) {
            OpLoadA(cpu, 0u);
            StoreStakeCommand(memory, cpu);
        } else {
            OpCmpValue(cpu, 5u);
            if (cpu->zero) {
                OpLda(memory, cpu, OpAbs(cpu, CARD_STAKE_LIMIT));
                StoreStakeCommand(memory, cpu);
            } else if (!ApplyStakeDirection(memory, cpu)) {
                OpRepWidths(cpu, 0x20u);
                OpTxa(cpu);
                OpCmpValue(cpu, 1u);
                if (!cpu->zero) {
                    OpCmpValue(cpu, 2u);
                    if (!cpu->zero)
                        continue;
                }
            }
        }
        OpRepWidths(cpu, 0x20u);
        return ExecutionReturned(0x84c3b3u);
    }
}
