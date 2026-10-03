/* Party record pointers and restoration ($81:F5ED, F60B, F78D, F7BD). */

#include "core/cpu_internal.h"
#include "lufia2/party.h"
#include "party/party_internal.h"
#include "system/dp_scratch.h"
#include "system/wram.h"

/* Copy a word within each active party record ($0A80). */
static void PartyCopy(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t from, uint16_t to) {
    LoadY16(cpu, 0x0006u);
    do {
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu,
            WRAM_MENU_PARTY_CHARACTER_OFFSET, cpu->y));
        if (!cpu->zero) {
            LoadAAbsolute8(memory, cpu, 0x000fu, cpu->x);
            cpu->zero = (A8(cpu) & 0x04u) == 0;
            if (cpu->zero) {
                SetAccumulatorWidth(cpu, 0);
                LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, from, cpu->x));
                StoreAAbsolute16(memory, cpu, to, cpu->x);
                SetAccumulatorWidth(cpu, 1);
            }
        }
        LoadY16(cpu, (uint16_t)(cpu->y - 2u));
    } while (!cpu->negative);
}

/* $81:F5ED: $11 = $25 in each active party record; M1X0. */
Lufia2ExecutionResult Lufia2PartyRestore11(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    PartyCopy(memory, cpu, 0x0025u, 0x0011u);
    return ExecutionReturned(0x81f60au);
}

/* $81:F60B: $13 = $27 in each active party record; M1X0. */
Lufia2ExecutionResult Lufia2PartyRestore13(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    PartyCopy(memory, cpu, 0x0027u, 0x0013u);
    return ExecutionReturned(0x81f628u);
}

/* $81:F7BD: X = word X of $85:9EBA; P kept. */
Lufia2ExecutionResult Lufia2BattleTable9EBA(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 0);
    TransferXToA(cpu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x859ebau, cpu->x)));
    TransferAToX(cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x81f7c9u);
}

/* $81:F78D: $0A80 = records of members $0A7B ($85:9EBA), 0 if bit 7. */
Lufia2ExecutionResult Lufia2PartyPointers(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadY16(cpu, 0x0000u);
    do {
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_MENU_PARTY_FIRST_ID,
            cpu->y));
        cpu->zero = (cpu->accumulator & 0x0080u) == 0;
        if (cpu->zero) {
            And16(cpu, 0x00ffu);
            AslA16(cpu);
            TransferAToX(cpu);
            LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x859ebau, cpu->x)));
            StoreWordAbsolute(memory, cpu, 0x09f2u, cpu->accumulator);
        } else {
            StoreWordAbsolute(memory, cpu, 0x09f2u, 0);
        }
        LoadA16(cpu, cpu->y);
        AslA16(cpu);
        TransferAToX(cpu);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x09f2u, 0));
        StoreAAbsolute16(memory, cpu, WRAM_MENU_PARTY_CHARACTER_OFFSET, cpu->x);
        IncrementY16(cpu);
        Compare16(cpu, cpu->y, 0x0004u);
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x81f7bcu);
}

/* $81:F789: far entry of Lufia2PartyPointers. */
Lufia2ExecutionResult Lufia2PartyPointersFar(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0xf78bu);
    (void)Lufia2PartyPointers(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    return ExecutionReturned(0x81f78cu);
}

/* $83:C652: search each member's list at record + $96 for A. */
Lufia2ExecutionResult Lufia2PartyListHasEntry(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);                                 /* C652 */
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_A), A8(cpu));
    LoadA8(cpu, 0x00u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    LoadY16(cpu, 0x0000u);
    do {
        PushY(memory, cpu);                                    /* C65C */
        TransferDirectToA(cpu);
        LoadAAbsolute8(memory, cpu, WRAM_MENU_PARTY_FIRST_ID, cpu->y);
        Compare8(cpu, A8(cpu), 0xffu);
        if (!cpu->zero) {
            AslA8(cpu);
            TransferAToX(cpu);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16Long(
                memory, LongIndexedAddress(0x859ebau, cpu->x)));
            cpu->carry = 0;
            Add16Value(cpu, 0x0096u);
            Write16Direct(memory, cpu, DP_SCRATCH_C, cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            LoadY16(cpu, 0x0000u);                             /* C675 */
            do {
                LoadA8(cpu, DirectByte(memory, cpu, DP_SCRATCH_A));
                Compare8(cpu, A8(cpu),
                         Read8(memory, AbsoluteIndexedAddress(
                                           cpu, Read16Direct(memory, cpu, DP_SCRATCH_C),
                                           cpu->y)));
                if (cpu->zero) {
                    cpu->y = PullIndexValue(memory, cpu);      /* C68E */
                    PullDataBank(memory, cpu);
                    cpu->carry = 1;
                    return ExecutionReturned(0x83c691u);
                }
                IncrementY16(cpu);
                Compare16(cpu, cpu->y, 0x0024u);
            } while (!cpu->carry);
        }
        cpu->y = PullIndexValue(memory, cpu);                  /* C684 */
        IncrementY16(cpu);
        Compare16(cpu, cpu->y, 0x0004u);
    } while (!cpu->carry);
    PullDataBank(memory, cpu);                                 /* C68B */
    cpu->carry = 0;
    return ExecutionReturned(0x83c68du);
}
