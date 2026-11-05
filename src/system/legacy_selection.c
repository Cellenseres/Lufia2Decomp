#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/legacy_fragments.h"

enum {
    SELECTION_MODE = 0x5f80u,
    SELECTION_RESULT = 0x5f8cu,
    SELECTION_NEXT_MODE = 0x5f90u,
    SELECTION_RECORD = 0x5fb4u,
    SELECTION_ACTION = 0x5fc5u,
    SELECTION_PENDING = 0x5fdeu,
    SELECTION_SPECIAL_PENDING = 0x5fdfu,
    SELECTION_DIRECTION = 0x5ff1u,
    SELECTION_SAVED_BYTE = 0x6004u,
    SELECTION_ACTION_RECORD = 0x6008u,
    SELECTION_REFRESH = 0x6147u,
    SELECTION_CONTROL = 0x62a4u,
    SELECTION_INDEX = 0x62a5u,
    SELECTION_SECOND_CONTROL = 0x62acu,
    SELECTION_DISABLED = 0x62aeu,
    SELECTION_RECORD_FLAG = 0x5befu,
    SELECTION_RECORD_OFFSET = 0xc0u,
    SELECTION_WAIT_COUNTER = 0x5eu,
    SELECTION_WAIT_READ_BUDGET = 4096u,
    SELECTION_ACTIVE = 0x001384u,
    SELECTION_SHARED_BYTE = 0x000c22u,
    SELECTION_MENU_FLAGS = 0x7f09d4u
};

typedef struct LegacySelection {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
} LegacySelection;

static bool LegacySelectionCall(LegacySelection *selection,
    uint32_t site, uint32_t target, uint8_t frame_size) {
    Lufia2CpuState *cpu = selection->cpu;
    if (!CallChildWithFrame(selection->memory, cpu, selection->child,
            selection->context, site, target, frame_size, 0x85u)) {
        selection->result = (Lufia2ExecutionResult){
            LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return false;
    }
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit != 1u ||
        cpu->index_is_8_bit != 0u) {
        selection->result = ExecutionHandoff(cpu,
            ((uint32_t)cpu->program_bank << 16) | (uint16_t)(site + frame_size + 1u));
        return false;
    }
    return true;
}

static Lufia2ExecutionResult LegacySelectionFinish(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, SELECTION_ACTIVE);
    return ExecutionReturned(0x85fbc0u);
}

static Lufia2ExecutionResult LegacySelectionWait(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    for (unsigned read = 0u; read < SELECTION_WAIT_READ_BUDGET; ++read) {
        EmitExecutionCheckpoint(memory, cpu, 0x85fbb6u);
        OpLda(memory, cpu, OpDp(cpu, SELECTION_WAIT_COUNTER));
        EmitExecutionCheckpoint(memory, cpu, 0x85fbb8u);
        if (cpu->zero)
            return LegacySelectionFinish(memory, cpu);
    }
    return ExecutionHandoff(cpu, 0x85fbb6u);
}

static Lufia2ExecutionResult LegacySelectionUseExistingRecord(
    LegacySelection *selection) {
    const Lufia2Memory *memory = selection->memory;
    Lufia2CpuState *cpu = selection->cpu;
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, SELECTION_CONTROL));
    OpSta(memory, cpu, OpAbs(cpu, SELECTION_SECOND_CONTROL));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, 0x16u));
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpDp(cpu, 0x17u));
    if (!LegacySelectionCall(selection, 0x85fae8u, 0x84ee50u, 3u))
        return selection->result;
    OpLda(memory, cpu, OpAbs(cpu, SELECTION_MODE));
    if (cpu->zero)
        return LegacySelectionFinish(memory, cpu);
    if (!LegacySelectionCall(selection, 0x85faf4u, 0x82f5eeu, 3u))
        return selection->result;
    OpLda(memory, cpu, SELECTION_SHARED_BYTE);
    OpSta(memory, cpu, OpAbs(cpu, SELECTION_SAVED_BYTE));
    if (!LegacySelectionCall(selection, 0x85faffu, 0x81fb55u, 3u))
        return selection->result;
    OpLda(memory, cpu, OpAbs(cpu, SELECTION_SAVED_BYTE));
    OpSta(memory, cpu, SELECTION_SHARED_BYTE);
    OpLdx(cpu, Read16AbsoluteIndexed(memory, cpu, SELECTION_RECORD, 0u));
    Compare16(cpu, cpu->x, 0xffffu);
    const bool missing_record = cpu->zero;
    OpLdx(cpu, 0u);
    if (!missing_record) {
        OpWriteX(memory, cpu, OpAbs(cpu, SELECTION_NEXT_MODE), cpu->x);
        OpLdx(cpu, 42u);
    }
    OpWriteX(memory, cpu, OpAbs(cpu, SELECTION_MODE), cpu->x);
    return LegacySelectionFinish(memory, cpu);
}

static Lufia2ExecutionResult LegacySelectionRunAction(LegacySelection *selection) {
    const Lufia2Memory *memory = selection->memory;
    Lufia2CpuState *cpu = selection->cpu;
    OpStz(memory, cpu, OpDp(cpu, 0x48u));
    OpStz(memory, cpu, OpDp(cpu, 0x4au));
    OpLdx(cpu, Read16AbsoluteIndexed(memory, cpu, SELECTION_MODE, 0u));
    Compare16(cpu, cpu->x, 16u);
    if (cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, SELECTION_ACTION));
        bool use_current = cpu->zero;
        if (!use_current) {
            OpCmpValue(cpu, 6u);
            use_current = cpu->zero;
        }
        if (!use_current) {
            OpCmpValue(cpu, 7u);
            use_current = cpu->zero;
        }
        if (use_current)
            OpLdx(cpu, Read16Direct(memory, cpu, SELECTION_RECORD_OFFSET));
        else
            OpLdx(cpu, Read16AbsoluteIndexed(memory, cpu, SELECTION_ACTION_RECORD, 0u));
    } else {
        OpLdx(cpu, Read16AbsoluteIndexed(memory, cpu, SELECTION_RECORD, 0u));
        Compare16(cpu, cpu->x, 0xffffu);
        if (!cpu->zero) {
            OpLdy(cpu, 22u);
            OpWriteX(memory, cpu, OpAbs(cpu, SELECTION_NEXT_MODE), cpu->y);
            OpLdy(cpu, 42u);
            OpWriteX(memory, cpu, OpAbs(cpu, SELECTION_MODE), cpu->y);
        } else {
            OpLdx(cpu, Read16Direct(memory, cpu, SELECTION_RECORD_OFFSET));
            OpLda(memory, cpu, OpAbsX(cpu, SELECTION_RECORD_FLAG));
            if (cpu->zero) {
                OpStz(memory, cpu, OpAbs(cpu, SELECTION_MODE));
                if (!LegacySelectionCall(selection, 0x85fb8eu, 0x81fb55u, 3u))
                    return selection->result;
                return LegacySelectionFinish(memory, cpu);
            }
            OpLdx(cpu, 22u);
            OpWriteX(memory, cpu, OpAbs(cpu, SELECTION_MODE), cpu->x);
            OpLdx(cpu, Read16Direct(memory, cpu, SELECTION_RECORD_OFFSET));
        }
    }
    if (!LegacySelectionCall(selection, 0x85fb9cu, 0x8581cfu, 2u))
        return selection->result;
    OpLda(memory, cpu, OpAbs(cpu, SELECTION_RESULT));
    OpCmpValue(cpu, 16u);
    if (cpu->zero) {
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpAbs(cpu, SELECTION_SPECIAL_PENDING));
    }
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, SELECTION_PENDING));
    if (!LegacySelectionCall(selection, 0x85fbb0u, 0x82d037u, 3u))
        return selection->result;
    OpStepMem(memory, cpu, OpDp(cpu, SELECTION_WAIT_COUNTER), 1);
    return LegacySelectionWait(memory, cpu);
}

static Lufia2ExecutionResult LegacySelectionRefresh(LegacySelection *selection) {
    const Lufia2Memory *memory = selection->memory;
    Lufia2CpuState *cpu = selection->cpu;
    if (!LegacySelectionCall(selection, 0x85fb2au, 0x82d194u, 3u))
        return selection->result;
    LoadA16(cpu, cpu->direct_page);
    OpLda(memory, cpu, OpAbs(cpu, SELECTION_INDEX));
    OpAndValue(cpu, 0x7fu);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, SELECTION_DISABLED));
    if (cpu->zero)
        return LegacySelectionRunAction(selection);
    OpLdy(cpu, 12u);
    OpLdx(cpu, Read16AbsoluteIndexed(memory, cpu, SELECTION_RECORD, 0u));
    Compare16(cpu, cpu->x, 0xffffu);
    if (!cpu->zero) {
        OpWriteX(memory, cpu, OpAbs(cpu, SELECTION_NEXT_MODE), cpu->y);
        OpLdy(cpu, 42u);
    }
    OpWriteX(memory, cpu, OpAbs(cpu, SELECTION_MODE), cpu->y);
    return LegacySelectionFinish(memory, cpu);
}

Lufia2ExecutionResult Lufia2LegacyRunSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || cpu->data_bank != 0x7fu ||
        cpu->accumulator_is_8_bit != 1u || cpu->index_is_8_bit != 0u ||
        cpu->direct_page || cpu->decimal ||
        cpu->stack < 0x1f04u || cpu->stack > 0x1ff8u ||
        (entry != 0x85fab6u && entry != 0x85fac8u &&
         entry != 0x85fb2au && entry != 0x85fbb6u) ||
        (!child && entry != 0x85fbb6u))
        return ExecutionHandoff(cpu, entry);
    if (entry == 0x85fbb6u)
        return LegacySelectionWait(memory, cpu);
    LegacySelection selection = {
        memory, cpu, child, context, ExecutionReturned(0x85fbc0u)};
    if (entry == 0x85fab6u) {
        OpTestBits(memory, cpu, OpDp(cpu, 0x60u), 1u);
        OpAndValue(cpu, 0x3fu);
        OpSta(memory, cpu, OpAbs(cpu, SELECTION_DIRECTION));
        if (!LegacySelectionCall(&selection, 0x85fabdu, 0x82e53cu, 3u))
            return selection.result;
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpAbs(cpu, SELECTION_REFRESH));
    } else if (entry == 0x85fac8u) {
        if (!LegacySelectionCall(&selection, 0x85fac8u, 0x82eaaau, 3u))
            return selection.result;
        OpLda(memory, cpu, SELECTION_MENU_FLAGS);
        if (!cpu->negative)
            return LegacySelectionUseExistingRecord(&selection);
        if (!LegacySelectionCall(&selection, 0x85fad2u, 0x82e435u, 3u))
            return selection.result;
    }
    return LegacySelectionRefresh(&selection);
}
