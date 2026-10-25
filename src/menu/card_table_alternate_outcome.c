#include "menu/card_table_outcome.h"
#include "lufia2/card_table.h"

static bool SlowAlternateColumn(TableService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    do {
        OpLoadA(cpu, 5u);
        if (!TableServiceCall(service, 0x84f4d7u, 0x808299u, false)) return false;
        cpu->carry = 0u;
        OpAdcValue(cpu, 5u);
        OpSta(memory, cpu, OpDp(cpu, OUTCOME_AMOUNT));
        OpLda(memory, cpu, OpAbsX(cpu, OUTCOME_COLUMN_STEPS));
        cpu->carry = 1u;
        OpSbcValue(cpu, 0x0100u);
        OpSta(memory, cpu, OpAbsX(cpu, OUTCOME_COLUMN_STEPS));
        do {
            if (!TableServiceCall(service, 0x84f4ebu, 0x84e9b1u, false)) return false;
            OpStepMem(memory, cpu, OpDp(cpu, OUTCOME_AMOUNT), -1);
        } while (!cpu->zero);
        OpLda(memory, cpu, OpAbsX(cpu, OUTCOME_COLUMN_STEPS));
        cpu->carry = 1u;
        OpSbcValue(cpu, 0x0100u);
    } while (!cpu->zero);
    OpLda(memory, cpu, OpAbsX(cpu, OUTCOME_COLUMN_STEPS));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbsX(cpu, OUTCOME_COLUMN_POSITIONS));
    OpCmpValue(cpu, 0x1c00u);
    while (cpu->negative) {
        if (!TableServiceCall(service, 0x84f507u, 0x84e9b1u, false)) return false;
        OpLda(memory, cpu, OpAbsX(cpu, OUTCOME_COLUMN_STEPS));
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpAbsX(cpu, OUTCOME_COLUMN_POSITIONS));
        OpCmpValue(cpu, 0x1c00u);
    }
    OpLda(memory, cpu, OpAbsX(cpu, OUTCOME_COLUMN_SPEEDS));
    OpDecA(cpu);
    OpCmpValue(cpu, 5u);
    if (cpu->negative) OpLoadA(cpu, 25u);
    OpSta(memory, cpu, OpAbsX(cpu, OUTCOME_COLUMN_SPEEDS));
    OpSta(memory, cpu, OpAbsX(cpu, OUTCOME_STOPPED_SYMBOLS));
    OpSta(memory, cpu, OpAbsX(cpu, OUTCOME_CURSORS));
    OpStz(memory, cpu, OpAbsX(cpu, OUTCOME_COLUMN_STEPS));
    OpStz(memory, cpu, OpAbsX(cpu, OUTCOME_COLUMN_POSITIONS));
    return TableServiceCall(service, 0x84f531u, 0x84ea4bu, false);
}

Lufia2ExecutionResult Lufia2CardTableStopAlternateColumns(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !OutcomeWordEntry(cpu)) return ExecutionHandoff(cpu, 0x84f4c6u);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84f53bu), false};
    OpStz(memory, cpu, OpAbs(cpu, OUTCOME_COLUMN));
    OpLoadA(cpu, 3u);
    OpSta(memory, cpu, OpDp(cpu, OUTCOME_REMAINING));
    do {
        OpLda(memory, cpu, OpAbs(cpu, OUTCOME_COLUMN));
        cpu->carry = 0u;
        RolA16(cpu);
        OpTax(cpu);
        if (!SlowAlternateColumn(&service)) return service.result;
        OpStepMem(memory, cpu, OpAbs(cpu, OUTCOME_COLUMN), 1);
        OpStepMem(memory, cpu, OpDp(cpu, OUTCOME_REMAINING), -1);
    } while (!cpu->zero);
    return service.result;
}

static bool CelebrateAlternateJackpot(TableService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    OpLoadA(cpu, 1000u);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, OUTCOME_AMOUNT));
    OpSta(memory, cpu, OpDp(cpu, OUTCOME_AMOUNT));
    OpLoadA(cpu, 0x25u);
    if (!TableServiceCall(service, 0x84f600u, 0x84a60cu, false) ||
        !TableServiceCall(service, 0x84f603u, 0x84f01au, false) ||
        !TableServiceCall(service, 0x84f606u, 0x84e9b1u, false)) return false;
    OpLoadA(cpu, 0x01a4u);
    do {
        if (!TableServiceCall(service, 0x84f60cu, 0x84a5a8u, false)) return false;
        OpDecA(cpu);
    } while (!cpu->zero);
    OpLoadA(cpu, 0x22u);
    if (!TableServiceCall(service, 0x84f615u, 0x84a60cu, false)) return false;
    return TableServiceCall(service, 0x84f618u, 0x84f01au, false);
}

static void CountAlternateSevens(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpStz(memory, cpu, OpDp(cpu, OUTCOME_AMOUNT));
    OpLdx(cpu, 0u);
    OpLda(memory, cpu, OpAbs(cpu, OUTCOME_SELECTION));
    OpSta(memory, cpu, OpDp(cpu, OUTCOME_MODE));
    do {
        OpLda(memory, cpu, OpAbsY(cpu, OUTCOME_SYMBOL_ROWS));
        OpCmpValue(cpu, 7u);
        if (cpu->zero) {
            OpStepMem(memory, cpu, OpDp(cpu, OUTCOME_AMOUNT), 1);
            OpStepMem(memory, cpu, OpDp(cpu, OUTCOME_AMOUNT), 1);
            OpStepMem(memory, cpu, OpAbsX(cpu, OUTCOME_LINE_COUNTERS), 1);
        }
        OpInx(cpu);
        OpInx(cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpCpx(cpu, 6u);
        if (cpu->zero) OpLdy(cpu, 2u);
        OpStepMem(memory, cpu, OpDp(cpu, OUTCOME_MODE), -1);
    } while (!cpu->zero);
}

static bool AddAlternateLinePayout(TableService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    OpLda(memory, cpu, OpAbsX(cpu, OUTCOME_MATCH_TYPES));
    if (cpu->zero) return true;
    OpCmpValue(cpu, 1u);
    if (cpu->zero) {
        OpLda(memory, cpu, OpAbsX(cpu, OUTCOME_MATCH_SYMBOLS));
        OpCmpValue(cpu, 7u);
        if (cpu->zero) {
            OpStepMem(memory, cpu, OpDp(cpu, OUTCOME_AMOUNT), 1);
            OpStepMem(memory, cpu, OpDp(cpu, OUTCOME_AMOUNT), 1);
            OpStepMem(memory, cpu, OpAbsX(cpu, OUTCOME_LINE_COUNTERS), 1);
        }
        return true;
    }
    OpLda(memory, cpu, OpAbsX(cpu, OUTCOME_MATCH_SYMBOLS));
    OpDecA(cpu);
    if (cpu->zero) return CelebrateAlternateJackpot(service);
    OpDecA(cpu);
    if (cpu->zero) {
        OpLoadA(cpu, 300u);
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpDp(cpu, OUTCOME_AMOUNT));
        OpSta(memory, cpu, OpDp(cpu, OUTCOME_AMOUNT));
        OpLoadA(cpu, 0x28u);
        return TableServiceCall(service, 0x84f62bu, 0x84a607u, false);
    }
    OpSta(memory, cpu, OpDp(cpu, OUTCOME_REMAINING));
    OpLoadA(cpu, 10u);
    for (;;) {
        OpStepMem(memory, cpu, OpDp(cpu, OUTCOME_REMAINING), -1);
        if (cpu->zero) break;
        cpu->carry = 0u;
        RolA16(cpu);
    }
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, OUTCOME_AMOUNT));
    OpSta(memory, cpu, OpDp(cpu, OUTCOME_AMOUNT));
    return true;
}

Lufia2ExecutionResult Lufia2CardTableCalculateAlternatePayout(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !OutcomeWordEntry(cpu)) return ExecutionHandoff(cpu, 0x84f5a5u);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84f647u), false};
    CountAlternateSevens(memory, cpu);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, OUTCOME_SELECTION)));
    OpLdx(cpu, 0u);
    do {
        if (!AddAlternateLinePayout(&service)) return service.result;
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    return service.result;
}

Lufia2ExecutionResult Lufia2CardTablePayAlternateRound(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !OutcomeWordEntry(cpu)) return ExecutionHandoff(cpu, 0x84f53cu);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84f588u), false};
    ClearOutcomeCounters(memory, cpu);
    if (!TableServiceCall(&service, 0x84f54au, 0x84f5a5u, false) ||
        !TableServiceCall(&service, 0x84f54du, 0x84f589u, false)) return service.result;
    OpLda(memory, cpu, OpDp(cpu, OUTCOME_AMOUNT));
    if (cpu->zero) return service.result;
    OpLda(memory, cpu, OpAbs(cpu, OUTCOME_STAKE));
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_PAYOUT_STEP));
    do {
        if (!TableServiceCall(&service, 0x84f55du, 0x84a421u, false)) return service.result;
        SetOutcomeTextMode(memory, cpu, 0x30u);
        OpLdx(cpu, 0x3694u);
        OpLdy(cpu, 0xf945u);
        if (!TableServiceCall(&service, 0x84f571u, 0x808878u, false)) return service.result;
        OpLoadA(cpu, 8u);
        OpSta(memory, cpu, OpDp(cpu, OUTCOME_REQUEST));
        if (!TableServiceCall(&service, 0x84f579u, 0x84a5a8u, false) ||
            !TableServiceCall(&service, 0x84f57cu, 0x84a5a8u, false)) return service.result;
        OpRepWidths(cpu, 0x20u);
        OpStepMem(memory, cpu, OpDp(cpu, OUTCOME_AMOUNT), -1);
    } while (!cpu->zero);
    TableServiceCall(&service, 0x84f585u, 0x84f589u, false);
    return service.result;
}

static bool WaitAlternateHighlight(TableService *service, uint32_t site) {
    Lufia2CpuState *cpu = service->cpu;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x88u);
    OpSta(service->memory, cpu, OpDp(cpu, OUTCOME_REQUEST));
    if (!TableServiceCall(service, site, 0x84e9b1u, false)) return false;
    OpRepWidths(cpu, 0x20u);
    return true;
}

static void SetAlternatePrompt(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t attribute) {
    OpLoadA(cpu, attribute);
    OpSta(memory, cpu, OpDp(cpu, OUTCOME_ATTRIBUTE));
    OpLoadA(cpu, 6u);
    OpSta(memory, cpu, OpDp(cpu, OUTCOME_AMOUNT));
    OpLoadA(cpu, 0x0642u);
    OpSta(memory, cpu, OpDp(cpu, OUTCOME_REMAINING));
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpDp(cpu, OUTCOME_ROWS));
}

static bool ChooseFirstAlternateLine(TableService *service, bool *cancelled) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    for (;;) {
        if (!TableServiceCall(service, 0x84f65au, 0x84f0d4u, false) ||
            !TableServiceCall(service, 0x84f65du, 0x84f09cu, false) ||
            !TableServiceCall(service, 0x84f660u, 0x84dc55u, false)) return false;
        OpCmpValue(cpu, 7u);
        if (cpu->zero) continue;
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            OpLoadA(cpu, 1u);
            *cancelled = true;
            return TableServiceCall(service, 0x84f672u, 0x84a607u, false);
        }
        OpAndValue(cpu, 0x0100u);
        if (!cpu->zero) break;
    }
    OpLoadA(cpu, 0x21u);
    if (!TableServiceCall(service, 0x84f680u, 0x84a607u, false) ||
        !TableServiceCall(service, 0x84f683u, 0x84edcbu, false) ||
        !TableServiceCall(service, 0x84f686u, 0x84f74eu, false)) return false;
    OpLoadA(cpu, 0x0c00u);
    OpSta(memory, cpu, OpDp(cpu, OUTCOME_ATTRIBUTE));
    OpLda(memory, cpu, OpAbs(cpu, OUTCOME_CHOICE_TILES));
    OpSta(memory, cpu, OpDp(cpu, OUTCOME_AMOUNT));
    OpLda(memory, cpu, OpAbs(cpu, OUTCOME_CHOICE_TILES + 2u));
    OpSta(memory, cpu, OpDp(cpu, OUTCOME_REMAINING));
    OpLoadA(cpu, 3u);
    OpSta(memory, cpu, OpDp(cpu, OUTCOME_ROWS));
    if (!TableServiceCall(service, 0x84f69du, 0x84f105u, false) ||
        !WaitAlternateHighlight(service, 0x84f6a6u)) return false;
    OpStepMem(memory, cpu, OpAbs(cpu, OUTCOME_SELECTION), 1);
    return true;
}

static bool ChooseAdditionalAlternateLines(TableService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    for (;;) {
        if (!TableServiceCall(service, 0x84f6cfu, 0x84f0d4u, false) ||
            !TableServiceCall(service, 0x84f6d2u, 0x84dc55u, false)) return false;
        OpCmpValue(cpu, 1u);
        if (cpu->zero) return true;
        OpAndValue(cpu, 0x0100u);
        if (cpu->zero) continue;
        OpLoadA(cpu, 0x21u);
        if (!TableServiceCall(service, 0x84f6e2u, 0x84a607u, false) ||
            !TableServiceCall(service, 0x84f6e5u, 0x84f74eu, false)) return false;
        OpDecA(cpu);
        if (cpu->negative) continue;
        OpLda(memory, cpu, OpAbs(cpu, OUTCOME_SELECTION));
        cpu->carry = 0u;
        RolA16(cpu);
        RolA16(cpu);
        OpTax(cpu);
        OpLoadA(cpu, 0x0c00u);
        OpSta(memory, cpu, OpDp(cpu, OUTCOME_ATTRIBUTE));
        OpLda(memory, cpu, OpAbsX(cpu, OUTCOME_CHOICE_TILES));
        OpSta(memory, cpu, OpDp(cpu, OUTCOME_AMOUNT));
        OpInx(cpu);
        OpInx(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, OUTCOME_CHOICE_TILES));
        OpSta(memory, cpu, OpDp(cpu, OUTCOME_REMAINING));
        OpInx(cpu);
        OpInx(cpu);
        OpLoadA(cpu, 3u);
        OpSta(memory, cpu, OpDp(cpu, OUTCOME_ROWS));
        if (!TableServiceCall(service, 0x84f70au, 0x84f105u, false) ||
            !WaitAlternateHighlight(service, 0x84f713u)) return false;
        OpStepMem(memory, cpu, OpAbs(cpu, OUTCOME_SELECTION), 1);
        OpLda(memory, cpu, OpAbs(cpu, OUTCOME_SELECTION));
        OpCmpValue(cpu, 5u);
        if (cpu->zero) return true;
    }
}

Lufia2ExecutionResult Lufia2CardTableChooseAlternateLines(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !OutcomeWordEntry(cpu)) return ExecutionHandoff(cpu, 0x84f648u);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84f74du), false};
    OpStz(memory, cpu, OpAbs(cpu, OUTCOME_SELECTION));
    OpLoadA(cpu, 0x0c00u);
    OpLda(memory, cpu, OpAbs(cpu, OUTCOME_ATTRIBUTES));
    OpLoadA(cpu, 5u);
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_LINE_TIMER));
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_ACCENT_TIMER));
    bool cancelled = false;
    if (!ChooseFirstAlternateLine(&service, &cancelled)) return service.result;
    if (!cancelled) {
        OpLda(memory, cpu, OpAbs(cpu, OUTCOME_VARIANT));
        bool finish = false;
        if (!cpu->zero) {
            OpAndValue(cpu, 9u);
            finish = cpu->zero != 0u;
        }
        if (!finish) {
            SetAlternatePrompt(memory, cpu, 0x0c00u);
            if (!TableServiceCall(&service, 0x84f6ccu, 0x84f105u, false) ||
                !ChooseAdditionalAlternateLines(&service)) return service.result;
        }
        OpLoadA(cpu, 2u);
        if (!TableServiceCall(&service, 0x84f726u, 0x84a607u, false)) return service.result;
    }
    SetAlternatePrompt(memory, cpu, 0x1800u);
    if (!TableServiceCall(&service, 0x84f73du, 0x84f105u, false)) return service.result;
    OpLoadA(cpu, 0x0672u);
    OpSta(memory, cpu, OpDp(cpu, OUTCOME_REMAINING));
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpDp(cpu, OUTCOME_ROWS));
    TableServiceCall(&service, 0x84f74au, 0x84f105u, false);
    return service.result;
}
