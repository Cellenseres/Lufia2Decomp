/* Field encounter handoff at $83:83E0. */

#include "core/cpu_ops.h"
#include "lufia2/field.h"

Lufia2ExecutionResult Lufia2FieldEncounterHandoff(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context) {
    Lufia2ExecutionResult result;

    LoadA8(cpu, 0xffu);                                        /* 83E0 */
    OpSta(memory, cpu, 0x7ff8a3u);                             /* 83E2 */
    SimulateJslFrame(memory, cpu, 0x83u, 0x83e9u);             /* 83E6 */
    if (!child(child_context, cpu, 0x8383ebu, 0x8383e6u, 3u)) {
        result = ExecutionReturned(0x8383e6u);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    return ExecutionReturned(0x8383eau);                       /* RTS */
}

static Lufia2ExecutionResult EncounterChildUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);

    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static uint8_t EncounterCallLong(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context,
    uint32_t site,
    uint32_t target) {
    SimulateJslFrame(memory, cpu, 0x83u, (uint16_t)(site + 3u));
    return child(child_context, cpu, target, site, 3u);
}

/* $83:845B: visual transition, Battle call, then Field-side continuation. */
Lufia2ExecutionResult Lufia2EncounterBattleSequence(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context) {
    uint8_t selected;

    PushAccumulator8(memory, cpu);                              /* 845B PHA */
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));                        /* 845F PHP */
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    Push8(memory, cpu, cpu->program_bank);                    /* 8464 PHK */
    PullDataBank(memory, cpu);
    if (!EncounterCallLong(memory, cpu, child, child_context,
                           0x838466u, 0x848bc7u))
        return EncounterChildUnwound(0x838466u);
    OpRepWidths(cpu, 0x10u);                                  /* 846A */
    LoadA8(cpu, 0x01u);
    OpTestBits(memory, cpu, OpAbs(cpu, 0x09a9u), 0);          /* 846E TRB */
    OpStz(memory, cpu, OpDp(cpu, 0x6au));                     /* 8471 */
    PushDataBank(memory, cpu);
    if (!EncounterCallLong(memory, cpu, child, child_context,
                           0x838474u, 0x818821u))
        return EncounterChildUnwound(0x838474u);
    PullDataBank(memory, cpu);

    OpLda(memory, cpu, 0x7ff8a3u);                           /* 8479 */
    OpCmpValue(cpu, 0xffu);
    selected = !cpu->zero;
    if (selected) {
        OpLda(memory, cpu, 0x7ff8a2u);
        OpCmpValue(cpu, 0x01u);
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, 0x099du));
            if (!EncounterCallLong(memory, cpu, child, child_context,
                                   0x83848cu, 0x8093feu))
                return EncounterChildUnwound(0x83848cu);
        }
    }
    OpLda(memory, cpu, 0x7ff8a3u);                           /* 8490 */
    OpCmpValue(cpu, 0xffu);
    if (!cpu->zero) {
        OpLda(memory, cpu, 0x7ff8a4u);
        cpu->carry = 0;
        OpAdcValue(cpu, 0x50u);
        if (!EncounterCallLong(memory, cpu, child, child_context,
                               0x83849fu, 0x80bf92u))
            return EncounterChildUnwound(0x83849fu);
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 0xa7u)));
        OpLda(memory, cpu, 0x7ff8a2u);
        OpCmpValue(cpu, 0x02u);
        if (cpu->zero) {
            if (!EncounterCallLong(memory, cpu, child, child_context,
                                   0x8384b3u, 0x8ebae7u))
                return EncounterChildUnwound(0x8384b3u);
        } else if (!EncounterCallLong(memory, cpu, child, child_context,
                                      0x8384adu, 0x8ebb17u)) {
            return EncounterChildUnwound(0x8384adu);
        }
    } else {
        LoadA8(cpu, 0xc0u);                                  /* 84B9 */
        OpTestBits(memory, cpu, OpAbs(cpu, 0x079du), 0);
        OpLda(memory, cpu, 0x7ff8a2u);
        if (!cpu->zero) {
            LoadA8(cpu, 0x80u);
            OpTestBits(memory, cpu, OpAbs(cpu, 0x079du), 1);
        }
        OpLda(memory, cpu, 0x7ff8a5u);
        if (cpu->zero) {
            LoadA8(cpu, 0x40u);
            OpTestBits(memory, cpu, OpAbs(cpu, 0x079du), 1);
        }
    }
    OpStz(memory, cpu, OpAbs(cpu, 0x05b3u));                 /* 84D4 */
    if (!EncounterCallLong(memory, cpu, child, child_context,
                           0x8384d7u, 0x8385dcu))
        return EncounterChildUnwound(0x8384d7u);
    UnpackStatus(cpu, Pull8(memory, cpu));                   /* 84DB PLP */
    PullDataBank(memory, cpu);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    LoadA8(cpu, Pull8(memory, cpu));                         /* 84DF PLA */
    return ExecutionReturned(0x8384e0u);                    /* RTL */
}
