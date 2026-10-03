/* Primary actor script VM ($83:C7F8). */

#include <stdbool.h>
#include <stddef.h>

#include "actor/actor_internal.h"
#include "core/cpu_internal.h"
#include "lufia2/actor.h"
#include "lufia2/system.h"
#include "system/dp_scratch.h"
#include "system/system_internal.h"
#include "system/wram.h"

/* Script tables and bases. Jump operands are offsets from PRIMARY_JUMP_BASE. */
#define PRIMARY_DISPATCH_TABLE 0xd467u /* handler pointer per opcode */
#define PRIMARY_JUMP_BASE 0xa1d4u
#define PRIMARY_JUMP_BASE_HIGH 0xf000u /* opcode $29 */
#define PRIMARY_PC_MASK 0x00ffffffu
#define PRIMARY_STEP_LIMIT 0x10000u /* handlers per update before a handoff */
#define PRIMARY_YIELD_PC 0x83c8d2u  /* handler end: the script gives up the frame */
#define ROM_PRIMARY_SCRIPT_OFFSETS 0x91a1d4u /* script offset word per actor id */
#define ROM_BLOCKED_EVENT_SCRIPT 0x918f2bu
#define BLOCKED_EVENT_SCRIPT_BASE 0x8ec7u
/* Section header at $7E:F000: flag, x, y records; $FF ends. */
#define PRIMARY_MAP_LIST 0x7ef000u
#define PRIMARY_MAP_LIST_X 0x7ef001u
#define PRIMARY_MAP_LIST_Y 0x7ef002u
/* Second byte of the looked-up map cell word. */
#define PRIMARY_MAP_CELL_ATTRIBUTES 0x7f0001u
/* Step tables in the program bank, by direction or facing. */
#define ROM_FLEE_ACTION_TABLE 0x83cd2au
#define ROM_FACING_ACTION_TABLE 0x83c1a5u
#define ROM_FACING_STEP_TABLE 0x83c1b0u
#define ROM_AHEAD_STEP_TABLE 0x83ce6bu
#define ROM_ACTION_NIBBLE_TABLE_A 0x83d447u
#define ROM_ACTION_NIBBLE_TABLE_B 0x83d457u

/* Primary handler DP cells; $54-$56 are shared scratch. */
enum {
    DP_SCRIPT_CURSOR = 0x2a,   /* offset of the next script byte */
    DP_RADIUS = 0x54,          /* half extent of a search box */
    DP_DIAMETER = 0x55,        /* full extent, twice the radius */
    DP_INTERVAL_VALUE = 0x54,  /* interval test: the value under test */
    DP_INTERVAL_START = 0x55,  /* interval test: lower limit */
    DP_INTERVAL_LENGTH = 0x56, /* interval test: width of the accepted range */
    DP_SEARCH_MIN_X = 0x9f,    /* search box corners, tile coordinates */
    DP_SEARCH_MIN_Y = 0xa0,
    DP_SEARCH_MAX_X = 0xa1,
    DP_SEARCH_MAX_Y = 0xa2
};

/* Clear flags $14, load the script, dispatch its opcode. */
Lufia2ActorScriptDispatchResult Lufia2ActorPrimaryScriptDispatch(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ActorScriptDispatchResult result;
    uint16_t actor_record;

    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_FLAGS, cpu->x)));
    And8(cpu, 0xebu);
    Write8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_FLAGS, cpu->x), A8(cpu));
    PushDataBank(memory, cpu);
    SetIndexWidth(cpu, 0);
    LoadXDirect16(memory, cpu, DP_SLOT_RECORD_OFFSET);
    actor_record = cpu->x;

    LoadA8(cpu, Read8(memory, LongIndexedAddress((WRAM_ACTOR_PRIMARY_SCRIPT + 2u),
                                                 actor_record)));
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_ACTOR_PRIMARY_SCRIPT,
        actor_record)));
    Write16Direct(memory, cpu, DP_SCRIPT_CURSOR, cpu->accumulator);
    TransferAToY(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadYDirect16(memory, cpu, DP_SCRIPT_CURSOR);
    StoreYDirect16(memory, cpu, DP_SCRIPT_CURSOR);
    TransferDirectToA(cpu);
    result.opcode = LoadScriptByteY(memory, cpu);
    AslA8(cpu);
    TransferAToX(cpu);
    result.handler_pc = JumpProgramTable(memory, cpu, PRIMARY_DISPATCH_TABLE);
    return result;
}

/* Interval test on A against DP $55/$56; carry answers. */
static void PrimaryIntervalCheck(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);

    Write8(memory, DirectAddress(cpu, DP_INTERVAL_VALUE), A8(cpu)); /* $83:CCC1 */
    cpu->carry = 0;                                     /* $83:CCC3 */
    Sbc8(cpu, Read8(memory, DirectAddress(cpu, DP_INTERVAL_START)));
    /* $83:CCC4 */
    Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_INTERVAL_LENGTH)));
    /* $83:CCC6 */
    if (!cpu->carry) {
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_INTERVAL_VALUE)));
        /* $83:CCCA */
        cpu->carry = 0;                                 /* $83:CCCC */
        Adc8(cpu, Read8(memory, DirectAddress(cpu, DP_INTERVAL_START)));
        /* $83:CCCD */
        Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_INTERVAL_LENGTH)));
        /* $83:CCCF */
        cpu->carry = cpu->carry ? 0u : 1u;             /* CLC/SEC */
    } else {
        cpu->carry = 1;                                /* $83:CCD5 SEC */
    }

    SimulateRtsFrame(memory, cpu);
}

/* Read the next script byte and dispatch, like $83:C85A. */
static Lufia2ActorScriptDispatchResult PrimaryRedispatch(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t reload_y) {
    Lufia2ActorScriptDispatchResult result;

    if (reload_y)
        LoadYDirect16(memory, cpu, DP_SCRIPT_CURSOR); /* $83:C85A */
    StoreYDirect16(memory, cpu, DP_SCRIPT_CURSOR);    /* $83:C85C */
    TransferDirectToA(cpu);                           /* $83:C85E */
    result.opcode = LoadScriptByteY(memory, cpu);     /* $83:C85F */
    AslA8(cpu);                                       /* $83:C862 */
    TransferAToX(cpu);                                /* $83:C863 */
    result.handler_pc =
        JumpProgramTable(memory, cpu, PRIMARY_DISPATCH_TABLE); /* $83:C864 */
    return result;
}

/* Dispatch result as a step that continues. */
static Lufia2ActorPrimaryScriptStepResult PrimaryStepRedispatched(
    Lufia2ActorScriptDispatchResult dispatch) {
    Lufia2ActorPrimaryScriptStepResult result;
    result.flow = LUFIA2_ACTOR_PRIMARY_SCRIPT_REDISPATCHED;
    result.opcode = dispatch.opcode;
    result.handler_pc = dispatch.handler_pc;
    return result;
}

/* $83:CBEC/$83:CBFF: leader within radius on one axis. */
static uint8_t PrimaryLeaderWithinRadius(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t coordinate_base) {
    const uint32_t leader = AbsoluteIndexedAddress(cpu, coordinate_base, 0);

    LoadA8(
        cpu, Read8(
            memory,
            AbsoluteIndexedAddress(cpu, coordinate_base, cpu->x)));
    cpu->carry = 0;
    Sbc8(cpu, Read8(memory, DirectAddress(cpu, DP_RADIUS)));
    Compare8(cpu, A8(cpu), Read8(memory, leader));
    if (cpu->carry)
        return 0;
    cpu->carry = 1;
    Adc8(cpu, Read8(memory, DirectAddress(cpu, DP_DIAMETER)));
    Compare8(cpu, A8(cpu), Read8(memory, leader));
    return cpu->carry;
}

/* $83:CC4E/$83:CC70: direction toward the leader. */
static void PrimaryLeaderDirection(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t coordinate_base,
    uint8_t action_if_ahead,
    uint8_t action_if_behind) {
    uint8_t ahead;

    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    TransferDirectToA(cpu);
    LoadA8(
        cpu, Read8(
            memory,
            AbsoluteIndexedAddress(cpu, coordinate_base, cpu->x)));
    Compare8(
        cpu, A8(cpu),
        Read8(memory, AbsoluteIndexedAddress(cpu, coordinate_base, 0)));
    if (cpu->zero) {
        cpu->carry = 0;
        return;
    }
    ahead = cpu->carry;
    LoadA8(cpu, action_if_ahead);
    if (!ahead)
        LoadA8(cpu, action_if_behind);
    cpu->carry = 1;
}

/* $83:D27F: X = operand8 * $28 + $A7 via the multiplier. */
static void PrimaryTargetRecordIndex(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x01u, cpu->y)));
    Write8(memory, AbsoluteIndexedAddress(cpu, SNES_WRMPYA, 0), A8(cpu));
    LoadA8(cpu, 0x28u);
    Write8(memory, AbsoluteIndexedAddress(cpu, SNES_WRMPYB, 0), A8(cpu));
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_ACTOR_SLOT)));
    cpu->carry = 0;
    Adc8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, SNES_RDMPYL, 0)));
    TransferAToX(cpu);
    SimulateRtsFrame(memory, cpu);
}

/* 16-bit operand + $A1D4 into $2A, then C85A. */
static Lufia2ActorScriptDispatchResult PrimaryJumpOperand(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t operand_offset) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(
        cpu, Read16AbsoluteIndexed(memory, cpu, operand_offset, cpu->y));
    cpu->carry = 0;
    Add16Immediate(cpu, PRIMARY_JUMP_BASE);
    Write16Direct(memory, cpu, DP_SCRIPT_CURSOR, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    return PrimaryRedispatch(memory, cpu, 1);
}

/* D350 call; zero means unknown D350 path. */
#define PRIMARY_ACTION_OR_BOUNDARY(return_address)                  \
    do {                                                             \
        if (!PrimaryCallActionCore(memory, cpu, (return_address))) { \
            result.handler_pc = cpu->resume_pc;                      \
            return result;                                           \
        }                                                            \
    } while (0)

/* Action core under a JSL frame; false hands off. */
static uint8_t PrimaryCallActionCore(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    Lufia2ActorPrimaryActionFlow flow;

    SimulateJslFrame(memory, cpu, 0x83u, return_address);
    flow = Lufia2ActorPrimaryActionCore(memory, cpu);
    if (flow != LUFIA2_ACTOR_PRIMARY_ACTION_RETURN_D3AE)
        return 0;
    SimulateRtlFrame(memory, cpu);
    return 1;
}

/* $83:C0EF: leader position to $8F/$91. */
void Lufia2ActorLeaderToProbe(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_TILE_X, 0)));
    Write8(memory, DirectAddress(cpu, DP_PROBE_X), A8(cpu));
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_TILE_Y, 0)));
    Write8(memory, DirectAddress(cpu, DP_PROBE_Y), A8(cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $83:C99A: actor position to $8F/$91. */
static void PrimaryActorToProbe(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadA8(
        cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_TILE_X, cpu->x)));
    Write8(memory, DirectAddress(cpu, DP_PROBE_X), A8(cpu));
    LoadA8(
        cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_TILE_Y, cpu->x)));
    Write8(memory, DirectAddress(cpu, DP_PROBE_Y), A8(cpu));
    SimulateRtsFrame(memory, cpu);
}

/* Probe +/- $54 on both axes into $9F..$A2. */
static void PrimaryProbeBox(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t x_offset,
    uint8_t y_offset) {
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, x_offset)));
    cpu->carry = 1;
    Sbc8(cpu, Read8(memory, DirectAddress(cpu, DP_RADIUS)));
    Write8(memory, DirectAddress(cpu, DP_SEARCH_MIN_X), A8(cpu));
    cpu->carry = 0;
    Adc8(cpu, Read8(memory, DirectAddress(cpu, DP_DIAMETER)));
    Write8(memory, DirectAddress(cpu, DP_SEARCH_MAX_X), A8(cpu));
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, y_offset)));
    cpu->carry = 1;
    Sbc8(cpu, Read8(memory, DirectAddress(cpu, DP_RADIUS)));
    Write8(memory, DirectAddress(cpu, DP_SEARCH_MIN_Y), A8(cpu));
    cpu->carry = 0;
    Adc8(cpu, Read8(memory, DirectAddress(cpu, DP_DIAMETER)));
    Write8(memory, DirectAddress(cpu, DP_SEARCH_MAX_Y), A8(cpu));
}

/* $83:C9A7: radius box around $8F/$91. */
static void PrimaryRadiusBox(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_RADIUS)));
    AslA8(cpu);
    Write8(memory, DirectAddress(cpu, DP_DIAMETER), A8(cpu));
    PrimaryProbeBox(memory, cpu, DP_PROBE_X, DP_PROBE_Y);
    SimulateRtsFrame(memory, cpu);
}

/* $83:C9F9/$83:CA08: step toward probe on one axis. */
static void PrimaryProbeAxisDirection(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t coordinate_base,
    uint8_t probe_offset,
    uint8_t action_if_ahead,
    uint8_t action_if_behind,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadA8(
        cpu, Read8(
            memory, AbsoluteIndexedAddress(cpu, coordinate_base, cpu->x)));
    Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, probe_offset)));
    if (cpu->zero) {
        cpu->carry = 0;                                        /* CA17 */
    } else {
        const uint8_t ahead = cpu->carry;
        LoadA8(cpu, ahead ? action_if_ahead : action_if_behind);
        cpu->carry = 1;
    }
    SimulateRtsFrame(memory, cpu);
}

/* $83:C9C5: step toward $8F/$91; $7F:E5A6 picks axis order. */
static void PrimaryApproachProbe(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);                   /* C9C5 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_ACTOR_CLAIMED_OBJECT_RECORD,
        cpu->x)));
    if (!cpu->zero) {
        PrimaryProbeAxisDirection(
            memory, cpu, WRAM_ACTOR_TILE_Y, DP_PROBE_Y, 0x00u, 0x01u, 0xc9cfu);
        if (!cpu->carry)
            PrimaryProbeAxisDirection(
                memory, cpu, WRAM_ACTOR_TILE_X, DP_PROBE_X, 0x02u, 0x03u, 0xc9d4u);
        if (cpu->carry) {
            ExchangeAccumulatorBytes(cpu);                     /* C9D7 */
            LoadA8(cpu, 0x00u);
            Write8(memory, LongIndexedAddress(WRAM_ACTOR_CLAIMED_OBJECT_RECORD, cpu->x),
                0x00u);
            ExchangeAccumulatorBytes(cpu);
            cpu->carry = 1;
        }
    } else {
        PrimaryProbeAxisDirection(
            memory, cpu, WRAM_ACTOR_TILE_X, DP_PROBE_X, 0x02u, 0x03u, 0xc9e4u);
        if (!cpu->carry)
            PrimaryProbeAxisDirection(
                memory, cpu, WRAM_ACTOR_TILE_Y, DP_PROBE_Y, 0x00u, 0x01u, 0xc9e9u);
        if (cpu->carry) {
            ExchangeAccumulatorBytes(cpu);                     /* C9EC */
            LoadA8(
                cpu, Read8(memory, LongIndexedAddress(WRAM_ACTOR_CLAIMED_OBJECT_RECORD,
                    cpu->x)));
            DecrementA8(cpu);
            Write8(
                memory, LongIndexedAddress(WRAM_ACTOR_CLAIMED_OBJECT_RECORD, cpu->x),
                    A8(cpu));
            ExchangeAccumulatorBytes(cpu);
            cpu->carry = 1;
        }
    }
    SimulateRtsFrame(memory, cpu);
}

/* $83:D0CB-$83:D0EB: listed point X inside the search box? */
static bool PrimaryListedPointInBox(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadA8(cpu, Read8(memory, LongIndexedAddress(PRIMARY_MAP_LIST_X, cpu->x)));
    Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_SEARCH_MIN_X)));
    if (!cpu->carry)
        return false;
    Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_SEARCH_MAX_X)));
    if (cpu->carry)
        return false;
    LoadA8(cpu, Read8(memory, LongIndexedAddress(PRIMARY_MAP_LIST_Y, cpu->x)));
    Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_SEARCH_MIN_Y)));
    if (!cpu->carry)
        return false;
    DecrementA8(cpu); /* D0E3 */
    Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_SEARCH_MAX_Y)));
    if (cpu->carry)
        return false;

    return true;
}

/* $83:CFE0-$83:D004: another live actor inside the search box? */
static bool PrimaryActorInSearchBox(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_UNK_7E05D2, cpu->x)));
    Compare8(cpu, A8(cpu), 0xffu); /* $83:CFE3 */
    if (cpu->zero)
        return false;
    Compare8(cpu, A8(cpu), 0x80u); /* $83:CFE7 */
    if (!cpu->carry)
        return false;
    Compare16(cpu, cpu->x, Read16Direct(memory, cpu, DP_ACTOR_SLOT));
    if (cpu->zero) /* $83:CFEB */
        return false;
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_TILE_X, cpu->x)));
    Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_SEARCH_MIN_X)));
    if (!cpu->carry)
        return false;
    Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_SEARCH_MAX_X)));
    if (cpu->carry)
        return false;
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_TILE_Y, cpu->x)));
    Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_SEARCH_MIN_Y)));
    if (!cpu->carry)
        return false;
    DecrementA8(cpu); /* $83:D001 */
    Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_SEARCH_MAX_Y)));
    if (cpu->carry)
        return false;

    return true;
}

typedef enum PrimaryListSearch {
    PRIMARY_LIST_STEPPED = 0,
    PRIMARY_LIST_EXHAUSTED = 1,
    PRIMARY_LIST_BOUNDARY = 2,
} PrimaryListSearch;

/* $83:D0AA: first steppable listed point in radius. */
static PrimaryListSearch PrimaryApproachListedPoint(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    uint32_t guard;

    SimulateJsrFrame(memory, cpu, return_address);
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_E), A8(cpu)); /* D0AA */
    Write8(memory, DirectAddress(cpu, 0x59u), 0x00u);          /* D0AC */
    PushIndex(memory, cpu);                                    /* D0AE */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x01u, cpu->y)));
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_A), A8(cpu)); /* D0B2 */
    PrimaryActorToProbe(memory, cpu, 0xd0b6u);                 /* D0B4 */
    PrimaryRadiusBox(memory, cpu, 0xd0b9u);                    /* D0B7 */
    cpu->x = PullIndexValue(memory, cpu);                      /* D0BA */
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_C), 0x00u);   /* D0BB */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(PRIMARY_MAP_LIST_X, cpu->x)));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(PRIMARY_MAP_LIST, cpu->x)));
    TransferAToX(cpu);                                         /* D0C6 */

    for (guard = 0;; ++guard) {
        if (guard >= PRIMARY_STEP_LIMIT) {
            cpu->resume_pc = 0x83d0c7u;
            return PRIMARY_LIST_BOUNDARY;
        }
        LoadA8(cpu, Read8(memory, LongIndexedAddress(PRIMARY_MAP_LIST, cpu->x)));
        Compare8(cpu, A8(cpu), 0xffu);                         /* D0CB */
        if (cpu->zero) {
            IncrementY16(cpu);                                 /* D10E */
            cpu->carry = 1;
            SimulateRtsFrame(memory, cpu);
            return PRIMARY_LIST_EXHAUSTED;
        }
        if (PrimaryListedPointInBox(memory, cpu)) {
            LoadA8(cpu, Read8(memory, LongIndexedAddress(PRIMARY_MAP_LIST_X, cpu->x)));
            Write8(memory, DirectAddress(cpu, DP_PROBE_X), A8(cpu)); /* D0F7 */
            LoadA8(cpu, Read8(memory, LongIndexedAddress(PRIMARY_MAP_LIST_Y, cpu->x)));
            Write8(memory, DirectAddress(cpu, DP_PROBE_Y), A8(cpu)); /* D0FD */
            PrimaryApproachProbe(memory, cpu, 0xd101u);              /* D0FF */
            if (cpu->carry) {
                if (!PrimaryCallActionCore(memory, cpu, 0xd107u)) /* D104 */
                    return PRIMARY_LIST_BOUNDARY;
                IncrementY16(cpu); /* D108 */
                IncrementY16(cpu);
                IncrementY16(cpu);
                IncrementY16(cpu);
                cpu->carry = 0;
                SimulateRtsFrame(memory, cpu);
                return PRIMARY_LIST_STEPPED;
            }
        }
        SetAccumulatorWidth(cpu, 0);                           /* D0E8 */
        LoadA16(cpu, cpu->x);
        cpu->carry = 0;
        Add16Value(cpu, Read16Direct(memory, cpu, DP_SCRATCH_E));
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
    }
}

/* JSR $D89E. */
static uint8_t PrimaryStepBlocked(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    if (!Lufia2ActorStepBlockedBody(memory, cpu))
        return 0;
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* $83:CDF6: clear same-height run length into $9D. */
static uint8_t PrimaryMeasureRun(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    uint32_t guard;

    SimulateJsrFrame(memory, cpu, return_address);
    Lufia2MapTileHeight(memory, cpu, 0xcdf8u);                 /* CDF6 */
    Write8(memory, DirectAddress(cpu, 0x99u), A8(cpu));
    Write8(memory, DirectAddress(cpu, 0x9du), 0x00u);          /* CDFB */

    for (guard = 0;; ++guard) {
        if (guard >= PRIMARY_STEP_LIMIT) {
            cpu->resume_pc = 0x83cdfdu;
            return 0;
        }
        CopyDirect8(memory, cpu, DP_PROBE_X, 0x95u);           /* CDFD */
        CopyDirect8(memory, cpu, DP_PROBE_Y, 0x96u);
        TransferDirectToA(cpu);                                /* CE05 */
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x94u)));
        if (!PrimaryStepBlocked(memory, cpu, 0xce0au))         /* CE08 */
            return 0;
        if (!cpu->zero)
            break;
        CopyDirect8(memory, cpu, 0x95u, DP_PROBE_X);                /* CE0D */
        CopyDirect8(memory, cpu, 0x96u, DP_PROBE_Y);
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x94u)));
        SimulateJslFrame(memory, cpu, 0x83u, 0xce1au);         /* CE17 */
        if (Lufia2ActorMovementStep(memory, cpu) == 0)
            return 0;
        SimulateRtlFrame(memory, cpu);
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_PROBE_X))); /* CE1B */
        Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_SEARCH_MIN_X)));
        if (!cpu->carry)
            break;
        Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_SEARCH_MAX_X)));
        if (cpu->carry)
            break;
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_PROBE_Y)));
        Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_SEARCH_MIN_Y)));
        if (!cpu->carry)
            break;
        Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_SEARCH_MAX_Y)));
        if (cpu->carry)
            break;
        Lufia2MapTileHeight(memory, cpu, 0xce31u);             /* CE2F */
        Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x99u)));
        if (!cpu->zero)
            break;
        IncrementDirect8(memory, cpu, 0x9du);                  /* CE36 */
    }
    SimulateRtsFrame(memory, cpu);                             /* CE3A */
    return 1;
}

/* $83:CE3B: walk A steps (0 = 256) in direction $94. */
static uint8_t PrimaryWalkSteps(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    uint16_t target;

    SimulateJsrFrame(memory, cpu, return_address);
    PushAccumulator8(memory, cpu);                             /* CE3B */
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x94u)));
    TransferAToX(cpu);                                         /* CE3F */
    LoadA8(cpu, Pull8(memory, cpu));                           /* CE40 */
    target = Read16ProgramIndexed(memory, cpu, 0xce48u, cpu->x);
    if (target != 0xce50u && target != 0xce53u &&
        target != 0xce56u && target != 0xce59u) {
        cpu->resume_pc = 0x83ce41u;
        return 0;
    }
    do {
        SimulateJsrFrame(memory, cpu, 0xce43u);                /* CE41 */
        if (target == 0xce50u)
            IncrementDirect8(memory, cpu, DP_PROBE_Y);
        else if (target == 0xce53u)
            DecrementDirect8(memory, cpu, DP_PROBE_X);
        else if (target == 0xce56u)
            DecrementDirect8(memory, cpu, DP_PROBE_Y);
        else
            IncrementDirect8(memory, cpu, DP_PROBE_X);
        SimulateRtsFrame(memory, cpu);
        DecrementA8(cpu);                                      /* CE44 */
    } while (!cpu->zero);
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* $83:CE5C: $8F/$91 to $7F:DB4C/$7F:DB4D + $A9. */
static void PrimaryRecordProbe(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_PROBE_X)));
    Write8(memory, LongIndexedAddress(WRAM_ACTOR_SCRATCH_A, cpu->x), A8(cpu));
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_PROBE_Y)));
    Write8(memory, LongIndexedAddress((WRAM_ACTOR_SCRATCH_A + 1u), cpu->x), A8(cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $83:CF11: origin $63/$64 back to $8F/$91. */
static void PrimaryResetProbe(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    CopyDirect8(memory, cpu, 0x63u, DP_PROBE_X);
    CopyDirect8(memory, cpu, 0x64u, DP_PROBE_Y);
    SimulateRtsFrame(memory, cpu);
}

/* $83:CEA8: random walk of $66..$65 steps on one axis. */
static uint8_t PrimaryWanderAxis(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_A), A8(cpu)); /* CEA8 */
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_ACTOR_BOX_MIN_X, cpu->x)));
    Write8(memory, DirectAddress(cpu, DP_SEARCH_MIN_X), A8(cpu));
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_ACTOR_BOX_MIN_Y, cpu->x)));
    Write8(memory, DirectAddress(cpu, DP_SEARCH_MIN_Y), A8(cpu));
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_ACTOR_BOX_MAX_X, cpu->x)));
    Write8(memory, DirectAddress(cpu, DP_SEARCH_MAX_X), A8(cpu));
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_ACTOR_BOX_MAX_Y, cpu->x)));
    Write8(memory, DirectAddress(cpu, DP_SEARCH_MAX_Y), A8(cpu));
    LoadA8(cpu, 0x02u);                                        /* CEC4 */
    Lufia2CallRandomScale(memory, cpu, 0xcec9u);
    AslA8(cpu);                                                /* CECA */
    AslA8(cpu);
    Adc8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_A)));
    Write8(memory, DirectAddress(cpu, 0x94u), A8(cpu));
    PrimaryResetProbe(memory, cpu, 0xced2u);                   /* CED0 */
    if (!PrimaryMeasureRun(memory, cpu, 0xced5u))
        return 0;
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x9du)));     /* CED6 */
    Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x66u)));
    if (!cpu->carry) {
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x94u))); /* CEDC */
        cpu->carry = 0;
        Adc8(cpu, 0x04u);
        And8(cpu, 0x06u);
        Write8(memory, DirectAddress(cpu, 0x94u), A8(cpu));
        PrimaryResetProbe(memory, cpu, 0xcee7u);
        if (!PrimaryMeasureRun(memory, cpu, 0xceeau))
            return 0;
    }
    PrimaryResetProbe(memory, cpu, 0xceedu);                   /* CEEB */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x9du)));
    Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x66u)));
    if (cpu->carry) {
        Compare8(
            cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x65u)));
        if (cpu->carry) {
            LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x65u)));
            cpu->carry = 1;                                    /* CEFA */
            Sbc8(cpu, Read8(memory, DirectAddress(cpu, 0x66u)));
        }
        DecrementA8(cpu);                                      /* CEFD */
        Lufia2CallRandomScale(memory, cpu, 0xcf01u);
        cpu->carry = 0;                                        /* CF02 */
        Adc8(cpu, Read8(memory, DirectAddress(cpu, 0x66u)));
        if (!PrimaryWalkSteps(memory, cpu, 0xcf07u))
            return 0;
        CopyDirect8(memory, cpu, DP_PROBE_X, 0x63u);           /* CF08 */
        CopyDirect8(memory, cpu, DP_PROBE_Y, 0x64u);
    }
    SimulateRtsFrame(memory, cpu);                             /* CF10 */
    return 1;
}

/* $83:D416: point the primary script at the actor's $070A entry. */
void Lufia2ActorLoadPrimaryScript(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);                   /* D416 */
    TransferDirectToA(cpu);
    LoadAAbsolute8(memory, cpu, WRAM_UNK_7E070A, cpu->x);
    AslA8(cpu);
    TransferAToX(cpu);                                         /* D41D */
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu,
            Read16Long(memory, LongIndexedAddress(ROM_PRIMARY_SCRIPT_OFFSETS, cpu->x)));
    LoadXDirect(memory, cpu, DP_SLOT_RECORD_OFFSET); /* D424 */
    cpu->carry = 0;
    Add16Immediate(cpu, PRIMARY_JUMP_BASE);
    Write16Long(
        memory, LongIndexedAddress(WRAM_ACTOR_PRIMARY_SCRIPT, cpu->x),
            cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);                               /* D42E */
    LoadA8(cpu, 0x91u);
    Write8(memory, LongIndexedAddress((WRAM_ACTOR_PRIMARY_SCRIPT + 2u), cpu->x),
           A8(cpu));
}

/* $83:C947: mark occupancy, reset flags, reload the script. */
void Lufia2ActorPrimaryReset(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJslFrame(memory, cpu, 0x83u, 0xc94au);             /* C947 */
    Lufia2ActorMarkMapOccupancy(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);                   /* C94B */
    if (cpu->zero) {
        LoadA8(cpu, 0x02u);                                    /* C94F */
        TestBitsAbsolute8(memory, cpu, WRAM_ACTOR_STATE, 0);
        LoadAAbsolute8(memory, cpu, WRAM_WINDOW_MODE, 0);
        And8(cpu, 0x01u);
        if (cpu->zero) {
            LoadA8(cpu, 0x01u);                                /* C95B */
            TestBitsAbsolute8(memory, cpu, WRAM_UNK_7E09A6, 0);
        }
    }
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->x);     /* C960 */
    And8(cpu, 0xf7u);
    StoreAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->x);
    LoadAAbsolute8(memory, cpu, WRAM_UNK_7E05D2, cpu->x);
    Compare8(cpu, A8(cpu), 0x0au);                             /* C96B */
    if (cpu->carry) {
        LoadAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->x);
        And8(cpu, 0xfdu);
        StoreAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->x);
    }
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_FLAGS, cpu->x);              /* C977 */
    And8(cpu, 0xfdu);
    StoreAAbsolute8(memory, cpu, WRAM_ACTOR_FLAGS, cpu->x);
    LoadA8(cpu, 0x08u);
    Write8(memory, LongIndexedAddress(WRAM_UNK_7FE4DE, cpu->x), A8(cpu));
    SimulateJslFrame(memory, cpu, 0x83u, 0xc988u);             /* C985 */
    Lufia2ActorLoadPrimaryScript(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $83:CB65: sets the five follow-slot bytes to $FF. */
void Lufia2ActorClearSlotLinks(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadX16(cpu, 0x04u); /* CB65 */
    LoadA8(cpu, 0xffu);
    do {
        StoreAAbsolute8(memory, cpu, WRAM_FOLLOW_SLOTS, cpu->x);
        LoadX16(cpu, (uint16_t)(cpu->x - 1u));
    } while (!cpu->negative);
}

/* $83:CA68: start the blocked-event script unless control bit $40. */
void Lufia2ActorBlockedEvent(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA8(cpu, Read8(memory, WRAM_FIELD_CONTROL_FLAGS));                     /* CA68 */
    BitImmediate8(cpu, 0x40u);
    if (!cpu->zero)
        return;
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_BLOCKED_EVENT_OBJECT, 0));
    LoadA8(cpu, 0x01u);                                        /* CA73 */
    Write8(memory, LongIndexedAddress(WRAM_UNK_7FDFAE, cpu->x), A8(cpu));
    SetAccumulatorWidth(cpu, 0);                               /* CA79 */
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_BLOCKED_EVENT_OBJECT, 0));
    Write16Direct(memory, cpu, DP_SCRATCH_C, cpu->accumulator);
    AslA16(cpu);
    Add16Value(cpu, Read16Direct(memory, cpu, DP_SCRATCH_C)); /* CA81 */
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, ROM_BLOCKED_EVENT_SCRIPT));
    cpu->carry = 0;
    Add16Immediate(cpu, BLOCKED_EVENT_SCRIPT_BASE);
    Write16Long(
        memory, LongIndexedAddress(WRAM_OBJECT_SCRIPT, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);                               /* CA90 */
}

/* Primary script handlers, targets of JMP ($D467,x). */
enum PrimaryOpcodeHandler {
    PRIMARY_OP_ACTION_0 = 0x83c867,            /* $00: action 0: step up */
    PRIMARY_OP_ACTION_1 = 0x83c86b,            /* $01: action 1: step down */
    PRIMARY_OP_ACTION_2 = 0x83c86f,            /* $02: action 2: step left */
    PRIMARY_OP_ACTION_3 = 0x83c873,            /* $03: action 3: step right */
    PRIMARY_OP_ACTION_FROM_OPERAND = 0x83c877, /* $4F */
    PRIMARY_OP_ACTION_81 = 0x83c87c,           /* $4A */
    PRIMARY_OP_ACTION_82 = 0x83c880,           /* $4B */
    PRIMARY_OP_ACTION_83 = 0x83c884,           /* $4C */
    PRIMARY_OP_ACTION_84 = 0x83c888,           /* $4D */
    PRIMARY_OP_INSTALL_SECONDARY = 0x83c891,   /* $08: secondary script operand + $18 */
    PRIMARY_OP_RANDOM_TIMER = 0x83c8af,        /* $09: timer = random(n) + n */
    PRIMARY_OP_COMMIT_CURSOR = 0x83c8c7,       /* $0A: store the script cursor, exit */
    PRIMARY_OP_RANDOM_TIMER_X8 = 0x83c8d4,     /* $0B: timer = (random(n) + n) * 8 */
    PRIMARY_OP_STORE_E4DE = 0x83c8ee,          /* $0C: operand into $7F:E4DE */
    PRIMARY_OP_SET_FLAG_1 = 0x83c8fc,          /* $0D: set $0736 bit 1 */
    PRIMARY_OP_CLEAR_FLAG_1 = 0x83c90a,        /* $0E: clear $0736 bit 1 */
    PRIMARY_OP_RESET = 0x83c918,               /* $0F $1E: reset via $83:C947 */
    PRIMARY_OP_STEP_TOWARD_LEADER = 0x83c98a,  /* $10 */
    PRIMARY_OP_STEP_TOWARD_LISTED_POINT = 0x83ca19, /* $11 */
    PRIMARY_OP_STEP_TOWARD_TARGET = 0x83ca29, /* $2E: blocked event via $83:CA68 */
    PRIMARY_OP_TIMED_STEP_TOWARD_TARGET = 0x83cab9, /* $47: $7F:E5A6/E5CE */
    PRIMARY_OP_RESET_48 = 0x83cad3,                 /* $48 */
    PRIMARY_OP_RESET_2F = 0x83cbb1,                 /* $2F */
    PRIMARY_OP_MAP_CELL_30_TEST = 0x83cbb7,         /* $30: flag $0736 bit 6 or skip */
    PRIMARY_OP_JUMP_IF_LEADER_NEAR = 0x83cbe1,      /* $31 */
    PRIMARY_OP_JUMP_IF_LEADER_X = 0x83cc1b,         /* $32 */
    PRIMARY_OP_JUMP_IF_LEADER_Y = 0x83cc2e,         /* $33 */
    PRIMARY_OP_STEP_TOWARD_LEADER_X = 0x83cc41,     /* $34 */
    PRIMARY_OP_STEP_TOWARD_LEADER_Y = 0x83cc63,     /* $35 */
    PRIMARY_OP_JUMP_IF_X_IN_RANGE = 0x83cc85,       /* $36 */
    PRIMARY_OP_JUMP_IF_Y_IN_RANGE = 0x83cca3,       /* $37 */
    PRIMARY_OP_STEP_AWAY_FROM_LEADER = 0x83ccd7,    /* $38 */
    PRIMARY_OP_STEP_AWAY_LEADER_X = 0x83ccf0,       /* $39: random when level */
    PRIMARY_OP_STEP_AWAY_LEADER_Y = 0x83cd0d,       /* $3A: random when level */
    PRIMARY_OP_COMMIT_CURSOR_3B = 0x83cd2e,         /* $3B */
    PRIMARY_OP_ROTATE_FACING = 0x83cd32,            /* $3C */
    PRIMARY_OP_JUMP_IF_LEADER_AHEAD = 0x83cd4f,     /* $3D */
    PRIMARY_OP_ACTION_FROM_D457 = 0x83cd92,     /* $3E: $47 low nibble via $83:D457 */
    PRIMARY_OP_WALK_AHEAD_OF_LEADER = 0x83cda5, /* $3F */
    PRIMARY_OP_ACTION_60 = 0x83ce73,            /* $40 */
    PRIMARY_OP_WANDER = 0x83ce7d,               /* $43 */
    PRIMARY_OP_WANDER_IN_BOX = 0x83cf1a,        /* $41 */
    PRIMARY_OP_RECORD_POSITION = 0x83cf6e,   /* $42: action $5F, position to $7F:DB9C */
    PRIMARY_OP_STEP_TOWARD_POINT = 0x83cf8c, /* $12: or skip */
    PRIMARY_OP_FIND_ACTOR_IN_RADIUS = 0x83cfb9,        /* $44: slot to $7F:DB4C */
    PRIMARY_OP_STEP_TOWARD_FOUND_ACTOR = 0x83d01e,     /* $45 */
    PRIMARY_OP_JUMP_IF_BLOCKED = 0x83d03f,             /* $46 */
    PRIMARY_OP_STEP_TOWARD_LISTED_POINT_13 = 0x83d09a, /* $13 */
    PRIMARY_OP_ACTION_FROM_D447 = 0x83d112,     /* $15: $47 low nibble via $83:D447 */
    PRIMARY_OP_RANDOM_DIRECTION = 0x83d125,     /* $16 */
    PRIMARY_OP_COMMIT_CURSOR_17 = 0x83d132,     /* $17 */
    PRIMARY_OP_SET_TILE_POSITION = 0x83d135,    /* $18 */
    PRIMARY_OP_CONDITIONAL_ACTION = 0x83d14d,   /* $19 */
    PRIMARY_OP_MERGE_1291 = 0x83d176,           /* $1A: low bits of $1291 */
    PRIMARY_OP_SET_TIMER = 0x83d188,            /* $1B */
    PRIMARY_OP_TARGET_AND_SECONDARY = 0x83d196, /* $1C: secondary script $1C */
    PRIMARY_OP_PLAY_SOUND = 0x83d1b5,           /* $1D: via $84:8766 */
    PRIMARY_OP_ACTION_IF_09A1_NEGATIVE = 0x83d1c1, /* $1F */
    PRIMARY_OP_JUMP_BY_LEADER_FACING = 0x83d1d0,   /* $2A */
    PRIMARY_OP_FACING_RELATIVE_ACTION = 0x83d1e6,  /* $2B: via $83:C1A5 */
    PRIMARY_OP_ADD_DISPLAY_OFFSET = 0x83d1fe,      /* $2C: via $83:FACB */
    PRIMARY_OP_MOVE_FINE_POSITION = 0x83d207,      /* $2D: via $83:FA81 */
    PRIMARY_OP_COMPARE_TARGET = 0x83d210,          /* $20: six compare modes */
    PRIMARY_OP_JUMP_UNLESS_MASK = 0x83d293,        /* $21: operand & $7F:E57E */
    PRIMARY_OP_JUMP = 0x83d2b4,                    /* $22: operand + $A1D4 */
    PRIMARY_OP_JUMP_ALIAS = 0x83d2bd,              /* $79: cursor low byte alias */
    PRIMARY_OP_OR_MASK = 0x83d2c4,                 /* $23: into $7F:E57E */
    PRIMARY_OP_AND_MASK = 0x83d2d5,                /* $24: into $7F:E57E */
    PRIMARY_OP_SET_TARGET = 0x83d2e6,              /* $25 */
    PRIMARY_OP_ADD_TARGET = 0x83d2f6,              /* $26 */
    PRIMARY_OP_SUBTRACT_TARGET = 0x83d30b,         /* $27 */
    PRIMARY_OP_RANDOM_JUMP = 0x83d320,             /* $28: random byte >= operand */
    PRIMARY_OP_JUMP_F000 = 0x83d340,               /* $29: operand + $F000 */
};

/* Handler stopped early: unknown path at handler_pc. */
static Lufia2ActorPrimaryScriptStepResult PrimaryStepStart(uint32_t handler_pc) {
    Lufia2ActorPrimaryScriptStepResult result;

    result.flow = LUFIA2_ACTOR_PRIMARY_SCRIPT_UNKNOWN_HANDLER;
    result.opcode = 0;
    result.handler_pc = handler_pc & PRIMARY_PC_MASK;
    return result;
}

/* Action id run by the fixed-action opcodes. */
static uint8_t PrimaryFixedActionId(uint32_t handler_pc) {
    switch (handler_pc & PRIMARY_PC_MASK) {
    case PRIMARY_OP_ACTION_1:
        return 0x01u;
    case PRIMARY_OP_ACTION_2:
        return 0x02u;
    case PRIMARY_OP_ACTION_3:
        return 0x03u;
    case PRIMARY_OP_ACTION_81:
        return 0x81u;
    case PRIMARY_OP_ACTION_82:
        return 0x82u;
    case PRIMARY_OP_ACTION_83:
        return 0x83u;
    case PRIMARY_OP_ACTION_84:
        return 0x84u;
    default:
        return 0x00u;
    }
}

/* Run this opcode's fixed action id through the action core. */
static Lufia2ActorPrimaryScriptStepResult PrimaryFixedAction(const Lufia2Memory *memory,
                                                             Lufia2CpuState *cpu,
                                                             uint32_t handler_pc) {
    Lufia2ActorPrimaryScriptStepResult result = PrimaryStepStart(handler_pc);

    LoadA8(cpu, PrimaryFixedActionId(handler_pc));
    if (!PrimaryCallActionCore(memory, cpu, 0xc88du)) {
        result.handler_pc = cpu->resume_pc;
        return result;
    }
    IncrementY16(cpu); /* $83:C8C6 */
    return Lufia2ActorPrimaryScriptExecuteKnownHandler(memory, cpu,
                                                       PRIMARY_OP_COMMIT_CURSOR);
}

/* $00: action 0: step up */
static Lufia2ActorPrimaryScriptStepResult PrimaryAction0(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu) {
    return PrimaryFixedAction(memory, cpu, PRIMARY_OP_ACTION_0);
}

/* $01: action 1: step down */
static Lufia2ActorPrimaryScriptStepResult PrimaryAction1(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu) {
    return PrimaryFixedAction(memory, cpu, PRIMARY_OP_ACTION_1);
}

/* $02: action 2: step left */
static Lufia2ActorPrimaryScriptStepResult PrimaryAction2(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu) {
    return PrimaryFixedAction(memory, cpu, PRIMARY_OP_ACTION_2);
}

/* $03: action 3: step right */
static Lufia2ActorPrimaryScriptStepResult PrimaryAction3(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu) {
    return PrimaryFixedAction(memory, cpu, PRIMARY_OP_ACTION_3);
}

/* $4A */
static Lufia2ActorPrimaryScriptStepResult PrimaryAction81(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu) {
    return PrimaryFixedAction(memory, cpu, PRIMARY_OP_ACTION_81);
}

/* $4B */
static Lufia2ActorPrimaryScriptStepResult PrimaryAction82(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu) {
    return PrimaryFixedAction(memory, cpu, PRIMARY_OP_ACTION_82);
}

/* $4C */
static Lufia2ActorPrimaryScriptStepResult PrimaryAction83(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu) {
    return PrimaryFixedAction(memory, cpu, PRIMARY_OP_ACTION_83);
}

/* $4D */
static Lufia2ActorPrimaryScriptStepResult PrimaryAction84(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu) {
    return PrimaryFixedAction(memory, cpu, PRIMARY_OP_ACTION_84);
}

/* $4F */
static Lufia2ActorPrimaryScriptStepResult
PrimaryActionFromOperand(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ActorPrimaryScriptStepResult result =
        PrimaryStepStart(PRIMARY_OP_ACTION_FROM_OPERAND);

    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x01u, cpu->y)));
    /* $83:C877 */
    if (!PrimaryCallActionCore(memory, cpu, 0xc88du)) {
        result.handler_pc = cpu->resume_pc;
        return result;
    }
    IncrementY16(cpu); /* $83:C8C6 */
    return Lufia2ActorPrimaryScriptExecuteKnownHandler(memory, cpu,
                                                       PRIMARY_OP_COMMIT_CURSOR);
}

/* $36 */
static Lufia2ActorPrimaryScriptStepResult
PrimaryJumpIfXInRange(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ActorScriptDispatchResult dispatch;

    IncrementY16(cpu);                         /* $83:CC85 */
    LoadXDirect16(memory, cpu, DP_ACTOR_SLOT); /* $83:CC86 */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_TILE_X, 0)));
    /* $83:CC88 */
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_C), A8(cpu));
    /* $83:CC8B */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x00u, cpu->y)));
    /* $83:CC8D */
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_B), A8(cpu));
    /* $83:CC90 */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_TILE_X, cpu->x)));
    /* $83:CC92 */
    PrimaryIntervalCheck(memory, cpu, 0xcc97u); /* JSR $CCC1 */
    if (!cpu->carry)
        return Lufia2ActorPrimaryScriptExecuteKnownHandler(
            memory, cpu, PRIMARY_OP_JUMP);        /* $83:CC9A */
    IncrementY16(cpu);                            /* $83:CC9D */
    IncrementY16(cpu);                            /* $83:CC9E */
    IncrementY16(cpu);                            /* $83:CC9F */
    dispatch = PrimaryRedispatch(memory, cpu, 0); /* $83:C85C */
    return PrimaryStepRedispatched(dispatch);
}

/* $37 */
static Lufia2ActorPrimaryScriptStepResult
PrimaryJumpIfYInRange(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ActorScriptDispatchResult dispatch;

    IncrementY16(cpu);                         /* $83:CCA3 */
    LoadXDirect16(memory, cpu, DP_ACTOR_SLOT); /* $83:CCA4 */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_TILE_Y, 0)));
    /* $83:CCA6 */
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_C), A8(cpu));
    /* $83:CCA9 */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x00u, cpu->y)));
    /* $83:CCAB */
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_B), A8(cpu));
    /* $83:CCAE */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_TILE_Y, cpu->x)));
    /* $83:CCB0 */
    PrimaryIntervalCheck(memory, cpu, 0xccb5u); /* JSR $CCC1 */
    if (!cpu->carry)
        return Lufia2ActorPrimaryScriptExecuteKnownHandler(
            memory, cpu, PRIMARY_OP_JUMP);        /* $83:CCB8 */
    IncrementY16(cpu);                            /* $83:CCBB */
    IncrementY16(cpu);                            /* $83:CCBC */
    IncrementY16(cpu);                            /* $83:CCBD */
    dispatch = PrimaryRedispatch(memory, cpu, 0); /* $83:C85C */
    return PrimaryStepRedispatched(dispatch);
}

/* $19 */
static Lufia2ActorPrimaryScriptStepResult
PrimaryConditionalAction(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ActorPrimaryScriptStepResult result =
        PrimaryStepStart(PRIMARY_OP_CONDITIONAL_ACTION);

    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_FOLLOW_SLOTS, 0)));
    /* $83:D14D */
    if (cpu->negative) { /* $83:D150 */
        LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_UNK_7E09A6, 0)));
        /* $83:D152 */
        BitImmediate8(cpu, 0x01u);                     /* $83:D155 */
        if (cpu->zero) {                               /* $83:D157 */
            LoadXDirect16(memory, cpu, DP_ACTOR_SLOT); /* $83:D159 */
            TransferDirectToA(cpu);                    /* $83:D15B */
            LoadA8(cpu, Read8(memory, LongIndexedAddress(
                                          WRAM_ACTOR_CLAIMED_OBJECT_RECORD, cpu->x)));
            /* $83:D15C */
            TransferAToX(cpu); /* $83:D160 */
            LoadA8(cpu, Read8(memory,
                              AbsoluteIndexedAddress(cpu, WRAM_FOLLOW_SLOTS, cpu->x)));
            /* $83:D161 */
            if (!cpu->negative) { /* $83:D164 */
                if (!PrimaryCallActionCore(memory, cpu, 0xd169u)) {
                    result.flow = LUFIA2_ACTOR_PRIMARY_SCRIPT_CONTINUE_D166;
                    result.handler_pc = cpu->resume_pc;
                    return result;
                }
                LoadA8(cpu, Read8(memory, WRAM_UNK_7FE4DE)); /* $83:D16A */
                Write8(memory, LongIndexedAddress(WRAM_UNK_7FE4DE, cpu->x),
                       A8(cpu)); /* $83:D16E */
            }
        }
    }

    IncrementY16(cpu); /* $83:D172 */
    return Lufia2ActorPrimaryScriptExecuteKnownHandler(
        memory, cpu, PRIMARY_OP_COMMIT_CURSOR); /* $83:D173 */
}

/* $0A: store the script cursor, exit */
static Lufia2ActorPrimaryScriptStepResult
PrimaryCommitCursor(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ActorPrimaryScriptStepResult result =
        PrimaryStepStart(PRIMARY_OP_COMMIT_CURSOR);

    LoadXDirect16(memory, cpu, DP_SLOT_RECORD_OFFSET); /* $83:C8C7 */
    SetAccumulatorWidth(cpu, 0);                       /* $83:C8C9 */
    LoadA16(cpu, cpu->y);                              /* $83:C8CB TYA */
    Write16Long(memory, LongIndexedAddress(WRAM_ACTOR_PRIMARY_SCRIPT, cpu->x),
                cpu->accumulator); /* $83:C8CC */
    SetAccumulatorWidth(cpu, 1);   /* $83:C8D0 */
    result.flow = LUFIA2_ACTOR_PRIMARY_SCRIPT_CONTINUE_C8D2;
    result.handler_pc = PRIMARY_YIELD_PC;
    return result;
}

/* $22: operand + $A1D4 */
static Lufia2ActorPrimaryScriptStepResult PrimaryJump(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu) {
    Lufia2ActorScriptDispatchResult dispatch;

    SetAccumulatorWidth(cpu, 0);                                     /* $83:D2B4 */
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x01u, cpu->y)); /* $83:D2B6 */
    cpu->carry = 0;                                                  /* $83:D2B9 CLC */
    Add16Immediate(cpu, PRIMARY_JUMP_BASE);                          /* $83:D2BA */
    Write16Direct(memory, cpu, DP_SCRIPT_CURSOR, cpu->accumulator);  /* $83:D2BD */
    SetAccumulatorWidth(cpu, 1);                                     /* $83:D2BF */
    dispatch = PrimaryRedispatch(memory, cpu, 1);                    /* $83:C85A */
    return PrimaryStepRedispatched(dispatch);
}

/* $79: cursor low byte alias */
static Lufia2ActorPrimaryScriptStepResult PrimaryJumpAlias(const Lufia2Memory *memory,
                                                           Lufia2CpuState *cpu) {
    Lufia2ActorScriptDispatchResult dispatch;

    Write8(memory, DirectAddress(cpu, DP_SCRIPT_CURSOR), A8(cpu));
    /* $83:D2BD */
    SetAccumulatorWidth(cpu, 1);                  /* $83:D2BF */
    dispatch = PrimaryRedispatch(memory, cpu, 1); /* $83:C85A */
    return PrimaryStepRedispatched(dispatch);
}

/* $23: into $7F:E57E */
static Lufia2ActorPrimaryScriptStepResult PrimaryOrMask(const Lufia2Memory *memory,
                                                        Lufia2CpuState *cpu) {
    Lufia2ActorScriptDispatchResult dispatch;

    LoadXDirect16(memory, cpu, DP_ACTOR_SLOT); /* $83:D2C4 */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x01u, cpu->y)));
    /* $83:D2C6 */
    Or8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FE57E, cpu->x)));
    /* $83:D2C9 */
    Write8(memory, LongIndexedAddress(WRAM_UNK_7FE57E, cpu->x), A8(cpu));
    /* $83:D2CD */
    IncrementY16(cpu);                            /* $83:D2D1 */
    dispatch = PrimaryRedispatch(memory, cpu, 0); /* $83:C85C */
    return PrimaryStepRedispatched(dispatch);
}

/* $24: into $7F:E57E */
static Lufia2ActorPrimaryScriptStepResult PrimaryAndMask(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu) {
    Lufia2ActorScriptDispatchResult dispatch;

    LoadXDirect16(memory, cpu, DP_ACTOR_SLOT); /* $83:D2D5 */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x01u, cpu->y)));
    /* $83:D2D7 */
    And8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FE57E, cpu->x)));
    /* $83:D2DA */
    Write8(memory, LongIndexedAddress(WRAM_UNK_7FE57E, cpu->x), A8(cpu));
    /* $83:D2DE */
    IncrementY16(cpu);                            /* $83:D2E2 */
    dispatch = PrimaryRedispatch(memory, cpu, 0); /* $83:C85C */
    return PrimaryStepRedispatched(dispatch);
}

/* $08: secondary script operand + $18 */
static Lufia2ActorPrimaryScriptStepResult
PrimaryInstallSecondary(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x01u, cpu->y)));
    /* $83:C891 */
    cpu->carry = 0;                          /* $83:C894 */
    Adc8(cpu, 0x18u);                        /* $83:C895 */
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT); /* $83:C897 */
    Write8(memory, LongIndexedAddress(WRAM_UNK_7FE466, cpu->x), A8(cpu));
    /* $83:C899 */
    SimulateJsrFrame(memory, cpu, 0xc89fu);         /* $83:C89D */
    Lufia2ActorInstallSecondaryScript(memory, cpu); /* $83:D3F7 */
    SimulateRtsFrame(memory, cpu);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT); /* $83:C8A0 */
    LoadA8(cpu, 0x80u);                      /* $83:C8A2 */
    Or8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_STATE, cpu->x)));
    /* $83:C8A4 */
    Write8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_STATE, cpu->x),
           A8(cpu));   /* $83:C8A7 */
    IncrementY16(cpu); /* $83:C8AA */
    IncrementY16(cpu); /* $83:C8AB */
    return Lufia2ActorPrimaryScriptExecuteKnownHandler(
        memory, cpu, PRIMARY_OP_COMMIT_CURSOR); /* $83:C8AC */
}

/* $09: timer = random(n) + n */
static Lufia2ActorPrimaryScriptStepResult PrimaryRandomTimer(const Lufia2Memory *memory,
                                                             Lufia2CpuState *cpu) {
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x01u, cpu->y)));
    /* $83:C8AF */
    Lufia2CallRandomScale(memory, cpu, 0xc8b5u); /* $83:C8B2 */
    cpu->carry = 0;                              /* $83:C8B6 */
    Adc8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x02u, cpu->y)));
    /* $83:C8B7 */
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT); /* $83:C8BA */
    Write8(memory, LongIndexedAddress(WRAM_ACTOR_PRIMARY_TIMER, cpu->x), A8(cpu));
    /* $83:C8BC */
    IncrementY16(cpu); /* $83:C8C0 */
    IncrementY16(cpu); /* $83:C8C1 */
    IncrementY16(cpu); /* $83:C8C2 */
    return Lufia2ActorPrimaryScriptExecuteKnownHandler(
        memory, cpu, PRIMARY_OP_COMMIT_CURSOR); /* $83:C8C3 */
}

/* $0B: timer = (random(n) + n) * 8 */
static Lufia2ActorPrimaryScriptStepResult
PrimaryRandomTimerX8(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT); /* $83:C8D4 */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x02u, cpu->y)));
    /* $83:C8D6 */
    Lufia2CallRandomScale(memory, cpu, 0xc8dcu); /* $83:C8D9 */
    cpu->carry = 0;                              /* $83:C8DD */
    Adc8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x01u, cpu->y)));
    /* $83:C8DE */
    AslA8(cpu); /* $83:C8E1 */
    AslA8(cpu); /* $83:C8E2 */
    AslA8(cpu); /* $83:C8E3 */
    Write8(memory, LongIndexedAddress(WRAM_ACTOR_PRIMARY_TIMER, cpu->x), A8(cpu));
    /* $83:C8E4 */
    IncrementY16(cpu); /* $83:C8E8 */
    IncrementY16(cpu); /* $83:C8E9 */
    IncrementY16(cpu); /* $83:C8EA */
    return Lufia2ActorPrimaryScriptExecuteKnownHandler(
        memory, cpu, PRIMARY_OP_COMMIT_CURSOR); /* $83:C8EB */
}

/* $0C: operand into $7F:E4DE */
static Lufia2ActorPrimaryScriptStepResult PrimaryStoreE4de(const Lufia2Memory *memory,
                                                           Lufia2CpuState *cpu) {
    Lufia2ActorScriptDispatchResult dispatch;

    LoadXDirect(memory, cpu, DP_ACTOR_SLOT); /* $83:C8EE */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x01u, cpu->y)));
    /* $83:C8F0 */
    Write8(memory, LongIndexedAddress(WRAM_UNK_7FE4DE, cpu->x), A8(cpu));
    /* $83:C8F3 */
    IncrementY16(cpu);                            /* $83:C8F7 */
    IncrementY16(cpu);                            /* $83:C8F8 */
    dispatch = PrimaryRedispatch(memory, cpu, 0); /* $83:C85C */
    return PrimaryStepRedispatched(dispatch);
}

static Lufia2ActorPrimaryScriptStepResult
PrimaryChangeActorFlag(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                       uint32_t handler_pc) {
    Lufia2ActorScriptDispatchResult dispatch;

    LoadXDirect(memory, cpu, DP_ACTOR_SLOT); /* $83:C8FC */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_FLAGS, cpu->x)));
    /* $83:C8FE */
    if ((handler_pc & PRIMARY_PC_MASK) == PRIMARY_OP_SET_FLAG_1)
        Or8(cpu, 0x02u); /* $83:C901 */
    else
        And8(cpu, 0xfdu); /* $83:C90F */
    Write8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_FLAGS, cpu->x),
           A8(cpu));                              /* $83:C903 */
    IncrementY16(cpu);                            /* $83:C906 */
    dispatch = PrimaryRedispatch(memory, cpu, 0); /* $83:C85C */
    return PrimaryStepRedispatched(dispatch);
}

/* $0D: set $0736 bit 1 */
static Lufia2ActorPrimaryScriptStepResult PrimarySetFlag1(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu) {
    return PrimaryChangeActorFlag(memory, cpu, PRIMARY_OP_SET_FLAG_1);
}

/* $0E: clear $0736 bit 1 */
static Lufia2ActorPrimaryScriptStepResult PrimaryClearFlag1(const Lufia2Memory *memory,
                                                            Lufia2CpuState *cpu) {
    return PrimaryChangeActorFlag(memory, cpu, PRIMARY_OP_CLEAR_FLAG_1);
}

/* $30: flag $0736 bit 6 or skip */
static Lufia2ActorPrimaryScriptStepResult
PrimaryMapCell30Test(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ActorPrimaryScriptStepResult result =
        PrimaryStepStart(PRIMARY_OP_MAP_CELL_30_TEST);
    Lufia2ActorScriptDispatchResult dispatch;

    LoadXDirect(memory, cpu, DP_ACTOR_SLOT); /* $83:CBB7 */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_TILE_X, cpu->x)));
    /* $83:CBB9 */
    Write8(memory, DirectAddress(cpu, DP_PROBE_X), A8(cpu));
    /* $83:CBBC */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_TILE_Y, cpu->x)));
    /* $83:CBBE */
    Write8(memory, DirectAddress(cpu, DP_PROBE_Y), A8(cpu));
    /* $83:CBC1 */
    SimulateJsrFrame(memory, cpu, 0xcbc5u);       /* $83:CBC3 */
    Lufia2ActorResolveMapCellOffset(memory, cpu); /* $83:F9D4 */
    SimulateRtsFrame(memory, cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(PRIMARY_MAP_CELL_ATTRIBUTES, cpu->x)));
    /* $83:CBC6 */
    And8(cpu, 0x30u);              /* $83:CBCA */
    Compare8(cpu, A8(cpu), 0x30u); /* $83:CBCC */
    if (!cpu->zero) {
        IncrementY16(cpu); /* $83:CBDD */
        dispatch = PrimaryRedispatch(memory, cpu, 0);
        return PrimaryStepRedispatched(dispatch);
    }
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT); /* $83:CBD0 */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_FLAGS, cpu->x)));
    /* $83:CBD2 */
    Or8(cpu, 0x40u); /* $83:CBD5 */
    Write8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_FLAGS, cpu->x),
           A8(cpu)); /* $83:CBD7 */
    result.flow = LUFIA2_ACTOR_PRIMARY_SCRIPT_CONTINUE_C8D2;
    result.handler_pc = PRIMARY_YIELD_PC; /* $83:CBDA */
    return result;
}

/* $31 */
static Lufia2ActorPrimaryScriptStepResult
PrimaryJumpIfLeaderNear(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ActorScriptDispatchResult dispatch;

    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x01u, cpu->y)));
    /* $83:CBE1 */
    IncrementY16(cpu); /* $83:CBE4 */
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_A), A8(cpu));
    /* $83:CBE5 */
    AslA8(cpu); /* $83:CBE7 */
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_B), A8(cpu));
    /* $83:CBE8 */
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT); /* $83:CBEA */
    if (!PrimaryLeaderWithinRadius(memory, cpu, WRAM_ACTOR_TILE_X) ||
        !PrimaryLeaderWithinRadius(memory, cpu, WRAM_ACTOR_TILE_Y))
        return Lufia2ActorPrimaryScriptExecuteKnownHandler(
            memory, cpu, PRIMARY_OP_JUMP);        /* $83:CC18 */
    IncrementY16(cpu);                            /* $83:CC12 */
    IncrementY16(cpu);                            /* $83:CC13 */
    IncrementY16(cpu);                            /* $83:CC14 */
    dispatch = PrimaryRedispatch(memory, cpu, 0); /* $83:C85C */
    return PrimaryStepRedispatched(dispatch);
}

static Lufia2ActorPrimaryScriptStepResult
PrimaryJumpIfLeaderAligned(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                           uint32_t handler_pc) {
    Lufia2ActorScriptDispatchResult dispatch;

    const uint16_t coordinate_base =
        (handler_pc & PRIMARY_PC_MASK) == PRIMARY_OP_JUMP_IF_LEADER_X
            ? WRAM_ACTOR_TILE_X
            : WRAM_ACTOR_TILE_Y;

    LoadXDirect(memory, cpu, DP_ACTOR_SLOT); /* $83:CC1B */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, coordinate_base, cpu->x)));
    /* $83:CC1D */
    Compare8(cpu, A8(cpu),
             Read8(memory, AbsoluteIndexedAddress(cpu, coordinate_base, 0)));
    /* $83:CC20 */
    if (cpu->zero)
        return Lufia2ActorPrimaryScriptExecuteKnownHandler(
            memory, cpu, PRIMARY_OP_JUMP);        /* $83:CC25 */
    IncrementY16(cpu);                            /* $83:CC28 */
    IncrementY16(cpu);                            /* $83:CC29 */
    IncrementY16(cpu);                            /* $83:CC2A */
    dispatch = PrimaryRedispatch(memory, cpu, 0); /* $83:C85C */
    return PrimaryStepRedispatched(dispatch);
}

/* $32 */
static Lufia2ActorPrimaryScriptStepResult
PrimaryJumpIfLeaderX(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return PrimaryJumpIfLeaderAligned(memory, cpu, PRIMARY_OP_JUMP_IF_LEADER_X);
}

/* $33 */
static Lufia2ActorPrimaryScriptStepResult
PrimaryJumpIfLeaderY(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return PrimaryJumpIfLeaderAligned(memory, cpu, PRIMARY_OP_JUMP_IF_LEADER_Y);
}

static Lufia2ActorPrimaryScriptStepResult
PrimaryStepTowardLeaderAlongAxis(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                 uint32_t handler_pc) {
    Lufia2ActorPrimaryScriptStepResult result = PrimaryStepStart(handler_pc);
    Lufia2ActorScriptDispatchResult dispatch;

    const uint8_t x_axis =
        (handler_pc & PRIMARY_PC_MASK) == PRIMARY_OP_STEP_TOWARD_LEADER_X;

    SimulateJsrFrame(memory, cpu, x_axis ? 0xcc43u : 0xcc65u); /* $83:CC41 */
    if (x_axis)
        PrimaryLeaderDirection(memory, cpu, WRAM_ACTOR_TILE_X, 0x02u, 0x03u);
    else
        PrimaryLeaderDirection(memory, cpu, WRAM_ACTOR_TILE_Y, 0x00u, 0x01u);
    SimulateRtsFrame(memory, cpu);
    if (cpu->carry && !PrimaryCallActionCore(memory, cpu, x_axis ? 0xcc49u : 0xcc6bu)) {
        result.handler_pc = cpu->resume_pc; /* $83:CC46 */
        return result;
    }
    IncrementY16(cpu);                            /* $83:CC4A */
    dispatch = PrimaryRedispatch(memory, cpu, 0); /* $83:C85C */
    return PrimaryStepRedispatched(dispatch);
}

/* $34 */
static Lufia2ActorPrimaryScriptStepResult
PrimaryStepTowardLeaderX(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return PrimaryStepTowardLeaderAlongAxis(memory, cpu,
                                            PRIMARY_OP_STEP_TOWARD_LEADER_X);
}

/* $35 */
static Lufia2ActorPrimaryScriptStepResult
PrimaryStepTowardLeaderY(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return PrimaryStepTowardLeaderAlongAxis(memory, cpu,
                                            PRIMARY_OP_STEP_TOWARD_LEADER_Y);
}

/* $28: random byte >= operand */
static Lufia2ActorPrimaryScriptStepResult PrimaryRandomJump(const Lufia2Memory *memory,
                                                            Lufia2CpuState *cpu) {
    Lufia2ActorScriptDispatchResult dispatch;

    Lufia2CallRandomByte(memory, cpu, 0xd323u); /* $83:D320 */
    Compare8(cpu, A8(cpu), Read8(memory, AbsoluteIndexedAddress(cpu, 0x01u, cpu->y)));
    /* $83:D324 */
    if (!cpu->carry) {
        IncrementY16(cpu); /* $83:D329 */
        IncrementY16(cpu);
        IncrementY16(cpu);
        IncrementY16(cpu);
        dispatch = PrimaryRedispatch(memory, cpu, 0);
        return PrimaryStepRedispatched(dispatch);
    }
    SetAccumulatorWidth(cpu, 0);                                     /* $83:D330 */
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x02u, cpu->y)); /* $83:D332 */
    cpu->carry = 0;                                                  /* $83:D335 */
    Add16Immediate(cpu, PRIMARY_JUMP_BASE);                          /* $83:D336 */
    Write16Direct(memory, cpu, DP_SCRIPT_CURSOR, cpu->accumulator);
    /* $83:D339 */
    SetAccumulatorWidth(cpu, 1);                  /* $83:D33B */
    dispatch = PrimaryRedispatch(memory, cpu, 1); /* $83:C85A */
    return PrimaryStepRedispatched(dispatch);
}

/* $29: operand + $F000 */
static Lufia2ActorPrimaryScriptStepResult PrimaryJumpF000(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu) {
    Lufia2ActorScriptDispatchResult dispatch;

    SetAccumulatorWidth(cpu, 0);                                     /* $83:D340 */
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x01u, cpu->y)); /* $83:D342 */
    cpu->carry = 0;                                                  /* $83:D345 */
    Add16Immediate(cpu, PRIMARY_JUMP_BASE_HIGH);                     /* $83:D346 */
    Write16Direct(memory, cpu, DP_SCRIPT_CURSOR, cpu->accumulator);
    /* $83:D349 */
    SetAccumulatorWidth(cpu, 1);                  /* $83:D34B */
    dispatch = PrimaryRedispatch(memory, cpu, 1); /* $83:C85A */
    return PrimaryStepRedispatched(dispatch);
}

/* $16 */
static Lufia2ActorPrimaryScriptStepResult
PrimaryRandomDirection(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ActorPrimaryScriptStepResult result =
        PrimaryStepStart(PRIMARY_OP_RANDOM_DIRECTION);

    LoadA8(cpu, 0x04u);                          /* $83:D125 */
    Lufia2CallRandomScale(memory, cpu, 0xd12au); /* $83:D127 */
    PRIMARY_ACTION_OR_BOUNDARY(0xd12eu);         /* $83:D12B */
    IncrementY16(cpu);                           /* $83:C8C6 */
    return Lufia2ActorPrimaryScriptExecuteKnownHandler(memory, cpu,
                                                       PRIMARY_OP_COMMIT_CURSOR);
}

/* $17 */
static Lufia2ActorPrimaryScriptStepResult
PrimaryCommitCursor17(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return Lufia2ActorPrimaryScriptExecuteKnownHandler(
        memory, cpu, PRIMARY_OP_COMMIT_CURSOR); /* $83:D132 */
}

static Lufia2ActorPrimaryScriptStepResult
PrimaryStepAwayAlongAxis(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                         uint32_t handler_pc) {
    Lufia2ActorPrimaryScriptStepResult result = PrimaryStepStart(handler_pc);
    Lufia2ActorScriptDispatchResult dispatch;

    const uint8_t x_axis =
        (handler_pc & PRIMARY_PC_MASK) == PRIMARY_OP_STEP_AWAY_LEADER_X;

    SimulateJsrFrame(memory, cpu, x_axis ? 0xccf2u : 0xcd0fu); /* $83:CCF0 */
    if (x_axis)
        PrimaryLeaderDirection(memory, cpu, WRAM_ACTOR_TILE_X, 0x02u, 0x03u);
    else
        PrimaryLeaderDirection(memory, cpu, WRAM_ACTOR_TILE_Y, 0x00u, 0x01u);
    SimulateRtsFrame(memory, cpu);
    if (cpu->carry) {
        TransferAToX(cpu); /* $83:CD00 */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(ROM_FLEE_ACTION_TABLE, cpu->x)));
    } else {
        LoadA8(cpu, 0x02u); /* $83:CCF5 */
        Lufia2CallRandomScale(memory, cpu, x_axis ? 0xccfau : 0xcd17u);
        cpu->carry = 0;                    /* $83:CCFB */
        Adc8(cpu, x_axis ? 0x02u : 0x00u); /* $83:CCFC */
    }
    PRIMARY_ACTION_OR_BOUNDARY(x_axis ? 0xcd08u : 0xcd25u);
    IncrementY16(cpu);                            /* $83:CD09 */
    dispatch = PrimaryRedispatch(memory, cpu, 0); /* $83:C85C */
    return PrimaryStepRedispatched(dispatch);
}

/* $39: random when level */
static Lufia2ActorPrimaryScriptStepResult
PrimaryStepAwayLeaderX(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return PrimaryStepAwayAlongAxis(memory, cpu, PRIMARY_OP_STEP_AWAY_LEADER_X);
}

/* $3A: random when level */
static Lufia2ActorPrimaryScriptStepResult
PrimaryStepAwayLeaderY(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return PrimaryStepAwayAlongAxis(memory, cpu, PRIMARY_OP_STEP_AWAY_LEADER_Y);
}

/* $3B */
static Lufia2ActorPrimaryScriptStepResult
PrimaryCommitCursor3b(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    IncrementY16(cpu); /* $83:CD2E */
    return Lufia2ActorPrimaryScriptExecuteKnownHandler(memory, cpu,
                                                       PRIMARY_OP_COMMIT_CURSOR);
}

/* $3C */
static Lufia2ActorPrimaryScriptStepResult
PrimaryRotateFacing(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ActorPrimaryScriptStepResult result =
        PrimaryStepStart(PRIMARY_OP_ROTATE_FACING);

    LoadXDirect(memory, cpu, DP_ACTOR_SLOT); /* $83:CD32 */
    TransferDirectToA(cpu);                  /* $83:CD34 */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_FACING, cpu->x)));
    /* $83:CD35 */
    LoadA8(cpu, (uint8_t)(A8(cpu) + 1u)); /* $83:CD38 */
    LoadA8(cpu, (uint8_t)(A8(cpu) + 1u)); /* $83:CD39 */
    And8(cpu, 0x07u);                     /* $83:CD3A */
    Write8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_FACING, cpu->x),
           A8(cpu));   /* $83:CD3C */
    TransferAToX(cpu); /* $83:CD3F */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(ROM_FACING_ACTION_TABLE, cpu->x)));
    /* $83:CD40 */
    cpu->carry = 0;                      /* $83:CD44 */
    Adc8(cpu, 0x24u);                    /* $83:CD45 */
    PRIMARY_ACTION_OR_BOUNDARY(0xcd4au); /* $83:CD47 */
    IncrementY16(cpu);                   /* $83:CD4B */
    return Lufia2ActorPrimaryScriptExecuteKnownHandler(memory, cpu,
                                                       PRIMARY_OP_COMMIT_CURSOR);
}

/* $3D */
static Lufia2ActorPrimaryScriptStepResult
PrimaryJumpIfLeaderAhead(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ActorPrimaryScriptStepResult result =
        PrimaryStepStart(PRIMARY_OP_JUMP_IF_LEADER_AHEAD);
    Lufia2ActorScriptDispatchResult dispatch;

    uint16_t helper_pc;

    LoadXDirect(memory, cpu, DP_ACTOR_SLOT); /* $83:CD4F */
    TransferDirectToA(cpu);                  /* $83:CD51 */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_FACING, cpu->x)));
    /* $83:CD52 */
    TransferAToX(cpu); /* $83:CD55 */
    helper_pc = Read16ProgramIndexed(memory, cpu, 0xcd66u, cpu->x);
    SimulateJsrFrame(memory, cpu, 0xcd58u); /* $83:CD56 */
    if (!Lufia2ActorFacingCompare(memory, cpu, helper_pc)) {
        cpu->resume_pc = 0x830000u | helper_pc;
        result.handler_pc = cpu->resume_pc;
        return result;
    }
    SimulateRtsFrame(memory, cpu);
    if (!cpu->zero && cpu->carry) /* $83:CD59 */
        return Lufia2ActorPrimaryScriptExecuteKnownHandler(
            memory, cpu, PRIMARY_OP_JUMP); /* $83:CD5D */
    IncrementY16(cpu);                     /* $83:CD60 */
    IncrementY16(cpu);
    IncrementY16(cpu);
    dispatch = PrimaryRedispatch(memory, cpu, 0); /* $83:C85C */
    return PrimaryStepRedispatched(dispatch);
}

static Lufia2ActorPrimaryScriptStepResult
PrimaryActionFromFacingTable(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                             uint32_t handler_pc) {
    Lufia2ActorPrimaryScriptStepResult result = PrimaryStepStart(handler_pc);

    const uint8_t cd92 = (handler_pc & PRIMARY_PC_MASK) == PRIMARY_OP_ACTION_FROM_D457;

    TransferDirectToA(cpu); /* $83:CD92 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x47u)));
    /* $83:CD93 */
    And8(cpu, 0x0fu); /* $83:CD95 */
    if (!cpu->zero) {
        TransferAToX(cpu); /* $83:CD99 */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(cd92 ? ROM_ACTION_NIBBLE_TABLE_B
                                                          : ROM_ACTION_NIBBLE_TABLE_A,
                                                     cpu->x)));
        /* $83:CD9A */
        PRIMARY_ACTION_OR_BOUNDARY(cd92 ? 0xcda1u : 0xd121u);
    }
    IncrementY16(cpu); /* $83:C8C6 */
    return Lufia2ActorPrimaryScriptExecuteKnownHandler(memory, cpu,
                                                       PRIMARY_OP_COMMIT_CURSOR);
}

/* $3E: $47 low nibble via $83:D457 */
static Lufia2ActorPrimaryScriptStepResult
PrimaryActionFromD457(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return PrimaryActionFromFacingTable(memory, cpu, PRIMARY_OP_ACTION_FROM_D457);
}

/* $15: $47 low nibble via $83:D447 */
static Lufia2ActorPrimaryScriptStepResult
PrimaryActionFromD447(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return PrimaryActionFromFacingTable(memory, cpu, PRIMARY_OP_ACTION_FROM_D447);
}

/* $40 */
static Lufia2ActorPrimaryScriptStepResult PrimaryAction60(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu) {
    Lufia2ActorPrimaryScriptStepResult result = PrimaryStepStart(PRIMARY_OP_ACTION_60);
    Lufia2ActorScriptDispatchResult dispatch;

    LoadA8(cpu, 0x60u);                           /* $83:CE73 */
    PRIMARY_ACTION_OR_BOUNDARY(0xce78u);          /* $83:CE75 */
    IncrementY16(cpu);                            /* $83:CE79 */
    dispatch = PrimaryRedispatch(memory, cpu, 0); /* $83:C85C */
    return PrimaryStepRedispatched(dispatch);
}

/* $47: $7F:E5A6/E5CE */
static Lufia2ActorPrimaryScriptStepResult
PrimaryTimedStepTowardTarget(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ActorPrimaryScriptStepResult result =
        PrimaryStepStart(PRIMARY_OP_TIMED_STEP_TOWARD_TARGET);
    Lufia2ActorScriptDispatchResult dispatch;

    LoadXDirect(memory, cpu, DP_ACTOR_SLOT); /* $83:CAB9 */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x01u, cpu->y)));
    /* $83:CABB */
    Write8(memory, LongIndexedAddress(WRAM_ACTOR_PRIMARY_TIMER, cpu->x), A8(cpu));
    /* $83:CABE */
    SimulateJsrFrame(memory, cpu, 0xcac4u);  /* $83:CAC2 */
    Lufia2ActorTargetDirection(memory, cpu); /* $83:CA93 */
    SimulateRtsFrame(memory, cpu);
    if (cpu->carry) {
        IncrementY16(cpu); /* $83:CACE */
        IncrementY16(cpu);
        dispatch = PrimaryRedispatch(memory, cpu, 0);
        return PrimaryStepRedispatched(dispatch);
    }
    PRIMARY_ACTION_OR_BOUNDARY(0xcacau); /* $83:CAC7 */
    return Lufia2ActorPrimaryScriptExecuteKnownHandler(
        memory, cpu, PRIMARY_OP_COMMIT_CURSOR); /* $83:CACB */
}

static Lufia2ActorPrimaryScriptStepResult
PrimaryStepRelativeToLeader(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                            uint32_t handler_pc) {
    Lufia2ActorPrimaryScriptStepResult result = PrimaryStepStart(handler_pc);
    Lufia2ActorScriptDispatchResult dispatch;

    const uint8_t flee =
        (handler_pc & PRIMARY_PC_MASK) == PRIMARY_OP_STEP_AWAY_FROM_LEADER;

    Lufia2ActorLeaderToProbe(memory, cpu, flee ? 0xccd9u : 0xc98cu); /* JSR $C0EF */
    PrimaryApproachProbe(memory, cpu, flee ? 0xccdcu : 0xc98fu);     /* JSR $C9C5 */
    if (cpu->carry) {
        if (flee) {
            ExchangeAccumulatorBytes(cpu); /* $83:CCDF */
            LoadA8(cpu, 0x00u);
            ExchangeAccumulatorBytes(cpu);
            TransferAToX(cpu); /* $83:CCE3 */
            LoadA8(cpu,
                   Read8(memory, LongIndexedAddress(ROM_FLEE_ACTION_TABLE, cpu->x)));
        }
        PRIMARY_ACTION_OR_BOUNDARY(flee ? 0xccebu : 0xc995u);
    }
    IncrementY16(cpu); /* $83:C996 */
    if (flee) {
        dispatch = PrimaryRedispatch(memory, cpu, 0);
        return PrimaryStepRedispatched(dispatch);
    }
    return Lufia2ActorPrimaryScriptExecuteKnownHandler(memory, cpu,
                                                       PRIMARY_OP_COMMIT_CURSOR);
}

/* $10 */
static Lufia2ActorPrimaryScriptStepResult
PrimaryStepTowardLeader(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return PrimaryStepRelativeToLeader(memory, cpu, PRIMARY_OP_STEP_TOWARD_LEADER);
}

/* $38 */
static Lufia2ActorPrimaryScriptStepResult
PrimaryStepAwayFromLeader(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return PrimaryStepRelativeToLeader(memory, cpu, PRIMARY_OP_STEP_AWAY_FROM_LEADER);
}

/* $45 */
static Lufia2ActorPrimaryScriptStepResult
PrimaryStepTowardFoundActor(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ActorPrimaryScriptStepResult result =
        PrimaryStepStart(PRIMARY_OP_STEP_TOWARD_FOUND_ACTOR);

    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET); /* $83:D01E */
    TransferDirectToA(cpu);                        /* $83:D020 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_ACTOR_SCRATCH_A, cpu->x)));
    TransferAToX(cpu); /* $83:D025 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_ACTOR_TILE_X, cpu->x)));
    Write8(memory, DirectAddress(cpu, DP_PROBE_X), A8(cpu));
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_ACTOR_TILE_Y, cpu->x)));
    Write8(memory, DirectAddress(cpu, DP_PROBE_Y), A8(cpu));
    PrimaryApproachProbe(memory, cpu, 0xd034u); /* $83:D032 */
    if (cpu->carry)
        PRIMARY_ACTION_OR_BOUNDARY(0xd03au); /* $83:D037 */
    IncrementY16(cpu);                       /* $83:D03B */
    return Lufia2ActorPrimaryScriptExecuteKnownHandler(memory, cpu,
                                                       PRIMARY_OP_COMMIT_CURSOR);
}

/* $42: action $5F, position to $7F:DB9C */
static Lufia2ActorPrimaryScriptStepResult
PrimaryRecordPosition(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ActorPrimaryScriptStepResult result =
        PrimaryStepStart(PRIMARY_OP_RECORD_POSITION);
    Lufia2ActorScriptDispatchResult dispatch;

    LoadA8(cpu, 0x5fu);                      /* $83:CF6E */
    PRIMARY_ACTION_OR_BOUNDARY(0xcf73u);     /* $83:CF70 */
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT); /* $83:CF74 */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_TILE_X, cpu->x)));
    ExchangeAccumulatorBytes(cpu); /* $83:CF79 */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_TILE_Y, cpu->x)));
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET); /* $83:CF7D */
    Write8(memory, LongIndexedAddress((WRAM_ACTOR_SCRATCH_B + 1u), cpu->x), A8(cpu));
    ExchangeAccumulatorBytes(cpu); /* $83:CF83 */
    Write8(memory, LongIndexedAddress(WRAM_ACTOR_SCRATCH_B, cpu->x), A8(cpu));
    IncrementY16(cpu);                            /* $83:CF88 */
    dispatch = PrimaryRedispatch(memory, cpu, 0); /* $83:C85C */
    return PrimaryStepRedispatched(dispatch);
}

/* $12: or skip */
static Lufia2ActorPrimaryScriptStepResult
PrimaryStepTowardPoint(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ActorPrimaryScriptStepResult result =
        PrimaryStepStart(PRIMARY_OP_STEP_TOWARD_POINT);
    Lufia2ActorScriptDispatchResult dispatch;

    uint8_t ahead;

    LoadXDirect(memory, cpu, DP_ACTOR_SLOT); /* $83:CF8C */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_TILE_X, cpu->x)));
    Compare8(cpu, A8(cpu), Read8(memory, AbsoluteIndexedAddress(cpu, 0x01u, cpu->y)));
    if (!cpu->zero) {
        ahead = cpu->carry;
        LoadA8(cpu, ahead ? 0x02u : 0x03u); /* $83:CF96 */
    } else {
        LoadA8(cpu,
               Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_TILE_Y, cpu->x)));
        Compare8(cpu, A8(cpu),
                 Read8(memory, AbsoluteIndexedAddress(cpu, 0x02u, cpu->y)));
        if (cpu->zero) {
            IncrementY16(cpu); /* $83:CFB3 */
            IncrementY16(cpu);
            IncrementY16(cpu);
            dispatch = PrimaryRedispatch(memory, cpu, 0);
            return PrimaryStepRedispatched(dispatch);
        }
        ahead = cpu->carry;
        LoadA8(cpu, ahead ? 0x00u : 0x01u); /* $83:CFA6 */
    }
    PRIMARY_ACTION_OR_BOUNDARY(0xcfafu); /* $83:CFAC */
    return Lufia2ActorPrimaryScriptExecuteKnownHandler(
        memory, cpu, PRIMARY_OP_COMMIT_CURSOR); /* $83:CFB0 */
}

/* $44: slot to $7F:DB4C */
static Lufia2ActorPrimaryScriptStepResult
PrimaryFindActorInRadius(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ActorScriptDispatchResult dispatch;

    LoadXDirect(memory, cpu, DP_ACTOR_SLOT); /* $83:CFB9 */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x01u, cpu->y)));
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_A), A8(cpu));
    AslA8(cpu); /* $83:CFC0 */
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_B), A8(cpu));
    for (uint8_t axis = 0; axis < 2u; ++axis) { /* $83:CFC3 */
        LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(
                                      cpu, axis ? WRAM_ACTOR_TILE_Y : WRAM_ACTOR_TILE_X,
                                      cpu->x)));
        cpu->carry = 1;
        Sbc8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_A)));
        Write8(memory, DirectAddress(cpu, axis ? DP_SEARCH_MIN_Y : DP_SEARCH_MIN_X),
               A8(cpu));
        cpu->carry = 0;
        Adc8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_B)));
        Write8(memory, DirectAddress(cpu, axis ? DP_SEARCH_MAX_Y : DP_SEARCH_MAX_X),
               A8(cpu));
    }
    LoadX16(cpu, 0x00u); /* $83:CFDD */
    do {
        if (PrimaryActorInSearchBox(memory, cpu)) {
            TransferXToA(cpu); /* $83:D006 */
            LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
            Write8(memory, LongIndexedAddress(WRAM_ACTOR_SCRATCH_A, cpu->x), A8(cpu));
            IncrementY16(cpu); /* $83:D00D */
            IncrementY16(cpu);
            IncrementY16(cpu);
            IncrementY16(cpu);
            dispatch = PrimaryRedispatch(memory, cpu, 0);
            return PrimaryStepRedispatched(dispatch);
        }
        LoadX16(cpu, (uint16_t)(cpu->x + 1u)); /* $83:D014 */
        Compare16(cpu, cpu->x, 0x28u);
    } while (!cpu->zero);
    IncrementY16(cpu); /* $83:D01A */
    return Lufia2ActorPrimaryScriptExecuteKnownHandler(memory, cpu, PRIMARY_OP_JUMP);
}

static Lufia2ActorPrimaryScriptStepResult
PrimaryListedPointStep(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                       uint32_t handler_pc) {
    Lufia2ActorPrimaryScriptStepResult result = PrimaryStepStart(handler_pc);
    Lufia2ActorScriptDispatchResult dispatch;

    const uint8_t d09a =
        (handler_pc & PRIMARY_PC_MASK) == PRIMARY_OP_STEP_TOWARD_LISTED_POINT_13;
    PrimaryListSearch search;

    LoadX16(cpu, d09a ? 0x02u : 0x26u); /* $83:D09A */
    LoadA8(cpu, d09a ? 0x0fu : 0x04u);  /* $83:D09D */
    search = PrimaryApproachListedPoint(memory, cpu,
                                        d09a ? 0xd0a1u : 0xca20u); /* JSR $D0AA */
    if (search == PRIMARY_LIST_BOUNDARY) {
        result.handler_pc = cpu->resume_pc;
        return result;
    }
    if (search == PRIMARY_LIST_EXHAUSTED)
        return Lufia2ActorPrimaryScriptExecuteKnownHandler(
            memory, cpu, PRIMARY_OP_JUMP);        /* $83:D0A4 */
    dispatch = PrimaryRedispatch(memory, cpu, 0); /* $83:C85C */
    return PrimaryStepRedispatched(dispatch);
}

/* $13 */
static Lufia2ActorPrimaryScriptStepResult
PrimaryStepTowardListedPoint13(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return PrimaryListedPointStep(memory, cpu, PRIMARY_OP_STEP_TOWARD_LISTED_POINT_13);
}

/* $11 */
static Lufia2ActorPrimaryScriptStepResult
PrimaryStepTowardListedPoint(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return PrimaryListedPointStep(memory, cpu, PRIMARY_OP_STEP_TOWARD_LISTED_POINT);
}

static Lufia2ActorPrimaryScriptStepResult
PrimaryWanderAround(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                    uint32_t handler_pc) {
    Lufia2ActorPrimaryScriptStepResult result = PrimaryStepStart(handler_pc);
    Lufia2ActorScriptDispatchResult dispatch;

    const uint8_t around_leader =
        (handler_pc & PRIMARY_PC_MASK) == PRIMARY_OP_WANDER_IN_BOX;

    LoadXDirect(memory, cpu, DP_ACTOR_SLOT); /* $83:CE7D */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_TILE_X,
                                                     around_leader ? 0 : cpu->x)));
    Write8(memory, DirectAddress(cpu, 0x63u), A8(cpu));
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_TILE_Y,
                                                     around_leader ? 0 : cpu->x)));
    Write8(memory, DirectAddress(cpu, 0x64u), A8(cpu));
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FE216, cpu->x)));
    Write8(memory, DirectAddress(cpu, 0x9au), A8(cpu));
    if (around_leader) {
        LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x01u, cpu->y)));
        Write8(memory, DirectAddress(cpu, 0x65u), A8(cpu));
        LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x02u, cpu->y)));
    } else {
        LoadA8(cpu, 0x10u); /* $83:CE8F */
        Write8(memory, DirectAddress(cpu, 0x65u), A8(cpu));
        LoadA8(cpu, 0x02u);
    }
    Write8(memory, DirectAddress(cpu, 0x66u), A8(cpu));
    LoadA8(cpu, 0x00u);
    if (!PrimaryWanderAxis(memory, cpu, around_leader ? 0xcf3au : 0xce9bu)) {
        result.handler_pc = cpu->resume_pc;
        return result;
    }
    LoadA8(cpu, 0x02u);
    if (!PrimaryWanderAxis(memory, cpu, around_leader ? 0xcf3fu : 0xcea0u)) {
        result.handler_pc = cpu->resume_pc;
        return result;
    }
    if (!around_leader) {
        PrimaryRecordProbe(memory, cpu, 0xcea3u); /* $83:CEA1 */
        IncrementY16(cpu);                        /* $83:CEA4 */
        dispatch = PrimaryRedispatch(memory, cpu, 0);
        return PrimaryStepRedispatched(dispatch);
    }
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT); /* $83:CF40 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_PROBE_X)));
    Compare8(cpu, A8(cpu),
             Read8(memory, LongIndexedAddress(WRAM_ACTOR_BOX_MIN_X, cpu->x)));
    if (cpu->carry) {
        Compare8(cpu, A8(cpu),
                 Read8(memory, LongIndexedAddress(WRAM_ACTOR_BOX_MAX_X, cpu->x)));
        if (!cpu->carry) {
            LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_PROBE_Y)));
            Compare8(cpu, A8(cpu),
                     Read8(memory, LongIndexedAddress(WRAM_ACTOR_BOX_MIN_Y, cpu->x)));
            if (cpu->carry) {
                Compare8(
                    cpu, A8(cpu),
                    Read8(memory, LongIndexedAddress(WRAM_ACTOR_BOX_MAX_Y, cpu->x)));
                if (!cpu->carry) {
                    PrimaryRecordProbe(memory, cpu, 0xcf60u);
                    for (int i = 0; i < 5; ++i) /* $83:CF61 */
                        IncrementY16(cpu);
                    dispatch = PrimaryRedispatch(memory, cpu, 0);
                    return PrimaryStepRedispatched(dispatch);
                }
            }
        }
    }
    IncrementY16(cpu); /* $83:CF69 */
    IncrementY16(cpu);
    return Lufia2ActorPrimaryScriptExecuteKnownHandler(memory, cpu, PRIMARY_OP_JUMP);
}

/* $43 */
static Lufia2ActorPrimaryScriptStepResult PrimaryWander(const Lufia2Memory *memory,
                                                        Lufia2CpuState *cpu) {
    return PrimaryWanderAround(memory, cpu, PRIMARY_OP_WANDER);
}

/* $41 */
static Lufia2ActorPrimaryScriptStepResult PrimaryWanderInBox(const Lufia2Memory *memory,
                                                             Lufia2CpuState *cpu) {
    return PrimaryWanderAround(memory, cpu, PRIMARY_OP_WANDER_IN_BOX);
}

/* $3F */
static Lufia2ActorPrimaryScriptStepResult
PrimaryWalkAheadOfLeader(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ActorPrimaryScriptStepResult result =
        PrimaryStepStart(PRIMARY_OP_WALK_AHEAD_OF_LEADER);
    Lufia2ActorScriptDispatchResult dispatch;

    LoadXDirect(memory, cpu, DP_ACTOR_SLOT); /* $83:CDA5 */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_TILE_X, 0)));
    Write8(memory, DirectAddress(cpu, DP_PROBE_X), A8(cpu));
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_TILE_Y, 0)));
    Write8(memory, DirectAddress(cpu, DP_PROBE_Y), A8(cpu));
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FE216, cpu->x)));
    Write8(memory, DirectAddress(cpu, 0x9au), A8(cpu));
    IncrementY16(cpu); /* $83:CDB7 */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x00u, cpu->y)));
    Write8(memory, DirectAddress(cpu, 0x93u), A8(cpu));
    TransferDirectToA(cpu); /* $83:CDBD */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_FACING, 0)));
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(ROM_AHEAD_STEP_TABLE, cpu->x)));
    Write8(memory, DirectAddress(cpu, 0x94u), A8(cpu));
    Write8(memory, DirectAddress(cpu, DP_SEARCH_MIN_X), 0x00u); /* $83:CDC8 */
    Write8(memory, DirectAddress(cpu, DP_SEARCH_MIN_Y), 0x00u);
    LoadA8(cpu, 0xffu);
    Write8(memory, DirectAddress(cpu, DP_SEARCH_MAX_X), A8(cpu));
    Write8(memory, DirectAddress(cpu, DP_SEARCH_MAX_Y), A8(cpu));
    if (!PrimaryMeasureRun(memory, cpu, 0xcdd4u)) {
        result.handler_pc = cpu->resume_pc;
        return result;
    }
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x9du)));
    Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x93u)));
    if (!cpu->carry)
        return Lufia2ActorPrimaryScriptExecuteKnownHandler(
            memory, cpu, PRIMARY_OP_JUMP); /* $83:CDF3 */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_TILE_X, 0)));
    Write8(memory, DirectAddress(cpu, DP_PROBE_X), A8(cpu));
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_TILE_Y, 0)));
    Write8(memory, DirectAddress(cpu, DP_PROBE_Y), A8(cpu));
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x93u)));
    if (!PrimaryWalkSteps(memory, cpu, 0xcde9u)) {
        result.handler_pc = cpu->resume_pc;
        return result;
    }
    PrimaryRecordProbe(memory, cpu, 0xcdecu); /* $83:CDEA */
    IncrementY16(cpu);                        /* $83:CDED */
    IncrementY16(cpu);
    IncrementY16(cpu);
    dispatch = PrimaryRedispatch(memory, cpu, 0);
    return PrimaryStepRedispatched(dispatch);
}

/* $46 */
static Lufia2ActorPrimaryScriptStepResult
PrimaryJumpIfBlocked(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ActorPrimaryScriptStepResult result =
        PrimaryStepStart(PRIMARY_OP_JUMP_IF_BLOCKED);
    Lufia2ActorScriptDispatchResult dispatch;

    uint8_t direction;

    PrimaryActorToProbe(memory, cpu, 0xd041u); /* $83:D03F */
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);   /* $83:D042 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FE216, cpu->x)));
    Write8(memory, DirectAddress(cpu, 0x9au), A8(cpu));
    TransferDirectToA(cpu); /* $83:D04A */
    IncrementY16(cpu);
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x00u, cpu->y)));
    if (!PrimaryStepBlocked(memory, cpu, 0xd051u)) {
        result.handler_pc = cpu->resume_pc;
        return result;
    }
    if (cpu->zero) {
        LoadXDirect(memory, cpu, DP_ACTOR_SLOT); /* $83:D054 */
        LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x00u, cpu->y)));
        direction = A8(cpu);
        Compare8(cpu, direction, 0x00u); /* $83:D059 */
        if (!cpu->zero) {
            Compare8(cpu, direction, 0x04u); /* $83:D068 */
            if (!cpu->zero)
                Compare8(cpu, direction, 0x02u); /* $83:D076 */
        }
        if (direction == 0x00u || direction == 0x02u || direction == 0x04u) {
            const uint8_t y_axis = direction != 0x02u;
            const uint32_t actor = y_axis ? WRAM_ACTOR_TILE_Y : WRAM_ACTOR_TILE_X;
            const uint32_t bound = direction == 0x00u   ? WRAM_ACTOR_BOX_MAX_Y
                                   : direction == 0x04u ? WRAM_ACTOR_BOX_MIN_Y
                                                        : WRAM_ACTOR_BOX_MIN_X;
            if (direction == 0x00u) {
                LoadA8(cpu, Read8(memory, LongIndexedAddress(bound, cpu->x)));
                DecrementA8(cpu);
                Compare8(cpu, A8(cpu),
                         Read8(memory, LongIndexedAddress(actor, cpu->x)));
            } else {
                LoadA8(cpu, Read8(memory, LongIndexedAddress(actor, cpu->x)));
                Compare8(cpu, A8(cpu),
                         Read8(memory, LongIndexedAddress(bound, cpu->x)));
            }
        } else {
            LoadA8(cpu,
                   Read8(memory, LongIndexedAddress(WRAM_ACTOR_BOX_MAX_X, cpu->x)));
            DecrementA8(cpu); /* $83:D088 */
            Compare8(cpu, A8(cpu),
                     Read8(memory, LongIndexedAddress(WRAM_ACTOR_TILE_X, cpu->x)));
        }
        if (!cpu->zero && cpu->carry) { /* $83:D08D */
            IncrementY16(cpu);          /* $83:D094 */
            IncrementY16(cpu);
            IncrementY16(cpu);
            dispatch = PrimaryRedispatch(memory, cpu, 0);
            return PrimaryStepRedispatched(dispatch);
        }
    }
    return Lufia2ActorPrimaryScriptExecuteKnownHandler(memory, cpu,
                                                       PRIMARY_OP_JUMP); /* $83:D091 */
}

/* Reset opcodes: $83:C947, then the $C918/$CAD3 extras. */
static Lufia2ActorPrimaryScriptStepResult PrimaryResetActor(const Lufia2Memory *memory,
                                                            Lufia2CpuState *cpu,
                                                            uint32_t handler_pc) {
    Lufia2ActorPrimaryScriptStepResult result = PrimaryStepStart(handler_pc);

    const uint16_t entry = (uint16_t)handler_pc;

    SimulateJsrFrame(memory, cpu, (uint16_t)(entry + 2u));
    Lufia2ActorPrimaryReset(memory, cpu); /* JSR $C947 */
    SimulateRtsFrame(memory, cpu);
    if (entry == 0xc918u) {
        LoadXDirect(memory, cpu, DP_ACTOR_SLOT); /* $83:C91B */
        LoadAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->x);
        Or8(cpu, 0x01u);
        StoreAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->x);
        LoadAAbsolute8(memory, cpu, WRAM_WINDOW_MODE, 0); /* $83:C925 */
        BitImmediate8(cpu, 0x01u);
        if (!cpu->zero) {
            LoadA8(cpu, 0x09u); /* $83:C92C */
            StoreAAbsolute8(memory, cpu, WRAM_UNK_7E070A, cpu->x);
            SimulateJslFrame(memory, cpu, 0x83u, 0xc934u);
            Lufia2ActorLoadPrimaryScript(memory, cpu);
            SimulateRtlFrame(memory, cpu);
        } else {
            LoadAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->x);
            BitImmediate8(cpu, 0x20u); /* $83:C93A */
            if (cpu->zero) {
                LoadA8(cpu, 0x04u);
                Write8(memory, LongIndexedAddress(WRAM_UNK_7FE4DE, cpu->x), A8(cpu));
            }
        }
    } else if (entry == 0xcad3u) {
        LoadA8(cpu, 0x01u); /* $83:CAD6 */
        TestBitsAbsolute8(memory, cpu, WRAM_ACTOR_STATE, 0);
        SimulateJsrFrame(memory, cpu, 0xcaddu);
        Lufia2ActorClearSlotLinks(memory, cpu); /* JSR $CB65 */
        SimulateRtsFrame(memory, cpu);
    }
    result.flow = LUFIA2_ACTOR_PRIMARY_SCRIPT_CONTINUE_C8D2;
    result.handler_pc = PRIMARY_YIELD_PC;
    return result;
}

/* $0F $1E: reset via $83:C947 */
static Lufia2ActorPrimaryScriptStepResult PrimaryReset(const Lufia2Memory *memory,
                                                       Lufia2CpuState *cpu) {
    return PrimaryResetActor(memory, cpu, PRIMARY_OP_RESET);
}

/* $48 */
static Lufia2ActorPrimaryScriptStepResult PrimaryReset48(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu) {
    return PrimaryResetActor(memory, cpu, PRIMARY_OP_RESET_48);
}

/* $2F */
static Lufia2ActorPrimaryScriptStepResult PrimaryReset2f(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu) {
    return PrimaryResetActor(memory, cpu, PRIMARY_OP_RESET_2F);
}

/* $2E: blocked event via $83:CA68 */
static Lufia2ActorPrimaryScriptStepResult
PrimaryStepTowardTarget(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ActorPrimaryScriptStepResult result =
        PrimaryStepStart(PRIMARY_OP_STEP_TOWARD_TARGET);
    Lufia2ActorScriptDispatchResult dispatch;

    LoadA8(cpu, 0x40u); /* $83:CA29 */
    TestBitsAbsolute8(memory, cpu, WRAM_ACTOR_STATE, 1);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT); /* $83:CA2E */
    TransferDirectToA(cpu);
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_Y, cpu->x);
    Write8(memory, DirectAddress(cpu, DP_PROBE_Y), A8(cpu));
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_X, cpu->x);
    Write8(memory, DirectAddress(cpu, DP_PROBE_X), A8(cpu));
    SimulateJsrFrame(memory, cpu, 0xca3du); /* $83:CA3B */
    Lufia2ActorTargetDirection(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    if (!cpu->carry) {
        Write8(memory, DirectAddress(cpu, DP_SCRATCH_C), A8(cpu));
        TransferAToX(cpu); /* $83:CA42 */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(ROM_FACING_STEP_TABLE, cpu->x)));
        SimulateJslFrame(memory, cpu, 0x83u, 0xca4au);
        if (Lufia2ActorMovementStep(memory, cpu) == 0) {
            result.handler_pc = cpu->resume_pc;
            return result;
        }
        SimulateRtlFrame(memory, cpu);
        Lufia2MapCellIndex(memory, cpu, 0xca4du, 1); /* $83:CA4B */
        LoadA8(cpu,
               Read8(memory, LongIndexedAddress(WRAM_FIELD_MAP_ATTRIBUTES, cpu->x)));
        BitImmediate8(cpu, 0x0bu); /* $83:CA52 */
        if (cpu->zero) {
            TransferDirectToA(cpu); /* $83:CA56 */
            LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_C)));
            PRIMARY_ACTION_OR_BOUNDARY(0xca5cu);
            return Lufia2ActorPrimaryScriptExecuteKnownHandler(
                memory, cpu, PRIMARY_OP_COMMIT_CURSOR); /* $83:CA5D */
        }
        SimulateJslFrame(memory, cpu, 0x83u, 0xca63u);
        Lufia2ActorBlockedEvent(memory, cpu); /* $83:CA60 */
        SimulateRtlFrame(memory, cpu);
    }
    IncrementY16(cpu); /* $83:CA64 */
    dispatch = PrimaryRedispatch(memory, cpu, 0);
    return PrimaryStepRedispatched(dispatch);
}

/* $18 */
static Lufia2ActorPrimaryScriptStepResult
PrimarySetTilePosition(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ActorScriptDispatchResult dispatch;

    LoadXDirect(memory, cpu, DP_ACTOR_SLOT); /* $83:D135 */
    LoadAAbsolute8(memory, cpu, 0x01u, cpu->y);
    StoreAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_X, cpu->x);
    LoadAAbsolute8(memory, cpu, 0x02u, cpu->y);
    StoreAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_Y, cpu->x);
    IncrementY16(cpu); /* $83:D143 */
    IncrementY16(cpu);
    IncrementY16(cpu);
    SimulateJslFrame(memory, cpu, 0x83u, 0xd149u);
    Lufia2ActorSyncFinePosition(memory, cpu); /* $83:A746 */
    SimulateRtlFrame(memory, cpu);
    dispatch = PrimaryRedispatch(memory, cpu, 0); /* $83:C85C */
    return PrimaryStepRedispatched(dispatch);
}

/* $1D: via $84:8766 */
static Lufia2ActorPrimaryScriptStepResult PrimaryPlaySound(const Lufia2Memory *memory,
                                                           Lufia2CpuState *cpu) {
    Lufia2ActorScriptDispatchResult dispatch;

    LoadAAbsolute8(memory, cpu, 0x01u, cpu->y); /* $83:D1B5 */
    SimulateJslFrame(memory, cpu, 0x83u, 0xd1bbu);
    Lufia2QueueDeferredSound(memory, cpu); /* $84:8766 */
    SimulateRtlFrame(memory, cpu);
    IncrementY16(cpu); /* $83:D1BC */
    IncrementY16(cpu);
    dispatch = PrimaryRedispatch(memory, cpu, 0);
    return PrimaryStepRedispatched(dispatch);
}

static Lufia2ActorPrimaryScriptStepResult
PrimaryFinePositionStep(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                        uint32_t handler_pc) {
    Lufia2ActorScriptDispatchResult dispatch;

    const uint8_t display =
        (handler_pc & PRIMARY_PC_MASK) == PRIMARY_OP_ADD_DISPLAY_OFFSET;

    SimulateJsrFrame(memory, cpu, display ? 0xd200u : 0xd209u);
    if (display)
        Lufia2ActorAddDisplayOffset(memory, cpu); /* JSR $FACB */
    else
        Lufia2ActorMoveFinePosition(memory, cpu); /* JSR $FA81 */
    SimulateRtsFrame(memory, cpu);
    TransferAToY(cpu); /* $83:D201 */
    SetAccumulatorWidth(cpu, 1);
    dispatch = PrimaryRedispatch(memory, cpu, 0); /* $83:C85C */
    return PrimaryStepRedispatched(dispatch);
}

/* $2C: via $83:FACB */
static Lufia2ActorPrimaryScriptStepResult
PrimaryAddDisplayOffset(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return PrimaryFinePositionStep(memory, cpu, PRIMARY_OP_ADD_DISPLAY_OFFSET);
}

/* $2D: via $83:FA81 */
static Lufia2ActorPrimaryScriptStepResult
PrimaryMoveFinePosition(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return PrimaryFinePositionStep(memory, cpu, PRIMARY_OP_MOVE_FINE_POSITION);
}

/* $1A: low bits of $1291 */
static Lufia2ActorPrimaryScriptStepResult PrimaryMerge1291(const Lufia2Memory *memory,
                                                           Lufia2CpuState *cpu) {
    Lufia2ActorScriptDispatchResult dispatch;

    LoadXDirect(memory, cpu, DP_ACTOR_SLOT); /* $83:D176 */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_UNK_7E1291, cpu->x)));
    And8(cpu, 0xf8u); /* $83:D17B */
    Or8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x01u, cpu->y)));
    Write8(memory, AbsoluteIndexedAddress(cpu, WRAM_UNK_7E1291, cpu->x),
           A8(cpu));   /* $83:D180 */
    IncrementY16(cpu); /* $83:D183 */
    IncrementY16(cpu);
    dispatch = PrimaryRedispatch(memory, cpu, 0); /* $83:C85C */
    return PrimaryStepRedispatched(dispatch);
}

/* $1B */
static Lufia2ActorPrimaryScriptStepResult PrimarySetTimer(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT); /* $83:D188 */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x01u, cpu->y)));
    Write8(memory, LongIndexedAddress(WRAM_ACTOR_PRIMARY_TIMER, cpu->x), A8(cpu));
    IncrementY16(cpu); /* $83:D191 */
    IncrementY16(cpu);
    return Lufia2ActorPrimaryScriptExecuteKnownHandler(memory, cpu,
                                                       PRIMARY_OP_COMMIT_CURSOR);
}

/* $1C: secondary script $1C */
static Lufia2ActorPrimaryScriptStepResult
PrimaryTargetAndSecondary(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ActorScriptDispatchResult dispatch;

    LoadXDirect(memory, cpu, DP_ACTOR_SLOT); /* $83:D196 */
    LoadA8(cpu, 0x80u);                      /* $83:D198 */
    Or8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_STATE, cpu->x)));
    Write8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_STATE, cpu->x),
           A8(cpu)); /* $83:D19D */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x01u, cpu->y)));
    Write8(memory, LongIndexedAddress(WRAM_ACTOR_CLAIMED_OBJECT_RECORD, cpu->x),
           A8(cpu));
    LoadA8(cpu, 0x1cu); /* $83:D1A7 */
    Write8(memory, LongIndexedAddress(WRAM_UNK_7FE466, cpu->x), A8(cpu));
    SimulateJsrFrame(memory, cpu, 0xd1afu);         /* $83:D1AD */
    Lufia2ActorInstallSecondaryScript(memory, cpu); /* $83:D3F7 */
    SimulateRtsFrame(memory, cpu);
    IncrementY16(cpu); /* $83:D1B0 */
    IncrementY16(cpu);
    dispatch = PrimaryRedispatch(memory, cpu, 0); /* $83:C85C */
    return PrimaryStepRedispatched(dispatch);
}

/* $1F */
static Lufia2ActorPrimaryScriptStepResult
PrimaryActionIf09a1Negative(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ActorPrimaryScriptStepResult result =
        PrimaryStepStart(PRIMARY_OP_ACTION_IF_09A1_NEGATIVE);

    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_FOLLOW_SLOTS, 0)));
    /* $83:D1C1 */
    if (cpu->negative) {
        LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x01u, cpu->y)));
        PRIMARY_ACTION_OR_BOUNDARY(0xd1ccu); /* $83:D1C9 */
    }
    return Lufia2ActorPrimaryScriptExecuteKnownHandler(
        memory, cpu, PRIMARY_OP_COMMIT_CURSOR); /* $83:D1CD */
}

/* $2A */
static Lufia2ActorPrimaryScriptStepResult
PrimaryJumpByLeaderFacing(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ActorScriptDispatchResult dispatch;

    uint16_t pointer;

    IncrementY16(cpu);                         /* $83:D1D0 */
    Write16Direct(memory, cpu, DP_SCRATCH_A, cpu->y); /* $83:D1D1 */
    TransferDirectToA(cpu);                    /* $83:D1D3 */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_FACING, 0)));
    TransferAToY(cpu);           /* $83:D1D7 */
    SetAccumulatorWidth(cpu, 0); /* $83:D1D8 */
    pointer = Read16Direct(memory, cpu, DP_SCRATCH_A);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, pointer, cpu->y));
    /* $83:D1DA */
    cpu->carry = 0;                               /* $83:D1DC */
    Add16Immediate(cpu, PRIMARY_JUMP_BASE);       /* $83:D1DD */
    TransferAToY(cpu);                            /* $83:D1E0 */
    SetAccumulatorWidth(cpu, 1);                  /* $83:D1E1 */
    dispatch = PrimaryRedispatch(memory, cpu, 0); /* $83:C85C */
    return PrimaryStepRedispatched(dispatch);
}

/* $2B: via $83:C1A5 */
static Lufia2ActorPrimaryScriptStepResult
PrimaryFacingRelativeAction(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ActorPrimaryScriptStepResult result =
        PrimaryStepStart(PRIMARY_OP_FACING_RELATIVE_ACTION);

    LoadXDirect(memory, cpu, DP_ACTOR_SLOT); /* $83:D1E6 */
    TransferDirectToA(cpu);                  /* $83:D1E8 */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_FACING, cpu->x)));
    TransferAToX(cpu); /* $83:D1EC */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(ROM_FACING_ACTION_TABLE, cpu->x)));
    cpu->carry = 0; /* $83:D1F1 */
    Adc8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x01u, cpu->y)));
    PRIMARY_ACTION_OR_BOUNDARY(0xd1f8u); /* $83:D1F5 */
    IncrementY16(cpu);                   /* $83:D1F9 */
    IncrementY16(cpu);
    return Lufia2ActorPrimaryScriptExecuteKnownHandler(memory, cpu,
                                                       PRIMARY_OP_COMMIT_CURSOR);
}

/* $20: six compare modes */
static Lufia2ActorPrimaryScriptStepResult
PrimaryCompareTarget(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ActorScriptDispatchResult dispatch;

    uint8_t take;

    SetAccumulatorWidth(cpu, 0); /* $83:D210 */
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x04u, cpu->y));
    cpu->carry = 0;                         /* $83:D215 */
    Add16Immediate(cpu, PRIMARY_JUMP_BASE); /* $83:D216 */
    Write16Direct(memory, cpu, DP_SCRATCH_C, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1); /* $83:D21B */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x03u, cpu->y)));
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_A), A8(cpu));
    PrimaryTargetRecordIndex(memory, cpu, 0xd224u); /* $83:D222 */
    SetAccumulatorWidth(cpu, 0);                    /* $83:D225 */
    LoadA16(cpu, cpu->y);                           /* $83:D227 */
    cpu->carry = 0;                                 /* $83:D228 */
    Add16Immediate(cpu, 0x06u);                     /* $83:D229 */
    Write16Direct(memory, cpu, DP_SCRIPT_CURSOR, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1); /* $83:D22E */
    LoadA8(cpu,
           Read8(memory, LongIndexedAddress(WRAM_ACTOR_CLAIMED_OBJECT_RECORD, cpu->x)));
    ExchangeAccumulatorBytes(cpu); /* $83:D234 */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x02u, cpu->y)));
    /* $83:D235 */
    if (cpu->zero) {
        ExchangeAccumulatorBytes(cpu); /* $83:D270 */
        DecrementA8(cpu);
        Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_SCRATCH_A)));
        take = cpu->carry;
    } else {
        const uint8_t mode = A8(cpu);

        for (uint8_t n = 1; n <= 4u; ++n) { /* $83:D23A */
            Compare8(cpu, mode, n);
            if (cpu->zero)
                break;
        }
        ExchangeAccumulatorBytes(cpu);
        if (mode == 0x03u)
            LoadA8(cpu, (uint8_t)(A8(cpu) + 1u)); /* $83:D259 */
        Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_SCRATCH_A)));
        if (mode == 0x02u)
            take = cpu->zero; /* $83:D263 */
        else if (mode <= 0x04u)
            take = cpu->carry;
        else
            take = !cpu->zero; /* $83:D24B */
    }
    if (take) {
        LoadYDirect16(memory, cpu, DP_SCRATCH_C); /* $83:D278 */
        StoreYDirect16(memory, cpu, DP_SCRIPT_CURSOR);
    }
    dispatch = PrimaryRedispatch(memory, cpu, 1); /* $83:C85A */
    return PrimaryStepRedispatched(dispatch);
}

/* $21: operand & $7F:E57E */
static Lufia2ActorPrimaryScriptStepResult
PrimaryJumpUnlessMask(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ActorScriptDispatchResult dispatch;

    LoadXDirect(memory, cpu, DP_ACTOR_SLOT); /* $83:D293 */
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x01u, cpu->y)));
    IncrementY16(cpu); /* $83:D298 */
    And8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FE57E, cpu->x)));
    if (cpu->zero)
        return PrimaryStepRedispatched(
            PrimaryJumpOperand(memory, cpu, 0x02u)); /* $83:D2A4 */
    IncrementY16(cpu);                               /* $83:D29F */
    IncrementY16(cpu);
    dispatch = PrimaryRedispatch(memory, cpu, 0); /* $83:C85C */
    return PrimaryStepRedispatched(dispatch);
}

static Lufia2ActorPrimaryScriptStepResult
PrimaryUpdateTarget(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                    uint32_t handler_pc) {
    Lufia2ActorScriptDispatchResult dispatch;

    const uint16_t entry = (uint16_t)handler_pc;
    const uint32_t target = 0x7fe5a6u;

    PrimaryTargetRecordIndex(memory, cpu, (uint16_t)(entry + 2u)); /* JSR $D27F */
    if (entry == 0xd2e6u) {
        LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x02u, cpu->y)));
    } else {
        LoadA8(cpu, Read8(memory, LongIndexedAddress(target, cpu->x)));
        cpu->carry = entry == 0xd30bu;
        if (entry == 0xd2f6u)
            Adc8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x02u, cpu->y)));
        else
            Sbc8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x02u, cpu->y)));
    }
    Write8(memory, LongIndexedAddress(target, cpu->x), A8(cpu));
    IncrementY16(cpu);
    IncrementY16(cpu);
    IncrementY16(cpu);
    dispatch = PrimaryRedispatch(memory, cpu, 0); /* $83:C85C */
    return PrimaryStepRedispatched(dispatch);
}

/* $25 */
static Lufia2ActorPrimaryScriptStepResult PrimarySetTarget(const Lufia2Memory *memory,
                                                           Lufia2CpuState *cpu) {
    return PrimaryUpdateTarget(memory, cpu, PRIMARY_OP_SET_TARGET);
}

/* $26 */
static Lufia2ActorPrimaryScriptStepResult PrimaryAddTarget(const Lufia2Memory *memory,
                                                           Lufia2CpuState *cpu) {
    return PrimaryUpdateTarget(memory, cpu, PRIMARY_OP_ADD_TARGET);
}

/* $27 */
static Lufia2ActorPrimaryScriptStepResult
PrimarySubtractTarget(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return PrimaryUpdateTarget(memory, cpu, PRIMARY_OP_SUBTRACT_TARGET);
}

typedef Lufia2ActorPrimaryScriptStepResult (*PrimaryHandler)(const Lufia2Memory *memory,
                                                             Lufia2CpuState *cpu);

typedef struct PrimaryHandlerEntry {
    uint32_t pc;
    PrimaryHandler run;
} PrimaryHandlerEntry;

/* Handler entry points in ROM order. */
static const PrimaryHandlerEntry primary_handlers[] = {
    {PRIMARY_OP_ACTION_0, PrimaryAction0},
    {PRIMARY_OP_ACTION_1, PrimaryAction1},
    {PRIMARY_OP_ACTION_2, PrimaryAction2},
    {PRIMARY_OP_ACTION_3, PrimaryAction3},
    {PRIMARY_OP_ACTION_81, PrimaryAction81},
    {PRIMARY_OP_ACTION_82, PrimaryAction82},
    {PRIMARY_OP_ACTION_83, PrimaryAction83},
    {PRIMARY_OP_ACTION_84, PrimaryAction84},
    {PRIMARY_OP_ACTION_FROM_OPERAND, PrimaryActionFromOperand},
    {PRIMARY_OP_JUMP_IF_X_IN_RANGE, PrimaryJumpIfXInRange},
    {PRIMARY_OP_JUMP_IF_Y_IN_RANGE, PrimaryJumpIfYInRange},
    {PRIMARY_OP_CONDITIONAL_ACTION, PrimaryConditionalAction},
    {PRIMARY_OP_COMMIT_CURSOR, PrimaryCommitCursor},
    {PRIMARY_OP_JUMP, PrimaryJump},
    {PRIMARY_OP_JUMP_ALIAS, PrimaryJumpAlias},
    {PRIMARY_OP_OR_MASK, PrimaryOrMask},
    {PRIMARY_OP_AND_MASK, PrimaryAndMask},
    {PRIMARY_OP_INSTALL_SECONDARY, PrimaryInstallSecondary},
    {PRIMARY_OP_RANDOM_TIMER, PrimaryRandomTimer},
    {PRIMARY_OP_RANDOM_TIMER_X8, PrimaryRandomTimerX8},
    {PRIMARY_OP_STORE_E4DE, PrimaryStoreE4de},
    {PRIMARY_OP_SET_FLAG_1, PrimarySetFlag1},
    {PRIMARY_OP_CLEAR_FLAG_1, PrimaryClearFlag1},
    {PRIMARY_OP_MAP_CELL_30_TEST, PrimaryMapCell30Test},
    {PRIMARY_OP_JUMP_IF_LEADER_NEAR, PrimaryJumpIfLeaderNear},
    {PRIMARY_OP_JUMP_IF_LEADER_X, PrimaryJumpIfLeaderX},
    {PRIMARY_OP_JUMP_IF_LEADER_Y, PrimaryJumpIfLeaderY},
    {PRIMARY_OP_STEP_TOWARD_LEADER_X, PrimaryStepTowardLeaderX},
    {PRIMARY_OP_STEP_TOWARD_LEADER_Y, PrimaryStepTowardLeaderY},
    {PRIMARY_OP_RANDOM_JUMP, PrimaryRandomJump},
    {PRIMARY_OP_JUMP_F000, PrimaryJumpF000},
    {PRIMARY_OP_RANDOM_DIRECTION, PrimaryRandomDirection},
    {PRIMARY_OP_COMMIT_CURSOR_17, PrimaryCommitCursor17},
    {PRIMARY_OP_STEP_AWAY_LEADER_X, PrimaryStepAwayLeaderX},
    {PRIMARY_OP_STEP_AWAY_LEADER_Y, PrimaryStepAwayLeaderY},
    {PRIMARY_OP_COMMIT_CURSOR_3B, PrimaryCommitCursor3b},
    {PRIMARY_OP_ROTATE_FACING, PrimaryRotateFacing},
    {PRIMARY_OP_JUMP_IF_LEADER_AHEAD, PrimaryJumpIfLeaderAhead},
    {PRIMARY_OP_ACTION_FROM_D457, PrimaryActionFromD457},
    {PRIMARY_OP_ACTION_FROM_D447, PrimaryActionFromD447},
    {PRIMARY_OP_ACTION_60, PrimaryAction60},
    {PRIMARY_OP_TIMED_STEP_TOWARD_TARGET, PrimaryTimedStepTowardTarget},
    {PRIMARY_OP_STEP_TOWARD_LEADER, PrimaryStepTowardLeader},
    {PRIMARY_OP_STEP_AWAY_FROM_LEADER, PrimaryStepAwayFromLeader},
    {PRIMARY_OP_STEP_TOWARD_FOUND_ACTOR, PrimaryStepTowardFoundActor},
    {PRIMARY_OP_RECORD_POSITION, PrimaryRecordPosition},
    {PRIMARY_OP_STEP_TOWARD_POINT, PrimaryStepTowardPoint},
    {PRIMARY_OP_FIND_ACTOR_IN_RADIUS, PrimaryFindActorInRadius},
    {PRIMARY_OP_STEP_TOWARD_LISTED_POINT_13, PrimaryStepTowardListedPoint13},
    {PRIMARY_OP_STEP_TOWARD_LISTED_POINT, PrimaryStepTowardListedPoint},
    {PRIMARY_OP_WANDER, PrimaryWander},
    {PRIMARY_OP_WANDER_IN_BOX, PrimaryWanderInBox},
    {PRIMARY_OP_WALK_AHEAD_OF_LEADER, PrimaryWalkAheadOfLeader},
    {PRIMARY_OP_JUMP_IF_BLOCKED, PrimaryJumpIfBlocked},
    {PRIMARY_OP_RESET, PrimaryReset},
    {PRIMARY_OP_RESET_48, PrimaryReset48},
    {PRIMARY_OP_RESET_2F, PrimaryReset2f},
    {PRIMARY_OP_STEP_TOWARD_TARGET, PrimaryStepTowardTarget},
    {PRIMARY_OP_SET_TILE_POSITION, PrimarySetTilePosition},
    {PRIMARY_OP_PLAY_SOUND, PrimaryPlaySound},
    {PRIMARY_OP_ADD_DISPLAY_OFFSET, PrimaryAddDisplayOffset},
    {PRIMARY_OP_MOVE_FINE_POSITION, PrimaryMoveFinePosition},
    {PRIMARY_OP_MERGE_1291, PrimaryMerge1291},
    {PRIMARY_OP_SET_TIMER, PrimarySetTimer},
    {PRIMARY_OP_TARGET_AND_SECONDARY, PrimaryTargetAndSecondary},
    {PRIMARY_OP_ACTION_IF_09A1_NEGATIVE, PrimaryActionIf09a1Negative},
    {PRIMARY_OP_JUMP_BY_LEADER_FACING, PrimaryJumpByLeaderFacing},
    {PRIMARY_OP_FACING_RELATIVE_ACTION, PrimaryFacingRelativeAction},
    {PRIMARY_OP_COMPARE_TARGET, PrimaryCompareTarget},
    {PRIMARY_OP_JUMP_UNLESS_MASK, PrimaryJumpUnlessMask},
    {PRIMARY_OP_SET_TARGET, PrimarySetTarget},
    {PRIMARY_OP_ADD_TARGET, PrimaryAddTarget},
    {PRIMARY_OP_SUBTRACT_TARGET, PrimarySubtractTarget},
};

Lufia2ActorPrimaryScriptStepResult
Lufia2ActorPrimaryScriptExecuteKnownHandler(const Lufia2Memory *memory,
                                            Lufia2CpuState *cpu, uint32_t handler_pc) {
    const uint32_t entry = handler_pc & PRIMARY_PC_MASK;
    size_t i;

    for (i = 0; i < sizeof primary_handlers / sizeof primary_handlers[0]; ++i) {
        if (primary_handlers[i].pc == entry)
            return primary_handlers[i].run(memory, cpu);
    }
    cpu->resume_pc = entry;
    return PrimaryStepStart(entry);
}

/* $83:C7F8: per-frame primary script update of one actor. */
Lufia2ExecutionResult Lufia2ActorPrimaryUpdate(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;
    Lufia2ActorScriptDispatchResult dispatch;
    Lufia2ActorPrimaryFlow flow;
    uint32_t handler;
    uint32_t steps;

    result.flow = LUFIA2_EXECUTION_RETURNED;
    result.pc = 0x83c83bu;
    result.dispatches = 0;

    flow = Lufia2ActorPrimaryUpdateFrontend(memory, cpu);      /* C7F8 */
    if (flow == LUFIA2_ACTOR_PRIMARY_RETURN)
        return result;
    if (flow == LUFIA2_ACTOR_PRIMARY_CONTINUE_C808) {
        DecrementA8(cpu);                                      /* C808 */
        Write8(
            memory, AbsoluteIndexedAddress(cpu, WRAM_UNK_7E1291, cpu->x),
            A8(cpu));
    }

    dispatch = Lufia2ActorPrimaryScriptDispatch(memory, cpu);  /* C83C */
    handler = dispatch.handler_pc;
    result.dispatches = 1;
    /* A script that never yields spins the ROM forever. */
    for (steps = 0; steps < PRIMARY_STEP_LIMIT; ++steps) {
        const Lufia2ActorPrimaryScriptStepResult step =
            Lufia2ActorPrimaryScriptExecuteKnownHandler(
                memory, cpu, handler);

        if (step.flow == LUFIA2_ACTOR_PRIMARY_SCRIPT_REDISPATCHED) {
            handler = step.handler_pc;
            ++result.dispatches;
            continue;
        }
        if (step.flow == LUFIA2_ACTOR_PRIMARY_SCRIPT_CONTINUE_C8D2) {
            PullDataBank(memory, cpu);                         /* C8D2 */
            result.pc = 0x83c8d3u;
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
