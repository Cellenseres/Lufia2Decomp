#include "lufia2/menu.h"
#include "menu/menu_action_internal.h"
#include "system/wram.h"

enum {
    DP_GLYPH_CELLS_LEFT = 2u,
    DP_GLYPH_ROWS_LEFT = 4u,
    DP_GLYPH_COLUMNS = 6u,
    DP_GLYPH_TEXT = 8u,
    DP_GLYPH_BUFFER_LENGTH = 11u,
    DP_GLYPH_TILE = 0u,
    DP_GLYPH_UPLOAD_SIZE = 0x58u,
    DP_GLYPH_UPLOAD_SOURCE = 0x5du,
    DP_GLYPH_UPLOAD_BANK = 0x5fu,
    DP_GLYPH_UPLOAD_DESTINATION = 0x60u,
    GLYPH_BUFFER_ADDRESS = 0xd000u,
    GLYPH_TILEMAP_ADDRESS = 0x3000u
};

static void SetGlyphDimensions(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpTxa(cpu);
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, DP_GLYPH_ROWS_LEFT));
    OpTxa(cpu);
    ExchangeAccumulatorBytes(cpu);
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, DP_GLYPH_COLUMNS));
}

static void AdvanceGlyphBuffer(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpStepMem(memory, cpu, OpDp(cpu, DP_GLYPH_BUFFER_LENGTH), 1);
    OpStepMem(memory, cpu, OpDp(cpu, DP_GLYPH_BUFFER_LENGTH), 1);
    OpStepMem(memory, cpu, OpDp(cpu, DP_GLYPH_BUFFER_LENGTH), 1);
}

static void AdvanceGlyphText(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_GLYPH_TEXT)));
    OpInx(cpu);
    OpWriteX(memory, cpu, OpDp(cpu, DP_GLYPH_TEXT), cpu->x);
}

static void ResetGlyphRow(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_GLYPH_COLUMNS)));
    OpWriteX(memory, cpu, OpDp(cpu, DP_GLYPH_CELLS_LEFT), cpu->x);
}

static void ReadGlyphText(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_GLYPH_TEXT));
}

static bool EmitBufferGlyph(MenuActionCall *call) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    OpCmpValue(cpu, 5u);
    if (cpu->carry) {
        OpCmpValue(cpu, 8u);
        if (!cpu->carry) {
            cpu->carry = 1u;
            OpSbcValue(cpu, 4u);
            OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_GLYPH_ATTRIBUTE));
            AdvanceGlyphText(memory, cpu);
            ReadGlyphText(memory, cpu);
        } else {
            OpCmpValue(cpu, 0x15u);
            if (cpu->zero) {
                OpLoadA(cpu, 2u);
                OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_GLYPH_ATTRIBUTE));
                OpLoadA(cpu, 0xecu);
            }
        }
    } else {
        OpCmpValue(cpu, 0x15u);
    }
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_GLYPH_CHARACTER));
    if (!MenuActionChild(call, 0x8290c9u, 0x80c7c2u, 3u, 1u))
        return false;
    OpRepWidths(cpu, 0x20u);
    AdvanceGlyphBuffer(memory, cpu);
    OpStepMem(memory, cpu, OpDp(cpu, DP_GLYPH_TEXT), 1);
    OpStepMem(memory, cpu, OpDp(cpu, DP_GLYPH_CELLS_LEFT), -1);
    OpSepWidths(cpu, 0x20u);
    return true;
}

Lufia2ExecutionResult Lufia2MenuBuildGlyphBuffer(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu,
                                                 Lufia2PushedChildCall child,
                                                 void *context) {
    if (!MenuActionReady(cpu, child, 1u) || !(cpu->x & 0xffu) || !(cpu->x >> 8))
        return ExecutionHandoff(cpu, 0x829073u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    if (!cpu->carry) {
        OpPushX(memory, cpu);
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09AD));
        if (!MenuActionChild(&call, 0x82907bu, 0x80c784u, 3u, 1u))
            return call.result;
        OpStz(memory, cpu, OpDp(cpu, DP_GLYPH_BUFFER_LENGTH));
        OpStz(memory, cpu, OpDp(cpu, DP_GLYPH_BUFFER_LENGTH + 1u));
        OpPullX(memory, cpu);
    }
    SetGlyphDimensions(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    ResetGlyphRow(memory, cpu);
    for (;;) {
        OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_GLYPH_ATTRIBUTE));
        ReadGlyphText(memory, cpu);
        if (cpu->zero) {
            do {
                if (!MenuActionChild(&call, 0x8290eau, 0x8290f6u, 2u, 1u))
                    return call.result;
                ResetGlyphRow(memory, cpu);
                OpStepMem(memory, cpu, OpDp(cpu, DP_GLYPH_ROWS_LEFT), -1);
            } while (!cpu->zero);
            return ExecutionReturned(0x8290f5u);
        }
        OpCmpValue(cpu, 0x0au);
        if (cpu->zero) {
            AdvanceGlyphText(memory, cpu);
            if (!MenuActionChild(&call, 0x8290e2u, 0x8290f6u, 2u, 1u))
                return call.result;
            OpStepMem(memory, cpu, OpDp(cpu, DP_GLYPH_ROWS_LEFT), -1);
            if (cpu->zero)
                return ExecutionReturned(0x8290e9u);
            ResetGlyphRow(memory, cpu);
        } else if (!EmitBufferGlyph(&call)) {
            return call.result;
        }
    }
}

Lufia2ExecutionResult Lufia2MenuPadGlyphRow(const Lufia2Memory *memory,
                                            Lufia2CpuState *cpu,
                                            Lufia2PushedChildCall child,
                                            void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x8290f6u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    for (;;) {
        OpLda(memory, cpu, OpDp(cpu, DP_GLYPH_CELLS_LEFT));
        if (cpu->zero)
            return ExecutionReturned(0x8290fau);
        OpLdx(cpu, 0x10u);
        OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_GLYPH_CHARACTER), cpu->x);
        if (!MenuActionChild(&call, 0x829101u, 0x80c7c2u, 3u, 1u))
            return call.result;
        OpRepWidths(cpu, 0x20u);
        AdvanceGlyphBuffer(memory, cpu);
        OpStepMem(memory, cpu, OpDp(cpu, DP_GLYPH_CELLS_LEFT), -1);
        OpSepWidths(cpu, 0x20u);
    }
}

Lufia2ExecutionResult Lufia2MenuUploadGlyphBuffer(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu,
                                                  Lufia2PushedChildCall child,
                                                  void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x829113u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpRepWidths(cpu, 0x20u);
    for (unsigned shift = 0u; shift < 4u; ++shift) {
        const uint32_t address = OpDp(cpu, DP_GLYPH_BUFFER_LENGTH);
        const uint16_t old = OpRead16(memory, address);
        const uint16_t length = (uint16_t)(old << 1);
        cpu->carry = (old & 0x8000u) != 0u;
        Write8(memory, OpNextByte(address), (uint8_t)(length >> 8));
        Write8(memory, address, (uint8_t)length);
        SetNz16(cpu, length);
    }
    OpSepWidths(cpu, 0x20u);
    OpLdy(cpu, GLYPH_BUFFER_ADDRESS);
    OpWriteX(memory, cpu, OpDp(cpu, DP_GLYPH_UPLOAD_SOURCE), cpu->y);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, DP_GLYPH_UPLOAD_BANK));
    OpWriteX(memory, cpu, OpDp(cpu, DP_GLYPH_UPLOAD_DESTINATION), cpu->x);
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_GLYPH_BUFFER_LENGTH)));
    OpWriteX(memory, cpu, OpDp(cpu, DP_GLYPH_UPLOAD_SIZE), cpu->y);
    if (!MenuActionChild(&call, 0x82912eu, 0x828044u, 3u, 1u))
        return call.result;
    return ExecutionReturned(0x829132u);
}

Lufia2ExecutionResult Lufia2MenuPlaceGlyphRectangle(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu,
                                                    Lufia2PushedChildCall child,
                                                    void *context) {
    (void)context;
    if (!MenuActionReady(cpu, child, 1u) || !(cpu->x & 0xffu) || !(cpu->x >> 8))
        return ExecutionHandoff(cpu, 0x829133u);
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7eu);
    SetGlyphDimensions(memory, cpu);
    do {
        PushY(memory, cpu);
        OpLda(memory, cpu, OpDp(cpu, DP_GLYPH_TILE));
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_GLYPH_COLUMNS)));
        do {
            OpSta(memory, cpu, OpAbsY(cpu, GLYPH_TILEMAP_ADDRESS));
            OpIncA(cpu);
            OpSta(memory, cpu, OpAbsY(cpu, GLYPH_TILEMAP_ADDRESS + 0x40u));
            OpIncA(cpu);
            OpIny(cpu);
            OpIny(cpu);
            OpDex(cpu);
        } while (!cpu->zero);
        OpSta(memory, cpu, OpDp(cpu, DP_GLYPH_TILE));
        PullAccumulator16(memory, cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x80u);
        OpTay(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, DP_GLYPH_ROWS_LEFT), -1);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x829168u);
}
