/* Field encounter handoff and Battle round-trip. */

#include "core/cpu_ops.h"
#include "lufia2/field.h"

enum {
    ENCOUNTER_SELECTOR = 0x7ff8a1u,
    ENCOUNTER_TYPE = 0x7ff8a2u,
    ENCOUNTER_SOURCE = 0x7ff8a3u,
    ENCOUNTER_VALUE = 0x7ff8a4u,
    ENCOUNTER_FLAGS = 0x7ff8a5u,
    FIELD_POST_BATTLE_FLAGS = 0x079du,
    FIELD_RELOAD_REQUEST = 0x05b3u,
};

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

Lufia2ExecutionResult Lufia2FieldEncounterHandoff(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu,
                                                  Lufia2PushedChildCall child,
                                                  void *child_context) {
    LoadA8(cpu, 0xffu); /* $83:83E0 */
    OpSta(memory, cpu, ENCOUNTER_SOURCE);
    SimulateJslFrame(memory, cpu, 0x83u, 0x83e9u);

    if (!child(child_context, cpu, 0x8383ebu, 0x8383e6u, 3u))
        return EncounterChildUnwound(0x8383e6u);

    return ExecutionReturned(0x8383eau);
}

static uint8_t ResumeFieldAfterBattle(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                      Lufia2PushedChildCall child, void *child_context,
                                      uint32_t *unwind_site) {
    OpLda(memory, cpu, ENCOUNTER_SOURCE); /* $83:8479 */
    OpCmpValue(cpu, 0xffu);
    if (!cpu->zero) {
        OpLda(memory, cpu, ENCOUNTER_TYPE);
        OpCmpValue(cpu, 1u);
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, 0x099du));
            if (!EncounterCallLong(memory, cpu, child, child_context, 0x83848cu,
                                   0x8093feu)) {
                *unwind_site = 0x83848cu;
                return 0;
            }
        }
    }

    OpLda(memory, cpu, ENCOUNTER_SOURCE);
    OpCmpValue(cpu, 0xffu);
    if (!cpu->zero) {
        OpLda(memory, cpu, ENCOUNTER_VALUE);
        cpu->carry = 0;
        OpAdcValue(cpu, 0x50u);
        if (!EncounterCallLong(memory, cpu, child, child_context, 0x83849fu,
                               0x80bf92u)) {
            *unwind_site = 0x83849fu;
            return 0;
        }

        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 0xa7u)));
        OpLda(memory, cpu, ENCOUNTER_TYPE);
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            if (!EncounterCallLong(memory, cpu, child, child_context, 0x8384b3u,
                                   0x8ebae7u)) {
                *unwind_site = 0x8384b3u;
                return 0;
            }
        } else if (!EncounterCallLong(memory, cpu, child, child_context, 0x8384adu,
                                      0x8ebb17u)) {
            *unwind_site = 0x8384adu;
            return 0;
        }
    } else {
        LoadA8(cpu, 0xc0u);
        OpTestBits(memory, cpu, OpAbs(cpu, FIELD_POST_BATTLE_FLAGS), 0u);

        OpLda(memory, cpu, ENCOUNTER_TYPE);
        if (!cpu->zero) {
            LoadA8(cpu, 0x80u);
            OpTestBits(memory, cpu, OpAbs(cpu, FIELD_POST_BATTLE_FLAGS), 1u);
        }

        OpLda(memory, cpu, ENCOUNTER_FLAGS);
        if (cpu->zero) {
            LoadA8(cpu, 0x40u);
            OpTestBits(memory, cpu, OpAbs(cpu, FIELD_POST_BATTLE_FLAGS), 1u);
        }
    }

    OpStz(memory, cpu, OpAbs(cpu, FIELD_RELOAD_REQUEST));
    if (!EncounterCallLong(memory, cpu, child, child_context, 0x8384d7u, 0x8385dcu)) {
        *unwind_site = 0x8384d7u;
        return 0;
    }

    return 1;
}

Lufia2ExecutionResult Lufia2EncounterBattleSequence(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu,
                                                    Lufia2PushedChildCall child,
                                                    void *child_context) {
    uint32_t unwind_site = 0;

    PushAccumulator8(memory, cpu); /* $83:845B */
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);

    if (!EncounterCallLong(memory, cpu, child, child_context, 0x838466u, 0x848bc7u))
        return EncounterChildUnwound(0x838466u);

    OpRepWidths(cpu, 0x10u);
    LoadA8(cpu, 0x01u);
    OpTestBits(memory, cpu, OpAbs(cpu, 0x09a9u), 0u);
    OpStz(memory, cpu, OpDp(cpu, 0x6au));

    PushDataBank(memory, cpu);
    if (!EncounterCallLong(memory, cpu, child, child_context, 0x838474u, 0x818821u))
        return EncounterChildUnwound(0x838474u);
    PullDataBank(memory, cpu);

    if (!ResumeFieldAfterBattle(memory, cpu, child, child_context, &unwind_site))
        return EncounterChildUnwound(unwind_site);

    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    LoadA8(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x8384e0u);
}
