#include "lufia2/field.h"
#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"

enum {
    REGION_X = 0x56,
    REGION_Y = 0x57,
    REGION_ITEM = 0x58,
    NAME_PREFIX = 0x60,
    NOTICE_FRAMES = 0x42,
    PLAYER_X = 0x06ba,
    PLAYER_Y = 0x06e2,
    NOTICE_FLAGS = 0x099c,
    OBJECT_ITEM = 0x09cf,
    OBJECT_ITEM_HIGH = 0x09d0,
    ITEM_RECORD_HIGH = 0x0a07,
    TEXT_SCRIPT_ADDRESS = 0x09b7,
    TEXT_CONTROL = 0x125d,
    FRAME_FLAGS = 0x0622,
    OBJECT_OPERAND = 0x7fd04e,
    OBJECT_FACING = 0x7fd09d,
    NOTICE_DURATION = 0x7fd0c1,
    REGION_LIST = 0xf000,
    REGION_LIST_OFFSET = 0xf002
};

static uint8_t ItemRegionWidths(const Lufia2CpuState *cpu, uint8_t wide) {
    return !cpu->index_is_8_bit && cpu->accumulator_is_8_bit != wide;
}

static uint8_t ItemRegionChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t site,
    uint32_t target, uint8_t frame, uint8_t wide,
    Lufia2ExecutionResult *result) {
    if (!CallChildWithFrame(memory, cpu, child, context,
            site, target, frame, 0x8eu)) {
        *result = ExecutionReturned(site);
        result->flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return 0u;
    }
    if (wide < 2u && !ItemRegionWidths(cpu, wide)) {
        *result = ExecutionHandoff(cpu, site + frame + 1u);
        return 0u;
    }
    return 1u;
}

static uint8_t ItemRegionContains(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t rectangle) {
    OpLda(memory, cpu, OpDp(cpu, REGION_X));
    OpCmpValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, rectangle)));
    if (!cpu->carry)
        return 0u;
    OpCmpValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, rectangle + 2u)));
    if (cpu->carry)
        return 0u;
    OpLda(memory, cpu, OpDp(cpu, REGION_Y));
    OpCmpValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, rectangle + 1u)));
    if (!cpu->carry)
        return 0u;
    OpCmpValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, rectangle + 3u)));
    return !cpu->carry;
}

static void FindItemRegion(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, REGION_LIST_OFFSET)));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, REGION_ITEM));
    for (;;) {
        OpLda(memory, cpu, OpAbsX(cpu, REGION_LIST));
        OpCmpValue(cpu, 0xffu);
        if (cpu->zero)
            return;
        uint8_t found = ItemRegionContains(memory, cpu, REGION_LIST + 5u);
        if (!found) {
            OpLda(memory, cpu, OpAbsX(cpu, REGION_LIST + 9u));
            OpCmpValue(cpu, 0xffu);
            if (!cpu->zero)
                found = ItemRegionContains(memory, cpu, REGION_LIST + 9u);
        }
        if (found) {
            OpLda(memory, cpu, OpAbsX(cpu, REGION_LIST + 13u));
            OpSta(memory, cpu, OpDp(cpu, REGION_ITEM));
            return;
        }
        OpRepWidths(cpu, 0x20u);
        OpLoadA(cpu, cpu->x);
        cpu->carry = 0u;
        OpAdcValue(cpu, 15u);
        OpLdx(cpu, cpu->accumulator);
        OpSepWidths(cpu, 0x20u);
    }
}

static Lufia2ExecutionResult ReturnItemRegion(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x8eb739u);
}

Lufia2ExecutionResult Lufia2FieldTryItemRegion(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ItemRegionWidths(cpu, 0u) || !child || cpu->decimal ||
            cpu->program_bank != 0x8eu)
        return ExecutionHandoff(cpu, 0x8eb63bu);
    Push8(memory, cpu, cpu->data_bank);
    OpLda(memory, cpu, OpAbs(cpu, PLAYER_X));
    OpSta(memory, cpu, OpDp(cpu, REGION_X));
    OpLda(memory, cpu, OpAbs(cpu, PLAYER_Y));
    OpSta(memory, cpu, OpDp(cpu, REGION_Y));
    OpLoadA(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpRepWidths(cpu, 0x10u);
    FindItemRegion(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, REGION_ITEM));
    OpCmpValue(cpu, 0xffu);
    cpu->carry = 0u;
    if (cpu->zero)
        return ReturnItemRegion(memory, cpu);
    OpLda(memory, cpu, OBJECT_FACING);
    OpCmpValue(cpu, 0u);
    if (cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, REGION_Y));
        OpCmpValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, REGION_LIST + 2u)));
    } else {
        OpCmpValue(cpu, 1u);
        if (!cpu->zero) {
            cpu->carry = 0u;
            return ReturnItemRegion(memory, cpu);
        }
        OpLda(memory, cpu, OpAbsX(cpu, REGION_LIST + 2u));
        OpCmpValue(cpu, OpReadM(memory, cpu, OpDp(cpu, REGION_Y)));
    }
    if (!cpu->carry)
        return ReturnItemRegion(memory, cpu);
    OpLda(memory, cpu, OpAbsX(cpu, REGION_LIST));
    OpSta(memory, cpu, OBJECT_OPERAND);
    OpLoadA(cpu, 3u);
    OpLdx(cpu, 2u);
    Lufia2ExecutionResult result;
    if (!ItemRegionChild(memory, cpu, child, context,
            0x8eb6e0u, 0x80c12eu, 3u, 0u, &result))
        return result;
    OpCpy(cpu, 0xffffu);
    if (!cpu->zero) {
        OpLda(memory, cpu, 0x0009b9u);
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        for (;;) {
            if (!ItemRegionChild(memory, cpu, child, context,
                    0x8eb6efu, 0x8eb7cdu, 2u, 0u, &result))
                return result;
            OpOraValue(cpu, 0u);
            if (cpu->zero)
                break;
            OpDecA(cpu);
            OpCmpValue(cpu, OpReadM(memory, cpu, OBJECT_OPERAND));
            if (!cpu->zero) {
                if (!ItemRegionChild(memory, cpu, child, context,
                        0x8eb724u, 0x8eb7cdu, 2u, 0u, &result))
                    return result;
                if (!ItemRegionChild(memory, cpu, child, context,
                        0x8eb727u, 0x8eb7cdu, 2u, 0u, &result))
                    return result;
                continue;
            }
            if (!ItemRegionChild(memory, cpu, child, context,
                    0x8eb6fdu, 0x8eb7cdu, 2u, 0u, &result))
                return result;
            OpSta(memory, cpu, OpAbs(cpu, OBJECT_ITEM));
            OpSta(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
            if (!ItemRegionChild(memory, cpu, child, context,
                    0x8eb706u, 0x8eb7cdu, 2u, 0u, &result))
                return result;
            OpSta(memory, cpu, OpAbs(cpu, OBJECT_ITEM_HIGH));
            OpSta(memory, cpu, OpAbs(cpu, ITEM_RECORD_HIGH));
            PushY(memory, cpu);
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpAbs(cpu, OBJECT_ITEM));
            if (!ItemRegionChild(memory, cpu, child, context,
                    0x8eb715u, 0x82fb1fu, 3u, 1u, &result))
                return result;
            OpSepWidths(cpu, 0x20u);
            OpPullY(memory, cpu);
            if (!cpu->carry) {
                if (!ItemRegionChild(memory, cpu, child, context,
                        0x8eb71eu, 0x8eb760u, 2u, 2u, &result))
                    return result;
                cpu->carry = 1u;
                return ReturnItemRegion(memory, cpu);
            }
            OpLda(memory, cpu, OpAbs(cpu, NOTICE_FLAGS));
            OpBitValue(cpu, 0x20u);
            if (!cpu->zero)
                return ReturnItemRegion(memory, cpu);
            OpLdx(cpu, 0x8007u);
            OpWriteX(memory, cpu, OpAbs(cpu, TEXT_SCRIPT_ADDRESS), cpu->x);
            OpLoadA(cpu, 0x85u);
            OpSta(memory, cpu, OpAbs(cpu, WRAM_SCENE_SCRIPT_BANK));
            OpLdx(cpu, 0x0180u);
            OpWriteX(memory, cpu, OpAbs(cpu, TEXT_CONTROL), cpu->x);
            if (!ItemRegionChild(memory, cpu, child, context,
                    0x8eb752u, 0x8eb550u, 2u, 0u, &result))
                return result;
            if (!ItemRegionChild(memory, cpu, child, context,
                    0x8eb755u, 0x80c8d5u, 3u, 0u, &result))
                return result;
            OpLoadA(cpu, 0x20u);
            OpTestBits(memory, cpu, OpAbs(cpu, NOTICE_FLAGS), 1u);
            return ReturnItemRegion(memory, cpu);
        }
    }
    OpLda(memory, cpu, OBJECT_OPERAND);
    ExchangeAccumulatorBytes(cpu);
    OpLoadA(cpu, 0u);
    if (!ItemRegionChild(memory, cpu, child, context,
            0x8eb733u, 0x83f559u, 3u, 2u, &result))
        return result;
    cpu->carry = 1u;
    return ReturnItemRegion(memory, cpu);
}

Lufia2ExecutionResult Lufia2FieldShowObjectItemNotice(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ItemRegionWidths(cpu, 0u) || !child || cpu->decimal ||
            cpu->program_bank != 0x8eu)
        return ExecutionHandoff(cpu, 0x8eb760u);
    Lufia2ExecutionResult result;
    if (!ItemRegionChild(memory, cpu, child, context,
            0x8eb760u, 0x81f1c5u, 3u, 0u, &result))
        return result;
    OpLdx(cpu, 0x8013u);
    OpWriteX(memory, cpu, OpDp(cpu, NAME_PREFIX), cpu->x);
    OpLdx(cpu, 0x0b77u);
    if (!ItemRegionChild(memory, cpu, child, context,
            0x8eb76cu, 0x8eb5fbu, 3u, 0u, &result))
        return result;
    OpLdx(cpu, 0xe000u);
    OpWriteX(memory, cpu, OpAbs(cpu, TEXT_SCRIPT_ADDRESS), cpu->x);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_SCENE_SCRIPT_BANK));
    OpLdx(cpu, 0x0180u);
    OpWriteX(memory, cpu, OpAbs(cpu, TEXT_CONTROL), cpu->x);
    if (!ItemRegionChild(memory, cpu, child, context,
            0x8eb781u, 0x8eb550u, 2u, 0u, &result))
        return result;
    if (!ItemRegionChild(memory, cpu, child, context,
            0x8eb784u, 0x80c8d5u, 3u, 0u, &result))
        return result;
    OpLoadA(cpu, 0x20u);
    OpTestBits(memory, cpu, OpAbs(cpu, NOTICE_FLAGS), 1u);
    OpLoadA(cpu, 0x3cu);
    OpSta(memory, cpu, NOTICE_DURATION);
    OpLda(memory, cpu, OBJECT_OPERAND);
    ExchangeAccumulatorBytes(cpu);
    OpLoadA(cpu, 0u);
    if (!ItemRegionChild(memory, cpu, child, context,
            0x8eb79au, 0x83f559u, 3u, 0u, &result))
        return result;
    OpLoadA(cpu, 1u);
    OpTestBits(memory, cpu, OpAbs(cpu, FRAME_FLAGS), 1u);
    OpStz(memory, cpu, OpDp(cpu, NOTICE_FRAMES));
    for (;;) {
        if (!ItemRegionChild(memory, cpu, child, context,
                0x8eb7a5u, 0x838192u, 3u, 0u, &result))
            return result;
        OpLoadA(cpu, 0x80u);
        if (!ItemRegionChild(memory, cpu, child, context,
                0x8eb7abu, 0x8eb142u, 3u, 0u, &result))
            return result;
        uint8_t dismissed = !cpu->zero;
        if (!dismissed) {
            OpLoadA(cpu, 0x8fu);
            if (!ItemRegionChild(memory, cpu, child, context,
                    0x8eb7b3u, 0x8eb149u, 3u, 0u, &result))
                return result;
            dismissed = !cpu->zero;
        }
        if (dismissed) {
            OpLoadA(cpu, 1u);
            OpSta(memory, cpu, NOTICE_DURATION);
            break;
        }
        OpLda(memory, cpu, OpDp(cpu, NOTICE_FRAMES));
        OpCmpValue(cpu, 0x3cu);
        if (cpu->carry)
            break;
    }
    OpLoadA(cpu, 1u);
    OpTestBits(memory, cpu, OpAbs(cpu, FRAME_FLAGS), 0u);
    return ExecutionReturned(0x8eb7ccu);
}
