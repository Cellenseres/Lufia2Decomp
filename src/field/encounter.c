/* Field encounter handoff at $83:83E0. */

#include "core/cpu_ops.h"
#include "lufia2/field.h"

Lufia2ExecutionResult Lufia2FieldEncounterHandoff(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context) {
    Lufia2ExecutionResult result;

    LoadA8(cpu, 0xffu);                                        /* 83E0 */
    OpSta(memory, cpu, 0x7ff8a3u);                             /* 83E2 */
    SimulateJslFrame(memory, cpu, 0x83u, 0x83e9u);             /* 83E6 */
    if (!child(child_context, cpu, 0x8383ebu, 0x8383e6u, 3u)) {
        result = ExecutionReturned(0x8383e6u);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    return ExecutionReturned(0x8383eau);                       /* RTS */
}
