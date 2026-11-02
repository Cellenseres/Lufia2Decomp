#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum {
    RIPPLE_PHASE = WRAM_BATTLE_RIPPLE_PHASE & 0xffffu,
    RIPPLE_STATE = WRAM_UNK_7E1B19 & 0xffffu,
    RIPPLE_HDMA_MODE = WRAM_BATTLE_RIPPLE_HDMA_MODE & 0xffffu,
    RIPPLE_HDMA_REGISTER = WRAM_BATTLE_RIPPLE_HDMA_REGISTER & 0xffffu,
    RIPPLE_HDMA_SOURCE = WRAM_BATTLE_RIPPLE_HDMA_SOURCE & 0xffffu,
    RIPPLE_HDMA_SOURCE_BANK = WRAM_BATTLE_RIPPLE_HDMA_SOURCE_BANK & 0xffffu,
    RIPPLE_HDMA_SOURCE_ADDRESS = 0x9f17u,
    RIPPLE_TEMPLATE = 0x859f80u,
    RIPPLE_TEMPLATE_SIZE = 23u,
    RIPPLE_SPLIT_SOURCE = 0x4408u,
    RIPPLE_SCROLL_TABLE = 0x7e4400u,
    RIPPLE_SCROLL_REFERENCE = 0x210du,
    RIPPLE_HORIZONTAL_SCROLL = WRAM_NMI_SCROLL_REGISTERS,
    RIPPLE_VERTICAL_SCROLL = WRAM_NMI_SCROLL_REGISTERS + 2u,
    RIPPLE_SCROLL_SCRATCH = 0x33u,
    RIPPLE_MODE = 0x11e5u,
    RIPPLE_MODE_TABLE = 0x81bad4u,
    DP_HDMA_ENABLE = 0xdau,
    DP_HDMA_UPDATE = 0xdbu
};

static void EnableRippleDescriptor(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t source, uint8_t bank) {
    OpLoadA(cpu, 0x42u);
    OpSta(memory, cpu, OpAbs(cpu, RIPPLE_HDMA_MODE));
    OpLoadA(cpu, 0x0du);
    OpSta(memory, cpu, OpAbs(cpu, RIPPLE_HDMA_REGISTER));
    OpLdx(cpu, source);
    OpWriteX(memory, cpu, OpAbs(cpu, RIPPLE_HDMA_SOURCE), cpu->x);
    OpLoadA(cpu, bank);
    OpSta(memory, cpu, OpAbs(cpu, RIPPLE_HDMA_SOURCE_BANK));
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_DASB(0u)));
    OpLoadA(cpu, 1u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_HDMA_ENABLE), 1u);
    OpStz(memory, cpu, OpAbs(cpu, RIPPLE_STATE));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E1B17));
    OpLoadA(cpu, 1u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_HDMA_UPDATE), 1u);
}

Lufia2ExecutionResult Lufia2BattleInitializeRipple(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x85a701u);
    OpLdy(cpu, 0u);
    OpTyx(cpu);
    OpWriteX(memory, cpu, OpAbs(cpu, RIPPLE_PHASE), cpu->y);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x85a708u, 0x85a736u, 2u, 0x85u)) {
        Lufia2ExecutionResult result = ExecutionReturned(0x85a708u);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    EnableRippleDescriptor(memory, cpu, RIPPLE_HDMA_SOURCE_ADDRESS, 0x85u);
    return ExecutionReturned(0x85a735u);
}

static bool SplitRippleReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x85u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->direct_page &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static void BuildSplitRippleTable(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, RIPPLE_PHASE));
    OpSta(memory, cpu, OpDp(cpu, RIPPLE_SCROLL_SCRATCH));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, RIPPLE_SCROLL_REFERENCE));
    OpSta(memory, cpu, RIPPLE_SCROLL_TABLE + 6u);
    for (unsigned layer = 0u; layer < 2u; ++layer) {
        OpLsrMem(memory, cpu, OpDp(cpu, RIPPLE_SCROLL_SCRATCH));
        OpLda(memory, cpu, OpAbs(cpu, RIPPLE_HORIZONTAL_SCROLL));
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpDp(cpu, RIPPLE_SCROLL_SCRATCH));
        OpSta(memory, cpu, RIPPLE_SCROLL_TABLE + 4u - layer * 2u);
    }
    OpLda(memory, cpu, OpDp(cpu, RIPPLE_SCROLL_SCRATCH));
    OpLsrA(cpu);
    OpAdc(memory, cpu, OpAbs(cpu, RIPPLE_SCROLL_REFERENCE));
    OpSta(memory, cpu, RIPPLE_SCROLL_TABLE);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x30u);
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, RIPPLE_VERTICAL_SCROLL)));
    OpSta(memory, cpu, RIPPLE_SCROLL_TABLE + 4u);
    OpLoadA(cpu, 0x48u);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, RIPPLE_VERTICAL_SCROLL));
    OpSta(memory, cpu, RIPPLE_SCROLL_TABLE + 0x13u);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_RIPPLE_ROW_DELAY));
    PullDataBank(memory, cpu);
}

Lufia2ExecutionResult Lufia2BattleBuildSplitRippleTable(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!SplitRippleReady(cpu))
        return ExecutionHandoff(cpu, 0x85a7b1u);
    BuildSplitRippleTable(memory, cpu);
    return ExecutionReturned(0x85a803u);
}

Lufia2ExecutionResult Lufia2BattleInitializeSplitRipple(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!SplitRippleReady(cpu))
        return ExecutionHandoff(cpu, 0x85a76eu);
    OpLdx(cpu, RIPPLE_TEMPLATE_SIZE - 1u);
    do {
        OpLda(memory, cpu, OpLongX(cpu, RIPPLE_TEMPLATE));
        OpSta(memory, cpu, OpLongX(cpu, RIPPLE_SCROLL_TABLE + 8u));
        OpDex(cpu);
    } while (!cpu->negative);
    OpLdy(cpu, 0u);
    OpTyx(cpu);
    OpWriteX(memory, cpu, OpAbs(cpu, RIPPLE_PHASE), cpu->y);
    SimulateJsrFrame(memory, cpu, 0xa785u);
    BuildSplitRippleTable(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    EnableRippleDescriptor(memory, cpu, RIPPLE_SPLIT_SOURCE, 0x7eu);
    return ExecutionReturned(0x85a7b0u);
}

Lufia2ExecutionResult Lufia2BattleSelectRippleMode(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x81bacbu);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpAbs(cpu, RIPPLE_MODE));
    OpAslA(cpu);
    OpTax(cpu);
    const uint16_t target = Read16Long(memory, RIPPLE_MODE_TABLE + cpu->x);
    if (target == 0xbadcu || target == 0xbae7u)
        return ExecutionReturned(0x810000u | target);
    if (target != 0xbaddu && target != 0xbae2u)
        return ExecutionHandoff(cpu, 0x810000u | target);
    const uint32_t destination = target == 0xbaddu ? 0x85a701u : 0x85a76eu;
    const uint32_t site = 0x810000u | target;
    if (!CallChildWithFrame(memory, cpu, child, context, site, destination, 3u, 0x81u)) {
        Lufia2ExecutionResult result = ExecutionReturned(site);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    const uint32_t returned = (uint32_t)cpu->program_bank << 16 | (uint16_t)(target + 4u);
    if (cpu->program_bank != 0x81u)
        return ExecutionHandoff(cpu, returned);
    return ExecutionReturned(returned);
}
