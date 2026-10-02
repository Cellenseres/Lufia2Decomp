#include "core/cpu_ops.h"
#include "lufia2/ancient_cave.h"
#include "system/wram.h"

enum { CAVE_CLEAR_LENGTH = 0x58, CAVE_INITIAL_ITEM_BYTES = 0x8f };

static Lufia2ExecutionResult ResetChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target) {
    SimulateJslFrame(memory, cpu, 0x84u, (uint16_t)(site + 3u));
    if (!child(context, cpu, target, site, 3u)) {
        Lufia2ExecutionResult result = ExecutionReturned(site);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, site + 4u);
    return ExecutionReturned(site + 4u);
}

static Lufia2ExecutionResult ClearCaveParty(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    unsigned steps = 0;
    OpLdx(cpu, 0u);
    for (;;) {
        if (++steps > 65536u)
            return ExecutionHandoff(cpu, 0x84888eu);
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbsX(cpu, 0x8b59u));
        OpCmpValue(cpu, 0xffffu);
        if (cpu->zero)
            break;
        OpSta(memory, cpu, OpAbs(cpu, 0x2181u));
        OpLda(memory, cpu, OpAbsX(cpu, 0x8b5cu));
        OpSta(memory, cpu, OpDp(cpu, CAVE_CLEAR_LENGTH));
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbsX(cpu, 0x8b5bu));
        OpSta(memory, cpu, OpAbs(cpu, 0x2183u));
        OpLdy(cpu, 0u);
        TransferDirectToA(cpu);
        do {
            if (++steps > 65536u)
                return ExecutionHandoff(cpu, 0x8488acu);
            OpSta(memory, cpu, OpAbs(cpu, 0x2180u));
            OpIny(cpu);
            OpCpy(cpu, OpRead16(memory, OpDp(cpu, CAVE_CLEAR_LENGTH)));
        } while (!cpu->carry);
        for (unsigned i = 0; i < 5u; ++i)
            OpInx(cpu);
    }
    OpLdx(cpu, 0x1402u);
    OpWrite16(memory, OpAbs(cpu, 0x0a8du), cpu->x);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x8488c3u);
}

static Lufia2ExecutionResult GiveCaveInitialItems(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    unsigned steps = 0;
    OpLdx(cpu, OpRead16(memory, OpDp(cpu, 0x60u)));
    OpWrite16(memory, OpDp(cpu, CAVE_INITIAL_ITEM_BYTES), cpu->x);
    OpLdx(cpu, 0u);
    for (;;) {
        if (++steps > 65536u)
            return ExecutionHandoff(cpu, 0x8488d0u);
        OpCpx(cpu, OpRead16(memory, OpDp(cpu, CAVE_INITIAL_ITEM_BYTES)));
        if (cpu->carry)
            break;
        PushIndex(memory, cpu);
        OpLda(memory, cpu, OpLongX(cpu, WRAM_ANCIENT_CAVE_SAVED_ITEMS));
        OpSta(memory, cpu, OpAbs(cpu, 0x09cfu));
        OpLda(memory, cpu, OpLongX(cpu, WRAM_ANCIENT_CAVE_SAVED_ITEMS + 1u));
        OpAndValue(cpu, 1u);
        OpSta(memory, cpu, OpAbs(cpu, 0x09d0u));
        OpLda(memory, cpu, OpLongX(cpu, WRAM_ANCIENT_CAVE_SAVED_ITEMS + 1u));
        OpLsrA(cpu);
        OpSta(memory, cpu, OpAbs(cpu, 0x09cdu));
        OpStz(memory, cpu, OpAbs(cpu, 0x09ceu));
        OpStz(memory, cpu, OpDp(cpu, 0x31u));
        OpLoadA(cpu, 4u);
        OpSta(memory, cpu, OpDp(cpu, 0x30u));
        Lufia2ExecutionResult result = ResetChild(
            memory, cpu, child, context, 0x8488f6u, 0x82e746u);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
        OpPullX(memory, cpu);
        OpInx(cpu);
        OpInx(cpu);
    }
    return ExecutionReturned(0x8488ffu);
}

Lufia2ExecutionResult Lufia2AncientCaveResetParty(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->program_bank != 0x84u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, ((uint32_t)cpu->program_bank << 16) | 0x8888u);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    Lufia2ExecutionResult result = ClearCaveParty(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpLda(memory, cpu, WRAM_ANCIENT_CAVE_INVENTORY_MODE);
    if (cpu->zero) {
        result = GiveCaveInitialItems(memory, cpu, child, context);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
    }
    OpSepWidths(cpu, 0x20u);
    result = ResetChild(memory, cpu, child, context, 0x848901u, 0x81ed35u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    result = ResetChild(memory, cpu, child, context, 0x848905u, 0x82c2fdu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x84890au);
}

static void ResetSetBits(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t offset) {
    uint32_t address = OpAbs(cpu, offset);
    uint16_t value = OpReadM(memory, cpu, address);
    cpu->zero = (value & OpA(cpu)) == 0;
    OpWriteM(memory, cpu, address, value | OpA(cpu));
}

Lufia2ExecutionResult Lufia2AncientCaveDefeat(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->program_bank != 0x84u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, ((uint32_t)cpu->program_bank << 16) | 0x8b9cu);
    EmitExecutionCheckpoint(memory, cpu, 0x848b9cu);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, WRAM_ANCIENT_CAVE_INVENTORY_MODE);
    SimulateJsrFrame(memory, cpu, 0x8ba4u);
    Lufia2ExecutionResult result = Lufia2AncientCaveResetParty(memory, cpu, child, context);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    uint8_t low = Pull8(memory, cpu), high = Pull8(memory, cpu);
    uint16_t frame = (uint16_t)(low | ((uint16_t)high << 8));
    if (frame != 0x8ba4u)
        return ExecutionHandoff(cpu, 0x840000u | (uint16_t)(frame + 1u));
    EmitExecutionCheckpoint(memory, cpu, 0x848ba5u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ANCIENT_CAVE_DEFEAT_COUNT & 0xffffu));
    OpIncA(cpu);
    if (cpu->zero)
        OpDecA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_ANCIENT_CAVE_DEFEAT_COUNT & 0xffffu));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0xf0u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_UNK_05C0 & 0xffffu));
    OpLoadA(cpu, 0x88u);
    ResetSetBits(memory, cpu, WRAM_FIELD_FLAGS & 0xffffu);
    OpLoadA(cpu, 0x40u);
    ResetSetBits(memory, cpu, WRAM_FIELD_RELOAD_FLAGS & 0xffffu);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_TRANSITION_SOURCE_MAP & 0xffffu));
    return ExecutionReturned(0x848bc6u);
}
