#include "battle/battle_internal.h"

static void TargetClearName(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdy(cpu, 0x7fu);
    TransferDirectToA(cpu);
    do {
        OpSta(memory, cpu, OpAbsY(cpu, 0x3800u));
        OpDey(cpu);
    } while (!cpu->negative);
}

static void TargetIndex(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, 0x26u));
    OpAslA(cpu);
    OpAslA(cpu);
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
}

static void TargetFirstEnemy(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    /* The original checks the first five records and falls back to slot five. */
    for (uint16_t address = 0x1bcau; address <= 0x1bdau; address += 4u) {
        OpLda(memory, cpu, OpAbs(cpu, address));
        if (cpu->zero)
            break;
        OpStepMem(memory, cpu, OpDp(cpu, 0x26u), 1);
    }
}

static void TargetClearParty(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    for (uint16_t address = 0x1bb3u; address <= 0x1bc3u; address += 4u)
        OpStz(memory, cpu, OpAbs(cpu, address));
}

static void TargetClearEnemies(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    for (uint16_t address = 0x1bcbu; address <= 0x1bdfu; address += 4u)
        OpStz(memory, cpu, OpAbs(cpu, address));
}

static void TargetPublish(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                          uint16_t base, uint16_t last) {
    OpStz(memory, cpu, OpDp(cpu, 0x27u));
    OpLdx(cpu, last);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, base));
        bool selected = false;
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbsX(cpu, (uint16_t)(base + 1u)));
            selected = !cpu->zero;
        }
        /* SEC/ROL for a selected valid record; ASL otherwise. */
        cpu->carry = selected;
        OpRolMem8(memory, cpu, OpDp(cpu, 0x27u));
        OpDex(cpu);
        OpDex(cpu);
        OpDex(cpu);
        OpDex(cpu);
    } while (!cpu->negative);
    OpStz(memory, cpu, OpAbs(cpu, 0x15d7u));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, 0x0012f3u);
}

Lufia2ExecutionResult Lufia2BattleChooseTargets(const Lufia2Memory *memory,
                                                Lufia2CpuState *cpu,
                                                Lufia2PushedChildCall child,
                                                void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);
    PushAccumulator8(memory, cpu);
    if (!BattleCall(&battle, 0xd4e1u, 0x85ec81u, 3u))
        goto unwound;
    LoadA8(cpu, Pull8(memory, cpu));
    OpPushX(memory, cpu);
    OpSta(memory, cpu, OpDp(cpu, 0x24u));
    PushDataBank(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0u);
    OpLoadA(cpu, 12u);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, 0x1bb2u));
        OpInx(cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpDecA(cpu);
    } while (!cpu->zero);
    OpLdx(cpu, 0u);
    OpTxy(cpu);
    do {
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpLongX(cpu, 0x97b5c9u));
        OpSta(memory, cpu, OpAbsY(cpu, 0x1bb0u));
        OpLda(memory, cpu, OpAbsX(cpu, 0x0a64u));
        if (cpu->zero) {
            OpLoadA(cpu, 0xffu);
            OpSta(memory, cpu, OpAbsY(cpu, 0x1bb2u));
        }
        OpSepWidths(cpu, 0x20u);
        OpIny(cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, 8u);
    } while (!cpu->zero);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, 0x1bc2u));
    TransferDirectToA(cpu);
    OpLdx(cpu, 0u);
    OpTxy(cpu);
    do {
        PushAccumulator8(memory, cpu);
        OpPushX(memory, cpu);
        PushY(memory, cpu);
        if (!BattleCall(&battle, 0xd52eu, 0x81b8b1u, 2u))
            goto unwound;
        OpPullY(memory, cpu);
        OpPullX(memory, cpu);
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
        OpSta(memory, cpu, OpAbsX(cpu, 0x1bcau));
        OpLda(memory, cpu, OpDp(cpu, 0x22u));
        OpSta(memory, cpu, OpAbsX(cpu, 0x1bc8u));
        OpLda(memory, cpu, OpDp(cpu, 0x23u));
        OpSta(memory, cpu, OpAbsX(cpu, 0x1bc9u));
        LoadA8(cpu, Pull8(memory, cpu));
        OpIny(cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpIncA(cpu);
        OpCmpValue(cpu, 6u);
    } while (!cpu->zero);
    OpLda(memory, cpu, OpDp(cpu, 0x24u));
    OpAndValue(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, 0x27u));
    OpStz(memory, cpu, OpDp(cpu, 0x26u));
    OpLda(memory, cpu, OpDp(cpu, 0x27u));
    if (cpu->negative)
        TargetFirstEnemy(memory, cpu);
    OpStz(memory, cpu, OpDp(cpu, 0x28u));
    OpLda(memory, cpu, OpAbs(cpu, 0x129eu));
    if (!BattleCall(&battle, 0xd581u, 0x81bebcu, 3u))
        goto unwound;
    OpSetDataBank(memory, cpu, 0x7eu);
redraw:
    TargetClearName(memory, cpu);
    OpLdx(cpu, 0u);
    OpStz(memory, cpu, OpAbs(cpu, 0x15dau));
    OpLda(memory, cpu, OpDp(cpu, 0x26u));
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    OpAslA(cpu);
    OpAslA(cpu);
    OpTay(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, 0x27u));
    if (cpu->zero) {
        OpStz(memory, cpu, OpAbs(cpu, 0x4201u));
        OpStz(memory, cpu, OpAbs(cpu, 0x4203u));
        if (!BattleCall(&battle, 0xd5afu, 0x81d92cu, 2u))
            goto unwound;
    } else {
        if (!BattleCall(&battle, 0xd5b4u, 0x81d920u, 2u))
            goto unwound;
        OpPushX(memory, cpu);
        OpLda(memory, cpu, OpDp(cpu, 0x26u));
        OpAslA(cpu);
        OpTax(cpu);
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpLongX(cpu, 0x859ec8u));
        OpTay(cpu);
        OpLda(memory, cpu, OpAbs(cpu, 0x4abeu));
        OpAndValue(cpu, 0xffu);
        OpLsrA(cpu);
        OpLsrA(cpu);
        OpCmpValue(cpu, 38u);
        if (cpu->carry)
            OpLoadA(cpu, 36u);
        cpu->carry = false;
        OpAdcValue(cpu, 0x3840u);
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, 0x7eu);
        OpSta(memory, cpu, OpDp(cpu, 0x5fu));
        OpLoadA(cpu, 0x20u);
        OpSta(memory, cpu, OpAbs(cpu, 0x0564u));
        if (!BattleCall(&battle, 0xd5e3u, 0x808878u, 3u))
            goto unwound;
        OpLda(memory, cpu, OpAbs(cpu, 0x4abeu));
        OpCmpValue(cpu, 0x98u);
        if (cpu->carry)
            OpLoadA(cpu, 0x90u);
        OpDecA(cpu);
        OpSta(memory, cpu, OpAbs(cpu, 0x4200u));
        OpDecA(cpu);
        OpSta(memory, cpu, OpAbs(cpu, 0x4202u));
        cpu->carry = false;
        OpAdcValue(cpu, 0x68u);
        OpSta(memory, cpu, OpAbs(cpu, 0x4201u));
        OpIncA(cpu);
        OpSta(memory, cpu, OpAbs(cpu, 0x4203u));
        OpPullX(memory, cpu);
    }
    OpLda(memory, cpu, OpDp(cpu, 0x28u));
    if (cpu->zero) {
        OpLdy(cpu, 0u);
        OpLoadA(cpu, 12u);
        OpSta(memory, cpu, OpDp(cpu, 0x25u));
        do {
            OpLda(memory, cpu, OpAbsY(cpu, 0x1bb2u));
            if (cpu->zero) {
                OpLda(memory, cpu, OpAbsY(cpu, 0x1bb3u));
                if (!cpu->zero && !BattleCall(&battle, 0xd618u, 0x81d948u, 2u))
                    goto unwound;
            }
            OpIny(cpu);
            OpIny(cpu);
            OpIny(cpu);
            OpIny(cpu);
            OpStepMem(memory, cpu, OpDp(cpu, 0x25u), -1);
        } while (!cpu->zero);
    }
    OpLda(memory, cpu, OpDp(cpu, 0x28u));
    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
    OpSta(memory, cpu, OpDp(cpu, 0x28u));
    OpLda(memory, cpu, OpAbs(cpu, 0x15dau));
    OpSta(memory, cpu, OpAbs(cpu, 0x15d7u));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, 0x0012f3u);
    if (!BattleCall(&battle, 0xd635u, 0x81d9d0u, 2u))
        goto unwound;
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, 0x0012f3u);
    OpRepWidths(cpu, 0x20u);
    if (!BattleCall(&battle, 0xd640u, 0x859cc0u, 3u) ||
        !BattleCall(&battle, 0xd644u, 0x859c64u, 3u))
        goto unwound;
    OpSepWidths(cpu, 0x20u);
    if (!BattleCall(&battle, 0xd64au, 0x85ec81u, 3u))
        goto unwound;
    OpLda(memory, cpu, OpDp(cpu, 0xddu));
    OpBitValue(cpu, 0xa0u);
    if (!cpu->zero)
        goto accept;
    OpBitValue(cpu, 0x30u);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, 0x24u));
        OpAndValue(cpu, 3u);
        OpCmpValue(cpu, 1u);
        if (cpu->zero)
            goto select_all;
    }
    OpLda(memory, cpu, OpDp(cpu, 0xdeu));
    if (cpu->negative)
        goto cancel;
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 15u);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, 0x97b5aau));
    if (cpu->zero)
        goto redraw;
    OpBitValue(cpu, 1u);
    if (cpu->zero)
        goto vertical;
    OpSta(memory, cpu, OpDp(cpu, 0x29u));
horizontal:
    OpLda(memory, cpu, OpDp(cpu, 0x26u));
    cpu->carry = false;
    OpAdc(memory, cpu, OpDp(cpu, 0x29u));
    if (cpu->negative) {
        OpLda(memory, cpu, OpDp(cpu, 0x27u));
        cpu->carry = false;
        RolA8(cpu);
        RolA8(cpu);
        OpAdcValue(cpu, 4u);
    }
    PushAccumulator8(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, 0x27u));
    if (cpu->zero) {
        LoadA8(cpu, Pull8(memory, cpu));
        OpCmpValue(cpu, 5u);
    } else {
        LoadA8(cpu, Pull8(memory, cpu));
        OpCmpValue(cpu, 6u);
    }
    if (cpu->zero)
        OpLoadA(cpu, 0u);
validate:
    OpSta(memory, cpu, OpDp(cpu, 0x26u));
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpDp(cpu, 0x27u));
    if (cpu->zero) {
        ExchangeAccumulatorBytes(cpu);
        OpRepWidths(cpu, 0x20u);
        OpAndValue(cpu, 0xffu);
        OpAslA(cpu);
        OpAslA(cpu);
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbsX(cpu, 0x1bb2u));
    } else {
        ExchangeAccumulatorBytes(cpu);
        OpRepWidths(cpu, 0x20u);
        OpAndValue(cpu, 0xffu);
        OpAslA(cpu);
        OpAslA(cpu);
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbsX(cpu, 0x1bcau));
    }
    if (!cpu->zero)
        goto horizontal;
    goto redraw;
select_all:
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpDp(cpu, 0x26u));
    OpAslA(cpu);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpDp(cpu, 0x27u));
    if (cpu->zero) {
        OpLda(memory, cpu, OpAbsX(cpu, 0x1bb3u));
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
        for (uint16_t address = 0x1bb3u; address <= 0x1bc3u; address += 4u)
            OpSta(memory, cpu, OpAbs(cpu, address));
    } else {
        OpLda(memory, cpu, OpAbsX(cpu, 0x1bcbu));
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
        for (uint16_t address = 0x1bcbu; address <= 0x1bdfu; address += 4u)
            OpSta(memory, cpu, OpAbs(cpu, address));
    }
    goto redraw;
vertical:
    OpSta(memory, cpu, OpDp(cpu, 0x29u));
    OpLda(memory, cpu, OpDp(cpu, 0x27u));
    if (!cpu->zero)
        goto enemy_vertical;
    OpLda(memory, cpu, OpDp(cpu, 0x29u));
    OpBitValue(cpu, 0x80u);
    if (!cpu->zero)
        goto party_up;
    TransferDirectToA(cpu);
party_down:
    OpLda(memory, cpu, OpDp(cpu, 0x26u));
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x97b5bau));
    if (cpu->negative)
        goto switch_enemy;
    OpSta(memory, cpu, OpDp(cpu, 0x26u));
    OpAslA(cpu);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, 0x1bb2u));
    if (!cpu->zero)
        goto party_down;
    goto redraw;
party_up:
    TransferDirectToA(cpu);
party_up_next:
    OpLda(memory, cpu, OpDp(cpu, 0x26u));
    OpTax(cpu);
    OpLda(memory, cpu, OpDp(cpu, 0x24u));
    OpAndValue(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, cpu->zero ? 0x97b5bfu : 0x97b5c4u));
    if (cpu->negative)
        goto switch_enemy;
    OpSta(memory, cpu, OpDp(cpu, 0x26u));
    OpAslA(cpu);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, 0x1bb2u));
    if (!cpu->zero)
        goto party_up_next;
    goto redraw;
switch_enemy:
    TargetClearParty(memory, cpu);
    OpStz(memory, cpu, OpDp(cpu, 0x26u));
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, 0x27u));
    TargetFirstEnemy(memory, cpu);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpDp(cpu, 0x29u));
    OpDecA(cpu);
    goto validate;
enemy_vertical:
    OpLda(memory, cpu, OpDp(cpu, 0x29u));
    OpBitValue(cpu, 0x80u);
    if (!cpu->zero)
        goto redraw;
    TargetClearEnemies(memory, cpu);
    OpStz(memory, cpu, OpDp(cpu, 0x26u));
    OpStz(memory, cpu, OpDp(cpu, 0x27u));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpDp(cpu, 0x29u));
    OpDecA(cpu);
    goto validate;
accept:
    OpLoadA(cpu, 2u);
    if (!BattleCall(&battle, 0xd7bdu, 0x80953bu, 3u))
        goto unwound;
    TargetClearName(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    if (!BattleCall(&battle, 0xd7cdu, 0x859cc0u, 3u))
        goto unwound;
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, 0x24u));
    OpAndValue(cpu, 3u);
    OpCmpValue(cpu, 2u);
    if (cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, 0x27u));
        if (!cpu->zero) {
            TargetIndex(memory, cpu);
            OpLoadA(cpu, 0xffu);
            OpSta(memory, cpu, OpAbsX(cpu, 0x1bcbu));
            goto publish_enemy;
        }
        TargetIndex(memory, cpu);
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpAbsX(cpu, 0x1bb3u));
        goto publish_party;
    }
    OpCmpValue(cpu, 3u);
    if (cpu->zero) {
        TargetIndex(memory, cpu);
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpAbsX(cpu, 0x1bb3u));
        TransferDirectToA(cpu);
        OpLdx(cpu, 12u);
        do {
            cpu->carry = false;
            OpAdc(memory, cpu, OpAbsX(cpu, 0x1bb3u));
            OpDex(cpu);
            OpDex(cpu);
            OpDex(cpu);
            OpDex(cpu);
        } while (!cpu->negative);
        OpCmpValue(cpu, 0xfeu);
        if (!cpu->zero)
            goto redraw;
        goto publish_party;
    }
    OpLda(memory, cpu, OpDp(cpu, 0x27u));
    if (!cpu->zero)
        goto enemy_accept;
    TargetIndex(memory, cpu);
    OpLda(memory, cpu, OpAbsX(cpu, 0x1bb3u));
    if (!cpu->zero)
        goto publish_party;
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbsX(cpu, 0x1bb3u));
    goto redraw;
publish_party:
    TargetPublish(memory, cpu, 0x1bb2u, 16u);
    OpLda(memory, cpu, OpDp(cpu, 0x27u));
    PullDataBank(memory, cpu);
    OpPullX(memory, cpu);
    return ExecutionReturned(0x81d876u);
enemy_accept:
    TargetIndex(memory, cpu);
    OpLda(memory, cpu, OpAbsX(cpu, 0x1bcbu));
    if (!cpu->zero)
        goto publish_enemy;
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbsX(cpu, 0x1bcbu));
    goto redraw;
publish_enemy:
    TargetPublish(memory, cpu, 0x1bcau, 20u);
    OpStz(memory, cpu, OpAbs(cpu, 0x4201u));
    OpStz(memory, cpu, OpAbs(cpu, 0x4203u));
    OpLda(memory, cpu, OpDp(cpu, 0x27u));
    OpOraValue(cpu, 0x80u);
    PullDataBank(memory, cpu);
    OpPullX(memory, cpu);
    return ExecutionReturned(0x81d8c1u);
cancel:
    OpLoadA(cpu, 1u);
    if (!BattleCall(&battle, 0xd8c4u, 0x80953bu, 3u))
        goto unwound;
    TargetClearName(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    if (!BattleCall(&battle, 0xd8d4u, 0x859cc0u, 3u))
        goto unwound;
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, 0x27u));
    if (cpu->zero) {
        TargetIndex(memory, cpu);
        OpLda(memory, cpu, OpAbsX(cpu, 0x1bb3u));
        if (!cpu->zero) {
            OpStz(memory, cpu, OpAbsX(cpu, 0x1bb3u));
            goto redraw;
        }
    } else {
        TargetIndex(memory, cpu);
        OpLda(memory, cpu, OpAbsX(cpu, 0x1bcbu));
        if (!cpu->zero) {
            OpStz(memory, cpu, OpAbsX(cpu, 0x1bcbu));
            goto redraw;
        }
    }
    OpStz(memory, cpu, OpAbs(cpu, 0x15d7u));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, 0x0012f3u);
    OpStz(memory, cpu, OpAbs(cpu, 0x4201u));
    OpStz(memory, cpu, OpAbs(cpu, 0x4203u));
    OpLoadA(cpu, 0xffu);
    PullDataBank(memory, cpu);
    OpPullX(memory, cpu);
    return ExecutionReturned(0x81d908u);
unwound:
    return BattleChildUnwound(&battle);
}
