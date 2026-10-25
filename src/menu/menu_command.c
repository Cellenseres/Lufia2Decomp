#include "core/cpu_ops.h"
#include "lufia2/menu_command.h"

enum {
    COMMAND_HELD_FIRST = 0x46u,
    COMMAND_HELD_SECOND = 0x47u,
    COMMAND_ARMED_FIRST = 0x4au,
    COMMAND_ARMED_SECOND = 0x4bu
};

typedef struct CommandButton {
    uint8_t mask;
    uint8_t armed;
    uint8_t held;
} CommandButton;

static bool ConsumeCommandButton(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, CommandButton button) {
    OpLoadA(cpu, button.mask);
    OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, button.armed)));
    if (cpu->zero) return false;
    OpLoadA(cpu, button.mask);
    OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, button.held)));
    if (cpu->zero) return false;
    OpTestBits(memory, cpu, OpDp(cpu, button.armed), false);
    return true;
}

Lufia2ExecutionResult Lufia2MenuConsumeCommand(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint8_t directions[] = {0x08u, 0x04u, 0x01u, 0x02u};
    static const CommandButton buttons[] = {
        {0x80u, COMMAND_ARMED_FIRST, COMMAND_HELD_FIRST},
        {0x80u, COMMAND_ARMED_SECOND, COMMAND_HELD_SECOND},
        {0x40u, COMMAND_ARMED_SECOND, COMMAND_HELD_SECOND},
        {0x40u, COMMAND_ARMED_FIRST, COMMAND_HELD_FIRST},
        {0x20u, COMMAND_ARMED_FIRST, COMMAND_HELD_FIRST},
        {0x10u, COMMAND_ARMED_FIRST, COMMAND_HELD_FIRST},
        {0x20u, COMMAND_ARMED_SECOND, COMMAND_HELD_SECOND},
        {0x10u, COMMAND_ARMED_SECOND, COMMAND_HELD_SECOND}
    };
    if (cpu->program_bank != 0x84u)
        return ExecutionHandoff(cpu, 0x849f1au);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    for (unsigned direction = 0u; direction < 4u; ++direction) {
        const CommandButton button = {
            directions[direction], COMMAND_ARMED_SECOND, COMMAND_HELD_SECOND};
        if (ConsumeCommandButton(memory, cpu, button)) {
            OpLoadA(cpu, (uint16_t)(1u << direction));
            break;
        }
    }
    ExchangeAccumulatorBytes(cpu);
    for (unsigned button = 0u; button < 8u; ++button) {
        if (ConsumeCommandButton(memory, cpu, buttons[button])) {
            OpLoadA(cpu, (uint16_t)(button + 1u));
            break;
        }
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x849fffu);
}
