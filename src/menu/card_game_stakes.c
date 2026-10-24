#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/card_game.h"
#include "system/wram.h"

enum {
    STAKE_PENDING = WRAM_CARD_GAME_RESERVED_STAKE & 0xffffu,
    STAKE_ACCEPTED = WRAM_CARD_GAME_ACCEPTED_STAKE & 0xffffu,
    STAKE_BALANCE = WRAM_MENU_EXCHANGE_BALANCE & 0xffffu,
    STAKE_MULTIPLY_LEFT = WRAM_SPECIAL_MENU_MULTIPLY_LEFT & 0xffffu,
    STAKE_MULTIPLY_RIGHT = WRAM_SPECIAL_MENU_MULTIPLY_RIGHT & 0xffffu,
    STAKE_PRODUCT = WRAM_SPECIAL_MENU_MULTIPLY_RESULT & 0xffffu,
    STAKE_OAM = WRAM_FIELD_OAM_LOW_BUFFER & 0xffffu,
    STAKE_TILEMAP = 0x2000u,
    STAKE_SELECTION = 0x0bu,
    STAKE_OAM_CURSOR = 0x2du,
    STAKE_FRAME_LIMIT = 65536u
};

typedef struct StakeDisplay {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
    unsigned calls;
} StakeDisplay;

static bool StakeReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x84u && !cpu->accumulator_is_8_bit &&
           !cpu->index_is_8_bit && !cpu->direct_page && !cpu->decimal;
}

static bool StakeCall(StakeDisplay *display, uint32_t site, uint32_t target) {
    Lufia2CpuState *cpu = display->cpu;
    const bool byte_accumulator = cpu->accumulator_is_8_bit != 0u;
    if (display->calls++ == STAKE_FRAME_LIMIT) {
        display->result = ExecutionHandoff(cpu, site);
        return false;
    }
    if (!CallChildWithFrame(display->memory, cpu, display->child,
                            display->context, site, target, 2u, 0x84u)) {
        display->result = ExecutionReturned(site);
        display->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (cpu->program_bank != 0x84u || cpu->index_is_8_bit ||
        cpu->direct_page || cpu->decimal ||
        cpu->accumulator_is_8_bit != byte_accumulator) {
        display->result = ExecutionHandoff(cpu, site + 3u);
        return false;
    }
    return true;
}

Lufia2ExecutionResult Lufia2CardGameRefundStake(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !StakeReady(cpu))
        return ExecutionHandoff(cpu, 0x84c1e8u);
    StakeDisplay display = {memory, cpu, child, context,
                            ExecutionReturned(0x84c209u), 0u};
    OpLda(memory, cpu, OpAbs(cpu, STAKE_PENDING));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, STAKE_SELECTION)));
    OpSta(memory, cpu, OpAbs(cpu, STAKE_MULTIPLY_LEFT));
    OpLoadA(cpu, 10u);
    OpSta(memory, cpu, OpAbs(cpu, STAKE_MULTIPLY_RIGHT));
    if (!StakeCall(&display, 0x84c1f7u, 0x84a5dfu))
        return display.result;
    OpLda(memory, cpu, OpAbs(cpu, STAKE_BALANCE));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, STAKE_PRODUCT));
    OpSta(memory, cpu, OpAbs(cpu, STAKE_BALANCE));
    if (cpu->carry)
        OpStepMem(memory, cpu, OpAbs(cpu, STAKE_BALANCE + 2u), 1);
    return display.result;
}

Lufia2ExecutionResult Lufia2CardGameChargeStake(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !StakeReady(cpu))
        return ExecutionHandoff(cpu, 0x84c20au);
    StakeDisplay display = {memory, cpu, child, context,
                            ExecutionReturned(0x84c22bu), 0u};
    OpLda(memory, cpu, OpDp(cpu, STAKE_SELECTION));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, STAKE_PENDING)));
    OpSta(memory, cpu, OpAbs(cpu, STAKE_MULTIPLY_LEFT));
    OpLoadA(cpu, 10u);
    OpSta(memory, cpu, OpAbs(cpu, STAKE_MULTIPLY_RIGHT));
    if (!StakeCall(&display, 0x84c219u, 0x84a5dfu))
        return display.result;
    OpLda(memory, cpu, OpAbs(cpu, STAKE_BALANCE));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, STAKE_PRODUCT)));
    OpSta(memory, cpu, OpAbs(cpu, STAKE_BALANCE));
    if (!cpu->carry)
        OpStepMem(memory, cpu, OpAbs(cpu, STAKE_BALANCE + 2u), -1);
    return display.result;
}

Lufia2ExecutionResult Lufia2CardGameClearStakeSprites(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!StakeReady(cpu))
        return ExecutionHandoff(cpu, 0x84c22cu);
    OpLdx(cpu, 0u);
    OpLdy(cpu, 12u);
    OpLoadA(cpu, 0xf000u);
    do {
        OpSta(memory, cpu, OpAbsX(cpu, STAKE_OAM));
        OpInx(cpu);
        OpInx(cpu);
        OpStz(memory, cpu, OpAbsX(cpu, STAKE_OAM));
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    return ExecutionReturned(0x84c242u);
}

Lufia2ExecutionResult Lufia2CardGameAnimateStakeSprites(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !StakeReady(cpu))
        return ExecutionHandoff(cpu, 0x84c243u);
    StakeDisplay display = {memory, cpu, child, context,
                            ExecutionReturned(0x84c26eu), 0u};
    OpLoadA(cpu, 23u);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    do {
        OpLdx(cpu, 0u);
        OpLdy(cpu, 11u);
        do {
            OpLda(memory, cpu, OpAbsX(cpu, STAKE_OAM));
            cpu->carry = 0u;
            OpAdcValue(cpu, 0x0204u);
            OpSta(memory, cpu, OpAbsX(cpu, STAKE_OAM));
            for (unsigned byte = 0u; byte < 4u; ++byte)
                OpInx(cpu);
            OpDey(cpu);
        } while (!cpu->negative);
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, 0x80u);
        OpSta(memory, cpu, OpDp(cpu, 0x72u));
        if (!StakeCall(&display, 0x84c265u, 0x84a5a8u))
            return display.result;
        OpRepWidths(cpu, 0x20u);
        OpStepMem(memory, cpu, OpDp(cpu, 0u), -1);
    } while (!cpu->negative);
    return display.result;
}

static void LocateStakeDigit(const Lufia2Memory *memory,
                              Lufia2CpuState *cpu) {
    OpSta(memory, cpu, OpDp(cpu, 4u));
    OpLda(memory, cpu, OpDp(cpu, 0u));
    cpu->carry = 0u;
    RolA16(cpu);
    RolA16(cpu);
    OpSta(memory, cpu, OpDp(cpu, 0x1cu));
    RolA16(cpu);
    OpAdc(memory, cpu, OpDp(cpu, 0x1cu));
    OpTax(cpu);
}

static void DrawStakeTileRows(const Lufia2Memory *memory,
                               Lufia2CpuState *cpu) {
    LocateStakeDigit(memory, cpu);
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpDp(cpu, 0x1cu));
    do {
        OpLda(memory, cpu, OpLongX(cpu, 0x84a611u));
        OpOra(memory, cpu, OpDp(cpu, 4u));
        OpSta(memory, cpu, OpAbsY(cpu, STAKE_TILEMAP));
        OpInx(cpu);
        OpInx(cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpLda(memory, cpu, OpLongX(cpu, 0x84a611u));
        OpOra(memory, cpu, OpDp(cpu, 4u));
        OpSta(memory, cpu, OpAbsY(cpu, STAKE_TILEMAP));
        OpInx(cpu);
        OpInx(cpu);
        OpTya(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, 62u);
        OpTay(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, 0x1cu), -1);
    } while (!cpu->negative);
}

Lufia2ExecutionResult Lufia2CardGameDrawStakeTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!StakeReady(cpu))
        return ExecutionHandoff(cpu, 0x84c299u);
    DrawStakeTileRows(memory, cpu);
    return ExecutionReturned(0x84c2cdu);
}

Lufia2ExecutionResult Lufia2CardGameDrawAcceptedStake(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !StakeReady(cpu))
        return ExecutionHandoff(cpu, 0x84c26fu);
    StakeDisplay display = {memory, cpu, child, context,
                            ExecutionReturned(0x84c2cdu), 0u};
    OpLda(memory, cpu, OpAbs(cpu, STAKE_ACCEPTED));
    OpCmpValue(cpu, 99u);
    if (!cpu->negative)
        OpLoadA(cpu, 99u);
    OpSta(memory, cpu, OpDp(cpu, 0x0eu));
    if (!StakeCall(&display, 0x84c27cu, 0x84c31bu))
        return display.result;
    OpLda(memory, cpu, OpDp(cpu, 0x0eu));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 2u)));
    OpSta(memory, cpu, OpDp(cpu, 2u));
    OpLdy(cpu, 0x05f4u);
    OpLoadA(cpu, 0x0c00u);
    if (!StakeCall(&display, 0x84c28cu, 0x84c299u))
        return display.result;
    OpLda(memory, cpu, OpDp(cpu, 2u));
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLdy(cpu, 0x05f8u);
    OpLoadA(cpu, 0x0800u);
    DrawStakeTileRows(memory, cpu);
    return display.result;
}

Lufia2ExecutionResult Lufia2CardGameDrawStakeSprites(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!StakeReady(cpu))
        return ExecutionHandoff(cpu, 0x84c2ceu);
    LocateStakeDigit(memory, cpu);
    OpWriteX(memory, cpu, OpDp(cpu, 6u), cpu->y);
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpDp(cpu, 0x1cu));
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, STAKE_OAM_CURSOR)));
    do {
        OpLda(memory, cpu, OpDp(cpu, 6u));
        OpSta(memory, cpu, OpAbsY(cpu, STAKE_OAM));
        cpu->carry = 0u;
        OpAdcValue(cpu, 8u);
        OpSta(memory, cpu, OpAbsY(cpu, STAKE_OAM + 4u));
        OpLda(memory, cpu, OpLongX(cpu, 0x84a611u));
        OpOra(memory, cpu, OpDp(cpu, 4u));
        OpSta(memory, cpu, OpAbsY(cpu, STAKE_OAM + 2u));
        OpInx(cpu);
        OpInx(cpu);
        OpLda(memory, cpu, OpLongX(cpu, 0x84a611u));
        OpOra(memory, cpu, OpDp(cpu, 4u));
        OpSta(memory, cpu, OpAbsY(cpu, STAKE_OAM + 6u));
        OpInx(cpu);
        OpInx(cpu);
        OpTya(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, 8u);
        OpTay(cpu);
        OpLda(memory, cpu, OpDp(cpu, 6u));
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x0800u);
        OpSta(memory, cpu, OpDp(cpu, 6u));
        OpStepMem(memory, cpu, OpDp(cpu, 0x1cu), -1);
    } while (!cpu->negative);
    OpWriteX(memory, cpu, OpDp(cpu, STAKE_OAM_CURSOR), cpu->y);
    return ExecutionReturned(0x84c31au);
}
