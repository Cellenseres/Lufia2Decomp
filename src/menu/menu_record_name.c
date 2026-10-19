#include "lufia2/menu.h"
#include "menu/menu_action_internal.h"

enum {
    DP_NAME_LENGTH = 0u,
    DP_NAME_OUTPUT = 8u,
    DP_NAME_OUTPUT_BANK = 10u,
    DP_NAME_SOURCE = 11u,
    NAME_CENTER_BUFFER = 0x4000u,
    NAME_LINE_WIDTH = 26u
};

static void ReadNameAtCursor(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, DP_NAME_SOURCE));
}

Lufia2ExecutionResult Lufia2MenuDrawCenteredRecordName(const Lufia2Memory *memory,
                                                       Lufia2CpuState *cpu,
                                                       Lufia2PushedChildCall child,
                                                       void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82ff3au);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, DP_NAME_OUTPUT_BANK));
    OpLdx(cpu, NAME_CENTER_BUFFER);
    OpWriteX(memory, cpu, OpDp(cpu, DP_NAME_OUTPUT), cpu->x);
    OpLdy(cpu, 0u);
    do {
        ReadNameAtCursor(memory, cpu);
        OpIny(cpu);
        OpCmpValue(cpu, 0u);
    } while (!cpu->zero);
    OpDey(cpu);
    OpDey(cpu);
    ReadNameAtCursor(memory, cpu);
    OpCmpValue(cpu, 0x20u);
    if (cpu->zero) {
        for (;;) {
            ReadNameAtCursor(memory, cpu);
            OpCmpValue(cpu, 0x20u);
            if (!cpu->zero)
                break;
            OpDey(cpu);
        }
    }
    OpRepWidths(cpu, 0x20u);
    OpIny(cpu);
    OpWriteX(memory, cpu, OpDp(cpu, DP_NAME_LENGTH), cpu->y);
    OpLoadA(cpu, NAME_LINE_WIDTH);
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_NAME_LENGTH)));
    OpLsrA(cpu);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLdy(cpu, 0u);
    OpCpx(cpu, 0u);
    if (!cpu->zero) {
        OpLoadA(cpu, 0x20u);
        do {
            OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, DP_NAME_OUTPUT));
            OpIny(cpu);
            OpDex(cpu);
        } while (!cpu->zero);
    }
    for (;;) {
        OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_NAME_SOURCE));
        OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, DP_NAME_OUTPUT));
        if (cpu->zero)
            break;
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_NAME_SOURCE)));
        OpInx(cpu);
        OpWriteX(memory, cpu, OpDp(cpu, DP_NAME_SOURCE), cpu->x);
        OpIny(cpu);
    }
    OpLdx(cpu, 0x1a01u);
    cpu->carry = 0u;
    if (!MenuActionChild(&call, 0x82ff8fu, 0x829073u, 2u, 1u))
        return call.result;
    OpLdx(cpu, 0x7000u);
    if (!MenuActionChild(&call, 0x82ff95u, 0x829113u, 2u, 1u))
        return call.result;
    return ExecutionReturned(0x82ff98u);
}
