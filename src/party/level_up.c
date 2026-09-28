/* Level-up check and new character records ($81:F979, $81:FC0B). */

#include "core/cpu_internal.h"
#include "lufia2/party.h"

static void ExperienceForLevel(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t ret) {
    SimulateJslFrame(memory, cpu, 0x81u, ret);
    (void)Lufia2PartyExperienceForLevel(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* A = record byte X + offset compared with $0A2A + index. */
static void CompareExp(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t field, uint16_t with) {
    LoadAAbsolute8(memory, cpu, field, cpu->x);
    Compare8(cpu, A8(cpu), AbsoluteByte(memory, cpu, with, 0));
}

/* $81:F979: level-up of member $09FA. */
Lufia2ExecutionResult Lufia2PartyLevelUpCheck(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    unsigned i;

    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x09fau, 0));
    SimulateJsrFrame(memory, cpu, 0xf97eu);
    (void)Lufia2BattleTable9EBA(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    LoadAAbsolute8(memory, cpu, 0x000eu, cpu->x);
    StoreAAbsolute8(memory, cpu, 0x09feu, 0);
    StoreZeroAbsolute8(memory, cpu, 0x09ffu, 0);
    ExperienceForLevel(memory, cpu, 0xf98bu);
    for (i = 0; i < 3u; ++i) {                                 /* next level */
        CompareExp(memory, cpu, (uint16_t)(0x0062u + i), (uint16_t)(0x0a2au + i));
        if (!cpu->zero) {
            LoadA8(cpu, 0x02u);
            return ExecutionReturned(0x81f9e8u);
        }
    }
    for (i = 0; i < 3u; ++i) {                                 /* current, high first */
        CompareExp(memory, cpu, (uint16_t)(0x0061u - i), (uint16_t)(0x0a2cu - i));
        if (!cpu->zero)
            break;
    }
    if (cpu->carry) {
        LoadAAbsolute8(memory, cpu, 0x000eu, cpu->x);
        Compare8(cpu, A8(cpu), 0x63u);
        if (!cpu->zero) {
            LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
            StoreAAbsolute8(memory, cpu, 0x000eu, cpu->x);
            StoreAAbsolute8(memory, cpu, 0x09feu, 0);
            ExperienceForLevel(memory, cpu, 0xf9cdu);
            for (i = 0; i < 3u; ++i) {
                LoadAAbsolute8(memory, cpu, (uint16_t)(0x0a2au + i), 0);
                StoreAAbsolute8(memory, cpu, (uint16_t)(0x0062u + i), cpu->x);
            }
            LoadA8(cpu, 0x01u);
            return ExecutionReturned(0x81f9e2u);
        }
    }
    LoadA8(cpu, 0x00u);
    return ExecutionReturned(0x81f9e5u);
}

static void Put8(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Write8(memory, DirectLongIndirectY(memory, cpu, 0xb2u), A8(cpu));
}

static void Put16(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const uint32_t at = DirectLongIndirectY(memory, cpu, 0xb2u);

    Write8(memory, at, (uint8_t)cpu->accumulator);
    Write8(memory, (at + 1u) & 0x00ffffffu, (uint8_t)(cpu->accumulator >> 8));
}

static uint16_t Get16(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const uint32_t at = DirectLongIndirectY(memory, cpu, 0xb2u);

    return (uint16_t)(Read8(memory, at) |
        (Read8(memory, (at + 1u) & 0x00ffffffu) << 8));
}

/* Copy words from $96:X to [$B2] + Y until Y reaches end. */
static void CopyWords(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t y, uint16_t end, int bytes_doubled) {
    LoadY16(cpu, y);
    do {
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x));
        if (bytes_doubled) {
            And16(cpu, 0x00ffu);
            AslA16(cpu);
        }
        Put16(memory, cpu);
        IncrementX16(cpu);
        if (!bytes_doubled)
            IncrementX16(cpu);
        IncrementY16(cpu);
        IncrementY16(cpu);
        Compare16(cpu, cpu->y, end);
    } while (!cpu->zero);
}

/* $81:FC0B: new record at $7E:[$B2] for character $09F2, then stats. */
Lufia2ExecutionResult Lufia2PartyNewRecord(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const uint8_t kCopies[4][2] = {
        {0x25u, 0x11u}, {0x25u, 0x51u}, {0x27u, 0x13u}, {0x27u, 0x53u}};
    unsigned i;

    LoadA8(cpu, 0x7eu);
    StoreADirect8(memory, cpu, 0xb4u);
    LoadY16(cpu, 0x00bdu);
    TransferDirectToA(cpu);
    do {
        Put8(memory, cpu);
        LoadY16(cpu, (uint16_t)(cpu->y - 1u));
    } while (!cpu->negative);
    PushDataBank(memory, cpu);
    LoadA8(cpu, 0x96u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    LoadY16(cpu, 0x0047u);
    Put8(memory, cpu);
    LoadAAbsolute8(memory, cpu, 0x09f2u, 0);
    LoadY16(cpu, 0x0050u);
    Put8(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToY(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x85c0u, cpu->y));
    cpu->carry = 0;
    Add16Value(cpu, 0x85c0u);
    TransferAToX(cpu);
    LoadY16(cpu, 0x0045u);
    Put16(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadY16(cpu, 0x0000u);                                     /* name */
    do {
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->x);
        Put8(memory, cpu);
        IncrementY16(cpu);
        IncrementX16(cpu);
        Compare16(cpu, cpu->y, 0x000du);
    } while (!cpu->zero);
    IncrementX16(cpu);
    LoadAAbsolute8(memory, cpu, 0x0000u, cpu->x);
    LoadY16(cpu, 0x000eu);
    Put8(memory, cpu);
    IncrementX16(cpu);
    IncrementX16(cpu);
    IncrementX16(cpu);
    SetAccumulatorWidth(cpu, 0);
    CopyWords(memory, cpu, 0x0025u, 0x0029u, 0);
    CopyWords(memory, cpu, 0x0078u, 0x007cu, 0);
    IncrementY16(cpu);
    IncrementY16(cpu);
    CopyWords(memory, cpu, cpu->y, 0x0086u, 1);
    SetAccumulatorWidth(cpu, 1);
    for (i = 0; i < 4u; ++i)
        IncrementX16(cpu);
    for (;;) {                                                 /* FCB2 */
        TransferDirectToA(cpu);
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->x);
        if (cpu->zero)
            break;
        IncrementX16(cpu);
        Compare8(cpu, A8(cpu), 0x06u);
        if (cpu->carry) {
            Sbc8(cpu, 0x06u);
            AslA8(cpu);
            SetAccumulatorWidth(cpu, 0);
            Add16Value(cpu, 0x0048u);
            TransferAToY(cpu);
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x));
            Put16(memory, cpu);
            SetAccumulatorWidth(cpu, 1);
        }
        IncrementX16(cpu);
        IncrementX16(cpu);
    }
    SetAccumulatorWidth(cpu, 0);
    for (i = 0; i < 4u; i += 2u) {
        LoadY16(cpu, kCopies[i][0]);
        LoadA16(cpu, Get16(memory, cpu));
        LoadY16(cpu, kCopies[i][1]);
        Put16(memory, cpu);
        LoadY16(cpu, kCopies[i + 1u][1]);
        Put16(memory, cpu);
    }
    LoadX16(cpu, Read16Direct(memory, cpu, 0xb2u));
    SimulateJslFrame(memory, cpu, 0x81u, 0xfcddu);
    (void)Lufia2PartyDerivedStats(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81fce1u);
}
