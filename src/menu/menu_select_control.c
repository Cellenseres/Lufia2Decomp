#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/menu.h"
#include "system/wram.h"

enum {
    SELECT_SPRITE_POOL = WRAM_TITLE_SPRITE_POOL & 0xffffu,
    SELECT_SPRITE_RECORD_BYTES = 13u,
    SELECT_SPRITE_STATUS = 1u,
    SELECT_DP_RECORD_COUNT = 0x04u,
    SELECT_STATE_FIRST = 0x1587u,
    SELECT_STATE_END = 0x15c0u,
    SELECT_DP_SLOT = 0x00u,
    SELECT_DP_HDMA_MASK = 0xf2u,
    SELECT_DP_HDMA_CHANNEL = 0xf3u,
    SELECT_DP_HDMA_SOURCE = 0xf4u,
    SELECT_DP_HDMA_SOURCE_BANK = 0xf6u,
    SELECT_DP_HDMA_TARGET = 0xf7u,
    SELECT_DP_HDMA_TARGET_BANK = 0xf9u,
    SELECT_DP_HDMA_REGISTER = 0xfau,
    SELECT_DP_HDMA_MODE = 0xfbu,
    SELECT_HDMA_REQUEST = WRAM_MENU_DISPLAY_REQUESTS,
    SELECT_WINDOW_REQUEST = WRAM_MENU_DISPLAY_REQUESTS + 3u,
    SELECT_MODE = 0x30u,
    SELECT_VRAM_OFFSET = WRAM_TITLE_ANIMATION_FRAME,
    SELECT_OBJECT_UPDATES = WRAM_FIELD_RECOVERY_OBJECT_UPDATES
};

static uint8_t SelectControlContext(const Lufia2CpuState *cpu, unsigned width_mode) {
    return cpu->program_bank == 0x86u && !cpu->direct_page && !cpu->decimal &&
           (width_mode == 2u || !cpu->index_is_8_bit) &&
           (width_mode || cpu->accumulator_is_8_bit) && cpu->stack >= 0x1f00u &&
           cpu->stack <= 0x1ffcu;
}

static uint8_t SelectControlCall(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                 Lufia2PushedChildCall child, void *context,
                                 uint32_t site, uint32_t target) {
    return CallChildWithFrame(memory, cpu, child, context, site, target, 2u, 0x86u);
}

static Lufia2ExecutionResult SelectControlUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2SelectScreenClearSpriteStatus(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu) {
    if (!SelectControlContext(cpu, 2u))
        return ExecutionHandoff(cpu, 0x8689fdu);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    OpLdx(cpu, SELECT_SPRITE_POOL);
    OpLoadA(cpu, WRAM_TITLE_SPRITE_POOL_COUNT);
    OpSta(memory, cpu, OpDp(cpu, SELECT_DP_RECORD_COUNT));
    do {
        OpStz(memory, cpu, OpAbsX(cpu, SELECT_SPRITE_STATUS));
        OpTxa(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, SELECT_SPRITE_RECORD_BYTES);
        OpTax(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, SELECT_DP_RECORD_COUNT), -1);
    } while (!cpu->zero);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x868a16u);
}

Lufia2ExecutionResult Lufia2SelectScreenClearState(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu) {
    if (!SelectControlContext(cpu, 0u))
        return ExecutionHandoff(cpu, 0x8687d3u);
    OpLdx(cpu, SELECT_STATE_FIRST);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, 0u));
        OpInx(cpu);
        OpCpx(cpu, SELECT_STATE_END);
    } while (!cpu->zero);
    return ExecutionReturned(0x8687dfu);
}

Lufia2ExecutionResult Lufia2SelectScreenConfigureHdmaSlot(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu,
                                                          Lufia2PushedChildCall child,
                                                          void *context) {
    if (!child || !SelectControlContext(cpu, 1u))
        return ExecutionHandoff(cpu, 0x868865u);
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0x00ffu);
    OpTay(cpu);
    OpSta(memory, cpu, OpDp(cpu, SELECT_DP_SLOT));
    OpAslA(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, SELECT_DP_SLOT));
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x8688acu));
    OpSta(memory, cpu, OpDp(cpu, SELECT_DP_HDMA_TARGET));
    OpLda(memory, cpu, OpLongX(cpu, 0x8688b5u));
    OpSta(memory, cpu, OpDp(cpu, SELECT_DP_HDMA_SOURCE));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, 0x8688aeu));
    OpSta(memory, cpu, OpDp(cpu, SELECT_DP_HDMA_TARGET_BANK));
    OpLda(memory, cpu, OpLongX(cpu, 0x8688b7u));
    OpSta(memory, cpu, OpDp(cpu, SELECT_DP_HDMA_SOURCE_BANK));
    OpTyx(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x8688a6u));
    OpSta(memory, cpu, OpDp(cpu, SELECT_DP_HDMA_CHANNEL));
    OpLda(memory, cpu, OpLongX(cpu, 0x8688a9u));
    OpSta(memory, cpu, OpDp(cpu, SELECT_DP_HDMA_REGISTER));
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpDp(cpu, SELECT_DP_HDMA_MODE));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SELECT_HDMA_REQUEST));
    if (!SelectControlCall(memory, cpu, child, context, 0x8688a2u, 0x868b48u))
        return SelectControlUnwound(0x8688a2u);
    return ExecutionReturned(0x8688a5u);
}

Lufia2ExecutionResult Lufia2SelectScreenInitializeLayout(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu,
                                                         Lufia2PushedChildCall child,
                                                         void *context) {
    static const struct {
        uint32_t site;
        uint16_t target, source;
    } layers[] = {{0x868793u, 0x2800u, 0x93c0u},
                  {0x86879cu, 0x2000u, 0xa3c0u},
                  {0x8687a5u, 0x3000u, 0xb3c0u}};
    static const uint32_t slot_sites[] = {0x8687b9u, 0x8687beu, 0x8687c3u};
    if (!child || !SelectControlContext(cpu, 0u))
        return ExecutionHandoff(cpu, 0x868784u);
    OpLdx(cpu, 0xffffu);
    OpWriteX(memory, cpu, OpDp(cpu, 0x4au), cpu->x);
    OpWriteX(memory, cpu, OpDp(cpu, 0x4cu), cpu->x);
    OpRepWidths(cpu, 0x20u);
    for (unsigned layer = 0u; layer < 3u; ++layer) {
        OpLdx(cpu, layers[layer].target);
        OpLdy(cpu, layers[layer].source);
        if (!SelectControlCall(memory, cpu, child, context, layers[layer].site,
                               0x868740u))
            return SelectControlUnwound(layers[layer].site);
    }
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0xa8u);
    OpSta(memory, cpu, OpAbs(cpu, SELECT_WINDOW_REQUEST));
    if (!SelectControlCall(memory, cpu, child, context, 0x8687afu, 0x8687d3u))
        return SelectControlUnwound(0x8687afu);
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, SELECT_VRAM_OFFSET));
    for (unsigned slot = 0u; slot < 3u; ++slot) {
        OpLoadA(cpu, (uint16_t)slot);
        if (!SelectControlCall(memory, cpu, child, context, slot_sites[slot],
                               0x868865u))
            return SelectControlUnwound(slot_sites[slot]);
    }
    OpLoadA(cpu, 0x38u);
    OpSta(memory, cpu, OpDp(cpu, SELECT_DP_HDMA_MASK));
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpAbs(cpu, SELECT_HDMA_REQUEST));
    if (!SelectControlCall(memory, cpu, child, context, 0x8687cfu, 0x868b48u))
        return SelectControlUnwound(0x8687cfu);
    return ExecutionReturned(0x8687d2u);
}

Lufia2ExecutionResult Lufia2SelectScreenControl(const Lufia2Memory *memory,
                                                Lufia2CpuState *cpu,
                                                Lufia2PushedChildCall child,
                                                void *context) {
    if (!child || !SelectControlContext(cpu, 0u))
        return ExecutionHandoff(cpu, 0x8681d0u);
    if (!SelectControlCall(memory, cpu, child, context, 0x8681d0u, 0x868784u))
        return SelectControlUnwound(0x8681d0u);
    PushDataBank(memory, cpu);
    OpLoadA(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpStz(memory, cpu, OpAbs(cpu, SELECT_OBJECT_UPDATES));
    if (!SelectControlCall(memory, cpu, child, context, 0x8681dbu, 0x8689fdu))
        return SelectControlUnwound(0x8681dbu);
    if (!SelectControlCall(memory, cpu, child, context, 0x8681deu, 0x868305u))
        return SelectControlUnwound(0x8681deu);
    OpLda(memory, cpu, OpDp(cpu, SELECT_MODE));
    const uint32_t site = cpu->zero ? 0x8681e7u : 0x8681eeu;
    const uint32_t target = cpu->zero ? 0x8681fbu : 0x868298u;
    if (!SelectControlCall(memory, cpu, child, context, site, target))
        return SelectControlUnwound(site);
    const uint8_t result_flag = cpu->carry;
    PullDataBank(memory, cpu);
    cpu->carry = result_flag;
    return ExecutionReturned(result_flag ? 0x8681fau : 0x8681f7u);
}
