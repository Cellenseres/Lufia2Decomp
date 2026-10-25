#include "menu/card_table_services.h"
#include "lufia2/card_table.h"
#include "system/wram.h"

enum {
    ATTRIBUTE_WIDTH = 0x00u,
    ATTRIBUTE_ORIGIN = 0x02u,
    ATTRIBUTE_BITS = 0x04u,
    ATTRIBUTE_ROWS = 0x06u,
    PANEL_ORIGIN = 0x13u,
    PANEL_ATTRIBUTES = 0x15u,
    PANEL_ROWS = 0x17u,
    ROUND_PENDING = 0x1fu,
    FRAME_REQUESTS = 0x74u,
    ATTRIBUTE_TILEMAP = WRAM_MUSIC_SAMPLE_CACHE & 0xffffu,
    ATTRIBUTE_LINE_LAYOUT = (WRAM_CARD_GAME_HAND_WINS - 3u) & 0xffffu,
    ATTRIBUTE_LINE_ACTIVE = WRAM_CARD_TABLE_STATE_COUNTERS & 0xffffu,
    ATTRIBUTE_LINE_TIMER = WRAM_CARD_TABLE_LINE_ATTRIBUTE_TIMER & 0xffffu,
    ATTRIBUTE_SESSION = WRAM_CARD_TABLE_SESSION_MODE & 0xffffu,
    ATTRIBUTE_ACCENT = WRAM_BOARD_GAME_LINE_TOTALS & 0xffffu,
    ATTRIBUTE_ACCENT_TIMER = (WRAM_BOARD_GAME_LINE_TOTALS + 2u) & 0xffffu,
    ATTRIBUTE_PANEL_TIMER = (WRAM_BOARD_GAME_LINE_TOTALS + 6u) & 0xffffu,
    ATTRIBUTE_PANEL_STATE = (WRAM_BOARD_GAME_LINE_TOTALS + 8u) & 0xffffu,
    ATTRIBUTE_PANEL_FLAGS = (WRAM_BOARD_GAME_LINE_TOTALS + 10u) & 0xffffu,
    ROUND_MODE = WRAM_SPECIAL_MENU_WINDOW_STATE & 0xffffu,
    ROUND_FIRST_BUFFER = (WRAM_POKER_ROUND_STATUS - 1u) & 0xffffu,
    ROUND_SECOND_BUFFER = (WRAM_POKER_RANK_ORDER + 5u) & 0xffffu
};

static bool AttributeWordEntry(const Lufia2CpuState *cpu) {
    return TableServiceReady(cpu, false) && !cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit;
}

static void ClearRoundWords(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                            uint16_t base, uint16_t count) {
    OpLoadA(cpu, count);
    OpLdx(cpu, 0u);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, base));
        OpInx(cpu);
        OpInx(cpu);
        OpDecA(cpu);
    } while (!cpu->zero);
}

Lufia2ExecutionResult Lufia2CardTableResetRoundBuffers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !AttributeWordEntry(cpu))
        return ExecutionHandoff(cpu, 0x84ed9eu);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84edcau), false};
    OpStz(memory, cpu, OpAbs(cpu, ROUND_MODE));
    ClearRoundWords(memory, cpu, ROUND_SECOND_BUFFER, 6u);
    ClearRoundWords(memory, cpu, ROUND_FIRST_BUFFER, 4u);
    OpStz(memory, cpu, OpDp(cpu, ROUND_PENDING));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x88u);
    OpSta(memory, cpu, OpDp(cpu, FRAME_REQUESTS));
    if (!TableServiceCall(&service, 0x84edc5u, 0x84e9b1u, true))
        return service.result;
    OpRepWidths(cpu, 0x20u);
    return service.result;
}

static void ReadLineRectangle(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbsX(cpu, ATTRIBUTE_LINE_LAYOUT));
    OpSta(memory, cpu, OpDp(cpu, ATTRIBUTE_WIDTH));
    OpInx(cpu);
    OpInx(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, ATTRIBUTE_LINE_LAYOUT));
    OpSta(memory, cpu, OpDp(cpu, ATTRIBUTE_ORIGIN));
    OpInx(cpu);
    OpInx(cpu);
}

Lufia2ExecutionResult Lufia2CardTableRestoreLineAttributes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !AttributeWordEntry(cpu))
        return ExecutionHandoff(cpu, 0x84edcbu);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84edf9u), false};
    OpLda(memory, cpu, OpAbs(cpu, ATTRIBUTE_SESSION));
    OpLdy(cpu, cpu->zero ? 5u : 8u);
    OpLdx(cpu, 0u);
    OpLoadA(cpu, 0x1800u);
    OpSta(memory, cpu, OpDp(cpu, ATTRIBUTE_BITS));
    do {
        ReadLineRectangle(memory, cpu);
        OpLoadA(cpu, 3u);
        OpSta(memory, cpu, OpDp(cpu, ATTRIBUTE_ROWS));
        if (!TableServiceCall(&service, 0x84edf3u, 0x84f105u, false))
            return service.result;
        OpDey(cpu);
    } while (!cpu->zero);
    return service.result;
}

static void ChoosePanelAttributes(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 0x1c00u);
    OpLda(memory, cpu, OpAbs(cpu, ATTRIBUTE_PANEL_FLAGS));
    if (!cpu->zero) {
        OpLdx(cpu, 0x0400u);
        OpAndValue(cpu, 8u);
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, ATTRIBUTE_PANEL_STATE));
            OpCmpValue(cpu, 0x0400u);
            if (cpu->zero) OpLdx(cpu, 0x1c00u);
        }
    }
    OpWriteX(memory, cpu, OpDp(cpu, PANEL_ATTRIBUTES), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, ATTRIBUTE_PANEL_STATE), cpu->x);
}

static void PaintPanel(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLoadA(cpu, 6u);
    OpSta(memory, cpu, OpDp(cpu, PANEL_ROWS));
    OpLdx(cpu, 0xf4u);
    OpWriteX(memory, cpu, OpDp(cpu, PANEL_ORIGIN), cpu->x);
    for (unsigned byte = 0u; byte < 4u; ++byte) OpInx(cpu);
    OpLdy(cpu, 4u);
    bool first_row = true;
    do {
        if (!first_row) OpLdy(cpu, 6u);
        first_row = false;
        do {
            OpLda(memory, cpu, OpAbsX(cpu, ATTRIBUTE_TILEMAP));
            OpAndValue(cpu, 0x03ffu);
            OpOra(memory, cpu, OpDp(cpu, PANEL_ATTRIBUTES));
            OpSta(memory, cpu, OpAbsX(cpu, ATTRIBUTE_TILEMAP));
            OpInx(cpu);
            OpInx(cpu);
            OpDey(cpu);
        } while (!cpu->zero);
        OpLda(memory, cpu, OpDp(cpu, PANEL_ORIGIN));
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x40u);
        OpSta(memory, cpu, OpDp(cpu, PANEL_ORIGIN));
        OpTax(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, PANEL_ROWS), -1);
    } while (!cpu->zero);
}

Lufia2ExecutionResult Lufia2CardTableAnimatePanelAttributes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!TableServiceReady(cpu, true) || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x84f034u);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, ATTRIBUTE_SESSION));
    if (!cpu->zero) {
        OpStepMem(memory, cpu, OpAbs(cpu, ATTRIBUTE_PANEL_TIMER), -1);
        if (cpu->zero) {
            OpLoadA(cpu, 20u);
            OpSta(memory, cpu, OpAbs(cpu, ATTRIBUTE_PANEL_TIMER));
            ChoosePanelAttributes(memory, cpu);
            PaintPanel(memory, cpu);
        }
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x84f09bu);
}

Lufia2ExecutionResult Lufia2CardTableAnimateLineAttributes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !AttributeWordEntry(cpu))
        return ExecutionHandoff(cpu, 0x84f09cu);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84f0d3u), false};
    OpStepMem(memory, cpu, OpAbs(cpu, ATTRIBUTE_LINE_TIMER), -1);
    if (!cpu->zero) return service.result;
    OpLdx(cpu, 0u);
    OpLdy(cpu, 5u);
    do {
        PushY(memory, cpu);
        ReadLineRectangle(memory, cpu);
        OpTxa(cpu);
        cpu->carry = 0u;
        RorA16(cpu);
        OpDecA(cpu);
        OpDecA(cpu);
        OpTay(cpu);
        OpLda(memory, cpu, OpAbsY(cpu, ATTRIBUTE_LINE_ACTIVE));
        if (!cpu->zero) {
            OpLoadA(cpu, 3u);
            OpSta(memory, cpu, OpDp(cpu, ATTRIBUTE_ROWS));
            if (!TableServiceCall(&service, 0x84f0c6u, 0x84f105u, false))
                return service.result;
        }
        OpPullY(memory, cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    OpLoadA(cpu, 30u);
    OpSta(memory, cpu, OpAbs(cpu, ATTRIBUTE_LINE_TIMER));
    return service.result;
}

Lufia2ExecutionResult Lufia2CardTableAnimateAccentAttributes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !AttributeWordEntry(cpu))
        return ExecutionHandoff(cpu, 0x84f0d4u);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84f104u), false};
    OpStepMem(memory, cpu, OpAbs(cpu, ATTRIBUTE_ACCENT_TIMER), -1);
    if (!cpu->zero) return service.result;
    OpLdx(cpu, 0x0c00u);
    OpLda(memory, cpu, OpAbs(cpu, ATTRIBUTE_ACCENT));
    OpCmpValue(cpu, 0x1800u);
    if (!cpu->zero) OpLdx(cpu, 0x1800u);
    OpWriteX(memory, cpu, OpDp(cpu, ATTRIBUTE_BITS), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, ATTRIBUTE_ACCENT), cpu->x);
    OpLoadA(cpu, 6u);
    OpSta(memory, cpu, OpDp(cpu, ATTRIBUTE_WIDTH));
    OpLoadA(cpu, 0x0672u);
    OpSta(memory, cpu, OpDp(cpu, ATTRIBUTE_ORIGIN));
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpDp(cpu, ATTRIBUTE_ROWS));
    if (!TableServiceCall(&service, 0x84f0fbu, 0x84f105u, false))
        return service.result;
    OpLoadA(cpu, 30u);
    OpSta(memory, cpu, OpAbs(cpu, ATTRIBUTE_ACCENT_TIMER));
    return service.result;
}

Lufia2ExecutionResult Lufia2CardTablePaintTileAttributes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!TableServiceReady(cpu, true) || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x84f105u);
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, Read16Direct(memory, cpu, ATTRIBUTE_ORIGIN));
    do {
        OpLdy(cpu, Read16Direct(memory, cpu, ATTRIBUTE_WIDTH));
        do {
            OpLda(memory, cpu, OpAbsX(cpu, ATTRIBUTE_TILEMAP));
            OpAndValue(cpu, 0x03ffu);
            OpOra(memory, cpu, OpDp(cpu, ATTRIBUTE_BITS));
            OpSta(memory, cpu, OpAbsX(cpu, ATTRIBUTE_TILEMAP));
            OpInx(cpu);
            OpInx(cpu);
            OpDey(cpu);
        } while (!cpu->zero);
        OpLda(memory, cpu, OpDp(cpu, ATTRIBUTE_ORIGIN));
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x40u);
        OpSta(memory, cpu, OpDp(cpu, ATTRIBUTE_ORIGIN));
        OpTax(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, ATTRIBUTE_ROWS), -1);
    } while (!cpu->zero);
    UnpackStatus(cpu, Pull8(memory, cpu));
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    return ExecutionReturned(0x84f12eu);
}
