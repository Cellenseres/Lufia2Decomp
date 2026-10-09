#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/menu.h"

enum {
    DP_SELECTION_BUTTONS = 0x47u,
    MENU_UNK_0B52 = WRAM_UNK_7E0B52,
    MENU_UNK_0B53 = WRAM_UNK_7E0B53,
    CAPSULE_UNK_119D = WRAM_UNK_7E119D,
    CAPSULE_UNK_119D_BYTES = 59u,
    CAPSULE_STATS_BYTES = 35u,
    CAPSULE_LEVEL_BYTES = 7u,
    CAPSULE_EXPERIENCE_BYTES = 21u,
    ROM_CAPSULE_INITIAL_STATS = 0x8ed558u,
    GOLD_LIMIT_LOW = 0x967fu,
    GOLD_LIMIT_HIGH = 0x98u
};

static bool MenuContextSupported(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit && !cpu->decimal;
}

static Lufia2ExecutionResult MenuContextUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2MenuCreditSelectionGold(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!MenuContextSupported(cpu))
        return ExecutionHandoff(cpu, 0x829808u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_GOLD));
    cpu->carry = false;
    OpAdc(memory, cpu, OpAbs(cpu, WRAM_MENU_PURCHASE_PRICE));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_GOLD));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_GOLD + 2u));
    OpAdc(memory, cpu, OpAbs(cpu, WRAM_MENU_PURCHASE_PRICE + 2u));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_GOLD + 2u));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_GOLD));
    cpu->carry = true;
    OpSbcValue(cpu, GOLD_LIMIT_LOW);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_GOLD + 2u));
    OpSbcValue(cpu, GOLD_LIMIT_HIGH);
    if (cpu->carry) {
        OpLdx(cpu, GOLD_LIMIT_LOW);
        OpWriteX(memory, cpu, OpAbs(cpu, WRAM_GOLD), cpu->x);
        OpLoadA(cpu, GOLD_LIMIT_HIGH);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_GOLD + 2u));
    }
    return ExecutionReturned(0x82983cu);
}

Lufia2ExecutionResult Lufia2MenuSaveFieldBuffers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!MenuContextSupported(cpu))
        return ExecutionHandoff(cpu, 0x82f660u);
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_FIELD_OAM_LOW_BUFFER));
        OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7E93C0));
        OpInx(cpu);
        OpCpx(cpu, WRAM_UNK_7E93C0_COUNT);
    } while (!cpu->zero);
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpLongX(cpu, WRAM_UNK_7FF080));
        OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7EA498));
        OpInx(cpu);
        OpCpx(cpu, WRAM_UNK_7EA498_COUNT);
    } while (!cpu->zero);
    return ExecutionReturned(0x82f681u);
}

Lufia2ExecutionResult Lufia2CapsuleInitializeSavedRecords(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!MenuContextSupported(cpu))
        return ExecutionHandoff(cpu, 0x82c2aeu);
    OpLdx(cpu, 0u);
    do {
        OpWriteM(memory, cpu, OpAbsX(cpu, CAPSULE_UNK_119D), 0u);
        OpInx(cpu);
        OpCpx(cpu, CAPSULE_UNK_119D_BYTES);
    } while (!cpu->zero);
    OpLdx(cpu, 0u);
    OpLoadA(cpu, 0u);
    do {
        OpSta(memory, cpu, OpLongX(cpu, WRAM_CAPSULE_SAVED_STATS));
        OpInx(cpu);
        OpCpx(cpu, CAPSULE_STATS_BYTES);
    } while (!cpu->zero);
    OpLdx(cpu, 0u);
    OpLoadA(cpu, 1u);
    do {
        OpSta(memory, cpu, OpLongX(cpu, WRAM_CAPSULE_SAVED_LEVELS));
        OpInx(cpu);
        OpCpx(cpu, CAPSULE_LEVEL_BYTES);
    } while (!cpu->zero);
    OpLdx(cpu, 0u);
    OpLoadA(cpu, 0u);
    do {
        OpSta(memory, cpu, OpLongX(cpu, WRAM_CAPSULE_SAVED_EXPERIENCE));
        OpInx(cpu);
        OpCpx(cpu, CAPSULE_EXPERIENCE_BYTES);
    } while (!cpu->zero);
    OpLdx(cpu, 0u);
    for (;;) {
        OpLda(memory, cpu, OpLongX(cpu, ROM_CAPSULE_INITIAL_STATS));
        if (cpu->zero)
            break;
        OpSta(memory, cpu, OpLongX(cpu, WRAM_CAPSULE_SAVED_STATS));
        OpInx(cpu);
    }
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E0A7F));
    return ExecutionReturned(0x82c2fcu);
}

Lufia2ExecutionResult Lufia2MenuResetSessionRecords(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !MenuContextSupported(cpu))
        return ExecutionHandoff(cpu, 0x829aa3u);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    OpWriteM(memory, cpu, OpAbs(cpu, WRAM_PLAY_TIME_FRAMES), 0u);
    OpWriteM(memory, cpu, OpAbs(cpu, WRAM_PLAY_TIME_SECONDS), 0u);
    OpWriteM(memory, cpu, OpAbs(cpu, WRAM_PLAY_TIME_MINUTES), 0u);
    OpWriteM(memory, cpu, OpAbs(cpu, WRAM_PLAY_TIME_HOURS), 0u);
    OpLdx(cpu, 6u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09C2), cpu->x);
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09C0), cpu->x);
    OpWriteM(memory, cpu, OpAbs(cpu, MENU_UNK_0B52), 0u);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, MENU_UNK_0B53));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_RESTORE_SELECTION_ENABLED));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x829acbu, 0x809634u, 3u, cpu->program_bank))
        return MenuContextUnwound(0x829acbu);
    OpAndValue(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E0B54));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x829ad4u, 0x809623u, 3u, cpu->program_bank))
        return MenuContextUnwound(0x829ad4u);
    OpWriteM(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09C8), 0u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x829adbu, 0x82c2aeu, 2u, cpu->program_bank))
        return MenuContextUnwound(0x829adbu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x829adfu);
}

Lufia2ExecutionResult Lufia2MenuRestoreSelectionCursor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !MenuContextSupported(cpu))
        return ExecutionHandoff(cpu, 0x8292f9u);
    OpLoadA(cpu, 6u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_CURSOR_SLOT)));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x8292feu, 0x82895bu, 2u, cpu->program_bank))
        return MenuContextUnwound(0x8292feu);
    return ExecutionReturned(0x829301u);
}

static bool MenuChoiceSoundEnabled(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E14BF));
    OpBitValue(cpu, 1u);
    return !cpu->zero;
}

Lufia2ExecutionResult Lufia2MenuPollSelectionChoice(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !MenuContextSupported(cpu))
        return ExecutionHandoff(cpu, 0x829246u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_CURSOR_SLOT)));
    for (unsigned offset = 0u; offset < 5u; ++offset)
        OpDex(cpu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E14BF));
    OpBitValue(cpu, 4u);
    const bool columns = !cpu->zero;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PARTY_MEMBER_COUNT));
    OpSta(memory, cpu, OpAbsX(cpu,
        columns ? WRAM_MENU_ITEM_COLUMNS : WRAM_MENU_ITEM_LIMIT));
    OpLoadA(cpu, 0u);
    for (;;) {
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x829265u, 0x828b08u, 2u, cpu->program_bank))
            return MenuContextUnwound(0x829265u);
        OpCmpValue(cpu, 1u);
        if (cpu->zero) {
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x829288u, 0x8292f9u, 2u, cpu->program_bank))
                return MenuContextUnwound(0x829288u);
            OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E14BF));
            OpBitValue(cpu, 0x20u);
            if (!cpu->zero) {
                OpLda(memory, cpu, OpDp(cpu, DP_SELECTION_BUTTONS));
                OpBitValue(cpu, 4u);
                if (!cpu->zero) {
                    if (MenuChoiceSoundEnabled(memory, cpu)) {
                        OpLoadA(cpu, 2u);
                        if (!CallChildWithFrame(memory, cpu, child, context,
                                0x8292a1u, 0x80953bu, 3u, cpu->program_bank))
                            return MenuContextUnwound(0x8292a1u);
                    }
                    OpLoadA(cpu, 3u);
                    cpu->carry = false;
                    return ExecutionReturned(0x8292a8u);
                }
            }
            OpLoadA(cpu, 1u);
            continue;
        }
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            if (MenuChoiceSoundEnabled(memory, cpu)) {
                OpLoadA(cpu, 2u);
                if (!CallChildWithFrame(memory, cpu, child, context,
                        0x8292b2u, 0x80953bu, 3u, cpu->program_bank))
                    return MenuContextUnwound(0x8292b2u);
            }
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x8292b6u, 0x8292f9u, 2u, cpu->program_bank))
                return MenuContextUnwound(0x8292b6u);
            OpLoadA(cpu, 0u);
            cpu->carry = false;
            return ExecutionReturned(0x8292bcu);
        }
        OpCmpValue(cpu, 3u);
        if (cpu->zero) {
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x8292bdu, 0x8292f9u, 2u, cpu->program_bank))
                return MenuContextUnwound(0x8292bdu);
            OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E14BF));
            OpBitValue(cpu, 2u);
            if (!cpu->zero) {
                OpLoadA(cpu, 0u);
                OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_CURSOR_SLOT)));
                OpTxy(cpu);
                OpDey(cpu);
                if (!CallChildWithFrame(memory, cpu, child, context,
                        0x8292ceu, 0x8289fau, 2u, cpu->program_bank))
                    return MenuContextUnwound(0x8292ceu);
                OpLoadA(cpu, 3u);
                OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_CURSOR_SLOT)));
                OpTxy(cpu);
                OpDey(cpu);
                if (!CallChildWithFrame(memory, cpu, child, context,
                        0x8292d8u, 0x8289bcu, 2u, cpu->program_bank))
                    return MenuContextUnwound(0x8292d8u);
            }
            cpu->carry = true;
            return ExecutionReturned(0x8292dcu);
        }
        OpCmpValue(cpu, 6u);
        bool shoulder = cpu->zero;
        if (!shoulder) {
            OpCmpValue(cpu, 7u);
            shoulder = cpu->zero;
        }
        if (shoulder) {
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x8292ddu, 0x8292f9u, 2u, cpu->program_bank))
                return MenuContextUnwound(0x8292ddu);
            OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E14BF));
            OpBitValue(cpu, 8u);
            if (cpu->zero) {
                OpLoadA(cpu, 1u);
                continue;
            }
            OpLoadA(cpu, 1u);
            cpu->carry = false;
            return ExecutionReturned(0x8292eau);
        }
        OpCmpValue(cpu, 4u);
        if (cpu->zero) {
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x8292ebu, 0x8292f9u, 2u, cpu->program_bank))
                return MenuContextUnwound(0x8292ebu);
            OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E14BF));
            OpBitValue(cpu, 0x10u);
            if (cpu->zero) {
                OpLoadA(cpu, 1u);
                continue;
            }
            OpLoadA(cpu, 2u);
            cpu->carry = false;
            return ExecutionReturned(0x8292f8u);
        }
        OpLoadA(cpu, 0u);
    }
}
