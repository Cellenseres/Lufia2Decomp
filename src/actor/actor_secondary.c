/* Secondary actor script VM ($83:D508). */

#include "core/cpu_internal.h"
#include "lufia2/actor.h"
#include "lufia2/system.h"
#include "actor/actor_internal.h"
#include "system/system_internal.h"
#include "system/wram.h"

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
    LoadXDirect16(memory, cpu, DP_SLOT_RECORD_OFFSET);
    actor_record = cpu->x;
    LoadA8(cpu, Read8(memory, LongIndexedAddress((WRAM_ACTOR_SECONDARY_SCRIPT + 2u),
                                                 actor_record)));
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_ACTOR_SECONDARY_SCRIPT,
        actor_record)));
    Write16Direct(memory, cpu, 0x2au, cpu->accumulator);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadXDirect16(memory, cpu, 0x2au);
    StoreXDirect16(memory, cpu, 0x2au);
    LoadYDirect16(memory, cpu, DP_ACTOR_SLOT);
    TransferDirectToA(cpu);
    result.opcode = LoadScriptByteX(memory, cpu);
    And8(cpu, 0xf0u);
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    TransferAToX(cpu);
    first_handler = JumpProgramTable(memory, cpu, SECONDARY_NIBBLE_TABLE);

    if (first_handler ==
            (((uint32_t)cpu->program_bank << 16) | SECONDARY_GROUP_F_HANDLER) ||
        first_handler ==
            (((uint32_t)cpu->program_bank << 16) | SECONDARY_GROUP_E_HANDLER) ||
        first_handler ==
            (((uint32_t)cpu->program_bank << 16) | SECONDARY_GROUP_D_HANDLER)) {
        uint16_t table;

        LoadXDirect16(memory, cpu, 0x2au);
        (void)LoadScriptByteX(memory, cpu);
        And8(cpu, 0x0fu);
        AslA8(cpu);
        TransferAToX(cpu);

        if ((uint16_t)first_handler == SECONDARY_GROUP_F_HANDLER)
            table = SECONDARY_GROUP_F_TABLE;
        else if ((uint16_t)first_handler == SECONDARY_GROUP_E_HANDLER)
            table = SECONDARY_GROUP_E_TABLE;
        else
            table = SECONDARY_GROUP_D_TABLE;
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
    LoadYDirect16(memory, cpu, DP_ACTOR_SLOT);
    TransferDirectToA(cpu);
    result.opcode = LoadScriptByteX(memory, cpu);
    And8(cpu, 0xf0u);
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    TransferAToX(cpu);
    first = JumpProgramTable(memory, cpu, SECONDARY_NIBBLE_TABLE); /* D5D1 */
    if (first == (SECONDARY_BANK_83 | SECONDARY_GROUP_F_HANDLER) ||
        first == (SECONDARY_BANK_83 | SECONDARY_GROUP_E_HANDLER) ||
        first == (SECONDARY_BANK_83 | SECONDARY_GROUP_D_HANDLER)) {
        const uint16_t table =
            first == (SECONDARY_BANK_83 | SECONDARY_GROUP_F_HANDLER)
                ? SECONDARY_GROUP_F_TABLE
            : first == (SECONDARY_BANK_83 | SECONDARY_GROUP_E_HANDLER)
                ? SECONDARY_GROUP_E_TABLE
                : SECONDARY_GROUP_D_TABLE;
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
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->x);
    And8(cpu, 0x7fu);
    StoreAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->x);
    TransferDirectToA(cpu);                                    /* DA94 */
    Write8(memory, LongIndexedAddress(WRAM_ACTOR_WALK_COUNTER, cpu->x), A8(cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $83:DA71: leader stop sets $05B5 bit 4. */
static void SecondaryLeaderStopFlag(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadYDirect16(memory, cpu, DP_ACTOR_SLOT);                 /* DA71 */
    if (cpu->zero) {
        uint8_t mark;

        LoadA8(cpu, Read8(memory, WRAM_FIELD_CONTROL_FLAGS));
        BitImmediate8(cpu, 0x04u);
        mark = !cpu->zero;
        if (!mark) {
            LoadAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->y); /* DA7D */
            BitImmediate8(cpu, 0x08u);
            mark = cpu->zero;
        }
        if (mark) {
            LoadA8(cpu, 0x10u);                                /* DA84 */
            TestBitsAbsolute8(memory, cpu, WRAM_FIELD_FLAGS, 1);
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
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);                   /* DA5B */
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_X, cpu->x);
    Write8(memory, DirectAddress(cpu, DP_PROBE_X), A8(cpu));
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_Y, cpu->x);
    Write8(memory, DirectAddress(cpu, DP_PROBE_Y), A8(cpu));
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_FACING, cpu->x);
    SimulateJslFrame(memory, cpu, 0x83u, 0xda6du);             /* DA6A */
    if (Lufia2ActorMovementStep(memory, cpu) == 0)
        return 0;
    SimulateRtlFrame(memory, cpu);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);                   /* DA6E */
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
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_X, cpu->y);
    Write8(memory, DirectAddress(cpu, DP_PROBE_X), A8(cpu));
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_Y, cpu->y);
    Write8(memory, DirectAddress(cpu, DP_PROBE_Y), A8(cpu));
    TransferYToX(cpu);                                         /* D888 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FE216, cpu->x)));
    Write8(memory, DirectAddress(cpu, 0x9au), A8(cpu));
    TransferDirectToA(cpu);                                    /* D88F */
    TransferXToA(cpu);
    AslA8(cpu);
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_ACTOR_DISPLAY_OFFSET_X, cpu->x)));
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
        const uint32_t cell = LongIndexedAddress(
            column ? MAP_BLOCKING_ATTRIBUTES : WRAM_FIELD_MAP_ATTRIBUTES, cpu->x);

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
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_X, cpu->y);              /* D9C6 */
    Write8(memory, DirectAddress(cpu, DP_PROBE_X), A8(cpu));
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_Y, cpu->y);
    Write8(memory, DirectAddress(cpu, DP_PROBE_Y), A8(cpu));
    TransferYToX(cpu);                                         /* D9D0 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FE216, cpu->x)));
    Write8(memory, DirectAddress(cpu, 0x9au), A8(cpu));
    TransferXToA(cpu);                                         /* D9D7 */
    if (cpu->zero) {
        LoadA8(cpu, Read8(memory, WRAM_UNK_7FD0FE));
        if (!cpu->zero) {
            LoadA8(cpu, 0x01u);                                /* D9E0 */
            Write8(memory, DirectAddress(cpu, 0x9au), A8(cpu));
        }
    }
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->y);     /* D9E4 */
    BitImmediate8(cpu, 0x0au);
    if (cpu->zero) {
        SecondaryOccupancy(memory, cpu, 0xd9edu, 0);           /* D9EB */
        if (!SecondaryAdvanceProbe(memory, cpu, 0xda0au))      /* DA08 */
            return 0;
        SecondaryOccupancy(memory, cpu, 0xda0du, 1);           /* DA0B */
    }
    LoadAAbsolute8(memory, cpu, WRAM_UNK_7E070A, cpu->y); /* DA28 */
    Compare8(cpu, A8(cpu), 0x03u);
    if (cpu->zero) {
        LoadAAbsolute8(memory, cpu, WRAM_UNK_7E09A6, 0); /* DA2F */
        BitImmediate8(cpu, 0x01u);
        if (cpu->zero) {
            /* Shift the follower trail in $09A1.. by one. */
            LoadX16(cpu, 0x0003u);                             /* DA36 */
            do {
                LoadAAbsolute8(memory, cpu, WRAM_FOLLOW_SLOTS, cpu->x);
                StoreAAbsolute8(memory, cpu, (WRAM_FOLLOW_SLOTS + 1u), cpu->x);
                LoadX16(cpu, (uint16_t)(cpu->x - 1u));
            } while (!cpu->negative);
            LoadXDirect(memory, cpu, DP_ACTOR_SLOT);           /* DA42 */
            LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FE466, cpu->x)));
            Or8(cpu, 0x80u);
            StoreAAbsolute8(memory, cpu, WRAM_FOLLOW_SLOTS, 0);
        }
    }
    if (!SecondaryAdvanceProbe(memory, cpu, 0xda4fu))          /* DA4D */
        return 0;
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_PROBE_X))); /* DA50 */
    StoreAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_X, cpu->x);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_PROBE_Y)));
    StoreAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_Y, cpu->x);
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
    case 0xdd24u:
        pair = WRAM_ACTOR_FINE_Y;
        break;
    case 0xdd20u:
        pair = WRAM_ACTOR_FINE_Y;
        break;
    case 0xdd32u:
        pair = WRAM_ACTOR_FINE_X;
        break;
    case 0xdd36u:
        pair = WRAM_ACTOR_FINE_X;
        break;
    default:
        cpu->resume_pc = SECONDARY_BANK_83 | target;
        return 0;
    }
    if (target == 0xdd20u || target == 0xdd32u) {
        LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffffu));  /* DD20 */
        IncrementA16(cpu);
    }
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
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
    LoadXDirect(memory, cpu, DP_SLOT_RECORD_OFFSET);
    Write16Long(
        memory, LongIndexedAddress(WRAM_ACTOR_SECONDARY_SCRIPT, cpu->x),
            cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    return SecondaryExit();
}

/* $83:DD09: store the cursor at $2A as the script position and exit. */
static SecondaryStep SecondarySaveWalkCursor(const Lufia2Memory *memory,
                                             Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_SLOT_RECORD_OFFSET);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x2au));
    Write16Long(memory, LongIndexedAddress(WRAM_ACTOR_SECONDARY_SCRIPT, cpu->x),
                cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    return SecondaryExit();
}

/* $83:DC45: walk one tile over 16 sub-steps. */
static SecondaryStep SecondaryWalk(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);                   /* DC45 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_ACTOR_WALK_COUNTER, cpu->x)));
    if (cpu->zero) {
        uint8_t commit = 0;

        LoadAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->x); /* DC4D */
        BitImmediate8(cpu, 0x0au);
        if (!cpu->zero) {
            commit = 1;
        } else {
            LoadAAbsolute8(memory, cpu, 0x057cu, 0);           /* DC54 */
            if (!cpu->zero) {
                LoadA8(cpu, 0x80u);
                And8(cpu, Read8(memory, DirectAddress(cpu, 0x47u)));
                if (!cpu->zero) {
                    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_ACTOR_SLOT)));
                    if (cpu->zero)
                        commit = 1;                            /* DC63 */
                }
            }
            if (!commit) {
                LoadAAbsolute8(memory, cpu, WRAM_ACTOR_FACING, cpu->x); /* DC65 */
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

    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);                   /* DC78 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_ACTOR_WALK_COUNTER, cpu->x)));
    Write8(memory, DirectAddress(cpu, 0x32u), A8(cpu));
    cpu->carry = 0;
    Adc8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FE4DE, cpu->x)));
    Write8(memory, LongIndexedAddress(WRAM_ACTOR_WALK_COUNTER, cpu->x), A8(cpu));
    And8(cpu, 0xfcu);                                          /* DC89 */
    Sbc8(cpu, Read8(memory, DirectAddress(cpu, 0x32u)));
    if (cpu->negative)
        return SecondarySaveWalkCursor(memory, cpu);           /* DC8D */
    LoadA8(cpu, 0x03u);                                        /* DC8F */
    TrbDirect8(memory, cpu, 0x32u);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_ACTOR_WALK_COUNTER, cpu->x)));
    Sbc8(cpu, Read8(memory, DirectAddress(cpu, 0x32u)));
    LsrA8(cpu);
    LsrA8(cpu);
    Write8(memory, DirectAddress(cpu, 0x32u), A8(cpu));
    cpu->carry = 0;                                            /* DC9D */
    Adc8(cpu, Read8(memory, LongIndexedAddress(WRAM_ACTOR_WALK_PHASE, cpu->x)));
    Write8(memory, LongIndexedAddress(WRAM_ACTOR_WALK_PHASE, cpu->x), A8(cpu));
    Compare8(cpu, A8(cpu), 0x04u);                             /* DCA6 */
    if (cpu->zero || (Compare8(cpu, A8(cpu), 0x0cu), cpu->zero)) {
        const int delta = A8(cpu) == 0x04u ? 1 : -1;
        LoadAAbsolute8(memory, cpu, WRAM_ACTOR_FLAGS, cpu->x);
        BitImmediate8(cpu, 0x02u);
        if (cpu->zero)
            StepMemory8(                                       /* DCB1 */
                memory, cpu,
                AbsoluteIndexedAddress(cpu, WRAM_UNK_7E066A, cpu->x), delta);
    }
    TransferDirectToA(cpu);                                    /* DCC4 */
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_FACING, cpu->y);
    if (!SecondaryFineStep(memory, cpu))
        return SecondaryBoundary(cpu);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);                   /* DCD0 */
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_FLAGS, cpu->x);
    BitImmediate8(cpu, 0x08u);
    if (!cpu->zero) {
        /* Bob offset from $83:DD44. */
        TransferDirectToA(cpu);                                /* DCD9 */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_ACTOR_WALK_PHASE, cpu->x)));
        DecrementA8(cpu);
        TransferAToX(cpu);
        LoadA8(cpu,
               Read8(memory, LongIndexedAddress(ROM_ACTOR_WALK_BOB_TABLE, cpu->x)));
        Lufia2SignExtendA8(memory, cpu, 0xdce6u);
        LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);                       /* DCE7 */
        cpu->carry = 0;
        Add16Value(
            cpu, Read16Long(memory, LongIndexedAddress(WRAM_ACTOR_DISPLAY_OFFSET_Y,
                cpu->x)));
        Write16Long(
            memory, LongIndexedAddress(WRAM_ACTOR_DISPLAY_OFFSET_Y, cpu->x),
            cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        LoadXDirect(memory, cpu, DP_ACTOR_SLOT);               /* DCF4 */
    }
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_ACTOR_WALK_PHASE, cpu->x)));
    Compare8(cpu, A8(cpu), 0x10u);                             /* DCFA */
    if (cpu->zero) {
        SetAccumulatorWidth(cpu, 0);                           /* DCFE */
        LoadA16(cpu, Read16Direct(memory, cpu, 0x2au));
        IncrementA16(cpu);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
        return SecondaryRedispatched(memory, cpu);
    }

    return SecondarySaveWalkCursor(memory, cpu);
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
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_X, cpu->x);
    Write8(memory, DirectAddress(cpu, DP_PROBE_X), A8(cpu));
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_Y, cpu->x);
    Write8(memory, DirectAddress(cpu, DP_PROBE_Y), A8(cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $83:D99F: signed operands onto $7F:DC8C/DD1C; M=0 exit. */
static void SecondaryAddDisplayOffset(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadYDirect16(memory, cpu, 0x2au);
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
    Lufia2ActorAddSignedPair(memory, cpu, WRAM_ACTOR_DISPLAY_OFFSET_X,
                             SECONDARY_OPERAND_1, 0xd9a9u);
    SetAccumulatorWidth(cpu, 1);
    Lufia2ActorAddSignedPair(memory, cpu, WRAM_ACTOR_DISPLAY_OFFSET_Y,
                             SECONDARY_OPERAND_2, 0xd9bbu);
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
    LoadA8(cpu, Read8(memory, LongIndexedAddress(ROM_QUARTER_SINE_TABLE, index)));
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
    StoreAAbsolute8(memory, cpu, SNES_M7B, 0);
    LoadA8(cpu, 0x00u);
    ExchangeAccumulatorBytes(cpu);
    Lufia2SignExtendA8(memory, cpu, 0xdea9u);
    SetAccumulatorWidth(cpu, 1);                               /* DEAA */
    StoreAAbsolute8(memory, cpu, SNES_M7A, 0);
    ExchangeAccumulatorBytes(cpu);
    StoreAAbsolute8(memory, cpu, SNES_M7A, 0);
    {
        const uint32_t address = AbsoluteIndexedAddress(cpu, SNES_MPYL, 0);
        const uint8_t value = Read8(memory, address);          /* DEB3 */

        cpu->carry = (value & 0x80u) != 0;
        Write8(memory, address, (uint8_t)(value << 1));
        SetNz8(cpu, (uint8_t)(value << 1));
    }
    SetAccumulatorWidth(cpu, 0);                               /* DEB6 */
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, SNES_MPYM, 0));
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
    LoadAAbsolute8(memory, cpu, SECONDARY_OPERAND_3, cpu->x);
    ExchangeAccumulatorBytes(cpu);
    LoadAAbsolute8(memory, cpu, SECONDARY_OPERAND_1, cpu->x);
    LsrA8(cpu);
    SecondaryWave(memory, cpu, 0, 0xde83u);
    SecondaryScaleRadius(memory, cpu, 0xde86u);
    Write16Direct(memory, cpu, DP_SCRATCH_A, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);                               /* DE89 */
    LoadAAbsolute8(memory, cpu, SECONDARY_OPERAND_2, cpu->x);
    ExchangeAccumulatorBytes(cpu);
    LoadAAbsolute8(memory, cpu, SECONDARY_OPERAND_1, cpu->x);
    LsrA8(cpu);
    SecondaryWave(memory, cpu, 1, 0xde96u);
    SecondaryScaleRadius(memory, cpu, 0xde99u);
    Write16Direct(memory, cpu, DP_SCRATCH_C, cpu->accumulator);
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
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
    LoadAAbsolute8(memory, cpu, SECONDARY_OPERAND_1, cpu->x);
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
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_ACTOR_SLOT))); /* D67A */
    PushAccumulator8(memory, cpu);
    LoadXDirect(memory, cpu, 0x2au);
    LoadAAbsolute8(memory, cpu, SECONDARY_OPERAND_1, cpu->x);
    SimulateJslFrame(memory, cpu, 0x83u, 0xd685u);
    Lufia2ActorSpawn(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);                   /* D686 */
    LoadYDirect16(memory, cpu, DP_SLOT_WORD_OFFSET);
    LoadA8(cpu, Pull8(memory, cpu));
    Write8(memory, DirectAddress(cpu, DP_ACTOR_SLOT), A8(cpu));
    SimulateJslFrame(memory, cpu, 0x83u, 0xd690u);
    Lufia2ActorRecordOffsets(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x94u)));    /* D691 */
    Write8(memory, LongIndexedAddress(WRAM_UNK_7FD9CC, cpu->x), A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    TransferYToX(cpu);
    LoadA16(cpu, Read16Direct(memory, cpu, DP_PROBE_X));
    Write16Long(memory, LongIndexedAddress(WRAM_OBJECT_FINE_X, cpu->x),
                cpu->accumulator);
    LoadA16(cpu, Read16Direct(memory, cpu, DP_PROBE_Y));
    Write16Long(memory, LongIndexedAddress(WRAM_OBJECT_FINE_Y, cpu->x),
                cpu->accumulator);
    SecondarySkipOperand(memory, cpu);                         /* D6A6 */
}

/* $83:D661: spawn child at the actor's position. */
static void SecondarySpawnAtActor(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);                           /* D661 */
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_ACTOR_FINE_X, cpu->x)));
    Write16Direct(memory, cpu, DP_PROBE_X, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_ACTOR_FINE_Y, cpu->x)));
    Write16Direct(memory, cpu, DP_PROBE_Y, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_FACING, cpu->x);
    Write8(memory, DirectAddress(cpu, 0x94u), A8(cpu));
    SecondarySpawnChild(memory, cpu);
}

/* Secondary script handlers from $83:DF17 and its D/E/F tables. */
enum SecondaryOpcodeHandler {
    SECONDARY_OP_0X_STOP_LEADER = 0x83d5fd,         /* $0x $4x $D3 */
    SECONDARY_OP_FA_STOP = 0x83d60f,                /* $FA */
    SECONDARY_OP_FB_JUMP = 0x83d615,                /* $FB */
    SECONDARY_OP_FC_SET_066A = 0x83d626,            /* $FC */
    SECONDARY_OP_FD_SPAWN_AT_ACTOR = 0x83d638,      /* $FD */
    SECONDARY_OP_D4_SPAWN_AT_ACTOR_SKIP = 0x83d63e, /* $D4 */
    SECONDARY_OP_D5 = 0x83d6ad,                     /* $D5 */
    SECONDARY_OP_D6 = 0x83d6c7,                     /* $D6 */
    SECONDARY_OP_D7_ASSIGN_STATE_BIT1 = 0x83d6ef,   /* $D7 */
    SECONDARY_OP_CX_SPAWN = 0x83d70c,               /* $Cx */
    SECONDARY_OP_FE_PLAY_SOUND = 0x83d760,          /* $FE */
    SECONDARY_OP_FF_MAP_CELL_TEST = 0x83d76e,       /* $FF */
    SECONDARY_OP_E0 = 0x83d78c,                     /* $E0 */
    SECONDARY_OP_E1_STEP_PROBE = 0x83d79c,          /* $E1 */
    SECONDARY_OP_E2_LOOP_START = 0x83d7b2,          /* $E2 */
    SECONDARY_OP_E3_LOOP_END = 0x83d7cc,            /* $E3 */
    SECONDARY_OP_E4_SIGNED_OPERAND = 0x83d7eb,      /* $E4 */
    SECONDARY_OP_E5 = 0x83d80e,                     /* $E5 */
    SECONDARY_OP_1X = 0x83d833,                     /* $1x */
    SECONDARY_OP_2X = 0x83d844,                     /* $2x */
    SECONDARY_OP_3X = 0x83d95e,                     /* $3x */
    SECONDARY_OP_E6 = 0x83d969,                     /* $E6 */
    SECONDARY_OP_BX_DISPLAY_OFFSET = 0x83d992,      /* $Bx */
    SECONDARY_OP_5X = 0x83da9a,                     /* $5x */
    SECONDARY_OP_EF = 0x83dab3,                     /* $EF */
    SECONDARY_OP_D0 = 0x83dac3,                     /* $D0 */
    SECONDARY_OP_D1 = 0x83dac9,                     /* $D1 */
    SECONDARY_OP_6X_SPRITE_RELOAD = 0x83dae9,       /* $6x */
    SECONDARY_OP_7X = 0x83db54,                     /* $7x */
    SECONDARY_OP_AX_MOVE_FINE = 0x83db5a,           /* $Ax */
    SECONDARY_OP_8X_CLEAR_OCCUPANCY = 0x83db63,     /* $8x */
    SECONDARY_OP_E9_MARK_OCCUPANCY = 0x83db6d,      /* $E9 */
    SECONDARY_OP_EA = 0x83db77,                     /* $EA */
    SECONDARY_OP_EB_SPAWN_CHILD = 0x83db95,         /* $EB */
    SECONDARY_OP_EC_SYNC_FINE_POSITION = 0x83dbc2,  /* $EC */
    SECONDARY_OP_ED = 0x83dbdc,                     /* $ED */
    SECONDARY_OP_EE_SYNC_FINE_POSITION = 0x83dc04,  /* $EE */
    SECONDARY_OP_F0_WALK = 0x83dc45,                /* $F0 */
    SECONDARY_OP_F1 = 0x83dd54,                     /* $F1 */
    SECONDARY_OP_F2_SIGNED_OFFSET = 0x83dd6e,       /* $F2 */
    SECONDARY_OP_F5_CLEAR_OCCUPANCY = 0x83ddfb,     /* $F5 */
    SECONDARY_OP_F6_ORBIT = 0x83de11,               /* $F6 */
    SECONDARY_OP_F7_ORBIT = 0x83de29,               /* $F7 */
    SECONDARY_OP_F8_ORBIT = 0x83de5d,               /* $F8 */
    SECONDARY_OP_F9 = 0x83debd,                     /* $F9 */
    SECONDARY_OP_9X_WAIT = 0x83dee9,                /* $9x */
};

static SecondaryStep SecondaryExecuteHandler(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t handler_pc);

/* $83:D5FD: secondary opcode $0x $4x $D3. */
static SecondaryStep SecondaryOp0XStopLeader(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SecondaryStopScript(memory, cpu, 0xd5ffu);             /* D5FD */
    SecondaryLeaderStopFlag(memory, cpu, 0xd602u);         /* D600 */
    return SecondaryExit();
}

/* $83:D60F: secondary opcode $FA. */
static SecondaryStep SecondaryOpFAStop(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SecondaryStopScript(memory, cpu, 0xd611u);             /* D60F */
    return SecondaryExit();
}

/* $83:D615: secondary opcode $FB. */
static SecondaryStep SecondaryOpFBJump(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, 0x2au);                       /* D615 */
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, SECONDARY_OPERAND_1, cpu->x));
    cpu->carry = 0;
    Add16Immediate(cpu, 0x8000u);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    return SecondaryRedispatched(memory, cpu);
}

/* $83:D626: secondary opcode $FC. */
static SecondaryStep SecondaryOpFCSet066A(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, 0x2au);                       /* D626 */
    LoadAAbsolute8(memory, cpu, WRAM_UNK_7E066A, cpu->y);
    And8(cpu, 0x06u);
    Or8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, SECONDARY_OPERAND_1, cpu->x)));
    StoreAAbsolute8(memory, cpu, WRAM_UNK_7E066A, cpu->y);
    IncrementX16(cpu);
    IncrementX16(cpu);
    return SecondaryRedispatched(memory, cpu);
}

/* $83:D6AD: $D5 stores the probe position in the claimed pending object's record. */
static SecondaryStep SecondaryOpStoreProbeInPendingObject(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);               /* D6AD */
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_ACTOR_CLAIMED_PENDING_OBJECT,
        cpu->x)));
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_PROBE_X)));
    Write8(memory, LongIndexedAddress(WRAM_FIELD_PENDING_RECORD_X, cpu->x), A8(cpu));
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_PROBE_Y)));
    Write8(memory, LongIndexedAddress(WRAM_FIELD_PENDING_RECORD_Y, cpu->x), A8(cpu));
    LoadXDirect(memory, cpu, 0x2au);
    IncrementX16(cpu);
    return SecondaryRedispatched(memory, cpu);
}

/* $83:D6C7: secondary opcode $D6. */
static SecondaryStep SecondaryOpD6(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);               /* D6C7 */
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_ACTOR_CLAIMED_OBJECT_RECORD,
        cpu->x)));
    AslA8(cpu);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 0);                           /* D6D0 */
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_ACTOR_FINE_X, cpu->x)));
    PushAccumulator16(memory, cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_ACTOR_FINE_Y, cpu->x)));
    IncrementA16(cpu);
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
    Write16Long(
        memory, LongIndexedAddress(WRAM_ACTOR_FINE_Y, cpu->x), cpu->accumulator);
    PullAccumulator16(memory, cpu);
    Write16Long(
        memory, LongIndexedAddress(WRAM_ACTOR_FINE_X, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);                           /* D6E7 */
    LoadXDirect(memory, cpu, 0x2au);
    IncrementX16(cpu);
    return SecondaryRedispatched(memory, cpu);
}

/* Secondary opcode $D7: operand bit 0 sets or clears state bit 1. */
static SecondaryStep SecondaryOpD7AssignStateBit1(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, 0x2au);                       /* D6EF */
    LoadAAbsolute8(memory, cpu, SECONDARY_OPERAND_1, cpu->x);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LsrA8(cpu);
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->x);
    if (cpu->carry)
        And8(cpu, 0xfdu);                                  /* D6FC */
    else
        Or8(cpu, 0x02u);                                   /* D700 */
    StoreAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->x);
    LoadXDirect(memory, cpu, 0x2au);                       /* D705 */
    IncrementX16(cpu);
    IncrementX16(cpu);
    return SecondaryRedispatched(memory, cpu);
}

/* Secondary opcode $1x: operand to $7F:E4DE[slot]. */
static SecondaryStep SecondaryOp1X(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadYDirect16(memory, cpu, 0x2au);                     /* D833 */
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadAAbsolute8(memory, cpu, SECONDARY_OPERAND_1, cpu->y);
    Write8(memory, LongIndexedAddress(WRAM_UNK_7FE4DE, cpu->x), A8(cpu));
    TransferYToX(cpu);
    IncrementX16(cpu);
    IncrementX16(cpu);
    return SecondaryRedispatched(memory, cpu);
}

/* Secondary opcode $2x: operand to $066A unless $0736 bit 1 is set. */
static SecondaryStep SecondaryOp2X(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, 0x2au);                       /* D844 */
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_FLAGS, cpu->y);
    BitImmediate8(cpu, 0x02u);
    if (cpu->zero) {
        LoadAAbsolute8(memory, cpu, SECONDARY_OPERAND_1, cpu->x); /* D84D */
        StoreAAbsolute8(memory, cpu, WRAM_UNK_7E066A, cpu->y);
    }
    IncrementX16(cpu);
    IncrementX16(cpu);
    return SecondaryRedispatched(memory, cpu);
}

/* $83:DA9A: secondary opcode $5x. */
static SecondaryStep SecondaryOp5X(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadYDirect16(memory, cpu, 0x2au);                     /* DA9A */
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadAAbsolute8(memory, cpu, SECONDARY_OPERAND_1, cpu->y);
    StoreAAbsolute8(memory, cpu, WRAM_ACTOR_FACING, cpu->x);
    TransferDirectToA(cpu);                                /* DAA4 */
    Write8(memory, LongIndexedAddress(WRAM_ACTOR_WALK_COUNTER, cpu->x), A8(cpu));
    Write8(memory, LongIndexedAddress(WRAM_ACTOR_WALK_PHASE, cpu->x), A8(cpu));
    TransferYToX(cpu);
    IncrementX16(cpu);
    IncrementX16(cpu);
    return SecondaryRedispatched(memory, cpu);
}

/* Secondary opcode $9x: wait for the low nibble in steps. */
static SecondaryStep SecondaryOp9XWait(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const uint32_t wait = WRAM_ACTOR_WALK_COUNTER;

    LoadYDirect16(memory, cpu, 0x2au);                     /* DEE9 */
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(wait, cpu->x)));
    if (!cpu->negative) {
        LoadAAbsolute8(memory, cpu, SECONDARY_OPCODE_BYTE, cpu->y); /* DEF3 */
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

/* Secondary opcode $E2: repeat count, loop start after the operand. */
static SecondaryStep SecondaryOpE2LoopStart(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadYDirect16(memory, cpu, 0x2au);                     /* D7B2 */
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
    LoadAAbsolute8(memory, cpu, SECONDARY_OPERAND_1, cpu->y);
    Write8(memory, LongIndexedAddress(WRAM_ACTOR_SCRATCH_A, cpu->x), A8(cpu));
    SetAccumulatorWidth(cpu, 0);                           /* D7BD */
    LoadA16(cpu, cpu->y);
    IncrementA16(cpu);
    IncrementA16(cpu);
    Write16Long(memory, LongIndexedAddress(WRAM_ACTOR_SCRATCH_B, cpu->x),
                cpu->accumulator);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    return SecondaryRedispatched(memory, cpu);
}

/* Secondary opcode $E3: count down, jump back to the loop start. */
static SecondaryStep SecondaryOpE3LoopEnd(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);                       /* D7CC */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_ACTOR_SCRATCH_A, cpu->x)));
    DecrementA8(cpu);
    Write8(memory, LongIndexedAddress(WRAM_ACTOR_SCRATCH_A, cpu->x), A8(cpu));
    if (cpu->zero)
        return SecondaryNextByte(memory, cpu, 1);          /* D7D9 */
    SetAccumulatorWidth(cpu, 0);                           /* D7DF */
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_ACTOR_SCRATCH_B, cpu->x)));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    return SecondaryRedispatched(memory, cpu);
}

/* $83:D80E: secondary opcode $E5. */
static SecondaryStep SecondaryOpE5(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);                       /* D80E */
    TransferDirectToA(cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu,
            Read16Long(memory, LongIndexedAddress(WRAM_ACTOR_OFFSET_ALT_X, cpu->x)));
    cpu->carry = 0;
    Add16Value(cpu,
               Read16Long(memory, LongIndexedAddress(WRAM_ACTOR_OFFSET_ALT_Y, cpu->x)));
    Write16Long(memory, LongIndexedAddress(WRAM_ACTOR_OFFSET_ALT_X, cpu->x),
                cpu->accumulator);
    LsrA16(cpu);                                           /* D820 */
    LsrA16(cpu);
    cpu->carry = 0;
    Add16Value(
        cpu, Read16Long(memory, LongIndexedAddress(WRAM_ACTOR_DISPLAY_OFFSET_Y,
            cpu->x)));
    Write16Long(
        memory, LongIndexedAddress(WRAM_ACTOR_DISPLAY_OFFSET_Y, cpu->x),
            cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    return SecondaryNextByte(memory, cpu, 1);
}

/* $83:D969: secondary opcode $E6. */
static SecondaryStep SecondaryOpE6(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadYDirect16(memory, cpu, 0x2au);                     /* D969 */
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
    SetAccumulatorWidth(cpu, 0);
    for (uint8_t i = 0; i < 2u; ++i) {
        const uint32_t pair =
            i ? WRAM_ACTOR_DISPLAY_OFFSET_Y : WRAM_ACTOR_DISPLAY_OFFSET_X;
        LoadA16(cpu, Read16AbsoluteIndexed(
                         memory, cpu, i ? SECONDARY_OPERAND_3 : SECONDARY_OPERAND_1,
                         cpu->y));
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
}

/* $83:DB77: $EA sets or clears bit 1 of the actor's flags from the operand. */
static SecondaryStep SecondaryOpSetActorFlagBit1(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, 0x2au);                       /* DB77 */
    LoadAAbsolute8(memory, cpu, SECONDARY_OPERAND_1, cpu->x);
    {
        const uint8_t clear = !cpu->zero;
        LoadAAbsolute8(memory, cpu, WRAM_ACTOR_FLAGS, cpu->y);
        if (clear)
            And8(cpu, 0xfdu);                              /* DB81 */
        else
            Or8(cpu, 0x02u);                               /* DB8B */
        StoreAAbsolute8(memory, cpu, WRAM_ACTOR_FLAGS, cpu->y);
    }
    IncrementX16(cpu);
    IncrementX16(cpu);
    return SecondaryRedispatched(memory, cpu);
}

/* $83:DBDC: $ED sets the secondary timer from the operand; a non-zero operand also sets
 * the blink bit. */
static SecondaryStep SecondaryOpSetTimerAndBlink(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, 0x2au);                       /* DBDC */
    LoadAAbsolute8(memory, cpu, SECONDARY_OPERAND_1, cpu->x);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    Write8(memory, LongIndexedAddress(WRAM_ACTOR_SECONDARY_TIMER, cpu->x), A8(cpu));
    Or8(cpu, 0x00u);
    {
        const uint8_t blink = !cpu->zero;
        LoadA8(
            cpu, Read8(memory, LongIndexedAddress(WRAM_ACTOR_FLAGS, cpu->x)));
        if (blink)
            Or8(cpu, 0x80u);                               /* DBF7 */
        else
            And8(cpu, 0x7fu);                              /* DBEF */
        Write8(memory, LongIndexedAddress(WRAM_ACTOR_FLAGS, cpu->x), A8(cpu));
    }
    return SecondaryNextByte(memory, cpu, 2);
}

/* $83:DAB3: secondary opcode $EF. */
static SecondaryStep SecondaryOpEF(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);               /* DAB3 */
    LoadAAbsolute8(memory, cpu, WRAM_UNK_7E066A, cpu->x);
    And8(cpu, 0x07u);
    StoreAAbsolute8(memory, cpu, WRAM_ACTOR_FACING, cpu->x);
    return SecondaryNextByte(memory, cpu, 1);
}

/* $83:DAC3: $D0 skips its operand byte. */
static SecondaryStep SecondaryOpSkipByte(const Lufia2Memory *memory,
                                         Lufia2CpuState *cpu) {
    return SecondaryNextByte(memory, cpu, 1);             /* DAC3 */
}

/* $83:DAC9: $D1 sets bit 7 of $7F:E316[slot] when the operand is non-zero, else clears
 * it. */
static SecondaryStep SecondaryOpSetBit7FromOperand(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, 0x2au);                       /* DAC9 */
    LoadAAbsolute8(memory, cpu, SECONDARY_OPERAND_1, cpu->x);
    Compare8(cpu, A8(cpu), 0x01u);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FE316, cpu->x)));
    if (cpu->carry)
        Or8(cpu, 0x80u);                                   /* DADC */
    else
        And8(cpu, 0x7fu);                                  /* DAD8 */
    Write8(memory, LongIndexedAddress(WRAM_UNK_7FE316, cpu->x), A8(cpu));
    return SecondaryNextByte(memory, cpu, 2);
}

/* $83:DD54: secondary opcode $F1. */
static SecondaryStep SecondaryOpF1(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, 0x2au);                       /* DD54 */
    cpu->y = cpu->x;                                       /* TXY */
    SetNz16(cpu, cpu->y);
    LoadAAbsolute8(memory, cpu, SECONDARY_OPERAND_1, cpu->x);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    Write8(memory, LongIndexedAddress(0x7fe2eeu, cpu->x), A8(cpu));
    LoadAAbsolute8(memory, cpu, WRAM_UNK_7E066A, cpu->x);
    And8(cpu, 0x07u);
    StoreAAbsolute8(memory, cpu, WRAM_UNK_7E066A, cpu->x);
    TransferYToX(cpu);
    IncrementX16(cpu);
    IncrementX16(cpu);
    return SecondaryRedispatched(memory, cpu);
}

/* $83:DEBD: $F9 copies the fine position and display offsets to the scratch words. */
static SecondaryStep SecondaryOpSavePositionScratch(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);                       /* DEBD */
    SetAccumulatorWidth(cpu, 0);
    CopyLong16(memory, cpu, WRAM_ACTOR_FINE_X, WRAM_ACTOR_SCRATCH_A);
    CopyLong16(memory, cpu, WRAM_ACTOR_FINE_Y, WRAM_ACTOR_SCRATCH_B);
    CopyLong16(memory, cpu, WRAM_ACTOR_DISPLAY_OFFSET_X, WRAM_ACTOR_OFFSET_ALT_X);
    CopyLong16(memory, cpu, WRAM_ACTOR_DISPLAY_OFFSET_Y, WRAM_ACTOR_OFFSET_ALT_Y);
    SetAccumulatorWidth(cpu, 1);
    return SecondaryNextByte(memory, cpu, 1);
}

/* $83:DB54: secondary opcode $7x $Ax. */
static SecondaryStep SecondaryOp7XAXMoveFine(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t handler_pc) {
    const uint8_t save = (handler_pc & 0x00ffffffu) == SECONDARY_OP_7X;

    SimulateJsrFrame(memory, cpu, save ? 0xdb56u : 0xdb5cu);
    Lufia2ActorMoveFinePosition(memory, cpu);              /* FA81 */
    SimulateRtsFrame(memory, cpu);
    if (save)
        return SecondarySaveCursorExit(memory, cpu);       /* DB57 */
    TransferAToX(cpu);                                     /* DB5D */
    SetAccumulatorWidth(cpu, 1);
    return SecondaryRedispatched(memory, cpu);
}

/* $83:DB6D: secondary opcode $E9. */
static SecondaryStep SecondaryOpE9MarkOccupancy(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJslFrame(memory, cpu, 0x83u, 0xdb70u);         /* DB6D */
    Lufia2ActorMarkMapOccupancy(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    return SecondaryNextByte(memory, cpu, 1);
}

/* $83:D760: secondary opcode $FE. */
static SecondaryStep SecondaryOpFEPlaySound(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, 0x2au);                       /* D760 */
    LoadAAbsolute8(memory, cpu, SECONDARY_OPERAND_1, cpu->x);
    SimulateJslFrame(memory, cpu, 0x83u, 0xd768u);
    Lufia2QueueDeferredSound(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    IncrementX16(cpu);
    IncrementX16(cpu);
    return SecondaryRedispatched(memory, cpu);
}

/* $83:D7EB: secondary opcode $E4. */
static SecondaryStep SecondaryOpE4SignedOperand(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadYDirect16(memory, cpu, 0x2au);                     /* D7EB */
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
    for (uint8_t i = 0; i < 2u; ++i) {
        LoadAAbsolute8(memory, cpu, i ? SECONDARY_OPERAND_2 : SECONDARY_OPERAND_1,
                       cpu->y);
        Lufia2SignExtendA8(memory, cpu, i ? 0xd800u : 0xd7f4u);
        Write16Long(memory,
                    LongIndexedAddress(
                        i ? WRAM_ACTOR_OFFSET_ALT_Y : WRAM_ACTOR_OFFSET_ALT_X, cpu->x),
                    cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
    }
    IncrementY16(cpu);                                     /* D807 */
    IncrementY16(cpu);
    IncrementY16(cpu);
    TransferYToX(cpu);
    return SecondaryRedispatched(memory, cpu);
}

/* $83:DBC2: secondary opcode $EC. */
static SecondaryStep SecondaryOpECSyncFinePosition(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);                       /* DBC2 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_ACTOR_SCRATCH_A, cpu->x)));
    StoreAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_X, cpu->y);
    LoadA8(cpu, Read8(memory, LongIndexedAddress((WRAM_ACTOR_SCRATCH_A + 1u), cpu->x)));
    StoreAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_Y, cpu->y);
    SimulateJslFrame(memory, cpu, 0x83u, 0xdbd5u);
    Lufia2ActorSyncFinePosition(memory, cpu);              /* A746 */
    SimulateRtlFrame(memory, cpu);
    return SecondaryNextByte(memory, cpu, 1);
}

/* $83:DC04: secondary opcode $EE. */
static SecondaryStep SecondaryOpEESyncFinePosition(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, 0x2au);                       /* DC04 */
    LoadAAbsolute8(memory, cpu, SECONDARY_OPERAND_1, cpu->x);
    {
        const uint8_t restore = !cpu->zero;
        uint32_t flags;

        LoadXDirect(memory, cpu, DP_ACTOR_SLOT);           /* DC0B */
        flags = LongIndexedAddress(WRAM_UNK_7FE316, cpu->x);
        LoadA8(cpu, Read8(memory, flags));
        if (restore) {
            And8(cpu, 0x7fu);                              /* DC11 */
            Write8(memory, flags, A8(cpu));
            LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
            LoadA8(cpu,
                   Read8(memory, LongIndexedAddress(WRAM_ACTOR_SCRATCH_A, cpu->x)));
            ExchangeAccumulatorBytes(cpu);
            LoadA8(cpu, Read8(memory,
                              LongIndexedAddress((WRAM_ACTOR_SCRATCH_A + 1u), cpu->x)));
        } else {
            Or8(cpu, 0x80u);                               /* DC2A */
            Write8(memory, flags, A8(cpu));
            TransferDirectToA(cpu);
        }
    }
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);               /* DC31 */
    StoreAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_Y, cpu->x);
    ExchangeAccumulatorBytes(cpu);
    StoreAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_X, cpu->x);
    SimulateJslFrame(memory, cpu, 0x83u, 0xdc3du);
    Lufia2ActorSyncFinePosition(memory, cpu);              /* A746 */
    SimulateRtlFrame(memory, cpu);
    return SecondaryNextByte(memory, cpu, 2);
}

/* $83:D78C: secondary opcode $E0 $E1. */
static SecondaryStep SecondaryOpE0E1StepProbe(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t handler_pc) {
    const uint8_t step = (handler_pc & 0x00ffffffu) == SECONDARY_OP_E0;

    SecondaryActorToProbe(memory, cpu, step ? 0xd78eu : 0xd79eu);
    if (step) {
        LoadAAbsolute8(memory, cpu, WRAM_ACTOR_FACING, cpu->x); /* D78F */
        SimulateJslFrame(memory, cpu, 0x83u, 0xd795u);
        if (Lufia2ActorMovementStep(memory, cpu) == 0)
            return SecondaryBoundary(cpu);
        SimulateRtlFrame(memory, cpu);
    }
    return SecondaryNextByte(memory, cpu, 1);
}

/* $83:D95E: secondary opcode $3x $Bx. */
static SecondaryStep SecondaryOp3XBXDisplayOffset(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t handler_pc) {
    const uint8_t save = (handler_pc & 0x00ffffffu) == SECONDARY_OP_3X;

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

/* $83:DB63: secondary opcode $8x. */
static SecondaryStep SecondaryOp8XClearOccupancy(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJslFrame(memory, cpu, 0x83u, 0xdb66u);         /* DB63 */
    Lufia2ActorClearMapOccupancy(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    return SecondaryNextByte(memory, cpu, 1);
}

/* $83:DDFB: secondary opcode $F5. */
static SecondaryStep SecondaryOpF5ClearOccupancy(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJslFrame(memory, cpu, 0x83u, 0xddfeu);         /* DDFB */
    Lufia2ActorClearMapOccupancy(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);               /* DDFF */
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->x);
    Or8(cpu, 0x04u);
    StoreAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->x);
    LoadA8(cpu, 0xffu);
    StoreAAbsolute8(memory, cpu, WRAM_UNK_7E05D2, cpu->x);
    return SecondaryExecuteHandler(memory, cpu, 0x83d5fdu);
}

/* $83:D76E: secondary opcode $FF. */
static SecondaryStep SecondaryOpFFMapCellTest(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadYDirect16(memory, cpu, DP_ACTOR_SLOT);             /* D76E */
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_X, cpu->y);
    Write8(memory, DirectAddress(cpu, DP_PROBE_X), A8(cpu));
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_Y, cpu->y);
    Write8(memory, DirectAddress(cpu, DP_PROBE_Y), A8(cpu));
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
}

/* $83:DD6E: secondary opcode $F2. */
static SecondaryStep SecondaryOpF2SignedOffset(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);                       /* DD6E */
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, WRAM_ACTOR_FINE_X));
    Write16Long(
        memory, LongIndexedAddress(WRAM_ACTOR_FINE_X, cpu->x), cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, WRAM_ACTOR_FINE_Y));
    Write16Long(
        memory, LongIndexedAddress(WRAM_ACTOR_FINE_Y, cpu->x), cpu->accumulator);
    LoadXDirect(memory, cpu, 0x2au);                       /* DD82 */
    SecondarySignedOperand(memory, cpu, 0xdd86u);
    Write16Direct(memory, cpu, DP_SCRATCH_A, cpu->accumulator);
    IncrementX16(cpu);
    SecondarySignedOperand(memory, cpu, 0xdd8cu);
    IncrementX16(cpu);
    PushIndex(memory, cpu);                                /* DD8E */
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
    AddLong16(memory, cpu, WRAM_ACTOR_FINE_Y);
    LoadA16(cpu, Read16Direct(memory, cpu, DP_SCRATCH_A)); /* DD9A */
    AddLong16(memory, cpu, WRAM_ACTOR_FINE_X);
    cpu->y = PullIndexValue(memory, cpu);                  /* DDA5 */
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, SECONDARY_OPERAND_1, cpu->x));
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
        memory, LongIndexedAddress(WRAM_ACTOR_DISPLAY_OFFSET_X, cpu->x),
            cpu->accumulator);
    IncrementX16(cpu);
    SetAccumulatorWidth(cpu, 1);
    TransferDirectToA(cpu);
    LoadAAbsolute8(memory, cpu, SECONDARY_OPERAND_1, cpu->x);
    Lufia2SignExtendA8(memory, cpu, 0xddbcu);
    Write16Long(
        memory, LongIndexedAddress(WRAM_ACTOR_DISPLAY_OFFSET_Y, cpu->x),
            cpu->accumulator);
    return SecondarySaveCursorPlus(memory, cpu, 0);
}

/* $83:DE11: secondary opcode $F6. */
static SecondaryStep SecondaryOpF6Orbit(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SecondaryOrbitOffsets(memory, cpu, 0xde13u);           /* DE11 */
    LoadA16(cpu, Read16Direct(memory, cpu, DP_SCRATCH_A));
    Write16Long(
        memory, LongIndexedAddress(WRAM_ACTOR_DISPLAY_OFFSET_Y, cpu->x),
            cpu->accumulator);
    LoadA16(cpu, Read16Direct(memory, cpu, DP_SCRATCH_C));
    Write16Long(
        memory, LongIndexedAddress(WRAM_ACTOR_DISPLAY_OFFSET_X, cpu->x),
            cpu->accumulator);
    return SecondarySaveCursorPlus(memory, cpu, 4);
}

/* $83:DE29: secondary opcode $F7. */
static SecondaryStep SecondaryOpF7Orbit(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SecondaryOrbitOffsets(memory, cpu, 0xde2bu);           /* DE29 */
    LoadA16(cpu, Read16Direct(memory, cpu, DP_SCRATCH_C));
    cpu->carry = 0;
    Add16Value(cpu,
               Read16Long(memory, LongIndexedAddress(WRAM_ACTOR_SCRATCH_B, cpu->x)));
    Write16Long(
        memory, LongIndexedAddress(WRAM_ACTOR_FINE_Y, cpu->x), cpu->accumulator);
    LsrA16(cpu);
    LsrA16(cpu);
    LsrA16(cpu);
    LsrA16(cpu);
    Add16Immediate(cpu, 0x0000u);                          /* DE3B */
    SetAccumulatorWidth(cpu, 1);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    StoreAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_Y, cpu->x);
    SetAccumulatorWidth(cpu, 0);                           /* DE45 */
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
    LoadA16(cpu, Read16Direct(memory, cpu, DP_SCRATCH_A));
    cpu->carry = 0;
    Add16Value(cpu,
               Read16Long(memory, LongIndexedAddress(WRAM_ACTOR_OFFSET_ALT_Y, cpu->x)));
    Write16Long(
        memory, LongIndexedAddress(WRAM_ACTOR_DISPLAY_OFFSET_Y, cpu->x),
            cpu->accumulator);
    return SecondarySaveCursorPlus(memory, cpu, 4);
}

/* $83:DE5D: secondary opcode $F8. */
static SecondaryStep SecondaryOpF8Orbit(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SecondaryOrbitOffsets(memory, cpu, 0xde5fu);           /* DE5D */
    LoadA16(cpu, Read16Direct(memory, cpu, DP_SCRATCH_C));
    cpu->carry = 1;
    Add16Value(cpu, (uint16_t)~Read16Direct(memory, cpu, DP_SCRATCH_A));
    LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffffu));
    IncrementA16(cpu);
    Write16Long(
        memory, LongIndexedAddress(WRAM_ACTOR_DISPLAY_OFFSET_Y, cpu->x),
            cpu->accumulator);
    return SecondarySaveCursorPlus(memory, cpu, 4);
}

/* $83:D70C: secondary opcode $Cx. */
static SecondaryStep SecondaryOpCXSpawn(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_ACTOR_SLOT))); /* D70C */
    PushAccumulator8(memory, cpu);
    LoadXDirect(memory, cpu, 0x2au);
    LoadAAbsolute8(memory, cpu, SECONDARY_OPERAND_1, cpu->x);
    SimulateJslFrame(memory, cpu, 0x83u, 0xd717u);
    Lufia2ActorSpawn(memory, cpu);                         /* DF87 */
    SimulateRtlFrame(memory, cpu);
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);                       /* D718 */
    StoreXDirect16(memory, cpu, DP_SCRATCH_A);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    StoreXDirect16(memory, cpu, DP_SCRATCH_C);
    LoadA8(cpu, Pull8(memory, cpu));
    Write8(memory, DirectAddress(cpu, DP_ACTOR_SLOT), A8(cpu));
    SimulateJslFrame(memory, cpu, 0x83u, 0xd726u);
    Lufia2ActorRecordOffsets(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);               /* D727 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_ACTOR_CLAIMED_OBJECT_RECORD,
        cpu->x)));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_ACTOR_CLAIMED_PENDING_OBJECT,
        cpu->x)));
    LoadXDirect(memory, cpu, DP_SCRATCH_C);
    Write8(memory, LongIndexedAddress(WRAM_OBJECT_ANIMATION_REQUEST, cpu->x), A8(cpu));
    ExchangeAccumulatorBytes(cpu);
    Write8(memory, LongIndexedAddress(WRAM_UNK_7FDA2C, cpu->x), A8(cpu));
    SetAccumulatorWidth(cpu, 0);                           /* D73D */
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_ACTOR_FINE_X, cpu->x)));
    Write16Direct(memory, cpu, DP_SCRATCH_C, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_ACTOR_FINE_Y, cpu->x)));
    LoadXDirect(memory, cpu, DP_SCRATCH_A);
    Write16Long(memory, LongIndexedAddress(WRAM_OBJECT_FINE_Y, cpu->x),
                cpu->accumulator);
    LoadA16(cpu, Read16Direct(memory, cpu, DP_SCRATCH_C));
    Write16Long(memory, LongIndexedAddress(WRAM_OBJECT_FINE_X, cpu->x),
                cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    return SecondaryNextByte(memory, cpu, 2);
}

/* $83:D638: secondary opcode $FD. */
static SecondaryStep SecondaryOpFDSpawnAtActor(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0xd63au);                /* D638 */
    SecondarySpawnAtActor(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    return SecondaryRedispatched(memory, cpu);
}

/* $83:D63E: secondary opcode $D4. */
static SecondaryStep SecondaryOpD4SpawnAtActorSkip(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0xd640u);                /* D63E */
    SecondarySpawnAtActor(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);               /* D641 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FE216, cpu->x)));
    Compare8(cpu, A8(cpu), 0x02u);
    TransferDirectToA(cpu);
    TransferYToX(cpu);
    SetAccumulatorWidth(cpu, 0);
    if (!cpu->carry)
        LoadA16(cpu, 0xfff8u);                             /* D64F */
    AddLong16(memory, cpu, WRAM_OBJECT_FINE_X);
    SimulateJsrFrame(memory, cpu, 0xd65du);
    SecondarySkipOperand(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    return SecondaryRedispatched(memory, cpu);
}

/* $83:DB95: secondary opcode $EB. */
static SecondaryStep SecondaryOpEBSpawnChild(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    unsigned i;

    SetAccumulatorWidth(cpu, 0);                           /* DB95 */
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
    for (i = 0; i < 2u; ++i) {
        LoadA16(cpu,
                Read16Long(memory, LongIndexedAddress(i ? (WRAM_ACTOR_SCRATCH_A + 1u)
                                                        : WRAM_ACTOR_SCRATCH_A,
                                                      cpu->x)));
        And16(cpu, 0x00ffu);
        AslA16(cpu);
        AslA16(cpu);
        AslA16(cpu);
        AslA16(cpu);
        Write16Direct(memory, cpu, i ? 0x91u : 0x8fu, cpu->accumulator);
    }
    SetAccumulatorWidth(cpu, 1);                           /* DBB3 */
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_FACING, cpu->x);
    Write8(memory, DirectAddress(cpu, 0x94u), A8(cpu));
    SimulateJsrFrame(memory, cpu, 0xdbbeu);
    SecondarySpawnChild(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    return SecondaryRedispatched(memory, cpu);
}

/* $83:DAE9: secondary opcode $6x. */
static SecondaryStep SecondaryOp6XSpriteReload(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ActorSpriteReload(memory, cpu);
    LoadXDirect(memory, cpu, 0x2au);                       /* DB34 */
    IncrementX16(cpu);
    return SecondaryRedispatched(memory, cpu);
}

/* $83:DC45: secondary opcode $F0. */
static SecondaryStep SecondaryOpF0Walk(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    return SecondaryWalk(memory, cpu);
}

static SecondaryStep SecondaryExecuteHandler(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t handler_pc) {
    switch (handler_pc & 0x00ffffffu) {
    case SECONDARY_OP_0X_STOP_LEADER:
        return SecondaryOp0XStopLeader(memory, cpu);
    case SECONDARY_OP_FA_STOP:
        return SecondaryOpFAStop(memory, cpu);
    case SECONDARY_OP_FB_JUMP:
        return SecondaryOpFBJump(memory, cpu);
    case SECONDARY_OP_FC_SET_066A:
        return SecondaryOpFCSet066A(memory, cpu);
    case SECONDARY_OP_D5:
        return SecondaryOpStoreProbeInPendingObject(memory, cpu);
    case SECONDARY_OP_D6:
        return SecondaryOpD6(memory, cpu);
    case SECONDARY_OP_D7_ASSIGN_STATE_BIT1:
        return SecondaryOpD7AssignStateBit1(memory, cpu);
    case SECONDARY_OP_1X:
        return SecondaryOp1X(memory, cpu);
    case SECONDARY_OP_2X:
        return SecondaryOp2X(memory, cpu);
    case SECONDARY_OP_5X:
        return SecondaryOp5X(memory, cpu);
    case SECONDARY_OP_9X_WAIT:
        return SecondaryOp9XWait(memory, cpu);
    case SECONDARY_OP_E2_LOOP_START:
        return SecondaryOpE2LoopStart(memory, cpu);
    case SECONDARY_OP_E3_LOOP_END:
        return SecondaryOpE3LoopEnd(memory, cpu);
    case SECONDARY_OP_E5:
        return SecondaryOpE5(memory, cpu);
    case SECONDARY_OP_E6:
        return SecondaryOpE6(memory, cpu);
    case SECONDARY_OP_EA:
        return SecondaryOpSetActorFlagBit1(memory, cpu);
    case SECONDARY_OP_ED:
        return SecondaryOpSetTimerAndBlink(memory, cpu);
    case SECONDARY_OP_EF:
        return SecondaryOpEF(memory, cpu);
    case SECONDARY_OP_D0:
        return SecondaryOpSkipByte(memory, cpu);
    case SECONDARY_OP_D1:
        return SecondaryOpSetBit7FromOperand(memory, cpu);
    case SECONDARY_OP_F1:
        return SecondaryOpF1(memory, cpu);
    case SECONDARY_OP_F9:
        return SecondaryOpSavePositionScratch(memory, cpu);
    case SECONDARY_OP_7X:
    case SECONDARY_OP_AX_MOVE_FINE:
        return SecondaryOp7XAXMoveFine(memory, cpu, handler_pc);
    case SECONDARY_OP_E9_MARK_OCCUPANCY:
        return SecondaryOpE9MarkOccupancy(memory, cpu);
    case SECONDARY_OP_FE_PLAY_SOUND:
        return SecondaryOpFEPlaySound(memory, cpu);
    case SECONDARY_OP_E4_SIGNED_OPERAND:
        return SecondaryOpE4SignedOperand(memory, cpu);
    case SECONDARY_OP_EC_SYNC_FINE_POSITION:
        return SecondaryOpECSyncFinePosition(memory, cpu);
    case SECONDARY_OP_EE_SYNC_FINE_POSITION:
        return SecondaryOpEESyncFinePosition(memory, cpu);
    case SECONDARY_OP_E0:
    case SECONDARY_OP_E1_STEP_PROBE:
        return SecondaryOpE0E1StepProbe(memory, cpu, handler_pc);
    case SECONDARY_OP_3X:
    case SECONDARY_OP_BX_DISPLAY_OFFSET:
        return SecondaryOp3XBXDisplayOffset(memory, cpu, handler_pc);
    case SECONDARY_OP_8X_CLEAR_OCCUPANCY:
        return SecondaryOp8XClearOccupancy(memory, cpu);
    case SECONDARY_OP_F5_CLEAR_OCCUPANCY:
        return SecondaryOpF5ClearOccupancy(memory, cpu);
    case SECONDARY_OP_FF_MAP_CELL_TEST:
        return SecondaryOpFFMapCellTest(memory, cpu);
    case SECONDARY_OP_F2_SIGNED_OFFSET:
        return SecondaryOpF2SignedOffset(memory, cpu);
    case SECONDARY_OP_F6_ORBIT:
        return SecondaryOpF6Orbit(memory, cpu);
    case SECONDARY_OP_F7_ORBIT:
        return SecondaryOpF7Orbit(memory, cpu);
    case SECONDARY_OP_F8_ORBIT:
        return SecondaryOpF8Orbit(memory, cpu);
    case SECONDARY_OP_CX_SPAWN:
        return SecondaryOpCXSpawn(memory, cpu);
    case SECONDARY_OP_FD_SPAWN_AT_ACTOR:
        return SecondaryOpFDSpawnAtActor(memory, cpu);
    case SECONDARY_OP_D4_SPAWN_AT_ACTOR_SKIP:
        return SecondaryOpD4SpawnAtActorSkip(memory, cpu);
    case SECONDARY_OP_EB_SPAWN_CHILD:
        return SecondaryOpEBSpawnChild(memory, cpu);
    case SECONDARY_OP_6X_SPRITE_RELOAD:
        return SecondaryOp6XSpriteReload(memory, cpu);
    case SECONDARY_OP_F0_WALK:
        return SecondaryOpF0Walk(memory, cpu);
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
