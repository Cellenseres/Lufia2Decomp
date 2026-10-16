#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/world_map.h"
#include "system/wram.h"

enum {
    CIRCLE_RADIUS = 0x54u,
    CIRCLE_LOWER_STEP = 0x56u,
    CIRCLE_CENTER_X = 0x58u,
    CIRCLE_CENTER_Y = 0x5au,
    CIRCLE_DESTINATION = 0x60u,
    CIRCLE_DECISION = 4u,
    CIRCLE_SCANLINE = 0u,
    CIRCLE_LEADING_ROWS = 6u,
    CIRCLE_ROW_POINTER = 0x0bu,
    CIRCLE_EMPTY_POINTER = 0x0eu,
    CIRCLE_ROWS_LEFT = 4u,
    CIRCLE_EXTENTS = 0xde00u,
    CIRCLE_SCREEN_HEIGHT = 0xe0u
};

static bool CircleEntry(const Lufia2CpuState *cpu, bool table_bank) {
    return cpu->program_bank == 0x86u && cpu->accumulator_is_8_bit &&
           !cpu->index_is_8_bit && !cpu->direct_page && !cpu->decimal &&
           (!table_bank || cpu->data_bank == 0x7fu);
}

Lufia2ExecutionResult Lufia2WorldMapBuildCircleExtentTable(const Lufia2Memory *memory,
                                                           Lufia2CpuState *cpu) {
    if (!CircleEntry(cpu, true))
        return ExecutionHandoff(cpu, 0x86c4a8u);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x30u);
    OpStz(memory, cpu, OpDp(cpu, CIRCLE_DECISION + 1u));
    OpStz(memory, cpu, OpDp(cpu, CIRCLE_SCANLINE + 1u));
    OpLdx(cpu, Read8(memory, DirectAddress(cpu, CIRCLE_DECISION)));
    OpLdy(cpu, 0u);
    for (;;) {
        Write8(memory, DirectAddress(cpu, CIRCLE_SCANLINE), (uint8_t)cpu->y);
        OpCpx(cpu, Read8(memory, DirectAddress(cpu, CIRCLE_SCANLINE)));
        if (!cpu->carry)
            break;
        OpTxa(cpu);
        OpSepWidths(cpu, 0x20u);
        OpSta(memory, cpu, OpAbsY(cpu, CIRCLE_EXTENTS));
        OpRepWidths(cpu, 0x20u);
        OpTya(cpu);
        OpLdy(cpu, (uint16_t)(cpu->y + 1u));
        OpAslA(cpu);
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffffu));
        OpAdc(memory, cpu, OpDp(cpu, CIRCLE_DECISION));
        OpSta(memory, cpu, OpDp(cpu, CIRCLE_DECISION));
        if (cpu->negative) {
            OpTya(cpu);
            OpLdx(cpu, (uint16_t)(cpu->x - 1u));
            OpSepWidths(cpu, 0x20u);
            OpSta(memory, cpu, OpAbsX(cpu, CIRCLE_EXTENTS));
            OpRepWidths(cpu, 0x20u);
            OpTxa(cpu);
            OpAslA(cpu);
            OpAdc(memory, cpu, OpDp(cpu, CIRCLE_DECISION));
            OpSta(memory, cpu, OpDp(cpu, CIRCLE_DECISION));
        }
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x86c4dfu);
}

static void AdvanceCircleHeader(Lufia2CpuState *cpu) {
    for (unsigned byte = 0u; byte < 3u; ++byte)
        OpLdx(cpu, (uint16_t)(cpu->x + 1u));
}

static void BeginCircleRows(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSta(memory, cpu, OpDp(cpu, CIRCLE_ROWS_LEFT));
    OpOraValue(cpu, 0x80u);
    OpSta(memory, cpu, OpAbsX(cpu, 0u));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, CIRCLE_ROW_POINTER));
    OpSta(memory, cpu, OpAbsX(cpu, 1u));
    AdvanceCircleHeader(cpu);
    PushIndex(memory, cpu);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
}

static void WriteCircleRow(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, CIRCLE_CENTER_X));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbsY(cpu, CIRCLE_EXTENTS)));
    if (!cpu->carry)
        OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbsX(cpu, 0u));
    OpLda(memory, cpu, OpDp(cpu, CIRCLE_CENTER_X));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbsY(cpu, CIRCLE_EXTENTS));
    if (cpu->carry)
        OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbsX(cpu, 1u));
    OpLdx(cpu, (uint16_t)(cpu->x + 1u));
    OpLdx(cpu, (uint16_t)(cpu->x + 1u));
}

static void FinishCircleRows(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpWrite16(memory, OpDp(cpu, CIRCLE_ROW_POINTER), cpu->x);
    OpPullX(memory, cpu);
}

Lufia2ExecutionResult Lufia2WorldMapAppendUpperCircleRows(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu) {
    if (!CircleEntry(cpu, true))
        return ExecutionHandoff(cpu, 0x86c40au);
    BeginCircleRows(memory, cpu);
    do {
        WriteCircleRow(memory, cpu);
        OpLdy(cpu, (uint16_t)(cpu->y - 1u));
        OpStepMem(memory, cpu, OpDp(cpu, CIRCLE_ROWS_LEFT), -1);
    } while (!cpu->zero);
    FinishCircleRows(memory, cpu);
    return ExecutionReturned(0x86c443u);
}

Lufia2ExecutionResult Lufia2WorldMapAppendLowerCircleRows(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu) {
    if (!CircleEntry(cpu, true))
        return ExecutionHandoff(cpu, 0x86c444u);
    BeginCircleRows(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, CIRCLE_LOWER_STEP));
    const bool double_step = !cpu->zero;
    do {
        WriteCircleRow(memory, cpu);
        OpLdy(cpu, (uint16_t)(cpu->y + 1u));
        if (double_step)
            OpLdy(cpu, (uint16_t)(cpu->y + 1u));
        OpStepMem(memory, cpu, OpDp(cpu, CIRCLE_ROWS_LEFT), -1);
    } while (!cpu->zero);
    FinishCircleRows(memory, cpu);
    return ExecutionReturned(double_step ? 0x86c4a7u : 0x86c481u);
}

typedef struct CircleWindow {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
} CircleWindow;

static bool CallCircleChild(CircleWindow *window, uint32_t site, uint32_t target) {
    if (!CallChildWithFrame(window->memory, window->cpu, window->child, window->context,
                            site, target, 2u, 0x86u)) {
        window->result = ExecutionReturned(site);
        window->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (!CircleEntry(window->cpu, true)) {
        window->result = ExecutionHandoff(window->cpu, site + 3u);
        return false;
    }
    return true;
}

static void WriteEmptyCircleBand(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSta(memory, cpu, OpAbsX(cpu, 0u));
    OpLda(memory, cpu, OpDp(cpu, CIRCLE_EMPTY_POINTER));
    OpSta(memory, cpu, OpAbsX(cpu, 1u));
    AdvanceCircleHeader(cpu);
}

static void PrepareCircleStream(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, CIRCLE_DESTINATION));
    OpTax(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x18u);
    OpSta(memory, cpu, OpDp(cpu, CIRCLE_ROW_POINTER));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbsX(cpu, 0x16u));
    OpTxa(cpu);
    OpAdcValue(cpu, 0x16u);
    OpSta(memory, cpu, OpDp(cpu, CIRCLE_EMPTY_POINTER));
    OpStz(memory, cpu, OpDp(cpu, CIRCLE_LEADING_ROWS));
    OpLda(memory, cpu, OpDp(cpu, CIRCLE_CENTER_Y));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, CIRCLE_RADIUS)));
    if (!cpu->negative && !cpu->zero) {
        OpSta(memory, cpu, OpDp(cpu, CIRCLE_LEADING_ROWS));
        OpLsrA(cpu);
        PushAccumulator16(memory, cpu);
        OpAdcValue(cpu, 0u);
        WriteEmptyCircleBand(memory, cpu);
        PullAccumulator16(memory, cpu);
        if (!cpu->zero)
            WriteEmptyCircleBand(memory, cpu);
    }
}

static bool AppendCircleUpperHalf(CircleWindow *window) {
    const Lufia2Memory *memory = window->memory;
    Lufia2CpuState *cpu = window->cpu;
    OpLda(memory, cpu, OpDp(cpu, CIRCLE_CENTER_Y));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, CIRCLE_LEADING_ROWS)));
    if (!cpu->zero) {
        OpTay(cpu);
        OpLdy(cpu, (uint16_t)(cpu->y - 1u));
        OpSepWidths(cpu, 0x20u);
        OpLsrA(cpu);
        PushAccumulator8(memory, cpu);
        OpAdcValue(cpu, 0u);
        if (!CallCircleChild(window, 0x86c3c2u, 0x86c40au))
            return false;
        OpLoadA(cpu, Pull8(memory, cpu));
        if (!cpu->zero && !CallCircleChild(window, 0x86c3c8u, 0x86c40au))
            return false;
        OpRepWidths(cpu, 0x20u);
    }
    return true;
}

static bool AppendCircleLowerHalf(CircleWindow *window) {
    const Lufia2Memory *memory = window->memory;
    Lufia2CpuState *cpu = window->cpu;
    OpLda(memory, cpu, OpDp(cpu, CIRCLE_LOWER_STEP));
    if (!cpu->zero)
        OpLsrMem(memory, cpu, OpDp(cpu, CIRCLE_RADIUS));
    OpLoadA(cpu, CIRCLE_SCREEN_HEIGHT);
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, CIRCLE_CENTER_Y)));
    OpCmp(memory, cpu, OpDp(cpu, CIRCLE_RADIUS));
    if (cpu->carry)
        OpLda(memory, cpu, OpDp(cpu, CIRCLE_RADIUS));
    Push8(memory, cpu, PackStatus(cpu));
    OpLdy(cpu, 0u);
    OpSepWidths(cpu, 0x20u);
    OpLsrA(cpu);
    PushAccumulator8(memory, cpu);
    OpAdcValue(cpu, 0u);
    if (!CallCircleChild(window, 0x86c3e9u, 0x86c444u))
        return false;
    OpLoadA(cpu, Pull8(memory, cpu));
    if (!cpu->zero && !CallCircleChild(window, 0x86c3efu, 0x86c444u))
        return false;
    UnpackStatus(cpu, Pull8(memory, cpu));
    if (cpu->carry) {
        OpLoadA(cpu, 0x81u);
        WriteEmptyCircleBand(memory, cpu);
    }
    return true;
}

Lufia2ExecutionResult Lufia2WorldMapBuildCircularWindow(const Lufia2Memory *memory,
                                                        Lufia2CpuState *cpu,
                                                        Lufia2PushedChildCall child,
                                                        void *context) {
    if (!child || !CircleEntry(cpu, false))
        return ExecutionHandoff(cpu, 0x86c358u);
    OpLda(memory, cpu, OpDp(cpu, CIRCLE_RADIUS));
    if (cpu->zero) {
        OpSta(memory, cpu, DirectLongPointer(memory, cpu, CIRCLE_DESTINATION));
        return ExecutionReturned(0x86c35eu);
    }
    OpStz(memory, cpu, OpDp(cpu, CIRCLE_RADIUS + 1u));
    OpStz(memory, cpu, OpDp(cpu, CIRCLE_CENTER_X + 1u));
    OpStz(memory, cpu, OpDp(cpu, CIRCLE_CENTER_Y + 1u));
    PushDataBank(memory, cpu);
    OpLoadA(cpu, 0x7fu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, CIRCLE_RADIUS));
    OpSta(memory, cpu, OpDp(cpu, CIRCLE_DECISION));
    CircleWindow window = {memory, cpu, child, context, ExecutionReturned(0x86c409u)};
    if (!CallCircleChild(&window, 0x86c36eu, 0x86c4a8u))
        return window.result;
    PrepareCircleStream(memory, cpu);
    if (!AppendCircleUpperHalf(&window) || !AppendCircleLowerHalf(&window))
        return window.result;
    OpSepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpAbsX(cpu, 0u));
    PullDataBank(memory, cpu);
    return window.result;
}
