#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "lufia2/ancient_cave.h"
#include "system/wram.h"

enum {
    DP_BLUE_ITEM = 0x54u,
    DP_SNAPSHOT_LENGTH = 0x58u,
    DP_SNAPSHOT_SOURCE = 0x5du,
    DP_CARRIED_ITEM_BYTES = 0x60u,
    ROM_BLUE_ITEMS = 0x94eea0u,
    ROM_PARTY_RECORDS = 0x859ebau,
    ROM_SNAPSHOT_RANGES = 0x8b20u,
    BLUE_ITEM_TABLE_BYTES = 0x52u,
    INVENTORY_BYTES = 0xc0u,
    EQUIPMENT_OFFSET = 0x66u,
    EQUIPMENT_BYTES = 12u
};

static void TestBlueItemBody(const Lufia2Memory *memory, Lufia2CpuState *cpu) {

    OpAndValue(cpu, 0x01ffu);
    OpSta(memory, cpu, OpDp(cpu, DP_BLUE_ITEM));
    OpLdx(cpu, 0u);
    for (;;) {
        OpLda(memory, cpu, OpLongX(cpu, ROM_BLUE_ITEMS));
        OpCmp(memory, cpu, OpDp(cpu, DP_BLUE_ITEM));
        if (cpu->zero) {
            cpu->carry = 1u;
            break;
        }
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, BLUE_ITEM_TABLE_BYTES);
        if (cpu->carry) {
            cpu->carry = 0u;
            break;
        }
    }
}

static void TestBlueItem(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                         uint16_t back) {
    SimulateJsrFrame(memory, cpu, back);
    TestBlueItemBody(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

static void CarryBlueItemBody(const Lufia2Memory *memory, Lufia2CpuState *cpu) {

    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_CARRIED_ITEM_BYTES)));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ANCIENT_CAVE_SAVED_ITEMS));
    OpInx(cpu);
    OpInx(cpu);
    OpWriteX(memory, cpu, OpDp(cpu, DP_CARRIED_ITEM_BYTES), cpu->x);
}

static void CarryBlueItem(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                          uint16_t back) {
    SimulateJsrFrame(memory, cpu, back);
    CarryBlueItemBody(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

static void CarryInventoryBlueItems(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdy(cpu, 0u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_CARRIED_ITEM_BYTES), cpu->y);
    OpRepWidths(cpu, 0x20u);
    do {
        OpLda(memory, cpu, OpAbsY(cpu, WRAM_INVENTORY_PACKED_ITEMS));
        TestBlueItem(memory, cpu, 0x87ceu);
        if (cpu->carry) {
            OpLda(memory, cpu, OpAbsY(cpu, WRAM_INVENTORY_PACKED_ITEMS));
            CarryBlueItem(memory, cpu, 0x87d6u);
            LoadA16(cpu, cpu->direct_page);
            OpSta(memory, cpu, OpAbsY(cpu, WRAM_INVENTORY_PACKED_ITEMS));
        }
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, INVENTORY_BYTES);
    } while (!cpu->carry);
}

static void CarryEquippedBlueItems(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0u);
    do {
        LoadA16(cpu, cpu->direct_page);
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_MENU_PARTY_FIRST_ID));
        OpAndValue(cpu, 0xffu);
        OpCmpValue(cpu, 0xffu);
        if (!cpu->zero) {
            OpPushX(memory, cpu);
            OpAslA(cpu);
            OpTax(cpu);
            OpLda(memory, cpu, OpLongX(cpu, ROM_PARTY_RECORDS));
            cpu->carry = 0u;
            OpAdcValue(cpu, EQUIPMENT_OFFSET);
            OpSta(memory, cpu, OpDp(cpu, DP_SNAPSHOT_SOURCE));
            OpLdy(cpu, 0u);
            do {
                OpLda(memory, cpu,
                      AbsoluteIndexedAddress(
                          cpu, Read16Direct(memory, cpu, DP_SNAPSHOT_SOURCE), cpu->y));
                TestBlueItem(memory, cpu, 0x8807u);
                if (cpu->carry) {
                    OpLda(memory, cpu,
                          AbsoluteIndexedAddress(
                              cpu, Read16Direct(memory, cpu, DP_SNAPSHOT_SOURCE),
                              cpu->y));
                    OpAndValue(cpu, 0x01ffu);
                    OpOraValue(cpu, 0x0200u);
                    CarryBlueItem(memory, cpu, 0x8814u);
                    LoadA16(cpu, cpu->direct_page);
                    OpSta(memory, cpu,
                          AbsoluteIndexedAddress(
                              cpu, Read16Direct(memory, cpu, DP_SNAPSHOT_SOURCE),
                              cpu->y));
                }
                OpIny(cpu);
                OpIny(cpu);
                OpCpy(cpu, EQUIPMENT_BYTES);
            } while (!cpu->carry);
            OpPullX(memory, cpu);
        }
        OpInx(cpu);
        OpCpx(cpu, 4u);
    } while (!cpu->carry);
    LoadA16(cpu, cpu->direct_page);
    CarryBlueItem(memory, cpu, 0x8829u);
}

static void SaveCaveEntryState(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, 0u);
    for (;;) {
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbsX(cpu, ROM_SNAPSHOT_RANGES));
        OpCmpValue(cpu, 0xffffu);
        if (cpu->zero)
            break;
        OpSta(memory, cpu, OpDp(cpu, DP_SNAPSHOT_SOURCE));
        OpLda(memory, cpu, OpAbsX(cpu, ROM_SNAPSHOT_RANGES + 3u));
        OpSta(memory, cpu, OpDp(cpu, DP_SNAPSHOT_LENGTH));
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbsX(cpu, ROM_SNAPSHOT_RANGES + 2u));
        OpSta(memory, cpu, OpDp(cpu, DP_SNAPSHOT_SOURCE + 2u));
        OpLdy(cpu, 0u);
        do {
            OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, DP_SNAPSHOT_SOURCE));
            OpSta(memory, cpu, OpAbs(cpu, SNES_WMDATA));
            OpIny(cpu);
            OpCpy(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SNAPSHOT_LENGTH)));
        } while (!cpu->carry);
        for (unsigned byte = 0u; byte < 5u; ++byte)
            OpInx(cpu);
    }
    OpSepWidths(cpu, 0x20u);
}

Lufia2ExecutionResult Lufia2AncientCaveEnter(const Lufia2Memory *memory,
                                             Lufia2CpuState *cpu,
                                             Lufia2PushedChildCall child,
                                             void *context) {
    if (cpu->program_bank != 0x84u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal ||
        cpu->stack < 0x1f20u || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x84878au);
    Push8(memory, cpu, PackStatus(cpu));
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpRepWidths(cpu, 0x10u);
    OpLdx(cpu, WRAM_ANCIENT_CAVE_STATE_BACKUP & 0xffffu);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_WMADDL), cpu->x);
    OpLoadA(cpu, 0x7fu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WMADDH));
    OpLoadA(cpu, 8u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_WINDOW_MODE), false);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_ANCIENT_CAVE_ENTRY_COUNT)));
    OpInx(cpu);
    if (cpu->zero)
        OpDex(cpu);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_ANCIENT_CAVE_ENTRY_COUNT), cpu->x);
    LoadA16(cpu, cpu->direct_page);
    const uint32_t cleared[] = {
        WRAM_UNK_7FE696,    WRAM_ANCIENT_CAVE_INVENTORY_MODE, WRAM_SPECIAL_REWARD_STATE,
        WRAM_CAVE_UNK_E75D, WRAM_CAVE_UNK_E75D + 1u,          WRAM_CAVE_UNK_E75D + 2u};
    for (unsigned i = 0u; i < sizeof cleared / sizeof cleared[0]; ++i)
        OpSta(memory, cpu, cleared[i]);
    CarryInventoryBlueItems(memory, cpu);
    CarryEquippedBlueItems(memory, cpu);
    SaveCaveEntryState(memory, cpu);
    if (!CallChildWithFrame(memory, cpu, child, context, 0x84885du, 0x848888u, 2u,
                            0x84u)) {
        const Lufia2ExecutionResult result = {LUFIA2_EXECUTION_CHILD_UNWOUND, 0x84885du,
                                              0u};
        return result;
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x848861u);
}

Lufia2ExecutionResult Lufia2AncientCaveAppendSavedItem(const Lufia2Memory *memory,
                                                       Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x84u || cpu->direct_page || cpu->decimal ||
        cpu->index_is_8_bit || cpu->accumulator_is_8_bit != 0u ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x848862u);
    CarryBlueItemBody(memory, cpu);
    return ExecutionReturned(0x84886cu);
}

Lufia2ExecutionResult Lufia2AncientCaveTestBlueItem(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x84u || cpu->direct_page || cpu->decimal ||
        cpu->index_is_8_bit || cpu->accumulator_is_8_bit != 0u ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x84886du);
    TestBlueItemBody(memory, cpu);
    return ExecutionReturned(cpu->carry ? 0x848887u : 0x848885u);
}
