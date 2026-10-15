#include "core/cpu_internal.h"
#include "lufia2/system.h"
#include "system/gold_internal.h"
#include "system/wram.h"

enum { GOLD_CAP_LOW = 0x967fu, GOLD_CAP_HIGH = 0x98u, DP_GOLD_AMOUNT = 0x54u };

static void AddGold(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_GOLD, 0));
    Write16Absolute(memory, cpu, WRAM_GOLD, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadAAbsolute8(memory, cpu, (WRAM_GOLD + 2u), 0);
    Adc8(cpu, 0x00u);
    StoreAAbsolute8(memory, cpu, (WRAM_GOLD + 2u), 0);
    Compare8(cpu, A8(cpu), GOLD_CAP_HIGH);
    if (cpu->carry) {
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_GOLD, 0));
        Compare16(cpu, cpu->accumulator, GOLD_CAP_LOW);
        if (cpu->carry) {
            LoadA16(cpu, GOLD_CAP_LOW);
            Write16Absolute(memory, cpu, WRAM_GOLD, cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            StoreAImmediate8(memory, cpu, GOLD_CAP_HIGH, WRAM_GOLD + 2u);
        }
    }
}

static void SpendGold(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Write16Direct(memory, cpu, DP_GOLD_AMOUNT, cpu->accumulator);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_GOLD, 0));
    cpu->carry = 1;
    Add16Value(cpu, (uint16_t)~Read16Direct(memory, cpu, DP_GOLD_AMOUNT));
    Write16Absolute(memory, cpu, WRAM_GOLD, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadAAbsolute8(memory, cpu, (WRAM_GOLD + 2u), 0);
    Sbc8(cpu, 0x00u);
    StoreAAbsolute8(memory, cpu, (WRAM_GOLD + 2u), 0);
    if (!cpu->carry) {
        uint8_t high;

        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_GOLD, 0));
        cpu->carry = 0;
        Add16Value(cpu, Read16Direct(memory, cpu, DP_GOLD_AMOUNT));
        Write16Absolute(memory, cpu, WRAM_GOLD, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        high =
            (uint8_t)(Read8(memory, AbsoluteIndexedAddress(cpu, (WRAM_GOLD + 2u), 0)) +
                      1u);
        Write8(memory, AbsoluteIndexedAddress(cpu, (WRAM_GOLD + 2u), 0), high);
        SetNz8(cpu, high);
    }
}

void Lufia2ChangeGold(const Lufia2Memory *memory, Lufia2CpuState *cpu, bool add) {
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 0u);
    if (add)
        AddGold(memory, cpu);
    else
        SpendGold(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
}

static Lufia2ExecutionResult ChangeGold(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                        bool add) {
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal ||
        cpu->stack < 0x1f20u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, add ? 0x80bf12u : 0x80bf43u);
    Lufia2ChangeGold(memory, cpu, add);
    return ExecutionReturned(add ? 0x80bf42u : 0x80bf6eu);
}

Lufia2ExecutionResult Lufia2GoldAdd(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return ChangeGold(memory, cpu, true);
}

Lufia2ExecutionResult Lufia2GoldSpend(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return ChangeGold(memory, cpu, false);
}
