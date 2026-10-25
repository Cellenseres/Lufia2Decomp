#include "menu/card_table_outcome.h"
#include "lufia2/card_table.h"

static void MarkPairedLines(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    do {
        OpLda(memory, cpu, OpAbsX(cpu, OUTCOME_MATCH_TYPES));
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbsX(cpu, OUTCOME_MATCH_SYMBOLS));
            if (!cpu->zero)
                OpStepMem(memory, cpu, OpAbsX(cpu, OUTCOME_LINE_COUNTERS), 1);
        }
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
}

Lufia2ExecutionResult Lufia2CardTableMarkPairedLines(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!OutcomeWordEntry(cpu)) return ExecutionHandoff(cpu, 0x84e3f6u);
    OpLda(memory, cpu, OpAbs(cpu, OUTCOME_SELECTION));
    cpu->carry = 0u;
    RolA16(cpu);
    OpDecA(cpu);
    OpTay(cpu);
    OpLdx(cpu, 0u);
    MarkPairedLines(memory, cpu);
    return ExecutionReturned(0x84e415u);
}

Lufia2ExecutionResult Lufia2CardTableMarkAlternatePairedLines(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!OutcomeWordEntry(cpu)) return ExecutionHandoff(cpu, 0x84f589u);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, OUTCOME_SELECTION)));
    OpLdx(cpu, 0u);
    MarkPairedLines(memory, cpu);
    return ExecutionReturned(0x84f5a4u);
}

Lufia2ExecutionResult Lufia2CardTableRestoreTilemap(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!TableServiceReady(cpu, false)) return ExecutionHandoff(cpu, 0x84f01au);
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    OpLdx(cpu, 0u);
    OpLdy(cpu, 0x0400u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, OUTCOME_SAVED_TILEMAP));
        OpSta(memory, cpu, OpAbsX(cpu, OUTCOME_VISIBLE_TILEMAP));
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    UnpackStatus(cpu, Pull8(memory, cpu));
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    return ExecutionReturned(0x84f033u);
}

Lufia2ExecutionResult Lufia2CardTableTryPayStake(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !OutcomeWordEntry(cpu)) return ExecutionHandoff(cpu, 0x84f74eu);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84f78eu), false};
    OpLda(memory, cpu, OpAbs(cpu, OUTCOME_BALANCE));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, OUTCOME_STAKE)));
    if (!cpu->carry) {
        OpLda(memory, cpu, OpAbs(cpu, OUTCOME_BALANCE_HIGH));
        cpu->carry = 1u;
        OpSbcValue(cpu, 1u);
        OpSta(memory, cpu, OpAbs(cpu, OUTCOME_BALANCE_HIGH));
        if (!cpu->carry) {
            OpStz(memory, cpu, OpAbs(cpu, OUTCOME_BALANCE_HIGH));
            OpLoadA(cpu, 0u);
            return ExecutionReturned(0x84f769u);
        }
    }
    OpLda(memory, cpu, OpAbs(cpu, OUTCOME_BALANCE));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, OUTCOME_STAKE)));
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_BALANCE));
    SetOutcomeTextMode(memory, cpu, 0x30u);
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0x3694u);
    OpLdy(cpu, 0xf945u);
    if (!TableServiceCall(&service, 0x84f787u, 0x808878u, false))
        return service.result;
    OpLoadA(cpu, 1u);
    return service.result;
}
