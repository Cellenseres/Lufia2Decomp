/* Status and equipment text dispatch. */

#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/menu.h"
#include "system/wram.h"

enum {
    STATUS_PARTY_SLOT = 0x22,
    STATUS_CHARACTER = 0x2a,
    STATUS_VALUE = 0x4e,
    STATUS_DIGITS = 0x51,
    STATUS_TABLE = 0x5d,
    STATUS_TEXT_BANK = 0x5f,
    STATUS_TARGET = 0x60,
    STATUS_TILE_CURSOR = 0xe7,
    STATUS_LOOP_LIMIT = 4096
};

static uint16_t StatusDispatchTarget(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0xa321u);
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0x00ffu);
    OpAslA(cpu);
    OpIncA(cpu);
    OpTay(cpu);
    PullAccumulator16(memory, cpu);
    OpSta(memory, cpu, OpDp(cpu, STATUS_TABLE));
    OpSepWidths(cpu, 0x20u);
    Push8(memory, cpu, cpu->program_bank);
    LoadA8(cpu, Pull8(memory, cpu));
    OpSta(memory, cpu, OpDp(cpu, STATUS_TEXT_BANK));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, STATUS_TABLE));
    OpSta(memory, cpu, OpDp(cpu, STATUS_TARGET));
    OpSepWidths(cpu, 0x20u);
    return Read16Bank(memory, 0u, STATUS_TARGET);
}

static uint8_t StatusChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target, uint8_t frame) {
    return CallChildWithFrame(
        memory, cpu, child, context, site, target, frame, 0x82u);
}

/* $82:A318: modes 0/1 party, 2 equipment, 3 equipment refresh, 4 details. */
Lufia2ExecutionResult Lufia2MenuDrawStatus(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, Lufia2ExecutionCheckpoint checkpoint,
    void *context) {
    uint16_t target;
    unsigned visits = 0;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit || !child)
        return ExecutionHandoff(cpu, 0x82a318u);
    PushAccumulator8(memory, cpu);
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    LoadA8(cpu, Pull8(memory, cpu));
    target = StatusDispatchTarget(memory, cpu);

#define STATUS_CALL(site, callee, frame)                                      \
    do {                                                                     \
        if (!StatusChild(memory, cpu, child, context, site, callee, frame)) {   \
            Lufia2ExecutionResult result = {                                 \
                LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};                    \
            return result;                                                   \
        }                                                                    \
    } while (0)

    switch (target) {
    case 0xa32cu:
        OpLoadA(cpu, 0x8eu);
        OpSta(memory, cpu, OpDp(cpu, STATUS_TEXT_BANK));
        OpLdy(cpu, 0xd1e9u);
        STATUS_CALL(0x82a333u, 0x808878u, 3u);
        OpLoadA(cpu, 0x8eu);
        OpSta(memory, cpu, OpDp(cpu, STATUS_TEXT_BANK));
        OpLdy(cpu, 0xd4deu);
        OpLdx(cpu, 0x35e0u);
        STATUS_CALL(0x82a341u, 0x808878u, 3u);
        /* Mode zero falls through to the party scan. */
        /* fall through */
    case 0xa345u:
        OpLdx(cpu, 0u);
        OpWriteX(memory, cpu, OpDp(cpu, STATUS_PARTY_SLOT), cpu->x);
        do {
            if (visits++ == STATUS_LOOP_LIMIT)
                return ExecutionHandoff(cpu, 0x82a34au);
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpDp(cpu, STATUS_PARTY_SLOT));
            OpAslA(cpu);
            OpTax(cpu);
            OpLda(memory, cpu, OpAbsX(cpu, WRAM_MENU_PARTY_CHARACTER_OFFSET));
            OpSta(memory, cpu, OpDp(cpu, STATUS_CHARACTER));
            OpLda(memory, cpu, OpLongX(cpu, 0x82a36bu));
            OpTax(cpu);
            OpSepWidths(cpu, 0x20u);
            STATUS_CALL(0x82a35cu, 0x8294c0u, 2u);
            OpStepMem(memory, cpu, OpDp(cpu, STATUS_PARTY_SLOT), 1);
            OpLda(memory, cpu, OpDp(cpu, STATUS_PARTY_SLOT));
            OpCmp(memory, cpu, OpAbs(cpu, WRAM_MENU_PARTY_MEMBER_COUNT));
        } while (!cpu->zero);
        break;
    case 0xa373u:
        STATUS_CALL(0x82a373u, 0x82f9c9u, 2u);
        OpLdx(cpu, 0x308au);
        STATUS_CALL(0x82a379u, 0x8294c0u, 2u);
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, STATUS_CHARACTER)));
        OpLda(memory, cpu, OpAbsX(cpu, 0x00bcu));
        OpSta(memory, cpu, OpAbs(cpu, 0x1570u));
        OpStz(memory, cpu, OpAbs(cpu, 0x1571u));
        OpLdy(cpu, 0x64u);
        OpWriteX(memory, cpu, OpAbs(cpu, 0x1572u), cpu->y);
        STATUS_CALL(0x82a38du, 0x828000u, 3u);
        OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, 0x1574u)));
        OpWriteX(memory, cpu, OpDp(cpu, STATUS_VALUE), cpu->y);
        OpLdy(cpu, 0xffu);
        OpWriteX(memory, cpu, OpDp(cpu, STATUS_DIGITS), cpu->y);
        STATUS_CALL(0x82a39bu, 0x808378u, 3u);
        OpLda(memory, cpu, OpDp(cpu, STATUS_VALUE));
        OpSta(memory, cpu, OpDp(cpu, 0u));
        STATUS_CALL(0x82a3a3u, 0x82942fu, 2u);
        OpLoadA(cpu, 0x20u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
        OpLdx(cpu, 0x1560u);
        OpWriteX(memory, cpu, OpDp(cpu, STATUS_TILE_CURSOR), cpu->x);
        OpLoadA(cpu, 0x8eu);
        OpSta(memory, cpu, OpDp(cpu, STATUS_TEXT_BANK));
        if (checkpoint)
            checkpoint(context, cpu, 0x82a3b4u);
        OpLdy(cpu, 0xc37du);
        STATUS_CALL(0x82a3b7u, 0x808878u, 3u);
        OpLdy(cpu, 0xc4ccu);
        STATUS_CALL(0x82a3beu, 0x808878u, 3u);
        OpLdy(cpu, 0xd292u);
        STATUS_CALL(0x82a3c5u, 0x808878u, 3u);
        OpLoadA(cpu, 0x8eu);
        OpSta(memory, cpu, OpDp(cpu, STATUS_TEXT_BANK));
        OpLdy(cpu, 0xd591u);
        STATUS_CALL(0x82a3d0u, 0x808878u, 3u);
        break;
    case 0xa3d6u:
        STATUS_CALL(0x82a3d6u, 0x829971u, 2u);
        STATUS_CALL(0x82a3d9u, 0x82f9c9u, 2u);
        OpLdx(cpu, 0x308au);
        STATUS_CALL(0x82a3dfu, 0x8294c0u, 2u);
        OpLoadA(cpu, 0x20u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
        OpLdx(cpu, 0x1560u);
        OpWriteX(memory, cpu, OpDp(cpu, STATUS_TILE_CURSOR), cpu->x);
        OpLoadA(cpu, 0x8eu);
        OpSta(memory, cpu, OpDp(cpu, STATUS_TEXT_BANK));
        if (checkpoint)
            checkpoint(context, cpu, 0x82a3f0u);
        OpLdy(cpu, 0xc37du);
        STATUS_CALL(0x82a3f3u, 0x808878u, 3u);
        OpLoadA(cpu, 0x8eu);
        OpSta(memory, cpu, OpDp(cpu, STATUS_TEXT_BANK));
        OpLdy(cpu, 0xd591u);
        STATUS_CALL(0x82a3feu, 0x808878u, 3u);
        return ExecutionReturned(0x82a402u);
    case 0xa403u:
        OpLoadA(cpu, 0u);
        do {
            if (visits++ == STATUS_LOOP_LIMIT)
                return ExecutionHandoff(cpu, 0x82a405u);
            PushAccumulator8(memory, cpu);
            OpRepWidths(cpu, 0x20u);
            OpAndValue(cpu, 0x00ffu);
            OpAslA(cpu);
            OpTax(cpu);
            OpLda(memory, cpu, OpAbsX(cpu, WRAM_MENU_PARTY_CHARACTER_OFFSET));
            OpSta(memory, cpu, OpDp(cpu, STATUS_CHARACTER));
            OpLda(memory, cpu, OpLongX(cpu, 0x82a425u));
            OpTax(cpu);
            OpSepWidths(cpu, 0x20u);
            STATUS_CALL(0x82a419u, 0x82950eu, 2u);
            LoadA8(cpu, Pull8(memory, cpu));
            OpIncA(cpu);
            OpCmp(memory, cpu, OpAbs(cpu, WRAM_MENU_PARTY_MEMBER_COUNT));
        } while (!cpu->zero);
        break;
    default:
        return ExecutionHandoff(cpu, 0x820000u | target);
    }
#undef STATUS_CALL
    OpLoadA(cpu, 0x08u);
    OpTestBits(memory, cpu, OpDp(cpu, 0x74u), 1u);
    return ExecutionReturned(0x82a431u);
}
