#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/plain_ops.h"
#include "lufia2/party.h"

enum {
    ROM_CAPSULE_ITEM_CHOICES = 0x95ff16u,
    ROM_CAPSULE_ITEM_LIMITS = 0x95ff18u,
    DP_ITEM_CHOICE_OFFSET = 0x54u
};

Lufia2ExecutionResult Lufia2CapsuleChooseMenuItem(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x82u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x82c554u);
    OpAndValue(cpu, 0xffu);
    OpAslA(cpu);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, ROM_CAPSULE_ITEM_CHOICES));
    PushStackWord(memory, cpu, cpu->accumulator);
    OpLda(memory, cpu, OpLongX(cpu, ROM_CAPSULE_ITEM_LIMITS));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82c563u, 0x808299u, 3u, 0x82u)) {
        Lufia2ExecutionResult result = ExecutionReturned(0x82c563u);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    OpAndValue(cpu, 0xffu);
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_ITEM_CHOICE_OFFSET));
    LoadA16(cpu, PullStackWord(memory, cpu));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, DP_ITEM_CHOICE_OFFSET));
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, ROM_CAPSULE_ITEM_CHOICES));
    return ExecutionReturned(0x82c576u);
}
