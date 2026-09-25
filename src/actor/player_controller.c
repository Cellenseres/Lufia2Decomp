/* Player controller ($83:C1B4). */

#include "core/cpu_internal.h"
#include "actor/actor_internal.h"

/* $83:867B / $83:8674: pressed-bit test, clear latch on hit. */
static void PlayerPressedClear(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t pressed,
    uint8_t latch,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    And8(cpu, Read8(memory, DirectAddress(cpu, pressed)));
    if (!cpu->zero)
        TrbDirect8(memory, cpu, latch);
    SimulateRtsFrame(memory, cpu);
}

/* $83:F9AD cell under $8F/$91. */
static void PlayerProbeCell(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    Lufia2MapCellIndex(memory, cpu, return_address, 1);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7e4000u, cpu->x)));
}

/* $83:BA76: solid bit of the probed cell. */
static void PlayerProbeSolid(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    PlayerProbeCell(memory, cpu, 0xba78u);
    And8(cpu, 0x01u);
}

/* $83:BA5C..BAAC: step the probe past ledges. */
static void PlayerProbeAhead(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t target) {
    switch (target) {
    case 0xba5cu:
        IncrementDirect8(memory, cpu, 0x91u);
        PlayerProbeCell(memory, cpu, 0xba60u);
        BitImmediate8(cpu, 0x01u);
        if (!cpu->zero)
            return;                                            /* BA67 */
        BitImmediate8(cpu, 0x10u);
        if (!cpu->zero) {
            IncrementDirect8(memory, cpu, 0x91u);              /* BA6D */
            SimulateJsrFrame(memory, cpu, 0xba71u);
            PlayerProbeSolid(memory, cpu);
            SimulateRtsFrame(memory, cpu);
            if (!cpu->zero)
                return;
            IncrementDirect8(memory, cpu, 0x91u);              /* BA74 */
        }
        PlayerProbeSolid(memory, cpu);
        return;
    case 0xba80u:
    case 0xba96u: {
        const uint8_t along_x = target == 0xba80u;
        const uint8_t axis = along_x ? 0x8fu : 0x91u;

        PlayerProbeCell(memory, cpu, along_x ? 0xba82u : 0xba98u);
        And8(cpu, along_x ? 0x20u : 0x10u);
        if (!cpu->zero) {
            DecrementDirect8(memory, cpu, axis);
            SimulateJsrFrame(memory, cpu, along_x ? 0xba8fu : 0xbaa5u);
            PlayerProbeSolid(memory, cpu);
            SimulateRtsFrame(memory, cpu);
            if (!cpu->zero)
                return;
        }
        DecrementDirect8(memory, cpu, axis);                   /* BA92 */
        PlayerProbeSolid(memory, cpu);
        return;
    }
    default:                                                   /* BAAC */
        IncrementDirect8(memory, cpu, 0x8fu);
        PlayerProbeCell(memory, cpu, 0xbab0u);
        BitImmediate8(cpu, 0x01u);
        if (!cpu->zero)
            return;
        BitImmediate8(cpu, 0x20u);
        if (cpu->zero)
            return;
        IncrementDirect8(memory, cpu, 0x8fu);
        PlayerProbeSolid(memory, cpu);
        return;
    }
}

/* $83:BAC2: actor 8..39 on $8F/$91; X and $56. */
static void PlayerFindActorAt(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadX16(cpu, 0x0008u);
    for (;;) {
        LoadAAbsolute8(memory, cpu, 0x0622u, cpu->x);          /* BAC5 */
        BitImmediate8(cpu, 0x04u);
        if (cpu->zero) {
            uint8_t hit = 0;
            unsigned i;

            /* The ROM tests the wide column twice. */
            for (i = 0; i < 2u && !hit; ++i) {
                LoadA8(cpu, Read8(
                    memory, LongIndexedAddress(0x7fe216u, cpu->x)));
                Compare8(cpu, A8(cpu), 0x02u);
                if (cpu->carry) {
                    LoadAAbsolute8(memory, cpu, 0x06bau, cpu->x);
                    LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
                    Compare8(
                        cpu, A8(cpu),
                        Read8(memory, DirectAddress(cpu, 0x8fu)));
                    hit = cpu->zero;
                }
            }
            if (!hit) {
                LoadAAbsolute8(memory, cpu, 0x06bau, cpu->x);  /* BAEC */
                Compare8(
                    cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x8fu)));
                hit = cpu->zero;
            }
            if (hit) {
                LoadAAbsolute8(memory, cpu, 0x06e2u, cpu->x);  /* BAF3 */
                Compare8(
                    cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x91u)));
                cpu->carry = 1;
                if (cpu->zero) {
                    StoreXDirect16(memory, cpu, 0x56u);        /* BB04 */
                    return;
                }
            }
        }
        IncrementX16(cpu);                                     /* BAFB */
        Compare16(cpu, cpu->x, 0x0028u);
        if (cpu->zero) {
            cpu->carry = 0;
            return;
        }
    }
}

/* $83:BA06: talkable actor ahead; carry = found. */
static uint8_t PlayerFindTalkTarget(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    uint16_t target;

    SimulateJsrFrame(memory, cpu, 0xc1d2u);
    SetIndexWidth(cpu, 0);                                     /* BA06 */
    LoadAAbsolute8(memory, cpu, 0x06bau, 0);
    Write8(memory, DirectAddress(cpu, 0x8fu), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x06e2u, 0);
    Write8(memory, DirectAddress(cpu, 0x91u), A8(cpu));
    Lufia2MapTileHeight(memory, cpu, 0xba14u);
    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
    TransferDirectToA(cpu);                                    /* BA17 */
    LoadAAbsolute8(memory, cpu, 0x0692u, 0);
    TransferAToX(cpu);
    target = Read16ProgramIndexed(memory, cpu, 0xba54u, cpu->x);
    if (target != 0xba5cu && target != 0xba80u &&
        target != 0xba96u && target != 0xbaacu) {
        cpu->resume_pc = 0x83ba1cu;
        return 0;
    }
    SimulateJsrFrame(memory, cpu, 0xba1eu);
    PlayerProbeAhead(memory, cpu, target);
    SimulateRtsFrame(memory, cpu);
    cpu->carry = 0;                                            /* BA1F */
    if (!cpu->zero) {
        SimulateJsrFrame(memory, cpu, 0xba24u);
        PlayerFindActorAt(memory, cpu);
        SimulateRtsFrame(memory, cpu);
        if (cpu->carry) {
            uint8_t same_height = 1;

            LoadAAbsolute8(memory, cpu, 0x05d2u, cpu->x);      /* BA27 */
            Compare8(cpu, A8(cpu), 0x70u);
            if (cpu->zero) {
                LoadAAbsolute8(memory, cpu, 0x09a7u, 0);
                BitImmediate8(cpu, 0x01u);
                if (!cpu->zero) {
                    LoadAAbsolute8(memory, cpu, 0x0692u, 0);   /* BA35 */
                    Compare8(cpu, A8(cpu), 0x04u);
                    cpu->carry = cpu->zero;
                    same_height = 0;
                }
            }
            if (same_height) {
                LoadAAbsolute8(memory, cpu, 0x06bau, cpu->x);  /* BA40 */
                Write8(memory, DirectAddress(cpu, 0x8fu), A8(cpu));
                LoadAAbsolute8(memory, cpu, 0x06e2u, cpu->x);
                Write8(memory, DirectAddress(cpu, 0x91u), A8(cpu));
                Lufia2MapTileHeight(memory, cpu, 0xba4cu);
                Compare8(
                    cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x54u)));
                cpu->carry = cpu->zero;
            }
        }
    }
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* $83:C161: D-pad to $22/$23; carry = held. */
static void PlayerReadDirection(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0xc1e5u);
    TransferDirectToA(cpu);                                    /* C161 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x47u)));
    And8(cpu, 0x0fu);
    cpu->carry = 0;
    if (!cpu->zero) {
        TrbDirect8(memory, cpu, 0x4bu);                        /* C169 */
        TransferAToX(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x83d437u, cpu->x)));
        Write8(memory, DirectAddress(cpu, 0x22u), A8(cpu));
        Write8(memory, 0x7fd4f6u, A8(cpu));
        TransferAToX(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x83c1b0u, cpu->x)));
        Write8(memory, DirectAddress(cpu, 0x23u), A8(cpu));
        cpu->carry = 1;
    }
    SimulateRtsFrame(memory, cpu);
}

/* $83:FC56: $057C set, B held, slot 0. */
static void PlayerSkipProbe(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0xc1eau);
    LoadAAbsolute8(memory, cpu, 0x057cu, 0);                   /* FC56 */
    cpu->carry = 0;
    if (!cpu->zero) {
        LoadA8(cpu, 0x80u);
        And8(cpu, Read8(memory, DirectAddress(cpu, 0x47u)));
        if (!cpu->zero) {
            LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0xa7u)));
            if (cpu->zero)
                cpu->carry = 1;                                /* FC67 */
        }
    }
    SimulateRtsFrame(memory, cpu);
}

/* $83:C246: $83:C1B0 entry for direction $22. */
static void PlayerFacingCode(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    TransferDirectToA(cpu);                                    /* C246 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x22u)));
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x83c1b0u, cpu->x)));
    SimulateRtsFrame(memory, cpu);
}

/* $83:FBBD: edge bit toward facing A; carry = set. */
static uint8_t PlayerEdgeAhead(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    uint16_t target;
    uint8_t mask;

    SimulateJsrFrame(memory, cpu, 0xc1f7u);
    ExchangeAccumulatorBytes(cpu);                             /* FBBD */
    LoadA8(cpu, 0x00u);
    ExchangeAccumulatorBytes(cpu);
    TransferAToX(cpu);
    target = Read16ProgramIndexed(memory, cpu, 0xfbc6u, cpu->x);
    if (target != 0xfbceu && target != 0xfbd7u &&
        target != 0xfbdeu && target != 0xfbe5u) {
        cpu->resume_pc = 0x83fbc2u;
        return 0;
    }
    SimulateJsrFrame(memory, cpu, 0xfbc4u);
    if (target == 0xfbceu)
        IncrementDirect8(memory, cpu, 0x91u);
    else if (target == 0xfbe5u)
        IncrementDirect8(memory, cpu, 0x8fu);
    mask = (target == 0xfbceu || target == 0xfbdeu) ? 0x10u : 0x20u;
    SimulateJsrFrame(                                          /* FBF1 */
        memory, cpu,
        target == 0xfbceu ? 0xfbd2u : target == 0xfbd7u ? 0xfbd9u :
        target == 0xfbdeu ? 0xfbe0u : 0xfbe9u);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x8fu)));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x91u)));
    Lufia2MapCellIndex(memory, cpu, 0xfbf8u, 0);            /* F9B6 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7e4000u, cpu->x)));
    SimulateRtsFrame(memory, cpu);
    And8(cpu, mask);                                           /* FBEC */
    cpu->carry = !cpu->zero;
    SimulateRtsFrame(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* $8E:BBAF: vehicle tile pair; boundary past $8E:BBD1. */
static uint8_t PlayerVehicleTile(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    cpu->program_bank = 0x8eu;
    LoadAAbsolute8(memory, cpu, 0x06bau, 0);                   /* BBAF */
    Write8(memory, DirectAddress(cpu, 0x8fu), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x06e2u, 0);
    Write8(memory, DirectAddress(cpu, 0x91u), A8(cpu));
    SimulateJslFrame(memory, cpu, 0x8eu, 0xbbbcu);
    cpu->program_bank = 0x83u;
    Lufia2ActorReadMapCellValue(memory, cpu);                  /* FB71 */
    SimulateRtlFrame(memory, cpu);
    cpu->program_bank = 0x8eu;
    Compare8(cpu, A8(cpu), 0x06u);                             /* BBBD */
    if (cpu->zero) {
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x23u)));
        SimulateJslFrame(memory, cpu, 0x8eu, 0xbbc6u);
        cpu->program_bank = 0x83u;
        if (Lufia2ActorMovementStep(memory, cpu) == 0)
            return 0;
        SimulateRtlFrame(memory, cpu);
        SimulateJslFrame(memory, cpu, 0x8eu, 0xbbcau);
        Lufia2ActorReadMapCellValue(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        cpu->program_bank = 0x8eu;
        Compare8(cpu, A8(cpu), 0x06u);                         /* BBCB */
        if (cpu->zero) {
            cpu->resume_pc = 0x8ebbd1u;
            return 0;
        }
    }
    cpu->carry = 0;                                            /* BBCF */
    return 1;
}

/* $8E:B65A: leader tile inside rectangle at base. */
static uint8_t PlayerInsideRect(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t base) {
    unsigned axis;

    for (axis = 0; axis < 2u; ++axis) {
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, (uint8_t)(0x56u + axis))));
        Compare8(
            cpu, A8(cpu), Read8(memory, AbsoluteIndexedAddress(
                cpu, (uint16_t)(base + axis), cpu->x)));
        if (!cpu->carry)
            return 0;
        Compare8(
            cpu, A8(cpu), Read8(memory, AbsoluteIndexedAddress(
                cpu, (uint16_t)(base + 2u + axis), cpu->x)));
        if (cpu->carry)
            return 0;
    }
    return 1;
}

/* $8E:B63B: door rectangles at $7E:F000; boundary $8E:B6D4. */
static uint8_t PlayerDoorRegion(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    uint32_t entries;

    cpu->program_bank = 0x8eu;
    PushDataBank(memory, cpu);                                 /* B63B */
    LoadAAbsolute8(memory, cpu, 0x06bau, 0);
    Write8(memory, DirectAddress(cpu, 0x56u), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x06e2u, 0);
    Write8(memory, DirectAddress(cpu, 0x57u), A8(cpu));
    LoadA8(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SetIndexWidth(cpu, 0);                                     /* B64A */
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xf002u, 0));
    LoadA8(cpu, 0xffu);
    Write8(memory, DirectAddress(cpu, 0x58u), A8(cpu));
    for (entries = 0;; ++entries) {
        /* A table without $FF spins the ROM. */
        if (entries == 0x10000u) {
            cpu->resume_pc = 0x8eb653u;
            return 0;
        }
        LoadAAbsolute8(memory, cpu, 0xf000u, cpu->x);          /* B653 */
        Compare8(cpu, A8(cpu), 0xffu);
        if (cpu->zero)
            break;
        if (PlayerInsideRect(memory, cpu, 0xf005u)) {
            LoadAAbsolute8(memory, cpu, 0xf00du, cpu->x);      /* B672 */
            Write8(memory, DirectAddress(cpu, 0x58u), A8(cpu));
            break;
        }
        LoadAAbsolute8(memory, cpu, 0xf009u, cpu->x);          /* B679 */
        Compare8(cpu, A8(cpu), 0xffu);
        if (!cpu->zero && PlayerInsideRect(memory, cpu, 0xf009u)) {
            LoadAAbsolute8(memory, cpu, 0xf00du, cpu->x);      /* B698 */
            Write8(memory, DirectAddress(cpu, 0x58u), A8(cpu));
            break;
        }
        SetAccumulatorWidth(cpu, 0);                           /* B69F */
        TransferXToA(cpu);
        cpu->carry = 0;
        Add16Immediate(cpu, 0x000fu);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
    }
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x58u)));     /* B6AB */
    Compare8(cpu, A8(cpu), 0xffu);
    cpu->carry = 0;
    if (!cpu->zero) {
        LoadA8(cpu, Read8(memory, 0x7fd09du));                 /* B6B5 */
        Compare8(cpu, A8(cpu), 0x00u);
        if (cpu->zero) {
            LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x57u)));
            Compare8(
                cpu, A8(cpu),
                Read8(memory, AbsoluteIndexedAddress(cpu, 0xf002u, cpu->x)));
        } else if (Compare8(cpu, A8(cpu), 0x01u), cpu->zero) {
            LoadAAbsolute8(memory, cpu, 0xf002u, cpu->x);      /* B6CD */
            Compare8(
                cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x57u)));
        } else {
            cpu->carry = 0;                                    /* B6C1 */
        }
        if (cpu->carry) {
            cpu->resume_pc = 0x8eb6d4u;
            return 0;
        }
    }
    PullDataBank(memory, cpu);                                 /* B738 */
    return 1;
}

/* $83:FC69: $057C and B pick walk speed $10/$08. */
static void PlayerWalkSpeed(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0xc234u);
    LoadAAbsolute8(memory, cpu, 0x057cu, 0);                   /* FC69 */
    if (!cpu->zero) {
        LoadA8(cpu, 0x80u);
        And8(cpu, Read8(memory, DirectAddress(cpu, 0x47u)));
        if (!cpu->zero) {
            LoadA8(cpu, 0x10u);
            Write8(memory, 0x7fe4deu, A8(cpu));
        } else {
            LoadA8(cpu, Read8(memory, 0x7fe4deu));             /* FC7C */
            Compare8(cpu, A8(cpu), 0x10u);
            if (cpu->zero) {
                LoadA8(cpu, 0x08u);
                Write8(memory, 0x7fe4deu, A8(cpu));
            }
        }
    }
    SimulateRtsFrame(memory, cpu);
}

static Lufia2ActorPrimaryUpdateResult PlayerReturned(uint32_t rts_pc) {
    Lufia2ActorPrimaryUpdateResult result;
    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED;
    result.pc = rts_pc;
    result.dispatches = 0;
    return result;
}

static Lufia2ActorPrimaryUpdateResult PlayerBoundary(
    Lufia2ActorFrontendCpu *cpu, uint32_t pc) {
    Lufia2ActorPrimaryUpdateResult result;
    if (pc)
        cpu->resume_pc = pc;
    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
    result.pc = cpu->resume_pc;
    result.dispatches = 0;
    return result;
}

/* D350 via JSL; 0 on its exact boundary. */
static uint8_t PlayerCallActionCore(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x83u, return_address);
    if (Lufia2ActorPrimaryActionCore(memory, cpu) !=
        LUFIA2_ACTOR_PRIMARY_ACTION_RETURN_D3AE)
        return 0;
    SimulateRtlFrame(memory, cpu);
    return 1;
}

Lufia2ActorPrimaryUpdateResult Lufia2PlayerSlotStandardUpdate(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadAAbsolute8(memory, cpu, 0x099bu, 0);                   /* C1B4 */
    if (cpu->negative)
        return PlayerReturned(0x83c1e2u);
    LoadA8(cpu, 0xa0u);                                        /* C1B9 */
    PlayerPressedClear(memory, cpu, 0x46u, 0x4au, 0xc1bdu);
    if (!cpu->zero) {
        if (!PlayerFindTalkTarget(memory, cpu))                /* C1D0 */
            return PlayerBoundary(cpu, 0);
        if (!cpu->carry)
            return PlayerBoundary(cpu, 0x83c1dcu);             /* $8E:C05F */
        LoadXDirect(memory, cpu, 0x56u);                       /* C1D5 */
        return PlayerBoundary(cpu, 0x83c1d7u);                 /* BB08 */
    }
    LoadAAbsolute8(memory, cpu, 0x057cu, 0);                   /* C1C0 */
    if (!cpu->zero) {
        LoadA8(cpu, 0x20u);
        PlayerPressedClear(memory, cpu, 0x47u, 0x4bu, 0xc1c9u);
        if (!cpu->zero)
            return PlayerBoundary(cpu, 0x83c1ccu);             /* 83E0 */
    }

    PlayerReadDirection(memory, cpu);                          /* C1E3 */
    if (!cpu->carry)
        return PlayerReturned(0x83c1e2u);
    PlayerSkipProbe(memory, cpu);                              /* C1E8 */
    if (!cpu->carry) {
        SetIndexWidth(cpu, 0);                                 /* C1ED */
        Lufia2ActorLeaderToProbe(memory, cpu, 0xc1f1u);
        PlayerFacingCode(memory, cpu, 0xc1f4u);
        if (!PlayerEdgeAhead(memory, cpu))
            return PlayerBoundary(cpu, 0);
        SetIndexWidth(cpu, 1);                                 /* C1F8 */
        if (cpu->carry) {
            SetIndexWidth(cpu, 0);                             /* C1FC */
            SimulateJslFrame(memory, cpu, 0x83u, 0xc201u);
            if (!PlayerVehicleTile(memory, cpu))
                return PlayerBoundary(cpu, 0);
            SimulateRtlFrame(memory, cpu);
            cpu->program_bank = 0x83u;
            SetIndexWidth(cpu, 1);                             /* C202 */
            if (cpu->carry)
                goto done;
        } else {
            SetIndexWidth(cpu, 0);                             /* C219 */
            Lufia2ActorLeaderToProbe(memory, cpu, 0xc21du);
            PlayerFacingCode(memory, cpu, 0xc220u);
            SimulateJslFrame(memory, cpu, 0x83u, 0xc224u);
            if (Lufia2ActorMovementStep(memory, cpu) == 0)
                return PlayerBoundary(cpu, 0);
            SimulateRtlFrame(memory, cpu);
            PlayerProbeCell(memory, cpu, 0xc227u);             /* C225 */
            SetIndexWidth(cpu, 1);
            BitImmediate8(cpu, 0x01u);
            if (cpu->zero)
                goto walk;
        }
        /* Blocked: turn toward $22 + $18. */
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x22u)));  /* C206 */
        Write8(memory, 0x7fd09du, A8(cpu));
        cpu->carry = 0;
        Adc8(cpu, 0x18u);
        if (!PlayerCallActionCore(memory, cpu, 0xc212u))
            return PlayerBoundary(cpu, 0);
        SimulateJslFrame(memory, cpu, 0x83u, 0xc216u);         /* C213 */
        if (!PlayerDoorRegion(memory, cpu))
            return PlayerBoundary(cpu, 0);
        SimulateRtlFrame(memory, cpu);
        cpu->program_bank = 0x83u;
        goto done;
    }

walk:
    PlayerWalkSpeed(memory, cpu);                              /* C232 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x22u)));
    if (!PlayerCallActionCore(memory, cpu, 0xc23au))
        return PlayerBoundary(cpu, 0);
    SimulateJsrFrame(memory, cpu, 0xc23du);                    /* C23B */
    LoadA8(cpu, 0x20u);                                        /* C0FA */
    TestBitsAbsolute8(memory, cpu, 0x099cu, 0);
    if (!cpu->zero) {
        LoadA8(cpu, 0x01u);
        Write8(memory, 0x7fd0c1u, A8(cpu));
    }
    SimulateRtsFrame(memory, cpu);
    LoadA8(cpu, 0x08u);                                        /* C23E */
    TestBitsAbsolute8(memory, cpu, 0x05b5u, 1);

done:
    SetIndexWidth(cpu, 1);                                     /* C243 */
    return PlayerReturned(0x83c245u);
}
