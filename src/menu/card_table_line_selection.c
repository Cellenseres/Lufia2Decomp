#include "menu/card_table_services.h"
#include "lufia2/card_table.h"
#include "system/wram.h"

enum {
    LINE_WIDTH = 0x00u,
    LINE_ORIGIN = 0x02u,
    LINE_ATTRIBUTES = 0x04u,
    LINE_ROWS = 0x06u,
    LINE_TEXT_BANK = 0x5fu,
    LINE_FRAME_REQUEST = 0x74u,
    LINE_LAYOUT = (WRAM_CARD_GAME_HAND_WINS - 3u) & 0xffffu,
    LINE_VARIANT = WRAM_CARD_TABLE_VARIANT_FLAGS & 0xffffu,
    LINE_SELECTED = WRAM_CARD_TABLE_SELECTION_ACTIVE & 0xffffu,
    LINE_TIMER = WRAM_CARD_TABLE_LINE_ATTRIBUTE_TIMER & 0xffffu,
    LINE_ACCENT = WRAM_BOARD_GAME_LINE_TOTALS & 0xffffu,
    LINE_ACCENT_TIMER = LINE_ACCENT + 2u,
    LINE_CREDITS = WRAM_CARD_TABLE_PAYOUT_REMAINING & 0xffffu,
    LINE_NUMBER = WRAM_CARD_TABLE_PAYOUT_INDEX & 0xffffu,
    LINE_NUMBER_SLOT = WRAM_CARD_TABLE_PAYOUT_STATE & 0xffffu,
    LINE_CHANGED_DIGITS = WRAM_CARD_TABLE_PAYOUT_VALUE & 0xffffu,
    LINE_PENDING_NUMBER = WRAM_CARD_TABLE_PAYOUT_ACTIVE & 0xffffu,
    LINE_NUMBER_MOVEMENT = WRAM_CARD_TABLE_PAYOUT_COPY & 0xffffu,
    LINE_CURRENCY = WRAM_MENU_EXCHANGE_BALANCE & 0xffffu,
    LINE_DRAW_MODE = WRAM_MENU_DRAW_MODE & 0xffffu
};

static bool LineWordEntry(const Lufia2CpuState *cpu) {
    return TableServiceReady(cpu, false) && !cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit;
}

static bool PaintLineRectangle(TableService *service, uint32_t site) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    OpLda(memory, cpu, OpAbsX(cpu, LINE_LAYOUT));
    OpSta(memory, cpu, OpDp(cpu, LINE_WIDTH));
    OpInx(cpu);
    OpInx(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, LINE_LAYOUT));
    OpSta(memory, cpu, OpDp(cpu, LINE_ORIGIN));
    OpInx(cpu);
    OpInx(cpu);
    OpLoadA(cpu, 3u);
    OpSta(memory, cpu, OpDp(cpu, LINE_ROWS));
    return TableServiceCall(service, site, 0x84f105u, false);
}

Lufia2ExecutionResult Lufia2CardTableMarkExtraLines(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !LineWordEntry(cpu))
        return ExecutionHandoff(cpu, 0x84e30fu);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84e333u), false};
    OpLdx(cpu, 20u);
    OpLdy(cpu, 3u);
    OpLoadA(cpu, 0x0c00u);
    OpSta(memory, cpu, OpDp(cpu, LINE_ATTRIBUTES));
    do {
        if (!PaintLineRectangle(&service, 0x84e32du)) return service.result;
        OpDey(cpu);
    } while (!cpu->zero);
    return service.result;
}

Lufia2ExecutionResult Lufia2CardTableSpendCredit(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !LineWordEntry(cpu))
        return ExecutionHandoff(cpu, 0x84e2a8u);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84e30eu), false};
    OpLda(memory, cpu, OpAbs(cpu, LINE_CREDITS));
    if (!cpu->zero) {
        OpDecA(cpu);
        OpSta(memory, cpu, OpAbs(cpu, LINE_CREDITS));
        OpSta(memory, cpu, OpAbs(cpu, LINE_NUMBER));
        OpLdx(cpu, 1u);
        OpWriteX(memory, cpu, OpAbs(cpu, LINE_NUMBER_SLOT), cpu->x);
        if (!TableServiceCall(&service, 0x84e2bau, 0x84eb36u, false))
            return service.result;
        OpLda(memory, cpu, OpAbs(cpu, LINE_CHANGED_DIGITS));
        OpSta(memory, cpu, OpAbs(cpu, LINE_NUMBER_MOVEMENT));
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, OpAbs(cpu, LINE_PENDING_NUMBER));
        if (!TableServiceCall(&service, 0x84e2c9u, 0x84ebdbu, false))
            return service.result;
    } else {
        OpLda(memory, cpu, OpAbs(cpu, LINE_CURRENCY));
        cpu->carry = 1u;
        OpSbcValue(cpu, 10u);
        if (!cpu->carry) {
            OpLda(memory, cpu, OpAbs(cpu, LINE_CURRENCY + 2u));
            cpu->carry = 1u;
            OpSbcValue(cpu, 1u);
            OpSta(memory, cpu, OpAbs(cpu, LINE_CURRENCY + 2u));
            if (!cpu->carry) {
                OpStz(memory, cpu, OpAbs(cpu, LINE_CURRENCY + 2u));
                OpLoadA(cpu, 0u);
                return ExecutionReturned(0x84e2e9u);
            }
        }
        OpLda(memory, cpu, OpAbs(cpu, LINE_CURRENCY));
        cpu->carry = 1u;
        OpSbcValue(cpu, 10u);
        OpSta(memory, cpu, OpAbs(cpu, LINE_CURRENCY));
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, 0x20u);
        OpSta(memory, cpu, OpAbs(cpu, LINE_DRAW_MODE));
        OpLoadA(cpu, 0x84u);
        OpSta(memory, cpu, OpDp(cpu, LINE_TEXT_BANK));
        OpRepWidths(cpu, 0x20u);
        OpLdx(cpu, 0x3694u);
        OpLdy(cpu, 0xf945u);
        if (!TableServiceCall(&service, 0x84e307u, 0x808878u, false))
            return service.result;
    }
    OpLoadA(cpu, 1u);
    return service.result;
}

static bool RequestLineFrame(TableService *service, uint32_t site) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x88u);
    OpSta(memory, cpu, OpDp(cpu, LINE_FRAME_REQUEST));
    if (!TableServiceCall(service, site, 0x84e9b1u, true)) return false;
    OpRepWidths(cpu, 0x20u);
    return true;
}

static bool PaintLineLabel(TableService *service, uint16_t attributes,
                           uint32_t site) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    OpLoadA(cpu, attributes);
    OpSta(memory, cpu, OpDp(cpu, LINE_ATTRIBUTES));
    OpLoadA(cpu, 6u);
    OpSta(memory, cpu, OpDp(cpu, LINE_WIDTH));
    OpLoadA(cpu, 0x642u);
    OpSta(memory, cpu, OpDp(cpu, LINE_ORIGIN));
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpDp(cpu, LINE_ROWS));
    return TableServiceCall(service, site, 0x84f105u, false);
}

static bool ChooseAdditionalLines(TableService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    if (!PaintLineLabel(service, 0x0c00u, 0x84e21bu)) return false;
    for (;;) {
        if (!TableServiceCall(service, 0x84e21eu, 0x84f0d4u, false) ||
            !TableServiceCall(service, 0x84e221u, 0x84dc55u, false)) return false;
        OpCmpValue(cpu, 1u);
        if (cpu->zero) return true;
        OpAndValue(cpu, 0x100u);
        if (cpu->zero) continue;
        OpLoadA(cpu, 0x21u);
        if (!TableServiceCall(service, 0x84e231u, 0x84a607u, false) ||
            !TableServiceCall(service, 0x84e234u, 0x84e2a8u, false)) return false;
        OpDecA(cpu);
        if (cpu->negative) continue;
        OpLda(memory, cpu, OpAbs(cpu, LINE_SELECTED));
        OpDecA(cpu);
        cpu->carry = 0u;
        for (unsigned shift = 0u; shift < 3u; ++shift) RolA16(cpu);
        OpAdcValue(cpu, 4u);
        OpTax(cpu);
        OpLdy(cpu, 2u);
        OpLoadA(cpu, 0x0c00u);
        OpSta(memory, cpu, OpDp(cpu, LINE_ATTRIBUTES));
        do {
            if (!PaintLineRectangle(service, 0x84e261u)) return false;
            OpDey(cpu);
        } while (!cpu->zero);
        if (!RequestLineFrame(service, 0x84e26du)) return false;
        OpStepMem(memory, cpu, OpAbs(cpu, LINE_SELECTED), 1);
        OpLda(memory, cpu, OpAbs(cpu, LINE_SELECTED));
        OpCmpValue(cpu, 3u);
        if (cpu->zero) return true;
    }
}

Lufia2ExecutionResult Lufia2CardTableChooseLines(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !LineWordEntry(cpu))
        return ExecutionHandoff(cpu, 0x84e194u);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84e2a7u), false};
    OpStz(memory, cpu, OpAbs(cpu, LINE_SELECTED));
    OpLoadA(cpu, 0x0c00u);
    OpLda(memory, cpu, OpAbs(cpu, LINE_ACCENT));
    OpLoadA(cpu, 5u);
    OpSta(memory, cpu, OpAbs(cpu, LINE_TIMER));
    OpSta(memory, cpu, OpAbs(cpu, LINE_ACCENT_TIMER));
    bool confirmed = false;
    for (;;) {
        if (!TableServiceCall(&service, 0x84e1a6u, 0x84f0d4u, false) ||
            !TableServiceCall(&service, 0x84e1a9u, 0x84f09cu, false) ||
            !TableServiceCall(&service, 0x84e1acu, 0x84dc55u, false))
            return service.result;
        OpCmpValue(cpu, 7u);
        if (cpu->zero) {
            if (!TableServiceCall(&service, 0x84e1b4u, 0x84ee4bu, false))
                return service.result;
            continue;
        }
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            OpLoadA(cpu, 1u);
            if (!TableServiceCall(&service, 0x84e1c1u, 0x84a607u, false))
                return service.result;
            break;
        }
        OpAndValue(cpu, 0x100u);
        if (cpu->zero) continue;
        OpLoadA(cpu, 0x21u);
        if (!TableServiceCall(&service, 0x84e1cfu, 0x84a607u, false) ||
            !TableServiceCall(&service, 0x84e1d2u, 0x84edcbu, false) ||
            !TableServiceCall(&service, 0x84e1d5u, 0x84e2a8u, false))
            return service.result;
        OpLoadA(cpu, 0x0c00u);
        OpSta(memory, cpu, OpDp(cpu, LINE_ATTRIBUTES));
        OpLda(memory, cpu, OpAbs(cpu, LINE_LAYOUT));
        OpSta(memory, cpu, OpDp(cpu, LINE_WIDTH));
        OpLda(memory, cpu, OpAbs(cpu, LINE_LAYOUT + 2u));
        OpSta(memory, cpu, OpDp(cpu, LINE_ORIGIN));
        OpLoadA(cpu, 3u);
        OpSta(memory, cpu, OpDp(cpu, LINE_ROWS));
        if (!TableServiceCall(&service, 0x84e1ecu, 0x84f105u, false) ||
            !RequestLineFrame(&service, 0x84e1f5u)) return service.result;
        OpStepMem(memory, cpu, OpAbs(cpu, LINE_SELECTED), 1);
        OpLda(memory, cpu, OpAbs(cpu, LINE_VARIANT));
        confirmed = true;
        if (!cpu->zero) {
            OpAndValue(cpu, 9u);
            if (cpu->zero) break;
        }
        if (!ChooseAdditionalLines(&service)) return service.result;
        break;
    }
    if (confirmed) {
        OpLoadA(cpu, 2u);
        if (!TableServiceCall(&service, 0x84e280u, 0x84a607u, false))
            return service.result;
    }
    if (!PaintLineLabel(&service, 0x1800u, 0x84e297u)) return service.result;
    OpLoadA(cpu, 0x672u);
    OpSta(memory, cpu, OpDp(cpu, LINE_ORIGIN));
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpDp(cpu, LINE_ROWS));
    if (!TableServiceCall(&service, 0x84e2a4u, 0x84f105u, false))
        return service.result;
    return service.result;
}

Lufia2ExecutionResult Lufia2CardTableReadCommand(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !LineWordEntry(cpu))
        return ExecutionHandoff(cpu, 0x84dc55u);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84dc5fu), false};
    if (!TableServiceCall(&service, 0x84dc55u, 0x808299u, false) ||
        !TableServiceCall(&service, 0x84dc59u, 0x84e9b1u, false) ||
        !TableServiceCall(&service, 0x84dc5cu, 0x849f1au, false))
        return service.result;
    return service.result;
}
