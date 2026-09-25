/* Actor positions, map cells and collision. */

#include "core/cpu_internal.h"
#include "actor/actor_internal.h"

static void PrimaryMapCoordinateToCellOffset(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu);

/* $83:F9AD / $83:F9B6: X = $8F + $91 * width. */
void Lufia2MapCellIndex(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address,
    uint8_t from_probe) {
    SimulateJsrFrame(memory, cpu, return_address);
    if (from_probe) {
        Write8(memory, DirectAddress(cpu, 0x90u), 0x00u);      /* F9AD */
        Write8(memory, DirectAddress(cpu, 0x92u), 0x00u);
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x8fu)));
        ExchangeAccumulatorBytes(cpu);
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x91u)));
    }
    Write8(memory, 0x004202u, A8(cpu));                        /* F9B6 */
    LoadA8(cpu, Read8(memory, 0x0005b9u));
    Write8(memory, 0x004203u, A8(cpu));
    LoadA8(cpu, 0x00u);
    ExchangeAccumulatorBytes(cpu);
    SetAccumulatorWidth(cpu, 0);
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, 0x004216u));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    SimulateRtsFrame(memory, cpu);
}

/* $83:F988: height bits 7-6 of the map cell at $8F/$91. */
void Lufia2MapTileHeight(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    SimulateJsrFrame(memory, cpu, 0xf98au);                    /* F988 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x8fu)));     /* F9F2 */
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x91u)));
    PrimaryMapCoordinateToCellOffset(memory, cpu);             /* F9F7 */
    SimulateRtsFrame(memory, cpu);
    SetAccumulatorWidth(cpu, 0);                               /* F98B */
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x05aau, 0));
    cpu->carry = 0;
    Add16Value(
        cpu, Read16Long(memory, LongIndexedAddress(0x7fd008u, cpu->x)));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7f0001u, cpu->x)));
    AslA8(cpu);                                                /* F99C */
    Adc8(cpu, 0x00u);
    AslA8(cpu);
    Adc8(cpu, 0x00u);
    And8(cpu, 0x03u);                                          /* F9A2 */
    SimulateRtsFrame(memory, cpu);
}

/* Collision byte test: A = mask & $7E:4000+offset,X. */
static void PrimaryCollisionTest(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t mask,
    uint8_t second_column) {
    LoadA8(cpu, mask);
    And8(
        cpu, Read8(
            memory,
            LongIndexedAddress(
                second_column ? 0x7e4001u : 0x7e4000u, cpu->x)));
}

/* $9A >= 2 means a two-column actor. */
static uint8_t PrimaryWideActor(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x9au)));
    Compare8(cpu, A8(cpu), 0x02u);
    return cpu->carry;
}

/* $83:D89E body: Z clear = blocked, 0 = unknown target. */
uint8_t Lufia2ActorStepBlockedBody(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    uint16_t target;

    TransferAToX(cpu);                                         /* D89E */
    target = Read16ProgramIndexed(memory, cpu, 0xd8a3u, cpu->x);
    SimulateJsrFrame(memory, cpu, 0xd8a1u);                    /* D89F */

    switch (target) {
    case 0xd8abu:
        IncrementDirect8(memory, cpu, 0x91u);                  /* D8AB */
        Lufia2MapCellIndex(memory, cpu, 0xd8afu, 1);
        PrimaryCollisionTest(memory, cpu, 0x9bu, 0);
        if (!cpu->zero)
            break;
        if (!PrimaryWideActor(memory, cpu)) {
            TransferDirectToA(cpu);                            /* D8BE */
            break;
        }
        PrimaryCollisionTest(memory, cpu, 0x9bu, 1);           /* D8C0 */
        break;

    case 0xd8c7u:
        Lufia2MapCellIndex(memory, cpu, 0xd8c9u, 1);        /* D8C7 */
        PrimaryCollisionTest(memory, cpu, 0x20u, 0);
        if (!cpu->zero)
            break;
        DecrementDirect8(memory, cpu, 0x8fu);                  /* D8D2 */
        Lufia2MapCellIndex(memory, cpu, 0xd8d6u, 1);
        PrimaryCollisionTest(memory, cpu, 0x8bu, 0);
        break;

    case 0xd8deu:
        Lufia2MapCellIndex(memory, cpu, 0xd8e0u, 1);        /* D8DE */
        PrimaryCollisionTest(memory, cpu, 0x10u, 0);
        if (!cpu->zero)
            break;
        if (PrimaryWideActor(memory, cpu)) {
            PrimaryCollisionTest(memory, cpu, 0x10u, 1);       /* D8EF */
            if (!cpu->zero)
                break;
        }
        DecrementDirect8(memory, cpu, 0x91u);                  /* D8F7 */
        Lufia2MapCellIndex(memory, cpu, 0xd8fbu, 1);
        PrimaryCollisionTest(memory, cpu, 0x8bu, 0);
        if (!cpu->zero)
            break;
        if (!PrimaryWideActor(memory, cpu)) {
            TransferDirectToA(cpu);                            /* D90A */
            break;
        }
        PrimaryCollisionTest(memory, cpu, 0x8bu, 1);           /* D90C */
        break;

    case 0xd913u:
        if (PrimaryWideActor(memory, cpu))                     /* D913 */
            IncrementDirect8(memory, cpu, 0x8fu);
        IncrementDirect8(memory, cpu, 0x8fu);                  /* D91B */
        Lufia2MapCellIndex(memory, cpu, 0xd91fu, 1);
        PrimaryCollisionTest(memory, cpu, 0xabu, 0);
        break;

    default:
        cpu->resume_pc = 0x830000u | target;
        return 0;
    }

    SimulateRtsFrame(memory, cpu);
    return 1;
}

void Lufia2ActorMarkMapOccupancy(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadXDirect(memory, cpu, 0xa7u);                           /* FA3F */
    Write8(memory, DirectAddress(cpu, 0x9eu), 0x00u);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
    Compare8(cpu, A8(cpu), 0x02u);                             /* FA47 */
    if (cpu->carry) {
        LoadAAbsolute8(memory, cpu, 0x05d2u, cpu->x);          /* FA4B */
        Compare8(cpu, A8(cpu), 0x71u);
        if (!cpu->zero) {
            Compare8(cpu, A8(cpu), 0x72u);
            if (!cpu->zero) {
                Compare8(cpu, A8(cpu), 0x73u);
                if (!cpu->zero) {
                    LoadA8(cpu, 0xffu);                        /* FA5A */
                    Write8(memory, DirectAddress(cpu, 0x9eu), A8(cpu));
                }
            }
        }
    }
    LoadAAbsolute8(memory, cpu, 0x06bau, cpu->x);              /* FA5E */
    ExchangeAccumulatorBytes(cpu);
    LoadAAbsolute8(memory, cpu, 0x06e2u, cpu->x);
    Lufia2MapCellIndex(memory, cpu, 0xfa67u, 0);            /* FA65 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7e4000u, cpu->x)));
    Or8(cpu, 0x01u);
    Write8(memory, LongIndexedAddress(0x7e4000u, cpu->x), A8(cpu));
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x9eu)));     /* FA72 */
    if (!cpu->zero) {
        LoadA8(
            cpu, Read8(memory, LongIndexedAddress(0x7e4001u, cpu->x)));
        Or8(cpu, 0x01u);
        Write8(memory, LongIndexedAddress(0x7e4001u, cpu->x), A8(cpu));
    }
}

void Lufia2ActorSyncFinePosition(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadXDirect(memory, cpu, 0xa7u);                           /* A746 */
    TransferDirectToA(cpu);
    LoadAAbsolute8(memory, cpu, 0x06bau, cpu->x);
    SetAccumulatorWidth(cpu, 0);                               /* A74C */
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    PushAccumulator16(memory, cpu);                            /* A752 */
    SetAccumulatorWidth(cpu, 1);
    TransferDirectToA(cpu);                                    /* A755 */
    LoadAAbsolute8(memory, cpu, 0x06e2u, cpu->x);
    SetAccumulatorWidth(cpu, 0);                               /* A759 */
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    LoadXDirect(memory, cpu, 0xa9u);                           /* A75F */
    Write16Long(
        memory, LongIndexedAddress(0x7fde3eu, cpu->x), cpu->accumulator);
    PullAccumulator16(memory, cpu);                            /* A765 */
    Write16Long(
        memory, LongIndexedAddress(0x7fddaeu, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);                               /* A76A */
}

/* $83:FAFA: sign-extend A.low, leave M=0. */
void Lufia2SignExtendA8(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Or8(cpu, 0x00u);                                           /* FAFA */
    if (cpu->negative) {
        ExchangeAccumulatorBytes(cpu);                         /* FAFE */
        LoadA8(cpu, 0xffu);
        ExchangeAccumulatorBytes(cpu);
    }
    SetAccumulatorWidth(cpu, 0);                               /* FB02 */
    SimulateRtsFrame(memory, cpu);
}

/* Signed operand bytes added to a 16-bit X/Y pair. */
void Lufia2ActorAddSignedPair(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint32_t pair,
    uint16_t operand,
    uint16_t return_address) {
    TransferDirectToA(cpu);
    LoadAAbsolute8(memory, cpu, operand, cpu->y);
    Lufia2SignExtendA8(memory, cpu, return_address);
    cpu->carry = 0;
    Add16Value(
        cpu, Read16Long(memory, LongIndexedAddress(pair, cpu->x)));
    Write16Long(memory, LongIndexedAddress(pair, cpu->x), cpu->accumulator);
}

void Lufia2ActorAddDisplayOffset(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadXDirect(memory, cpu, 0xa9u);                           /* FACB */
    Lufia2ActorAddSignedPair(memory, cpu, 0x7fdc8cu, 0x0001u, 0xfad3u);
    SetAccumulatorWidth(cpu, 1);                               /* FADD */
    Lufia2ActorAddSignedPair(memory, cpu, 0x7fdd1cu, 0x0002u, 0xfae5u);
    LoadA16(cpu, cpu->y);                                      /* FAEF */
    IncrementA16(cpu);
    IncrementA16(cpu);
    IncrementA16(cpu);
}

/* ADC #0 after LSR x4 rounds the 1/16 position. */
static void PrimaryFineToTile(Lufia2ActorFrontendCpu *cpu) {
    LsrA16(cpu);
    LsrA16(cpu);
    LsrA16(cpu);
    LsrA16(cpu);
    Add16Value(cpu, 0x0000u);
    SetAccumulatorWidth(cpu, 1);
}

void Lufia2ActorMoveFinePosition(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadYDirect16(memory, cpu, 0x2au);                         /* FA81 */
    TransferDirectToA(cpu);
    LoadAAbsolute8(memory, cpu, 0x0001u, cpu->y);
    Lufia2SignExtendA8(memory, cpu, 0xfa89u);
    LoadXDirect(memory, cpu, 0xa9u);                           /* FA8A */
    cpu->carry = 0;
    Add16Value(
        cpu, Read16Long(memory, LongIndexedAddress(0x7fddaeu, cpu->x)));
    Write16Long(
        memory, LongIndexedAddress(0x7fddaeu, cpu->x), cpu->accumulator);
    PrimaryFineToTile(cpu);                                    /* FA95 */
    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
    Lufia2ActorAddSignedPair(memory, cpu, 0x7fde3eu, 0x0002u, 0xfaa6u);
    PrimaryFineToTile(cpu);                                    /* FAB0 */
    LoadYDirect16(memory, cpu, 0xa7u);                         /* FAB9 */
    StoreAAbsolute8(memory, cpu, 0x06e2u, cpu->y);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    StoreAAbsolute8(memory, cpu, 0x06bau, cpu->y);
    SetAccumulatorWidth(cpu, 0);                               /* FAC3 */
    LoadA16(cpu, Read16Direct(memory, cpu, 0x2au));
    IncrementA16(cpu);
    IncrementA16(cpu);
    IncrementA16(cpu);
}



static void PrimaryMapCoordinateToCellOffset(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    Write8(memory, 0x004202u, A8(cpu));                  /* $83:F9F7 */
    LoadA8(cpu, Read8(memory, 0x0005b9u));              /* $83:F9FB */
    Write8(memory, 0x004203u, A8(cpu));                 /* $83:F9FF */
    LoadA8(cpu, 0x00u);                                 /* $83:FA03 */
    ExchangeAccumulatorBytes(cpu);                      /* $83:FA05 */
    SetAccumulatorWidth(cpu, 0);                        /* $83:FA06 */
    cpu->carry = 0;                                     /* $83:FA08 */
    Add16Value(cpu, Read16Long(memory, 0x004216u));     /* $83:FA09 */
    AslA16(cpu);                                        /* $83:FA0D */
    TransferAToX(cpu);                                  /* $83:FA0E */
    SetAccumulatorWidth(cpu, 1);                        /* $83:FA0F */
}

uint32_t Lufia2ActorMovementStep(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    uint16_t target;
    uint32_t address;
    uint8_t value;

    ExchangeAccumulatorBytes(cpu);                      /* $83:FB12 */
    LoadA8(cpu, 0x00u);                                 /* $83:FB13 */
    ExchangeAccumulatorBytes(cpu);                      /* $83:FB15 */
    TransferAToX(cpu);                                  /* $83:FB16 */
    target = Read16ProgramIndexed(memory, cpu, 0xfb1au, cpu->x);
                                                               /* $83:FB17 */

    switch (target) {
    case 0xfb22u:
        address = DirectAddress(cpu, 0x91u);
        value = (uint8_t)(Read8(memory, address) + 1u);
        Write8(memory, address, value);
        SetNz8(cpu, value);
        return 0x83fb24u;
    case 0xfb25u:
        address = DirectAddress(cpu, 0x8fu);
        value = (uint8_t)(Read8(memory, address) - 1u);
        Write8(memory, address, value);
        SetNz8(cpu, value);
        return 0x83fb27u;
    case 0xfb28u:
        address = DirectAddress(cpu, 0x91u);
        value = (uint8_t)(Read8(memory, address) - 1u);
        Write8(memory, address, value);
        SetNz8(cpu, value);
        return 0x83fb2au;
    case 0xfb2bu:
        address = DirectAddress(cpu, 0x8fu);
        value = (uint8_t)(Read8(memory, address) + 1u);
        Write8(memory, address, value);
        SetNz8(cpu, value);
        return 0x83fb2du;
    default:
        cpu->resume_pc = 0x83fb17u;
        return 0;
    }
}

void Lufia2ActorResolveMapCellOffset(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x8fu)));     /* F9D4 */
    ExchangeAccumulatorBytes(cpu);                             /* F9D6 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x91u)));     /* F9D7 */

    SimulateJsrFrame(memory, cpu, 0xf9dbu);                    /* F9D9 */
    PrimaryMapCoordinateToCellOffset(memory, cpu);             /* F9F7 */
    SimulateRtsFrame(memory, cpu);

    SetAccumulatorWidth(cpu, 0);                               /* F9DC */
    PushIndex(memory, cpu);                                    /* F9DE */
    LoadA16(cpu, Read16Long(memory, 0x0005aau));               /* F9DF */
    TransferAToX(cpu);                                         /* F9E3 */
    PullAccumulator16(memory, cpu);                            /* F9E4 */
    cpu->carry = 0;                                            /* F9E5 */
    Add16Value(
        cpu, Read16Long(
            memory, LongIndexedAddress(0x7fd008u, cpu->x)));   /* F9E6 */
    TransferAToX(cpu);                                         /* F9EA */
    SetAccumulatorWidth(cpu, 1);                               /* F9EB */
}

void Lufia2ActorReadMapCellValue(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0xfb73u);                    /* FB71 */
    Lufia2ActorResolveMapCellOffset(memory, cpu);              /* F9D4 */
    SimulateRtsFrame(memory, cpu);

    SetAccumulatorWidth(cpu, 0);                               /* FB74 */
    LoadA16(
        cpu, Read16Long(
            memory, LongIndexedAddress(0x7f0000u, cpu->x)));   /* FB76 */
    And16(cpu, 0x03ffu);                                       /* FB7A */
    cpu->carry = 0;                                            /* FB7D */
    Add16Value(cpu, Read16Long(memory, 0x7fd03eu));            /* FB7E */
    TransferAToX(cpu);                                         /* FB82 */
    SetAccumulatorWidth(cpu, 1);                               /* FB83 */
    TransferDirectToA(cpu);                                    /* FB85 */
    LoadA8(
        cpu, Read8(
            memory, LongIndexedAddress(0x7f0000u, cpu->x)));   /* FB86 */
}

static void ClearCellBit0(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint32_t base) {
    const uint32_t address = LongIndexedAddress(base, cpu->x);
    LoadA8(cpu, Read8(memory, address));
    And8(cpu, 0xfeu);
    Write8(memory, address, A8(cpu));
}

/* $83:FA12: clear occupancy bit 0 under the actor. */
void Lufia2ActorClearMapOccupancy(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadXDirect(memory, cpu, 0xa7u);                           /* FA12 */
    LoadAAbsolute8(memory, cpu, 0x06bau, cpu->x);
    ExchangeAccumulatorBytes(cpu);
    LoadAAbsolute8(memory, cpu, 0x06e2u, cpu->x);
    Lufia2MapCellIndex(memory, cpu, 0xfa1du, 0);            /* F9B6 */
    ClearCellBit0(memory, cpu, 0x7e4000u);                     /* FA1E */
    PushIndex(memory, cpu);
    LoadXDirect(memory, cpu, 0xa7u);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
    Compare8(cpu, A8(cpu), 0x02u);
    cpu->x = PullIndexValue(memory, cpu);                      /* FA31 */
    if (cpu->carry)
        ClearCellBit0(memory, cpu, 0x7e4001u);                 /* FA34 */
}
