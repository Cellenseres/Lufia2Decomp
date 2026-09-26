#ifndef LUFIA2_CORE_CPU_INTERNAL_H
#define LUFIA2_CORE_CPU_INTERNAL_H

/* 65816 register, flag and stack semantics. */

#include "core/memory_internal.h"
#include "core/snes_registers.h"

static inline void SetNz8(Lufia2CpuState *cpu, uint8_t value) {
    cpu->negative = (value & 0x80u) != 0;
    cpu->zero = value == 0;
}

static inline void SetNz16(Lufia2CpuState *cpu, uint16_t value) {
    cpu->negative = (value & 0x8000u) != 0;
    cpu->zero = value == 0;
}

static inline uint8_t A8(const Lufia2CpuState *cpu) {
    return (uint8_t)cpu->accumulator;
}

static inline void LoadA8(Lufia2CpuState *cpu, uint8_t value) {
    cpu->accumulator =
        (uint16_t)((cpu->accumulator & 0xff00u) | value);
    SetNz8(cpu, value);
}

static inline void LoadA16(Lufia2CpuState *cpu, uint16_t value) {
    cpu->accumulator = value;
    SetNz16(cpu, value);
}

static inline void LoadX16(Lufia2CpuState *cpu, uint16_t value) {
    cpu->x = value;
    SetNz16(cpu, value);
}

static inline void LoadY16(Lufia2CpuState *cpu, uint16_t value) {
    cpu->y = value;
    SetNz16(cpu, value);
}

static inline void And8(Lufia2CpuState *cpu, uint8_t value) {
    LoadA8(cpu, (uint8_t)(A8(cpu) & value));
}

static inline void AslA8(Lufia2CpuState *cpu) {
    const uint8_t old = A8(cpu);
    cpu->carry = (old & 0x80u) != 0;
    LoadA8(cpu, (uint8_t)(old << 1));
}

static inline void AslA16(Lufia2CpuState *cpu) {
    const uint16_t old = cpu->accumulator;
    cpu->carry = (old & 0x8000u) != 0;
    LoadA16(cpu, (uint16_t)(old << 1));
}

static inline void LsrA8(Lufia2CpuState *cpu) {
    const uint8_t old = A8(cpu);
    cpu->carry = old & 1u;
    LoadA8(cpu, (uint8_t)(old >> 1));
}

static inline void TransferAToX(Lufia2CpuState *cpu) {
    if (cpu->index_is_8_bit) {
        cpu->x = A8(cpu);
        SetNz8(cpu, (uint8_t)cpu->x);
    } else {
        cpu->x = cpu->accumulator;
        SetNz16(cpu, cpu->x);
    }
}

static inline void TransferAToY(Lufia2CpuState *cpu) {
    if (cpu->index_is_8_bit) {
        cpu->y = A8(cpu);
        SetNz8(cpu, (uint8_t)cpu->y);
    } else {
        cpu->y = cpu->accumulator;
        SetNz16(cpu, cpu->y);
    }
}

static inline void TransferXToA(Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit)
        LoadA8(cpu, (uint8_t)cpu->x);
    else
        LoadA16(cpu, cpu->x);
}

static inline void TransferDirectToA(Lufia2CpuState *cpu) {
    cpu->accumulator = cpu->direct_page;
    SetNz16(cpu, cpu->accumulator);
}

static inline void SetIndexWidth(Lufia2CpuState *cpu, uint8_t narrow) {
    cpu->index_is_8_bit = narrow != 0;
    if (narrow) {
        cpu->x &= 0x00ffu;
        cpu->y &= 0x00ffu;
    }
}

static inline void SetAccumulatorWidth(
    Lufia2CpuState *cpu, uint8_t narrow) {
    cpu->accumulator_is_8_bit = narrow != 0;
}


static inline void ExchangeAccumulatorBytes(Lufia2CpuState *cpu) {
    const uint16_t value = cpu->accumulator;
    cpu->accumulator =
        (uint16_t)((value << 8) | (value >> 8));
    SetNz8(cpu, A8(cpu));
}

static inline void And16(Lufia2CpuState *cpu, uint16_t value) {
    LoadA16(cpu, (uint16_t)(cpu->accumulator & value));
}

static inline void Add16Value(
    Lufia2CpuState *cpu, uint16_t value) {
    const uint16_t old = cpu->accumulator;
    const uint32_t sum =
        (uint32_t)old + value + (cpu->carry ? 1u : 0u);
    cpu->accumulator = (uint16_t)sum;
    cpu->carry = sum > 0xffffu;
    cpu->overflow =
        ((~(old ^ value) & (old ^ (uint16_t)sum)) & 0x8000u) != 0;
    SetNz16(cpu, cpu->accumulator);
}

static inline void Push8(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t value) {
    Write8(memory, cpu->stack, value);
    cpu->stack = (uint16_t)(cpu->stack - 1u);
}

static inline uint8_t Pull8(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    cpu->stack = (uint16_t)(cpu->stack + 1u);
    return Read8(memory, cpu->stack);
}


static inline void PushIndex(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (cpu->index_is_8_bit) {
        Push8(memory, cpu, (uint8_t)cpu->x);
    } else {
        Push8(memory, cpu, (uint8_t)(cpu->x >> 8));
        Push8(memory, cpu, (uint8_t)cpu->x);
    }
}

static inline void PullAccumulator16(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const uint8_t low = Pull8(memory, cpu);
    const uint8_t high = Pull8(memory, cpu);
    LoadA16(cpu, (uint16_t)(low | ((uint16_t)high << 8)));
}

static inline void PushDataBank(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Push8(memory, cpu, cpu->data_bank);
}

static inline void PushAccumulator8(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Push8(memory, cpu, A8(cpu));
}

static inline void PullDataBank(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    cpu->data_bank = Pull8(memory, cpu);
    SetNz8(cpu, cpu->data_bank);
}

static inline void LoadXDirect16(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t offset) {
    LoadX16(cpu, Read16Direct(memory, cpu, offset));
}

static inline void LoadXDirect(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t offset) {
    if (cpu->index_is_8_bit) {
        const uint8_t value = Read8(memory, DirectAddress(cpu, offset));
        cpu->x = value;
        SetNz8(cpu, value);
    } else {
        LoadXDirect16(memory, cpu, offset);
    }
}

static inline void LoadYDirect16(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t offset);

/* LDY dp at the current index width. */
static inline void LoadYDirect(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t offset) {
    if (cpu->index_is_8_bit) {
        const uint8_t value = Read8(memory, DirectAddress(cpu, offset));

        cpu->y = value;
        SetNz8(cpu, value);
    } else {
        LoadYDirect16(memory, cpu, offset);
    }
}

static inline void LoadYDirect16(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t offset) {
    LoadY16(cpu, Read16Direct(memory, cpu, offset));
}

static inline void StoreXDirect16(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu,
    uint8_t offset) {
    Write16Direct(memory, cpu, offset, cpu->x);
}

static inline void StoreYDirect16(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu,
    uint8_t offset) {
    Write16Direct(memory, cpu, offset, cpu->y);
}

static inline void TrbDirect8(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t offset) {
    const uint32_t address = DirectAddress(cpu, offset);
    const uint8_t value = Read8(memory, address);
    const uint8_t a = A8(cpu);
    cpu->zero = (value & a) == 0;
    Write8(memory, address, (uint8_t)(value & (uint8_t)~a));
}

static inline uint8_t LoadScriptByteY(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const uint8_t value = Read8(
        memory, AbsoluteIndexedAddress(cpu, 0x0000u, cpu->y));
    LoadA8(cpu, value);
    return value;
}

static inline uint8_t LoadScriptByteX(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const uint8_t value = Read8(
        memory, AbsoluteIndexedAddress(cpu, 0x0000u, cpu->x));
    LoadA8(cpu, value);
    return value;
}

static inline uint32_t JumpProgramTable(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu,
    uint16_t base) {
    const uint16_t pc = Read16ProgramIndexed(memory, cpu, base, cpu->x);
    return ((uint32_t)cpu->program_bank << 16) | pc;
}

static inline void Or8(Lufia2CpuState *cpu, uint8_t value) {
    LoadA8(cpu, (uint8_t)(A8(cpu) | value));
}

static inline void Add16Immediate(
    Lufia2CpuState *cpu, uint16_t value) {
    Add16Value(cpu, value);
}

static inline void IncrementY16(Lufia2CpuState *cpu) {
    cpu->y = (uint16_t)(cpu->y + 1u);
    SetNz16(cpu, cpu->y);
}

static inline void BitImmediate8(
    Lufia2CpuState *cpu, uint8_t value) {
    cpu->zero = (A8(cpu) & value) == 0;
}

static inline void Compare8(
    Lufia2CpuState *cpu, uint8_t left, uint8_t right) {
    cpu->carry = left >= right;
    SetNz8(cpu, (uint8_t)(left - right));
}

static inline void DecrementA8(Lufia2CpuState *cpu) {
    LoadA8(cpu, (uint8_t)(A8(cpu) - 1u));
}

/* Binary mode only; the game never sets D. */
static inline void Adc8(Lufia2CpuState *cpu, uint8_t value) {
    const uint8_t old = A8(cpu);
    const uint16_t sum =
        (uint16_t)old + value + (cpu->carry ? 1u : 0u);
    cpu->carry = sum > 0xffu;
    cpu->overflow =
        ((~(old ^ value) & (old ^ (uint8_t)sum)) & 0x80u) != 0;
    LoadA8(cpu, (uint8_t)sum);
}

static inline void Sbc8(Lufia2CpuState *cpu, uint8_t value) {
    Adc8(cpu, (uint8_t)~value);
}

static inline void SimulateJsrFrame(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    Push8(memory, cpu, (uint8_t)(return_address >> 8));
    Push8(memory, cpu, (uint8_t)return_address);
}

static inline void SimulateRtsFrame(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    (void)Pull8(memory, cpu);
    (void)Pull8(memory, cpu);
}


static inline void SimulateJslFrame(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t return_bank,
    uint16_t return_address) {
    Push8(memory, cpu, return_bank);
    Push8(memory, cpu, (uint8_t)(return_address >> 8));
    Push8(memory, cpu, (uint8_t)return_address);
}

static inline void SimulateRtlFrame(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    (void)Pull8(memory, cpu);
    (void)Pull8(memory, cpu);
    (void)Pull8(memory, cpu);
}

static inline uint16_t PullIndexValue(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

static inline void Compare16(
    Lufia2CpuState *cpu, uint16_t left, uint16_t right) {
    cpu->carry = left >= right;
    SetNz16(cpu, (uint16_t)(left - right));
}

static inline void IncrementDirect8(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t offset) {
    const uint8_t value =
        (uint8_t)(Read8(memory, DirectAddress(cpu, offset)) + 1u);
    Write8(memory, DirectAddress(cpu, offset), value);
    SetNz8(cpu, value);
}

static inline void DecrementDirect8(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t offset) {
    const uint8_t value =
        (uint8_t)(Read8(memory, DirectAddress(cpu, offset)) - 1u);
    Write8(memory, DirectAddress(cpu, offset), value);
    SetNz8(cpu, value);
}

static inline void CopyDirect8(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t from,
    uint8_t to) {
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, from)));
    Write8(memory, DirectAddress(cpu, to), A8(cpu));
}

static inline void TestBitsAbsolute8(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t address,
    uint8_t set) {
    const uint32_t effective = AbsoluteIndexedAddress(cpu, address, 0);
    const uint8_t value = Read8(memory, effective);
    const uint8_t a = A8(cpu);

    cpu->zero = (value & a) == 0;
    Write8(
        memory, effective,
        set ? (uint8_t)(value | a) : (uint8_t)(value & (uint8_t)~a));
}

static inline void LsrA16(Lufia2CpuState *cpu) {
    const uint16_t old = cpu->accumulator;
    cpu->carry = old & 1u;
    LoadA16(cpu, (uint16_t)(old >> 1));
}

static inline void PushAccumulator16(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Push8(memory, cpu, (uint8_t)(cpu->accumulator >> 8));
    Push8(memory, cpu, (uint8_t)cpu->accumulator);
}

static inline void IncrementA16(Lufia2CpuState *cpu) {
    LoadA16(cpu, (uint16_t)(cpu->accumulator + 1u));
}

static inline void LoadAAbsolute8(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t address,
    uint16_t index) {
    LoadA8(
        cpu, Read8(memory, AbsoluteIndexedAddress(cpu, address, index)));
}

static inline void StoreAAbsolute8(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu,
    uint16_t address,
    uint16_t index) {
    Write8(memory, AbsoluteIndexedAddress(cpu, address, index), A8(cpu));
}

static inline uint8_t PackStatus(const Lufia2CpuState *cpu) {
    return (uint8_t)(
        (cpu->negative ? 0x80u : 0u) |
        (cpu->overflow ? 0x40u : 0u) |
        (cpu->accumulator_is_8_bit ? 0x20u : 0u) |
        (cpu->index_is_8_bit ? 0x10u : 0u) |
        (cpu->decimal ? 0x08u : 0u) |
        (cpu->irq_disable ? 0x04u : 0u) |
        (cpu->zero ? 0x02u : 0u) |
        (cpu->carry ? 0x01u : 0u));
}

static inline void UnpackStatus(Lufia2CpuState *cpu, uint8_t status) {
    cpu->negative = (status & 0x80u) != 0;
    cpu->overflow = (status & 0x40u) != 0;
    cpu->decimal = (status & 0x08u) != 0;
    cpu->irq_disable = (status & 0x04u) != 0;
    cpu->zero = (status & 0x02u) != 0;
    cpu->carry = status & 0x01u;
    SetAccumulatorWidth(cpu, status & 0x20u);
    SetIndexWidth(cpu, status & 0x10u);
}

static inline void PushY(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!cpu->index_is_8_bit)
        Push8(memory, cpu, (uint8_t)(cpu->y >> 8));
    Push8(memory, cpu, (uint8_t)cpu->y);
}

static inline uint16_t PullIndexValue(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const uint8_t low = Pull8(memory, cpu);
    uint16_t value;

    if (cpu->index_is_8_bit) {
        SetNz8(cpu, low);
        return low;
    }
    value = (uint16_t)(low | ((uint16_t)Pull8(memory, cpu) << 8));
    SetNz16(cpu, value);
    return value;
}

static inline void LoadX8(Lufia2CpuState *cpu, uint8_t value) {
    cpu->x = value;
    SetNz8(cpu, value);
}

static inline void LoadY8(Lufia2CpuState *cpu, uint8_t value) {
    cpu->y = value;
    SetNz8(cpu, value);
}

/* Secondary actor VM ($83:D508). */

static inline void IncrementX16(Lufia2CpuState *cpu) {
    LoadX16(cpu, (uint16_t)(cpu->x + 1u));
}

static inline void TransferYToX(Lufia2CpuState *cpu) {
    if (cpu->index_is_8_bit) {
        cpu->x = (uint8_t)cpu->y;
        SetNz8(cpu, (uint8_t)cpu->x);
    } else {
        cpu->x = cpu->y;
        SetNz16(cpu, cpu->x);
    }
}

static inline void StepMemory8(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t address,
    int delta) {
    const uint8_t value = (uint8_t)(Read8(memory, address) + delta);
    Write8(memory, address, value);
    SetNz8(cpu, value);
}

static inline void CopyLong16(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t from,
    uint32_t to) {
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(from, cpu->x)));
    Write16Long(memory, LongIndexedAddress(to, cpu->x), cpu->accumulator);
}

static inline void AddLong16(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t address) {
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, LongIndexedAddress(address, cpu->x)));
    Write16Long(memory, LongIndexedAddress(address, cpu->x), cpu->accumulator);
}

/* TYA with M=1. */
static inline void TransferYToA8(Lufia2CpuState *cpu) {
    LoadA8(cpu, (uint8_t)cpu->y);
}

static inline void ToggleLong8(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t address,
    uint8_t bits) {
    LoadA8(cpu, Read8(memory, address));
    LoadA8(cpu, (uint8_t)(A8(cpu) ^ bits));
    Write8(memory, address, A8(cpu));
}

static inline void StoreYIndex(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu,
    uint16_t address) {
    if (cpu->index_is_8_bit)
        Write8(memory, AbsoluteIndexedAddress(cpu, address, 0), (uint8_t)cpu->y);
    else
        Write16Absolute(memory, cpu, address, cpu->y);
}

static inline void StoreA8Absolute(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t address,
    uint8_t value) {
    LoadA8(cpu, value);
    StoreAAbsolute8(memory, cpu, address, 0);
}

/* A = base - value, 16-bit SBC after SEC. */
static inline void Subtract16(Lufia2CpuState *cpu, uint16_t value) {
    cpu->carry = 1;
    Add16Value(cpu, (uint16_t)~value);
}

static inline void Decrement16Direct(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t offset) {
    const uint16_t value = (uint16_t)(Read16Direct(memory, cpu, offset) - 1u);
    Write16Direct(memory, cpu, offset, value);
    SetNz16(cpu, value);
}

static inline void RolA8(Lufia2CpuState *cpu) {
    const uint8_t old = A8(cpu);
    const uint8_t value = (uint8_t)((old << 1) | (cpu->carry ? 1u : 0u));
    cpu->carry = (old & 0x80u) != 0;
    LoadA8(cpu, value);
}

static inline void PushAndSetDataBank(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t bank) {
    PushDataBank(memory, cpu);
    LoadA8(cpu, bank);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
}

static inline void StoreADirect8(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu,
    uint8_t offset) {
    Write8(memory, DirectAddress(cpu, offset), A8(cpu));
}

static inline void StoreZeroAbsolute8(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu,
    uint16_t address,
    uint16_t index) {
    Write8(memory, AbsoluteIndexedAddress(cpu, address, index), 0x00u);
}

static inline void RorA8(Lufia2CpuState *cpu) {
    const uint8_t old = A8(cpu);
    const uint8_t value =
        (uint8_t)((old >> 1) | (cpu->carry ? 0x80u : 0u));
    cpu->carry = old & 1u;
    LoadA8(cpu, value);
}

/* BIT abs, 16-bit: N and V from memory. */
static inline void BitAbsolute16(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t address) {
    const uint16_t value = Read16AbsoluteIndexed(memory, cpu, address, 0);

    cpu->zero = (cpu->accumulator & value) == 0;
    cpu->negative = (value & 0x8000u) != 0;
    cpu->overflow = (value & 0x4000u) != 0;
}

/* TSB/TRB dp at the current accumulator width. */
static inline void TestBitsDirect(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t offset,
    uint8_t set) {
    if (cpu->accumulator_is_8_bit) {
        const uint8_t value = DirectByte(memory, cpu, offset);

        cpu->zero = (value & A8(cpu)) == 0;
        Write8(memory, DirectAddress(cpu, offset), set
            ? (uint8_t)(value | A8(cpu)) : (uint8_t)(value & ~A8(cpu)));
    } else {
        const uint16_t value = Read16Direct(memory, cpu, offset);

        cpu->zero = (value & cpu->accumulator) == 0;
        Write16Direct(memory, cpu, offset, set
            ? (uint16_t)(value | cpu->accumulator)
            : (uint16_t)(value & ~cpu->accumulator));
    }
}

static inline void LoadXDirect8(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t offset) {
    LoadX8(cpu, DirectByte(memory, cpu, offset));
}

static inline void LoadYDirect8(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t offset) {
    LoadY8(cpu, DirectByte(memory, cpu, offset));
}

static inline void LoadADirect16(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t offset) {
    LoadA16(cpu, Read16Direct(memory, cpu, offset));
}

static inline void StoreADirect16(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu,
    uint8_t offset) {
    Write16Direct(memory, cpu, offset, cpu->accumulator);
}

static inline void StoreAAbsolute16(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu,
    uint16_t address,
    uint16_t index) {
    Write16Long(memory, AbsoluteIndexedAddress(cpu, address, index),
        cpu->accumulator);
}

static inline void StoreAImmediate8(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t value,
    uint16_t address) {
    LoadA8(cpu, value);
    StoreAAbsolute8(memory, cpu, address, 0);
}

static inline void CopyAbsolute8(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t from,
    uint16_t to) {
    LoadAAbsolute8(memory, cpu, from, 0);
    StoreAAbsolute8(memory, cpu, to, 0);
}

static inline Lufia2ExecutionResult ExecutionReturned(uint32_t exit) {
    Lufia2ExecutionResult result;

    result.flow = LUFIA2_EXECUTION_RETURNED;
    result.pc = exit;
    result.dispatches = 0;
    return result;
}

static inline Lufia2ExecutionResult ExecutionHandoff(
    Lufia2CpuState *cpu, uint32_t pc) {
    Lufia2ExecutionResult result = ExecutionReturned(pc);

    result.flow = LUFIA2_EXECUTION_BOUNDARY;
    cpu->resume_pc = pc;
    return result;
}

/* LDX #last; loop: STZ address,X; DEX; BPL loop (X16). */
static inline void ClearDescendingX16(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t address,
    uint16_t last) {
    LoadX16(cpu, last);
    do {
        StoreZeroAbsolute8(memory, cpu, address, cpu->x);
        LoadX16(cpu, (uint16_t)(cpu->x - 1u));
    } while (!cpu->negative);
}

#endif
