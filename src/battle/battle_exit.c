/* Battle-side exit wrapper at $81:876B. */

#include "core/cpu_ops.h"
#include "lufia2/battle.h"

static Lufia2ExecutionResult ExitChildUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static uint8_t ExitCall(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                        Lufia2PushedChildCall child, void *context,
                        uint32_t site, uint32_t target, uint8_t frame_size) {
    if (frame_size == 2u)
        SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    else
        SimulateJslFrame(memory, cpu, 0x81u, (uint16_t)(site + 3u));
    return child(context, cpu, target, site, frame_size);
}

#define EXIT_CALL(site, target, frame) \
    do { \
        if (!ExitCall(memory, cpu, child, child_context, \
                      0x810000u | (site), (target), (frame))) \
            return ExitChildUnwound(0x810000u | (site)); \
    } while (0)

Lufia2ExecutionResult Lufia2BattleExit(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    EXIT_CALL(0x876bu, 0x85ee3eu, 3u);
    EXIT_CALL(0x876fu, 0x85eddbu, 3u);
    OpLda(memory, cpu, 0x7ff8a2u);
    OpCmpValue(cpu, 1u);
    if (!cpu->zero) goto regular_exit;
    OpLda(memory, cpu, 0x7ff8a3u);
    if (!cpu->negative) goto special_exit;
    OpLda(memory, cpu, 0x7ff8a4u);
    OpCmpValue(cpu, 11u);
    if (cpu->zero) goto regular_exit;
    OpCmpValue(cpu, 37u);
    if (cpu->zero) goto regular_exit;
special_exit:
    EXIT_CALL(0x878du, 0x85eaefu, 3u);
    goto common_exit;
regular_exit:
    EXIT_CALL(0x8793u, 0x81c321u, 2u);
common_exit:
    EXIT_CALL(0x8796u, 0x81c2fbu, 2u);
    EXIT_CALL(0x8799u, 0x81c30eu, 2u);
    OpRepWidths(cpu, 0x20u);
    EXIT_CALL(0x879eu, 0x859bc3u, 3u);
    OpSepWidths(cpu, 0x20u);
    EXIT_CALL(0x87a4u, 0x85ec81u, 3u);
    OpStz(memory, cpu, OpDp(cpu, 0x6au));
    OpStz(memory, cpu, OpAbs(cpu, 0x420cu));
    LoadA8(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, 0x0583u));
    EXIT_CALL(0x87b2u, 0x85ec81u, 3u);
    return ExecutionReturned(0x8187b6u);
}
