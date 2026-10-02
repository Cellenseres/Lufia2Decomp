/* Map cell attributes at map load ($80:ED9C). */

#include "core/cpu_internal.h"
#include "lufia2/field.h"
#include "actor/actor_internal.h"
#include "field/event_script_internal.h"
#include "field/field_internal.h"
#include "system/wram.h"

enum {
    MAP_WIDTH = 0x05b9u,                /* cells per row, word */
    MAP_HEIGHT = 0x05bbu,               /* rows, word */
    ATTRIBUTES = 0x7e4000u,             /* one byte per cell */
};

/* Cell X |= bit in the attribute map. */
static void AttributeSet(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t base,
    uint8_t bit) {
    LoadA8(cpu, Read8(memory, LongIndexedAddress(base, cpu->x)));
    Or8(cpu, bit);
    Write8(memory, LongIndexedAddress(base, cpu->x), A8(cpu));
}

/* $80:EF2F: occupancy bit 0 under visible map actors. */
static void AttributeActors(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJslFrame(memory, cpu, 0x80u, 0xee33u);
    Push8(memory, cpu, 0x80u);                                 /* EF2F */
    PullDataBank(memory, cpu);
    LoadY16(cpu, 0x0027u);
    do {
        LoadAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->y);          /* EF34 */
        BitImmediate8(cpu, 0x06u);
        if (cpu->zero) {
            LoadAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_Y, cpu->y);      /* row */
            StoreAAbsolute8(memory, cpu, 0x4202u, 0);
            LoadAAbsolute8(memory, cpu, MAP_WIDTH, 0);
            StoreAAbsolute8(memory, cpu, 0x4203u, 0);
            TransferYToX(cpu);
            Write8(memory, DirectAddress(cpu, 0x9au), 0x00u);
            LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FE216, cpu->x)));
            Compare8(cpu, A8(cpu), 0x02u);
            if (cpu->carry) {
                uint8_t kind;

                LoadAAbsolute8(memory, cpu, WRAM_UNK_7E05D2, cpu->x);  /* kind */
                for (kind = 0x71u; kind <= 0x73u; ++kind) {
                    Compare8(cpu, A8(cpu), kind);
                    if (cpu->zero)
                        break;
                }
                if (kind > 0x73u) {
                    LoadA8(cpu, 0xffu);                        /* two cells */
                    StoreADirect8(memory, cpu, 0x9au);
                }
            }
            TransferDirectToA(cpu);                            /* EF65 */
            LoadAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_X, cpu->y);      /* column */
            SetAccumulatorWidth(cpu, 0);
            cpu->carry = 0;
            Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4216u, 0));
            TransferAToX(cpu);
            SetAccumulatorWidth(cpu, 1);
            AttributeSet(memory, cpu, ATTRIBUTES, 0x01u);
            LoadA8(cpu, DirectByte(memory, cpu, 0x9au));
            if (!cpu->zero)
                AttributeSet(memory, cpu, ATTRIBUTES + 1u, 0x01u);
        }
        LoadY16(cpu, (uint16_t)(cpu->y - 1u));                 /* EF8A */
    } while (!cpu->negative);
    SimulateRtlFrame(memory, cpu);
}

/* $80:EEEB: bit 2 at each listed cell; DB = $7E. */
static void AttributeListBit2(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0xee77u);
    LoadA8(cpu, 0x7eu);                                        /* EEEB */
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7ef000u, cpu->x)));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    for (;;) {
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7ef000u, cpu->x)));
        Compare8(cpu, A8(cpu), 0xffu);
        if (cpu->zero)
            break;
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7ef002u, cpu->x)));
        Write8(memory, 0x004202u, A8(cpu));
        LoadAAbsolute8(memory, cpu, MAP_WIDTH, 0);
        Write8(memory, 0x004203u, A8(cpu));
        TransferDirectToA(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7ef001u, cpu->x)));
        SetAccumulatorWidth(cpu, 0);
        cpu->carry = 0;
        Add16Value(cpu, Read16Long(memory, 0x004216u));
        TransferAToY(cpu);
        LoadA16(cpu, cpu->x);
        cpu->carry = 0;
        Add16Value(cpu, 0x0003u);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
        LoadAAbsolute8(memory, cpu, 0x4000u, cpu->y);
        Or8(cpu, 0x04u);
        StoreAAbsolute8(memory, cpu, 0x4000u, cpu->y);
    }
    SimulateRtsFrame(memory, cpu);
}

/* $80:ED9C: attribute map $7E:4000 of map A. */
Lufia2ExecutionResult Lufia2FieldBuildAttributes(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result = ExecutionReturned(0x80eeeau); /* RTL */

    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x80ed9cu);
    PushAccumulator8(memory, cpu);                             /* ED9C */
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, 0x00u);
    ExchangeAccumulatorBytes(cpu);
    TransferAToX(cpu);                                         /* map */
    LoadA8(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_FIELD_LAYER_WIDTH, cpu->x)));
    Write8(memory, 0x004202u, A8(cpu));
    StoreAAbsolute8(memory, cpu, MAP_WIDTH, 0);
    StoreZeroAbsolute8(memory, cpu, 0x05bau, 0);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_FIELD_LAYER_HEIGHT, cpu->x)));
    Write8(memory, 0x004203u, A8(cpu));
    StoreAAbsolute8(memory, cpu, MAP_HEIGHT, 0);
    StoreZeroAbsolute8(memory, cpu, 0x05bcu, 0);
    LoadA8(cpu, 0x7fu);
    StoreADirect8(memory, cpu, 0x62u);
    SetAccumulatorWidth(cpu, 0);                               /* EDCE */
    LoadA16(cpu, Read16Long(memory, WRAM_FIELD_METATILE_ATTRIBUTE_BASE));
    StoreADirect16(memory, cpu, 0x60u);                        /* tile classes */
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_FIELD_LAYER_CELL_BASE,
        cpu->x)));
    TransferAToX(cpu);                                         /* cells */
    LoadY16(cpu, 0x0000u);
    LoadA16(cpu, Read16Long(memory, 0x004216u));
    cpu->carry = 0;
    Add16Value(cpu, 0x0100u);
    StoreADirect16(memory, cpu, 0x54u);                        /* cell count */
    SetAccumulatorWidth(cpu, 1);
    for (;;) {
        uint8_t class_bits;

        PushY(memory, cpu);                                    /* EDEA */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7f0001u, cpu->x)));
        And8(cpu, 0x03u);
        ExchangeAccumulatorBytes(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7f0000u, cpu->x)));
        TransferAToY(cpu);                                     /* metatile */
        Write8(memory, DirectAddress(cpu, 0x56u), 0x00u);
        LoadA8(cpu, Read8IndirectLongY(memory, cpu, 0x60u));
        class_bits = 0;
        if (A8(cpu) & 0xf0u) {
            BitImmediate8(cpu, 0xf0u);
            class_bits = 0x08u;                                /* EDFF */
        } else {
            BitImmediate8(cpu, 0xf0u);
            Compare8(cpu, A8(cpu), 0x08u);
            if (cpu->zero) {
                class_bits = 0x02u;                            /* EE13 */
            } else {
                Compare8(cpu, A8(cpu), 0x09u);
                if (cpu->zero)
                    class_bits = 0x80u;                        /* EE0F */
            }
        }
        if (class_bits) {
            LoadA8(cpu, class_bits);
            StoreADirect8(memory, cpu, 0x56u);
        }
        cpu->y = PullIndexValue(memory, cpu);                  /* EE17 */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7f0001u, cpu->x)));
        And8(cpu, 0x0cu);
        AslA8(cpu);
        AslA8(cpu);
        Or8(cpu, DirectByte(memory, cpu, 0x56u));
        StoreAAbsolute8(memory, cpu, 0x4000u, cpu->y);
        IncrementY16(cpu);
        IncrementX16(cpu);
        IncrementX16(cpu);
        DecrementDirect8(memory, cpu, 0x54u);
        if (!cpu->zero)
            continue;
        DecrementDirect8(memory, cpu, 0x55u);
        if (cpu->zero)
            break;
    }
    AttributeActors(memory, cpu);                              /* EE30 */
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, 0x7ef026u));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    for (;;) {
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7ef000u, cpu->x))); /* EE3D */
        Compare8(cpu, A8(cpu), 0xffu);
        if (cpu->zero)
            break;
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7ef002u, cpu->x)));
        StoreAAbsolute8(memory, cpu, 0x4202u, 0);
        LoadAAbsolute8(memory, cpu, MAP_WIDTH, 0);
        StoreAAbsolute8(memory, cpu, 0x4203u, 0);
        TransferDirectToA(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7ef001u, cpu->x)));
        PushIndex(memory, cpu);
        SetAccumulatorWidth(cpu, 0);
        cpu->carry = 0;
        Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4216u, 0));
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
        AttributeSet(memory, cpu, ATTRIBUTES, 0x01u);
        cpu->x = PullIndexValue(memory, cpu);
        IncrementX16(cpu);
        IncrementX16(cpu);
        IncrementX16(cpu);
        IncrementX16(cpu);
    }
    LoadX16(cpu, 0x001eu);                                     /* EE72 */
    AttributeListBit2(memory, cpu);
    LoadX16(cpu, 0x0000u);
    do {
        PushIndex(memory, cpu);                                /* EE7B */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_FIELD_PENDING_RECORD_X,
            cpu->x)));
        Compare8(cpu, A8(cpu), 0xffu);
        if (!cpu->zero) {
            StoreADirect8(memory, cpu, DP_PROBE_X);                 /* column */
            LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_FIELD_PENDING_RECORD_Y,
                cpu->x)));
            StoreADirect8(memory, cpu, DP_PROBE_Y);                 /* row */
            LoadA8(cpu, 0x0au);
            ExchangeAccumulatorBytes(cpu);
            LoadA8(cpu, Read8(memory,
                LongIndexedAddress(WRAM_FIELD_PENDING_OBJECT_RECORD, cpu->x)));
            cpu->carry = 1;
            Sbc8(cpu, 0x10u);
            LoadX16(cpu, 0x0016u);
            if (!Lufia2FieldListSearch(memory, cpu, 0x80u, 0xee9cu)) {
                result = ExecutionHandoff(cpu, 0x80bfbcu);
                result.dispatches = EVENT_SEARCH_LIMIT;
                return result;
            }
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7ef007u, cpu->x)));
            Or8(cpu, Read8(memory, LongIndexedAddress(0x7ef008u, cpu->x)));
            if (!cpu->zero) {
                LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7ef005u, cpu->x)));
                StoreADirect8(memory, cpu, 0x54u);             /* size */
                LoadA8(cpu, DirectByte(memory, cpu, DP_PROBE_Y));
                cpu->carry = 1;
                Sbc8(cpu, DirectByte(memory, cpu, 0x54u));
                LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
                StoreADirect8(memory, cpu, DP_PROBE_Y);
                SimulateJslFrame(memory, cpu, 0x80u, 0xeeb8u);
                cpu->program_bank = 0x83u;
                Lufia2MapCellIndex(memory, cpu, 0xf9a7u, 1);   /* $83:F9A5 */
                cpu->program_bank = 0x80u;
                SimulateRtlFrame(memory, cpu);
                LoadA8(cpu, DirectByte(memory, cpu, 0x54u));
                Compare8(cpu, A8(cpu), 0x02u);
                if (cpu->zero) {
                    AttributeSet(memory, cpu, ATTRIBUTES, 0x40u);
                    SetAccumulatorWidth(cpu, 0);
                    TransferXToA(cpu);
                    cpu->carry = 0;
                    Add16Value(cpu, Read16Long(memory, WRAM_FIELD_SECTION_WIDTH));
                    TransferAToX(cpu);
                    SetAccumulatorWidth(cpu, 1);
                }
                AttributeSet(memory, cpu, ATTRIBUTES, 0x08u);  /* EED4 */
            }
        }
        cpu->x = PullIndexValue(memory, cpu);                  /* EEDE */
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x0030u);
    } while (!cpu->zero);
    UnpackStatus(cpu, Pull8(memory, cpu));                     /* EEE5 */
    PullDataBank(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    if (cpu->accumulator_is_8_bit)
        LoadA8(cpu, Pull8(memory, cpu));
    else
        PullAccumulator16(memory, cpu);
    return result;
}
