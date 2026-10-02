/* Map headers and their initial field-event state. */

#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "field/event_script_internal.h"
#include "system/wram.h"

enum {
    MAP_HEADER_POINTER = 0x5d,
    MAP_HEADER_POINTER_BANK = 0x5f,
    MAP_HEADER_RECORD_SIZE = 10,
    EVENT_CALL_RECORD_SIZE = 4,
    EVENT_MAP_PARAMETER = 0x7fd1a0,
    EVENT_MAP_PARAMETER_TABLE = 0x91b8b5,
    EVENT_SCRIPT_OFFSET_TABLE = 0x8f8000,
    EVENT_SCRIPT_OFFSET_BANK = 0x8f8002,
    EVENT_SCRIPT_OFFSET_NONE = 0xffff,
    EVENT_SCRIPT_OFFSET_WINDOW = 0x8000,
    EVENT_SCRIPT_BANK_ORIGIN = 0x8f,
};

/* $80:E844: clears the event call records, loads the map's parameter word,
 * and points the script base at the map's script block. A map whose offset
 * entry is $FFFF has no script block: the base keeps the $FFFF offset and the
 * accumulator's low byte as bank, as the original does. Registers on return
 * (A holds the bank, X the map's offset entry index) are part of the contract. */
Lufia2ExecutionResult Lufia2FieldInitializeMapEvents(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, (WRAM_FIELD_EVENT_CALL_RECORDS_COUNT - 1u) * EVENT_CALL_RECORD_SIZE);
    TransferDirectToA(cpu);
    do {
        OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_EVENT_CALL_RECORDS));
        OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_EVENT_CALL_RECORDS + 2u));
        for (unsigned i = 0; i < EVENT_CALL_RECORD_SIZE; ++i)
            OpDex(cpu);
    } while (!cpu->negative);

    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID));
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, EVENT_MAP_PARAMETER_TABLE));
    OpSta(memory, cpu, EVENT_MAP_PARAMETER);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID));
    OpAslA(cpu);
    OpAdc(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID));
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, EVENT_SCRIPT_OFFSET_TABLE));
    OpCmpValue(cpu, EVENT_SCRIPT_OFFSET_NONE);
    if (cpu->zero) {
        OpSta(memory, cpu, EVENT_SCRIPT_BASE);
        OpSepWidths(cpu, 0x20u);
        OpSta(memory, cpu, EVENT_SCRIPT_BASE_BANK);
    } else {
        cpu->carry = 0;
        OpAdcValue(cpu, EVENT_SCRIPT_OFFSET_WINDOW);
        OpSta(memory, cpu, EVENT_SCRIPT_BASE);
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpLongX(cpu, EVENT_SCRIPT_OFFSET_BANK));
        cpu->carry = 0;
        OpAdcValue(cpu, EVENT_SCRIPT_BANK_ORIGIN);
        OpSta(memory, cpu, EVENT_SCRIPT_BASE_BANK);
    }
    return ExecutionReturned(0x80e897u);
}

static Lufia2ExecutionResult MapHeaderChildUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static void MapHeaderSource(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_FLAGS));
    OpBitValue(cpu, 0x0001u);
    if (!cpu->zero) {
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, WRAM_CAVE_MAP_HEADER_BANK);
        OpSta(memory, cpu, OpAbs(cpu, 0x057fu));
        OpSta(memory, cpu, OpDp(cpu, MAP_HEADER_POINTER_BANK));
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, WRAM_CAVE_MAP_HEADER_POINTER);
    } else {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID));
        OpAslA(cpu);
        OpAdc(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID));
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpLongX(cpu, 0xcffcbeu));
        OpSta(memory, cpu, OpAbs(cpu, 0x057fu));
        OpSta(memory, cpu, OpDp(cpu, MAP_HEADER_POINTER_BANK));
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpLongX(cpu, 0xcffcbcu));
    }
    OpSta(memory, cpu, OpDp(cpu, MAP_HEADER_POINTER));
    OpTax(cpu);
    OpLdy(cpu, 0xf000u);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, MAP_HEADER_POINTER));
}

static uint8_t MapHeaderPublishObjectAttributes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, 0xf017u));
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpAbs(cpu, 0xf016u));
    OpTay(cpu);
    unsigned records = 0;
    for (;;) {
        if (records++ == 0x10000u) {
            cpu->resume_pc = 0x83b635u;
            return 0;
        }
        TransferDirectToA(cpu);
        OpLda(memory, cpu, OpAbsY(cpu, 0xf000u));
        OpCmpValue(cpu, 0x00ffu);
        if (cpu->zero)
            return 1;
        OpTax(cpu);
        OpLda(memory, cpu, OpAbsY(cpu, 0xf007u));
        OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_MAP_RECORD_BYTE7));
        OpLda(memory, cpu, OpAbsY(cpu, 0xf008u));
        OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_MAP_RECORD_BYTE8));
        OpInx(cpu);
        OpRepWidths(cpu, 0x20u);
        OpTya(cpu);
        cpu->carry = 0;
        OpAdcValue(cpu, MAP_HEADER_RECORD_SIZE);
        OpTay(cpu);
        OpSepWidths(cpu, 0x20u);
    }
}

Lufia2ExecutionResult Lufia2FieldLoadMapHeader(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    OpLoadA(cpu, 0x007eu);
    OpSta(memory, cpu, OpAbs(cpu, 0x057eu));
    OpRepWidths(cpu, 0x30u);
    MapHeaderSource(memory, cpu);
    SimulateJsrFrame(memory, cpu, 0xb61au);
    if (!child(context, cpu, 0x83057du, 0x83b618u, 2u))
        return MapHeaderChildUnwound(0x83b618u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_FLAGS));
    OpBitValue(cpu, 0x0001u);
    if (!cpu->zero) {
        SimulateJslFrame(memory, cpu, 0x83u, 0xb626u);
        if (!child(context, cpu, 0x8eb847u, 0x83b623u, 3u))
            return MapHeaderChildUnwound(0x83b623u);
    }
    OpSepWidths(cpu, 0x20u);
    OpSetDataBank(memory, cpu, 0x7eu);
    if (!MapHeaderPublishObjectAttributes(memory, cpu))
        return ExecutionHandoff(cpu, 0x83b635u);
    SimulateJslFrame(memory, cpu, 0x83u, 0xb65cu);
    if (!child(context, cpu, 0x80e844u, 0x83b659u, 3u))
        return MapHeaderChildUnwound(0x83b659u);
    OpLdx(cpu, 0x0000u);
    OpLdy(cpu, 0x001eu);
    SimulateJslFrame(memory, cpu, 0x83u, 0xb666u);
    if (!child(context, cpu, 0x80e722u, 0x83b663u, 3u))
        return MapHeaderChildUnwound(0x83b663u);
    SimulateJslFrame(memory, cpu, 0x83u, 0xb66au);
    if (!child(context, cpu, 0x80cbaeu, 0x83b667u, 3u))
        return MapHeaderChildUnwound(0x83b667u);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x83b66du);
}
