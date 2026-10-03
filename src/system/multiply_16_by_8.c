#include "core/cpu_ops.h"
#include "core/arithmetic_value.h"
#include "core/snes_registers.h"
#include "core/wram_view.h"
#include "lufia2/system.h"

/* Work bytes of the multiplication. */
enum {
    MULTIPLICAND_LOW = 0x4eu,
    MULTIPLICAND_HIGH = 0x4fu,
    MULTIPLIER = 0x50u,
    PRODUCT = 0x51u, /* 24 bits: $51, $52, $53 */
    PRODUCT_MIDDLE = 0x52u,
    PRODUCT_TOP = 0x53u
};

/* 24-bit product of word $4E and byte $50 at $51. */
Lufia2ExecutionResult Lufia2Multiply16By8(const Lufia2Memory *memory,
                                          Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint16_t low_product, high_product, middle;
    uint8_t high_byte, multiplier;

    if (cpu->accumulator_is_8_bit)
        PushAccumulator8(memory, cpu);
    else
        PushAccumulator16(memory, cpu);
    OpPushX(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));

    WramWrite(wram, PRODUCT_TOP, 0);
    WramWrite(wram, SNES_WRMPYA, WramRead(wram, MULTIPLIER));
    WramWrite(wram, SNES_WRMPYB, WramRead(wram, MULTIPLICAND_LOW));
    high_byte = WramRead(wram, MULTIPLICAND_HIGH);
    multiplier = WramRead(wram, MULTIPLIER);
    low_product = WramRead16(wram, SNES_RDMPYL);
    WramWrite16(wram, SNES_WRMPYA,
        (uint16_t)(multiplier | ((uint16_t)high_byte << 8)));
    WramWrite16(wram, PRODUCT, low_product);
    middle = WramRead16(wram, PRODUCT_MIDDLE);
    high_product = WramRead16(wram, SNES_RDMPYL);
    cpu->accumulator = Sum16Mode(middle, high_product, false, cpu->decimal).value;
    WramWrite16(wram, PRODUCT_MIDDLE, cpu->accumulator);

    UnpackStatus(cpu, Pull8(memory, cpu));
    OpPullX(memory, cpu);
    if (cpu->accumulator_is_8_bit)
        LoadA8(cpu, Pull8(memory, cpu));
    else
        PullAccumulator16(memory, cpu);
    return ExecutionReturned(0x808377u);
}
