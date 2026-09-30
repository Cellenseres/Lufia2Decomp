#include "battle/battle_internal.h"

/* $85:9150: copy a name and remove trailing blank glyphs. */
Lufia2ExecutionResult Lufia2BattleStatusName(const Lufia2Memory *memory,
                                             Lufia2CpuState *cpu) {
    OpPushX(memory, cpu);
    OpLdy(cpu, 0u);
    for (;;) {
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        OpSta(memory, cpu, OpAbsY(cpu, 0x1269u));
        if (cpu->zero)
            break;
        OpInx(cpu);
        OpIny(cpu);
    }
    do {
        OpLda(memory, cpu, OpAbsY(cpu, 0x1268u));
        OpCmpValue(cpu, 0x10u);
        if (!cpu->zero)
            break;
        TransferDirectToA(cpu);
        OpSta(memory, cpu, OpAbsY(cpu, 0x1268u));
        OpDey(cpu);
    } while (!cpu->zero);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x1266u), cpu->y);
    OpPullX(memory, cpu);
    return ExecutionReturned(0x859172u);
}

/* $85:9173: append the selected status phrase from the original ROM. */
Lufia2ExecutionResult Lufia2BattleStatusPhrase(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    OpPushX(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x85efd3u));
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpSetDataBank(memory, cpu, 0x85u);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, 0x1266u)));
    for (;;) {
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        OpSta(memory, cpu, OpAbsY(cpu, 0x1269u));
        if (cpu->zero)
            break;
        OpInx(cpu);
        OpIny(cpu);
    }
    OpWriteX(memory, cpu, OpAbs(cpu, 0x1266u), cpu->y);
    OpPullX(memory, cpu);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x85919bu);
}

/* $85:91A1: synchronize five party status markers. */
Lufia2ExecutionResult Lufia2BattleSyncStatusMarkers(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpLdy(cpu, 0u);
    OpTyx(cpu);
    do {
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbsX(cpu, 0x0a64u));
        if (!cpu->zero) {
            OpPushX(memory, cpu);
            OpTax(cpu);
            OpSepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpAbsY(cpu, 0x147au));
            ExchangeAccumulatorBytes(cpu);
            OpLda(memory, cpu, OpAbsX(cpu, 0x0fu));
            OpSta(memory, cpu, OpAbsY(cpu, 0x147au));
            if (!cpu->zero) {
                ExchangeAccumulatorBytes(cpu);
                if (cpu->zero) {
                    OpLoadA(cpu, 0xffu);
                    OpSta(memory, cpu, OpAbsY(cpu, 0x147bu));
                    OpSta(memory, cpu, OpAbsY(cpu, 0x1479u));
                }
            } else {
                TransferDirectToA(cpu);
                OpSta(memory, cpu, OpAbsY(cpu, 0x1479u));
            }
            OpPullX(memory, cpu);
        }
        OpInx(cpu);
        OpInx(cpu);
        for (unsigned i = 0; i < 4u; ++i)
            OpIny(cpu);
        Compare16(cpu, cpu->y, 0x14u);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x8591dfu);
}

/* $85:91E0: clear party markers and five sprite-state words. */
Lufia2ExecutionResult Lufia2BattleClearStatusMarkers(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpLdy(cpu, 0u);
    OpTyx(cpu);
    TransferDirectToA(cpu);
    do {
        OpSta(memory, cpu, OpAbsY(cpu, 0x1479u));
        OpInx(cpu);
        OpInx(cpu);
        for (unsigned i = 0; i < 4u; ++i)
            OpIny(cpu);
        Compare16(cpu, cpu->y, 0x14u);
    } while (!cpu->zero);
    OpRepWidths(cpu, 0x20u);
    OpLdy(cpu, 0u);
    do {
        TransferDirectToA(cpu);
        OpSta(memory, cpu, OpAbsY(cpu, 0x1435u));
        OpTya(cpu);
        cpu->carry = 0;
        OpAdcValue(cpu, 0x0du);
        OpTay(cpu);
        Compare16(cpu, cpu->y, 0x41u);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x85920du);
}

/* $85:D9C9: effect-work pointer for a nonzero target mask. */
Lufia2ExecutionResult Lufia2BattleEffectRecord(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu) {
    OpSta(memory, cpu, OpAbs(cpu, 0x09fbu));
    OpAndValue(cpu, 0x80u);
    if (!cpu->zero)
        OpLoadA(cpu, 5u);
    OpSta(memory, cpu, OpAbs(cpu, 0x09fau));
    OpLoadA(cpu, 0xffu);
    do {
        OpIncA(cpu);
        const uint32_t address = OpAbs(cpu, 0x09fbu);
        const uint8_t old = Read8(memory, address);
        cpu->carry = old & 1u;
        Write8(memory, address, (uint8_t)(old >> 1));
        SetNz8(cpu, (uint8_t)(old >> 1));
    } while (!cpu->carry);
    cpu->carry = 0;
    OpAdc(memory, cpu, OpAbs(cpu, 0x09fau));
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x859ed6u));
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x85d9efu);
}
