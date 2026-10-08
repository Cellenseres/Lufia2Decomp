#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/party.h"

enum {
    ROM_CAPSULE_FLAG_MASKS = 0x8ed8bbu,
    CAPSULE_FORMS = WRAM_CAPSULE_FORMS & 0xffffu,
    CAPSULE_ITEMS = WRAM_CAPSULE_MENU_ITEMS & 0xffffu
};

static uint8_t CapsuleSelectionContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x82u && !cpu->decimal &&
        cpu->direct_page == 0u && cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

Lufia2ExecutionResult Lufia2CapsuleGetFlagMask(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CapsuleSelectionContext(cpu))
        return ExecutionHandoff(cpu, 0x82c4e4u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_SELECTED_ID));
    OpAndValue(cpu, 0xffu);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, ROM_CAPSULE_FLAG_MASKS));
    return ExecutionReturned(0x82c4f3u);
}

Lufia2ExecutionResult Lufia2CapsuleGetFormAddress(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CapsuleSelectionContext(cpu))
        return ExecutionHandoff(cpu, 0x82c4f4u);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_SELECTED_ID));
    OpAndValue(cpu, 0xffu);
    cpu->carry = 0u;
    OpAdcValue(cpu, CAPSULE_FORMS);
    OpTax(cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x82c503u);
}

Lufia2ExecutionResult Lufia2CapsuleGetItemAddress(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CapsuleSelectionContext(cpu))
        return ExecutionHandoff(cpu, 0x82c504u);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_SELECTED_ID));
    OpAndValue(cpu, 0xffu);
    OpAslA(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, CAPSULE_ITEMS);
    OpTax(cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x82c514u);
}
