#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/menu.h"

static bool MenuDispatchSupported(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit && !cpu->decimal;
}

static Lufia2ExecutionResult MenuDispatchUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static Lufia2ExecutionResult NameEntryMoveCursor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint8_t *repeat) {
    OpLdx(cpu, 5u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PRESSED_B));
    OpBitValue(cpu, 8u);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_MENU_ITEM_ROW));
        OpCmpValue(cpu, 8u);
        if (cpu->zero) *repeat = 1u;
        return ExecutionReturned(0u);
    }
    OpBitValue(cpu, 4u);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_MENU_ITEM_ROW));
        if (cpu->zero) *repeat = 1u;
        return ExecutionReturned(0u);
    }
    OpBitValue(cpu, 2u);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_MENU_ITEM_COLUMN));
        OpCmpValue(cpu, 4u);
        if (!cpu->zero) return ExecutionReturned(0u);
    } else {
        OpBitValue(cpu, 1u);
        if (cpu->zero) return ExecutionReturned(0u);
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_MENU_ITEM_COLUMN));
        if (!cpu->zero) return ExecutionReturned(0u);
    }
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_MENU_ITEM_ORIGIN_X_LOW));
    OpCmpValue(cpu, 0x34u);
    if (!cpu->zero) {
        OpLdy(cpu, 0xafu);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82eca0u, 0x82891eu, 2u, cpu->program_bank))
            return MenuDispatchUnwound(0x82eca0u);
    } else {
        OpLdy(cpu, 0xb6u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82eca9u, 0x82891eu, 2u, cpu->program_bank))
            return MenuDispatchUnwound(0x82eca9u);
    }
    return ExecutionReturned(0u);
}

Lufia2ExecutionResult Lufia2MenuRunNameEntry(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !MenuDispatchSupported(cpu))
        return ExecutionHandoff(cpu, 0x82ebf2u);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E155B));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82ebf7u, 0x82f0a2u, 2u, cpu->program_bank))
        return MenuDispatchUnwound(0x82ebf7u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82ebfau, 0x82f0f6u, 2u, cpu->program_bank))
        return MenuDispatchUnwound(0x82ebfau);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82ebfdu, 0x82f117u, 2u, cpu->program_bank))
        return MenuDispatchUnwound(0x82ebfdu);
    OpLoadA(cpu, 1u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82ec02u, 0x8293f6u, 2u, cpu->program_bank))
        return MenuDispatchUnwound(0x82ec02u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82ec05u, 0x828704u, 2u, cpu->program_bank))
        return MenuDispatchUnwound(0x82ec05u);
    uint8_t repeat = 0u;
    for (;;) {
        OpLoadA(cpu, repeat);
        repeat = 0u;
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82ec0au, 0x828b08u, 2u, cpu->program_bank))
            return MenuDispatchUnwound(0x82ec0au);
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            OpLoadA(cpu, 2u);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ec2bu, 0x80953bu, 3u, cpu->program_bank))
                return MenuDispatchUnwound(0x82ec2bu);
            OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E155B));
            if (!cpu->zero) {
                OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E155B));
                if (!CallChildWithFrame(memory, cpu, child, context,
                        0x82ec37u, 0x82f5afu, 2u, cpu->program_bank))
                    return MenuDispatchUnwound(0x82ec37u);
            }
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ec3au, 0x82f523u, 2u, cpu->program_bank))
                return MenuDispatchUnwound(0x82ec3au);
            continue;
        }
        OpCmpValue(cpu, 3u);
        if (cpu->zero) {
            OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E155B));
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ec57u, 0x82f585u, 2u, cpu->program_bank))
                return MenuDispatchUnwound(0x82ec57u);
            OpLoadA(cpu, 8u);
            OpTestBits(memory, cpu, OpDp(cpu, DP_MENU_FRAME_FLAGS), true);
            continue;
        }
        OpCmpValue(cpu, 8u);
        if (!cpu->zero) OpCmpValue(cpu, 4u);
        if (cpu->zero) {
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ec3fu, 0x82ecafu, 2u, cpu->program_bank))
                return MenuDispatchUnwound(0x82ec3fu);
            if (cpu->carry) {
                if (!CallChildWithFrame(memory, cpu, child, context,
                        0x82ec44u, 0x8297e3u, 2u, cpu->program_bank))
                    return MenuDispatchUnwound(0x82ec44u);
                continue;
            }
            OpLoadA(cpu, 2u);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ec4bu, 0x80953bu, 3u, cpu->program_bank))
                return MenuDispatchUnwound(0x82ec4bu);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ec4fu, 0x868b32u, 3u, cpu->program_bank))
                return MenuDispatchUnwound(0x82ec4fu);
            return ExecutionReturned(0x82ec53u);
        }
        OpCmpValue(cpu, 1u);
        if (cpu->zero) {
            Lufia2ExecutionResult move = NameEntryMoveCursor(memory, cpu, child, context, &repeat);
            if (move.flow != LUFIA2_EXECUTION_RETURNED) return move;
        }
    }
}

Lufia2ExecutionResult Lufia2MenuRunItemSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !MenuDispatchSupported(cpu))
        return ExecutionHandoff(cpu, 0x82b104u);
    OpLdx(cpu, 0xffu);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_MODE), cpu->x);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82b10au, 0x829d55u, 2u, cpu->program_bank))
        return MenuDispatchUnwound(0x82b10au);
    uint8_t repeat = 0u;
    for (;;) {
        OpLoadA(cpu, repeat);
        repeat = 0u;
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82b10fu, 0x828b08u, 2u, cpu->program_bank))
            return MenuDispatchUnwound(0x82b10fu);
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_INDEX));
            if (!cpu->zero) {
                OpLoadA(cpu, 2u);
                if (!CallChildWithFrame(memory, cpu, child, context,
                        0x82b131u, 0x80953bu, 3u, cpu->program_bank))
                    return MenuDispatchUnwound(0x82b131u);
                OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_INDEX));
                OpCmpValue(cpu, 1u);
                if (!cpu->zero) {
                    OpCmpValue(cpu, 3u);
                    if (!cpu->zero && !CallChildWithFrame(memory, cpu, child, context,
                            0x82b140u, 0x829c52u, 2u, cpu->program_bank))
                        return MenuDispatchUnwound(0x82b140u);
                }
            }
            OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_INDEX));
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82b146u, 0x82b14bu, 2u, cpu->program_bank))
                return MenuDispatchUnwound(0x82b146u);
            repeat = 1u;
            continue;
        }
        OpCmpValue(cpu, 3u);
        if (cpu->zero) {
            OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTED_SPELL)));
            bool can_close = cpu->zero;
            if (!can_close) {
                OpWriteX(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID), cpu->x);
                if (!CallChildWithFrame(memory, cpu, child, context,
                        0x82b160u, 0x81f194u, 3u, cpu->program_bank))
                    return MenuDispatchUnwound(0x82b160u);
                OpBitValue(cpu, 0x20u);
                if (cpu->zero) {
                    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E1562));
                    can_close = cpu->zero;
                }
            }
            if (can_close) {
                if (!CallChildWithFrame(memory, cpu, child, context,
                        0x82b16du, 0x868b32u, 3u, cpu->program_bank))
                    return MenuDispatchUnwound(0x82b16du);
                return ExecutionReturned(0x82b171u);
            }
            repeat = 1u;
            continue;
        }
        OpCmpValue(cpu, 6u);
        if (!cpu->zero) OpCmpValue(cpu, 7u);
        if (cpu->zero) {
            OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 1u));
            OpLsrA(cpu);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82b175u, 0x828ce9u, 2u, cpu->program_bank))
                return MenuDispatchUnwound(0x82b175u);
            if (!cpu->carry && !CallChildWithFrame(memory, cpu, child, context,
                    0x82b17au, 0x82ac84u, 2u, cpu->program_bank))
                return MenuDispatchUnwound(0x82b17au);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82b17du, 0x828c43u, 2u, cpu->program_bank))
                return MenuDispatchUnwound(0x82b17du);
            repeat = 1u;
        }
    }
}

Lufia2ExecutionResult Lufia2MenuRunScenarioSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !MenuDispatchSupported(cpu))
        return ExecutionHandoff(cpu, 0x82eef2u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82eef2u, 0x8ee5efu, 3u, cpu->program_bank))
        return MenuDispatchUnwound(0x82eef2u);
    return ExecutionReturned(0x82eef6u);
}
