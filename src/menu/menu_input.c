/* Menu cursor input: moving between the items of a grid menu from the
 * pressed buttons, and the loop that polls it once per frame. */

#include <stdbool.h>
#include <stddef.h>

#include "core/cpu_internal.h"
#include "core/plain_ops.h"
#include "core/wram_view.h"
#include "lufia2/menu.h"

enum {
    PRESSED_A = 0x14abu,
    PRESSED_B = 0x14acu,
    ITEM_LIMIT = 0x14c7u,
    ITEM_FLAGS = 0x14c1u,
    ITEM_ROW = 0x14ebu,
    ITEM_ROWS = 0x14f7u,
    ITEM_INDEX = 0x14b3u,
    MENU_ITEM = 0x14a9u,
    BUTTON_UP = 0x08u,
    BUTTON_DOWN = 0x04u,
    BUTTON_LEFT = 0x02u,
    BUTTON_RIGHT = 0x01u,
    EDGE_TOP = 0x80u,
    EDGE_BOTTOM = 0x40u,
    SLOT_SKIP = 5u,
    MOVE_CODE = 1u,
    CURSOR_RETURNED = 0x82889du,
    CURSOR_NONE = 0x82889fu
};

static void ItemIndexCall(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    (void)Lufia2MenuItemIndex(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* The accumulator holds `value` after a bit test against `mask`. */
static void LeaveBitTest(Lufia2CpuState *cpu, uint8_t value, uint8_t mask) {
    LoadA8(cpu, value);
    BitImmediate8(cpu, mask);
}

/* $82:8892: the item's index and position follow the move; A = 1. The caller
 * has left the carry that the first call keeps. */
static Lufia2ExecutionResult MoveDone(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    ItemIndexCall(memory, cpu, 0x8896u);
    SimulateJsrFrame(memory, cpu, 0x8899u);
    (void)Lufia2MenuItemPosition(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    LoadA8(cpu, MOVE_CODE);
    cpu->carry = false;
    return ExecutionReturned(CURSOR_RETURNED);
}

/* The button sound is requested by $80:953B, which stays original code. The
 * carry is whatever the move left; the accumulator holds the sound. */
static Lufia2ExecutionResult Sound(
    Lufia2CpuState *cpu, uint8_t sound, bool carry, uint32_t call) {
    LoadA8(cpu, sound);
    cpu->carry = carry;
    return ExecutionHandoff(cpu, call);
}

/* A pressed direction on the item's own button: index, then the action code
 * in A with carry clear. */
static Lufia2ExecutionResult Action(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t return_address, uint8_t code) {
    ItemIndexCall(memory, cpu, return_address);
    LoadA8(cpu, code);
    cpu->carry = false;
    return ExecutionReturned(CURSOR_RETURNED);
}

/* Moving up, $82:8727. The row steps back; below the first row it either
 * marks the top edge of a scrolling item, wraps to the last row, or (for an
 * item with an index limit) wraps only while the index is past the limit. */
static Lufia2ExecutionResult MoveUp(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram) {
    const uint8_t row = (uint8_t)(WramReadAt(wram, ITEM_ROW, cpu->x) - 1u);
    bool carry = true;

    WramWriteAt(wram, ITEM_ROW, cpu->x, row);
    if ((row & 0x80u) != 0) {
        const uint8_t flags = WramReadAt(wram, ITEM_FLAGS, cpu->x);
        const uint8_t marked = (uint8_t)(flags | EDGE_TOP);

        if (flags != 0) {
            WramWriteAt(wram, ITEM_FLAGS, cpu->x, marked);
            WramWriteAt(wram, ITEM_ROW, cpu->x, 0);
            cpu->carry = carry;
            return MoveDone(memory, cpu);
        }
        WramWriteAt(wram, ITEM_ROW, cpu->x,
            (uint8_t)(WramReadAt(wram, ITEM_ROWS, cpu->x) - 1u));
        if (WramReadAt(wram, ITEM_LIMIT, cpu->x) != 0) {
            Byte8Result past;

            cpu->carry = carry;
            ItemIndexCall(memory, cpu, 0x8752u);
            past = Difference8(WramRead(wram, ITEM_INDEX),
                WramReadAt(wram, ITEM_LIMIT, cpu->x));
            carry = past.carry;
            if ((past.value & 0x80u) != 0)
                return Sound(cpu, 0, carry, 0x8287a8u);
            WramWriteAt(wram, ITEM_ROW, cpu->x, 0);
        }
    }
    return Sound(cpu, 0, carry, 0x828760u);
}

/* Moving down, $82:876B; the mirror of the above. */
static Lufia2ExecutionResult MoveDown(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram) {
    const uint8_t row = (uint8_t)(WramReadAt(wram, ITEM_ROW, cpu->x) + 1u);
    const uint8_t flags = WramReadAt(wram, ITEM_FLAGS, cpu->x);
    Byte8Result past;

    WramWriteAt(wram, ITEM_ROW, cpu->x, row);
    if (flags != 0) {
        past = Difference8(row, WramReadAt(wram, ITEM_ROWS, cpu->x));
        if ((past.value & 0x80u) != 0)
            return Sound(cpu, 0, past.carry, 0x8287a8u);
        WramWriteAt(wram, ITEM_FLAGS, cpu->x, (uint8_t)(flags | EDGE_BOTTOM));
        WramWriteAt(wram, ITEM_ROW, cpu->x, (uint8_t)(row - 1u));
        cpu->carry = past.carry;
        return MoveDone(memory, cpu);
    }
    if (WramReadAt(wram, ITEM_LIMIT, cpu->x) != 0) {
        ItemIndexCall(memory, cpu, 0x8790u);
        past = Difference8(
            WramRead(wram, ITEM_INDEX), WramReadAt(wram, ITEM_LIMIT, cpu->x));
    } else {
        past = Difference8(WramReadAt(wram, ITEM_ROW, cpu->x),
            WramReadAt(wram, ITEM_ROWS, cpu->x));
    }
    if ((past.value & 0x80u) == 0)
        WramWriteAt(wram, ITEM_ROW, cpu->x, 0);
    return Sound(cpu, 0, past.carry, 0x8287a8u);
}

typedef enum {
    RULE_MOVE_UP,
    RULE_MOVE_DOWN,
    RULE_SOUND,
    RULE_ACTION
} ButtonRule;

/* One button of one pad and what it does: the sound to request (and where
 * that request is handed off) or the action code (and the return address of
 * its item lookup). */
typedef struct {
    bool second_pad;
    uint8_t mask;
    ButtonRule rule;
    uint8_t value;
    uint32_t target;
} ButtonEntry;

/* In the order the original tests them; `second_pad` is $14AC, else $14AB. */
static const ButtonEntry kButtons[] = {
    { true, BUTTON_UP, RULE_MOVE_UP, 0, 0 },
    { true, BUTTON_DOWN, RULE_MOVE_DOWN, 0, 0 },
    { true, BUTTON_LEFT, RULE_SOUND, 0, 0x8287b5u },
    { true, BUTTON_RIGHT, RULE_SOUND, 0, 0x8287e6u },
    { false, 0x80u, RULE_ACTION, 2, 0x8814u },
    { true, 0x80u, RULE_SOUND, 1, 0x828823u },
    { false, 0x40u, RULE_ACTION, 4, 0x8838u },
    { true, 0x40u, RULE_ACTION, 5, 0x8847u },
    { false, 0x20u, RULE_SOUND, 0, 0x828856u },
    { false, 0x10u, RULE_SOUND, 0, 0x828868u },
    { true, 0x10u, RULE_ACTION, 8, 0x887du },
    { true, 0x20u, RULE_ACTION, 9, 0x888cu }
};

/* $82:8720: moves the cursor of item X from the buttons just pressed. Carry
 * set (A unchanged) when nothing was pressed; else carry clear with A = 1 for
 * a move or a code of 2 to 9 for the action buttons. The directions that
 * play a button sound hand off at its call. M1. */
Lufia2ExecutionResult Lufia2MenuCursor(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const uint8_t pressed_a = WramRead(wram, PRESSED_A);
    const uint8_t pressed_b = WramRead(wram, PRESSED_B);
    const size_t count = sizeof(kButtons) / sizeof(kButtons[0]);
    size_t i;

    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x828720u);
    for (i = 0; i < count; ++i) {
        const ButtonEntry *button = &kButtons[i];
        const uint8_t pressed = button->second_pad ? pressed_b : pressed_a;

        if ((pressed & button->mask) == 0)
            continue;
        switch (button->rule) {
        case RULE_MOVE_UP:
            return MoveUp(memory, cpu, wram);
        case RULE_MOVE_DOWN:
            return MoveDown(memory, cpu, wram);
        case RULE_SOUND:
            return Sound(cpu, button->value, cpu->carry, button->target);
        default:
            return Action(memory, cpu, (uint16_t)button->target, button->value);
        }
    }
    LeaveBitTest(cpu, pressed_b, kButtons[count - 1u].mask);
    cpu->carry = true;
    return ExecutionReturned(CURSOR_NONE);
}

/* The JSR to a child that may hand off: the frame stays pushed then. */
static int Call(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t return_address, Lufia2ExecutionResult (*child)(
        const Lufia2Memory *, Lufia2CpuState *),
    Lufia2ExecutionResult *result) {
    SimulateJsrFrame(memory, cpu, return_address);
    *result = child(memory, cpu);
    if (result->flow != LUFIA2_EXECUTION_RETURNED)
        return 1;
    SimulateRtsFrame(memory, cpu);
    return 0;
}

/* $82:8B08: one pass of the menu loop. With A not 0, or no button or move,
 * the window request and the cursor blink run and the sprite frame is left
 * to the original (hand off at its JSL). A move to confirm returns A = 1, or
 * 10 and 11 for the item's edge flags. M1X0. */
Lufia2ExecutionResult Lufia2MenuInputLoop(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    Lufia2ExecutionResult result;
    const uint8_t pending = A8(cpu);

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x828b08u);
    Compare8(cpu, pending, 0u);
    if (pending == 0) {
        const uint16_t slot = (uint16_t)(WramRead16(wram, MENU_ITEM) - SLOT_SKIP);
        const uint8_t kept = WramReadAt(wram, ITEM_FLAGS, slot) & 0x01u;

        cpu->x = slot;
        WramWriteAt(wram, ITEM_FLAGS, slot, kept);
        SimulateJslFrame(memory, cpu, 0x82u, 0x8b1fu);
        (void)Lufia2MenuButtons(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        if (!cpu->carry) {
            if (Call(memory, cpu, 0x8b24u, Lufia2MenuCursor, &result))
                return result;
            if (!cpu->carry) {
                uint8_t flags;

                Compare8(cpu, A8(cpu), MOVE_CODE);
                if (!cpu->zero)
                    return ExecutionReturned(0x828b4au);
                flags = WramReadAt(wram, ITEM_FLAGS, cpu->x);
                cpu->carry = (flags & EDGE_TOP) != 0;
                if (cpu->carry) {
                    LoadA8(cpu, 0x0au);
                    return ExecutionReturned(0x828b47u);
                }
                cpu->carry = (flags & EDGE_BOTTOM) != 0;
                if (cpu->carry) {
                    LoadA8(cpu, 0x0bu);
                    return ExecutionReturned(0x828b4au);
                }
                LoadA8(cpu, MOVE_CODE);
                return ExecutionReturned(0x828b44u);
            }
        }
    }
    if (Call(memory, cpu, 0x8b38u, Lufia2MenuWindowRequest, &result))
        return result;
    if (Call(memory, cpu, 0x8b3bu, Lufia2MenuCursorBlink, &result))
        return result;
    return ExecutionHandoff(cpu, 0x828b3cu);
}
