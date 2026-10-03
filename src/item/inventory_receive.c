#include "core/cpu_ops.h"
#include "lufia2/item.h"
#include "system/wram.h"

static Lufia2ExecutionResult ReceiveUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

/* $81:F085: load the received item, add it, and report any remainder. */
Lufia2ExecutionResult Lufia2InventoryReceive(const Lufia2Memory *memory,
                                             Lufia2CpuState *cpu,
                                             Lufia2PushedChildCall child,
                                             void *context) {
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID)));
    OpWriteX(memory, cpu, OpAbs(cpu, 0x09f2u), cpu->x);
    OpLda(memory, cpu, OpAbs(cpu, (WRAM_ITEM_RECORD_ID + 1u)));
    OpLsrA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, 0x09f4u));
    SimulateJslFrame(memory, cpu, 0x81u, 0xf095u);
    if (!child(context, cpu, 0x81f1c5u, 0x81f092u, 3u))
        return ReceiveUnwound(0x81f092u);
    SimulateJsrFrame(memory, cpu, 0xf098u);
    if (!child(context, cpu, 0x81f0a2u, 0x81f096u, 2u))
        return ReceiveUnwound(0x81f096u);
    EmitExecutionCheckpoint(memory, cpu, 0x81f099u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID)));
    if (!cpu->zero) {
        cpu->carry = 1;
        return ExecutionReturned(0x81f09fu);
    }
    cpu->carry = 0;
    return ExecutionReturned(0x81f0a1u);
}
