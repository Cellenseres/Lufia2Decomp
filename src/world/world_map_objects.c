/* World map objects: animation steps, the sprite buffer reset and the
 * on-screen test that builds the list of visible objects. */

#include <stdbool.h>

#include "core/cpu_internal.h"
#include "core/plain_ops.h"
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
    OBJECT_SIZE = 0x1du,
    OBJECT_TABLE = 0x1469u
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
    VISIBLE_LIST_START = 0x124fu,
    VISIBLE_OBJECT_LIMIT = 21u,
    VISIBLE_OBJECT_LAST = 0x1ff1u,
    VISIBLE_LIST_LAST = 0x1fd2u,
    VISIBLE_STACK_FIRST = 0x1f00u,
    VISIBLE_STACK_LAST = 0x1ffcu,
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

/* These banks map the object-list work fields to RAM. */
static bool WorldBankHasWorkRam(uint8_t bank) {
    return bank < 0x40u || bank == 0x7eu || bank == 0x7fu ||
        (bank >= 0x80u && bank < 0xc0u);
}

static uint32_t Absolute(
    const Lufia2Wram wram, uint16_t offset, uint16_t index) {
    return (((uint32_t)wram.data_bank << 16) + offset + index) & 0x00ffffffu;
}

static uint16_t ReadAbsolute16(
    const Lufia2Wram wram, uint16_t offset, uint16_t index) {
    const uint32_t low = Absolute(wram, offset, index);

    return Read16Long(wram.memory, low);
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
static void AppendIfOnScreen(
    Lufia2Wram wram, Lufia2CpuState *cpu, uint16_t object) {
    const uint16_t y = WramRead16At(wram, OBJECT_Y, object);
    Word16Result top;
    Word16Result bottom;
    Word16Result left;
    Word16Result right;
    Word16Result screen_y;
    uint16_t count;

    if ((y & 0x8000u) != 0) {
        LeaveWord(cpu, y);
        return;
    }
    top = Difference16Mode(y, WramRead16(wram, CAMERA_Y), cpu->decimal);
    bottom = Sum16Mode(top.value, WramRead16At(wram, OBJECT_HEIGHT, object),
        false, cpu->decimal);
    if (bottom.value >= SCREEN_HEIGHT) {
        LeaveSumCompared(cpu, bottom, SCREEN_HEIGHT);
        return;
    }
    left = Sum16Mode(WramRead16At(wram, OBJECT_X, object), SPRITE_MARGIN,
        false, cpu->decimal);
    left = Difference16Mode(left.value, WramRead16(wram, CAMERA_X), cpu->decimal);
    right = Sum16Mode(left.value, WramRead16At(wram, OBJECT_WIDTH, object),
        false, cpu->decimal);
    if (right.value >= SCREEN_WIDTH) {
        LeaveSumCompared(cpu, right, SCREEN_WIDTH);
        return;
    }
    left = Difference16Mode(WramRead16At(wram, OBJECT_X, object),
        WramRead16(wram, CAMERA_X), cpu->decimal);
    WramWrite16At(wram, OBJECT_SCREEN_X, object, left.value);
    screen_y = Difference16Mode(WramRead16At(wram, OBJECT_Y, object),
        WramRead16(wram, CAMERA_Y), cpu->decimal);
    WramWrite16At(wram, OBJECT_SCREEN_Y, object, screen_y.value);
    WriteAbsolute16(wram, VISIBLE_KEY, cpu->y,
        WramRead16At(wram, OBJECT_Y, object));
    WriteAbsolute16(wram, VISIBLE_OBJECT, cpu->y, object);
    cpu->y = (uint16_t)(cpu->y + 2u);
    count = (uint16_t)(ReadAbsolute16(wram, VISIBLE_COUNT, 0) + 1u);
    WriteModified16(wram, VISIBLE_COUNT, 0, count);
    cpu->carry = screen_y.carry;
    cpu->overflow = screen_y.overflow;
    cpu->accumulator = object;
    LeaveCounter(cpu, count);
}

Lufia2ExecutionResult Lufia2WorldMapTestObject(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit ||
        cpu->direct_page != 0u || !WorldBankHasWorkRam(cpu->data_bank) ||
        cpu->x > VISIBLE_OBJECT_LAST || cpu->y > VISIBLE_LIST_LAST)
        return ExecutionHandoff(cpu, 0x86e295u);
    AppendIfOnScreen(WramViewOfCaller(memory, cpu), cpu, cpu->x);
    return ExecutionReturned(0x86e2d1u);
}

/* $86:E287: tests every object, X stepping through the table. The first
 * object is the caller's X; the count is in $22. */
Lufia2ExecutionResult Lufia2WorldMapTestObjects(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;
    Word16Result next = {0, false, false};
    uint16_t objects_left;

    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit ||
        cpu->direct_page != 0u || !WorldBankHasWorkRam(cpu->data_bank) ||
        cpu->x != OBJECT_TABLE || cpu->y != VISIBLE_LIST_START ||
        cpu->stack < VISIBLE_STACK_FIRST || cpu->stack > VISIBLE_STACK_LAST)
        return ExecutionHandoff(cpu, 0x86e287u);
    wram = WramViewOfCaller(memory, cpu);
    objects_left = WramRead16(wram, OBJECTS_LEFT);
    if (objects_left == 0u || objects_left > VISIBLE_OBJECT_LIMIT)
        return ExecutionHandoff(cpu, 0x86e287u);
    do {
        SimulateJsrFrame(memory, cpu, 0xe289u);
        AppendIfOnScreen(wram, cpu, cpu->x);
        SimulateRtsFrame(memory, cpu);
        next = Sum16Mode(cpu->x, OBJECT_SIZE, false, cpu->decimal);
        cpu->x = next.value;
        objects_left = StepDirect16(wram, OBJECTS_LEFT, -1);
    } while (objects_left != 0);
    LeaveSum(cpu, next);
    LeaveCounter(cpu, objects_left);
    return ExecutionReturned(0x86e294u);
}

/* $86:E640: clears the flag words of the 32 object slots. M1X0. */
Lufia2ExecutionResult Lufia2WorldMapClearSlotFlags(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;
    uint16_t slot = SLOT_TABLE;
    unsigned count;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86e640u);
    wram = WramViewOfCaller(memory, cpu);
    for (count = 0; count < SLOT_COUNT; ++count) {
        WramWrite16At(wram, SLOT_FLAGS, slot, 0);
        slot = (uint16_t)(slot + 2u);
    }
    cpu->x = slot;
    cpu->y = 0;
    LoadA8(cpu, 0);
    return ExecutionReturned(0x86e64fu);
}

/* $86:E650: hides all 128 sprites, clears the OAM high table. */
Lufia2ExecutionResult Lufia2WorldMapClearSprites(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    /* The Y bytes of the eight sprites of an OAM row, from the row start. */
    static const uint8_t kYOffsets[8] = {0x01u, 0x05u, 0x09u, 0x0du,
                                         0x11u, 0x15u, 0x19u, 0x1du};
    Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint16_t at = OAM_BUFFER;
    unsigned row;
    unsigned i;
    bool carry = false;

    Push8(memory, cpu, PackStatus(cpu));
    for (row = 0; row < OAM_ROWS; ++row) {
        for (i = 0; i < 8u; ++i)
            WramWriteAt(wram, kYOffsets[i], at, OAM_HIDDEN_Y);
        const Word16Result next = Sum16Mode(at, OAM_ROW_SIZE, carry, cpu->decimal);
        at = next.value;
        carry = next.carry;
    }
    cpu->accumulator = at;
    for (i = 0; i < OAM_HIGH_TABLE_WORDS; ++i) {
        WramWrite16At(wram, 0, at, 0);
        at = (uint16_t)(at + 2u);
    }
    cpu->x = at;
    cpu->y = 0;
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x86e685u);
}

/* Animation fields of an object, and the layout of the animation data in
 * ROM: a table of animations, each a table of per-pose records. */
enum {
    OBJECT_POSE = 0x0fu,
    OBJECT_ANIMATION = 0x10u,
    OBJECT_FRAME = 0x11u,
    OBJECT_FRAME_COUNT = 0x14u,
    OBJECT_FRAME_END = 0x13u,
    OBJECT_FRAME_RESTART = 0x12u,
    OBJECT_SPRITE_FLAGS = 0x12u,
    OBJECT_TIMER = 0x15u,
    OBJECT_NEXT_ANIMATION = 0x18u,
    OBJECT_STEP_X = 0x0bu,
    OBJECT_STEP_X_HIGH = 0x0cu,
    OBJECT_STEP_Y = 0x0du,
    OBJECT_STEP_Y_HIGH = 0x0eu,
    OBJECT_FRAME_POINTER = 0x19u,
    OBJECT_TILE_BASE = 0x17u,
    ANIMATION_POINTERS = 0xeba7u,
    TABLE_POINTER_SCRATCH = 0x00u,
    FRAME_SIZE = 5u,
    OBJECT_COUNT = 0x16u
};

/* Bytes of an animation record, reached through the data bank. */
enum {
    RECORD_COUNT = 0u,    /* frames in the animation */
    RECORD_RESTART = 1u,
    RECORD_END = 2u,      /* bit 0 set: loop; clear: go on to the next one */
    RECORD_POINTER = 3u,  /* word; bit 15 set: a tile base byte lies there */
    RECORD_STEP_X = 5u,   /* signed */
    RECORD_STEP_Y = 6u,   /* signed */
    RECORD_TIMER = 7u,
    TILE_BASE_MASK = 0x7fu,
    LOOP_BIT = 0x01u
};

static bool IsNegative16(uint16_t value) {
    return (value & 0x8000u) != 0;
}

/* Stores a signed step byte with its sign extension. */
static void StoreStep(
    Lufia2Wram wram, uint16_t object, uint8_t low_field, uint8_t high_field,
    uint8_t step) {
    WramWriteAt(wram, low_field, object, step);
    WramWriteAt(wram, high_field, object, (step & 0x80u) != 0 ? 0xffu : 0u);
}

/* Copies the step and timer of the record into the object; `whole` also
 * copies the frame bounds and restarts the frame counter. Returns the last
 * byte read. */
static uint8_t LoadFrameStep(
    Lufia2Wram wram, uint16_t object, uint16_t record, bool whole) {
    uint8_t byte = Read8(wram.memory, Absolute(wram, RECORD_STEP_X, record));

    StoreStep(wram, object, OBJECT_STEP_X, OBJECT_STEP_X_HIGH, byte);
    byte = Read8(wram.memory, Absolute(wram, RECORD_STEP_Y, record));
    StoreStep(wram, object, OBJECT_STEP_Y, OBJECT_STEP_Y_HIGH, byte);
    byte = Read8(wram.memory, Absolute(wram, RECORD_TIMER, record));
    WramWriteAt(wram, OBJECT_TIMER, object, byte);
    if (whole) {
        byte = Read8(wram.memory, Absolute(wram, RECORD_COUNT, record));
        WramWriteAt(wram, OBJECT_FRAME_COUNT, object, byte);
        byte = Read8(wram.memory, Absolute(wram, RECORD_END, record));
        WramWriteAt(wram, OBJECT_FRAME_END, object, byte);
        WramWriteAt(wram, OBJECT_FRAME, object, 0);
        byte = Read8(wram.memory, Absolute(wram, RECORD_RESTART, record));
        WramWriteAt(wram, OBJECT_FRAME_RESTART, object, byte);
    }
    return byte;
}

/* Finds the record of the object's pose in the table of an animation
 * (`animation_offset` is twice the animation number). The table pointer is
 * left in the scratch word at $00. */
static uint16_t FindPoseRecord(
    Lufia2Wram wram, uint16_t object, uint16_t animation_offset) {
    const uint16_t table =
        ReadAbsolute16(wram, ANIMATION_POINTERS, animation_offset);
    uint16_t pose;

    WramWrite16(wram, TABLE_POINTER_SCRATCH, table);
    pose = (uint16_t)((WramRead16At(wram, OBJECT_POSE, object) & 0x00ffu) << 1);
    return ReadAbsolute16(wram, WramRead16(wram, TABLE_POINTER_SCRATCH), pose);
}

/* The tile base of a frame: the byte a negative record pointer leads to,
 * or zero. */
static uint8_t TileBaseOf(Lufia2Wram wram, uint16_t pointer) {
    const uint8_t base =
        IsNegative16(pointer) ? Read8(wram.memory, Absolute(wram, 0u, pointer)) : 0;

    return (uint8_t)(base & TILE_BASE_MASK);
}

/* $86:E0B9: starts animation A on object X. M1X0. */
Lufia2ExecutionResult Lufia2WorldMapStartAnimation(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;
    uint16_t object;
    uint16_t record;
    uint16_t pointer;
    uint8_t tile_base;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86e0b9u);
    wram = WramViewOfCaller(memory, cpu);
    object = cpu->x;
    WramWriteAt(wram, OBJECT_ANIMATION, object, A8(cpu));
    record = FindPoseRecord(wram, object, (uint16_t)(A8(cpu) << 1));
    pointer = ReadAbsolute16(wram, RECORD_POINTER, record);
    WramWrite16At(wram, OBJECT_FRAME_POINTER, object, pointer);
    (void)LoadFrameStep(wram, object, record, true);
    cpu->y = WramRead16At(wram, OBJECT_FRAME_POINTER, object);
    if (IsNegative16(cpu->y))
        PushStackWord(memory, cpu, object);
    tile_base = TileBaseOf(wram, cpu->y);
    if (IsNegative16(cpu->y))
        (void)PullStackWord(memory, cpu);
    WramWriteAt(wram, OBJECT_TILE_BASE, object, tile_base);
    cpu->accumulator = (uint16_t)((pointer & 0xff00u) | tile_base);
    cpu->carry = false;
    SetNz8(cpu, tile_base);
    return ExecutionReturned(0x86e11au);
}

/* $86:E186: after the frame counter moved, finds the record of the new
 * frame (the product in $4216 is the frame number times the record size) and
 * loads the object's frame pointer and tile base. M1X0. Returns the record. */
static uint16_t NextFrameRecord(
    Lufia2Wram wram, Lufia2CpuState *cpu, uint16_t object) {
    const uint16_t animation_offset =
        (uint16_t)(WramReadAt(wram, OBJECT_ANIMATION, object) << 1);
    const uint16_t pose_record = FindPoseRecord(wram, object, animation_offset);
    const uint16_t record = Sum16Mode(pose_record,
        WramRead16(wram, SNES_RDMPYL), false, cpu->decimal).value;
    const uint16_t pointer = ReadAbsolute16(wram, RECORD_POINTER, record);
    uint8_t tile_base;

    WramWrite16At(wram, OBJECT_FRAME_POINTER, object, pointer);
    PushStackWord(wram.memory, cpu, object);
    tile_base = TileBaseOf(wram, pointer);
    (void)PullStackWord(wram.memory, cpu);
    WramWriteAt(wram, OBJECT_TILE_BASE, object, tile_base);
    return record;
}

/* The timer of the object ran out: moves on to the next frame, looping or
 * starting the follow-up animation at the end. Returns Y as it is left. */
static uint16_t AdvanceFrame(
    Lufia2Wram wram, Lufia2CpuState *cpu, uint16_t object) {
    const Lufia2Memory *memory = wram.memory;
    uint8_t frame = (uint8_t)(WramReadAt(wram, OBJECT_FRAME, object) + 1u);
    uint16_t record;

    if (frame >= WramReadAt(wram, OBJECT_FRAME_COUNT, object)) {
        if ((WramReadAt(wram, OBJECT_FRAME_END, object) & LOOP_BIT) == 0) {
            cpu->x = object;
            LoadA8(cpu, WramReadAt(wram, OBJECT_NEXT_ANIMATION, object));
            SimulateJsrFrame(memory, cpu, 0xe17fu);
            (void)Lufia2WorldMapStartAnimation(memory, cpu);
            SimulateRtsFrame(memory, cpu);
            return cpu->y;
        }
        frame = 0;
    }
    WramWriteAt(wram, OBJECT_FRAME, object, frame);
    WramWrite(wram, SNES_WRMPYA, frame);
    WramWrite(wram, SNES_WRMPYB, FRAME_SIZE);
    SimulateJsrFrame(memory, cpu, 0xe142u);
    record = NextFrameRecord(wram, cpu, object);
    SimulateRtsFrame(memory, cpu);
    (void)LoadFrameStep(wram, object, record, false);
    return record;
}

/* $86:E11F: steps the animation of all 22 objects. A timer of 1 runs out
 * into the next frame record; a positive timer counts down. M1X0. */
Lufia2ExecutionResult Lufia2WorldMapStepAnimations(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;
    Word16Result next = {OBJECT_TABLE, false, false};
    uint16_t object = OBJECT_TABLE;
    uint16_t y = cpu->y;
    uint8_t left = OBJECT_COUNT;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit ||
        !DirectWorkByteAvailable(cpu, OBJECTS_LEFT))
        return ExecutionHandoff(cpu, 0x86e11fu);
    wram = WramViewOfCaller(memory, cpu);
    WramWrite(wram, OBJECTS_LEFT, left);
    do {
        const uint8_t timer = WramReadAt(wram, OBJECT_TIMER, object);

        if (timer == 0 || (timer & 0x80u) != 0)
            WramWriteAt(wram, OBJECT_TIMER, object, timer);
        else if (timer != 1)
            WramWriteAt(wram, OBJECT_TIMER, object, (uint8_t)(timer - 1u));
        else
            y = AdvanceFrame(wram, cpu, object);
        next = Sum16Mode(object, OBJECT_SIZE, false, cpu->decimal);
        object = next.value;
        left = (uint8_t)(WramRead(wram, OBJECTS_LEFT) - 1u);
        WramWrite(wram, OBJECTS_LEFT, left);
    } while (left != 0);
    cpu->x = object;
    cpu->y = y;
    LeaveSum(cpu, next);
    SetNz8(cpu, left);
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

/* Merges the two high bits of the x position into their field of the
 * high-table byte: the keep mask retains the other sprites. */
static uint8_t MergeXBits(Lufia2Wram wram, uint8_t old_byte, unsigned field) {
    static const uint8_t keep_masks[4] = {0xfcu, 0xf3u, 0xcfu, 0x3fu};
    const uint8_t kept = (uint8_t)(old_byte & keep_masks[field]);
    uint8_t bits;

    WramWrite(wram, DRAW_ATTRIBUTES, kept);
    bits = (uint8_t)(WramRead(wram, DRAW_SCREEN_X_HIGH) & 3u);
    return (uint8_t)((bits << (2u * field)) | WramRead(wram, DRAW_ATTRIBUTES));
}

/* The shared body of $86:E5BB and of the copies of it inside $86:E479 and
 * $86:E4E7; they differ only in the return address their indirect call
 * leaves on the stack. The sprite counter picks a 2-bit field (its low
 * bits) of a byte (the rest) of the OAM high table. M0X0. */
static void StoreHighBitsBody(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram,
    uint16_t handler_return) {
    const uint16_t counter = ReadAbsolute16(wram, SPRITE_COUNTER, 0);
    const unsigned field = counter & 3u;
    const uint16_t byte_index = (uint16_t)(counter >> 2);
    const uint32_t table_byte = Absolute(wram, OAM_HIGH_TABLE, byte_index);
    const uint32_t counter_byte = Absolute(wram, SPRITE_COUNTER, 0);
    uint8_t merged = Read8(memory, table_byte);
    uint8_t next;

    SimulateJsrFrame(memory, cpu, handler_return);
    merged = MergeXBits(wram, merged, field);
    SimulateRtsFrame(memory, cpu);
    Write8(memory, table_byte, merged);
    next = (uint8_t)(Read8(memory, counter_byte) + 1u);
    Write8(memory, counter_byte, next);
    cpu->x = (uint16_t)(field << 1);
    cpu->y = byte_index;
    cpu->accumulator = (uint16_t)((byte_index & 0xff00u) | merged);
    cpu->carry = field == 0 ? ((counter >> 1) & 1u) != 0 : false;
    SetNz8(cpu, next);
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

/* Sets bits in the attribute flags byte. */
static void AddAttributeFlags(Lufia2Wram wram, uint8_t bits) {
    WramWrite(wram, DRAW_ATTRIBUTE_FLAGS,
        (uint8_t)(WramRead(wram, DRAW_ATTRIBUTE_FLAGS) | bits));
}

/* Starts the sprite: the base attributes and the OAM slot offset, which is
 * four bytes per sprite counted. Returns the object. */
static uint16_t BeginSprite(Lufia2Wram wram, uint16_t *slot) {
    const uint16_t object = WramRead16(wram, DRAW_OBJECT);

    WramWrite16(wram, DRAW_ATTRIBUTES, DRAW_BASE_ATTRIBUTES);
    *slot = (uint16_t)(ReadAbsolute16(wram, SPRITE_COUNTER, 0) << 2);
    return object;
}

/* The horizontal position of a sprite: the object's screen x moved by its
 * step, mirrored for poses 3 and up, which also sets the mirror attribute. */
static uint16_t SpriteScreenX(Lufia2Wram wram, uint16_t object, bool decimal) {
    const bool mirrored = WramReadAt(wram, OBJECT_POSE, object) >= DRAW_MIRROR_POSE;
    uint16_t position;
    uint16_t step;

    if (mirrored)
        AddAttributeFlags(wram, DRAW_MIRROR_FLAG);
    position = WramRead16At(wram, OBJECT_SCREEN_X, object);
    step = WramRead16At(wram, OBJECT_STEP_X, object);
    return mirrored ? Difference16Mode(position, step, decimal).value :
        Sum16Mode(position, step, false, decimal).value;
}

/* The vertical position: screen y plus the y step, less `bias`. */
static Byte8Result SpriteScreenY(
    Lufia2Wram wram, uint16_t object, uint8_t bias, bool decimal) {
    const uint8_t screen_y = WramReadAt(wram, OBJECT_SCREEN_Y, object);
    const uint8_t height = WramReadAt(wram, OBJECT_HEIGHT, object);

    return Difference8Mode(Sum8Mode(screen_y, height, false, decimal).value,
        bias, decimal);
}

/* The tile word of the sprite: the tile number with the attributes. */
static uint16_t SpriteTile(Lufia2Wram wram) {
    const uint16_t tile = WramRead16(wram, DRAW_TILE);
    const uint16_t attributes = WramRead16(wram, DRAW_ATTRIBUTES);

    return (uint16_t)(tile | attributes);
}

/* $86:E555: writes the pair of hardware sprites of the object at $02 into
 * the OAM buffer slots selected by the sprite counter, mirroring objects
 * whose pose is 3 or more, then stores both x bits. M0X0 only. */
Lufia2ExecutionResult Lufia2WorldMapDrawSpritePair(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;
    uint16_t object;
    uint16_t slot;
    uint16_t screen_x;
    uint8_t top;
    uint16_t tile;
    Word16Result lower_tile;

    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86e555u);
    wram = WramViewOfCaller(memory, cpu);
    object = BeginSprite(wram, &slot);
    screen_x = Difference16Mode(SpriteScreenX(wram, object, cpu->decimal),
        DRAW_X_BIAS, cpu->decimal).value & DRAW_X_MASK;
    WramWrite16(wram, DRAW_SCREEN_X, screen_x);
    Write8(memory, Absolute(wram, OAM_BUFFER, slot), (uint8_t)screen_x);
    Write8(memory, Absolute(wram, OAM_BUFFER + 4u, slot), (uint8_t)screen_x);
    AddAttributeFlags(wram, WramReadAt(wram, OBJECT_SPRITE_FLAGS, object));
    top = SpriteScreenY(wram, object, DRAW_Y_BIAS, cpu->decimal).value;
    Write8(memory, Absolute(wram, OAM_BUFFER + 1u, slot), top);
    Write8(memory, Absolute(wram, OAM_BUFFER + 5u, slot),
        Sum8Mode(top, DRAW_LOWER_ROW, false, cpu->decimal).value);
    tile = SpriteTile(wram);
    WriteAbsolute16(wram, OAM_BUFFER + 2u, slot, tile);
    lower_tile = Sum16Mode(tile, DRAW_NEXT_TILE, false, cpu->decimal);
    WriteAbsolute16(wram, OAM_BUFFER + 6u, slot, lower_tile.value);
    SimulateJsrFrame(memory, cpu, 0xe5b6u);
    (void)Lufia2WorldMapStoreHighBits(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    SimulateJsrFrame(memory, cpu, 0xe5b9u);
    (void)Lufia2WorldMapStoreHighBits(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    cpu->overflow = lower_tile.overflow;
    return ExecutionReturned(0x86e5bau);
}

/* The single sprites differ in how the x position is adjusted, the y bias
 * and the return address their high-bit store leaves. */
static void DrawSingleSprite(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, bool wide,
    uint16_t high_bits_return) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint16_t slot;
    const uint16_t object = BeginSprite(wram, &slot);
    uint16_t screen_x;
    Byte8Result top;

    AddAttributeFlags(wram, WramReadAt(wram, OBJECT_SPRITE_FLAGS, object));
    screen_x = SpriteScreenX(wram, object, cpu->decimal);
    if (wide)
        screen_x = Difference16Mode(screen_x, DRAW_WIDE_X_BIAS,
            cpu->decimal).value | DRAW_WIDE_SIZE_BIT;
    else
        screen_x = Difference16Mode(screen_x, DRAW_X_BIAS,
            cpu->decimal).value & DRAW_X_MASK;
    WramWrite16(wram, DRAW_SCREEN_X, screen_x);
    Write8(memory, Absolute(wram, OAM_BUFFER, slot), (uint8_t)screen_x);
    top = SpriteScreenY(wram, object, wide ? DRAW_Y_BIAS : DRAW_SMALL_Y_BIAS,
        cpu->decimal);
    Write8(memory, Absolute(wram, OAM_BUFFER + 1u, slot), top.value);
    WriteAbsolute16(wram, OAM_BUFFER + 2u, slot, SpriteTile(wram));
    StoreHighBitsBody(memory, cpu, wram, high_bits_return);
    cpu->overflow = top.overflow;
}

/* $86:E479: one 16-pixel-wide sprite for the object at $02 (mirrored from
 * pose 3), then its x bits. M0X0 only. */
Lufia2ExecutionResult Lufia2WorldMapDrawSprite(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86e479u);
    DrawSingleSprite(memory, cpu, true, 0xe4ddu);
    return ExecutionReturned(0x86e4e6u);
}

/* $86:E4E7: one 8-pixel-wide sprite for the object at $02, then its x bits.
 * M0X0 only. */
Lufia2ExecutionResult Lufia2WorldMapDrawSmallSprite(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86e4e7u);
    DrawSingleSprite(memory, cpu, false, 0xe54bu);
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
    SORT_LEFT = 0x22u,
    SORT_STACK_FIRST = 0x1f02u
};

/* Keep the sentinel, keys and object references in one bounded RAM list. */
static bool VisibleSortReady(Lufia2Wram wram, const Lufia2CpuState *cpu) {
    if (cpu->direct_page != 0u || cpu->stack < SORT_STACK_FIRST ||
        cpu->stack > VISIBLE_STACK_LAST || wram.data_bank == 0x7fu ||
        !WorldBankHasWorkRam(wram.data_bank))
        return false;
    return ReadAbsolute16(wram, VISIBLE_COUNT, 0) <= VISIBLE_OBJECT_LIMIT;
}

/* $86:E686: insertion sort of the visible list by its key words, descending.
 * The word before the list is set to $FFFF so the scan stops there. M0X0.
 * The list is reached through the direct page, entry by entry. */
Lufia2ExecutionResult Lufia2WorldMapSortVisible(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;
    uint16_t count;
    uint16_t entry = LIST_FIRST_MOVED;
    uint16_t moved_object = 0;
    uint16_t left;

    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86e686u);
    wram = WramViewOfCaller(memory, cpu);
    if (!VisibleSortReady(wram, cpu))
        return ExecutionHandoff(cpu, 0x86e686u);
    WriteAbsolute16(wram, LIST_SENTINEL, 0, 0xffffu);
    count = ReadAbsolute16(wram, VISIBLE_COUNT, 0);
    if (count <= 1u) {
        LeaveWord(cpu, 0);
        return ExecutionReturned(0x86e6c3u);
    }
    WramWrite16(wram, SORT_LEFT, (uint16_t)(count - 1u));
    do {
        uint16_t scan;

        WramWrite16(wram, SORT_KEY, WramRead16At(wram, SORT_KEY, entry));
        WramWrite16(wram, SORT_OBJECT, WramRead16At(wram, LIST_OBJECTS, entry));
        PushStackWord(memory, cpu, entry);
        /* Shift smaller keys forward until the descending-order position is found. */
        for (scan = (uint16_t)(entry - 2u);; scan = (uint16_t)(scan - 2u)) {
            const uint16_t key = WramRead16At(wram, SORT_KEY, scan);

            if (key >= WramRead16(wram, SORT_KEY))
                break;
            WramWrite16At(wram, 2u, scan, key);
            WramWrite16At(wram, LIST_OBJECTS + 2u, scan,
                WramRead16At(wram, LIST_OBJECTS, scan));
        }
        WramWrite16At(wram, 2u, scan, WramRead16(wram, SORT_KEY));
        moved_object = WramRead16(wram, SORT_OBJECT);
        WramWrite16At(wram, LIST_OBJECTS + 2u, scan, moved_object);
        entry = (uint16_t)(PullStackWord(memory, cpu) + 2u);
        left = StepDirect16(wram, SORT_LEFT, -1);
    } while (left != 0);
    cpu->x = entry;
    cpu->carry = true;
    LeaveWord(cpu, moved_object);
    LeaveCounter(cpu, left);
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

/* One more object uses the pattern of `slot`; returns the new user count. */
static uint16_t AddSlotUser(Lufia2Wram wram, uint16_t slot) {
    const uint16_t users = (uint16_t)(WramRead16At(wram, SLOT_USERS, slot) + 1u);

    WriteModified16(wram, SLOT_USERS, slot, users);
    return users;
}

/* $86:E430: finds or adds the pattern of the object at $02 in the slot list
 * starting at $08 (Y entries), then in the shared list at $1367. Carry set:
 * a new use was recorded for the object (its slot index stored); carry
 * clear: the pattern is already in this frame's pool and $11 holds its tile
 * block. M0X0 only. */
Lufia2ExecutionResult Lufia2WorldMapAssignSlot(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;
    uint16_t object;
    uint16_t pattern;
    uint16_t slot;
    uint16_t remaining;
    uint16_t last_user;
    uint16_t user;
    bool found = false;

    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86e430u);
    wram = WramViewOfCaller(memory, cpu);
    object = WramRead16(wram, DRAW_OBJECT);
    pattern = WramRead16At(wram, OBJECT_PATTERN, object);
    cpu->x = object;
    LeaveWord(cpu, pattern);
    if (!IsNegative16(pattern)) {
        WramWrite16(wram, DRAW_TILE, pattern);
        cpu->carry = false;
        return ExecutionReturned(0x86e478u);
    }
    /* A pattern already in this kind's pool only needs another user. */
    slot = WramRead16(wram, SLOT_POOL_POINTER);
    remaining = cpu->y;
    do {
        if (pattern == WramRead16At(wram, SLOT_PATTERN, slot)) {
            const uint16_t users = AddSlotUser(wram, slot);
            const uint16_t tile = WramRead16At(wram, SLOT_TILE_BLOCK, slot);

            WramWrite16(wram, DRAW_TILE, tile);
            cpu->x = slot;
            cpu->y = remaining;
            LeaveCounter(cpu, users);
            LeaveWord(cpu, tile);
            cpu->carry = false;
            return ExecutionReturned(0x86e478u);
        }
        slot = (uint16_t)(slot + 2u);
        remaining = (uint16_t)(remaining - 1u);
    } while (remaining != 0);
    /* Otherwise look through the patterns of the frame, adding it last. */
    slot = SLOT_LIST;
    remaining = ReadAbsolute16(wram, SLOT_LIST_COUNT, 0);
    while (remaining != 0) {
        if (pattern == WramRead16At(wram, SLOT_PATTERN, slot)) {
            found = true;
            break;
        }
        slot = (uint16_t)(slot + 2u);
        remaining = (uint16_t)(remaining - 1u);
    }
    if (found) {
        (void)AddSlotUser(wram, slot);
        last_user = WramRead16At(wram, SLOT_LAST_USER, slot);
        user = WramRead16(wram, DRAW_OBJECT);
        WramWrite16At(wram, SLOT_LAST_USER, slot, user);
        WriteAbsolute16(wram, OBJECT_NEXT_USER, last_user, user);
        cpu->y = last_user;
    } else {
        WramWrite16At(wram, SLOT_PATTERN, slot, pattern);
        WramWrite16At(wram, SLOT_USERS, slot, 1u);
        user = WramRead16(wram, DRAW_OBJECT);
        WramWrite16At(wram, SLOT_TILE_BLOCK, slot, user);
        WramWrite16At(wram, SLOT_LAST_USER, slot, user);
        WriteModified16(wram, SLOT_LIST_COUNT, 0,
            (uint16_t)(ReadAbsolute16(wram, SLOT_LIST_COUNT, 0) + 1u));
        cpu->y = 0;
    }
    /* Both ways end by recording the slot index in the object. */
    object = WramRead16(wram, DRAW_OBJECT);
    {
        const uint8_t index = Read8(memory, Absolute(wram, SPRITE_COUNTER, 0));

        WramWriteAt(wram, OBJECT_SLOT_INDEX, object, index);
        cpu->x = object;
        cpu->accumulator = (uint16_t)((object & 0xff00u) | index);
        SetNz8(cpu, index);
    }
    cpu->carry = true;
    return ExecutionReturned(0x86e42fu);
}

/* The three object kinds, by the byte at $17: the pool of tile blocks the
 * slots come from, how many blocks it has, how many sprites the kind uses
 * and the sprite writer, with the return addresses the original indirect
 * calls leave. */
enum {
    OBJECT_KIND = 0x17u,
    KIND_POOL_SMALL = 0x12b5u,
    KIND_POOL_SMALL_BLOCKS = 0x0010u,
    KIND_POOL_WIDE = 0x12a5u,
    KIND_POOL_PAIR = 0x12d5u,
    KIND_POOL_BLOCKS = 0x0008u,
    PLAYER_OBJECT = 0x16cau
};

typedef Lufia2ExecutionResult (*SpriteWriter)(
    const Lufia2Memory *, Lufia2CpuState *);

typedef struct {
    uint16_t pool;
    uint16_t blocks;
    unsigned sprites;
    uint16_t assign_return;
    uint16_t draw_return;
    SpriteWriter draw;
} ObjectKind;

static const ObjectKind kObjectKinds[] = {
    {KIND_POOL_WIDE, KIND_POOL_BLOCKS, 1u, 0xe400u, 0xe405u,
        Lufia2WorldMapDrawSprite},
    {KIND_POOL_SMALL, KIND_POOL_SMALL_BLOCKS, 1u, 0xe3efu, 0xe3f4u,
        Lufia2WorldMapDrawSmallSprite},
    {KIND_POOL_PAIR, KIND_POOL_BLOCKS, 2u, 0xe415u, 0xe41au,
        Lufia2WorldMapDrawSpritePair},
};
#define KIND_COUNT (sizeof kObjectKinds / sizeof kObjectKinds[0])

/* Takes the pattern slot of the object, then draws it, or only counts its
 * sprites when the pattern is new (its first user draws it later). */
static void DrawObjectKind(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram,
    const ObjectKind *kind) {
    unsigned i;

    WramWrite16(wram, SLOT_POOL_POINTER, kind->pool);
    cpu->x = kind->pool;
    cpu->y = kind->blocks;
    SimulateJsrFrame(memory, cpu, kind->assign_return);
    (void)Lufia2WorldMapAssignSlot(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    if (cpu->carry) {
        for (i = 0; i < kind->sprites; ++i) {
            const uint16_t counter =
                (uint16_t)(ReadAbsolute16(wram, SPRITE_COUNTER, 0) + 1u);

            WriteModified16(wram, SPRITE_COUNTER, 0, counter);
            LeaveCounter(cpu, counter);
        }
        return;
    }
    SimulateJsrFrame(memory, cpu, kind->draw_return);
    (void)kind->draw(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $86:E3D2: draws the object at X by its kind. M0X0 only. An unknown kind
 * hands the original dispatch back at its indirect call. */
Lufia2ExecutionResult Lufia2WorldMapDrawObjectByKind(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;
    uint16_t kind_offset;

    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86e3d2u);
    wram = WramViewOfCaller(memory, cpu);
    WramWrite16(wram, DRAW_OBJECT, cpu->x);
    kind_offset = (uint16_t)((WramRead16At(wram, OBJECT_KIND, cpu->x) & 0x00ffu) << 1);
    cpu->x = kind_offset;
    cpu->carry = false;
    LeaveWord(cpu, kind_offset);
    if (kind_offset >= 2u * KIND_COUNT)
        return ExecutionHandoff(cpu, 0x86e3dbu);
    SimulateJsrFrame(memory, cpu, 0xe3ddu);
    DrawObjectKind(memory, cpu, wram, &kObjectKinds[kind_offset >> 1]);
    SimulateRtsFrame(memory, cpu);
    return ExecutionReturned(0x86e3deu);
}

/* $86:E3CE: the object draw entry; a call to $E3D2. M0X0 only. */
static Lufia2ExecutionResult DrawObject(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;

    SimulateJsrFrame(memory, cpu, 0xe3d0u);
    result = Lufia2WorldMapDrawObjectByKind(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
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
    uint16_t count;
    uint16_t entry = LIST_KEYS;
    uint16_t left;
    uint16_t player_y;

    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86e3abu);
    wram = WramViewOfCaller(memory, cpu);
    count = ReadAbsolute16(wram, VISIBLE_COUNT, 0);
    LeaveWord(cpu, count);
    if (count != 0) {
        WramWrite16(wram, SORT_LEFT, count);
        do {
            cpu->y = entry;
            PushStackWord(memory, cpu, entry);
            cpu->x = ReadAbsolute16(wram, LIST_OBJECTS, entry);
            SimulateJsrFrame(memory, cpu, 0xe3bbu);
            result = DrawObject(memory, cpu);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
            SimulateRtsFrame(memory, cpu);
            entry = (uint16_t)(PullStackWord(memory, cpu) + 2u);
            cpu->y = entry;
            left = StepDirect16(wram, SORT_LEFT, -1);
            LeaveCounter(cpu, left);
        } while (left != 0);
    }
    cpu->x = PLAYER_OBJECT;
    player_y = WramRead16At(wram, OBJECT_Y, PLAYER_OBJECT);
    cpu->y = player_y;
    LeaveCounter(cpu, player_y);
    if (!IsNegative16(player_y)) {
        SimulateJsrFrame(memory, cpu, 0xe3ccu);
        result = DrawObject(memory, cpu);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
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

/* $86:A5A9: restoring division; quotient replaces the dividend. */
/* A zero divisor yields an all-ones quotient. */
Lufia2ExecutionResult Lufia2WorldMapDivide32(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint16_t low;
    uint16_t high;
    uint16_t remainder = 0;
    unsigned bit;

    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    for (bit = 0; bit < DIVISION_BITS; ++bit) {
        bool low_out;
        bool shifted_out;
        const bool carry = (remainder >> 15) != 0u;

        low = WramRead16(wram, DIVIDEND_LOW);
        low_out = (low >> 15) != 0u;
        low = (uint16_t)(low << 1);
        WriteModified16(wram, DIVIDEND_LOW, 0, low);
        high = WramRead16(wram, DIVIDEND_HIGH);
        shifted_out = (high >> 15) != 0u;
        high = (uint16_t)((high << 1) | (low_out ? 1u : 0u));
        WriteModified16(wram, DIVIDEND_HIGH, 0, high);
        remainder = (uint16_t)((remainder << 1) | (shifted_out ? 1u : 0u));
        if (carry || remainder >= WramRead16(wram, DIVISOR)) {
            remainder = Difference16Mode(remainder,
                WramRead16(wram, DIVISOR), cpu->decimal).value;
            low = (uint16_t)(WramRead16(wram, DIVIDEND_LOW) + 1u);
            WriteModified16(wram, DIVIDEND_LOW, 0, low);
        }
    }
    cpu->accumulator = remainder;
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
    PROJECTION_FIRST_PASS_BIAS = 0x0100u,
    TILT_WORD = 0x12u,
    TILT_DEPTH = 0x01u,
    CAMERA_BOTTOM_OFFSET = 0x70u
};

/* $86:E356: scales the distance in $4E by the two depth factors $12 and $11
 * with the hardware multiplier, then divides the 32-bit result. The sum of
 * the second pass is what the division starts from, and the flags it left
 * are the ones the division returns with. M0X0. */
static void ProjectDistance(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram) {
    const uint8_t factors[2] = {WramRead(wram, PROJECTION_SCALE_NEAR),
                                WramRead(wram, PROJECTION_SCALE)};
    Word16Result sum = {0, false, false};
    unsigned pass;

    for (pass = 0; pass < 2u; ++pass) {
        WramWrite(wram, SNES_WRMPYA, factors[pass]);
        WramWrite(wram, SNES_WRMPYB, WramRead(wram, PROJECTION_DISTANCE));
        WramWrite(wram, PROJECTION_PRODUCT_TOP, 0);
        WramWrite16(wram, PROJECTION_PRODUCT, WramRead16(wram, SNES_RDMPYL));
        WramWrite(wram, SNES_WRMPYB, WramRead(wram, PROJECTION_DISTANCE_HIGH));
        const uint16_t product_high = WramRead16(wram, PROJECTION_PRODUCT_HIGH);
        const uint16_t product = WramRead16(wram, SNES_RDMPYL);
        sum = Sum16Mode(product_high, product, false, cpu->decimal);
        if (pass == 0u) {
            sum = Sum16Mode(sum.value, PROJECTION_FIRST_PASS_BIAS,
                sum.carry, cpu->decimal);
            WramWrite16(wram, PROJECTION_ROW_X, sum.value);
        } else {
            WramWrite16(wram, PROJECTION_COLUMN, sum.value);
        }
    }
    WramWrite16(wram, DIVIDEND_LOW, 0);
    LeaveSum(cpu, sum);
    SimulateJsrFrame(memory, cpu, 0xe3a9u);
    (void)Lufia2WorldMapDivide32(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* The part of $86:E2D2 for an object with a negative tilt word: projects
 * its distance below the camera and appends it to the visible list at Y. */
static void ProjectObject(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram,
    uint16_t object) {
    Word16Result below;
    Word16Result row;
    Word16Result column;
    uint16_t count;
    const uint16_t y = WramRead16At(wram, OBJECT_Y, object);

    if (IsNegative16(y))
        return;
    below = Difference16Mode(y, WramRead16(wram, CAMERA_BOTTOM), cpu->decimal);
    if (below.carry)
        return;
    WramWrite16(wram, PROJECTION_DISTANCE, (uint16_t)(0u - below.value));
    SimulateJsrFrame(memory, cpu, 0xe31eu);
    ProjectDistance(memory, cpu, wram);
    SimulateRtsFrame(memory, cpu);
    row = Difference16Mode(PROJECTION_ROW_LIMIT,
        WramRead16(wram, TILT_DEPTH), cpu->decimal);
    if (!row.carry)
        return;
    WramWrite16At(wram, OBJECT_SCREEN_Y, object, row.value);
    const uint16_t x = WramRead16At(wram, OBJECT_X, object);
    column = Difference16Mode(x, WramRead16(wram, CAMERA_ROW), cpu->decimal);
    if (column.value >= PROJECTION_LIMIT)
        return;
    WramWrite16At(wram, OBJECT_SCREEN_X, object, PROJECTION_CENTRE);
    WriteAbsolute16(wram, VISIBLE_KEY, cpu->y,
        WramRead16At(wram, OBJECT_Y, object));
    WriteAbsolute16(wram, VISIBLE_OBJECT, cpu->y, object);
    cpu->y = (uint16_t)(cpu->y + 2u);
    count = (uint16_t)(ReadAbsolute16(wram, VISIBLE_COUNT, 0) + 1u);
    WriteModified16(wram, VISIBLE_COUNT, 0, count);
}

/* The two depth factors of the horizon row: the far one from the row after
 * next, the near one from the rows left below the limit. */
static uint8_t DepthFactor(const Lufia2Memory *memory, uint16_t row_offset) {
    return Read8(memory, (DEPTH_TABLE + row_offset) & 0x00ffffffu);
}

/* $86:E2D2: the object visibility test of the tilted map. Objects with a
 * negative tilt word are projected by their distance below the horizon row;
 * the others use the plain test. A horizon row beyond the table limit uses
 * the plain test for every object. M0X0 only. */
Lufia2ExecutionResult Lufia2WorldMapProjectObjects(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;
    uint16_t horizon;
    uint16_t object;
    uint16_t left;
    Word16Result next = {0, false, false};

    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit ||
        !DirectWorkWordAvailable(cpu, OBJECTS_LEFT))
        return ExecutionHandoff(cpu, 0x86e2d2u);
    wram = WramViewOfCaller(memory, cpu);
    horizon = (uint16_t)(ReadAbsolute16(wram, VIEW_HORIZON, 0) & 0x00ffu);
    if (horizon >= VIEW_HORIZON_LIMIT)
        return Lufia2WorldMapTestObjects(memory, cpu);
    PushStackWord(memory, cpu, cpu->x);
    WramWrite(wram, PROJECTION_SCALE,
        DepthFactor(memory, (uint16_t)((horizon + 2u) << 1)));
    WramWrite(wram, PROJECTION_SCALE_NEAR, DepthFactor(memory,
        (uint8_t)(Difference8Mode(VIEW_HORIZON_LIMIT,
            Read8(memory, Absolute(wram, VIEW_HORIZON, 0)), cpu->decimal).value << 1)));
    object = PullStackWord(memory, cpu);
    WramWrite16(wram, CAMERA_ROW, ReadAbsolute16(wram, VIEW_X, 0));
    WramWrite16(wram, CAMERA_BOTTOM,
        Sum16Mode(ReadAbsolute16(wram, VIEW_Y, 0), CAMERA_BOTTOM_OFFSET,
            false, cpu->decimal).value);
    do {
        if (IsNegative16(WramRead16At(wram, TILT_WORD, object))) {
            ProjectObject(memory, cpu, wram, object);
        } else {
            uint16_t return_address;

            cpu->x = object;
            SimulateJsrFrame(memory, cpu, 0xe353u);
            AppendIfOnScreen(wram, cpu, object);
            return_address = PullStackWord(memory, cpu);
            if (return_address != 0xe353u)
                return ExecutionHandoff(cpu,
                    0x860000u | (uint16_t)(return_address + 1u));
        }
        next = Sum16Mode(object, OBJECT_SIZE, false, cpu->decimal);
        object = next.value;
        left = StepDirect16(wram, OBJECTS_LEFT, -1);
    } while (left != 0);
    cpu->x = object;
    LeaveSum(cpu, next);
    LeaveCounter(cpu, left);
    return ExecutionReturned(0x86e350u);
}

/* Per-frame object update of the world map: the pools of tile blocks by
 * kind (start of the pool in the direct page, number of blocks) and the
 * direct-page scratch the loop below uses. */
enum {
    POOL_STARTS = 0xeb9bu,
    POOL_SIZES = 0xeba1u,
    UPDATE_OBJECT_COUNT = VISIBLE_OBJECT_LIMIT,
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
 * just taken from the pool (`slot`). The users are chained by their
 * next-user field. M0X0 on entry and exit; unknown object kinds hand the
 * original dispatch back. */
static Lufia2ExecutionResult DrawSlotUsers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram,
    uint16_t slot) {
    const uint16_t pointer = WramRead16(wram, UPDATE_SLOT_POINTER);
    const uint16_t users = ReadAbsolute16(wram, SLOT_USERS, pointer);
    uint16_t tile_block;
    Word16Result block_address;
    uint16_t object;
    uint16_t next_user;
    uint16_t left;

    SetAccumulatorWidth(cpu, 0);
    cpu->y = pointer;
    WramWrite16At(wram, SLOT_USERS, slot, users);
    WramWrite16(wram, UPDATE_BLOCKS_LEFT, users);
    tile_block = WramRead16At(wram, SLOT_TILE_BLOCK, slot);
    WramWrite16(wram, DRAW_TILE, tile_block);
    /* The block address: sixteen bytes a tile block on top of the base, and
     * the last bit shifted out of the tile block goes in as a carry. */
    block_address = Sum16Mode((uint16_t)(tile_block << 4), SLOT_BLOCK_BASE,
        ((tile_block >> 12) & 1u) != 0, cpu->decimal);
    WriteAbsolute16(wram, SLOT_USERS, pointer, block_address.value);
    WramWrite16At(wram, SLOT_PATTERN, slot,
        ReadAbsolute16(wram, SLOT_PATTERN, pointer));
    object = ReadAbsolute16(wram, SLOT_TILE_BLOCK, pointer);
    do {
        uint16_t kind_offset;

        WramWrite16(wram, DRAW_OBJECT, object);
        WriteAbsolute16(wram, SPRITE_COUNTER, 0,
            (uint16_t)(WramRead16At(wram, OBJECT_SLOT_INDEX, object) & 0x00ffu));
        kind_offset = (uint16_t)(
            (WramRead16At(wram, OBJECT_KIND, object) & 0x00ffu) << 1);
        if (kind_offset >= 2u * KIND_COUNT) {
            cpu->x = kind_offset;
            cpu->carry = false;
            LeaveWord(cpu, kind_offset);
            return ExecutionHandoff(cpu, 0x86e26bu);
        }
        SimulateJsrFrame(memory, cpu, 0xe26du);
        (void)kObjectKinds[kind_offset >> 1].draw(memory, cpu);
        SimulateRtsFrame(memory, cpu);
        cpu->x = WramRead16(wram, DRAW_OBJECT);
        next_user = WramRead16At(wram, OBJECT_NEXT_USER, cpu->x);
        left = StepDirect16(wram, UPDATE_BLOCKS_LEFT, -1);
        object = next_user;
    } while (left != 0);
    LeaveWord(cpu, next_user);
    LeaveCounter(cpu, left);
    return ExecutionReturned(0x86e274u);
}

/* Looks in the pool of the slot's object kind for a block nobody uses.
 * Returns whether one was found and leaves it in `*block`. The last user
 * count looked at stays in A, and the top bit of the kind byte in the carry,
 * as the caller leaves them. */
static bool FindFreeBlock(
    Lufia2Wram wram, Lufia2CpuState *cpu, uint16_t slot_pointer,
    uint16_t *block) {
    const uint16_t owner = ReadAbsolute16(wram, SLOT_TILE_BLOCK, slot_pointer);
    const uint8_t kind = Read8(wram.memory, Absolute(wram, OBJECT_KIND, owner));
    const uint8_t pool_index = (uint8_t)(kind << 1);
    uint16_t candidate = ReadAbsolute16(wram, POOL_STARTS, pool_index);
    uint16_t left = Read8(wram.memory, Absolute(wram, POOL_SIZES, pool_index));
    uint8_t users;

    cpu->carry = (kind & 0x80u) != 0;
    for (;;) {
        users = WramReadAt(wram, SLOT_USERS, candidate);
        cpu->accumulator = users;
        if (users == 0) {
            *block = candidate;
            return true;
        }
        candidate = (uint16_t)(candidate + 2u);
        left = (uint16_t)(left - 1u);
        if (left == 0) {
            cpu->y = 0;
            return false;
        }
    }
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
    uint16_t slot_pointer;
    uint8_t slots_left;
    bool tilted;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit ||
        !DirectWorkWordAvailable(cpu, OBJECTS_LEFT) ||
        !DirectWorkByteAvailable(cpu, UPDATE_SLOTS_LEFT) ||
        !DirectWorkWordAvailable(cpu, UPDATE_BLOCKS_LEFT))
        return ExecutionHandoff(cpu, 0x86e1b9u);
    wram = WramViewOfCaller(memory, cpu);
    SimulateJsrFrame(memory, cpu, 0xe1bbu);
    result = Lufia2WorldMapClearSprites(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    SimulateRtsFrame(memory, cpu);
    SimulateJsrFrame(memory, cpu, 0xe1beu);
    result = Lufia2WorldMapClearSlotFlags(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    SimulateRtsFrame(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    WriteAbsolute16(wram, VISIBLE_COUNT, 0, 0);
    WriteAbsolute16(wram, SPRITE_COUNTER, 0, 0);
    WriteAbsolute16(wram, SLOT_LIST_COUNT, 0, 0);
    WramWrite16(wram, OBJECTS_LEFT, UPDATE_OBJECT_COUNT);
    cpu->x = OBJECT_TABLE;
    cpu->y = LIST_KEYS;
    WramWrite16(wram, CAMERA_X,
        Difference16Mode(ReadAbsolute16(wram, VIEW_X, 0), CAMERA_LEFT, cpu->decimal).value);
    WramWrite16(wram, CAMERA_Y,
        Difference16Mode(ReadAbsolute16(wram, VIEW_Y, 0), CAMERA_TOP, cpu->decimal).value);
    tilted = Read8(memory, Absolute(wram, VIEW_MODE_PLAIN, 0)) == 0 &&
             Read8(memory, Absolute(wram, VIEW_MODE_TILTED, 0)) != 0;
    SimulateJsrFrame(memory, cpu, tilted ? 0xe1feu : 0xe1f7u);
    if (tilted)
        result = Lufia2WorldMapProjectObjects(memory, cpu);
    else
        result = Lufia2WorldMapTestObjects(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    SimulateRtsFrame(memory, cpu);
    SimulateJsrFrame(memory, cpu, 0xe201u);
    result = Lufia2WorldMapSortVisible(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    SimulateRtsFrame(memory, cpu);
    SimulateJsrFrame(memory, cpu, 0xe204u);
    result = Lufia2WorldMapDrawObjects(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    SimulateRtsFrame(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    slots_left = Read8(memory, Absolute(wram, SLOT_LIST_COUNT, 0));
    LoadA8(cpu, slots_left);
    if (slots_left == 0)
        return ExecutionReturned(0x86e280u);
    WramWrite(wram, UPDATE_SLOTS_LEFT, slots_left);
    WramWrite16(wram, UPDATE_SLOT_POINTER, SLOT_LIST);
    do {
        uint16_t block;

        SetAccumulatorWidth(cpu, 1);
        slot_pointer = WramRead16(wram, UPDATE_SLOT_POINTER);
        cpu->accumulator = 0;
        if (FindFreeBlock(wram, cpu, slot_pointer, &block)) {
            result = DrawSlotUsers(memory, cpu, wram, block);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
        } else {
            SetAccumulatorWidth(cpu, 0);
            cpu->x = WramRead16(wram, UPDATE_SLOT_POINTER);
            WramWrite16At(wram, SLOT_USERS, cpu->x, 0);
        }
        WriteModified16(wram, UPDATE_SLOT_POINTER, 0,
            (uint16_t)(WramRead16(wram, UPDATE_SLOT_POINTER) + 1u));
        WriteModified16(wram, UPDATE_SLOT_POINTER, 0,
            (uint16_t)(WramRead16(wram, UPDATE_SLOT_POINTER) + 1u));
        SetAccumulatorWidth(cpu, 1);
        slots_left = (uint8_t)(WramRead(wram, UPDATE_SLOTS_LEFT) - 1u);
        WramWrite(wram, UPDATE_SLOTS_LEFT, slots_left);
        SetNz8(cpu, slots_left);
    } while (slots_left != 0);
    return ExecutionReturned(0x86e280u);
}
