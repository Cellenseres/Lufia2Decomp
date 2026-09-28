#include "core/cpu_internal.h"
#include "lufia2/actor.h"
#include "actor/actor_slot_view.h"
#include "system/wram.h"

/* $83:C7F8 and $83:D508 front-ends; M=1. */

static void LoadAAbsolute(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t address) {
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, address, 0)));
}

static void LoadALong(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t address) {
    LoadA8(cpu, Read8(memory, address));
}

static void LoadADirect(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t offset) {
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, offset)));
}

static void IncrementA8(Lufia2CpuState *cpu) {
    LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
}

static void ExclusiveOr8(Lufia2CpuState *cpu, uint8_t value) {
    LoadA8(cpu, (uint8_t)(A8(cpu) ^ value));
}

Lufia2ActorPrimaryFlow Lufia2ActorPrimaryUpdateFrontend(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ActorSlotView slot;

    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);         /* $83:C7F8 */
    slot = Lufia2ActorSlotAt(memory, cpu, cpu->x);
    LoadA8(cpu, Lufia2ActorSlotState(&slot));        /* $83:C7FA */
    And8(cpu, 0x80u);                                /* $83:C7FD */
    if (!cpu->zero)                                  /* $83:C7FF */
        return LUFIA2_ACTOR_PRIMARY_RETURN;

    LoadA8(cpu, Lufia2ActorSlotReadMirrored(&slot, WRAM_UNK_7E1291));
                                                     /* $83:C801 */
    BitImmediate8(cpu, 0x07u);                       /* $83:C804 */
    if (!cpu->zero)                                  /* $83:C806 */
        return LUFIA2_ACTOR_PRIMARY_CONTINUE_C808;

    LoadAAbsolute(memory, cpu, WRAM_WINDOW_MODE);    /* $83:C80E */
    BitImmediate8(cpu, 0x01u);                       /* $83:C811 */
    if (cpu->zero)                                   /* $83:C813 */
        goto timer;

    LoadAAbsolute(memory, cpu, WRAM_TEXT_WAIT_ACTOR); /* $83:C815 */
    if (!cpu->negative)                              /* $83:C818 */
        goto timer;

    LoadADirect(memory, cpu, DP_ACTOR_SLOT);         /* $83:C81A */
    if (cpu->zero)                                   /* $83:C81C */
        return LUFIA2_ACTOR_PRIMARY_CONTINUE_C83C;

    LoadA8(cpu, Lufia2ActorSlotState(&slot));        /* $83:C81E */
    BitImmediate8(cpu, 0x08u);                       /* $83:C821 */
    if (!cpu->zero)                                  /* $83:C823 */
        goto timer;
    BitImmediate8(cpu, 0x40u);                       /* $83:C825 */
    if (cpu->zero)                                   /* $83:C827 */
        return LUFIA2_ACTOR_PRIMARY_RETURN;
    And8(cpu, 0xbfu);                                /* $83:C829 */
    Lufia2ActorSlotSetState(&slot, A8(cpu));         /* $83:C82B */

timer:
    LoadA8(cpu, Lufia2ActorSlotPrimaryTimer(&slot)); /* $83:C82E */
    if (cpu->zero)                                   /* $83:C832 */
        return LUFIA2_ACTOR_PRIMARY_CONTINUE_C83C;
    DecrementA8(cpu);                                /* $83:C834 */
    Lufia2ActorSlotSetPrimaryTimer(&slot, A8(cpu));  /* $83:C835 */
    if (cpu->zero)                                   /* $83:C839 */
        return LUFIA2_ACTOR_PRIMARY_CONTINUE_C83C;
    return LUFIA2_ACTOR_PRIMARY_RETURN;              /* $83:C83B */
}

Lufia2ActorSecondaryFlow Lufia2ActorSecondaryUpdateFrontend(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ActorSlotView slot;

    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);         /* $83:D508 */
    slot = Lufia2ActorSlotAt(memory, cpu, cpu->x);
    /* LDA $000736,X: the long form of the same array. */
    LoadA8(cpu, Lufia2ActorSlotReadLong(&slot, WRAM_UNK_7E0736));
                                                     /* $83:D50A */
    BitImmediate8(cpu, 0x80u);                       /* $83:D50E */
    if (!cpu->zero) {
        LoadA8(cpu, Lufia2ActorSlotSecondaryTimer(&slot)); /* $83:D512 */
        DecrementA8(cpu);
        Lufia2ActorSlotSetSecondaryTimer(&slot, A8(cpu));
        And8(cpu, 0x0fu);                            /* $83:D51B */
        if (cpu->zero) {
            /* Reload the period from the high nibble. */
            LoadA8(cpu, Lufia2ActorSlotSecondaryTimer(&slot)); /* $83:D51F */
            LsrA8(cpu);
            LsrA8(cpu);
            LsrA8(cpu);
            LsrA8(cpu);
            LoadA8(cpu, (uint8_t)(A8(cpu) |
                Lufia2ActorSlotSecondaryTimer(&slot)));
            Lufia2ActorSlotSetSecondaryTimer(&slot, A8(cpu));
            LoadA8(cpu, Lufia2ActorSlotReadLong(&slot, WRAM_UNK_7FE316));
                                                     /* $83:D52F */
            ExclusiveOr8(cpu, 0x80u);
            Lufia2ActorSlotWriteLong(&slot, WRAM_UNK_7FE316, A8(cpu));
        }
    }

    LoadA8(cpu, Lufia2ActorSlotState(&slot));        /* $83:D539 */
    And8(cpu, 0x80u);                                /* $83:D53C */
    if (!cpu->zero)                                  /* $83:D53E */
        return LUFIA2_ACTOR_SECONDARY_CONTINUE_D59A;

    LoadAAbsolute(memory, cpu, WRAM_WINDOW_MODE);    /* $83:D540 */
    BitImmediate8(cpu, 0x01u);                       /* $83:D543 */
    if (cpu->zero)                                   /* $83:D545 */
        goto final_gate;

    LoadA8(cpu, (uint8_t)cpu->x);                    /* $83:D547 */
    if (!cpu->zero)                                  /* $83:D548 */
        goto state_gate;

    LoadALong(memory, cpu, WRAM_UNK_7FD0FE);         /* $83:D54A */
    if (cpu->zero)                                   /* $83:D54E */
        goto final_gate;
    goto walk_counter;                               /* $83:D550 */

state_gate:
    LoadA8(cpu, Lufia2ActorSlotReadMirrored(&slot, WRAM_UNK_7E0736));
                                                     /* $83:D552 */
    BitImmediate8(cpu, 0x42u);                       /* $83:D555 */
    if (!cpu->zero)                                  /* $83:D557 */
        goto final_gate;

    LoadA8(cpu, Lufia2ActorSlotReadMirrored(&slot, WRAM_UNK_7E05D2));
                                                     /* $83:D559 */
    Compare8(cpu, A8(cpu), 0x80u);                   /* $83:D55C */
    if (!cpu->carry)                                 /* $83:D55E */
        goto final_gate;
    Compare8(cpu, A8(cpu), 0xa4u);                   /* $83:D560 */
    if (cpu->zero)                                   /* $83:D562 */
        goto final_gate;

walk_counter:
    LoadA8(cpu, Lufia2ActorSlotWalkCounter(&slot));  /* $83:D564 */
    IncrementA8(cpu);                                /* $83:D568 */
    Lufia2ActorSlotSetWalkCounter(&slot, A8(cpu));   /* $83:D569 */
    Compare8(cpu, A8(cpu), 0x20u);                   /* $83:D56D */
    if (!cpu->zero)                                  /* $83:D56F */
        goto final_gate;

    TransferDirectToA(cpu);                          /* $83:D571 */
    Lufia2ActorSlotSetWalkCounter(&slot, A8(cpu));   /* $83:D572 */
    LoadA8(cpu, Lufia2ActorSlotReadMirrored(&slot, WRAM_UNK_7E066A));
                                                     /* $83:D576 */
    ExclusiveOr8(cpu, 0x01u);                        /* $83:D579 */
    Lufia2ActorSlotWriteMirrored(&slot, WRAM_UNK_7E066A, A8(cpu));
                                                     /* $83:D57B */

final_gate:
    LoadA8(cpu, Lufia2ActorSlotReadMirrored(&slot, WRAM_UNK_7E0736));
                                                     /* $83:D57E */
    BitImmediate8(cpu, 0x04u);                       /* $83:D581 */
    if (!cpu->zero) {
        LoadA8(cpu, Lufia2ActorSlotWalkCounter(&slot)); /* $83:D585 */
        BitImmediate8(cpu, 0x02u);
        if (!cpu->zero) {
            /* The fine position is word-indexed through $A9. */
            LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET); /* $83:D58D */
            LoadA8(cpu, Read8(memory,
                LongIndexedAddress(WRAM_ACTOR_FINE_X, cpu->x)));
            ExclusiveOr8(cpu, 0x01u);
            Write8(memory, LongIndexedAddress(WRAM_ACTOR_FINE_X, cpu->x),
                A8(cpu));
        }
    }
    return LUFIA2_ACTOR_SECONDARY_RETURN;            /* $83:D599 */
}
