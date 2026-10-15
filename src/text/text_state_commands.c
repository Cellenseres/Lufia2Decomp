#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "system/wram.h"
#include "text/text_internal.h"

enum {
    SUBTRACT_VARIABLE = 0xa3ffu,
    SET_REGION_COORDINATES = 0xabf9u,
    SET_SOUND_SUPPRESSION = 0xac73u,
    SET_ACTOR_CONTROL = 0xadecu,
    SET_LAYER_SECTION = 0xb67du,
    START_FADE_OUT = 0xb9b2u,
    START_FADE_IN = 0xb9dau,
    START_MOSAIC_TRANSITION = 0xba00u,
    ADD_PALETTE_WORD = 0xb7a6u,
    SUBTRACT_PALETTE_WORD = 0xb7b4u,
    SET_PALETTE_WORD = 0xb7c2u,
    SET_GLYPH_OFFSET = 0xb7efu,
    NO_OPERATION = 0xb33eu,
    TEXT_VARIABLES = 0x079eu,
    TEXT_UNK_09E6 = 0x09e6u,
    TEXT_UNK_09E7 = 0x09e7u,
    TEXT_GLYPH_OFFSET = 0x125du,
    DP_PALETTE_WORD = 0x54u
};

static void SubtractVariable(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    TransferDirectToA(cpu);
    Lufia2TextNextByte(memory, cpu, 0xa402u);
    OpTax(cpu);
    Lufia2TextNextByte(memory, cpu, 0xa406u);
    cpu->carry = true;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, TEXT_VARIABLES)));
    OpLoadA(cpu, (uint16_t)(cpu->accumulator ^ 0xffu));
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbsX(cpu, TEXT_VARIABLES));
}

static void SetRegionCoordinates(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2TextNextByte(memory, cpu, 0xabfbu);
    OpCmpValue(cpu, 2u);
    if (!cpu->carry) {
        OpStz(memory, cpu, OpAbs(cpu, TEXT_UNK_09E7));
        Lufia2TextNextByte(memory, cpu, 0xac05u);
        OpSta(memory, cpu, OpAbs(cpu, TEXT_UNK_09E6));
    } else {
        OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_UNK_05C0));
        Lufia2TextNextByte(memory, cpu, 0xac11u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E05C1));
    }
}

static void SetSoundSuppression(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2TextNextByte(memory, cpu, 0xac75u);
    cpu->carry = true;
    OpSbcValue(cpu, 0xffu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_FLAGS));
    if (cpu->carry)
        OpAndValue(cpu, 0xfdu);
    else
        OpOraValue(cpu, 2u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_FLAGS));
}

static void SetActorControl(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2TextNextByte(memory, cpu, 0xadeeu);
    OpOraValue(cpu, 0u);
    const bool enabled = cpu->zero;
    OpLoadA(cpu, 1u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09A6), enabled);
}

static void SetLayerSection(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2TextNextByte(memory, cpu, 0xb67fu);
    ExchangeAccumulatorBytes(cpu);
    OpLoadA(cpu, 0u);
    ExchangeAccumulatorBytes(cpu);
    OpTax(cpu);
    Lufia2TextNextByte(memory, cpu, 0xb687u);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_LAYER_SECTION_WORD));
    Lufia2TextNextByte(memory, cpu, 0xb68eu);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_LAYER_SECTION_WORD + 1u));
}

static void StartBrightnessFade(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                uint16_t handler) {
    const bool darken = handler == START_FADE_OUT;
    OpLda(memory, cpu, OpAbsY(cpu, 0u));
    OpAndValue(cpu, 0x0fu);
    OpSta(memory, cpu, WRAM_FIELD_FADE_FRAMES_PER_STEP);
    Lufia2TextNextByte(memory, cpu, darken ? 0xb9bdu : 0xb9e5u);
    for (unsigned bit = 0; bit < 4u; ++bit)
        OpLsrA(cpu);
    OpSta(memory, cpu, WRAM_FIELD_FADE_BRIGHTNESS_STEP);
    OpLoadA(cpu, darken ? 15u : 0u);
    OpSta(memory, cpu, WRAM_FIELD_FADE_BRIGHTNESS);
    if (darken)
        OpLoadA(cpu, 0u);
    OpSta(memory, cpu, WRAM_FIELD_FADE_FRAME_COUNTER);
    OpLoadA(cpu, darken ? 1u : 2u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_SCREEN_EFFECTS), true);
}

static void StartMosaicTransition(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLoadA(cpu, 0xf8u);
    OpSta(memory, cpu, WRAM_FIELD_MOSAIC_STATE);
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, WRAM_FIELD_MOSAIC_ACCUMULATOR);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_TRANSITION_FLAGS));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FADE_LEVEL));
    OpLoadA(cpu, 0x88u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FADE_CONTROL));
}

static void ReadPaletteWord(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    TransferDirectToA(cpu);
    Lufia2TextNextByte(memory, cpu, 0xb7d8u);
    OpAslA(cpu);
    OpTax(cpu);
    Lufia2TextNextByte(memory, cpu, 0xb7ddu);
    OpSta(memory, cpu, OpDp(cpu, DP_PALETTE_WORD));
    Lufia2TextNextByte(memory, cpu, 0xb7e2u);
    OpSta(memory, cpu, OpDp(cpu, DP_PALETTE_WORD + 1u));
}

static void ChangePaletteWord(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                              uint16_t handler) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(handler + 2u));
    ReadPaletteWord(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    if (handler == SET_PALETTE_WORD) {
        OpLda(memory, cpu, OpDp(cpu, DP_PALETTE_WORD));
    } else {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_CGRAM_BUFFER));
        cpu->carry = handler == SUBTRACT_PALETTE_WORD;
        if (cpu->carry)
            OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_PALETTE_WORD)));
        else
            OpAdc(memory, cpu, OpDp(cpu, DP_PALETTE_WORD));
    }
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_CGRAM_BUFFER));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpDp(cpu, 0x73u));
}

static void SetGlyphOffset(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2TextNextByte(memory, cpu, 0xb7f1u);
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_PALETTE_WORD));
    Lufia2TextNextByte(memory, cpu, 0xb7f7u);
    SimulateJsrFrame(memory, cpu, 0xb7fau);
    OpStz(memory, cpu, OpDp(cpu, DP_PALETTE_WORD + 1u));
    OpSta(memory, cpu, OpAbs(cpu, SNES_M7A));
    OpStz(memory, cpu, OpAbs(cpu, SNES_M7A));
    OpLoadA(cpu, 0x40u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_M7B));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, SNES_MPYL));
    cpu->carry = false;
    OpAdc(memory, cpu, OpDp(cpu, DP_PALETTE_WORD));
    OpSta(memory, cpu, OpAbs(cpu, TEXT_GLYPH_OFFSET));
    OpSepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpAbs(cpu, TEXT_TYPING_SOUND));
    SimulateRtsFrame(memory, cpu);
}

bool Lufia2TextExecuteStateCommand(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                   uint16_t handler) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->direct_page)
        return false;
    switch (handler) {
    case SUBTRACT_VARIABLE:
        SubtractVariable(memory, cpu);
        break;
    case SET_REGION_COORDINATES:
        SetRegionCoordinates(memory, cpu);
        break;
    case SET_SOUND_SUPPRESSION:
        SetSoundSuppression(memory, cpu);
        break;
    case SET_ACTOR_CONTROL:
        SetActorControl(memory, cpu);
        break;
    case SET_LAYER_SECTION:
        SetLayerSection(memory, cpu);
        break;
    case START_FADE_OUT:
    case START_FADE_IN:
        StartBrightnessFade(memory, cpu, handler);
        break;
    case START_MOSAIC_TRANSITION:
        StartMosaicTransition(memory, cpu);
        break;
    case ADD_PALETTE_WORD:
    case SUBTRACT_PALETTE_WORD:
    case SET_PALETTE_WORD:
        ChangePaletteWord(memory, cpu, handler);
        break;
    case SET_GLYPH_OFFSET:
        SetGlyphOffset(memory, cpu);
        break;
    case NO_OPERATION:
        break;
    default:
        return false;
    }
    return true;
}
