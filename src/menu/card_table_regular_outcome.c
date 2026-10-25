#include "menu/card_table_outcome.h"
#include "lufia2/card_table.h"

static bool AnimateRoundCounter(TableService *service,
    uint32_t draw_site, uint32_t animate_site) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    if (!TableServiceCall(service, draw_site, 0x84eb36u, false)) return false;
    OpLda(memory, cpu, OpAbs(cpu, OUTCOME_CHANGED_DIGITS));
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_FIRST_MOVEMENT));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_FIRST_PENDING));
    return TableServiceCall(service, animate_site, 0x84ebdbu, false);
}

static void CountLeadingSevens(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLdx(cpu, 0u);
    OpLda(memory, cpu, OpAbs(cpu, OUTCOME_SELECTION));
    OpTay(cpu);
    OpCmpValue(cpu, 1u);
    if (!cpu->zero) OpLdy(cpu, 3u);
    bool found = false;
    do {
        OpLda(memory, cpu, OpAbsX(cpu, OUTCOME_SYMBOL_ROWS));
        OpCmpValue(cpu, 7u);
        if (cpu->zero) {
            found = true;
            break;
        }
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    if (!found) return;
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpDp(cpu, OUTCOME_AMOUNT));
    OpCpx(cpu, 0u);
    if (cpu->zero) {
        OpStepMem(memory, cpu, OpAbs(cpu, OUTCOME_LINE_COUNTERS), 1);
    } else {
        OpStepMem(memory, cpu, OpAbsX(cpu, OUTCOME_LINE_COUNTERS), 1);
        OpInx(cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpStepMem(memory, cpu, OpAbsX(cpu, OUTCOME_LINE_COUNTERS), 1);
        OpStepMem(memory, cpu, OpDp(cpu, OUTCOME_AMOUNT), 1);
        OpStepMem(memory, cpu, OpDp(cpu, OUTCOME_AMOUNT), 1);
    }
}

static bool BeginJackpotRounds(TableService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_REFRESH_TIMER));
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_PENDING_MODE));
    OpLoadA(cpu, 15u);
    OpSta(memory, cpu, OpDp(cpu, OUTCOME_AMOUNT));
    OpLoadA(cpu, 12u);
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_VARIANT));
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_SAVED_VARIANT));
    cpu->carry = 0u;
    RorA16(cpu);
    if (!TableServiceCall(service, 0x84e535u, 0x808299u, false)) return false;
    cpu->carry = 0u;
    OpAdcValue(cpu, 4u);
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_COUNTDOWN));
    OpLoadA(cpu, 0x23u);
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_SONG));
    OpLoadA(cpu, 3u);
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_SPECIAL_COUNTDOWN));
    OpLoadA(cpu, 30u);
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_SPECIAL_VALUE));
    OpLoadA(cpu, 0x25u);
    if (!TableServiceCall(service, 0x84e555u, 0x84a60cu, false) ||
        !TableServiceCall(service, 0x84e558u, 0x84f01au, false) ||
        !TableServiceCall(service, 0x84e55bu, 0x84e9b1u, false)) return false;
    OpLoadA(cpu, 0x01a4u);
    do {
        if (!TableServiceCall(service, 0x84e561u, 0x84a5a8u, false)) return false;
        OpDecA(cpu);
    } while (!cpu->zero);
    return true;
}

static bool BeginBonusRounds(TableService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_PENDING_MODE));
    OpLoadA(cpu, 15u);
    OpSta(memory, cpu, OpDp(cpu, OUTCOME_AMOUNT));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_SPECIAL_COUNTDOWN));
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_RANDOM_MODE));
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_VARIANT));
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_SAVED_VARIANT));
    OpLoadA(cpu, 6u);
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_COUNTDOWN));
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_ROUNDS));
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_NUMBER));
    OpIncA(cpu);
    OpIncA(cpu);
    OpSta(memory, cpu, OpDp(cpu, 0x0eu));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_NUMBER_SLOT));
    if (!AnimateRoundCounter(service, 0x84e59au, 0x84e5a9u)) return false;
    OpLoadA(cpu, 0x24u);
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_SONG));
    return true;
}

static void AdvanceOrdinaryMatchMode(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, OUTCOME_VARIANT));
    OpCmpValue(cpu, 1u);
    if (!cpu->zero) return;
    OpStepMem(memory, cpu, OpAbs(cpu, OUTCOME_COUNTDOWN), -1);
    if (!cpu->zero) return;
    OpStz(memory, cpu, OpAbs(cpu, OUTCOME_COUNTDOWN));
    OpStz(memory, cpu, OpAbs(cpu, OUTCOME_VARIANT));
}

static bool EvaluateRegularMatches(TableService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    CountLeadingSevens(memory, cpu);
    OpLda(memory, cpu, OpAbs(cpu, OUTCOME_SELECTION));
    cpu->carry = 0u;
    RolA16(cpu);
    OpDecA(cpu);
    OpTay(cpu);
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, OUTCOME_MATCH_TYPES));
        if (!cpu->zero) {
            OpCmpValue(cpu, 1u);
            if (cpu->zero) {
                OpLda(memory, cpu, OpAbsX(cpu, OUTCOME_MATCH_SYMBOLS));
                OpCmpValue(cpu, 7u);
                if (cpu->zero) {
                    OpStepMem(memory, cpu, OpDp(cpu, OUTCOME_AMOUNT), 1);
                    OpStepMem(memory, cpu, OpDp(cpu, OUTCOME_AMOUNT), 1);
                    OpStepMem(memory, cpu, OpAbsX(cpu, OUTCOME_LINE_COUNTERS), 1);
                    AdvanceOrdinaryMatchMode(memory, cpu);
                    return true;
                }
            } else {
                OpLda(memory, cpu, OpAbsX(cpu, OUTCOME_MATCH_SYMBOLS));
                OpDecA(cpu);
                if (cpu->zero) return BeginJackpotRounds(service);
                OpDecA(cpu);
                if (cpu->zero) return BeginBonusRounds(service);
                OpLda(memory, cpu, OpDp(cpu, OUTCOME_AMOUNT));
                cpu->carry = 0u;
                OpAdcValue(cpu, 8u);
                OpSta(memory, cpu, OpDp(cpu, OUTCOME_AMOUNT));
                AdvanceOrdinaryMatchMode(memory, cpu);
                return true;
            }
        }
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    AdvanceOrdinaryMatchMode(memory, cpu);
    return true;
}

Lufia2ExecutionResult Lufia2CardTableEvaluateMatches(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !OutcomeWordEntry(cpu)) return ExecutionHandoff(cpu, 0x84e489u);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84e51au), false};
    EvaluateRegularMatches(&service);
    return service.result;
}

Lufia2ExecutionResult Lufia2CardTableUpdateRoundOutcome(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !OutcomeWordEntry(cpu)) return ExecutionHandoff(cpu, 0x84e416u);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84e51au), false};
    OpStz(memory, cpu, OpDp(cpu, OUTCOME_AMOUNT));
    OpLda(memory, cpu, OpDp(cpu, OUTCOME_MODE));
    if (cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, OUTCOME_VARIANT));
        if (cpu->zero) {
            EvaluateRegularMatches(&service);
            return service.result;
        }
        OpAndValue(cpu, 9u);
        if (!cpu->zero) {
            EvaluateRegularMatches(&service);
            return service.result;
        }
    }
    cpu->carry = 1u;
    OpSbcValue(cpu, 7u);
    bool fixed_payout;
    if (cpu->negative) {
        OpLda(memory, cpu, OpAbs(cpu, OUTCOME_MATCH_TYPES));
        OpCmpValue(cpu, 2u);
        if (!cpu->zero) {
            OpStepMem(memory, cpu, OpAbs(cpu, OUTCOME_COUNTDOWN), 1);
            OpStepMem(memory, cpu, OpAbs(cpu, OUTCOME_ROUNDS), 1);
            return service.result;
        }
        fixed_payout = true;
    } else {
        OpLoadA(cpu, 2u);
        OpSta(memory, cpu, OpAbs(cpu, OUTCOME_MATCH_TYPES));
        OpLda(memory, cpu, OpDp(cpu, OUTCOME_MODE));
        OpCmpValue(cpu, 7u);
        fixed_payout = cpu->zero != 0u;
        if (!fixed_payout) {
            OpLoadA(cpu, 7u);
            OpSta(memory, cpu, OpDp(cpu, 0x0eu));
            if (!TableServiceCall(&service, 0x84e43eu, 0x84e829u, false) ||
                !TableServiceCall(&service, 0x84e441u, 0x84e489u, false) ||
                !TableServiceCall(&service, 0x84e444u, 0x84e3f6u, false)) return service.result;
            OpLoadA(cpu, 8u);
            OpSta(memory, cpu, OpDp(cpu, 0x0eu));
            if (!TableServiceCall(&service, 0x84e44cu, 0x84e829u, false)) return service.result;
        }
    }
    if (fixed_payout) {
        OpLoadA(cpu, 15u);
        OpSta(memory, cpu, OpDp(cpu, OUTCOME_AMOUNT));
    }
    OpLda(memory, cpu, OpAbs(cpu, OUTCOME_ROUNDS));
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_NUMBER));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_NUMBER_SLOT));
    if (!AnimateRoundCounter(&service, 0x84e472u, 0x84e481u)) return service.result;
    OpStz(memory, cpu, OpDp(cpu, OUTCOME_MODE));
    return service.result;
}

static bool AnimatePayoutUnit(TableService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    OpLda(memory, cpu, OpAbs(cpu, OUTCOME_DISPLAY_TOTAL));
    OpCmpValue(cpu, 50u);
    if (cpu->zero) {
        if (!TableServiceCall(service, 0x84e38eu, 0x84a421u, false)) return false;
        SetOutcomeTextMode(memory, cpu, 0x20u);
        OpRepWidths(cpu, 0x20u);
        OpLdx(cpu, 0x3694u);
        OpLdy(cpu, 0xf945u);
        if (!TableServiceCall(service, 0x84e3a4u, 0x808878u, false)) return false;
    } else {
        OpIncA(cpu);
        OpSta(memory, cpu, OpAbs(cpu, OUTCOME_DISPLAY_TOTAL));
        OpSta(memory, cpu, OpAbs(cpu, OUTCOME_NUMBER));
        OpLdx(cpu, 1u);
        OpWriteX(memory, cpu, OpAbs(cpu, OUTCOME_NUMBER_SLOT), cpu->x);
        OpLoadA(cpu, 0x22u);
        if (!TableServiceCall(service, 0x84e37au, 0x84a607u, false) ||
            !TableServiceCall(service, 0x84e37du, 0x84eb36u, false)) return false;
        OpLda(memory, cpu, OpAbs(cpu, OUTCOME_CHANGED_DIGITS));
        OpSta(memory, cpu, OpAbs(cpu, OUTCOME_SECOND_MOVEMENT));
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, OpAbs(cpu, OUTCOME_SECOND_PENDING));
    }
    OpLda(memory, cpu, OpDp(cpu, OUTCOME_REMAINING));
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_NUMBER));
    OpLdx(cpu, 2u);
    OpWriteX(memory, cpu, OpAbs(cpu, OUTCOME_NUMBER_SLOT), cpu->x);
    if (!TableServiceCall(service, 0x84e3b3u, 0x84eb36u, false)) return false;
    OpLda(memory, cpu, OpAbs(cpu, OUTCOME_CHANGED_DIGITS));
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_THIRD_MOVEMENT));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_THIRD_PENDING));
    if (!TableServiceCall(service, 0x84e3c2u, 0x84ebdbu, false)) return false;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x88u);
    OpSta(memory, cpu, OpDp(cpu, OUTCOME_REQUEST));
    if (!TableServiceCall(service, 0x84e3cbu, 0x84e9b1u, false)) return false;
    OpRepWidths(cpu, 0x20u);
    return true;
}

static bool RestoreRoundSong(TableService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    OpLda(memory, cpu, OpAbs(cpu, OUTCOME_SONG));
    if (cpu->zero) return true;
    if (!TableServiceCall(service, 0x84e3ddu, 0x84a60cu, false)) return false;
    OpStz(memory, cpu, OpAbs(cpu, OUTCOME_SONG));
    if (!TableServiceCall(service, 0x84e3e3u, 0x84f01au, false)) return false;
    OpLda(memory, cpu, OpAbs(cpu, OUTCOME_VARIANT));
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_SAVED_VARIANT));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_REFRESH_TIMER));
    return TableServiceCall(service, 0x84e3f2u, 0x84e9b1u, false);
}

Lufia2ExecutionResult Lufia2CardTablePayRound(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !OutcomeWordEntry(cpu)) return ExecutionHandoff(cpu, 0x84e334u);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84e3f5u), false};
    ClearOutcomeCounters(memory, cpu);
    if (!TableServiceCall(&service, 0x84e342u, 0x84e416u, false) ||
        !TableServiceCall(&service, 0x84e345u, 0x84e3f6u, false)) return service.result;
    OpLda(memory, cpu, OpDp(cpu, OUTCOME_AMOUNT));
    if (cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, OUTCOME_MODE));
        OpStz(memory, cpu, OpDp(cpu, OUTCOME_MODE));
    } else {
        OpStepMem(memory, cpu, OpDp(cpu, OUTCOME_AMOUNT), 1);
        OpLoadA(cpu, 10u);
        OpSta(memory, cpu, OpAbs(cpu, OUTCOME_PAYOUT_STEP));
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, OpDp(cpu, OUTCOME_REMAINING));
        do {
            if (!AnimatePayoutUnit(&service)) return service.result;
            OpStepMem(memory, cpu, OpDp(cpu, OUTCOME_REMAINING), 1);
            OpLda(memory, cpu, OpDp(cpu, OUTCOME_REMAINING));
            OpCmp(memory, cpu, OpDp(cpu, OUTCOME_AMOUNT));
        } while (!cpu->zero);
    }
    RestoreRoundSong(&service);
    return service.result;
}
