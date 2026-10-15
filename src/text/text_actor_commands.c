#include "actor/actor_internal.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "text/text_internal.h"

enum {
    REQUEST_ACTOR_SCRIPT = 0xa6aeu,
    SET_SCRIPT_FACING = 0xa709u,
    SET_FACING_FROM_TABLE = 0xa794u,
    SET_OBJECT_BINDING = 0xa99du,
    REQUEST_IDLE_ACTOR = 0xafe7u,
    WAIT_ACTOR_MOTION = 0xb005u,
    WAIT_ALL_ACTOR_MOTION = 0xa6e7u,
    REFRESH_ACTOR_SCRIPTS = 0xa6c2u,
    RESTART_ACTOR_SCRIPT = 0xa7bbu,
    ACTOR_COUNT = 40u,
    ROM_FACING_TABLE = 0x80a7b6u,
    DP_MOTION_FLAGS = 0x54u
};

static void FindActor(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                      uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x80u, return_address);
    Lufia2TextFindWindowActor(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

static void ActorRecordOffsets(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                               uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x80u, return_address);
    cpu->program_bank = 0x84u;
    Lufia2ActorRecordOffsets(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    cpu->program_bank = 0x80u;
}

static void LoadPrimaryScript(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                              uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x80u, return_address);
    cpu->program_bank = 0x83u;
    Lufia2ActorLoadPrimaryScript(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    cpu->program_bank = 0x80u;
}

static void SelectActor(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                        uint16_t handler) {
    Lufia2TextNextByte(memory, cpu, (uint16_t)(handler + 2u));
    FindActor(memory, cpu, (uint16_t)(handler + 6u));
}

static void LoadActorSlot(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
}

static void RequestActorScript(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SelectActor(memory, cpu, REQUEST_ACTOR_SCRIPT);
    LoadActorSlot(memory, cpu);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
    OpOraValue(cpu, 1u);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
}

static void SetScriptFacing(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SelectActor(memory, cpu, SET_SCRIPT_FACING);
    Lufia2TextNextByte(memory, cpu, 0xa712u);
    LoadActorSlot(memory, cpu);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E066A));
}

static void SetFacingFromTable(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SelectActor(memory, cpu, SET_FACING_FROM_TABLE);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpAbsY(cpu, 0u));
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, ROM_FACING_TABLE));
    LoadActorSlot(memory, cpu);
    OpCmp(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E066A));
    if (!cpu->zero) {
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_FACING));
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E066A));
    }
    Lufia2TextNextByte(memory, cpu, 0xa7b3u);
}

static void SetObjectBinding(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SelectActor(memory, cpu, SET_OBJECT_BINDING);
    LoadActorSlot(memory, cpu);
    Lufia2TextNextByte(memory, cpu, 0xa9a8u);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_CLAIMED_PENDING_OBJECT));
    Lufia2TextNextByte(memory, cpu, 0xa9afu);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_CLAIMED_OBJECT_RECORD));
}

static Lufia2TextCommandFlow RequestIdleActor(const Lufia2Memory *memory,
                                              Lufia2CpuState *cpu) {
    SelectActor(memory, cpu, REQUEST_IDLE_ACTOR);
    LoadActorSlot(memory, cpu);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
    OpBitValue(cpu, 0x80u);
    if (cpu->zero) {
        OpOraValue(cpu, 1u);
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
        return LUFIA2_TEXT_COMMAND_NEXT;
    }
    Lufia2TextPrevByte(memory, cpu, 0xb001u);
    return LUFIA2_TEXT_COMMAND_YIELD;
}

static Lufia2TextCommandFlow WaitActorMotion(const Lufia2Memory *memory,
                                             Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbsY(cpu, 0u));
    FindActor(memory, cpu, 0xb00bu);
    LoadActorSlot(memory, cpu);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
    OpAndValue(cpu, 8u);
    if (cpu->zero) {
        Lufia2TextNextByte(memory, cpu, 0xb017u);
        return LUFIA2_TEXT_COMMAND_NEXT;
    }
    Lufia2TextPrevByte(memory, cpu, 0xb01du);
    return LUFIA2_TEXT_COMMAND_YIELD;
}

static Lufia2TextCommandFlow WaitAllActorMotion(const Lufia2Memory *memory,
                                                Lufia2CpuState *cpu) {
    OpLdx(cpu, ACTOR_COUNT - 1u);
    OpStz(memory, cpu, OpDp(cpu, DP_MOTION_FLAGS));
    OpLoadA(cpu, 0u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
        OpBitValue(cpu, 4u);
        if (cpu->zero)
            OpTestBits(memory, cpu, OpDp(cpu, DP_MOTION_FLAGS), true);
        OpDex(cpu);
    } while (!cpu->negative);
    OpLda(memory, cpu, OpDp(cpu, DP_MOTION_FLAGS));
    OpBitValue(cpu, 8u);
    if (cpu->zero)
        return LUFIA2_TEXT_COMMAND_NEXT;
    Lufia2TextPrevByte(memory, cpu, 0xa702u);
    return LUFIA2_TEXT_COMMAND_YIELD;
}

static void RefreshActorScripts(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 5u);
    do {
        PushIndex(memory, cpu);
        OpWriteX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT), cpu->x);
        ActorRecordOffsets(memory, cpu, 0xa6cbu);
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E070A));
        PushAccumulator8(memory, cpu);
        OpLoadA(cpu, 9u);
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E070A));
        LoadPrimaryScript(memory, cpu, 0xa6d8u);
        OpLoadA(cpu, Pull8(memory, cpu));
        cpu->x = PullIndexValue(memory, cpu);
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E070A));
        OpInx(cpu);
        OpCpx(cpu, ACTOR_COUNT);
    } while (!cpu->zero);
}

static void RestartAllActorScripts(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, ACTOR_COUNT - 1u);
    do {
        OpWriteX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT), cpu->x);
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
        OpBitValue(cpu, 1u);
        if (!cpu->zero) {
            OpAndValue(cpu, 0xfeu);
            OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
            ActorRecordOffsets(memory, cpu, 0xa7d6u);
            PushIndex(memory, cpu);
            LoadPrimaryScript(memory, cpu, 0xa7dbu);
            cpu->x = PullIndexValue(memory, cpu);
        }
        OpDex(cpu);
    } while (!cpu->negative);
    Lufia2TextNextByte(memory, cpu, 0xa7e2u);
}

static void RestartActorScript(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2TextNextByte(memory, cpu, 0xa7bdu);
    OpCmpValue(cpu, 0xffu);
    if (cpu->zero) {
        RestartAllActorScripts(memory, cpu);
        return;
    }
    FindActor(memory, cpu, 0xa7e9u);
    if (cpu->carry) {
        Lufia2TextNextByte(memory, cpu, 0xa7eeu);
        return;
    }
    LoadActorSlot(memory, cpu);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
    OpAndValue(cpu, 0xfeu);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
    Lufia2TextNextByte(memory, cpu, 0xa7feu);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E070A));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_PRIMARY_TIMER));
    LoadPrimaryScript(memory, cpu, 0xa80bu);
}

Lufia2TextCommandFlow Lufia2TextExecuteActorCommand(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu,
                                                    uint16_t handler) {
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal ||
        cpu->stack < 0x1f10u || cpu->stack > 0x1ffcu)
        return LUFIA2_TEXT_COMMAND_UNHANDLED;
    switch (handler) {
    case REQUEST_ACTOR_SCRIPT:
        RequestActorScript(memory, cpu);
        break;
    case SET_SCRIPT_FACING:
        SetScriptFacing(memory, cpu);
        break;
    case SET_FACING_FROM_TABLE:
        SetFacingFromTable(memory, cpu);
        break;
    case SET_OBJECT_BINDING:
        SetObjectBinding(memory, cpu);
        break;
    case REQUEST_IDLE_ACTOR:
        return RequestIdleActor(memory, cpu);
    case WAIT_ACTOR_MOTION:
        return WaitActorMotion(memory, cpu);
    case WAIT_ALL_ACTOR_MOTION:
        return WaitAllActorMotion(memory, cpu);
    case REFRESH_ACTOR_SCRIPTS:
        RefreshActorScripts(memory, cpu);
        break;
    case RESTART_ACTOR_SCRIPT:
        RestartActorScript(memory, cpu);
        break;
    default:
        return LUFIA2_TEXT_COMMAND_UNHANDLED;
    }
    return LUFIA2_TEXT_COMMAND_NEXT;
}
