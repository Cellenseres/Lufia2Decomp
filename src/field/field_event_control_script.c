#include "core/cpu_ops.h"
#include "field/event_script_internal.h"
#include "lufia2/field.h"
#include "system/wram.h"
#include "text/text_internal.h"

unsigned Lufia2EventControlOpcode(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t handler, uint32_t *handoff) {
    if (handler != EVENT_OP_BEGIN_SCENE_CONTROL) {
        *handoff = 0x800000u | handler;
        return EVENT_OPCODE_HANDOFF;
    }
    OpLoadA(cpu, 0x80u);
    OpTestBits(memory, cpu, OpAbs(cpu, TEXT_WINDOW_STATE), 0u);
    Lufia2EventNextByte(memory, cpu, 0xd6bbu);
    PushDataBank(memory, cpu);
    PushY(memory, cpu);
    OpLdx(cpu, 4u);
    SimulateJslFrame(memory, cpu, 0x80u, 0xd6c4u);
    cpu->program_bank = 0x83u;
    Lufia2ExecutionResult result = Lufia2FieldBeginEventControl(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED) {
        *handoff = result.pc;
        return EVENT_OPCODE_HANDOFF;
    }
    uint8_t low = Pull8(memory, cpu);
    uint8_t high = Pull8(memory, cpu);
    uint8_t bank = Pull8(memory, cpu);
    uint16_t back = (uint16_t)(low | ((uint16_t)high << 8));
    cpu->program_bank = bank;
    if (back != 0xd6c4u || bank != 0x80u) {
        *handoff = ((uint32_t)bank << 16) | (uint16_t)(back + 1u);
        cpu->resume_pc = *handoff;
        return EVENT_OPCODE_HANDOFF;
    }
    OpPullY(memory, cpu);
    PullDataBank(memory, cpu);
    return EVENT_OPCODE_NEXT;
}
