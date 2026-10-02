/* World map objects: animation steps, the sprite buffer reset and the
 * on-screen test that builds the list of visible objects. */

#include <stdbool.h>

#include "core/cpu_internal.h"
#include "core/snes_registers.h"
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

/* Animation record fields of an object, and the layout of an animation frame
 * in ROM (a pointer to a table of frame records, indexed by pose). */
enum {
    OBJECT_POSE = 0x0fu,
    OBJECT_ANIMATION = 0x10u,
    OBJECT_FRAME = 0x11u,
    OBJECT_FRAME_COUNT = 0x14u,
    OBJECT_FRAME_END = 0x13u,
    OBJECT_FRAME_RESTART = 0x12u,
    OBJECT_SPRITE_FLAGS = 0x12u,
    OBJECT_TIMER = 0x15u,
    OBJECT_ANIMATION_FLAGS = 0x18u,
    OBJECT_STEP_X = 0x0bu,
    OBJECT_STEP_X_HIGH = 0x0cu,
    OBJECT_STEP_Y = 0x0du,
    OBJECT_STEP_Y_HIGH = 0x0eu,
    OBJECT_FRAME_POINTER = 0x19u,
    OBJECT_TILE_BASE = 0x17u,
    ANIMATION_POINTERS = 0xeba7u,
    FRAME_POINTER_LOCATION = 0x00u,
    FRAME_SIZE = 5u,
    OBJECT_COUNT = 0x16u,
    OBJECT_TABLE = 0x1469u
};

static uint16_t ReadAbsolute16(
    const Lufia2Wram wram, uint16_t offset, uint16_t index) {
    const uint32_t low = Absolute(wram, offset, index);

    return (uint16_t)(Read8(wram.memory, low) |
        ((uint16_t)Read8(wram.memory, (low + 1u) & 0x00ffffffu) << 8));
}

/* Stores a step byte and its sign extension (A is left $FF for a negative
 * step). */
static void StoreStep(
    Lufia2Wram wram, Lufia2CpuState *cpu, uint16_t object, uint8_t low,
    uint8_t high) {
    WramWriteAt(wram, low, object, A8(cpu));
    if (cpu->negative) {
        LoadA8(cpu, 0xffu);
        WramWriteAt(wram, high, object, A8(cpu));
    } else {
        WramWriteAt(wram, high, object, 0);
    }
}

/* Reads the frame record at Y into the object's step and timer fields; with
 * `full` also its frame bounds. M=1 on return. */
static void LoadFrameStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram,
    bool full) {
    const uint16_t object = cpu->x;
    const uint16_t frame = cpu->y;

    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, Absolute(wram, 5u, frame)));
    StoreStep(wram, cpu, object, OBJECT_STEP_X, OBJECT_STEP_X_HIGH);
    LoadA8(cpu, Read8(memory, Absolute(wram, 6u, frame)));
    StoreStep(wram, cpu, object, OBJECT_STEP_Y, OBJECT_STEP_Y_HIGH);
    LoadA8(cpu, Read8(memory, Absolute(wram, 7u, frame)));
    WramWriteAt(wram, OBJECT_TIMER, object, A8(cpu));
    if (full) {
        LoadA8(cpu, Read8(memory, Absolute(wram, 0u, frame)));
        WramWriteAt(wram, OBJECT_FRAME_COUNT, object, A8(cpu));
        LoadA8(cpu, Read8(memory, Absolute(wram, 2u, frame)));
        WramWriteAt(wram, OBJECT_FRAME_END, object, A8(cpu));
        WramWriteAt(wram, OBJECT_FRAME, object, 0);
        LoadA8(cpu, Read8(memory, Absolute(wram, 1u, frame)));
        WramWriteAt(wram, OBJECT_FRAME_RESTART, object, A8(cpu));
    }
}

/* Loads the frame pointer of pose/animation: the record address Y. */
static uint16_t FindFrame(Lufia2Wram wram, Lufia2CpuState *cpu) {
    const uint16_t table = ReadAbsolute16(wram, ANIMATION_POINTERS, cpu->y);
    uint32_t at;

    LoadA16(cpu, table);
    WramWrite16(wram, FRAME_POINTER_LOCATION, cpu->accumulator);
    LoadA16(cpu, WramRead16At(wram, OBJECT_POSE, cpu->x));
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToY(cpu);
    at = (((uint32_t)wram.data_bank << 16) + WramRead16(wram, FRAME_POINTER_LOCATION)
          + cpu->y) & 0x00ffffffu;
    LoadA16(cpu, (uint16_t)(Read8(wram.memory, at) |
        ((uint16_t)Read8(wram.memory, (at + 1u) & 0x00ffffffu) << 8)));
    return cpu->accumulator;
}

/* $86:E0B9: starts animation A on object X. M1X0. */
Lufia2ExecutionResult Lufia2WorldMapStartAnimation(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86e0b9u);
    wram = WramViewOfCaller(memory, cpu);
    WramWriteAt(wram, OBJECT_ANIMATION, cpu->x, A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToY(cpu);
    (void)FindFrame(wram, cpu);
    TransferAToY(cpu);
    LoadA16(cpu, ReadAbsolute16(wram, 3u, cpu->y));
    WramWrite16At(wram, OBJECT_FRAME_POINTER, cpu->x, cpu->accumulator);
    LoadFrameStep(memory, cpu, wram, true);
    LoadY16(cpu, WramRead16At(wram, OBJECT_FRAME_POINTER, cpu->x));
    if (cpu->negative) {
        const uint16_t object = cpu->x;

        Push8(memory, cpu, (uint8_t)(object >> 8));
        Push8(memory, cpu, (uint8_t)object);
        cpu->x = cpu->y;
        SetNz16(cpu, cpu->x);
        LoadA8(cpu, Read8(memory, Absolute(wram, 0u, cpu->x)));
        cpu->x = PullIndexValue(memory, cpu);
    } else {
        LoadA8(cpu, 0);
    }
    And8(cpu, 0x7fu);
    WramWriteAt(wram, OBJECT_TILE_BASE, cpu->x, A8(cpu));
    return ExecutionReturned(0x86e11au);
}

/* $86:E186: after the frame counter moved, finds the frame record for the
 * product in $4216 and loads the object's frame pointer. M1X0. */
static void NextFrameRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram) {
    const uint16_t object = cpu->x;

    LoadA8(cpu, WramReadAt(wram, OBJECT_ANIMATION, object));
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToY(cpu);
    (void)FindFrame(wram, cpu);
    Add16Value(cpu, WramRead16(wram, SNES_RDMPYL));
    TransferAToY(cpu);
    LoadA16(cpu, ReadAbsolute16(wram, 3u, cpu->y));
    WramWrite16At(wram, OBJECT_FRAME_POINTER, object, cpu->accumulator);
    Push8(memory, cpu, (uint8_t)(object >> 8));
    Push8(memory, cpu, (uint8_t)object);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    if (cpu->negative)
        LoadA8(cpu, Read8(memory, Absolute(wram, 0u, cpu->x)));
    else
        LoadA8(cpu, 0);
    cpu->x = PullIndexValue(memory, cpu);
    And8(cpu, 0x7fu);
    WramWriteAt(wram, OBJECT_TILE_BASE, cpu->x, A8(cpu));
}

/* $86:E11F: steps the animation of all 22 objects. A timer of 1 runs out
 * into the next frame record; a positive timer counts down. M1X0. */
Lufia2ExecutionResult Lufia2WorldMapStepAnimations(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;
    bool restart;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86e11fu);
    wram = WramViewOfCaller(memory, cpu);
    cpu->x = OBJECT_TABLE;
    LoadA8(cpu, OBJECT_COUNT);
    WramWrite(wram, OBJECTS_LEFT, A8(cpu));
    do {
        LoadA8(cpu, WramReadAt(wram, OBJECT_TIMER, cpu->x));
        if (cpu->zero || cpu->negative) {
            WramWriteAt(wram, OBJECT_TIMER, cpu->x, A8(cpu));
        } else {
            DecrementA8(cpu);
            if (!cpu->zero) {
                WramWriteAt(wram, OBJECT_TIMER, cpu->x, A8(cpu));
            } else {
                LoadA8(cpu, WramReadAt(wram, OBJECT_FRAME, cpu->x));
                LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
                Compare8(cpu, A8(cpu), WramReadAt(wram, OBJECT_FRAME_COUNT, cpu->x));
                restart = false;
                if (cpu->carry) {
                    LoadA8(cpu, WramReadAt(wram, OBJECT_FRAME_END, cpu->x));
                    BitImmediate8(cpu, 1u);
                    if (cpu->zero) {
                        LoadA8(cpu, WramReadAt(wram, OBJECT_ANIMATION_FLAGS, cpu->x));
                        SimulateJsrFrame(memory, cpu, 0xe17fu);
                        (void)Lufia2WorldMapStartAnimation(memory, cpu);
                        SimulateRtsFrame(memory, cpu);
                        restart = true;
                    } else {
                        LoadA8(cpu, 0);
                    }
                }
                if (!restart) {
                    WramWriteAt(wram, OBJECT_FRAME, cpu->x, A8(cpu));
                    WramWrite(wram, SNES_WRMPYA, A8(cpu));
                    LoadA8(cpu, FRAME_SIZE);
                    WramWrite(wram, SNES_WRMPYB, A8(cpu));
                    SimulateJsrFrame(memory, cpu, 0xe142u);
                    NextFrameRecord(memory, cpu, wram);
                    SimulateRtsFrame(memory, cpu);
                    LoadFrameStep(memory, cpu, wram, false);
                }
            }
        }
        SetAccumulatorWidth(cpu, 0);
        TransferXToA(cpu);
        cpu->carry = false;
        Add16Value(cpu, OBJECT_SIZE);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
        {
            const uint8_t left = (uint8_t)(WramRead(wram, OBJECTS_LEFT) - 1u);

            WramWrite(wram, OBJECTS_LEFT, left);
            SetNz8(cpu, left);
        }
    } while (!cpu->zero);
    return ExecutionReturned(0x86e174u);
}

/* Direct page of the sprite pair drawer: the object pointer, the attribute
 * word being built, the screen x of the pair and the tile word. */
enum {
    DRAW_OBJECT = 0x02u,
    DRAW_ATTRIBUTES = 0x00u,
    DRAW_ATTRIBUTE_FLAGS = 0x01u,
    DRAW_SCREEN_X = 0x04u,
    DRAW_SCREEN_X_HIGH = 0x05u,
    DRAW_TILE = 0x11u,
    DRAW_BASE_ATTRIBUTES = 0x3000u,
    DRAW_MIRROR_FLAG = 0x40u,
    DRAW_MIRROR_POSE = 3u,
    DRAW_X_BIAS = 8u,
    DRAW_Y_BIAS = 0x21u,
    DRAW_LOWER_ROW = 0x10u,
    DRAW_NEXT_TILE = 0x20u,
    DRAW_X_MASK = 0xfdffu
};

/* Absolute work RAM: the sprite counter (bits 0-1 pick the 2-bit field of a
 * high-table byte, the rest is the byte index) and the OAM high table. */
enum {
    SPRITE_COUNTER = 0x1467u,
    OAM_HIGH_TABLE = 0x0300u
};

/* High-table field handlers, indexed by the counter's low two bits. Each gets
 * the old high-table byte in A and the x bits at $05, and leaves the new byte
 * in A (M1). */
static void MergeHighBits(
    Lufia2Wram wram, Lufia2CpuState *cpu, unsigned field) {
    static const uint8_t kKeep[4] = {0xfcu, 0xf3u, 0xcfu, 0x3fu};

    And8(cpu, kKeep[field]);
    WramWrite(wram, DRAW_ATTRIBUTES, A8(cpu));
    LoadA8(cpu, WramRead(wram, DRAW_SCREEN_X_HIGH));
    And8(cpu, 3u);
    switch (field) {
    case 1:
        AslA8(cpu);
        AslA8(cpu);
        break;
    case 2:
        AslA8(cpu);
        AslA8(cpu);
        AslA8(cpu);
        AslA8(cpu);
        break;
    case 3:
        LsrA8(cpu);
        RorA8(cpu);
        RorA8(cpu);
        break;
    default:
        break;
    }
    Or8(cpu, WramRead(wram, DRAW_ATTRIBUTES));
}

/* $86:E5BB: stores the next two x bits of the sprite pair into the OAM high
 * table and advances the counter. M0X0 only. */
Lufia2ExecutionResult Lufia2WorldMapStoreHighBits(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;
    uint16_t counter;
    uint32_t location;

    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86e5bbu);
    wram = WramViewOfCaller(memory, cpu);
    counter = ReadAbsolute16(wram, SPRITE_COUNTER, 0);
    LoadA16(cpu, counter);
    And16(cpu, 3u);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, counter);
    LsrA16(cpu);
    LsrA16(cpu);
    TransferAToY(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, Absolute(wram, OAM_HIGH_TABLE, cpu->y)));
    SimulateJsrFrame(memory, cpu, 0xe5d0u);
    MergeHighBits(wram, cpu, cpu->x >> 1);
    SimulateRtsFrame(memory, cpu);
    Write8(memory, Absolute(wram, OAM_HIGH_TABLE, cpu->y), A8(cpu));
    location = Absolute(wram, SPRITE_COUNTER, 0);
    {
        const uint8_t next = (uint8_t)(Read8(memory, location) + 1u);

        Write8(memory, location, next);
        SetNz8(cpu, next);
    }
    SetAccumulatorWidth(cpu, 0);
    return ExecutionReturned(0x86e5d9u);
}

/* $86:E555: writes the pair of hardware sprites of the object at $02 into
 * the OAM buffer slots selected by the sprite counter, mirroring objects
 * whose pose is 3 or more, then stores both x bits. M0X0 only. */
Lufia2ExecutionResult Lufia2WorldMapDrawSpritePair(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;
    uint32_t slot;
    uint8_t flags;

    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86e555u);
    wram = WramViewOfCaller(memory, cpu);
    LoadX16(cpu, WramRead16(wram, DRAW_OBJECT));
    LoadY16(cpu, DRAW_BASE_ATTRIBUTES);
    WramWrite16(wram, DRAW_ATTRIBUTES, cpu->y);
    LoadA16(cpu, ReadAbsolute16(wram, SPRITE_COUNTER, 0));
    AslA16(cpu);
    AslA16(cpu);
    TransferAToY(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, WramReadAt(wram, OBJECT_POSE, cpu->x));
    Compare8(cpu, A8(cpu), DRAW_MIRROR_POSE);
    if (cpu->carry) {
        const uint8_t old = WramRead(wram, DRAW_ATTRIBUTE_FLAGS);

        LoadA8(cpu, DRAW_MIRROR_FLAG);
        cpu->zero = (old & A8(cpu)) == 0;
        WramWrite(wram, DRAW_ATTRIBUTE_FLAGS, (uint8_t)(old | A8(cpu)));
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, WramRead16At(wram, OBJECT_SCREEN_X, cpu->x));
        Subtract16(cpu, WramRead16At(wram, OBJECT_STEP_X, cpu->x));
    } else {
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, WramRead16At(wram, OBJECT_SCREEN_X, cpu->x));
        cpu->carry = false;
        Add16Value(cpu, WramRead16At(wram, OBJECT_STEP_X, cpu->x));
    }
    Subtract16(cpu, DRAW_X_BIAS);
    And16(cpu, DRAW_X_MASK);
    WramWrite16(wram, DRAW_SCREEN_X, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    slot = Absolute(wram, OAM_BUFFER, cpu->y);
    Write8(memory, slot, A8(cpu));
    Write8(memory, Absolute(wram, OAM_BUFFER + 4u, cpu->y), A8(cpu));
    LoadA8(cpu, WramReadAt(wram, OBJECT_SPRITE_FLAGS, cpu->x));
    flags = WramRead(wram, DRAW_ATTRIBUTE_FLAGS);
    cpu->zero = (flags & A8(cpu)) == 0;
    WramWrite(wram, DRAW_ATTRIBUTE_FLAGS, (uint8_t)(flags | A8(cpu)));
    LoadA8(cpu, WramReadAt(wram, OBJECT_SCREEN_Y, cpu->x));
    cpu->carry = false;
    Adc8(cpu, (uint8_t)WramReadAt(wram, OBJECT_HEIGHT, cpu->x));
    cpu->carry = true;
    Sbc8(cpu, DRAW_Y_BIAS);
    Write8(memory, Absolute(wram, OAM_BUFFER + 1u, cpu->y), A8(cpu));
    cpu->carry = false;
    Adc8(cpu, DRAW_LOWER_ROW);
    Write8(memory, Absolute(wram, OAM_BUFFER + 5u, cpu->y), A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, WramRead16(wram, DRAW_TILE));
    Or16(cpu, WramRead16(wram, DRAW_ATTRIBUTES));
    WriteAbsolute16(wram, OAM_BUFFER + 2u, cpu->y, cpu->accumulator);
    cpu->carry = false;
    Add16Value(cpu, DRAW_NEXT_TILE);
    WriteAbsolute16(wram, OAM_BUFFER + 6u, cpu->y, cpu->accumulator);
    SimulateJsrFrame(memory, cpu, 0xe5b6u);
    (void)Lufia2WorldMapStoreHighBits(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    SimulateJsrFrame(memory, cpu, 0xe5b9u);
    (void)Lufia2WorldMapStoreHighBits(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    return ExecutionReturned(0x86e5bau);
}
