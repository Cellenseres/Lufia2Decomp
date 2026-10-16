#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/world_map.h"
#include "world/world_scene_resources_internal.h"

enum {
    EDGE_INCREMENT = 0u,
    EDGE_ANGLE = 2u,
    EDGE_ROWS = 3u,
    EDGE_POSITION = 4u,
    EDGE_START_ROW = 6u,
    WINDOW_FIRST_START = 0x0bu,
    WINDOW_SECOND_START = 0x0cu,
    WINDOW_FIRST_END = 0x0eu,
    WINDOW_SECOND_END = 0x0fu,
    WINDOW_OFFSET_Y = 0x26u,
    WINDOW_HEIGHT = 0x28u,
    WINDOW_CENTER_Y = 0x5au,
    WINDOW_BUFFER = 0x60u,
    WINDOW_HEADER_SIZE = 3u,
    WINDOW_ROW_OFFSET = 0x12u,
    WINDOW_SCREEN_HEIGHT = 0xe0u,
    WINDOW_EMPTY_ROW = 0xdbfeu,
    EDGE_SLOPES = 0x86dbb9u
};

static bool WindowEdgeEntry(const Lufia2CpuState *cpu, bool narrow) {
    return cpu->program_bank == 0x86u && cpu->data_bank == 0x7fu &&
           cpu->accumulator_is_8_bit == narrow && !cpu->index_is_8_bit &&
           !cpu->direct_page && !cpu->decimal;
}

static Lufia2ExecutionResult DrawWindowEdge(const Lufia2Memory *memory,
                                            Lufia2CpuState *cpu, bool complement,
                                            bool subtract, bool descending,
                                            uint32_t entry, uint32_t terminal) {
    if (!WindowEdgeEntry(cpu, false))
        return ExecutionHandoff(cpu, entry);
    OpLda(memory, cpu, OpDp(cpu, EDGE_ANGLE));
    OpAndValue(cpu, 0x3fu);
    if (complement) {
        if (cpu->zero)
            return ExecutionReturned(terminal);
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffffu));
        OpIncA(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x40u);
    }
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, EDGE_SLOPES));
    OpSta(memory, cpu, OpDp(cpu, EDGE_INCREMENT));
    cpu->carry = subtract;
    for (;;) {
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpDp(cpu, EDGE_POSITION));
        if (subtract)
            OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, EDGE_INCREMENT)));
        else
            OpAdc(memory, cpu, OpDp(cpu, EDGE_INCREMENT));
        if (cpu->carry != subtract)
            break;
        OpSta(memory, cpu, OpDp(cpu, EDGE_POSITION));
        OpSepWidths(cpu, 0x20u);
        ExchangeAccumulatorBytes(cpu);
        OpSta(memory, cpu, OpAbsY(cpu, 0u));
        for (unsigned byte = 0u; byte < 2u; ++byte)
            OpLdy(cpu, (uint16_t)(cpu->y + (descending ? -1 : 1)));
        OpStepMem(memory, cpu, OpDp(cpu, EDGE_ROWS), -1);
        if (cpu->zero)
            break;
    }
    return ExecutionReturned(terminal);
}

Lufia2ExecutionResult Lufia2WorldMapDrawUpperRightWindowEdge(const Lufia2Memory *memory,
                                                             Lufia2CpuState *cpu) {
    return DrawWindowEdge(memory, cpu, true, false, true, 0x86ca2fu, 0x86ca5du);
}

Lufia2ExecutionResult Lufia2WorldMapDrawLowerRightWindowEdge(const Lufia2Memory *memory,
                                                             Lufia2CpuState *cpu) {
    return DrawWindowEdge(memory, cpu, false, false, false, 0x86ca5eu, 0x86ca82u);
}

Lufia2ExecutionResult Lufia2WorldMapDrawLowerLeftWindowEdge(const Lufia2Memory *memory,
                                                            Lufia2CpuState *cpu) {
    return DrawWindowEdge(memory, cpu, true, true, false, 0x86ca83u, 0x86cab1u);
}

Lufia2ExecutionResult Lufia2WorldMapDrawUpperLeftWindowEdge(const Lufia2Memory *memory,
                                                            Lufia2CpuState *cpu) {
    return DrawWindowEdge(memory, cpu, false, true, true, 0x86cab2u, 0x86cad6u);
}

Lufia2ExecutionResult Lufia2WorldMapDrawSlopedWindowEdge(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu,
                                                         Lufia2PushedChildCall child,
                                                         void *context) {
    if (!child || !WindowEdgeEntry(cpu, true))
        return ExecutionHandoff(cpu, 0x86ca06u);
    PushY(memory, cpu);
    OpStz(memory, cpu, OpDp(cpu, EDGE_POSITION));
    OpStz(memory, cpu, OpDp(cpu, EDGE_START_ROW + 1u));
    OpRepWidths(cpu, 0x20u);
    OpTya(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, EDGE_START_ROW));
    OpAdc(memory, cpu, OpDp(cpu, EDGE_START_ROW));
    OpAdc(memory, cpu, OpDp(cpu, WINDOW_BUFFER));
    OpTay(cpu);
    OpLda(memory, cpu, OpDp(cpu, EDGE_ANGLE));
    for (unsigned bit = 0u; bit < 3u; ++bit)
        OpAslA(cpu);
    ExchangeAccumulatorBytes(cpu);
    OpAndValue(cpu, 6u);
    OpTax(cpu);
    uint16_t vector = (uint16_t)(0xca27u + cpu->x);
    uint32_t target = 0x860000u | Read16Bank(memory, 0x86u, vector);
    if (!CallChildWithFrame(memory, cpu, child, context, 0x86ca20u, target, 2u,
                            0x86u)) {
        Lufia2ExecutionResult result = ExecutionReturned(0x86ca20u);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    if (cpu->program_bank != 0x86u || cpu->data_bank != 0x7fu || cpu->index_is_8_bit ||
        cpu->direct_page || cpu->decimal)
        return ExecutionHandoff(cpu, 0x86ca23u);
    OpSepWidths(cpu, 0x20u);
    OpPullY(memory, cpu);
    return ExecutionReturned(0x86ca26u);
}

static void AdvanceWindowHeader(Lufia2CpuState *cpu) {
    for (unsigned byte = 0u; byte < WINDOW_HEADER_SIZE; ++byte)
        OpLdx(cpu, (uint16_t)(cpu->x + 1u));
}

Lufia2ExecutionResult Lufia2WorldMapInitializeWindowRows(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu) {
    if (!WindowEdgeEntry(cpu, true))
        return ExecutionHandoff(cpu, 0x86cad7u);
    OpLdx(cpu, OpRead16(memory, OpDp(cpu, WINDOW_BUFFER)));
    OpLoadA(cpu, 0xf0u);
    OpSta(memory, cpu, OpAbsX(cpu, 0u));
    OpSta(memory, cpu, OpAbsX(cpu, WINDOW_HEADER_SIZE));
    OpStz(memory, cpu, OpAbsX(cpu, 6u));
    OpRepWidths(cpu, 0x20u);
    OpTxa(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, WINDOW_ROW_OFFSET);
    OpSta(memory, cpu, OpAbsX(cpu, 1u));
    PushAccumulator16(memory, cpu);
    OpAdcValue(cpu, WINDOW_SCREEN_HEIGHT);
    OpSta(memory, cpu, OpAbsX(cpu, 4u));
    OpLoadA(cpu, 28u);
    OpSta(memory, cpu, OpDp(cpu, EDGE_INCREMENT));
    OpPullX(memory, cpu);
    cpu->carry = 0u;
    do {
        OpTya(cpu);
        for (unsigned byte = 0u; byte < 16u; byte += 2u)
            OpSta(memory, cpu, OpAbsX(cpu, (uint16_t)byte));
        OpTxa(cpu);
        OpAdcValue(cpu, 16u);
        OpTax(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, EDGE_INCREMENT), -1);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x86cb20u);
}

Lufia2ExecutionResult Lufia2WorldMapCountWindowEdgeRows(const Lufia2Memory *memory,
                                                        Lufia2CpuState *cpu) {
    if (!WindowEdgeEntry(cpu, true))
        return ExecutionHandoff(cpu, 0x86c831u);
    cpu->carry = 1u;
    OpSbcValue(cpu, 0x40u);
    if (!cpu->negative) {
        OpLoadA(cpu, WINDOW_SCREEN_HEIGHT);
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, EDGE_INCREMENT)));
    } else {
        OpLda(memory, cpu, OpDp(cpu, EDGE_INCREMENT));
        OpIncA(cpu);
    }
    OpSta(memory, cpu, OpDp(cpu, EDGE_ROWS));
    return ExecutionReturned(0x86c842u);
}

Lufia2ExecutionResult Lufia2WorldMapResolveWindowEdgeEnd(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu) {
    if (!WindowEdgeEntry(cpu, true))
        return ExecutionHandoff(cpu, 0x86c843u);
    OpLda(memory, cpu, OpDp(cpu, EDGE_ANGLE));
    cpu->carry = 1u;
    OpSbcValue(cpu, 0x40u);
    if (!cpu->negative) {
        OpLoadA(cpu, WINDOW_SCREEN_HEIGHT);
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, EDGE_ROWS)));
    } else {
        OpLda(memory, cpu, OpDp(cpu, EDGE_ROWS));
    }
    return ExecutionReturned(0x86c853u);
}

static void WriteEmptyWindowHeader(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSta(memory, cpu, OpAbsX(cpu, 0u));
    OpLoadA(cpu, WINDOW_EMPTY_ROW);
    OpSta(memory, cpu, OpAbsX(cpu, 1u));
    AdvanceWindowHeader(cpu);
}

static void WriteLeadingWindowRows(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSta(memory, cpu, OpDp(cpu, EDGE_INCREMENT));
    OpLsrA(cpu);
    PushAccumulator16(memory, cpu);
    OpAdcValue(cpu, 0u);
    WriteEmptyWindowHeader(memory, cpu);
    PullAccumulator16(memory, cpu);
    if (!cpu->zero)
        WriteEmptyWindowHeader(memory, cpu);
}

static void WriteWindowRowPointer(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpAslA(cpu);
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, EDGE_START_ROW));
    OpLda(memory, cpu, OpDp(cpu, EDGE_INCREMENT));
    OpAslA(cpu);
    OpAdc(memory, cpu, OpDp(cpu, WINDOW_BUFFER));
    OpAdcValue(cpu, WINDOW_ROW_OFFSET);
    OpSta(memory, cpu, OpAbsX(cpu, 1u));
}

static void FillEmptyWindowRows(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLoadA(cpu, 0xffu);
    do {
        OpSta(memory, cpu, OpAbsX(cpu, WINDOW_ROW_OFFSET));
        OpLdx(cpu, (uint16_t)(cpu->x + 1u));
        OpLdx(cpu, (uint16_t)(cpu->x + 1u));
        OpLdy(cpu, (uint16_t)(cpu->y - 1u));
    } while (!cpu->zero);
}

static void ClearWindowLeadingRows(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    if (!cpu->zero) {
        OpTay(cpu);
        OpLdx(cpu, OpRead16(memory, OpDp(cpu, WINDOW_BUFFER)));
        FillEmptyWindowRows(memory, cpu);
    }
    OpSepWidths(cpu, 0x20u);
}

static void ClearWindowTrailingRows(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpCmpValue(cpu, WINDOW_SCREEN_HEIGHT);
    if (!cpu->carry) {
        OpRepWidths(cpu, 0x20u);
        OpAndValue(cpu, 0xffu);
        OpSta(memory, cpu, OpDp(cpu, EDGE_INCREMENT));
        OpAslA(cpu);
        OpAdc(memory, cpu, OpDp(cpu, WINDOW_BUFFER));
        OpTax(cpu);
        OpLoadA(cpu, WINDOW_SCREEN_HEIGHT);
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, EDGE_INCREMENT)));
        OpTay(cpu);
        FillEmptyWindowRows(memory, cpu);
        OpSepWidths(cpu, 0x20u);
    }
}

Lufia2ExecutionResult Lufia2WorldMapBuildLowerWindowBand(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu) {
    if (!WindowEdgeEntry(cpu, true))
        return ExecutionHandoff(cpu, 0x86c854u);
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, OpRead16(memory, OpDp(cpu, WINDOW_BUFFER)));
    OpLda(memory, cpu, OpDp(cpu, WINDOW_CENTER_Y));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, WINDOW_OFFSET_Y));
    OpCmpValue(cpu, WINDOW_SCREEN_HEIGHT);
    if (!cpu->carry) {
        WriteLeadingWindowRows(memory, cpu);
        OpLda(memory, cpu, OpDp(cpu, EDGE_INCREMENT));
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpDp(cpu, WINDOW_HEIGHT));
        OpCmpValue(cpu, WINDOW_SCREEN_HEIGHT);
        OpLda(memory, cpu, OpDp(cpu, WINDOW_HEIGHT));
        if (cpu->carry) {
            OpLoadA(cpu, WINDOW_SCREEN_HEIGHT);
            cpu->carry = 1u;
            OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, EDGE_INCREMENT)));
        }
        Push8(memory, cpu, PackStatus(cpu));
        OpLsrA(cpu);
        PushAccumulator16(memory, cpu);
        OpAdcValue(cpu, 0x80u);
        OpSta(memory, cpu, OpAbsX(cpu, 0u));
        WriteWindowRowPointer(memory, cpu);
        AdvanceWindowHeader(cpu);
        OpAdc(memory, cpu, OpDp(cpu, EDGE_START_ROW));
        OpTay(cpu);
        PullAccumulator16(memory, cpu);
        if (!cpu->zero) {
            OpOraValue(cpu, 0x80u);
            OpSta(memory, cpu, OpAbsX(cpu, 0u));
            OpTya(cpu);
            OpSta(memory, cpu, OpAbsX(cpu, 1u));
            AdvanceWindowHeader(cpu);
        }
        UnpackStatus(cpu, Pull8(memory, cpu));
        if (!cpu->carry) {
            OpLoadA(cpu, 0x81u);
            WriteEmptyWindowHeader(memory, cpu);
        }
    }
    OpSepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpAbsX(cpu, 0u));
    OpLda(memory, cpu, OpDp(cpu, WINDOW_FIRST_START));
    OpCmp(memory, cpu, OpDp(cpu, WINDOW_SECOND_START));
    if (!cpu->carry)
        OpLda(memory, cpu, OpDp(cpu, WINDOW_SECOND_START));
    ClearWindowLeadingRows(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, WINDOW_FIRST_END));
    OpCmp(memory, cpu, OpDp(cpu, WINDOW_SECOND_END));
    if (!cpu->carry)
        OpLda(memory, cpu, OpDp(cpu, WINDOW_SECOND_END));
    ClearWindowTrailingRows(memory, cpu);
    return ExecutionReturned(0x86c927u);
}

Lufia2ExecutionResult Lufia2WorldMapBuildUpperWindowBand(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu) {
    if (!WindowEdgeEntry(cpu, true))
        return ExecutionHandoff(cpu, 0x86c928u);
    OpRepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpDp(cpu, EDGE_INCREMENT));
    OpLdx(cpu, OpRead16(memory, OpDp(cpu, WINDOW_BUFFER)));
    OpLda(memory, cpu, OpDp(cpu, WINDOW_CENTER_Y));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, WINDOW_OFFSET_Y)));
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, WINDOW_HEIGHT)));
    bool leading = !cpu->zero;
    if (leading) {
        OpCmpValue(cpu, WINDOW_SCREEN_HEIGHT);
        if (!cpu->carry) {
            WriteLeadingWindowRows(memory, cpu);
            OpLoadA(cpu, 0u);
        }
    }
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, WINDOW_HEIGHT));
    OpSta(memory, cpu, OpDp(cpu, EDGE_START_ROW));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, EDGE_INCREMENT));
    OpCmpValue(cpu, WINDOW_SCREEN_HEIGHT);
    Push8(memory, cpu, PackStatus(cpu));
    OpLda(memory, cpu, OpDp(cpu, EDGE_START_ROW));
    if (cpu->carry) {
        OpLoadA(cpu, WINDOW_SCREEN_HEIGHT);
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, EDGE_INCREMENT)));
    }
    OpLsrA(cpu);
    PushAccumulator16(memory, cpu);
    OpAdcValue(cpu, 0x80u);
    OpSta(memory, cpu, OpAbsX(cpu, 0u));
    WriteWindowRowPointer(memory, cpu);
    OpAdc(memory, cpu, OpDp(cpu, EDGE_START_ROW));
    OpTay(cpu);
    PullAccumulator16(memory, cpu);
    if (!cpu->zero) {
        AdvanceWindowHeader(cpu);
        OpOraValue(cpu, 0x80u);
        OpSta(memory, cpu, OpAbsX(cpu, 0u));
        OpTya(cpu);
        OpSta(memory, cpu, OpAbsX(cpu, 1u));
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
    if (!cpu->carry) {
        OpLoadA(cpu, 0x81u);
        OpSta(memory, cpu, OpAbsX(cpu, 3u));
        OpLoadA(cpu, WINDOW_EMPTY_ROW);
        OpSta(memory, cpu, OpAbsX(cpu, 4u));
        AdvanceWindowHeader(cpu);
    }
    OpSepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpAbsX(cpu, 3u));
    OpLda(memory, cpu, OpDp(cpu, WINDOW_FIRST_END));
    OpCmp(memory, cpu, OpDp(cpu, WINDOW_SECOND_END));
    if (cpu->carry)
        OpLda(memory, cpu, OpDp(cpu, WINDOW_SECOND_END));
    ClearWindowLeadingRows(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, WINDOW_FIRST_START));
    OpCmp(memory, cpu, OpDp(cpu, WINDOW_SECOND_START));
    if (cpu->carry)
        OpLda(memory, cpu, OpDp(cpu, WINDOW_SECOND_START));
    OpIncA(cpu);
    ClearWindowTrailingRows(memory, cpu);
    return ExecutionReturned(0x86ca05u);
}

enum {
    SLOPED_ANGLE = 0x22u,
    SLOPED_ANGLE_WIDTH = 0x23u,
    SLOPED_DISTANCE = 0x24u,
    SLOPED_UPPER_ANGLE = 0x13u,
    SLOPED_LOWER_ANGLE = 0x14u,
    SLOPED_POINT_X = 0x15u,
    SLOPED_POINT_Y = 0x17u,
    SLOPED_OTHER_X = 0x19u,
    SLOPED_OTHER_Y = 0x1cu,
    SLOPED_CLIP_ANGLE = 0x11u,
    SLOPED_MOTION_ANGLE = 0x1248u,
    SLOPED_MOTION_DISTANCE = 0x1249u,
    SLOPED_CENTER_X = 0x58u
};

typedef struct SlopedWindow {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
    uint8_t bank;
} SlopedWindow;

static bool CallSlopedWindowChild(SlopedWindow *window, uint32_t site,
                                  uint32_t target) {
    if (!CallChildWithFrame(window->memory, window->cpu, window->child, window->context,
                            site, target, 2u, 0x86u)) {
        window->result = ExecutionReturned(site);
        window->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    const Lufia2CpuState *cpu = window->cpu;
    if (cpu->program_bank != 0x86u || cpu->data_bank != window->bank ||
        !cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->direct_page ||
        cpu->decimal) {
        window->result = ExecutionHandoff(window->cpu, site + 3u);
        return false;
    }
    return true;
}

static void PrepareSlopedWindowAngles(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, SLOPED_ANGLE_WIDTH));
    OpLsrA(cpu);
    OpSta(memory, cpu, OpDp(cpu, EDGE_INCREMENT));
    OpAdc(memory, cpu, OpDp(cpu, SLOPED_ANGLE));
    OpSta(memory, cpu, OpDp(cpu, SLOPED_UPPER_ANGLE));
    OpLda(memory, cpu, OpDp(cpu, SLOPED_ANGLE));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, EDGE_INCREMENT)));
    OpSta(memory, cpu, OpDp(cpu, SLOPED_LOWER_ANGLE));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, SLOPED_DISTANCE));
    OpLsrA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, SLOPED_MOTION_DISTANCE));
    OpAdcValue(cpu, 0u);
    PushAccumulator16(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, SLOPED_ANGLE));
    cpu->carry = 1u;
    OpSbcValue(cpu, 0x40u);
    OpSta(memory, cpu, OpAbs(cpu, SLOPED_MOTION_ANGLE));
}

static void AddSlopedWindowCenter(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, SLOPED_CENTER_X));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, 8u));
    OpSta(memory, cpu, OpDp(cpu, SLOPED_POINT_X));
    OpLda(memory, cpu, OpDp(cpu, WINDOW_CENTER_Y));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, 0x0bu));
    OpSta(memory, cpu, OpDp(cpu, SLOPED_POINT_Y));
    OpSepWidths(cpu, 0x20u);
}

static bool PositionSlopedWindowEndpoints(SlopedWindow *window) {
    const Lufia2Memory *memory = window->memory;
    Lufia2CpuState *cpu = window->cpu;
    PrepareSlopedWindowAngles(memory, cpu);
    if (!CallSlopedWindowChild(window, 0x86c627u, 0x86a417u))
        return false;
    AddSlopedWindowCenter(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, SLOPED_LOWER_ANGLE));
    OpSta(memory, cpu, OpDp(cpu, SLOPED_CLIP_ANGLE));
    if (!CallSlopedWindowChild(window, 0x86c640u, 0x86cb21u))
        return false;
    OpLdx(cpu, OpRead16(memory, OpDp(cpu, SLOPED_POINT_X)));
    OpWrite16(memory, OpDp(cpu, SLOPED_OTHER_X), cpu->x);
    OpLdx(cpu, OpRead16(memory, OpDp(cpu, SLOPED_POINT_Y)));
    OpWrite16(memory, OpDp(cpu, SLOPED_OTHER_Y), cpu->x);
    OpPullX(memory, cpu);
    OpWrite16(memory, OpAbs(cpu, SLOPED_MOTION_DISTANCE), cpu->x);
    OpLda(memory, cpu, OpDp(cpu, SLOPED_ANGLE));
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x40u);
    OpSta(memory, cpu, OpAbs(cpu, SLOPED_MOTION_ANGLE));
    if (!CallSlopedWindowChild(window, 0x86c657u, 0x86a417u))
        return false;
    AddSlopedWindowCenter(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, SLOPED_UPPER_ANGLE));
    OpSta(memory, cpu, OpDp(cpu, SLOPED_CLIP_ANGLE));
    return CallSlopedWindowChild(window, 0x86c670u, 0x86cb21u);
}

static bool DrawSlopedBandEdge(SlopedWindow *window, bool upper, bool other) {
    const Lufia2Memory *memory = window->memory;
    Lufia2CpuState *cpu = window->cpu;
    uint8_t point_y = other ? SLOPED_OTHER_Y : SLOPED_POINT_Y;
    OpLda(memory, cpu, OpDp(cpu, (uint8_t)(point_y + 1u)));
    if (!cpu->zero)
        return true;
    OpLda(memory, cpu, OpDp(cpu, other ? SLOPED_LOWER_ANGLE : SLOPED_UPPER_ANGLE));
    OpSta(memory, cpu, OpDp(cpu, EDGE_ANGLE));
    OpLda(memory, cpu, OpDp(cpu, other ? SLOPED_OTHER_X : SLOPED_POINT_X));
    OpSta(memory, cpu, OpDp(cpu, EDGE_POSITION + 1u));
    OpLda(memory, cpu, OpDp(cpu, point_y));
    OpSta(memory, cpu, OpDp(cpu, EDGE_START_ROW));
    if (upper) {
        OpIncA(cpu);
    } else {
        OpSta(memory, cpu, OpDp(cpu, other ? WINDOW_SECOND_START : WINDOW_FIRST_START));
        OpLoadA(cpu, WINDOW_SCREEN_HEIGHT);
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, point_y)));
    }
    OpSta(memory, cpu, OpDp(cpu, EDGE_ROWS));
    uint32_t site =
        upper ? (other ? 0x86c73fu : 0x86c722u) : (other ? 0x86c6f8u : 0x86c6d2u);
    if (!CallSlopedWindowChild(window, site, 0x86ca06u))
        return false;
    if (upper) {
        OpLda(memory, cpu, OpDp(cpu, EDGE_ROWS));
    } else {
        OpLoadA(cpu, WINDOW_SCREEN_HEIGHT);
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, EDGE_ROWS)));
    }
    OpSta(memory, cpu, OpDp(cpu, other ? WINDOW_SECOND_END : WINDOW_FIRST_END));
    return true;
}

static bool DrawSlopedWindowBand(SlopedWindow *window, bool upper) {
    const Lufia2Memory *memory = window->memory;
    Lufia2CpuState *cpu = window->cpu;
    OpLdy(cpu, upper ? 0u : 0xe0e0u);
    OpWrite16(memory, OpDp(cpu, WINDOW_FIRST_END), cpu->y);
    if (!upper) {
        OpLdy(cpu, 0u);
        OpWrite16(memory, OpDp(cpu, WINDOW_FIRST_START), cpu->y);
    }
    OpLdy(cpu, upper ? 0x12u : 0x13u);
    if (!DrawSlopedBandEdge(window, upper, false))
        return false;
    OpLdy(cpu, upper ? 0x13u : 0x12u);
    if (!DrawSlopedBandEdge(window, upper, true))
        return false;
    if (upper) {
        OpLda(memory, cpu, OpDp(cpu, SLOPED_POINT_Y));
        OpSta(memory, cpu, OpDp(cpu, WINDOW_FIRST_START));
        OpLda(memory, cpu, OpDp(cpu, SLOPED_OTHER_Y));
        OpSta(memory, cpu, OpDp(cpu, WINDOW_SECOND_START));
    }
    return CallSlopedWindowChild(window, upper ? 0x86c74eu : 0x86c702u,
                                 upper ? 0x86c928u : 0x86c854u);
}

static bool DrawSplitWindowEdge(SlopedWindow *window, bool other) {
    const Lufia2Memory *memory = window->memory;
    Lufia2CpuState *cpu = window->cpu;
    uint8_t point_y = other ? SLOPED_OTHER_Y : SLOPED_POINT_Y;
    OpLda(memory, cpu, OpDp(cpu, (uint8_t)(point_y + 1u)));
    if (!cpu->zero)
        return true;
    OpLda(memory, cpu, OpDp(cpu, point_y));
    OpSta(memory, cpu, OpDp(cpu, EDGE_INCREMENT));
    OpLda(memory, cpu, OpDp(cpu, other ? SLOPED_LOWER_ANGLE : SLOPED_UPPER_ANGLE));
    OpSta(memory, cpu, OpDp(cpu, EDGE_ANGLE));
    if (!CallSlopedWindowChild(window, other ? 0x86c789u : 0x86c76au, 0x86c831u))
        return false;
    OpLda(memory, cpu, OpDp(cpu, other ? SLOPED_OTHER_X : SLOPED_POINT_X));
    OpSta(memory, cpu, OpDp(cpu, EDGE_POSITION + 1u));
    OpLda(memory, cpu, OpDp(cpu, point_y));
    OpSta(memory, cpu, OpDp(cpu, EDGE_START_ROW));
    if (!CallSlopedWindowChild(window, other ? 0x86c794u : 0x86c775u, 0x86ca06u) ||
        !CallSlopedWindowChild(window, other ? 0x86c797u : 0x86c778u, 0x86c843u))
        return false;
    OpSta(memory, cpu, OpDp(cpu, other ? WINDOW_SECOND_END : WINDOW_FIRST_END));
    return true;
}

static void ClearSplitWindowMargins(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, WINDOW_FIRST_END));
    OpCmp(memory, cpu, OpDp(cpu, WINDOW_SECOND_END));
    OpRepWidths(cpu, 0x20u);
    if (cpu->carry) {
        OpLda(memory, cpu, OpDp(cpu, WINDOW_FIRST_END));
        ExchangeAccumulatorBytes(cpu);
        OpSta(memory, cpu, OpDp(cpu, WINDOW_FIRST_END));
    }
    OpAndValue(cpu, 0xffu);
    if (!cpu->zero) {
        OpTay(cpu);
        OpLdx(cpu, OpRead16(memory, OpDp(cpu, WINDOW_BUFFER)));
        FillEmptyWindowRows(memory, cpu);
    }
    OpLda(memory, cpu, OpDp(cpu, WINDOW_SECOND_END));
    OpAndValue(cpu, 0xffu);
    OpCmpValue(cpu, WINDOW_SCREEN_HEIGHT);
    if (!cpu->carry) {
        OpSta(memory, cpu, OpDp(cpu, EDGE_INCREMENT));
        OpAslA(cpu);
        OpAdc(memory, cpu, OpDp(cpu, WINDOW_BUFFER));
        OpTax(cpu);
        OpLoadA(cpu, WINDOW_SCREEN_HEIGHT);
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, EDGE_INCREMENT)));
        OpTay(cpu);
        FillEmptyWindowRows(memory, cpu);
    }
    OpSepWidths(cpu, 0x20u);
}

static void FillSplitWindowInterior(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, SLOPED_OTHER_Y));
    OpSta(memory, cpu, OpDp(cpu, WINDOW_SECOND_END));
    OpLda(memory, cpu, OpDp(cpu, SLOPED_POINT_Y));
    OpCmp(memory, cpu, OpDp(cpu, WINDOW_SECOND_END));
    if (cpu->carry) {
        OpSta(memory, cpu, OpDp(cpu, WINDOW_SECOND_END));
        OpLda(memory, cpu, OpDp(cpu, SLOPED_OTHER_Y));
    }
    OpSta(memory, cpu, OpDp(cpu, WINDOW_FIRST_END));
    OpLda(memory, cpu, OpDp(cpu, SLOPED_OTHER_X));
    OpSta(memory, cpu, OpDp(cpu, 1u));
    OpLda(memory, cpu, OpDp(cpu, SLOPED_POINT_X));
    OpCmp(memory, cpu, OpDp(cpu, 1u));
    if (cpu->carry) {
        OpSta(memory, cpu, OpDp(cpu, 1u));
        OpLda(memory, cpu, OpDp(cpu, SLOPED_OTHER_X));
    }
    OpSta(memory, cpu, OpDp(cpu, EDGE_INCREMENT));
    OpLda(memory, cpu, OpDp(cpu, WINDOW_FIRST_END));
    OpCmpValue(cpu, WINDOW_SCREEN_HEIGHT);
    if (cpu->carry)
        return;
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    OpAslA(cpu);
    OpAdc(memory, cpu, OpDp(cpu, WINDOW_BUFFER));
    OpTax(cpu);
    OpLda(memory, cpu, OpDp(cpu, WINDOW_SECOND_END));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, WINDOW_FIRST_END)));
    OpAndValue(cpu, 0xffu);
    OpIncA(cpu);
    OpTay(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, SLOPED_ANGLE));
    if (!cpu->negative) {
        OpLdx(cpu, (uint16_t)(cpu->x + 1u));
        OpLda(memory, cpu, OpDp(cpu, EDGE_INCREMENT));
    } else {
        OpLda(memory, cpu, OpDp(cpu, 1u));
    }
    do {
        OpSta(memory, cpu, OpAbsX(cpu, WINDOW_ROW_OFFSET));
        OpLdx(cpu, (uint16_t)(cpu->x + 1u));
        OpLdx(cpu, (uint16_t)(cpu->x + 1u));
        OpDey(cpu);
    } while (!cpu->zero);
}

static bool DrawSplitSlopedWindow(SlopedWindow *window) {
    const Lufia2Memory *memory = window->memory;
    Lufia2CpuState *cpu = window->cpu;
    OpLdy(cpu, 0x12u);
    OpLda(memory, cpu, OpDp(cpu, SLOPED_ANGLE));
    if (!cpu->negative)
        OpLdy(cpu, (uint16_t)(cpu->y + 1u));
    if (!DrawSplitWindowEdge(window, false) || !DrawSplitWindowEdge(window, true))
        return false;
    ClearSplitWindowMargins(memory, cpu);
    FillSplitWindowInterior(memory, cpu);
    return true;
}

Lufia2ExecutionResult Lufia2WorldMapBuildSlopedWindow(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu,
                                                      Lufia2PushedChildCall child,
                                                      void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86c5f8u);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    OpLdy(cpu, OpRead16(memory, OpDp(cpu, WINDOW_HEIGHT)));
    SlopedWindow window = {memory, cpu, child, context, ExecutionReturned(0x86c755u),
                           0x86u};
    bool empty = cpu->zero;
    if (!empty) {
        if (!PositionSlopedWindowEndpoints(&window))
            return window.result;
        OpLda(memory, cpu, OpDp(cpu, SLOPED_POINT_Y + 1u));
        OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, SLOPED_OTHER_Y + 1u)));
        empty = cpu->negative;
    }
    if (empty) {
        OpLoadA(cpu, 0u);
        OpSta(memory, cpu, DirectLongPointer(memory, cpu, WINDOW_BUFFER));
        UnpackStatus(cpu, Pull8(memory, cpu));
        return ExecutionReturned(0x86c67eu);
    }
    PushDataBank(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, WINDOW_BUFFER + 2u));
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    window.bank = cpu->data_bank;
    OpLdy(cpu, 0xff00u);
    if (!CallSlopedWindowChild(&window, 0x86c687u, 0x86cad7u))
        return window.result;
    OpStz(memory, cpu, OpDp(cpu, EDGE_INCREMENT));
    static const uint8_t angles[] = {SLOPED_UPPER_ANGLE, SLOPED_LOWER_ANGLE};
    for (unsigned edge = 0u; edge < 2u; ++edge) {
        OpLda(memory, cpu, OpDp(cpu, angles[edge]));
        cpu->carry = 1u;
        OpSbcValue(cpu, 0x40u);
        if (!cpu->negative)
            OpStepMem(memory, cpu, OpDp(cpu, EDGE_INCREMENT), 1);
    }
    OpLsrMem(memory, cpu, OpDp(cpu, EDGE_INCREMENT));
    if (cpu->carry) {
        if (!DrawSplitSlopedWindow(&window))
            return window.result;
    } else {
        OpLda(memory, cpu, OpDp(cpu, SLOPED_ANGLE));
        cpu->carry = 1u;
        OpSbcValue(cpu, 0x40u);
        if (!DrawSlopedWindowBand(&window, cpu->negative))
            return window.result;
    }
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return window.result;
}
