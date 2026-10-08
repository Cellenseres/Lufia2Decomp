#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/plain_ops.h"
#include "system/wram.h"
#include "lufia2/party.h"

enum {
    MENU_CURSOR_VISIBLE = WRAM_MENU_CURSOR_VISIBLE & 0xffffu,
    MENU_CURSOR_TIMER = WRAM_MENU_CURSOR_TIMER & 0xffffu,
    CAPSULE_CURSOR_FORM = WRAM_CAPSULE_MENU_FORM & 0xffffu,
    CAPSULE_CURSOR_ITEM = WRAM_CAPSULE_MENU_ITEM & 0xffffu,
    DP_TEXT_POINTER = 0x08u,
    DP_TEXT_BANK = 0x0au,
    DP_TEXT_SOURCE_BANK = 0x5fu
};

static uint8_t CapsuleMenuContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x82u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && cpu->direct_page == 0u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static Lufia2ExecutionResult CapsuleChildUnwind(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2CapsuleCheckMenuForm(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CapsuleMenuContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82cc3cu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82cc3cu, 0x82c482u, 2u, 0x82u))
        return CapsuleChildUnwind(0x82cc3cu);
    OpLda(memory, cpu, OpAbsX(cpu, 0u));
    OpAndValue(cpu, 0x08u);
    if (cpu->zero) {
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        OpAndValue(cpu, 0x07u);
        OpCmpValue(cpu, 4u);
        if (!cpu->zero) {
            OpCmp(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_SELECTED_FORM));
            if (cpu->zero) {
                cpu->carry = 0u;
                return ExecutionReturned(0x82cc55u);
            }
        }
    }
    cpu->carry = 1u;
    return ExecutionReturned(0x82cc57u);
}

Lufia2ExecutionResult Lufia2CapsuleEnsureMenuItem(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CapsuleMenuContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82c577u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82c577u, 0x82cc3cu, 2u, 0x82u))
        return CapsuleChildUnwind(0x82c577u);
    if (cpu->carry)
        return ExecutionReturned(0x82c57cu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82c57du, 0x82c4e4u, 2u, 0x82u))
        return CapsuleChildUnwind(0x82c57du);
    OpBit(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_FLAG_ANY));
    if (cpu->zero) {
        OpOra(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_FLAG_ANY));
        OpSta(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_FLAG_ANY));
        OpRepWidths(cpu, 0x20u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82c58du, 0x82c4f4u, 2u, 0x82u))
            return CapsuleChildUnwind(0x82c58du);
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82c593u, 0x82c554u, 2u, 0x82u))
            return CapsuleChildUnwind(0x82c593u);
        PushStackWord(memory, cpu, cpu->accumulator);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82c597u, 0x82c504u, 2u, 0x82u))
            return CapsuleChildUnwind(0x82c597u);
        LoadA16(cpu, PullStackWord(memory, cpu));
        OpSta(memory, cpu, OpAbsX(cpu, 0u));
    }
    OpSepWidths(cpu, 0x20u);
    cpu->carry = 0u;
    return ExecutionReturned(0x82c5a1u);
}

Lufia2ExecutionResult Lufia2CapsuleUpdateItemCursor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CapsuleMenuContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82c5f3u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82c5f3u, 0x82c577u, 2u, 0x82u))
        return CapsuleChildUnwind(0x82c5f3u);
    if (cpu->carry)
        return ExecutionReturned(0x82c626u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82c5f8u, 0x82c504u, 2u, 0x82u))
        return CapsuleChildUnwind(0x82c5f8u);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, 0u)));
    OpWriteX(memory, cpu, OpAbs(cpu, CAPSULE_CURSOR_ITEM), cpu->y);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82c601u, 0x82c4f4u, 2u, 0x82u))
        return CapsuleChildUnwind(0x82c601u);
    OpLda(memory, cpu, OpAbsX(cpu, 0u));
    OpSta(memory, cpu, OpAbs(cpu, CAPSULE_CURSOR_FORM));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, CAPSULE_CURSOR_ITEM));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82c60fu, 0x82fb1fu, 3u, 0x82u))
        return CapsuleChildUnwind(0x82c60fu);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0u);
    OpAdcValue(cpu, 0u);
    OpLoadA(cpu, (uint16_t)(cpu->accumulator ^ 1u));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_CURSOR_ENABLED));
    OpLoadA(cpu, 0x10u);
    OpSta(memory, cpu, OpAbs(cpu, MENU_CURSOR_TIMER));
    OpStz(memory, cpu, OpAbs(cpu, MENU_CURSOR_VISIBLE));
    return ExecutionReturned(0x82c626u);
}

Lufia2ExecutionResult Lufia2CapsuleRefreshMenuItem(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CapsuleMenuContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82c5afu);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_CURSOR_ENABLED));
    OpStz(memory, cpu, OpAbs(cpu, CAPSULE_CURSOR_ITEM));
    OpStz(memory, cpu, OpAbs(cpu, CAPSULE_CURSOR_ITEM + 1u));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E0A7F));
    OpCmpValue(cpu, 7u);
    if (!cpu->zero)
        return ExecutionReturned(0x82c5f2u);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x059cu);
    OpLdx(cpu, 0x1004u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82c5c7u, 0x8283ebu, 2u, 0x82u))
        return CapsuleChildUnwind(0x82c5c7u);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09C8));
    OpBitValue(cpu, 1u);
    if (cpu->zero)
        return ExecutionReturned(0x82c5f2u);
    Push8(memory, cpu, cpu->program_bank);
    OpLoadA(cpu, Pull8(memory, cpu));
    OpSta(memory, cpu, OpDp(cpu, DP_TEXT_BANK));
    OpLdx(cpu, 0x10dfu);
    OpWriteX(memory, cpu, OpDp(cpu, DP_TEXT_POINTER), cpu->x);
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, DP_TEXT_SOURCE_BANK));
    OpLdy(cpu, 0xc8dbu);
    OpLdx(cpu, 0x359eu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82c5ebu, 0x808878u, 3u, 0x82u))
        return CapsuleChildUnwind(0x82c5ebu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82c5efu, 0x82c5f3u, 2u, 0x82u))
        return CapsuleChildUnwind(0x82c5efu);
    return ExecutionReturned(0x82c5f2u);
}
