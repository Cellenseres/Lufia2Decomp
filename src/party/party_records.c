/* Party record pointers and restoration ($81:F5ED, F60B, F78D, F7BD). */

#include "core/cpu_internal.h"
#include "core/cpu_ops.h"
#include "lufia2/party.h"
#include "party/party_internal.h"
#include "system/dp_scratch.h"
#include "system/wram.h"

enum { MENU_PARTY_CAPACITY = 4u };

static bool MenuPartyReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x81u && cpu->accumulator_is_8_bit &&
           cpu->stack >= 0x1f10u && cpu->stack <= 0x1ffcu;
}

static void RefreshMenuPartyRecords(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                    uint16_t back) {
    SimulateJsrFrame(memory, cpu, back);
    Lufia2PartyPointers(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

Lufia2ExecutionResult Lufia2PartyAddMenuMember(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu) {
    if (!MenuPartyReady(cpu))
        return ExecutionHandoff(cpu, 0x81f723u);
    OpSepWidths(cpu, 0x10u);
    OpLdx(cpu, 0u);
    do {
        OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, WRAM_MENU_PARTY_FIRST_ID)));
        if (cpu->negative) {
            OpSta(memory, cpu, OpAbsX(cpu, WRAM_MENU_PARTY_FIRST_ID));
            OpStepMem(memory, cpu, OpAbs(cpu, WRAM_MENU_PARTY_MEMBER_COUNT), 1);
            OpRepWidths(cpu, 0x10u);
            RefreshMenuPartyRecords(memory, cpu, 0xf74cu);
            cpu->carry = 0u;
            return ExecutionReturned(0x81f74eu);
        }
        OpCmp(memory, cpu, OpAbsX(cpu, WRAM_MENU_PARTY_FIRST_ID));
        if (cpu->zero) {
            OpRepWidths(cpu, 0x10u);
            OpLoadA(cpu, 2u);
            cpu->carry = 1u;
            return ExecutionReturned(0x81f741u);
        }
        OpInx(cpu);
        OpCpx(cpu, MENU_PARTY_CAPACITY);
    } while (!cpu->zero);
    OpRepWidths(cpu, 0x10u);
    OpLoadA(cpu, 1u);
    cpu->carry = 1u;
    return ExecutionReturned(0x81f73bu);
}

Lufia2ExecutionResult Lufia2PartyRemoveMenuMember(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu) {
    if (!MenuPartyReady(cpu))
        return ExecutionHandoff(cpu, 0x81f74fu);
    OpSepWidths(cpu, 0x10u);
    OpLdx(cpu, 0u);
    for (;;) {
        OpCmp(memory, cpu, OpAbsX(cpu, WRAM_MENU_PARTY_FIRST_ID));
        if (cpu->zero)
            break;
        OpInx(cpu);
        OpCpx(cpu, MENU_PARTY_CAPACITY);
        if (cpu->zero) {
            OpRepWidths(cpu, 0x10u);
            OpLoadA(cpu, 2u);
            cpu->carry = 1u;
            return ExecutionReturned(0x81f762u);
        }
    }
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_PARTY_MEMBER_COUNT)));
    OpDey(cpu);
    if (cpu->zero) {
        OpRepWidths(cpu, 0x10u);
        OpLoadA(cpu, 1u);
        cpu->carry = 1u;
        return ExecutionReturned(0x81f76eu);
    }
    for (;;) {
        OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_PARTY_MEMBER_COUNT), cpu->y);
        OpInx(cpu);
        OpCpx(cpu, MENU_PARTY_CAPACITY);
        if (cpu->zero)
            break;
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_MENU_PARTY_FIRST_ID));
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_MENU_PARTY_MEMBER_COUNT));
    }
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_MENU_PARTY_MEMBER_COUNT));
    OpRepWidths(cpu, 0x10u);
    RefreshMenuPartyRecords(memory, cpu, 0xf786u);
    cpu->carry = 0u;
    return ExecutionReturned(0x81f788u);
}

/* Copy a word within each active party record ($0A80). */
static void PartyCopy(const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t from,
                      uint16_t to) {
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
            LoadA8(cpu, DirectByte(memory, cpu, DP_SCRATCH_A));
            do {
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
