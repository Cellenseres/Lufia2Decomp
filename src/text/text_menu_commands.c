#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "lufia2/title.h"
#include "system/wram.h"
#include "text/text_internal.h"

enum {
    REQUEST_MAP_CHANGE = 0xa2bau,
    OPEN_SCENE_MENU = 0xa32bu,
    OPEN_MAIN_MENU = 0xa372u,
    GIVE_ITEM_PAGE_ZERO = 0xa414u,
    GIVE_ITEM_PAGE_ONE = 0xa41fu,
    OPEN_SCRIPT_MENU = 0xa51du,
    START_SCRIPTED_BATTLE = 0xb909u,
    ITEM_REWARD_ID = 0x7fd4efu,
    ITEM_REWARD_PAGE = 0x7fd4f0u,
    ITEM_REWARD_COUNT = 0x7fd4f1u,
    DP_MENU_STATE = 0x30u,
    DP_MENU_PHASE = 0x31u,
    DP_MENU_FINISHED = 0x6au
};

typedef struct TextMenuService {
    const Lufia2Memory *memory;
    uint32_t handoff;
} TextMenuService;

static uint8_t RunMenuChild(void *context, Lufia2CpuState *cpu, uint32_t target,
                            uint32_t site, uint8_t frame) {
    TextMenuService *service = context;
    const Lufia2Memory *memory = service->memory;
    const uint8_t caller_bank = cpu->program_bank;
    cpu->program_bank = (uint8_t)(target >> 16);
    Lufia2ExecutionResult result;
    switch (target) {
    case 0x83afeau:
        result = Lufia2FieldFadeOut(memory, cpu, RunMenuChild, context);
        break;
    case 0x82e746u:
        result = Lufia2TitleStateDispatch(memory, cpu);
        break;
    case 0x8ec1c5u:
        result = Lufia2FieldCountValue2B(memory, cpu);
        break;
    case 0x83845bu:
        result = Lufia2EncounterBattleSequence(memory, cpu, RunMenuChild, context);
        break;
    default:
        service->handoff = target;
        return 0u;
    }
    if (result.flow != LUFIA2_EXECUTION_RETURNED) {
        if (!service->handoff)
            service->handoff = result.pc;
        return 0u;
    }
    uint16_t back = Pull8(memory, cpu);
    back |= (uint16_t)Pull8(memory, cpu) << 8;
    cpu->program_bank = frame == 3u ? Pull8(memory, cpu) : caller_bank;
    const uint32_t next = ((uint32_t)cpu->program_bank << 16) | (uint16_t)(back + 1u);
    if (next != site + frame + 1u) {
        service->handoff = next;
        return 0u;
    }
    return 1u;
}

static uint32_t CallMenuService(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                uint32_t target, uint16_t back) {
    SimulateJslFrame(memory, cpu, 0x80u, back);
    TextMenuService service = {memory, 0u};
    RunMenuChild(&service, cpu, target, 0x800000u | (uint16_t)(back - 3u), 3u);
    return service.handoff;
}

static uint32_t RequestMapChange(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2TextNextByte(memory, cpu, 0xa2bcu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_DESTINATION_MAP));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FIELD_DESTINATION_PARAMETERS));
    Lufia2TextNextByte(memory, cpu, 0xa2c5u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E05B8));
    Lufia2TextNextByte(memory, cpu, 0xa2cbu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_DESTINATION_X));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_RELOAD_FLAGS));
    OpLoadA(cpu, 0x88u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_FIELD_FLAGS), true);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_INTERACTION_FACING_ACTOR_ID));
    return LUFIA2_TEXT_COMMAND_YIELD;
}

static uint32_t OpenFieldMenu(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                              bool scene_menu) {
    if (scene_menu)
        Lufia2TextNextByte(memory, cpu, 0xa32du);
    OpWriteX(memory, cpu, OpAbs(cpu, TEXT_SCRIPT_POINTER), cpu->y);
    if (scene_menu) {
        OpSta(memory, cpu, OpDp(cpu, DP_MENU_STATE));
        OpStz(memory, cpu, OpAbs(cpu, WRAM_TEXT_STATE));
    } else {
        OpLoadA(cpu, 2u);
        OpSta(memory, cpu, OpDp(cpu, DP_MENU_STATE));
        OpStz(memory, cpu, OpDp(cpu, DP_MENU_PHASE));
    }
    uint32_t handoff =
        CallMenuService(memory, cpu, 0x83afeau, scene_menu ? 0xa339u : 0xa37eu);
    if (handoff)
        return handoff;
    OpStz(memory, cpu, OpDp(cpu, DP_MENU_FINISHED));
    if (scene_menu) {
        OpLoadA(cpu, 0x0fu);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E0563));
        handoff = CallMenuService(memory, cpu, 0x82d69cu, 0xa344u);
    } else {
        OpLoadA(cpu, 1u);
        OpTestBits(memory, cpu, OpAbs(cpu, WRAM_ACTOR_STATE), false);
        PushY(memory, cpu);
        handoff = CallMenuService(memory, cpu, 0x82e746u, 0xa38au);
    }
    return handoff ? handoff : scene_menu ? 0x80a345u : 0x80a38bu;
}

static uint32_t OpenScriptMenu(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    PushY(memory, cpu);
    uint32_t handoff = CallMenuService(memory, cpu, 0x83afeau, 0xa522u);
    if (handoff)
        return handoff;
    OpStz(memory, cpu, OpDp(cpu, DP_MENU_PHASE));
    OpLoadA(cpu, 7u);
    OpSta(memory, cpu, OpDp(cpu, DP_MENU_STATE));
    handoff = CallMenuService(memory, cpu, 0x82e746u, 0xa52cu);
    return handoff ? handoff : 0x80a52du;
}

static uint32_t GiveItem(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                         bool second_page) {
    if (second_page)
        OpLoadA(cpu, 1u);
    else
        LoadA16(cpu, cpu->direct_page);
    OpSta(memory, cpu, ITEM_REWARD_PAGE);
    SimulateJsrFrame(memory, cpu, second_page ? 0xa427u : 0xa41bu);
    Lufia2TextNextByte(memory, cpu, 0xa42du);
    OpSta(memory, cpu, ITEM_REWARD_ID);
    Lufia2TextNextByte(memory, cpu, 0xa434u);
    OpSta(memory, cpu, ITEM_REWARD_COUNT);
    PushY(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_ITEM_QUANTITY));
    OpLda(memory, cpu, ITEM_REWARD_ID);
    OpSta(memory, cpu, OpAbs(cpu, 0x09cfu));
    uint32_t handoff = CallMenuService(memory, cpu, 0x8ec1c5u, 0xa44cu);
    if (handoff)
        return handoff;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpDp(cpu, DP_MENU_PHASE));
    OpLoadA(cpu, 4u);
    OpSta(memory, cpu, OpDp(cpu, DP_MENU_STATE));
    handoff = CallMenuService(memory, cpu, 0x82e746u, 0xa45au);
    return handoff ? handoff : 0x80a45bu;
}

static uint32_t StartBattle(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, 0x0bbbu));
    OpSta(memory, cpu, 0x7fd4f7u);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, 0x7ff8a3u);
    Lufia2TextNextByte(memory, cpu, 0xb918u);
    OpSta(memory, cpu, 0x7ff8a4u);
    const uint32_t handoff = CallMenuService(memory, cpu, 0x83845bu, 0xb920u);
    return handoff ? handoff : 0x80b921u;
}

uint32_t Lufia2TextExecuteMenuCommand(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                      uint16_t handler) {
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal ||
        cpu->stack < 0x1f10u || cpu->stack > 0x1ffcu)
        return 0u;
    switch (handler) {
    case REQUEST_MAP_CHANGE:
        return RequestMapChange(memory, cpu);
    case OPEN_SCENE_MENU:
    case OPEN_MAIN_MENU:
        return OpenFieldMenu(memory, cpu, handler == OPEN_SCENE_MENU);
    case GIVE_ITEM_PAGE_ZERO:
    case GIVE_ITEM_PAGE_ONE:
        return GiveItem(memory, cpu, handler == GIVE_ITEM_PAGE_ONE);
    case OPEN_SCRIPT_MENU:
        return OpenScriptMenu(memory, cpu);
    case START_SCRIPTED_BATTLE:
        return StartBattle(memory, cpu);
    default:
        return 0u;
    }
}
