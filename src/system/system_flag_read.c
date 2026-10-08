#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/system.h"

enum {
    DP_EVENT_FLAG = 0x54u,
    DP_EVENT_FLAG_BYTE = 0x56u,
    DP_EVENT_FLAG_MASK = 0x57u,
    ROM_EVENT_FLAG_MASKS = 0x80be45u
};

static uint8_t EventFlagEntry(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x80u && !cpu->decimal;
}

static Lufia2ExecutionResult EventFlagUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2SystemResolveEventFlagBit(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!EventFlagEntry(cpu))
        return ExecutionHandoff(cpu, 0x80be30u);
    OpSepWidths(cpu, 0x30u);
    OpSta(memory, cpu, OpDp(cpu, DP_EVENT_FLAG));
    for (unsigned shift = 0; shift < 3u; ++shift)
        OpLsrA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_EVENT_FLAG_BYTE));
    OpLda(memory, cpu, OpDp(cpu, DP_EVENT_FLAG));
    OpAndValue(cpu, 7u);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, ROM_EVENT_FLAG_MASKS));
    OpSta(memory, cpu, OpDp(cpu, DP_EVENT_FLAG_MASK));
    return ExecutionReturned(0x80be44u);
}

Lufia2ExecutionResult Lufia2SystemTestEventFlag(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    if (!EventFlagEntry(cpu) || !cpu->accumulator_is_8_bit || cpu->stack < 4u || !child)
        return ExecutionHandoff(cpu, 0x80be1eu);
    PushY(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x80be20u, 0x80be30u, 2u, 0x80u))
        return EventFlagUnwound(0x80be20u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_EVENT_FLAG_BYTE)));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_EVENT_FLAGS));
    OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_EVENT_FLAG_MASK)));
    UnpackStatus(cpu, Pull8(memory, cpu));
    cpu->y = PullIndexValue(memory, cpu);
    OpOraValue(cpu, 0u);
    return ExecutionReturned(0x80be2fu);
}

Lufia2ExecutionResult Lufia2SystemTestEventFlagLong(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    if (!EventFlagEntry(cpu) || !cpu->accumulator_is_8_bit || cpu->stack < 6u || !child)
        return ExecutionHandoff(cpu, 0x80be1au);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x80be1au, 0x80be1eu, 2u, 0x80u))
        return EventFlagUnwound(0x80be1au);
    return ExecutionReturned(0x80be1du);
}
