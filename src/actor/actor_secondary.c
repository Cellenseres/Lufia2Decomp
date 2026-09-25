/* Secondary actor script VM ($83:D508). */

#include "core/cpu_internal.h"
#include "lufia2/actor.h"
#include "lufia2/system.h"
#include "actor/actor_internal.h"
#include "system/system_internal.h"

Lufia2ActorScriptDispatchResult Lufia2ActorSecondaryScriptDispatch(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ActorScriptDispatchResult result;
    uint16_t actor_record;
    uint32_t first_handler;

    TransferXToA(cpu);
    if (cpu->zero) {
        LoadA8(cpu, 0xc0u);
        And8(cpu, Read8(memory, DirectAddress(cpu, 0x47u)));
        if (!cpu->zero) {
            TrbDirect8(memory, cpu, 0x4bu);
            if (!cpu->zero)
                Write8(memory, 0x7fd0aeu, A8(cpu));
        }
    }

    SetIndexWidth(cpu, 0);
    PushDataBank(memory, cpu);
    LoadXDirect16(memory, cpu, 0xabu);
    actor_record = cpu->x;
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe3f0u, actor_record)));
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fe3eeu, actor_record)));
    Write16Direct(memory, cpu, 0x2au, cpu->accumulator);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadXDirect16(memory, cpu, 0x2au);
    StoreXDirect16(memory, cpu, 0x2au);
    LoadYDirect16(memory, cpu, 0xa7u);
    TransferDirectToA(cpu);
    result.opcode = LoadScriptByteX(memory, cpu);
    And8(cpu, 0xf0u);
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    TransferAToX(cpu);
    first_handler = JumpProgramTable(memory, cpu, 0xdf17u);

    if (first_handler == (((uint32_t)cpu->program_bank << 16) | 0xd5d4u) ||
        first_handler == (((uint32_t)cpu->program_bank << 16) | 0xd5e0u) ||
        first_handler == (((uint32_t)cpu->program_bank << 16) | 0xd5ecu)) {
        uint16_t table;

        LoadXDirect16(memory, cpu, 0x2au);
        (void)LoadScriptByteX(memory, cpu);
        And8(cpu, 0x0fu);
        AslA8(cpu);
        TransferAToX(cpu);

        if ((uint16_t)first_handler == 0xd5d4u)
            table = 0xdf37u;
        else if ((uint16_t)first_handler == 0xd5e0u)
            table = 0xdf57u;
        else
            table = 0xdf77u;
        result.handler_pc = JumpProgramTable(memory, cpu, table);
    } else {
        result.handler_pc = first_handler;
    }

    return result;
}

/* $83:D5C3: STX $2A, then two-level table dispatch. */
static Lufia2ActorScriptDispatchResult SecondaryRedispatch(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ActorScriptDispatchResult result;
    uint32_t first;

    StoreXDirect16(memory, cpu, 0x2au);                        /* D5C3 */
    LoadYDirect16(memory, cpu, 0xa7u);
    TransferDirectToA(cpu);
    result.opcode = LoadScriptByteX(memory, cpu);
    And8(cpu, 0xf0u);
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    TransferAToX(cpu);
    first = JumpProgramTable(memory, cpu, 0xdf17u);            /* D5D1 */
    if (first == 0x83d5d4u || first == 0x83d5e0u || first == 0x83d5ecu) {
        const uint16_t table = first == 0x83d5d4u ? 0xdf37u :
                               first == 0x83d5e0u ? 0xdf57u : 0xdf77u;
        LoadXDirect16(memory, cpu, 0x2au);
        (void)LoadScriptByteX(memory, cpu);
        And8(cpu, 0x0fu);
        AslA8(cpu);
        TransferAToX(cpu);
        result.handler_pc = JumpProgramTable(memory, cpu, table);
    } else {
        result.handler_pc = first;
    }
    return result;
}

/* $83:DA8A: clear $0622 bit 7 and the walk counter. */
static void SecondaryStopScript(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadXDirect(memory, cpu, 0xa7u);
    LoadAAbsolute8(memory, cpu, 0x0622u, cpu->x);
    And8(cpu, 0x7fu);
    StoreAAbsolute8(memory, cpu, 0x0622u, cpu->x);
    TransferDirectToA(cpu);                                    /* DA94 */
    Write8(memory, LongIndexedAddress(0x7fe48eu, cpu->x), A8(cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $83:DA71: leader stop sets $05B5 bit 4. */
static void SecondaryLeaderStopFlag(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadYDirect16(memory, cpu, 0xa7u);                         /* DA71 */
    if (cpu->zero) {
        uint8_t mark;

        LoadA8(cpu, Read8(memory, 0x7fd0a1u));
        BitImmediate8(cpu, 0x04u);
        mark = !cpu->zero;
        if (!mark) {
            LoadAAbsolute8(memory, cpu, 0x0622u, cpu->y);      /* DA7D */
            BitImmediate8(cpu, 0x08u);
            mark = cpu->zero;
        }
        if (mark) {
            LoadA8(cpu, 0x10u);                                /* DA84 */
            TestBitsAbsolute8(memory, cpu, 0x05b5u, 1);
        }
    }
    SimulateRtsFrame(memory, cpu);
}

/* $83:DA5B: probe = actor tile moved one step by facing. */
static uint8_t SecondaryAdvanceProbe(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadXDirect(memory, cpu, 0xa7u);                           /* DA5B */
    LoadAAbsolute8(memory, cpu, 0x06bau, cpu->x);
    Write8(memory, DirectAddress(cpu, 0x8fu), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x06e2u, cpu->x);
    Write8(memory, DirectAddress(cpu, 0x91u), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x0692u, cpu->x);
    SimulateJslFrame(memory, cpu, 0x83u, 0xda6du);             /* DA6A */
    if (Lufia2ActorMovementStep(memory, cpu) == 0)
        return 0;
    SimulateRtlFrame(memory, cpu);
    LoadXDirect(memory, cpu, 0xa7u);                           /* DA6E */
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* $83:D87D: probe the step in direction A from the actor tile. */
static uint8_t SecondaryStepBlocked(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    PushAccumulator8(memory, cpu);                             /* D87D */
    LoadAAbsolute8(memory, cpu, 0x06bau, cpu->y);
    Write8(memory, DirectAddress(cpu, 0x8fu), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x06e2u, cpu->y);
    Write8(memory, DirectAddress(cpu, 0x91u), A8(cpu));
    TransferYToX(cpu);                                         /* D888 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
    Write8(memory, DirectAddress(cpu, 0x9au), A8(cpu));
    TransferDirectToA(cpu);                                    /* D88F */
    TransferXToA(cpu);
    AslA8(cpu);
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdc8cu, cpu->x)));
    if (!cpu->zero) {
        LoadA8(cpu, 0x01u);                                    /* D899 */
        Write8(memory, DirectAddress(cpu, 0x9au), A8(cpu));
    }
    LoadA8(cpu, Pull8(memory, cpu));                           /* D89D */
    if (!Lufia2ActorStepBlockedBody(memory, cpu))
        return 0;
    SimulateRtsFrame(memory, cpu);                             /* D8A2 */
    return 1;
}

/* Occupancy bit 0 at the probe tile, both columns if wide. */
static void SecondaryOccupancy(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address,
    uint8_t set) {
    Lufia2MapCellIndex(memory, cpu, return_address, 1);
    for (uint8_t column = 0; column < 2u; ++column) {
        const uint32_t cell =
            LongIndexedAddress(column ? 0x7e4001u : 0x7e4000u, cpu->x);

        if (column) {
            LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x9au)));
            Compare8(cpu, A8(cpu), 0x02u);
            if (!cpu->carry)
                break;
        }
        LoadA8(cpu, Read8(memory, cell));
        if (set)
            Or8(cpu, 0x01u);
        else
            And8(cpu, 0xfeu);
        Write8(memory, cell, A8(cpu));
    }
}

/* $83:D9C6: move occupancy and the actor one step by facing. */
static uint8_t SecondaryCommitStep(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadAAbsolute8(memory, cpu, 0x06bau, cpu->y);              /* D9C6 */
    Write8(memory, DirectAddress(cpu, 0x8fu), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x06e2u, cpu->y);
    Write8(memory, DirectAddress(cpu, 0x91u), A8(cpu));
    TransferYToX(cpu);                                         /* D9D0 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
    Write8(memory, DirectAddress(cpu, 0x9au), A8(cpu));
    TransferXToA(cpu);                                         /* D9D7 */
    if (cpu->zero) {
        LoadA8(cpu, Read8(memory, 0x7fd0feu));
        if (!cpu->zero) {
            LoadA8(cpu, 0x01u);                                /* D9E0 */
            Write8(memory, DirectAddress(cpu, 0x9au), A8(cpu));
        }
    }
    LoadAAbsolute8(memory, cpu, 0x0622u, cpu->y);              /* D9E4 */
    BitImmediate8(cpu, 0x0au);
    if (cpu->zero) {
        SecondaryOccupancy(memory, cpu, 0xd9edu, 0);           /* D9EB */
        if (!SecondaryAdvanceProbe(memory, cpu, 0xda0au))      /* DA08 */
            return 0;
        SecondaryOccupancy(memory, cpu, 0xda0du, 1);           /* DA0B */
    }
    LoadAAbsolute8(memory, cpu, 0x070au, cpu->y);              /* DA28 */
    Compare8(cpu, A8(cpu), 0x03u);
    if (cpu->zero) {
        LoadAAbsolute8(memory, cpu, 0x09a6u, 0);               /* DA2F */
        BitImmediate8(cpu, 0x01u);
        if (cpu->zero) {
            /* Shift the follower trail in $09A1.. by one. */
            LoadX16(cpu, 0x0003u);                             /* DA36 */
            do {
                LoadAAbsolute8(memory, cpu, 0x09a1u, cpu->x);
                StoreAAbsolute8(memory, cpu, 0x09a2u, cpu->x);
                LoadX16(cpu, (uint16_t)(cpu->x - 1u));
            } while (!cpu->negative);
            LoadXDirect(memory, cpu, 0xa7u);                   /* DA42 */
            LoadA8(
                cpu, Read8(memory, LongIndexedAddress(0x7fe466u, cpu->x)));
            Or8(cpu, 0x80u);
            StoreAAbsolute8(memory, cpu, 0x09a1u, 0);
        }
    }
    if (!SecondaryAdvanceProbe(memory, cpu, 0xda4fu))          /* DA4D */
        return 0;
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x8fu)));     /* DA50 */
    StoreAAbsolute8(memory, cpu, 0x06bau, cpu->x);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x91u)));
    StoreAAbsolute8(memory, cpu, 0x06e2u, cpu->x);
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* $83:DD18 targets: signed step along facing, M=1 on exit. */
static uint8_t SecondaryFineStep(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    uint16_t target;
    uint32_t pair;

    TransferAToX(cpu);                                         /* DCC8 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x32u)));
    SetAccumulatorWidth(cpu, 0);                               /* DCCB */
    target = Read16ProgramIndexed(memory, cpu, 0xdd18u, cpu->x);
    SimulateJsrFrame(memory, cpu, 0xdccfu);                    /* DCCD */
    switch (target) {
    case 0xdd24u: pair = 0x7fde3eu; break;
    case 0xdd20u: pair = 0x7fde3eu; break;
    case 0xdd32u: pair = 0x7fddaeu; break;
    case 0xdd36u: pair = 0x7fddaeu; break;
    default:
        cpu->resume_pc = 0x830000u | target;
        return 0;
    }
    if (target == 0xdd20u || target == 0xdd32u) {
        LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffffu));  /* DD20 */
        IncrementA16(cpu);
    }
    LoadXDirect(memory, cpu, 0xa9u);
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, LongIndexedAddress(pair, cpu->x)));
    Write16Long(memory, LongIndexedAddress(pair, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    SimulateRtsFrame(memory, cpu);
    return 1;
}

typedef enum SecondaryStepFlow {
    SECONDARY_STEP_UNKNOWN = 0,
    SECONDARY_STEP_REDISPATCHED = 1,
    SECONDARY_STEP_EXIT_D60D = 2,
} SecondaryStepFlow;

typedef struct SecondaryStep {
    SecondaryStepFlow flow;
    uint32_t handler_pc;
} SecondaryStep;

static SecondaryStep SecondaryRedispatched(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SecondaryStep step;
    step.flow = SECONDARY_STEP_REDISPATCHED;
    step.handler_pc = SecondaryRedispatch(memory, cpu).handler_pc;
    return step;
}

static SecondaryStep SecondaryExit(void) {
    SecondaryStep step;
    step.flow = SECONDARY_STEP_EXIT_D60D;
    step.handler_pc = 0x83d60du;
    return step;
}

static SecondaryStep SecondaryBoundary(const Lufia2CpuState *cpu) {
    SecondaryStep step;
    step.flow = SECONDARY_STEP_UNKNOWN;
    step.handler_pc = cpu->resume_pc;
    return step;
}

/* A16 cursor to $7F:E3EE+record, exit ($83:D605/$83:DD09). */
static SecondaryStep SecondarySaveCursorExit(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, 0xabu);
    Write16Long(
        memory, LongIndexedAddress(0x7fe3eeu, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    return SecondaryExit();
}

/* $83:DC45: walk one tile over 16 sub-steps. */
static SecondaryStep SecondaryWalk(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, 0xa7u);                           /* DC45 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe48eu, cpu->x)));
    if (cpu->zero) {
        uint8_t commit = 0;

        LoadAAbsolute8(memory, cpu, 0x0622u, cpu->x);          /* DC4D */
        BitImmediate8(cpu, 0x0au);
        if (!cpu->zero) {
            commit = 1;
        } else {
            LoadAAbsolute8(memory, cpu, 0x057cu, 0);           /* DC54 */
            if (!cpu->zero) {
                LoadA8(cpu, 0x80u);
                And8(cpu, Read8(memory, DirectAddress(cpu, 0x47u)));
                if (!cpu->zero) {
                    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0xa7u)));
                    if (cpu->zero)
                        commit = 1;                            /* DC63 */
                }
            }
            if (!commit) {
                LoadAAbsolute8(memory, cpu, 0x0692u, cpu->x);  /* DC65 */
                if (!SecondaryStepBlocked(memory, cpu, 0xdc6au))
                    return SecondaryBoundary(cpu);
                if (!cpu->zero) {
                    /* Blocked: skip the opcode and stop. */
                    SetAccumulatorWidth(cpu, 0);               /* DC6D */
                    LoadA16(cpu, Read16Direct(memory, cpu, 0x2au));
                    IncrementA16(cpu);
                    return SecondarySaveCursorExit(memory, cpu);
                }
                commit = 1;
            }
        }
        if (commit && !SecondaryCommitStep(memory, cpu, 0xdc77u))
            return SecondaryBoundary(cpu);                     /* DC75 */
    }

    LoadXDirect(memory, cpu, 0xa7u);                           /* DC78 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe48eu, cpu->x)));
    Write8(memory, DirectAddress(cpu, 0x32u), A8(cpu));
    cpu->carry = 0;
    Adc8(cpu, Read8(memory, LongIndexedAddress(0x7fe4deu, cpu->x)));
    Write8(memory, LongIndexedAddress(0x7fe48eu, cpu->x), A8(cpu));
    And8(cpu, 0xfcu);                                          /* DC89 */
    Sbc8(cpu, Read8(memory, DirectAddress(cpu, 0x32u)));
    if (cpu->negative)
        goto save_cursor;                                      /* DC8D */
    LoadA8(cpu, 0x03u);                                        /* DC8F */
    TrbDirect8(memory, cpu, 0x32u);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe48eu, cpu->x)));
    Sbc8(cpu, Read8(memory, DirectAddress(cpu, 0x32u)));
    LsrA8(cpu);
    LsrA8(cpu);
    Write8(memory, DirectAddress(cpu, 0x32u), A8(cpu));
    cpu->carry = 0;                                            /* DC9D */
    Adc8(cpu, Read8(memory, LongIndexedAddress(0x7fe4b6u, cpu->x)));
    Write8(memory, LongIndexedAddress(0x7fe4b6u, cpu->x), A8(cpu));
    Compare8(cpu, A8(cpu), 0x04u);                             /* DCA6 */
    if (cpu->zero || (Compare8(cpu, A8(cpu), 0x0cu), cpu->zero)) {
        const int delta = A8(cpu) == 0x04u ? 1 : -1;
        LoadAAbsolute8(memory, cpu, 0x0736u, cpu->x);
        BitImmediate8(cpu, 0x02u);
        if (cpu->zero)
            StepMemory8(                                       /* DCB1 */
                memory, cpu,
                AbsoluteIndexedAddress(cpu, 0x066au, cpu->x), delta);
    }
    TransferDirectToA(cpu);                                    /* DCC4 */
    LoadAAbsolute8(memory, cpu, 0x0692u, cpu->y);
    if (!SecondaryFineStep(memory, cpu))
        return SecondaryBoundary(cpu);
    LoadXDirect(memory, cpu, 0xa7u);                           /* DCD0 */
    LoadAAbsolute8(memory, cpu, 0x0736u, cpu->x);
    BitImmediate8(cpu, 0x08u);
    if (!cpu->zero) {
        /* Bob offset from $83:DD44. */
        TransferDirectToA(cpu);                                /* DCD9 */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe4b6u, cpu->x)));
        DecrementA8(cpu);
        TransferAToX(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x83dd44u, cpu->x)));
        Lufia2SignExtendA8(memory, cpu, 0xdce6u);
        LoadXDirect(memory, cpu, 0xa9u);                       /* DCE7 */
        cpu->carry = 0;
        Add16Value(
            cpu, Read16Long(memory, LongIndexedAddress(0x7fdd1cu, cpu->x)));
        Write16Long(
            memory, LongIndexedAddress(0x7fdd1cu, cpu->x),
            cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        LoadXDirect(memory, cpu, 0xa7u);                       /* DCF4 */
    }
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe4b6u, cpu->x)));
    Compare8(cpu, A8(cpu), 0x10u);                             /* DCFA */
    if (cpu->zero) {
        SetAccumulatorWidth(cpu, 0);                           /* DCFE */
        LoadA16(cpu, Read16Direct(memory, cpu, 0x2au));
        IncrementA16(cpu);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
        return SecondaryRedispatched(memory, cpu);
    }

save_cursor:
    LoadXDirect(memory, cpu, 0xabu);                           /* DD09 */
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x2au));
    Write16Long(
        memory, LongIndexedAddress(0x7fe3eeu, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    return SecondaryExit();
}

/* $83:D5F8: next byte. */
static SecondaryStep SecondaryNextByte(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t length) {
    LoadXDirect(memory, cpu, 0x2au);
    while (length--)
        IncrementX16(cpu);
    return SecondaryRedispatched(memory, cpu);
}

/* $83:D7A5: actor tile to $8F/$91. */
static void SecondaryActorToProbe(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadXDirect(memory, cpu, 0xa7u);
    LoadAAbsolute8(memory, cpu, 0x06bau, cpu->x);
    Write8(memory, DirectAddress(cpu, 0x8fu), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x06e2u, cpu->x);
    Write8(memory, DirectAddress(cpu, 0x91u), A8(cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $83:D99F: signed operands onto $7F:DC8C/DD1C; M=0 exit. */
static void SecondaryAddDisplayOffset(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadYDirect16(memory, cpu, 0x2au);
    LoadXDirect(memory, cpu, 0xa9u);
    Lufia2ActorAddSignedPair(memory, cpu, 0x7fdc8cu, 0x0001u, 0xd9a9u);
    SetAccumulatorWidth(cpu, 1);
    Lufia2ActorAddSignedPair(memory, cpu, 0x7fdd1cu, 0x0002u, 0xd9bbu);
    SimulateRtsFrame(memory, cpu);
}

/* $80:8450 sine, $80:8486 cosine; sign in bit 7. */
static void SecondaryWave(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t cosine,
    uint16_t return_address) {
    const uint8_t angle = A8(cpu);
    uint8_t index;
    uint8_t negative;

    SimulateJslFrame(memory, cpu, 0x83u, return_address);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    if (!cosine) {
        if (angle < 0x2du) {
            index = angle;
            negative = 0;
        } else if (angle < 0x5au) {
            index = (uint8_t)(0x5au - angle);
            negative = 0;
        } else if (angle < 0x87u) {
            index = (uint8_t)(angle - 0x5au);
            negative = 1;
        } else {
            index = (uint8_t)(0xb4u - angle);
            negative = 1;
        }
    } else {
        if (angle < 0x2du) {
            index = (uint8_t)(0x2du - angle);
            negative = 0;
        } else if (angle < 0x5au) {
            index = (uint8_t)(angle - 0x2du);
            negative = 1;
        } else if (angle < 0x87u) {
            index = (uint8_t)(0x87u - angle);
            negative = 1;
        } else {
            index = (uint8_t)(angle - 0x87u);
            negative = 0;
        }
    }
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x8084bfu, index)));
    if (negative)
        Or8(cpu, 0x80u);
    UnpackStatus(cpu, Pull8(memory, cpu));                     /* PLP */
    cpu->y = PullIndexValue(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $83:DE9F: radius times magnitude via Mode 7. */
static void SecondaryScaleRadius(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    And8(cpu, 0x7fu);                                          /* DE9F */
    StoreAAbsolute8(memory, cpu, 0x211cu, 0);
    LoadA8(cpu, 0x00u);
    ExchangeAccumulatorBytes(cpu);
    Lufia2SignExtendA8(memory, cpu, 0xdea9u);
    SetAccumulatorWidth(cpu, 1);                               /* DEAA */
    StoreAAbsolute8(memory, cpu, 0x211bu, 0);
    ExchangeAccumulatorBytes(cpu);
    StoreAAbsolute8(memory, cpu, 0x211bu, 0);
    {
        const uint32_t address = AbsoluteIndexedAddress(cpu, 0x2134u, 0);
        const uint8_t value = Read8(memory, address);          /* DEB3 */

        cpu->carry = (value & 0x80u) != 0;
        Write8(memory, address, (uint8_t)(value << 1));
        SetNz8(cpu, (uint8_t)(value << 1));
    }
    SetAccumulatorWidth(cpu, 0);                               /* DEB6 */
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x2135u, 0));
    {
        const uint16_t value = cpu->accumulator;               /* DEBB */
        const uint8_t carry = cpu->carry;

        cpu->carry = (value & 0x8000u) != 0;
        LoadA16(cpu, (uint16_t)((value << 1) | carry));
    }
    SimulateRtsFrame(memory, cpu);
}

/* $83:DE76: orbit offsets into $54/$56; M=0 exit. */
static void SecondaryOrbitOffsets(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadXDirect(memory, cpu, 0x2au);                           /* DE76 */
    LoadAAbsolute8(memory, cpu, 0x0003u, cpu->x);
    ExchangeAccumulatorBytes(cpu);
    LoadAAbsolute8(memory, cpu, 0x0001u, cpu->x);
    LsrA8(cpu);
    SecondaryWave(memory, cpu, 0, 0xde83u);
    SecondaryScaleRadius(memory, cpu, 0xde86u);
    Write16Direct(memory, cpu, 0x54u, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);                               /* DE89 */
    LoadAAbsolute8(memory, cpu, 0x0002u, cpu->x);
    ExchangeAccumulatorBytes(cpu);
    LoadAAbsolute8(memory, cpu, 0x0001u, cpu->x);
    LsrA8(cpu);
    SecondaryWave(memory, cpu, 1, 0xde96u);
    SecondaryScaleRadius(memory, cpu, 0xde99u);
    Write16Direct(memory, cpu, 0x56u, cpu->accumulator);
    LoadXDirect(memory, cpu, 0xa9u);
    SimulateRtsFrame(memory, cpu);
}

/* $83:FAF4: signed operand at X+1; M=0 exit. */
static void SecondarySignedOperand(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    SetAccumulatorWidth(cpu, 1);                               /* FAF4 */
    TransferDirectToA(cpu);
    LoadAAbsolute8(memory, cpu, 0x0001u, cpu->x);
    Or8(cpu, 0x00u);                                           /* FAFA */
    if (cpu->negative) {
        ExchangeAccumulatorBytes(cpu);
        LoadA8(cpu, 0xffu);
        ExchangeAccumulatorBytes(cpu);
    }
    SetAccumulatorWidth(cpu, 0);
    SimulateRtsFrame(memory, cpu);
}

/* A16 = $2A + length, save cursor, exit. */
static SecondaryStep SecondarySaveCursorPlus(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t length) {
    LoadA16(cpu, Read16Direct(memory, cpu, 0x2au));
    while (length--)
        IncrementA16(cpu);
    return SecondarySaveCursorExit(memory, cpu);
}

/* $83:D6A6: skip a one-byte operand. */
static void SecondarySkipOperand(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 1);
    LoadXDirect(memory, cpu, 0x2au);
    IncrementX16(cpu);
    IncrementX16(cpu);
}

/* $83:D67A: spawn child at $8F/$91 facing $94. */
static void SecondarySpawnChild(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0xa7u)));    /* D67A */
    PushAccumulator8(memory, cpu);
    LoadXDirect(memory, cpu, 0x2au);
    LoadAAbsolute8(memory, cpu, 0x0001u, cpu->x);
    SimulateJslFrame(memory, cpu, 0x83u, 0xd685u);
    Lufia2ActorSpawn(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    LoadXDirect(memory, cpu, 0xa7u);                           /* D686 */
    LoadYDirect16(memory, cpu, 0xa9u);
    LoadA8(cpu, Pull8(memory, cpu));
    Write8(memory, DirectAddress(cpu, 0xa7u), A8(cpu));
    SimulateJslFrame(memory, cpu, 0x83u, 0xd690u);
    Lufia2ActorRecordOffsets(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x94u)));    /* D691 */
    Write8(memory, LongIndexedAddress(0x7fd9ccu, cpu->x), A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    TransferYToX(cpu);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x8fu));
    Write16Long(memory, LongIndexedAddress(0x7fddfeu, cpu->x), cpu->accumulator);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x91u));
    Write16Long(memory, LongIndexedAddress(0x7fde8eu, cpu->x), cpu->accumulator);
    SecondarySkipOperand(memory, cpu);                         /* D6A6 */
}

/* $83:D661: spawn child at the actor's position. */
static void SecondarySpawnAtActor(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, 0xa9u);                           /* D661 */
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fddaeu, cpu->x)));
    Write16Direct(memory, cpu, 0x8fu, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fde3eu, cpu->x)));
    Write16Direct(memory, cpu, 0x91u, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadXDirect(memory, cpu, 0xa7u);
    LoadAAbsolute8(memory, cpu, 0x0692u, cpu->x);
    Write8(memory, DirectAddress(cpu, 0x94u), A8(cpu));
    SecondarySpawnChild(memory, cpu);
}

static SecondaryStep SecondaryExecuteHandler(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t handler_pc) {
    switch (handler_pc & 0x00ffffffu) {
    case 0x83d5fdu:
        SecondaryStopScript(memory, cpu, 0xd5ffu);             /* D5FD */
        SecondaryLeaderStopFlag(memory, cpu, 0xd602u);         /* D600 */
        return SecondaryExit();

    case 0x83d60fu:
        SecondaryStopScript(memory, cpu, 0xd611u);             /* D60F */
        return SecondaryExit();

    case 0x83d615u:
        LoadXDirect(memory, cpu, 0x2au);                       /* D615 */
        SetAccumulatorWidth(cpu, 0);
        LoadA16(
            cpu, Read16AbsoluteIndexed(memory, cpu, 0x0001u, cpu->x));
        cpu->carry = 0;
        Add16Immediate(cpu, 0x8000u);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
        return SecondaryRedispatched(memory, cpu);

    case 0x83d626u:
        LoadXDirect(memory, cpu, 0x2au);                       /* D626 */
        LoadAAbsolute8(memory, cpu, 0x066au, cpu->y);
        And8(cpu, 0x06u);
        Or8(
            cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x0001u, cpu->x)));
        StoreAAbsolute8(memory, cpu, 0x066au, cpu->y);
        IncrementX16(cpu);
        IncrementX16(cpu);
        return SecondaryRedispatched(memory, cpu);

    case 0x83d6adu:
        LoadXDirect(memory, cpu, 0xa7u);                       /* D6AD */
        TransferDirectToA(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe5ceu, cpu->x)));
        TransferAToX(cpu);
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x8fu)));
        Write8(memory, LongIndexedAddress(0x7fd69cu, cpu->x), A8(cpu));
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x91u)));
        Write8(memory, LongIndexedAddress(0x7fd6ccu, cpu->x), A8(cpu));
        LoadXDirect(memory, cpu, 0x2au);
        IncrementX16(cpu);
        return SecondaryRedispatched(memory, cpu);

    case 0x83d6c7u:
        LoadXDirect(memory, cpu, 0xa7u);                       /* D6C7 */
        TransferDirectToA(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe5a6u, cpu->x)));
        AslA8(cpu);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 0);                           /* D6D0 */
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fddaeu, cpu->x)));
        PushAccumulator16(memory, cpu);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fde3eu, cpu->x)));
        IncrementA16(cpu);
        LoadXDirect(memory, cpu, 0xa9u);
        Write16Long(
            memory, LongIndexedAddress(0x7fde3eu, cpu->x), cpu->accumulator);
        PullAccumulator16(memory, cpu);
        Write16Long(
            memory, LongIndexedAddress(0x7fddaeu, cpu->x), cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);                           /* D6E7 */
        LoadXDirect(memory, cpu, 0x2au);
        IncrementX16(cpu);
        return SecondaryRedispatched(memory, cpu);

    case 0x83d6efu:
        LoadXDirect(memory, cpu, 0x2au);                       /* D6EF */
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->x);
        LoadXDirect(memory, cpu, 0xa7u);
        LsrA8(cpu);
        LoadAAbsolute8(memory, cpu, 0x0622u, cpu->x);
        if (cpu->carry)
            And8(cpu, 0xfdu);                                  /* D6FC */
        else
            Or8(cpu, 0x02u);                                   /* D700 */
        StoreAAbsolute8(memory, cpu, 0x0622u, cpu->x);
        LoadXDirect(memory, cpu, 0x2au);                       /* D705 */
        IncrementX16(cpu);
        IncrementX16(cpu);
        return SecondaryRedispatched(memory, cpu);

    case 0x83d833u:
        LoadYDirect16(memory, cpu, 0x2au);                     /* D833 */
        LoadXDirect(memory, cpu, 0xa7u);
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->y);
        Write8(memory, LongIndexedAddress(0x7fe4deu, cpu->x), A8(cpu));
        TransferYToX(cpu);
        IncrementX16(cpu);
        IncrementX16(cpu);
        return SecondaryRedispatched(memory, cpu);

    case 0x83d844u:
        LoadXDirect(memory, cpu, 0x2au);                       /* D844 */
        LoadAAbsolute8(memory, cpu, 0x0736u, cpu->y);
        BitImmediate8(cpu, 0x02u);
        if (cpu->zero) {
            LoadAAbsolute8(memory, cpu, 0x0001u, cpu->x);      /* D84D */
            StoreAAbsolute8(memory, cpu, 0x066au, cpu->y);
        }
        IncrementX16(cpu);
        IncrementX16(cpu);
        return SecondaryRedispatched(memory, cpu);

    case 0x83da9au:
        LoadYDirect16(memory, cpu, 0x2au);                     /* DA9A */
        LoadXDirect(memory, cpu, 0xa7u);
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->y);
        StoreAAbsolute8(memory, cpu, 0x0692u, cpu->x);
        TransferDirectToA(cpu);                                /* DAA4 */
        Write8(memory, LongIndexedAddress(0x7fe48eu, cpu->x), A8(cpu));
        Write8(memory, LongIndexedAddress(0x7fe4b6u, cpu->x), A8(cpu));
        TransferYToX(cpu);
        IncrementX16(cpu);
        IncrementX16(cpu);
        return SecondaryRedispatched(memory, cpu);

    case 0x83dee9u: {
        const uint32_t wait = 0x7fe48eu;

        LoadYDirect16(memory, cpu, 0x2au);                     /* DEE9 */
        LoadXDirect(memory, cpu, 0xa7u);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(wait, cpu->x)));
        if (!cpu->negative) {
            LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);      /* DEF3 */
            And8(cpu, 0x0fu);
            Or8(cpu, 0x80u);
            Write8(memory, LongIndexedAddress(wait, cpu->x), A8(cpu));
        } else {
            DecrementA8(cpu);                                  /* DF00 */
            Write8(memory, LongIndexedAddress(wait, cpu->x), A8(cpu));
            And8(cpu, 0x0fu);
            if (cpu->zero) {
                Write8(memory, LongIndexedAddress(wait, cpu->x), A8(cpu));
                return SecondaryNextByte(memory, cpu, 1);      /* DF0D */
            }
        }
        SetAccumulatorWidth(cpu, 0);                           /* DF10 */
        LoadA16(cpu, Read16Direct(memory, cpu, 0x2au));
        return SecondarySaveCursorExit(memory, cpu);
    }

    case 0x83d7b2u:
        LoadYDirect16(memory, cpu, 0x2au);                     /* D7B2 */
        LoadXDirect(memory, cpu, 0xa9u);
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->y);
        Write8(memory, LongIndexedAddress(0x7fdb4cu, cpu->x), A8(cpu));
        SetAccumulatorWidth(cpu, 0);                           /* D7BD */
        LoadA16(cpu, cpu->y);
        IncrementA16(cpu);
        IncrementA16(cpu);
        Write16Long(
            memory, LongIndexedAddress(0x7fdb9cu, cpu->x), cpu->accumulator);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
        return SecondaryRedispatched(memory, cpu);

    case 0x83d7ccu:
        LoadXDirect(memory, cpu, 0xa9u);                       /* D7CC */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdb4cu, cpu->x)));
        DecrementA8(cpu);
        Write8(memory, LongIndexedAddress(0x7fdb4cu, cpu->x), A8(cpu));
        if (cpu->zero)
            return SecondaryNextByte(memory, cpu, 1);          /* D7D9 */
        SetAccumulatorWidth(cpu, 0);                           /* D7DF */
        LoadA16(
            cpu, Read16Long(memory, LongIndexedAddress(0x7fdb9cu, cpu->x)));
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
        return SecondaryRedispatched(memory, cpu);

    case 0x83d80eu:
        LoadXDirect(memory, cpu, 0xa9u);                       /* D80E */
        TransferDirectToA(cpu);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(
            cpu, Read16Long(memory, LongIndexedAddress(0x7fdbecu, cpu->x)));
        cpu->carry = 0;
        Add16Value(
            cpu, Read16Long(memory, LongIndexedAddress(0x7fdc3cu, cpu->x)));
        Write16Long(
            memory, LongIndexedAddress(0x7fdbecu, cpu->x), cpu->accumulator);
        LsrA16(cpu);                                           /* D820 */
        LsrA16(cpu);
        cpu->carry = 0;
        Add16Value(
            cpu, Read16Long(memory, LongIndexedAddress(0x7fdd1cu, cpu->x)));
        Write16Long(
            memory, LongIndexedAddress(0x7fdd1cu, cpu->x), cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        return SecondaryNextByte(memory, cpu, 1);

    case 0x83d969u:
        LoadYDirect16(memory, cpu, 0x2au);                     /* D969 */
        LoadXDirect(memory, cpu, 0xa9u);
        SetAccumulatorWidth(cpu, 0);
        for (uint8_t i = 0; i < 2u; ++i) {
            const uint32_t pair = i ? 0x7fdd1cu : 0x7fdc8cu;
            LoadA16(
                cpu, Read16AbsoluteIndexed(
                    memory, cpu, i ? 0x0003u : 0x0001u, cpu->y));
            cpu->carry = 0;
            Add16Value(
                cpu, Read16Long(memory, LongIndexedAddress(pair, cpu->x)));
            Write16Long(
                memory, LongIndexedAddress(pair, cpu->x), cpu->accumulator);
        }
        LoadA16(cpu, cpu->y);                                  /* D987 */
        cpu->carry = 0;
        Add16Immediate(cpu, 0x0005u);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
        return SecondaryRedispatched(memory, cpu);

    case 0x83db77u:
        LoadXDirect(memory, cpu, 0x2au);                       /* DB77 */
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->x);
        {
            const uint8_t clear = !cpu->zero;
            LoadAAbsolute8(memory, cpu, 0x0736u, cpu->y);
            if (clear)
                And8(cpu, 0xfdu);                              /* DB81 */
            else
                Or8(cpu, 0x02u);                               /* DB8B */
            StoreAAbsolute8(memory, cpu, 0x0736u, cpu->y);
        }
        IncrementX16(cpu);
        IncrementX16(cpu);
        return SecondaryRedispatched(memory, cpu);

    case 0x83dbdcu:
        LoadXDirect(memory, cpu, 0x2au);                       /* DBDC */
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->x);
        LoadXDirect(memory, cpu, 0xa7u);
        Write8(memory, LongIndexedAddress(0x7fe35eu, cpu->x), A8(cpu));
        Or8(cpu, 0x00u);
        {
            const uint8_t blink = !cpu->zero;
            LoadA8(
                cpu, Read8(memory, LongIndexedAddress(0x000736u, cpu->x)));
            if (blink)
                Or8(cpu, 0x80u);                               /* DBF7 */
            else
                And8(cpu, 0x7fu);                              /* DBEF */
            Write8(memory, LongIndexedAddress(0x000736u, cpu->x), A8(cpu));
        }
        return SecondaryNextByte(memory, cpu, 2);

    case 0x83dab3u:
        LoadXDirect(memory, cpu, 0xa7u);                       /* DAB3 */
        LoadAAbsolute8(memory, cpu, 0x066au, cpu->x);
        And8(cpu, 0x07u);
        StoreAAbsolute8(memory, cpu, 0x0692u, cpu->x);
        return SecondaryNextByte(memory, cpu, 1);

    case 0x83dac3u:
        return SecondaryNextByte(memory, cpu, 1);             /* DAC3 */

    case 0x83dac9u:
        LoadXDirect(memory, cpu, 0x2au);                       /* DAC9 */
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->x);
        Compare8(cpu, A8(cpu), 0x01u);
        LoadXDirect(memory, cpu, 0xa7u);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe316u, cpu->x)));
        if (cpu->carry)
            Or8(cpu, 0x80u);                                   /* DADC */
        else
            And8(cpu, 0x7fu);                                  /* DAD8 */
        Write8(memory, LongIndexedAddress(0x7fe316u, cpu->x), A8(cpu));
        return SecondaryNextByte(memory, cpu, 2);

    case 0x83dd54u:
        LoadXDirect(memory, cpu, 0x2au);                       /* DD54 */
        cpu->y = cpu->x;                                       /* TXY */
        SetNz16(cpu, cpu->y);
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->x);
        LoadXDirect(memory, cpu, 0xa7u);
        Write8(memory, LongIndexedAddress(0x7fe2eeu, cpu->x), A8(cpu));
        LoadAAbsolute8(memory, cpu, 0x066au, cpu->x);
        And8(cpu, 0x07u);
        StoreAAbsolute8(memory, cpu, 0x066au, cpu->x);
        TransferYToX(cpu);
        IncrementX16(cpu);
        IncrementX16(cpu);
        return SecondaryRedispatched(memory, cpu);

    case 0x83debdu:
        LoadXDirect(memory, cpu, 0xa9u);                       /* DEBD */
        SetAccumulatorWidth(cpu, 0);
        CopyLong16(memory, cpu, 0x7fddaeu, 0x7fdb4cu);
        CopyLong16(memory, cpu, 0x7fde3eu, 0x7fdb9cu);
        CopyLong16(memory, cpu, 0x7fdc8cu, 0x7fdbecu);
        CopyLong16(memory, cpu, 0x7fdd1cu, 0x7fdc3cu);
        SetAccumulatorWidth(cpu, 1);
        return SecondaryNextByte(memory, cpu, 1);

    case 0x83db54u:
    case 0x83db5au: {
        const uint8_t save = (handler_pc & 0x00ffffffu) == 0x83db54u;

        SimulateJsrFrame(memory, cpu, save ? 0xdb56u : 0xdb5cu);
        Lufia2ActorMoveFinePosition(memory, cpu);              /* FA81 */
        SimulateRtsFrame(memory, cpu);
        if (save)
            return SecondarySaveCursorExit(memory, cpu);       /* DB57 */
        TransferAToX(cpu);                                     /* DB5D */
        SetAccumulatorWidth(cpu, 1);
        return SecondaryRedispatched(memory, cpu);
    }

    case 0x83db6du:
        SimulateJslFrame(memory, cpu, 0x83u, 0xdb70u);         /* DB6D */
        Lufia2ActorMarkMapOccupancy(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        return SecondaryNextByte(memory, cpu, 1);

    case 0x83d760u:
        LoadXDirect(memory, cpu, 0x2au);                       /* D760 */
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->x);
        SimulateJslFrame(memory, cpu, 0x83u, 0xd768u);
        Lufia2QueueDeferredSound(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        IncrementX16(cpu);
        IncrementX16(cpu);
        return SecondaryRedispatched(memory, cpu);

    case 0x83d7ebu:
        LoadYDirect16(memory, cpu, 0x2au);                     /* D7EB */
        LoadXDirect(memory, cpu, 0xa9u);
        for (uint8_t i = 0; i < 2u; ++i) {
            LoadAAbsolute8(memory, cpu, i ? 0x0002u : 0x0001u, cpu->y);
            Lufia2SignExtendA8(memory, cpu, i ? 0xd800u : 0xd7f4u);
            Write16Long(
                memory, LongIndexedAddress(i ? 0x7fdc3cu : 0x7fdbecu, cpu->x),
                cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
        }
        IncrementY16(cpu);                                     /* D807 */
        IncrementY16(cpu);
        IncrementY16(cpu);
        TransferYToX(cpu);
        return SecondaryRedispatched(memory, cpu);

    case 0x83dbc2u:
        LoadXDirect(memory, cpu, 0xa9u);                       /* DBC2 */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdb4cu, cpu->x)));
        StoreAAbsolute8(memory, cpu, 0x06bau, cpu->y);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdb4du, cpu->x)));
        StoreAAbsolute8(memory, cpu, 0x06e2u, cpu->y);
        SimulateJslFrame(memory, cpu, 0x83u, 0xdbd5u);
        Lufia2ActorSyncFinePosition(memory, cpu);              /* A746 */
        SimulateRtlFrame(memory, cpu);
        return SecondaryNextByte(memory, cpu, 1);

    case 0x83dc04u:
        LoadXDirect(memory, cpu, 0x2au);                       /* DC04 */
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->x);
        {
            const uint8_t restore = !cpu->zero;
            uint32_t flags;

            LoadXDirect(memory, cpu, 0xa7u);                   /* DC0B */
            flags = LongIndexedAddress(0x7fe316u, cpu->x);
            LoadA8(cpu, Read8(memory, flags));
            if (restore) {
                And8(cpu, 0x7fu);                              /* DC11 */
                Write8(memory, flags, A8(cpu));
                LoadXDirect(memory, cpu, 0xa9u);
                LoadA8(
                    cpu, Read8(memory, LongIndexedAddress(0x7fdb4cu, cpu->x)));
                ExchangeAccumulatorBytes(cpu);
                LoadA8(
                    cpu, Read8(memory, LongIndexedAddress(0x7fdb4du, cpu->x)));
            } else {
                Or8(cpu, 0x80u);                               /* DC2A */
                Write8(memory, flags, A8(cpu));
                TransferDirectToA(cpu);
            }
        }
        LoadXDirect(memory, cpu, 0xa7u);                       /* DC31 */
        StoreAAbsolute8(memory, cpu, 0x06e2u, cpu->x);
        ExchangeAccumulatorBytes(cpu);
        StoreAAbsolute8(memory, cpu, 0x06bau, cpu->x);
        SimulateJslFrame(memory, cpu, 0x83u, 0xdc3du);
        Lufia2ActorSyncFinePosition(memory, cpu);              /* A746 */
        SimulateRtlFrame(memory, cpu);
        return SecondaryNextByte(memory, cpu, 2);

    case 0x83d78cu:
    case 0x83d79cu: {
        const uint8_t step = (handler_pc & 0x00ffffffu) == 0x83d78cu;

        SecondaryActorToProbe(memory, cpu, step ? 0xd78eu : 0xd79eu);
        if (step) {
            LoadAAbsolute8(memory, cpu, 0x0692u, cpu->x);      /* D78F */
            SimulateJslFrame(memory, cpu, 0x83u, 0xd795u);
            if (Lufia2ActorMovementStep(memory, cpu) == 0)
                return SecondaryBoundary(cpu);
            SimulateRtlFrame(memory, cpu);
        }
        return SecondaryNextByte(memory, cpu, 1);
    }

    case 0x83d95eu:
    case 0x83d992u: {
        const uint8_t save = (handler_pc & 0x00ffffffu) == 0x83d95eu;

        SecondaryAddDisplayOffset(memory, cpu, save ? 0xd960u : 0xd994u);
        if (save) {
            LoadA16(cpu, Read16Direct(memory, cpu, 0x2au));    /* D961 */
            IncrementA16(cpu);
            IncrementA16(cpu);
            IncrementA16(cpu);
            return SecondarySaveCursorExit(memory, cpu);
        }
        SetAccumulatorWidth(cpu, 1);                           /* D995 */
        return SecondaryNextByte(memory, cpu, 3);
    }

    case 0x83db63u:
        SimulateJslFrame(memory, cpu, 0x83u, 0xdb66u);         /* DB63 */
        Lufia2ActorClearMapOccupancy(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        return SecondaryNextByte(memory, cpu, 1);

    case 0x83ddfbu:
        SimulateJslFrame(memory, cpu, 0x83u, 0xddfeu);         /* DDFB */
        Lufia2ActorClearMapOccupancy(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        LoadXDirect(memory, cpu, 0xa7u);                       /* DDFF */
        LoadAAbsolute8(memory, cpu, 0x0622u, cpu->x);
        Or8(cpu, 0x04u);
        StoreAAbsolute8(memory, cpu, 0x0622u, cpu->x);
        LoadA8(cpu, 0xffu);
        StoreAAbsolute8(memory, cpu, 0x05d2u, cpu->x);
        return SecondaryExecuteHandler(memory, cpu, 0x83d5fdu);

    case 0x83d76eu:
        LoadYDirect16(memory, cpu, 0xa7u);                     /* D76E */
        LoadAAbsolute8(memory, cpu, 0x06bau, cpu->y);
        Write8(memory, DirectAddress(cpu, 0x8fu), A8(cpu));
        LoadAAbsolute8(memory, cpu, 0x06e2u, cpu->y);
        Write8(memory, DirectAddress(cpu, 0x91u), A8(cpu));
        SimulateJslFrame(memory, cpu, 0x83u, 0xd77du);
        Lufia2ActorReadMapCellValue(memory, cpu);              /* FB71 */
        SimulateRtlFrame(memory, cpu);
        Compare8(cpu, A8(cpu), 0x09u);                         /* D77E */
        if (cpu->zero) {
            /* $80:E7DF event queue stays LLE. */
            cpu->resume_pc = 0x83d782u;
            return SecondaryBoundary(cpu);
        }
        return SecondaryNextByte(memory, cpu, 1);

    case 0x83dd6eu:
        LoadXDirect(memory, cpu, 0xa9u);                       /* DD6E */
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16Long(memory, 0x7fddaeu));
        Write16Long(
            memory, LongIndexedAddress(0x7fddaeu, cpu->x), cpu->accumulator);
        LoadA16(cpu, Read16Long(memory, 0x7fde3eu));
        Write16Long(
            memory, LongIndexedAddress(0x7fde3eu, cpu->x), cpu->accumulator);
        LoadXDirect(memory, cpu, 0x2au);                       /* DD82 */
        SecondarySignedOperand(memory, cpu, 0xdd86u);
        Write16Direct(memory, cpu, 0x54u, cpu->accumulator);
        IncrementX16(cpu);
        SecondarySignedOperand(memory, cpu, 0xdd8cu);
        IncrementX16(cpu);
        PushIndex(memory, cpu);                                /* DD8E */
        LoadXDirect(memory, cpu, 0xa9u);
        AddLong16(memory, cpu, 0x7fde3eu);
        LoadA16(cpu, Read16Direct(memory, cpu, 0x54u));        /* DD9A */
        AddLong16(memory, cpu, 0x7fddaeu);
        cpu->y = PullIndexValue(memory, cpu);                  /* DDA5 */
        LoadXDirect(memory, cpu, 0xa9u);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0001u, cpu->x));
        /* JSR $FAFA with M=0: ORA/TSB/LDA. */
        SimulateJsrFrame(memory, cpu, 0xddaeu);
        LoadA16(cpu, (uint16_t)(cpu->accumulator | 0x1000u));
        {
            const uint16_t value = Read16Direct(memory, cpu, 0xebu);

            cpu->zero = (value & cpu->accumulator) == 0;
            Write16Direct(
                memory, cpu, 0xebu, (uint16_t)(value | cpu->accumulator));
        }
        LoadA16(cpu, 0xebffu);
        SimulateRtsFrame(memory, cpu);
        Write16Long(                                           /* DDAF */
            memory, LongIndexedAddress(0x7fdc8cu, cpu->x), cpu->accumulator);
        IncrementX16(cpu);
        SetAccumulatorWidth(cpu, 1);
        TransferDirectToA(cpu);
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->x);
        Lufia2SignExtendA8(memory, cpu, 0xddbcu);
        Write16Long(
            memory, LongIndexedAddress(0x7fdd1cu, cpu->x), cpu->accumulator);
        return SecondarySaveCursorPlus(memory, cpu, 0);

    case 0x83de11u:
        SecondaryOrbitOffsets(memory, cpu, 0xde13u);           /* DE11 */
        LoadA16(cpu, Read16Direct(memory, cpu, 0x54u));
        Write16Long(
            memory, LongIndexedAddress(0x7fdd1cu, cpu->x), cpu->accumulator);
        LoadA16(cpu, Read16Direct(memory, cpu, 0x56u));
        Write16Long(
            memory, LongIndexedAddress(0x7fdc8cu, cpu->x), cpu->accumulator);
        return SecondarySaveCursorPlus(memory, cpu, 4);

    case 0x83de29u:
        SecondaryOrbitOffsets(memory, cpu, 0xde2bu);           /* DE29 */
        LoadA16(cpu, Read16Direct(memory, cpu, 0x56u));
        cpu->carry = 0;
        Add16Value(
            cpu, Read16Long(memory, LongIndexedAddress(0x7fdb9cu, cpu->x)));
        Write16Long(
            memory, LongIndexedAddress(0x7fde3eu, cpu->x), cpu->accumulator);
        LsrA16(cpu);
        LsrA16(cpu);
        LsrA16(cpu);
        LsrA16(cpu);
        Add16Immediate(cpu, 0x0000u);                          /* DE3B */
        SetAccumulatorWidth(cpu, 1);
        LoadXDirect(memory, cpu, 0xa7u);
        StoreAAbsolute8(memory, cpu, 0x06e2u, cpu->x);
        SetAccumulatorWidth(cpu, 0);                           /* DE45 */
        LoadXDirect(memory, cpu, 0xa9u);
        LoadA16(cpu, Read16Direct(memory, cpu, 0x54u));
        cpu->carry = 0;
        Add16Value(
            cpu, Read16Long(memory, LongIndexedAddress(0x7fdc3cu, cpu->x)));
        Write16Long(
            memory, LongIndexedAddress(0x7fdd1cu, cpu->x), cpu->accumulator);
        return SecondarySaveCursorPlus(memory, cpu, 4);

    case 0x83de5du:
        SecondaryOrbitOffsets(memory, cpu, 0xde5fu);           /* DE5D */
        LoadA16(cpu, Read16Direct(memory, cpu, 0x56u));
        cpu->carry = 1;
        Add16Value(cpu, (uint16_t)~Read16Direct(memory, cpu, 0x54u));
        LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffffu));
        IncrementA16(cpu);
        Write16Long(
            memory, LongIndexedAddress(0x7fdd1cu, cpu->x), cpu->accumulator);
        return SecondarySaveCursorPlus(memory, cpu, 4);

    case 0x83d70cu:
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0xa7u))); /* D70C */
        PushAccumulator8(memory, cpu);
        LoadXDirect(memory, cpu, 0x2au);
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->x);
        SimulateJslFrame(memory, cpu, 0x83u, 0xd717u);
        Lufia2ActorSpawn(memory, cpu);                      /* DF87 */
        SimulateRtlFrame(memory, cpu);
        LoadXDirect(memory, cpu, 0xa9u);                       /* D718 */
        StoreXDirect16(memory, cpu, 0x54u);
        LoadXDirect(memory, cpu, 0xa7u);
        StoreXDirect16(memory, cpu, 0x56u);
        LoadA8(cpu, Pull8(memory, cpu));
        Write8(memory, DirectAddress(cpu, 0xa7u), A8(cpu));
        SimulateJslFrame(memory, cpu, 0x83u, 0xd726u);
        Lufia2ActorRecordOffsets(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        LoadXDirect(memory, cpu, 0xa7u);                       /* D727 */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe5a6u, cpu->x)));
        ExchangeAccumulatorBytes(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe5ceu, cpu->x)));
        LoadXDirect(memory, cpu, 0x56u);
        Write8(memory, LongIndexedAddress(0x7fda4cu, cpu->x), A8(cpu));
        ExchangeAccumulatorBytes(cpu);
        Write8(memory, LongIndexedAddress(0x7fda2cu, cpu->x), A8(cpu));
        SetAccumulatorWidth(cpu, 0);                           /* D73D */
        LoadXDirect(memory, cpu, 0xa9u);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fddaeu, cpu->x)));
        Write16Direct(memory, cpu, 0x56u, cpu->accumulator);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fde3eu, cpu->x)));
        LoadXDirect(memory, cpu, 0x54u);
        Write16Long(
            memory, LongIndexedAddress(0x7fde8eu, cpu->x), cpu->accumulator);
        LoadA16(cpu, Read16Direct(memory, cpu, 0x56u));
        Write16Long(
            memory, LongIndexedAddress(0x7fddfeu, cpu->x), cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        return SecondaryNextByte(memory, cpu, 2);

    case 0x83d638u:
        SimulateJsrFrame(memory, cpu, 0xd63au);                /* D638 */
        SecondarySpawnAtActor(memory, cpu);
        SimulateRtsFrame(memory, cpu);
        return SecondaryRedispatched(memory, cpu);

    case 0x83d63eu:
        SimulateJsrFrame(memory, cpu, 0xd640u);                /* D63E */
        SecondarySpawnAtActor(memory, cpu);
        SimulateRtsFrame(memory, cpu);
        LoadXDirect(memory, cpu, 0xa7u);                       /* D641 */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
        Compare8(cpu, A8(cpu), 0x02u);
        TransferDirectToA(cpu);
        TransferYToX(cpu);
        SetAccumulatorWidth(cpu, 0);
        if (!cpu->carry)
            LoadA16(cpu, 0xfff8u);                             /* D64F */
        AddLong16(memory, cpu, 0x7fddfeu);
        SimulateJsrFrame(memory, cpu, 0xd65du);
        SecondarySkipOperand(memory, cpu);
        SimulateRtsFrame(memory, cpu);
        return SecondaryRedispatched(memory, cpu);

    case 0x83db95u: {
        unsigned i;

        SetAccumulatorWidth(cpu, 0);                           /* DB95 */
        LoadXDirect(memory, cpu, 0xa9u);
        for (i = 0; i < 2u; ++i) {
            LoadA16(cpu, Read16Long(
                memory, LongIndexedAddress(i ? 0x7fdb4du : 0x7fdb4cu, cpu->x)));
            And16(cpu, 0x00ffu);
            AslA16(cpu);
            AslA16(cpu);
            AslA16(cpu);
            AslA16(cpu);
            Write16Direct(memory, cpu, i ? 0x91u : 0x8fu, cpu->accumulator);
        }
        SetAccumulatorWidth(cpu, 1);                           /* DBB3 */
        LoadXDirect(memory, cpu, 0xa7u);
        LoadAAbsolute8(memory, cpu, 0x0692u, cpu->x);
        Write8(memory, DirectAddress(cpu, 0x94u), A8(cpu));
        SimulateJsrFrame(memory, cpu, 0xdbbeu);
        SecondarySpawnChild(memory, cpu);
        SimulateRtsFrame(memory, cpu);
        return SecondaryRedispatched(memory, cpu);
    }

    case 0x83dae9u:
        Lufia2ActorSpriteReload(memory, cpu);
        LoadXDirect(memory, cpu, 0x2au);                       /* DB34 */
        IncrementX16(cpu);
        return SecondaryRedispatched(memory, cpu);

    case 0x83dc45u:
        return SecondaryWalk(memory, cpu);

    default: {
        SecondaryStep step;
        cpu->resume_pc = handler_pc & 0x00ffffffu;
        step.flow = SECONDARY_STEP_UNKNOWN;
        step.handler_pc = cpu->resume_pc;
        return step;
    }
    }
}

Lufia2ExecutionResult Lufia2ActorSecondaryUpdate(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;
    uint32_t handler;
    uint32_t steps;

    result.flow = LUFIA2_EXECUTION_RETURNED;
    result.pc = 0x83d599u;
    result.dispatches = 0;

    if (Lufia2ActorSecondaryUpdateFrontend(memory, cpu) !=
        LUFIA2_ACTOR_SECONDARY_CONTINUE_D59A)
        return result;

    handler = Lufia2ActorSecondaryScriptDispatch(memory, cpu).handler_pc;
    result.dispatches = 1;
    /* A script that never yields spins the ROM forever. */
    for (steps = 0; steps < 0x10000u; ++steps) {
        const SecondaryStep step =
            SecondaryExecuteHandler(memory, cpu, handler);

        if (step.flow == SECONDARY_STEP_REDISPATCHED) {
            handler = step.handler_pc;
            ++result.dispatches;
            continue;
        }
        if (step.flow == SECONDARY_STEP_EXIT_D60D) {
            PullDataBank(memory, cpu);                         /* D60D */
            result.pc = 0x83d60eu;
            return result;
        }
        result.flow = LUFIA2_EXECUTION_BOUNDARY;
        result.pc = step.handler_pc;
        return result;
    }
    cpu->resume_pc = handler;
    result.flow = LUFIA2_EXECUTION_BOUNDARY;
    result.pc = handler;
    return result;
}
