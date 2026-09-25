/* Actor slot traversal and spawning. */

#include "core/cpu_internal.h"
#include "lufia2/actor.h"
#include "actor/actor_internal.h"
#include "system/wram.h"

/* $83:AB4F: record offsets for actor $A7. */
void Lufia2ActorRecordOffsets(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Write8(memory, DirectAddress(cpu, 0xa8u), 0x00u);          /* AB4F */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_ACTOR_SLOT)));
    AslA8(cpu);
    Write8(memory, DirectAddress(cpu, 0xa9u), A8(cpu));
    Write8(memory, DirectAddress(cpu, 0xaau), 0x00u);
    Adc8(cpu, Read8(memory, DirectAddress(cpu, DP_ACTOR_SLOT)));
    Write8(memory, DirectAddress(cpu, 0xabu), A8(cpu));
    Write8(memory, DirectAddress(cpu, 0xacu), 0x00u);
    Write8(memory, DirectAddress(cpu, 0xadu), 0x00u);
}

/* $83:DFFD: script pointer from table $91:8EC7. */
static void SecondarySpawnScript(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);                               /* DFFD */
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x918ec7u, cpu->x)));
    LoadXDirect(memory, cpu, 0xabu);
    cpu->carry = 0;
    Add16Immediate(cpu, 0x8ec7u);
    Write16Long(memory, LongIndexedAddress(0x7fdeeeu, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x91u);
    Write8(memory, LongIndexedAddress(0x7fdef0u, cpu->x), A8(cpu));
}

/* $83:DFA5: initialise actor X with spawn id $54. */
static void SecondarySpawnInit(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const uint32_t zeroed[4] = {
        0x7fdaecu, 0x7fdb0cu, 0x7fe286u, 0x7fe2ceu};
    static const uint32_t filled[3] = {0x7fe1aeu, 0x7fdb2cu, 0x7fe3a6u};
    static const uint32_t cleared[4] = {
        0x7fdcdcu, 0x7fdcddu, 0x7fdd6cu, 0x7fdd6du};
    unsigned i;

    StoreXDirect16(memory, cpu, DP_ACTOR_SLOT);                /* DFA5 */
    SimulateJslFrame(memory, cpu, 0x83u, 0xdfaau);
    Lufia2ActorRecordOffsets(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    LoadA8(cpu, 0x84u);                                        /* DFAB */
    StoreAAbsolute8(memory, cpu, 0x064au, cpu->x);
    LoadA8(cpu, 0x01u);
    Write8(memory, LongIndexedAddress(0x7fdfaeu, cpu->x), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x0692u, 0);
    Write8(memory, LongIndexedAddress(0x7fd9ccu, cpu->x), A8(cpu));
    LoadA8(cpu, 0x20u);
    Write8(memory, LongIndexedAddress(0x7fe33eu, cpu->x), A8(cpu));
    TransferDirectToA(cpu);                                    /* DFC3 */
    for (i = 0; i < 4u; ++i)
        Write8(memory, LongIndexedAddress(zeroed[i], cpu->x), A8(cpu));
    LoadA8(cpu, 0xffu);                                        /* DFD4 */
    for (i = 0; i < 3u; ++i)
        Write8(memory, LongIndexedAddress(filled[i], cpu->x), A8(cpu));
    LoadXDirect(memory, cpu, 0xa9u);                           /* DFE2 */
    TransferDirectToA(cpu);
    for (i = 0; i < 4u; ++i)
        Write8(memory, LongIndexedAddress(cleared[i], cpu->x), A8(cpu));
    TransferDirectToA(cpu);                                    /* DFF5 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    SimulateJslFrame(memory, cpu, 0x83u, 0xdffbu);
    SecondarySpawnScript(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $83:DF87: spawn id A into the first free slot. */
void Lufia2ActorSpawn(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);                                 /* DF87 */
    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
    Push8(memory, cpu, 0x83u);
    PullDataBank(memory, cpu);
    LoadX16(cpu, 0x0000u);
    for (;;) {
        LoadAAbsolute8(memory, cpu, 0x064au, cpu->x);          /* DF8F */
        BitImmediate8(cpu, 0x80u);
        if (cpu->zero) {
            LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
            SimulateJsrFrame(memory, cpu, 0xdf9au);
            SecondarySpawnInit(memory, cpu);
            SimulateRtsFrame(memory, cpu);
            break;
        }
        IncrementX16(cpu);                                     /* DF9D */
        Compare16(cpu, cpu->x, 0x0020u);
        if (cpu->zero)
            break;
    }
    PullDataBank(memory, cpu);                                 /* DFA3 */
}

Lufia2ExecutionResult Lufia2UpdateActorSlots(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2ActorSlotChild child,
    void *child_context) {
    Lufia2ExecutionResult result;

    result.flow = LUFIA2_EXECUTION_RETURNED;
    result.pc = 0x83bbf2u;
    result.dispatches = 0;

    LoadA8(cpu, Read8(memory, 0x7fd0feu));                     /* BB93 */
    if (!cpu->zero) {
        LoadA8(cpu, 0x01u);
        Write8(memory, 0x7fe216u, A8(cpu));
    }
    Write8(memory, DirectAddress(cpu, DP_ACTOR_SLOT), 0x00u);  /* BB9F */
    for (;;) {
        ++result.dispatches;
        /* A child left D set: AB4F's ADC goes BCD. */
        if (cpu->decimal) {
            cpu->resume_pc = 0x83bba1u;
            result.flow = LUFIA2_EXECUTION_BOUNDARY;
            result.pc = cpu->resume_pc;
            return result;
        }
        SimulateJslFrame(memory, cpu, 0x83u, 0xbba4u);         /* BBA1 */
        Lufia2ActorRecordOffsets(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        if (cpu->index_is_8_bit) {                             /* BBA5 */
            cpu->y = Read8(memory, DirectAddress(cpu, DP_ACTOR_SLOT));
            SetNz8(cpu, (uint8_t)cpu->y);
        } else {
            LoadYDirect16(memory, cpu, DP_ACTOR_SLOT);
        }
        LoadAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->y);
        BitImmediate8(cpu, 0x04u);
        if (cpu->zero) {
            uint32_t primary = 0;
            uint8_t gate = 0;

            BitImmediate8(cpu, 0x18u);                         /* BBAE */
            if (!cpu->zero) {
                gate = 1;
            } else {
                BitImmediate8(cpu, 0x01u);
                if (cpu->zero) {
                    TransferYToA8(cpu);                        /* BBB6 */
                    if (cpu->zero)
                        primary = 0x83bbf3u;
                    else
                        gate = 1;
                }
            }
            if (gate) {
                LoadA8(cpu, Read8(memory, 0x7fd0a1u));         /* BBBE */
                BitImmediate8(cpu, 0x2cu);
                if (!cpu->zero)
                    TransferYToA8(cpu);
                if (cpu->zero)
                    primary = 0x83c7f8u;
            }
            if (primary != 0) {
                if (!child(child_context, cpu, primary,
                        primary == 0x83bbf3u ? 0x83bbb9u : 0x83bbc9u)) {
                    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
                    return result;
                }
            }
            if (!child(child_context, cpu, 0x83d508u, 0x83bbccu)) {
                result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
                return result;
            }
            SetAccumulatorWidth(cpu, 1);                       /* BBCF */
            SetIndexWidth(cpu, 1);
        }
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_ACTOR_SLOT))); /* BBD1 */
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        Write8(memory, DirectAddress(cpu, DP_ACTOR_SLOT), A8(cpu));
        Compare8(cpu, A8(cpu), 0x28u);
        if (cpu->zero)
            break;
    }
    LoadA8(cpu, Read8(memory, 0x7fd0feu));                     /* BBDA */
    if (!cpu->zero) {
        LoadA8(cpu, 0x03u);
        Write8(memory, 0x7fe216u, A8(cpu));
    }
    LoadAAbsolute8(memory, cpu, 0x09a1u, 0);                   /* BBE6 */
    Compare8(cpu, A8(cpu), 0xffu);
    if (!cpu->zero) {
        LoadA8(cpu, 0x80u);
        TestBitsAbsolute8(memory, cpu, 0x09a1u, 0);
    }
    return result;
}
