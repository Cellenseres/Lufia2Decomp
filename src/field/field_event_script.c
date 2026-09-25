/* Field event script slots and their wait timers. */

#include "core/cpu_internal.h"
#include "lufia2/field.h"
#include "field/field_internal.h"
#include "system/wram.h"

/* Eight slots; bit 7 armed, bits 0-6 frames left. */
#define EVENT_SLOT_TIMERS 0x7fd18cu
#define EVENT_SLOT_COUNT 0x0008u
/* Layer redraw requests from the scripts, become $74 bits. */
#define WRAM_EVENT_REDRAW 0x1273u
/* $05B5 bit 1: a slot is still armed. */
#define FIELD_FLAG_EVENT_ARMED 0x02u

/* $80:CBAE: count down the slot timers; 0 = handoff. */
uint8_t Lufia2FieldEventTimerBody(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    cpu->program_bank = 0x80u;
    PushDataBank(memory, cpu);                                 /* CBAE */
    Push8(memory, cpu, PackStatus(cpu));
    SetIndexWidth(cpu, 0);
    Write8(memory, AbsoluteIndexedAddress(cpu, WRAM_EVENT_REDRAW, 0), 0x00u);
    LoadA8(cpu, FIELD_FLAG_EVENT_ARMED);
    TestBitsAbsolute8(memory, cpu, WRAM_FIELD_FLAGS, 0);
    LoadX16(cpu, 0x0000u);
    do {
        const uint32_t timer = LongIndexedAddress(EVENT_SLOT_TIMERS, cpu->x);

        LoadA8(cpu, Read8(memory, timer));                     /* CBBD */
        if (cpu->negative) {
            /* An armed $80 wraps to $7F and disarms. */
            DecrementA8(cpu);
            Write8(memory, timer, A8(cpu));
            And8(cpu, 0x7fu);
            if (cpu->zero) {
                /* Event script VM $80:CC35 resumes the slot. */
                cpu->resume_pc = 0x80cbccu;
                return 0;
            }
        }
        IncrementX16(cpu);                                     /* CBF0 */
        Compare16(cpu, cpu->x, EVENT_SLOT_COUNT);
    } while (!cpu->carry);
    LoadX16(cpu, 0x0007u);                                     /* CBF6 */
    for (;;) {
        LoadA8(cpu, Read8(memory,
            LongIndexedAddress(EVENT_SLOT_TIMERS, cpu->x)));
        if (cpu->negative) {
            LoadA8(cpu, FIELD_FLAG_EVENT_ARMED);               /* CBFF */
            TestBitsAbsolute8(memory, cpu, WRAM_FIELD_FLAGS, 1);
            break;
        }
        LoadX16(cpu, (uint16_t)(cpu->x - 1u));                 /* CC06 */
        if (cpu->negative)
            break;
    }
    /* Non-zero only after a script ran, or DB in ROM. */
    LoadAAbsolute8(memory, cpu, WRAM_EVENT_REDRAW, 0);         /* CC09 */
    if (!cpu->zero) {
        cpu->resume_pc = 0x80cc0eu;
        return 0;
    }
    UnpackStatus(cpu, Pull8(memory, cpu));                     /* CC23 */
    PullDataBank(memory, cpu);
    return 1;
}

/* $80:CBAE from a JSL; M=0 decodes differently, LLE. */
Lufia2ExecutionResult Lufia2FieldEventTimerTick(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;

    result.flow = LUFIA2_EXECUTION_RETURNED;
    result.pc = 0x80cc25u;
    result.dispatches = 0;
    if (!cpu->accumulator_is_8_bit) {
        result.flow = LUFIA2_EXECUTION_BOUNDARY;
        result.pc = cpu->resume_pc = 0x80cbaeu;
        return result;
    }
    if (!Lufia2FieldEventTimerBody(memory, cpu)) {
        result.flow = LUFIA2_EXECUTION_BOUNDARY;
        result.pc = cpu->resume_pc;
    }
    return result;
}
