#include "lufia2/battle.h"
#include "core/cpu_ops.h"
#include "core/child_call.h"
#include "core/snes_registers.h"
#include "system/wram.h"

enum {
    HDMA_COMMAND_STREAM = 0xc3,
    HDMA_COMMAND_VALUE = 0xca,
    HDMA_REQUEST_DISABLE = 0xd9,
    HDMA_REQUEST_ENABLE = 0xda,
    HDMA_UPLOAD_REQUEST = 0xdb,
    HDMA_COMMAND_TARGET = (WRAM_UNK_7E1291 & 0xffffu) + 10u,
    ROM_HDMA_COMMANDS = 0x8e5c,
    HDMA_INDIRECT_BANK = 0x4317,
    HDMA_REQUEST_BIT = 2
};

static uint8_t HdmaWidths(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit;
}

static Lufia2ExecutionResult HdmaChildUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2BattleEffectDispatchHdmaCommand(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!HdmaWidths(cpu) || !child || cpu->program_bank != 0x81u)
        return ExecutionHandoff(cpu, 0x8192beu);

    TransferDirectToA(cpu);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, HDMA_COMMAND_STREAM));
    OpRepWidths(cpu, 0x20u);
    OpStepMem(memory, cpu, OpDp(cpu, HDMA_COMMAND_STREAM), 1);
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpDp(cpu, HDMA_COMMAND_VALUE));
    OpAslA(cpu);
    OpAdc(memory, cpu, OpDp(cpu, HDMA_COMMAND_VALUE));
    OpTax(cpu);
    for (uint16_t part = 0u; part != 3u; ++part) {
        OpLda(memory, cpu, OpAbsX(cpu, (uint16_t)(ROM_HDMA_COMMANDS + part)));
        OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(HDMA_COMMAND_TARGET + part)));
    }

    PushY(memory, cpu);
    SimulateJslFrame(memory, cpu, cpu->program_bank, 0x92e6u);
    const uint16_t offset = Read16Long(memory, HDMA_COMMAND_TARGET);
    const uint8_t bank = Read8(memory, HDMA_COMMAND_TARGET + 2u);
    const uint32_t target = ((uint32_t)bank << 16) | offset;
    cpu->program_bank = bank;
    if (!child(context, cpu, target, 0x8192e4u, 3u))
        return HdmaChildUnwound(0x8192e4u);
    OpPullY(memory, cpu);
    return ExecutionReturned(0x8192e8u);
}

static Lufia2ExecutionResult EndHdmaMode(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint32_t entry, uint32_t terminal) {
    if (!HdmaWidths(cpu))
        return ExecutionHandoff(cpu, entry);
    OpLoadA(cpu, HDMA_REQUEST_BIT);
    OpTestBits(memory, cpu, OpDp(cpu, HDMA_REQUEST_DISABLE), 0u);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_BATTLE_RESULT_HDMA_REQUEST));
    return ExecutionReturned(terminal);
}

static Lufia2ExecutionResult EndHdmaModeWithChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t entry, uint32_t target) {
    if (!HdmaWidths(cpu) || !child || cpu->program_bank != 0x85u)
        return ExecutionHandoff(cpu, entry);
    if (!CallChildWithFrame(memory, cpu, child, context, entry, target, 2u, 0x85u))
        return HdmaChildUnwound(entry);
    return ExecutionReturned(entry + 3u);
}

static Lufia2ExecutionResult BeginHdmaMode(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t entry, uint32_t terminal, uint16_t site,
    uint32_t builder, uint8_t phase, uint8_t register_byte,
    uint16_t descriptor, uint8_t mode) {
    if (!HdmaWidths(cpu) || !child || cpu->program_bank != 0x85u)
        return ExecutionHandoff(cpu, entry);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    if (phase == 1u) {
        OpLdy(cpu, 0u);
        OpTyx(cpu);
        OpWriteX(memory, cpu, OpAbs(cpu, WRAM_BATTLE_RESULT_SCROLL_PHASE), cpu->y);
    } else if (phase == 2u) {
        OpLdx(cpu, 0u);
        OpLdy(cpu, 0x17eu);
        OpWriteX(memory, cpu, OpAbs(cpu, WRAM_BATTLE_RESULT_SCROLL_PHASE), cpu->y);
    }
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x850000u | site, builder, 2u, 0x85u))
        return HdmaChildUnwound(0x850000u | site);
    if (!HdmaWidths(cpu))
        return ExecutionHandoff(cpu, 0x850000u | (uint16_t)(site + 3u));

    OpLoadA(cpu, 0x42u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_TRANSITION_HDMA_CONTROL));
    OpLoadA(cpu, register_byte);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_TRANSITION_HDMA_REGISTER));
    OpLdx(cpu, descriptor);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_BATTLE_TRANSITION_HDMA_SOURCE), cpu->x);
    OpLoadA(cpu, 0x85u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_TRANSITION_HDMA_SOURCE_BANK));
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpAbs(cpu, HDMA_INDIRECT_BANK));
    OpLoadA(cpu, HDMA_REQUEST_BIT);
    OpTestBits(memory, cpu, OpDp(cpu, HDMA_REQUEST_ENABLE), 1u);
    OpLoadA(cpu, mode);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_RESULT_HDMA_CHANNELS));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_RESULT_HDMA_REQUEST));
    OpSta(memory, cpu, OpDp(cpu, HDMA_UPLOAD_REQUEST));
    PullDataBank(memory, cpu);
    return ExecutionReturned(terminal);
}

Lufia2ExecutionResult Lufia2BattleEffectBeginBackwardWave(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return BeginHdmaMode(memory, cpu, child, context,
        0x85ada8u, 0x85ade0u, 0xadb2u, 0x85ade1u,
        1u, 0x12u, 0xa295u, 2u);
}

Lufia2ExecutionResult Lufia2BattleEffectEndBackwardWave(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    return EndHdmaMode(memory, cpu, 0x85ae27u, 0x85ae2eu);
}

Lufia2ExecutionResult Lufia2BattleEffectBeginForwardWave(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return BeginHdmaMode(memory, cpu, child, context,
        0x85ae2fu, 0x85ae67u, 0xae39u, 0x85ae68u,
        1u, 0x12u, 0xa295u, 3u);
}

Lufia2ExecutionResult Lufia2BattleEffectEndForwardWave(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    return EndHdmaMode(memory, cpu, 0x85aeb1u, 0x85aeb8u);
}

Lufia2ExecutionResult Lufia2BattleEffectBeginWaveFill(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return BeginHdmaMode(memory, cpu, child, context,
        0x85aeb9u, 0x85aeeau, 0xaebcu, 0x85aeebu,
        0u, 0x12u, 0xa295u, 4u);
}

Lufia2ExecutionResult Lufia2BattleEffectEndWaveFill(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    return EndHdmaMode(memory, cpu, 0x85af1du, 0x85af24u);
}

Lufia2ExecutionResult Lufia2BattleEffectBeginHdmaMode5(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return BeginHdmaMode(memory, cpu, child, context,
        0x85af25u, 0x85af5du, 0xaf2fu, 0x85af5eu,
        1u, 0x11u, 0xa114u, 5u);
}

Lufia2ExecutionResult Lufia2BattleEffectEndHdmaMode5(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    return EndHdmaMode(memory, cpu, 0x85af9fu, 0x85afa6u);
}

Lufia2ExecutionResult Lufia2BattleEffectBeginHdmaMode6(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return BeginHdmaMode(memory, cpu, child, context,
        0x85afa7u, 0x85afdfu, 0xafb1u, 0x85afe0u,
        1u, 0x11u, 0xa114u, 6u);
}

Lufia2ExecutionResult Lufia2BattleEffectEndHdmaMode6(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    return EndHdmaMode(memory, cpu, 0x85b01eu, 0x85b025u);
}

Lufia2ExecutionResult Lufia2BattleEffectBeginHdmaMode7(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return BeginHdmaMode(memory, cpu, child, context,
        0x85b026u, 0x85b057u, 0xb029u, 0x85b058u,
        0u, 0x11u, 0xa114u, 7u);
}

Lufia2ExecutionResult Lufia2BattleEffectEndHdmaMode7(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    return EndHdmaMode(memory, cpu, 0x85b082u, 0x85b089u);
}

Lufia2ExecutionResult Lufia2BattleEffectBeginHdmaMode8(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return BeginHdmaMode(memory, cpu, child, context,
        0x85b08au, 0x85b0c2u, 0xb094u, 0x85b0c3u,
        1u, 0x11u, 0xa39cu, 8u);
}

Lufia2ExecutionResult Lufia2BattleEffectEndHdmaMode8(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return EndHdmaModeWithChild(memory, cpu, child, context,
        0x85b0fcu, 0x85b0f4u);
}

Lufia2ExecutionResult Lufia2BattleEffectBeginHdmaMode9(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return BeginHdmaMode(memory, cpu, child, context,
        0x85b100u, 0x85b13au, 0xb10cu, 0x85b13bu,
        2u, 0x11u, 0xa39cu, 9u);
}

Lufia2ExecutionResult Lufia2BattleEffectEndHdmaMode9(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return EndHdmaModeWithChild(memory, cpu, child, context,
        0x85b16eu, 0x85b166u);
}

Lufia2ExecutionResult Lufia2BattleEffectBeginHdmaMode10(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return BeginHdmaMode(memory, cpu, child, context,
        0x85b172u, 0x85b1a3u, 0xb175u, 0x85b1a4u,
        0u, 0x11u, 0xa39cu, 10u);
}

Lufia2ExecutionResult Lufia2BattleEffectEndHdmaMode10(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    return EndHdmaMode(memory, cpu, 0x85b1ceu, 0x85b1d5u);
}

