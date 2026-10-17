/* Field colour and screen effects. */

#include <stddef.h>

#include "core/cpu_ops.h"
#include "core/child_call.h"
#include "field/field_internal.h"
#include "lufia2/field.h"
#include "lufia2/system.h"
#include "system/dp_scratch.h"
#include "system/system_internal.h"
#include "system/wram.h"

/* Brightness fade and color math step state. */
enum {
    FADE_FRAMES_PER_STEP = WRAM_FIELD_FADE_FRAMES_PER_STEP,
    FADE_BRIGHTNESS_STEP = WRAM_FIELD_FADE_BRIGHTNESS_STEP,
    FADE_BRIGHTNESS = WRAM_FIELD_FADE_BRIGHTNESS,
    FADE_FRAME_COUNTER = WRAM_FIELD_FADE_FRAME_COUNTER,
    COLOR_STEP_TIMER = WRAM_FIELD_FIXED_COLOR_STEP_TIMER,
    COLOR_MATH_INTENSITY = 0x1271u,
    COLOR_FADE_SOURCE = WRAM_SCREEN_COLOR_FADE_SOURCE,
    COLOR_FADE_TARGET = WRAM_SCREEN_COLOR_FADE_TARGET,
    COLOR_FADE_PROGRESS = WRAM_SCREEN_FADE_PROGRESS,
    COLOR_FADE_SPEED = WRAM_SCREEN_FADE_STEP
};

/* $83:AF05-$83:AF4C: advance palette cycle X; 0 = cap hit. */
static uint8_t FieldPaletteAdvance(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                   uint32_t *visits) {
    SetAccumulatorWidth(cpu, 0); /* AF05 */
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xec00u, cpu->x));
    IncrementA16(cpu);
    IncrementA16(cpu);
    IncrementA16(cpu);
    Write16Long(memory, AbsoluteIndexedAddress(cpu, 0xec00u, cpu->x), cpu->accumulator);
    cpu->y = cpu->x; /* TXY */
    SetNz16(cpu, cpu->y);
    for (;;) {
        if (*visits >= 0x10000u) {
            /* Zero-length frames can chain forever. */
            cpu->resume_pc = 0x83af11u;
            return 0;
        }
        ++*visits;
        TransferAToX(cpu); /* AF11 */
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0xa10000u, cpu->x)));
        StoreAAbsolute8(memory, cpu, 0xed01u, cpu->y);
        SetAccumulatorWidth(cpu, 0);
        if (!cpu->zero)
            break;
        LoadA16(cpu, cpu->y); /* AF1F */
        cpu->carry = 0;
        Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0xd0f5u, 0));
        TransferAToX(cpu);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0xa10003u, cpu->x)));
        cpu->carry = 1;
        Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0xd0f5u, 0));
        Write16Long(memory, AbsoluteIndexedAddress(cpu, 0xec00u, cpu->y),
                    cpu->accumulator);
    }
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0xa10001u, cpu->x)));
    Write16Direct(memory, cpu, DP_SCRATCH_A, cpu->accumulator); /* AF36 */
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xed00u, cpu->y));
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Direct(memory, cpu, DP_SCRATCH_A));
    Write16Long(memory, LongIndexedAddress(WRAM_CGRAM_BUFFER, cpu->x),
                cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    cpu->x = cpu->y; /* TYX */
    SetNz16(cpu, cpu->x);
    LoadA8(cpu, 0x01u);
    {
        const uint32_t flags = DirectAddress(cpu, DP_NMI_UPLOAD_FLAGS); /* TSB $73 */
        const uint8_t value = Read8(memory, flags);

        cpu->zero = (value & 0x01u) == 0;
        Write8(memory, flags, (uint8_t)(value | 0x01u));
    }
    return 1;
}

/* $83:AEED: palette cycles from bank $A1; 0 = cap hit. */
static uint8_t FieldPaletteCyclesBody(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                      uint32_t *visits) {

    LoadA8(cpu, Read8(memory, 0x0009a9u)); /* AEED */
    BitImmediate8(cpu, 0x06u);
    if (!cpu->zero) {
        return 1;
    }
    LoadA8(cpu, Read8(memory, WRAM_FIELD_SCENE_RECORD_COUNT));
    if (cpu->zero) {
        return 1;
    }
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_E), A8(cpu));
    LoadX16(cpu, 0x0000u);
    do {
        const uint32_t timer = AbsoluteIndexedAddress(cpu, 0xed01u, cpu->x);
        const uint8_t left = (uint8_t)(Read8(memory, timer) - 1u);

        Write8(memory, timer, left);                           /* AF00 */
        SetNz8(cpu, left);
        if (cpu->zero && !FieldPaletteAdvance(memory, cpu, visits))
            return 0;
        IncrementX16(cpu);                                     /* AF4D */
        IncrementX16(cpu);
        DecrementDirect8(memory, cpu, DP_SCRATCH_E);
    } while (!cpu->zero);
    return 1;
}

static uint8_t FieldPaletteCycles(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                  uint32_t *visits) {
    SimulateJsrFrame(memory, cpu, 0xaee6u);
    if (!FieldPaletteCyclesBody(memory, cpu, visits))
        return 0u;
    SimulateRtsFrame(memory, cpu);
    return 1u;
}

/* Body of $83:AF54 without its JSR frame. */
static void FieldWaveTableBody(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadAAbsolute8(memory, cpu, 0xd0cau, 0);                   /* AF54 */
    Compare8(cpu, A8(cpu), 0xffu);
    if (cpu->zero)
        return;
    ExchangeAccumulatorBytes(cpu);
    LoadAAbsolute8(memory, cpu, 0xd0c9u, 0);
    TransferAToX(cpu);
    LoadAAbsolute8(memory, cpu, 0xd0c8u, 0);
    LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
    Write8(memory, WRAM_UNK_7FD0C8, A8(cpu));
    Compare8(cpu, A8(cpu), Read8(memory, LongIndexedAddress(0x7e0001u, cpu->x)));
    if (!cpu->zero)
        return;
    TransferDirectToA(cpu);                                    /* AF6E */
    StoreAAbsolute8(memory, cpu, 0xd0c8u, 0);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7e0000u, cpu->x)));
    AslA8(cpu);
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_B), A8(cpu));
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_A), 0x00u);
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7e0000u, cpu->x)));
    SetAccumulatorWidth(cpu, 0);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffffu));
    IncrementA16(cpu);
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, DP_SCRATCH_A));
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, WRAM_FIELD_AUXILIARY_RESOURCE_OFFSET));
    Write16Long(memory, SNES_A1TL(1), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    IncrementX16(cpu);                                         /* AF99 */
    IncrementX16(cpu);
    Write16Long(memory, AbsoluteIndexedAddress(cpu, 0xd0c9u, 0), cpu->x);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7e0000u, cpu->x)));
    Compare8(cpu, A8(cpu), 0xffu);
    if (cpu->zero) {
        LoadX16(cpu, 0xc002u);                                 /* AFA6 */
        Write16Long(memory, AbsoluteIndexedAddress(cpu, 0xd0c9u, 0), cpu->x);
    }
    Push8(memory, cpu, 0x83u);                                 /* AFAC */
    PullDataBank(memory, cpu);
    LoadA8(cpu, 0x01u);
    StoreAAbsolute8(memory, cpu, SNES_DMAP(1), 0);
    LoadA8(cpu, 0x7eu);
    StoreAAbsolute8(memory, cpu, SNES_A1B(1), 0);
    LoadX16(cpu, 0x01e0u);
    Write16Long(memory, AbsoluteIndexedAddress(cpu, SNES_DASL(1), 0), cpu->x);
    LoadA8(cpu, 0x18u);
    StoreAAbsolute8(memory, cpu, SNES_BBAD(1), 0);
    LoadX16(cpu, 0x4010u);
    StoreXDirect16(memory, cpu, 0x7bu);
    LoadA8(cpu, 0x42u);
    Write8(memory, DirectAddress(cpu, 0x76u), A8(cpu));
}

/* $83:AF54: step the HDMA wave table; set up channel 1. */
static void FieldWaveTable(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0xaee9u);
    FieldWaveTableBody(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

static Lufia2ExecutionResult ColourBoundary(
    Lufia2CpuState *cpu, uint32_t site) {
    return ExecutionHandoff(cpu, site);
}

static Lufia2ExecutionResult ColourChildUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static Lufia2ExecutionResult FieldColourEffectsBody(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    Lufia2ExecutionResult result = ExecutionReturned(0x83aeecu);

    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    OpLda(memory, cpu, 0x0009a9u);
    OpBitValue(cpu, 0x02u);
    if (!cpu->zero) {
        OpStepMem(memory, cpu, OpAbs(cpu, WRAM_FIELD_RECOVERY_PALETTE_DELAY), -1);
        if (cpu->zero) {
            if (!child)
                return ColourBoundary(cpu, 0x83aec7u);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x83aec7u, 0x848e07u, 3u, 0x83u))
                return ColourChildUnwound(0x83aec7u);
            OpLda(memory, cpu, 0x0009a9u);
        }
    }
    OpBitValue(cpu, 0x04u);
    if (!cpu->zero) {
        if (!child)
            return ColourBoundary(cpu, 0x83aed3u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x83aed3u, 0x848d54u, 3u, 0x83u))
            return ColourChildUnwound(0x83aed3u);
        OpLda(memory, cpu, 0x0009a9u);
    }
    OpBitValue(cpu, 0x01u);
    if (!cpu->zero) {
        PushDataBank(memory, cpu);
        OpLoadA(cpu, 0x7fu);
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        if (!FieldPaletteCycles(memory, cpu, &result.dispatches)) {
            result.flow = LUFIA2_EXECUTION_BOUNDARY;
            result.pc = cpu->resume_pc;
            return result;
        }
        FieldWaveTable(memory, cpu);
        PullDataBank(memory, cpu);
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
    return result;
}

Lufia2ExecutionResult Lufia2FieldColourEffects(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return FieldColourEffectsBody(memory, cpu, NULL, NULL);
}

Lufia2ExecutionResult Lufia2FieldColourEffectsWithServices(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->decimal)
        return ExecutionHandoff(cpu, 0x83aeb5u);
    return FieldColourEffectsBody(memory, cpu, child, context);
}

/* $84:8145: random shake offsets into $7F:D081/D083. */
static uint8_t ScreenEffectContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x84u && cpu->accumulator_is_8_bit &&
        !cpu->decimal;
}

static uint8_t AdvanceEffectChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target, uint8_t frame, uint8_t accumulator_width,
    uint8_t index_width,
    Lufia2ExecutionResult *result) {
    if (!CallChildWithFrame(memory, cpu, child, context,
            site, target, frame, 0x84u)) {
        *result = (Lufia2ExecutionResult){
            LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return 0u;
    }
    if (cpu->accumulator_is_8_bit != accumulator_width ||
        cpu->index_is_8_bit != index_width || cpu->decimal) {
        *result = ExecutionHandoff(cpu, site + frame + 1u);
        return 0u;
    }
    return 1u;
}

static Lufia2ExecutionResult AdvanceFixedColor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint8_t brighten) {
    if (!ScreenEffectContext(cpu))
        return ExecutionHandoff(cpu, brighten ? 0x8480d3u : 0x8480c0u);
    OpLda(memory, cpu, OpAbs(cpu, COLOR_MATH_INTENSITY));
    if (brighten)
        OpIncA(cpu);
    else
        OpDecA(cpu);
    OpOraValue(cpu, 0xe0u);
    OpSta(memory, cpu, OpAbs(cpu, COLOR_MATH_INTENSITY));
    OpCmpValue(cpu, brighten ? 0xffu : 0xe0u);
    if (cpu->zero) {
        OpLoadA(cpu, brighten ? 0x20u : 0x10u);
        OpTestBits(memory, cpu, OpAbs(cpu, WRAM_SCREEN_EFFECTS), 0u);
    }
    return ExecutionReturned(brighten ? 0x8480e5u : 0x8480d2u);
}

Lufia2ExecutionResult Lufia2FieldLowerFixedColor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    return AdvanceFixedColor(memory, cpu, 0u);
}

Lufia2ExecutionResult Lufia2FieldRaiseFixedColor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    return AdvanceFixedColor(memory, cpu, 1u);
}

static Lufia2ExecutionResult AdvanceBrightness(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint8_t brighten) {
    if (!ScreenEffectContext(cpu))
        return ExecutionHandoff(cpu, brighten ? 0x8480e6u : 0x848115u);
    OpLda(memory, cpu, FADE_FRAME_COUNTER);
    OpIncA(cpu);
    OpSta(memory, cpu, FADE_FRAME_COUNTER);
    OpCmp(memory, cpu, FADE_FRAMES_PER_STEP);
    if (cpu->carry) {
        OpLoadA(cpu, 0u);
        OpSta(memory, cpu, FADE_FRAME_COUNTER);
        OpLda(memory, cpu, FADE_BRIGHTNESS);
        cpu->carry = brighten ? 0u : 1u;
        if (brighten)
            OpAdc(memory, cpu, FADE_BRIGHTNESS_STEP);
        else
            OpSbcValue(cpu, OpReadM(memory, cpu, FADE_BRIGHTNESS_STEP));
        OpSta(memory, cpu, FADE_BRIGHTNESS);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
        if (brighten) {
            OpCmpValue(cpu, 0x0fu);
            if (cpu->carry) {
                OpLoadA(cpu, 2u);
                OpTestBits(memory, cpu, OpAbs(cpu, WRAM_SCREEN_EFFECTS), 0u);
            }
        } else if (cpu->negative) {
            OpLoadA(cpu, BRIGHTNESS_FORCED_BLANK);
            OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
            OpStz(memory, cpu, OpAbs(cpu, WRAM_SCREEN_EFFECTS));
        }
    }
    return ExecutionReturned(brighten ? 0x848114u : 0x848144u);
}

Lufia2ExecutionResult Lufia2FieldRaiseBrightness(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    return AdvanceBrightness(memory, cpu, 1u);
}

Lufia2ExecutionResult Lufia2FieldLowerBrightness(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    return AdvanceBrightness(memory, cpu, 0u);
}

Lufia2ExecutionResult Lufia2FieldUpdateScreenShake(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ScreenEffectContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x848145u);
    Lufia2ExecutionResult result;
    const uint8_t index_width = cpu->index_is_8_bit;
    OpLoadA(cpu, 0xffu);
    if (!AdvanceEffectChild(memory, cpu, child, context,
            0x848147u, 0x808299u, 3u, 1u, index_width, &result))
        return result;
    OpCmp(memory, cpu, FIELD_SHAKE_CHANCE);
    if (!cpu->carry) {
        OpLda(memory, cpu, FIELD_SHAKE_AMPLITUDE);
        if (!AdvanceEffectChild(memory, cpu, child, context,
                0x848155u, 0x808299u, 3u, 1u, index_width, &result))
            return result;
        OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
        OpStz(memory, cpu, OpDp(cpu, DP_SCRATCH_B));
        TransferDirectToA(cpu);
        OpLda(memory, cpu, FIELD_SHAKE_AMPLITUDE);
        OpRepWidths(cpu, 0x20u);
        OpLsrA(cpu);
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_SCRATCH_A)));
        OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
        OpLoadA(cpu, 2u);
        if (!AdvanceEffectChild(memory, cpu, child, context,
                0x84816du, 0x808299u, 3u, 0u, index_width, &result))
            return result;
        OpOraValue(cpu, 0u);
        OpRepWidths(cpu, 0x20u);
        if (!cpu->zero) {
            OpLda(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
            OpSta(memory, cpu, FIELD_SCREEN_OFFSET_X);
            TransferDirectToA(cpu);
            OpSta(memory, cpu, FIELD_SCREEN_OFFSET_Y);
        } else {
            OpLda(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
            OpSta(memory, cpu, FIELD_SCREEN_OFFSET_Y);
            TransferDirectToA(cpu);
            OpSta(memory, cpu, FIELD_SCREEN_OFFSET_X);
        }
        OpSepWidths(cpu, 0x20u);
    }
    return ExecutionReturned(0x848192u);
}

Lufia2ExecutionResult Lufia2FieldUpdateScreenEffects(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ScreenEffectContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x848000u);
    Lufia2ExecutionResult result;

    LoadAAbsolute8(memory, cpu, WRAM_SCREEN_EFFECTS, 0);
    BitImmediate8(cpu, 0x04u);
    if (!cpu->zero) {
        if (!AdvanceEffectChild(memory, cpu, child, context,
                0x848007u, 0x848145u, 2u, 1u, cpu->index_is_8_bit, &result))
            return result;
        LoadAAbsolute8(memory, cpu, WRAM_SCREEN_EFFECTS, 0);
    }
    BitImmediate8(cpu, 0x02u);
    if (!cpu->zero) {
        if (!AdvanceEffectChild(memory, cpu, child, context,
                0x848011u, 0x8480e6u, 2u, 1u, cpu->index_is_8_bit, &result))
            return result;
        LoadAAbsolute8(memory, cpu, WRAM_SCREEN_EFFECTS, 0);
    }
    BitImmediate8(cpu, 0x01u);
    if (!cpu->zero) {
        if (!AdvanceEffectChild(memory, cpu, child, context,
                0x84801bu, 0x848115u, 2u, 1u, cpu->index_is_8_bit, &result))
            return result;
        LoadAAbsolute8(memory, cpu, WRAM_SCREEN_EFFECTS, 0);
    }
    BitImmediate8(cpu, 0x30u);
    if (!cpu->zero) {
        LoadA8(cpu, (uint8_t)(Read8(memory, COLOR_STEP_TIMER) - 1u));
        Write8(memory, COLOR_STEP_TIMER, A8(cpu));
        if (cpu->zero) {
            LoadA8(cpu, 0x06u);
            Write8(memory, COLOR_STEP_TIMER, A8(cpu));
            LoadAAbsolute8(memory, cpu, WRAM_SCREEN_EFFECTS, 0);
            BitImmediate8(cpu, 0x10u);
            if (!AdvanceEffectChild(memory, cpu, child, context,
                    cpu->zero ? 0x848042u : 0x84803du,
                    cpu->zero ? 0x8480d3u : 0x8480c0u,
                    2u, 1u, cpu->index_is_8_bit, &result))
                return result;
        }
        LoadAAbsolute8(memory, cpu, WRAM_SCREEN_EFFECTS, 0);
    }
    BitImmediate8(cpu, 0x80u);
    if (!cpu->zero) {
        LoadAAbsolute8(memory, cpu, COLOR_FADE_SPEED, 0);
        LsrA8(cpu);
        LsrA8(cpu);
        LsrA8(cpu);
        StoreADirect8(memory, cpu, DP_SCRATCH_B);
        LoadAAbsolute8(memory, cpu, COLOR_FADE_PROGRESS, 0);
        cpu->carry = 0;
        Adc8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, COLOR_FADE_SPEED, 0)));
        StoreAAbsolute8(memory, cpu, COLOR_FADE_PROGRESS, 0);
        LsrA8(cpu);
        LsrA8(cpu);
        LsrA8(cpu);
        StoreADirect8(memory, cpu, DP_SCRATCH_A);
        LoadAAbsolute8(memory, cpu, COLOR_FADE_SOURCE, 0);
        And8(cpu, 0x1fu);
        Compare8(cpu, A8(cpu),
            Read8(memory, AbsoluteIndexedAddress(cpu, COLOR_FADE_TARGET, 0)));
        if (cpu->carry) {
            LoadA8(cpu, 0x1fu);
            Sbc8(cpu, DirectByte(memory, cpu, DP_SCRATCH_A));
            StoreADirect8(memory, cpu, DP_SCRATCH_A);
            cpu->carry = 1;
            Sbc8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, COLOR_FADE_TARGET, 0)));
        } else {
            LoadAAbsolute8(memory, cpu, COLOR_FADE_TARGET, 0);
            cpu->carry = 1;
            Sbc8(cpu, DirectByte(memory, cpu, DP_SCRATCH_A));
        }
        Compare8(cpu, A8(cpu), DirectByte(memory, cpu, DP_SCRATCH_B));
        if (!cpu->carry) {
            LoadAAbsolute8(memory, cpu, COLOR_FADE_TARGET, 0);
            StoreADirect8(memory, cpu, DP_SCRATCH_A);
            LoadA8(cpu, 0x80u);
            TestBitsAbsolute8(memory, cpu, WRAM_SCREEN_EFFECTS, 0);
        }
        LoadAAbsolute8(memory, cpu, COLOR_FADE_SOURCE, 0);
        And8(cpu, 0xe0u);
        Or8(cpu, DirectByte(memory, cpu, DP_SCRATCH_A));
        StoreAAbsolute8(memory, cpu, SNES_COLDATA, 0);
    }
    LoadAAbsolute8(memory, cpu, WRAM_PALETTE_FADE, 0);
    BitImmediate8(cpu, 0x01u);
    if (!cpu->zero) {
        if (!AdvanceEffectChild(memory, cpu, child, context,
                0x8480a2u, 0x848e07u, 3u, 1u, 0u, &result))
            return result;
        LoadAAbsolute8(memory, cpu, COLOR_FADE_PROGRESS, 0);
        cpu->carry = 0;
        Adc8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, COLOR_FADE_SPEED, 0)));
        StoreAAbsolute8(memory, cpu, COLOR_FADE_PROGRESS, 0);
        LsrA8(cpu);
        LsrA8(cpu);
        LsrA8(cpu);
        Compare8(cpu, A8(cpu), 0x1fu);
        if (cpu->zero) {
            LoadA8(cpu, 0x01u);
            TestBitsAbsolute8(memory, cpu, WRAM_PALETTE_FADE, 0);
            LoadAAbsolute8(memory, cpu, WRAM_PALETTE_FADE, 0);
        }
    }
    return ExecutionReturned(0x8480bfu);
}

enum {
    DP_PALETTE_COLOR = 0x54,
    DP_PALETTE_COMBINED = 0x56,
    DP_PALETTE_RED_STEP = 0x58,
    DP_PALETTE_GREEN_STEP = 0x5a,
    DP_PALETTE_BLUE_STEP = 0x63
};

static void AdvancePaletteLevels(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint8_t subtract) {
    static const uint8_t output[] = {
        DP_PALETTE_RED_STEP, DP_PALETTE_GREEN_STEP, DP_PALETTE_BLUE_STEP};

    for (unsigned channel = 0; channel < 3u; ++channel) {
        const uint16_t level =
            (uint16_t)(WRAM_FIELD_RECOVERY_PALETTE_LEVELS + channel * 2u);
        const uint16_t step =
            (uint16_t)(WRAM_FIELD_RECOVERY_PALETTE_STEPS + channel * 2u);
        OpLda(memory, cpu, OpAbs(cpu, level));
        if (!subtract) {
            cpu->carry = 0;
            OpAdc(memory, cpu, OpAbs(cpu, step));
        } else if (!cpu->zero) {
            cpu->carry = 1;
            OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, step)));
        }
        OpSta(memory, cpu, OpAbs(cpu, level));
        OpSta(memory, cpu, OpDp(cpu, output[channel]));
    }
}

static void FadePaletteColor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint8_t subtract) {
    static const uint16_t masks[] = {0x001fu, 0x03e0u, 0x7c00u};
    static const uint8_t offsets[] = {
        DP_PALETTE_RED_STEP, DP_PALETTE_GREEN_STEP, DP_PALETTE_BLUE_STEP};

    OpLda(memory, cpu, OpLongX(cpu, 0x9b0000u));
    OpSta(memory, cpu, OpDp(cpu, DP_PALETTE_COLOR));
    for (unsigned channel = 0; channel < 3u; ++channel) {
        if (channel)
            OpLda(memory, cpu, OpDp(cpu, DP_PALETTE_COLOR));
        OpAndValue(cpu, masks[channel]);
        cpu->carry = subtract;
        if (subtract) {
            OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, offsets[channel])));
            if (channel ? cpu->negative : !cpu->carry)
                TransferDirectToA(cpu);
        } else {
            OpAdc(memory, cpu, OpDp(cpu, offsets[channel]));
            if (channel < 2u) {
                OpBitValue(cpu, channel ? 0x0400u : 0x0020u);
                if (!cpu->zero)
                    OpLoadA(cpu, masks[channel]);
            } else if (cpu->negative) {
                OpLoadA(cpu, masks[channel]);
            }
        }
        if (!channel)
            OpSta(memory, cpu, OpDp(cpu, DP_PALETTE_COMBINED));
        else if (channel == 1u)
            OpTestBits(memory, cpu, OpDp(cpu, DP_PALETTE_COMBINED), 1u);
        else
            OpOra(memory, cpu, OpDp(cpu, DP_PALETTE_COMBINED));
    }
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_CGRAM_BUFFER));
}

static void FadeScenePalette(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_RECOVERY_PALETTE_PERIOD));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_RECOVERY_PALETTE_DELAY));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_RECOVERY_PALETTE_CONTROL));
    OpBitValue(cpu, 0x80u);
    const uint8_t subtract_levels = !cpu->zero;
    OpRepWidths(cpu, 0x30u);
    AdvancePaletteLevels(memory, cpu, subtract_levels);
    OpLda(memory, cpu, WRAM_FIELD_PALETTE_SOURCE);
    OpTax(cpu);
    OpLda(memory, cpu, WRAM_FIELD_PALETTE_SKIP_BYTES);
    OpTay(cpu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_RECOVERY_PALETTE_CONTROL));
    OpBitValue(cpu, 0x0040u);
    const uint8_t subtract_colors = !cpu->zero;
    do {
        FadePaletteColor(memory, cpu, subtract_colors);
        OpInx(cpu);
        OpInx(cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, 0x00e0u);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 1u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_NMI_UPLOAD_FLAGS), 1u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_RECOVERY_PALETTE_LENGTH));
    OpDecA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_RECOVERY_PALETTE_LENGTH));
    OpAndValue(cpu, 0x1fu);
    if (cpu->zero) {
        OpLda(memory, cpu, 0x0009a9u);
        OpAndValue(cpu, 0xfdu);
        OpSta(memory, cpu, 0x0009a9u);
    }
}

Lufia2ExecutionResult Lufia2FieldFadeScenePalette(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x848e07u);
    FadeScenePalette(memory, cpu);
    return ExecutionReturned(0x848f0fu);
}


static uint8_t ScreenEffectChild(void *context,Lufia2CpuState *cpu,
    uint32_t target,uint32_t site,uint8_t frame) {
    const Lufia2Memory *memory = context;
    cpu->program_bank = (uint8_t)(target >> 16);
    Lufia2ExecutionResult result;
    switch (target) {
    case 0x8480c0u:
        result = Lufia2FieldLowerFixedColor(memory, cpu, ScreenEffectChild, context);
        break;
    case 0x8480d3u:
        result = Lufia2FieldRaiseFixedColor(memory, cpu, ScreenEffectChild, context);
        break;
    case 0x8480e6u:
        result = Lufia2FieldRaiseBrightness(memory, cpu, ScreenEffectChild, context);
        break;
    case 0x848115u:
        result = Lufia2FieldLowerBrightness(memory, cpu, ScreenEffectChild, context);
        break;
    case 0x848145u:
        result = Lufia2FieldUpdateScreenShake(memory, cpu, ScreenEffectChild, context);
        break;
    case 0x848e07u:
        result = Lufia2FieldFadeScenePalette(memory, cpu);
        break;
    case 0x808299u:
        Lufia2RandomScale(memory, cpu);
        result = ExecutionReturned(0x8082c6u);
        break;
    default:
        cpu->resume_pc = target;
        return 0u;
    }
    if (result.flow != LUFIA2_EXECUTION_RETURNED) {
        if (result.flow == LUFIA2_EXECUTION_BOUNDARY)
            cpu->resume_pc = result.pc;
        return 0u;
    }
    uint16_t word = Pull8(memory, cpu);
    word |= (uint16_t)Pull8(memory, cpu) << 8;
    if (frame == 3u)
        cpu->program_bank = Pull8(memory, cpu);
    cpu->resume_pc = ((uint32_t)cpu->program_bank << 16) | (uint16_t)(word + 1u);
    return cpu->resume_pc == ((site & 0xff0000u) | (uint16_t)(site + frame + 1u));
}

Lufia2ExecutionResult Lufia2FieldScreenEffects(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return Lufia2FieldUpdateScreenEffects(
        memory, cpu, ScreenEffectChild, (void *)memory);
}

static bool FieldPaletteEntryContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x83u && cpu->data_bank == 0x7fu &&
           cpu->accumulator_is_8_bit && !cpu->index_is_8_bit && !cpu->decimal &&
           !cpu->direct_page && cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}
Lufia2ExecutionResult Lufia2FieldAdvancePaletteCycles(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu) {
    if (!FieldPaletteEntryContext(cpu))
        return ExecutionHandoff(cpu, 0x83aeedu);
    Lufia2ExecutionResult result = ExecutionReturned(0x83af53u);
    if (!FieldPaletteCyclesBody(memory, cpu, &result.dispatches))
        return ExecutionHandoff(cpu, cpu->resume_pc);
    return result;
}
Lufia2ExecutionResult Lufia2FieldStepWaveTable(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu) {
    if (!FieldPaletteEntryContext(cpu))
        return ExecutionHandoff(cpu, 0x83af54u);
    FieldWaveTableBody(memory, cpu);
    return ExecutionReturned(0x83afccu);
}
