#include "core/cpu_ops.h"
#include "core/child_call.h"
#include "core/snes_registers.h"
#include "lufia2/text.h"
#include "system/wram.h"

enum {
    ROM_DISPLAY_TEXT = 0x919affu,
    ROM_DISPLAY_GLYPHS = 0x8eb59bu,
    DISPLAY_TILEMAP = 0x2000u,
    DISPLAY_SECOND_ROW = 0x2040u,
    DISPLAY_TILE_STRIP = 0x2012u,
    DISPLAY_STRIP_SECOND_ROW = 0x2052u,
    DP_STRIP_COLUMNS = 0x54u,
    DP_CLEAR_COLUMNS = 0x56u,
    DP_TEXT_LENGTH = 0x58u,
    DP_TEXT_LENGTH_HIGH = 0x59u,
    DP_DISPLAY_ROW = 0x60u,
    DP_TEXT_STATE = 0x65u
};

static bool DisplayEntry(const Lufia2CpuState *cpu, uint8_t byte_tiles) {
    return cpu->program_bank == 0x8eu && !cpu->direct_page &&
           cpu->accumulator_is_8_bit == byte_tiles &&
           !cpu->index_is_8_bit && !cpu->decimal;
}

Lufia2ExecutionResult Lufia2AlternateDisplayMeasureText(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!DisplayEntry(cpu, 1u))
        return ExecutionHandoff(cpu, 0x8eb4fcu);
    TransferDirectToA(cpu);
    OpPushX(memory, cpu);
    OpStz(memory, cpu, OpDp(cpu, DP_TEXT_LENGTH));
    OpStz(memory, cpu, OpDp(cpu, DP_TEXT_LENGTH_HIGH));
    for (;;) {
        OpLda(memory, cpu, OpLongX(cpu, ROM_DISPLAY_TEXT));
        if (cpu->zero)
            break;
        OpInx(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, DP_TEXT_LENGTH), 1);
    }
    OpPullX(memory, cpu);
    return ExecutionReturned(0x8eb50eu);
}

Lufia2ExecutionResult Lufia2AlternateDisplayDrawText(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!DisplayEntry(cpu, 1u))
        return ExecutionHandoff(cpu, 0x8eb50fu);
    for (;;) {
        TransferDirectToA(cpu);
        OpLda(memory, cpu, OpLongX(cpu, ROM_DISPLAY_TEXT));
        if (cpu->zero)
            break;
        OpPushX(memory, cpu);
        cpu->carry = 1u;
        OpSbcValue(cpu, 0x20u);
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, ROM_DISPLAY_GLYPHS));
        OpSta(memory, cpu, OpAbsY(cpu, DISPLAY_TILEMAP));
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x10u);
        OpSta(memory, cpu, OpAbsY(cpu, DISPLAY_SECOND_ROW));
        OpPullX(memory, cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpInx(cpu);
    }
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpDp(cpu, DP_TEXT_STATE));
    OpInx(cpu);
    return ExecutionReturned(0x8eb533u);
}

Lufia2ExecutionResult Lufia2AlternateDisplayDrawTileStrip(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!DisplayEntry(cpu, 0u))
        return ExecutionHandoff(cpu, 0x8eb534u);
    Push8(memory, cpu, (uint8_t)(cpu->accumulator >> 8));
    Push8(memory, cpu, (uint8_t)cpu->accumulator);
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpDp(cpu, DP_STRIP_COLUMNS));
    PullAccumulator16(memory, cpu);
    do {
        OpSta(memory, cpu, OpAbsY(cpu, DISPLAY_TILE_STRIP));
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x10u);
        OpSta(memory, cpu, OpAbsY(cpu, DISPLAY_STRIP_SECOND_ROW));
        cpu->carry = 1u;
        OpSbcValue(cpu, 0x0eu);
        OpIny(cpu);
        OpIny(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, DP_STRIP_COLUMNS), -1);
    } while (!cpu->zero);
    return ExecutionReturned(0x8eb54fu);
}

Lufia2ExecutionResult Lufia2AlternateDisplayUploadTilemap(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!DisplayEntry(cpu, 1u))
        return ExecutionHandoff(cpu, 0x8eb557u);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_VMADDL), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_DASL(0)), cpu->y);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_DMAP(0)));
    OpLdx(cpu, 0x6000u);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_A1TL(0)), cpu->x);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_A1B(0)));
    OpLoadA(cpu, 0x18u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_BBAD(0)));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_MDMAEN));
    return ExecutionReturned(0x8eb577u);
}

Lufia2ExecutionResult Lufia2AlternateDisplayAdvanceRow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!DisplayEntry(cpu, 1u))
        return ExecutionHandoff(cpu, 0x8eb578u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DP_DISPLAY_ROW));
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x80u);
    OpAndValue(cpu, 0x0fffu);
    OpSta(memory, cpu, OpDp(cpu, DP_DISPLAY_ROW));
    OpTay(cpu);
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpDp(cpu, DP_CLEAR_COLUMNS));
    TransferDirectToA(cpu);
    do {
        OpSta(memory, cpu, OpAbsY(cpu, DISPLAY_TILEMAP));
        OpSta(memory, cpu, OpAbsY(cpu, DISPLAY_SECOND_ROW));
        OpIny(cpu);
        OpIny(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, DP_CLEAR_COLUMNS), -1);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x8eb59au);
}

enum {
    DISPLAY_POLL_SLICE = 64u,
    DISPLAY_SRAM_FLAGS = 0x700000u,
    DISPLAY_RESOURCE = 0x40u,
    DISPLAY_RASTER_STATUS = 0x00213fu,
    DP_DISPLAY_SCROLL = 0x5du,
    DP_DISPLAY_SPARE_SCROLL = 0x5au,
    DP_DISPLAY_BANK = 0x62u,
    DP_DISPLAY_SCROLL_STEPS = 0x55u,
    DP_DISPLAY_NMI_MODE = 0x72u
};

typedef struct AlternateDisplay {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
} AlternateDisplay;

static bool DisplayCall(AlternateDisplay *display, uint32_t site,
                         uint32_t target, uint8_t frame) {
    if (CallChildWithFrame(display->memory, display->cpu, display->child,
                          display->context, site, target, frame, 0x8eu))
        return true;
    display->result = ExecutionReturned(site);
    display->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return false;
}

static bool DisplayWaitBoundary(AlternateDisplay *display, uint32_t pc) {
    display->result = ExecutionHandoff(display->cpu, pc);
    return false;
}

static void UpdateDisplaySaveFlags(const Lufia2Memory *memory,
                                    Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E0B51));
    OpBitValue(cpu, 2u);
    if (cpu->zero) {
        OpLsrA(cpu);
        OpLda(memory, cpu, DISPLAY_SRAM_FLAGS);
        if (cpu->carry) {
            OpAndValue(cpu, 0xfbu);
            OpOraValue(cpu, 8u);
        } else {
            OpAndValue(cpu, 0xf7u);
            OpOraValue(cpu, 4u);
        }
        OpSta(memory, cpu, DISPLAY_SRAM_FLAGS);
    }
}

static void ExpandDisplayTiles(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0x4000u);
    OpLdy(cpu, 0x6000u);
    do {
        OpLoadA(cpu, 0x0fu);
        OpMoveNext(memory, cpu, 0x7eu, 0x7eu);
        OpLoadA(cpu, 0x10u);
        OpSta(memory, cpu, OpDp(cpu, DP_STRIP_COLUMNS));
        TransferDirectToA(cpu);
        do {
            OpSta(memory, cpu, OpAbsY(cpu, 0u));
            OpIny(cpu);
            OpStepMem(memory, cpu, OpDp(cpu, DP_STRIP_COLUMNS), -1);
        } while (!cpu->zero);
        OpCpx(cpu, 0x5800u);
    } while (!cpu->carry);
    OpLdx(cpu, 0x07feu);
    OpLoadA(cpu, 0u);
    do {
        OpSta(memory, cpu, OpAbsX(cpu, DISPLAY_TILEMAP));
        OpSta(memory, cpu, OpAbsX(cpu, DISPLAY_TILEMAP + 0x800u));
        OpSta(memory, cpu, OpAbsX(cpu, DISPLAY_TILEMAP + 0x1000u));
        OpSta(memory, cpu, OpAbsX(cpu, DISPLAY_TILEMAP + 0x1800u));
        OpDex(cpu);
        OpDex(cpu);
    } while (!cpu->negative);
    PullDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
}

static bool LoadAlternateDisplay(AlternateDisplay *display) {
    const Lufia2Memory *memory = display->memory;
    Lufia2CpuState *cpu = display->cpu;
    OpLdx(cpu, 0x4000u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_DISPLAY_ROW), cpu->x);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, DP_DISPLAY_BANK));
    OpLdx(cpu, DISPLAY_RESOURCE);
    OpWriteX(memory, cpu, OpDp(cpu, DP_STRIP_COLUMNS), cpu->x);
    if (!DisplayCall(display, 0x8eb326u, 0x808e9du, 3u))
        return false;
    ExpandDisplayTiles(memory, cpu);
    OpLdx(cpu, 0x4000u);
    OpLdy(cpu, 0x3000u);
    return DisplayCall(display, 0x8eb36bu, 0x8eb557u, 2u);
}

static bool ConfigureAlternateDisplay(AlternateDisplay *display) {
    const Lufia2Memory *memory = display->memory;
    Lufia2CpuState *cpu = display->cpu;
    OpLdx(cpu, 6u);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, SNES_BG1HOFS));
        OpStz(memory, cpu, OpAbsX(cpu, SNES_BG1HOFS));
        OpStz(memory, cpu, OpAbsX(cpu, SNES_BG1VOFS));
        OpStz(memory, cpu, OpAbsX(cpu, SNES_BG1VOFS));
        for (unsigned offset = 0u; offset < 4u; ++offset)
            OpStz(memory, cpu, OpAbsX(cpu,
                (uint16_t)(WRAM_NMI_SCROLL_REGISTERS + offset)));
        OpDex(cpu);
    } while (!cpu->negative);
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_BG1SC));
    OpSta(memory, cpu, OpAbs(cpu, SNES_BG2SC));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_TM));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_TS));
    OpStz(memory, cpu, OpAbs(cpu, SNES_CGWSEL));
    OpLoadA(cpu, 0xe0u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_COLDATA));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_CGADSUB));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, 0x2133u));
    OpStz(memory, cpu, OpAbs(cpu, SNES_W12SEL));
    OpStz(memory, cpu, OpAbs(cpu, SNES_W34SEL));
    OpStz(memory, cpu, OpAbs(cpu, SNES_WOBJSEL));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WH0));
    OpStz(memory, cpu, OpAbs(cpu, SNES_WH1));
    if (!DisplayCall(display, 0x8eb3c1u, 0x83ac7au, 3u))
        return false;
    OpRepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_CGRAM_BUFFER));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_CGRAM_BUFFER));
    OpLoadA(cpu, 0x7bdeu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_CGRAM_BUFFER + 2u));
    OpLoadA(cpu, 0x6318u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_CGRAM_BUFFER + 4u));
    OpLoadA(cpu, 0x318cu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_CGRAM_BUFFER + 6u));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, DP_DISPLAY_NMI_MODE));
    OpLoadA(cpu, 0xa0u);
    OpSta(memory, cpu, OpDp(cpu, DP_MENU_FRAME_FLAGS));
    OpLoadA(cpu, 6u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_BGMODE));
    if (!DisplayCall(display, 0x8eb3eeu, 0x808285u, 3u))
        return false;
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, DP_NMI_UPLOAD_FLAGS));
    OpLoadA(cpu, 0x0fu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
    OpLoadA(cpu, 0x81u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_NMITIMEN));
    OpSetDataBank(memory, cpu, 0x7eu);
    OpLoadA(cpu, 0x50u);
    OpSta(memory, cpu, OpDp(cpu, DP_MENU_FRAME_FLAGS));
    OpStz(memory, cpu, OpDp(cpu, DP_TEXT_STATE));
    for (unsigned polls = 0u; polls < DISPLAY_POLL_SLICE; ++polls) {
        OpLda(memory, cpu, DISPLAY_RASTER_STATUS);
        if (!cpu->negative)
            return true;
    }
    return DisplayWaitBoundary(display, 0x8eb40au);
}

static bool ScrollAlternateDisplayRow(AlternateDisplay *display) {
    const Lufia2Memory *memory = display->memory;
    Lufia2CpuState *cpu = display->cpu;
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpDp(cpu, DP_DISPLAY_SCROLL_STEPS));
    do {
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpDp(cpu, DP_DISPLAY_SCROLL));
        OpSta(memory, cpu, OpAbs(cpu, WRAM_NMI_SCROLL_REGISTERS + 2u));
        OpIncA(cpu);
        OpIncA(cpu);
        OpSta(memory, cpu, OpDp(cpu, DP_DISPLAY_SCROLL));
        OpSepWidths(cpu, 0x20u);
        if (!DisplayCall(display, 0x8eb431u, 0x8eb550u, 2u) ||
            !DisplayCall(display, 0x8eb434u, 0x8eb550u, 2u) ||
            !DisplayCall(display, 0x8eb437u, 0x8eb550u, 2u) ||
            !DisplayCall(display, 0x8eb43au, 0x8eb550u, 2u))
            return false;
        OpStepMem(memory, cpu, OpDp(cpu, DP_DISPLAY_SCROLL_STEPS), -1);
    } while (!cpu->zero);
    if (!DisplayCall(display, 0x8eb441u, 0x8eb578u, 2u))
        return false;
    OpStepMem(memory, cpu, OpDp(cpu, DP_TEXT_STATE), -1);
    return true;
}

static void DoubleDisplayTextLength(const Lufia2Memory *memory,
                                     Lufia2CpuState *cpu) {
    const uint16_t length = Read16Direct(memory, cpu, DP_TEXT_LENGTH);
    const uint16_t doubled = (uint16_t)(length << 1);
    cpu->carry = (length & 0x8000u) != 0;
    Write8(memory, OpDp(cpu, DP_TEXT_LENGTH_HIGH), (uint8_t)(doubled >> 8));
    Write8(memory, OpDp(cpu, DP_TEXT_LENGTH), (uint8_t)doubled);
    SetNz16(cpu, doubled);
}

static bool DrawAlternateDisplayCommand(AlternateDisplay *display) {
    const Lufia2Memory *memory = display->memory;
    Lufia2CpuState *cpu = display->cpu;
    OpCmpValue(cpu, 5u);
    if (cpu->zero) {
        OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_DISPLAY_ROW)));
        for (unsigned column = 0u; column < 4u; ++column)
            OpIny(cpu);
        return DisplayCall(display, 0x8eb45eu, 0x8eb50fu, 2u);
    }
    OpCmpValue(cpu, 1u);
    if (cpu->zero) {
        if (!DisplayCall(display, 0x8eb468u, 0x8eb4fcu, 2u))
            return false;
        OpRepWidths(cpu, 0x20u);
        DoubleDisplayTextLength(memory, cpu);
        OpLoadA(cpu, 0x3cu);
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_TEXT_LENGTH)));
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpDp(cpu, DP_DISPLAY_ROW));
        OpTay(cpu);
        OpSepWidths(cpu, 0x20u);
        return DisplayCall(display, 0x8eb47bu, 0x8eb50fu, 2u);
    }
    OpCmpValue(cpu, 4u);
    if (cpu->zero) {
        OpLoadA(cpu, 0x80u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
        return true;
    }
    OpCmpValue(cpu, 3u);
    if (cpu->zero) {
        OpRepWidths(cpu, 0x20u);
        OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_DISPLAY_ROW)));
        OpLda(memory, cpu, OpLongX(cpu, ROM_DISPLAY_TEXT));
        if (!DisplayCall(display, 0x8eb499u, 0x8eb534u, 2u))
            return false;
        OpLda(memory, cpu, OpLongX(cpu, ROM_DISPLAY_TEXT));
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x40u);
        if (!DisplayCall(display, 0x8eb4a4u, 0x8eb534u, 2u))
            return false;
        OpSepWidths(cpu, 0x20u);
        OpInx(cpu);
        OpInx(cpu);
        OpLoadA(cpu, 1u);
    } else {
        OpLda(memory, cpu, OpLongX(cpu, ROM_DISPLAY_TEXT));
        OpSta(memory, cpu, OpDp(cpu, DP_TEXT_STATE));
        OpInx(cpu);
        return true;
    }
    OpSta(memory, cpu, OpDp(cpu, DP_TEXT_STATE));
    return true;
}

static bool RunAlternateDisplayScript(AlternateDisplay *display) {
    const Lufia2Memory *memory = display->memory;
    Lufia2CpuState *cpu = display->cpu;
    OpLdx(cpu, 0x0f00u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_DISPLAY_ROW), cpu->x);
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_DISPLAY_SCROLL), cpu->x);
    OpWriteX(memory, cpu, OpDp(cpu, DP_DISPLAY_SPARE_SCROLL), cpu->x);
    for (;;) {
        OpLda(memory, cpu, OpDp(cpu, DP_TEXT_STATE));
        if (!cpu->zero) {
            if (!ScrollAlternateDisplayRow(display))
                return false;
            continue;
        }
        OpLda(memory, cpu, OpLongX(cpu, ROM_DISPLAY_TEXT));
        OpInx(cpu);
        OpCmpValue(cpu, 0u);
        if (cpu->zero)
            return true;
        if (!DrawAlternateDisplayCommand(display))
            return false;
    }
}

static void ReadDisplayFade(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, DP_NMI_FRAME_COUNTER));
    for (unsigned shift = 0u; shift < 4u; ++shift)
        OpLsrA(cpu);
}

static bool FinishAlternateDisplay(AlternateDisplay *display) {
    const Lufia2Memory *memory = display->memory;
    Lufia2CpuState *cpu = display->cpu;
    OpStz(memory, cpu, OpDp(cpu, DP_NMI_FRAME_COUNTER));
    unsigned polls = 0u;
    do {
        ReadDisplayFade(memory, cpu);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
        OpCmpValue(cpu, 0x0fu);
        if (cpu->zero)
            break;
        if (++polls == DISPLAY_POLL_SLICE)
            return DisplayWaitBoundary(display, 0x8eb4beu);
    } while (true);
    OpLoadA(cpu, 0x3cu);
    OpSta(memory, cpu, OpDp(cpu, DP_STRIP_COLUMNS));
    do {
        OpStz(memory, cpu, OpDp(cpu, DP_NMI_FRAME_COUNTER));
        polls = 0u;
        do {
            OpLda(memory, cpu, OpDp(cpu, DP_NMI_FRAME_COUNTER));
            OpCmpValue(cpu, 0x3cu);
            if (cpu->carry)
                break;
            if (++polls == DISPLAY_POLL_SLICE)
                return DisplayWaitBoundary(display, 0x8eb4d1u);
        } while (true);
        OpStepMem(memory, cpu, OpDp(cpu, DP_STRIP_COLUMNS), -1);
    } while (!cpu->zero);
    OpLoadA(cpu, 0x0fu);
    OpSta(memory, cpu, OpDp(cpu, DP_STRIP_COLUMNS));
    OpStz(memory, cpu, OpDp(cpu, DP_NMI_FRAME_COUNTER));
    polls = 0u;
    do {
        ReadDisplayFade(memory, cpu);
        cpu->carry = 1u;
        OpSbcValue(cpu, 0x0fu);
        OpLoadA(cpu, OpA(cpu) ^ 0xffu);
        OpIncA(cpu);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
        if (cpu->zero)
            break;
        if (++polls == DISPLAY_POLL_SLICE)
            return DisplayWaitBoundary(display, 0x8eb4e1u);
    } while (true);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
    return DisplayCall(display, 0x8eb4f7u, 0x8eb550u, 2u);
}

Lufia2ExecutionResult Lufia2RunAlternateDisplay(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!DisplayEntry(cpu, 1u) || !child)
        return ExecutionHandoff(cpu, 0x8eb2eeu);
    AlternateDisplay display = {memory, cpu, child, context, {0}};
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
    if (!DisplayCall(&display, 0x8eb2f6u, 0x8eb550u, 2u))
        return display.result;
    OpStz(memory, cpu, OpAbs(cpu, SNES_NMITIMEN));
    UpdateDisplaySaveFlags(memory, cpu);
    if (!LoadAlternateDisplay(&display) || !ConfigureAlternateDisplay(&display) ||
        !RunAlternateDisplayScript(&display) || !FinishAlternateDisplay(&display))
        return display.result;
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x8eb4fbu);
}
