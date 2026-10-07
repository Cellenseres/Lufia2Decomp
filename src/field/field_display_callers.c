#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/system_internal.h"
#include "system/wram.h"

enum {
    DISPLAY_LAYER_REQUEST = 0x74u,
    DISPLAY_DMA_REQUEST = 0x75u,
    DISPLAY_DMA_VRAM = 0x79u
};

static Lufia2ExecutionResult FieldFadeScreen(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t entry,
    uint8_t start_when_blank, uint8_t control) {
    if (cpu->program_bank != 0x83u)
        return ExecutionHandoff(cpu, entry);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x30u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
    if (cpu->negative == start_when_blank) {
        OpStz(memory, cpu, OpAbs(cpu, WRAM_FADE_LEVEL));
        OpLoadA(cpu, control);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_FADE_CONTROL));
        do {
            if (!CallChildWithFrame(memory, cpu, child, context,
                    entry + 0x10u, 0x83aeb5u, 2u, 0x83u)) {
                Lufia2ExecutionResult result = ExecutionReturned(entry + 0x10u);
                result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
                return result;
            }
            if (!CallChildWithFrame(memory, cpu, child, context,
                    entry + 0x13u, 0x83900cu, 2u, 0x83u)) {
                Lufia2ExecutionResult result = ExecutionReturned(entry + 0x13u);
                result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
                return result;
            }
            OpLda(memory, cpu, OpAbs(cpu, WRAM_FADE_CONTROL));
        } while (!cpu->zero);
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(entry + 0x1cu);
}

Lufia2ExecutionResult Lufia2FieldFadeIn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return FieldFadeScreen(memory, cpu, child, context, 0x83afcdu, 1u, 0x90u);
}

Lufia2ExecutionResult Lufia2FieldFadeOut(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return FieldFadeScreen(memory, cpu, child, context, 0x83afeau, 0u, 0xd0u);
}

Lufia2ExecutionResult Lufia2SceneUploadFixedGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x84u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x8482e7u);
    PushIndex(memory, cpu);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_DMAP(0)));
    OpLdx(cpu, 0xba80u);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_A1TL(0)), cpu->x);
    OpLoadA(cpu, 0x9fu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_A1B(0)));
    OpLdx(cpu, 0x1000u);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_DASL(0)), cpu->x);
    OpLoadA(cpu, 0x18u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_BBAD(0)));
    OpLdx(cpu, 0x1000u);
    OpWriteX(memory, cpu, OpDp(cpu, DISPLAY_DMA_VRAM), cpu->x);
    OpLoadA(cpu, 0x41u);
    OpSta(memory, cpu, OpDp(cpu, DISPLAY_DMA_REQUEST));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x84830cu, 0x848363u, 2u, 0x84u)) {
        Lufia2ExecutionResult result = ExecutionReturned(0x84830cu);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    cpu->x = PullIndexValue(memory, cpu);
    SetNz16(cpu, cpu->x);
    return ExecutionReturned(0x848310u);
}

Lufia2ExecutionResult Lufia2FieldRestoreBackgroundLayers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x84u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x848311u);
    OpLdx(cpu, 2u);
    do {
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x848314u, 0x80f47au, 3u, 0x84u)) {
            Lufia2ExecutionResult result = ExecutionReturned(0x848314u);
            result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
            return result;
        }
        OpDex(cpu);
        OpDex(cpu);
    } while (!cpu->negative);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, DISPLAY_LAYER_REQUEST));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x848320u, 0x848363u, 2u, 0x84u)) {
        Lufia2ExecutionResult result = ExecutionReturned(0x848320u);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpDp(cpu, DISPLAY_LAYER_REQUEST));
    return ExecutionReturned(0x848327u);
}
