/* Screen initialization and brightness fade. */

#include "core/child_call.h"
#include "core/cpu_internal.h"
#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "lufia2/system.h"
#include "system/dp_scratch.h"
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

enum {
    DP_AUXILIARY_CALL_STUB = 0x6cu,
    DP_INITIAL_REQUEST_FIRST = 0x81u,
    DP_INITIAL_REQUEST_SECOND = 0x82u,
    RUNTIME_CALL_OPCODE = 0x22u,
    RUNTIME_RETURN_OPCODE = 0x60u,
    RUNTIME_MOVE_OPCODE = 0x54u
};

static void InitializeRuntimeCallTemplates(const Lufia2Memory *memory,
                                           Lufia2CpuState *cpu) {
    OpLoadA(cpu, RUNTIME_CALL_OPCODE);
    OpSta(memory, cpu, OpDp(cpu, DP_NMI_STUB));
    OpSta(memory, cpu, OpDp(cpu, DP_AUXILIARY_CALL_STUB));
    OpLoadA(cpu, RUNTIME_RETURN_OPCODE);
    OpSta(memory, cpu, OpDp(cpu, DP_NMI_STUB + 4u));
    OpSta(memory, cpu, OpDp(cpu, DP_AUXILIARY_CALL_STUB + 4u));
    OpLoadA(cpu, RUNTIME_MOVE_OPCODE);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_RAM_BLOCK_MOVE_OPCODE));
    OpLoadA(cpu, RUNTIME_RETURN_OPCODE);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_RAM_BLOCK_MOVE_RETURN));
}

Lufia2ExecutionResult Lufia2InitializeRuntimeDefaults(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x80u || cpu->data_bank != 0x80u ||
        cpu->direct_page || cpu->decimal)
        return ExecutionHandoff(cpu, 0x8080e1u);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x30u);
    InitializeRuntimeCallTemplates(memory, cpu);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FADE_CONTROL));
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
    OpStz(memory, cpu, OpDp(cpu, DP_NMI_STUB + 3u));
    OpStz(memory, cpu, OpDp(cpu, DP_AUXILIARY_CALL_STUB + 3u));
    OpStz(memory, cpu, OpDp(cpu, DP_INITIAL_REQUEST_FIRST));
    OpStz(memory, cpu, OpDp(cpu, DP_INITIAL_REQUEST_SECOND));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E057C));
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x0f30u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E0562));
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x808116u);
}

enum {
    STARTUP_RESOURCE_ID = 0x003fu,
    STARTUP_RESOURCE_BUFFER = 0x4000u,
    STARTUP_MAP_FIRST = 0x2000u,
    STARTUP_MAP_SECOND = 0x2800u,
    STARTUP_MAP_BYTES = 0x0800u,
    STARTUP_IMAGE_ROW_FIRST = 0x0280u,
    STARTUP_IMAGE_ROW_END = 0x0480u,
    STARTUP_IMAGE_ROW_BYTES = 0x0200u,
    STARTUP_PALETTE_OFFSET = 0x0008u,
    STARTUP_PALETTE_BYTES = 0x000cu,
    STARTUP_PPU_BG1SC = 0x2107u,
    STARTUP_PPU_BG12NBA = 0x210bu,
    STARTUP_PPU_BG34NBA = 0x210cu,
    DP_STARTUP_RESOURCE_OUTPUT = 0x60u,
    DP_STARTUP_REQUEST = 0x50u
};

static void ConfigureStartupVideo(const Lufia2Memory *memory,
                                  Lufia2CpuState *cpu) {
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_NMI_SCROLL_REGISTERS), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_NMI_SCROLL_REGISTERS + 2u), cpu->x);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_INIDISP));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_BGMODE));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbs(cpu, STARTUP_PPU_BG1SC));
    OpLoadA(cpu, 0x44u);
    OpSta(memory, cpu, OpAbs(cpu, STARTUP_PPU_BG12NBA));
    OpSta(memory, cpu, OpAbs(cpu, STARTUP_PPU_BG34NBA));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_TM));
    OpStz(memory, cpu, OpAbs(cpu, SNES_TS));
    OpStz(memory, cpu, OpAbs(cpu, SNES_CGWSEL));
    OpStz(memory, cpu, OpAbs(cpu, SNES_CGADSUB));
    OpLoadA(cpu, 0xe0u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_COLDATA));
}

static void ClearStartupMaps(const Lufia2Memory *memory,
                              Lufia2CpuState *cpu) {
    OpLdx(cpu, 0u);
    do {
        OpLoadA(cpu, 2u);
        OpSta(memory, cpu, OpAbsX(cpu, STARTUP_MAP_FIRST));
        TransferDirectToA(cpu);
        OpSta(memory, cpu, OpAbsX(cpu, STARTUP_MAP_SECOND));
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, STARTUP_MAP_BYTES);
    } while (!cpu->carry);
}

static void CopyStartupImageRow(const Lufia2Memory *memory,
                                 Lufia2CpuState *cpu, uint16_t destination) {
    OpLdy(cpu, STARTUP_IMAGE_ROW_FIRST);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, STARTUP_RESOURCE_BUFFER));
        OpSta(memory, cpu, OpAbsY(cpu, destination));
        OpInx(cpu);
        OpInx(cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, STARTUP_IMAGE_ROW_END);
    } while (!cpu->carry);
}

static uint8_t CopyStartupPalette(const Lufia2Memory *memory,
                                   Lufia2CpuState *cpu) {
    OpLdx(cpu, OpReadX(memory, cpu,
        OpAbs(cpu, STARTUP_RESOURCE_BUFFER + STARTUP_PALETTE_OFFSET)));
    OpLdy(cpu, 0u);
    unsigned copies = 0u;
    do {
        OpLda(memory, cpu, OpAbsX(cpu, STARTUP_RESOURCE_BUFFER));
        OpSta(memory, cpu, OpAbsY(cpu, WRAM_CGRAM_BUFFER));
        OpInx(cpu);
        OpInx(cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, OpReadX(memory, cpu,
            OpAbs(cpu, STARTUP_RESOURCE_BUFFER + STARTUP_PALETTE_BYTES)));
        if (!cpu->carry && ++copies == 4096u)
            return 0u;
    } while (!cpu->carry);
    return 1u;
}

Lufia2ExecutionResult Lufia2InitializeStartupImage(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!cpu->accumulator_is_8_bit || !cpu->index_is_8_bit ||
        cpu->program_bank != 0x80u || cpu->data_bank != 0x80u ||
        cpu->direct_page || cpu->decimal)
        return ExecutionHandoff(cpu, 0x8091dfu);
    PushAccumulator8(memory, cpu);
    PushIndex(memory, cpu);
    Push8(memory, cpu, (uint8_t)cpu->y);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    OpLdx(cpu, STARTUP_RESOURCE_ID);
    OpWriteX(memory, cpu, OpDp(cpu, DP_SCRATCH_A), cpu->x);
    OpLdx(cpu, STARTUP_RESOURCE_BUFFER);
    OpWriteX(memory, cpu, OpDp(cpu, DP_STARTUP_RESOURCE_OUTPUT), cpu->x);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, DP_STARTUP_RESOURCE_OUTPUT + 2u));
    if (!CallChildWithFrame(memory, cpu, child, context,
                           0x8091f6u, 0x808e9du, 3u, 0x80u)) {
        const Lufia2ExecutionResult result = {
            LUFIA2_EXECUTION_CHILD_UNWOUND, 0x8091f6u, 0u};
        return result;
    }
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit ||
        cpu->program_bank != 0x80u || cpu->data_bank != 0x80u ||
        cpu->direct_page || cpu->decimal)
        return ExecutionHandoff(cpu, 0x8091fau);
    ConfigureStartupVideo(memory, cpu);
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpRepWidths(cpu, 0x20u);
    ClearStartupMaps(memory, cpu);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, STARTUP_RESOURCE_BUFFER)));
    CopyStartupImageRow(memory, cpu, STARTUP_MAP_FIRST);
    OpLda(memory, cpu, OpAbs(cpu, STARTUP_RESOURCE_BUFFER));
    cpu->carry = 0u;
    OpAdcValue(cpu, STARTUP_IMAGE_ROW_BYTES);
    OpTax(cpu);
    CopyStartupImageRow(memory, cpu, STARTUP_MAP_SECOND);
    if (!CopyStartupPalette(memory, cpu))
        return ExecutionHandoff(cpu, 0x80927du);
    PullDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x80u);
    const uint8_t flags = Read8(memory, OpDp(cpu, DP_NMI_UPLOAD_FLAGS));
    cpu->zero = (flags & A8(cpu)) == 0u;
    Write8(memory, OpDp(cpu, DP_NMI_UPLOAD_FLAGS), (uint8_t)(flags | A8(cpu)));
    OpStz(memory, cpu, OpDp(cpu, DP_STARTUP_REQUEST));
    OpLdx(cpu, 0x92a4u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_NMI_STUB + 1u), cpu->x);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, DP_NMI_STUB + 3u));
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    LoadA8(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x8092a3u);
}
