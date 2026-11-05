#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/legacy_fragments.h"

static bool EquipmentCursorContext(const Lufia2CpuState *cpu) {
    const uint8_t bank = cpu->data_bank;
    return cpu->program_bank == 0x82u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->direct_page && !cpu->decimal &&
        (bank < 0x40u || (bank >= 0x80u && bank < 0xc0u) || bank == 0x7eu);
}

static bool EquipmentCursorChild(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t site,
    uint32_t target, uint8_t frame, Lufia2ExecutionResult *result) {
    if (!CallChildWithFrame(memory, cpu, child, context, site, target, frame, 0x82u)) {
        *result = (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return false;
    }
    if (!EquipmentCursorContext(cpu)) {
        *result = ExecutionHandoff(cpu, ((uint32_t)cpu->program_bank << 16) |
            ((site + frame + 1u) & 0xffffu));
        return false;
    }
    return true;
}

Lufia2ExecutionResult Lufia2MenuContinueLegacyEquipmentCursor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !EquipmentCursorContext(cpu) ||
        cpu->stack < 0x1f04u || cpu->stack > 0x1ff8u)
        return ExecutionHandoff(cpu, 0x82b830u);
    Lufia2ExecutionResult result;
    OpLoadA(cpu, 5u);
    OpLdx(cpu, 7u);
    if (!EquipmentCursorChild(memory, cpu, child, context,
            0x82b835u, 0x82895bu, 2u, &result) ||
        !EquipmentCursorChild(memory, cpu, child, context,
            0x82b838u, 0x868b55u, 3u, &result))
        return result;
    OpLoadA(cpu, 3u);
    OpLdx(cpu, 7u);
    if (!EquipmentCursorChild(memory, cpu, child, context,
            0x82b841u, 0x82895bu, 2u, &result))
        return result;
    return ExecutionHandoff(cpu, 0x82b846u);
}
