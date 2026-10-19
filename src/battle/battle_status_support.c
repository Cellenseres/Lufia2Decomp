#include "battle/battle_internal.h"
#include "core/wram_view.h"
#include "core/child_call.h"
#include "battle/status_target_internal.h"
#include "battle/status_cache_internal.h"

enum {
    STATUS_TEXT_LENGTH = 0x1266u,
    STATUS_TEXT_BUFFER = WRAM_TEXT_WAIT_ACTOR,
    STATUS_PHRASE_TABLE = 0x85efd3u,
    STATUS_MARKER_COUNT = 5u,
    STATUS_MARKER_SPRITE_STATE = 0x1435u,
    TEXT_BLANK_GLYPH = 0x10u,
    EFFECT_TARGET_MASK = 0x09fbu,
    EFFECT_BASE = 0x09fau,
    EFFECT_ENEMY_SIDE_BASE = 5u,
    EFFECT_RECORD_TABLE = 0x859ed6u,
};

/* Copy zero-terminated text into the status buffer at Y. */
static void CopyStatusText(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);

    for (;;) {
        LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0u, cpu->x)));
        WramWriteAt(wram, STATUS_TEXT_BUFFER, cpu->y, A8(cpu));
        if (cpu->zero)
            break;
        cpu->x = (uint16_t)(cpu->x + 1u);
        cpu->y = (uint16_t)(cpu->y + 1u);
    }
}

/* $85:9150: copy a name and remove trailing blank glyphs. */
Lufia2ExecutionResult Lufia2BattleStatusName(const Lufia2Memory *memory,
                                             Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);

    OpPushX(memory, cpu);
    cpu->y = 0;
    CopyStatusText(memory, cpu);
    /* Blank trailing glyphs become zero, walking back. */
    do {
        LoadA8(cpu, WramReadAt(wram, STATUS_TEXT_BUFFER - 1u, cpu->y));
        Compare8(cpu, A8(cpu), TEXT_BLANK_GLYPH);
        if (!cpu->zero)
            break;
        TransferDirectToA(cpu);
        WramWriteAt(wram, STATUS_TEXT_BUFFER - 1u, cpu->y, A8(cpu));
        cpu->y = (uint16_t)(cpu->y - 1u);
    } while (cpu->y != 0);
    WramWrite16(wram, STATUS_TEXT_LENGTH, cpu->y);
    OpPullX(memory, cpu);
    return ExecutionReturned(0x859172u);
}

/* $85:9173: append the selected status phrase from the original ROM. */
Lufia2ExecutionResult Lufia2BattleStatusPhrase(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    OpPushX(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(STATUS_PHRASE_TABLE, cpu->x)));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    OpSetDataBank(memory, cpu, 0x85u);
    {
        const Lufia2Wram wram = WramViewOfCaller(memory, cpu);

        LoadY16(cpu, WramRead16(wram, STATUS_TEXT_LENGTH));
        CopyStatusText(memory, cpu);
        WramWrite16(wram, STATUS_TEXT_LENGTH, cpu->y);
    }
    OpPullX(memory, cpu);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x85919bu);
}

void Lufia2BattleUpdateStatusMarkers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint8_t marker_bank) {
    PushDataBank(memory, cpu);
    Push8(memory, cpu, marker_bank);
    PullDataBank(memory, cpu);
    {
        const Lufia2Wram wram = WramViewOfCaller(memory, cpu);

        cpu->y = 0;
        cpu->x = 0;
        do {
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, WramRead16At(wram, WRAM_BATTLE_PARTY_RECORDS, cpu->x));
            if (!cpu->zero) {
                /* New status markers start at $FF. */
                OpPushX(memory, cpu);
                TransferAToX(cpu);
                SetAccumulatorWidth(cpu, 1);
                LoadA8(cpu, WramReadAt(wram, BATTLE_ICON_RECORD_STATUS, cpu->y));
                ExchangeAccumulatorBytes(cpu);
                LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(
                                              cpu, BATTLE_BATTLER_STATUS, cpu->x)));
                WramWriteAt(wram, BATTLE_ICON_RECORD_STATUS, cpu->y, A8(cpu));
                if (!cpu->zero) {
                    ExchangeAccumulatorBytes(cpu);
                    if (cpu->zero) {
                        LoadA8(cpu, 0xffu);
                        WramWriteAt(wram, BATTLE_ICON_RECORD_TIMER, cpu->y, A8(cpu));
                        WramWriteAt(wram, WRAM_BATTLE_STATUS_ICON_RECORDS, cpu->y,
                                    A8(cpu));
                    }
                } else {
                    TransferDirectToA(cpu);
                    WramWriteAt(wram, WRAM_BATTLE_STATUS_ICON_RECORDS, cpu->y, A8(cpu));
                }
                OpPullX(memory, cpu);
            }
            cpu->x = (uint16_t)(cpu->x + BATTLE_POINTER_SIZE);
            cpu->y = (uint16_t)(cpu->y + BATTLE_STATUS_ICON_RECORD_SIZE);
            Compare16(cpu, cpu->y,
                      (uint16_t)(STATUS_MARKER_COUNT * BATTLE_STATUS_ICON_RECORD_SIZE));
        } while (!cpu->zero);
    }
    SetAccumulatorWidth(cpu, 1);
    PullDataBank(memory, cpu);
}

Lufia2ExecutionResult Lufia2BattleSyncStatusMarkers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2BattleUpdateStatusMarkers(memory, cpu, cpu->program_bank);
    return ExecutionReturned(0x8591dfu);
}

/* $85:91E0: clear party markers and five sprite-state words. */
Lufia2ExecutionResult Lufia2BattleClearStatusMarkers(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    {
        const Lufia2Wram wram = WramViewOfCaller(memory, cpu);

        cpu->y = 0;
        cpu->x = 0;
        TransferDirectToA(cpu);
        do {
            WramWriteAt(wram, WRAM_BATTLE_STATUS_ICON_RECORDS, cpu->y, A8(cpu));
            cpu->x = (uint16_t)(cpu->x + BATTLE_POINTER_SIZE);
            cpu->y = (uint16_t)(cpu->y + BATTLE_STATUS_ICON_RECORD_SIZE);
            Compare16(cpu, cpu->y,
                      (uint16_t)(STATUS_MARKER_COUNT * BATTLE_STATUS_ICON_RECORD_SIZE));
        } while (!cpu->zero);
        SetAccumulatorWidth(cpu, 0);
        cpu->y = 0;
        do {
            TransferDirectToA(cpu);
            WramWrite16At(wram, STATUS_MARKER_SPRITE_STATE, cpu->y, cpu->accumulator);
            OpTya(cpu);
            cpu->carry = 0;
            Add16Value(cpu, BATTLE_STATUS_SPRITE_RECORD_SIZE);
            TransferAToY(cpu);
            Compare16(
                cpu, cpu->y,
                (uint16_t)(STATUS_MARKER_COUNT * BATTLE_STATUS_SPRITE_RECORD_SIZE));
        } while (!cpu->zero);
    }
    SetAccumulatorWidth(cpu, 1);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x85920du);
}

/* $85:D9C9: effect record from the target mask's lowest bit. */
Lufia2ExecutionResult Lufia2BattleEffectRecord(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);

    WramWrite(wram, EFFECT_TARGET_MASK, A8(cpu));
    And8(cpu, BATTLE_ACTOR_ENEMY_SIDE);
    if (!cpu->zero)
        LoadA8(cpu, EFFECT_ENEMY_SIDE_BASE);
    WramWrite(wram, EFFECT_BASE, A8(cpu));
    LoadA8(cpu, 0xffu);
    do {
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        uint8_t mask = WramRead(wram, EFFECT_TARGET_MASK);
        cpu->carry = mask & 1u;
        mask >>= 1;
        WramWrite(wram, EFFECT_TARGET_MASK, mask);
        SetNz8(cpu, mask);
    } while (!cpu->carry);
    cpu->carry = 0;
    Adc8(cpu, WramRead(wram, EFFECT_BASE));
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(EFFECT_RECORD_TABLE, cpu->x)));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x85d9efu);
}

enum {
    STATUS_MESSAGE_FRAMES = 45u,
    STATUS_INPUT_FIRST = 0u,
    STATUS_INPUT_SECOND = 1u,
    STATUS_SELECTED_TARGET = 3u,
    STATUS_RECORD_TIMER = 6u,
    STATUS_EXPIRED_PHRASE = 6u,
    STATUS_RECORD_BYTES = 30u,
    STATUS_TIMED_FLAG = 0x80u,
    STATUS_WRAM_DATA_PORT = 0x2180u,
    STATUS_WRAM_ADDRESS_PORT = 0x2181u,
    STATUS_WRAM_BANK_PORT = 0x2183u,
    ROM_SAVE_MESSAGE_STATE = 0x859aaau,
    ROM_RESTORE_MESSAGE_STATE = 0x859abcu,
    ROM_DISPLAY_MESSAGE = 0x8595feu,
    ROM_CLEAR_MESSAGE = 0x859671u
};

static uint8_t StatusCallerContext(const Lufia2CpuState *cpu, uint8_t m) {
    return cpu->program_bank == 0x85u && cpu->accumulator_is_8_bit == m &&
           !cpu->index_is_8_bit && !cpu->decimal;
}

static uint8_t StatusCall(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                          Lufia2PushedChildCall child, void *context, uint32_t site,
                          uint32_t target, uint8_t frame, uint8_t m,
                          Lufia2ExecutionResult *result) {
    if (!CallChildWithFrame(memory, cpu, child, context, site, target, frame, 0x85u)) {
        *result = (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return 0u;
    }
    if (!StatusCallerContext(cpu, m)) {
        *result = ExecutionHandoff(cpu, site + frame + 1u);
        return 0u;
    }
    return 1u;
}

static uint8_t StatusMessageFrame(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                  Lufia2PushedChildCall child, void *context,
                                  uint32_t sprite_site, uint32_t input_site,
                                  Lufia2ExecutionResult *result) {
    if (!StatusCall(memory, cpu, child, context, sprite_site, BATTLE_ROUTINE_SPRITES,
                    3u, 1u, result))
        return 0u;
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, BATTLE_SPRITE_REBUILD_REQUEST);
    return StatusCall(memory, cpu, child, context, input_site,
                      BATTLE_ROUTINE_FRAME_INPUT, 3u, 1u, result);
}

Lufia2ExecutionResult Lufia2BattleShowStatusMessage(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu,
                                                    Lufia2PushedChildCall child,
                                                    void *context) {
    if (!StatusCallerContext(cpu, 1u) || !child)
        return ExecutionHandoff(cpu, 0x859114u);
    Lufia2ExecutionResult result;
    OpPushX(memory, cpu);
    if (!StatusCall(memory, cpu, child, context, 0x859115u, ROM_SAVE_MESSAGE_STATE, 3u,
                    1u, &result))
        return result;
    OpLdx(cpu, 0xffffu);
    Write16Absolute(memory, cpu, WRAM_BATTLE_WAIT_COUNTER, cpu->x);
    if (!StatusCall(memory, cpu, child, context, 0x85911fu, ROM_DISPLAY_MESSAGE, 3u, 1u,
                    &result))
        return result;
    OpLoadA(cpu, STATUS_MESSAGE_FRAMES);
    do {
        PushAccumulator8(memory, cpu);
        if (!StatusMessageFrame(memory, cpu, child, context, 0x859126u, 0x859130u,
                                &result))
            return result;
        LoadA8(cpu, Pull8(memory, cpu));
        OpDecA(cpu);
    } while (!cpu->zero);
    if (!StatusCall(memory, cpu, child, context, 0x859138u, ROM_CLEAR_MESSAGE, 3u, 1u,
                    &result))
        return result;
    if (!StatusMessageFrame(memory, cpu, child, context, 0x85913cu, 0x859146u, &result))
        return result;
    if (!StatusCall(memory, cpu, child, context, 0x85914au, ROM_RESTORE_MESSAGE_STATE,
                    3u, 1u, &result))
        return result;
    OpPullX(memory, cpu);
    return ExecutionReturned(0x85914fu);
}

Lufia2ExecutionResult Lufia2BattleShowStatusUpdates(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu,
                                                    Lufia2PushedChildCall child,
                                                    void *context) {
    if (!StatusCallerContext(cpu, 1u) || !child)
        return ExecutionHandoff(cpu, 0x85903fu);
    Lufia2ExecutionResult result;
    OpPushX(memory, cpu);
    if (!StatusCall(memory, cpu, child, context, 0x859040u, ROM_SAVE_MESSAGE_STATE, 3u,
                    1u, &result))
        return result;
    OpLdx(cpu, 0xffffu);
    Write16Absolute(memory, cpu, WRAM_BATTLE_WAIT_COUNTER, cpu->x);
    if (!StatusCall(memory, cpu, child, context, 0x85904au, ROM_DISPLAY_MESSAGE, 3u, 1u,
                    &result))
        return result;
    if (!StatusCall(memory, cpu, child, context, 0x85904eu,
                    BATTLE_ROUTINE_SYNC_STATUS_MARKERS, 3u, 1u, &result))
        return result;
    if (!StatusCall(memory, cpu, child, context, 0x859052u, 0x81bae8u, 3u, 1u, &result))
        return result;
    OpRepWidths(cpu, 0x20u);
    if (!StatusCall(memory, cpu, child, context, 0x859058u,
                    BATTLE_ROUTINE_QUEUE_STATUS_SPRITES, 3u, 0u, &result))
        return result;
    OpSepWidths(cpu, 0x20u);
    if (!StatusMessageFrame(memory, cpu, child, context, 0x85905eu, 0x859068u, &result))
        return result;
    OpLoadA(cpu, STATUS_MESSAGE_FRAMES);
    do {
        PushAccumulator8(memory, cpu);
        if (!StatusMessageFrame(memory, cpu, child, context, 0x85906fu, 0x859079u,
                                &result))
            return result;
        LoadA8(cpu, Pull8(memory, cpu));
        OpDecA(cpu);
    } while (!cpu->zero);
    if (!StatusCall(memory, cpu, child, context, 0x859081u, ROM_CLEAR_MESSAGE, 3u, 1u,
                    &result))
        return result;
    if (!StatusMessageFrame(memory, cpu, child, context, 0x859085u, 0x85908fu, &result))
        return result;
    if (!StatusCall(memory, cpu, child, context, 0x859093u, ROM_RESTORE_MESSAGE_STATE,
                    3u, 1u, &result))
        return result;
    OpPullX(memory, cpu);
    return ExecutionReturned(0x859098u);
}

BattleStatusTargetState BattlePrepareStatusTarget(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu,
                                                  Lufia2PushedChildCall child,
                                                  void *context, uint8_t recovery,
                                                  Lufia2ExecutionResult *result) {
    OpLda(memory, cpu, OpDp(cpu, STATUS_INPUT_FIRST));
    OpOraValue(cpu, OpReadM(memory, cpu, OpDp(cpu, STATUS_INPUT_SECOND)));
    OpSta(memory, cpu, OpDp(cpu, STATUS_SELECTED_TARGET));
    if (!StatusCall(memory, cpu, child, context, recovery ? 0x858f90u : 0x8590c2u,
                    0x85d9c9u, 3u, 1u, result))
        return BATTLE_STATUS_TARGET_STOPPED;
    Write16Absolute(memory, cpu, STATUS_WRAM_ADDRESS_PORT, cpu->x);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, STATUS_WRAM_BANK_PORT));
    OpLdx(cpu, STATUS_RECORD_BYTES);
    do {
        OpStz(memory, cpu, OpAbs(cpu, STATUS_WRAM_DATA_PORT));
        OpDex(cpu);
    } while (!cpu->zero);
    OpLda(memory, cpu, OpDp(cpu, STATUS_SELECTED_TARGET));
    if (!StatusCall(memory, cpu, child, context, recovery ? 0x858fa7u : 0x8590d9u,
                    0x81b2b5u, 3u, 1u, result))
        return BATTLE_STATUS_TARGET_STOPPED;
    Compare16(cpu, cpu->x, 0u);
    if (cpu->zero)
        return BATTLE_STATUS_TARGET_SKIPPED;
    OpLda(memory, cpu, OpAbsX(cpu, BATTLE_BATTLER_STATUS));
    OpBitValue(cpu, BATTLE_STATUS_DOWNED);
    if (!cpu->zero)
        return BATTLE_STATUS_TARGET_SKIPPED;
    return BATTLE_STATUS_TARGET_READY;
}

Lufia2ExecutionResult Lufia2BattleTickStatusMessage(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu,
                                                    Lufia2PushedChildCall child,
                                                    void *context) {
    if (!StatusCallerContext(cpu, 1u) || !child)
        return ExecutionHandoff(cpu, 0x8590bcu);
    Lufia2ExecutionResult result;
    const BattleStatusTargetState target =
        BattlePrepareStatusTarget(memory, cpu, child, context, 0u, &result);
    if (target == BATTLE_STATUS_TARGET_STOPPED)
        return result;
    if (target == BATTLE_STATUS_TARGET_SKIPPED)
        return ExecutionReturned(0x859113u);
    OpBitValue(cpu, STATUS_TIMED_FLAG);
    if (cpu->zero)
        return ExecutionReturned(0x859113u);
    OpTxy(cpu);
    OpLda(memory, cpu, OpDp(cpu, STATUS_SELECTED_TARGET));
    if (!StatusCall(memory, cpu, child, context, 0x8590f0u, 0x81b2dbu, 3u, 1u, &result))
        return result;
    OpLda(memory, cpu, OpAbsX(cpu, STATUS_RECORD_TIMER));
    OpDecA(cpu);
    OpSta(memory, cpu, OpAbsX(cpu, STATUS_RECORD_TIMER));
    if (!cpu->zero)
        return ExecutionReturned(0x859113u);
    OpTyx(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, BATTLE_BATTLER_STATUS));
    OpAndValue(cpu, 0x7fu);
    OpSta(memory, cpu, OpAbsX(cpu, BATTLE_BATTLER_STATUS));
    if (!StatusCall(memory, cpu, child, context, 0x859106u, 0x859150u, 3u, 1u, &result))
        return result;
    OpLoadA(cpu, STATUS_EXPIRED_PHRASE);
    if (!StatusCall(memory, cpu, child, context, 0x85910cu, 0x859173u, 3u, 1u, &result))
        return result;
    if (!StatusCall(memory, cpu, child, context, 0x859110u, 0x859114u, 2u, 1u, &result))
        return result;
    return ExecutionReturned(0x859113u);
}
