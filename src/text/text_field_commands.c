#include "actor/actor_internal.h"
#include "core/cpu_ops.h"
#include "field/event_script_internal.h"
#include "lufia2/field.h"
#include "system/wram.h"
#include "text/text_internal.h"

enum {
    RADIAL_AT_ACTOR_3C = 0xa9b7u,
    RADIAL_AT_SCENE_3C = 0xa9beu,
    RADIAL_AT_ACTOR_4F = 0xa9c5u,
    RADIAL_AT_SCENE_4F = 0xa9ccu,
    RADIAL_AT_SCENE_CUSTOM = 0xa9d3u,
    RADIAL_AT_ACTOR_CUSTOM = 0xa9dbu,
    SPAWN_AT_SCENE = 0xb257u,
    SPAWN_AT_ACTOR = 0xb269u,
    DIRECTIONAL_ACTOR_ACTION = 0xa936u,
    ACTOR_ACTION = 0xa94eu,
    LINKED_ACTOR_ACTION = 0xa95cu,
    ACTOR_ACTION_61 = 0xb054u,
    ACTOR_ACTION_63 = 0xb066u,
    SCENE_FINE_POSITION = WRAM_FIELD_LAST_PENDING_OBJECT,
    DP_ACTION = 0x54u,
    DP_LINKED_ACTION = 0x58u,
    DP_LINKED_SLOT = 0x59u,
    DP_SPAWN_WORD_OFFSET = 0x58u,
    DP_RADIAL_COUNT = 0x00u,
    DP_RADIAL_MODE = 0x01u,
    DP_RADIAL_X = 0x02u,
    DP_RADIAL_Y = 0x04u,
    DP_RADIAL_ZOOM = 0x06u,
    DP_RADIAL_ANGLE_A = 0x08u,
    DP_RADIAL_ANGLE_B = 0x09u,
    DP_RADIAL_SPAWN_ID = 0x0au,
    ROM_FACING_ACTION_OFFSETS = 0x83c1a5u
};

typedef struct TextObjectSpawn {
    const Lufia2Memory *memory;
} TextObjectSpawn;

static void FindActor(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                      uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x80u, return_address);
    Lufia2TextFindWindowActor(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

static void SpawnObject(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                        uint8_t return_bank, uint16_t return_address) {
    SimulateJslFrame(memory, cpu, return_bank, return_address);
    cpu->program_bank = 0x83u;
    Lufia2ActorSpawn(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    cpu->program_bank = return_bank;
}

static uint8_t SpawnRadialObject(void *context, Lufia2CpuState *cpu, uint32_t target,
                                 uint32_t site, uint8_t frame) {
    const TextObjectSpawn *spawn = context;
    (void)target;
    (void)site;
    (void)frame;
    cpu->program_bank = 0x83u;
    Lufia2ActorSpawn(spawn->memory, cpu);
    SimulateRtlFrame(spawn->memory, cpu);
    return 1u;
}

void Lufia2TextScaleSceneCoordinates(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                  uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x30u);
    OpLdx(cpu, 3u);
    OpLdy(cpu, 6u);
    do {
        OpLoadA(cpu, 0u);
        ExchangeAccumulatorBytes(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E120A));
        OpRepWidths(cpu, 0x20u);
        for (unsigned bit = 0; bit < 4u; ++bit)
            OpAslA(cpu);
        OpSta(memory, cpu, OpAbsY(cpu, SCENE_FINE_POSITION));
        OpSepWidths(cpu, 0x20u);
        OpDey(cpu);
        OpDey(cpu);
        OpDex(cpu);
    } while (!cpu->negative);
    UnpackStatus(cpu, Pull8(memory, cpu));
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

static uint32_t ReadRadialOrigin(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                 uint16_t return_address, bool scene) {
    SimulateJsrFrame(memory, cpu, return_address);
    Lufia2TextNextByte(memory, cpu, scene ? 0xaa04u : 0xa9e5u);
    OpSta(memory, cpu, OpDp(cpu, DP_RADIAL_MODE));
    Lufia2TextNextByte(memory, cpu, scene ? 0xaa09u : 0xa9eau);
    if (scene) {
        const uint32_t handoff = Lufia2TextReadSceneCell(memory, cpu, 0xaa0du);
        if (handoff)
            return handoff;
        Lufia2TextScaleSceneCoordinates(memory, cpu, 0xaa10u);
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbs(cpu, SCENE_FINE_POSITION));
        OpSta(memory, cpu, OpDp(cpu, DP_RADIAL_X));
        OpLda(memory, cpu, OpAbs(cpu, SCENE_FINE_POSITION + 2u));
        OpSta(memory, cpu, OpDp(cpu, DP_RADIAL_Y));
    } else {
        FindActor(memory, cpu, 0xa9eeu);
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SLOT_WORD_OFFSET)));
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpLongX(cpu, WRAM_ACTOR_FINE_X));
        OpSta(memory, cpu, OpDp(cpu, DP_RADIAL_X));
        OpLda(memory, cpu, OpLongX(cpu, WRAM_ACTOR_FINE_Y));
        OpSta(memory, cpu, OpDp(cpu, DP_RADIAL_Y));
    }
    OpSepWidths(cpu, 0x20u);
    SimulateRtsFrame(memory, cpu);
    return 0u;
}

static uint32_t SpawnRadialObjects(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                   uint16_t handler) {
    const bool scene = handler == RADIAL_AT_SCENE_3C || handler == RADIAL_AT_SCENE_4F ||
                       handler == RADIAL_AT_SCENE_CUSTOM;
    uint32_t handoff = ReadRadialOrigin(memory, cpu, (uint16_t)(handler + 2u), scene);
    if (handoff)
        return handoff;
    if (handler == RADIAL_AT_SCENE_CUSTOM || handler == RADIAL_AT_ACTOR_CUSTOM)
        Lufia2TextNextByte(memory, cpu, (uint16_t)(handler + 5u));
    else
        OpLoadA(cpu, handler == RADIAL_AT_ACTOR_3C || handler == RADIAL_AT_SCENE_3C
                         ? 0x3cu
                         : 0x4fu);
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpDp(cpu, DP_RADIAL_SPAWN_ID));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SLOT_WORD_OFFSET)));
    Lufia2TextNextByte(memory, cpu, 0xaa28u);
    OpSta(memory, cpu, OpDp(cpu, DP_RADIAL_COUNT));
    Lufia2TextNextByte(memory, cpu, 0xaa2du);
    OpSta(memory, cpu, OpDp(cpu, DP_RADIAL_ZOOM));
    Lufia2TextNextByte(memory, cpu, 0xaa32u);
    OpSta(memory, cpu, OpDp(cpu, DP_RADIAL_ANGLE_A));
    Lufia2TextNextByte(memory, cpu, 0xaa37u);
    OpSta(memory, cpu, OpDp(cpu, DP_RADIAL_ANGLE_B));
    PushY(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x80u, 0xaa3eu);
    cpu->program_bank = 0x83u;
    TextObjectSpawn spawn = {memory};
    const Lufia2ExecutionResult result =
        Lufia2FieldSpawnRadialObjects(memory, cpu, SpawnRadialObject, &spawn);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result.pc;
    SimulateRtlFrame(memory, cpu);
    cpu->program_bank = 0x80u;
    OpPullY(memory, cpu);
    return LUFIA2_TEXT_COMMAND_NEXT;
}

static uint32_t SpawnObjectAtPosition(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                      uint16_t handler) {
    Lufia2TextNextByte(memory, cpu, (uint16_t)(handler + 2u));
    PushY(memory, cpu);
    SpawnObject(memory, cpu, 0x80u, (uint16_t)(handler + 7u));
    OpPullY(memory, cpu);
    if (handler == SPAWN_AT_SCENE) {
        Lufia2TextNextByte(memory, cpu, 0xb262u);
        const uint32_t handoff = Lufia2TextReadSceneCell(memory, cpu, 0xb266u);
        if (handoff)
            return handoff;
    } else {
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SLOT_WORD_OFFSET)));
        OpWriteX(memory, cpu, OpDp(cpu, DP_SPAWN_WORD_OFFSET), cpu->x);
        Lufia2TextNextByte(memory, cpu, 0xb278u);
        Lufia2EventFindActorId(memory, cpu, 0xb27cu);
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_X));
        OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E120A));
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_Y));
        OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E120B));
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SPAWN_WORD_OFFSET)));
        OpWriteX(memory, cpu, OpDp(cpu, DP_SLOT_WORD_OFFSET), cpu->x);
    }
    Lufia2TextScaleSceneCoordinates(memory, cpu, 0xb28fu);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SLOT_WORD_OFFSET)));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, SCENE_FINE_POSITION));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_FINE_X));
    OpLda(memory, cpu, OpAbs(cpu, SCENE_FINE_POSITION + 2u));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_FINE_Y));
    OpSepWidths(cpu, 0x20u);
    return LUFIA2_TEXT_COMMAND_NEXT;
}

static void ReadActorAction(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                            uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Lufia2TextNextByte(memory, cpu, 0xa988u);
    FindActor(memory, cpu, 0xa98cu);
    Lufia2TextNextByte(memory, cpu, 0xa98fu);
    OpSta(memory, cpu, OpDp(cpu, DP_ACTION));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
    OpAndValue(cpu, 0xfeu);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
    SimulateRtsFrame(memory, cpu);
}

static uint32_t StartActorAction(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                 uint16_t handler) {
    uint16_t return_address;
    if (handler == ACTOR_ACTION || handler == DIRECTIONAL_ACTOR_ACTION) {
        ReadActorAction(memory, cpu, (uint16_t)(handler + 2u));
        if (handler == DIRECTIONAL_ACTOR_ACTION) {
            TransferDirectToA(cpu);
            OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_FACING));
            OpTax(cpu);
            OpLda(memory, cpu, OpLongX(cpu, ROM_FACING_ACTION_OFFSETS));
            cpu->carry = 0u;
            OpAdc(memory, cpu, OpDp(cpu, DP_ACTION));
            return_address = 0xa949u;
        } else {
            OpLda(memory, cpu, OpDp(cpu, DP_ACTION));
            return_address = 0xa957u;
        }
    } else if (handler == LINKED_ACTOR_ACTION) {
        Lufia2TextNextByte(memory, cpu, 0xa95eu);
        OpSta(memory, cpu, OpDp(cpu, DP_LINKED_ACTION));
        Lufia2TextNextByte(memory, cpu, 0xa963u);
        FindActor(memory, cpu, 0xa967u);
        OpLda(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT));
        OpSta(memory, cpu, OpDp(cpu, DP_LINKED_SLOT));
        Lufia2TextNextByte(memory, cpu, 0xa96eu);
        FindActor(memory, cpu, 0xa972u);
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
        OpLda(memory, cpu, OpDp(cpu, DP_LINKED_SLOT));
        OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_CLAIMED_OBJECT_RECORD));
        OpLda(memory, cpu, OpDp(cpu, DP_LINKED_ACTION));
        return_address = 0xa981u;
    } else {
        Lufia2TextNextByte(memory, cpu, (uint16_t)(handler + 2u));
        FindActor(memory, cpu, (uint16_t)(handler + 6u));
        return_address = (uint16_t)(handler + 13u);
    }
    PushY(memory, cpu);
    if (handler == ACTOR_ACTION_61 || handler == ACTOR_ACTION_63)
        OpLoadA(cpu, handler == ACTOR_ACTION_61 ? 0x61u : 0x63u);
    uint32_t handoff;
    if (!Lufia2EventActorAction(memory, cpu, return_address, &handoff))
        return handoff;
    OpPullY(memory, cpu);
    return LUFIA2_TEXT_COMMAND_NEXT;
}

uint32_t Lufia2TextExecuteFieldCommand(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                       uint16_t handler) {
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal ||
        cpu->stack < 0x1f10u || cpu->stack > 0x1ffcu)
        return 0u;
    switch (handler) {
    case RADIAL_AT_ACTOR_3C:
    case RADIAL_AT_SCENE_3C:
    case RADIAL_AT_ACTOR_4F:
    case RADIAL_AT_SCENE_4F:
    case RADIAL_AT_SCENE_CUSTOM:
    case RADIAL_AT_ACTOR_CUSTOM:
        return SpawnRadialObjects(memory, cpu, handler);
    case SPAWN_AT_SCENE:
    case SPAWN_AT_ACTOR:
        return SpawnObjectAtPosition(memory, cpu, handler);
    case DIRECTIONAL_ACTOR_ACTION:
    case ACTOR_ACTION:
    case LINKED_ACTOR_ACTION:
    case ACTOR_ACTION_61:
    case ACTOR_ACTION_63:
        return StartActorAction(memory, cpu, handler);
    default:
        return 0u;
    }
}
