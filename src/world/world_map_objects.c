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

/* Stores a word the way a read-modify-write instruction does: high byte
 * first. */
static void WriteModified16(
    Lufia2Wram wram, uint32_t location, uint16_t index, uint16_t value) {
    const uint32_t low = WramAddress(wram, location, index);

    Write8(wram.memory, WramNextAddress(location, low), (uint8_t)(value >> 8));
    Write8(wram.memory, low, (uint8_t)value);
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
    DRAW_X_MASK = 0xfdffu,
    DRAW_WIDE_X_BIAS = 0x10u,
    DRAW_WIDE_SIZE_BIT = 0x0200u,
    DRAW_SMALL_Y_BIAS = 0x11u
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

/* The shared body of $86:E5BB and of the copies of it inside $86:E479 and
 * $86:E4E7; they differ only in the return address their indirect call
 * leaves on the stack. M0X0. */
static void StoreHighBitsBody(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram,
    uint16_t handler_return) {
    uint16_t counter;
    uint32_t location;

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
    SimulateJsrFrame(memory, cpu, handler_return);
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
}

/* $86:E5BB: stores the next two x bits of the sprite pair into the OAM high
 * table and advances the counter. M0X0 only. */
Lufia2ExecutionResult Lufia2WorldMapStoreHighBits(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86e5bbu);
    StoreHighBitsBody(memory, cpu, WramViewOfCaller(memory, cpu), 0xe5d0u);
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

/* TSB $01: sets the bits of A in the attribute flags byte. */
static void SetAttributeFlags(Lufia2Wram wram, Lufia2CpuState *cpu) {
    const uint8_t flags = WramRead(wram, DRAW_ATTRIBUTE_FLAGS);

    cpu->zero = (flags & A8(cpu)) == 0;
    WramWrite(wram, DRAW_ATTRIBUTE_FLAGS, (uint8_t)(flags | A8(cpu)));
}

/* The horizontal position of a sprite: the object's screen x moved by its
 * step, mirrored for poses 3 and up, which also sets the mirror attribute.
 * Leaves M16 with the position in A. */
static void MirroredScreenX(Lufia2Wram wram, Lufia2CpuState *cpu) {
    Compare8(cpu, A8(cpu), DRAW_MIRROR_POSE);
    if (cpu->carry) {
        LoadA8(cpu, DRAW_MIRROR_FLAG);
        SetAttributeFlags(wram, cpu);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, WramRead16At(wram, OBJECT_SCREEN_X, cpu->x));
        Subtract16(cpu, WramRead16At(wram, OBJECT_STEP_X, cpu->x));
    } else {
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, WramRead16At(wram, OBJECT_SCREEN_X, cpu->x));
        cpu->carry = false;
        Add16Value(cpu, WramRead16At(wram, OBJECT_STEP_X, cpu->x));
    }
}

/* The vertical position: screen y plus the y step, less `bias`. M8. */
static void SpriteScreenY(Lufia2Wram wram, Lufia2CpuState *cpu, uint8_t bias) {
    LoadA8(cpu, WramReadAt(wram, OBJECT_SCREEN_Y, cpu->x));
    cpu->carry = false;
    Adc8(cpu, WramReadAt(wram, OBJECT_HEIGHT, cpu->x));
    cpu->carry = true;
    Sbc8(cpu, bias);
}

/* $86:E479: one 16-pixel-wide sprite for the object at $02 (mirrored from
 * pose 3), then its x bits. M0X0 only. */
Lufia2ExecutionResult Lufia2WorldMapDrawSprite(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;

    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86e479u);
    wram = WramViewOfCaller(memory, cpu);
    LoadX16(cpu, WramRead16(wram, DRAW_OBJECT));
    LoadY16(cpu, DRAW_BASE_ATTRIBUTES);
    WramWrite16(wram, DRAW_ATTRIBUTES, cpu->y);
    LoadA16(cpu, ReadAbsolute16(wram, SPRITE_COUNTER, 0));
    AslA16(cpu);
    AslA16(cpu);
    TransferAToY(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, WramReadAt(wram, OBJECT_SPRITE_FLAGS, cpu->x));
    SetAttributeFlags(wram, cpu);
    LoadA8(cpu, WramReadAt(wram, OBJECT_POSE, cpu->x));
    MirroredScreenX(wram, cpu);
    Subtract16(cpu, DRAW_WIDE_X_BIAS);
    Or16(cpu, DRAW_WIDE_SIZE_BIT);
    WramWrite16(wram, DRAW_SCREEN_X, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    Write8(memory, Absolute(wram, OAM_BUFFER, cpu->y), A8(cpu));
    SpriteScreenY(wram, cpu, DRAW_Y_BIAS);
    Write8(memory, Absolute(wram, OAM_BUFFER + 1u, cpu->y), A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, WramRead16(wram, DRAW_TILE));
    Or16(cpu, WramRead16(wram, DRAW_ATTRIBUTES));
    WriteAbsolute16(wram, OAM_BUFFER + 2u, cpu->y, cpu->accumulator);
    StoreHighBitsBody(memory, cpu, wram, 0xe4ddu);
    return ExecutionReturned(0x86e4e6u);
}

/* $86:E4E7: one 8-pixel-wide sprite for the object at $02, then its x bits.
 * M0X0 only. */
Lufia2ExecutionResult Lufia2WorldMapDrawSmallSprite(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;

    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86e4e7u);
    wram = WramViewOfCaller(memory, cpu);
    LoadX16(cpu, WramRead16(wram, DRAW_OBJECT));
    LoadY16(cpu, DRAW_BASE_ATTRIBUTES);
    WramWrite16(wram, DRAW_ATTRIBUTES, cpu->y);
    LoadA16(cpu, ReadAbsolute16(wram, SPRITE_COUNTER, 0));
    AslA16(cpu);
    AslA16(cpu);
    TransferAToY(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, WramReadAt(wram, OBJECT_SPRITE_FLAGS, cpu->x));
    SetAttributeFlags(wram, cpu);
    LoadA8(cpu, WramReadAt(wram, OBJECT_POSE, cpu->x));
    MirroredScreenX(wram, cpu);
    Subtract16(cpu, DRAW_X_BIAS);
    And16(cpu, DRAW_X_MASK);
    WramWrite16(wram, DRAW_SCREEN_X, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    Write8(memory, Absolute(wram, OAM_BUFFER, cpu->y), A8(cpu));
    SpriteScreenY(wram, cpu, DRAW_SMALL_Y_BIAS);
    Write8(memory, Absolute(wram, OAM_BUFFER + 1u, cpu->y), A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, WramRead16(wram, DRAW_TILE));
    Or16(cpu, WramRead16(wram, DRAW_ATTRIBUTES));
    WriteAbsolute16(wram, OAM_BUFFER + 2u, cpu->y, cpu->accumulator);
    StoreHighBitsBody(memory, cpu, wram, 0xe54bu);
    return ExecutionReturned(0x86e554u);
}

/* The frame's sorted visible list: sort key words from LIST_KEYS on, the
 * object each belongs to LIST_OBJECTS bytes after it, and a count word. */
enum {
    LIST_SENTINEL = 0x124du,
    LIST_KEYS = 0x124fu,
    LIST_FIRST_MOVED = 0x1251u,
    LIST_OBJECTS = 0x002cu,
    SORT_KEY = 0x00u,
    SORT_OBJECT = 0x06u,
    SORT_LEFT = 0x22u
};

/* $86:E686: insertion sort of the visible list by its key words, ascending.
 * The word before the list is set to $FFFF so the scan stops there. M0X0. */
Lufia2ExecutionResult Lufia2WorldMapSortVisible(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;

    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86e686u);
    wram = WramViewOfCaller(memory, cpu);
    LoadA16(cpu, 0xffffu);
    WriteAbsolute16(wram, LIST_SENTINEL, 0, cpu->accumulator);
    LoadA16(cpu, ReadAbsolute16(wram, VISIBLE_COUNT, 0));
    if (cpu->zero)
        return ExecutionReturned(0x86e6c3u);
    LoadA16(cpu, (uint16_t)(cpu->accumulator - 1u));
    if (cpu->zero)
        return ExecutionReturned(0x86e6c3u);
    WramWrite16(wram, SORT_LEFT, cpu->accumulator);
    LoadX16(cpu, LIST_FIRST_MOVED);
    do {
        LoadA16(cpu, WramRead16At(wram, SORT_KEY, cpu->x));
        WramWrite16(wram, SORT_KEY, cpu->accumulator);
        LoadA16(cpu, WramRead16At(wram, LIST_OBJECTS, cpu->x));
        WramWrite16(wram, SORT_OBJECT, cpu->accumulator);
        PushIndex(memory, cpu);
        LoadX16(cpu, (uint16_t)(cpu->x - 2u));
        for (;;) {
            LoadA16(cpu, WramRead16At(wram, SORT_KEY, cpu->x));
            Compare16(cpu, cpu->accumulator, WramRead16(wram, SORT_KEY));
            if (cpu->carry)
                break;
            WramWrite16At(wram, 2u, cpu->x, cpu->accumulator);
            LoadA16(cpu, WramRead16At(wram, LIST_OBJECTS, cpu->x));
            WramWrite16At(wram, LIST_OBJECTS + 2u, cpu->x, cpu->accumulator);
            LoadX16(cpu, (uint16_t)(cpu->x - 2u));
        }
        LoadA16(cpu, WramRead16(wram, SORT_KEY));
        WramWrite16At(wram, 2u, cpu->x, cpu->accumulator);
        LoadA16(cpu, WramRead16(wram, SORT_OBJECT));
        WramWrite16At(wram, LIST_OBJECTS + 2u, cpu->x, cpu->accumulator);
        cpu->x = PullIndexValue(memory, cpu);
        LoadX16(cpu, (uint16_t)(cpu->x + 2u));
        SetNz16(cpu, StepDirect16(wram, SORT_LEFT, -1));
    } while (!cpu->zero);
    return ExecutionReturned(0x86e6c3u);
}

/* Sprite pattern slots: a list of up to 32 patterns in use this frame (the
 * pattern pointer, its user count, the tile block and the last user), the
 * object field linking users of one pattern and the counter of slots. */
enum {
    SLOT_LIST = 0x1367u,
    SLOT_LIST_COUNT = 0x1365u,
    SLOT_PATTERN = 0x00u,
    SLOT_USERS = 0x40u,
    SLOT_TILE_BLOCK = 0x80u,
    SLOT_LAST_USER = 0xc0u,
    SLOT_POOL_POINTER = 0x08u,
    OBJECT_PATTERN = 0x19u,
    OBJECT_NEXT_USER = 0x1bu,
    OBJECT_SLOT_INDEX = 0x16u
};

/* $86:E430: finds or adds the pattern of the object at $02 in the slot list
 * starting at $08 (Y entries), then in the shared list at $1367. Carry set:
 * a new use was recorded for the object (its slot index stored); carry
 * clear: the pattern is already in this frame's pool and $11 holds its tile
 * block. M0X0 only. */
Lufia2ExecutionResult Lufia2WorldMapAssignSlot(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;
    bool found = false;

    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86e430u);
    wram = WramViewOfCaller(memory, cpu);
    LoadX16(cpu, WramRead16(wram, DRAW_OBJECT));
    LoadA16(cpu, WramRead16At(wram, OBJECT_PATTERN, cpu->x));
    if (!cpu->negative) {
        WramWrite16(wram, DRAW_TILE, cpu->accumulator);
        cpu->carry = false;
        return ExecutionReturned(0x86e478u);
    }
    LoadX16(cpu, WramRead16(wram, SLOT_POOL_POINTER));
    do {
        Compare16(cpu, cpu->accumulator, WramRead16At(wram, SLOT_PATTERN, cpu->x));
        if (cpu->zero) {
            WriteModified16(wram, SLOT_USERS, cpu->x,
                (uint16_t)(WramRead16At(wram, SLOT_USERS, cpu->x) + 1u));
            SetNz16(cpu, WramRead16At(wram, SLOT_USERS, cpu->x));
            LoadA16(cpu, WramRead16At(wram, SLOT_TILE_BLOCK, cpu->x));
            WramWrite16(wram, DRAW_TILE, cpu->accumulator);
            cpu->carry = false;
            return ExecutionReturned(0x86e478u);
        }
        LoadX16(cpu, (uint16_t)(cpu->x + 2u));
        LoadY16(cpu, (uint16_t)(cpu->y - 1u));
    } while (!cpu->zero);
    LoadX16(cpu, SLOT_LIST);
    LoadY16(cpu, ReadAbsolute16(wram, SLOT_LIST_COUNT, 0));
    if (!cpu->zero) {
        do {
            Compare16(cpu, cpu->accumulator, WramRead16At(wram, SLOT_PATTERN, cpu->x));
            if (cpu->zero) {
                found = true;
                break;
            }
            LoadX16(cpu, (uint16_t)(cpu->x + 2u));
            LoadY16(cpu, (uint16_t)(cpu->y - 1u));
        } while (!cpu->zero);
    }
    if (found) {
        uint16_t last;

        WriteModified16(wram, SLOT_USERS, cpu->x,
            (uint16_t)(WramRead16At(wram, SLOT_USERS, cpu->x) + 1u));
        SetNz16(cpu, WramRead16At(wram, SLOT_USERS, cpu->x));
        LoadY16(cpu, WramRead16At(wram, SLOT_LAST_USER, cpu->x));
        LoadA16(cpu, WramRead16(wram, DRAW_OBJECT));
        WramWrite16At(wram, SLOT_LAST_USER, cpu->x, cpu->accumulator);
        last = cpu->y;
        WriteAbsolute16(wram, OBJECT_NEXT_USER, last, cpu->accumulator);
    } else {
        WramWrite16At(wram, SLOT_PATTERN, cpu->x, cpu->accumulator);
        LoadA16(cpu, 1u);
        WramWrite16At(wram, SLOT_USERS, cpu->x, cpu->accumulator);
        LoadA16(cpu, WramRead16(wram, DRAW_OBJECT));
        WramWrite16At(wram, SLOT_TILE_BLOCK, cpu->x, cpu->accumulator);
        WramWrite16At(wram, SLOT_LAST_USER, cpu->x, cpu->accumulator);
        WriteModified16(wram, SLOT_LIST_COUNT, 0,
            (uint16_t)(ReadAbsolute16(wram, SLOT_LIST_COUNT, 0) + 1u));
    }
    /* Both ways end by recording the slot index in the object. */
    LoadX16(cpu, WramRead16(wram, DRAW_OBJECT));
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, Absolute(wram, SPRITE_COUNTER, 0)));
    WramWriteAt(wram, OBJECT_SLOT_INDEX, cpu->x, A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    cpu->carry = true;
    return ExecutionReturned(0x86e42fu);
}

/* The three object kinds, by the byte at $17: the sprite writer, the pool
 * of tile blocks it draws from and the number of blocks in the pool. */
enum {
    OBJECT_KIND = 0x17u,
    KIND_COUNT = 3u,
    KIND_POOL_SMALL = 0x12b5u,
    KIND_POOL_SMALL_BLOCKS = 0x0010u,
    KIND_POOL_WIDE = 0x12a5u,
    KIND_POOL_PAIR = 0x12d5u,
    KIND_POOL_BLOCKS = 0x0008u,
    PLAYER_OBJECT = 0x16cau
};

/* The slot lookup shared by the three kinds below: Y blocks from `pool`,
 * then the draw, or just another use of the sprite counter. */
static void DrawObjectKind(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram,
    unsigned kind) {
    static const uint16_t kPool[KIND_COUNT] = {
        KIND_POOL_WIDE, KIND_POOL_SMALL, KIND_POOL_PAIR};
    static const uint16_t kBlocks[KIND_COUNT] = {
        KIND_POOL_BLOCKS, KIND_POOL_SMALL_BLOCKS, KIND_POOL_BLOCKS};
    static const uint16_t kAssignReturn[KIND_COUNT] = {0xe400u, 0xe3efu, 0xe415u};
    static const uint16_t kDrawReturn[KIND_COUNT] = {0xe405u, 0xe3f4u, 0xe41au};
    const unsigned extra_uses = kind == 2u ? 2u : 1u;
    unsigned i;

    LoadX16(cpu, kPool[kind]);
    WramWrite16(wram, SLOT_POOL_POINTER, cpu->x);
    LoadY16(cpu, kBlocks[kind]);
    SimulateJsrFrame(memory, cpu, kAssignReturn[kind]);
    (void)Lufia2WorldMapAssignSlot(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    if (cpu->carry) {
        for (i = 0; i < extra_uses; ++i) {
            const uint16_t counter = ReadAbsolute16(wram, SPRITE_COUNTER, 0);

            WriteModified16(wram, SPRITE_COUNTER, 0, (uint16_t)(counter + 1u));
            SetNz16(cpu, (uint16_t)(counter + 1u));
        }
        return;
    }
    SimulateJsrFrame(memory, cpu, kDrawReturn[kind]);
    if (kind == 0u)
        (void)Lufia2WorldMapDrawSprite(memory, cpu);
    else if (kind == 1u)
        (void)Lufia2WorldMapDrawSmallSprite(memory, cpu);
    else
        (void)Lufia2WorldMapDrawSpritePair(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $86:E3D2: draws the object at X by its kind. M0X0 only. An unknown kind
 * hands the original dispatch back at its indirect call. */
Lufia2ExecutionResult Lufia2WorldMapDrawObjectByKind(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;

    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86e3d2u);
    wram = WramViewOfCaller(memory, cpu);
    WramWrite16(wram, DRAW_OBJECT, cpu->x);
    LoadA16(cpu, WramRead16At(wram, OBJECT_KIND, cpu->x));
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    if (cpu->x >= 2u * KIND_COUNT)
        return ExecutionHandoff(cpu, 0x86e3dbu);
    SimulateJsrFrame(memory, cpu, 0xe3ddu);
    DrawObjectKind(memory, cpu, wram, cpu->x >> 1);
    SimulateRtsFrame(memory, cpu);
    return ExecutionReturned(0x86e3deu);
}

/* $86:E3CE: the object draw entry; a call to $E3D2. M0X0 only. */
static Lufia2ExecutionResult DrawObject(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;

    SimulateJsrFrame(memory, cpu, 0xe3d0u);
    result = Lufia2WorldMapDrawObjectByKind(memory, cpu);
    if (result.flow == LUFIA2_EXECUTION_BOUNDARY)
        return result;
    SimulateRtsFrame(memory, cpu);
    return ExecutionReturned(0x86e3d1u);
}

/* $86:E3AB: draws every object of the sorted visible list, then the player
 * object when it is on the map. M0X0 only. */
Lufia2ExecutionResult Lufia2WorldMapDrawObjects(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;
    Lufia2ExecutionResult result;

    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86e3abu);
    wram = WramViewOfCaller(memory, cpu);
    LoadA16(cpu, ReadAbsolute16(wram, VISIBLE_COUNT, 0));
    if (!cpu->zero) {
        WramWrite16(wram, SORT_LEFT, cpu->accumulator);
        LoadY16(cpu, LIST_KEYS);
        do {
            PushY(memory, cpu);
            LoadX16(cpu, ReadAbsolute16(wram, LIST_OBJECTS, cpu->y));
            SimulateJsrFrame(memory, cpu, 0xe3bbu);
            result = DrawObject(memory, cpu);
            if (result.flow == LUFIA2_EXECUTION_BOUNDARY)
                return result;
            SimulateRtsFrame(memory, cpu);
            cpu->y = PullIndexValue(memory, cpu);
            LoadY16(cpu, (uint16_t)(cpu->y + 2u));
            SetNz16(cpu, StepDirect16(wram, SORT_LEFT, -1));
        } while (!cpu->zero);
    }
    LoadX16(cpu, PLAYER_OBJECT);
    LoadY16(cpu, WramRead16At(wram, OBJECT_Y, cpu->x));
    if (!cpu->negative) {
        SimulateJsrFrame(memory, cpu, 0xe3ccu);
        result = DrawObject(memory, cpu);
        if (result.flow == LUFIA2_EXECUTION_BOUNDARY)
            return result;
        SimulateRtsFrame(memory, cpu);
    }
    return ExecutionReturned(0x86e3cdu);
}

/* Divisor, dividend and results of the 32-bit division: the quotient
 * replaces the dividend and the remainder is left in A. */
enum {
    DIVIDEND_LOW = 0x00u,
    DIVIDEND_HIGH = 0x02u,
    DIVISOR = 0x04u,
    DIVISION_BITS = 32u
};

/* $86:A5A9: unsigned 32-bit by 16-bit division of $00/$02 by $04, one bit
 * at a time: quotient to $00/$02, remainder in A. A remainder that carries
 * out of 16 bits counts as not less than the divisor, so a zero divisor
 * yields an all-ones quotient. Any entry widths; they are restored. */
Lufia2ExecutionResult Lufia2WorldMapDivide32(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const uint16_t divisor = WramRead16(wram, DIVISOR);
    uint16_t low;
    uint16_t high;
    uint16_t remainder = 0;
    unsigned bit;

    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    low = WramRead16(wram, DIVIDEND_LOW);
    high = WramRead16(wram, DIVIDEND_HIGH);
    for (bit = 0; bit < DIVISION_BITS; ++bit) {
        const bool low_out = (low >> 15) != 0u;
        const bool shifted_out = (high >> 15) != 0u;
        const bool carry = (remainder >> 15) != 0u;

        low = (uint16_t)(low << 1);
        WriteModified16(wram, DIVIDEND_LOW, 0, low);
        high = (uint16_t)((high << 1) | (low_out ? 1u : 0u));
        WriteModified16(wram, DIVIDEND_HIGH, 0, high);
        remainder = (uint16_t)((remainder << 1) | (shifted_out ? 1u : 0u));
        if (carry || remainder >= divisor) {
            remainder = (uint16_t)(remainder - divisor);
            low = (uint16_t)(low + 1u);
            WriteModified16(wram, DIVIDEND_LOW, 0, low);
        }
    }
    LoadA16(cpu, remainder);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x86a790u);
}

/* Perspective of the tilted world map: the camera row, the depth table in
 * ROM and the scratch words of the projection. */
enum {
    VIEW_X = 0x11e8u,
    VIEW_Y = 0x11eau,
    VIEW_HORIZON = 0x11ffu,
    VIEW_HORIZON_LIMIT = 0x3eu,
    DEPTH_TABLE = 0x97b226u,
    CAMERA_ROW = 0x54u,
    CAMERA_BOTTOM = 0x56u,
    PROJECTION_SCALE = 0x11u,
    PROJECTION_SCALE_NEAR = 0x12u,
    PROJECTION_DISTANCE = 0x4eu,
    PROJECTION_PRODUCT = 0x51u,
    PROJECTION_PRODUCT_HIGH = 0x52u,
    PROJECTION_PRODUCT_TOP = 0x53u,
    PROJECTION_DISTANCE_HIGH = 0x4fu,
    PROJECTION_ROW_X = 0x04u,
    PROJECTION_COLUMN = 0x02u,
    PROJECTION_ROW_LIMIT = 0xe0u,
    PROJECTION_LIMIT = 0xf0u,
    PROJECTION_CENTRE = 0x80u,
    TILT_WORD = 0x12u,
    TILT_DEPTH = 0x01u
};

/* $86:E356: scales the distance in $4E by the two depth factors $12 and $11
 * with the hardware multiplier, then divides the 32-bit result. M0X0. */
static void ProjectDistance(const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram) {
    const uint8_t factors[2] = {WramRead(wram, PROJECTION_SCALE_NEAR),
                                WramRead(wram, PROJECTION_SCALE)};
    unsigned pass;

    for (pass = 0; pass < 2u; ++pass) {
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, factors[pass]);
        WramWrite(wram, SNES_WRMPYA, A8(cpu));
        LoadA8(cpu, WramRead(wram, PROJECTION_DISTANCE));
        WramWrite(wram, SNES_WRMPYB, A8(cpu));
        WramWrite(wram, PROJECTION_PRODUCT_TOP, 0);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, WramRead16(wram, SNES_RDMPYL));
        WramWrite16(wram, PROJECTION_PRODUCT, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, WramRead(wram, PROJECTION_DISTANCE_HIGH));
        WramWrite(wram, SNES_WRMPYB, A8(cpu));
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, WramRead16(wram, PROJECTION_PRODUCT_HIGH));
        cpu->carry = false;
        Add16Value(cpu, WramRead16(wram, SNES_RDMPYL));
        if (pass == 0u) {
            Add16Value(cpu, 0x0100u);
            WramWrite16(wram, PROJECTION_ROW_X, cpu->accumulator);
        } else {
            WramWrite16(wram, PROJECTION_COLUMN, cpu->accumulator);
        }
    }
    WramWrite16(wram, DIVIDEND_LOW, 0);
    SimulateJsrFrame(memory, cpu, 0xe3a9u);
    (void)Lufia2WorldMapDivide32(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* The part of $86:E2D2 for an object with a negative tilt word: projects
 * its distance below the camera and appends it to the visible list at Y. */
static void ProjectObject(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram) {
    uint16_t count;

    LoadA16(cpu, WramRead16At(wram, OBJECT_Y, cpu->x));
    if (cpu->negative)
        return;
    Subtract16(cpu, WramRead16(wram, CAMERA_BOTTOM));
    if (cpu->carry)
        return;
    LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffffu));
    LoadA16(cpu, (uint16_t)(cpu->accumulator + 1u));
    WramWrite16(wram, PROJECTION_DISTANCE, cpu->accumulator);
    SimulateJsrFrame(memory, cpu, 0xe31eu);
    ProjectDistance(memory, cpu, wram);
    SimulateRtsFrame(memory, cpu);
    LoadA16(cpu, PROJECTION_ROW_LIMIT);
    Subtract16(cpu, WramRead16(wram, TILT_DEPTH));
    if (!cpu->carry)
        return;
    WramWrite16At(wram, OBJECT_SCREEN_Y, cpu->x, cpu->accumulator);
    LoadA16(cpu, WramRead16At(wram, OBJECT_X, cpu->x));
    Subtract16(cpu, WramRead16(wram, CAMERA_ROW));
    Compare16(cpu, cpu->accumulator, PROJECTION_LIMIT);
    if (cpu->carry)
        return;
    LoadA16(cpu, PROJECTION_CENTRE);
    WramWrite16At(wram, OBJECT_SCREEN_X, cpu->x, cpu->accumulator);
    LoadA16(cpu, WramRead16At(wram, OBJECT_Y, cpu->x));
    WriteAbsolute16(wram, VISIBLE_KEY, cpu->y, cpu->accumulator);
    LoadA16(cpu, cpu->x);
    WriteAbsolute16(wram, VISIBLE_OBJECT, cpu->y, cpu->accumulator);
    cpu->y = (uint16_t)(cpu->y + 2u);
    SetNz16(cpu, cpu->y);
    count = (uint16_t)(ReadAbsolute16(wram, VISIBLE_COUNT, 0) + 1u);
    WriteModified16(wram, VISIBLE_COUNT, 0, count);
    SetNz16(cpu, count);
}

/* $86:E2D2: the object visibility test of the tilted map. Objects with a
 * negative tilt word are projected by their distance below the horizon row;
 * the others use the plain test. A horizon row beyond the table limit uses
 * the plain test for every object. M0X0 only. */
Lufia2ExecutionResult Lufia2WorldMapProjectObjects(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;

    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86e2d2u);
    wram = WramViewOfCaller(memory, cpu);
    LoadA16(cpu, ReadAbsolute16(wram, VIEW_HORIZON, 0));
    And16(cpu, 0x00ffu);
    Compare16(cpu, cpu->accumulator, VIEW_HORIZON_LIMIT);
    if (cpu->carry)
        return Lufia2WorldMapTestObjects(memory, cpu);
    LoadA16(cpu, (uint16_t)(cpu->accumulator + 1u));
    LoadA16(cpu, (uint16_t)(cpu->accumulator + 1u));
    AslA16(cpu);
    PushIndex(memory, cpu);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, (DEPTH_TABLE + cpu->x) & 0x00ffffffu));
    WramWrite(wram, PROJECTION_SCALE, A8(cpu));
    LoadA8(cpu, VIEW_HORIZON_LIMIT);
    cpu->carry = true;
    Sbc8(cpu, Read8(memory, Absolute(wram, VIEW_HORIZON, 0)));
    AslA8(cpu);
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, (DEPTH_TABLE + cpu->x) & 0x00ffffffu));
    WramWrite(wram, PROJECTION_SCALE_NEAR, A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    cpu->x = PullIndexValue(memory, cpu);
    LoadA16(cpu, ReadAbsolute16(wram, VIEW_X, 0));
    WramWrite16(wram, CAMERA_ROW, cpu->accumulator);
    LoadA16(cpu, ReadAbsolute16(wram, VIEW_Y, 0));
    cpu->carry = false;
    Add16Value(cpu, 0x0070u);
    WramWrite16(wram, CAMERA_BOTTOM, cpu->accumulator);
    do {
        LoadA16(cpu, WramRead16At(wram, TILT_WORD, cpu->x));
        if (cpu->negative) {
            ProjectObject(memory, cpu, wram);
        } else {
            SimulateJsrFrame(memory, cpu, 0xe353u);
            TestObjectVisible(memory, cpu, wram);
            SimulateRtsFrame(memory, cpu);
        }
        TransferXToA(cpu);
        cpu->carry = false;
        Add16Value(cpu, OBJECT_SIZE);
        TransferAToX(cpu);
        SetNz16(cpu, StepDirect16(wram, OBJECTS_LEFT, -1));
    } while (!cpu->zero);
    return ExecutionReturned(0x86e350u);
}

/* Per-frame object update of the world map: the pools of tile blocks by
 * kind (start of the pool in the direct page, number of blocks) and the
 * direct-page scratch the loop below uses. */
enum {
    POOL_STARTS = 0xeb9bu,
    POOL_SIZES = 0xeba1u,
    UPDATE_OBJECT_COUNT = 0x15u,
    UPDATE_SLOTS_LEFT = 0x26u,
    UPDATE_BLOCKS_LEFT = 0x28u,
    UPDATE_SLOT_POINTER = 0x08u,
    SLOT_BLOCK_BASE = 0x4000u,
    VIEW_MODE_PLAIN = 0x11ddu,
    VIEW_MODE_TILTED = 0x11deu,
    CAMERA_LEFT = 0x80u,
    CAMERA_TOP = 0x70u
};

/* The loop of $86:E1B9 that draws all users of one pattern from the block
 * just taken from the pool (X). The users are chained by their next-user
 * field. M0X0 on entry and exit; M0X0 for every object kind but unknown
 * kinds, which hand the original dispatch back. */
static Lufia2ExecutionResult DrawSlotUsers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram) {
    SetAccumulatorWidth(cpu, 0);
    LoadY16(cpu, WramRead16(wram, UPDATE_SLOT_POINTER));
    LoadA16(cpu, ReadAbsolute16(wram, SLOT_USERS, cpu->y));
    WramWrite16At(wram, SLOT_USERS, cpu->x, cpu->accumulator);
    WramWrite16(wram, UPDATE_BLOCKS_LEFT, cpu->accumulator);
    LoadA16(cpu, WramRead16At(wram, SLOT_TILE_BLOCK, cpu->x));
    WramWrite16(wram, DRAW_TILE, cpu->accumulator);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    Add16Value(cpu, SLOT_BLOCK_BASE);
    WriteAbsolute16(wram, SLOT_USERS, cpu->y, cpu->accumulator);
    LoadA16(cpu, ReadAbsolute16(wram, SLOT_PATTERN, cpu->y));
    WramWrite16At(wram, SLOT_PATTERN, cpu->x, cpu->accumulator);
    LoadA16(cpu, ReadAbsolute16(wram, SLOT_TILE_BLOCK, cpu->y));
    do {
        WramWrite16(wram, DRAW_OBJECT, cpu->accumulator);
        TransferAToX(cpu);
        LoadA16(cpu, WramRead16At(wram, OBJECT_SLOT_INDEX, cpu->x));
        And16(cpu, 0x00ffu);
        WriteAbsolute16(wram, SPRITE_COUNTER, 0, cpu->accumulator);
        LoadA16(cpu, WramRead16At(wram, OBJECT_KIND, cpu->x));
        And16(cpu, 0x00ffu);
        AslA16(cpu);
        TransferAToX(cpu);
        if (cpu->x >= 2u * KIND_COUNT)
            return ExecutionHandoff(cpu, 0x86e26bu);
        SimulateJsrFrame(memory, cpu, 0xe26du);
        if (cpu->x == 0u)
            (void)Lufia2WorldMapDrawSprite(memory, cpu);
        else if (cpu->x == 2u)
            (void)Lufia2WorldMapDrawSmallSprite(memory, cpu);
        else
            (void)Lufia2WorldMapDrawSpritePair(memory, cpu);
        SimulateRtsFrame(memory, cpu);
        LoadX16(cpu, WramRead16(wram, DRAW_OBJECT));
        LoadA16(cpu, WramRead16At(wram, OBJECT_NEXT_USER, cpu->x));
        SetNz16(cpu, StepDirect16(wram, UPDATE_BLOCKS_LEFT, -1));
    } while (!cpu->zero);
    return ExecutionReturned(0x86e274u);
}

/* $86:E1B9: the whole per-frame object pass of the world map: hides the
 * sprites, clears the slot flags, tests which objects are visible, sorts
 * them, draws them (sharing tile block patterns between objects) and then
 * draws every pattern's later users from its block. M1X0 only. */
Lufia2ExecutionResult Lufia2WorldMapUpdateObjects(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;
    Lufia2ExecutionResult result;
    bool found;
    bool tilted;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86e1b9u);
    wram = WramViewOfCaller(memory, cpu);
    SimulateJsrFrame(memory, cpu, 0xe1bbu);
    (void)Lufia2WorldMapClearSprites(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    SimulateJsrFrame(memory, cpu, 0xe1beu);
    (void)Lufia2WorldMapClearSlotFlags(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    WriteAbsolute16(wram, VISIBLE_COUNT, 0, 0);
    WriteAbsolute16(wram, SPRITE_COUNTER, 0, 0);
    WriteAbsolute16(wram, SLOT_LIST_COUNT, 0, 0);
    LoadX16(cpu, UPDATE_OBJECT_COUNT);
    WramWrite16(wram, OBJECTS_LEFT, cpu->x);
    LoadX16(cpu, OBJECT_TABLE);
    LoadY16(cpu, LIST_KEYS);
    LoadA16(cpu, ReadAbsolute16(wram, VIEW_X, 0));
    Subtract16(cpu, CAMERA_LEFT);
    WramWrite16(wram, CAMERA_X, cpu->accumulator);
    LoadA16(cpu, ReadAbsolute16(wram, VIEW_Y, 0));
    Subtract16(cpu, CAMERA_TOP);
    WramWrite16(wram, CAMERA_Y, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, Absolute(wram, VIEW_MODE_PLAIN, 0)));
    tilted = false;
    if (cpu->zero) {
        LoadA8(cpu, Read8(memory, Absolute(wram, VIEW_MODE_TILTED, 0)));
        tilted = !cpu->zero;
    }
    SetAccumulatorWidth(cpu, 0);
    SimulateJsrFrame(memory, cpu, tilted ? 0xe1feu : 0xe1f7u);
    if (tilted)
        (void)Lufia2WorldMapProjectObjects(memory, cpu);
    else
        (void)Lufia2WorldMapTestObjects(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    SimulateJsrFrame(memory, cpu, 0xe201u);
    (void)Lufia2WorldMapSortVisible(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    SimulateJsrFrame(memory, cpu, 0xe204u);
    result = Lufia2WorldMapDrawObjects(memory, cpu);
    if (result.flow == LUFIA2_EXECUTION_BOUNDARY)
        return result;
    SimulateRtsFrame(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, Absolute(wram, SLOT_LIST_COUNT, 0)));
    if (cpu->zero)
        return ExecutionReturned(0x86e280u);
    WramWrite(wram, UPDATE_SLOTS_LEFT, A8(cpu));
    LoadX16(cpu, SLOT_LIST);
    WramWrite16(wram, UPDATE_SLOT_POINTER, cpu->x);
    do {
        SetAccumulatorWidth(cpu, 1);
        LoadX16(cpu, WramRead16(wram, UPDATE_SLOT_POINTER));
        LoadY16(cpu, ReadAbsolute16(wram, SLOT_TILE_BLOCK, cpu->x));
        cpu->accumulator = 0;
        LoadA8(cpu, Read8(memory, Absolute(wram, OBJECT_KIND, cpu->y)));
        AslA8(cpu);
        TransferAToY(cpu);
        LoadX16(cpu, ReadAbsolute16(wram, POOL_STARTS, cpu->y));
        LoadA8(cpu, Read8(memory, Absolute(wram, POOL_SIZES, cpu->y)));
        TransferAToY(cpu);
        found = false;
        for (;;) {
            LoadA8(cpu, WramReadAt(wram, SLOT_USERS, cpu->x));
            if (cpu->zero) {
                found = true;
                break;
            }
            LoadX16(cpu, (uint16_t)(cpu->x + 2u));
            LoadY16(cpu, (uint16_t)(cpu->y - 1u));
            if (cpu->zero)
                break;
        }
        if (found) {
            result = DrawSlotUsers(memory, cpu, wram);
            if (result.flow == LUFIA2_EXECUTION_BOUNDARY)
                return result;
        } else {
            SetAccumulatorWidth(cpu, 0);
            LoadX16(cpu, WramRead16(wram, UPDATE_SLOT_POINTER));
            WramWrite16At(wram, SLOT_USERS, cpu->x, 0);
        }
        WriteModified16(wram, UPDATE_SLOT_POINTER, 0,
            (uint16_t)(WramRead16(wram, UPDATE_SLOT_POINTER) + 1u));
        WriteModified16(wram, UPDATE_SLOT_POINTER, 0,
            (uint16_t)(WramRead16(wram, UPDATE_SLOT_POINTER) + 1u));
        SetAccumulatorWidth(cpu, 1);
        {
            const uint8_t left = (uint8_t)(WramRead(wram, UPDATE_SLOTS_LEFT) - 1u);

            WramWrite(wram, UPDATE_SLOTS_LEFT, left);
            SetNz8(cpu, left);
        }
    } while (!cpu->zero);
    return ExecutionReturned(0x86e280u);
}
