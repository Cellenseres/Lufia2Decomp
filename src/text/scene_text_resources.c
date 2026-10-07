#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "lufia2/text.h"

enum {
    SCENE_TEXT_DICTIONARY = 0x878000u,
    SCENE_TEXT_REFERENCE = 0x54u,
    SCENE_TEXT_REFERENCE_HIGH = 0x55u,
    SCENE_TEXT_REFERENCE_LENGTH = 0x56u,
    SCENE_TEXT_REFERENCE_MASK = 0x0fffu,
    SCENE_TEXT_REFERENCE_COMMAND = 0x0au,
    SCENE_TEXT_UPLOAD_DESTINATION = 0x79u,
    SCENE_TEXT_UPLOAD_REQUEST = 0x75u,
    SCENE_TEXT_CLEAR_PASSES = 0x50u,
    SCENE_TEXT_UPLOAD_BYTES = 0x100u
};

static void SceneTextCopyReference(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpInx(cpu);
    OpLda(memory, cpu, OpLongX(cpu, SCENE_TEXT_DICTIONARY));
    OpSta(memory, cpu, OpDp(cpu, SCENE_TEXT_REFERENCE));
    OpInx(cpu);
    OpLda(memory, cpu, OpLongX(cpu, SCENE_TEXT_DICTIONARY));
    OpSta(memory, cpu, OpDp(cpu, SCENE_TEXT_REFERENCE_HIGH));
    OpInx(cpu);
    for (unsigned bit = 0; bit < 4u; ++bit)
        OpLsrA(cpu);
    OpIncA(cpu);
    OpIncA(cpu);
    OpSta(memory, cpu, OpDp(cpu, SCENE_TEXT_REFERENCE_LENGTH));
    OpPushX(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, SCENE_TEXT_REFERENCE));
    OpAndValue(cpu, SCENE_TEXT_REFERENCE_MASK);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    do {
        OpLda(memory, cpu, OpLongX(cpu, SCENE_TEXT_DICTIONARY));
        OpSta(memory, cpu, OpAbsY(cpu, 0u));
        OpIny(cpu);
        OpInx(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, SCENE_TEXT_REFERENCE_LENGTH), -1);
    } while (!cpu->zero);
    OpPullX(memory, cpu);
}

Lufia2ExecutionResult Lufia2TextExpandSceneString(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x80u || cpu->index_is_8_bit ||
        cpu->direct_page)
        return ExecutionHandoff(cpu, 0x80c9c0u);
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    OpSetDataBank(memory, cpu, 0x7eu);
    for (;;) {
        OpLda(memory, cpu, OpLongX(cpu, SCENE_TEXT_DICTIONARY));
        if (cpu->zero)
            break;
        OpCmpValue(cpu, SCENE_TEXT_REFERENCE_COMMAND);
        if (cpu->zero) {
            SceneTextCopyReference(memory, cpu);
        } else {
            OpSta(memory, cpu, OpAbsY(cpu, 0u));
            OpIny(cpu);
            OpInx(cpu);
        }
    }
    TransferDirectToA(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, 0u));
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x80ca13u);
}

Lufia2ExecutionResult Lufia2TextClearUploadRows(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x80u)
        return ExecutionHandoff(cpu, 0x80c61du);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_A1TL(0)), cpu->x);
    OpWriteX(memory, cpu, OpDp(cpu, SCENE_TEXT_UPLOAD_DESTINATION), cpu->y);
    OpLdy(cpu, SCENE_TEXT_CLEAR_PASSES);
    OpLoadA(cpu, 0u);
    do {
        OpSta(memory, cpu, OpLongX(cpu, 0x7e0000u));
        OpSta(memory, cpu, OpLongX(cpu, 0x7e0002u));
        for (unsigned byte = 0; byte < 4u; ++byte)
            OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->negative);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_A1B(0)));
    OpLdx(cpu, SCENE_TEXT_UPLOAD_BYTES);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_DASL(0)), cpu->x);
    OpLoadA(cpu, 0x18u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_BBAD(0)));
    OpLoadA(cpu, 0x41u);
    OpSta(memory, cpu, OpDp(cpu, SCENE_TEXT_UPLOAD_REQUEST));
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x80c651u);
}
