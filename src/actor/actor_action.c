/* Actor action and movement core ($83:D350). */

#include "core/cpu_internal.h"
#include "lufia2/actor.h"
#include "actor/actor_internal.h"

/* $83:CA93: step toward $7F:E5A6/E5CE target. */
void Lufia2ActorTargetDirection(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, 0xa7u);                           /* CA93 */
    LoadA8(
        cpu, Read8(
            memory, AbsoluteIndexedAddress(cpu, 0x06bau, cpu->x)));
    Compare8(
        cpu, A8(cpu),
        Read8(memory, LongIndexedAddress(0x7fe5a6u, cpu->x)));
    if (!cpu->zero) {
        const uint8_t ahead = cpu->carry;
        LoadA8(cpu, ahead ? 0x02u : 0x03u);                    /* CA9E */
        cpu->carry = 0;                                        /* CAB5 */
        return;
    }
    LoadA8(
        cpu, Read8(
            memory, AbsoluteIndexedAddress(cpu, 0x06e2u, cpu->x)));
    Compare8(
        cpu, A8(cpu),
        Read8(memory, LongIndexedAddress(0x7fe5ceu, cpu->x)));
    if (!cpu->zero) {
        const uint8_t ahead = cpu->carry;
        LoadA8(cpu, ahead ? 0x00u : 0x01u);                    /* CAAF */
        cpu->carry = 0;
        return;
    }
    cpu->carry = 1;                                            /* CAB7 */
}

/* $83:CD6E..$83:CD91 facing comparators. */
uint8_t Lufia2ActorFacingCompare(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t helper_pc) {
    uint16_t base;
    uint8_t leader_first;

    switch (helper_pc) {
    case 0xcd6eu: base = 0x06e2u; leader_first = 1; break;
    case 0xcd77u: base = 0x06bau; leader_first = 0; break;
    case 0xcd80u: base = 0x06e2u; leader_first = 0; break;
    case 0xcd89u: base = 0x06bau; leader_first = 1; break;
    default: return 0;
    }

    LoadXDirect(memory, cpu, 0xa7u);
    if (leader_first) {
        LoadA8(
            cpu, Read8(memory, AbsoluteIndexedAddress(cpu, base, 0)));
        Compare8(
            cpu, A8(cpu),
            Read8(memory, AbsoluteIndexedAddress(cpu, base, cpu->x)));
    } else {
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, base, cpu->x)));
        Compare8(
            cpu, A8(cpu),
            Read8(memory, AbsoluteIndexedAddress(cpu, base, 0)));
    }
    return 1;
}

static void PrimaryActionBoundaryHelper(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t helper_pc) {
    uint8_t coordinate;

    LoadXDirect(memory, cpu, 0xa7u);

    switch (helper_pc) {
    case 0xd3b7u:
        coordinate = Read8(
            memory, AbsoluteIndexedAddress(cpu, 0x06e2u, cpu->x));
        LoadA8(cpu, coordinate);
        cpu->carry = 0;
        if (!cpu->zero) {
            DecrementA8(cpu);
            Compare8(
                cpu, A8(cpu),
                Read8(memory, LongIndexedAddress(0x7fe61eu, cpu->x)));
        }
        break;

    case 0xd3c5u:
        coordinate = Read8(
            memory, AbsoluteIndexedAddress(cpu, 0x06e2u, cpu->x));
        LoadA8(cpu, coordinate);
        cpu->carry = 0;
        if (!cpu->zero) {
            LoadA8(
                cpu,
                Read8(memory, LongIndexedAddress(0x7fe66eu, cpu->x)));
            DecrementA8(cpu);
            DecrementA8(cpu);
            Compare8(
                cpu, A8(cpu),
                Read8(
                    memory,
                    AbsoluteIndexedAddress(cpu, 0x06e2u, cpu->x)));
        }
        break;

    case 0xd3d7u:
        coordinate = Read8(
            memory, AbsoluteIndexedAddress(cpu, 0x06bau, cpu->x));
        LoadA8(cpu, coordinate);
        cpu->carry = 0;
        if (!cpu->zero) {
            DecrementA8(cpu);
            Compare8(
                cpu, A8(cpu),
                Read8(memory, LongIndexedAddress(0x7fe5f6u, cpu->x)));
        }
        break;

    case 0xd3e5u:
        coordinate = Read8(
            memory, AbsoluteIndexedAddress(cpu, 0x06bau, cpu->x));
        LoadA8(cpu, coordinate);
        cpu->carry = 0;
        if (!cpu->zero) {
            LoadA8(
                cpu,
                Read8(memory, LongIndexedAddress(0x7fe646u, cpu->x)));
            DecrementA8(cpu);
            DecrementA8(cpu);
            Compare8(
                cpu, A8(cpu),
                Read8(
                    memory,
                    AbsoluteIndexedAddress(cpu, 0x06bau, cpu->x)));
        }
        break;

    default:
        break;
    }
}

void Lufia2ActorInstallSecondaryScript(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const uint8_t action = A8(cpu);

    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, action);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(
        cpu, Read16Long(
            memory, LongIndexedAddress(0x918000u, cpu->x)));
    LoadXDirect(memory, cpu, 0xabu);
    cpu->carry = 0;
    Add16Immediate(cpu, 0x8000u);
    Write16Long(
        memory, LongIndexedAddress(0x7fe3eeu, cpu->x),
        cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x91u);
    Write8(
        memory, LongIndexedAddress(0x7fe3f0u, cpu->x), A8(cpu));
}

Lufia2ActorPrimaryActionFlow Lufia2ActorPrimaryActionCore(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    uint16_t helper_pc;

    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));       /* D350 */
    LoadXDirect(memory, cpu, 0xa7u);                          /* D352 */
    Write8(
        memory, LongIndexedAddress(0x7fe466u, cpu->x), A8(cpu));
                                                               /* D354 */
    LoadA8(
        cpu, Read8(
            memory, AbsoluteIndexedAddress(cpu, 0x0736u, cpu->x)));
                                                               /* D358 */
    And8(cpu, 0xefu);                                         /* D35B */
    Write8(
        memory, AbsoluteIndexedAddress(cpu, 0x0736u, cpu->x),
        A8(cpu));                                              /* D35D */
    LoadA8(
        cpu, Read8(
            memory, AbsoluteIndexedAddress(cpu, 0x0622u, cpu->x)));
                                                               /* D360 */
    And8(cpu, 0x28u);                                         /* D363 */
    if (!cpu->zero)
        goto install_secondary_script;                         /* D365 */

    TransferDirectToA(cpu);                                   /* D367 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));     /* D368 */
    Compare8(cpu, A8(cpu), 0x04u);                            /* D36A */
    if (cpu->carry)
        goto install_secondary_script;                         /* D36C */

    AslA8(cpu);                                               /* D36E */
    TransferAToX(cpu);                                        /* D36F */
    helper_pc = Read16ProgramIndexed(memory, cpu, 0xd3afu, cpu->x);
                                                               /* D370 */

    if (helper_pc != 0xd3b7u && helper_pc != 0xd3c5u &&
        helper_pc != 0xd3d7u && helper_pc != 0xd3e5u) {
        cpu->resume_pc = 0x83d370u;
        return LUFIA2_ACTOR_PRIMARY_ACTION_UNKNOWN_D370_TARGET;
    }

    SimulateJsrFrame(memory, cpu, 0xd372u);
    PrimaryActionBoundaryHelper(memory, cpu, helper_pc);
    SimulateRtsFrame(memory, cpu);

    if (!cpu->carry)
        return LUFIA2_ACTOR_PRIMARY_ACTION_RETURN_D3AE;         /* D373 */

    LoadXDirect(memory, cpu, 0xa7u);                           /* D375 */
    LoadA8(
        cpu, Read8(
            memory, AbsoluteIndexedAddress(cpu, 0x06bau, cpu->x)));
                                                               /* D377 */
    Write8(memory, DirectAddress(cpu, 0x8fu), A8(cpu));        /* D37A */
    LoadA8(
        cpu, Read8(
            memory, AbsoluteIndexedAddress(cpu, 0x06e2u, cpu->x)));
                                                               /* D37C */
    Write8(memory, DirectAddress(cpu, 0x91u), A8(cpu));        /* D37F */
    TransferDirectToA(cpu);                                   /* D381 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));     /* D382 */
    TransferAToX(cpu);                                        /* D384 */
    LoadA8(
        cpu, Read8(memory, LongIndexedAddress(0x83c1b0u, cpu->x)));
                                                               /* D385 */

    SimulateJslFrame(memory, cpu, 0x83u, 0xd38cu);             /* D389 */
    if (Lufia2ActorMovementStep(memory, cpu) == 0)
        return LUFIA2_ACTOR_PRIMARY_ACTION_UNKNOWN_D370_TARGET;
    SimulateRtlFrame(memory, cpu);

    /* X8: F9D4's PHX/PLA pair derails its RTS. */
    if (cpu->index_is_8_bit) {
        cpu->resume_pc = 0x83d38du;
        return LUFIA2_ACTOR_PRIMARY_ACTION_X8_BOUNDARY_D38D;
    }
    SimulateJslFrame(memory, cpu, 0x83u, 0xd390u);             /* D38D */
    Lufia2ActorReadMapCellValue(memory, cpu);                   /* FB71 */
    SimulateRtlFrame(memory, cpu);

    Compare8(cpu, A8(cpu), 0x07u);                             /* D391 */
    if (cpu->zero)
        goto install_secondary_script;
    Compare8(cpu, A8(cpu), 0x01u);                             /* D395 */
    if (cpu->zero)
        goto install_secondary_script;
    Or8(cpu, 0x00u);                                          /* D399 */
    if (!cpu->zero)
        return LUFIA2_ACTOR_PRIMARY_ACTION_RETURN_D3AE;         /* D39B */

install_secondary_script:
    LoadXDirect(memory, cpu, 0xa7u);                           /* D39D */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));     /* D39F */
    SimulateJsrFrame(memory, cpu, 0xd3a3u);                   /* D3A1 */
    Lufia2ActorInstallSecondaryScript(memory, cpu);                /* D3F7 */
    SimulateRtsFrame(memory, cpu);
    LoadXDirect(memory, cpu, 0xa7u);                           /* D3A4 */
    LoadA8(cpu, 0x80u);                                       /* D3A6 */
    Or8(
        cpu,
        Read8(memory, AbsoluteIndexedAddress(cpu, 0x0622u, cpu->x)));
                                                               /* D3A8 */
    Write8(
        memory, AbsoluteIndexedAddress(cpu, 0x0622u, cpu->x),
        A8(cpu));                                              /* D3AB */
    return LUFIA2_ACTOR_PRIMARY_ACTION_RETURN_D3AE;
}
