/* Field event script conditions: tests, lists and branches. */

#include "core/cpu_internal.h"
#include "actor/actor_internal.h"
#include "field/event_script_internal.h"
#include "system/wram.h"

/* Operand of the last condition. */
#define EVENT_CONDITION_OPERAND 0x7fd19bu
/* Cell bits tested by the map tests; set by each opcode. */
#define DP_EVENT_CELL_MASK 0xaeu
/* Entries per list walk before an exact handoff. */
#define EVENT_LIST_LIMIT 65536u

/* $80:E4C5: result bytes into the slot; N = result. */
static void EventStoreResult(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);                 /* E4C5 */
    LoadA8(cpu, Read8(memory, EVENT_CONDITION_OPERAND));
    Write8(memory, LongIndexedAddress(0x7fd15cu, cpu->x), A8(cpu));
    LoadA8(cpu, Read8(memory, EVENT_CONDITION));
    Write8(memory, LongIndexedAddress(EVENT_SLOT_BITS, cpu->x), A8(cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $80:E4BA: store, goto when true. */
static unsigned EventGotoIfTrue(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    EventStoreResult(memory, cpu, 0xe4bcu);                    /* E4BA */
    if (cpu->negative)
        Lufia2EventGoto(memory, cpu);
    else
        Lufia2EventSkipWord(memory, cpu);
    return EVENT_OPCODE_NEXT;
}

/* $80:E4AF: store, goto when false. */
static unsigned EventGotoIfFalse(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    EventStoreResult(memory, cpu, 0xe4b1u);                    /* E4AF */
    if (cpu->negative)
        Lufia2EventSkipWord(memory, cpu);
    else
        Lufia2EventGoto(memory, cpu);
    return EVENT_OPCODE_NEXT;
}

/* $80:E40C: store the result in the slot only. */
static unsigned EventKeepResult(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);                 /* E40C */
    LoadA8(cpu, Read8(memory, EVENT_CONDITION));
    Write8(memory, LongIndexedAddress(EVENT_SLOT_BITS, cpu->x), A8(cpu));
    LoadA8(cpu, Read8(memory, EVENT_CONDITION_OPERAND));
    Write8(memory, LongIndexedAddress(0x7fd15cu, cpu->x), A8(cpu));
    return EVENT_OPCODE_NEXT;
}

/* $80:E55B: invert the result. */
static void EventNegate(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadA8(cpu, (uint8_t)(Read8(memory, EVENT_CONDITION) ^ 0xffu));
    Write8(memory, EVENT_CONDITION, A8(cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $80:E3D9: keep bit 7; bit 0 = result differs from script flag n. */
static unsigned EventCompareFlag(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    uint8_t bit;

    TransferDirectToA(cpu);                                    /* E3D9 */
    LoadA8(cpu, Read8(memory, EVENT_CONDITION_OPERAND));
    Lufia2EventFlagBit(memory, cpu, 0xe3e1u);
    StoreADirect8(memory, cpu, 0x54u);
    And8(cpu, Read8(memory, LongIndexedAddress(EVENT_SCRIPT_FLAGS, cpu->x)));
    StoreADirect8(memory, cpu, 0x55u);
    LoadA8(cpu, DirectByte(memory, cpu, 0x54u));
    And8(cpu, Read8(memory, EVENT_CONDITION));
    LoadA8(cpu, (uint8_t)(A8(cpu) ^ DirectByte(memory, cpu, 0x55u)));
    bit = cpu->zero ? 0x00u : 0x01u;
    LoadA8(cpu, (uint8_t)(Read8(memory, EVENT_CONDITION) & 0x80u));
    if (bit)
        Or8(cpu, 0x01u);                                       /* E400 */
    Write8(memory, EVENT_CONDITION, A8(cpu));
    return EventKeepResult(memory, cpu);
}

/* $80:E132: true when the leader stands on the position. */
static uint8_t EventLeaderAt(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address,
    uint32_t *handoff) {
    SimulateJsrFrame(memory, cpu, return_address);
    if (!Lufia2EventPosition(memory, cpu, 0xe134u, handoff))   /* E132 */
        return 0;
    Compare8(cpu, A8(cpu), AbsoluteByte(memory, cpu, 0x06bau, 0));
    if (cpu->zero) {
        ExchangeAccumulatorBytes(cpu);
        Compare8(cpu, A8(cpu), AbsoluteByte(memory, cpu, 0x06e2u, 0));
        if (cpu->zero) {
            LoadA8(cpu, 0xffu);
            Write8(memory, EVENT_CONDITION, A8(cpu));
        }
    }
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* Carry = b <= value < e, as CMP/BCC/BCS pairs. */
static uint8_t EventInRange(
    Lufia2CpuState *cpu, uint8_t value, uint8_t low, uint8_t high) {
    Compare8(cpu, value, low);
    if (!cpu->carry)
        return 0;
    Compare8(cpu, value, high);
    return !cpu->carry;
}

/* $80:E15A: carry = leader inside the box of an operand. */
static uint8_t EventLeaderInArea(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address,
    uint32_t *handoff) {
    SimulateJsrFrame(memory, cpu, return_address);
    if (!Lufia2EventArea(memory, cpu, 0xe15cu, handoff))       /* E15A */
        return 0;
    LoadAAbsolute8(memory, cpu, 0x06bau, 0);
    if (EventInRange(cpu, A8(cpu), DirectByte(memory, cpu, 0x9fu),
                     DirectByte(memory, cpu, 0xa1u))) {
        LoadAAbsolute8(memory, cpu, 0x06e2u, 0);
        if (EventInRange(cpu, A8(cpu), DirectByte(memory, cpu, 0xa0u),
                         DirectByte(memory, cpu, 0xa2u))) {
            cpu->carry = 1;
            SimulateRtsFrame(memory, cpu);
            return 1;
        }
    }
    cpu->carry = 0;                                            /* E175 */
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* BIT $AE: Z from A & mask, N and V from the mask. */
static void EventBitCellMask(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const uint8_t mask = DirectByte(memory, cpu, DP_EVENT_CELL_MASK);

    cpu->zero = (A8(cpu) & mask) == 0;
    cpu->negative = (mask & 0x80u) != 0;
    cpu->overflow = (mask & 0x40u) != 0;
}

/* $80:E458: carry = a map cell in the box has a $AE bit. */
static uint8_t EventCellsInArea(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address,
    uint32_t *handoff) {
    SimulateJsrFrame(memory, cpu, return_address);
    if (!Lufia2EventArea(memory, cpu, 0xe45au, handoff))       /* E458 */
        return 0;
    SetAccumulatorWidth(cpu, 0);
    LoadADirect16(memory, cpu, 0xa1u);
    Subtract16(cpu, Read16Direct(memory, cpu, 0x9fu));
    StoreADirect16(memory, cpu, 0x58u);                        /* width, height */
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, DirectByte(memory, cpu, 0x9fu));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0xa0u));
    SimulateJslFrame(memory, cpu, 0x80u, 0xe46eu);
    Lufia2MapCellIndex(memory, cpu, 0xf9abu, 0);               /* $83:F9A9 */
    SimulateRtlFrame(memory, cpu);
    do {
        CopyDirect8(memory, cpu, 0x58u, 0x55u);                /* E46F */
        StoreXDirect16(memory, cpu, 0x5du);
        do {
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7e4000u, cpu->x)));
            EventBitCellMask(memory, cpu);
            if (!cpu->zero) {
                LoadA8(cpu, 0xffu);                            /* E499 */
                Write8(memory, EVENT_CONDITION, A8(cpu));
                cpu->carry = 1;
                SimulateRtsFrame(memory, cpu);
                return 1;
            }
            IncrementX16(cpu);
            DecrementDirect8(memory, cpu, 0x55u);
        } while (!cpu->zero);
        SetAccumulatorWidth(cpu, 0);                           /* E482 */
        LoadADirect16(memory, cpu, 0x5du);
        cpu->carry = 0;
        Add16Value(cpu, Read16Long(memory, 0x0005b9u));
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
        DecrementDirect8(memory, cpu, 0x59u);
    } while (!cpu->zero);
    TransferDirectToA(cpu);                                    /* E492 */
    Write8(memory, EVENT_CONDITION, A8(cpu));
    cpu->carry = 0;
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* $80:CD8B: carry = a placed object of type $AE in box $9F-$A2;
   its position becomes the slot's. */
static void EventObjectsInBox(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    PushAndSetDataBank(memory, cpu, 0x7fu);                    /* CD8B */
    LoadX16(cpu, 0x0000u);
    for (;;) {
        LoadAAbsolute8(memory, cpu, 0xd69cu, cpu->x);          /* CD93 */
        if (EventInRange(cpu, A8(cpu), DirectByte(memory, cpu, 0x9fu),
                         DirectByte(memory, cpu, 0xa1u))) {
            LoadAAbsolute8(memory, cpu, 0xd6ccu, cpu->x);
            if (EventInRange(cpu, A8(cpu), DirectByte(memory, cpu, 0xa0u),
                             DirectByte(memory, cpu, 0xa2u))) {
                LoadA8(cpu, DirectByte(memory, cpu, DP_EVENT_CELL_MASK));
                Compare8(cpu, A8(cpu), AbsoluteByte(memory, cpu, 0xd6fcu, cpu->x));
                if (cpu->zero)
                    break;
            }
        }
        IncrementX16(cpu);                                     /* CDB0 */
        Compare16(cpu, cpu->x, 0x0030u);
        if (cpu->zero) {
            cpu->carry = 0;
            PullDataBank(memory, cpu);
            return;
        }
    }
    LoadAAbsolute8(memory, cpu, 0xd69cu, cpu->x);              /* CDB9 */
    ExchangeAccumulatorBytes(cpu);
    LoadAAbsolute8(memory, cpu, 0xd6ccu, cpu->x);
    LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);
    StoreAAbsolute8(memory, cpu, 0xd184u, cpu->x);
    ExchangeAccumulatorBytes(cpu);
    StoreAAbsolute8(memory, cpu, 0xd17cu, cpu->x);
    LoadA8(cpu, 0xffu);
    Write8(memory, EVENT_CONDITION, A8(cpu));
    cpu->carry = 1;
    PullDataBank(memory, cpu);                                 /* CDD0 */
}

/* $80:CD88: objects of type $AE in the box of an operand. */
static uint8_t EventObjectsInArea(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address,
    uint32_t *handoff) {
    SimulateJsrFrame(memory, cpu, return_address);
    if (!Lufia2EventArea(memory, cpu, 0xcd8au, handoff))       /* CD88 */
        return 0;
    EventObjectsInBox(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* $80:E8E2: $7F:D197/D199 = Y and DB (M=0). */
static void EventStorePointer(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    PushAccumulator16(memory, cpu);                            /* E8E2 */
    LoadA16(cpu, cpu->y);
    Write16Long(memory, EVENT_SCRIPT_POINTER, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    PushDataBank(memory, cpu);
    LoadA8(cpu, Pull8(memory, cpu));
    Write8(memory, EVENT_SCRIPT_BANK, A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    PullAccumulator16(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $80:E78D: Y = entry Y/2 of the (key, word) table at [base + X];
   carry set when missing. 0 = handoff at $80:E7B8. */
uint8_t Lufia2EventFindList(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    unsigned steps;
    uint8_t key;

    SimulateJslFrame(memory, cpu, 0x80u, return_address);
    PushDataBank(memory, cpu);                                 /* E78D */
    Write16Direct(memory, cpu, 0x56u, cpu->y);
    key = DirectByte(memory, cpu, 0x56u);
    cpu->carry = key & 1u;
    key >>= 1;
    Write8(memory, DirectAddress(cpu, 0x56u), key);
    SetNz8(cpu, key);
    LoadA8(cpu, Read8(memory, EVENT_SCRIPT_BASE_BANK));
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, cpu->x);
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, EVENT_SCRIPT_BASE));
    Lufia2EventSetPointer(memory, cpu, 0xe7a2u);
    EventStorePointer(memory, cpu, 0xe7a5u);
    SetAccumulatorWidth(cpu, 1);
    Lufia2EventNextWord(memory, cpu, 0xe7aau);
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, EVENT_SCRIPT_BASE));
    Lufia2EventSetPointer(memory, cpu, 0xe7b2u);
    EventStorePointer(memory, cpu, 0xe7b5u);
    SetAccumulatorWidth(cpu, 1);
    for (steps = 0;; ++steps) {
        if (steps >= EVENT_LIST_LIMIT) {
            cpu->resume_pc = 0x80e7b8u;
            return 0;
        }
        Lufia2EventNextByte(memory, cpu, 0xe7bau);             /* E7B8 */
        Compare8(cpu, A8(cpu), 0xffu);
        cpu->carry = 1;
        if (cpu->zero)
            break;
        Compare8(cpu, A8(cpu), DirectByte(memory, cpu, 0x56u));
        if (cpu->zero) {
            Lufia2EventNextWord(memory, cpu, 0xe7ceu);         /* E7CC */
            cpu->carry = 0;
            Add16Value(cpu, Read16Long(memory, EVENT_SCRIPT_BASE));
            Lufia2EventSetPointer(memory, cpu, 0xe7d6u);
            EventStorePointer(memory, cpu, 0xe7d9u);
            cpu->carry = 0;
            break;
        }
        Lufia2EventNextByte(memory, cpu, 0xe7c6u);
        Lufia2EventNextByte(memory, cpu, 0xe7c9u);
    }
    SetAccumulatorWidth(cpu, 1);                               /* E7DB */
    PullDataBank(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    return 1;
}

/* $80:E566: condition list $7F:D19B - $80; each entry + $60 is a box
   for test X (0 leader, 2 map cells, 4 objects); a hit sets the
   result. 0 = handoff. */
static uint8_t EventConditionList(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address,
    uint32_t *handoff) {
    unsigned entries;

    SimulateJsrFrame(memory, cpu, return_address);
    StoreXDirect16(memory, cpu, 0x8bu);                        /* E566 */
    LoadA8(cpu, Read8(memory, EVENT_SCRIPT_BANK));
    PushAccumulator8(memory, cpu);
    PushDataBank(memory, cpu);
    PushY(memory, cpu);
    LoadA8(cpu, Read8(memory, EVENT_CONDITION_OPERAND));
    cpu->carry = 1;
    Sbc8(cpu, 0x80u);
    AslA8(cpu);
    TransferAToY(cpu);
    LoadX16(cpu, 0x000au);
    if (!Lufia2EventFindList(memory, cpu, 0xe57eu)) {
        *handoff = cpu->resume_pc;
        return 0;
    }
    for (entries = 0;; ++entries) {
        uint16_t test;
        uint8_t found;

        if (entries >= EVENT_LIST_LIMIT) {
            *handoff = 0x80e57fu;
            return 0;
        }
        Lufia2EventNextByte(memory, cpu, 0xe581u);             /* E57F */
        Compare8(cpu, A8(cpu), 0xffu);
        if (cpu->zero)
            break;
        cpu->carry = 0;
        Adc8(cpu, 0x60u);
        LoadXDirect16(memory, cpu, 0x8bu);
        test = Read16Bank(memory, 0x80u, (uint16_t)(0xe59eu + cpu->x));
        if (test == 0xe15au)
            found = EventLeaderInArea(memory, cpu, 0xe58du, handoff);
        else if (test == 0xe458u)
            found = EventCellsInArea(memory, cpu, 0xe58du, handoff);
        else if (test == 0xcd88u)
            found = EventObjectsInArea(memory, cpu, 0xe58du, handoff);
        else {
            *handoff = 0x80e58bu;
            return 0;
        }
        if (!found)
            return 0;
        if (cpu->carry) {
            LoadA8(cpu, 0xffu);                                /* E590 */
            Write8(memory, EVENT_CONDITION, A8(cpu));
            break;
        }
    }
    cpu->y = PullIndexValue(memory, cpu);                      /* E596 */
    PullDataBank(memory, cpu);
    LoadA8(cpu, Pull8(memory, cpu));
    Write8(memory, EVENT_SCRIPT_BANK, A8(cpu));
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* Return addresses of the leader condition ($80:E0DC or $80:E105). */
typedef struct EventLeaderSites {
    uint16_t fetch, value, at, area, list;
} EventLeaderSites;

static const EventLeaderSites kLeaderSubroutine = {
    0xe0e3u, 0xe0e6u, 0xe0f9u, 0xe0feu, 0xe103u};
static const EventLeaderSites kLeaderFlag = {
    0xe10cu, 0xe10fu, 0xe122u, 0xe128u, 0xe12eu};

/* $80:E0DC / $80:E105: result = leader at a position ($00-$5F),
   in a box ($60-$7F, $E0-$FF) or in a list of boxes ($80-$DF). */
static uint8_t EventLeaderCondition(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    const EventLeaderSites *sites,
    uint32_t *handoff) {
    TransferDirectToA(cpu);
    Write8(memory, EVENT_CONDITION, A8(cpu));
    Lufia2EventNextByte(memory, cpu, sites->fetch);
    Lufia2EventValue(memory, cpu, sites->value);
    Write8(memory, EVENT_CONDITION_OPERAND, A8(cpu));
    Compare8(cpu, A8(cpu), 0xe0u);
    if (!cpu->carry) {
        Compare8(cpu, A8(cpu), 0x80u);
        if (cpu->carry) {
            /* $80:E153 */
            SimulateJsrFrame(memory, cpu, sites->list);
            LoadX16(cpu, 0x0000u);
            if (!EventConditionList(memory, cpu, 0xe158u, handoff))
                return 0;
            SimulateRtsFrame(memory, cpu);
            return 1;
        }
        Compare8(cpu, A8(cpu), 0x60u);
        if (!cpu->carry)
            return EventLeaderAt(memory, cpu, sites->at, handoff);
    }
    /* $80:E147 */
    SimulateJsrFrame(memory, cpu, sites->area);
    if (!EventLeaderInArea(memory, cpu, 0xe149u, handoff))
        return 0;
    if (cpu->carry) {
        LoadA8(cpu, 0xffu);
        Write8(memory, EVENT_CONDITION, A8(cpu));
    }
    SimulateRtsFrame(memory, cpu);
    return 1;
}


/* $80:E37B body: result = map cell at a position has a $AE bit. */
static uint8_t EventCellAt(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t *handoff) {
    if (!Lufia2EventPosition(memory, cpu, 0xe37du, handoff))   /* E37B */
        return 0;
    ExchangeAccumulatorBytes(cpu);
    SimulateJslFrame(memory, cpu, 0x80u, 0xe382u);
    Lufia2MapCellIndex(memory, cpu, 0xf9abu, 0);               /* $83:F9A9 */
    SimulateRtlFrame(memory, cpu);
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7e4000u, cpu->x)));
    EventBitCellMask(memory, cpu);
    if (!cpu->zero) {
        LoadA8(cpu, 0xffu);
        ExchangeAccumulatorBytes(cpu);
    }
    /* A bit-clear cell leaves B, the DP high byte, as the result. */
    ExchangeAccumulatorBytes(cpu);                             /* E38F */
    Write8(memory, EVENT_CONDITION, A8(cpu));
    return 1;
}

/* $80:E395 body: map cells in a box. */
static uint8_t EventCellBox(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t *handoff) {
    return EventCellsInArea(memory, cpu, 0xe397u, handoff);
}

/* $80:E399 body: map cells in a list of boxes. */
static uint8_t EventCellList(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t *handoff) {
    LoadX16(cpu, 0x0002u);
    return EventConditionList(memory, cpu, 0xe39eu, handoff);
}

/* $80:E365: map cell condition with mask $AE: at a position ($00-$5F),
   in a box ($60-$7F, $E0-$FF) or in a list of boxes ($80-$DF). */
static uint8_t EventCellCondition(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address,
    uint32_t *handoff) {
    uint8_t done;

    SimulateJsrFrame(memory, cpu, return_address);
    Lufia2EventNextByte(memory, cpu, 0xe367u);                 /* E365 */
    Lufia2EventValue(memory, cpu, 0xe36au);
    Write8(memory, EVENT_CONDITION_OPERAND, A8(cpu));
    Compare8(cpu, A8(cpu), 0xe0u);
    if (cpu->carry) {
        done = EventCellBox(memory, cpu, handoff);
    } else {
        Compare8(cpu, A8(cpu), 0x80u);
        if (cpu->carry) {
            done = EventCellList(memory, cpu, handoff);
        } else {
            Compare8(cpu, A8(cpu), 0x60u);
            done = cpu->carry ? EventCellBox(memory, cpu, handoff)
                              : EventCellAt(memory, cpu, handoff);
        }
    }
    if (!done)
        return 0;
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* $80:CD41: result = placed object of type n + $10 at a position
   (one-tile box), in a box or in a list of boxes. */
static uint8_t EventObjectCondition(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address,
    uint32_t *handoff) {
    SimulateJsrFrame(memory, cpu, return_address);
    TransferDirectToA(cpu);                                    /* CD41 */
    Write8(memory, EVENT_CONDITION, A8(cpu));
    Lufia2EventNextByte(memory, cpu, 0xcd48u);
    Lufia2EventValue(memory, cpu, 0xcd4bu);
    Write8(memory, EVENT_CONDITION_OPERAND, A8(cpu));
    Lufia2EventNextByte(memory, cpu, 0xcd52u);
    cpu->carry = 0;
    Adc8(cpu, 0x10u);
    StoreADirect8(memory, cpu, DP_EVENT_CELL_MASK);
    LoadA8(cpu, Read8(memory, EVENT_CONDITION_OPERAND));
    Compare8(cpu, A8(cpu), 0xe0u);
    if (!cpu->carry) {
        Compare8(cpu, A8(cpu), 0x80u);
        if (cpu->carry) {
            LoadX16(cpu, 0x0004u);                             /* CD7A */
            if (!EventConditionList(memory, cpu, 0xcd7fu, handoff))
                return 0;
            SimulateRtsFrame(memory, cpu);
            return 1;
        }
        Compare8(cpu, A8(cpu), 0x60u);
        if (!cpu->carry) {
            if (!Lufia2EventPosition(memory, cpu, 0xcd6au, handoff))
                return 0;
            StoreADirect8(memory, cpu, 0x9fu);                 /* CD6B */
            LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
            StoreADirect8(memory, cpu, 0xa1u);
            ExchangeAccumulatorBytes(cpu);
            StoreADirect8(memory, cpu, 0xa0u);
            LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
            StoreADirect8(memory, cpu, 0xa2u);
            SimulateJsrFrame(memory, cpu, 0xcd78u);
            EventObjectsInBox(memory, cpu);
            SimulateRtsFrame(memory, cpu);
            SimulateRtsFrame(memory, cpu);
            return 1;
        }
    }
    if (!Lufia2EventArea(memory, cpu, 0xcd83u, handoff))       /* CD81 */
        return 0;
    SimulateJsrFrame(memory, cpu, 0xcd86u);
    EventObjectsInBox(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* $05/$04/$70: object condition vs flag, goto if true, if false. */
static unsigned EventOpObjects(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler,
    uint32_t *handoff) {
    if (!EventObjectCondition(memory, cpu, (uint16_t)(handler + 2u), handoff))
        return EVENT_OPCODE_HANDOFF;
    if (handler == EVENT_OP_FLAG_OBJECTS)
        return EventCompareFlag(memory, cpu);
    LoadA8(cpu, Read8(memory, EVENT_CONDITION));               /* CD26 */
    if (cpu->negative == (handler == EVENT_OP_GOTO_IF_OBJECTS))
        Lufia2EventGoto(memory, cpu);
    else
        Lufia2EventSkipWord(memory, cpu);
    return EVENT_OPCODE_NEXT;
}

/* $80:E1CE: result $01, or $81 when $7F:D0A1 bit 0 and n = $7F:D0F4. */
static void EventD0F4Condition(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Lufia2EventNextByte(memory, cpu, 0xe1d0u);                 /* E1CE */
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, 0x01u);
    Write8(memory, EVENT_CONDITION, A8(cpu));
    LoadA8(cpu, Read8(memory, 0x7fd0a1u));
    BitImmediate8(cpu, 0x01u);
    if (!cpu->zero) {
        ExchangeAccumulatorBytes(cpu);
        Compare8(cpu, A8(cpu), Read8(memory, 0x7fd0f4u));
        if (cpu->zero) {
            LoadA8(cpu, 0x81u);
            Write8(memory, EVENT_CONDITION, A8(cpu));
        }
    }
    SimulateRtsFrame(memory, cpu);
}

enum {
    EVENT_THEN_COMPARE_FLAG,        /* $80:E3D9 */
    EVENT_THEN_GOTO_IF_TRUE,        /* $80:E4BA */
    EVENT_THEN_KEEP                 /* $80:E40C */
};

static unsigned EventConditionTail(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    unsigned tail) {
    if (tail == EVENT_THEN_COMPARE_FLAG)
        return EventCompareFlag(memory, cpu);
    if (tail == EVENT_THEN_GOTO_IF_TRUE)
        return EventGotoIfTrue(memory, cpu);
    return EventKeepResult(memory, cpu);
}

/* Map cell opcodes: mask, optional inversion and tail. */
static const struct {
    uint16_t handler;
    uint8_t mask;
    uint8_t negate;
    uint8_t tail;
} kEventCellOps[9] = {
    {EVENT_OP_FLAG_CELLS_09, 0x09u, 0, EVENT_THEN_COMPARE_FLAG},
    {EVENT_OP_GOTO_IF_CELLS_09, 0x09u, 0, EVENT_THEN_GOTO_IF_TRUE},
    {EVENT_OP_GOTO_UNLESS_CELLS_09, 0x09u, 1, EVENT_THEN_GOTO_IF_TRUE},
    {EVENT_OP_FLAG_CELLS_08, 0x08u, 0, EVENT_THEN_COMPARE_FLAG},
    {EVENT_OP_GOTO_IF_CELLS_08, 0x08u, 0, EVENT_THEN_GOTO_IF_TRUE},
    {EVENT_OP_GOTO_UNLESS_CELLS_08, 0x08u, 1, EVENT_THEN_GOTO_IF_TRUE},
    {EVENT_OP_FLAG_CELLS_01, 0x01u, 0, EVENT_THEN_COMPARE_FLAG},
    {EVENT_OP_GOTO_IF_CELLS_01, 0x01u, 0, EVENT_THEN_GOTO_IF_TRUE},
    {EVENT_OP_GOTO_UNLESS_CELLS_01, 0x01u, 1, EVENT_THEN_GOTO_IF_TRUE},
};

/* $80:E203: result $FF when two positions hold the same tile in
   any layer of mask n ($7F:D008 layer bases). 0 = handoff. */
static uint8_t EventSameTiles(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address,
    uint32_t *handoff) {
    static const uint16_t kReturns[2][4] = {
        {0xe20cu, 0xe20fu, 0xe212u, 0xe217u},
        {0xe21cu, 0xe21fu, 0xe222u, 0xe227u}};
    unsigned i;

    SimulateJsrFrame(memory, cpu, return_address);
    Lufia2EventNextByte(memory, cpu, 0xe205u);                 /* E203 */
    StoreADirect8(memory, cpu, 0x60u);
    Write8(memory, DirectAddress(cpu, 0x61u), 0x00u);
    for (i = 0; i < 2u; ++i) {
        Lufia2EventNextByte(memory, cpu, kReturns[i][0]);
        Lufia2EventValue(memory, cpu, kReturns[i][1]);
        if (!Lufia2EventPosition(memory, cpu, kReturns[i][2], handoff))
            return 0;
        ExchangeAccumulatorBytes(cpu);
        SimulateJslFrame(memory, cpu, 0x80u, kReturns[i][3]);  /* $83:F9EE */
        SimulateJsrFrame(memory, cpu, 0xf9f0u);
        Lufia2MapCellOffset(memory, cpu);
        SimulateRtsFrame(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        Write16Direct(memory, cpu, i ? 0x56u : 0x58u, cpu->x);
    }
    SetAccumulatorWidth(cpu, 0);                               /* E22A */
    Write16Direct(memory, cpu, 0x5du, 0x0000u);
    Write16Direct(memory, cpu, 0x5au, 0x0000u);
    do {
        uint16_t mask = Read16Direct(memory, cpu, 0x60u);

        cpu->carry = mask & 1u;                                /* E230 */
        mask >>= 1;
        Write16Direct(memory, cpu, 0x60u, mask);
        SetNz16(cpu, mask);
        if (cpu->carry) {
            LoadXDirect16(memory, cpu, 0x5au);
            LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fd008u, cpu->x)));
            StoreADirect16(memory, cpu, 0x63u);
            cpu->carry = 0;
            Add16Value(cpu, Read16Direct(memory, cpu, 0x58u));
            TransferAToX(cpu);
            LoadA16(cpu, (uint16_t)(Read16Long(memory,
                LongIndexedAddress(0x7f0000u, cpu->x)) & 0x03ffu));
            StoreADirect16(memory, cpu, 0x65u);
            LoadA16(cpu, Read16Direct(memory, cpu, 0x63u));    /* E249 */
            cpu->carry = 0;
            Add16Value(cpu, Read16Direct(memory, cpu, 0x56u));
            TransferAToX(cpu);
            LoadA16(cpu, (uint16_t)(Read16Long(memory,
                LongIndexedAddress(0x7f0000u, cpu->x)) & 0x03ffu));
            Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, 0x65u));
            if (cpu->zero) {
                LoadA16(cpu, Read16Direct(memory, cpu, 0x5du));
                Compare16(cpu, cpu->accumulator, 0xffffu);
                if (!cpu->zero) {
                    LoadA16(cpu, 0xffffu);
                    StoreADirect16(memory, cpu, 0x5du);
                }
            }
        }
        Write16Direct(memory, cpu, 0x5au,                      /* E266 */
            (uint16_t)(Read16Direct(memory, cpu, 0x5au) + 1u));
        Write16Direct(memory, cpu, 0x5au,
            (uint16_t)(Read16Direct(memory, cpu, 0x5au) + 1u));
        SetNz16(cpu, Read16Direct(memory, cpu, 0x5au));
        LoadA16(cpu, Read16Direct(memory, cpu, 0x60u));
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);                               /* E26E */
    LoadA8(cpu, DirectByte(memory, cpu, 0x5du));
    Write8(memory, EVENT_CONDITION, A8(cpu));
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* $87 keeps, $88 gotos on, $89 gotos unless the same tiles. */
static unsigned EventOpSameTiles(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler,
    uint32_t *handoff) {
    if (!EventSameTiles(memory, cpu, (uint16_t)(handler + 2u), handoff))
        return EVENT_OPCODE_HANDOFF;
    if (handler == EVENT_OP_KEEP_SAME_TILES)
        return EventKeepResult(memory, cpu);
    if (handler == EVENT_OP_GOTO_UNLESS_SAME_TILES)
        EventNegate(memory, cpu, 0xe1ffu);
    return EventGotoIfTrue(memory, cpu);
}

/* $80:E2ED: result 0 and carry when the $D04C x $D04D tile block at
   $5D differs from the one at $60 in layer X; DB = $7F. */
static void EventBlockDiffers(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    PushY(memory, cpu);                                        /* E2ED */
    LoadAAbsolute8(memory, cpu, 0xd04du, 0);
    StoreADirect8(memory, cpu, 0x5au);
    Write8(memory, DirectAddress(cpu, 0x5bu), 0x00u);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xd008u, cpu->x));
    StoreADirect16(memory, cpu, 0x54u);
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x5du));
    TransferAToX(cpu);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x54u));
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x60u));
    TransferAToY(cpu);
    do {
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xd04cu, 0));  /* E306 */
        And16(cpu, 0x00ffu);
        StoreADirect16(memory, cpu, 0x58u);
        do {
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x));
            And16(cpu, 0x03ffu);
            StoreADirect16(memory, cpu, 0x54u);
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->y));
            And16(cpu, 0x03ffu);
            Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, 0x54u));
            if (!cpu->zero) {
                cpu->carry = 1;                                /* E338 */
                SetAccumulatorWidth(cpu, 1);
                TransferDirectToA(cpu);
                Write8(memory, EVENT_CONDITION, A8(cpu));
                cpu->y = PullIndexValue(memory, cpu);
                SimulateRtsFrame(memory, cpu);
                return;
            }
            IncrementX16(cpu);
            IncrementX16(cpu);
            IncrementY16(cpu);
            IncrementY16(cpu);
            Decrement16Direct(memory, cpu, 0x58u);
        } while (!cpu->zero);
        LoadA16(cpu, cpu->x);                                  /* E328 */
        cpu->carry = 0;
        Add16Value(cpu, Read16Direct(memory, cpu, 0x56u));
        TransferAToX(cpu);
        LoadA16(cpu, cpu->y);
        Add16Value(cpu, Read16Direct(memory, cpu, 0x56u));
        TransferAToY(cpu);
        Decrement16Direct(memory, cpu, 0x5au);
    } while (!cpu->zero);
    cpu->carry = 0;
    cpu->y = PullIndexValue(memory, cpu);                      /* E340 */
    SetAccumulatorWidth(cpu, 1);
    SimulateRtsFrame(memory, cpu);
}

/* $80:E28C: result $FF unless the tiles at a position differ from
   map object n's block in its layers ($7F:D05F bits 0-1). */
static uint8_t EventBlockMatches(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address,
    uint32_t *handoff) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadA8(cpu, 0xffu);                                        /* E28C */
    Write8(memory, EVENT_CONDITION, A8(cpu));
    Lufia2EventNextByte(memory, cpu, 0xe294u);
    Lufia2EventValue(memory, cpu, 0xe297u);
    if (!Lufia2EventPosition(memory, cpu, 0xe29au, handoff))
        return 0;
    ExchangeAccumulatorBytes(cpu);
    SimulateJslFrame(memory, cpu, 0x80u, 0xe29fu);             /* $83:F9EE */
    SimulateJsrFrame(memory, cpu, 0xf9f0u);
    Lufia2MapCellOffset(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    StoreXDirect16(memory, cpu, 0x5du);
    Lufia2EventNextByte(memory, cpu, 0xe2a4u);                 /* E2A2 */
    if (!Lufia2EventMapObject(memory, cpu, 0xe2a8u, handoff))
        return 0;
    LoadA8(cpu, Read8(memory, 0x7fd04au));                     /* E2A9 */
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Read8(memory, 0x7fd04bu));
    SimulateJslFrame(memory, cpu, 0x80u, 0xe2b5u);
    SimulateJsrFrame(memory, cpu, 0xf9f0u);
    Lufia2MapCellOffset(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    StoreXDirect16(memory, cpu, 0x60u);
    TransferDirectToA(cpu);                                    /* E2B8 */
    LoadAAbsolute8(memory, cpu, 0x05b9u, 0);
    cpu->carry = 1;
    Sbc8(cpu, Read8(memory, 0x7fd04cu));
    SetAccumulatorWidth(cpu, 0);
    AslA16(cpu);
    StoreADirect16(memory, cpu, 0x56u);
    SetAccumulatorWidth(cpu, 1);
    PushDataBank(memory, cpu);                                 /* E2C8 */
    LoadA8(cpu, 0x7fu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    LoadA8(cpu, Read8(memory, 0x7fd05fu));
    BitImmediate8(cpu, 0x01u);
    if (!cpu->zero) {
        LoadX16(cpu, 0x0000u);
        EventBlockDiffers(memory, cpu, 0xe2dau);
        if (cpu->carry) {
            PullDataBank(memory, cpu);
            SimulateRtsFrame(memory, cpu);
            return 1;
        }
        LoadA8(cpu, Read8(memory, 0x7fd05fu));
    }
    BitImmediate8(cpu, 0x02u);                                 /* E2E1 */
    if (!cpu->zero) {
        LoadX16(cpu, 0x0002u);
        EventBlockDiffers(memory, cpu, 0xe2eau);
    }
    PullDataBank(memory, cpu);                                 /* E2EB */
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* Read-only worst case of $80:E2ED for $94-$96: the map object
   ($80:BFAA on $7E:F016, stride 10) gives the block size and layers;
   0 counts as 65536. */
static uint64_t EventBlockCompares(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu) {
    const uint8_t key = Lufia2EventPeekByte(memory, cpu, 1u);
    uint16_t x = Read16Long(memory, 0x7ef016u);
    uint64_t width, height;
    uint8_t layers;
    unsigned steps;

    for (steps = 0; steps < EVENT_SEARCH_LIMIT; ++steps) {
        const uint8_t value = Read8(memory, 0x7ef000u + x);

        if (value == key || value == 0xffu)
            break;
        x = (uint16_t)(x + 10u);
    }
    if (steps == EVENT_SEARCH_LIMIT)
        return 0;
    layers = Read8(memory, LongIndexedAddress(0x7ef001u, x));
    width = Read8(memory, LongIndexedAddress(0x7ef004u, x));
    height = Read8(memory, LongIndexedAddress(0x7ef005u, x));
    return (uint64_t)((layers & 1u) + ((layers >> 1) & 1u)) *
           (width ? width : 65536u) * (height ? height : 65536u);
}

/* $94 keeps, $95 gotos on, $96 gotos unless the block matches. */
static unsigned EventOpBlockMatches(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler,
    uint32_t *handoff) {
    /* Over 4096 tile pairs hand off before the opcode. */
    if (EventBlockCompares(memory, cpu) > EVENT_OPCODE_LIMIT)
        return EVENT_OPCODE_HANDOFF;
    if (!EventBlockMatches(memory, cpu, (uint16_t)(handler + 2u), handoff))
        return EVENT_OPCODE_HANDOFF;
    if (handler == EVENT_OP_KEEP_BLOCK_MATCH)
        return EventKeepResult(memory, cpu);
    if (handler == EVENT_OP_GOTO_UNLESS_BLOCK_MATCH)
        EventNegate(memory, cpu, 0xe288u);
    return EventGotoIfTrue(memory, cpu);
}

/* $80:BF92: slot of actor id A in $05FA (DB-relative) into $A7;
   carry set and X = $28 when missing. */
static void EventFindActorId(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x80u, return_address);
    for (LoadX16(cpu, 0x0000u);;) {                            /* BF92 */
        Compare8(cpu, A8(cpu),
            Read8(memory, AbsoluteIndexedAddress(cpu, 0x05fau, cpu->x)));
        if (cpu->zero) {
            StoreXDirect16(memory, cpu, DP_ACTOR_SLOT);        /* BFA2 */
            SimulateJslFrame(memory, cpu, 0x80u, 0xbfa7u);
            Lufia2ActorRecordOffsets(memory, cpu);             /* $84:82D5 */
            SimulateRtlFrame(memory, cpu);
            cpu->carry = 0;
            break;
        }
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x0028u);
        if (cpu->zero) {
            cpu->carry = 1;
            break;
        }
    }
    SimulateRtlFrame(memory, cpu);
}

/* $20: goto unless every listed actor id (+$4F, to $FF) has $0736
   bit 5; a missing id tests entry $28. */
static unsigned EventOpGotoUnlessActorsBit5(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t *handoff) {
    unsigned listed = 0;

    /* Over 64 ids (the table has 40) hands off before the opcode. */
    if (!Lufia2EventListEnds(memory, cpu, 65u))
        return EVENT_OPCODE_HANDOFF;
    LoadA8(cpu, DirectByte(memory, cpu, DP_ACTOR_SLOT));       /* E09D */
    PushAccumulator8(memory, cpu);
    LoadA8(cpu, 0xffu);
    Write8(memory, EVENT_CONDITION, A8(cpu));
    for (;;) {
        /* No $FF in any bank loops forever. */
        if (listed++ >= 0x01000000u) {
            *handoff = 0x80e0a6u;
            return EVENT_OPCODE_HANDOFF;
        }
        TransferDirectToA(cpu);                                /* E0A6 */
        Lufia2EventNextByte(memory, cpu, 0xe0a9u);
        Compare8(cpu, A8(cpu), 0xffu);
        if (cpu->zero)
            break;
        cpu->carry = 0;
        Adc8(cpu, 0x4fu);
        EventFindActorId(memory, cpu, 0xe0b4u);
        LoadAAbsolute8(memory, cpu, 0x0736u, cpu->x);
        BitImmediate8(cpu, 0x20u);
        if (cpu->zero) {
            TransferDirectToA(cpu);                            /* E0BC */
            Write8(memory, EVENT_CONDITION, A8(cpu));
        }
    }
    LoadA8(cpu, Pull8(memory, cpu));                           /* E0C3 */
    StoreADirect8(memory, cpu, DP_ACTOR_SLOT);
    SimulateJslFrame(memory, cpu, 0x80u, 0xe0c9u);
    Lufia2ActorRecordOffsets(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    return EventGotoIfFalse(memory, cpu);
}

/* $15-$18, $6E, $6F, $72-$74: map cell conditions. */
static unsigned EventOpCells(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    unsigned index,
    uint32_t *handoff) {
    const uint16_t handler = kEventCellOps[index].handler;

    LoadA8(cpu, kEventCellOps[index].mask);
    StoreADirect8(memory, cpu, DP_EVENT_CELL_MASK);
    if (!EventCellCondition(memory, cpu, (uint16_t)(handler + 6u), handoff))
        return EVENT_OPCODE_HANDOFF;
    if (kEventCellOps[index].negate)
        EventNegate(memory, cpu, (uint16_t)(handler + 9u));
    return EventConditionTail(memory, cpu, kEventCellOps[index].tail);
}

/* $14: goto when script flag n is set, else when the map cell
   condition (mask 9) is false. */
static unsigned EventOp14(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t *handoff) {
    uint8_t done;

    LoadA8(cpu, 0x09u);                                        /* E3A0 */
    StoreADirect8(memory, cpu, DP_EVENT_CELL_MASK);
    Lufia2EventNextByte(memory, cpu, 0xe3a6u);
    Lufia2EventValue(memory, cpu, 0xe3a9u);
    Write8(memory, EVENT_CONDITION_OPERAND, A8(cpu));
    Lufia2EventFlagBit(memory, cpu, 0xe3b1u);
    And8(cpu, Read8(memory, LongIndexedAddress(EVENT_SCRIPT_FLAGS, cpu->x)));
    if (!cpu->zero) {
        Lufia2EventGoto(memory, cpu);
        return EVENT_OPCODE_NEXT;
    }
    LoadA8(cpu, Read8(memory, EVENT_CONDITION_OPERAND));       /* E3BB */
    Compare8(cpu, A8(cpu), 0x80u);
    if (cpu->carry) {
        SimulateJsrFrame(memory, cpu, 0xe3d5u);
        done = EventCellList(memory, cpu, handoff);
    } else {
        Compare8(cpu, A8(cpu), 0x60u);
        if (cpu->carry) {
            SimulateJsrFrame(memory, cpu, 0xe3cfu);
            done = EventCellBox(memory, cpu, handoff);
        } else {
            SimulateJsrFrame(memory, cpu, 0xe3c9u);
            done = EventCellAt(memory, cpu, handoff);
        }
    }
    if (!done)
        return EVENT_OPCODE_HANDOFF;
    SimulateRtsFrame(memory, cpu);
    return EventGotoIfFalse(memory, cpu);
}

/* $12/$6D/$13: leader position conditions. */
static unsigned EventOpLeader(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler,
    uint32_t *handoff) {
    if (handler == EVENT_OP_FLAG_LEADER_AT) {
        if (!EventLeaderCondition(memory, cpu, &kLeaderFlag, handoff))
            return EVENT_OPCODE_HANDOFF;                       /* E105 */
        return EventCompareFlag(memory, cpu);
    }
    SimulateJsrFrame(memory, cpu, (uint16_t)(handler + 2u));  /* JSR E0DC */
    if (!EventLeaderCondition(memory, cpu, &kLeaderSubroutine, handoff))
        return EVENT_OPCODE_HANDOFF;
    SimulateRtsFrame(memory, cpu);
    if (handler == EVENT_OP_GOTO_UNLESS_LEADER_AT)
        EventNegate(memory, cpu, 0xe0d8u);
    return EventGotoIfTrue(memory, cpu);
}

/* $75-$77: the $7F:D0F4 condition kept, or goto if (not) true. */
static unsigned EventOpD0F4(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    EventD0F4Condition(memory, cpu, (uint16_t)(handler + 2u));
    if (handler == EVENT_OP_KEEP_D0F4)
        return EventKeepResult(memory, cpu);
    if (handler == EVENT_OP_GOTO_UNLESS_D0F4)
        EventNegate(memory, cpu, 0xe1cau);
    return EventGotoIfTrue(memory, cpu);
}

/* Condition opcodes; others hand off. */
unsigned Lufia2EventConditionOpcode(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler,
    uint32_t *handoff) {
    unsigned i;

    for (i = 0; i < 9u; ++i)
        if (handler == kEventCellOps[i].handler)
            return EventOpCells(memory, cpu, i, handoff);
    switch (handler) {
    case EVENT_OP_GOTO_IF_LEADER_AT:
    case EVENT_OP_GOTO_UNLESS_LEADER_AT:
    case EVENT_OP_FLAG_LEADER_AT:
        return EventOpLeader(memory, cpu, handler, handoff);
    case EVENT_OP_14:
        return EventOp14(memory, cpu, handoff);
    case EVENT_OP_FLAG_OBJECTS:
    case EVENT_OP_GOTO_IF_OBJECTS:
    case EVENT_OP_GOTO_UNLESS_OBJECTS:
        return EventOpObjects(memory, cpu, handler, handoff);
    case EVENT_OP_KEEP_D0F4:
    case EVENT_OP_GOTO_IF_D0F4:
    case EVENT_OP_GOTO_UNLESS_D0F4:
        return EventOpD0F4(memory, cpu, handler);
    case EVENT_OP_KEEP_SAME_TILES:
    case EVENT_OP_GOTO_IF_SAME_TILES:
    case EVENT_OP_GOTO_UNLESS_SAME_TILES:
        return EventOpSameTiles(memory, cpu, handler, handoff);
    case EVENT_OP_KEEP_BLOCK_MATCH:
    case EVENT_OP_GOTO_IF_BLOCK_MATCH:
    case EVENT_OP_GOTO_UNLESS_BLOCK_MATCH:
        return EventOpBlockMatches(memory, cpu, handler, handoff);
    case EVENT_OP_GOTO_UNLESS_ACTORS_BIT_5:
        return EventOpGotoUnlessActorsBit5(memory, cpu, handoff);
    default:
        return EVENT_OPCODE_HANDOFF;
    }
}
