#include "menu/card_table_services.h"
#include "lufia2/special_menu.h"
#include "system/wram.h"

enum {
    PAYOUT_REMAINING = 0u,
    PAYOUT_TEXT_DELAY = 0x74u,
    PAYOUT_WINDOW_HEIGHT = (WRAM_SPECIAL_MENU_WINDOW_PARAMETERS + 6u) & 0xffffu,
    PAYOUT_STEP = WRAM_SPECIAL_MENU_PAYOUT_STEP & 0xffffu,
    PAYOUT_ITERATION_LIMIT = 65536u
};

static bool DrawPayout(TableService *service, uint32_t first_site) {
    static const uint16_t positions[] = {0x3316u, 0x3656u};
    static const uint16_t strings[] = {0xf9f1u, 0xf945u};
    for (unsigned line = 0u; line < 2u; ++line) {
        OpLdx(service->cpu, positions[line]);
        OpLdy(service->cpu, strings[line]);
        if (!TableServiceCall(service, first_site + line * 10u, 0x808878u, false))
            return false;
    }
    return true;
}

Lufia2ExecutionResult Lufia2SpecialMenuPayAward(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !TableServiceReady(cpu, true) ||
        cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x84c7a6u);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84c818u), true};
    OpLoadA(cpu, 5u);
    OpSta(memory, cpu, OpAbs(cpu, PAYOUT_WINDOW_HEIGHT));
    if (!TableServiceCall(&service, 0x84c7acu, 0x84a2b3u, false) ||
        !DrawPayout(&service, 0x84c7b5u))
        return service.result;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x88u);
    OpSta(memory, cpu, OpDp(cpu, PAYOUT_TEXT_DELAY));
    OpRepWidths(cpu, 0x20u);
    if (!TableServiceCall(&service, 0x84c7cbu, 0x84c794u, false))
        return service.result;
    OpLoadA(cpu, 10000u);
    OpSta(memory, cpu, OpAbs(cpu, PAYOUT_STEP));
    for (unsigned step = 0u; ; ++step) {
        if (step == PAYOUT_ITERATION_LIMIT)
            return ExecutionHandoff(cpu, 0x84c7d4u);
        OpLda(memory, cpu, OpDp(cpu, PAYOUT_REMAINING));
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, PAYOUT_STEP)));
        if (cpu->negative) {
            OpLda(memory, cpu, OpAbs(cpu, PAYOUT_STEP));
            OpCmpValue(cpu, 1u);
            if (cpu->zero)
                break;
            if (!TableServiceCall(&service, 0x84c7e4u, 0x84a3fcu, false))
                return service.result;
            continue;
        }
        OpSta(memory, cpu, OpDp(cpu, PAYOUT_REMAINING));
        if (!TableServiceCall(&service, 0x84c7ebu, 0x84a421u, false) ||
            !DrawPayout(&service, 0x84c7f4u))
            return service.result;
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, 8u);
        OpSta(memory, cpu, OpDp(cpu, PAYOUT_TEXT_DELAY));
        if (!TableServiceCall(&service, 0x84c808u, 0x84a5a8u, false) ||
            !TableServiceCall(&service, 0x84c80bu, 0x84a5a8u, false))
            return service.result;
        OpRepWidths(cpu, 0x20u);
    }
    if (!TableServiceCall(&service, 0x84c812u, 0x84c794u, false) ||
        !TableServiceCall(&service, 0x84c815u, 0x84a39bu, false))
        return service.result;
    return ExecutionReturned(0x84c818u);
}
