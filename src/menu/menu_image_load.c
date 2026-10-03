/* Menu image loaders of bank $86: copy rows of an image into the work
 * buffer at $7E:6000 through the RAM block move, queue the buffer for the
 * video upload, and copy the palette blocks of bank $9F. */

#include "core/cpu_ops.h"
#include "core/cpu_internal.h"
#include "lufia2/menu.h"
#include "lufia2/system.h"

enum {
    ROW_SOURCE = 0x08u,
    ROW_BANK = 0x0au,
    ROW_TARGET = 0x0bu,
    IMAGE_BANK = 0x1bu,
    STAGE_SOURCE = 0x5du,
    STAGE_BANK = 0x5fu,
    STAGE_TARGET = 0x60u,
    STAGE_SIZE = 0x58u,
    MOVE_DESTINATION = 0x057eu,
    MOVE_SOURCE = 0x057fu,
    BUFFER = 0x6000u,
    BUFFER_BANK = 0x7eu
};

/* INC/DEC dp, 16-bit: the high byte is stored first. */
static void StepDirect16(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t offset, int delta) {
    const uint16_t value =
        (uint16_t)(Read16Direct(memory, cpu, offset) + delta);

    Write8(memory, DirectAddress(cpu, (uint8_t)(offset + 1u)),
        (uint8_t)(value >> 8));
    Write8(memory, DirectAddress(cpu, offset), (uint8_t)value);
    SetNz16(cpu, value);
}

/* JSR $057D; a result that is not a plain return is passed on. */
static int MoveBlock(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t last_byte, Lufia2ExecutionResult *result) {
    SimulateJsrFrame(memory, cpu, last_byte);
    *result = Lufia2RamBlockMove(memory, cpu);
    if (result->flow != LUFIA2_EXECUTION_RETURNED)
        return 0;
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* $86:9009: one 256-byte row from the image bank at $1B to $7E:$0B. */
Lufia2ExecutionResult Lufia2MenuCopyImageRow256(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x869009u);
    LoadA8(cpu, BUFFER_BANK);
    StoreAAbsolute8(memory, cpu, MOVE_DESTINATION, 0);
    LoadA8(cpu, DirectByte(memory, cpu, IMAGE_BANK));
    StoreAAbsolute8(memory, cpu, MOVE_SOURCE, 0);
    SetAccumulatorWidth(cpu, 0);
    LoadXDirect16(memory, cpu, ROW_SOURCE);
    LoadYDirect16(memory, cpu, ROW_TARGET);
    LoadA16(cpu, 0x00ffu);
    if (!MoveBlock(memory, cpu, 0x901eu, &result))
        return result;
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x869021u);
}

/* $86:8FF6: two consecutive rows, the second one line further on. */
Lufia2ExecutionResult Lufia2MenuCopyImageBlock(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;

    if (cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x868ff6u);
    SetAccumulatorWidth(cpu, 1);
    PushDataBank(memory, cpu);
    SimulateJsrFrame(memory, cpu, 0x8ffbu);
    result = Lufia2MenuCopyImageRow256(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    SimulateRtsFrame(memory, cpu);
    IncrementDirect8(memory, cpu, 0x09u);
    IncrementDirect8(memory, cpu, 0x0cu);
    IncrementDirect8(memory, cpu, 0x0cu);
    SimulateJsrFrame(memory, cpu, 0x9004u);
    result = Lufia2MenuCopyImageRow256(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    SimulateRtsFrame(memory, cpu);
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    return ExecutionReturned(0x869008u);
}

/* $86:906A: one 128-byte row from bank $0A to $7E:$0B; both positions then
 * move on, the source by $80 and the target by $200. */
Lufia2ExecutionResult Lufia2MenuCopyImageRow128(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;

    if (cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86906au);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, BUFFER_BANK);
    StoreAAbsolute8(memory, cpu, MOVE_DESTINATION, 0);
    LoadA8(cpu, DirectByte(memory, cpu, ROW_BANK));
    StoreAAbsolute8(memory, cpu, MOVE_SOURCE, 0);
    SetAccumulatorWidth(cpu, 0);
    LoadXDirect16(memory, cpu, ROW_SOURCE);
    LoadYDirect16(memory, cpu, ROW_TARGET);
    LoadA16(cpu, 0x007fu);
    if (!MoveBlock(memory, cpu, 0x9081u, &result))
        return result;
    LoadADirect16(memory, cpu, ROW_SOURCE);
    cpu->carry = 0;
    Add16Value(cpu, 0x0080u);
    StoreADirect16(memory, cpu, ROW_SOURCE);
    LoadADirect16(memory, cpu, ROW_TARGET);
    cpu->carry = 0;
    Add16Value(cpu, 0x0200u);
    StoreADirect16(memory, cpu, ROW_TARGET);
    return ExecutionReturned(0x869092u);
}

/* JSR $906A from inside a loader. */
static int CopyRow128(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t last_byte, Lufia2ExecutionResult *result) {
    SimulateJsrFrame(memory, cpu, last_byte);
    *result = Lufia2MenuCopyImageRow128(memory, cpu);
    if (result->flow != LUFIA2_EXECUTION_RETURNED)
        return 0;
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* Stages the buffer for the video upload and waits for it; the wait is
 * left to the original code with the returns of the callers pushed. */
static Lufia2ExecutionResult StageBuffer(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t size,
    uint16_t return_address) {
    LoadX16(cpu, BUFFER);
    StoreXDirect16(memory, cpu, STAGE_SOURCE);
    LoadA8(cpu, BUFFER_BANK);
    StoreADirect8(memory, cpu, STAGE_BANK);
    LoadX16(cpu, 0x3000u);
    StoreXDirect16(memory, cpu, STAGE_TARGET);
    LoadX16(cpu, size);
    StoreXDirect16(memory, cpu, STAGE_SIZE);
    SimulateJslFrame(memory, cpu, 0x86u, return_address);
    return Lufia2MenuQueueVideoWrite(memory, cpu);
}

/* $86:9022: seven image rows of 128 bytes from the table at $86:90AB. */
Lufia2ExecutionResult Lufia2MenuLoadImageGrid(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;

    if (cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x869022u);
    SetAccumulatorWidth(cpu, 0);
    LoadX16(cpu, BUFFER);
    StoreXDirect16(memory, cpu, ROW_TARGET);
    LoadX16(cpu, 0);
    do {
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x8690adu, cpu->x)));
        StoreADirect8(memory, cpu, ROW_BANK);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16Long(memory,
            LongIndexedAddress(0x8690abu, cpu->x)));
        StoreADirect16(memory, cpu, ROW_SOURCE);
        PushIndex(memory, cpu);
        PushDataBank(memory, cpu);
        if (!CopyRow128(memory, cpu, 0x9040u, &result) ||
            !CopyRow128(memory, cpu, 0x9043u, &result))
            return result;
        LoadADirect16(memory, cpu, ROW_TARGET);
        Subtract16(cpu, 0x0380u);
        StoreADirect16(memory, cpu, ROW_TARGET);
        if (!CopyRow128(memory, cpu, 0x904eu, &result) ||
            !CopyRow128(memory, cpu, 0x9051u, &result))
            return result;
        LoadADirect16(memory, cpu, ROW_TARGET);
        Subtract16(cpu, 0x0080u);
        StoreADirect16(memory, cpu, ROW_TARGET);
        PullDataBank(memory, cpu);
        cpu->x = PullIndexValue(memory, cpu);
        cpu->x = (uint16_t)(cpu->x + 3u);
        Compare16(cpu, cpu->x, 0x0015u);
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);
    SimulateJsrFrame(memory, cpu, 0x9068u);
    return StageBuffer(memory, cpu, 0x1c00u, 0x90a9u);
}

/* JSR $8FF6 from inside the image set loader. */
static int CopyBlock(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t last_byte, Lufia2ExecutionResult *result) {
    SimulateJsrFrame(memory, cpu, last_byte);
    *result = Lufia2MenuCopyImageBlock(memory, cpu);
    if (result->flow != LUFIA2_EXECUTION_RETURNED)
        return 0;
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* $86:8F6F: three image blocks per entry of the list at $0A7B, taken from
 * the image table at $8E:E5C2, then the upload of the whole buffer. M8/X16. */
Lufia2ExecutionResult Lufia2MenuLoadImageSet(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x868f6fu);
    LoadX16(cpu, BUFFER);
    StoreXDirect16(memory, cpu, 0x1cu);
    Write8(memory, DirectAddress(cpu, 0x04u), 0);
    Write8(memory, DirectAddress(cpu, 0x05u), 0);
    do {
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, 0x00u);
        ExchangeAccumulatorBytes(cpu);
        LoadXDirect16(memory, cpu, 0x04u);
        LoadA8(cpu, AbsoluteByte(memory, cpu, 0x0a7bu, cpu->x));
        AslA8(cpu);
        cpu->carry = 0;
        Adc8(cpu, AbsoluteByte(memory, cpu, 0x0a7bu, cpu->x));
        TransferAToX(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x8ee5c4u, cpu->x)));
        StoreADirect8(memory, cpu, IMAGE_BANK);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16Long(memory,
            LongIndexedAddress(0x8ee5c2u, cpu->x)));
        cpu->carry = 0;
        Add16Value(cpu, 0x0200u);
        StoreADirect16(memory, cpu, 0x19u);
        LoadADirect16(memory, cpu, 0x19u);
        StoreADirect16(memory, cpu, ROW_SOURCE);
        LoadADirect16(memory, cpu, 0x1cu);
        StoreADirect16(memory, cpu, ROW_TARGET);
        if (!CopyBlock(memory, cpu, 0x8fa4u, &result))
            return result;
        LoadADirect16(memory, cpu, 0x19u);
        cpu->carry = 0;
        Add16Value(cpu, 0x0400u);
        StoreADirect16(memory, cpu, ROW_SOURCE);
        LoadADirect16(memory, cpu, 0x1cu);
        cpu->carry = 0;
        Add16Value(cpu, 0x0100u);
        StoreADirect16(memory, cpu, ROW_TARGET);
        if (!CopyBlock(memory, cpu, 0x8fb7u, &result))
            return result;
        LoadADirect16(memory, cpu, 0x19u);
        cpu->carry = 0;
        Add16Value(cpu, 0x0a00u);
        StoreADirect16(memory, cpu, ROW_SOURCE);
        LoadADirect16(memory, cpu, 0x1cu);
        cpu->carry = 0;
        Add16Value(cpu, 0x0400u);
        StoreADirect16(memory, cpu, ROW_TARGET);
        if (!CopyBlock(memory, cpu, 0x8fcau, &result))
            return result;
        LoadADirect16(memory, cpu, 0x1cu);
        cpu->carry = 0;
        Add16Value(cpu, 0x0800u);
        StoreADirect16(memory, cpu, 0x1cu);
        SetAccumulatorWidth(cpu, 1);
        IncrementDirect8(memory, cpu, 0x04u);
        LoadA8(cpu, DirectByte(memory, cpu, 0x04u));
        Compare8(cpu, A8(cpu), AbsoluteByte(memory, cpu, 0x0a7au, 0));
    } while (!cpu->zero);
    return StageBuffer(memory, cpu, 0x2000u, 0x8ff4u);
}

/* The palette blocks of bank $9F copied to $0320 and up. Each one runs
 * MVN with the destination bank $00. */
static Lufia2ExecutionResult CopyPalette(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t source,
    uint16_t target,
    uint16_t count,
    uint32_t return_address) {
    SetAccumulatorWidth(cpu, 0);
    PushDataBank(memory, cpu);
    LoadX16(cpu, source);
    LoadY16(cpu, target);
    LoadA16(cpu, count);
    OpMoveNext(memory, cpu, 0x00u, 0x9fu);
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(return_address);
}

#define PALETTE(name, entry, source, target, count)                           \
    Lufia2ExecutionResult name(const Lufia2Memory *memory,                    \
        Lufia2CpuState *cpu) {                                                \
        if (cpu->index_is_8_bit)                                              \
            return ExecutionHandoff(cpu, entry);                              \
        return CopyPalette(memory, cpu, source, target, count, (entry) + 0x12u); \
    }

PALETTE(Lufia2MenuLoadPalette0, 0x8690c0u, 0x8200u, 0x0320u, 0x003fu)
PALETTE(Lufia2MenuLoadPalette1, 0x8690d3u, 0x8420u, 0x0340u, 0x001fu)
PALETTE(Lufia2MenuLoadPalette2, 0x8690e6u, 0x8040u, 0x0360u, 0x00bfu)
PALETTE(Lufia2MenuLoadPalette3, 0x8690f9u, 0x8100u, 0x0420u, 0x007fu)
PALETTE(Lufia2MenuLoadPalette4, 0x86910cu, 0x8140u, 0x04c0u, 0x001fu)

/* $86:911F: one 32-byte block of bank $97 per entry of the list at $0A7B,
 * chosen by the class table at $86:9165, stored from $04A0 on. X16. */
Lufia2ExecutionResult Lufia2MenuLoadSlotPalettes(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86911fu);
    SetAccumulatorWidth(cpu, 0);
    LoadX16(cpu, 0x04a0u);
    StoreXDirect16(memory, cpu, 0x02u);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a7au, 0));
    And16(cpu, 0x00ffu);
    StoreADirect16(memory, cpu, 0x15u);
    Write16Direct(memory, cpu, 0x04u, 0);
    do {
        LoadXDirect16(memory, cpu, 0x04u);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a7bu, cpu->x));
        And16(cpu, 0x00ffu);
        TransferAToX(cpu);
        PushDataBank(memory, cpu);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x869165u, cpu->x)));
        And16(cpu, 0x00ffu);
        AslA16(cpu);
        AslA16(cpu);
        AslA16(cpu);
        AslA16(cpu);
        AslA16(cpu);
        cpu->carry = 0;
        Add16Value(cpu, 0xccf8u);
        TransferAToX(cpu);
        LoadYDirect16(memory, cpu, 0x02u);
        LoadA16(cpu, 0x001fu);
        OpMoveNext(memory, cpu, 0x00u, 0x97u);
        PullDataBank(memory, cpu);
        LoadADirect16(memory, cpu, 0x02u);
        cpu->carry = 0;
        Add16Value(cpu, 0x0020u);
        StoreADirect16(memory, cpu, 0x02u);
        StepDirect16(memory, cpu, 0x04u, 1);
        StepDirect16(memory, cpu, 0x15u, -1);
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x869164u);
}

/* $82:8044: sets up the video transfer of $58 bytes from the long address
 * $5D to the VRAM address $60 and waits for the frame that performs it.
 * The wait is left to the original code. M8/X16, JSL. */
Lufia2ExecutionResult Lufia2MenuQueueVideoWrite(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x828044u);
    StoreAImmediate8(memory, cpu, 0x01u, 0x4300u);
    LoadXDirect16(memory, cpu, STAGE_SOURCE);
    Write16Absolute(memory, cpu, 0x4302u, cpu->x);
    LoadA8(cpu, DirectByte(memory, cpu, STAGE_BANK));
    StoreAAbsolute8(memory, cpu, 0x4304u, 0);
    LoadXDirect16(memory, cpu, STAGE_SIZE);
    Write16Absolute(memory, cpu, 0x4305u, cpu->x);
    StoreAImmediate8(memory, cpu, 0x18u, 0x4301u);
    LoadXDirect16(memory, cpu, STAGE_TARGET);
    StoreXDirect16(memory, cpu, 0x79u);
    LoadA8(cpu, 0x41u);
    StoreADirect8(memory, cpu, 0x75u);
    SimulateJsrFrame(memory, cpu, 0x8067u);
    cpu->program_bank = 0x82u;
    return ExecutionHandoff(cpu, 0x8293c2u);
}
