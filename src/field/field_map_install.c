/* Regular map resources and attribute-grid installation. */

#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    MAP_ID = 0x24u,
    MAP_ID_HIGH = 0x25u,
    MAP_TABLE_OFFSET = 0x22u,
    MAP_SECOND_SECTION_OFFSET = 0x2du,
    MAP_RESOURCE_ID = 0x54u,
    MAP_RESOURCE_DESTINATION = 0x60u,
    MAP_RESOURCE_DESTINATION_BANK = 0x62u,
    MAP_SECTION_SOURCE = 0x5du,
    MAP_SECTION_FLAGS = 0x28u,
    MAP_SECTION_FLAGS_HIGH = 0x29u,
};

static Lufia2ExecutionResult MapUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static uint8_t MapChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target, uint8_t frame) {
    if (frame == 3u)
        SimulateJslFrame(memory, cpu, cpu->program_bank, (uint16_t)(site + 3u));
    else
        SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    return child(context, cpu, target, site, frame);
}

#define MAP_CALL(site, target, frame) \
    do { \
        if (!MapChild(memory, cpu, child, context, site, target, frame)) \
            return MapUnwound(site); \
    } while (0)

Lufia2ExecutionResult Lufia2FieldInstallMap(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, Lufia2ExecutionCheckpoint checkpoint,
    void *context) {
    if (!child || !cpu->accumulator_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x83b53bu);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x10u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_FLAGS));
    OpBitValue(cpu, 1u);
    uint8_t loaded = cpu->zero;
    if (loaded) {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID));
        if (checkpoint)
            checkpoint(context, cpu, 0x83b548u);
        MAP_CALL(0x83b548u, 0x80eae7u, 3u);
        OpLda(memory, cpu, WRAM_FIELD_LAYER_WIDTH + 2u);
        OpSta(memory, cpu, OpAbs(cpu, 0x4202u));
        OpLda(memory, cpu, WRAM_FIELD_LAYER_HEIGHT + 2u);
        OpSta(memory, cpu, OpAbs(cpu, 0x4203u));
        PushDataBank(memory, cpu);
        TransferDirectToA(cpu);
        OpSta(memory, cpu, WRAM_FIELD_MAP_ATTRIBUTES);
        OpRepWidths(cpu, 0x20u);
        OpLdx(cpu, 0x4000u);
        OpTxy(cpu);
        OpIny(cpu);
        OpLda(memory, cpu, OpAbs(cpu, 0x4216u));
        OpDecA(cpu);
        OpDecA(cpu);
        OpMoveNext(memory, cpu, 0x7eu, 0x7eu);
        OpSepWidths(cpu, 0x20u);
        PullDataBank(memory, cpu);
        MAP_CALL(0x83b572u, 0x83b581u, 2u);
        MAP_CALL(0x83b575u, 0x80ea5bu, 3u);
        MAP_CALL(0x83b579u, 0x8386eau, 2u);
        MAP_CALL(0x83b57cu, 0x838728u, 2u);
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
    if (loaded && checkpoint)
        checkpoint(context, cpu, 0x83b580u);
    return ExecutionReturned(0x83b580u);
}

static void MapResetSectionHeaders(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x30u);
    OpStz(memory, cpu, OpDp(cpu, MAP_SECOND_SECTION_OFFSET));
    OpLdx(cpu, 0u);
    do {
        TransferDirectToA(cpu);
        OpSta(memory, cpu, OpLongX(cpu, 0x7fd000u));
        OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_LAYER_CELL_BASE));
        OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_LAYER_WIDTH));
        OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_LAYER_HEIGHT));
        OpLoadA(cpu, 0xffffu);
        OpSta(memory, cpu, OpLongX(cpu, 0x7fd020u));
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, 8u);
    } while (!cpu->carry);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, 0x7fd038u);
}

Lufia2ExecutionResult Lufia2FieldLoadMapResources(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !cpu->accumulator_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x80eae7u);
    PushAccumulator8(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpSta(memory, cpu, OpDp(cpu, MAP_ID));
    OpStz(memory, cpu, OpDp(cpu, MAP_ID_HIGH));
    MapResetSectionHeaders(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, MAP_ID));
    OpSta(memory, cpu, OpDp(cpu, MAP_TABLE_OFFSET));
    {
        const uint32_t address = OpDp(cpu, MAP_TABLE_OFFSET);
        const uint16_t old = OpReadM(memory, cpu, address);
        const uint16_t value = (uint16_t)(old << 1);
        cpu->carry = (old & 0x8000u) != 0;
        Write8(memory, OpNextByte(address), (uint8_t)(value >> 8));
        Write8(memory, address, (uint8_t)value);
        SetNz16(cpu, value);
    }
    cpu->carry = 0;
    OpAdcValue(cpu, 0x3fu);
    OpSta(memory, cpu, OpDp(cpu, MAP_RESOURCE_ID));
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpDp(cpu, MAP_RESOURCE_DESTINATION), cpu->x);
    OpLoadA(cpu, 0x7fu);
    OpSta(memory, cpu, OpDp(cpu, MAP_RESOURCE_DESTINATION_BANK));
    MAP_CALL(0x80eb34u, 0x808e9du, 3u);
    MAP_CALL(0x80eb38u, 0x80ecf2u, 2u);
    OpLdy(cpu, 0u);
    OpWriteX(memory, cpu, OpDp(cpu, MAP_SECTION_SOURCE), cpu->y);
    MAP_CALL(0x80eb40u, 0x80ebaau, 3u);
    OpStz(memory, cpu, OpDp(cpu, MAP_SECTION_FLAGS));
    OpStz(memory, cpu, OpDp(cpu, MAP_SECTION_FLAGS_HIGH));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, MAP_ID)));
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0xcff90eu));
    if (!cpu->zero) {
        OpAslA(cpu);
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, 0xcff9feu));
        OpSta(memory, cpu, OpDp(cpu, MAP_SECTION_FLAGS));
        OpLda(memory, cpu, OpLongX(cpu, 0xcff9ffu));
        OpSta(memory, cpu, OpDp(cpu, MAP_SECTION_FLAGS_HIGH));
        TransferDirectToA(cpu);
        OpLda(memory, cpu, OpDp(cpu, MAP_SECTION_FLAGS));
        OpAndValue(cpu, 0x0fu);
        if (!cpu->zero) {
            OpRepWidths(cpu, 0x20u);
            cpu->carry = 0;
            OpAdcValue(cpu, 0x162u);
            OpSta(memory, cpu, OpDp(cpu, MAP_RESOURCE_ID));
            OpLda(memory, cpu, OpDp(cpu, MAP_SECOND_SECTION_OFFSET));
            OpSta(memory, cpu, OpDp(cpu, MAP_RESOURCE_DESTINATION));
            OpSepWidths(cpu, 0x20u);
            OpLoadA(cpu, 0x7fu);
            OpSta(memory, cpu, OpDp(cpu, MAP_RESOURCE_DESTINATION_BANK));
            MAP_CALL(0x80eb78u, 0x808e9du, 3u);
            OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, MAP_SECOND_SECTION_OFFSET)));
            OpWriteX(memory, cpu, OpDp(cpu, MAP_SECTION_SOURCE), cpu->y);
            MAP_CALL(0x80eb80u, 0x80ecf2u, 2u);
            MAP_CALL(0x80eb83u, 0x80ebaau, 3u);
        }
    }
    OpRepWidths(cpu, 0x30u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, MAP_ID)));
    OpLda(memory, cpu, OpLongX(cpu, 0xcff72eu));
    OpAndValue(cpu, 0xffu);
    cpu->carry = 0;
    OpAdcValue(cpu, 0x130u);
    MAP_CALL(0x80eb96u, 0x80ec98u, 3u);
    OpSepWidths(cpu, 0x20u);
    MAP_CALL(0x80eb9cu, 0x80ec78u, 3u);
    MAP_CALL(0x80eba0u, 0x80ec18u, 3u);
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    LoadA8(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x80eba9u);
}
