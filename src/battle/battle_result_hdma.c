#include "battle/battle_internal.h"

enum {
    RESULT_SCROLL_TABLE = 0x7e4000u,
    RESULT_SCROLL_PHASE = WRAM_BATTLE_RESULT_SCROLL_PHASE,
    RESULT_SCROLL_CHANNELS = WRAM_BATTLE_RESULT_HDMA_CHANNELS,
    RESULT_SCROLL_REQUEST = WRAM_BATTLE_RESULT_HDMA_REQUEST,
};

static void FillResultScrollRun(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t scroll, uint16_t end) {
    OpLoadA(cpu, scroll);
    do {
        OpSta(memory, cpu, OpLongX(cpu, RESULT_SCROLL_TABLE));
        OpInx(cpu);
        OpInx(cpu);
        OpDecA(cpu);
        OpCpx(cpu, end);
    } while (!cpu->zero);
}

static void SetResultHdmaDescriptor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t address, uint8_t control, uint8_t reg, uint16_t source) {
    if (control == 0u)
        TransferDirectToA(cpu);
    else
        OpLoadA(cpu, control);
    OpSta(memory, cpu, OpAbs(cpu, address));
    OpLoadA(cpu, reg);
    OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(address + 1u)));
    OpLdx(cpu, source);
    OpWriteX(memory, cpu, OpAbs(cpu, (uint16_t)(address + 2u)), cpu->x);
    OpLoadA(cpu, 0x85u);
    OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(address + 4u)));
}

Lufia2ExecutionResult Lufia2BattleConfigureResultHdma(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    BattleContext battle = BattleContextCreate(memory, cpu, child, context, 0x85u);

    OpLdy(cpu, 0u);
    OpTyx(cpu);
    OpWriteX(memory, cpu, OpAbs(cpu, RESULT_SCROLL_PHASE), cpu->y);
    OpLdx(cpu, 0u);
    OpRepWidths(cpu, 0x20u);
    FillResultScrollRun(memory, cpu, 0x00c0u, 0x0022u);
    FillResultScrollRun(memory, cpu, 0x0080u, 0x005au);
    FillResultScrollRun(memory, cpu, 0xff40u, 0x009au);
    FillResultScrollRun(memory, cpu, 0xff40u, 0x00dau);
    OpLoadA(cpu, 0x00c0u);
    OpSta(memory, cpu, OpLongX(cpu, RESULT_SCROLL_TABLE));
    OpLoadA(cpu, 0x0017u);
    OpSta(memory, cpu, OpLongX(cpu, RESULT_SCROLL_TABLE + 2u));
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpAbs(cpu, RESULT_SCROLL_PHASE), cpu->x);
    if (!BattleCall(&battle, 0xa9d0u, 0x85aa3du, 2u))
        return BattleChildUnwound(&battle);

    SetResultHdmaDescriptor(memory, cpu,
        WRAM_BATTLE_TRANSITION_HDMA_CONTROL, 0x42u, 0x12u, 0xa03eu);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpAbs(cpu, 0x4317u));
    SetResultHdmaDescriptor(memory, cpu,
        WRAM_BATTLE_COLOR_HDMA_DESCRIPTOR, 0u, 0x2du, 0xa05du);
    SetResultHdmaDescriptor(memory, cpu,
        WRAM_BATTLE_RESULT_COLOR_HDMA_DESCRIPTOR, 1u, 0x30u, 0xa062u);
    SetResultHdmaDescriptor(memory, cpu,
        WRAM_BATTLE_LAYER_HDMA_DESCRIPTOR, 0u, 0x2cu, 0xa069u);
    OpLoadA(cpu, 0x36u);
    OpTestBits(memory, cpu, OpDp(cpu, 0xdau), 1u);
    OpLoadA(cpu, 0x0cu);
    OpSta(memory, cpu, OpAbs(cpu, RESULT_SCROLL_CHANNELS));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, RESULT_SCROLL_REQUEST));
    OpLoadA(cpu, 2u);
    OpTestBits(memory, cpu, OpDp(cpu, 0xdbu), 1u);
    return ExecutionReturned(0x85aa3cu);
}
