#ifndef LUFIA2_CORE_CPU_OPS_H
#define LUFIA2_CORE_CPU_OPS_H

/* Width-aware instruction adapters over cpu_internal.h. */

#include "core/cpu_internal.h"

/* OP_DP_WRAP keeps 16-bit direct-page accesses in bank 0. */
#define OP_DP_WRAP 0x01000000u

static inline uint32_t OpDp(const Lufia2CpuState *cpu, uint8_t offset) {
    return DirectAddress(cpu, offset) | OP_DP_WRAP;
}

/* Stack-relative operands also wrap within bank zero. */
static inline uint32_t OpStack(const Lufia2CpuState *cpu, uint8_t offset) {
    return (uint16_t)(cpu->stack + offset) | OP_DP_WRAP;
}

static inline uint32_t OpAbs(const Lufia2CpuState *cpu, uint16_t address) {
    return AbsoluteIndexedAddress(cpu, address, 0);
}

static inline uint32_t OpAbsX(const Lufia2CpuState *cpu, uint16_t address) {
    return AbsoluteIndexedAddress(cpu, address, cpu->x);
}

static inline uint32_t OpAbsY(const Lufia2CpuState *cpu, uint16_t address) {
    return AbsoluteIndexedAddress(cpu, address, cpu->y);
}

static inline uint32_t OpLongX(const Lufia2CpuState *cpu, uint32_t address) {
    return LongIndexedAddress(address, cpu->x);
}

static inline uint32_t OpNextByte(uint32_t address) {
    if (address & OP_DP_WRAP)
        return (uint16_t)(address + 1u);
    return (address + 1u) & 0x00ffffffu;
}

static inline uint16_t OpRead16(const Lufia2Memory *memory, uint32_t address) {
    const uint8_t low = Read8(memory, address);
    return (uint16_t)(low | ((uint16_t)Read8(memory, OpNextByte(address)) << 8));
}

static inline void OpWrite16(
    const Lufia2Memory *memory, uint32_t address, uint16_t value) {
    Write8(memory, address, (uint8_t)value);
    Write8(memory, OpNextByte(address), (uint8_t)(value >> 8));
}

/* Accumulator operations at the current M width. */
static inline uint16_t OpA(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit ? A8(cpu) : cpu->accumulator;
}

static inline uint16_t OpReadM(
    const Lufia2Memory *memory, const Lufia2CpuState *cpu, uint32_t address) {
    return cpu->accumulator_is_8_bit ? Read8(memory, address)
                                     : OpRead16(memory, address);
}

static inline void OpWriteM(
    const Lufia2Memory *memory, const Lufia2CpuState *cpu, uint32_t address,
    uint16_t value) {
    if (cpu->accumulator_is_8_bit)
        Write8(memory, address, (uint8_t)value);
    else
        OpWrite16(memory, address, value);
}

static inline void OpLoadA(Lufia2CpuState *cpu, uint16_t value) {
    if (cpu->accumulator_is_8_bit)
        LoadA8(cpu, (uint8_t)value);
    else
        LoadA16(cpu, value);
}

static inline void OpLda(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t address) {
    OpLoadA(cpu, OpReadM(memory, cpu, address));
}

static inline void OpSta(
    const Lufia2Memory *memory, const Lufia2CpuState *cpu, uint32_t address) {
    OpWriteM(memory, cpu, address, cpu->accumulator);
}

static inline void OpStz(
    const Lufia2Memory *memory, const Lufia2CpuState *cpu, uint32_t address) {
    OpWriteM(memory, cpu, address, 0);
}

static inline void OpCmpValue(Lufia2CpuState *cpu, uint16_t value) {
    if (cpu->accumulator_is_8_bit)
        Compare8(cpu, A8(cpu), (uint8_t)value);
    else
        Compare16(cpu, cpu->accumulator, value);
}

static inline void OpCmp(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t address) {
    OpCmpValue(cpu, OpReadM(memory, cpu, address));
}

static inline void OpAdcValue(Lufia2CpuState *cpu, uint16_t value) {
    AccumulatorArithmetic(cpu, value, false);
}

static inline void OpAdc(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t address) {
    OpAdcValue(cpu, OpReadM(memory, cpu, address));
}

static inline void OpSbcValue(Lufia2CpuState *cpu, uint16_t value) {
    AccumulatorArithmetic(cpu, value, true);
}

static inline void OpAndValue(Lufia2CpuState *cpu, uint16_t value) {
    OpLoadA(cpu, (uint16_t)(OpA(cpu) & value));
}

static inline void OpOraValue(Lufia2CpuState *cpu, uint16_t value) {
    OpLoadA(cpu, (uint16_t)(OpA(cpu) | value));
}

static inline void OpOra(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t address) {
    OpOraValue(cpu, OpReadM(memory, cpu, address));
}

/* BIT #imm: only Z. */
static inline void OpBitValue(Lufia2CpuState *cpu, uint16_t value) {
    cpu->zero = (OpA(cpu) & value) == 0;
}

/* BIT mem: Z from A & m, N and V from m. */
static inline void OpBit(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t address) {
    const uint16_t value = OpReadM(memory, cpu, address);
    const uint16_t top = cpu->accumulator_is_8_bit ? 0x80u : 0x8000u;

    cpu->zero = (OpA(cpu) & value) == 0;
    cpu->negative = (value & top) != 0;
    cpu->overflow = (value & (top >> 1)) != 0;
}

static inline void OpIncA(Lufia2CpuState *cpu) {
    OpLoadA(cpu, (uint16_t)(OpA(cpu) + 1u));
}

static inline void OpDecA(Lufia2CpuState *cpu) {
    OpLoadA(cpu, (uint16_t)(OpA(cpu) - 1u));
}

static inline void OpAslA(Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit)
        AslA8(cpu);
    else
        AslA16(cpu);
}

static inline void OpLsrA(Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit)
        LsrA8(cpu);
    else
        LsrA16(cpu);
}

static inline void OpLsrMem(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t address) {
    const uint16_t old = OpReadM(memory, cpu, address);
    const uint16_t value = (uint16_t)(old >> 1);

    cpu->carry = old & 1u;
    if (cpu->accumulator_is_8_bit) {
        Write8(memory, address, (uint8_t)value);
        SetNz8(cpu, (uint8_t)value);
    } else {
        Write8(memory, OpNextByte(address), (uint8_t)(value >> 8));
        Write8(memory, address, (uint8_t)value);
        SetNz16(cpu, value);
    }
}

/* INC/DEC/TSB/TRB/ROL on memory at the current M width. */
static inline void OpStepMem(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t address,
    int delta) {
    const uint16_t value =
        (uint16_t)(OpReadM(memory, cpu, address) + delta);

    if (cpu->accumulator_is_8_bit) {
        Write8(memory, address, (uint8_t)value);
        SetNz8(cpu, (uint8_t)value);
    } else {
        /* 16-bit read-modify-write stores the high byte first. */
        Write8(memory, OpNextByte(address), (uint8_t)(value >> 8));
        Write8(memory, address, (uint8_t)value);
        SetNz16(cpu, value);
    }
}

static inline void OpTestBits(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t address,
    uint8_t set) {
    const uint16_t value = OpReadM(memory, cpu, address);
    const uint16_t a = OpA(cpu);

    cpu->zero = (value & a) == 0;
    const uint16_t result =
        set ? (uint16_t)(value | a) : (uint16_t)(value & (uint16_t)~a);
    if (cpu->accumulator_is_8_bit) {
        Write8(memory, address, (uint8_t)result);
    } else {
        /* Original word TSB/TRB stores the high byte before the low byte. */
        Write8(memory, OpNextByte(address), (uint8_t)(result >> 8));
        Write8(memory, address, (uint8_t)result);
    }
}

static inline void OpRolMem8(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t address) {
    const uint8_t old = Read8(memory, address);
    const uint8_t value = (uint8_t)((old << 1) | (cpu->carry ? 1u : 0u));

    cpu->carry = (old & 0x80u) != 0;
    Write8(memory, address, value);
    SetNz8(cpu, value);
}

/* Index operations at the current X width. */
static inline uint16_t OpReadX(
    const Lufia2Memory *memory, const Lufia2CpuState *cpu, uint32_t address) {
    return cpu->index_is_8_bit ? Read8(memory, address)
                               : OpRead16(memory, address);
}

static inline void OpWriteX(
    const Lufia2Memory *memory, const Lufia2CpuState *cpu, uint32_t address,
    uint16_t value) {
    if (cpu->index_is_8_bit)
        Write8(memory, address, (uint8_t)value);
    else
        OpWrite16(memory, address, value);
}

static inline uint16_t OpIndexValue(const Lufia2CpuState *cpu, uint16_t v) {
    return cpu->index_is_8_bit ? (uint8_t)v : v;
}

static inline void OpLdx(Lufia2CpuState *cpu, uint16_t value) {
    if (cpu->index_is_8_bit)
        LoadX8(cpu, (uint8_t)value);
    else
        LoadX16(cpu, value);
}

static inline void OpLdy(Lufia2CpuState *cpu, uint16_t value) {
    if (cpu->index_is_8_bit)
        LoadY8(cpu, (uint8_t)value);
    else
        LoadY16(cpu, value);
}

static inline void OpInx(Lufia2CpuState *cpu) {
    OpLdx(cpu, (uint16_t)(cpu->x + 1u));
}

static inline void OpDex(Lufia2CpuState *cpu) {
    OpLdx(cpu, (uint16_t)(cpu->x - 1u));
}

static inline void OpIny(Lufia2CpuState *cpu) {
    OpLdy(cpu, (uint16_t)(cpu->y + 1u));
}

static inline void OpDey(Lufia2CpuState *cpu) {
    OpLdy(cpu, (uint16_t)(cpu->y - 1u));
}

static inline void OpCompareIndex(
    Lufia2CpuState *cpu, uint16_t left, uint16_t right) {
    if (cpu->index_is_8_bit)
        Compare8(cpu, (uint8_t)left, (uint8_t)right);
    else
        Compare16(cpu, left, right);
}

static inline void OpCpx(Lufia2CpuState *cpu, uint16_t value) {
    OpCompareIndex(cpu, cpu->x, value);
}

static inline void OpCpy(Lufia2CpuState *cpu, uint16_t value) {
    OpCompareIndex(cpu, cpu->y, value);
}

static inline void OpTax(Lufia2CpuState *cpu) {
    TransferAToX(cpu);
}

static inline void OpTay(Lufia2CpuState *cpu) {
    TransferAToY(cpu);
}

static inline void OpTxa(Lufia2CpuState *cpu) {
    TransferXToA(cpu);
}

static inline void OpTya(Lufia2CpuState *cpu) {
    OpLoadA(cpu, cpu->y);
}

static inline void OpTxy(Lufia2CpuState *cpu) {
    OpLdy(cpu, cpu->x);
}

static inline void OpTyx(Lufia2CpuState *cpu) {
    TransferYToX(cpu);
}

static inline void OpPushX(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushIndex(memory, cpu);
}

static inline void OpPullX(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    cpu->x = PullIndexValue(memory, cpu);
}

static inline void OpPullY(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    cpu->y = PullIndexValue(memory, cpu);
}

/* M/X width bits only; not a general REP/SEP. */
static inline void OpRepWidths(Lufia2CpuState *cpu, uint8_t width_bits) {
    if (width_bits & 0x20u)
        SetAccumulatorWidth(cpu, 0);
    if (width_bits & 0x10u)
        SetIndexWidth(cpu, 0);
}

static inline void OpSepWidths(Lufia2CpuState *cpu, uint8_t width_bits) {
    if (width_bits & 0x20u)
        SetAccumulatorWidth(cpu, 1);
    if (width_bits & 0x10u)
        SetIndexWidth(cpu, 1);
}

/* LDA #bank; PHA; PLB. */
static inline void OpSetDataBank(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint8_t bank) {
    LoadA8(cpu, bank);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
}

/* MVN dst,src with X16/Y16: A+1 bytes ascending, DB = dst, A = $FFFF. */
static inline void OpMoveNext(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t destination, uint8_t source) {
    cpu->data_bank = destination;
    do {
        Write8(memory, ((uint32_t)destination << 16) | cpu->y,
            Read8(memory, ((uint32_t)source << 16) | cpu->x));
        cpu->x = OpIndexValue(cpu, (uint16_t)(cpu->x + 1u));
        cpu->y = OpIndexValue(cpu, (uint16_t)(cpu->y + 1u));
        cpu->accumulator = (uint16_t)(cpu->accumulator - 1u);
    } while (cpu->accumulator != 0xffffu);
}

#endif
