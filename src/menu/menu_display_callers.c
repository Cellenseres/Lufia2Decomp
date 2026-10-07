#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "menu/menu_sprite_slots.h"
#include "lufia2/menu.h"
#include "system/wram.h"

enum {
    PARTY_COUNT = WRAM_MENU_PARTY_MEMBER_COUNT,
    PARTY_RECORDS = WRAM_MENU_PARTY_CHARACTER_OFFSET,
    WINDOW_REFRESH = WRAM_MENU_WINDOW_REFRESH,
    PORTRAIT_STATUS = 0x000fu,
    PORTRAIT_HP = 0x0011u,
    PORTRAIT_MAX_HP = 0x0025u,
    PORTRAIT_POSITION_POINTER = 0x5du,
    DISPLAY_REQUEST = 0x74u,
    DISPLAY_SECOND_REQUEST = 0x72u,
    DISPLAY_OAM_FIRST = 0x4au,
    DISPLAY_OAM_SECOND = 0x4cu,
    SELECTION_TABLE = 0x54u,
    SELECTION_SIZE = 0x56u
};

static uint8_t DisplayCallerReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x82u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit;
}

static uint8_t DisplayCall(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t site,
    uint32_t target, uint8_t frame) {
    return CallChildWithFrame(memory, cpu, child, context,
        site, target, frame, 0x82u);
}

static Lufia2ExecutionResult DisplayUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static void PortraitStorePosition(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, LongIndexedAddress(
        DirectLongPointer(memory, cpu, PORTRAIT_POSITION_POINTER), cpu->y));
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_X_LOW));
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_Y_LOW));
    OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_Y_HIGH));
    OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_X_HIGH));
}

Lufia2ExecutionResult Lufia2MenuPlacePartyPortraits(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!DisplayCallerReady(cpu))
        return ExecutionHandoff(cpu, 0x8293cfu);
    OpSepWidths(cpu, 0x10u);
    OpLdx(cpu, 0u);
    do {
        OpTxa(cpu);
        OpAslA(cpu);
        OpTay(cpu);
        if (!DisplayCall(memory, cpu, child, context,
                0x8293d6u, 0x868cdau, 3u))
            return DisplayUnwound(0x8293d6u);
        PortraitStorePosition(memory, cpu);
        OpInx(cpu);
        OpCpx(cpu, OpReadX(memory, cpu, OpAbs(cpu, PARTY_COUNT)));
    } while (!cpu->zero);
    OpRepWidths(cpu, 0x10u);
    return ExecutionReturned(0x8293f5u);
}

Lufia2ExecutionResult Lufia2MenuInitializePartyPortrait(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!DisplayCallerReady(cpu))
        return ExecutionHandoff(cpu, 0x8299beu);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ACTIVE));
    OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_FRAME_INDEX));
    OpLda(memory, cpu, OpAbsY(cpu, PORTRAIT_STATUS));
    OpBitValue(cpu, 4u);
    if (!cpu->zero) {
        OpLoadA(cpu, 4u);
    } else {
        OpBitValue(cpu, 1u);
        if (!cpu->zero) {
            OpLoadA(cpu, 1u);
        } else {
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpAbsY(cpu, PORTRAIT_MAX_HP));
            OpLsrA(cpu);
            OpLsrA(cpu);
            OpLsrA(cpu);
            OpCmp(memory, cpu, OpAbsY(cpu, PORTRAIT_HP));
            OpSepWidths(cpu, 0x20u);
            OpLoadA(cpu, cpu->carry ? 2u : 0u);
        }
    }
    if (!DisplayCall(memory, cpu, child, context,
            0x8299eeu, 0x868cf5u, 3u))
        return DisplayUnwound(0x8299eeu);
    return ExecutionReturned(0x8299f2u);
}

Lufia2ExecutionResult Lufia2MenuInitializePartyPortraits(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!DisplayCallerReady(cpu))
        return ExecutionHandoff(cpu, 0x82999bu);
    OpLdx(cpu, 0x001fu);
    if (!DisplayCall(memory, cpu, child, context,
            0x82999eu, 0x829214u, 3u))
        return DisplayUnwound(0x82999eu);
    OpLdx(cpu, 0u);
    do {
        OpRepWidths(cpu, 0x20u);
        OpTxa(cpu);
        OpAslA(cpu);
        OpTay(cpu);
        OpLda(memory, cpu, OpAbsY(cpu, PARTY_RECORDS));
        OpTay(cpu);
        OpSepWidths(cpu, 0x20u);
        if (!DisplayCall(memory, cpu, child, context,
                0x8299b0u, 0x8299beu, 2u))
            return DisplayUnwound(0x8299b0u);
        OpSepWidths(cpu, 0x10u);
        OpInx(cpu);
        OpCpx(cpu, OpReadX(memory, cpu, OpAbs(cpu, PARTY_COUNT)));
        OpRepWidths(cpu, 0x10u);
    } while (!cpu->zero);
    return ExecutionReturned(0x8299bdu);
}

Lufia2ExecutionResult Lufia2MenuBuildMainWindows(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!DisplayCallerReady(cpu))
        return ExecutionHandoff(cpu, 0x829b10u);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WINDOW_REFRESH));
    OpLdx(cpu, 0xfff0u);
    if (!DisplayCall(memory, cpu, child, context,
            0x829b18u, 0x829214u, 3u))
        return DisplayUnwound(0x829b18u);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x0350u);
    OpLdx(cpu, 0x1708u);
    if (!DisplayCall(memory, cpu, child, context,
            0x829b24u, 0x82810eu, 2u))
        return DisplayUnwound(0x829b24u);
    OpLoadA(cpu, 0x055cu);
    OpLdx(cpu, 0x1105u);
    if (!DisplayCall(memory, cpu, child, context,
            0x829b2du, 0x82810eu, 2u))
        return DisplayUnwound(0x829b2du);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 1u);
    OpLdx(cpu, 5u);
    if (!DisplayCall(memory, cpu, child, context,
            0x829b37u, 0x82895bu, 2u))
        return DisplayUnwound(0x829b37u);
    OpLdy(cpu, 0u);
    OpLdx(cpu, 0u);
    if (!DisplayCall(memory, cpu, child, context,
            0x829b40u, 0x82891eu, 2u))
        return DisplayUnwound(0x829b40u);
    if (!DisplayCall(memory, cpu, child, context,
            0x829b43u, 0x82c5afu, 2u))
        return DisplayUnwound(0x829b43u);
    OpLoadA(cpu, 0u);
    if (!DisplayCall(memory, cpu, child, context,
            0x829b48u, 0x82a318u, 2u))
        return DisplayUnwound(0x829b48u);
    OpLoadA(cpu, 0u);
    if (!DisplayCall(memory, cpu, child, context,
            0x829b4du, 0x8293f6u, 2u))
        return DisplayUnwound(0x829b4du);
    return ExecutionReturned(0x829b50u);
}

Lufia2ExecutionResult Lufia2MenuPresentMainDisplay(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!DisplayCallerReady(cpu))
        return ExecutionHandoff(cpu, 0x828704u);
    OpLdx(cpu, 0xffffu);
    OpWriteX(memory, cpu, OpDp(cpu, DISPLAY_OAM_FIRST), cpu->x);
    OpWriteX(memory, cpu, OpDp(cpu, DISPLAY_OAM_SECOND), cpu->x);
    if (!DisplayCall(memory, cpu, child, context,
            0x82870bu, 0x868bcfu, 3u))
        return DisplayUnwound(0x82870bu);
    if (!DisplayCall(memory, cpu, child, context,
            0x82870fu, 0x868bf5u, 3u))
        return DisplayUnwound(0x82870fu);
    OpLoadA(cpu, 0x88u);
    OpSta(memory, cpu, OpDp(cpu, DISPLAY_REQUEST));
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, DISPLAY_SECOND_REQUEST));
    if (!DisplayCall(memory, cpu, child, context,
            0x82871bu, 0x868b1cu, 3u))
        return DisplayUnwound(0x82871bu);
    return ExecutionReturned(0x82871fu);
}

Lufia2ExecutionResult Lufia2MenuBuildAlternateCursor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!DisplayCallerReady(cpu))
        return ExecutionHandoff(cpu, 0x829ef2u);
    OpLdx(cpu, 1u);
    if (!DisplayCall(memory, cpu, child, context,
            0x829ef5u, 0x82898du, 2u))
        return DisplayUnwound(0x829ef5u);
    OpLdx(cpu, 14u);
    OpWriteX(memory, cpu, OpDp(cpu, SELECTION_TABLE), cpu->x);
    OpLdx(cpu, 0x0406u);
    OpWriteX(memory, cpu, OpDp(cpu, SELECTION_SIZE), cpu->x);
    OpLdx(cpu, 5u);
    OpLdy(cpu, 6u);
    if (!DisplayCall(memory, cpu, child, context,
            0x829f08u, 0x8289ecu, 2u))
        return DisplayUnwound(0x829f08u);
    return ExecutionReturned(0x829f0bu);
}
