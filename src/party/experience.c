/* Party experience curves ($81:F9E9). */

#include "core/cpu_internal.h"
#include "lufia2/party.h"

enum {
    PARTY_MEMBER = 0x09fau,
    PARTY_LEVEL = 0x09feu,
    PARTY_EXPERIENCE = 0x0a2au,        /* 24-bit, $0A2A-$0A2C */
};

/* One experience step: the 24-bit step $58-$5A grows by itself times
   the factor in $4202 / 256 (hardware multiplier, rounded down). */
static void PartyGrowStep(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA8(cpu, DirectByte(memory, cpu, 0x5bu));               /* FA42 */
    StoreAAbsolute8(memory, cpu, 0x4203u, 0);
    LoadA8(cpu, DirectByte(memory, cpu, 0x59u));
    PushIndex(memory, cpu);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4216u, 0));
    StoreAAbsolute8(memory, cpu, 0x4203u, 0);
    StoreXDirect16(memory, cpu, 0x56u);
    LoadA8(cpu, DirectByte(memory, cpu, 0x5au));
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4216u, 0));
    StoreAAbsolute8(memory, cpu, 0x4203u, 0);
    StoreXDirect16(memory, cpu, 0x54u);
    LoadA8(cpu, DirectByte(memory, cpu, 0x58u));
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4216u, 0));
    StoreAAbsolute8(memory, cpu, 0x4203u, 0);
    TransferDirectToA(cpu);                                    /* FA64 */
    LoadA8(cpu, DirectByte(memory, cpu, 0x54u));
    ExchangeAccumulatorBytes(cpu);
    SetAccumulatorWidth(cpu, 0);
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4216u, 0));
    SetAccumulatorWidth(cpu, 1);
    ExchangeAccumulatorBytes(cpu);
    StoreADirect8(memory, cpu, 0x54u);
    SetAccumulatorWidth(cpu, 0);
    TransferXToA(cpu);
    Add16Value(cpu, Read16Direct(memory, cpu, 0x55u));
    StoreADirect16(memory, cpu, 0x55u);
    if (cpu->carry)
        Increment16Direct(memory, cpu, 0x57u);
    LoadADirect16(memory, cpu, 0x54u);                         /* FA7E */
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x58u));
    StoreADirect16(memory, cpu, 0x58u);
    LoadADirect16(memory, cpu, 0x56u);
    Add16Value(cpu, Read16Direct(memory, cpu, 0x5au));
    StoreADirect16(memory, cpu, 0x5au);
    SetAccumulatorWidth(cpu, 1);
    cpu->x = PullIndexValue(memory, cpu);
}

/* $81:F9E9: experience needed for level $09FE of member $09FA: the
   sum over levels of a step that starts at the member's base ($97:B99E)
   and grows by a factor from $97:B633 + member * $70 (a new factor every
   8 levels); minus 10. Level 98 and up: 9,999,999. */
Lufia2ExecutionResult Lufia2PartyExperienceForLevel(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x81f9e9u);
    PushDataBank(memory, cpu);                                 /* F9E9 */
    LoadA8(cpu, 0x97u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    PushIndex(memory, cpu);
    LoadA8(cpu, 0x70u);
    StoreAAbsolute8(memory, cpu, 0x4202u, 0);
    TransferDirectToA(cpu);
    LoadAAbsolute8(memory, cpu, PARTY_MEMBER, 0);
    StoreAAbsolute8(memory, cpu, 0x4203u, 0);
    AslA8(cpu);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, 0xb633u);                                     /* growth rows */
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4216u, 0));
    TransferAToY(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xb99eu, cpu->x));
    ExchangeAccumulatorBytes(cpu);
    StoreADirect16(memory, cpu, 0x58u);                        /* step */
    Write16Direct(memory, cpu, 0x5au, 0x0000u);
    Write16Direct(memory, cpu, 0x63u, 0x0000u);                /* sum */
    Write16Direct(memory, cpu, 0x65u, 0x0000u);
    SetAccumulatorWidth(cpu, 1);
    LoadAAbsolute8(memory, cpu, PARTY_LEVEL, 0);
    Compare8(cpu, A8(cpu), 0x62u);
    if (cpu->carry) {
        LoadX16(cpu, 0x967fu);                                 /* FABB */
        Write16Absolute(memory, cpu, PARTY_EXPERIENCE, cpu->x);
        LoadA8(cpu, 0x98u);
        StoreAAbsolute8(memory, cpu, 0x0a2cu, 0);
    } else {
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);          /* FA1F */
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        StoreAAbsolute8(memory, cpu, 0x4202u, 0);
        LoadX16(cpu, 0x0001u);
        for (;;) {
            SetAccumulatorWidth(cpu, 0);                       /* FA8F */
            LoadADirect16(memory, cpu, 0x58u);
            cpu->carry = 0;
            Add16Value(cpu, Read16Direct(memory, cpu, 0x63u));
            StoreADirect16(memory, cpu, 0x63u);
            LoadADirect16(memory, cpu, 0x5au);
            Add16Value(cpu, Read16Direct(memory, cpu, 0x65u));
            StoreADirect16(memory, cpu, 0x65u);
            SetAccumulatorWidth(cpu, 1);
            Compare16(cpu, cpu->x,
                Read16AbsoluteIndexed(memory, cpu, PARTY_LEVEL, 0));
            if (cpu->zero)
                break;
            TransferXToA(cpu);                                 /* FA2B */
            LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
            And8(cpu, 0x07u);
            if (cpu->zero) {
                SetAccumulatorWidth(cpu, 0);                   /* next factor */
                LoadA16(cpu, cpu->y);
                cpu->carry = 0;
                Add16Value(cpu, 0x0008u);
                TransferAToY(cpu);
                SetAccumulatorWidth(cpu, 1);
                LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
                LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
                StoreAAbsolute8(memory, cpu, 0x4202u, 0);
            }
            PartyGrowStep(memory, cpu);
            IncrementX16(cpu);
        }
        SetAccumulatorWidth(cpu, 0);                           /* FAA5 */
        LoadADirect16(memory, cpu, 0x64u);
        Subtract16(cpu, 0x000au);
        StoreAAbsolute16(memory, cpu, PARTY_EXPERIENCE, 0);
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, DirectByte(memory, cpu, 0x66u));
        Sbc8(cpu, 0x00u);
        StoreAAbsolute8(memory, cpu, 0x0a2cu, 0);
    }
    cpu->x = PullIndexValue(memory, cpu);                      /* FAC6 */
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81fac8u);                       /* RTL */
}
