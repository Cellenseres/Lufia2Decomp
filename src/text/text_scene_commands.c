#include "core/cpu_ops.h"
#include "field/event_script_internal.h"
#include "lufia2/field.h"
#include "system/wram.h"
#include "text/text_internal.h"

enum {
    PLACE_ACTOR_AT_SCENE_CELL = 0xa72eu,
    SET_SCENE_TRANSITION = 0xac48u,
    SET_SCENE_DESTINATION = 0xbdf1u,
    ROM_HEADER_SEARCH = 0x80bfaau,
    DP_POSITION_X = 0x54u,
    DP_POSITION_Y = 0x56u,
    DP_DESTINATION_PARAMETERS = 0x54u
};

typedef struct SceneCellSearch {
    const Lufia2Memory *memory;
    uint32_t handoff;
} SceneCellSearch;

static uint8_t SearchSceneCell(void *context, Lufia2CpuState *cpu, uint32_t target,
                               uint32_t site, uint8_t frame) {
    SceneCellSearch *search = context;
    (void)site;
    (void)frame;
    if (target != ROM_HEADER_SEARCH) {
        search->handoff = target;
        return 0u;
    }
    const Lufia2ExecutionResult result =
        Lufia2FieldFindHeaderRecord(search->memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED) {
        search->handoff = result.pc;
        return 0u;
    }
    SimulateRtlFrame(search->memory, cpu);
    return 1u;
}

uint32_t Lufia2TextReadSceneCell(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                              uint16_t return_address) {
    SceneCellSearch search = {memory, 0u};
    SimulateJslFrame(memory, cpu, 0x80u, return_address);
    const Lufia2ExecutionResult result =
        Lufia2SceneReadActorCell(memory, cpu, SearchSceneCell, &search);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return search.handoff ? search.handoff : result.pc;
    SimulateRtlFrame(memory, cpu);
    return 0u;
}

static void PlaceActorAtSceneCell(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E120A));
    OpSta(memory, cpu, OpDp(cpu, DP_POSITION_X));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E120B));
    OpSta(memory, cpu, OpDp(cpu, DP_POSITION_Y));
    Lufia2EventMoveActorTo(memory, cpu, 0xa749u);
}

static void SetSceneTransition(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E120A));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E05BD));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E120B));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E05BE));
    Lufia2TextNextByte(memory, cpu, 0xac5du);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E05BF));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, WRAM_UNK_7FD0BF);
}

static void SetSceneDestination(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E120A));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_DESTINATION_X));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E120B));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_DESTINATION_Y));
    Lufia2TextNextByte(memory, cpu, 0xbe06u);
    for (unsigned bit = 0; bit < 4u; ++bit)
        OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_DESTINATION_PARAMETERS));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_DESTINATION_PARAMETERS));
    OpAndValue(cpu, 0x0fu);
    OpOra(memory, cpu, OpDp(cpu, DP_DESTINATION_PARAMETERS));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_DESTINATION_PARAMETERS));
}

uint32_t Lufia2TextExecuteSceneCommand(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                       uint16_t handler) {
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal ||
        cpu->stack < 0x1f10u || cpu->stack > 0x1ffcu)
        return 0u;
    uint16_t return_address;
    switch (handler) {
    case PLACE_ACTOR_AT_SCENE_CELL:
        Lufia2TextNextByte(memory, cpu, 0xa730u);
        SimulateJslFrame(memory, cpu, 0x80u, 0xa734u);
        Lufia2TextFindWindowActor(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        Lufia2TextNextByte(memory, cpu, 0xa737u);
        return_address = 0xa73bu;
        break;
    case SET_SCENE_TRANSITION:
    case SET_SCENE_DESTINATION:
        Lufia2TextNextByte(memory, cpu, (uint16_t)(handler + 2u));
        return_address = (uint16_t)(handler + 6u);
        break;
    default:
        return 0u;
    }
    const uint32_t handoff = Lufia2TextReadSceneCell(memory, cpu, return_address);
    if (handoff)
        return handoff;
    if (handler == PLACE_ACTOR_AT_SCENE_CELL)
        PlaceActorAtSceneCell(memory, cpu);
    else if (handler == SET_SCENE_TRANSITION)
        SetSceneTransition(memory, cpu);
    else
        SetSceneDestination(memory, cpu);
    return LUFIA2_TEXT_COMMAND_NEXT;
}
