/* World map objects: the sprite buffer reset and the on-screen test that
 * builds the list of visible objects. */

#include <stdbool.h>

#include "core/cpu_internal.h"
#include "core/wram_view.h"
#include "lufia2/world_map.h"

/* Object record fields, reached through X (direct page indexed). */
enum {
    OBJECT_X = 0x02u,
    OBJECT_Y = 0x05u,
    OBJECT_SCREEN_X = 0x07u,
    OBJECT_SCREEN_Y = 0x09u,
    OBJECT_WIDTH = 0x0bu,
    OBJECT_HEIGHT = 0x0du,
    OBJECT_SIZE = 0x1du
};

/* Direct page: the camera corner subtracted from object positions, and the
 * count of objects left to test. */
enum {
    CAMERA_X = 0x58u,
    CAMERA_Y = 0x5au,
    OBJECTS_LEFT = 0x22u,
    SCREEN_WIDTH = 0x0120u,
    SCREEN_HEIGHT = 0x0100u,
    SPRITE_MARGIN = 0x0010u
};

/* Absolute work RAM (data bank): visible object list. */
enum {
    VISIBLE_COUNT = 0x124bu,
    VISIBLE_KEY = 0x0000u,   /* per entry, from Y: sort key */
    VISIBLE_OBJECT = 0x002cu /* per entry, from Y: object offset */
};

/* Object slot tables and the hardware sprite buffer. */
enum {
    SLOT_TABLE = 0x12a5u,
    SLOT_COUNT = 0x20u,
    SLOT_FLAGS = 0x40u,
    OAM_BUFFER = 0x0100u,
    OAM_ROWS = 0x10u,
    OAM_ROW_SIZE = 0x20u,
    OAM_HIDDEN_Y = 0xe0u,
    OAM_HIGH_TABLE_WORDS = 0x10u
};

static uint32_t Absolute(
    const Lufia2Wram wram, uint16_t offset, uint16_t index) {
    return (((uint32_t)wram.data_bank << 16) + offset + index) & 0x00ffffffu;
}

static void WriteAbsolute16(
    const Lufia2Wram wram, uint16_t offset, uint16_t index, uint16_t value) {
    const uint32_t low = Absolute(wram, offset, index);

    Write8(wram.memory, low, (uint8_t)value);
    Write8(wram.memory, (low + 1u) & 0x00ffffffu, (uint8_t)(value >> 8));
}

/* Adds to a direct-page word the way a read-modify-write does. */
static uint16_t StepDirect16(Lufia2Wram wram, uint32_t location, int delta) {
    return WramStep16(wram, location, delta);
}

/* $86:E295: appends object X to the visible list at Y when its box overlaps
 * the screen, storing its screen position. M0X0. */
static void TestObjectVisible(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram) {
    const uint16_t object = cpu->x;
    uint32_t count;
    uint16_t value;

    LoadA16(cpu, WramRead16At(wram, OBJECT_Y, object));
    if (cpu->negative)
        return;
    Subtract16(cpu, WramRead16(wram, CAMERA_Y));
    cpu->carry = false;
    Add16Value(cpu, WramRead16At(wram, OBJECT_HEIGHT, object));
    Compare16(cpu, cpu->accumulator, SCREEN_HEIGHT);
    if (cpu->carry)
        return;
    LoadA16(cpu, WramRead16At(wram, OBJECT_X, object));
    cpu->carry = false;
    Add16Value(cpu, SPRITE_MARGIN);
    Subtract16(cpu, WramRead16(wram, CAMERA_X));
    cpu->carry = false;
    Add16Value(cpu, WramRead16At(wram, OBJECT_WIDTH, object));
    Compare16(cpu, cpu->accumulator, SCREEN_WIDTH);
    if (cpu->carry)
        return;
    LoadA16(cpu, WramRead16At(wram, OBJECT_X, object));
    Subtract16(cpu, WramRead16(wram, CAMERA_X));
    WramWrite16At(wram, OBJECT_SCREEN_X, object, cpu->accumulator);
    LoadA16(cpu, WramRead16At(wram, OBJECT_Y, object));
    Subtract16(cpu, WramRead16(wram, CAMERA_Y));
    WramWrite16At(wram, OBJECT_SCREEN_Y, object, cpu->accumulator);
    LoadA16(cpu, WramRead16At(wram, OBJECT_Y, object));
    WriteAbsolute16(wram, VISIBLE_KEY, cpu->y, cpu->accumulator);
    LoadA16(cpu, object);
    WriteAbsolute16(wram, VISIBLE_OBJECT, cpu->y, cpu->accumulator);
    cpu->y = (uint16_t)(cpu->y + 2u);
    SetNz16(cpu, cpu->y);
    count = Absolute(wram, VISIBLE_COUNT, 0);
    value = (uint16_t)(Read8(memory, count) |
                       ((uint16_t)Read8(memory, (count + 1u) & 0x00ffffffu) << 8));
    value = (uint16_t)(value + 1u);
    Write8(memory, (count + 1u) & 0x00ffffffu, (uint8_t)(value >> 8));
    Write8(memory, count, (uint8_t)value);
    SetNz16(cpu, value);
}

Lufia2ExecutionResult Lufia2WorldMapTestObject(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86e295u);
    TestObjectVisible(memory, cpu, WramViewOfCaller(memory, cpu));
    return ExecutionReturned(0x86e2d1u);
}

/* $86:E287: tests every object, X stepping through the table. The first
 * object is the caller's X; the count is in $22. */
Lufia2ExecutionResult Lufia2WorldMapTestObjects(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;

    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86e287u);
    wram = WramViewOfCaller(memory, cpu);
    do {
        SimulateJsrFrame(memory, cpu, 0xe289u);
        TestObjectVisible(memory, cpu, wram);
        SimulateRtsFrame(memory, cpu);
        TransferXToA(cpu);
        cpu->carry = false;
        Add16Value(cpu, OBJECT_SIZE);
        TransferAToX(cpu);
        SetNz16(cpu, StepDirect16(wram, OBJECTS_LEFT, -1));
    } while (!cpu->zero);
    return ExecutionReturned(0x86e294u);
}

/* $86:E640: clears the flag words of the 32 object slots. M1X0. */
Lufia2ExecutionResult Lufia2WorldMapClearSlotFlags(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;
    uint8_t left;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86e640u);
    wram = WramViewOfCaller(memory, cpu);
    cpu->x = SLOT_TABLE;
    LoadA8(cpu, SLOT_COUNT);
    LoadY16(cpu, 0);
    for (left = SLOT_COUNT; left != 0; --left) {
        WramWrite16At(wram, SLOT_FLAGS, cpu->x, cpu->y);
        cpu->x = (uint16_t)(cpu->x + 2u);
        SetNz16(cpu, cpu->x);
        DecrementA8(cpu);
    }
    return ExecutionReturned(0x86e64fu);
}

/* $86:E650: hides all 128 hardware sprites (Y = $E0) and clears the high
 * table of the OAM buffer. Any entry widths; they are restored. */
Lufia2ExecutionResult Lufia2WorldMapClearSprites(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const uint8_t kYOffsets[8] = {0x01u, 0x05u, 0x09u, 0x0du,
                                         0x11u, 0x15u, 0x19u, 0x1du};
    Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    unsigned row;
    unsigned i;

    Push8(memory, cpu, PackStatus(cpu));
    SetIndexWidth(cpu, 0);
    cpu->x = OAM_BUFFER;
    cpu->y = OAM_ROWS;
    cpu->carry = false;
    for (row = 0; row < OAM_ROWS; ++row) {
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, OAM_HIDDEN_Y);
        for (i = 0; i < 8u; ++i)
            WramWriteAt(wram, kYOffsets[i], cpu->x, A8(cpu));
        SetAccumulatorWidth(cpu, 0);
        TransferXToA(cpu);
        Add16Value(cpu, OAM_ROW_SIZE);
        TransferAToX(cpu);
        cpu->y = (uint16_t)(cpu->y - 1u);
        SetNz16(cpu, cpu->y);
    }
    cpu->y = OAM_HIGH_TABLE_WORDS;
    SetNz16(cpu, cpu->y);
    for (i = 0; i < OAM_HIGH_TABLE_WORDS; ++i) {
        WramWrite16At(wram, 0, cpu->x, 0);
        cpu->x = (uint16_t)(cpu->x + 2u);
        SetNz16(cpu, cpu->x);
        cpu->y = (uint16_t)(cpu->y - 1u);
        SetNz16(cpu, cpu->y);
    }
    SetAccumulatorWidth(cpu, 1);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x86e685u);
}
