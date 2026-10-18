#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/menu.h"
#include "menu/menu_sprite_slots.h"
#include "system/wram.h"

enum {
    SCENARIO_PAGE = 0xdfu,
    SCENARIO_PAGE_COUNT = 4u,
    SCENARIO_MEMBER_COUNT = 7u,
    SCENARIO_MEMBER_RECORD = 0x0badu,
    SCENARIO_MEMBER_STRIDE = 0xbeu,
    SCENARIO_MAP_HEADER = 0xe1u,
    SCENARIO_TEXT_BANK = 0x5fu,
    SCENARIO_PALETTE_REQUEST = 0x73u,
    SCENARIO_FRAME_REQUEST = 0x74u,
    SCENARIO_SELECTION = 0x31u,
    SCENARIO_TEXT = 0xd623u,
    SCENARIO_PORTRAIT_POSITIONS = 0xd903u,
    SCENARIO_HEADER_SOURCE = 0x91b8b5u,
    SCENARIO_FIRST_LAYER = 0x2000u,
    SCENARIO_SECOND_LAYER = 0x3000u,
    SCENARIO_LAYER_BYTES = 0x0800u
};

typedef struct ScenarioView {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
} ScenarioView;

static bool ScenarioChild(ScenarioView *view, uint32_t site, uint32_t target,
                          uint8_t index_width) {
    Lufia2CpuState *cpu = view->cpu;
    if (!CallChildWithFrame(view->memory, cpu, view->child, view->context, site, target,
                            3u, 0x8eu)) {
        view->result =
            (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return false;
    }
    if (cpu->program_bank != 0x8eu || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit != index_width || cpu->direct_page || cpu->decimal) {
        view->result = ExecutionHandoff(cpu, site + 4u);
        return false;
    }
    return true;
}

static bool BuildScenarioPortraits(ScenarioView *view) {
    const Lufia2Memory *memory = view->memory;
    Lufia2CpuState *cpu = view->cpu;
    OpLdx(cpu, 0u);
    OpLdy(cpu, 0u);
    do {
        PushY(memory, cpu);
        OpSepWidths(cpu, 0x10u);
        OpLdy(cpu, 8u);
        if (!ScenarioChild(view, 0x8ee6b2u, 0x868cdau, 1u))
            return false;
        OpRepWidths(cpu, 0x10u);
        OpPullY(memory, cpu);
        PushDataBank(memory, cpu);
        OpSetDataBank(memory, cpu, 0x8eu);
        OpLda(memory, cpu, OpAbsY(cpu, SCENARIO_PORTRAIT_POSITIONS));
        OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_X_LOW));
        OpLda(memory, cpu, OpAbsY(cpu, SCENARIO_PORTRAIT_POSITIONS + 1u));
        OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_Y_LOW));
        OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_X_HIGH));
        OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_Y_HIGH));
        PullDataBank(memory, cpu);
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ACTIVE));
        OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_FRAME_INDEX));
        OpTxa(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, 7u);
        if (!ScenarioChild(view, 0x8ee6ddu, 0x868cf5u, 0u))
            return false;
        OpInx(cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpCpx(cpu, SCENARIO_MEMBER_COUNT);
    } while (!cpu->zero);
    return true;
}

static bool DrawScenarioPage(ScenarioView *view) {
    const Lufia2Memory *memory = view->memory;
    Lufia2CpuState *cpu = view->cpu;
    static const uint8_t pointers[] = {0u, 2u, 4u, 6u, 0x11u, 0x13u, 0x15u};
    for (unsigned member = 0u; member < SCENARIO_MEMBER_COUNT; ++member) {
        OpLdx(cpu,
              (uint16_t)(SCENARIO_MEMBER_RECORD + member * SCENARIO_MEMBER_STRIDE));
        OpWriteX(memory, cpu, OpDp(cpu, pointers[member]), cpu->x);
    }
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, SCENARIO_HEADER_SOURCE);
    OpSta(memory, cpu, OpDp(cpu, SCENARIO_MAP_HEADER));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, SCENARIO_TEXT_BANK));
    OpLdy(cpu, SCENARIO_TEXT);
    if (!ScenarioChild(view, 0x8ee67bu, 0x808878u, 0u))
        return false;
    OpLoadA(cpu, 0x88u);
    OpSta(memory, cpu, OpDp(cpu, SCENARIO_FRAME_REQUEST));
    return ScenarioChild(view, 0x8ee683u, 0x868b1cu, 0u);
}

static bool ClearScenarioLayers(ScenarioView *view) {
    const Lufia2Memory *memory = view->memory;
    Lufia2CpuState *cpu = view->cpu;
    OpRepWidths(cpu, 0x20u);
    PushDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0u);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, SCENARIO_FIRST_LAYER));
        OpStz(memory, cpu, OpAbsX(cpu, SCENARIO_SECOND_LAYER));
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, SCENARIO_LAYER_BYTES);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    return true;
}

static bool ScenarioLocal(ScenarioView *view, uint16_t site,
                          bool (*routine)(ScenarioView *)) {
    Lufia2CpuState *cpu = view->cpu;
    SimulateJsrFrame(view->memory, cpu, (uint16_t)(site + 2u));
    if (!routine(view))
        return false;
    uint16_t landing = Pull8(view->memory, cpu);
    landing |= (uint16_t)Pull8(view->memory, cpu) << 8;
    ++landing;
    if (landing != (uint16_t)(site + 3u)) {
        view->result = ExecutionHandoff(cpu, 0x8e0000u | landing);
        return false;
    }
    return true;
}

Lufia2ExecutionResult Lufia2MenuRunScenarioViewer(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu,
                                                  Lufia2PushedChildCall child,
                                                  void *context) {
    if (!child || cpu->program_bank != 0x8eu || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal ||
        cpu->stack < 0x1f20u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x8ee5efu);
    ScenarioView view = {memory, cpu, child, context, {0}};
    if (!ScenarioChild(&view, 0x8ee5efu, 0x869022u, 0u) ||
        !ScenarioChild(&view, 0x8ee5f3u, 0x86916cu, 0u))
        return view.result;
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, SCENARIO_PALETTE_REQUEST));
    for (;;) {
        if (!ScenarioLocal(&view, 0xe5fbu, BuildScenarioPortraits))
            return view.result;
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, OpDp(cpu, SCENARIO_PAGE));
        do {
            if (!ScenarioLocal(&view, 0xe602u, DrawScenarioPage))
                return view.result;
            OpLdx(cpu, 0x80u);
            OpWriteX(memory, cpu, OpAbs(cpu, WRAM_UNK_7E156C), cpu->x);
            OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E156E));
            OpLda(memory, cpu, OpDp(cpu, SCENARIO_SELECTION));
            OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E156E + 1u));
            if (!ScenarioChild(&view, 0x8ee613u, 0x82935du, 0u))
                return view.result;
            if (cpu->carry) {
                OpLoadA(cpu, 1u);
                if (!ScenarioChild(&view, 0x8ee639u, 0x80953bu, 0u) ||
                    !ScenarioChild(&view, 0x8ee63du, 0x868b32u, 0u))
                    return view.result;
                return ExecutionReturned(0x8ee641u);
            }
            OpLoadA(cpu, 2u);
            if (!ScenarioChild(&view, 0x8ee61bu, 0x80953bu, 0u) ||
                !ScenarioChild(&view, 0x8ee61fu, 0x868b32u, 0u) ||
                !ScenarioLocal(&view, 0xe623u, ClearScenarioLayers))
                return view.result;
            OpLdx(cpu, 0xffffu);
            if (!ScenarioChild(&view, 0x8ee629u, 0x829214u, 0u))
                return view.result;
            OpStepMem(memory, cpu, OpDp(cpu, SCENARIO_PAGE), 1);
            OpLda(memory, cpu, OpDp(cpu, SCENARIO_PAGE));
            OpCmpValue(cpu, SCENARIO_PAGE_COUNT);
        } while (!cpu->zero);
    }
}
