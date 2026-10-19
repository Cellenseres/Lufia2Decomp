#include "core/cpu_ops.h"
#include "lufia2/system.h"

Lufia2ExecutionResult Lufia2SoundPollStep(const Lufia2Memory *memory,
                                          Lufia2CpuState *cpu, uint32_t entry) {
    if (cpu->program_bank != 0x80u || cpu->index_is_8_bit > 1u ||
        cpu->accumulator_is_8_bit != (entry != 0x809c18u))
        return ExecutionHandoff(cpu, entry);
    uint32_t port;
    unsigned fetched = 4u;
    bool read_reply = false;
    switch (entry) {
    case 0x809399u:
        port = 0x002140u;
        break;
    case 0x8093a1u:
        port = 0x002141u;
        break;
    case 0x80994bu:
    case 0x809997u:
    case 0x809868u:
    case 0x809a0fu:
        port = 0x002142u;
        break;
    case 0x809a17u:
        port = 0x002143u;
        break;
    case 0x80980du:
    case 0x809a26u:
        port = 0x002142u;
        read_reply = true;
        break;
    case 0x809a2cu:
        port = 0x002143u;
        read_reply = true;
        break;
    case 0x8098f7u:
        port = OpAbs(cpu, 0x2142u);
        fetched = 3u;
        break;
    case 0x809c18u:
    case 0x809c30u:
    case 0x809c40u:
    case 0x809c69u:
        port = OpAbs(cpu, 0x2140u);
        fetched = 3u;
        break;
    default:
        return ExecutionHandoff(cpu, entry);
    }
    if (read_reply)
        OpLda(memory, cpu, port);
    else
        OpCmp(memory, cpu, port);
    return ExecutionHandoff(cpu, entry + fetched);
}
