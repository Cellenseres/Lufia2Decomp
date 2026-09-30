#include "battle/battle_internal.h"

static void SpriteShiftByte(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                            uint8_t offset) {
    const uint32_t address = OpDp(cpu, offset);
    const uint8_t old = Read8(memory, address);
    const uint8_t value = (uint8_t)(old << 1);
    cpu->carry = (old & 0x80u) != 0u;
    Write8(memory, address, value);
    SetNz8(cpu, value);
}

/* $81:B705: append A.low five-byte source records at DB:X into OAM at DB:Y. */
Lufia2ExecutionResult Lufia2BattleAppendOamSprites(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu) {
    for (;;) {
        OpSta(memory, cpu, OpDp(cpu, 0x5au));
        OpBitValue(cpu, 0xffu);
        if (cpu->zero)
            return ExecutionReturned(0x81b77fu);
        OpCmpValue(cpu, 4u);
        bool group = cpu->carry;
        if (group) {
            OpLda(memory, cpu, OpDp(cpu, 0x58u));
            OpBitValue(cpu, 3u);
            group = cpu->zero;
        }
        SetAccumulatorWidth(cpu, 0);
        if (group) {
            for (unsigned i = 0; i < 4u; ++i) {
                OpLda(memory, cpu, OpAbsX(cpu, (uint16_t)(i * 5u)));
                OpSta(memory, cpu, OpAbsY(cpu, (uint16_t)(0x100u + i * 4u)));
                OpLda(memory, cpu, OpAbsX(cpu, (uint16_t)(i * 5u + 2u)));
                OpSta(memory, cpu, OpAbsY(cpu, (uint16_t)(0x102u + i * 4u)));
            }
            OpTya(cpu);
            cpu->carry = 0;
            OpAdcValue(cpu, 0x10u);
            PushAccumulator16(memory, cpu);
            for (unsigned i = 0; i < 4u; ++i)
                OpLsrA(cpu);
            OpTay(cpu);
            OpTxa(cpu);
            cpu->carry = 0;
            OpAdcValue(cpu, 0x14u);
            PushAccumulator16(memory, cpu);
            SetAccumulatorWidth(cpu, 1);
            OpLda(memory, cpu, OpAbsX(cpu, 0x13u));
            for (int i = 2; i >= 0; --i) {
                OpAslA(cpu);
                OpAslA(cpu);
                OpOra(memory, cpu, OpAbsX(cpu, (uint16_t)(i * 5u + 4u)));
            }
            OpSta(memory, cpu, OpAbsY(cpu, 0x2ffu));
            OpPullX(memory, cpu);
            OpPullY(memory, cpu);
            OpLda(memory, cpu, OpDp(cpu, 0x58u));
            cpu->carry = 0;
            OpAdcValue(cpu, 4u);
            OpSta(memory, cpu, OpDp(cpu, 0x58u));
            OpLda(memory, cpu, OpDp(cpu, 0x5au));
            cpu->carry = 1;
            OpSbcValue(cpu, 4u);
            if (cpu->zero)
                return ExecutionReturned(0x81b77fu);
        } else {
            OpLda(memory, cpu, OpAbsX(cpu, 0u));
            OpSta(memory, cpu, OpAbsY(cpu, 0x100u));
            OpLda(memory, cpu, OpAbsX(cpu, 2u));
            OpSta(memory, cpu, OpAbsY(cpu, 0x102u));
            OpTya(cpu);
            PushAccumulator16(memory, cpu);
            for (unsigned i = 0; i < 4u; ++i)
                OpLsrA(cpu);
            OpTay(cpu);
            OpTxa(cpu);
            cpu->carry = 0;
            OpAdcValue(cpu, 5u);
            PushAccumulator16(memory, cpu);
            SetAccumulatorWidth(cpu, 1);
            OpLda(memory, cpu, OpAbsX(cpu, 4u));
            OpAndValue(cpu, 3u);
            OpSta(memory, cpu, OpDp(cpu, 0x5bu));
            OpLda(memory, cpu, OpDp(cpu, 0x58u));
            OpAndValue(cpu, 3u);
            OpSta(memory, cpu, OpDp(cpu, 0x59u));
            OpLoadA(cpu, 0xfcu);
            for (;;) {
                OpStepMem(memory, cpu, OpDp(cpu, 0x59u), -1);
                if (cpu->negative)
                    break;
                SpriteShiftByte(memory, cpu, 0x5bu);
                SpriteShiftByte(memory, cpu, 0x5bu);
                OpAslA(cpu);
                OpAslA(cpu);
                OpOraValue(cpu, 3u);
            }
            OpAndValue(cpu, OpReadM(memory, cpu, OpAbsY(cpu, 0x300u)));
            OpOra(memory, cpu, OpDp(cpu, 0x5bu));
            OpSta(memory, cpu, OpAbsY(cpu, 0x300u));
            OpPullX(memory, cpu);
            OpPullY(memory, cpu);
            SetAccumulatorWidth(cpu, 0);
            OpTya(cpu);
            cpu->carry = 0;
            OpAdcValue(cpu, 4u);
            OpTay(cpu);
            SetAccumulatorWidth(cpu, 1);
            OpStepMem(memory, cpu, OpDp(cpu, 0x58u), 1);
            OpLda(memory, cpu, OpDp(cpu, 0x5au));
            OpDecA(cpu);
            if (cpu->zero)
                return ExecutionReturned(0x81b7d8u);
        }
    }
}

typedef struct SpriteGroup {
    uint16_t flag, mark, source, count, site;
    bool indirect_source;
} SpriteGroup;

static bool EmitSpriteGroup(BattleContext *battle, const SpriteGroup *group) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    OpLda(memory, cpu, OpAbs(cpu, group->flag));
    if (cpu->zero)
        return true;
    OpLda(memory, cpu, OpDp(cpu, 0x58u));
    OpSta(memory, cpu, OpAbs(cpu, group->mark));
    OpLdx(cpu, group->indirect_source ? OpReadX(memory, cpu, OpAbs(cpu, group->source))
                                      : group->source);
    OpLda(memory, cpu, OpAbs(cpu, group->count));
    return BattleCall(battle, group->site, 0x81b705u, 2u);
}

/* $81:B5C4: build the battle OAM groups and apply the final position overlays. */
Lufia2ExecutionResult Lufia2BattleBuildSprites(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu,
                                               Lufia2PushedChildCall child,
                                               void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);
    OpStz(memory, cpu, OpDp(cpu, 0x72u));
    if (!BattleCall(&battle, 0xb5c6u, 0x81b5a3u, 2u))
        return BattleChildUnwound(&battle);
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpStz(memory, cpu, OpDp(cpu, 0x58u));
    OpLdy(cpu, 0u);
    static const SpriteGroup first[] = {
        {0x15d7u, 0x15c4u, 0x15d8u, 0x15dau, 0xb5e3u, true},
        {0x15dfu, 0x15c2u, 0x4c8au, 0x15e2u, 0xb5f6u, false},
        {0x15cfu, 0x15c5u, 0x493du, 0x15d2u, 0xb609u, false},
        {0x15cbu, 0x15bau, 0x15ccu, 0x15ceu, 0xb61cu, true},
        {0x15c7u, 0x15b9u, 0x48c0u, 0x15cau, 0xb62fu, false},
        {0x15e3u, 0x15c3u, 0x4dcau, 0x15e6u, 0xb642u, false},
    };
    for (unsigned i = 0; i < 6u; ++i)
        if (!EmitSpriteGroup(&battle, &first[i]))
            return BattleChildUnwound(&battle);
    OpLda(memory, cpu, OpAbs(cpu, 0x15d3u));
    if (!cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, 0x58u));
        OpSta(memory, cpu, OpDp(cpu, 0x5fu));
        OpLdx(cpu, 0x4956u);
        OpLda(memory, cpu, OpAbs(cpu, 0x15d6u));
        if (!cpu->zero && !BattleCall(&battle, 0xb656u, 0x81b705u, 2u))
            return BattleChildUnwound(&battle);
    }
    static const SpriteGroup last[] = {
        {0x15dbu, 0x15c1u, 0x4b4au, 0x15deu, 0xb669u, false},
        {0x15e7u, 0x15c6u, 0x4b36u, 0x15eau, 0xb67cu, false},
    };
    for (unsigned i = 0; i < 2u; ++i)
        if (!EmitSpriteGroup(&battle, &last[i]))
            return BattleChildUnwound(&battle);
    PullDataBank(memory, cpu);
    OpLda(memory, cpu, OpAbs(cpu, 0x154eu));
    if (!cpu->zero) {
        SetIndexWidth(cpu, 1);
        OpLdx(cpu, 0u);
        OpLda(memory, cpu, OpDp(cpu, 0x5fu));
        do {
            OpSta(memory, cpu, OpAbsX(cpu, 0x15bbu));
            cpu->carry = 0;
            OpAdc(memory, cpu, OpAbsX(cpu, 0x157fu));
            OpInx(cpu);
            OpCpx(cpu, OpReadX(memory, cpu, OpAbs(cpu, 0x154eu)));
        } while (!cpu->zero);
        SetIndexWidth(cpu, 0);
        OpLda(memory, cpu, OpAbs(cpu, 0x154eu));
        if (cpu->zero)
            return ExecutionReturned(0x81b69fu);
        OpSta(memory, cpu, OpDp(cpu, 0x55u));
        OpLdx(cpu, 0u);
        do {
            OpLda(memory, cpu, OpAbsX(cpu, 0x1534u));
            if (!cpu->zero) {
                OpLda(memory, cpu, OpAbsX(cpu, 0x157fu));
                OpSta(memory, cpu, OpDp(cpu, 0x54u));
                OpLda(memory, cpu, OpAbsX(cpu, 0x158fu));
                OpSta(memory, cpu, OpDp(cpu, 0x58u));
                OpLda(memory, cpu, OpAbsX(cpu, 0x1597u));
                OpSta(memory, cpu, OpDp(cpu, 0x59u));
                OpLda(memory, cpu, OpAbsX(cpu, 0x15bbu));
                OpPushX(memory, cpu);
                SetAccumulatorWidth(cpu, 0);
                OpAndValue(cpu, 0xffu);
                OpAslA(cpu);
                OpAslA(cpu);
                OpTay(cpu);
                OpLda(memory, cpu, OpAbsX(cpu, 0x1577u));
                OpAndValue(cpu, 0xffu);
                OpSta(memory, cpu, OpDp(cpu, 0x5du));
                OpAslA(cpu);
                OpAslA(cpu);
                OpAdc(memory, cpu, OpDp(cpu, 0x5du));
                OpTax(cpu);
                SetAccumulatorWidth(cpu, 1);
                do {
                    OpLda(memory, cpu, OpDp(cpu, 0x58u));
                    cpu->carry = 0;
                    OpAdc(memory, cpu, OpLongX(cpu, 0x7e4956u));
                    OpSta(memory, cpu, OpAbsY(cpu, 0x100u));
                    OpInx(cpu);
                    OpIny(cpu);
                    OpLda(memory, cpu, OpDp(cpu, 0x59u));
                    cpu->carry = 0;
                    OpAdc(memory, cpu, OpLongX(cpu, 0x7e4956u));
                    OpSta(memory, cpu, OpAbsY(cpu, 0x100u));
                    for (unsigned i = 0; i < 4u; ++i)
                        OpInx(cpu);
                    for (unsigned i = 0; i < 3u; ++i)
                        OpIny(cpu);
                    OpStepMem(memory, cpu, OpDp(cpu, 0x54u), -1);
                } while (!cpu->zero);
                OpPullX(memory, cpu);
            }
            OpInx(cpu);
            OpCpx(cpu, 6u);
            /* Original DEC $55 replaces CPX's flags before the loop branch. */
            OpStepMem(memory, cpu, OpDp(cpu, 0x55u), -1);
        } while (!cpu->zero);
    }
    OpLoadA(cpu, 0x40u);
    OpSta(memory, cpu, OpDp(cpu, 0x72u));
    return ExecutionReturned(0x81b704u);
}
