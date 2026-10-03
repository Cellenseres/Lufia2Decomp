#include "core/cpu_ops.h"
#include "lufia2/ancient_cave.h"
#include "system/dp_scratch.h"
#include "system/wram.h"

enum { CAVE_CARRY_ITEM = 0x54, CAVE_CARRY_COUNT = 0x58 };

Lufia2ExecutionResult Lufia2AncientCaveCarryBlueItem(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x84u || cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, ((uint32_t)cpu->program_bank << 16) | 0x8af4u);
    OpSta(memory, cpu, OpDp(cpu, CAVE_CARRY_ITEM));
    OpAndValue(cpu, 0x01ffu);
    cpu->carry = 0;
    if (!cpu->zero) {
        OpLdx(cpu, 0u);
        do {
            OpCmp(memory, cpu, OpLongX(cpu, 0x94eea0u));
            if (cpu->zero) {
                OpStepMem(memory, cpu, OpDp(cpu, CAVE_CARRY_COUNT), 1);
                OpLda(memory, cpu, OpDp(cpu, CAVE_CARRY_ITEM));
                OpSepWidths(cpu, 0x20u);
                OpSta(memory, cpu, OpAbs(cpu, SNES_WMDATA));
                ExchangeAccumulatorBytes(cpu);
                OpSta(memory, cpu, OpAbs(cpu, SNES_WMDATA));
                OpRepWidths(cpu, 0x20u);
                cpu->carry = 1;
                return ExecutionReturned(0x848b1fu);
            }
            OpInx(cpu);
            OpInx(cpu);
            OpCpx(cpu, 0x52u);
        } while (!cpu->carry);
        cpu->carry = 0;
    }
    return ExecutionReturned(0x848b1fu);
}

enum {
    CAVE_RESTORE_DESTINATION = 0x5d,
    CAVE_RESTORE_BANK = 0x5f,
    CAVE_ITEM_ID = 0x09cf,
    CAVE_ITEM_QUANTITY = 0x09cd,
    CAVE_ITEM_RESULT = 0x09d0,
    CAVE_LOOP_LIMIT = 65536,
    CAVE_CARRY_LOOP_LIMIT = 4096,
};

/* Runs the child routine at target as a JSL from site in bank $84. Reports an
 * unwound child, a hand-off when the CPU comes back in an unsupported mode, or
 * a return at site + 4. */
static Lufia2ExecutionResult CaveChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target) {
    SimulateJslFrame(memory, cpu, 0x84u, (uint16_t)(site + 3u));
    if (!child(context, cpu, target, site, 3u)) {
        Lufia2ExecutionResult result = ExecutionReturned(site);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, site + 4u);
    return ExecutionReturned(site + 4u);
}

/* Calls Lufia2AncientCaveCarryBlueItem as a JSR from site and hands off when
 * the return address it leaves on the stack is not the one pushed. */
static Lufia2ExecutionResult CarryBlueItem(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site) {
    uint8_t low, high;
    uint16_t frame;
    Lufia2ExecutionResult result;
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    result = Lufia2AncientCaveCarryBlueItem(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    low = Pull8(memory, cpu);
    high = Pull8(memory, cpu);
    frame = (uint16_t)(low | ((uint16_t)high << 8));
    if (frame != (uint16_t)(site + 2u))
        return ExecutionHandoff(cpu, 0x840000u | (uint16_t)(frame + 1u));
    return result;
}

/* Tests address against the mask in A (zero when no bit is set), then sets
 * those bits, or clears them when clear is non-zero. */
static void CaveTestSetBits(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint32_t address, uint8_t clear) {
    const uint16_t value = OpReadM(memory, cpu, address);
    cpu->zero = (value & OpA(cpu)) == 0u;
    OpWriteM(memory, cpu, address,
        clear ? value & (uint16_t)~OpA(cpu) : value | OpA(cpu));
}

/* Builds the list of items that leave the cave with the party. Items from
 * the $0A8D list that match the table at $91:FFCA are appended to the carry
 * list and set an event flag ($80:BE1A); blue equipment from that list and
 * from the equipment words at $0C13 in each party record (stride $BE) is
 * appended by Lufia2AncientCaveCarryBlueItem. The count is stored last. */
static Lufia2ExecutionResult CollectCaveCarryItems(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    Lufia2ExecutionResult result;
    unsigned steps = 0u;
    OpLdx(cpu, WRAM_ANCIENT_CAVE_CARRY_ITEMS & 0xffffu);
    OpWrite16(memory, OpAbs(cpu, SNES_WMADDL), cpu->x);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WMADDH));
    OpRepWidths(cpu, 0x20u);
    OpLdy(cpu, 0u);
    OpWrite16(memory, OpDp(cpu, CAVE_CARRY_COUNT), cpu->y);
    do {
        if (++steps > CAVE_CARRY_LOOP_LIMIT)
            return ExecutionHandoff(cpu, 0x84896bu);
        OpLda(memory, cpu, OpAbsY(cpu, 0x0a8du));
        OpAndValue(cpu, 0x01ffu);
        OpLdx(cpu, 0u);
        do {
            OpCmp(memory, cpu, OpLongX(cpu, 0x91ffcau));
            if (cpu->zero) {
                PushIndex(memory, cpu);
                OpStepMem(memory, cpu, OpDp(cpu, CAVE_CARRY_COUNT), 1);
                OpSepWidths(cpu, 0x20u);
                OpSta(memory, cpu, OpAbs(cpu, SNES_WMDATA));
                ExchangeAccumulatorBytes(cpu);
                OpOraValue(cpu, 2u);
                OpSta(memory, cpu, OpAbs(cpu, SNES_WMDATA));
                OpTxa(cpu);
                OpLsrA(cpu);
                cpu->carry = 0;
                OpAdcValue(cpu, 0xc8u);
                result = CaveChild(memory, cpu, child, context, 0x84898du, 0x80be1au);
                if (result.flow != LUFIA2_EXECUTION_RETURNED)
                    return result;
                OpLda(memory, cpu, OpLongX(cpu, WRAM_EVENT_FLAGS));
                OpOraValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_SCRATCH_D)));
                OpSta(memory, cpu, OpLongX(cpu, WRAM_EVENT_FLAGS));
                OpPullX(memory, cpu);
                OpRepWidths(cpu, 0x20u);
            }
            OpInx(cpu);
            OpInx(cpu);
            OpCpx(cpu, 0x12u);
        } while (!cpu->carry);
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, 0xc0u);
    } while (!cpu->carry);
    OpLdy(cpu, 0u);
    do {
        if (++steps > CAVE_CARRY_LOOP_LIMIT)
            return ExecutionHandoff(cpu, 0x8489afu);
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbsY(cpu, 0x0a8du));
        result = CarryBlueItem(memory, cpu, 0x89b4u);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
        OpSepWidths(cpu, 0x20u);
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, 0xc0u);
    } while (!cpu->carry);
    OpRepWidths(cpu, 0x20u);
    OpLdy(cpu, 0u);
    do {
        if (++steps > CAVE_CARRY_LOOP_LIMIT)
            return ExecutionHandoff(cpu, 0x8489c5u);
        for (unsigned slot = 0u; slot < 6u; ++slot) {
            OpLda(memory, cpu, OpAbsY(cpu, (uint16_t)(0x0c13u + 2u * slot)));
            result = CarryBlueItem(memory, cpu, (uint16_t)(0x89c8u + 6u * slot));
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
        }
        OpTya(cpu);
        cpu->carry = 0;
        OpAdcValue(cpu, 0xbeu);
        OpTay(cpu);
        OpCmpValue(cpu, 0x0532u);
    } while (!cpu->carry);
    OpLda(memory, cpu, OpDp(cpu, CAVE_CARRY_COUNT));
    OpSta(memory, cpu, WRAM_ANCIENT_CAVE_CARRY_COUNT);
    return ExecutionReturned(0x8489fau);
}

/* Copies the backup stream in $7F back to the ranges listed at $84:8B20, read
 * through the WRAM data port. Each entry is five bytes: address, bank, length;
 * a $FFFF address ends the list. */
static Lufia2ExecutionResult RestoreCaveBackup(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    unsigned steps = 0u;
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, WRAM_ANCIENT_CAVE_STATE_BACKUP & 0xffffu);
    OpWrite16(memory, OpAbs(cpu, SNES_WMADDL), cpu->x);
    OpLoadA(cpu, 0x7fu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WMADDH));
    OpLdx(cpu, 0u);
    for (;;) {
        if (++steps > CAVE_LOOP_LIMIT)
            return ExecutionHandoff(cpu, 0x848a0au);
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbsX(cpu, 0x8b20u));
        OpCmpValue(cpu, 0xffffu);
        if (cpu->zero)
            break;
        OpSta(memory, cpu, OpDp(cpu, CAVE_RESTORE_DESTINATION));
        OpLda(memory, cpu, OpAbsX(cpu, 0x8b23u));
        OpSta(memory, cpu, OpDp(cpu, CAVE_CARRY_COUNT));
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbsX(cpu, 0x8b22u));
        OpSta(memory, cpu, OpDp(cpu, CAVE_RESTORE_BANK));
        OpLdy(cpu, 0u);
        do {
            uint32_t destination;
            if (++steps > CAVE_LOOP_LIMIT)
                return ExecutionHandoff(cpu, 0x848a25u);
            OpLda(memory, cpu, OpAbs(cpu, SNES_WMDATA));
            destination = Read16Direct(memory, cpu, CAVE_RESTORE_DESTINATION);
            destination |= (uint32_t)Read8(memory,
                DirectAddress(cpu, CAVE_RESTORE_BANK)) << 16;
            OpSta(memory, cpu, (destination + cpu->y) & 0xffffffu);
            OpIny(cpu);
            OpCpy(cpu, OpRead16(memory, OpDp(cpu, CAVE_CARRY_COUNT)));
        } while (!cpu->carry);
        for (unsigned i = 0u; i < 5u; ++i)
            OpInx(cpu);
    }
    OpSepWidths(cpu, 0x20u);
    EmitExecutionCheckpoint(memory, cpu, 0x848a38u);
    return ExecutionReturned(0x848a38u);
}

/* Gives items back through $82:E746. With the inventory mode set it returns
 * the saved items until an empty entry; otherwise it returns the carry list,
 * recording each result in a bit of DP $AE. */
static Lufia2ExecutionResult ReturnCaveItems(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    Lufia2ExecutionResult result;
    unsigned steps = 0u;
    OpLda(memory, cpu, WRAM_ANCIENT_CAVE_INVENTORY_MODE);
    if (!cpu->zero) {
        OpLdx(cpu, 0u);
        for (;;) {
            if (++steps > CAVE_LOOP_LIMIT)
                return ExecutionHandoff(cpu, 0x848a41u);
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpLongX(cpu, WRAM_ANCIENT_CAVE_SAVED_ITEMS));
            if (cpu->zero)
                break;
            OpAndValue(cpu, 0x01ffu);
            OpSta(memory, cpu, OpAbs(cpu, CAVE_ITEM_ID));
            OpSepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpLongX(cpu, WRAM_ANCIENT_CAVE_SAVED_ITEMS + 1u));
            OpLsrA(cpu);
            OpSta(memory, cpu, OpAbs(cpu, CAVE_ITEM_QUANTITY));
            OpStz(memory, cpu, OpAbs(cpu, 0x09ceu));
            PushIndex(memory, cpu);
            OpStz(memory, cpu, OpDp(cpu, 0x31u));
            OpLoadA(cpu, 4u);
            OpSta(memory, cpu, OpDp(cpu, 0x30u));
            result = CaveChild(memory, cpu, child, context, 0x848a63u, 0x82e746u);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
            OpPullX(memory, cpu);
            OpInx(cpu);
            OpInx(cpu);
        }
        OpSepWidths(cpu, 0x20u);
    } else {
        OpLda(memory, cpu, WRAM_ANCIENT_CAVE_CARRY_COUNT);
        if (!cpu->zero) {
            OpAslA(cpu);
            OpSta(memory, cpu, WRAM_ANCIENT_CAVE_CARRY_COUNT);
            OpStz(memory, cpu, OpDp(cpu, 0xaeu));
            OpLdx(cpu, 0u);
            do {
                if (++steps > CAVE_LOOP_LIMIT)
                    return ExecutionHandoff(cpu, 0x848a81u);
                PushIndex(memory, cpu);
                OpRepWidths(cpu, 0x20u);
                OpLda(memory, cpu, OpLongX(cpu, WRAM_ANCIENT_CAVE_CARRY_ITEMS));
                OpAndValue(cpu, 0x01ffu);
                OpSta(memory, cpu, OpAbs(cpu, CAVE_ITEM_ID));
                OpSepWidths(cpu, 0x20u);
                OpLda(memory, cpu, OpLongX(cpu, WRAM_ANCIENT_CAVE_CARRY_ITEMS + 1u));
                OpLsrA(cpu);
                if (cpu->zero)
                    OpIncA(cpu);
                OpSta(memory, cpu, OpAbs(cpu, CAVE_ITEM_QUANTITY));
                OpStz(memory, cpu, OpDp(cpu, 0x31u));
                OpLoadA(cpu, 4u);
                OpSta(memory, cpu, OpDp(cpu, 0x30u));
                result = CaveChild(memory, cpu, child, context, 0x848aa1u, 0x82e746u);
                if (result.flow != LUFIA2_EXECUTION_RETURNED)
                    return result;
                OpLda(memory, cpu, OpAbs(cpu, CAVE_ITEM_RESULT));
                CaveTestSetBits(memory, cpu, OpDp(cpu, 0xaeu), 0u);
                OpPullX(memory, cpu);
                OpInx(cpu);
                OpInx(cpu);
                OpTxa(cpu);
                OpCmp(memory, cpu, WRAM_ANCIENT_CAVE_CARRY_COUNT);
            } while (!cpu->carry);
        }
    }
    return ExecutionReturned(0x848ab4u);
}

Lufia2ExecutionResult Lufia2AncientCaveExit(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    Lufia2ExecutionResult result;
    if (!child || cpu->program_bank != 0x84u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, ((uint32_t)cpu->program_bank << 16) | 0x890bu);
    EmitExecutionCheckpoint(memory, cpu, 0x84890bu);
    Push8(memory, cpu, PackStatus(cpu));
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpLoadA(cpu, 8u);
    CaveTestSetBits(memory, cpu, OpAbs(cpu, WRAM_WINDOW_MODE), 0u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_PLAY_TIME_MINUTES));
    OpSta(memory, cpu, WRAM_ANCIENT_CAVE_EXIT_MINUTES);
    cpu->carry = 0;
    OpAdc(memory, cpu, OpAbs(cpu, WRAM_ANCIENT_CAVE_TOTAL_MINUTES & 0xffffu));
    OpCmpValue(cpu, 60u);
    if (cpu->carry) {
        cpu->carry = 1;
        OpSbcValue(cpu, 60u);
        OpStepMem(memory, cpu, OpAbs(cpu, WRAM_ANCIENT_CAVE_TOTAL_HOURS & 0xffffu), 1);
    }
    OpSta(memory, cpu, OpAbs(cpu, WRAM_ANCIENT_CAVE_TOTAL_MINUTES & 0xffffu));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_PLAY_TIME_HOURS));
    OpSta(memory, cpu, WRAM_ANCIENT_CAVE_EXIT_HOURS);
    cpu->carry = 0;
    OpAdc(memory, cpu, OpAbs(cpu, WRAM_ANCIENT_CAVE_TOTAL_HOURS & 0xffffu));
    if (cpu->carry)
        OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_ANCIENT_CAVE_TOTAL_HOURS & 0xffffu));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ANCIENT_CAVE_COUNTER & 0xffffu));
    cpu->carry = 0;
    OpAdc(memory, cpu, OpAbs(cpu, WRAM_ANCIENT_CAVE_TOTAL_COUNTER & 0xffffu));
    if (cpu->carry)
        OpLoadA(cpu, 0xffffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_ANCIENT_CAVE_TOTAL_COUNTER & 0xffffu));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, WRAM_ANCIENT_CAVE_INVENTORY_MODE);
    if (cpu->zero) {
        result = CollectCaveCarryItems(memory, cpu, child, context);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
    }
    result = RestoreCaveBackup(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    result = ReturnCaveItems(memory, cpu, child, context);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpLoadA(cpu, 0x80u);
    CaveTestSetBits(memory, cpu, OpAbs(cpu, WRAM_PLAY_TIME_FRAMES), 0u);
    OpLda(memory, cpu, WRAM_ANCIENT_CAVE_EXIT_MINUTES);
    cpu->carry = 0;
    OpAdc(memory, cpu, OpAbs(cpu, WRAM_PLAY_TIME_MINUTES));
    OpCmpValue(cpu, 60u);
    if (cpu->carry) {
        cpu->carry = 1;
        OpSbcValue(cpu, 60u);
        OpStepMem(memory, cpu, OpAbs(cpu, WRAM_PLAY_TIME_HOURS), 1);
    }
    OpSta(memory, cpu, OpAbs(cpu, WRAM_PLAY_TIME_MINUTES));
    OpLda(memory, cpu, WRAM_ANCIENT_CAVE_EXIT_HOURS);
    cpu->carry = 0;
    OpAdc(memory, cpu, OpAbs(cpu, WRAM_PLAY_TIME_HOURS));
    if (cpu->carry)
        OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_PLAY_TIME_HOURS));
    OpLoadA(cpu, 0x80u);
    CaveTestSetBits(memory, cpu, OpAbs(cpu, WRAM_PLAY_TIME_FRAMES), 1u);
    result = CaveChild(memory, cpu, child, context, 0x848ae2u, 0x82c515u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    result = CaveChild(memory, cpu, child, context, 0x848ae6u, 0x82c261u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpDp(cpu, 0x30u));
    result = CaveChild(memory, cpu, child, context, 0x848aeeu, 0x82e746u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x848af3u);
}
