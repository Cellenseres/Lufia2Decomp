#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/menu.h"
#include "system/wram.h"

enum { DP_SAVE_MEMBER_RESULT = 0x31u };

static Lufia2ExecutionResult SelectionUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

#define SELECTION_CALL(site, target, frame) \
    do { \
        if (!CallChildWithFrame(memory, cpu, child, context, \
                                site, target, frame, 0x82u)) \
            return SelectionUnwound(site); \
    } while (0)

Lufia2ExecutionResult Lufia2SaveSelectCharacter(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->program_bank != 0x82u ||
        !cpu->accumulator_is_8_bit || cpu->index_is_8_bit ||
        cpu->decimal || cpu->direct_page || cpu->stack < 0x1f00u ||
        cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x82ecc7u);
    for (;;) {
        OpLoadA(cpu, 0u);
        SELECTION_CALL(0x82ecc9u, 0x828b08u, 2u);
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            OpLoadA(cpu, 2u);
            SELECTION_CALL(0x82ecdeu, 0x80953bu, 3u);
            OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_INDEX));
            OpDecA(cpu);
            OpSta(memory, cpu, OpDp(cpu, DP_SAVE_MEMBER_RESULT));
            cpu->carry = 0u;
            return ExecutionReturned(0x82ece9u);
        }
        OpCmpValue(cpu, 3u);
        if (!cpu->zero)
            continue;
        SELECTION_CALL(0x82eceau, 0x868b32u, 3u);
        SELECTION_CALL(0x82eceeu, 0x82838fu, 2u);
        cpu->carry = 1u;
        return ExecutionReturned(0x82ecf2u);
    }
}
