/* Upcoming text dimensions ($80:C652). */
#include "core/cpu_internal.h"
#include "lufia2/text.h"
#include "system/dp_scratch.h"
#include "system/wram.h"
#include "text/text_internal.h"

/* $54: maximum width; $55: lines beyond the first; $56: current width. */
static void TextMeasureLine(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadA8(cpu, DirectByte(memory, cpu, DP_SCRATCH_C));
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_C), 0);
    Compare8(cpu, A8(cpu), DirectByte(memory, cpu, DP_SCRATCH_A));
    if (cpu->carry)
        StoreADirect8(memory, cpu, DP_SCRATCH_A);
}

/* $80:C701: dictionary index B:A, terminated by a zero byte. */
static void TextMeasureDictionary(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    StoreAAbsolute8(memory, cpu, 0x09b0u, 0);
    ExchangeAccumulatorBytes(cpu);
    StoreAAbsolute8(memory, cpu, 0x09afu, 0);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x09afu, 0));
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x8eea00u, cpu->x)));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    for (;;) {
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x8eea00u, cpu->x)));
        if (cpu->zero)
            return;
        IncrementDirect8(memory, cpu, DP_SCRATCH_C);
        IncrementX16(cpu);
    }
}

Lufia2ExecutionResult Lufia2TextMeasure(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Write16Absolute(memory, cpu, 0x09b7u, cpu->y);             /* C652 */
    LoadAAbsolute8(memory, cpu, WRAM_SCENE_SCRIPT_BANK, 0);
    StoreADirect8(memory, cpu, DP_SCRATCH_D);
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_A), 0);
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_B), 0);
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_C), 0);
    for (;;) {
        Lufia2TextNextByte(memory, cpu, 0xc662u);             /* C660 */
        Compare8(cpu, A8(cpu), 0x80u);
        if (cpu->carry) {
            cpu->carry = 1;
            Sbc8(cpu, 0x80u);                               /* C6EB */
            ExchangeAccumulatorBytes(cpu);
            LoadA8(cpu, 2);
            TextMeasureDictionary(memory, cpu);
            continue;
        }
        Compare8(cpu, A8(cpu), 0x09u);
        if (cpu->zero) {
            LoadX16(cpu, 0);
            for (;;) {                                     /* C6DC */
                LoadAAbsolute8(memory, cpu, 0x0badu, cpu->x);
                if (cpu->zero)
                    break;
                IncrementDirect8(memory, cpu, DP_SCRATCH_C);
                IncrementX16(cpu);
            }
            continue;
        }
        Compare8(cpu, A8(cpu), 0);
        if (cpu->zero)
            break;
        Compare8(cpu, A8(cpu), 1);
        if (cpu->zero)
            break;
        Compare8(cpu, A8(cpu), 0x0bu);
        if (cpu->zero)
            break;
        Compare8(cpu, A8(cpu), 5);
        if (cpu->zero) {
            Lufia2TextNextByte(memory, cpu, 0xc6f5u);
            ExchangeAccumulatorBytes(cpu);
            LoadA8(cpu, 0);
            TextMeasureDictionary(memory, cpu);
            continue;
        }
        Compare8(cpu, A8(cpu), 6);
        if (cpu->zero) {
            Lufia2TextNextByte(memory, cpu, 0xc6fdu);
            ExchangeAccumulatorBytes(cpu);
            LoadA8(cpu, 1);
            TextMeasureDictionary(memory, cpu);
            continue;
        }
        Compare8(cpu, A8(cpu), 0x0au);
        if (cpu->zero) {
            Lufia2TextNextByte(memory, cpu, 0xc6c8u);
            Lufia2TextNextByte(memory, cpu, 0xc6cbu);
            LsrA8(cpu); LsrA8(cpu); LsrA8(cpu); LsrA8(cpu);
            cpu->carry = 0;
            Adc8(cpu, 2);
            Adc8(cpu, DirectByte(memory, cpu, DP_SCRATCH_C));
            StoreADirect8(memory, cpu, DP_SCRATCH_C);
            continue;
        }
        Compare8(cpu, A8(cpu), 0x10u);
        if (cpu->carry) {
            IncrementDirect8(memory, cpu, DP_SCRATCH_C);
            continue;
        }
        ExchangeAccumulatorBytes(cpu);                      /* C693 */
        LoadA8(cpu, 0);
        ExchangeAccumulatorBytes(cpu);
        SetAccumulatorWidth(cpu, 0);
        AslA16(cpu);
        TransferAToX(cpu);
        LoadA16(cpu, cpu->y);
        cpu->carry = 0;
        Add16Value(cpu, Read16Long(memory, LongIndexedAddress(0x80c744u, cpu->x)));
        Lufia2TextSetScriptPointer(memory, cpu, 0xc6a3u);
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x80c764u, cpu->x)));
        cpu->carry = 0;
        Adc8(cpu, DirectByte(memory, cpu, DP_SCRATCH_C));
        StoreADirect8(memory, cpu, DP_SCRATCH_C);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x80c765u, cpu->x)));
        if (cpu->zero)
            continue;
        cpu->carry = 0;
        Adc8(cpu, DirectByte(memory, cpu, DP_SCRATCH_B));
        StoreADirect8(memory, cpu, DP_SCRATCH_B);
        TextMeasureLine(memory, cpu);
    }
    TextMeasureLine(memory, cpu);                            /* C724 */
    LoadA8(cpu, DirectByte(memory, cpu, DP_SCRATCH_A));
    StoreAAbsolute8(memory, cpu, 0x125bu, 0);
    LoadA8(cpu, DirectByte(memory, cpu, DP_SCRATCH_B));
    LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
    StoreAAbsolute8(memory, cpu, 0x125cu, 0);
    LoadA8(cpu, DirectByte(memory, cpu, DP_SCRATCH_D));
    StoreAAbsolute8(memory, cpu, WRAM_SCENE_SCRIPT_BANK, 0);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x09b7u, 0));
    return ExecutionReturned(0x80c743u);
}
