#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "lufia2/special_menu.h"
#include "system/wram.h"

enum {
    SPECIAL_MENU_SCROLL_OFFSETS = WRAM_NMI_SCROLL_REGISTERS,
    SPECIAL_MENU_PALETTE_SOURCE = 0xec20u,
    SPECIAL_MENU_PALETTE_BYTES = 0x100u,
    SPECIAL_MENU_PALETTE_FLAGS = 0x73u,
    SPECIAL_MENU_GRAPHICS_RESOURCE = 0x54u,
    SPECIAL_MENU_DMA_LENGTH = 0u,
    SPECIAL_MENU_DMA_DESTINATION = 2u,
    SPECIAL_MENU_RESOURCE_DESTINATION = 0x60u,
    SPECIAL_MENU_RESOURCE_BANK = 0x62u
};

static bool SpecialVideoReady(const Lufia2CpuState *cpu, uint8_t byte_accumulator) {
    return cpu->program_bank == 0x84u &&
           cpu->accumulator_is_8_bit == byte_accumulator &&
           !cpu->index_is_8_bit && !cpu->direct_page && !cpu->decimal;
}

static bool SpecialVideoCall(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                             Lufia2PushedChildCall child, void *context,
                             uint32_t site, uint32_t target, uint8_t frame_size,
                             uint8_t byte_accumulator,
                             Lufia2ExecutionResult *result) {
    if (!CallChildWithFrame(memory, cpu, child, context, site, target,
                            frame_size, 0x84u)) {
        *result = ExecutionReturned(site);
        result->flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (!SpecialVideoReady(cpu, byte_accumulator)) {
        *result = ExecutionHandoff(cpu, site + frame_size + 1u);
        return false;
    }
    return true;
}

Lufia2ExecutionResult Lufia2SpecialMenuInitializeVideo(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SpecialVideoReady(cpu, 1u))
        return ExecutionHandoff(cpu, 0x84a455u);
    Lufia2ExecutionResult result = ExecutionReturned(0x84a4dcu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    static const struct {
        uint16_t address;
        uint8_t value;
    } video_registers[] = {
        {SNES_OBSEL, 1u}, {SNES_BG12NBA, 0x22u}, {SNES_BG34NBA, 0x11u},
        {SNES_BG1SC, 0u}, {SNES_BG2SC, 4u}, {SNES_BG3SC, 8u},
        {SNES_BG4SC, 12u}, {SNES_BGMODE, 9u}
    };
    for (unsigned reg = 0u; reg < sizeof video_registers / sizeof video_registers[0]; ++reg) {
        OpLoadA(cpu, video_registers[reg].value);
        OpSta(memory, cpu, OpAbs(cpu, video_registers[reg].address));
    }
    OpRepWidths(cpu, 0x20u);
    OpLdy(cpu, 0u);
    OpLoadA(cpu, 0u);
    do {
        OpSta(memory, cpu, OpAbsY(cpu, SPECIAL_MENU_SCROLL_OFFSETS));
        OpIny(cpu);
        OpIny(cpu);
        Compare16(cpu, cpu->y, 16u);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    for (uint16_t reg = SNES_W12SEL; reg <= SNES_WH3; ++reg)
        OpStz(memory, cpu, OpAbs(cpu, reg));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_DMAP(0)));
    OpLdx(cpu, 0x8500u);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_A1TL(0)), cpu->x);
    OpLoadA(cpu, 0x9fu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_A1B(0)));
    OpLdx(cpu, 0x1000u);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_DASL(0)), cpu->x);
    OpLoadA(cpu, 0x18u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_BBAD(0)));
    OpLdx(cpu, 0x1000u);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_VMADDL), cpu->x);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_MDMAEN));
    if (!SpecialVideoCall(memory, cpu, child, context, 0x84a4d3u, 0x84a4ddu,
                          2u, 1u, &result) ||
        !SpecialVideoCall(memory, cpu, child, context, 0x84a4d6u, 0x84a4fdu,
                          2u, 1u, &result))
        return result;
    UnpackStatus(cpu, Pull8(memory, cpu));
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    return result;
}

Lufia2ExecutionResult Lufia2SpecialMenuInitializePalettes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SpecialVideoReady(cpu, 1u))
        return ExecutionHandoff(cpu, 0x84a529u);
    Lufia2ExecutionResult result = ExecutionReturned(0x84a568u);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    PushDataBank(memory, cpu);
    for (unsigned palette = 0u; palette < 2u; ++palette) {
        OpLdx(cpu, SPECIAL_MENU_PALETTE_SOURCE);
        OpLdy(cpu, (uint16_t)(0x0320u + palette * SPECIAL_MENU_PALETTE_BYTES));
        OpLoadA(cpu, SPECIAL_MENU_PALETTE_BYTES - 1u);
        OpMoveNext(memory, cpu, 0u, 0xa6u);
    }
    PullDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, SPECIAL_MENU_PALETTE_FLAGS));
    if (!SpecialVideoCall(memory, cpu, child, context, 0x84a54eu, 0x84a5a8u,
                          2u, 1u, &result))
        return result;
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x0295u);
    OpSta(memory, cpu, OpDp(cpu, SPECIAL_MENU_GRAPHICS_RESOURCE));
    OpLoadA(cpu, 0x2000u);
    OpSta(memory, cpu, OpDp(cpu, SPECIAL_MENU_DMA_LENGTH));
    OpLoadA(cpu, 0x3000u);
    OpSta(memory, cpu, OpDp(cpu, SPECIAL_MENU_DMA_DESTINATION));
    if (!SpecialVideoCall(memory, cpu, child, context, 0x84a562u, 0x84a569u,
                          2u, 0u, &result))
        return result;
    UnpackStatus(cpu, Pull8(memory, cpu));
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    return result;
}

Lufia2ExecutionResult Lufia2SpecialMenuUploadGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SpecialVideoReady(cpu, 0u))
        return ExecutionHandoff(cpu, 0x84a569u);
    Lufia2ExecutionResult result = ExecutionReturned(0x84a5a7u);
    PushY(memory, cpu);
    PushIndex(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    OpLdx(cpu, 0x4000u);
    OpWriteX(memory, cpu, OpDp(cpu, SPECIAL_MENU_RESOURCE_DESTINATION), cpu->x);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, SPECIAL_MENU_RESOURCE_BANK));
    if (!SpecialVideoCall(memory, cpu, child, context, 0x84a579u, 0x808e9du,
                          3u, 1u, &result))
        return result;
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_DMAP(0)));
    OpLdx(cpu, 0x4000u);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_A1TL(0)), cpu->x);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_A1B(0)));
    OpLdx(cpu, OpRead16(memory, OpDp(cpu, SPECIAL_MENU_DMA_LENGTH)));
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_DASL(0)), cpu->x);
    OpLoadA(cpu, 0x18u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_BBAD(0)));
    OpLdx(cpu, OpRead16(memory, OpDp(cpu, SPECIAL_MENU_DMA_DESTINATION)));
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_VMADDL), cpu->x);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_MDMAEN));
    if (!SpecialVideoCall(memory, cpu, child, context, 0x84a5a1u, 0x84a5a8u,
                          2u, 1u, &result))
        return result;
    UnpackStatus(cpu, Pull8(memory, cpu));
    OpPullX(memory, cpu);
    OpPullY(memory, cpu);
    return result;
}
