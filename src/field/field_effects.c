/* Field colour and screen effects. */

#include "core/cpu_internal.h"
#include "lufia2/field.h"
#include "lufia2/system.h"
#include "field/field_internal.h"
#include "system/system_internal.h"

/* $83:AEED: palette cycles from bank $A1; 0 = cap hit. */
static uint8_t FieldPaletteCycles(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t *visits) {

    SimulateJsrFrame(memory, cpu, 0xaee6u);
    LoadA8(cpu, Read8(memory, 0x0009a9u));                     /* AEED */
    BitImmediate8(cpu, 0x06u);
    if (!cpu->zero) {
        SimulateRtsFrame(memory, cpu);
        return 1;
    }
    LoadA8(cpu, Read8(memory, 0x7fd0f7u));
    if (cpu->zero) {
        SimulateRtsFrame(memory, cpu);
        return 1;
    }
    Write8(memory, DirectAddress(cpu, 0x58u), A8(cpu));
    LoadX16(cpu, 0x0000u);
    do {
        const uint32_t timer = AbsoluteIndexedAddress(cpu, 0xed01u, cpu->x);
        const uint8_t left = (uint8_t)(Read8(memory, timer) - 1u);

        Write8(memory, timer, left);                           /* AF00 */
        SetNz8(cpu, left);
        if (!cpu->zero)
            goto next;
        SetAccumulatorWidth(cpu, 0);                           /* AF05 */
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xec00u, cpu->x));
        IncrementA16(cpu);
        IncrementA16(cpu);
        IncrementA16(cpu);
        Write16Long(memory, AbsoluteIndexedAddress(cpu, 0xec00u, cpu->x),
            cpu->accumulator);
        cpu->y = cpu->x;                                       /* TXY */
        SetNz16(cpu, cpu->y);
        for (;;) {
            if (*visits >= 0x10000u) {
                /* Zero-length frames can chain forever. */
                cpu->resume_pc = 0x83af11u;
                return 0;
            }
            ++*visits;
            TransferAToX(cpu);                                 /* AF11 */
            SetAccumulatorWidth(cpu, 1);
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0xa10000u, cpu->x)));
            StoreAAbsolute8(memory, cpu, 0xed01u, cpu->y);
            SetAccumulatorWidth(cpu, 0);
            if (!cpu->zero)
                break;
            LoadA16(cpu, cpu->y);                              /* AF1F */
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
        Write16Direct(memory, cpu, 0x54u, cpu->accumulator);   /* AF36 */
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xed00u, cpu->y));
        And16(cpu, 0x00ffu);
        AslA16(cpu);
        TransferAToX(cpu);
        LoadA16(cpu, Read16Direct(memory, cpu, 0x54u));
        Write16Long(memory, LongIndexedAddress(0x000320u, cpu->x),
            cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        cpu->x = cpu->y;                                       /* TYX */
        SetNz16(cpu, cpu->x);
        LoadA8(cpu, 0x01u);
        {
            const uint32_t flags = DirectAddress(cpu, 0x73u);  /* TSB $73 */
            const uint8_t value = Read8(memory, flags);

            cpu->zero = (value & 0x01u) == 0;
            Write8(memory, flags, (uint8_t)(value | 0x01u));
        }
next:
        IncrementX16(cpu);                                     /* AF4D */
        IncrementX16(cpu);
        DecrementDirect8(memory, cpu, 0x58u);
    } while (!cpu->zero);
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* $83:AF54: step the HDMA wave table; set up channel 1. */
static void FieldWaveTable(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0xaee9u);
    LoadAAbsolute8(memory, cpu, 0xd0cau, 0);                   /* AF54 */
    Compare8(cpu, A8(cpu), 0xffu);
    if (cpu->zero)
        goto done;
    ExchangeAccumulatorBytes(cpu);
    LoadAAbsolute8(memory, cpu, 0xd0c9u, 0);
    TransferAToX(cpu);
    LoadAAbsolute8(memory, cpu, 0xd0c8u, 0);
    LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
    Write8(memory, 0x7fd0c8u, A8(cpu));
    Compare8(cpu, A8(cpu), Read8(memory, LongIndexedAddress(0x7e0001u, cpu->x)));
    if (!cpu->zero)
        goto done;
    TransferDirectToA(cpu);                                    /* AF6E */
    StoreAAbsolute8(memory, cpu, 0xd0c8u, 0);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7e0000u, cpu->x)));
    AslA8(cpu);
    Write8(memory, DirectAddress(cpu, 0x55u), A8(cpu));
    Write8(memory, DirectAddress(cpu, 0x54u), 0x00u);
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
    Add16Value(cpu, Read16Direct(memory, cpu, 0x54u));
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, 0x7fd0c6u));
    Write16Long(memory, 0x004312u, cpu->accumulator);
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
    StoreAAbsolute8(memory, cpu, 0x4310u, 0);
    LoadA8(cpu, 0x7eu);
    StoreAAbsolute8(memory, cpu, 0x4314u, 0);
    LoadX16(cpu, 0x01e0u);
    Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x4315u, 0), cpu->x);
    LoadA8(cpu, 0x18u);
    StoreAAbsolute8(memory, cpu, 0x4311u, 0);
    LoadX16(cpu, 0x4010u);
    StoreXDirect16(memory, cpu, 0x7bu);
    LoadA8(cpu, 0x42u);
    Write8(memory, DirectAddress(cpu, 0x76u), A8(cpu));
done:
    SimulateRtsFrame(memory, cpu);
}

/* $83:AEB5: per-frame field palette and wave effects. */
Lufia2ExecutionResult Lufia2FieldColourEffects(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;

    result.flow = LUFIA2_EXECUTION_BOUNDARY;
    result.pc = 0x83aeecu;
    result.dispatches = 0;
    Push8(memory, cpu, PackStatus(cpu));                       /* AEB5 */
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    LoadA8(cpu, Read8(memory, 0x0009a9u));
    BitImmediate8(cpu, 0x02u);
    if (!cpu->zero) {
        const uint32_t timer = AbsoluteIndexedAddress(cpu, 0x1280u, 0);
        const uint8_t left = (uint8_t)(Read8(memory, timer) - 1u);

        Write8(memory, timer, left);                           /* AEC2 */
        SetNz8(cpu, left);
        if (cpu->zero) {
            /* $84:8E07 stays LLE. */
            result.pc = cpu->resume_pc = 0x83aec7u;
            return result;
        }
    }
    BitImmediate8(cpu, 0x04u);                                 /* AECF */
    if (!cpu->zero) {
        /* $84:8D54 stays LLE. */
        result.pc = cpu->resume_pc = 0x83aed3u;
        return result;
    }
    BitImmediate8(cpu, 0x01u);                                 /* AEDB */
    if (!cpu->zero) {
        PushDataBank(memory, cpu);
        LoadA8(cpu, 0x7fu);
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        if (!FieldPaletteCycles(memory, cpu, &result.dispatches)) {
            result.pc = cpu->resume_pc;
            return result;
        }
        FieldWaveTable(memory, cpu);
        PullDataBank(memory, cpu);                             /* AEEA */
    }
    UnpackStatus(cpu, Pull8(memory, cpu));                     /* AEEB */
    result.flow = LUFIA2_EXECUTION_RETURNED;
    return result;
}

/* $84:8145: random shake offsets into $7F:D081/D083. */
static void ScreenShake(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0x8009u);
    LoadA8(cpu, 0xffu);                                        /* 8145 */
    SimulateJslFrame(memory, cpu, 0x84u, 0x814au);
    Lufia2RandomScale(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    Compare8(cpu, A8(cpu), Read8(memory, 0x7fd080u));
    if (!cpu->carry) {
        uint8_t first;

        LoadA8(cpu, Read8(memory, 0x7fd07fu));                 /* 8151 */
        SimulateJslFrame(memory, cpu, 0x84u, 0x8158u);
        Lufia2RandomScale(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        StoreADirect8(memory, cpu, 0x54u);
        Write8(memory, DirectAddress(cpu, 0x55u), 0x00u);
        TransferDirectToA(cpu);
        LoadA8(cpu, Read8(memory, 0x7fd07fu));
        SetAccumulatorWidth(cpu, 0);
        LsrA16(cpu);
        cpu->carry = 1;
        Add16Value(cpu, (uint16_t)~Read16Direct(memory, cpu, 0x54u));
        Write16Direct(memory, cpu, 0x54u, cpu->accumulator);
        LoadA16(cpu, 0x0002u);
        SimulateJslFrame(memory, cpu, 0x84u, 0x8170u);
        Lufia2RandomScale(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        LoadA16(cpu, cpu->accumulator);                        /* ORA #0 */
        first = !cpu->zero;
        LoadA16(cpu, Read16Direct(memory, cpu, 0x54u));
        Write16Long(memory, first ? 0x7fd081u : 0x7fd083u, cpu->accumulator);
        TransferDirectToA(cpu);
        Write16Long(memory, first ? 0x7fd083u : 0x7fd081u, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
    }
    SimulateRtsFrame(memory, cpu);
}

/* $84:80E6/$84:8115: brightness step every $7F:D08F frames. */
static void ScreenFade(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t in) {
    SimulateJsrFrame(memory, cpu, in ? 0x8013u : 0x801du);
    LoadA8(cpu, (uint8_t)(Read8(memory, 0x7fd092u) + 1u));
    Write8(memory, 0x7fd092u, A8(cpu));
    Compare8(cpu, A8(cpu), Read8(memory, 0x7fd08fu));
    if (cpu->carry) {
        LoadA8(cpu, 0x00u);
        Write8(memory, 0x7fd092u, A8(cpu));
        LoadA8(cpu, Read8(memory, 0x7fd091u));
        cpu->carry = in ? 0u : 1u;
        if (in)
            Adc8(cpu, Read8(memory, 0x7fd090u));
        else
            Sbc8(cpu, Read8(memory, 0x7fd090u));
        Write8(memory, 0x7fd091u, A8(cpu));
        StoreAAbsolute8(memory, cpu, 0x0583u, 0);
        if (in) {
            Compare8(cpu, A8(cpu), 0x0fu);
            if (cpu->carry) {
                LoadA8(cpu, 0x02u);
                TestBitsAbsolute8(memory, cpu, 0x1261u, 0);
            }
        } else if (cpu->negative) {
            StoreAImmediate8(memory, cpu, 0x80u, 0x0583u);
            StoreZeroAbsolute8(memory, cpu, 0x1261u, 0);
        }
    }
    SimulateRtsFrame(memory, cpu);
}

/* $84:80C0/$84:80D3: color math intensity $1271 down or up. */
static void ScreenColorStep(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t down) {
    SimulateJsrFrame(memory, cpu, down ? 0x803fu : 0x8044u);
    LoadAAbsolute8(memory, cpu, 0x1271u, 0);
    LoadA8(cpu, (uint8_t)((A8(cpu) + (down ? 0xffu : 0x01u)) | 0xe0u));
    StoreAAbsolute8(memory, cpu, 0x1271u, 0);
    Compare8(cpu, A8(cpu), down ? 0xe0u : 0xffu);
    if (cpu->zero) {
        LoadA8(cpu, down ? 0x10u : 0x20u);
        TestBitsAbsolute8(memory, cpu, 0x1261u, 0);
    }
    SimulateRtsFrame(memory, cpu);
}

/* $84:8E07: fade palette $9B:[$7F:D0F8] into $0320 by $58/$5A/$63. */
static void ScreenPaletteFade(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const struct {
        uint16_t level;
        uint16_t step;
        uint8_t out;
    } channels[3] = {
        {0x1274u, 0x127au, 0x58u},
        {0x1276u, 0x127cu, 0x5au},
        {0x1278u, 0x127eu, 0x63u}};
    uint8_t darken;
    unsigned i;

    SimulateJslFrame(memory, cpu, 0x84u, 0x80a5u);
    LoadAAbsolute8(memory, cpu, 0x1281u, 0);                   /* 8E07 */
    StoreAAbsolute8(memory, cpu, 0x1280u, 0);
    LoadAAbsolute8(memory, cpu, 0x1283u, 0);
    BitImmediate8(cpu, 0x80u);
    darken = !cpu->zero;
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    for (i = 0; i < 3u; ++i) {
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, channels[i].level, 0));
        if (!darken) {
            cpu->carry = 0;
            Add16Value(cpu,
                Read16AbsoluteIndexed(memory, cpu, channels[i].step, 0));
        } else if (!cpu->zero) {
            cpu->carry = 1;
            Add16Value(cpu, (uint16_t)~Read16AbsoluteIndexed(
                memory, cpu, channels[i].step, 0));
        }
        Write16Absolute(memory, cpu, channels[i].level, cpu->accumulator);
        Write16Direct(memory, cpu, channels[i].out, cpu->accumulator);
    }
    LoadX16(cpu, Read16Long(memory, 0x7fd0f8u));               /* 8E66 */
    LoadY16(cpu, Read16Long(memory, 0x7fd0fau));
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1283u, 0));
    cpu->zero = (cpu->accumulator & 0x0040u) == 0;
    darken = !cpu->zero;
    do {
        const uint16_t color =
            Read16Long(memory, LongIndexedAddress(0x9b0000u, cpu->x));

        LoadA16(cpu, color);
        Write16Direct(memory, cpu, 0x54u, color);
        if (darken) {
            LoadA16(cpu, (uint16_t)(color & 0x001fu));         /* 8E78 */
            cpu->carry = 1;
            Add16Value(cpu, (uint16_t)~Read16Direct(memory, cpu, 0x58u));
            if (!cpu->carry)
                TransferDirectToA(cpu);
            Write16Direct(memory, cpu, 0x56u, cpu->accumulator);
            LoadA16(cpu, (uint16_t)(color & 0x03e0u));
            cpu->carry = 1;
            Add16Value(cpu, (uint16_t)~Read16Direct(memory, cpu, 0x5au));
            if (cpu->negative)
                TransferDirectToA(cpu);
            TestBitsDirect(memory, cpu, 0x56u, 1);
            LoadA16(cpu, (uint16_t)(color & 0x7c00u));
            cpu->carry = 1;
            Add16Value(cpu, (uint16_t)~Read16Direct(memory, cpu, 0x63u));
            if (cpu->negative)
                TransferDirectToA(cpu);
        } else {
            LoadA16(cpu, (uint16_t)(color & 0x001fu));         /* 8EB1 */
            cpu->carry = 0;
            Add16Value(cpu, Read16Direct(memory, cpu, 0x58u));
            if (cpu->accumulator & 0x0020u)
                LoadA16(cpu, 0x001fu);
            Write16Direct(memory, cpu, 0x56u, cpu->accumulator);
            LoadA16(cpu, (uint16_t)(color & 0x03e0u));
            cpu->carry = 0;
            Add16Value(cpu, Read16Direct(memory, cpu, 0x5au));
            if (cpu->accumulator & 0x0400u)
                LoadA16(cpu, 0x03e0u);
            TestBitsDirect(memory, cpu, 0x56u, 1);
            LoadA16(cpu, (uint16_t)(color & 0x7c00u));
            cpu->carry = 0;
            Add16Value(cpu, Read16Direct(memory, cpu, 0x63u));
            if (cpu->negative)
                LoadA16(cpu, 0x7c00u);
        }
        LoadA16(cpu, (uint16_t)(cpu->accumulator |
            Read16Direct(memory, cpu, 0x56u)));
        Write8(memory, AbsoluteIndexedAddress(cpu, 0x0320u, cpu->y),
            (uint8_t)cpu->accumulator);
        Write8(memory, AbsoluteIndexedAddress(cpu, 0x0321u, cpu->y),
            (uint8_t)(cpu->accumulator >> 8));
        IncrementX16(cpu);
        IncrementX16(cpu);
        IncrementY16(cpu);
        IncrementY16(cpu);
        Compare16(cpu, cpu->y, 0x00e0u);
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);                               /* 8EF4 */
    LoadA8(cpu, 0x01u);
    TestBitsDirect(memory, cpu, 0x73u, 1);
    LoadAAbsolute8(memory, cpu, 0x1282u, 0);
    DecrementA8(cpu);
    StoreAAbsolute8(memory, cpu, 0x1282u, 0);
    And8(cpu, 0x1fu);
    if (cpu->zero) {
        LoadA8(cpu, (uint8_t)(Read8(memory, 0x0009a9u) & 0xfdu));
        Write8(memory, 0x0009a9u, A8(cpu));
    }
    SimulateRtlFrame(memory, cpu);
}

/* $84:8000: screen effects of $1261 and the $1262 palette fade. */
void Lufia2FieldScreenEffects(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadAAbsolute8(memory, cpu, 0x1261u, 0);                   /* 8000 */
    BitImmediate8(cpu, 0x04u);
    if (!cpu->zero) {
        ScreenShake(memory, cpu);
        LoadAAbsolute8(memory, cpu, 0x1261u, 0);
    }
    BitImmediate8(cpu, 0x02u);                                 /* 800D */
    if (!cpu->zero) {
        ScreenFade(memory, cpu, 1);
        LoadAAbsolute8(memory, cpu, 0x1261u, 0);
    }
    BitImmediate8(cpu, 0x01u);                                 /* 8017 */
    if (!cpu->zero) {
        ScreenFade(memory, cpu, 0);
        LoadAAbsolute8(memory, cpu, 0x1261u, 0);
    }
    BitImmediate8(cpu, 0x30u);                                 /* 8021 */
    if (!cpu->zero) {
        LoadA8(cpu, (uint8_t)(Read8(memory, 0x7fd094u) - 1u));
        Write8(memory, 0x7fd094u, A8(cpu));
        if (cpu->zero) {
            LoadA8(cpu, 0x06u);
            Write8(memory, 0x7fd094u, A8(cpu));
            LoadAAbsolute8(memory, cpu, 0x1261u, 0);
            BitImmediate8(cpu, 0x10u);
            ScreenColorStep(memory, cpu, !cpu->zero);
        }
        LoadAAbsolute8(memory, cpu, 0x1261u, 0);               /* 8045 */
    }
    BitImmediate8(cpu, 0x80u);                                 /* 8048 */
    if (!cpu->zero) {
        LoadAAbsolute8(memory, cpu, 0x1289u, 0);               /* COLDATA fade */
        LsrA8(cpu);
        LsrA8(cpu);
        LsrA8(cpu);
        StoreADirect8(memory, cpu, 0x55u);
        LoadAAbsolute8(memory, cpu, 0x1288u, 0);
        cpu->carry = 0;
        Adc8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x1289u, 0)));
        StoreAAbsolute8(memory, cpu, 0x1288u, 0);
        LsrA8(cpu);
        LsrA8(cpu);
        LsrA8(cpu);
        StoreADirect8(memory, cpu, 0x54u);
        LoadAAbsolute8(memory, cpu, 0x1286u, 0);
        And8(cpu, 0x1fu);
        Compare8(cpu, A8(cpu),
            Read8(memory, AbsoluteIndexedAddress(cpu, 0x1287u, 0)));
        if (cpu->carry) {
            LoadA8(cpu, 0x1fu);                                /* toward black */
            Sbc8(cpu, DirectByte(memory, cpu, 0x54u));
            StoreADirect8(memory, cpu, 0x54u);
            cpu->carry = 1;
            Sbc8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x1287u, 0)));
        } else {
            LoadAAbsolute8(memory, cpu, 0x1287u, 0);           /* 807D */
            cpu->carry = 1;
            Sbc8(cpu, DirectByte(memory, cpu, 0x54u));
        }
        Compare8(cpu, A8(cpu), DirectByte(memory, cpu, 0x55u));
        if (!cpu->carry) {
            LoadAAbsolute8(memory, cpu, 0x1287u, 0);           /* 8087 */
            StoreADirect8(memory, cpu, 0x54u);
            LoadA8(cpu, 0x80u);
            TestBitsAbsolute8(memory, cpu, 0x1261u, 0);
        }
        LoadAAbsolute8(memory, cpu, 0x1286u, 0);               /* 8091 */
        And8(cpu, 0xe0u);
        Or8(cpu, DirectByte(memory, cpu, 0x54u));
        StoreAAbsolute8(memory, cpu, 0x2132u, 0);
    }
    LoadAAbsolute8(memory, cpu, 0x1262u, 0);                   /* 809B */
    BitImmediate8(cpu, 0x01u);
    if (!cpu->zero) {
        ScreenPaletteFade(memory, cpu);
        LoadAAbsolute8(memory, cpu, 0x1288u, 0);               /* 80A6 */
        cpu->carry = 0;
        Adc8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x1289u, 0)));
        StoreAAbsolute8(memory, cpu, 0x1288u, 0);
        LsrA8(cpu);
        LsrA8(cpu);
        LsrA8(cpu);
        Compare8(cpu, A8(cpu), 0x1fu);
        if (cpu->zero) {
            LoadA8(cpu, 0x01u);
            TestBitsAbsolute8(memory, cpu, 0x1262u, 0);
            LoadAAbsolute8(memory, cpu, 0x1262u, 0);
        }
    }
}
