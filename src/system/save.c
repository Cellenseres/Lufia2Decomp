/* Original save-file loading, packing and byte-stream encryption. */

#include "core/cpu_ops.h"
#include "lufia2/system.h"
#include "system/wram.h"

enum {
    SAVE_SELECTED_FILE = 0x65u,
    SAVE_FILE_INDEX = 0x54u,
    SAVE_FILE_POINTER = 0x60u,
    SAVE_FILE_POINTER_BANK = 0x62u,
    SAVE_CHECKSUM_POINTER = 0x5du,
    SAVE_CHECKSUM_POINTER_BANK = 0x5fu,
    SAVE_CHECKSUM = 0x56u,
};

static void SavePushA(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit)
        PushAccumulator8(memory, cpu);
    else
        PushAccumulator16(memory, cpu);
}

static void SavePullA(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit)
        LoadA8(cpu, Pull8(memory, cpu));
    else
        PullAccumulator16(memory, cpu);
}

static Lufia2ExecutionResult SaveUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static uint8_t SaveChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target, uint8_t frame) {
    if (frame == 3u)
        SimulateJslFrame(memory, cpu, cpu->program_bank, (uint16_t)(site + 3u));
    else
        SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    return child(context, cpu, target, site, frame);
}

#define SAVE_CALL(site, target, frame) \
    do { \
        if (!SaveChild(memory, cpu, child, context, site, target, frame)) \
            return SaveUnwound(site); \
    } while (0)

Lufia2ExecutionResult Lufia2LoadGameFile(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, Lufia2ExecutionCheckpoint checkpoint,
    void *context) {
    if (!child || cpu->decimal)
        return ExecutionHandoff(cpu, 0x809099u);
    if (checkpoint)
        checkpoint(context, cpu, 0x809099u);
    cpu->carry = 0;
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    OpRepWidths(cpu, 0x30u);
    SavePushA(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpDp(cpu, SAVE_SELECTED_FILE));
    SAVE_CALL(0x8090a5u, 0x80914bu, 3u);
    OpLda(memory, cpu, WRAM_SAVE_FILE_BUFFER);
    OpAndValue(cpu, 0xf0u);
    if (cpu->zero) {
        SAVE_CALL(0x8090b1u, 0x8eb993u, 3u);
        SAVE_CALL(0x8090b5u, 0x85c60eu, 3u);
    } else {
        OpLda(memory, cpu, OpStack(cpu, 8u));
        OpOraValue(cpu, 1u);
        OpSta(memory, cpu, OpStack(cpu, 8u));
    }
    OpRepWidths(cpu, 0x30u);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    SavePullA(memory, cpu);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x8090c8u);
}

Lufia2ExecutionResult Lufia2SaveGameFile(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, Lufia2ExecutionCheckpoint checkpoint,
    void *context) {
    if (!child || cpu->decimal)
        return ExecutionHandoff(cpu, 0x8090c9u);
    if (checkpoint)
        checkpoint(context, cpu, 0x8090c9u);
    SavePushA(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    SavePushA(memory, cpu);
    SAVE_CALL(0x8090d3u, 0x8eba0du, 3u);
    SAVE_CALL(0x8090d7u, 0x85c954u, 3u);
    SavePullA(memory, cpu);
    SAVE_CALL(0x8090dcu, 0x809184u, 2u);
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    SavePullA(memory, cpu);
    return ExecutionReturned(0x8090e4u);
}

Lufia2ExecutionResult Lufia2ResolveSaveFileAddress(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    (void)memory;
    ExchangeAccumulatorBytes(cpu);
    OpLoadA(cpu, 0u);
    OpRepWidths(cpu, 0x20u);
    OpAslA(cpu);
    OpAslA(cpu);
    OpAslA(cpu);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x8091deu);
}

Lufia2ExecutionResult Lufia2SaveFileChecksum(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->decimal || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x8090fcu);
    SAVE_CALL(0x8090fcu, 0x8091d3u, 2u);
    OpLoadA(cpu, 0x70u);
    OpSta(memory, cpu, OpDp(cpu, SAVE_CHECKSUM_POINTER_BANK));
    OpRepWidths(cpu, 0x20u);
    OpWriteX(memory, cpu, OpDp(cpu, SAVE_CHECKSUM_POINTER), cpu->x);
    OpLdy(cpu, 4u);
    OpLoadA(cpu, 0x6502u);
    cpu->carry = 0;
    do {
        OpAdc(memory, cpu, DirectLongIndirectY(memory, cpu, SAVE_CHECKSUM_POINTER));
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, 0x800u);
    } while (!cpu->zero);
    OpSta(memory, cpu, OpDp(cpu, SAVE_CHECKSUM));
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x80911bu);
}

Lufia2ExecutionResult Lufia2ReadGameFile(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, Lufia2ExecutionCheckpoint checkpoint,
    void *context) {
    if (!child || cpu->decimal || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x80914bu);
    if (checkpoint)
        checkpoint(context, cpu, 0x80914bu);
    OpSta(memory, cpu, OpDp(cpu, SAVE_FILE_INDEX));
    SAVE_CALL(0x80914du, 0x8091d3u, 2u);
    OpWriteX(memory, cpu, OpDp(cpu, SAVE_FILE_POINTER), cpu->x);
    OpLoadA(cpu, 0x70u);
    OpSta(memory, cpu, OpDp(cpu, SAVE_FILE_POINTER_BANK));
    OpLdy(cpu, 1u);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, SAVE_FILE_POINTER));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_RANDOM_SEED_WORK));
    SAVE_CALL(0x80915eu, 0x8082e7u, 3u);
    OpLdx(cpu, 4u);
    OpTxy(cpu);
    do {
        OpLoadA(cpu, 0xffu);
        SAVE_CALL(0x809168u, 0x808299u, 3u);
        cpu->carry = 1;
        OpSbcValue(cpu, OpReadM(memory, cpu,
            DirectLongIndirectY(memory, cpu, SAVE_FILE_POINTER)));
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
        OpIncA(cpu);
        OpSta(memory, cpu, OpLongX(cpu, WRAM_SAVE_FILE_BUFFER));
        OpInx(cpu);
        OpIny(cpu);
        OpCpy(cpu, 0x800u);
    } while (!cpu->zero);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, SAVE_FILE_POINTER));
    OpSta(memory, cpu, WRAM_SAVE_FILE_BUFFER);
    return ExecutionReturned(0x809183u);
}

Lufia2ExecutionResult Lufia2WriteGameFile(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->decimal || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x809184u);
    OpSta(memory, cpu, OpDp(cpu, SAVE_FILE_INDEX));
    SAVE_CALL(0x809186u, 0x8091d3u, 2u);
    OpWriteX(memory, cpu, OpDp(cpu, SAVE_FILE_POINTER), cpu->x);
    OpLoadA(cpu, 0x70u);
    OpSta(memory, cpu, OpDp(cpu, SAVE_FILE_POINTER_BANK));
    OpLdy(cpu, 1u);
    SAVE_CALL(0x809192u, 0x808299u, 3u);
    OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, SAVE_FILE_POINTER));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_RANDOM_SEED_WORK));
    SAVE_CALL(0x80919bu, 0x8082e7u, 3u);
    OpLdx(cpu, 4u);
    OpTxy(cpu);
    do {
        OpLoadA(cpu, 0xffu);
        SAVE_CALL(0x8091a5u, 0x808299u, 3u);
        cpu->carry = 0;
        OpAdc(memory, cpu, OpLongX(cpu, WRAM_SAVE_FILE_BUFFER));
        OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, SAVE_FILE_POINTER));
        OpInx(cpu);
        OpIny(cpu);
        OpCpy(cpu, 0x800u);
    } while (!cpu->zero);
    OpLda(memory, cpu, OpDp(cpu, SAVE_FILE_INDEX));
    SAVE_CALL(0x8091b9u, 0x8090fcu, 2u);
    OpRepWidths(cpu, 0x20u);
    OpLdy(cpu, 2u);
    OpLda(memory, cpu, OpDp(cpu, SAVE_CHECKSUM));
    OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, SAVE_FILE_POINTER));
    OpSepWidths(cpu, 0x20u);
    OpLdy(cpu, 0u);
    OpLda(memory, cpu, 0x700000u);
    OpAndValue(cpu, 0x0fu);
    OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, SAVE_FILE_POINTER));
    return ExecutionReturned(0x8091d2u);
}

