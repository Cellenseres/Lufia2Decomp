#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/world_map.h"

enum {
    DP_MAP_RESOURCE = 0x54u,
    DP_MAP_DESTINATION = 0x60u,
    DP_MAP_DESTINATION_BANK = 0x62u,
    WORLD_MAP_TILE_RESOURCES = 0xce32u,
    WORLD_MAP_BLOCK_RESOURCES = 0xce34u,
    WORLD_MAP_INITIAL_EXTRA_RESOURCE = 0x0193u
};

static void SelectMapDestination(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t destination) {
    OpLdx(cpu, destination);
    OpWriteX(memory, cpu, OpDp(cpu, DP_MAP_DESTINATION), cpu->x);
    OpLoadA(cpu, 0x7fu);
    OpSta(memory, cpu, OpDp(cpu, DP_MAP_DESTINATION_BANK));
}

static void SelectMapResource(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t table, uint16_t destination) {
    OpLdx(cpu, OpReadX(memory, cpu,
        OpAbs(cpu, WRAM_WORLD_MAP_RESOURCE_TABLE_OFFSET)));
    OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, table)));
    OpWriteX(memory, cpu, OpDp(cpu, DP_MAP_RESOURCE), cpu->y);
    SelectMapDestination(memory, cpu, destination);
}

static Lufia2ExecutionResult MapLoadUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2WorldMapLoadResourceBlocks(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x86u || cpu->data_bank != 0x86u ||
        cpu->direct_page || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x86ccfcu);
    SelectMapResource(memory, cpu, WORLD_MAP_TILE_RESOURCES, 0u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x86cd0du, 0x808e9du, 3u, 0x86u))
        return MapLoadUnwound(0x86cd0du);
    SelectMapResource(memory, cpu, WORLD_MAP_BLOCK_RESOURCES, 0x4000u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x86cd22u, 0x808e9du, 3u, 0x86u))
        return MapLoadUnwound(0x86cd22u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x86cd26u, 0x86ae4du, 2u, 0x86u))
        return MapLoadUnwound(0x86cd26u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_RESOURCE_ID));
    if (!cpu->zero)
        return ExecutionReturned(0x86cd40u);
    OpLdx(cpu, WORLD_MAP_INITIAL_EXTRA_RESOURCE);
    OpWriteX(memory, cpu, OpDp(cpu, DP_MAP_RESOURCE), cpu->x);
    SelectMapDestination(memory, cpu, 0xe000u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x86cd3cu, 0x808e9du, 3u, 0x86u))
        return MapLoadUnwound(0x86cd3cu);
    return ExecutionReturned(0x86cd40u);
}
