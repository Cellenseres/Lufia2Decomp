/* Field triggers, touch scans and rectangles. */

#include "core/cpu_internal.h"
#include "lufia2/field.h"
#include "actor/actor_internal.h"
#include "field/field_internal.h"
#include "system/wram.h"

static void FieldIdle(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Lufia2FieldIdleBody(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $83:D927: edge bit of the cell next to $8F/$91. */
static void FieldEdgeTest(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const uint16_t sites[4] = {0xd936u, 0xd940u, 0xd94au, 0xd956u};
    const unsigned slot = (cpu->x >> 1) & 3u;

    SimulateJsrFrame(memory, cpu, 0xb966u);                    /* B964 */
    if (slot == 0)
        IncrementDirect8(memory, cpu, DP_PROBE_Y);             /* D932 */
    else if (slot == 3)
        IncrementDirect8(memory, cpu, DP_PROBE_X);             /* D952 */
    Lufia2MapCellIndex(memory, cpu, sites[slot], 1);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7e4000u, cpu->x)));
    BitImmediate8(cpu, (slot == 0 || slot == 2) ? 0x10u : 0x20u);
    SimulateRtsFrame(memory, cpu);
}

/* $83:B8BF: actor 8..39 touching the leader; 0 = handoff. */
static uint8_t FieldTouchScan(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0x81f8u);
    LoadAAbsolute8(memory, cpu, 0x057cu, 0);                   /* B8BF */
    if (!cpu->zero) {
        LoadA8(cpu, 0x80u);
        And8(cpu, Read8(memory, DirectAddress(cpu, 0x47u)));
        if (!cpu->zero) {
            SimulateRtsFrame(memory, cpu);                     /* B8CA */
            return 1;
        }
    }
    SetIndexWidth(cpu, 0);                                     /* B8CB */
    LoadAAbsolute8(memory, cpu, 0x06bau, 0);
    Write8(memory, DirectAddress(cpu, DP_PROBE_X), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x06e2u, 0);
    Write8(memory, DirectAddress(cpu, DP_PROBE_Y), A8(cpu));
    Lufia2MapTileHeight(memory, cpu, 0xb8d9u);
    Write8(memory, DirectAddress(cpu, 0x56u), A8(cpu));
    LoadX16(cpu, 0x0008u);
    for (;;) {
        uint8_t hit = 0;

        LoadAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->x); /* B8DF */
        BitImmediate8(cpu, 0x04u);
        if (!cpu->zero)
            goto next;
        BitImmediate8(cpu, 0x80u);
        if (!cpu->zero)
            goto next;
        LoadAAbsolute8(memory, cpu, 0x0736u, cpu->x);
        BitImmediate8(cpu, 0x14u);
        if (!cpu->zero)
            goto next;
        LoadAAbsolute8(memory, cpu, 0x05fau, cpu->x);
        Compare8(cpu, A8(cpu), 0xfdu);
        if (cpu->zero)
            goto next;
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
        And8(cpu, 0x02u);
        LsrA8(cpu);
        Write8(memory, DirectAddress(cpu, 0x55u), A8(cpu));
        LoadAAbsolute8(memory, cpu, 0x06e2u, cpu->x);          /* B901 */
        Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_PROBE_Y)));
        if (!cpu->zero) {
            LoadA8(cpu, (uint8_t)(A8(cpu) - 2u));              /* B908 */
            Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_PROBE_Y)));
            if (cpu->carry)
                goto next;
            LoadA8(cpu, (uint8_t)(A8(cpu) + 3u));              /* B90E */
            Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_PROBE_Y)));
            if (!cpu->carry)
                goto next;
            LoadAAbsolute8(memory, cpu, 0x06bau, cpu->x);      /* B915 */
            Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_PROBE_X)));
            if (cpu->zero) {
                hit = 1;
            } else {
                cpu->carry = 0;
                Adc8(cpu, Read8(memory, DirectAddress(cpu, 0x55u)));
                Compare8(
                    cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_PROBE_X)));
                hit = cpu->zero;
            }
        } else {
            LoadAAbsolute8(memory, cpu, 0x06bau, cpu->x);      /* B925 */
            LoadA8(cpu, (uint8_t)(A8(cpu) - 2u));
            Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_PROBE_X)));
            if (cpu->carry)
                goto next;
            LoadA8(cpu, (uint8_t)(A8(cpu) + 3u));              /* B92E */
            cpu->carry = 0;
            Adc8(cpu, Read8(memory, DirectAddress(cpu, 0x55u)));
            Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_PROBE_X)));
            hit = cpu->carry;
        }
        if (hit) {
            uint8_t side;

            StoreXDirect16(memory, cpu, 0x65u);                /* B942 */
            LoadAAbsolute8(memory, cpu, 0x06e2u, cpu->x);
            Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_PROBE_Y)));
            if (!cpu->zero) {
                side = cpu->carry ? 0u : 4u;                   /* B94B */
            } else {
                LoadAAbsolute8(memory, cpu, 0x06bau, cpu->x);  /* B955 */
                Compare8(
                    cpu, A8(cpu), Read8(memory, DirectAddress(cpu, DP_PROBE_X)));
                if (cpu->zero) {
                    cpu->resume_pc = 0x83b96du;
                    return 0;
                }
                side = cpu->carry ? 6u : 2u;
            }
            LoadX16(cpu, side);
            FieldEdgeTest(memory, cpu);
            if (cpu->zero) {
                cpu->resume_pc = 0x83b96du;                    /* B967 */
                return 0;
            }
            LoadXDirect16(memory, cpu, 0x65u);                 /* B969 */
        }
next:
        IncrementX16(cpu);                                     /* B93A */
        Compare16(cpu, cpu->x, 0x0028u);
        if (cpu->zero)
            break;
    }
    cpu->carry = 0;                                            /* B940 */
    SimulateRtsFrame(memory, cpu);
    return 1;
}

Lufia2ExecutionResult Lufia2FieldTriggerUpdate(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;
    unsigned passes;

    result.flow = LUFIA2_EXECUTION_BOUNDARY;
    result.dispatches = 0;
    Push8(memory, cpu, PackStatus(cpu));                       /* 81C6 */
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    SimulateJslFrame(memory, cpu, 0x83u, 0x81ceu);
    if (!Lufia2FieldEventTimerBody(memory, cpu, &passes)) {
        result.pc = cpu->resume_pc;
        result.dispatches = passes;
        return result;
    }
    SimulateRtlFrame(memory, cpu);
    cpu->program_bank = 0x83u;
    LoadAAbsolute8(memory, cpu, 0x09a7u, 0);                   /* 81CF */
    BitImmediate8(cpu, 0x01u);
    if (!cpu->zero) {
        SetIndexWidth(cpu, 1);                                 /* 81D6 */
        FieldIdle(memory, cpu, 0x81dau);
        SetIndexWidth(cpu, 0);
        if (cpu->zero) {
            LoadA8(cpu, Read8(memory, 0x7fd0a1u));             /* 81DF */
            BitImmediate8(cpu, 0x3cu);
            if (cpu->zero) {
                PushDataBank(memory, cpu);                     /* 81E7 */
                LoadAAbsolute8(memory, cpu, 0x06bau, 0);
                Write8(memory, DirectAddress(cpu, DP_PROBE_X), A8(cpu));
                LoadAAbsolute8(memory, cpu, 0x06e2u, 0);
                Write8(memory, DirectAddress(cpu, DP_PROBE_Y), A8(cpu));
                LoadA8(cpu, 0x7eu);
                PushAccumulator8(memory, cpu);
                PullDataBank(memory, cpu);
                if (!FieldTouchScan(memory, cpu)) {
                    result.pc = cpu->resume_pc;
                    return result;
                }
                PullDataBank(memory, cpu);                     /* 81F9 */
            }
        }
    }
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, 0);          /* 81FA */
    BitImmediate8(cpu, 0x40u);
    if (!cpu->zero) {
        BitImmediate8(cpu, 0x80u);
        if (cpu->zero) {
            uint8_t armed = 0;

            LoadA8(cpu, Read8(memory, 0x7fd0a1u));             /* 8205 */
            BitImmediate8(cpu, 0x04u);
            if (!cpu->zero) {
                armed = 1;
            } else {
                BitImmediate8(cpu, 0x38u);
                if (cpu->zero) {
                    LoadAAbsolute8(memory, cpu, 0x09a8u, 0);
                    BitImmediate8(cpu, 0x08u);
                    armed = cpu->zero;
                }
            }
            if (armed) {
                LoadAAbsolute8(memory, cpu, WRAM_FIELD_FLAGS, 0); /* 8218 */
                BitImmediate8(cpu, 0x02u);
                if (cpu->zero) {
                    LoadA8(cpu, Read8(memory, 0x7fd0a3u));
                    if (!cpu->zero) {
                        /* $80:E722 queues the event. */
                        result.pc = cpu->resume_pc = 0x838225u;
                        return result;
                    }
                }
            }
        }
    }
    LoadAAbsolute8(memory, cpu, WRAM_FIELD_FLAGS, 0);          /* 823B */
    BitImmediate8(cpu, 0x10u);
    if (!cpu->zero) {
        LoadAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, 0);
        BitImmediate8(cpu, 0x80u);
        if (cpu->zero) {
            LoadA8(cpu, Read8(memory, 0x7fd0a1u));
            BitImmediate8(cpu, 0x08u);
            if (cpu->zero) {
                /* Door/warp handling stays in LLE. */
                result.pc = cpu->resume_pc = 0x838251u;
                return result;
            }
        }
    }
    UnpackStatus(cpu, Pull8(memory, cpu));                     /* 829E */
    result.flow = LUFIA2_EXECUTION_RETURNED;
    result.pc = 0x83829fu;
    return result;
}

/* $83:B882: first $7E:F000 rectangle holding ($8F, $91); 0 = handoff. */
static uint8_t FieldRectSearch(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    static const uint8_t edges[4] = {0x01u, 0x03u, 0x02u, 0x04u};
    uint32_t entries;

    SimulateJsrFrame(memory, cpu, return_address);
    SetAccumulatorWidth(cpu, 0);                               /* B882 */
    StoreYDirect16(memory, cpu, 0x54u);
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
        SetAccumulatorWidth(cpu, 1);                           /* B88B */
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
            Compare8(cpu, A8(cpu), Read8(memory,
                LongIndexedAddress(0x7ef000u + edges[i], cpu->x)));
            inside = (i & 1u) ? !cpu->carry : cpu->carry;
        }
        if (inside) {
            cpu->carry = 1;                                    /* B8B2 */
            break;
        }
        SetAccumulatorWidth(cpu, 0);                           /* B8B5 */
        TransferXToA(cpu);
        cpu->carry = 0;
        Add16Value(cpu, Read16Direct(memory, cpu, 0x54u));
        TransferAToX(cpu);
    }
    SimulateRtsFrame(memory, cpu);
    return 1;
}

static Lufia2ExecutionResult FieldRectHandoff(
    Lufia2CpuState *cpu) {
    return ExecutionHandoff(cpu, cpu->resume_pc);
}

/* $83:B66E: stair and slope rectangles; sets $05B5 bit 5. */
Lufia2ExecutionResult Lufia2FieldStairRects(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x83b66eu);
    LoadX16(cpu, 0x0002u);                                     /* B66E */
    LoadY16(cpu, 0x000fu);
    if (!FieldRectSearch(memory, cpu, 0xb676u))
        return FieldRectHandoff(cpu);
    if (cpu->carry) {
        LoadAAbsolute8(memory, cpu, 0xf00eu, cpu->x);          /* B685 */
        if (cpu->zero)
            return ExecutionReturned(0x83b710u);
    } else {
        LoadX16(cpu, 0x000au);                                 /* B679 */
        LoadY16(cpu, 0x0005u);
        if (!FieldRectSearch(memory, cpu, 0xb681u))
            return FieldRectHandoff(cpu);
        if (!cpu->carry)
            return ExecutionReturned(0x83b684u);
    }
    LoadAAbsolute8(memory, cpu, 0xf000u, cpu->x);              /* B68D */
    StoreADirect8(memory, cpu, 0x54u);
    LoadA8(cpu, Read8(memory, 0x7fd0bfu));
    And8(cpu, 0x7fu);
    Compare8(cpu, A8(cpu), DirectByte(memory, cpu, 0x54u));
    if (!cpu->zero) {
        LoadA8(cpu, 0xffu);
        Write8(memory, 0x7fd0bfu, A8(cpu));
    }
    LoadAAbsolute8(memory, cpu, 0xf002u, cpu->x);              /* B6A2 */
    Compare8(cpu, A8(cpu), DirectByte(memory, cpu, DP_PROBE_Y));
    if (!cpu->zero) {
        LoadAAbsolute8(memory, cpu, 0xf004u, cpu->x);          /* B6A9 */
        DecrementA8(cpu);
        Compare8(cpu, A8(cpu), DirectByte(memory, cpu, DP_PROBE_Y));
        if (!cpu->zero)
            return ExecutionReturned(0x83b710u);
        LoadA8(cpu, Read8(memory, 0x7fd0bfu));
        StoreADirect8(memory, cpu, 0x55u);
        Compare8(cpu, A8(cpu), 0xffu);
        if (cpu->zero)
            StoreADirect8(memory, cpu, 0x55u);
        LoadA8(cpu, Read8(memory, 0x7fd0bfu));                 /* B6BD */
        Or8(cpu, 0x80u);
        Write8(memory, 0x7fd0bfu, A8(cpu));
        LoadA8(cpu, DirectByte(memory, cpu, 0x55u));
        if (cpu->negative)
            goto store;
        LoadAAbsolute8(memory, cpu, 0xf004u, cpu->x);
        StoreAAbsolute8(memory, cpu, 0x05beu, 0);
    } else {
        LoadA8(cpu, Read8(memory, 0x7fd0bfu));                 /* B6D3 */
        StoreADirect8(memory, cpu, 0x55u);
        Compare8(cpu, A8(cpu), 0xffu);
        if (cpu->zero)
            Write8(memory, DirectAddress(cpu, 0x55u), 0x00u);
        LoadA8(cpu, Read8(memory, 0x7fd0bfu));                 /* B6DF */
        And8(cpu, 0x7fu);
        Write8(memory, 0x7fd0bfu, A8(cpu));
        LoadA8(cpu, DirectByte(memory, cpu, 0x55u));
        if (!cpu->negative)
            goto store;
        LoadAAbsolute8(memory, cpu, 0xf002u, cpu->x);
        StoreAAbsolute8(memory, cpu, 0x05beu, 0);
    }
    LoadA8(cpu, 0x20u);                                        /* B6F3 */
    TestBitsAbsolute8(memory, cpu, WRAM_FIELD_FLAGS, 1);
    LoadAAbsolute8(memory, cpu, 0xf001u, cpu->x);
    StoreAAbsolute8(memory, cpu, 0x05bdu, 0);
    CopyAbsolute8(memory, cpu, 0x0692u, 0x05bfu);
store:
    LoadA8(cpu, Read8(memory, 0x7fd0bfu));                     /* B704 */
    And8(cpu, 0x80u);
    Or8(cpu, DirectByte(memory, cpu, 0x54u));
    Write8(memory, 0x7fd0bfu, A8(cpu));
    return ExecutionReturned(0x83b710u);
}

/* $83:B711: event rectangles; a hit runs $83:B727 on LLE. */
Lufia2ExecutionResult Lufia2FieldEventRects(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x83b711u);
    LoadX16(cpu, 0x000cu);                                     /* B711 */
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
Lufia2ExecutionResult Lufia2FieldAreaRects(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x83b747u);
    SetIndexWidth(cpu, 0);                                     /* B747 */
    LoadX16(cpu, 0x0006u);
    LoadY16(cpu, 0x0009u);
    if (!FieldRectSearch(memory, cpu, 0xb751u))
        return FieldRectHandoff(cpu);
    if (!cpu->carry)
        return ExecutionReturned(0x83b76du);
    LoadAAbsolute8(memory, cpu, 0xf005u, cpu->x);              /* B754 */
    And8(cpu, 0x0fu);
    Compare8(cpu, A8(cpu), 0x02u);
    if (cpu->zero) {
        LoadAAbsolute8(memory, cpu, 0x0692u, 0);
        Compare8(cpu, A8(cpu), 0x04u);
        if (!cpu->zero)
            return ExecutionReturned(0x83b76du);
        LoadA8(cpu, 0x08u);
        TestBitsAbsolute8(memory, cpu, WRAM_FIELD_FLAGS, 1);
    }
    return ExecutionHandoff(cpu, 0x83b769u);
}
