#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/ancient_cave.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    DEVELOPER_INPUT_ENABLED = WRAM_UNK_7E057C,
    DEVELOPER_MAP_ENABLED = WRAM_UNK_7FE75A,
    DIAGNOSTIC_SPRITES = 0x0200u,
    DIAGNOSTIC_SPRITE_COUNT = 64u
};

static bool DeveloperChild(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t site, uint32_t target,
    Lufia2ExecutionResult *result) {
    if (!CallChildWithFrame(memory, cpu, child, context, site, target, 3u, 0x8eu)) {
        *result = (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return false;
    }
    if (cpu->program_bank != 0x8eu || !cpu->accumulator_is_8_bit ||
        cpu->direct_page || cpu->decimal || cpu->stack < 0x1f20u ||
        cpu->stack > 0x1ffcu) {
        *result = ExecutionHandoff(cpu,
            (uint32_t)cpu->program_bank << 16 | (uint16_t)(site + 4u));
        return false;
    }
    return true;
}

static void ConsumeDeveloperHigh(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint8_t mask, uint16_t returned) {
    OpLoadA(cpu, mask);
    SimulateJsrFrame(memory, cpu, returned);
    (void)Lufia2FieldConsumeSecondPadHigh(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

Lufia2ExecutionResult Lufia2FieldRunDeveloperControls(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x8eu || !cpu->accumulator_is_8_bit ||
        cpu->direct_page || cpu->decimal || cpu->stack < 0x1f20u ||
        cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x8eb0d4u);
    Lufia2ExecutionResult result;
    ConsumeDeveloperHigh(memory, cpu, 0x20u, 0xb0d8u);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, DEVELOPER_INPUT_ENABLED));
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
        OpSta(memory, cpu, OpAbs(cpu, DEVELOPER_INPUT_ENABLED));
        OpLoadA(cpu, 0x1bu);
        if (!DeveloperChild(memory, cpu, child, context, 0x8eb0e5u,
            0x848766u, &result))
            return result;
    }
    OpLda(memory, cpu, OpAbs(cpu, DEVELOPER_INPUT_ENABLED));
    if (!cpu->zero) {
        OpLoadA(cpu, 0x10u);
        SimulateJslFrame(memory, cpu, 0x8eu, 0xb0f3u);
        (void)Lufia2FieldConsumePressedHigh(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        if (!cpu->zero) {
            if (!DeveloperChild(memory, cpu, child, context, 0x8eb0f6u,
                0x858045u, &result))
                return result;
            OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_LAYER_TABLE_OFFSET));
            if (!DeveloperChild(memory, cpu, child, context, 0x8eb0fdu,
                0x80ed9cu, &result))
                return result;
        }
        ConsumeDeveloperHigh(memory, cpu, 0x40u, 0xb105u);
        if (!cpu->zero && !DeveloperChild(memory, cpu, child, context,
            0x8eb108u, 0x8584c6u, &result))
            return result;
        ConsumeDeveloperHigh(memory, cpu, 0x80u, 0xb110u);
        if (!cpu->zero && !DeveloperChild(memory, cpu, child, context,
            0x8eb113u, 0x83ac7au, &result))
            return result;
        OpLoadA(cpu, 0x80u);
        SimulateJsrFrame(memory, cpu, 0xb11bu);
        (void)Lufia2FieldConsumeSecondPadLow(memory, cpu);
        SimulateRtsFrame(memory, cpu);
        if (!cpu->zero) {
            OpLda(memory, cpu, DEVELOPER_MAP_ENABLED);
            OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
            OpSta(memory, cpu, DEVELOPER_MAP_ENABLED);
        }
        OpLda(memory, cpu, DEVELOPER_MAP_ENABLED);
        if (!cpu->zero) {
            SimulateJsrFrame(memory, cpu, 0xb130u);
            result = Lufia2CaveDrawDiagnostics(memory, cpu);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
            SimulateRtsFrame(memory, cpu);
        }
    }
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x8eb133u);
}

Lufia2ExecutionResult Lufia2FieldDrawDiagnosticSpriteGrid(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
        cpu->stack < 0x1f20u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x8584c6u);
    PushAccumulator8(memory, cpu);
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x30u);
    OpStz(memory, cpu, OpDp(cpu, 0x56u));
    OpLdx(cpu, 0u);
    OpLdy(cpu, 0u);
    for (unsigned sprite = 0u; sprite < DIAGNOSTIC_SPRITE_COUNT; ++sprite) {
        OpTya(cpu);
        OpAndValue(cpu, 7u);
        for (unsigned shift = 0u; shift < 4u; ++shift)
            OpAslA(cpu);
        OpAdcValue(cpu, 8u);
        OpSta(memory, cpu, OpAbsX(cpu, DIAGNOSTIC_SPRITES));
        OpTya(cpu);
        OpAndValue(cpu, 0xf8u);
        OpAslA(cpu);
        OpAdcValue(cpu, 8u);
        OpSta(memory, cpu, OpAbsX(cpu, DIAGNOSTIC_SPRITES + 1u));
        OpTya(cpu);
        OpAndValue(cpu, 0xf8u);
        OpAslA(cpu);
        OpAslA(cpu);
        OpSta(memory, cpu, OpDp(cpu, 0x54u));
        OpTya(cpu);
        OpAndValue(cpu, 7u);
        OpAslA(cpu);
        OpAdc(memory, cpu, OpDp(cpu, 0x54u));
        OpSta(memory, cpu, OpAbsX(cpu, DIAGNOSTIC_SPRITES + 2u));
        OpLoadA(cpu, 0x30u);
        OpSta(memory, cpu, OpAbsX(cpu, DIAGNOSTIC_SPRITES + 3u));
        OpIny(cpu);
        for (unsigned byte = 0u; byte < 4u; ++byte)
            OpInx(cpu);
    }
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, 0x72u));
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    if (cpu->accumulator_is_8_bit)
        LoadA8(cpu, Pull8(memory, cpu));
    else
        PullAccumulator16(memory, cpu);
    return ExecutionReturned(0x85850du);
}
