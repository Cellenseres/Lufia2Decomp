#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "lufia2/menu.h"
#include "system/wram.h"

enum {
    MENU_UPLOAD_RESOURCE = 0x54u,
    MENU_UPLOAD_BYTES = 0x58u,
    MENU_UPLOAD_SOURCE = 0x5du,
    MENU_UPLOAD_SOURCE_BANK = 0x5fu,
    MENU_UPLOAD_TARGET = 0x60u,
    MENU_UPLOAD_TARGET_BANK = 0x62u,
    MENU_STAGE_BUFFER = 0x4000u,
    MENU_STAGE_BANK = 0x7eu,
    MENU_FRAME_SERVICE = 0x68u,
    MENU_FRAME_SERVICE_BANK = 0x6au,
    MENU_INPUT_REQUEST = 0x71u,
    MENU_LAYER_REQUEST = 0x72u,
    MENU_PALETTE_REQUEST = 0x73u,
    MENU_TILEMAP_REQUEST = 0x74u,
    MENU_FRAME_REQUEST = 0xf2u,
    MENU_UPDATE_FLAGS = 0x0b50u,
    MENU_REQUEST_FIRST = WRAM_MENU_DISPLAY_REQUESTS,
    MENU_LIST_TOP_ROW = WRAM_MENU_LIST_TOP_ROW,
    MENU_LIST_OVERFLOW = WRAM_MENU_LIST_OVERFLOW,
    MENU_SCROLL_ROW = WRAM_MENU_SCROLL_ROW,
    MENU_SELECTION_KIND = WRAM_UNK_7E09C9,
    MENU_WINDOW_REFRESH = WRAM_MENU_WINDOW_REFRESH,
    MENU_UNK_156B = WRAM_UNK_7E156B,
    MENU_CURSOR_ENABLED = WRAM_MENU_CURSOR_ENABLED,
    MENU_EQUIPMENT_CURSOR = WRAM_MENU_EQUIPMENT_CURSOR,
    MENU_TEXT_FIRST = WRAM_UNK_7E0562,
    MENU_TEXT_SECOND = WRAM_UNK_7E0563
};

static uint8_t MenuGraphicsCall(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target) {
    return CallChildWithFrame(memory, cpu, child, context,
        site, target, 3u, 0x86u);
}

static Lufia2ExecutionResult MenuGraphicsUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static void MenuGraphicsSetSource(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t address, uint8_t bank) {
    OpLdx(cpu, address);
    OpWriteX(memory, cpu, OpDp(cpu, MENU_UPLOAD_SOURCE), cpu->x);
    OpLoadA(cpu, bank);
    OpSta(memory, cpu, OpDp(cpu, MENU_UPLOAD_SOURCE_BANK));
}

static void MenuGraphicsSetTransfer(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t target, uint16_t size) {
    OpLdx(cpu, target);
    OpWriteX(memory, cpu, OpDp(cpu, MENU_UPLOAD_TARGET), cpu->x);
    OpLdx(cpu, size);
    OpWriteX(memory, cpu, OpDp(cpu, MENU_UPLOAD_BYTES), cpu->x);
}

static void MenuGraphicsSetResource(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t resource) {
    OpLdx(cpu, resource);
    OpWriteX(memory, cpu, OpDp(cpu, MENU_UPLOAD_RESOURCE), cpu->x);
    OpLdx(cpu, MENU_STAGE_BUFFER);
    OpWriteX(memory, cpu, OpDp(cpu, MENU_UPLOAD_TARGET), cpu->x);
    OpLoadA(cpu, MENU_STAGE_BANK);
    OpSta(memory, cpu, OpDp(cpu, MENU_UPLOAD_TARGET_BANK));
}

Lufia2ExecutionResult Lufia2MenuResetDisplayRequests(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x86u || !cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x868dbbu);
    OpStz(memory, cpu, OpAbs(cpu, SNES_HDMAEN));
    OpStz(memory, cpu, OpDp(cpu, MENU_INPUT_REQUEST));
    OpStz(memory, cpu, OpDp(cpu, MENU_FRAME_SERVICE_BANK));
    OpStz(memory, cpu, OpDp(cpu, MENU_FRAME_REQUEST));
    OpStz(memory, cpu, OpDp(cpu, MENU_LAYER_REQUEST));
    OpStz(memory, cpu, OpDp(cpu, MENU_PALETTE_REQUEST));
    OpStz(memory, cpu, OpDp(cpu, MENU_TILEMAP_REQUEST));
    for (unsigned request = 0; request < 4u; ++request)
        OpStz(memory, cpu, OpAbs(cpu, (uint16_t)(MENU_REQUEST_FIRST + request)));
    return ExecutionReturned(0x868dd6u);
}

Lufia2ExecutionResult Lufia2MenuInitializeDisplayState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x86u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x868e79u);
    OpLdx(cpu, 0x939cu);
    OpWriteX(memory, cpu, OpDp(cpu, MENU_FRAME_SERVICE), cpu->x);
    OpLoadA(cpu, 0x82u);
    OpSta(memory, cpu, OpDp(cpu, MENU_FRAME_SERVICE_BANK));
    OpLoadA(cpu, 0x80u);
    OpTestBits(memory, cpu, OpAbs(cpu, MENU_UPDATE_FLAGS), 1u);
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpAbs(cpu, MENU_LIST_TOP_ROW), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, MENU_LIST_OVERFLOW), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, MENU_SCROLL_ROW), cpu->x);
    OpStz(memory, cpu, OpAbs(cpu, MENU_SELECTION_KIND));
    OpStz(memory, cpu, OpAbs(cpu, MENU_WINDOW_REFRESH));
    OpStz(memory, cpu, OpAbs(cpu, MENU_UNK_156B));
    OpStz(memory, cpu, OpAbs(cpu, MENU_CURSOR_ENABLED));
    OpLdx(cpu, 999u);
    OpWriteX(memory, cpu, OpAbs(cpu, MENU_EQUIPMENT_CURSOR), cpu->x);
    OpLoadA(cpu, 0x30u);
    OpSta(memory, cpu, OpAbs(cpu, MENU_TEXT_FIRST));
    OpLoadA(cpu, 0x0fu);
    OpSta(memory, cpu, OpAbs(cpu, MENU_TEXT_SECOND));
    return ExecutionReturned(0x868eafu);
}

Lufia2ExecutionResult Lufia2MenuLoadPrimaryGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x86u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x868eb0u);
    MenuGraphicsSetSource(memory, cpu, 0x8500u, 0x9fu);
    MenuGraphicsSetTransfer(memory, cpu, 0x6000u, 0x1000u);
    if (!MenuGraphicsCall(memory, cpu, child, context, 0x868ec3u, 0x828044u))
        return MenuGraphicsUnwound(0x868ec3u);
    MenuGraphicsSetResource(memory, cpu, 0x0289u);
    if (!MenuGraphicsCall(memory, cpu, child, context, 0x868ed5u, 0x808e9du))
        return MenuGraphicsUnwound(0x868ed5u);
    MenuGraphicsSetSource(memory, cpu, MENU_STAGE_BUFFER, MENU_STAGE_BANK);
    MenuGraphicsSetTransfer(memory, cpu, 0x6800u, 0x1000u);
    if (!MenuGraphicsCall(memory, cpu, child, context, 0x868eecu, 0x828044u))
        return MenuGraphicsUnwound(0x868eecu);
    return ExecutionReturned(0x868ef0u);
}

static Lufia2ExecutionResult MenuGraphicsLoadStaged(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t entry,
    uint32_t decode_site, uint32_t upload_site, uint16_t resource,
    uint16_t target, uint16_t size) {
    if (cpu->program_bank != 0x86u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, entry);
    MenuGraphicsSetResource(memory, cpu, resource);
    if (!MenuGraphicsCall(memory, cpu, child, context, decode_site, 0x808e9du))
        return MenuGraphicsUnwound(decode_site);
    MenuGraphicsSetSource(memory, cpu, MENU_STAGE_BUFFER, MENU_STAGE_BANK);
    MenuGraphicsSetTransfer(memory, cpu, target, size);
    if (!MenuGraphicsCall(memory, cpu, child, context, upload_site, 0x828044u))
        return MenuGraphicsUnwound(upload_site);
    return ExecutionReturned(upload_site + 4u);
}

Lufia2ExecutionResult Lufia2MenuLoadAuxiliaryGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return MenuGraphicsLoadStaged(memory, cpu, child, context, 0x868ef1u,
        0x868effu, 0x868f16u, 0x028au, 0x6a00u, 0x0100u);
}

Lufia2ExecutionResult Lufia2MenuLoadLargeGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return MenuGraphicsLoadStaged(memory, cpu, child, context, 0x868f1bu,
        0x868f29u, 0x868f40u, 0x0282u, 0x4000u, 0x3200u);
}

Lufia2ExecutionResult Lufia2MenuLoadMediumGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return MenuGraphicsLoadStaged(memory, cpu, child, context, 0x868f45u,
        0x868f53u, 0x868f6au, 0x0281u, 0x2000u, 0x2000u);
}
