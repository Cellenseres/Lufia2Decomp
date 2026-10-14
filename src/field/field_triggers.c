/* Field triggers, touch scans and rectangles. */

#include <stdbool.h>

#include "actor/actor_internal.h"
#include "core/child_call.h"
#include "core/cpu_internal.h"
#include "core/cpu_ops.h"
#include "field/field_internal.h"
#include "lufia2/field.h"
#include "system/dp_scratch.h"
#include "system/wram.h"

/* $83:D927: edge bit of the cell next to $8F/$91. */
static void FieldEdgeTest(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint16_t sites[4] = {0xd936u, 0xd940u, 0xd94au, 0xd956u};
    const unsigned slot = (cpu->x >> 1) & 3u;

    SimulateJsrFrame(memory, cpu, 0xb966u); /* B964 */
    if (slot == 0)
        IncrementDirect8(memory, cpu, DP_PROBE_Y); /* D932 */
    else if (slot == 3)
        IncrementDirect8(memory, cpu, DP_PROBE_X); /* D952 */
    Lufia2MapCellIndex(memory, cpu, sites[slot], 1);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_FIELD_MAP_ATTRIBUTES, cpu->x)));
    BitImmediate8(cpu, (slot == 0 || slot == 2) ? 0x10u : 0x20u);
    SimulateRtsFrame(memory, cpu);
}

/* $83:B8DF-$83:B93A: does actor X touch the leader? */
static bool FieldTouchOverlaps(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    uint8_t hit = 0;

    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->x); /* B8DF */
    BitImmediate8(cpu, 0x04u);
    if (!cpu->zero)
        return false;
    BitImmediate8(cpu, 0x80u);
    if (!cpu->zero)
        return false;
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_FLAGS, cpu->x);
    BitImmediate8(cpu, 0x14u);
    if (!cpu->zero)
        return false;
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_ID, cpu->x);
    Compare8(cpu, A8(cpu), 0xfdu);
    if (cpu->zero)
        return false;
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FE216, cpu->x)));
    And8(cpu, 0x02u);
    LsrA8(cpu);
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_B), A8(cpu));
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_Y, cpu->x); /* B901 */
    Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_PROBE_Y)));
    if (!cpu->zero) {
        LoadA8(cpu, (uint8_t)(A8(cpu) - 2u)); /* B908 */
        Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_PROBE_Y)));
        if (cpu->carry)
            return false;
        LoadA8(cpu, (uint8_t)(A8(cpu) + 3u)); /* B90E */
        Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_PROBE_Y)));
        if (!cpu->carry)
            return false;
        LoadAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_X, cpu->x); /* B915 */
        Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_PROBE_X)));
        if (cpu->zero) {
            hit = 1;
        } else {
            cpu->carry = 0;
            Adc8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_B)));
            Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_PROBE_X)));
            hit = cpu->zero;
        }
    } else {
        LoadAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_X, cpu->x); /* B925 */
        LoadA8(cpu, (uint8_t)(A8(cpu) - 2u));
        Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_PROBE_X)));
        if (cpu->carry)
            return false;
        LoadA8(cpu, (uint8_t)(A8(cpu) + 3u)); /* B92E */
        cpu->carry = 0;
        Adc8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_B)));
        Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_PROBE_X)));
        hit = cpu->carry;
    }
    return hit != 0;
}

/* $83:B8BF: actor 8..39 touching the leader; 0 = handoff. */
static uint8_t FieldTouchScanBody(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadAAbsolute8(memory, cpu, 0x057cu, 0); /* B8BF */
    if (!cpu->zero) {
        LoadA8(cpu, 0x80u);
        And8(cpu, Read8(memory, DirectAddress(cpu, 0x47u)));
        if (!cpu->zero) {
            SimulateRtsFrame(memory, cpu); /* B8CA */
            return 1;
        }
    }
    SetIndexWidth(cpu, 0); /* B8CB */
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_X, 0);
    Write8(memory, DirectAddress(cpu, DP_PROBE_X), A8(cpu));
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_Y, 0);
    Write8(memory, DirectAddress(cpu, DP_PROBE_Y), A8(cpu));
    Lufia2MapTileHeight(memory, cpu, 0xb8d9u);
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_C), A8(cpu));
    LoadX16(cpu, 0x0008u);
    for (;;) {
        if (FieldTouchOverlaps(memory, cpu)) {
            uint8_t side;

            StoreXDirect16(memory, cpu, 0x65u); /* B942 */
            LoadAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_Y, cpu->x);
            Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_PROBE_Y)));
            if (!cpu->zero) {
                side = cpu->carry ? 0u : 4u; /* B94B */
            } else {
                LoadAAbsolute8(memory, cpu, WRAM_ACTOR_TILE_X, cpu->x); /* B955 */
                Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_PROBE_X)));
                if (cpu->zero) {
                    cpu->resume_pc = 0x83b96du;
                    return 0;
                }
                side = cpu->carry ? 6u : 2u;
            }
            LoadX16(cpu, side);
            FieldEdgeTest(memory, cpu);
            if (cpu->zero) {
                cpu->resume_pc = 0x83b96du; /* B967 */
                return 0;
            }
            LoadXDirect16(memory, cpu, 0x65u); /* B969 */
        }
        IncrementX16(cpu); /* B93A */
        Compare16(cpu, cpu->x, 0x0028u);
        if (cpu->zero)
            break;
    }
    cpu->carry = 0; /* B940 */
    SimulateRtsFrame(memory, cpu);
    return 1;
}

enum {
    DP_TRIGGER_TILE_X = 0x8fu,
    DP_TRIGGER_TILE_Y = 0x91u,
    FIELD_TRIGGER_STATE = 0x09a8u,
    FIELD_PENDING_EVENT = 0x7fd0a3u
};

typedef struct {
    const Lufia2Memory *memory;
    Lufia2ExecutionResult result;
} FieldTriggerLegacy;

static uint8_t LegacyTriggerChild(void *context, Lufia2CpuState *cpu, uint32_t target,
                                  uint32_t site, uint8_t frame_size) {
    FieldTriggerLegacy *legacy = context;
    const Lufia2Memory *memory = legacy->memory;
    (void)site;
    (void)frame_size;
    if (target == 0x80cbaeu) {
        unsigned passes;
        if (!Lufia2FieldEventTimerBody(memory, cpu, &passes)) {
            legacy->result = ExecutionHandoff(cpu, cpu->resume_pc);
            legacy->result.dispatches = passes;
            return 0u;
        }
        SimulateRtlFrame(memory, cpu);
        cpu->program_bank = 0x83u;
        return 1u;
    }
    if (target == 0x8380cdu) {
        Lufia2FieldIdleBody(memory, cpu);
        SimulateRtsFrame(memory, cpu);
        return 1u;
    }
    if (target == 0x83b8bfu) {
        if (FieldTouchScanBody(memory, cpu))
            return 1u;
        legacy->result = ExecutionHandoff(cpu, cpu->resume_pc);
        return 0u;
    }
    legacy->result = ExecutionHandoff(cpu, target);
    return 0u;
}

static uint8_t FieldTriggerChild(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                 Lufia2PushedChildCall child, void *context,
                                 uint32_t site, uint32_t target, uint8_t frame_size,
                                 uint8_t index_width, Lufia2ExecutionResult *result) {
    const uint8_t data_bank = cpu->data_bank;
    if (!CallChildWithFrame(memory, cpu, child, context, site, target, frame_size,
                            0x83u)) {
        *result =
            child == LegacyTriggerChild
                ? ((FieldTriggerLegacy *)context)->result
                : (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return 0u;
    }
    if (child == LegacyTriggerChild)
        return 1u;
    if (cpu->program_bank != 0x83u || cpu->data_bank != data_bank ||
        !cpu->accumulator_is_8_bit || cpu->index_is_8_bit != index_width ||
        cpu->direct_page || cpu->decimal) {
        *result = ExecutionHandoff(cpu, site + frame_size + 1u);
        return 0u;
    }
    return 1u;
}

static void ReadTriggerPosition(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_TILE_X));
    OpSta(memory, cpu, OpDp(cpu, DP_TRIGGER_TILE_X));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_TILE_Y));
    OpSta(memory, cpu, OpDp(cpu, DP_TRIGGER_TILE_Y));
}

static void UseTriggerWramBank(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLoadA(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
}

static uint8_t ProbeFieldContact(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                 Lufia2PushedChildCall child, void *context,
                                 Lufia2ExecutionResult *result) {
    OpLda(memory, cpu, OpAbs(cpu, WRAM_WINDOW_MODE));
    OpBitValue(cpu, 1u);
    if (cpu->zero)
        return 1u;
    OpSepWidths(cpu, 0x10u);
    if (!FieldTriggerChild(memory, cpu, child, context, 0x8381d8u, 0x8380cdu, 2u, 1u,
                           result))
        return 0u;
    OpRepWidths(cpu, 0x10u);
    if (!cpu->zero)
        return 1u;
    OpLda(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
    OpBitValue(cpu, 0x3cu);
    if (!cpu->zero)
        return 1u;
    PushDataBank(memory, cpu);
    ReadTriggerPosition(memory, cpu);
    UseTriggerWramBank(memory, cpu);
    if (!FieldTriggerChild(memory, cpu, child, context, 0x8381f6u, 0x83b8bfu, 2u, 0u,
                           result))
        return 0u;
    PullDataBank(memory, cpu);
    return 1u;
}

static uint8_t RunPendingFieldEvent(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                    Lufia2PushedChildCall child, void *context,
                                    Lufia2ExecutionResult *result) {
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_STATE));
    OpBitValue(cpu, 0x40u);
    if (cpu->zero)
        return 1u;
    OpBitValue(cpu, 0x80u);
    if (!cpu->zero)
        return 1u;
    OpLda(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
    OpBitValue(cpu, 4u);
    if (cpu->zero) {
        OpBitValue(cpu, 0x38u);
        if (!cpu->zero)
            return 1u;
        OpLda(memory, cpu, OpAbs(cpu, FIELD_TRIGGER_STATE));
        OpBitValue(cpu, 8u);
        if (!cpu->zero)
            return 1u;
    }
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_FLAGS));
    OpBitValue(cpu, 2u);
    if (!cpu->zero)
        return 1u;
    OpLda(memory, cpu, FIELD_PENDING_EVENT);
    if (cpu->zero)
        return 1u;
    if (child == LegacyTriggerChild) {
        *result = ExecutionHandoff(cpu, 0x838225u);
        return 0u;
    }
    OpLoadA(cpu, 0x40u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_ACTOR_STATE), 0u);
    if (!FieldTriggerChild(memory, cpu, child, context, 0x83822au, 0x83c0efu, 2u, 0u,
                           result))
        return 0u;
    OpLdx(cpu, 0u);
    OpLdy(cpu, 0u);
    return FieldTriggerChild(memory, cpu, child, context, 0x838233u, 0x80e722u, 3u, 0u,
                             result) &&
           FieldTriggerChild(memory, cpu, child, context, 0x838237u, 0x80cbaeu, 3u, 0u,
                             result);
}

static uint8_t ProbeFieldRegions(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                 Lufia2PushedChildCall child, void *context,
                                 Lufia2ExecutionResult *result) {
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_FLAGS));
    OpBitValue(cpu, 0x10u);
    if (cpu->zero)
        return 1u;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_STATE));
    OpBitValue(cpu, 0x80u);
    if (!cpu->zero)
        return 1u;
    OpLda(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
    OpBitValue(cpu, 8u);
    if (!cpu->zero)
        return 1u;
    if (child == LegacyTriggerChild) {
        *result = ExecutionHandoff(cpu, 0x838251u);
        return 0u;
    }
    OpLoadA(cpu, 0x10u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_FIELD_FLAGS), 0u);
    ReadTriggerPosition(memory, cpu);
    PushDataBank(memory, cpu);
    UseTriggerWramBank(memory, cpu);
    if (!FieldTriggerChild(memory, cpu, child, context, 0x838265u, 0x83b66eu, 2u, 0u,
                           result) ||
        !FieldTriggerChild(memory, cpu, child, context, 0x838268u, 0x83b747u, 2u, 0u,
                           result))
        return 0u;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_STATE));
    OpBitValue(cpu, 1u);
    if (cpu->zero) {
        OpLda(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
        OpBitValue(cpu, 4u);
        if (cpu->zero && !FieldTriggerChild(memory, cpu, child, context, 0x83827au,
                                            0x83b711u, 2u, 0u, result))
            return 0u;
    }
    PullDataBank(memory, cpu);
    if (!FieldTriggerChild(memory, cpu, child, context, 0x83827eu, 0x83c0efu, 2u, 0u,
                           result) ||
        !FieldTriggerChild(memory, cpu, child, context, 0x838281u, 0x83fb71u, 3u, 0u,
                           result))
        return 0u;
    OpCmpValue(cpu, 0x0bu);
    if (cpu->zero)
        return FieldTriggerChild(memory, cpu, child, context, 0x838289u, 0x83816du, 2u,
                                 0u, result);
    OpCmpValue(cpu, 0x0cu);
    if (cpu->zero)
        return FieldTriggerChild(memory, cpu, child, context, 0x838292u, 0x83813bu, 2u,
                                 0u, result);
    OpCmpValue(cpu, 0x0du);
    return !cpu->zero || FieldTriggerChild(memory, cpu, child, context, 0x83829bu,
                                           0x838386u, 2u, 0u, result);
}

static Lufia2ExecutionResult RunFieldTriggers(const Lufia2Memory *memory,
                                              Lufia2CpuState *cpu,
                                              Lufia2PushedChildCall child,
                                              void *context) {
    Lufia2ExecutionResult result = {0};
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    if (!FieldTriggerChild(memory, cpu, child, context, 0x8381cbu, 0x80cbaeu, 3u, 0u,
                           &result) ||
        !ProbeFieldContact(memory, cpu, child, context, &result) ||
        !RunPendingFieldEvent(memory, cpu, child, context, &result) ||
        !ProbeFieldRegions(memory, cpu, child, context, &result))
        return result;
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x83829fu);
}

Lufia2ExecutionResult Lufia2FieldTriggerUpdate(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu) {
    FieldTriggerLegacy legacy = {memory, {0}};
    return RunFieldTriggers(memory, cpu, LegacyTriggerChild, &legacy);
}

Lufia2ExecutionResult
Lufia2FieldTriggerUpdateWithServices(const Lufia2Memory *memory,
                                          Lufia2CpuState *cpu,
                                          Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x83u || !cpu->accumulator_is_8_bit || cpu->direct_page ||
        cpu->decimal || !child)
        return ExecutionHandoff(cpu, 0x8381c6u);
    return RunFieldTriggers(memory, cpu, child, context);
}

/* $83:B882: first $7E:F000 rectangle holding ($8F, $91); 0 = handoff. */
static uint8_t FieldRectSearch(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                               uint16_t return_address) {
    static const uint8_t edges[4] = {0x01u, 0x03u, 0x02u, 0x04u};
    uint32_t entries;

    SimulateJsrFrame(memory, cpu, return_address);
    SetAccumulatorWidth(cpu, 0); /* B882 */
    StoreYDirect16(memory, cpu, DP_SCRATCH_A);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7ef000u, cpu->x)));
    TransferAToX(cpu);
    for (entries = 0;; ++entries) {
        unsigned i;
        uint8_t inside = 1;

        /* A list without $FF spins the ROM. */
        if (entries == 0x10000u) {
            cpu->resume_pc = 0x83b88bu;
            return 0;
        }
        SetAccumulatorWidth(cpu, 1); /* B88B */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7ef000u, cpu->x)));
        Compare8(cpu, A8(cpu), 0xffu);
        if (cpu->zero) {
            cpu->carry = 0;
            break;
        }
        for (i = 0; i < 4u && inside; ++i) {
            if (i == 0)
                LoadA8(cpu, DirectByte(memory, cpu, DP_PROBE_X));
            else if (i == 2)
                LoadA8(cpu, DirectByte(memory, cpu, DP_PROBE_Y));
            Compare8(cpu, A8(cpu),
                     Read8(memory, LongIndexedAddress(0x7ef000u + edges[i], cpu->x)));
            inside = (i & 1u) ? !cpu->carry : cpu->carry;
        }
        if (inside) {
            cpu->carry = 1; /* B8B2 */
            break;
        }
        SetAccumulatorWidth(cpu, 0); /* B8B5 */
        TransferXToA(cpu);
        cpu->carry = 0;
        Add16Value(cpu, Read16Direct(memory, cpu, DP_SCRATCH_A));
        TransferAToX(cpu);
    }
    SimulateRtsFrame(memory, cpu);
    return 1;
}

static Lufia2ExecutionResult FieldRectHandoff(Lufia2CpuState *cpu) {
    return ExecutionHandoff(cpu, cpu->resume_pc);
}

/* $83:B66E: stair and slope rectangles; sets $05B5 bit 5. */
Lufia2ExecutionResult Lufia2FieldStairRects(const Lufia2Memory *memory,
                                            Lufia2CpuState *cpu) {
    bool publish;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x83b66eu);
    LoadX16(cpu, 0x0002u); /* B66E */
    LoadY16(cpu, 0x000fu);
    if (!FieldRectSearch(memory, cpu, 0xb676u))
        return FieldRectHandoff(cpu);
    if (cpu->carry) {
        LoadAAbsolute8(memory, cpu, 0xf00eu, cpu->x); /* B685 */
        if (cpu->zero)
            return ExecutionReturned(0x83b710u);
    } else {
        LoadX16(cpu, 0x000au); /* B679 */
        LoadY16(cpu, 0x0005u);
        if (!FieldRectSearch(memory, cpu, 0xb681u))
            return FieldRectHandoff(cpu);
        if (!cpu->carry)
            return ExecutionReturned(0x83b684u);
    }
    LoadAAbsolute8(memory, cpu, 0xf000u, cpu->x); /* B68D */
    StoreADirect8(memory, cpu, DP_SCRATCH_A);
    LoadA8(cpu, Read8(memory, 0x7fd0bfu));
    And8(cpu, 0x7fu);
    Compare8(cpu, A8(cpu), DirectByte(memory, cpu, DP_SCRATCH_A));
    if (!cpu->zero) {
        LoadA8(cpu, 0xffu);
        Write8(memory, 0x7fd0bfu, A8(cpu));
    }
    LoadAAbsolute8(memory, cpu, 0xf002u, cpu->x); /* B6A2 */
    Compare8(cpu, A8(cpu), DirectByte(memory, cpu, DP_PROBE_Y));
    if (!cpu->zero) {
        LoadAAbsolute8(memory, cpu, 0xf004u, cpu->x); /* B6A9 */
        DecrementA8(cpu);
        Compare8(cpu, A8(cpu), DirectByte(memory, cpu, DP_PROBE_Y));
        if (!cpu->zero)
            return ExecutionReturned(0x83b710u);
        LoadA8(cpu, Read8(memory, 0x7fd0bfu));
        StoreADirect8(memory, cpu, DP_SCRATCH_B);
        Compare8(cpu, A8(cpu), 0xffu);
        if (cpu->zero)
            StoreADirect8(memory, cpu, DP_SCRATCH_B);
        LoadA8(cpu, Read8(memory, 0x7fd0bfu)); /* B6BD */
        Or8(cpu, 0x80u);
        Write8(memory, 0x7fd0bfu, A8(cpu));
        LoadA8(cpu, DirectByte(memory, cpu, DP_SCRATCH_B));
        publish = !cpu->negative;
        if (publish) {
            LoadAAbsolute8(memory, cpu, 0xf004u, cpu->x);
            StoreAAbsolute8(memory, cpu, 0x05beu, 0);
        }
    } else {
        LoadA8(cpu, Read8(memory, 0x7fd0bfu)); /* B6D3 */
        StoreADirect8(memory, cpu, DP_SCRATCH_B);
        Compare8(cpu, A8(cpu), 0xffu);
        if (cpu->zero)
            Write8(memory, DirectAddress(cpu, DP_SCRATCH_B), 0x00u);
        LoadA8(cpu, Read8(memory, 0x7fd0bfu)); /* B6DF */
        And8(cpu, 0x7fu);
        Write8(memory, 0x7fd0bfu, A8(cpu));
        LoadA8(cpu, DirectByte(memory, cpu, DP_SCRATCH_B));
        publish = cpu->negative;
        if (publish) {
            LoadAAbsolute8(memory, cpu, 0xf002u, cpu->x);
            StoreAAbsolute8(memory, cpu, 0x05beu, 0);
        }
    }
    if (publish) {
        LoadA8(cpu, 0x20u); /* B6F3 */
        TestBitsAbsolute8(memory, cpu, WRAM_FIELD_FLAGS, 1);
        LoadAAbsolute8(memory, cpu, 0xf001u, cpu->x);
        StoreAAbsolute8(memory, cpu, 0x05bdu, 0);
        CopyAbsolute8(memory, cpu, WRAM_ACTOR_FACING, 0x05bfu);
    }
    LoadA8(cpu, Read8(memory, 0x7fd0bfu)); /* B704 */
    And8(cpu, 0x80u);
    Or8(cpu, DirectByte(memory, cpu, DP_SCRATCH_A));
    Write8(memory, 0x7fd0bfu, A8(cpu));
    return ExecutionReturned(0x83b710u);
}

/* $83:B711: event rectangles; a hit runs $83:B727 on LLE. */
Lufia2ExecutionResult Lufia2FieldEventRects(const Lufia2Memory *memory,
                                            Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x83b711u);
    LoadX16(cpu, 0x000cu); /* B711 */
    LoadY16(cpu, 0x0005u);
    if (!FieldRectSearch(memory, cpu, 0xb719u))
        return FieldRectHandoff(cpu);
    if (!cpu->carry)
        return ExecutionReturned(0x83b726u);
    LoadAAbsolute8(memory, cpu, 0xf000u, cpu->x);
    LoadX16(cpu, 0x0008u);
    return ExecutionHandoff(cpu, 0x83b722u);
}

/* $83:B747: area rectangles; a hit runs $83:B76E on LLE. */
Lufia2ExecutionResult Lufia2FieldAreaRects(const Lufia2Memory *memory,
                                           Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x83b747u);
    SetIndexWidth(cpu, 0); /* B747 */
    LoadX16(cpu, 0x0006u);
    LoadY16(cpu, 0x0009u);
    if (!FieldRectSearch(memory, cpu, 0xb751u))
        return FieldRectHandoff(cpu);
    if (!cpu->carry)
        return ExecutionReturned(0x83b76du);
    LoadAAbsolute8(memory, cpu, 0xf005u, cpu->x); /* B754 */
    And8(cpu, 0x0fu);
    Compare8(cpu, A8(cpu), 0x02u);
    if (cpu->zero) {
        LoadAAbsolute8(memory, cpu, WRAM_ACTOR_FACING, 0);
        Compare8(cpu, A8(cpu), 0x04u);
        if (!cpu->zero)
            return ExecutionReturned(0x83b76du);
        LoadA8(cpu, 0x08u);
        TestBitsAbsolute8(memory, cpu, WRAM_FIELD_FLAGS, 1);
    }
    return ExecutionHandoff(cpu, 0x83b769u);
}
