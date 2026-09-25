/* Field loop steps and event tick. */

#include "core/cpu_internal.h"
#include "lufia2/field.h"
#include "field/field_internal.h"
#include "text/text_internal.h"

/* $83:80CD: field idle test; zero = no event running. */
void Lufia2FieldIdleBody(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const uint16_t gates[5] = {0x09a8u, 0x0622u, 0x05b7u, 0x05b5u,
                                      0x17aau};
    static const uint8_t masks[5] = {0x08u, 0x88u, 0x07u, 0xa2u, 0x00u};
    unsigned i;

    for (i = 0; i < 5u; ++i) {
        LoadAAbsolute8(memory, cpu, gates[i], 0);              /* 80CD */
        if (masks[i])
            BitImmediate8(cpu, masks[i]);
        if (!cpu->zero)
            return;
    }
    LoadAAbsolute8(memory, cpu, 0x099bu, 0);                   /* 80EE */
    BitImmediate8(cpu, 0x80u);
    if (!cpu->zero)
        return;
    LoadX8(cpu, 0x00u);                                        /* 80F5 */
    do {
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd057u, cpu->x)));
        if (cpu->negative)
            return;
        LoadX8(cpu, (uint8_t)(cpu->x + 1u));                   /* 80FD */
        Compare8(cpu, (uint8_t)cpu->x, 0x08u);
    } while (!cpu->zero);
}

/* $83:80CD from the field loop's JSR (X8 only). */
Lufia2ExecutionResult Lufia2FieldIdleTest(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;

    result.flow = LUFIA2_EXECUTION_RETURNED;
    result.pc = 0x838102u;
    result.dispatches = 0;
    if (!cpu->index_is_8_bit) {
        result.flow = LUFIA2_EXECUTION_BOUNDARY;
        result.pc = cpu->resume_pc = 0x8380cdu;
        return result;
    }
    Lufia2FieldIdleBody(memory, cpu);
    return result;
}

/* $83:8682: tick the eight animation slots at $7F:D057. */
Lufia2ExecutionResult Lufia2FieldAnimationTicks(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const uint32_t slots = 0x7fd057u;
    Lufia2ExecutionResult result;

    result.flow = LUFIA2_EXECUTION_RETURNED;
    result.pc = 0x8386e9u;
    result.dispatches = 0;
    Push8(memory, cpu, PackStatus(cpu));                       /* 8682 */
    SetIndexWidth(cpu, 0);
    Write8(memory, AbsoluteIndexedAddress(cpu, 0x17aau, 0), 0x00u);
    LoadA8(cpu, 0x01u);
    StoreAAbsolute8(memory, cpu, 0x17abu, 0);
    LoadX16(cpu, 0x0000u);
    do {
        LoadA8(cpu, Read8(memory, LongIndexedAddress(slots, cpu->x)));
        if (cpu->negative) {
            ExchangeAccumulatorBytes(cpu);                     /* 8696 */
            LoadAAbsolute8(memory, cpu, 0x17abu, 0);
            TestBitsAbsolute8(memory, cpu, 0x17aau, 1);
            AslA8(cpu);
            StoreAAbsolute8(memory, cpu, 0x17abu, 0);
            ExchangeAccumulatorBytes(cpu);
            cpu->carry = 0;
            Adc8(cpu, 0x08u);
            Write8(memory, LongIndexedAddress(slots, cpu->x), A8(cpu));
            And8(cpu, 0x18u);
            Compare8(cpu, A8(cpu), 0x18u);
            if (cpu->zero) {
                LoadA8(cpu, Read8(memory, LongIndexedAddress(slots, cpu->x)));
                And8(cpu, 0xe7u);                              /* 86B3 */
                Write8(memory, LongIndexedAddress(slots, cpu->x), A8(cpu));
                PushIndex(memory, cpu);
                Write8(memory, 0x7fd049u, A8(cpu));
                LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
                BitImmediate8(cpu, 0x02u);
                if (!cpu->zero)
                    TransferDirectToA(cpu);                    /* 86C3 */
                Write8(memory, LongIndexedAddress(slots, cpu->x), A8(cpu));
                LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd04fu, cpu->x)));
                Write8(memory, 0x7fd04eu, A8(cpu));
                LoadA8(cpu, Read8(memory, 0x7fd04eu));
                /* The slot handlers $83:8783/87CC stay LLE. */
                result.flow = LUFIA2_EXECUTION_BOUNDARY;
                result.pc = cpu->resume_pc =
                    cpu->negative ? 0x8386d6u : 0x8386dbu;
                return result;
            }
        }
        IncrementX16(cpu);                                     /* 86DF */
        Compare16(cpu, cpu->x, 0x0008u);
    } while (!cpu->zero);
    UnpackStatus(cpu, Pull8(memory, cpu));                     /* 86E8 */
    return result;
}

static Lufia2ExecutionResult FieldTickBoundary(
    Lufia2ExecutionResult result,
    Lufia2CpuState *cpu,
    uint32_t pc) {
    result.flow = LUFIA2_EXECUTION_BOUNDARY;
    result.pc = cpu->resume_pc = pc;
    return result;
}

Lufia2ExecutionResult Lufia2FieldEventTick(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;

    result.flow = LUFIA2_EXECUTION_RETURNED;
    result.pc = 0x809cb7u;
    result.dispatches = 0;
    SimulateJslFrame(memory, cpu, 0x80u, 0x9c75u);             /* 9C72 */
    cpu->program_bank = 0x84u;
    Lufia2FieldScreenEffects(memory, cpu);                           /* $84:8000 */
    SimulateRtlFrame(memory, cpu);
    cpu->program_bank = 0x80u;
    SimulateJsrFrame(memory, cpu, 0x9c78u);                    /* 9C76 */
    LoadA8(cpu, Read8(memory, 0x7fd0c1u));                     /* C21A */
    Compare8(cpu, A8(cpu), 0xffu);
    if (!cpu->zero) {
        LoadA8(cpu, (uint8_t)(A8(cpu) - 1u));
        Write8(memory, 0x7fd0c1u, A8(cpu));
        if (cpu->zero) {
            LoadA8(cpu, 0xffu);                                /* C229 */
            Write8(memory, 0x7fd0c1u, A8(cpu));
            LoadA8(cpu, 0xa0u);
            TestBitsAbsolute8(memory, cpu, 0x099cu, 0);
            Lufia2TextWindowClear(memory, cpu, 0xc237u);
            LoadA8(cpu, 0x08u);
            Write8(memory, DirectAddress(cpu, 0x74u), A8(cpu));
        }
    }
    SimulateRtsFrame(memory, cpu);
    LoadAAbsolute8(memory, cpu, 0x099bu, 0);                   /* 9C79 */
    And8(cpu, 0x0au);
    if (!cpu->zero)
        return Lufia2TextPromptTick(memory, cpu, result);
    LoadAAbsolute8(memory, cpu, 0x099bu, 0);                   /* 9CB2 */
    if (cpu->negative)
        return Lufia2TextEngineStepBody(memory, cpu, result);
    return result;
}

/* $83:83A0: menu request; the menu itself runs on LLE. */
Lufia2ExecutionResult Lufia2FieldMenuRequest(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x8383a0u);
    LoadA8(cpu, 0x40u);                                        /* 83A0 */
    TestBitsAbsolute8(memory, cpu, 0x05b5u, 0);
    if (!cpu->zero)
        return ExecutionHandoff(cpu, 0x8383bdu);
    LoadAAbsolute8(memory, cpu, 0x09a7u, 0);
    BitImmediate8(cpu, 0x02u);
    if (!cpu->zero) {
        SetAccumulatorWidth(cpu, 0);                           /* 83AE */
        LoadA16(cpu, 0x9080u);
        And16(cpu, Read16Direct(memory, cpu, 0x46u));
        if (!cpu->zero) {
            TestBitsDirect(memory, cpu, 0x4au, 0);
            if (!cpu->zero) {
                SetAccumulatorWidth(cpu, 1);
                return ExecutionHandoff(cpu, 0x8383bdu);
            }
        }
        SetAccumulatorWidth(cpu, 1);                           /* 83DD */
    }
    return ExecutionReturned(0x8383dfu);
}

/* $83:867B: A & pressed buttons; consume them from $4A. */
Lufia2ExecutionResult Lufia2FieldTakeButtons(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x83867bu);
    And8(cpu, DirectByte(memory, cpu, 0x46u));                 /* 867B */
    if (!cpu->zero)
        TestBitsDirect(memory, cpu, 0x4au, 0);
    return ExecutionReturned(0x838681u);
}

/* $83:8103: $05B7 requests; any set request runs on LLE. */
Lufia2ExecutionResult Lufia2FieldStatusRequests(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x838103u);
    LoadAAbsolute8(memory, cpu, 0x09a8u, 0);                   /* 8103 */
    BitImmediate8(cpu, 0x08u);
    if (cpu->zero) {
        SetIndexWidth(cpu, 0);
        LoadAAbsolute8(memory, cpu, 0x05b7u, 0);               /* 810C */
        BitImmediate8(cpu, 0x02u);
        if (!cpu->zero)
            return ExecutionHandoff(cpu, 0x838113u);
        BitImmediate8(cpu, 0x04u);                             /* 8119 */
        if (!cpu->zero)
            return ExecutionHandoff(cpu, 0x83811du);
        BitImmediate8(cpu, 0x01u);                             /* 8123 */
        if (!cpu->zero)
            return ExecutionHandoff(cpu, 0x838127u);
    }
    SetIndexWidth(cpu, 1);                                     /* 812B */
    return ExecutionReturned(0x83812du);
}

/* $83:85DC: field reload setup; loading runs on LLE. */
Lufia2ExecutionResult Lufia2FieldReloadSetup(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const uint16_t cleared[7] = {
        0x099bu, 0x099cu, 0x1261u, 0x1262u, 0x1254u, 0x09a6u, 0x09adu};
    unsigned i;

    Push8(memory, cpu, PackStatus(cpu));                       /* 85DC */
    PushDataBank(memory, cpu);
    Push8(memory, cpu, 0x83u);                                 /* PHK */
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    StoreZeroAbsolute8(memory, cpu, 0x4200u, 0);
    TransferDirectToA(cpu);
    Write8(memory, 0x7fd0ffu, A8(cpu));
    StoreAAbsolute8(memory, cpu, 0x0562u, 0);
    StoreAAbsolute8(memory, cpu, 0x0563u, 0);
    LoadA8(cpu, 0x80u);
    StoreAAbsolute8(memory, cpu, 0x0583u, 0);
    StoreAAbsolute8(memory, cpu, 0x2100u, 0);
    TransferDirectToA(cpu);                                    /* 85FA */
    StoreADirect8(memory, cpu, 0x6au);
    StoreADirect8(memory, cpu, 0x6fu);
    Write8(memory, DirectAddress(cpu, 0x72u), 0x00u);
    Write8(memory, DirectAddress(cpu, 0x74u), 0x00u);
    Write8(memory, DirectAddress(cpu, 0x73u), 0x00u);
    StoreAAbsolute8(memory, cpu, 0x1255u, 0);
    StoreAAbsolute8(memory, cpu, 0x1256u, 0);
    StoreADirect8(memory, cpu, 0x81u);
    StoreAAbsolute8(memory, cpu, 0x420cu, 0);
    LoadA8(cpu, 0xffu);                                        /* 8610 */
    Write8(memory, 0x7fd0b0u, A8(cpu));
    Write8(memory, 0x7fd4f5u, A8(cpu));
    StoreAAbsolute8(memory, cpu, 0x17acu, 0);
    for (i = 0; i < 4u; ++i)
        StoreZeroAbsolute8(memory, cpu, cleared[i], 0);
    LoadA8(cpu, 0xffu);
    StoreAAbsolute8(memory, cpu, 0x1269u, 0);
    for (i = 4; i < 7u; ++i)
        StoreZeroAbsolute8(memory, cpu, cleared[i], 0);
    /* Map loading from JSR $B062 on. */
    return ExecutionHandoff(cpu, 0x838637u);
}
