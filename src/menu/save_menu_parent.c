#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/menu.h"

enum {
    SAVE_INITIAL_CURSOR_ROW = WRAM_MENU_ITEM_ROW + 7u,
    DP_SAVE_TEXT_BANK = 0x5fu,
    ROM_SAVE_WINDOW_STYLES_LONG = 0x82efafu,
    ROM_SAVE_DISPLAY_TEXT_LONG = 0x8ed3e9u
};

static Lufia2ExecutionResult SaveParentChildUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2SaveOpenMenu(
    const Lufia2Memory *memory,Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,void *context) {
    if (!child || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu,0x82e941u);
    OpLoadA(cpu,0x2bu);
    if (!CallChildWithFrame(memory,cpu,child,context,
            0x82e943u,0x8093feu,3u,cpu->program_bank))
        return SaveParentChildUnwound(0x82e943u);
    if (!CallChildWithFrame(memory,cpu,child,context,
            0x82e947u,0x82f20au,2u,cpu->program_bank))
        return SaveParentChildUnwound(0x82e947u);
    if (!CallChildWithFrame(memory,cpu,child,context,
            0x82e94au,0x82838fu,2u,cpu->program_bank))
        return SaveParentChildUnwound(0x82e94au);
    if (!CallChildWithFrame(memory,cpu,child,context,
            0x82e94du,0x82f481u,2u,cpu->program_bank))
        return SaveParentChildUnwound(0x82e94du);
    if (!CallChildWithFrame(memory,cpu,child,context,
            0x82e950u,0x82eef7u,2u,cpu->program_bank))
        return SaveParentChildUnwound(0x82e950u);
    OpLdx(cpu,OpReadX(memory,cpu,OpAbs(cpu,WRAM_UNK_7E09C2)));
    if (!CallChildWithFrame(memory,cpu,child,context,
            0x82e956u,0x82ef25u,2u,cpu->program_bank))
        return SaveParentChildUnwound(0x82e956u);
    if (!CallChildWithFrame(memory,cpu,child,context,
            0x82e959u,0x82e9d5u,2u,cpu->program_bank))
        return SaveParentChildUnwound(0x82e959u);
    return ExecutionReturned(0x82e95cu);
}

Lufia2ExecutionResult Lufia2SaveRunAlternateMenu(
    const Lufia2Memory *memory,Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,void *context) {
    if (!child || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu,0x82e95du);
    for (;;) {
        OpLoadA(cpu,1u);
        if (!CallChildWithFrame(memory,cpu,child,context,
                0x82e95fu,0x82e917u,2u,cpu->program_bank))
            return SaveParentChildUnwound(0x82e95fu);
        if (!CallChildWithFrame(memory,cpu,child,context,
                0x82e962u,0x82f139u,2u,cpu->program_bank))
            return SaveParentChildUnwound(0x82e962u);
        if (!CallChildWithFrame(memory,cpu,child,context,
                0x82e965u,0x828704u,2u,cpu->program_bank))
            return SaveParentChildUnwound(0x82e965u);
        for (;;) {
            if (!CallChildWithFrame(memory,cpu,child,context,
                    0x82e968u,0x82ecc7u,2u,cpu->program_bank))
                return SaveParentChildUnwound(0x82e968u);
            if (cpu->carry)
                return ExecutionReturned(0x82e997u);
            OpCmpValue(cpu,0xffu);
            if (cpu->zero)
                break;
            if (!CallChildWithFrame(memory,cpu,child,context,
                    0x82e971u,0x82f190u,2u,cpu->program_bank))
                return SaveParentChildUnwound(0x82e971u);
            if (!CallChildWithFrame(memory,cpu,child,context,
                    0x82e974u,0x82ecf3u,2u,cpu->program_bank))
                return SaveParentChildUnwound(0x82e974u);
            if (!CallChildWithFrame(memory,cpu,child,context,
                    0x82e977u,0x82f139u,2u,cpu->program_bank))
                return SaveParentChildUnwound(0x82e977u);
        }
        OpLoadA(cpu,5u);
        OpLdx(cpu,5u);
        if (!CallChildWithFrame(memory,cpu,child,context,
                0x82e981u,0x82895bu,2u,cpu->program_bank))
            return SaveParentChildUnwound(0x82e981u);
        if (!CallChildWithFrame(memory,cpu,child,context,
                0x82e984u,0x868b55u,3u,cpu->program_bank))
            return SaveParentChildUnwound(0x82e984u);
        if (!CallChildWithFrame(memory,cpu,child,context,
                0x82e988u,0x868b32u,3u,cpu->program_bank))
            return SaveParentChildUnwound(0x82e988u);
        if (!CallChildWithFrame(memory,cpu,child,context,
                0x82e98cu,0x82838fu,2u,cpu->program_bank))
            return SaveParentChildUnwound(0x82e98cu);
        if (!CallChildWithFrame(memory,cpu,child,context,
                0x82e98fu,0x82f660u,2u,cpu->program_bank))
            return SaveParentChildUnwound(0x82e98fu);
        if (!CallChildWithFrame(memory,cpu,child,context,
                0x82e992u,0x82eaf4u,2u,cpu->program_bank))
            return SaveParentChildUnwound(0x82e992u);
    }
}

Lufia2ExecutionResult Lufia2SavePrepareSlotDisplay(
    const Lufia2Memory *memory,Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,void *context) {
    if (!child || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu,0x82ef25u);
    OpPushX(memory,cpu);
    OpLdx(cpu,0xffffu);
    if (!CallChildWithFrame(memory,cpu,child,context,
            0x82ef29u,0x829214u,3u,cpu->program_bank))
        return SaveParentChildUnwound(0x82ef29u);
    OpLdx(cpu,0xffffu);
    if (!CallChildWithFrame(memory,cpu,child,context,
            0x82ef30u,0x82922du,3u,cpu->program_bank))
        return SaveParentChildUnwound(0x82ef30u);
    OpLda(memory,cpu,OpAbs(cpu,WRAM_UNK_7E1558));
    if (cpu->zero) {
        OpSepWidths(cpu,0x20u);
        OpLda(memory,cpu,OpAbs(cpu,WRAM_UNK_7E1559));
        OpIncA(cpu);
        OpSta(memory,cpu,OpAbs(cpu,SAVE_INITIAL_CURSOR_ROW));
        OpRepWidths(cpu,0x20u);
        OpLda(memory,cpu,OpAbs(cpu,WRAM_UNK_7E1559));
        OpAndValue(cpu,0xffu);
        OpAslA(cpu);
        OpTax(cpu);
        OpLda(memory,cpu,OpLongX(cpu,ROM_SAVE_WINDOW_STYLES_LONG));
        OpTax(cpu);
    } else {
        OpLdx(cpu,0x0803u);
    }
    OpRepWidths(cpu,0x20u);
    OpLoadA(cpu,0x0082u);
    if (!CallChildWithFrame(memory,cpu,child,context,
            0x82ef5bu,0x82810eu,2u,cpu->program_bank))
        return SaveParentChildUnwound(0x82ef5bu);
    static const uint16_t windows[] = {0x0142u,0x0160u,0x0402u,0x0420u};
    for (unsigned window=0u;window<4u;++window) {
        const uint32_t site=0x82ef64u+window*9u;
        OpLoadA(cpu,windows[window]);
        OpLdx(cpu,0x0f0bu);
        if (!CallChildWithFrame(memory,cpu,child,context,
                site,0x82810eu,2u,cpu->program_bank))
            return SaveParentChildUnwound(site);
    }
    OpSepWidths(cpu,0x20u);
    OpLoadA(cpu,0x20u);
    OpSta(memory,cpu,OpAbs(cpu,WRAM_MENU_DRAW_MODE));
    OpLoadA(cpu,ROM_SAVE_DISPLAY_TEXT_LONG>>16);
    OpSta(memory,cpu,OpDp(cpu,DP_SAVE_TEXT_BANK));
    OpLdy(cpu,ROM_SAVE_DISPLAY_TEXT_LONG&0xffffu);
    if (!CallChildWithFrame(memory,cpu,child,context,
            0x82ef90u,0x808878u,3u,cpu->program_bank))
        return SaveParentChildUnwound(0x82ef90u);
    if (!CallChildWithFrame(memory,cpu,child,context,
            0x82ef94u,0x82f2ceu,2u,cpu->program_bank))
        return SaveParentChildUnwound(0x82ef94u);
    OpLoadA(cpu,1u);
    OpPullX(memory,cpu);
    if (!CallChildWithFrame(memory,cpu,child,context,
            0x82ef9au,0x82895bu,2u,cpu->program_bank))
        return SaveParentChildUnwound(0x82ef9au);
    OpLdx(cpu,2u);
    if (!CallChildWithFrame(memory,cpu,child,context,
            0x82efa0u,0x8288a0u,2u,cpu->program_bank))
        return SaveParentChildUnwound(0x82efa0u);
    if (!CallChildWithFrame(memory,cpu,child,context,
            0x82efa3u,0x82f3fcu,2u,cpu->program_bank))
        return SaveParentChildUnwound(0x82efa3u);
    OpLoadA(cpu,1u);
    if (!CallChildWithFrame(memory,cpu,child,context,
            0x82efa8u,0x8293f6u,2u,cpu->program_bank))
        return SaveParentChildUnwound(0x82efa8u);
    if (!CallChildWithFrame(memory,cpu,child,context,
            0x82efabu,0x828704u,2u,cpu->program_bank))
        return SaveParentChildUnwound(0x82efabu);
    return ExecutionReturned(0x82efaeu);
}
