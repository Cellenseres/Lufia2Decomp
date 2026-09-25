/* NMI brightness fade ($80:86C1). */

#include "core/cpu_internal.h"
#include "lufia2/system.h"
#include "system/system_internal.h"
#include "system/wram.h"

/* $80:86C1: screen fade from $0581 into the $0583 brightness. */
Lufia2ExecutionResult Lufia2ScreenFade(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x8086c1u);
    LoadAAbsolute8(memory, cpu, WRAM_FADE_CONTROL, 0);         /* 86C1 */
    if (!cpu->negative)
        return ExecutionReturned(0x808702u);
    ExchangeAccumulatorBytes(cpu);
    LoadAAbsolute8(memory, cpu, WRAM_FADE_CONTROL, 0);
    And8(cpu, 0x3fu);
    cpu->carry = 0;
    Adc8(cpu, AbsoluteByte(memory, cpu, WRAM_FADE_LEVEL, 0));
    StoreAAbsolute8(memory, cpu, WRAM_FADE_LEVEL, 0);
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
        StoreAAbsolute8(memory, cpu, WRAM_BRIGHTNESS, 0);
        DecrementA8(cpu);
        if (!cpu->negative)
            return ExecutionReturned(0x808702u);
        StoreZeroAbsolute8(memory, cpu, WRAM_FADE_CONTROL, 0);
        StoreAImmediate8(memory, cpu, BRIGHTNESS_FORCED_BLANK, WRAM_BRIGHTNESS);
    } else {
        ExchangeAccumulatorBytes(cpu);                         /* 86F2 */
        StoreAAbsolute8(memory, cpu, WRAM_BRIGHTNESS, 0);
        Compare8(cpu, A8(cpu), 0x0fu);
        if (!cpu->carry)
            return ExecutionReturned(0x808702u);
        StoreZeroAbsolute8(memory, cpu, WRAM_FADE_CONTROL, 0);
        StoreAImmediate8(memory, cpu, BRIGHTNESS_FULL, WRAM_BRIGHTNESS);
    }
    return ExecutionReturned(0x808702u);
}
