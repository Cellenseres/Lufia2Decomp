#include "battle/battle_internal.h"
#include "core/child_call.h"
#include "core/snes_registers.h"

enum {
    ACTION_PENDING = WRAM_BATTLE_ACTION_EFFECT_PENDING & 0xffffu,
    ACTION_DISABLED = WRAM_BATTLE_ACTION_DISABLED & 0xffffu,
    ACTION_EFFECT_ROUTES = 0xafd9u,
    ACTION_EFFECT_ID = WRAM_PALETTE_FADE & 0xffffu,
    ACTION_MESSAGE_COUNT = WRAM_BATTLE_MESSAGE_COUNT & 0xffffu,
    ACTION_SPECIAL_RESULT = WRAM_BATTLE_ACTION_SPECIAL_RESULT & 0xffffu,
    ACTION_SPECIAL_PARTY = WRAM_BATTLE_EFFECT_SPECIAL_PARTY & 0xffffu,
    ACTION_RESULT_COUNT = WRAM_BATTLE_RESULT_COUNT & 0xffffu,
    ACTION_RESULT_STATE = WRAM_FIELD_BATTLE_RESULT,
    ACTION_ACTOR = WRAM_BATTLE_ACTION_WORK,
    ACTION_TYPE = WRAM_BATTLE_ACTION_WORK + 6u,
    ACTION_FRAME_SETUP = WRAM_BATTLE_SPRITE_BUILD_MODE & 0xffffu,
    ACTION_ALTERNATE = WRAM_BATTLE_ACTION_ALTERNATE,
    ACTION_WAIT_LIMIT = 4096u,
    ACTION_ROUTINE_ACTOR_EFFECT = 0x85e5c2u,
    ACTION_ROUTINE_PLAY_EFFECT = 0x81895eu,
    ACTION_ROUTINE_MESSAGE = 0x8595feu
};

typedef struct ActionPresentation {
    BattleContext battle;
    Lufia2ExecutionResult result;
} ActionPresentation;

static bool PresentationContext(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit &&
        !cpu->decimal && cpu->direct_page == 0u &&
        cpu->program_bank == 0x81u && cpu->data_bank == 0x97u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static bool PresentationCall(ActionPresentation *action, uint16_t site,
                             uint32_t target, uint8_t frame) {
    Lufia2CpuState *cpu = action->battle.cpu;

    if (!BattleCall(&action->battle, site, target, frame)) {
        action->result = BattleChildUnwound(&action->battle);
        return false;
    }
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit ||
        cpu->decimal || cpu->direct_page != 0u) {
        action->result = ExecutionHandoff(cpu,
            0x810000u | (uint16_t)(site + (frame == 3u ? 4u : 3u)));
        return false;
    }
    return true;
}

static bool PresentationWait(ActionPresentation *action, uint8_t frames,
                             uint16_t start, uint16_t sprites, uint16_t input) {
    const Lufia2Memory *memory = action->battle.memory;
    Lufia2CpuState *cpu = action->battle.cpu;

    LoadA8(cpu, frames);
    for (unsigned frame = 0u; frame < ACTION_WAIT_LIMIT; ++frame) {
        PushAccumulator8(memory, cpu);
        if (!PresentationCall(action, sprites, BATTLE_ROUTINE_SPRITES, 3u))
            return false;
        LoadA8(cpu, 0xffu);
        Write8(memory, BATTLE_SPRITE_REBUILD_REQUEST, A8(cpu));
        if (!PresentationCall(action, input, BATTLE_ROUTINE_FRAME_INPUT, 3u))
            return false;
        LoadA8(cpu, Pull8(memory, cpu));
        DecrementA8(cpu);
        if (cpu->zero)
            return true;
    }
    action->result = ExecutionHandoff(cpu, 0x810000u | start);
    return false;
}

static Lufia2ExecutionResult ActorActionEffect(ActionPresentation *action,
                                              uint16_t site, uint16_t stop) {
    LoadA8(action->battle.cpu, Read8(action->battle.memory, ACTION_ACTOR));
    if (!PresentationCall(action, site, ACTION_ROUTINE_ACTOR_EFFECT, 3u))
        return action->result;
    return ExecutionReturned(0x810000u | stop);
}

static Lufia2ExecutionResult PartyActionEffect(ActionPresentation *action) {
    const Lufia2Memory *memory = action->battle.memory;
    Lufia2CpuState *cpu = action->battle.cpu;

    LoadA8(cpu, Read8(memory, ACTION_ACTOR));
    if (cpu->negative)
        return ActorActionEffect(action, 0xb01cu, 0xb046u);
    Compare8(cpu, A8(cpu), 0x10u);
    if (cpu->zero) {
        LoadA8(cpu, 0x93u);
        if (!PresentationCall(action, 0xb042u, ACTION_ROUTINE_PLAY_EFFECT, 3u))
            return action->result;
    } else {
        LoadA8(cpu, Read8(memory, ACTION_ACTOR));
        SetIndexWidth(cpu, 1);
        LoadX8(cpu, A8(cpu));
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x97fe26u, cpu->x)));
        LoadX8(cpu, A8(cpu));
        LoadAAbsolute8(memory, cpu, 0x153du, cpu->x);
        SetIndexWidth(cpu, 0);
        cpu->carry = false;
        Adc8(cpu, 0x40u);
        if (!PresentationCall(action, 0xb03au, ACTION_ROUTINE_PLAY_EFFECT, 3u))
            return action->result;
    }
    return ExecutionReturned(0x81b046u);
}

Lufia2ExecutionResult Lufia2BattlePrepareActionEffect(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    ActionPresentation action = {
        BattleContextCreate(memory, cpu, child, child_context, 0x81u), {0}
    };

    if (!PresentationContext(cpu))
        return ExecutionHandoff(cpu, 0x81afc4u);
    LoadAAbsolute8(memory, cpu, ACTION_PENDING, 0u);
    if (cpu->zero)
        return ExecutionReturned(0x81afd8u);
    Write8(memory, AbsoluteIndexedAddress(cpu, ACTION_PENDING, 0u), 0u);
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, ACTION_TYPE));
    And8(cpu, 0x0fu);
    AslA8(cpu);
    LoadX16(cpu, cpu->accumulator);
    const uint32_t route = JumpProgramTable(memory, cpu, ACTION_EFFECT_ROUTES);
    switch (route) {
    case 0x81aff9u:
        return ActorActionEffect(&action, 0xaffdu, 0xb001u);
    case 0x81b002u:
        LoadAAbsolute8(memory, cpu, ACTION_SPECIAL_PARTY, 0u);
        if (!cpu->zero) {
            LoadA8(cpu, Read8(memory, ACTION_ACTOR));
            if (cpu->negative && !PresentationCall(
                &action, 0xb00du, ACTION_ROUTINE_ACTOR_EFFECT, 3u))
                return action.result;
        }
        return ExecutionReturned(0x81b011u);
    case 0x81b012u:
        return PartyActionEffect(&action);
    case 0x81b047u:
        LoadA8(cpu, 0x93u);
        if (!PresentationCall(&action, 0xb049u, ACTION_ROUTINE_PLAY_EFFECT, 3u))
            return action.result;
        return ExecutionReturned(0x81b04du);
    case 0x81b04eu:
        return ActorActionEffect(&action, 0xb052u, 0xb056u);
    default:
        return ExecutionHandoff(cpu, route);
    }
}

Lufia2ExecutionResult Lufia2BattleRunActorAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    ActionPresentation action = {
        BattleContextCreate(memory, cpu, child, child_context, 0x81u), {0}
    };

    if (!PresentationContext(cpu))
        return ExecutionHandoff(cpu, 0x81b057u);
    LoadAAbsolute8(memory, cpu, ACTION_DISABLED, 0u);
    if (!cpu->zero)
        return ExecutionReturned(0x81b089u);
    LoadA8(cpu, Read8(memory, ACTION_ALTERNATE));
    if (cpu->zero) {
        LoadA8(cpu, Read8(memory, ACTION_ACTOR));
        Compare8(cpu, A8(cpu), 0x20u);
        if (!cpu->zero) {
            if (!PresentationCall(&action, 0xb06au, 0x81b2b5u, 3u))
                return action.result;
            LoadAAbsolute8(memory, cpu, BATTLE_BATTLER_STATUS, cpu->x);
            BitImmediate8(cpu, BATTLE_STATUS_NO_TURN_MASK);
            if (!cpu->zero)
                return ExecutionReturned(0x81b089u);
        }
    }
    if (!PresentationCall(&action, 0xb075u, 0x85cd8cu, 3u) ||
        !PresentationCall(&action, 0xb079u, 0x85ce21u, 3u) ||
        !PresentationCall(&action, 0xb07du, 0x85cd9bu, 3u) ||
        !PresentationCall(&action, 0xb081u, ACTION_ROUTINE_MESSAGE, 3u) ||
        !PresentationCall(&action, 0xb085u, 0x81b08au, 3u))
        return action.result;
    return ExecutionReturned(0x81b089u);
}

static bool ResultMessage(ActionPresentation *action, bool previous) {
    const Lufia2Memory *memory = action->battle.memory;
    Lufia2CpuState *cpu = action->battle.cpu;

    if (!previous) {
        LoadA8(cpu, 2u);
        Write8(memory, ACTION_RESULT_STATE, A8(cpu));
    }
    LoadX16(cpu, previous ? 0xf1f0u : 0xf1e7u);
    if (!PresentationCall(action, previous ? 0xb0d4u : 0xb101u, 0x8594e7u, 3u))
        return false;
    LoadX16(cpu, 1u);
    Write16Absolute(memory, cpu, ACTION_MESSAGE_COUNT, cpu->x);
    return PresentationCall(action, previous ? 0xb0deu : 0xb10bu,
        ACTION_ROUTINE_MESSAGE, 3u);
}

static void ChangeResultCount(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                              bool increment) {
    const uint16_t value = Read16AbsoluteIndexed(memory, cpu, ACTION_RESULT_COUNT, 0u);
    const uint16_t changed = (uint16_t)(increment ? value + 1u : value - 1u);

    Write8(memory, AbsoluteIndexedAddress(cpu, ACTION_RESULT_COUNT + 1u, 0u),
        (uint8_t)(changed >> 8));
    Write8(memory, AbsoluteIndexedAddress(cpu, ACTION_RESULT_COUNT, 0u),
        (uint8_t)changed);
    SetNz16(cpu, changed);
}

Lufia2ExecutionResult Lufia2BattleRunActionPresentation(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    ActionPresentation action = {
        BattleContextCreate(memory, cpu, child, child_context, 0x81u), {0}
    };

    if (!PresentationContext(cpu))
        return ExecutionHandoff(cpu, 0x81b08au);
    if (!PresentationCall(&action, 0xb08au, 0x81afc4u, 2u))
        return action.result;
    LoadAAbsolute8(memory, cpu, ACTION_EFFECT_ID, 0u);
    Compare8(cpu, A8(cpu), 0x72u);
    if (!cpu->zero) {
        if (!PresentationCall(&action, 0xb094u, 0x81b139u, 3u))
            return action.result;
        LoadA8(cpu, Read8(memory, ACTION_ALTERNATE));
        if (cpu->zero) {
            if (!PresentationCall(&action, 0xb09eu, 0x81b174u, 3u))
                return action.result;
        } else {
            LoadA8(cpu, 0xdeu);
            if (!PresentationCall(&action, 0xb0a6u, ACTION_ROUTINE_PLAY_EFFECT, 3u))
                return action.result;
        }
        LoadAAbsolute8(memory, cpu, ACTION_FRAME_SETUP, 0u);
        if (!PresentationCall(&action, 0xb0adu, 0x858a39u, 3u))
            return action.result;
        LoadA8(cpu, 0xffu);
        Write8(memory, BATTLE_SPRITE_REBUILD_REQUEST, A8(cpu));
        if (!PresentationCall(&action, 0xb0b7u, BATTLE_ROUTINE_FRAME_INPUT, 3u) ||
            !PresentationCall(&action, 0xb0bbu, 0x8593b7u, 3u))
            return action.result;
        return cpu->carry ? ExecutionHandoff(cpu, 0x8188d5u)
                          : ExecutionReturned(0x81b0c1u);
    }
    LoadAAbsolute8(memory, cpu, ACTION_EFFECT_ID, 0u);
    if (!PresentationCall(&action, 0xb0c8u, ACTION_ROUTINE_PLAY_EFFECT, 3u))
        return action.result;
    LoadAAbsolute8(memory, cpu, ACTION_SPECIAL_RESULT, 0u);
    if (!cpu->zero) {
        if (!ResultMessage(&action, true) ||
            !PresentationWait(&action, 30u, 0xb0e4u, 0xb0e5u, 0xb0efu))
            return action.result;
        return ExecutionReturned(0x81b0f7u);
    }
    if (!ResultMessage(&action, false))
        return action.result;
    SetAccumulatorWidth(cpu, 0);
    ChangeResultCount(memory, cpu, true);
    if (cpu->zero)
        ChangeResultCount(memory, cpu, false);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x31u);
    if (!PresentationCall(&action, 0xb11du, 0x80953bu, 3u) ||
        !PresentationWait(&action, 60u, 0xb123u, 0xb124u, 0xb12eu))
        return action.result;
    return ExecutionHandoff(cpu, 0x818855u);
}

enum {
    DP_FLASH_PALETTE = 0x11u,
    DP_FLASH_ACTOR = 0x22u,
    DP_FLASH_ORIGINAL_PALETTE = 0x23u,
    DP_FLASH_FRAME_PALETTE = 0x24u,
    DP_FLASH_SPRITE_OFFSET = 0x2au,
    FLASH_ACTOR_SLOTS = 0x97fe26u,
    FLASH_ENEMY_STATE = WRAM_BATTLE_ENEMY_SPRITE_FLAGS + 2u,
    FLASH_PARTY_STATE = WRAM_BATTLE_EFFECT_ENEMY_POSITIONS + 3u,
    FLASH_PALETTE_BYTES = 32u,
    FLASH_ENEMY_PALETTES = (WRAM_UNK_7FF1DB + 9u * FLASH_PALETTE_BYTES) & 0xffffu,
    FLASH_PARTY_PALETTES = (WRAM_UNK_7FF1DB + 12u * FLASH_PALETTE_BYTES) & 0xffffu,
    FLASH_TEMP_PALETTE = (WRAM_UNK_7FF1DB + 14u * FLASH_PALETTE_BYTES) & 0xffffu,
    FLASH_ENEMY_SPRITE_BYTES = 15u,
    FLASH_PARTY_SPRITE_BYTES = 13u
};

typedef enum ActorFlashKind {
    ACTOR_FLASH_ENEMY,
    ACTOR_FLASH_PARTY,
    ACTOR_FLASH_CAPSULE
} ActorFlashKind;

typedef struct ActorFlash {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
} ActorFlash;

static bool ActorFlashContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x85u && !cpu->direct_page && !cpu->decimal &&
           cpu->accumulator_is_8_bit && !cpu->index_is_8_bit;
}

static bool ActorFlashCall(ActorFlash *flash, uint32_t site, uint32_t target) {
    Lufia2CpuState *cpu = flash->cpu;
    if (!CallChildWithFrame(flash->memory, cpu, flash->child, flash->context, site,
                            target, 3u, 0x85u)) {
        flash->result =
            (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return false;
    }
    if (!ActorFlashContext(cpu)) {
        flash->result = ExecutionHandoff(cpu, site + 4u);
        return false;
    }
    return true;
}

static bool DrawActorFlashFrame(ActorFlash *flash, ActorFlashKind kind) {
    const Lufia2Memory *memory = flash->memory;
    Lufia2CpuState *cpu = flash->cpu;
    if (kind == ACTOR_FLASH_CAPSULE)
        return ActorFlashCall(flash, 0x85e709u, BATTLE_ROUTINE_FRAME_INPUT);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_FLASH_SPRITE_OFFSET)));
    OpLda(memory, cpu, OpDp(cpu, DP_FLASH_FRAME_PALETTE));
    OpSta(
        memory, cpu,
        OpAbsX(cpu, kind == ACTOR_FLASH_ENEMY ? FLASH_ENEMY_STATE : FLASH_PARTY_STATE));
    if (!ActorFlashCall(flash, kind == ACTOR_FLASH_ENEMY ? 0x85e677u : 0x85e7adu,
                        BATTLE_ROUTINE_SPRITES))
        return false;
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, BATTLE_SPRITE_REBUILD_REQUEST);
    return ActorFlashCall(flash, kind == ACTOR_FLASH_ENEMY ? 0x85e681u : 0x85e7b7u,
                          BATTLE_ROUTINE_FRAME_INPUT);
}

static bool RunActorFlashFrames(ActorFlash *flash, ActorFlashKind kind, uint16_t site,
                                bool repeat) {
    const Lufia2Memory *memory = flash->memory;
    Lufia2CpuState *cpu = flash->cpu;
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    if (repeat) {
        const uint16_t frame_site = kind == ACTOR_FLASH_ENEMY   ? 0xe668u
                                    : kind == ACTOR_FLASH_PARTY ? 0xe79eu
                                                                : 0xe701u;
        OpLoadA(cpu, 5u);
        do {
            PushAccumulator8(memory, cpu);
            SimulateJsrFrame(memory, cpu, (uint16_t)(frame_site + 2u));
            if (!DrawActorFlashFrame(flash, kind))
                return false;
            SimulateRtsFrame(memory, cpu);
            LoadA8(cpu, Pull8(memory, cpu));
            OpDecA(cpu);
        } while (!cpu->zero);
    } else if (!DrawActorFlashFrame(flash, kind)) {
        return false;
    }
    SimulateRtsFrame(memory, cpu);
    return true;
}

static void SelectActorFlashPalette(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                    ActorFlashKind kind) {
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, FLASH_ACTOR_SLOTS));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
    OpLoadA(cpu, kind == ACTOR_FLASH_ENEMY ? FLASH_ENEMY_SPRITE_BYTES
                                           : FLASH_PARTY_SPRITE_BYTES);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, SNES_RDMPYL)));
    OpWriteX(memory, cpu, OpDp(cpu, DP_FLASH_SPRITE_OFFSET), cpu->x);
    OpLda(
        memory, cpu,
        OpAbsX(cpu, kind == ACTOR_FLASH_ENEMY ? FLASH_ENEMY_STATE : FLASH_PARTY_STATE));
    PushAccumulator8(memory, cpu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
    OpLoadA(cpu, FLASH_PALETTE_BYTES);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, SNES_RDMPYL)));
    OpLdy(cpu, 0u);
    PushDataBank(memory, cpu);
    OpLoadA(cpu, 0x7fu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    do {
        OpLda(memory, cpu,
              OpAbsX(cpu, kind == ACTOR_FLASH_ENEMY ? FLASH_ENEMY_PALETTES
                                                    : FLASH_PARTY_PALETTES));
        OpSta(memory, cpu, OpAbsY(cpu, FLASH_TEMP_PALETTE));
        OpInx(cpu);
        OpIny(cpu);
        OpCpy(cpu, FLASH_PALETTE_BYTES);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
}

static void InvertActorFlashPalette(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadA8(cpu, Pull8(memory, cpu));
    OpSta(memory, cpu, OpDp(cpu, DP_FLASH_ORIGINAL_PALETTE));
    OpLdy(cpu, FLASH_PALETTE_BYTES - 1u);
    PushDataBank(memory, cpu);
    OpLoadA(cpu, 0x7fu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    do {
        OpLda(memory, cpu, OpAbsY(cpu, FLASH_TEMP_PALETTE));
        LoadA8(cpu, A8(cpu) ^ 0xffu);
        OpSta(memory, cpu, OpAbsY(cpu, FLASH_TEMP_PALETTE));
        OpDey(cpu);
    } while (!cpu->negative);
    PullDataBank(memory, cpu);
}

static bool FlashOriginalPalette(ActorFlash *flash, ActorFlashKind kind) {
    const Lufia2Memory *memory = flash->memory;
    Lufia2CpuState *cpu = flash->cpu;
    const bool enemy = kind == ACTOR_FLASH_ENEMY;
    SelectActorFlashPalette(memory, cpu, kind);
    OpLoadA(cpu, 14u);
    OpSta(memory, cpu, OpDp(cpu, DP_FLASH_PALETTE));
    if (!ActorFlashCall(flash, enemy ? 0x85e622u : 0x85e758u, 0x81b367u))
        return false;
    InvertActorFlashPalette(memory, cpu);
    if (!ActorFlashCall(flash, enemy ? 0x85e63du : 0x85e773u,
                        BATTLE_ROUTINE_COMMIT_PALETTES))
        return false;
    static const uint16_t enemy_sites[] = {0xe645u, 0xe64cu, 0xe653u, 0xe65au};
    static const uint16_t party_sites[] = {0xe77bu, 0xe782u, 0xe789u, 0xe790u};
    for (unsigned phase = 0u; phase < 4u; ++phase) {
        if (phase & 1u)
            OpLda(memory, cpu, OpDp(cpu, DP_FLASH_ORIGINAL_PALETTE));
        else
            OpLoadA(cpu, enemy ? 5u : 2u);
        OpSta(memory, cpu, OpDp(cpu, DP_FLASH_FRAME_PALETTE));
        if (!RunActorFlashFrames(flash, kind,
                                 enemy ? enemy_sites[phase] : party_sites[phase],
                                 phase != 3u))
            return false;
    }
    return true;
}

static bool FlashFixedPalette(ActorFlash *flash, ActorFlashKind kind) {
    const Lufia2Memory *memory = flash->memory;
    Lufia2CpuState *cpu = flash->cpu;
    const bool enemy = kind == ACTOR_FLASH_ENEMY;
    static const uint32_t enemy_gray[] = {0x85e68au, 0x85e6a4u};
    static const uint32_t capsule_gray[] = {0x85e6c6u, 0x85e6e0u};
    static const uint32_t enemy_upload[] = {0x85e695u, 0x85e6afu};
    static const uint32_t capsule_upload[] = {0x85e6d1u, 0x85e6ebu};
    static const uint16_t enemy_frames[] = {0xe692u, 0xe69du, 0xe6acu, 0xe6b7u};
    static const uint16_t capsule_frames[] = {0xe6ceu, 0xe6d9u, 0xe6e8u, 0xe6f3u};
    for (unsigned phase = 0u; phase < 4u; ++phase) {
        if (!(phase & 1u)) {
            OpLoadA(cpu, enemy ? 5u : 15u);
            OpSta(memory, cpu, OpDp(cpu, DP_FLASH_PALETTE));
            if (!ActorFlashCall(
                    flash, enemy ? enemy_gray[phase / 2u] : capsule_gray[phase / 2u],
                    0x81b336u))
                return false;
        } else if (!ActorFlashCall(flash,
                                   enemy ? enemy_upload[phase / 2u]
                                         : capsule_upload[phase / 2u],
                                   BATTLE_ROUTINE_COMMIT_PALETTES)) {
            return false;
        }
        OpLoadA(cpu, 2u);
        OpSta(memory, cpu, OpDp(cpu, DP_FLASH_FRAME_PALETTE));
        if (!RunActorFlashFrames(flash, kind,
                                 enemy ? enemy_frames[phase] : capsule_frames[phase],
                                 phase != 3u))
            return false;
    }
    return true;
}

Lufia2ExecutionResult Lufia2BattleFlashActionActor(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    if (!child || cpu->program_bank != 0x85u || cpu->direct_page || cpu->decimal ||
        cpu->stack < 0x1f20u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x85e5c2u);
    ActorFlash flash = {memory, cpu, child, context, {0}};
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    OpRepWidths(cpu, 0x30u);
    PushAccumulator16(memory, cpu);
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpDp(cpu, DP_FLASH_ACTOR));
    OpBitValue(cpu, 0x80u);
    uint32_t end;
    if (!cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_EFFECT_SPECIAL_PARTY));
        if (cpu->zero) {
            TransferDirectToA(cpu);
            OpLda(memory, cpu, OpDp(cpu, DP_FLASH_ACTOR));
            OpAndValue(cpu, 0x7fu);
            if (!FlashOriginalPalette(&flash, ACTOR_FLASH_ENEMY))
                return flash.result;
            end = 0x85e664u;
        } else {
            if (!FlashFixedPalette(&flash, ACTOR_FLASH_ENEMY))
                return flash.result;
            end = 0x85e6c1u;
        }
    } else {
        TransferDirectToA(cpu);
        OpLda(memory, cpu, OpDp(cpu, DP_FLASH_ACTOR));
        OpAndValue(cpu, 0x7fu);
        OpCmpValue(cpu, 0x10u);
        if (cpu->zero) {
            if (!FlashFixedPalette(&flash, ACTOR_FLASH_CAPSULE))
                return flash.result;
            end = 0x85e6fdu;
        } else {
            if (!FlashOriginalPalette(&flash, ACTOR_FLASH_PARTY))
                return flash.result;
            end = 0x85e79au;
        }
    }
    OpRepWidths(cpu, 0x30u);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    PullAccumulator16(memory, cpu);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(end);
}
