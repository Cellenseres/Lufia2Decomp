#include "core/cpu_ops.h"
#include "lufia2/system.h"

enum {
    DP_VECTOR_DIVISOR = 0x58u,
    DP_VECTOR_QUOTIENT = 0x63u,
    DP_VECTOR_DIVIDEND_HIGH = 0x65u,
    VECTOR_DIVISION_BITS = 16u
};

static void ShiftDividendWord(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t offset, uint8_t incoming_bit) {
    const uint32_t address = OpDp(cpu, offset);
    const uint16_t original = OpRead16(memory, address);
    const uint16_t shifted = (uint16_t)((original << 1) | incoming_bit);
    cpu->carry = (original & 0x8000u) != 0u;
    Write8(memory, OpNextByte(address), (uint8_t)(shifted >> 8));
    Write8(memory, address, (uint8_t)shifted);
    SetNz16(cpu, shifted);
}

Lufia2ExecutionResult Lufia2SystemDivideVectorMagnitude(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x85u || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x85db6du);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    OpLda(memory, cpu, OpDp(cpu, DP_VECTOR_DIVIDEND_HIGH + 1u));
    OpAndValue(cpu, 0xffu);
    OpLdx(cpu, OpRead16(memory, OpDp(cpu, DP_VECTOR_QUOTIENT + 1u)));
    OpWriteX(memory, cpu, OpDp(cpu, DP_VECTOR_DIVIDEND_HIGH), cpu->x);
    OpLdx(cpu, OpRead16(memory, OpDp(cpu, DP_VECTOR_QUOTIENT)));
    OpWriteX(memory, cpu, OpDp(cpu, DP_VECTOR_QUOTIENT + 1u), cpu->x);
    for (unsigned bit = 0u; bit < VECTOR_DIVISION_BITS; ++bit) {
        ShiftDividendWord(memory, cpu, DP_VECTOR_QUOTIENT, 0u);
        ShiftDividendWord(memory, cpu, DP_VECTOR_DIVIDEND_HIGH, cpu->carry);
        RolA16(cpu);
        if (!cpu->carry)
            OpCmp(memory, cpu, OpDp(cpu, DP_VECTOR_DIVISOR));
        if (cpu->carry) {
            OpSbcValue(cpu, OpRead16(memory, OpDp(cpu, DP_VECTOR_DIVISOR)));
            OpStepMem(memory, cpu, OpDp(cpu, DP_VECTOR_QUOTIENT), 1);
        }
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x85dc6eu);
}
