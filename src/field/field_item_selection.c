#include "lufia2/field.h"
#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"

enum {
    FIELD_ITEM_TABLE = 0xc633,
    CHOICE_OFFSET = 0x54,
    CHOICE_BYTES = 0x56,
    ACTOR_SLOT = 0xa7,
    SELECTION_ACTOR_COUNT = 32,
    CHOICE_LOOP_LIMIT = 4096
};

static uint8_t ItemSelectionChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target, uint8_t frame,
    Lufia2ExecutionResult *result) {
    if (!CallChildWithFrame(memory, cpu, child, context,
            site, target, frame, 0x83u)) {
        *result = (Lufia2ExecutionResult){
            LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return 0u;
    }
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit) {
        *result = ExecutionHandoff(cpu, site + frame + 1u);
        return 0u;
    }
    return 1u;
}

static uint8_t CollectFieldItemChoices(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    Lufia2ExecutionResult *result) {
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_COUNT));
    OpLdy(cpu, 0u);
    OpLdx(cpu, 0u);
    for (unsigned visited = 0u; visited < CHOICE_LOOP_LIMIT; ++visited) {
        OpLda(memory, cpu, OpAbsX(cpu, FIELD_ITEM_TABLE));
        OpSta(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
        OpLda(memory, cpu, OpAbsX(cpu, FIELD_ITEM_TABLE + 1u));
        OpSta(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID + 1u));
        OpOraValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID)));
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_COUNT));
            return 1u;
        }
        OpPushX(memory, cpu);
        PushY(memory, cpu);
        if (!ItemSelectionChild(memory, cpu, child, context,
                0x83c48au, 0x81f057u, 3u, result))
            return 0u;
        cpu->y = PullIndexValue(memory, cpu);
        OpPullX(memory, cpu);
        OpOraValue(cpu, 0u);
        if (!cpu->zero) {
            OpStepMem(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_COUNT), 1);
            OpLda(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
            OpSta(memory, cpu, OpAbsY(cpu, WRAM_FIELD_ITEM_CHOICES));
            OpLda(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID + 1u));
            OpSta(memory, cpu, OpAbsY(cpu, WRAM_FIELD_ITEM_CHOICES + 1u));
            OpIny(cpu);
            OpIny(cpu);
        }
        OpInx(cpu);
        OpInx(cpu);
    }
    *result = ExecutionHandoff(cpu, 0x83c477u);
    return 0u;
}

static uint8_t RotateFieldItemChoices(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2ExecutionResult *result) {
    OpRepWidths(cpu, 0x20u);
    OpLdy(cpu, 0u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_SELECTED_ITEM));
    if (!cpu->zero) {
        OpSta(memory, cpu, OpDp(cpu, CHOICE_OFFSET));
        OpLdy(cpu, 0u);
        unsigned visited = 0u;
        for (;;) {
            if (visited++ == CHOICE_LOOP_LIMIT) {
                *result = ExecutionHandoff(cpu, 0x83c4beu);
                return 0u;
            }
            OpLda(memory, cpu, OpAbsY(cpu, WRAM_FIELD_ITEM_CHOICES));
            if (cpu->zero) {
                OpLdy(cpu, 0u);
                break;
            }
            OpCmp(memory, cpu, OpDp(cpu, CHOICE_OFFSET));
            if (cpu->zero)
                break;
            OpIny(cpu);
            OpIny(cpu);
        }
    }
    OpWriteX(memory, cpu, OpDp(cpu, CHOICE_OFFSET), cpu->y);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_COUNT));
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, CHOICE_BYTES));
    OpStz(memory, cpu, OpDp(cpu, CHOICE_BYTES + 1u));
    OpLdx(cpu, 0u);
    for (unsigned visited = 0u;; ++visited) {
        if (visited == CHOICE_LOOP_LIMIT) {
            *result = ExecutionHandoff(cpu, 0x83c4dbu);
            return 0u;
        }
        OpLda(memory, cpu, OpAbsY(cpu, WRAM_FIELD_ITEM_CHOICES));
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_MENU_LIST_COUNT));
        OpInx(cpu);
        OpInx(cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, OpReadX(memory, cpu, OpDp(cpu, CHOICE_BYTES)));
        if (cpu->zero)
            break;
    }
    OpLdy(cpu, 0u);
    OpCpy(cpu, OpReadX(memory, cpu, OpDp(cpu, CHOICE_OFFSET)));
    for (unsigned visited = 0u; !cpu->zero; ++visited) {
        if (visited == CHOICE_LOOP_LIMIT) {
            *result = ExecutionHandoff(cpu, 0x83c4f0u);
            return 0u;
        }
        OpLda(memory, cpu, OpAbsY(cpu, WRAM_FIELD_ITEM_CHOICES));
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_MENU_LIST_COUNT));
        OpInx(cpu);
        OpInx(cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, OpReadX(memory, cpu, OpDp(cpu, CHOICE_OFFSET)));
    }
    return 1u;
}

static uint8_t SpawnFieldItemSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    Lufia2ExecutionResult *result) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, WRAM_ACTOR_FINE_X);
    OpSta(memory, cpu, OpDp(cpu, 2u));
    OpLda(memory, cpu, WRAM_ACTOR_FINE_Y);
    OpSta(memory, cpu, OpDp(cpu, 4u));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x7fu);
    OpSta(memory, cpu, OpDp(cpu, 6u));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpDp(cpu, 9u));
    OpLdx(cpu, 0xffffu);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_MARKER_SLOT), cpu->x);
    OpLoadA(cpu, 0x26u);
    if (!ItemSelectionChild(memory, cpu, child, context,
            0x83c51eu, 0x83c652u, 3u, result))
        return 0u;
    if (cpu->carry) {
        OpStz(memory, cpu, OpDp(cpu, 8u));
        OpLoadA(cpu, 2u);
        OpSta(memory, cpu, OpDp(cpu, 1u));
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, OpDp(cpu, 0u));
        OpLoadA(cpu, 0x2eu);
        OpSta(memory, cpu, OpDp(cpu, 10u));
        if (!ItemSelectionChild(memory, cpu, child, context,
                0x83c532u, 0x83c729u, 3u, result))
            return 0u;
    }
    OpLoadA(cpu, 0x40u);
    OpSta(memory, cpu, OpDp(cpu, 8u));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpDp(cpu, 1u));
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLoadA(cpu, 0x14u);
    OpStz(memory, cpu, OpDp(cpu, 14u));
    OpSta(memory, cpu, OpDp(cpu, 15u));
    OpLoadA(cpu, 0x2du);
    OpSta(memory, cpu, OpDp(cpu, 10u));
    if (!ItemSelectionChild(memory, cpu, child, context,
            0x83c54au, 0x83c729u, 3u, result))
        return 0u;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_COUNT));
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpStz(memory, cpu, OpDp(cpu, 1u));
    OpLoadA(cpu, 0x2eu);
    OpSta(memory, cpu, OpDp(cpu, 10u));
    return ItemSelectionChild(memory, cpu, child, context,
        0x83c559u, 0x83c729u, 3u, result);
}

static uint8_t ConfigureFieldItemObject(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    Lufia2ExecutionResult *result) {
    OpWriteX(memory, cpu, OpDp(cpu, ACTOR_SLOT), cpu->x);
    if (!ItemSelectionChild(memory, cpu, child, context,
            0x83c57du, 0x83ab4fu, 3u, result))
        return 0u;
    const uint16_t fields[] = {
        (WRAM_BATTLE_CURSOR_SOURCE + 1u), WRAM_FIELD_RECOVERY_ZOOM_LIMITS,
        WRAM_FIELD_RECOVERY_SPIN_RATES, WRAM_MENU_ITEM_COLUMN,
        WRAM_FIELD_ITEM_SECOND_VELOCITY, WRAM_FIELD_ITEM_SECOND_ANGLE,
        WRAM_FIELD_RECOVERY_SECOND_SPIN_RATES};
    const uint8_t values[] = {0xd1u, 0x98u, 0xe1u, 0x40u, 0xf8u, 0xb0u, 0x21u};
    for (unsigned i = 0u; i < sizeof fields / sizeof fields[0]; ++i) {
        OpLoadA(cpu, values[i]);
        OpSta(memory, cpu, OpAbsX(cpu, fields[i]));
    }
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E1559));
    if (cpu->zero)
        return ItemSelectionChild(memory, cpu, child, context,
            0x83c5cau, 0x83c5feu, 2u, result);
    OpCmpValue(cpu, 2u);
    if (!cpu->zero)
        return 1u;
    OpLoadA(cpu, 0x21u);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_FIELD_RECOVERY_SPIN_RATES));
    OpLoadA(cpu, 0x98u);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_FIELD_ITEM_SECOND_VELOCITY));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_MENU_ITEM_COLUMN));
    OpLoadA(cpu, 0x18u);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_ANIMATION_REQUEST));
    OpLoadA(cpu, 5u);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7FDA2C));
    return 1u;
}

static void CommitFieldItemSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 1u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_FIELD_REQUESTS), 1u);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E11D8));
    OpLoadA(cpu, 1u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09A9), 0u);
    OpLoadA(cpu, 0xfeu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_RECOVERY_PALETTE_LENGTH));
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_PALETTE_SCALE_PHASE));
    OpLoadA(cpu, 0x40u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_PALETTE_SCALE_TARGET));
    OpLda(memory, cpu, 0x0009a9u);
    OpOraValue(cpu, 4u);
    OpSta(memory, cpu, 0x0009a9u);
}

Lufia2ExecutionResult Lufia2FieldOpenItemSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x83u || !cpu->accumulator_is_8_bit ||
            cpu->index_is_8_bit || cpu->decimal || !child)
        return ExecutionHandoff(cpu, 0x83c460u);
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E1559));
        if (cpu->zero)
            return ExecutionReturned(0x83c4aeu);
        OpInx(cpu);
        OpCpx(cpu, SELECTION_ACTOR_COUNT);
    } while (!cpu->zero);
    Lufia2ExecutionResult result;
    if (!CollectFieldItemChoices(memory, cpu, child, context, &result))
        return result;
    if (cpu->zero)
        return ExecutionReturned(0x83c4aeu);
    if (!RotateFieldItemChoices(memory, cpu, &result) ||
        !SpawnFieldItemSelection(memory, cpu, child, context, &result))
        return result;
    OpLdx(cpu, 0u);
    OpTxy(cpu);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E1559));
        uint8_t configure = 1u;
        if (cpu->zero)
            OpWriteX(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_IDLE_SLOT), cpu->x);
        else {
            OpCmpValue(cpu, 2u);
            if (cpu->zero)
                OpWriteX(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_MARKER_SLOT), cpu->x);
            else {
                OpCmpValue(cpu, 1u);
                configure = cpu->zero;
                if (configure)
                    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_ACTIVE_SLOT), cpu->x);
            }
        }
        if (configure && !ConfigureFieldItemObject(memory, cpu, child, context, &result))
            return result;
        OpInx(cpu);
        OpCpx(cpu, SELECTION_ACTOR_COUNT);
    } while (!cpu->zero);
    CommitFieldItemSelection(memory, cpu);
    return ExecutionReturned(0x83c5fdu);
}
