#include "battle/battle_internal.h"

static void StartCommandList(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    unsigned offset;

    OpLoadA(cpu, 0xffu);
    for (offset = 0x10u; offset <= 0xb0u; offset += 0x10u)
        OpSta(memory, cpu, 0x7edf00u + offset);
    OpLdx(cpu, 0xdf00u);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x2181u), cpu->x);
    OpStz(memory, cpu, OpAbs(cpu, 0x2183u));
}

static void CommandListPadding(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                               uint16_t count) {
    OpLdy(cpu, count);
    OpLoadA(cpu, 0xffu);
    do {
        OpSta(memory, cpu, OpAbs(cpu, 0x2180u));
        OpDey(cpu);
    } while (!cpu->zero);
}

static void CommandListName(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                            uint16_t length) {
    OpLdy(cpu, 0u);
    do {
        OpLda(memory, cpu, OpAbsY(cpu, WRAM_RECORD_BUFFER));
        OpSta(memory, cpu, OpAbs(cpu, 0x2180u));
        OpIny(cpu);
        Compare16(cpu, cpu->y, length);
    } while (!cpu->zero);
}

/* $4214/$4216 are read in place: the divider remains bus-visible. */
static void CommandListUnits(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, 0x4216u));
    cpu->carry = 0;
    OpAdcValue(cpu, 0x30u);
    OpSta(memory, cpu, OpAbs(cpu, 0x2180u));
}

/* $81:BF3F: inventory entries for battle commands. */
Lufia2ExecutionResult Lufia2BattleItemCommands(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu,
                                               Lufia2PushedChildCall child,
                                               void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);

    StartCommandList(memory, cpu);
    OpLdx(cpu, 0u);
    OpStz(memory, cpu, OpDp(cpu, 0x0du));
    do {
        OpLda(memory, cpu, OpAbsX(cpu, 0x0a8eu));
        OpBitValue(cpu, 0xfeu);
        if (cpu->zero) {
            CommandListPadding(memory, cpu, 0x20u);
        } else {
            OpSta(memory, cpu, OpAbs(cpu, 0x0a07u));
            OpLsrA(cpu);
            OpSta(memory, cpu, OpAbs(cpu, 0x4204u));
            OpStz(memory, cpu, OpAbs(cpu, 0x4205u));
            OpLoadA(cpu, 10u);
            OpSta(memory, cpu, OpAbs(cpu, 0x4206u));
            OpLda(memory, cpu, OpAbsX(cpu, 0x0a8du));
            OpSta(memory, cpu, OpAbs(cpu, 0x0a06u));
            OpLda(memory, cpu, OpAbsX(cpu, 0x0a8eu));
            OpAndValue(cpu, 1u);
            OpSta(memory, cpu, OpAbs(cpu, 0x0a07u));
            OpPushX(memory, cpu);
            if (!BattleCall(&battle, 0xbfa3u, 0x81f1c5u, 3u))
                return BattleChildUnwound(&battle);
            OpPullX(memory, cpu);
            OpLda(memory, cpu, OpAbs(cpu, 0x0b84u));
            OpAndValue(cpu, 0x80u);
            if (!cpu->zero)
                TransferDirectToA(cpu);
            else
                OpLoadA(cpu, 1u);
            OpSta(memory, cpu, OpAbs(cpu, 0x2180u));
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpAbs(cpu, 0x0a06u));
            OpSepWidths(cpu, 0x20u);
            OpSta(memory, cpu, OpAbs(cpu, 0x2180u));
            ExchangeAccumulatorBytes(cpu);
            OpSta(memory, cpu, OpAbs(cpu, 0x2180u));
            OpLda(memory, cpu, OpAbs(cpu, 0x0b87u));
            OpSta(memory, cpu, OpAbs(cpu, 0x2180u));
            CommandListName(memory, cpu, 12u);
            OpLoadA(cpu, 0x3au);
            OpSta(memory, cpu, OpAbs(cpu, 0x2180u));
            OpLda(memory, cpu, OpAbs(cpu, 0x4214u));
            if (!cpu->zero) {
                cpu->carry = 0;
                OpAdcValue(cpu, 0x30u);
                OpSta(memory, cpu, OpAbs(cpu, 0x2180u));
            } else {
                OpStz(memory, cpu, OpAbs(cpu, 0x2180u));
            }
            CommandListUnits(memory, cpu);
            OpLda(memory, cpu, OpAbs(cpu, 0x0b86u));
            OpSta(memory, cpu, OpAbs(cpu, 0x2180u));
            CommandListPadding(memory, cpu, 12u);
        }
        OpStepMem(memory, cpu, OpDp(cpu, 0x0du), 1);
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, 0xc0u);
    } while (!cpu->zero);
    OpLda(memory, cpu, OpDp(cpu, 0x0du));
    OpBitValue(cpu, 1u);
    if (!cpu->zero) {
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpAbs(cpu, 0x2180u));
        OpLda(memory, cpu, OpDp(cpu, 0x0du));
    }
    OpTxa(cpu);
    return ExecutionReturned(0x81c030u);
}

/* $81:C031: spell entries for battle commands. */
Lufia2ExecutionResult Lufia2BattleSpellCommands(const Lufia2Memory *memory,
                                                Lufia2CpuState *cpu,
                                                Lufia2PushedChildCall child,
                                                void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);

    StartCommandList(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, WRAM_BATTLE_PARTY_SLOT);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_PARTY_RECORDS));
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, 0x13u));
    OpCmpValue(cpu, 0xffu);
    if (cpu->carry)
        OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, 0xcau));
    OpSepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpDp(cpu, 0x0du));
    do {
        OpLda(memory, cpu, OpAbsX(cpu, 0x96u));
        OpCmpValue(cpu, 0xffu);
        if (cpu->zero) {
            CommandListPadding(memory, cpu, 16u);
        } else {
            OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_RECORD_ID));
            OpPushX(memory, cpu);
            if (!BattleCall(&battle, 0xc093u, 0x81f414u, 3u))
                return BattleChildUnwound(&battle);
            OpPullX(memory, cpu);
            OpLda(memory, cpu, OpAbs(cpu, 0x0b80u));
            OpAndValue(cpu, 0x80u);
            if (cpu->zero)
                goto unavailable;
            OpLda(memory, cpu, OpDp(cpu, 0x1bu));
            OpCmpValue(cpu, 2u);
            if (cpu->zero)
                OpStz(memory, cpu, OpAbs(cpu, 0x0b84u));
            OpLda(memory, cpu, OpAbs(cpu, 0x0b84u));
            OpCmp(memory, cpu, OpDp(cpu, 0xcau));
            if (!cpu->zero && cpu->carry) {
            unavailable:
                OpLoadA(cpu, 1u);
            } else {
                TransferDirectToA(cpu);
            }
            OpSta(memory, cpu, OpAbs(cpu, 0x2180u));
            OpRepWidths(cpu, 0x20u);
            OpTxa(cpu);
            OpSepWidths(cpu, 0x20u);
            OpSta(memory, cpu, OpAbs(cpu, 0x2180u));
            ExchangeAccumulatorBytes(cpu);
            OpSta(memory, cpu, OpAbs(cpu, 0x2180u));
            CommandListName(memory, cpu, 8u);
            OpLda(memory, cpu, OpAbs(cpu, 0x0b84u));
            OpSta(memory, cpu, OpAbs(cpu, 0x4204u));
            OpStz(memory, cpu, OpAbs(cpu, 0x4205u));
            OpLoadA(cpu, 10u);
            OpSta(memory, cpu, OpAbs(cpu, 0x4206u));
            OpStz(memory, cpu, OpAbs(cpu, 0x2180u));
            OpLoadA(cpu, 0x3au);
            OpSta(memory, cpu, OpAbs(cpu, 0x2180u));
            PushAccumulator8(memory, cpu);
            LoadA8(cpu, Pull8(memory, cpu));
            OpLda(memory, cpu, OpAbs(cpu, 0x4214u));
            if (!cpu->zero) {
                cpu->carry = 0;
                OpAdcValue(cpu, 0x30u);
            }
            OpSta(memory, cpu, OpAbs(cpu, 0x2180u));
            CommandListUnits(memory, cpu);
            OpLda(memory, cpu, OpAbs(cpu, 0x0b83u));
            OpSta(memory, cpu, OpAbs(cpu, 0x2180u));
        }
        OpInx(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, 0x0du), 1);
        OpLda(memory, cpu, OpDp(cpu, 0x0du));
        OpCmpValue(cpu, 36u);
    } while (!cpu->zero);
    OpLda(memory, cpu, OpDp(cpu, 0x0du));
    OpBitValue(cpu, 1u);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, 0x0du));
        OpIncA(cpu);
    }
    return ExecutionReturned(0x81c128u);
}
