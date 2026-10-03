#include "battle/battle_internal.h"
#include "core/wram_view.h"

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

/* $85:91A1: synchronize five party status markers. */
Lufia2ExecutionResult Lufia2BattleSyncStatusMarkers(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    {
        const Lufia2Wram wram = WramViewOfCaller(memory, cpu);

        cpu->y = 0;
        cpu->x = 0;
        do {
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, WramRead16At(wram, WRAM_BATTLE_PARTY_RECORDS, cpu->x));
            if (!cpu->zero) {
                /* Marker follows the battler's status; new ones start at $FF. */
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
    uint8_t mask = A8(cpu);
    uint8_t bit = 0xffu;
    bool lowest;

    WramWrite(wram, EFFECT_TARGET_MASK, mask);
    And8(cpu, BATTLE_ACTOR_ENEMY_SIDE);
    if (!cpu->zero)
        LoadA8(cpu, EFFECT_ENEMY_SIDE_BASE);
    WramWrite(wram, EFFECT_BASE, A8(cpu));
    do {
        ++bit;
        lowest = (mask & 1u) != 0;
        mask = (uint8_t)(mask >> 1);
        WramWrite(wram, EFFECT_TARGET_MASK, mask);
    } while (!lowest);
    cpu->carry = 0;
    LoadA8(cpu, bit);
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
