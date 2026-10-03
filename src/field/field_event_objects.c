/* Field event opcodes for map objects. */

#include <stdbool.h>

#include "core/cpu_internal.h"
#include "lufia2/actor.h"
#include "lufia2/field.h"
#include "lufia2/system.h"
#include "actor/actor_internal.h"
#include "field/event_script_internal.h"
#include "field/field_internal.h"
#include "system/wram.h"

/* $80:E9B0: bit operand; $C0-$FA as 0-$3A, $FB-$FF variables. */
static void EventBitOperand(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    Compare8(cpu, A8(cpu), 0xfbu);                             /* E9B0 */
    if (cpu->carry) {
        Lufia2EventVariable(memory, cpu, return_address);
        return;
    }
    SimulateJsrFrame(memory, cpu, return_address);
    Compare8(cpu, A8(cpu), 0xc0u);
    if (cpu->carry) {
        cpu->carry = 1;
        Sbc8(cpu, 0xc0u);
    }
    SimulateRtsFrame(memory, cpu);
}

/* $83:8AF7: object bit A: byte index in X, mask in $54. */
static void EventObjectBitIndex(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    StoreADirect8(memory, cpu, 0x55u);                         /* 8AF7 */
    TransferDirectToA(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x55u));
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    TransferAToX(cpu);
    PushIndex(memory, cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x55u));
    And8(cpu, 0x07u);
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x80be45u, cpu->x)));
    StoreADirect8(memory, cpu, 0x54u);
    cpu->x = PullIndexValue(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

enum EventObjectBitAccess {
    EVENT_OBJECT_BIT_TEST,                                     /* $83:8AC9 */
    EVENT_OBJECT_BIT_SET,                                      /* $83:8AD5 */
    EVENT_OBJECT_BIT_CLEAR                                     /* $83:8AE5 */
};

/* Object state bits at $7F:D095; the test leaves Z. */
static void EventObjectBit(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    enum EventObjectBitAccess access,
    uint16_t return_address) {
    static const uint16_t kReturns[3] = {0x8accu, 0x8ad8u, 0x8ae8u};
    uint32_t bits;

    SimulateJslFrame(memory, cpu, 0x80u, return_address);
    PushIndex(memory, cpu);
    EventObjectBitIndex(memory, cpu, kReturns[access]);
    bits = LongIndexedAddress(WRAM_FIELD_OBJECT_STATE_BITS, cpu->x);
    switch (access) {
    case EVENT_OBJECT_BIT_TEST:
        LoadA8(cpu, Read8(memory, bits));
        cpu->x = PullIndexValue(memory, cpu);
        And8(cpu, DirectByte(memory, cpu, 0x54u));
        break;
    case EVENT_OBJECT_BIT_SET:
        LoadA8(cpu, (uint8_t)(Read8(memory, bits) |
            DirectByte(memory, cpu, 0x54u)));
        Write8(memory, bits, A8(cpu));
        cpu->x = PullIndexValue(memory, cpu);
        break;
    default:
        LoadA8(cpu, (uint8_t)(DirectByte(memory, cpu, 0x54u) ^ 0xffu));
        And8(cpu, Read8(memory, bits));
        Write8(memory, bits, A8(cpu));
        cpu->x = PullIndexValue(memory, cpu);
        break;
    }
    SimulateRtlFrame(memory, cpu);
}

/* $83:F559: queue an object animation. */
static unsigned EventQueueObjectAnimation(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address,
    uint32_t *handoff) {
    SimulateJslFrame(memory, cpu, 0x80u, return_address);
    StoreADirect8(memory, cpu, 0x56u);                         /* F559 */
    ExchangeAccumulatorBytes(cpu);
    StoreADirect8(memory, cpu, 0x57u);
    LoadA8(cpu, Read8(memory, WRAM_BRIGHTNESS));
    if (cpu->negative) {
        cpu->program_bank = 0x83u;
        *handoff = 0x83f564u;
        return EVENT_OPCODE_HANDOFF;
    }
    for (LoadX16(cpu, 0x0000u);;) {                            /* F581 */
        LoadA8(cpu,
               Read8(memory, LongIndexedAddress(EVENT_ANIMATION_SLOT_STATE, cpu->x)));
        if (cpu->negative) {
            LoadA8(cpu, Read8(memory,
                              LongIndexedAddress(EVENT_ANIMATION_SLOT_OBJECT, cpu->x)));
            Compare8(cpu, A8(cpu), DirectByte(memory, cpu, 0x57u));
            if (cpu->zero) {
                SimulateRtlFrame(memory, cpu);
                return EVENT_OPCODE_NEXT;
            }
        }
        IncrementX16(cpu);                                     /* F592 */
        Compare16(cpu, cpu->x, 0x0008u);
        if (cpu->carry)
            break;
    }
    for (LoadX16(cpu, 0x0000u);;) {                            /* F598 */
        LoadA8(cpu,
               Read8(memory, LongIndexedAddress(EVENT_ANIMATION_SLOT_STATE, cpu->x)));
        if (!cpu->negative)
            break;
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x0008u);
        if (cpu->zero) {
            LoadX16(cpu, 0x0000u);
            break;
        }
    }
    LoadA8(cpu, (uint8_t)(DirectByte(memory, cpu, 0x56u) | 0x90u));  /* F5AA */
    Write8(memory, LongIndexedAddress(EVENT_ANIMATION_SLOT_STATE, cpu->x), A8(cpu));
    LoadA8(cpu, DirectByte(memory, cpu, 0x57u));
    Write8(memory, LongIndexedAddress(EVENT_ANIMATION_SLOT_OBJECT, cpu->x), A8(cpu));
    SimulateRtlFrame(memory, cpu);
    return EVENT_OPCODE_NEXT;
}

/* $80:CCDB: carry set when object $7F:D04E's rows cover $06E2. */
static unsigned EventObjectCoversRow(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t *handoff) {
    SimulateJsrFrame(memory, cpu, 0xccc8u);
    LoadA8(cpu, 0x0fu);                                        /* CCDB */
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Read8(memory, EVENT_OBJECT_OPERAND));
    LoadX16(cpu, 0x0002u);
    if (!Lufia2FieldListSearch(memory, cpu, 0x80u, 0xcce8u)) {
        *handoff = EVENT_SEARCH_HANDOFF;
        return EVENT_OPCODE_HANDOFF;
    }
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7ef001u, cpu->x)));
    Compare8(cpu, A8(cpu),
        Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_TILE_X, 0)));
    cpu->carry = 0;
    if (cpu->zero) {
        LoadA8(cpu, (uint8_t)(Read8(memory,
            LongIndexedAddress(0x7ef002u, cpu->x)) - 1u));     /* CCF3 */
        Compare8(cpu, A8(cpu),
            Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_TILE_Y, 0)));
        if (!cpu->carry) {
            StoreADirect8(memory, cpu, 0x56u);
            LoadA8(cpu, 0x0au);
            ExchangeAccumulatorBytes(cpu);
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7ef00du, cpu->x)));
            LoadX16(cpu, 0x0004u);
            if (!Lufia2FieldListSearch(memory, cpu, 0x80u, 0xcd0cu)) {
                *handoff = EVENT_SEARCH_HANDOFF;
                return EVENT_OPCODE_HANDOFF;
            }
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7ef005u, cpu->x)));
            cpu->carry = 0;
            Adc8(cpu, DirectByte(memory, cpu, 0x56u));
            Compare8(cpu, A8(cpu),
                Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_ACTOR_TILE_Y, 0)));
            if (cpu->carry) {
                cpu->carry = 1;                                /* CD19 */
                SimulateRtsFrame(memory, cpu);
                return EVENT_OPCODE_NEXT;
            }
        }
    }
    cpu->carry = 0;                                            /* CD1B */
    SimulateRtsFrame(memory, cpu);
    return EVENT_OPCODE_NEXT;
}

/* $02/$03/$28/$A0/$A1: set or clear object bit n. */
unsigned Lufia2EventOpObjectBit(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler,
    uint32_t *handoff) {
    static const uint16_t kReturns[4][3] = {
        {0xcc7au, 0xcc7du, 0xcc85u},                           /* $02 */
        {0xcca9u, 0xccacu, 0xccb4u},                           /* $03 */
        {0xcf9fu, 0xcfa2u, 0xcfa6u},                           /* $A0 */
        {0xcfacu, 0xcfafu, 0xcfb3u}};                          /* $A1 */
    unsigned kind;

    if (handler == EVENT_OP_OBJECT_BIT_BY_RESULT) {
        LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);             /* E4A1 */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(EVENT_SLOT_BITS, cpu->x)));
        handler = cpu->negative ? EVENT_OP_OBJECT_BIT_SET
                                : EVENT_OP_OBJECT_BIT_CLEAR;
    }
    kind = handler == EVENT_OP_OBJECT_BIT_SET ? 0u
         : handler == EVENT_OP_OBJECT_BIT_CLEAR ? 1u
         : handler == EVENT_OP_OBJECT_BIT_ON ? 2u : 3u;
    Lufia2EventNextByte(memory, cpu, kReturns[kind][0]);
    EventBitOperand(memory, cpu, kReturns[kind][1]);
    if (kind >= 2u) {
        EventObjectBit(memory, cpu,
            kind == 2u ? EVENT_OBJECT_BIT_SET : EVENT_OBJECT_BIT_CLEAR,
            kReturns[kind][2]);
        return EVENT_OPCODE_NEXT;
    }
    Write8(memory, EVENT_OBJECT_OPERAND, A8(cpu));
    EventObjectBit(memory, cpu, EVENT_OBJECT_BIT_TEST, kReturns[kind][2]);
    if (cpu->zero != (kind == 0u))
        return EVENT_OPCODE_NEXT;
    LoadAAbsolute8(memory, cpu, WRAM_BRIGHTNESS, 0);
    if (cpu->negative) {
        LoadA8(cpu, Read8(memory, EVENT_OBJECT_OPERAND));
        EventObjectBit(memory, cpu,
            kind == 0u ? EVENT_OBJECT_BIT_SET : EVENT_OBJECT_BIT_CLEAR,
            kind == 0u ? 0xcc94u : 0xccc3u);
        return EVENT_OPCODE_NEXT;
    }
    if (kind == 1u) {
        if (EventObjectCoversRow(memory, cpu, handoff) != EVENT_OPCODE_NEXT)
            return EVENT_OPCODE_HANDOFF;
        if (cpu->carry)
            return EVENT_OPCODE_NEXT;
    }
    LoadA8(cpu, Read8(memory, EVENT_OBJECT_OPERAND));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, kind == 0u ? 0x00u : 0x40u);
    PushY(memory, cpu);
    if (EventQueueObjectAnimation(memory, cpu,
            kind == 0u ? 0xcca2u : 0xccd6u, handoff) != EVENT_OPCODE_NEXT)
        return EVENT_OPCODE_HANDOFF;
    cpu->y = PullIndexValue(memory, cpu);
    return EVENT_OPCODE_NEXT;
}

/* $83:F422: $7F:D046 = ($8F, $91 - $D04D + 1); A = the y. */
void Lufia2EventObjectOriginFrom(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t return_bank,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, return_bank, return_address);
    LoadA8(cpu, DirectByte(memory, cpu, DP_PROBE_X));          /* F422 */
    Write8(memory, WRAM_FIELD_PENDING_OBJECT_X, A8(cpu));
    LoadA8(cpu, DirectByte(memory, cpu, DP_PROBE_Y));
    cpu->carry = 1;
    Sbc8(cpu, Read8(memory, WRAM_FIELD_OBJECT_HEIGHT));
    LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
    Write8(memory, WRAM_FIELD_PENDING_OBJECT_Y, A8(cpu));
    SimulateRtlFrame(memory, cpu);
}

static void EventObjectOrigin(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    Lufia2EventObjectOriginFrom(memory, cpu, 0x80u, return_address);
}

/* $83:F442: clear a placed object's attribute bits. */
static void EventObjectClearAttributes(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    unsigned bit;

    SimulateJslFrame(memory, cpu, 0x80u, return_address);
    LoadA8(cpu, Read8(memory, WRAM_FIELD_PENDING_OBJECT_X)); /* F442 */
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Read8(memory, WRAM_FIELD_PENDING_OBJECT_Y));
    Lufia2MapCellIndex(memory, cpu, 0xf44du, 0);               /* $83:F9B6 */
    for (bit = 0; bit < 2u; ++bit) {
        const uint8_t keep = bit ? 0xf7u : 0xbfu;

        if (!bit) {
            LoadA8(cpu, Read8(memory, WRAM_FIELD_OBJECT_HEIGHT));
            Compare8(cpu, A8(cpu), 0x02u);
            if (!cpu->zero)
                continue;
        }
        LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_FIELD_MAP_ATTRIBUTES,
            cpu->x)));
        And8(cpu, keep);
        Write8(memory, LongIndexedAddress(WRAM_FIELD_MAP_ATTRIBUTES, cpu->x), A8(cpu));
        LoadA8(cpu, Read8(memory, WRAM_FIELD_OBJECT_WIDTH));
        Compare8(cpu, A8(cpu), 0x02u);
        if (cpu->zero) {
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7e4001u, cpu->x)));
            And8(cpu, keep);
            Write8(memory, LongIndexedAddress(0x7e4001u, cpu->x), A8(cpu));
        }
        if (!bit) {
            SetAccumulatorWidth(cpu, 0);                       /* F472 */
            LoadA16(cpu, cpu->x);
            cpu->carry = 0;
            Add16Value(cpu, Read16Long(memory, WRAM_FIELD_SECTION_WIDTH));
            TransferAToX(cpu);
            SetAccumulatorWidth(cpu, 1);
        }
    }
    SimulateRtlFrame(memory, cpu);
}

/* $83:8A6F: clear an object's tile block in layer A. */
static void EventObjectClearTiles(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x80u, return_address);
    StoreADirect8(memory, cpu, 0x54u);                         /* 8A6F */
    Write8(memory, DirectAddress(cpu, 0x55u), 0x00u);
    LoadA8(cpu, Read8(memory, WRAM_FIELD_PENDING_OBJECT_X));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Read8(memory, WRAM_FIELD_PENDING_OBJECT_Y));
    SimulateJsrFrame(memory, cpu, 0x8a7eu);
    Lufia2MapCellOffset(memory, cpu);                          /* $83:F9F7 */
    SimulateRtsFrame(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, cpu->x);
    LoadXDirect(memory, cpu, 0x54u);
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, LongIndexedAddress(WRAM_FIELD_LAYER_CELL_BASE,
        cpu->x)));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    TransferDirectToA(cpu);                                    /* 8A8C */
    LoadAAbsolute8(memory, cpu, WRAM_FIELD_SECTION_WIDTH, 0);
    cpu->carry = 1;
    Sbc8(cpu, Read8(memory, WRAM_FIELD_OBJECT_WIDTH));
    SetAccumulatorWidth(cpu, 0);
    AslA16(cpu);
    StoreADirect16(memory, cpu, 0x54u);
    LoadA16(cpu, Read16Long(memory, WRAM_FIELD_OBJECT_HEIGHT));
    And16(cpu, 0x00ffu);
    StoreADirect16(memory, cpu, 0x58u);
    do {
        LoadA16(cpu, Read16Long(memory, WRAM_FIELD_OBJECT_WIDTH));           /* 8AA3 */
        And16(cpu, 0x00ffu);
        StoreADirect16(memory, cpu, 0x56u);
        do {
            const uint32_t tile = LongIndexedAddress(0x7f0000u, cpu->x);

            LoadA16(cpu, Read16Long(memory, tile));
            And16(cpu, 0xfc00u);
            Write16Long(memory, tile, cpu->accumulator);
            IncrementX16(cpu);
            IncrementX16(cpu);
            Decrement16Direct(memory, cpu, 0x56u);
        } while (!cpu->zero);
        LoadA16(cpu, cpu->x);                                  /* 8ABD */
        cpu->carry = 0;
        Add16Value(cpu, Read16Direct(memory, cpu, 0x54u));
        TransferAToX(cpu);
        Decrement16Direct(memory, cpu, 0x58u);
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);
    SimulateRtlFrame(memory, cpu);
}

/* Whether $83:8A6F overwrites its own block size. */
static uint8_t EventClearHitsSize(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu,
    unsigned width,
    unsigned height) {
    const uint8_t map_width = Read8(memory, WRAM_FIELD_SECTION_WIDTH);
    const uint16_t cell = (uint16_t)(2u * ((uint16_t)(
        Read8(memory, WRAM_FIELD_PENDING_OBJECT_Y) * map_width) + Read8(memory,
            WRAM_FIELD_PENDING_OBJECT_X)));
    const uint16_t skip = (uint16_t)(((cpu->direct_page & 0xff00u) |
        (uint8_t)(Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_FIELD_SECTION_WIDTH,
            0)) -
                  width)) << 1);
    uint16_t row = (uint16_t)(cell + Read16Long(memory, 0x7fd00au));
    unsigned r;

    for (r = 0; r < height; ++r) {
        const uint16_t size = (uint16_t)(0xd04cu - row);

        if (size < 2u * width || (uint16_t)(size + 1u) < 2u * width)
            return 1;
        row = (uint16_t)(row + 2u * width + skip);
    }
    return 0;
}

/* $83:F784: tile bits of cell X (DB) = A. */
static void EventTileWord(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    const uint32_t cell = AbsoluteIndexedAddress(cpu, 0x0000u, cpu->x);

    SimulateJsrFrame(memory, cpu, return_address);
    And16(cpu, 0x03ffu);                                       /* F784 */
    StoreADirect16(memory, cpu, 0x54u);
    LoadA16(cpu, Read16Long(memory, cell));
    And16(cpu, 0xfc00u);
    LoadA16(cpu, (uint16_t)(cpu->accumulator | Read16Direct(memory, cpu, 0x54u)));
    Write16Long(memory, cell, cpu->accumulator);
    SimulateRtsFrame(memory, cpu);
}

/* $83:F750: tiles of pending object A at $8F/$91. */
static void EventObjectSetTiles(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x80u, return_address);
    AslA8(cpu);                                                /* F750 */
    AslA8(cpu);
    TransferAToY(cpu);
    SimulateJsrFrame(memory, cpu, 0xf755u);
    LoadA8(cpu, DirectByte(memory, cpu, DP_PROBE_X));          /* F9D4 */
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, DP_PROBE_Y));
    SimulateJsrFrame(memory, cpu, 0xf9dbu);
    Lufia2MapCellOffset(memory, cpu);                          /* $83:F9F7 */
    SimulateRtsFrame(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    PushIndex(memory, cpu);
    LoadA16(cpu, Read16Long(memory, WRAM_FIELD_LAYER_TABLE_OFFSET));
    TransferAToX(cpu);
    PullAccumulator16(memory, cpu);
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, LongIndexedAddress(WRAM_FIELD_LAYER_CELL_BASE,
        cpu->x)));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    SimulateRtsFrame(memory, cpu);
    PushDataBank(memory, cpu);                                 /* F756 */
    LoadA8(cpu, 0x7fu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(
                     memory, cpu,
                     ((uint16_t)(WRAM_FIELD_PENDING_OBJECT_TILES & 0xffffu)), cpu->y));
    EventTileWord(memory, cpu, 0xf762u);
    LoadA16(cpu, Read16Long(memory, WRAM_FIELD_OBJECT_HEIGHT));
    And16(cpu, 0x00ffu);
    Compare16(cpu, cpu->accumulator, 0x0002u);
    if (cpu->zero) {
        LoadA16(cpu, cpu->x);                                  /* F76F */
        cpu->carry = 0;
        Add16Value(cpu, Read16Long(memory, WRAM_FIELD_SECTION_WIDTH));
        Add16Value(cpu, Read16Long(memory, WRAM_FIELD_SECTION_WIDTH));
        TransferAToX(cpu);
        LoadA16(cpu, Read16AbsoluteIndexed(
                         memory, cpu,
                         ((uint16_t)((WRAM_FIELD_PENDING_OBJECT_TILES + 2u) & 0xffffu)),
                         cpu->y));
        EventTileWord(memory, cpu, 0xf77fu);
    }
    PullDataBank(memory, cpu);                                 /* F780 */
    SetAccumulatorWidth(cpu, 1);
    SimulateRtlFrame(memory, cpu);
}

/* $80:D3C6: redraw the object's region. */
static uint8_t EventObjectRedraw(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    EventRun *run,
    uint16_t return_address,
    uint32_t *handoff) {
    uint32_t redraw;
    uint8_t value;

    SimulateJsrFrame(memory, cpu, return_address);
    TransferDirectToA(cpu);                                    /* D3C6 */
    LoadA8(cpu, Read8(memory, WRAM_FIELD_LAYER_TABLE_OFFSET));
    TransferAToX(cpu);
    if (Lufia2FieldRegionCells(memory, cpu) > EVENT_OPCODE_LIMIT) {
        run->visits = run->region_redraws[run->depth];
        run->has_visits = 1;
        *handoff = 0x80d3ccu;                                  /* JSL $83:8E85 */
        return 0;
    }
    ++run->region_redraws[run->depth];
    Lufia2FieldRedrawRegion(memory, cpu, 0x80u, 0xd3cfu);      /* $83:8E85 */
    LoadA8(cpu, 0x02u);
    redraw = AbsoluteIndexedAddress(cpu, WRAM_EVENT_REDRAW, 0); /* TSB */
    value = Read8(memory, redraw);
    cpu->zero = (value & A8(cpu)) == 0;
    Write8(memory, redraw, (uint8_t)(value | A8(cpu)));
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* $80:D357: toggle the map object at $8F/$91. */
static uint8_t EventObjectTiles(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    EventRun *run,
    uint16_t return_address,
    uint32_t *handoff) {
    SimulateJsrFrame(memory, cpu, return_address);
    for (LoadX16(cpu, 0x0000u);;) {                            /* D357 */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_FIELD_PENDING_RECORD_X,
            cpu->x)));
        Compare8(cpu, A8(cpu), DirectByte(memory, cpu, DP_PROBE_X));
        if (cpu->zero) {
            LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_FIELD_PENDING_RECORD_Y,
                cpu->x)));
            Compare8(cpu, A8(cpu), DirectByte(memory, cpu, DP_PROBE_Y));
            if (cpu->zero)
                break;
        }
        IncrementX16(cpu);                                     /* D36A */
        Compare16(cpu, cpu->x, 0x0030u);
        if (cpu->zero) {
            unsigned width, height;

            Lufia2EventCellValue(memory, cpu, 0x80u, 0xd373u);       /* D370 */
            BitImmediate8(cpu, 0xf0u);
            if (cpu->zero) {
                SimulateRtsFrame(memory, cpu);
                return 1;
            }
            cpu->carry = 1;
            Sbc8(cpu, 0x10u);
            if (!Lufia2EventMapObject(memory, cpu, 0xd37eu, handoff))
                return 0;
            EventObjectOrigin(memory, cpu, 0xd382u);
            StoreADirect8(memory, cpu, DP_PROBE_Y);
            EventObjectClearAttributes(memory, cpu, 0xd388u);
            LoadA8(cpu, 0x02u);                                /* D389 */
            width = Read8(memory, WRAM_FIELD_OBJECT_WIDTH);
            height = Read8(memory, WRAM_FIELD_OBJECT_HEIGHT);
            if (!width || !height || width * height > EVENT_OPCODE_LIMIT ||
                EventClearHitsSize(memory, cpu, width, height)) {
                run->visits = run->tile_clears[run->depth];
                run->has_visits = 1;
                *handoff = 0x80d38bu;                          /* JSL $83:8A6F */
                return 0;
            }
            ++run->tile_clears[run->depth];
            EventObjectClearTiles(memory, cpu, 0xd38eu);
            if (!EventObjectRedraw(memory, cpu, run, 0xd391u, handoff))
                return 0;
            SimulateRtsFrame(memory, cpu);
            return 1;
        }
    }
    PushY(memory, cpu);                                        /* D393 */
    StoreXDirect16(memory, cpu, 0x56u);
    LoadA8(cpu, 0xffu);
    Write8(memory, LongIndexedAddress(WRAM_FIELD_PENDING_RECORD_X, cpu->x), A8(cpu));
    Write8(memory, LongIndexedAddress(WRAM_FIELD_PENDING_RECORD_Y, cpu->x), A8(cpu));
    TransferDirectToA(cpu);
    Write8(memory, LongIndexedAddress(EVENT_UNK_7FD75C, cpu->x), A8(cpu));
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_FIELD_PENDING_OBJECT_RECORD,
        cpu->x)));
    cpu->carry = 1;
    Sbc8(cpu, 0x10u);
    if (!Lufia2EventMapObject(memory, cpu, 0xd3afu, handoff))
        return 0;
    EventObjectOrigin(memory, cpu, 0xd3b3u);
    StoreADirect8(memory, cpu, DP_PROBE_Y);
    EventObjectClearAttributes(memory, cpu, 0xd3b9u);
    TransferDirectToA(cpu);                                    /* D3BA */
    LoadA8(cpu, DirectByte(memory, cpu, 0x56u));
    EventObjectSetTiles(memory, cpu, 0xd3c0u);
    if (!EventObjectRedraw(memory, cpu, run, 0xd3c3u, handoff))
        return 0;
    cpu->y = PullIndexValue(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* $21/$22: toggle the map object at a position. */
unsigned Lufia2EventOpObjectTilesAt(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler,
    EventRun *run,
    uint32_t *handoff) {
    Lufia2EventNextByte(memory, cpu, (uint16_t)(handler + 2u));
    if (handler == EVENT_OP_OBJECT_TILES_AT) {
        StoreADirect8(memory, cpu, DP_PROBE_X);                /* D339 */
        Lufia2EventNextByte(memory, cpu, 0xd33du);
        StoreADirect8(memory, cpu, DP_PROBE_Y);
    } else if (!Lufia2EventProbePosition(memory, cpu, 0xd34bu, handoff)) {
        return EVENT_OPCODE_HANDOFF;
    }
    if (!EventObjectTiles(memory, cpu, run,
            handler == EVENT_OP_OBJECT_TILES_AT ? 0xd342u : 0xd353u, handoff))
        return EVENT_OPCODE_HANDOFF;
    return EVENT_OPCODE_NEXT;
}

/* $83:F9D4/$83:F9D9: layer cell of a position. */
static void EventLayerCell(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address,
    uint8_t from_probe) {
    SimulateJsrFrame(memory, cpu, return_address);
    if (from_probe) {
        LoadA8(cpu, DirectByte(memory, cpu, DP_PROBE_X));      /* F9D4 */
        ExchangeAccumulatorBytes(cpu);
        LoadA8(cpu, DirectByte(memory, cpu, DP_PROBE_Y));
    }
    (void)Lufia2LayerCellOffset(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $83:FB9F: pending object at $8F/$91; carry when none. */
static void EventFindPending(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    for (LoadX16(cpu, 0x0000u);;) {                            /* FB9F */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_FIELD_PENDING_RECORD_X,
            cpu->x)));
        Compare8(cpu, A8(cpu), DirectByte(memory, cpu, DP_PROBE_X));
        if (cpu->zero) {
            LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_FIELD_PENDING_RECORD_Y,
                cpu->x)));
            Compare8(cpu, A8(cpu), DirectByte(memory, cpu, DP_PROBE_Y));
            if (cpu->zero) {
                cpu->carry = 0;                                /* FBBB */
                break;
            }
        }
        IncrementX16(cpu);                                     /* FBB2 */
        Compare16(cpu, cpu->x, 0x0030u);
        if (cpu->zero) {
            TransferDirectToA(cpu);
            cpu->carry = 1;
            break;
        }
    }
    SimulateRtsFrame(memory, cpu);
}

/* $83:F80D: mark attributes under a pending object. */
static void EventMarkAttributes(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    uint32_t cell;

    SimulateJsrFrame(memory, cpu, return_address);
    Lufia2MapCellIndex(memory, cpu, 0xf80fu, 1);               /* $83:F9AD */
    cell = LongIndexedAddress(WRAM_FIELD_MAP_ATTRIBUTES, cpu->x); /* F810 */
    LoadA8(cpu, Read8(memory, cell));
    StoreADirect8(memory, cpu, 0x5au);
    And8(cpu, DirectByte(memory, cpu, 0x54u));
    LoadA8(cpu, (uint8_t)(A8(cpu) | DirectByte(memory, cpu, 0x55u)));
    Write8(memory, cell, A8(cpu));
    LoadA8(cpu, DirectByte(memory, cpu, DP_PROBE_Y));
    cpu->carry = 1;
    Sbc8(cpu, Read8(memory, WRAM_FIELD_OBJECT_HEIGHT));
    LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
    StoreADirect8(memory, cpu, DP_PROBE_Y);
    Write8(memory, DirectAddress(cpu, 0x58u), 0x00u);
    Write8(memory, DirectAddress(cpu, 0x59u), 0x00u);
    LoadA8(cpu, Read8(memory, WRAM_FIELD_OBJECT_HEIGHT));
    Compare8(cpu, A8(cpu), 0x02u);
    cpu->carry = 0;
    if (cpu->zero) {
        LoadA8(cpu, 0xf0u);                                    /* F835 */
        StoreADirect8(memory, cpu, 0x58u);
        LoadA8(cpu, 0xffu);
        StoreADirect8(memory, cpu, 0x59u);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, cpu->x);
        StoreADirect16(memory, cpu, 0x54u);
        Subtract16(cpu, Read16Long(memory, WRAM_FIELD_SECTION_WIDTH));
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
        cell = LongIndexedAddress(WRAM_FIELD_MAP_ATTRIBUTES, cpu->x);
        LoadA8(cpu, Read8(memory, cell));
        And8(cpu, DirectByte(memory, cpu, 0x56u));
        LoadA8(cpu, (uint8_t)(A8(cpu) | DirectByte(memory, cpu, 0x57u)));
        Write8(memory, cell, A8(cpu));
        cpu->carry = 1;
    }
    LoadA8(cpu, DirectByte(memory, cpu, 0x5au));               /* F857 */
    SimulateRtsFrame(memory, cpu);
}

/* $83:F85A: pending tile offsets; leaves M=0. */
static void EventPendingOffsets(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    SetAccumulatorWidth(cpu, 0);                               /* F85A */
    LoadA16(cpu, cpu->x);
    AslA16(cpu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, cpu->y);
    AslA16(cpu);
    AslA16(cpu);
    TransferAToY(cpu);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x58u));
    if (!cpu->zero) {
        IncrementY16(cpu);
        IncrementY16(cpu);
    }
    SimulateRtsFrame(memory, cpu);
}

/* $83:F91F: tile bits of cell Y (DB) = those of cell X. */
static void EventCopyTile(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    (void)Lufia2FieldCopyCellTile(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $83:F8D4-$83:F91B: move the tile bits of the object's source cell onto its
 * destination cell (and the row below when the object is two cells tall). */
static void EventPlaceCopyTiles(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    EventLayerCell(memory, cpu, 0xf8d6u, 1); /* F8D4 */
    LoadY16(cpu, cpu->x);
    LoadA8(cpu, Read8(memory, WRAM_FIELD_OBJECT_SOURCE_X));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Read8(memory, WRAM_FIELD_OBJECT_SOURCE_Y));
    EventLayerCell(memory, cpu, 0xf8e3u, 0);
    StoreXDirect16(memory, cpu, 0x63u);
    TransferDirectToA(cpu); /* F8E6 */
    LoadA8(cpu, DirectByte(memory, cpu, 0x65u));
    AslA8(cpu);
    AslA8(cpu);
    SetAccumulatorWidth(cpu, 0);
    StoreADirect16(memory, cpu, 0x56u);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, WRAM_FIELD_SECTION_WIDTH));
    AslA16(cpu);
    StoreADirect16(memory, cpu, 0x54u);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->y));
    Write16Long(
        memory,
        AbsoluteIndexedAddress(
            cpu, ((uint16_t)(WRAM_FIELD_PENDING_OBJECT_TILES & 0xffffu)), cpu->x),
        cpu->accumulator);
    LoadXDirect(memory, cpu, 0x63u);
    EventCopyTile(memory, cpu, 0xf901u);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x58u));
    if (!cpu->zero) {
        LoadA16(cpu, cpu->y); /* F906 */
        cpu->carry = 0;
        Add16Value(cpu, Read16Direct(memory, cpu, 0x54u));
        TransferAToY(cpu);
        LoadXDirect(memory, cpu, 0x56u);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->y));
        Write16Long(memory,
                    AbsoluteIndexedAddress(
                        cpu,
                        ((uint16_t)((WRAM_FIELD_PENDING_OBJECT_TILES + 2u) & 0xffffu)),
                        cpu->x),
                    cpu->accumulator);
        LoadA16(cpu, Read16Direct(memory, cpu, 0x63u));
        cpu->carry = 0;
        Add16Value(cpu, Read16Direct(memory, cpu, 0x54u));
        TransferAToX(cpu);
        EventCopyTile(memory, cpu, 0xf91bu);
    }
}

/* $83:F86B: register a pending object and swap tiles. */
static void EventPlacePending(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    bool copy_tiles = true;

    SimulateJslFrame(memory, cpu, 0x80u, return_address);
    StoreADirect8(memory, cpu, 0x65u);                         /* F86B */
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    LoadA8(cpu, 0x7fu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    TransferDirectToA(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x65u));
    TransferAToX(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, DP_PROBE_X));
    StoreAAbsolute8(memory, cpu, ((uint16_t)(WRAM_FIELD_PENDING_RECORD_X & 0xffffu)),
                    cpu->x);
    LoadA8(cpu, DirectByte(memory, cpu, DP_PROBE_Y));
    StoreAAbsolute8(memory, cpu, ((uint16_t)(WRAM_FIELD_PENDING_RECORD_Y & 0xffffu)),
                    cpu->x);
    LoadA8(cpu, 0xffu);
    StoreADirect8(memory, cpu, 0x54u);
    StoreADirect8(memory, cpu, 0x56u);
    LoadA8(cpu, 0x08u);
    StoreADirect8(memory, cpu, 0x55u);
    LoadA8(cpu, 0x40u);
    StoreADirect8(memory, cpu, 0x57u);
    EventMarkAttributes(memory, cpu, 0xf891u);
    BitImmediate8(cpu, 0x40u);
    if (!cpu->zero) {
        uint8_t row;

        LoadA8(cpu, DirectByte(memory, cpu, 0x58u));           /* F896 */
        if (!cpu->zero) {
            row = (uint8_t)(DirectByte(memory, cpu, DP_PROBE_Y) + 1u);
            Write8(memory, DirectAddress(cpu, DP_PROBE_Y), row);
            SetNz8(cpu, row);
        }
        EventFindPending(memory, cpu, 0xf89eu);
        LoadY16(cpu, cpu->x);
        row = (uint8_t)(DirectByte(memory, cpu, DP_PROBE_Y) + 1u);
        Write8(memory, DirectAddress(cpu, DP_PROBE_Y), row);
        SetNz8(cpu, row);
        EventFindPending(memory, cpu, 0xf8a4u);
        EventPendingOffsets(memory, cpu, 0xf8a7u);
        LoadA16(cpu,
                Read16AbsoluteIndexed(
                    memory, cpu,
                    ((uint16_t)(WRAM_FIELD_PENDING_OBJECT_TILES & 0xffffu)), cpu->x));
        Write16Long(
            memory,
            AbsoluteIndexedAddress(
                cpu, ((uint16_t)(WRAM_FIELD_PENDING_OBJECT_TILES & 0xffffu)), cpu->y),
            cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);                           /* F8AE */
        LoadY16(cpu, cpu->x);
        LoadAAbsolute8(memory, cpu, ((uint16_t)(WRAM_FIELD_OBJECT_SOURCE_X & 0xffffu)),
                       0);
        ExchangeAccumulatorBytes(cpu);
        LoadAAbsolute8(memory, cpu, ((uint16_t)(WRAM_FIELD_OBJECT_SOURCE_Y & 0xffffu)),
                       0);
        cpu->carry = 0;
        Adc8(cpu, Read8(memory,
                        AbsoluteIndexedAddress(
                            cpu, ((uint16_t)(WRAM_FIELD_OBJECT_HEIGHT & 0xffffu)), 0)));
        DecrementA8(cpu);
        EventLayerCell(memory, cpu, 0xf8bfu, 0);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x));
        Write16Long(
            memory,
            AbsoluteIndexedAddress(
                cpu, ((uint16_t)(WRAM_FIELD_PENDING_OBJECT_TILES & 0xffffu)), cpu->y),
            cpu->accumulator);
        LoadA16(cpu, Read16Direct(memory, cpu, 0x58u));
        if (!cpu->zero) {
            Write16Direct(memory, cpu, 0x58u, 0x0000u);
            SetAccumulatorWidth(cpu, 1);
            row = (uint8_t)(DirectByte(memory, cpu, DP_PROBE_Y) - 2u);
            Write8(memory, DirectAddress(cpu, DP_PROBE_Y), row);
            SetNz8(cpu, row);
        } else {
            copy_tiles = false;
        }
    }
    if (copy_tiles)
        EventPlaceCopyTiles(memory, cpu);
    PullDataBank(memory, cpu);                                 /* F91C */
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtlFrame(memory, cpu);
}

/* $83:8E76: redraw region $7F:D046 in layers 0 and 1. */
static uint8_t EventRedrawLowLayers(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    EventRun *run,
    uint16_t return_address,
    uint32_t *handoff) {
    SimulateJslFrame(memory, cpu, 0x80u, return_address);
    cpu->program_bank = 0x83u;
    for (LoadX16(cpu, 0x0000u);;) {                            /* 8E76 */
        if (Lufia2FieldRegionCells(memory, cpu) > 1024u) {
            run->visits = run->layer_redraws[run->depth];
            run->has_visits = 1;
            *handoff = 0x838e79u;                              /* JSL $83:8E85 */
            return 0;
        }
        ++run->layer_redraws[run->depth];
        Lufia2FieldRedrawRegion(memory, cpu, 0x83u, 0x8e7cu);
        IncrementX16(cpu);                                     /* 8E7D */
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x0004u);
        if (cpu->zero)
            break;
    }
    cpu->program_bank = 0x80u;
    SimulateRtlFrame(memory, cpu);
    return 1;
}

/* $2A: place map object n as pending. */
unsigned Lufia2EventOpPlaceObject(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    EventRun *run,
    uint32_t *handoff) {
    Lufia2EventNextByte(memory, cpu, 0xd3d8u);                 /* D3D6 */
    if (!Lufia2EventProbePosition(memory, cpu, 0xd3dbu, handoff))
        return EVENT_OPCODE_HANDOFF;
    SimulateJsrFrame(memory, cpu, 0xd3e3u);
    for (LoadX16(cpu, 0x0000u);;) {                            /* D426 */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(EVENT_UNK_7FD75C, cpu->x)));
        if (!cpu->negative)
            break;
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x0030u);
        if (cpu->zero) {
            LoadX16(cpu, 0x0000u);
            break;
        }
    }
    SimulateRtsFrame(memory, cpu);
    StoreXDirect16(memory, cpu, 0x56u);                        /* D3E4 */
    LoadA8(cpu, DirectByte(memory, cpu, DP_PROBE_X));
    Write8(memory, LongIndexedAddress(WRAM_FIELD_PENDING_RECORD_X, cpu->x), A8(cpu));
    LoadA8(cpu, DirectByte(memory, cpu, DP_PROBE_Y));
    Write8(memory, LongIndexedAddress(WRAM_FIELD_PENDING_RECORD_Y, cpu->x), A8(cpu));
    LoadA8(cpu, 0x80u);
    Write8(memory, LongIndexedAddress(EVENT_UNK_7FD75C, cpu->x), A8(cpu));
    Lufia2EventNextByte(memory, cpu, 0xd3fau);
    PushY(memory, cpu);
    cpu->carry = 0;
    Adc8(cpu, 0x10u);
    Write8(memory, LongIndexedAddress(WRAM_FIELD_PENDING_OBJECT_RECORD, cpu->x),
        A8(cpu));
    cpu->carry = 1;
    Sbc8(cpu, 0x10u);
    if (!Lufia2EventMapObject(memory, cpu, 0xd409u, handoff))
        return EVENT_OPCODE_HANDOFF;
    {
        const uint32_t redraw = AbsoluteIndexedAddress(cpu, WRAM_EVENT_REDRAW, 0);
        const uint8_t value = Read8(memory, redraw);

        LoadA8(cpu, Read8(memory, WRAM_FIELD_OBJECT_FLAGS));                 /* D40A */
        cpu->zero = (value & A8(cpu)) == 0;                    /* TSB */
        Write8(memory, redraw, (uint8_t)(value | A8(cpu)));
    }
    LoadXDirect(memory, cpu, 0x56u);
    EventObjectOrigin(memory, cpu, 0xd416u);
    TransferDirectToA(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x56u));
    EventPlacePending(memory, cpu, 0xd41du);
    if (!EventRedrawLowLayers(memory, cpu, run, 0xd421u, handoff))
        return EVENT_OPCODE_HANDOFF;
    cpu->y = PullIndexValue(memory, cpu);                      /* D422 */
    return EVENT_OPCODE_NEXT;
}

/* $83:F49A / $83:F4A7: save or restore $8F/$91 at $7F:D2A4. */
static void EventProbeSave(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address,
    uint8_t restore) {
    unsigned i;

    SimulateJsrFrame(memory, cpu, return_address);
    for (i = 0; i < 2u; ++i) {
        const uint8_t probe = i ? DP_PROBE_Y : DP_PROBE_X;
        const uint32_t saved = 0x7fd2a4u + i;

        if (restore) {
            LoadA8(cpu, Read8(memory, saved));                 /* F4A7 */
            StoreADirect8(memory, cpu, probe);
        } else {
            LoadA8(cpu, DirectByte(memory, cpu, probe));       /* F49A */
            Write8(memory, saved, A8(cpu));
        }
    }
    SimulateRtsFrame(memory, cpu);
}

/* $83:FB12: step $8F/$91 by direction A. */
static uint8_t EventProbeStep(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t return_bank,
    uint16_t return_address,
    uint32_t *handoff) {
    const uint8_t bank = cpu->program_bank;

    SimulateJslFrame(memory, cpu, return_bank, return_address);
    cpu->program_bank = 0x83u;
    if (!Lufia2ActorMovementStep(memory, cpu)) {
        *handoff = cpu->resume_pc;
        return 0;
    }
    SimulateRtlFrame(memory, cpu);
    cpu->program_bank = bank;
    return 1;
}

/* $83:C33D: carry when a secondary actor is at $8F/$91. */
static void EventSecondaryAtProbe(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Write8(memory, DirectAddress(cpu, 0x90u), 0x00u);          /* C33D */
    Write8(memory, DirectAddress(cpu, 0x92u), 0x00u);
    LoadY16(cpu, 0x001fu);
    LoadX16(cpu, 0x003eu);
    SetAccumulatorWidth(cpu, 0);
    cpu->carry = 0;
    do {
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_OBJECT_STATE, cpu->y));
        if (cpu->accumulator & 0x0080u) {                      /* BIT #$80 */
            LoadA16(cpu,
                    (uint16_t)(Read16Long(memory, LongIndexedAddress(WRAM_OBJECT_FINE_X,
                                                                     cpu->x)) >>
                               4));
            Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, DP_PROBE_X));
            if (cpu->zero) {
                LoadA16(cpu, (uint16_t)(Read16Long(memory,
                                                   LongIndexedAddress(
                                                       WRAM_OBJECT_FINE_Y, cpu->x)) >>
                                        4));
                Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, DP_PROBE_Y));
                if (cpu->zero) {
                    cpu->carry = 1;
                    break;
                }
            }
        }
        LoadY16(cpu, (uint16_t)(cpu->y - 1u));                 /* C36C */
        LoadX16(cpu, (uint16_t)(cpu->x - 2u));
        if (cpu->negative)
            cpu->carry = 0;
    } while (!cpu->negative);
    SetAccumulatorWidth(cpu, 1);
    SimulateRtsFrame(memory, cpu);
}

/* $83:C0B8-$83:C0DB: when the cell below the pushed object is open ground
 * facing the pending object, load the attribute of that object's tile;
 * false when the ground check is skipped. */
static bool EventPushPendingTile(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    EventProbeSave(memory, cpu, 0xc0bau, 1); /* C0B8 */
    LoadA8(cpu, DirectByte(memory, cpu, 0x94u));
    Compare8(cpu, A8(cpu), 0x04u);
    if (!cpu->zero)
        return false;
    SimulateJsrFrame(memory, cpu, 0xc0c3u);
    EventFindPending(memory, cpu, 0xf412u); /* F410 */
    if (!cpu->carry) {
        LoadY16(cpu, cpu->x);
        TransferDirectToA(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_FIELD_PENDING_OBJECT_RECORD,
                                                     cpu->x)));
        TransferAToX(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd7fcu, cpu->x)));
        cpu->carry = 0;
    }
    SimulateRtsFrame(memory, cpu);
    if (cpu->carry)
        return false;
    Compare8(cpu, A8(cpu), 0x01u);
    if (!cpu->zero)
        return false;
    SetAccumulatorWidth(cpu, 0); /* C0CA */
    LoadA16(cpu, cpu->y);
    AslA16(cpu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_FIELD_PENDING_OBJECT_TILES,
                                                       cpu->x)));
    SimulateJslFrame(memory, cpu, 0x83u, 0xc0d7u);
    And16(cpu, 0x03ffu); /* FB7A */
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, WRAM_FIELD_METATILE_ATTRIBUTE_BASE));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7f0000u, cpu->x)));
    SimulateRtlFrame(memory, cpu);
    SetNz8(cpu, A8(cpu)); /* C0D8 */
    return true;
}

/* $83:C09B-$83:C0EA: whether a pushed object may step onto a cell that is not
 * marked passable; 0 = handoff. */
static uint8_t EventPushGround(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                               uint32_t *handoff, bool *allowed) {
    bool tile_checked = true;

    EventProbeSave(memory, cpu, 0xc09bu, 1);
    TransferDirectToA(cpu);                                    /* C09C */
    LoadA8(cpu, DirectByte(memory, cpu, 0x94u));
    TransferAToX(cpu);
    Write8(memory, DirectAddress(cpu, 0x9au), 0x00u);
    SimulateJsrFrame(memory, cpu, 0xc0a4u);
    if (!Lufia2ActorStepBlockedBody(memory, cpu)) {            /* $83:D89E */
        *handoff = cpu->resume_pc;
        return 0;
    }
    SimulateRtsFrame(memory, cpu);
    if (!cpu->zero) {
        *allowed = false;
        return 1;
    }
    EventProbeSave(memory, cpu, 0xc0a9u, 1);
    LoadA8(cpu, DirectByte(memory, cpu, 0x94u));
    if (!EventProbeStep(memory, cpu, 0x83u, 0xc0afu, handoff))
        return 0;
    SimulateJslFrame(memory, cpu, 0x83u, 0xc0b3u);
    Lufia2ActorReadMapCellValue(memory, cpu);                  /* $83:FB71 */
    SimulateRtlFrame(memory, cpu);
    SetNz8(cpu, A8(cpu));                                      /* ORA #0 */
    if (cpu->zero)
        tile_checked = EventPushPendingTile(memory, cpu);
    if (tile_checked && !cpu->zero) {
        Compare8(cpu, A8(cpu), 0x09u);
        if (!cpu->zero) {
            Compare8(cpu, A8(cpu), 0x01u);
            if (!cpu->zero) {
                *allowed = false;
                return 1;
            }
        }
    }
    Lufia2MapTileHeight(memory, cpu, 0xc0e6u);                 /* C0E4 */
    Compare8(cpu, A8(cpu), DirectByte(memory, cpu, 0x56u));
    *allowed = cpu->zero;
    return 1;
}

/* $83:C079: carry when a pushed object may move. */
static uint8_t EventPushAllowed(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                uint16_t return_address, uint32_t *handoff) {
    bool allowed = true;

    SimulateJslFrame(memory, cpu, 0x80u, return_address);
    cpu->program_bank = 0x83u;
    StoreADirect8(memory, cpu, 0x94u); /* C079 */
    EventProbeSave(memory, cpu, 0xc07du, 0);
    Lufia2MapTileHeight(memory, cpu, 0xc080u); /* $83:F988 */
    StoreADirect8(memory, cpu, 0x56u);
    LoadA8(cpu, DirectByte(memory, cpu, 0x94u));
    if (!EventProbeStep(memory, cpu, 0x83u, 0xc088u, handoff))
        return 0;
    EventSecondaryAtProbe(memory, cpu, 0xc08bu);
    if (cpu->carry) {
        allowed = false;
    } else {
        Lufia2MapCellIndex(memory, cpu, 0xc090u, 1); /* $83:F9AD */
        LoadA8(cpu,
               Read8(memory, LongIndexedAddress(WRAM_FIELD_MAP_ATTRIBUTES, cpu->x)));
        And8(cpu, 0x80u);
        if (cpu->zero && !EventPushGround(memory, cpu, handoff, &allowed))
            return 0;
    }
    cpu->carry = allowed; /* C0EB, C0ED */
    SimulateRtlFrame(memory, cpu);
    cpu->program_bank = 0x80u;
    return 1;
}

/* $5A-$5D: push pending map object n one cell. */
unsigned Lufia2EventOpPushObject(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler,
    uint32_t *handoff) {
    static const uint8_t kDirections[4] = {0x04u, 0x00u, 0x02u, 0x06u};
    uint8_t allowed;

    LoadA8(cpu, kDirections[(handler - EVENT_OP_PUSH_OBJECT_DOWN) / 5u]);
    StoreADirect8(memory, cpu, 0x23u);                         /* DCC0 */
    Lufia2EventNextByte(memory, cpu, 0xdcc4u);
    Lufia2EventValue(memory, cpu, 0xdcc7u);
    Write8(memory, EVENT_PUSH_OBJECT_INDEX, A8(cpu));
    Lufia2EventNextByte(memory, cpu, 0xdcceu);
    StoreADirect8(memory, cpu, 0x22u);
    PushY(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x80u, 0xdcd5u);
    TransferDirectToA(cpu);                                    /* DCDA */
    LoadA8(cpu, Read8(memory, EVENT_PUSH_OBJECT_INDEX));
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_FIELD_PENDING_RECORD_X, cpu->x)));
    StoreADirect8(memory, cpu, 0x9fu);
    StoreADirect8(memory, cpu, DP_PROBE_X);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_FIELD_PENDING_RECORD_Y, cpu->x)));
    StoreADirect8(memory, cpu, 0xa0u);
    StoreADirect8(memory, cpu, DP_PROBE_Y);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_FIELD_PENDING_OBJECT_RECORD,
        cpu->x)));
    cpu->carry = 1;
    Sbc8(cpu, 0x10u);
    Write8(memory, EVENT_PUSH_OBJECT_ID, A8(cpu));
    PushY(memory, cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x23u));
    if (!EventPushAllowed(memory, cpu, 0xdd01u, handoff))
        return EVENT_OPCODE_HANDOFF;
    allowed = cpu->carry;
    cpu->y = PullIndexValue(memory, cpu);
    if (allowed) {
        Lufia2EventClaimActor(memory, cpu, 0xdd09u);                 /* DD06 */
        TransferDirectToA(cpu);
        LoadA8(cpu, Read8(memory, EVENT_PUSH_OBJECT_INDEX));
        TransferAToX(cpu);
        LoadA8(cpu, DirectByte(memory, cpu, DP_ACTOR_SLOT));
        Write8(memory, LongIndexedAddress(EVENT_LISTED_ACTOR_SLOTS, cpu->x), A8(cpu));
        LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);
        LoadA8(cpu, 0x01u);
        Write8(memory, LongIndexedAddress(WRAM_UNK_7FE216, cpu->x), A8(cpu));
        LoadA8(cpu, DirectByte(memory, cpu, 0x9fu));
        StoreAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_X, cpu->x);
        StoreADirect8(memory, cpu, DP_PROBE_X);
        LoadA8(cpu, DirectByte(memory, cpu, 0xa0u));
        StoreAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_Y, cpu->x);
        StoreADirect8(memory, cpu, DP_PROBE_Y);
        SimulateJslFrame(memory, cpu, 0x80u, 0xdd2fu);
        Lufia2ActorSyncFinePosition(memory, cpu);              /* $83:A746 */
        SimulateRtlFrame(memory, cpu);
        LoadXDirect16(memory, cpu, DP_ACTOR_SLOT);             /* DD30 */
        LoadA8(cpu, DirectByte(memory, cpu, 0x23u));
        StoreAAbsolute8(memory, cpu, WRAM_EVENT_MAP_0692, cpu->x);
        LoadA8(cpu, DirectByte(memory, cpu, 0x22u));
        Write8(memory, LongIndexedAddress(WRAM_UNK_7FE4DE, cpu->x), A8(cpu));
        LoadA8(cpu, 0x01u);
        Write8(memory, LongIndexedAddress(WRAM_ACTOR_PRIMARY_TIMER, cpu->x), A8(cpu));
        LoadAAbsolute8(memory, cpu, WRAM_ACTOR_FLAGS, cpu->x);
        And8(cpu, 0xf7u);
        StoreAAbsolute8(memory, cpu, WRAM_ACTOR_FLAGS, cpu->x);
        LoadA8(cpu, 0x09u);
        StoreAAbsolute8(memory, cpu, WRAM_UNK_7E070A, cpu->x);
        SimulateJslFrame(memory, cpu, 0x80u, 0xdd53u);
        Lufia2ActorLoadPrimaryScript(memory, cpu);             /* $83:D416 */
        SimulateRtlFrame(memory, cpu);
        SimulateJslFrame(memory, cpu, 0x80u, 0xdd57u);         /* $83:F9A5 */
        Lufia2MapCellIndex(memory, cpu, 0xf9a7u, 1);
        SimulateRtlFrame(memory, cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_FIELD_MAP_ATTRIBUTES,
            cpu->x)));
        And8(cpu, 0xf7u);
        Write8(memory, LongIndexedAddress(WRAM_FIELD_MAP_ATTRIBUTES, cpu->x), A8(cpu));
        LoadA8(cpu, DirectByte(memory, cpu, 0x23u));
        if (!EventProbeStep(memory, cpu, 0x80u, 0xdd67u, handoff))
            return EVENT_OPCODE_HANDOFF;
        SimulateJslFrame(memory, cpu, 0x80u, 0xdd6bu);
        Lufia2MapCellIndex(memory, cpu, 0xf9a7u, 1);
        SimulateRtlFrame(memory, cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_FIELD_MAP_ATTRIBUTES,
            cpu->x)));
        Or8(cpu, 0x08u);
        Write8(memory, LongIndexedAddress(WRAM_FIELD_MAP_ATTRIBUTES, cpu->x), A8(cpu));
        TransferDirectToA(cpu);                                /* DD76 */
        LoadA8(cpu, DirectByte(memory, cpu, 0x23u));
        TransferAToX(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x83c1a5u, cpu->x)));
        cpu->carry = 0;
        Adc8(cpu, 0x48u);
        if (!Lufia2EventActorAction(memory, cpu, 0xdd84u, handoff))
            return EVENT_OPCODE_HANDOFF;
    }
    SimulateRtlFrame(memory, cpu);                             /* DD05 / DD85 */
    cpu->y = PullIndexValue(memory, cpu);                      /* DCD6 */
    return EVENT_OPCODE_NEXT;
}
