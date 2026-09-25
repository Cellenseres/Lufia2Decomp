/* Field event script opcodes for actors, positions and points. */

#include "core/cpu_internal.h"
#include "lufia2/actor.h"
#include "actor/actor_internal.h"
#include "field/event_script_internal.h"
#include "system/wram.h"

/* $80:DD9B: actor X still moving (bit 7, not bit 2)? */
static uint8_t EventActorBusy(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    TransferAToX(cpu);                                         /* DD9B */
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->x);
    BitImmediate8(cpu, 0x04u);
    if (!cpu->zero)
        return 0;
    BitImmediate8(cpu, 0x80u);
    return !cpu->zero;
}

/* $5F/$68/$6B: wait one frame at a time while the actor moves. */
static unsigned EventOpWaitForActor(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    if (handler == EVENT_OP_WAIT_FOR_LEADER) {
        LoadAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, 0);      /* D4CA */
        BitImmediate8(cpu, 0x04u);
        if (!cpu->zero)
            return EVENT_OPCODE_NEXT;
        BitImmediate8(cpu, 0x80u);
        if (cpu->zero)
            return EVENT_OPCODE_NEXT;
        Lufia2EventPrevByte(memory, cpu, 0xd4d7u);
    } else {
        if (handler == EVENT_OP_WAIT_FOR_ACTOR)
            TransferDirectToA(cpu);                            /* DD86 */
        Lufia2EventNextByte(memory, cpu, (uint16_t)(handler +
            (handler == EVENT_OP_WAIT_FOR_ACTOR ? 3u : 2u)));
        Lufia2EventValue(memory, cpu, (uint16_t)(handler +
            (handler == EVENT_OP_WAIT_FOR_ACTOR ? 6u : 5u)));
        if (handler == EVENT_OP_WAIT_FOR_LISTED_ACTOR) {
            /* $7F:D72C maps the operand to an actor slot. */
            TransferAToX(cpu);                                 /* DD95 */
            TransferDirectToA(cpu);
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd72cu, cpu->x)));
        }
        if (!EventActorBusy(memory, cpu))
            return EVENT_OPCODE_NEXT;
        Lufia2EventPrevByte(memory, cpu, 0xdda9u);                   /* DDA7 */
        Lufia2EventPrevByte(memory, cpu, 0xddacu);
    }
    LoadA8(cpu, 0x01u);
    return Lufia2EventSleep(memory, cpu);
}

/* $80:DAE5: X = point operand - $E0, A = next byte, carry clear. */
static void EventPointOperands(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    TransferDirectToA(cpu);                                    /* DAE5 */
    Lufia2EventNextByte(memory, cpu, 0xdae8u);
    cpu->carry = 1;
    Sbc8(cpu, 0xe0u);
    TransferAToX(cpu);
    Lufia2EventNextByte(memory, cpu, 0xdaefu);
    cpu->carry = 0;
    SimulateRtsFrame(memory, cpu);
}

/* $83/$84: point copy $7F:D223/D263 = point X/Y + n. */
static unsigned EventOpOffsetPoint(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    const uint8_t y = handler == EVENT_OP_OFFSET_POINT_Y;

    EventPointOperands(memory, cpu, (uint16_t)(handler + 2u));
    Adc8(cpu, Read8(memory, LongIndexedAddress(
        y ? EVENT_POINT_Y : EVENT_POINT_X, cpu->x)));
    Write8(memory, LongIndexedAddress(
        y ? EVENT_POINT_D263 : EVENT_POINT_D223, cpu->x), A8(cpu));
    return EVENT_OPCODE_NEXT;
}

/* $80:CFFC: $56 = variable, $54 = point operand - $E0 + next byte. */
static void EventPointVariableOperands(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Lufia2EventNextByte(memory, cpu, 0xcffeu);                       /* CFFC */
    Lufia2EventVariable(memory, cpu, 0xd001u);
    StoreADirect8(memory, cpu, 0x56u);
    Write8(memory, DirectAddress(cpu, 0x57u), 0x00u);
    Lufia2EventNextByte(memory, cpu, 0xd008u);
    Lufia2EventValue(memory, cpu, 0xd00bu);
    cpu->carry = 1;
    Sbc8(cpu, 0xe0u);
    StoreADirect8(memory, cpu, 0x54u);
    Write8(memory, DirectAddress(cpu, 0x55u), 0x00u);
    TransferDirectToA(cpu);
    Lufia2EventNextByte(memory, cpu, 0xd016u);
    cpu->carry = 0;
    Adc8(cpu, DirectByte(memory, cpu, 0x54u));
    StoreADirect8(memory, cpu, 0x54u);
    SimulateRtsFrame(memory, cpu);
}

/* $9D/$9E: copy between point X ($54) and variable ($56). */
static unsigned EventOpCopyPointX(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    const uint8_t to_variable = handler == EVENT_OP_POINT_X_TO_VARIABLE;

    EventPointVariableOperands(memory, cpu, (uint16_t)(handler + 2u));
    LoadXDirect16(memory, cpu, to_variable ? 0x54u : 0x56u);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(
        to_variable ? EVENT_POINT_X : EVENT_VARIABLES, cpu->x)));
    LoadXDirect16(memory, cpu, to_variable ? 0x56u : 0x54u);
    Write8(memory, LongIndexedAddress(
        to_variable ? EVENT_VARIABLES : EVENT_POINT_X, cpu->x), A8(cpu));
    return EVENT_OPCODE_NEXT;
}

/* $80:BFAA: find key A in the $7E:F000 list at [X], stride B;
   carry clear = found. 0 = handoff at $80:BFBC. */
static uint8_t EventListSearch(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t return_bank,
    uint16_t return_address) {
    unsigned steps;

    SimulateJslFrame(memory, cpu, return_bank, return_address);
    PushDataBank(memory, cpu);                                 /* BFAA */
    StoreADirect8(memory, cpu, 0x54u);
    ExchangeAccumulatorBytes(cpu);
    StoreADirect8(memory, cpu, 0x5au);
    LoadA8(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xf000u, cpu->x));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    for (steps = 0; steps < EVENT_SEARCH_LIMIT; ++steps) {
        LoadAAbsolute8(memory, cpu, 0xf000u, cpu->x);          /* BFBC */
        Compare8(cpu, A8(cpu), DirectByte(memory, cpu, 0x54u));
        if (cpu->zero) {
            PullDataBank(memory, cpu);                         /* BFD3 */
            cpu->carry = 0;
            SimulateRtlFrame(memory, cpu);
            return 1;
        }
        Compare8(cpu, A8(cpu), 0xffu);
        if (cpu->zero) {
            PullDataBank(memory, cpu);                         /* BFD6 */
            cpu->carry = 1;
            SimulateRtlFrame(memory, cpu);
            return 1;
        }
        /* TXA with M=1 keeps B, which carries the high byte. */
        LoadA8(cpu, (uint8_t)cpu->x);                          /* BFC7 */
        cpu->carry = 0;
        Adc8(cpu, DirectByte(memory, cpu, 0x5au));
        if (cpu->carry) {
            ExchangeAccumulatorBytes(cpu);
            LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
            ExchangeAccumulatorBytes(cpu);
        }
        TransferAToX(cpu);
    }
    return 0;
}

/* $80:E912: actor A in the map's actor list [$7E:F022], stride 3;
   not found gives the first entry. 0 = handoff. */
static uint8_t EventFindActor(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address,
    uint32_t *handoff) {
    SimulateJsrFrame(memory, cpu, return_address);
    ExchangeAccumulatorBytes(cpu);                             /* E912 */
    LoadA8(cpu, 0x03u);
    ExchangeAccumulatorBytes(cpu);
    LoadX16(cpu, 0x0022u);
    if (!EventListSearch(memory, cpu, 0x80u, 0xe91cu)) {
        *handoff = 0x80bfbcu;
        return 0;
    }
    if (cpu->carry) {
        SetAccumulatorWidth(cpu, 0);                           /* E91F */
        LoadA16(cpu, Read16Long(memory, 0x7ef022u));
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
        cpu->carry = 0;
    }
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* $80:EA09: position operand in A (x) and B (y); 0 = handoff. */
uint8_t Lufia2EventPosition(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address,
    uint32_t *handoff) {
    SimulateJsrFrame(memory, cpu, return_address);
    StoreADirect8(memory, cpu, 0x54u);                         /* EA09 */
    Lufia2EventVariable(memory, cpu, 0xea0du);
    StoreADirect8(memory, cpu, 0x55u);
    LoadA8(cpu, DirectByte(memory, cpu, 0x54u));
    Compare8(cpu, A8(cpu), 0xfbu);
    if (cpu->zero) {
        /* $FB: this slot's own position. */
        LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd184u, cpu->x)));
        ExchangeAccumulatorBytes(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd17cu, cpu->x)));
    } else {
        TransferDirectToA(cpu);                                /* EA22 */
        LoadA8(cpu, DirectByte(memory, cpu, 0x55u));
        Compare8(cpu, A8(cpu), 0xe0u);
        if (!cpu->carry) {
            /* Other operands name map actor n - $20. */
            cpu->carry = 1;                                    /* EA37 */
            Sbc8(cpu, 0x20u);
            if (!EventFindActor(memory, cpu, 0xea3cu, handoff))
                return 0;
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7ef002u, cpu->x)));
            ExchangeAccumulatorBytes(cpu);
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7ef001u, cpu->x)));
            SimulateRtsFrame(memory, cpu);
            return 1;
        }
        cpu->carry = 1;
        Sbc8(cpu, 0xe0u);
        TransferAToX(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(EVENT_POINT_Y, cpu->x)));
        ExchangeAccumulatorBytes(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(EVENT_POINT_X, cpu->x)));
    }
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* $24/$25: spawn a secondary actor at a tile position. */
static unsigned EventOpSpawn(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler,
    uint32_t *handoff) {
    if (handler == EVENT_OP_SPAWN_AT) {
        Lufia2EventNextByte(memory, cpu, 0xd4e2u);                   /* D4E0 */
        StoreADirect8(memory, cpu, DP_PROBE_X);
        Lufia2EventNextByte(memory, cpu, 0xd4e7u);
        StoreADirect8(memory, cpu, DP_PROBE_Y);
    } else {
        Lufia2EventNextByte(memory, cpu, 0xd4eeu);                   /* D4EC */
        if (!Lufia2EventPosition(memory, cpu, 0xd4f1u, handoff))
            return EVENT_OPCODE_HANDOFF;
        StoreADirect8(memory, cpu, DP_PROBE_X);
        ExchangeAccumulatorBytes(cpu);
        StoreADirect8(memory, cpu, DP_PROBE_Y);
    }
    Write8(memory, DirectAddress(cpu, 0x90u), 0x00u);          /* D4F7 */
    Write8(memory, DirectAddress(cpu, 0x92u), 0x00u);
    LoadA8(cpu, DirectByte(memory, cpu, DP_ACTOR_SLOT));
    PushAccumulator8(memory, cpu);
    Lufia2EventNextByte(memory, cpu, 0xd500u);
    Lufia2EventValue(memory, cpu, 0xd503u);
    SimulateJslFrame(memory, cpu, 0x80u, 0xd507u);
    Lufia2ActorSpawn(memory, cpu);                             /* $83:DF87 */
    SimulateRtlFrame(memory, cpu);
    /* $83:E018: fine position of actor $A9 from $8F/$91. */
    SimulateJslFrame(memory, cpu, 0x80u, 0xd50bu);
    LoadXDirect16(memory, cpu, 0xa9u);
    SetAccumulatorWidth(cpu, 0);
    LoadADirect16(memory, cpu, DP_PROBE_X);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    Write16Long(memory, LongIndexedAddress(0x7fddfeu, cpu->x),
        cpu->accumulator);
    LoadADirect16(memory, cpu, DP_PROBE_Y);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    Write16Long(memory, LongIndexedAddress(0x7fde8eu, cpu->x),
        cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    SimulateRtlFrame(memory, cpu);
    LoadA8(cpu, Pull8(memory, cpu));                           /* D50C */
    StoreADirect8(memory, cpu, DP_ACTOR_SLOT);
    SimulateJslFrame(memory, cpu, 0x80u, 0xd512u);
    Lufia2ActorRecordOffsets(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    return EVENT_OPCODE_NEXT;
}

/* $80:E92A: box $9F-$A2 of an operand: own slot, point, map
   entity $60-$DF, else a position operand. 0 = handoff. */
uint8_t Lufia2EventArea(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address,
    uint32_t *handoff) {
    SimulateJsrFrame(memory, cpu, return_address);
    Compare8(cpu, A8(cpu), 0xfbu);                             /* E92A */
    if (cpu->zero) {
        LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd17cu, cpu->x)));
        StoreADirect8(memory, cpu, 0x9fu);
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        StoreADirect8(memory, cpu, 0xa1u);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd184u, cpu->x)));
        StoreADirect8(memory, cpu, 0xa0u);
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        StoreADirect8(memory, cpu, 0xa2u);
        SimulateRtsFrame(memory, cpu);
        return 1;
    }
    Compare8(cpu, A8(cpu), 0xe0u);                             /* E945 */
    if (cpu->carry) {
        static const uint32_t kBox[4] = {
            EVENT_POINT_X, EVENT_POINT_Y, EVENT_POINT_D223, EVENT_POINT_D263};
        unsigned i;

        cpu->carry = 1;
        Sbc8(cpu, 0xe0u);
        ExchangeAccumulatorBytes(cpu);
        LoadA8(cpu, 0x00u);
        ExchangeAccumulatorBytes(cpu);
        TransferAToX(cpu);
        for (i = 0; i < 4u; ++i) {
            LoadA8(cpu, Read8(memory, LongIndexedAddress(kBox[i], cpu->x)));
            StoreADirect8(memory, cpu, (uint8_t)(0x9fu + i));
        }
        SimulateRtsFrame(memory, cpu);
        return 1;
    }
    Compare8(cpu, A8(cpu), 0x60u);                             /* E96B */
    if (cpu->carry) {
        cpu->carry = 1;
        Sbc8(cpu, 0x60u);
        ExchangeAccumulatorBytes(cpu);
        LoadA8(cpu, 0x05u);
        ExchangeAccumulatorBytes(cpu);
        LoadX16(cpu, 0x0024u);
        if (!EventListSearch(memory, cpu, 0x80u, 0xe97cu)) {
            *handoff = 0x80bfbcu;
            return 0;
        }
        if (!cpu->carry) {
            SetAccumulatorWidth(cpu, 0);                       /* E982 */
            LoadA16(cpu, Read16Long(memory,
                LongIndexedAddress(0x7ef001u, cpu->x)));
            StoreADirect16(memory, cpu, 0x9fu);
            LoadA16(cpu, Read16Long(memory,
                LongIndexedAddress(0x7ef003u, cpu->x)));
            StoreADirect16(memory, cpu, 0xa1u);
            SetAccumulatorWidth(cpu, 1);
        }
        SimulateRtsFrame(memory, cpu);
        return 1;
    }
    if (!Lufia2EventPosition(memory, cpu, 0xe996u, handoff))        /* E994 */
        return 0;
    StoreADirect8(memory, cpu, 0x9fu);
    ExchangeAccumulatorBytes(cpu);
    StoreADirect8(memory, cpu, 0xa0u);
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* $55/$69: script point n = box of an operand ($80:E92A). */
static unsigned EventOpSetPoint(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler,
    uint32_t *handoff) {
    static const uint32_t kBox[4] = {
        EVENT_POINT_X, EVENT_POINT_Y, EVENT_POINT_D223, EVENT_POINT_D263};
    const uint8_t value = handler == EVENT_OP_SET_POINT_VALUE;
    unsigned i;

    TransferDirectToA(cpu);
    Lufia2EventNextByte(memory, cpu, (uint16_t)(handler + 3u));
    if (value)
        Lufia2EventValue(memory, cpu, 0xda9eu);
    cpu->carry = 1;
    Sbc8(cpu, 0xe0u);
    TransferAToX(cpu);
    PushIndex(memory, cpu);
    Lufia2EventNextByte(memory, cpu, value ? 0xdaa6u : 0xda8du);
    Lufia2EventValue(memory, cpu, value ? 0xdaa9u : 0xda90u);
    if (!Lufia2EventArea(memory, cpu, value ? 0xdaacu : 0xda93u, handoff))
        return EVENT_OPCODE_HANDOFF;
    cpu->x = PullIndexValue(memory, cpu);
    for (i = 0; i < 4u; ++i) {                                 /* DAAE */
        LoadA8(cpu, DirectByte(memory, cpu, (uint8_t)(0x9fu + i)));
        Write8(memory, LongIndexedAddress(kBox[i], cpu->x), A8(cpu));
    }
    return EVENT_OPCODE_NEXT;
}

/* $85: $05BD/$05BE = position, $05BF = byte, stair state reset. */
static unsigned EventOp85(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t *handoff) {
    Lufia2EventNextByte(memory, cpu, 0xdaf4u);                       /* DAF2 */
    Lufia2EventValue(memory, cpu, 0xdaf7u);
    if (!Lufia2EventPosition(memory, cpu, 0xdafau, handoff))
        return EVENT_OPCODE_HANDOFF;
    StoreAAbsolute8(memory, cpu, 0x05bdu, 0);
    ExchangeAccumulatorBytes(cpu);
    StoreAAbsolute8(memory, cpu, 0x05beu, 0);
    Lufia2EventNextByte(memory, cpu, 0xdb04u);
    StoreAAbsolute8(memory, cpu, 0x05bfu, 0);
    LoadA8(cpu, 0xffu);
    Write8(memory, 0x7fd0bfu, A8(cpu));
    return EVENT_OPCODE_NEXT;
}

/* $80:EA47: remember the running slot in $7F:D2A3. */
static void EventSaveSlot(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    PushAccumulator8(memory, cpu);                             /* EA47 */
    LoadA8(cpu, DirectByte(memory, cpu, DP_ACTOR_SLOT));
    Write8(memory, 0x7fd2a3u, A8(cpu));
    LoadA8(cpu, Pull8(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $80:EA50: back to the running slot. */
static void EventRestoreSlot(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadA8(cpu, Read8(memory, 0x7fd2a3u));                     /* EA50 */
    StoreADirect8(memory, cpu, DP_ACTOR_SLOT);
    SimulateJslFrame(memory, cpu, 0x80u, 0xea59u);
    Lufia2ActorRecordOffsets(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $83:D350 from bank 80; 0 = handoff inside it. */
static uint8_t EventActorAction(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address,
    uint32_t *handoff) {
    SimulateJslFrame(memory, cpu, 0x80u, return_address);
    cpu->program_bank = 0x83u;
    if (Lufia2ActorPrimaryActionCore(memory, cpu) !=
            LUFIA2_ACTOR_PRIMARY_ACTION_RETURN_D3AE) {
        *handoff = cpu->resume_pc;
        return 0;
    }
    SimulateRtlFrame(memory, cpu);
    cpu->program_bank = 0x80u;
    return 1;
}

/* $80:E004: $22 = direction, $23 = facing step; box of an operand
   ($80:E92A) to $8F/$91, stepped once and stored back to its point
   unless the step is negative. 0 = handoff. */
static uint8_t EventActorTarget(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address,
    uint32_t *handoff) {
    SimulateJsrFrame(memory, cpu, return_address);
    StoreADirect8(memory, cpu, 0x22u);                         /* E004 */
    ExchangeAccumulatorBytes(cpu);
    StoreADirect8(memory, cpu, 0x23u);
    Lufia2EventNextByte(memory, cpu, 0xe00bu);
    Lufia2EventValue(memory, cpu, 0xe00eu);
    StoreADirect8(memory, cpu, 0x25u);
    if (!Lufia2EventArea(memory, cpu, 0xe013u, handoff))
        return 0;
    CopyDirect8(memory, cpu, 0x9fu, DP_PROBE_X);
    CopyDirect8(memory, cpu, 0xa0u, DP_PROBE_Y);
    LoadA8(cpu, DirectByte(memory, cpu, 0x23u));
    if (!cpu->negative) {
        SimulateJslFrame(memory, cpu, 0x80u, 0xe023u);         /* E020 */
        cpu->program_bank = 0x83u;
        if (!Lufia2ActorMovementStep(memory, cpu)) {
            *handoff = cpu->resume_pc;
            return 0;
        }
        SimulateRtlFrame(memory, cpu);
        cpu->program_bank = 0x80u;
        TransferDirectToA(cpu);                                /* E024 */
        LoadA8(cpu, DirectByte(memory, cpu, 0x25u));
        cpu->carry = 1;
        Sbc8(cpu, 0xe0u);
        TransferAToX(cpu);
        LoadA8(cpu, DirectByte(memory, cpu, DP_PROBE_X));
        Write8(memory, LongIndexedAddress(EVENT_POINT_X, cpu->x), A8(cpu));
        LoadA8(cpu, DirectByte(memory, cpu, DP_PROBE_Y));
        Write8(memory, LongIndexedAddress(EVENT_POINT_Y, cpu->x), A8(cpu));
    }
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* $83:8B40: map object A ($7E:F016 list, stride 10) into
   $7F:D04A/D04C/D05F; a miss reads the list's end entry. 0 = handoff. */
static uint8_t EventMapObject(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address,
    uint32_t *handoff) {
    SimulateJslFrame(memory, cpu, 0x80u, return_address);
    ExchangeAccumulatorBytes(cpu);                             /* $83:8B40 */
    LoadA8(cpu, 0x0au);
    ExchangeAccumulatorBytes(cpu);
    LoadX16(cpu, 0x0016u);
    if (!EventListSearch(memory, cpu, 0x83u, 0x8b4au)) {
        *handoff = 0x80bfbcu;
        return 0;
    }
    SetAccumulatorWidth(cpu, 0);                               /* $83:8B4D */
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7ef002u, cpu->x)));
    Write16Long(memory, 0x7fd04au, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7ef004u, cpu->x)));
    Write16Long(memory, 0x7fd04cu, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7ef001u, cpu->x)));
    Write8(memory, 0x7fd05fu, A8(cpu));
    SimulateRtlFrame(memory, cpu);
    return 1;
}

/* $83:C108: claim a free actor (bit 2 set, $05D2 = $FF; none gives
   slot $28) at the leader's tile. */
static void EventClaimActor(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJslFrame(memory, cpu, 0x80u, 0xdfcbu);
    TransferDirectToA(cpu);                                    /* $83:C108 */
    Write8(memory, 0x7fd0a3u, A8(cpu));
    SimulateJsrFrame(memory, cpu, 0xc10fu);
    LoadX16(cpu, 0x0000u);                                     /* $83:FC3C */
    for (;;) {
        LoadAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->x);
        BitImmediate8(cpu, 0x04u);
        if (!cpu->zero) {
            LoadAAbsolute8(memory, cpu, 0x05d2u, cpu->x);
            Compare8(cpu, A8(cpu), 0xffu);
            cpu->carry = 0;
            if (cpu->zero)
                break;
        }
        IncrementX16(cpu);                                     /* $83:FC4E */
        Compare16(cpu, cpu->x, 0x0028u);
        if (cpu->zero) {
            cpu->carry = 1;
            break;
        }
    }
    SimulateRtsFrame(memory, cpu);
    StoreXDirect16(memory, cpu, DP_ACTOR_SLOT);                /* $83:C110 */
    SimulateJslFrame(memory, cpu, 0x83u, 0xc115u);
    Lufia2ActorRecordOffsets(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    LoadA8(cpu, 0xfeu);
    StoreAAbsolute8(memory, cpu, 0x05d2u, cpu->x);
    LoadA8(cpu, 0xffu);
    StoreAAbsolute8(memory, cpu, 0x066au, cpu->x);
    StoreAAbsolute8(memory, cpu, 0x1471u, cpu->x);
    LoadA8(cpu, DirectByte(memory, cpu, DP_ACTOR_SLOT));
    Write8(memory, 0x7fd0a2u, A8(cpu));
    LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);
    LoadAAbsolute8(memory, cpu, WRAM_EVENT_MAP_0692, 0);
    StoreAAbsolute8(memory, cpu, WRAM_EVENT_MAP_0692, cpu->x);
    LoadA8(cpu, 0x20u);
    Write8(memory, LongIndexedAddress(0x7fe316u, cpu->x), A8(cpu));
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->x);
    And8(cpu, 0xfbu);
    Or8(cpu, 0x22u);
    StoreAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->x);
    LoadAAbsolute8(memory, cpu, 0x0736u, cpu->x);
    Or8(cpu, 0x10u);
    StoreAAbsolute8(memory, cpu, 0x0736u, cpu->x);
    TransferDirectToA(cpu);
    Write8(memory, LongIndexedAddress(0x7fe3c6u, cpu->x), A8(cpu));
    LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);
    LoadAAbsolute8(memory, cpu, 0x06bau, 0);
    StoreAAbsolute8(memory, cpu, 0x06bau, cpu->x);
    LoadAAbsolute8(memory, cpu, 0x06e2u, 0);
    StoreAAbsolute8(memory, cpu, 0x06e2u, cpu->x);
    SimulateJslFrame(memory, cpu, 0x83u, 0xc15fu);
    Lufia2ActorSyncFinePosition(memory, cpu);                  /* $83:A746 */
    SimulateRtlFrame(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $80:DFB5: new actor for map object n at box $9F/$A0 ($80:DFC2),
   then its $7F:E4DE byte. 0 = handoff. */
static uint8_t EventPlaceActor(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address,
    uint32_t *handoff) {
    SimulateJsrFrame(memory, cpu, return_address);
    SimulateJsrFrame(memory, cpu, 0xdfb7u);                    /* DFB5 */
    StoreADirect8(memory, cpu, 0x24u);                         /* DFC2 */
    if (!EventMapObject(memory, cpu, 0xdfc7u, handoff))
        return 0;
    EventClaimActor(memory, cpu);
    LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);                 /* DFCC */
    LoadA8(cpu, (uint8_t)cpu->x);
    Write8(memory, 0x7fd133u, A8(cpu));
    LoadA8(cpu, DirectByte(memory, cpu, 0x9fu));
    StoreAAbsolute8(memory, cpu, 0x06bau, cpu->x);
    LoadA8(cpu, DirectByte(memory, cpu, 0xa0u));
    StoreAAbsolute8(memory, cpu, 0x06e2u, cpu->x);
    SimulateJslFrame(memory, cpu, 0x80u, 0xdfe0u);
    Lufia2ActorSyncFinePosition(memory, cpu);                  /* $83:A746 */
    SimulateRtlFrame(memory, cpu);
    LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);
    LoadA8(cpu, DirectByte(memory, cpu, 0x24u));
    Write8(memory, LongIndexedAddress(0x7fe5a6u, cpu->x), A8(cpu));
    LoadA8(cpu, DirectByte(memory, cpu, 0x23u));
    if (!cpu->negative)
        StoreAAbsolute8(memory, cpu, WRAM_EVENT_MAP_0692, cpu->x);
    LoadAAbsolute8(memory, cpu, 0x0736u, cpu->x);              /* DFF0 */
    And8(cpu, 0xf7u);
    Or8(cpu, 0x12u);
    StoreAAbsolute8(memory, cpu, 0x0736u, cpu->x);
    LoadA8(cpu, 0x09u);
    StoreAAbsolute8(memory, cpu, 0x070au, cpu->x);
    SimulateJslFrame(memory, cpu, 0x80u, 0xe002u);
    Lufia2ActorLoadPrimaryScript(memory, cpu);                 /* $83:D416 */
    SimulateRtlFrame(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    Lufia2EventNextByte(memory, cpu, 0xdfbau);                       /* DFB8 */
    LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);
    Write8(memory, LongIndexedAddress(0x7fe4deu, cpu->x), A8(cpu));
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* $22 + base: the action handed to $83:D350. */
static void EventActionCode(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t base) {
    LoadA8(cpu, DirectByte(memory, cpu, 0x22u));
    cpu->carry = 0;
    Adc8(cpu, base);
}

/* Step (B) and direction (A) of the placement opcodes. */
static uint16_t EventActorDirection(uint16_t handler) {
    switch (handler) {
    case EVENT_OP_PLACE_ACTOR_64:
    case EVENT_OP_PLACE_ACTOR_7C:
    case EVENT_OP_PLACE_ACTOR_A3:
    case EVENT_OP_MOVE_ACTOR_AF:
        return 0x0400u;
    case EVENT_OP_PLACE_ACTOR_65:
    case EVENT_OP_PLACE_ACTOR_7D:
    case EVENT_OP_PLACE_ACTOR_A4:
    case EVENT_OP_MOVE_ACTOR_B0:
        return 0x0001u;
    case EVENT_OP_PLACE_ACTOR_66:
    case EVENT_OP_PLACE_ACTOR_7E:
    case EVENT_OP_PLACE_ACTOR_A5:
    case EVENT_OP_MOVE_ACTOR_B1:
        return 0x0202u;
    default:
        return 0x0603u;
    }
}

/* $64-$67 ($80:DE8C), $7C-$7F ($80:DF19), $A3-$A6 ($80:DEC4): a new
   actor for map object n at a target, action $4C + direction; the
   last two groups also set $7F:E316. */
static unsigned EventOpPlaceActor(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler,
    uint32_t *handoff) {
    const uint16_t direction = EventActorDirection(handler);

    LoadA8(cpu, (uint8_t)(direction >> 8));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, (uint8_t)direction);
    if (handler == EVENT_OP_PLACE_ACTOR_64 ||
        handler == EVENT_OP_PLACE_ACTOR_65 ||
        handler == EVENT_OP_PLACE_ACTOR_66 ||
        handler == EVENT_OP_PLACE_ACTOR_67) {
        EventSaveSlot(memory, cpu, 0xde8eu);                   /* DE8C */
        if (!EventActorTarget(memory, cpu, 0xde91u, handoff))
            return EVENT_OPCODE_HANDOFF;
        Lufia2EventNextByte(memory, cpu, 0xde94u);
        Lufia2EventVariable(memory, cpu, 0xde97u);
        if (!EventPlaceActor(memory, cpu, 0xde9au, handoff))
            return EVENT_OPCODE_HANDOFF;
        EventActionCode(memory, cpu, 0x4cu);
        if (!EventActorAction(memory, cpu, 0xdea3u, handoff))
            return EVENT_OPCODE_HANDOFF;
        EventRestoreSlot(memory, cpu, 0xdea6u);
        return EVENT_OPCODE_NEXT;
    }
    if (handler == EVENT_OP_PLACE_ACTOR_A3 ||
        handler == EVENT_OP_PLACE_ACTOR_A4 ||
        handler == EVENT_OP_PLACE_ACTOR_A5 ||
        handler == EVENT_OP_PLACE_ACTOR_A6) {
        EventSaveSlot(memory, cpu, 0xdec6u);                   /* DEC4 */
        if (!EventActorTarget(memory, cpu, 0xdec9u, handoff))
            return EVENT_OPCODE_HANDOFF;
        Lufia2EventNextByte(memory, cpu, 0xdeccu);
        Lufia2EventValue(memory, cpu, 0xdecfu);
    } else {
        EventSaveSlot(memory, cpu, 0xdf1bu);                   /* DF19 */
        if (!EventActorTarget(memory, cpu, 0xdf1eu, handoff))
            return EVENT_OPCODE_HANDOFF;
        Lufia2EventNextByte(memory, cpu, 0xdf21u);
        Lufia2EventVariable(memory, cpu, 0xdf24u);
    }
    if (!EventPlaceActor(memory, cpu, 0xdf27u, handoff))       /* DF25 */
        return EVENT_OPCODE_HANDOFF;
    EventActionCode(memory, cpu, 0x4cu);
    if (!EventActorAction(memory, cpu, 0xdf30u, handoff))
        return EVENT_OPCODE_HANDOFF;
    LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);
    Lufia2EventNextByte(memory, cpu, 0xdf35u);
    Write8(memory, LongIndexedAddress(0x7fe316u, cpu->x), A8(cpu));
    EventRestoreSlot(memory, cpu, 0xdf3cu);
    return EVENT_OPCODE_NEXT;
}

/* $80 ($80:DEEF, no step) and $81 ($80:DE11): action $5C; $80 also
   sets $7F:E316. */
static unsigned EventOpPlaceActor5C(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler,
    uint32_t *handoff) {
    const uint8_t is_80 = handler == EVENT_OP_PLACE_ACTOR_80;

    TransferDirectToA(cpu);
    EventSaveSlot(memory, cpu, is_80 ? 0xdef2u : 0xde14u);
    LoadA8(cpu, is_80 ? 0xffu : 0x06u);
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, is_80 ? 0xffu : 0x03u);
    if (!EventActorTarget(memory, cpu, is_80 ? 0xdefau : 0xde1cu, handoff))
        return EVENT_OPCODE_HANDOFF;
    Lufia2EventNextByte(memory, cpu, is_80 ? 0xdefdu : 0xde1fu);
    Lufia2EventVariable(memory, cpu, is_80 ? 0xdf00u : 0xde22u);
    if (!EventPlaceActor(memory, cpu, is_80 ? 0xdf03u : 0xde25u, handoff))
        return EVENT_OPCODE_HANDOFF;
    LoadA8(cpu, 0x5cu);
    if (!EventActorAction(memory, cpu, is_80 ? 0xdf09u : 0xde2bu, handoff))
        return EVENT_OPCODE_HANDOFF;
    if (is_80) {
        Lufia2EventNextByte(memory, cpu, 0xdf0cu);                   /* DF0A */
        LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);
        Write8(memory, LongIndexedAddress(0x7fe316u, cpu->x), A8(cpu));
    }
    EventRestoreSlot(memory, cpu, is_80 ? 0xdf15u : 0xde2eu);
    return EVENT_OPCODE_NEXT;
}

/* $AF-$B2 ($80:DF83): existing actor n goes to a target, action
   $74 + direction; $7F:E4DE and $0736 bit 4 set. */
static unsigned EventOpMoveActor(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler,
    uint32_t *handoff) {
    const uint16_t direction = EventActorDirection(handler);

    LoadA8(cpu, (uint8_t)(direction >> 8));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, (uint8_t)direction);
    EventSaveSlot(memory, cpu, 0xdf85u);                       /* DF83 */
    if (!EventActorTarget(memory, cpu, 0xdf88u, handoff))
        return EVENT_OPCODE_HANDOFF;
    Lufia2EventNextByte(memory, cpu, 0xdf8bu);
    Lufia2EventValue(memory, cpu, 0xdf8eu);
    StoreADirect8(memory, cpu, DP_ACTOR_SLOT);
    SimulateJslFrame(memory, cpu, 0x80u, 0xdf94u);
    Lufia2ActorRecordOffsets(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    EventActionCode(memory, cpu, 0x74u);
    if (!EventActorAction(memory, cpu, 0xdf9du, handoff))
        return EVENT_OPCODE_HANDOFF;
    Lufia2EventNextByte(memory, cpu, 0xdfa0u);
    LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);
    Write8(memory, LongIndexedAddress(0x7fe4deu, cpu->x), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x0736u, cpu->x);
    Or8(cpu, 0x10u);
    StoreAAbsolute8(memory, cpu, 0x0736u, cpu->x);
    EventRestoreSlot(memory, cpu, 0xdfb1u);
    return EVENT_OPCODE_NEXT;
}

/* $B6/$B7: leader $0622 bit 1 on/off, map occupancy cleared/set. */
static unsigned EventOpLeaderOccupancy(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    const uint8_t set = handler == EVENT_OP_B6;

    LoadA8(cpu, 0x02u);
    TestBitsAbsolute8(memory, cpu, WRAM_ACTOR_STATE, set);
    EventSaveSlot(memory, cpu, set ? 0xdbe4u : 0xdbfcu);
    Write8(memory, DirectAddress(cpu, DP_ACTOR_SLOT), 0x00u);
    SimulateJslFrame(memory, cpu, 0x80u, set ? 0xdbeau : 0xdc02u);
    Lufia2ActorRecordOffsets(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x80u, set ? 0xdbeeu : 0xdc06u);
    cpu->program_bank = 0x83u;
    if (set)
        Lufia2ActorClearMapOccupancy(memory, cpu);             /* $83:FA12 */
    else
        Lufia2ActorMarkMapOccupancy(memory, cpu);              /* $83:FA3F */
    cpu->program_bank = 0x80u;
    SimulateRtlFrame(memory, cpu);
    EventRestoreSlot(memory, cpu, set ? 0xdbf1u : 0xdc09u);
    return EVENT_OPCODE_NEXT;
}

/* $BD: $7F:E33E-E35D = n ($83:E033). */
static unsigned EventOpFillE33E(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2EventNextByte(memory, cpu, 0xced1u);                 /* CECF */
    SimulateJslFrame(memory, cpu, 0x80u, 0xced5u);
    LoadX16(cpu, 0x001fu);                                     /* $83:E033 */
    do {
        Write8(memory, LongIndexedAddress(0x7fe33eu, cpu->x), A8(cpu));
        LoadX16(cpu, (uint16_t)(cpu->x - 1u));
    } while (!cpu->negative);
    SimulateRtlFrame(memory, cpu);
    return EVENT_OPCODE_NEXT;
}

/* $82: point n = placed object m ($7F:D69C/D6CC, used when
   $7F:D75C bit 7), box one tile wide. */
static unsigned EventOpPointFromObject(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2EventNextByte(memory, cpu, 0xe03au);                 /* E038 */
    Lufia2EventValue(memory, cpu, 0xe03du);
    cpu->carry = 1;
    Sbc8(cpu, 0xe0u);
    StoreADirect8(memory, cpu, 0x56u);
    Write8(memory, DirectAddress(cpu, 0x57u), 0x00u);
    TransferDirectToA(cpu);
    Lufia2EventNextByte(memory, cpu, 0xe048u);
    Lufia2EventValue(memory, cpu, 0xe04bu);
    Compare8(cpu, A8(cpu), 0xffu);
    if (cpu->zero)
        return EVENT_OPCODE_NEXT;
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd75cu, cpu->x)));
    if (!cpu->negative)
        return EVENT_OPCODE_NEXT;
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd69cu, cpu->x)));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd6ccu, cpu->x)));
    LoadXDirect16(memory, cpu, 0x56u);                         /* E060 */
    Write8(memory, LongIndexedAddress(EVENT_POINT_Y, cpu->x), A8(cpu));
    LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
    Write8(memory, LongIndexedAddress(EVENT_POINT_D263, cpu->x), A8(cpu));
    ExchangeAccumulatorBytes(cpu);
    Write8(memory, LongIndexedAddress(EVENT_POINT_X, cpu->x), A8(cpu));
    LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
    Write8(memory, LongIndexedAddress(EVENT_POINT_D223, cpu->x), A8(cpu));
    return EVENT_OPCODE_NEXT;
}

/* $80:D49B: when $7F:D0A1 bit 2, clear leader $0622 bit 3 and
   record the blocked step ($83:CA68). */
static void EventLeaderUnblock(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadA8(cpu, Read8(memory, 0x7fd0a1u));                     /* D49B */
    BitImmediate8(cpu, 0x04u);
    if (!cpu->zero) {
        LoadAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, 0);
        And8(cpu, 0xf7u);
        StoreAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, 0);
        SimulateJslFrame(memory, cpu, 0x80u, 0xd4aeu);
        cpu->program_bank = 0x83u;
        Lufia2ActorBlockedEvent(memory, cpu);                  /* $83:CA68 */
        cpu->program_bank = 0x80u;
        SimulateRtlFrame(memory, cpu);
    }
    SimulateRtsFrame(memory, cpu);
}

/* $AB for actor A, as JSL $83:AB4F. */
static void EventSelectActor(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    StoreADirect8(memory, cpu, DP_ACTOR_SLOT);
    SimulateJslFrame(memory, cpu, 0x80u, return_address);
    Lufia2ActorRecordOffsets(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $23 ($80:D439): leader action n + facing offset ($83:C1A5 by
   $0692); $6A ($80:D47E): leader action n. */
static unsigned EventOpLeaderAction(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler,
    uint32_t *handoff) {
    const uint8_t facing = handler == EVENT_OP_LEADER_FACING_ACTION;

    LoadA8(cpu, DirectByte(memory, cpu, DP_ACTOR_SLOT));
    PushAccumulator8(memory, cpu);
    Write8(memory, DirectAddress(cpu, DP_ACTOR_SLOT), 0x00u);
    SimulateJslFrame(memory, cpu, 0x80u, facing ? 0xd441u : 0xd486u);
    Lufia2ActorRecordOffsets(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    Lufia2EventNextByte(memory, cpu, facing ? 0xd444u : 0xd489u);
    if (facing) {
        StoreADirect8(memory, cpu, 0x54u);                     /* D445 */
        TransferDirectToA(cpu);
        LoadAAbsolute8(memory, cpu, WRAM_EVENT_MAP_0692, 0);
        TransferAToX(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x83c1a5u, cpu->x)));
        cpu->carry = 0;
        Adc8(cpu, DirectByte(memory, cpu, 0x54u));
    }
    if (!EventActorAction(memory, cpu, facing ? 0xd456u : 0xd48du, handoff))
        return EVENT_OPCODE_HANDOFF;
    EventLeaderUnblock(memory, cpu, facing ? 0xd459u : 0xd490u);
    if (facing) {
        LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);             /* D45A */
        LoadA8(cpu, 0x08u);
        Write8(memory, LongIndexedAddress(0x7fe4deu, cpu->x), A8(cpu));
        LoadAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->x);
        And8(cpu, 0x87u);
        Or8(cpu, 0x20u);
        StoreAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->x);
        LoadAAbsolute8(memory, cpu, 0x0736u, cpu->x);
        And8(cpu, 0xfdu);
        StoreAAbsolute8(memory, cpu, 0x0736u, cpu->x);
    }
    LoadA8(cpu, Pull8(memory, cpu));
    EventSelectActor(memory, cpu, facing ? 0xd47au : 0xd497u);
    return EVENT_OPCODE_NEXT;
}

/* $AE: action n for the last claimed actor ($7F:D0A2). */
static unsigned EventOpClaimedAction(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t *handoff) {
    EventSaveSlot(memory, cpu, 0xd4b2u);                       /* D4B0 */
    LoadA8(cpu, Read8(memory, 0x7fd0a2u));
    EventSelectActor(memory, cpu, 0xd4bcu);
    Lufia2EventNextByte(memory, cpu, 0xd4bfu);
    if (!EventActorAction(memory, cpu, 0xd4c3u, handoff))
        return EVENT_OPCODE_HANDOFF;
    EventRestoreSlot(memory, cpu, 0xd4c6u);
    return EVENT_OPCODE_NEXT;
}

/* $8F/$91 = a position operand ($80:EA09); 0 = handoff. */
static uint8_t EventProbePosition(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address,
    uint32_t *handoff) {
    if (!Lufia2EventPosition(memory, cpu, return_address, handoff))
        return 0;
    StoreADirect8(memory, cpu, DP_PROBE_X);
    ExchangeAccumulatorBytes(cpu);
    StoreADirect8(memory, cpu, DP_PROBE_Y);
    return 1;
}

/* $83:FB71 map cell value at $8F/$91, from bank 80. */
static void EventCellValue(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t return_bank,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, return_bank, return_address);
    cpu->program_bank = 0x83u;
    Lufia2ActorReadMapCellValue(memory, cpu);
    cpu->program_bank = 0x80u;
    SimulateRtlFrame(memory, cpu);
}

/* $0F/$6C: goto when bit 0 of the cell type ($83:FB51: $7F:D296 by
   the cell's high nibble, 0 when it is clear) is set / clear. */
static unsigned EventOpCellType(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler,
    uint32_t *handoff) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(handler + 2u));  /* JSR D2FB */
    Lufia2EventNextByte(memory, cpu, 0xd2fdu);
    if (!EventProbePosition(memory, cpu, 0xd300u, handoff))
        return EVENT_OPCODE_HANDOFF;
    PushY(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x80u, 0xd30au);
    EventCellValue(memory, cpu, 0x83u, 0xfb54u);               /* $83:FB51 */
    BitImmediate8(cpu, 0xf0u);
    if (cpu->zero) {
        TransferDirectToA(cpu);
    } else {
        TransferAToX(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd296u, cpu->x)));
    }
    SimulateRtlFrame(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    BitImmediate8(cpu, 0x01u);
    if (cpu->zero == (handler == EVENT_OP_GOTO_IF_CELL_TYPE_BIT0))
        Lufia2EventSkipWord(memory, cpu);
    else
        Lufia2EventGoto(memory, cpu);
    return EVENT_OPCODE_NEXT;
}

/* $A7: $7F:D133 = map cell value at a position. */
static unsigned EventOpCellValue(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t *handoff) {
    Lufia2EventNextByte(memory, cpu, 0xcfc1u);                 /* CFBF */
    Lufia2EventValue(memory, cpu, 0xcfc4u);
    if (!EventProbePosition(memory, cpu, 0xcfc7u, handoff))
        return EVENT_OPCODE_HANDOFF;
    EventCellValue(memory, cpu, 0x80u, 0xcfd0u);
    Write8(memory, 0x7fd133u, A8(cpu));
    return EVENT_OPCODE_NEXT;
}

/* $9C: variable n = height bits of the cell at a position. */
static unsigned EventOpCellHeight(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t *handoff) {
    Lufia2EventNextByte(memory, cpu, 0xcf3cu);                 /* CF3A */
    Lufia2EventVariable(memory, cpu, 0xcf3fu);
    StoreADirect8(memory, cpu, 0x56u);
    Lufia2EventNextByte(memory, cpu, 0xcf44u);
    Lufia2EventValue(memory, cpu, 0xcf47u);
    if (!EventProbePosition(memory, cpu, 0xcf4au, handoff))
        return EVENT_OPCODE_HANDOFF;
    SimulateJslFrame(memory, cpu, 0x80u, 0xcf53u);
    cpu->program_bank = 0x83u;
    Lufia2MapTileHeight(memory, cpu, 0xf986u);                 /* $83:F984 */
    cpu->program_bank = 0x80u;
    SimulateRtlFrame(memory, cpu);
    StoreADirect8(memory, cpu, 0x57u);
    TransferDirectToA(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x56u));
    TransferAToX(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x57u));
    Write8(memory, LongIndexedAddress(EVENT_VARIABLES, cpu->x), A8(cpu));
    return EVENT_OPCODE_NEXT;
}

/* $9F: $7F:D10B bit 7 = the two positions are equal. */
static unsigned EventOpSamePosition(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t *handoff) {
    uint8_t same = 0;

    Lufia2EventNextByte(memory, cpu, 0xcf65u);                 /* CF63 */
    Lufia2EventValue(memory, cpu, 0xcf68u);
    if (!EventProbePosition(memory, cpu, 0xcf6bu, handoff))
        return EVENT_OPCODE_HANDOFF;
    Lufia2EventNextByte(memory, cpu, 0xcf73u);
    Lufia2EventValue(memory, cpu, 0xcf76u);
    if (!Lufia2EventPosition(memory, cpu, 0xcf79u, handoff))
        return EVENT_OPCODE_HANDOFF;
    Compare8(cpu, A8(cpu), DirectByte(memory, cpu, DP_PROBE_X));
    if (cpu->zero) {
        ExchangeAccumulatorBytes(cpu);
        Compare8(cpu, A8(cpu), DirectByte(memory, cpu, DP_PROBE_Y));
        same = cpu->zero;
    }
    LoadA8(cpu, Read8(memory, 0x7fd10bu));
    if (same)
        Or8(cpu, 0x80u);                                       /* CF83 */
    else
        And8(cpu, 0x7fu);                                      /* CF90 */
    Write8(memory, 0x7fd10bu, A8(cpu));
    return EVENT_OPCODE_NEXT;
}

/* $5E: variable n = placed object at a position ($83:FB9F), $FF
   when none. */
static unsigned EventOpObjectAt(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t *handoff) {
    TransferDirectToA(cpu);                                    /* E078 */
    Lufia2EventNextByte(memory, cpu, 0xe07bu);
    StoreADirect8(memory, cpu, 0x5du);
    Write8(memory, DirectAddress(cpu, 0x5eu), 0x00u);
    Lufia2EventNextByte(memory, cpu, 0xe082u);
    if (!EventProbePosition(memory, cpu, 0xe085u, handoff))
        return EVENT_OPCODE_HANDOFF;
    SimulateJslFrame(memory, cpu, 0x80u, 0xe08eu);
    SimulateJsrFrame(memory, cpu, 0xfb9du);
    LoadX16(cpu, 0x0000u);                                     /* $83:FB9F */
    for (;;) {
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd69cu, cpu->x)));
        Compare8(cpu, A8(cpu), DirectByte(memory, cpu, DP_PROBE_X));
        if (cpu->zero) {
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd6ccu, cpu->x)));
            Compare8(cpu, A8(cpu), DirectByte(memory, cpu, DP_PROBE_Y));
            if (cpu->zero) {
                cpu->carry = 0;
                break;
            }
        }
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x0030u);
        if (cpu->zero) {
            TransferDirectToA(cpu);
            cpu->carry = 1;
            break;
        }
    }
    SimulateRtsFrame(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    LoadA8(cpu, (uint8_t)cpu->x);                              /* E08F */
    LoadXDirect16(memory, cpu, 0x5du);
    if (cpu->carry)
        LoadA8(cpu, 0xffu);
    Write8(memory, LongIndexedAddress(EVENT_VARIABLES, cpu->x), A8(cpu));
    return EVENT_OPCODE_NEXT;
}

/* Actor, position and point opcodes; the rest go to the conditions. */
unsigned Lufia2EventActorOpcode(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler,
    uint32_t *handoff) {
    switch (handler) {
    case EVENT_OP_OFFSET_POINT_X:
    case EVENT_OP_OFFSET_POINT_Y:
        return EventOpOffsetPoint(memory, cpu, handler);
    case EVENT_OP_POINT_X_TO_VARIABLE:
    case EVENT_OP_VARIABLE_TO_POINT_X:
        return EventOpCopyPointX(memory, cpu, handler);
    case EVENT_OP_WAIT_FOR_LISTED_ACTOR:
    case EVENT_OP_WAIT_FOR_ACTOR:
    case EVENT_OP_WAIT_FOR_LEADER:
        return EventOpWaitForActor(memory, cpu, handler);
    case EVENT_OP_SPAWN_AT:
    case EVENT_OP_SPAWN_AT_POSITION:
        return EventOpSpawn(memory, cpu, handler, handoff);
    case EVENT_OP_SET_POINT:
    case EVENT_OP_SET_POINT_VALUE:
        return EventOpSetPoint(memory, cpu, handler, handoff);
    case EVENT_OP_85:
        return EventOp85(memory, cpu, handoff);
    case EVENT_OP_PLACE_ACTOR_64:
    case EVENT_OP_PLACE_ACTOR_65:
    case EVENT_OP_PLACE_ACTOR_66:
    case EVENT_OP_PLACE_ACTOR_67:
    case EVENT_OP_PLACE_ACTOR_7C:
    case EVENT_OP_PLACE_ACTOR_7D:
    case EVENT_OP_PLACE_ACTOR_7E:
    case EVENT_OP_PLACE_ACTOR_7F:
    case EVENT_OP_PLACE_ACTOR_A3:
    case EVENT_OP_PLACE_ACTOR_A4:
    case EVENT_OP_PLACE_ACTOR_A5:
    case EVENT_OP_PLACE_ACTOR_A6:
        return EventOpPlaceActor(memory, cpu, handler, handoff);
    case EVENT_OP_PLACE_ACTOR_80:
    case EVENT_OP_PLACE_ACTOR_81:
        return EventOpPlaceActor5C(memory, cpu, handler, handoff);
    case EVENT_OP_MOVE_ACTOR_AF:
    case EVENT_OP_MOVE_ACTOR_B0:
    case EVENT_OP_MOVE_ACTOR_B1:
    case EVENT_OP_MOVE_ACTOR_B2:
        return EventOpMoveActor(memory, cpu, handler, handoff);
    case EVENT_OP_B6:
    case EVENT_OP_B7:
        return EventOpLeaderOccupancy(memory, cpu, handler);
    case EVENT_OP_FILL_E33E:
        return EventOpFillE33E(memory, cpu);
    case EVENT_OP_LEADER_FACING_ACTION:
    case EVENT_OP_LEADER_ACTION:
        return EventOpLeaderAction(memory, cpu, handler, handoff);
    case EVENT_OP_CLAIMED_ACTION:
        return EventOpClaimedAction(memory, cpu, handoff);
    case EVENT_OP_GOTO_IF_CELL_TYPE_BIT0:
    case EVENT_OP_GOTO_UNLESS_CELL_TYPE_BIT0:
        return EventOpCellType(memory, cpu, handler, handoff);
    case EVENT_OP_CELL_VALUE:
        return EventOpCellValue(memory, cpu, handoff);
    case EVENT_OP_CELL_HEIGHT:
        return EventOpCellHeight(memory, cpu, handoff);
    case EVENT_OP_SAME_POSITION:
        return EventOpSamePosition(memory, cpu, handoff);
    case EVENT_OP_OBJECT_AT:
        return EventOpObjectAt(memory, cpu, handoff);
    case EVENT_OP_POINT_FROM_OBJECT:
        return EventOpPointFromObject(memory, cpu);
    default:
        return Lufia2EventConditionOpcode(memory, cpu, handler, handoff);
    }
}
