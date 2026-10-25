#include "menu/arithmetic_wait.h"
#include "menu/card_table_services.h"
#include "lufia2/card_table.h"
#include "system/wram.h"

enum {
    NUMBER_VALUE = WRAM_CARD_TABLE_PAYOUT_INDEX & 0xffffu,
    NUMBER_SLOT = WRAM_CARD_TABLE_PAYOUT_STATE & 0xffffu,
    NUMBER_CHANGED = WRAM_CARD_TABLE_PAYOUT_VALUE & 0xffffu,
    NUMBER_SPRITE_OFFSET = WRAM_CARD_TABLE_NUMBER_SPRITE_OFFSET & 0xffffu,
    NUMBER_COORDINATES = WRAM_CARD_TABLE_NUMBER_COORDINATES & 0xffffu,
    NUMBER_QUOTIENT = WRAM_CARD_TABLE_NUMBER_QUOTIENT & 0xffffu,
    NUMBER_PENDING = WRAM_CARD_TABLE_FIRST_NUMBER_PENDING & 0xffffu,
    NUMBER_MOVEMENT = WRAM_CARD_TABLE_FIRST_NUMBER_MOVEMENT & 0xffffu,
    NUMBER_OAM = WRAM_FIELD_OAM_LOW_BUFFER & 0xffffu,
    NUMBER_TILES = 0x84f2dcu,
    NUMBER_DIVIDEND = 0x004204u,
    NUMBER_DIVISOR = 0x004206u,
    NUMBER_DIVISION_RESULT = 0x004214u
};

static bool NumberWordEntry(const Lufia2CpuState *cpu) {
    return TableServiceReady(cpu, false) && !cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit;
}

Lufia2ExecutionResult Lufia2CardTableDivideNumberByTen(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!TableServiceReady(cpu, false))
        return ExecutionHandoff(cpu, 0x84ebc0u);
    OpSta(memory, cpu, NUMBER_DIVIDEND);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 10u);
    OpSta(memory, cpu, NUMBER_DIVISOR);
    WaitForArithmetic(memory, cpu, 0x84ebccu, 8u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, NUMBER_DIVISION_RESULT);
    return ExecutionReturned(0x84ebdau);
}

static void PrepareNumberSprites(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpStz(memory, cpu, OpAbs(cpu, NUMBER_CHANGED));
    OpLda(memory, cpu, OpAbs(cpu, NUMBER_SLOT));
    cpu->carry = 0u;
    for (unsigned shift = 0u; shift < 4u; ++shift) RolA16(cpu);
    OpTay(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x120u);
    OpSta(memory, cpu, OpAbs(cpu, NUMBER_SPRITE_OFFSET));
    OpTax(cpu);
    OpTya(cpu);
    RolA16(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, 0xd84fu);
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpAbs(cpu, NUMBER_COORDINATES));
    OpLdy(cpu, 4u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, NUMBER_OAM));
        OpSta(memory, cpu, OpAbsX(cpu, NUMBER_OAM + 8u));
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
}

static void DrawNumberTens(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSta(memory, cpu, OpAbs(cpu, NUMBER_QUOTIENT));
    cpu->carry = 0u;
    RolA16(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, NUMBER_TILES));
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, NUMBER_SPRITE_OFFSET)));
    OpCmp(memory, cpu, OpAbsX(cpu, NUMBER_OAM + 2u));
    if (!cpu->zero) {
        OpSta(memory, cpu, OpAbsX(cpu, NUMBER_OAM + 2u));
        OpLda(memory, cpu, OpAbs(cpu, NUMBER_COORDINATES));
        OpSta(memory, cpu, OpAbsX(cpu, NUMBER_OAM));
        OpStepMem(memory, cpu, OpAbs(cpu, NUMBER_CHANGED), 1);
    }
}

static void DrawNumberUnits(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, NUMBER_VALUE));
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, NUMBER_QUOTIENT)));
    for (;;) {
        OpDex(cpu);
        if (cpu->negative) break;
        cpu->carry = 1u;
        OpSbcValue(cpu, 10u);
    }
    cpu->carry = 0u;
    RolA16(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, NUMBER_TILES));
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, NUMBER_SPRITE_OFFSET)));
    OpCmp(memory, cpu, OpAbsX(cpu, NUMBER_OAM + 6u));
    if (!cpu->zero) {
        OpSta(memory, cpu, OpAbsX(cpu, NUMBER_OAM + 6u));
        OpLda(memory, cpu, OpAbs(cpu, NUMBER_COORDINATES));
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x10u);
        OpSta(memory, cpu, OpAbsX(cpu, NUMBER_OAM + 4u));
        OpLoadA(cpu, 2u);
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpAbs(cpu, NUMBER_CHANGED));
        OpSta(memory, cpu, OpAbs(cpu, NUMBER_CHANGED));
    }
}

Lufia2ExecutionResult Lufia2CardTableDrawNumber(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !TableServiceReady(cpu, false) || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x84eb36u);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84ebbfu), false};
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x20u);
    PrepareNumberSprites(memory, cpu);
    OpLda(memory, cpu, OpAbs(cpu, NUMBER_VALUE));
    if (!TableServiceCall(&service, 0x84eb68u, 0x84ebc0u, false))
        return service.result;
    DrawNumberTens(memory, cpu);
    DrawNumberUnits(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return service.result;
}

static void AdvanceDigitPair(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                             uint16_t offset) {
    for (unsigned copy = 0u; copy < 2u; ++copy) {
        const uint16_t address = (uint16_t)(NUMBER_OAM + offset + copy * 8u);
        OpLda(memory, cpu, OpAbsX(cpu, address));
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x400u);
        OpSta(memory, cpu, OpAbsX(cpu, address));
    }
}

Lufia2ExecutionResult Lufia2CardTableAdvanceNumberSprites(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!NumberWordEntry(cpu))
        return ExecutionHandoff(cpu, 0x84ec18u);
    PushY(memory, cpu);
    OpTay(cpu);
    OpAndValue(cpu, 1u);
    if (!cpu->zero) AdvanceDigitPair(memory, cpu, 0u);
    OpTya(cpu);
    OpAndValue(cpu, 2u);
    if (!cpu->zero) AdvanceDigitPair(memory, cpu, 4u);
    OpPullY(memory, cpu);
    return ExecutionReturned(0x84ec4eu);
}

Lufia2ExecutionResult Lufia2CardTableAnimateNumbers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !NumberWordEntry(cpu))
        return ExecutionHandoff(cpu, 0x84ebdbu);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84ec17u), false};
    static const uint32_t sites[] = {0x84ebe9u, 0x84ebf7u, 0x84ec05u};
    OpLdy(cpu, 4u);
    do {
        for (unsigned number = 0u; number < 3u; ++number) {
            OpLda(memory, cpu, OpAbs(cpu, (uint16_t)(NUMBER_PENDING + number * 2u)));
            if (!cpu->zero) {
                OpLda(memory, cpu, OpAbs(cpu, (uint16_t)(NUMBER_MOVEMENT + number * 2u)));
                OpLdx(cpu, (uint16_t)(0x120u + number * 0x10u));
                if (!TableServiceCall(&service, sites[number], 0x84ec18u, false))
                    return service.result;
            }
        }
        if (!TableServiceCall(&service, 0x84ec08u, 0x84e9b1u, false))
            return service.result;
        OpDey(cpu);
    } while (!cpu->zero);
    for (unsigned number = 0u; number < 3u; ++number)
        OpStz(memory, cpu, OpAbs(cpu, (uint16_t)(NUMBER_PENDING + number * 2u)));
    return service.result;
}
