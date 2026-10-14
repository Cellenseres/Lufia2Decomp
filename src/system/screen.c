/* Screen initialization and brightness fade. */

#include "core/cpu_internal.h"
#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "lufia2/system.h"
#include "system/system_internal.h"
#include "system/wram.h"

/* $80:86C1: screen fade from $0581 into the $0583 brightness. */
Lufia2ExecutionResult Lufia2ScreenFade(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x8086c1u);
    LoadAAbsolute8(memory, cpu, WRAM_FADE_CONTROL, 0);         /* 86C1 */
    if (!cpu->negative)
        return ExecutionReturned(0x808702u);
    ExchangeAccumulatorBytes(cpu);
    LoadAAbsolute8(memory, cpu, WRAM_FADE_CONTROL, 0);
    And8(cpu, 0x3fu);
    cpu->carry = 0;
    Adc8(cpu, AbsoluteByte(memory, cpu, WRAM_FADE_LEVEL, 0));
    StoreAAbsolute8(memory, cpu, WRAM_FADE_LEVEL, 0);
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    ExchangeAccumulatorBytes(cpu);
    BitImmediate8(cpu, 0x40u);
    if (!cpu->zero) {
        ExchangeAccumulatorBytes(cpu);                         /* 86DB */
        cpu->carry = 1;
        Sbc8(cpu, 0x0fu);
        LoadA8(cpu, (uint8_t)(A8(cpu) ^ 0xffu));
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        StoreAAbsolute8(memory, cpu, WRAM_BRIGHTNESS, 0);
        DecrementA8(cpu);
        if (!cpu->negative)
            return ExecutionReturned(0x808702u);
        StoreZeroAbsolute8(memory, cpu, WRAM_FADE_CONTROL, 0);
        StoreAImmediate8(memory, cpu, BRIGHTNESS_FORCED_BLANK, WRAM_BRIGHTNESS);
    } else {
        ExchangeAccumulatorBytes(cpu);                         /* 86F2 */
        StoreAAbsolute8(memory, cpu, WRAM_BRIGHTNESS, 0);
        Compare8(cpu, A8(cpu), 0x0fu);
        if (!cpu->carry)
            return ExecutionReturned(0x808702u);
        StoreZeroAbsolute8(memory, cpu, WRAM_FADE_CONTROL, 0);
        StoreAImmediate8(memory, cpu, BRIGHTNESS_FULL, WRAM_BRIGHTNESS);
    }
    return ExecutionReturned(0x808702u);
}

enum {
    ROM_VIDEO_REGISTER_BYTES = 0x8212u,
    ROM_VIDEO_REGISTER_PAIRS = 0x825eu,
    ROM_VIDEO_CLEAR_BYTE = 0x8211u,
    INITIAL_RAM_FLAGS = 0x0100u,
    INITIAL_RAM_BYTES = 0x0300u
};

static void InitializeVideoRegisters(const Lufia2Memory *memory,
                                     Lufia2CpuState *cpu, uint16_t table,
                                     uint8_t paired) {
    OpLdy(cpu, 0u);
    do {
        OpLdx(cpu, OpReadX(memory, cpu, OpAbsY(cpu, table)));
        OpIny(cpu);
        OpIny(cpu);
        for (;;) {
            OpLda(memory, cpu, OpAbsY(cpu, table));
            OpIny(cpu);
            OpCmpValue(cpu, 0xffu);
            if (cpu->zero)
                break;
            OpSta(memory, cpu, DirectIndexedAddress(cpu, 0u, cpu->x));
            if (paired) {
                OpLda(memory, cpu, OpAbsY(cpu, table));
                OpSta(memory, cpu, DirectIndexedAddress(cpu, 0u, cpu->x));
                OpIny(cpu);
            }
            OpInx(cpu);
        }
        OpLda(memory, cpu, OpAbsY(cpu, table));
        OpCmpValue(cpu, 0xffu);
    } while (!cpu->zero);
}

static void PrepareVideoClearDma(const Lufia2Memory *memory,
                                 Lufia2CpuState *cpu, unsigned channel,
                                 uint8_t mode, uint8_t port, uint16_t count) {
    OpLoadA(cpu, mode);
    OpSta(memory, cpu, OpAbs(cpu, (uint16_t)SNES_DMAP(channel)));
    OpLoadA(cpu, port);
    OpSta(memory, cpu, OpAbs(cpu, (uint16_t)SNES_BBAD(channel)));
    OpLdx(cpu, ROM_VIDEO_CLEAR_BYTE);
    OpWriteX(memory, cpu, OpAbs(cpu, (uint16_t)SNES_A1TL(channel)), cpu->x);
    OpStz(memory, cpu, OpAbs(cpu, (uint16_t)SNES_A1B(channel)));
    OpLdx(cpu, count);
    OpWriteX(memory, cpu, OpAbs(cpu, (uint16_t)SNES_DASL(channel)), cpu->x);
}

static void ClearVideoMemory(const Lufia2Memory *memory,
                            Lufia2CpuState *cpu) {
    OpStz(memory, cpu, OpAbs(cpu, SNES_VMADDL));
    OpStz(memory, cpu, OpAbs(cpu, SNES_VMADDH));
    PrepareVideoClearDma(memory, cpu, 0u, 9u, 0x18u, 0u);
    OpStz(memory, cpu, OpAbs(cpu, SNES_OAMADDL));
    OpStz(memory, cpu, OpAbs(cpu, SNES_OAMADDL + 1u));
    PrepareVideoClearDma(memory, cpu, 1u, 8u, 4u, 0x2020u);
    OpStz(memory, cpu, OpAbs(cpu, SNES_CGADD));
    PrepareVideoClearDma(memory, cpu, 2u, 0u, 0x22u, 0x0200u);
    OpLoadA(cpu, 7u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_MDMAEN));
}

static void InitializeLowRam(const Lufia2Memory *memory,
                             Lufia2CpuState *cpu) {
    OpLdx(cpu, 0x01feu);
    OpLoadA(cpu, 0xffu);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, INITIAL_RAM_FLAGS));
        OpSta(memory, cpu, OpAbsX(cpu, INITIAL_RAM_FLAGS + 1u));
        OpDex(cpu);
        OpDex(cpu);
    } while (!cpu->zero);
    OpLdx(cpu, 0x0010u);
    OpLoadA(cpu, 0u);
    do {
        OpSta(memory, cpu, OpAbsX(cpu, INITIAL_RAM_BYTES));
        OpSta(memory, cpu, OpAbsX(cpu, INITIAL_RAM_BYTES + 1u));
        OpDex(cpu);
        OpDex(cpu);
    } while (!cpu->zero);
}

static void ClearInitialRamRange(const Lufia2Memory *memory,
                                 Lufia2CpuState *cpu, uint16_t base,
                                 uint16_t last) {
    OpLoadA(cpu, last);
    do {
        OpTax(cpu);
        for (uint16_t offset = 0u; offset < 8u; offset += 2u)
            OpStz(memory, cpu, OpAbsX(cpu, (uint16_t)(base + offset)));
        cpu->carry = 1u;
        OpSbcValue(cpu, 8u);
    } while (!cpu->negative);
}

Lufia2ExecutionResult Lufia2SystemInitializeVideoMemory(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x80u || cpu->decimal || cpu->direct_page)
        return ExecutionHandoff(cpu, 0x808117u);
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    OpLoadA(cpu, 0x80u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    InitializeVideoRegisters(memory, cpu, ROM_VIDEO_REGISTER_BYTES, 0u);
    InitializeVideoRegisters(memory, cpu, ROM_VIDEO_REGISTER_PAIRS, 1u);
    ClearVideoMemory(memory, cpu);
    InitializeLowRam(memory, cpu);
    OpLoadA(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    ClearInitialRamRange(memory, cpu, 0u, 0xdff8u);
    ClearInitialRamRange(memory, cpu, 0x2000u, 0x1ff8u);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x808210u);
}
