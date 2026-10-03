#include "battle/battle_internal.h"
#include "system/wram.h"

enum {
    SPRITE_DP_OAM_COUNT = 0x58u,
    SPRITE_DP_OVERLAY_X = 0x58u,
    SPRITE_DP_OVERLAY_Y = 0x59u,
    SPRITE_DP_ATTRIBUTE_SHIFT = 0x59u,
    SPRITE_DP_REMAINING_COUNT = 0x5au,
    SPRITE_DP_HIGH_ATTRIBUTES = 0x5bu,
    SPRITE_DP_OVERLAY_SPRITES_LEFT = 0x54u,
    SPRITE_DP_OVERLAYS_LEFT = 0x55u,
    SPRITE_DP_SOURCE_OFFSET = 0x5du,
    SPRITE_DP_OVERLAY_FIRST_OAM = 0x5fu,
};

enum {
    OAM_LOW_TABLE = 0x100u,
    OAM_HIGH_TABLE = 0x300u,
    OAM_FOUR_SPRITE_SOURCE_STEP = 4u * BATTLE_SPRITE_SOURCE_SIZE,
    OAM_FOUR_SPRITE_OAM_STEP = 4u * BATTLE_OAM_ENTRY_SIZE,
    OAM_SOURCE_ATTRIBUTE = 4u,
};

/* Copies the position and tile words of source sprite `index` (relative to X)
 * into the low OAM entry `index` entries after Y. */
static void OamCopySprite(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                          unsigned index) {
    const uint16_t source = (uint16_t)(index * BATTLE_SPRITE_SOURCE_SIZE);
    const uint16_t entry = (uint16_t)(OAM_LOW_TABLE + index * BATTLE_OAM_ENTRY_SIZE);
    Write16Long(memory, OpAbsY(cpu, entry), Read16Long(memory, OpAbsX(cpu, source)));
    Write16Long(memory, OpAbsY(cpu, (uint16_t)(entry + 2u)),
                Read16Long(memory, OpAbsX(cpu, (uint16_t)(source + 2u))));
}

/* The high OAM byte for four sprites: the 2-bit attributes of sprites 0..3 in
 * ascending bit pairs. Reads the attribute byte of each source record. */
static uint8_t OamPackFourAttributes(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    uint8_t packed = Read8(memory, OpAbsX(cpu, 0x13u));
    for (int i = 2; i >= 0; --i)
        packed = (uint8_t)((uint8_t)(packed << 2) |
                           Read8(memory,
                                 OpAbsX(cpu, (uint16_t)(i * BATTLE_SPRITE_SOURCE_SIZE +
                                                        OAM_SOURCE_ATTRIBUTE))));
    return packed;
}

/* Replaces the 2-bit attribute at `slot` of a high OAM byte. */
static uint8_t OamMergeAttribute(uint8_t old, unsigned slot, uint8_t attribute) {
    const unsigned shift = slot * 2u;
    return (uint8_t)((old & ~(3u << shift)) | ((unsigned)(attribute & 3u) << shift));
}

/* Appends four source records at once; only valid when the OAM count is a
 * multiple of four. Leaves the registers the way the long form would. */
static void OamAppendFour(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    for (unsigned i = 0; i < 4u; ++i)
        OamCopySprite(memory, cpu, i);
    OpTya(cpu);
    cpu->carry = 0;
    OpAdcValue(cpu, OAM_FOUR_SPRITE_OAM_STEP);
    PushAccumulator16(memory, cpu);
    for (unsigned i = 0; i < 4u; ++i)
        OpLsrA(cpu);
    OpTay(cpu);
    OpTxa(cpu);
    cpu->carry = 0;
    OpAdcValue(cpu, OAM_FOUR_SPRITE_SOURCE_STEP);
    PushAccumulator16(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, OamPackFourAttributes(memory, cpu));
    OpSta(memory, cpu, OpAbsY(cpu, OAM_HIGH_TABLE - 1u));
    OpPullX(memory, cpu);
    OpPullY(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, SPRITE_DP_OAM_COUNT));
    cpu->carry = 0;
    OpAdcValue(cpu, 4u);
    OpSta(memory, cpu, OpDp(cpu, SPRITE_DP_OAM_COUNT));
}

/* Appends one source record: copies its words and merges its attribute bits
 * into the high OAM byte shared by four sprites. */
static void OamAppendOne(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OamCopySprite(memory, cpu, 0u);
    OpTya(cpu);
    PushAccumulator16(memory, cpu);
    for (unsigned i = 0; i < 4u; ++i)
        OpLsrA(cpu);
    OpTay(cpu);
    OpTxa(cpu);
    cpu->carry = 0;
    OpAdcValue(cpu, BATTLE_SPRITE_SOURCE_SIZE);
    PushAccumulator16(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    const uint8_t attribute =
        (uint8_t)(Read8(memory, OpAbsX(cpu, OAM_SOURCE_ATTRIBUTE)) & 3u);
    const unsigned slot = Read8(memory, OpDp(cpu, SPRITE_DP_OAM_COUNT)) & 3u;
    Write8(memory, OpDp(cpu, SPRITE_DP_HIGH_ATTRIBUTES), attribute);
    Write8(memory, OpDp(cpu, SPRITE_DP_ATTRIBUTE_SHIFT), (uint8_t)slot);
    /* The original shifts the attribute scratch byte left in place, two bits
     * per slot, counting the slot down to $FF; keep those writes. */
    for (unsigned done = 0; done < slot; ++done) {
        Write8(memory, OpDp(cpu, SPRITE_DP_ATTRIBUTE_SHIFT),
               (uint8_t)(slot - 1u - done));
        Write8(memory, OpDp(cpu, SPRITE_DP_HIGH_ATTRIBUTES),
               (uint8_t)(attribute << (2u * done + 1u)));
        Write8(memory, OpDp(cpu, SPRITE_DP_HIGH_ATTRIBUTES),
               (uint8_t)(attribute << (2u * done + 2u)));
    }
    Write8(memory, OpDp(cpu, SPRITE_DP_ATTRIBUTE_SHIFT), 0xffu);
    const uint8_t merged =
        OamMergeAttribute(Read8(memory, OpAbsY(cpu, OAM_HIGH_TABLE)), slot, attribute);
    LoadA8(cpu, merged);
    OpSta(memory, cpu, OpAbsY(cpu, OAM_HIGH_TABLE));
    OpPullX(memory, cpu);
    OpPullY(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    OpTya(cpu);
    cpu->carry = 0;
    OpAdcValue(cpu, BATTLE_OAM_ENTRY_SIZE);
    OpTay(cpu);
    SetAccumulatorWidth(cpu, 1);
    OpStepMem(memory, cpu, OpDp(cpu, SPRITE_DP_OAM_COUNT), 1);
}

/* $81:B705: append A.low five-byte source records at DB:X into OAM at DB:Y. */
Lufia2ExecutionResult Lufia2BattleAppendOamSprites(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu) {
    for (;;) {
        OpSta(memory, cpu, OpDp(cpu, SPRITE_DP_REMAINING_COUNT));
        OpBitValue(cpu, 0xffu);
        if (cpu->zero)
            return ExecutionReturned(0x81b77fu);
        OpCmpValue(cpu, 4u);
        bool pack_four_sprites = cpu->carry;
        if (pack_four_sprites) {
            OpLda(memory, cpu, OpDp(cpu, SPRITE_DP_OAM_COUNT));
            OpBitValue(cpu, 3u);
            pack_four_sprites = cpu->zero;
        }
        SetAccumulatorWidth(cpu, 0);
        if (pack_four_sprites) {
            OamAppendFour(memory, cpu);
            OpLda(memory, cpu, OpDp(cpu, SPRITE_DP_REMAINING_COUNT));
            cpu->carry = 1;
            OpSbcValue(cpu, 4u);
            if (cpu->zero)
                return ExecutionReturned(0x81b77fu);
        } else {
            OamAppendOne(memory, cpu);
            OpLda(memory, cpu, OpDp(cpu, SPRITE_DP_REMAINING_COUNT));
            OpDecA(cpu);
            if (cpu->zero)
                return ExecutionReturned(0x81b7d8u);
        }
    }
}

typedef struct BattleSpriteGroup {
    uint16_t enabled_flag_address;
    uint16_t first_oam_index_address;
    uint16_t source;
    uint16_t sprite_count_address;
    uint16_t call_site;
    bool indirect_source;
} BattleSpriteGroup;

static bool EmitSpriteGroup(BattleContext *battle, const BattleSpriteGroup *group) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    OpLda(memory, cpu, OpAbs(cpu, group->enabled_flag_address));
    if (cpu->zero)
        return true;
    OpLda(memory, cpu, OpDp(cpu, SPRITE_DP_OAM_COUNT));
    OpSta(memory, cpu, OpAbs(cpu, group->first_oam_index_address));
    OpLdx(cpu, group->indirect_source ? OpReadX(memory, cpu, OpAbs(cpu, group->source))
                                      : group->source);
    OpLda(memory, cpu, OpAbs(cpu, group->sprite_count_address));
    return BattleCall(battle, group->call_site, 0x81b705u, 2u);
}

static bool BattleApplySpritePositionOverlays(const Lufia2Memory *memory,
                                              Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, 0x154eu));
    if (!cpu->zero) {
        SetIndexWidth(cpu, 1);
        OpLdx(cpu, 0u);
        OpLda(memory, cpu, OpDp(cpu, SPRITE_DP_OVERLAY_FIRST_OAM));
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
            return false;
        OpSta(memory, cpu, OpDp(cpu, SPRITE_DP_OVERLAYS_LEFT));
        OpLdx(cpu, 0u);
        do {
            OpLda(memory, cpu, OpAbsX(cpu, 0x1534u));
            if (!cpu->zero) {
                OpLda(memory, cpu, OpAbsX(cpu, 0x157fu));
                OpSta(memory, cpu, OpDp(cpu, SPRITE_DP_OVERLAY_SPRITES_LEFT));
                OpLda(memory, cpu, OpAbsX(cpu, 0x158fu));
                OpSta(memory, cpu, OpDp(cpu, SPRITE_DP_OVERLAY_X));
                OpLda(memory, cpu, OpAbsX(cpu, 0x1597u));
                OpSta(memory, cpu, OpDp(cpu, SPRITE_DP_OVERLAY_Y));
                OpLda(memory, cpu, OpAbsX(cpu, 0x15bbu));
                OpPushX(memory, cpu);
                SetAccumulatorWidth(cpu, 0);
                OpAndValue(cpu, 0xffu);
                OpAslA(cpu);
                OpAslA(cpu);
                OpTay(cpu);
                OpLda(memory, cpu, OpAbsX(cpu, (WRAM_SYSTEM_MULTIPLY_PRODUCT + 3u)));
                OpAndValue(cpu, 0xffu);
                OpSta(memory, cpu, OpDp(cpu, SPRITE_DP_SOURCE_OFFSET));
                OpAslA(cpu);
                OpAslA(cpu);
                OpAdc(memory, cpu, OpDp(cpu, SPRITE_DP_SOURCE_OFFSET));
                OpTax(cpu);
                SetAccumulatorWidth(cpu, 1);
                do {
                    OpLda(memory, cpu, OpDp(cpu, SPRITE_DP_OVERLAY_X));
                    cpu->carry = 0;
                    OpAdc(memory, cpu, OpLongX(cpu, 0x7e4956u));
                    OpSta(memory, cpu, OpAbsY(cpu, 0x100u));
                    OpInx(cpu);
                    OpIny(cpu);
                    OpLda(memory, cpu, OpDp(cpu, SPRITE_DP_OVERLAY_Y));
                    cpu->carry = 0;
                    OpAdc(memory, cpu, OpLongX(cpu, 0x7e4956u));
                    OpSta(memory, cpu, OpAbsY(cpu, 0x100u));
                    for (unsigned i = 0; i < 4u; ++i)
                        OpInx(cpu);
                    for (unsigned i = 0; i < 3u; ++i)
                        OpIny(cpu);
                    OpStepMem(memory, cpu, OpDp(cpu, SPRITE_DP_OVERLAY_SPRITES_LEFT),
                              -1);
                } while (!cpu->zero);
                OpPullX(memory, cpu);
            }
            OpInx(cpu);
            OpCpx(cpu, 6u);
            /* Original DEC $55 replaces CPX's flags before the loop branch. */
            OpStepMem(memory, cpu, OpDp(cpu, SPRITE_DP_OVERLAYS_LEFT), -1);
        } while (!cpu->zero);
    }
    return true;
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
    OpStz(memory, cpu, OpDp(cpu, SPRITE_DP_OAM_COUNT));
    OpLdy(cpu, 0u);
    static const BattleSpriteGroup primary_groups[] = {
        {WRAM_BATTLE_CURSOR_ENABLED, 0x15c4u, WRAM_BATTLE_CURSOR_SOURCE,
         WRAM_BATTLE_CURSOR_COUNT, 0xb5e3u, true},
        {0x15dfu, 0x15c2u, 0x4c8au, 0x15e2u, 0xb5f6u, false},
        {0x15cfu, 0x15c5u, 0x493du, 0x15d2u, 0xb609u, false},
        {0x15cbu, 0x15bau, 0x15ccu, 0x15ceu, 0xb61cu, true},
        {0x15c7u, 0x15b9u, 0x48c0u, 0x15cau, 0xb62fu, false},
        {0x15e3u, 0x15c3u, 0x4dcau, 0x15e6u, 0xb642u, false},
    };
    for (unsigned i = 0; i < sizeof(primary_groups) / sizeof(primary_groups[0]); ++i)
        if (!EmitSpriteGroup(&battle, &primary_groups[i]))
            return BattleChildUnwound(&battle);
    OpLda(memory, cpu, OpAbs(cpu, 0x15d3u));
    if (!cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, SPRITE_DP_OAM_COUNT));
        OpSta(memory, cpu, OpDp(cpu, SPRITE_DP_OVERLAY_FIRST_OAM));
        OpLdx(cpu, 0x4956u);
        OpLda(memory, cpu, OpAbs(cpu, 0x15d6u));
        if (!cpu->zero && !BattleCall(&battle, 0xb656u, 0x81b705u, 2u))
            return BattleChildUnwound(&battle);
    }
    static const BattleSpriteGroup trailing_groups[] = {
        {0x15dbu, 0x15c1u, 0x4b4au, 0x15deu, 0xb669u, false},
        {0x15e7u, 0x15c6u, 0x4b36u, 0x15eau, 0xb67cu, false},
    };
    for (unsigned i = 0; i < sizeof(trailing_groups) / sizeof(trailing_groups[0]); ++i)
        if (!EmitSpriteGroup(&battle, &trailing_groups[i]))
            return BattleChildUnwound(&battle);
    PullDataBank(memory, cpu);
    if (!BattleApplySpritePositionOverlays(memory, cpu))
        return ExecutionReturned(0x81b69fu);
    OpLoadA(cpu, 0x40u);
    OpSta(memory, cpu, OpDp(cpu, 0x72u));
    return ExecutionReturned(0x81b704u);
}
