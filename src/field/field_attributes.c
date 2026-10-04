/* Map cell attributes at map load ($80:ED9C). */

#include "actor/actor_internal.h"
#include "core/cpu_internal.h"
#include "core/cpu_ops.h"
#include "core/plain_ops.h"
#include "core/wram_view.h"
#include "field/event_script_internal.h"
#include "field/field_internal.h"
#include "lufia2/field.h"
#include "system/dp_scratch.h"
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
    Push8(memory, cpu, 0x80u);
    PullDataBank(memory, cpu);
    LoadY16(cpu, 0x0027u);
    do {
        LoadAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->y);
        BitImmediate8(cpu, 0x06u);
        if (cpu->zero) {
            LoadAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_Y, cpu->y);      /* row */
            StoreAAbsolute8(memory, cpu, SNES_WRMPYA, 0);
            LoadAAbsolute8(memory, cpu, MAP_WIDTH, 0);
            StoreAAbsolute8(memory, cpu, SNES_WRMPYB, 0);
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
            TransferDirectToA(cpu);
            LoadAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_X, cpu->y);      /* column */
            SetAccumulatorWidth(cpu, 0);
            cpu->carry = 0;
            Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, SNES_RDMPYL, 0));
            TransferAToX(cpu);
            SetAccumulatorWidth(cpu, 1);
            AttributeSet(memory, cpu, ATTRIBUTES, 0x01u);
            LoadA8(cpu, DirectByte(memory, cpu, 0x9au));
            if (!cpu->zero)
                AttributeSet(memory, cpu, ATTRIBUTES + 1u, 0x01u);
        }
        LoadY16(cpu, (uint16_t)(cpu->y - 1u));
    } while (!cpu->negative);
    SimulateRtlFrame(memory, cpu);
}

/* $80:EEEB: bit 2 at each listed cell; DB = $7E. */
static void AttributeListBit2(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0xee77u);
    LoadA8(cpu, 0x7eu);
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
    PushAccumulator8(memory, cpu);
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
    StoreZeroAbsolute8(memory, cpu, (WRAM_FIELD_SECTION_WIDTH + 1u), 0);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_FIELD_LAYER_HEIGHT, cpu->x)));
    Write8(memory, 0x004203u, A8(cpu));
    StoreAAbsolute8(memory, cpu, MAP_HEIGHT, 0);
    StoreZeroAbsolute8(memory, cpu, 0x05bcu, 0);
    LoadA8(cpu, 0x7fu);
    StoreADirect8(memory, cpu, 0x62u);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, WRAM_FIELD_METATILE_ATTRIBUTE_BASE));
    StoreADirect16(memory, cpu, 0x60u);                        /* tile classes */
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_FIELD_LAYER_CELL_BASE,
        cpu->x)));
    TransferAToX(cpu);                                         /* cells */
    LoadY16(cpu, 0x0000u);
    LoadA16(cpu, Read16Long(memory, 0x004216u));
    cpu->carry = 0;
    Add16Value(cpu, 0x0100u);
    StoreADirect16(memory, cpu, DP_SCRATCH_A); /* cell count */
    SetAccumulatorWidth(cpu, 1);
    for (;;) {
        uint8_t class_bits;

        PushY(memory, cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7f0001u, cpu->x)));
        And8(cpu, 0x03u);
        ExchangeAccumulatorBytes(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7f0000u, cpu->x)));
        TransferAToY(cpu);                                     /* metatile */
        Write8(memory, DirectAddress(cpu, DP_SCRATCH_C), 0x00u);
        LoadA8(cpu, Read8IndirectLongY(memory, cpu, 0x60u));
        class_bits = 0;
        if (A8(cpu) & 0xf0u) {
            BitImmediate8(cpu, 0xf0u);
            class_bits = 0x08u;
        } else {
            BitImmediate8(cpu, 0xf0u);
            Compare8(cpu, A8(cpu), 0x08u);
            if (cpu->zero) {
                class_bits = 0x02u;
            } else {
                Compare8(cpu, A8(cpu), 0x09u);
                if (cpu->zero)
                    class_bits = 0x80u;
            }
        }
        if (class_bits) {
            LoadA8(cpu, class_bits);
            StoreADirect8(memory, cpu, DP_SCRATCH_C);
        }
        cpu->y = PullIndexValue(memory, cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7f0001u, cpu->x)));
        And8(cpu, 0x0cu);
        AslA8(cpu);
        AslA8(cpu);
        Or8(cpu, DirectByte(memory, cpu, DP_SCRATCH_C));
        StoreAAbsolute8(memory, cpu, 0x4000u, cpu->y);
        IncrementY16(cpu);
        IncrementX16(cpu);
        IncrementX16(cpu);
        DecrementDirect8(memory, cpu, DP_SCRATCH_A);
        if (!cpu->zero)
            continue;
        DecrementDirect8(memory, cpu, DP_SCRATCH_B);
        if (cpu->zero)
            break;
    }
    AttributeActors(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, 0x7ef026u));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    for (;;) {
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7ef000u, cpu->x)));
        Compare8(cpu, A8(cpu), 0xffu);
        if (cpu->zero)
            break;
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7ef002u, cpu->x)));
        StoreAAbsolute8(memory, cpu, SNES_WRMPYA, 0);
        LoadAAbsolute8(memory, cpu, MAP_WIDTH, 0);
        StoreAAbsolute8(memory, cpu, SNES_WRMPYB, 0);
        TransferDirectToA(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7ef001u, cpu->x)));
        PushIndex(memory, cpu);
        SetAccumulatorWidth(cpu, 0);
        cpu->carry = 0;
        Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, SNES_RDMPYL, 0));
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
        AttributeSet(memory, cpu, ATTRIBUTES, 0x01u);
        cpu->x = PullIndexValue(memory, cpu);
        IncrementX16(cpu);
        IncrementX16(cpu);
        IncrementX16(cpu);
        IncrementX16(cpu);
    }
    LoadX16(cpu, 0x001eu);
    AttributeListBit2(memory, cpu);
    LoadX16(cpu, 0x0000u);
    do {
        PushIndex(memory, cpu);
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
                StoreADirect8(memory, cpu, DP_SCRATCH_A); /* size */
                LoadA8(cpu, DirectByte(memory, cpu, DP_PROBE_Y));
                cpu->carry = 1;
                Sbc8(cpu, DirectByte(memory, cpu, DP_SCRATCH_A));
                LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
                StoreADirect8(memory, cpu, DP_PROBE_Y);
                SimulateJslFrame(memory, cpu, 0x80u, 0xeeb8u);
                cpu->program_bank = 0x83u;
                Lufia2MapCellIndex(memory, cpu, 0xf9a7u, 1);   /* $83:F9A5 */
                cpu->program_bank = 0x80u;
                SimulateRtlFrame(memory, cpu);
                LoadA8(cpu, DirectByte(memory, cpu, DP_SCRATCH_A));
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
                AttributeSet(memory, cpu, ATTRIBUTES, 0x08u);
            }
        }
        cpu->x = PullIndexValue(memory, cpu);
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x0030u);
    } while (!cpu->zero);
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    if (cpu->accumulator_is_8_bit)
        LoadA8(cpu, Pull8(memory, cpu));
    else
        PullAccumulator16(memory, cpu);
    return result;
}

enum {
    PACKED_LAYOUT = 0x05aau,
    PACKED_WIDTH = 0x7fd010u,
    PACKED_HEIGHT = 0x7fd018u,
    PACKED_TARGET = 0x7fd008u,
    PACKED_SOURCE = 0x7fc000u,
    ATTRIBUTE_BYTE = 0x7f0001u,
    PACKED_BYTE = 0x54u,
    ATTRIBUTE_FIELD = 0x55u,
    PACKED_GROUP_COUNT = 0x58u,
    ATTRIBUTE_MASK = 0xcfu,
    CELLS_PER_GROUP = 4u,
    ATTRIBUTE_STRIDE = 2u,
    ATTRIBUTE_BANK = 0x7fu,
    FIELD_STACK_MIN = 0x1f00u,
    FIELD_STACK_MAX = 0x1ffcu
};

/* The count guard uses only RAM before the routine changes any state. */
static uint16_t PackedAttributeGroups(const Lufia2Memory *memory, bool decimal) {
    const uint8_t layout = Read8(memory, PACKED_LAYOUT);
    const uint8_t width = Read8(memory, PACKED_WIDTH + layout);
    const uint8_t height = Read8(memory, PACKED_HEIGHT + layout);
    const Word16Result rounded = Sum16Mode((uint16_t)(width * height),
        CELLS_PER_GROUP - 1u, false, decimal);

    return (uint16_t)(rounded.value >> 2);
}

/* Preserve the other attribute bits and advance to the next cell. */
static uint8_t MergePackedAttribute(Lufia2Wram wram, uint16_t cell) {
    const uint8_t original = WramReadAt(wram, ATTRIBUTE_BYTE, cell);
    const uint8_t field = WramRead(wram, ATTRIBUTE_FIELD);
    const uint8_t merged = (uint8_t)((original & ATTRIBUTE_MASK) | field);

    WramWriteAt(wram, ATTRIBUTE_BYTE, cell, merged);
    return merged;
}

/* Four packed two-bit fields become bits 4-5 of consecutive attributes. */
Lufia2ExecutionResult Lufia2FieldUnpackAttributes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewInBank(memory, cpu, ATTRIBUTE_BANK);
    uint8_t layout;
    uint8_t width;
    uint8_t height;
    uint8_t last_attribute = 0;
    uint16_t cell;
    uint16_t group = 0;
    uint16_t group_count;
    Word16Result rounded;

    if (cpu->direct_page != 0u || cpu->stack < FIELD_STACK_MIN ||
        cpu->stack > FIELD_STACK_MAX ||
        PackedAttributeGroups(memory, cpu->decimal) == 0u)
        return ExecutionHandoff(cpu, 0x80ed0eu);
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    OpSetDataBank(memory, cpu, ATTRIBUTE_BANK);
    layout = Read8(memory, PACKED_LAYOUT);
    width = Read8(memory, PACKED_WIDTH + layout);
    Write8(memory, SNES_WRMPYA, width);
    height = Read8(memory, PACKED_HEIGHT + layout);
    Write8(memory, SNES_WRMPYB, height);
    cell = Read16Long(memory, PACKED_TARGET + layout);
    rounded = Sum16Mode(Read16Long(memory, SNES_RDMPYL),
        CELLS_PER_GROUP - 1u, false, cpu->decimal);
    group_count = (uint16_t)(rounded.value >> 2);
    WramWrite16(wram, PACKED_GROUP_COUNT, group_count);
    do {
        unsigned pair;
        uint8_t packed = WramReadAt(wram, PACKED_SOURCE, group);

        WramWrite(wram, PACKED_BYTE, packed);
        for (pair = 0; pair < CELLS_PER_GROUP; ++pair) {
            uint8_t field;

            if (pair != 0u)
                packed = WramRead(wram, PACKED_BYTE);
            field = (uint8_t)(((packed >> (pair * 2u)) & 3u) << 4);
            WramWrite(wram, ATTRIBUTE_FIELD, field);
            last_attribute = MergePackedAttribute(wram, cell);
            cell = (uint16_t)(cell + ATTRIBUTE_STRIDE);
        }
        group = (uint16_t)(group + 1u);
    } while (group != WramRead16(wram, PACKED_GROUP_COUNT));
    cpu->x = cell;
    cpu->y = group;
    cpu->accumulator = (uint16_t)((group_count & 0xff00u) | last_attribute);
    cpu->carry = 1;
    cpu->overflow = rounded.overflow;
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x80ed9bu);
}
