/* NMI brightness fade ($80:86C1). */

#include "core/cpu_internal.h"
#include "system/system_internal.h"

/* $80:86C1: screen fade from $0581 into the $0583 brightness. */
Lufia2ActorPrimaryUpdateResult Lufia2ScreenFade(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    if (!cpu->accumulator_is_8_bit)
        return FieldLoopHandoff(cpu, 0x8086c1u);
    LoadAAbsolute8(memory, cpu, 0x0581u, 0);                   /* 86C1 */
    if (!cpu->negative)
        return FieldLoopResult(0x808702u);
    ExchangeAccumulatorBytes(cpu);
    LoadAAbsolute8(memory, cpu, 0x0581u, 0);
    And8(cpu, 0x3fu);
    cpu->carry = 0;
    Adc8(cpu, AbsoluteByte(memory, cpu, 0x0582u, 0));
    StoreAAbsolute8(memory, cpu, 0x0582u, 0);
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    ExchangeAccumulatorBytes(cpu);
    BitImmediate8(cpu, 0x40u);
    if (!cpu->zero) {
        ExchangeAccumulatorBytes(cpu);                         /* 86DB */
        cpu->carry = 1;
        Sbc8(cpu, 0x0fu);
        LoadA8(cpu, (uint8_t)(A8(cpu) ^ 0xffu));
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        StoreAAbsolute8(memory, cpu, 0x0583u, 0);
        DecrementA8(cpu);
        if (!cpu->negative)
            return FieldLoopResult(0x808702u);
        StoreZeroAbsolute8(memory, cpu, 0x0581u, 0);
        StoreAImmediate8(memory, cpu, 0x80u, 0x0583u);
    } else {
        ExchangeAccumulatorBytes(cpu);                         /* 86F2 */
        StoreAAbsolute8(memory, cpu, 0x0583u, 0);
        Compare8(cpu, A8(cpu), 0x0fu);
        if (!cpu->carry)
            return FieldLoopResult(0x808702u);
        StoreZeroAbsolute8(memory, cpu, 0x0581u, 0);
        StoreAImmediate8(memory, cpu, 0x0fu, 0x0583u);
    }
    return FieldLoopResult(0x808702u);
}
