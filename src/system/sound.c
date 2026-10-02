/* Deferred sound commands ($84:8766). */

#include "core/cpu_internal.h"
#include "lufia2/system.h"
#include "system/system_internal.h"
#include "system/wram.h"

void Lufia2QueueDeferredSound(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    ExchangeAccumulatorBytes(cpu);                             /* 8766 */
    LoadA8(cpu, Read8(memory, WRAM_FIELD_MAP_FLAGS));
    BitImmediate8(cpu, 0x02u);
    if (cpu->zero) {
        ExchangeAccumulatorBytes(cpu);                         /* 876F */
        Write8(memory, WRAM_SOUND_COMMAND, A8(cpu));
    }
}
