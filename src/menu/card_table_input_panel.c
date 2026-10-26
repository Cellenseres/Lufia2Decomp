#include "menu/card_table_services.h"
#include "lufia2/card_table.h"
#include "system/wram.h"

enum {
    PANEL_RESOURCE = 0x54u,
    PANEL_RESOURCE_DESTINATION = 0x60u,
    PANEL_RESOURCE_BANK = 0x62u,
    PANEL_COMMAND_COUNT = (WRAM_BOARD_GAME_LINE_TOTALS + 14u) & 0xffffu,
    PANEL_COMMANDS = PANEL_COMMAND_COUNT + 2u,
    PANEL_CURRENT_COMMAND = (WRAM_BOARD_GAME_SAVED_GROUP_CELLS + 6u) & 0xffffu,
    PANEL_SEQUENCE_FLAG = PANEL_CURRENT_COMMAND + 2u,
    PANEL_SEQUENCE_INDEX = 0u,
    PANEL_SEQUENCE_TABLE = 0x84f308u,
    PANEL_SEQUENCE_RESULTS = 0x84f37au,
    PANEL_MODE = (WRAM_CARD_TABLE_RANDOM_SELECTION + 2u) & 0xffffu,
    PANEL_PENDING_WORD = WRAM_CARD_TABLE_PENDING_WORDS & 0xffffu,
    PANEL_LAST_COMMAND = 28u,
    PANEL_SCAN_LIMIT = 65536u,
    PANEL_INPUT_LIMIT = 4096u,
    PANEL_PALETTE_REQUEST = 0x73u,
    PANEL_TRANSFER_REQUEST = 0x74u,
    PANEL_TEXT_BANK = 0x5fu
};

static bool PanelReady(const Lufia2CpuState *cpu) {
    return TableServiceReady(cpu, false) && !cpu->index_is_8_bit;
}

Lufia2ExecutionResult Lufia2CardTableLoadPanelResource(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !PanelReady(cpu))
        return ExecutionHandoff(cpu, 0x84ef4eu);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84ef5fu), false};
    OpSepWidths(cpu, 0x20u);
    OpWriteX(memory, cpu, OpDp(cpu, PANEL_RESOURCE), cpu->x);
    OpLdx(cpu, 0xa000u);
    OpWriteX(memory, cpu, OpDp(cpu, PANEL_RESOURCE_DESTINATION), cpu->x);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, PANEL_RESOURCE_BANK));
    TableServiceCall(&service, 0x84ef5bu, 0x808e9du, true);
    return service.result;
}

Lufia2ExecutionResult Lufia2CardTableAppendPanelCommand(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!PanelReady(cpu))
        return ExecutionHandoff(cpu, 0x84ef88u);
    OpRepWidths(cpu, 0x20u);
    OpCmpValue(cpu, 0u);
    if (cpu->zero)
        return ExecutionReturned(0x84efc3u);
    OpCmpValue(cpu, 8u);
    if (cpu->zero) {
        OpStz(memory, cpu, OpAbs(cpu, PANEL_COMMAND_COUNT));
        return ExecutionReturned(0x84efc3u);
    }
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, PANEL_COMMAND_COUNT)));
    Compare16(cpu, cpu->x, PANEL_LAST_COMMAND);
    if (!cpu->zero) {
        OpInx(cpu);
        OpInx(cpu);
        OpWriteX(memory, cpu, OpAbs(cpu, PANEL_COMMAND_COUNT), cpu->x);
    } else {
        OpLdx(cpu, 0u);
        OpLdy(cpu, 2u);
        do {
            OpLda(memory, cpu, OpAbsY(cpu, PANEL_COMMANDS));
            OpSta(memory, cpu, OpAbsX(cpu, PANEL_COMMANDS));
            OpInx(cpu);
            OpInx(cpu);
            OpIny(cpu);
            OpIny(cpu);
            Compare16(cpu, cpu->x, PANEL_LAST_COMMAND);
        } while (!cpu->zero);
    }
    OpLda(memory, cpu, OpAbs(cpu, PANEL_CURRENT_COMMAND));
    OpSta(memory, cpu, OpAbsX(cpu, PANEL_COMMAND_COUNT));
    return ExecutionReturned(0x84efc3u);
}

static void AdvancePanelSequence(Lufia2CpuState *cpu) {
    OpInx(cpu);
    OpInx(cpu);
}

static void ApplyPanelSequence(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, PANEL_SEQUENCE_INDEX));
    OpCmpValue(cpu, 4u);
    if (cpu->zero) {
        OpStepMem(memory, cpu, OpAbs(cpu, PANEL_SEQUENCE_FLAG), 1);
        return;
    }
    cpu->carry = 0u;
    RolA16(cpu);
    RolA16(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, PANEL_SEQUENCE_RESULTS));
    OpSta(memory, cpu, OpAbs(cpu, PANEL_MODE));
    OpLda(memory, cpu, OpLongX(cpu, PANEL_SEQUENCE_RESULTS + 2u));
    OpSta(memory, cpu, OpAbs(cpu, PANEL_PENDING_WORD));
}

Lufia2ExecutionResult Lufia2CardTableMatchPanelSequence(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !PanelReady(cpu))
        return ExecutionHandoff(cpu, 0x84efc4u);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84f019u), false};
    OpRepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpDp(cpu, PANEL_SEQUENCE_INDEX));
    OpLdx(cpu, 0xfffeu);
    OpLdy(cpu, 0u);
    unsigned work = 0u;
    for (;;) {
        if (work++ == PANEL_SCAN_LIMIT)
            return ExecutionHandoff(cpu, 0x84efceu);
        AdvancePanelSequence(cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpLda(memory, cpu, OpLongX(cpu, PANEL_SEQUENCE_TABLE));
        if (cpu->zero)
            break;
        OpCmpValue(cpu, OpReadM(memory, cpu,
            OpAbsY(cpu, PANEL_COMMAND_COUNT)));
        if (cpu->zero)
            continue;
        OpLdy(cpu, 0u);
        do {
            if (work++ == PANEL_SCAN_LIMIT)
                return ExecutionHandoff(cpu, 0x84efe0u);
            AdvancePanelSequence(cpu);
            OpLda(memory, cpu, OpLongX(cpu, PANEL_SEQUENCE_TABLE));
        } while (!cpu->zero);
        OpLda(memory, cpu, OpLongX(cpu, PANEL_SEQUENCE_TABLE + 2u));
        if (cpu->negative)
            return service.result;
        OpStepMem(memory, cpu, OpDp(cpu, PANEL_SEQUENCE_INDEX), 1);
    }
    ApplyPanelSequence(memory, cpu);
    OpLoadA(cpu, 0x1bu);
    if (!TableServiceCall(&service, 0x84f013u, 0x84a607u, false))
        return service.result;
    OpStz(memory, cpu, OpAbs(cpu, PANEL_COMMANDS));
    return service.result;
}

static void LoadPanelPalette(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t source) {
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
    PushDataBank(memory, cpu);
    OpLdx(cpu, source);
    OpLdy(cpu, 0x0320u);
    OpLoadA(cpu, 0xffu);
    OpMoveNext(memory, cpu, 0u, 0xa6u);
    PullDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, PANEL_PALETTE_REQUEST));
}

static bool LoadPanelLayer(TableService *service, uint16_t resource,
    uint32_t load_site, uint32_t copy_site, bool second) {
    OpLdx(service->cpu, resource);
    if (!TableServiceCall(service, load_site, 0x84ef4eu, true) ||
        !TableServiceCall(service, copy_site,
            second ? 0x84ef74u : 0x84ef60u, true))
        return false;
    if (service->cpu->accumulator_is_8_bit) {
        service->result = ExecutionHandoff(service->cpu, copy_site + 3u);
        return false;
    }
    return true;
}

static void RequestPanelTransfer(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint8_t mask) {
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, mask);
    OpSta(memory, cpu, OpDp(cpu, PANEL_TRANSFER_REQUEST));
}

static bool ShowPanelLayers(TableService *service, bool restoring) {
    const uint32_t first_wait = restoring ? 0x84ef3du : 0x84ee8bu;
    const uint32_t second_wait = restoring ? 0x84ef44u : 0x84ee92u;
    const uint32_t show = restoring ? 0x84ef49u : 0x84ee97u;
    RequestPanelTransfer(service->memory, service->cpu, 0xa0u);
    if (!TableServiceCall(service, first_wait, 0x84a5a8u, false))
        return false;
    OpLoadA(service->cpu, 8u);
    OpSta(service->memory, service->cpu,
        OpDp(service->cpu, PANEL_TRANSFER_REQUEST));
    if (!TableServiceCall(service, second_wait, 0x84a5a8u, false))
        return false;
    OpRepWidths(service->cpu, 0x20u);
    return TableServiceCall(service, show, 0x84a5b5u, false);
}

static bool BeginInputPanel(TableService *service) {
    Lufia2CpuState *cpu = service->cpu;
    if (!TableServiceCall(service, 0x84ee4eu, 0x84a5cau, false))
        return false;
    LoadPanelPalette(service->memory, cpu, 0xf120u);
    if (!TableServiceCall(service, 0x84ee6bu, 0x84a5a8u, false))
        return false;
    OpRepWidths(cpu, 0x20u);
    return TableServiceCall(service, 0x84ee70u, 0x84a4ddu, false) &&
        LoadPanelLayer(service, 0x02a5u, 0x84ee76u, 0x84ee79u, false) &&
        LoadPanelLayer(service, 0x02a3u, 0x84ee7fu, 0x84ee82u, true) &&
        ShowPanelLayers(service, false);
}

static bool ToggleInputPanel(TableService *service) {
    Lufia2CpuState *cpu = service->cpu;
    Compare16(cpu, cpu->x, 0u);
    const bool alternate = cpu->zero != 0u;
    if (!LoadPanelLayer(service, alternate ? 0x02a4u : 0x02a3u,
            alternate ? 0x84eec2u : 0x84eed8u,
            alternate ? 0x84eec5u : 0x84eedbu, true))
        return false;
    RequestPanelTransfer(service->memory, cpu, 0x20u);
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, alternate ? 1u : 0u);
    return true;
}

static bool RestorePanelDisplay(TableService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    if (!TableServiceCall(service, 0x84eeebu, 0x84a5cau, false) ||
        !TableServiceCall(service, 0x84eeeeu, 0x84efc4u, false))
        return false;
    LoadPanelPalette(memory, cpu, 0xf020u);
    if (!TableServiceCall(service, 0x84ef0bu, 0x84a5a8u, false))
        return false;
    OpRepWidths(cpu, 0x20u);
    if (!LoadPanelLayer(service, 0x02a2u, 0x84ef13u, 0x84ef16u, true) ||
        !LoadPanelLayer(service, 0x02a1u, 0x84ef1cu, 0x84ef1fu, false))
        return false;
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    OpLoadA(cpu, 0x84u);
    OpSta(memory, cpu, OpDp(cpu, PANEL_TEXT_BANK));
    OpLdx(cpu, 0x3694u);
    OpLdy(cpu, 0xf945u);
    return TableServiceCall(service, 0x84ef33u, 0x808878u, true) &&
        ShowPanelLayers(service, true);
}

Lufia2ExecutionResult Lufia2CardTableRunInputPanel(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !PanelReady(cpu))
        return ExecutionHandoff(cpu, 0x84ee4bu);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84ef4du), false};
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x20u);
    if (!BeginInputPanel(&service))
        return service.result;
    OpLdx(cpu, 0u);
    for (unsigned frame = 0u; frame < PANEL_INPUT_LIMIT; ++frame) {
        PushIndex(memory, cpu);
        if (!TableServiceCall(&service, 0x84ee9eu, 0x84a5a8u, false) ||
            !TableServiceCall(&service, 0x84eea1u, 0x849f1au, false))
            return service.result;
        OpSta(memory, cpu, OpAbs(cpu, PANEL_CURRENT_COMMAND));
        if (!TableServiceCall(&service, 0x84eea7u, 0x84ef88u, false))
            return service.result;
        OpLda(memory, cpu, OpAbs(cpu, PANEL_CURRENT_COMMAND));
        OpLdx(cpu, PullIndexValue(memory, cpu));
        OpCmpValue(cpu, 1u);
        if (cpu->zero) {
            if (!ToggleInputPanel(&service))
                return service.result;
        } else {
            OpCmpValue(cpu, 2u);
            if (cpu->zero) {
                if (!RestorePanelDisplay(&service))
                    return service.result;
                UnpackStatus(cpu, Pull8(memory, cpu));
                return service.result;
            }
        }
    }
    return ExecutionHandoff(cpu, 0x84ee9du);
}
