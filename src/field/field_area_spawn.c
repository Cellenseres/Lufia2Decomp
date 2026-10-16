#include "actor/actor_internal.h"
#include "core/cpu_ops.h"
#include "field/field_internal.h"
#include "lufia2/actor.h"
#include "lufia2/field.h"
#include "lufia2/system.h"
#include "system/wram.h"

enum {
    DP_SPAWN_COUNT = 0x00u,
    DP_SPAWN_ID = 0x01u,
    DP_COORDINATE_ORIGIN = 0x54u,
    DP_COORDINATE_CELL_OFFSET = 0x55u,
    DP_COORDINATE_PIXEL_OFFSET = 0x56u,
    DP_AREA_MIN_X = 0x9fu,
    DP_AREA_EXTENT_X = 0xa1u,
    AREA_CELL_PIXELS = 16u
};

static void RandomAreaCoordinate(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    StoreADirect8(memory, cpu, DP_COORDINATE_ORIGIN);
    ExchangeAccumulatorBytes(cpu);
    SimulateJslFrame(memory, cpu, 0x80u, 0xd589u);
    Lufia2RandomScale(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    StoreADirect8(memory, cpu, DP_COORDINATE_CELL_OFFSET);
    LoadA8(cpu, AREA_CELL_PIXELS);
    SimulateJslFrame(memory, cpu, 0x80u, 0xd591u);
    Lufia2RandomScale(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    StoreADirect8(memory, cpu, DP_COORDINATE_PIXEL_OFFSET);
    Write8(memory, DirectAddress(cpu, DP_COORDINATE_PIXEL_OFFSET + 1u), 0x00u);
    TransferDirectToA(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, DP_COORDINATE_ORIGIN));
    cpu->carry = 0;
    Adc8(cpu, DirectByte(memory, cpu, DP_COORDINATE_CELL_OFFSET));
    SetAccumulatorWidth(cpu, 0);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    Add16Value(cpu, Read16Direct(memory, cpu, DP_COORDINATE_PIXEL_OFFSET));
}

static void RandomCoordinateCall(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                 uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    RandomAreaCoordinate(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

void Lufia2FieldSpawnAreaBody(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    unsigned axis;
    TransferDirectToA(cpu);
    for (axis = 0; axis < 2u; ++axis) {
        LoadA8(cpu, DirectByte(memory, cpu, (uint8_t)(DP_AREA_EXTENT_X + axis)));
        cpu->carry = 1;
        Sbc8(cpu, DirectByte(memory, cpu, (uint8_t)(DP_AREA_MIN_X + axis)));
        DecrementA8(cpu);
        StoreADirect8(memory, cpu, (uint8_t)(DP_AREA_EXTENT_X + axis));
    }
    LoadA8(cpu, DirectByte(memory, cpu, DP_ACTOR_SLOT));
    PushAccumulator8(memory, cpu);
    do {
        uint8_t count;

        for (axis = 0; axis < 2u; ++axis) {
            LoadA8(cpu, DirectByte(memory, cpu, (uint8_t)(DP_AREA_EXTENT_X + axis)));
            ExchangeAccumulatorBytes(cpu);
            LoadA8(cpu, DirectByte(memory, cpu, (uint8_t)(DP_AREA_MIN_X + axis)));
            RandomCoordinateCall(memory, cpu, axis ? 0xd55au : 0xd54eu);
            StoreADirect16(memory, cpu, axis ? DP_PROBE_Y : DP_PROBE_X);
            SetAccumulatorWidth(cpu, 1);
        }
        LoadA8(cpu, DirectByte(memory, cpu, DP_SPAWN_ID));
        SimulateJslFrame(memory, cpu, 0x80u, 0xd564u);
        cpu->program_bank = 0x83u;
        Lufia2ActorSpawn(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        cpu->program_bank = 0x80u;
        LoadXDirect16(memory, cpu, DP_SLOT_WORD_OFFSET);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16Direct(memory, cpu, DP_PROBE_X));
        Write16Long(memory, LongIndexedAddress(WRAM_OBJECT_FINE_X, cpu->x),
                    cpu->accumulator);
        LoadA16(cpu, Read16Direct(memory, cpu, DP_PROBE_Y));
        Write16Long(memory, LongIndexedAddress(WRAM_OBJECT_FINE_Y, cpu->x),
                    cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        count = (uint8_t)(DirectByte(memory, cpu, DP_SPAWN_COUNT) - 1u);
        Write8(memory, DirectAddress(cpu, DP_SPAWN_COUNT), count);
        SetNz8(cpu, count);
    } while (!cpu->zero);
    LoadA8(cpu, Pull8(memory, cpu));
    StoreADirect8(memory, cpu, DP_ACTOR_SLOT);
    SimulateJslFrame(memory, cpu, 0x80u, 0xd581u);
    Lufia2ActorRecordOffsets(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

static bool AreaSpawnContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x80u && cpu->accumulator_is_8_bit &&
           !cpu->index_is_8_bit && !cpu->decimal && !cpu->direct_page &&
           cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

Lufia2ExecutionResult Lufia2FieldSpawnActorsInArea(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu) {
    if (!AreaSpawnContext(cpu))
        return ExecutionHandoff(cpu, 0x80d533u);
    Lufia2FieldSpawnAreaBody(memory, cpu);
    return ExecutionReturned(0x80d582u);
}

Lufia2ExecutionResult Lufia2FieldRandomAreaCoordinate(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu) {
    if (!AreaSpawnContext(cpu))
        return ExecutionHandoff(cpu, 0x80d583u);
    RandomAreaCoordinate(memory, cpu);
    return ExecutionReturned(0x80d5a4u);
}
