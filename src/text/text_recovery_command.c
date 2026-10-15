#include "text/text_child_service.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "text/text_internal.h"

enum { RECOVERY_FADE_POLL_LIMIT = 65536u, RECOVERY_WAIT_FRAMES = 100u };

static bool RecoveryWaitForFade(TextChildService *recovery, uint32_t poll) {
    for (uint32_t reads = 0u; reads < RECOVERY_FADE_POLL_LIMIT; ++reads) {
        OpLda(recovery->memory, recovery->cpu, OpAbs(recovery->cpu, WRAM_FADE_CONTROL));
        if (recovery->cpu->zero)
            return true;
    }
    recovery->result = ExecutionHandoff(recovery->cpu, poll);
    return false;
}

static void RecoveryRequestFade(TextChildService *recovery, uint8_t request) {
    OpStz(recovery->memory, recovery->cpu, OpAbs(recovery->cpu, WRAM_FADE_LEVEL));
    OpLoadA(recovery->cpu, request);
    OpSta(recovery->memory, recovery->cpu, OpAbs(recovery->cpu, WRAM_FADE_CONTROL));
}

Lufia2ExecutionResult Lufia2TextRecoverParty(const Lufia2Memory *memory,
                                             Lufia2CpuState *cpu,
                                             Lufia2PushedChildCall child,
                                             void *context) {
    if (!child || cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal ||
        cpu->stack < 0x1f20u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x80a2e1u);
    TextChildService recovery = {memory, cpu, child, context, ExecutionReturned(0x809d00u)};
    PushY(memory, cpu);
    OpLoadA(cpu, 4u);
    if (!TextCallChild(&recovery, 0x80a2e4u, 0x8093feu, 3u))
        return recovery.result;
    RecoveryRequestFade(&recovery, 0xc1u);
    if (!RecoveryWaitForFade(&recovery, 0x80a2f0u))
        return recovery.result;
    if (!TextCallChild(&recovery, 0x80a2f5u, 0x81f5bdu, 3u))
        return recovery.result;
    OpLdy(cpu, RECOVERY_WAIT_FRAMES);
    do {
        if (!TextCallChild(&recovery, 0x80a2fcu, 0x80bf0bu, 2u))
            return recovery.result;
        OpDey(cpu);
    } while (!cpu->zero);
    RecoveryRequestFade(&recovery, 0x81u);
    if (!RecoveryWaitForFade(&recovery, 0x80a30au))
        return recovery.result;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E099D));
    if (!TextCallChild(&recovery, 0x80a312u, 0x80969fu, 3u))
        return recovery.result;
    OpPullY(memory, cpu);
    if (!TextCallChild(&recovery, 0x80a317u, 0x848311u, 3u) ||
        !TextCallChild(&recovery, 0x80a31bu, 0x80bf0bu, 2u))
        return recovery.result;
    Lufia2TextNextByte(memory, cpu, 0xa320u);
    ExchangeAccumulatorBytes(cpu);
    OpLoadA(cpu, 0u);
    ExchangeAccumulatorBytes(cpu);
    if (!TextCallChild(&recovery, 0x80a325u, 0x80bf43u, 2u))
        return recovery.result;
    return recovery.result;
}
