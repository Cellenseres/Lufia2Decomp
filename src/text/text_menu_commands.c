#include <stddef.h>

#include "core/cpu_ops.h"
#include "core/child_call.h"
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
    OPEN_PARAMETERIZED_MENU = 0xbd99u,
    ITEM_REWARD_ID = 0x7fd4efu,
    ITEM_REWARD_PAGE = 0x7fd4f0u,
    ITEM_REWARD_COUNT = 0x7fd4f1u,
    ITEM_REWARD_MENU_VALUE = WRAM_MENU_SELECTED_SPELL,
    ITEM_REWARD_STEP_LIMIT = 4096u,
    DP_MENU_STATE = 0x30u,
    DP_MENU_PHASE = 0x31u,
    DP_MENU_FINISHED = 0x6au
};

typedef struct TextMenuService {
    const Lufia2Memory *memory;
    uint32_t handoff;
    Lufia2PushedChildCall child;
    void *context;
    bool unwound;
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

static uint32_t MenuCall(TextMenuService *service, Lufia2CpuState *cpu, uint32_t target,
                         uint32_t site, uint8_t frame) {
    service->handoff = 0u;
    if (service->child) {
        if (!CallChildWithFrame(service->memory, cpu, service->child, service->context,
                                site, target, frame, 0x80u)) {
            service->unwound = true;
            service->handoff = site;
        }
    } else {
        if (frame == 3u)
            SimulateJslFrame(service->memory, cpu, 0x80u, (uint16_t)(site + 3u));
        else
            SimulateJsrFrame(service->memory, cpu, (uint16_t)(site + 2u));
        RunMenuChild(service, cpu, target, site, frame);
    }
    return service->handoff;
}

static uint32_t CallMenuService(TextMenuService *service, Lufia2CpuState *cpu,
                                uint32_t target, uint16_t back) {
    return MenuCall(service, cpu, target, 0x800000u | (uint16_t)(back - 3u), 3u);
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
                              TextMenuService *service, bool scene_menu) {
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
        CallMenuService(service, cpu, 0x83afeau, scene_menu ? 0xa339u : 0xa37eu);
    if (handoff)
        return handoff;
    OpStz(memory, cpu, OpDp(cpu, DP_MENU_FINISHED));
    if (service->child && !cpu->accumulator_is_8_bit)
        return scene_menu ? 0x80a33cu : 0x80a381u;
    if (scene_menu) {
        OpLoadA(cpu, 0x0fu);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E0563));
        handoff = CallMenuService(service, cpu, 0x82d69cu, 0xa344u);
    } else {
        OpLoadA(cpu, 1u);
        OpTestBits(memory, cpu, OpAbs(cpu, WRAM_ACTOR_STATE), false);
        PushY(memory, cpu);
        handoff = CallMenuService(service, cpu, 0x82e746u, 0xa38au);
    }
    if (handoff)
        return handoff;
    if (!service->child)
        return scene_menu ? 0x80a345u : 0x80a38bu;
    if (scene_menu) {
        handoff = MenuCall(service, cpu, 0x80a350u, 0x80a345u, 2u);
        if (handoff)
            return handoff;
        if (!cpu->accumulator_is_8_bit)
            return 0x80a348u;
        OpLoadA(cpu, 1u);
        OpTestBits(memory, cpu, OpAbs(cpu, WRAM_ACTOR_STATE), false);
    } else {
        OpPullY(memory, cpu);
        handoff = MenuCall(service, cpu, 0x80a350u, 0x80a38cu, 2u);
        if (handoff)
            return handoff;
    }
    return LUFIA2_TEXT_COMMAND_YIELD;
}

static uint32_t OpenScriptMenu(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                               TextMenuService *service) {
    PushDataBank(memory, cpu);
    PushY(memory, cpu);
    uint32_t handoff = CallMenuService(service, cpu, 0x83afeau, 0xa522u);
    if (handoff)
        return handoff;
    OpStz(memory, cpu, OpDp(cpu, DP_MENU_PHASE));
    if (service->child && !cpu->accumulator_is_8_bit)
        return 0x80a525u;
    OpLoadA(cpu, 7u);
    OpSta(memory, cpu, OpDp(cpu, DP_MENU_STATE));
    handoff = CallMenuService(service, cpu, 0x82e746u, 0xa52cu);
    if (handoff || !service->child)
        return handoff ? handoff : 0x80a52du;
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    if (!cpu->accumulator_is_8_bit)
        return 0x80a52fu;
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, WRAM_SCENE_RELOAD_UNK_D0C2);
    OpSta(memory, cpu, WRAM_SCENE_RELOAD_UNK_D0C2 + 1u);
    handoff = MenuCall(service, cpu, 0x8385dcu, 0x80a539u, 3u);
    if (handoff)
        return handoff;
    handoff = MenuCall(service, cpu, 0x80a368u, 0x80a53du, 2u);
    if (handoff)
        return handoff;
    OpPullY(memory, cpu);
    PullDataBank(memory, cpu);
    if (!cpu->accumulator_is_8_bit)
        return 0x80a542u;
    OpLoadA(cpu, 1u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_ACTOR_STATE), true);
    return LUFIA2_TEXT_COMMAND_NEXT;
}

static uint32_t OpenParameterizedMenu(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                      TextMenuService *service) {
    Lufia2TextNextByte(memory, cpu, 0xbd9bu);
    OpSta(memory, cpu, OpDp(cpu, DP_MENU_PHASE));
    PushDataBank(memory, cpu);
    PushY(memory, cpu);
    OpLoadA(cpu, 3u);
    OpSta(memory, cpu, OpDp(cpu, DP_MENU_STATE));
    uint32_t handoff = CallMenuService(service, cpu, 0x83afeau, 0xbda7u);
    if (handoff)
        return handoff;
    OpStz(memory, cpu, OpDp(cpu, DP_MENU_FINISHED));
    handoff = CallMenuService(service, cpu, 0x82e746u, 0xbdadu);
    if (handoff || !service->child)
        return handoff ? handoff : 0x80bdaeu;
    handoff = MenuCall(service, cpu, 0x80a350u, 0x80bdaeu, 2u);
    if (handoff)
        return handoff;
    OpPullY(memory, cpu);
    PullDataBank(memory, cpu);
    if (!cpu->accumulator_is_8_bit)
        return 0x80bdb3u;
    OpLoadA(cpu, 1u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_ACTOR_STATE), true);
    return LUFIA2_TEXT_COMMAND_NEXT;
}

static uint32_t GiveItem(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                         TextMenuService *service, bool second_page) {
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
    for (unsigned rewards = 0u; rewards < ITEM_REWARD_STEP_LIMIT; ++rewards) {
        OpRepWidths(cpu, 0x20u);
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_ITEM_QUANTITY));
        OpLda(memory, cpu, ITEM_REWARD_ID);
        OpSta(memory, cpu, OpAbs(cpu, ITEM_REWARD_MENU_VALUE));
        uint32_t handoff = CallMenuService(service, cpu, 0x8ec1c5u, 0xa44cu);
        if (handoff)
            return handoff;
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, OpDp(cpu, DP_MENU_PHASE));
        OpLoadA(cpu, 4u);
        OpSta(memory, cpu, OpDp(cpu, DP_MENU_STATE));
        handoff = CallMenuService(service, cpu, 0x82e746u, 0xa45au);
        if (handoff || !service->child)
            return handoff ? handoff : 0x80a45bu;
        LoadA16(cpu, cpu->direct_page);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E0562));
        if (!cpu->accumulator_is_8_bit)
            return 0x80a45fu;
        OpLoadA(cpu, 0x0fu);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E0563));
        OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, ITEM_REWARD_MENU_VALUE)));
        if (cpu->negative) {
            PushDataBank(memory, cpu);
            handoff = MenuCall(service, cpu, 0x80a350u, 0x80a46au, 2u);
            if (handoff)
                return handoff;
            PullDataBank(memory, cpu);
        }
        OpLda(memory, cpu, ITEM_REWARD_COUNT);
        OpDecA(cpu);
        OpSta(memory, cpu, ITEM_REWARD_COUNT);
        if (!cpu->zero)
            continue;
        if (!cpu->accumulator_is_8_bit)
            return 0x80a479u;
        OpLoadA(cpu, 0x80u);
        OpTestBits(memory, cpu, OpAbs(cpu, WRAM_TEXT_STATE), true);
        OpPullY(memory, cpu);
        uint16_t back = Pull8(memory, cpu);
        back |= (uint16_t)Pull8(memory, cpu) << 8;
        const uint32_t next =
            ((uint32_t)cpu->program_bank << 16) | (uint16_t)(back + 1u);
        if (next != (second_page ? 0x80a428u : 0x80a41cu))
            return next;
        return LUFIA2_TEXT_COMMAND_NEXT;
    }
    return 0x80a43au;
}

static uint32_t StartBattle(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                            TextMenuService *service) {
    OpLda(memory, cpu, OpAbs(cpu, 0x0bbbu));
    OpSta(memory, cpu, 0x7fd4f7u);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, 0x7ff8a3u);
    Lufia2TextNextByte(memory, cpu, 0xb918u);
    OpSta(memory, cpu, 0x7ff8a4u);
    uint32_t handoff = CallMenuService(service, cpu, 0x83845bu, 0xb920u);
    if (handoff || !service->child)
        return handoff ? handoff : 0x80b921u;
    handoff = MenuCall(service, cpu, 0x80a368u, 0x80b921u, 2u);
    if (handoff)
        return handoff;
    if (!cpu->accumulator_is_8_bit)
        return 0x80b924u;
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_TEXT_STATE));
    return LUFIA2_TEXT_COMMAND_NEXT;
}

static bool MenuContextValid(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x80u && cpu->accumulator_is_8_bit &&
           !cpu->index_is_8_bit && !cpu->direct_page && !cpu->decimal &&
           cpu->stack >= 0x1f10u && cpu->stack <= 0x1ffcu;
}

static uint32_t ExecuteMenuCommand(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                   uint16_t handler, TextMenuService *service) {
    if (!MenuContextValid(cpu))
        return 0u;
    switch (handler) {
    case OPEN_PARAMETERIZED_MENU:
        return OpenParameterizedMenu(memory, cpu, service);
    case REQUEST_MAP_CHANGE:
        return RequestMapChange(memory, cpu);
    case OPEN_SCENE_MENU:
    case OPEN_MAIN_MENU:
        return OpenFieldMenu(memory, cpu, service, handler == OPEN_SCENE_MENU);
    case GIVE_ITEM_PAGE_ZERO:
    case GIVE_ITEM_PAGE_ONE:
        return GiveItem(memory, cpu, service, handler == GIVE_ITEM_PAGE_ONE);
    case OPEN_SCRIPT_MENU:
        return OpenScriptMenu(memory, cpu, service);
    case START_SCRIPTED_BATTLE:
        return StartBattle(memory, cpu, service);
    default:
        return 0u;
    }
}

uint32_t Lufia2TextExecuteMenuCommand(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                      uint16_t handler) {
    TextMenuService service = {memory, 0u, NULL, NULL, false};
    return ExecuteMenuCommand(memory, cpu, handler, &service);
}

Lufia2ExecutionResult Lufia2TextRunMenuCommand(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu, uint16_t handler,
                                               Lufia2PushedChildCall child,
                                               void *context) {
    switch (handler) {
    case GIVE_ITEM_PAGE_ZERO:
    case GIVE_ITEM_PAGE_ONE:
    case OPEN_SCENE_MENU:
    case OPEN_MAIN_MENU:
    case OPEN_SCRIPT_MENU:
    case OPEN_PARAMETERIZED_MENU:
    case START_SCRIPTED_BATTLE:
        break;
    default:
        return ExecutionReturned(0u);
    }
    if (!child || !MenuContextValid(cpu))
        return ExecutionHandoff(cpu, 0x800000u | handler);
    TextMenuService service = {memory, 0u, child, context, false};
    const uint32_t flow = ExecuteMenuCommand(memory, cpu, handler, &service);
    if (service.unwound) {
        Lufia2ExecutionResult result = ExecutionReturned(service.handoff);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    if (flow == LUFIA2_TEXT_COMMAND_NEXT || flow == LUFIA2_TEXT_COMMAND_YIELD)
        return ExecutionReturned(flow);
    return ExecutionHandoff(cpu, flow);
}
