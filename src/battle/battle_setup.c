/* Battle initialization at $81:8000. */

#include "battle/battle_internal.h"

enum {
    ENCOUNTER_SELECTOR = 0x7ff8a1u,
    ENCOUNTER_SOURCE = 0x7ff8a3u,
    ENCOUNTER_FLAGS = 0x7ff8a5u,
    BATTLE_FORMATION_MARKER = 0x11e8u,
    BATTLE_SCENARIO = 0x11e0u,
};

static uint8_t SelectBattleOpening(
    Lufia2BattleChildCalls *calls) {
    const Lufia2Memory *memory = calls->memory;
    Lufia2CpuState *cpu = calls->cpu;
    uint16_t marker = 0u;
    uint8_t control = 3u;

    TransferDirectToA(cpu);                                  /* $81:8000 */
    OpSta(memory, cpu, ENCOUNTER_FLAGS);

    if (!Lufia2BattleCallChild(calls, 0x8005u, 0x85edb2u, 3u))
        return 0;
    if (!Lufia2BattleCallChild(calls, 0x8009u, 0x85edf1u, 3u))
        return 0;

    OpLdx(cpu, 0x0f20u);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x0562u), cpu->x);

    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, 0x0b5bu)));
    OpInx(cpu);
    if (!cpu->zero)
        OpWriteX(memory, cpu, OpAbs(cpu, 0x0b5bu), cpu->x);

    OpLda(memory, cpu, ENCOUNTER_SOURCE);
    if (cpu->zero) {
        OpLda(memory, cpu, ENCOUNTER_SELECTOR);

        OpCmpValue(cpu, 0x7fu);
        if (cpu->zero) {
            marker = 0xffffu;
            control = 1u;
        } else {
            OpCmpValue(cpu, 0x80u);
            if (cpu->zero) {
                marker = 0xffffu;
                control = 2u;
            } else {
                OpCmpValue(cpu, 0x3fu);
                if (cpu->zero) {
                    LoadA8(cpu, 2u);
                    if (!Lufia2BattleCallChild(
                            calls, 0x803au, 0x808299u, 3u))
                        return 0;
                    OpCmpValue(cpu, 0u);
                    if (!cpu->zero) {
                        marker = 0xffffu;
                        control = 1u;
                    }
                } else {
                    OpCmpValue(cpu, 0xbfu);
                    if (cpu->zero) {
                        LoadA8(cpu, 2u);
                        if (!Lufia2BattleCallChild(
                                calls, 0x8051u, 0x808299u, 3u))
                            return 0;
                        OpCmpValue(cpu, 0u);
                        if (!cpu->zero) {
                            marker = 0xffffu;
                            control = 2u;
                        }
                    }
                }
            }
        }
    }

    OpLdx(cpu, marker);
    OpWriteX(memory, cpu, OpAbs(cpu, BATTLE_FORMATION_MARKER), cpu->x);
    LoadA8(cpu, control);
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_WRAM_CONTROL_FLAGS));
    return 1;
}

static void CopyPartyFormation(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, 0x0a7au));                 /* $81:8071 */
    OpSta(memory, cpu, OpAbs(cpu, 0x153cu));

    OpLdx(cpu, 3u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, 0x0a7bu));
        OpSta(memory, cpu, OpAbsX(cpu, 0x153du));
        OpDex(cpu);
    } while (!cpu->negative);

    OpLdx(cpu, 3u);
    do {
        OpRepWidths(cpu, 0x20u);
        OpTxa(cpu);
        OpAslA(cpu);
        OpTay(cpu);

        OpLda(memory, cpu, OpAbsY(cpu, 0x0a80u));
        OpSta(memory, cpu, OpAbsY(cpu, 0x0a64u));
        OpTay(cpu);

        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbsY(cpu, 0x000fu));
        OpSta(memory, cpu, OpAbsX(cpu, 0x1542u));
        TransferDirectToA(cpu);
        OpSta(memory, cpu, OpAbsY(cpu, 0x0010u));
        OpDex(cpu);
    } while (!cpu->negative);
}

static uint8_t InitializeBattleRecords(
    Lufia2BattleChildCalls *calls) {
    const Lufia2Memory *memory = calls->memory;
    Lufia2CpuState *cpu = calls->cpu;

    Lufia2BattleLoadControlFlags(memory, cpu);
    OpBitValue(cpu, BATTLE_CONTROL_MODE_1);
    if (cpu->zero &&
        !Lufia2BattleCallChild(calls, 0x80a8u, 0x859419u, 3u))
        return 0;

    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, 0x1104u));
    OpSta(memory, cpu, OpAbs(cpu, 0x10f0u));
    OpStz(memory, cpu, OpAbs(cpu, 0x160au));
    OpStz(memory, cpu, OpAbs(cpu, 0x1607u));
    OpRepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpAbs(cpu, 0x1608u));
    OpStz(memory, cpu, OpAbs(cpu, 0x1605u));
    OpStz(memory, cpu, OpAbs(cpu, 0x160bu));

    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpLongX(cpu, 0x859ec8u));
        OpSta(memory, cpu, OpAbsX(cpu, 0x0a6eu));
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, 12u);
    } while (!cpu->zero);

    OpSepWidths(cpu, 0x20u);
    if (!Lufia2BattleCallChild(calls, 0x80d8u, 0x8181e6u, 2u))
        return 0;

    OpLdx(cpu, 0u);
    OpTxy(cpu);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, 0x1345u));
        OpCmpValue(cpu, 0xffu);
        if (cpu->zero) {
            OpIncA(cpu);
            OpSta(memory, cpu, OpAbsY(cpu, 0x0a6eu));
            OpSta(memory, cpu, OpAbsY(cpu, 0x0a6fu));
        } else {
            OpSta(memory, cpu, OpAbs(cpu, 0x09f2u));
            PushY(memory, cpu);
            OpPushX(memory, cpu);
            OpLdx(cpu, OpReadX(memory, cpu, OpAbsY(cpu, 0x0a6eu)));
            OpWriteX(memory, cpu, OpAbs(cpu, 0x00b2u), cpu->x);
            if (!Lufia2BattleCallChild(calls, 0x80fau, 0x81fc0bu, 3u))
                return 0;
            OpPullX(memory, cpu);
            OpPullY(memory, cpu);
        }

        OpIny(cpu);
        OpIny(cpu);
        OpInx(cpu);
        OpCpx(cpu, 6u);
    } while (!cpu->zero);

    return 1;
}

static void ApplyScenarioLayout(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    const uint8_t slots[5],
    const uint16_t words[4],
    uint16_t first_value,
    uint16_t second_value) {
    for (uint16_t i = 0; i < 5u; ++i) {
        LoadA8(cpu, slots[i]);
        OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(0x153cu + i)));
    }

    for (uint16_t i = 0; i < 4u; ++i) {
        OpLdx(cpu, words[i]);
        OpWriteX(
            memory, cpu, OpAbs(cpu, (uint16_t)(0x0a64u + 2u * i)), cpu->x);
    }

    OpLda(memory, cpu, OpAbs(cpu, first_value));
    OpSta(memory, cpu, OpAbs(cpu, 0x1542u));
    OpLda(memory, cpu, OpAbs(cpu, second_value));
    OpSta(memory, cpu, OpAbs(cpu, 0x1543u));
}

static void ApplyBattleScenario(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint8_t scenario_two_slots[5] = {
        2u, 2u, 1u, 0xffu, 0xffu};
    static const uint16_t scenario_two_words[4] = {
        0x0d29u, 0x0c6bu, 0u, 0u};
    static const uint8_t scenario_one_slots[5] = {
        2u, 0u, 4u, 0xffu, 0xffu};
    static const uint16_t scenario_one_words[4] = {
        0x0badu, 0x0ea5u, 0u, 0u};

    OpLda(memory, cpu, OpAbs(cpu, BATTLE_SCENARIO));
    OpCmpValue(cpu, 3u);
    if (cpu->zero) {
        LoadA8(cpu, 1u);
        OpSta(memory, cpu, ENCOUNTER_FLAGS);
        return;
    }

    OpCmpValue(cpu, 2u);
    if (cpu->zero) {
        ApplyScenarioLayout(
            memory, cpu,
            scenario_two_slots, scenario_two_words,
            0x0d38u, 0x0c7au);
        return;
    }

    OpCmpValue(cpu, 1u);
    if (cpu->zero) {
        ApplyScenarioLayout(
            memory, cpu,
            scenario_one_slots, scenario_one_words,
            0x0bbcu, 0x0eb4u);
    }
}

static uint8_t FinishBattleSetup(
    Lufia2BattleChildCalls *calls) {
    const Lufia2Memory *memory = calls->memory;
    Lufia2CpuState *cpu = calls->cpu;

    if (!Lufia2BattleCallChild(calls, 0x8191u, 0x858a03u, 3u))
        return 0;

    OpLda(memory, cpu, OpAbs(cpu, 0x1144u));
    OpSta(memory, cpu, OpAbs(cpu, 0x154du));
    OpLdx(cpu, 0x10dfu);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x0a6cu), cpu->x);

    OpLda(memory, cpu, OpAbs(cpu, 0x0a7fu));
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, 0x154cu));
    if (cpu->zero) {
        OpLdx(cpu, 0u);
        OpWriteX(memory, cpu, OpAbs(cpu, 0x0a6cu), cpu->x);
        LoadA8(cpu, 4u);
        OpTestBits(memory, cpu, OpAbs(cpu, 0x10eeu), 1u);
    }

    if (!Lufia2BattleCallChild(calls, 0x81b5u, 0x85eddbu, 3u))
        return 0;
    if (!Lufia2BattleCallChild(calls, 0x81b9u, 0x81851eu, 2u))
        return 0;
    if (!Lufia2BattleCallChild(calls, 0x81bcu, 0x8591a1u, 3u))
        return 0;
    if (!Lufia2BattleCallChild(calls, 0x81c0u, 0x858a2fu, 3u))
        return 0;

    LoadA8(cpu, 0xffu);
    OpSta(memory, cpu, 0x0012f3u);
    OpLda(memory, cpu, OpDp(cpu, 0x40u));
    OpSta(memory, cpu, OpDp(cpu, 0xd4u));

    if (!Lufia2BattleCallChild(calls, 0x81ceu, 0x81c339u, 2u))
        return 0;

    for (;;) {
        OpLdx(
            cpu,
            OpReadX(
                memory, cpu, OpAbs(cpu, BATTLE_WRAM_STARTUP_COUNTER)));
        if (cpu->zero)
            return 1;

        if (!Lufia2BattleCallChild(calls, 0x81d6u, 0x81d9d0u, 2u))
            return 0;
        LoadA8(cpu, 0xffu);
        OpSta(memory, cpu, 0x0012f3u);
        if (!Lufia2BattleCallChild(calls, 0x81dfu, 0x85ec81u, 3u))
            return 0;
    }
}

Lufia2ExecutionResult Lufia2BattleSetup(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context) {
    Lufia2BattleChildCalls calls = {
        memory, cpu, child, child_context, 0x81u, 0u};

    if (!SelectBattleOpening(&calls))
        return Lufia2BattleChildUnwound(&calls);

    CopyPartyFormation(memory, cpu);

    if (!InitializeBattleRecords(&calls))
        return Lufia2BattleChildUnwound(&calls);

    ApplyBattleScenario(memory, cpu);

    if (!FinishBattleSetup(&calls))
        return Lufia2BattleChildUnwound(&calls);

    return ExecutionReturned(0x8181e5u);
}
