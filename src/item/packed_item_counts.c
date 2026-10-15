#include "core/cpu_ops.h"
#include "lufia2/item.h"
#include "system/wram.h"

enum {
    DP_COUNT_AMOUNT = 0x54u,
    DP_LOW_COUNT = 0x56u,
    DP_HIGH_COUNT = 0x57u,
    PACKED_COUNT_MAX = 15u
};

static bool PackedCountReady(const Lufia2CpuState *cpu, uint16_t stack_min) {
    return cpu->program_bank == 0x82u && !cpu->accumulator_is_8_bit &&
           !cpu->index_is_8_bit && !cpu->decimal && !cpu->direct_page &&
           cpu->stack >= stack_min && cpu->stack <= 0x1ffcu;
}

Lufia2ExecutionResult Lufia2ItemSelectPackedCount(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu) {
    if (!PackedCountReady(cpu, 0x1f08u))
        return ExecutionHandoff(cpu, 0x82fcedu);
    OpAndValue(cpu, 0xffu);
    OpCmpValue(cpu, PACKED_COUNT_MAX);
    if (cpu->carry)
        OpLoadA(cpu, PACKED_COUNT_MAX);
    OpSta(memory, cpu, OpDp(cpu, DP_COUNT_AMOUNT));
    OpTxa(cpu);
    OpAndValue(cpu, 0x01ffu);
    OpLsrA(cpu);
    OpTax(cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, WRAM_UNK_7FF080));
    OpAndValue(cpu, 0x0fu);
    OpSta(memory, cpu, OpDp(cpu, DP_LOW_COUNT));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_UNK_7FF080));
    OpAndValue(cpu, 0xf0u);
    OpSta(memory, cpu, OpDp(cpu, DP_HIGH_COUNT));
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x82fd14u);
}

static Lufia2ExecutionResult AdjustPackedCount(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu, bool subtract) {
    SimulateJsrFrame(memory, cpu, subtract ? 0xfcc9u : 0xfc99u);
    const Lufia2ExecutionResult result = Lufia2ItemSelectPackedCount(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    SimulateRtsFrame(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    const bool high = cpu->carry != 0u;
    if (high)
        for (unsigned bit = 0u; bit < 4u; ++bit)
            AslDirect8(memory, cpu, DP_COUNT_AMOUNT);
    OpLda(memory, cpu, OpDp(cpu, high ? DP_HIGH_COUNT : DP_LOW_COUNT));
    cpu->carry = subtract ? 1u : 0u;
    const uint16_t amount = OpReadM(memory, cpu, OpDp(cpu, DP_COUNT_AMOUNT));
    if (subtract) {
        OpSbcValue(cpu, amount);
    } else {
        OpAdcValue(cpu, amount);
        if (high) {
            if (cpu->carry)
                OpLoadA(cpu, 0xf0u);
        } else {
            OpBitValue(cpu, 0xf0u);
            if (!cpu->zero)
                OpLoadA(cpu, PACKED_COUNT_MAX);
        }
    }
    OpOra(memory, cpu, OpDp(cpu, high ? DP_LOW_COUNT : DP_HIGH_COUNT));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7FF080));
    OpRepWidths(cpu, 0x20u);
    return ExecutionReturned(subtract ? 0x82fcecu : 0x82fcc6u);
}

Lufia2ExecutionResult Lufia2ItemIncrementPackedCount(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu) {
    if (!PackedCountReady(cpu, 0x1f0au))
        return ExecutionHandoff(cpu, 0x82fc97u);
    return AdjustPackedCount(memory, cpu, false);
}

Lufia2ExecutionResult Lufia2ItemDecrementPackedCount(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu) {
    if (!PackedCountReady(cpu, 0x1f0au))
        return ExecutionHandoff(cpu, 0x82fcc7u);
    return AdjustPackedCount(memory, cpu, true);
}
