#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/world_map.h"

enum {
    DP_BLOCK_DESTINATION = 0x08u,
    DP_BLOCK_SOURCE = 0x0bu,
    DP_BLOCK_X = 0x58u,
    DP_BLOCK_Y = 0x5au,
    WORLD_RESOURCE = 0x09eau,
    WORLD_BLOCK_FLAGS = 0xae97u,
    WORLD_BLOCK_SOURCE_COORDINATES = 0xae98u,
    WORLD_BLOCK_DESTINATION_COORDINATES = 0xae9au,
    WORLD_BLOCK_RECORD_BYTES = 5u
};

static Lufia2ExecutionResult BlockCopyUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static void ReadBlockCoordinates(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t table) {
    OpLda(memory, cpu, OpAbsY(cpu, table));
    OpSta(memory, cpu, OpDp(cpu, DP_BLOCK_X));
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_BLOCK_Y));
}

static void CopyFlaggedBlock(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSepWidths(cpu, 0x20u);
    Push8(memory, cpu, cpu->data_bank);
    OpLoadA(cpu, 0x7fu);
    PushAccumulator8(memory, cpu);
    cpu->data_bank = Pull8(memory, cpu);
    SetNz8(cpu, cpu->data_bank);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu,
        OpRead16(memory, OpDp(cpu, DP_BLOCK_SOURCE))));
    OpSta(memory, cpu, OpAbs(cpu,
        OpRead16(memory, OpDp(cpu, DP_BLOCK_DESTINATION))));
    cpu->data_bank = Pull8(memory, cpu);
    SetNz8(cpu, cpu->data_bank);
}

Lufia2ExecutionResult Lufia2WorldMapCopyFlaggedBlocks(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x86u || cpu->data_bank != 0x86u ||
        cpu->direct_page || cpu->index_is_8_bit || cpu->decimal ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x86ae4du);
    OpLda(memory, cpu, OpAbs(cpu, WORLD_RESOURCE));
    if (!cpu->zero)
        return ExecutionReturned(0x86ae96u);
    OpLdy(cpu, 0u);
    for (;;) {
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbsY(cpu, WORLD_BLOCK_FLAGS));
        if (cpu->zero)
            return ExecutionReturned(0x86ae96u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x86ae5cu, 0x80be1au, 3u, 0x86u))
            return BlockCopyUnwound(0x86ae5cu);
        OpRepWidths(cpu, 0x20u);
        if (!cpu->zero) {
            ReadBlockCoordinates(memory, cpu, WORLD_BLOCK_SOURCE_COORDINATES);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x86ae6cu, 0x86ae05u, 2u, 0x86u))
                return BlockCopyUnwound(0x86ae6cu);
            cpu->carry = 0u;
            OpAdcValue(cpu, 0x8000u);
            OpSta(memory, cpu, OpDp(cpu, DP_BLOCK_SOURCE));
            ReadBlockCoordinates(memory, cpu, WORLD_BLOCK_DESTINATION_COORDINATES);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x86ae7du, 0x86ae05u, 2u, 0x86u))
                return BlockCopyUnwound(0x86ae7du);
            CopyFlaggedBlock(memory, cpu);
        }
        OpTya(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, WORLD_BLOCK_RECORD_BYTES);
        OpTay(cpu);
    }
}
