#include "battle/battle_internal.h"
#include "core/snes_registers.h"

enum {
    EFFECT_STREAM = 0xc3u,
    EFFECT_STREAM_BANK = 0xc5u,
    EFFECT_REQUEST_FLAGS = 0xd9u,
    EFFECT_SCRIPT_SLOTS = WRAM_BATTLE_EFFECT_SCRIPT_SLOTS & 0xffffu,
    EFFECT_SCRIPT_STRIDE = 0x2cu,
    EFFECT_SCRIPT_COUNT = WRAM_BATTLE_EFFECT_SCRIPT_SLOTS_COUNT,
    EFFECT_ACTOR_SLOTS = WRAM_BATTLE_EFFECT_ACTOR_SLOTS & 0xffffu,
    EFFECT_ACTOR_STRIDE = 0x2du,
    EFFECT_ACTOR_COUNT = WRAM_BATTLE_EFFECT_ACTOR_SLOTS_COUNT,
    EFFECT_ACTIVE_COUNT = WRAM_BATTLE_EFFECT_ACTIVE_COUNT,
    EFFECT_FRAME_COUNTER = WRAM_BATTLE_EFFECT_FRAME_COUNTER,
    EFFECT_SLOT_COUNTER = WRAM_BATTLE_EFFECT_SLOT_COUNTER,
    EFFECT_SAVED_SPRITE_MODE = WRAM_BATTLE_EFFECT_SAVED_SPRITE_MODE,
    EFFECT_REQUEST_BG1 = WRAM_BATTLE_EFFECT_BG1_REQUEST,
    EFFECT_REQUEST_BG2 = WRAM_BATTLE_EFFECT_BG2_REQUEST,
    EFFECT_REQUEST_BG3 = WRAM_BATTLE_EFFECT_BG3_REQUEST,
    EFFECT_REQUEST_STATUS = WRAM_BATTLE_EFFECT_STATUS_REQUEST,
    EFFECT_UNK_7E15B1 = WRAM_UNK_7E15B1,
    EFFECT_BACKGROUND_HOLD = WRAM_BATTLE_BACKGROUND_MAP_STATE,
    EFFECT_BACKGROUND_REQUEST = WRAM_BATTLE_BACKGROUND_UPLOAD_REQUEST,
    EFFECT_DISPLAY_MODE = WRAM_BATTLE_DISPLAY_MODE,
    EFFECT_SLOT_ACTIVE = 0u,
    EFFECT_SLOT_TIMER = 1u,
    EFFECT_SLOT_STREAM = 3u,
    EFFECT_ACTOR_MOTION_TARGET = 0x25u,
    EFFECT_ACTOR_VELOCITY_X = 0x1bu,
    EFFECT_ACTOR_VELOCITY_Y = 0x1du,
    EFFECT_ACTOR_POSITION_X = 0x1fu,
    EFFECT_ACTOR_POSITION_Y = 0x21u,
    EFFECT_DISPATCH_LIMIT = 65536u,
    EFFECT_FRAME_LIMIT = 4096u,
    ROM_EFFECT_STREAMS = 0x958000u,
    ROM_EFFECT_HANDLERS = 0x8c5cu,
    ROM_EFFECT_BACKGROUND_DESTINATIONS = 0x97b554u
};

typedef struct EffectEngine {
    BattleContext battle;
    Lufia2ExecutionResult result;
} EffectEngine;

static bool EffectCall(EffectEngine *effect, uint16_t site, uint32_t target,
                       uint8_t frame_size, bool wide_accumulator) {
    Lufia2CpuState *cpu = effect->battle.cpu;

    if (!BattleCall(&effect->battle, site, target, frame_size)) {
        effect->result = BattleChildUnwound(&effect->battle);
        return false;
    }
    if (cpu->direct_page != 0u || cpu->index_is_8_bit ||
        cpu->accumulator_is_8_bit == wide_accumulator || cpu->decimal) {
        effect->result = ExecutionHandoff(cpu,
            0x810000u | (uint16_t)(site + frame_size + 1u));
        return false;
    }
    return true;
}

static uint32_t EffectRts(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const uint8_t low = Pull8(memory, cpu);
    const uint8_t high = Pull8(memory, cpu);
    return 0x810000u | (uint16_t)(((high << 8) | low) + 1u);
}

static void EffectClearRequest(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t address) {
    const uint8_t value = Read8(memory, address);
    cpu->zero = (value & A8(cpu)) == 0u;
    Write8(memory, address, (uint8_t)(value & (uint8_t)~A8(cpu)));
}

static Lufia2ExecutionResult EffectKnownCommand(
    EffectEngine *effect, uint32_t target, bool *known) {
    const Lufia2Memory *memory = effect->battle.memory;
    Lufia2CpuState *cpu = effect->battle.cpu;

    *known = true;
    switch (target) {
    case 0x81915bu: return Lufia2BattleEffectEnd(memory, cpu);
    case 0x819169u: return Lufia2BattleEffectYield(memory, cpu);
    case 0x81917fu: return Lufia2BattleEffectDelay(memory, cpu);
    case 0x81918fu: return Lufia2BattleEffectJump(memory, cpu);
    case 0x819198u: return Lufia2BattleEffectRepeatStart(memory, cpu);
    case 0x8191f2u: return Lufia2BattleEffectSpawnScriptAt(memory, cpu);
    case 0x81929eu: return Lufia2BattleEffectSpawnActorAt(memory, cpu);
    case 0x81920fu: return Lufia2BattleEffectSpawnScriptsForTargets(memory, cpu);
    case 0x8191b7u: return Lufia2BattleEffectBranchIfField(memory, cpu);
    case 0x8194cau: return Lufia2BattleEffectSelectPortraitStream(memory, cpu);
    case 0x819ba3u: return Lufia2BattleEffectSkipArgument(memory, cpu);
    case 0x8191adu: return Lufia2BattleEffectRepeatJump(memory, cpu);
    case 0x8194ebu: return Lufia2BattleEffectLoopStart(memory, cpu);
    case 0x819500u: return Lufia2BattleEffectLoopStart2(memory, cpu);
    case 0x819515u: return Lufia2BattleEffectLoopStart3(memory, cpu);
    case 0x81952au: return Lufia2BattleEffectLoopStart4(memory, cpu);
    case 0x81953fu: return Lufia2BattleEffectRepeat(memory, cpu);
    case 0x819553u: return Lufia2BattleEffectLoopNext2(memory, cpu);
    case 0x819567u: return Lufia2BattleEffectLoopNext3(memory, cpu);
    case 0x81957bu: return Lufia2BattleEffectLoopNext4(memory, cpu);
    case 0x81963au: return Lufia2BattleEffectVideoRegister(memory, cpu);
    case 0x819653u: return Lufia2BattleEffectBg3Map(memory, cpu);
    case 0x819999u: return Lufia2BattleEffectBackgroundRequest(memory, cpu);
    case 0x8199a5u: return Lufia2BattleEffectBackgroundRelease(memory, cpu);
    case 0x8199b0u: return Lufia2BattleEffectBackgroundCopy(memory, cpu);
    case 0x819ab1u: return Lufia2BattleEffectWindowBand(memory, cpu);
    case 0x8199d0u:
        return Lufia2BattleEffectGraphics(memory, cpu, effect->battle.child,
                                         effect->battle.child_context);
    case 0x819a12u:
        return Lufia2BattleEffectGraphicsAlternate(memory, cpu, effect->battle.child,
                                                  effect->battle.child_context);
    default:
        *known = false;
        return ExecutionReturned(target);
    }
}

static bool EffectRunSlot(EffectEngine *effect, bool actor) {
    const Lufia2Memory *memory = effect->battle.memory;
    Lufia2CpuState *cpu = effect->battle.cpu;
    const uint16_t dispatch = actor ? 0x8b15u : 0x8aa4u;
    const uint16_t stream_step = actor ? 0x8b08u : 0x8a95u;
    const uint32_t ordinary_return = 0x810000u | (uint16_t)(dispatch + 3u);
    const uint32_t yielded_return = actor ? 0x818b1au : 0x818aa9u;

    OpLdx(cpu, Read16AbsoluteIndexed(memory, cpu, EFFECT_SLOT_STREAM, cpu->y));
    OpWriteX(memory, cpu, OpDp(cpu, EFFECT_STREAM), cpu->x);
    Push8(memory, cpu, actor ? 0x8bu : 0x8au);
    Push8(memory, cpu, actor ? 0x19u : 0xa8u);
    for (unsigned commands = 0; commands < EFFECT_DISPATCH_LIMIT; ++commands) {
        uint32_t target;
        uint32_t landing;
        bool known;
        Lufia2ExecutionResult command;

        if (actor)
            TransferDirectToA(cpu);
        OpLoadA(cpu, Read8(memory, DirectLongPointer(memory, cpu, EFFECT_STREAM)));
        OpRepWidths(cpu, 0x20u);
        if (!actor)
            OpAndValue(cpu, 0xffu);
        OpAslA(cpu);
        OpTax(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, EFFECT_STREAM), 1);
        OpSepWidths(cpu, 0x20u);
        Push8(memory, cpu, cpu->program_bank);
        PullDataBank(memory, cpu);
        SimulateJsrFrame(memory, cpu, (uint16_t)(dispatch + 2u));
        target = 0x810000u | Read16ProgramIndexed(
            memory, cpu, ROM_EFFECT_HANDLERS, cpu->x);
        command = EffectKnownCommand(effect, target, &known);
        if (known) {
            if (command.flow != LUFIA2_EXECUTION_RETURNED) {
                effect->result = command;
                return false;
            }
            landing = EffectRts(memory, cpu);
        } else {
            if (!effect->battle.child(effect->battle.child_context, cpu,
                    target, 0x810000u | dispatch, 2u)) {
                effect->battle.unwind_site = 0x810000u | dispatch;
                effect->result = BattleChildUnwound(&effect->battle);
                return false;
            }
            landing = ordinary_return;
        }
        if (landing == yielded_return) {
            PullDataBank(memory, cpu);
            return true;
        }
        if (landing != ordinary_return || cpu->direct_page != 0u ||
            cpu->index_is_8_bit || !cpu->accumulator_is_8_bit || cpu->decimal) {
            effect->result = ExecutionHandoff(cpu, landing);
            return false;
        }
    }
    effect->result = ExecutionHandoff(cpu, 0x810000u | stream_step);
    return false;
}

static void EffectActorMotion(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbsY(cpu, EFFECT_ACTOR_MOTION_TARGET));
    if (cpu->negative)
        return;
    OpSta(memory, cpu, SNES_WRMPYA);
    OpLoadA(cpu, EFFECT_ACTOR_STRIDE);
    OpSta(memory, cpu, SNES_WRMPYB);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, EFFECT_ACTOR_SLOTS);
    cpu->carry = false;
    OpAdc(memory, cpu, SNES_RDMPYL);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, EFFECT_ACTOR_VELOCITY_X));
    cpu->carry = false;
    OpAdc(memory, cpu, OpAbsX(cpu, EFFECT_ACTOR_POSITION_X));
    OpSta(memory, cpu, OpAbsY(cpu, EFFECT_ACTOR_POSITION_X));
    OpLda(memory, cpu, OpAbsX(cpu, EFFECT_ACTOR_VELOCITY_Y));
    cpu->carry = false;
    OpAdc(memory, cpu, OpAbsX(cpu, EFFECT_ACTOR_POSITION_Y));
    OpSta(memory, cpu, OpAbsY(cpu, EFFECT_ACTOR_POSITION_Y));
    OpSepWidths(cpu, 0x20u);
}

static bool EffectRunSlots(EffectEngine *effect, bool actors) {
    const Lufia2Memory *memory = effect->battle.memory;
    Lufia2CpuState *cpu = effect->battle.cpu;

    OpLoadA(cpu, actors ? EFFECT_ACTOR_COUNT : EFFECT_SCRIPT_COUNT);
    OpSta(memory, cpu, OpAbs(cpu, EFFECT_SLOT_COUNTER));
    OpLdy(cpu, actors ? EFFECT_ACTOR_SLOTS : EFFECT_SCRIPT_SLOTS);
    do {
        OpLda(memory, cpu, OpAbsY(cpu, EFFECT_SLOT_ACTIVE));
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbsY(cpu, EFFECT_SLOT_TIMER));
            OpDecA(cpu);
            OpSta(memory, cpu, OpAbsY(cpu, EFFECT_SLOT_TIMER));
            if (cpu->zero) {
                PushDataBank(memory, cpu);
                if (actors)
                    EffectActorMotion(memory, cpu);
                if (!EffectRunSlot(effect, actors))
                    return false;
            }
        }
        OpRepWidths(cpu, 0x20u);
        OpTya(cpu);
        cpu->carry = false;
        OpAdcValue(cpu, actors ? EFFECT_ACTOR_STRIDE : EFFECT_SCRIPT_STRIDE);
        OpTay(cpu);
        OpSepWidths(cpu, 0x20u);
        OpStepMem(memory, cpu, OpAbs(cpu, EFFECT_SLOT_COUNTER), -1);
    } while (!cpu->zero);
    return true;
}

static bool EffectUploadChanges(EffectEngine *effect) {
    const Lufia2Memory *memory = effect->battle.memory;
    Lufia2CpuState *cpu = effect->battle.cpu;
    static const uint16_t requests[] = {
        EFFECT_REQUEST_STATUS, EFFECT_REQUEST_BG1,
        EFFECT_REQUEST_BG2, EFFECT_REQUEST_BG3
    };
    static const uint16_t sites[] = {0x8b36u, 0x8b45u, 0x8b54u, 0x8b63u};
    static const uint32_t uploaders[] = {0x859bdau, 0x859bf1u, 0x859af4u, 0x859c08u};

    if (!EffectCall(effect, 0x8b2au, 0x818e92u, 2u, false))
        return false;
    for (unsigned layer = 0; layer < 4u; ++layer) {
        OpLoadA(cpu, 0xffu);
        EffectClearRequest(memory, cpu, OpAbs(cpu, requests[layer]));
        if (!cpu->zero) {
            OpRepWidths(cpu, 0x20u);
            if (!EffectCall(effect, sites[layer], uploaders[layer], 3u, true))
                return false;
            OpSepWidths(cpu, 0x20u);
        }
    }
    OpLda(memory, cpu, OpAbs(cpu, EFFECT_BACKGROUND_REQUEST));
    if (!cpu->negative) {
        OpRepWidths(cpu, 0x20u);
        OpAndValue(cpu, 0xffu);
        OpAslA(cpu);
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, ROM_EFFECT_BACKGROUND_DESTINATIONS));
        PushAccumulator16(memory, cpu);
        if (!EffectCall(effect, 0x8b7au, 0x85ecdbu, 3u, true))
            return false;
        OpLoadA(cpu, 0x3800u);
        OpSta(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_VRAM_QUEUE + 2u));
        PullAccumulator16(memory, cpu);
        OpSta(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_VRAM_QUEUE + 4u));
        OpLoadA(cpu, 0x800u);
        OpSta(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_VRAM_QUEUE));
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpAbs(cpu, EFFECT_BACKGROUND_REQUEST));
    }
    OpLda(memory, cpu, OpAbs(cpu, EFFECT_BACKGROUND_HOLD));
    if (!cpu->zero) {
        OpStz(memory, cpu, OpAbs(cpu, EFFECT_BACKGROUND_HOLD));
        if (cpu->negative) {
            OpLoadA(cpu, 1u);
            EffectClearRequest(memory, cpu, OpDp(cpu, EFFECT_REQUEST_FLAGS));
            TransferDirectToA(cpu);
            OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E1B17));
            OpLoadA(cpu, 0xffu);
            OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E15B3));
        } else {
            OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_BACKGROUND_ID));
            if (cpu->zero) {
                PushDataBank(memory, cpu);
                OpSetDataBank(memory, cpu, 0x97u);
                if (!EffectCall(effect, 0x8bb8u, 0x85a701u, 3u, false))
                    return false;
                PullDataBank(memory, cpu);
            }
        }
    }
    if (!EffectCall(effect, 0x8bbdu, BATTLE_ROUTINE_SPRITES, 3u, false))
        return false;
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, BATTLE_SPRITE_REBUILD_REQUEST);
    if (!EffectCall(effect, 0x8bc7u, BATTLE_ROUTINE_FRAME_INPUT, 3u, false))
        return false;
    OpStepMem(memory, cpu, OpAbs(cpu, EFFECT_FRAME_COUNTER), 1);
    OpLda(memory, cpu, OpAbs(cpu, EFFECT_ACTIVE_COUNT));
    return true;
}

static bool EffectFinish(EffectEngine *effect) {
    const Lufia2Memory *memory = effect->battle.memory;
    Lufia2CpuState *cpu = effect->battle.cpu;

    OpSetDataBank(memory, cpu, 0x97u);
    OpLda(memory, cpu, OpAbs(cpu, EFFECT_SAVED_SPRITE_MODE));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_SPRITE_BUILD_MODE));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_SPRITE_BUILD_MODE));
    if (!EffectCall(effect, 0x8be3u, 0x858a39u, 3u, false) ||
        !EffectCall(effect, 0x8be7u, BATTLE_ROUTINE_SPRITES, 3u, false) ||
        !EffectCall(effect, 0x8bebu, BATTLE_ROUTINE_FRAME_INPUT, 3u, false))
        return false;
    OpLdx(cpu, 0x3000u);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_WMADDL), cpu->x);
    OpStz(memory, cpu, OpAbs(cpu, SNES_WMADDH));
    OpLdx(cpu, 0x800u);
    do {
        OpStz(memory, cpu, OpAbs(cpu, SNES_WMDATA));
        OpDex(cpu);
    } while (!cpu->zero);
    OpStz(memory, cpu, OpAbs(cpu, BATTLE_FRAME_STATE));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, BATTLE_SPRITE_REBUILD_REQUEST);
    OpLoadA(cpu, 0x97u);
    OpSta(memory, cpu, OpDp(cpu, 0x24u));
    OpLdy(cpu, 0xcd38u);
    OpLoadA(cpu, 8u);
    if (!EffectCall(effect, 0x8c13u, BATTLE_ROUTINE_LOAD_PALETTE, 2u, false))
        return false;
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0xffffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_NMI_SCROLL_REGISTERS + 10u));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_NMI_SCROLL_REGISTERS + 8u));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_NMI_SCROLL_REGISTERS + 6u));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_NMI_SCROLL_REGISTERS + 4u));
    if (!EffectCall(effect, 0x8c27u, 0x859c08u, 3u, true) ||
        !EffectCall(effect, 0x8c2bu, 0x859bf1u, 3u, true))
        return false;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E15B3));
    if (!EffectCall(effect, 0x8c36u, BATTLE_ROUTINE_COMMIT_PALETTES, 3u, false))
        return false;
    OpLoadA(cpu, 0x24u);
    EffectClearRequest(memory, cpu, OpDp(cpu, EFFECT_REQUEST_FLAGS));
    if (!EffectCall(effect, 0x8c3eu, 0x81c2c0u, 2u, false) ||
        !EffectCall(effect, 0x8c41u, 0x859aaau, 3u, false) ||
        !EffectCall(effect, 0x8c45u, BATTLE_ROUTINE_SPRITES, 3u, false) ||
        !EffectCall(effect, 0x8c49u, 0x859abcu, 3u, false) ||
        !EffectCall(effect, 0x8c4du, BATTLE_ROUTINE_FRAME_INPUT, 3u, false))
        return false;
    OpLoadA(cpu, 9u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_BGMODE));
    PullDataBank(memory, cpu);
    effect->result = ExecutionReturned(0x818c57u);
    return true;
}

static bool EffectInitialize(EffectEngine *effect) {
    const Lufia2Memory *memory = effect->battle.memory;
    Lufia2CpuState *cpu = effect->battle.cpu;

    PushDataBank(memory, cpu);
    PushAccumulator8(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpAbs(cpu, EFFECT_FRAME_COUNTER));
    OpLoadA(cpu, 9u);
    OpSta(memory, cpu, OpAbs(cpu, EFFECT_DISPLAY_MODE));
    if (!EffectCall(effect, 0x896au, 0x85ab98u, 3u, false))
        return false;
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E15B3));
    OpSta(memory, cpu, OpAbs(cpu, EFFECT_BACKGROUND_REQUEST));
    OpSetDataBank(memory, cpu, 0x7eu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_SPRITE_BUILD_MODE));
    OpSta(memory, cpu, OpAbs(cpu, EFFECT_SAVED_SPRITE_MODE));
    for (unsigned offset = 0; offset < 5u; ++offset)
        OpStz(memory, cpu, OpAbs(cpu, (uint16_t)(EFFECT_REQUEST_BG2 + offset)));
    OpStz(memory, cpu, OpAbs(cpu, EFFECT_BACKGROUND_HOLD));
    LoadA8(cpu, Pull8(memory, cpu));
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, ROM_EFFECT_STREAMS));
    OpSta(memory, cpu, 0x7e0000u | (EFFECT_SCRIPT_SLOTS + EFFECT_SLOT_STREAM));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x95u);
    OpSta(memory, cpu, OpDp(cpu, EFFECT_STREAM_BANK));
    OpLoadA(cpu, 1u);
    for (unsigned offset = 0; offset < 3u; ++offset)
        OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(EFFECT_SCRIPT_SLOTS + offset)));
    OpSta(memory, cpu, OpAbs(cpu, EFFECT_ACTIVE_COUNT));
    for (unsigned slot = 1; slot < EFFECT_SCRIPT_COUNT; ++slot)
        OpStz(memory, cpu, OpAbs(cpu,
            (uint16_t)(EFFECT_SCRIPT_SLOTS + slot * EFFECT_SCRIPT_STRIDE)));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E1BEB));
    OpRepWidths(cpu, 0x20u);
    for (unsigned offset = 0; offset < 16u; offset += 2u)
        OpStz(memory, cpu, OpAbs(cpu, (uint16_t)(WRAM_UNK_7E1BF1 + offset)));
    OpLoadA(cpu, 128u);
    OpSta(memory, cpu, OpAbs(cpu, EFFECT_SCRIPT_SLOTS + EFFECT_ACTOR_POSITION_X));
    OpLoadA(cpu, 100u);
    OpSta(memory, cpu, OpAbs(cpu, EFFECT_SCRIPT_SLOTS + EFFECT_ACTOR_POSITION_Y));
    OpStz(memory, cpu, OpAbs(cpu, EFFECT_SCRIPT_SLOTS + EFFECT_ACTOR_VELOCITY_X));
    OpStz(memory, cpu, OpAbs(cpu, EFFECT_SCRIPT_SLOTS + EFFECT_ACTOR_VELOCITY_Y));
    TransferDirectToA(cpu);
    do {
        OpTax(cpu);
        OpStz(memory, cpu, OpAbsX(cpu, EFFECT_ACTOR_SLOTS));
        cpu->carry = false;
        OpAdcValue(cpu, EFFECT_ACTOR_STRIDE);
        OpCmpValue(cpu, EFFECT_ACTOR_COUNT * EFFECT_ACTOR_STRIDE);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    if (!EffectCall(effect, 0x8a4cu, 0x859aaau, 3u, false) ||
        !EffectCall(effect, 0x8a50u, BATTLE_ROUTINE_SPRITES, 3u, false) ||
        !EffectCall(effect, 0x8a54u, 0x859abcu, 3u, false) ||
        !EffectCall(effect, 0x8a58u, BATTLE_ROUTINE_FRAME_INPUT, 3u, false))
        return false;
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    if (!EffectCall(effect, 0x8a5fu, BATTLE_ROUTINE_CLEAR_WINDOW_TILEMAP, 2u, false) ||
        !EffectCall(effect, 0x8a62u, 0x81c30eu, 2u, false))
        return false;
    PullDataBank(memory, cpu);
    return EffectCall(effect, 0x8a66u, 0x859aaau, 3u, false) &&
        EffectCall(effect, 0x8a6au, BATTLE_ROUTINE_SPRITES, 3u, false) &&
        EffectCall(effect, 0x8a6eu, 0x859abcu, 3u, false) &&
        EffectCall(effect, 0x8a72u, BATTLE_ROUTINE_FRAME_INPUT, 3u, false);
}

Lufia2ExecutionResult Lufia2BattlePlayEffect(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    EffectEngine effect = {
        BattleContextCreate(memory, cpu, child, child_context, 0x81u),
        ExecutionReturned(0x818c57u)
    };

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal ||
        cpu->direct_page != 0u || cpu->program_bank != 0x81u ||
        cpu->stack < 0x1f10u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x81895eu);
    if (!EffectInitialize(&effect))
        return effect.result;
    for (unsigned frame = 0; frame < EFFECT_FRAME_LIMIT; ++frame) {
        if (!EffectRunSlots(&effect, false) || !EffectRunSlots(&effect, true) ||
            !EffectUploadChanges(&effect))
            return effect.result;
        if (cpu->zero) {
            (void)EffectFinish(&effect);
            return effect.result;
        }
    }
    return ExecutionHandoff(cpu, 0x818a76u);
}
