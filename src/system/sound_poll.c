#include "core/cpu_ops.h"
#include "lufia2/system.h"

static bool SoundPollDescription(const Lufia2CpuState *cpu, uint32_t entry,
                                 uint32_t *port, unsigned *fetched, bool *read_reply) {
    if (cpu->program_bank != 0x80u || cpu->index_is_8_bit > 1u ||
        cpu->accumulator_is_8_bit != (entry != 0x809c18u))
        return false;
    *fetched = 4u;
    *read_reply = false;
    switch (entry) {
    case 0x809399u:
        *port = 0x002140u;
        break;
    case 0x8093a1u:
        *port = 0x002141u;
        break;
    case 0x80994bu:
    case 0x809997u:
    case 0x809868u:
    case 0x809a0fu:
        *port = 0x002142u;
        break;
    case 0x809a17u:
        *port = 0x002143u;
        break;
    case 0x80980du:
    case 0x809a26u:
        *port = 0x002142u;
        *read_reply = true;
        break;
    case 0x809a2cu:
        *port = 0x002143u;
        *read_reply = true;
        break;
    case 0x8098f7u:
        *port = OpAbs(cpu, 0x2142u);
        *fetched = 3u;
        break;
    case 0x809c18u:
    case 0x809c30u:
    case 0x809c40u:
    case 0x809c69u:
        *port = OpAbs(cpu, 0x2140u);
        *fetched = 3u;
        break;
    default:
        return false;
    }
    return true;
}

static void ReadSoundPoll(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                          uint32_t port, bool read_reply) {
    if (read_reply)
        OpLda(memory, cpu, port);
    else
        OpCmp(memory, cpu, port);
}

Lufia2ExecutionResult Lufia2SoundPollStep(const Lufia2Memory *memory,
                                          Lufia2CpuState *cpu, uint32_t entry) {
    uint32_t port;
    unsigned fetched;
    bool read_reply;
    if (!SoundPollDescription(cpu, entry, &port, &fetched, &read_reply))
        return ExecutionHandoff(cpu, entry);
    ReadSoundPoll(memory, cpu, port, read_reply);
    return ExecutionHandoff(cpu, entry + fetched);
}

uint32_t Lufia2SoundPollBranchTarget(uint32_t entry, uint8_t zero, uint8_t overflow) {
    uint32_t target;
    bool take = !zero;
    switch (entry) {
    case 0x80994fu:
        target = 0x80994bu;
        break;
    case 0x80999bu:
        target = 0x809997u;
        break;
    case 0x8098fau:
        target = 0x8098f7u;
        break;
    case 0x809811u:
        target = 0x80980du;
        take = zero != 0u;
        break;
    case 0x80986cu:
        target = 0x809868u;
        break;
    case 0x809a13u:
        target = 0x809a0fu;
        break;
    case 0x809a1bu:
        target = 0x809a17u;
        break;
    case 0x809a2au:
        target = 0x809a26u;
        break;
    case 0x809a30u:
        target = 0x809a2cu;
        break;
    case 0x80939du:
        target = 0x809399u;
        break;
    case 0x8093a5u:
        target = 0x8093a1u;
        break;
    case 0x809c1bu:
        target = 0x809c18u;
        break;
    case 0x809c33u:
        target = 0x809c30u;
        break;
    case 0x809c43u:
        target = 0x809c40u;
        break;
    case 0x809c6cu:
        target = 0x809c69u;
        break;
    case 0x809c47u:
        target = 0x809c45u;
        take = zero != 0u;
        break;
    case 0x809c6eu:
        target = 0x809c23u;
        take = overflow != 0u;
        break;
    default:
        return entry;
    }
    return take ? target : entry + 2u;
}

Lufia2ExecutionResult Lufia2SoundPollBranchStep(Lufia2CpuState *cpu, uint32_t entry) {
    if (cpu->program_bank != 0x80u || cpu->accumulator_is_8_bit > 1u ||
        cpu->index_is_8_bit > 1u)
        return ExecutionHandoff(cpu, entry);
    return ExecutionHandoff(
        cpu, Lufia2SoundPollBranchTarget(entry, cpu->zero, cpu->overflow));
}

Lufia2ExecutionResult Lufia2SoundPollIteration(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu, uint32_t entry) {
    uint32_t port;
    unsigned fetched;
    bool read_reply;
    if (!SoundPollDescription(cpu, entry, &port, &fetched, &read_reply))
        return ExecutionHandoff(cpu, entry);
    EmitExecutionCheckpoint(memory, cpu, entry);
    ReadSoundPoll(memory, cpu, port, read_reply);
    const uint32_t branch = entry + fetched;
    EmitExecutionCheckpoint(memory, cpu, branch);
    return ExecutionHandoff(
        cpu, Lufia2SoundPollBranchTarget(branch, cpu->zero, cpu->overflow));
}
