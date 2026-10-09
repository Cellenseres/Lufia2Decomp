#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/menu.h"

enum {
    DP_SAVE_MEMBER = 0x22u,
    DP_SAVE_FRAME_FLAGS = 0x74u,
    SRAM_SAVE_FLAGS_LONG = 0x700000u
};

static Lufia2ExecutionResult SaveDispatchChildUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static Lufia2ExecutionResult SaveRunSharedActions(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E155B));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82ebf7u, 0x82f0a2u, 2u, cpu->program_bank))
        return SaveDispatchChildUnwound(0x82ebf7u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82ebfau, 0x82f0f6u, 2u, cpu->program_bank))
        return SaveDispatchChildUnwound(0x82ebfau);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82ebfdu, 0x82f117u, 2u, cpu->program_bank))
        return SaveDispatchChildUnwound(0x82ebfdu);
    OpLoadA(cpu, 1u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82ec02u, 0x8293f6u, 2u, cpu->program_bank))
        return SaveDispatchChildUnwound(0x82ec02u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82ec05u, 0x828704u, 2u, cpu->program_bank))
        return SaveDispatchChildUnwound(0x82ec05u);
    unsigned poll_state = 0u;
    for (;;) {
        OpLoadA(cpu, (uint16_t)poll_state);
        poll_state = 0u;
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82ec0au, 0x828b08u, 2u, cpu->program_bank))
            return SaveDispatchChildUnwound(0x82ec0au);
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            OpLoadA(cpu, 2u);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ec2bu, 0x80953bu, 3u, cpu->program_bank))
                return SaveDispatchChildUnwound(0x82ec2bu);
            OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E155B));
            if (!cpu->zero) {
                OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E155B));
                if (!CallChildWithFrame(memory, cpu, child, context,
                        0x82ec37u, 0x82f5afu, 2u, cpu->program_bank))
                    return SaveDispatchChildUnwound(0x82ec37u);
            }
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ec3au, 0x82f523u, 2u, cpu->program_bank))
                return SaveDispatchChildUnwound(0x82ec3au);
            continue;
        }
        OpCmpValue(cpu, 3u);
        if (cpu->zero) {
            OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E155B));
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ec57u, 0x82f585u, 2u, cpu->program_bank))
                return SaveDispatchChildUnwound(0x82ec57u);
            OpLoadA(cpu, 8u);
            OpTestBits(memory, cpu, OpDp(cpu, DP_SAVE_FRAME_FLAGS), true);
            continue;
        }
        OpCmpValue(cpu, 8u);
        bool leave = cpu->zero;
        if (!leave) {
            OpCmpValue(cpu, 4u);
            leave = cpu->zero;
        }
        if (leave) {
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ec3fu, 0x82ecafu, 2u, cpu->program_bank))
                return SaveDispatchChildUnwound(0x82ec3fu);
            if (cpu->carry) {
                if (!CallChildWithFrame(memory, cpu, child, context,
                        0x82ec44u, 0x8297e3u, 2u, cpu->program_bank))
                    return SaveDispatchChildUnwound(0x82ec44u);
                continue;
            }
            OpLoadA(cpu, 2u);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ec4bu, 0x80953bu, 3u, cpu->program_bank))
                return SaveDispatchChildUnwound(0x82ec4bu);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ec4fu, 0x868b32u, 3u, cpu->program_bank))
                return SaveDispatchChildUnwound(0x82ec4fu);
            return ExecutionReturned(0x82ec53u);
        }
        OpCmpValue(cpu, 1u);
        if (!cpu->zero)
            continue;
        OpLdx(cpu, 5u);
        OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PRESSED_B));
        OpBitValue(cpu, 8u);
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbsX(cpu, WRAM_MENU_ITEM_ROW));
            OpCmpValue(cpu, 8u);
            if (cpu->zero)
                poll_state = 1u;
            continue;
        }
        OpBitValue(cpu, 4u);
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbsX(cpu, WRAM_MENU_ITEM_ROW));
            if (cpu->zero)
                poll_state = 1u;
            continue;
        }
        OpBitValue(cpu, 2u);
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbsX(cpu, WRAM_MENU_ITEM_COLUMN));
            OpCmpValue(cpu, 4u);
            if (!cpu->zero)
                continue;
        } else {
            OpBitValue(cpu, 1u);
            if (cpu->zero)
                continue;
            OpLda(memory, cpu, OpAbsX(cpu, WRAM_MENU_ITEM_COLUMN));
            if (!cpu->zero)
                continue;
        }
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_MENU_ITEM_ORIGIN_X_LOW));
        OpCmpValue(cpu, 0x34u);
        if (cpu->zero) {
            OpLdy(cpu, 0xb6u);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82eca9u, 0x82891eu, 2u, cpu->program_bank))
                return SaveDispatchChildUnwound(0x82eca9u);
        } else {
            OpLdy(cpu, 0xafu);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82eca0u, 0x82891eu, 2u, cpu->program_bank))
                return SaveDispatchChildUnwound(0x82eca0u);
        }
    }
}

Lufia2ExecutionResult Lufia2SaveRunMenuActions(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x82e9d5u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09C2)));
    OpCpx(cpu, 6u);
    bool poll_needed = cpu->zero;
    for (;;) {
        if (!poll_needed) {
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ea35u, 0x82ea3bu, 2u, cpu->program_bank))
                return SaveDispatchChildUnwound(0x82ea35u);
            if (!cpu->carry)
                return ExecutionReturned(0x82ea3au);
        }
        poll_needed = true;
        OpLoadA(cpu, 0u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82e9dfu, 0x828b08u, 2u, cpu->program_bank))
            return SaveDispatchChildUnwound(0x82e9dfu);
        OpCmpValue(cpu, 8u);
        bool selected = cpu->zero;
        if (!selected) {
            OpCmpValue(cpu, 2u);
            selected = cpu->zero;
        }
        if (selected) {
            OpLoadA(cpu, 2u);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82e9f8u, 0x80953bu, 3u, cpu->program_bank))
                return SaveDispatchChildUnwound(0x82e9f8u);
            OpLoadA(cpu, 5u);
            OpLdx(cpu, 6u);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ea01u, 0x82895bu, 2u, cpu->program_bank))
                return SaveDispatchChildUnwound(0x82ea01u);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ea04u, 0x868b55u, 3u, cpu->program_bank))
                return SaveDispatchChildUnwound(0x82ea04u);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ea08u, 0x868b32u, 3u, cpu->program_bank))
                return SaveDispatchChildUnwound(0x82ea08u);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ea0cu, 0x82838fu, 2u, cpu->program_bank))
                return SaveDispatchChildUnwound(0x82ea0cu);
            OpStz(memory, cpu, OpDp(cpu, DP_SAVE_MEMBER));
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ea11u, 0x829971u, 2u, cpu->program_bank))
                return SaveDispatchChildUnwound(0x82ea11u);
            OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_INDEX));
            OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E0B51));
            return SaveRunSharedActions(memory, cpu, child, context);
        }
        OpCmpValue(cpu, 1u);
        if (!cpu->zero)
            continue;
        OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PRESSED_B));
        OpBitValue(cpu, 4u);
        if (cpu->zero)
            continue;
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82ea29u, 0x82efb5u, 2u, cpu->program_bank))
            return SaveDispatchChildUnwound(0x82ea29u);
        OpLdx(cpu, 2u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82ea2fu, 0x8288a0u, 2u, cpu->program_bank))
            return SaveDispatchChildUnwound(0x82ea2fu);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82ea32u, 0x82f3fcu, 2u, cpu->program_bank))
            return SaveDispatchChildUnwound(0x82ea32u);
        poll_needed = false;
    }
}

Lufia2ExecutionResult Lufia2SaveRunSlotActions(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x82ea3bu);
    for (;;) {
        OpLoadA(cpu, 0u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82ea3du, 0x828b08u, 2u, cpu->program_bank))
            return SaveDispatchChildUnwound(0x82ea3du);
        OpCmpValue(cpu, 8u);
        bool selected = cpu->zero;
        if (!selected) {
            OpCmpValue(cpu, 2u);
            selected = cpu->zero;
        }
        if (!selected) {
            OpCmpValue(cpu, 1u);
            if (!cpu->zero)
                continue;
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ea54u, 0x82f3ddu, 2u, cpu->program_bank))
                return SaveDispatchChildUnwound(0x82ea54u);
            OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PRESSED_B));
            OpBitValue(cpu, 8u);
            bool change = !cpu->zero;
            if (change) {
                OpLdx(cpu, 2u);
                OpLda(memory, cpu, OpAbsX(cpu, WRAM_MENU_ITEM_ROW));
                change = !cpu->zero;
            }
            if (!change) {
                if (!CallChildWithFrame(memory, cpu, child, context,
                        0x82ea7eu, 0x82f3fcu, 2u, cpu->program_bank))
                    return SaveDispatchChildUnwound(0x82ea7eu);
                continue;
            }
            OpStz(memory, cpu, OpAbsX(cpu, WRAM_MENU_ITEM_ROW));
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ea69u, 0x8288cbu, 2u, cpu->program_bank))
                return SaveDispatchChildUnwound(0x82ea69u);
            OpLoadA(cpu, 1u);
            OpLdx(cpu, 6u);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ea71u, 0x82895bu, 2u, cpu->program_bank))
                return SaveDispatchChildUnwound(0x82ea71u);
            OpLoadA(cpu, 1u);
            OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_EFFECT_SPECIAL_PARTY));
            OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E11DF));
            cpu->carry = true;
            return ExecutionReturned(0x82ea7du);
        }
        OpLoadA(cpu, 2u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82ea99u, 0x80953bu, 3u, cpu->program_bank))
            return SaveDispatchChildUnwound(0x82ea99u);
        OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_INDEX));
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82eaa0u, 0x80905fu, 3u, cpu->program_bank))
            return SaveDispatchChildUnwound(0x82eaa0u);
        if (cpu->carry) {
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ea84u, 0x868b32u, 3u, cpu->program_bank))
                return SaveDispatchChildUnwound(0x82ea84u);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ea88u, 0x82838fu, 2u, cpu->program_bank))
                return SaveDispatchChildUnwound(0x82ea88u);
            OpStz(memory, cpu, OpDp(cpu, DP_SAVE_MEMBER));
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82ea8du, 0x829971u, 2u, cpu->program_bank))
                return SaveDispatchChildUnwound(0x82ea8du);
            OpPullX(memory, cpu);
            OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E0B51));
            return SaveRunSharedActions(memory, cpu, child, context);
        }
        OpLoadA(cpu, 5u);
        OpLdx(cpu, 7u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82eaabu, 0x82895bu, 2u, cpu->program_bank))
            return SaveDispatchChildUnwound(0x82eaabu);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82eaaeu, 0x868b55u, 3u, cpu->program_bank))
            return SaveDispatchChildUnwound(0x82eaaeu);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82eab2u, 0x82eff6u, 2u, cpu->program_bank))
            return SaveDispatchChildUnwound(0x82eab2u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82eab5u, 0x82eb9au, 2u, cpu->program_bank))
            return SaveDispatchChildUnwound(0x82eab5u);
        if (cpu->carry) {
            OpLoadA(cpu, 0u);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82eabcu, 0x82e917u, 2u, cpu->program_bank))
                return SaveDispatchChildUnwound(0x82eabcu);
            OpLdx(cpu, 7u);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82eac2u, 0x82ef25u, 2u, cpu->program_bank))
                return SaveDispatchChildUnwound(0x82eac2u);
            continue;
        }
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82eac8u, 0x868b32u, 3u, cpu->program_bank))
            return SaveDispatchChildUnwound(0x82eac8u);
        OpLdx(cpu, 2u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82eacfu, 0x8288a0u, 2u, cpu->program_bank))
            return SaveDispatchChildUnwound(0x82eacfu);
        OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_INDEX));
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82ead5u, 0x809099u, 3u, cpu->program_bank))
            return SaveDispatchChildUnwound(0x82ead5u);
        OpLoadA(cpu, 0x80u);
        OpTestBits(memory, cpu, OpAbs(cpu, WRAM_UNK_7E0B51), true);
        OpLda(memory, cpu, SRAM_SAVE_FLAGS_LONG);
        OpAndValue(cpu, 0xfcu);
        OpOraValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_INDEX)));
        OpSta(memory, cpu, SRAM_SAVE_FLAGS_LONG);
        OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E0B54));
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82eaeeu, 0x809623u, 3u, cpu->program_bank))
            return SaveDispatchChildUnwound(0x82eaeeu);
        cpu->carry = false;
        return ExecutionReturned(0x82eaf3u);
    }
}
