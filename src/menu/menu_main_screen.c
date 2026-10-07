#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "lufia2/menu.h"
#include "system/wram.h"

enum {
    MAIN_MENU_INPUT_REQUEST = 0x71u,
    MAIN_MENU_PALETTE_REQUEST = 0x73u,
    MAIN_MENU_SELECTION = WRAM_MENU_SELECTION_REFERENCE,
    MAIN_MENU_UPDATE_FLAGS = WRAM_PLAY_TIME_FRAMES
};

static uint8_t MainMenuCall(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target, uint8_t frame) {
    return CallChildWithFrame(memory, cpu, child, context,
        site, target, frame, 0x82u);
}

static Lufia2ExecutionResult MainMenuUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static uint32_t MainMenuLoadScreen(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    static const uint32_t targets[] = {
        0x868dd7u, 0x868e6bu, 0x868eb0u, 0x868f1bu, 0x868f45u,
        0x868f6fu, 0x8690c0u, 0x8690e6u, 0x8690f9u, 0x86911fu
    };
    for (unsigned part = 0u; part < sizeof(targets) / sizeof(targets[0]); ++part) {
        const uint32_t site = 0x829a5au + part * 4u;
        if (!MainMenuCall(memory, cpu, child, context, site, targets[part], 3u))
            return site;
    }
    return 0u;
}

Lufia2ExecutionResult Lufia2MenuRunMainScreen(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x82u)
        return ExecutionHandoff(cpu, 0x829a4eu);
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    if (!MainMenuCall(memory, cpu, child, context, 0x829a52u, 0x868dbbu, 3u))
        return MainMenuUnwound(0x829a52u);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, MAIN_MENU_INPUT_REQUEST));
    const uint32_t loading_site = MainMenuLoadScreen(memory, cpu, child, context);
    if (loading_site)
        return MainMenuUnwound(loading_site);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, MAIN_MENU_PALETTE_REQUEST));
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpAbs(cpu, MAIN_MENU_SELECTION), cpu->x);
    if (!MainMenuCall(memory, cpu, child, context, 0x829a8cu, 0x828069u, 2u))
        return MainMenuUnwound(0x829a8cu);
    if (!MainMenuCall(memory, cpu, child, context, 0x829a8fu, 0x8289a4u, 2u))
        return MainMenuUnwound(0x829a8fu);
    if (!MainMenuCall(memory, cpu, child, context, 0x829a92u, 0x868e79u, 3u))
        return MainMenuUnwound(0x829a92u);
    if (!MainMenuCall(memory, cpu, child, context, 0x829a96u, 0x82a432u, 2u))
        return MainMenuUnwound(0x829a96u);
    OpLoadA(cpu, 0x80u);
    OpTestBits(memory, cpu, OpAbs(cpu, MAIN_MENU_UPDATE_FLAGS), 0u);
    if (!MainMenuCall(memory, cpu, child, context, 0x829a9eu, 0x868dbbu, 3u))
        return MainMenuUnwound(0x829a9eu);
    return ExecutionReturned(0x829aa2u);
}
