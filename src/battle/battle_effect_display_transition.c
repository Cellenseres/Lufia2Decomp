#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum {
    DP_DISPLAY_STREAM = 0xc3u,
    DP_DISPLAY_RESOURCE = 0x54u,
    DP_DISPLAY_DESTINATION = 0x60u,
    DP_DISPLAY_DESTINATION_BANK = 0x62u,
    DISPLAY_SAVED_MODE_FLAG = 0x7ff4d8u,
    DISPLAY_GRAPHICS_RESOURCES = 0x85ef53u,
    DISPLAY_GRAPHICS_BANK = 0x7eu
};

typedef struct DisplayTransition {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    uint32_t interrupted;
} DisplayTransition;

static uint8_t DisplayCall(DisplayTransition *display, uint32_t site,
    uint32_t target) {
    if (CallChildWithFrame(display->memory, display->cpu, display->child,
            display->context, site, target, 3u, 0x81u))
        return 1u;
    display->interrupted = site;
    return 0u;
}

static uint8_t PrepareDisplay(DisplayTransition *display) {
    const Lufia2Memory *memory = display->memory;
    Lufia2CpuState *cpu = display->cpu;
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_DISPLAY_STREAM));
    PushY(memory, cpu);
    OpCmpValue(cpu, 4u);
    if (cpu->carry) {
        OpAndValue(cpu, 1u);
        OpSta(memory, cpu, OpDp(cpu, DP_DISPLAY_RESOURCE));
        OpLda(memory, cpu, DISPLAY_SAVED_MODE_FLAG);
        OpAslA(cpu);
        TransferDirectToA(cpu);
        RolA8(cpu);
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 1u));
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ OpReadM(memory, cpu,
            OpDp(cpu, DP_DISPLAY_RESOURCE))));
    }
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_SPRITE_BUILD_MODE));
    if (!DisplayCall(display, 0x8195a8u, 0x858a39u))
        return 0u;
    OpRepWidths(cpu, 0x20u);
    if (!DisplayCall(display, 0x8195aeu, 0x859bf1u))
        return 0u;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, WRAM_BATTLE_EFFECT_FRAME_MARKER);
    if (!DisplayCall(display, 0x8195bau, 0x81b9afu) ||
        !DisplayCall(display, 0x8195beu, 0x85ec81u))
        return 0u;
    cpu->y = PullIndexValue(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpStepMem(memory, cpu, OpDp(cpu, DP_DISPLAY_STREAM), 1);
    return 1u;
}

static uint8_t LoadDisplayResource(DisplayTransition *display, unsigned layer) {
    const Lufia2Memory *memory = display->memory;
    Lufia2CpuState *cpu = display->cpu;
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_DISPLAY_STREAM));
    OpStepMem(memory, cpu, OpDp(cpu, DP_DISPLAY_STREAM), 1);
    OpStepMem(memory, cpu, OpDp(cpu, DP_DISPLAY_STREAM), 1);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, DISPLAY_GRAPHICS_RESOURCES));
    OpSta(memory, cpu, OpDp(cpu, DP_DISPLAY_RESOURCE));
    OpBitValue(cpu, 0xffffu);
    if (cpu->zero)
        return 1u;
    OpLdx(cpu, layer ? 0xdf00u : 0xa000u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_DISPLAY_DESTINATION), cpu->x);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, DISPLAY_GRAPHICS_BANK);
    OpSta(memory, cpu, OpDp(cpu, DP_DISPLAY_DESTINATION_BANK));
    PushY(memory, cpu);
    if (!DisplayCall(display, layer ? 0x819624u : 0x8195e6u, 0x808e9du))
        return 0u;
    OpRepWidths(cpu, 0x20u);
    if (!DisplayCall(display, layer ? 0x81962au : 0x8195ecu,
            layer ? 0x859c4du : 0x859c1fu))
        return 0u;
    OpSepWidths(cpu, 0x20u);
    if (!DisplayCall(display, layer ? 0x819630u : 0x8195f2u, 0x85ec81u))
        return 0u;
    if (!layer) {
        OpRepWidths(cpu, 0x20u);
        if (!DisplayCall(display, 0x8195f8u, 0x859c36u))
            return 0u;
        OpSepWidths(cpu, 0x20u);
        if (!DisplayCall(display, 0x8195feu, 0x85ec81u))
            return 0u;
        OpRepWidths(cpu, 0x20u);
    }
    cpu->y = PullIndexValue(memory, cpu);
    if (layer)
        OpRepWidths(cpu, 0x20u);
    return 1u;
}

Lufia2ExecutionResult Lufia2BattleEffectChangeDisplay(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x81u || cpu->data_bank != 0x81u ||
        cpu->direct_page || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x81958fu);
    DisplayTransition display = {memory, cpu, child, context, 0u};
    if (!PrepareDisplay(&display) || !LoadDisplayResource(&display, 0u) ||
        !LoadDisplayResource(&display, 1u)) {
        Lufia2ExecutionResult result = ExecutionReturned(display.interrupted);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x819639u);
}
