/* Menu cursor input: moving between the items of a grid menu from the
 * pressed buttons, and the loop that polls it once per frame. */

#include "core/cpu_internal.h"
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
    CURSOR_RETURNED = 0x82889du,
    CURSOR_NONE = 0x82889fu
};

static void ItemIndexCall(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    (void)Lufia2MenuItemIndex(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $82:8892: the item's index and position follow the move; A = 1. */
static Lufia2ExecutionResult MoveDone(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    ItemIndexCall(memory, cpu, 0x8896u);
    SimulateJsrFrame(memory, cpu, 0x8899u);
    (void)Lufia2MenuItemPosition(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    LoadA8(cpu, 1u);
    cpu->carry = 0;
    return ExecutionReturned(CURSOR_RETURNED);
}

/* The button sound is requested by $80:953B, which stays original code. */
static Lufia2ExecutionResult Sound(
    Lufia2CpuState *cpu, uint8_t sound, uint32_t call) {
    LoadA8(cpu, sound);
    return ExecutionHandoff(cpu, call);
}

/* A pressed direction on the item's own button: index, then the action code
 * in A with carry clear. */
static Lufia2ExecutionResult Action(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t return_address, uint8_t code) {
    ItemIndexCall(memory, cpu, return_address);
    LoadA8(cpu, code);
    cpu->carry = 0;
    return ExecutionReturned(CURSOR_RETURNED);
}

/* Moving up, $82:8727. */
static Lufia2ExecutionResult MoveUp(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram) {
    uint8_t row = (uint8_t)(WramReadAt(wram, ITEM_ROW, cpu->x) - 1u);

    WramWriteAt(wram, ITEM_ROW, cpu->x, row);
    LoadA8(cpu, WramReadAt(wram, ITEM_ROW, cpu->x));
    Compare8(cpu, A8(cpu), 0u);
    if (cpu->negative) {
        LoadA8(cpu, WramReadAt(wram, ITEM_FLAGS, cpu->x));
        if (!cpu->zero) {
            LoadA8(cpu, WramReadAt(wram, ITEM_FLAGS, cpu->x));
            Or8(cpu, EDGE_TOP);
            WramWriteAt(wram, ITEM_FLAGS, cpu->x, A8(cpu));
            WramWriteAt(wram, ITEM_ROW, cpu->x, 0);
            return MoveDone(memory, cpu);
        }
        LoadA8(cpu, WramReadAt(wram, ITEM_ROWS, cpu->x));
        DecrementA8(cpu);
        WramWriteAt(wram, ITEM_ROW, cpu->x, A8(cpu));
        LoadA8(cpu, WramReadAt(wram, ITEM_LIMIT, cpu->x));
        if (!cpu->zero) {
            ItemIndexCall(memory, cpu, 0x8752u);
            LoadA8(cpu, WramRead(wram, ITEM_INDEX));
            Compare8(cpu, A8(cpu), WramReadAt(wram, ITEM_LIMIT, cpu->x));
            if (cpu->negative)
                return Sound(cpu, 0, 0x8287a8u);
            WramWriteAt(wram, ITEM_ROW, cpu->x, 0);
        }
    }
    return Sound(cpu, 0, 0x828760u);
}

/* Moving down, $82:876B. */
static Lufia2ExecutionResult MoveDown(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram) {
    const uint8_t row = (uint8_t)(WramReadAt(wram, ITEM_ROW, cpu->x) + 1u);

    WramWriteAt(wram, ITEM_ROW, cpu->x, row);
    LoadA8(cpu, WramReadAt(wram, ITEM_FLAGS, cpu->x));
    if (!cpu->zero) {
        LoadA8(cpu, WramReadAt(wram, ITEM_ROW, cpu->x));
        Compare8(cpu, A8(cpu), WramReadAt(wram, ITEM_ROWS, cpu->x));
        if (cpu->negative)
            return Sound(cpu, 0, 0x8287a8u);
        LoadA8(cpu, WramReadAt(wram, ITEM_FLAGS, cpu->x));
        Or8(cpu, EDGE_BOTTOM);
        WramWriteAt(wram, ITEM_FLAGS, cpu->x, A8(cpu));
        WramWriteAt(wram, ITEM_ROW, cpu->x,
            (uint8_t)(WramReadAt(wram, ITEM_ROW, cpu->x) - 1u));
        return MoveDone(memory, cpu);
    }
    LoadA8(cpu, WramReadAt(wram, ITEM_LIMIT, cpu->x));
    if (!cpu->zero) {
        ItemIndexCall(memory, cpu, 0x8790u);
        LoadA8(cpu, WramRead(wram, ITEM_INDEX));
        Compare8(cpu, A8(cpu), WramReadAt(wram, ITEM_LIMIT, cpu->x));
        if (cpu->negative)
            return Sound(cpu, 0, 0x8287a8u);
    } else {
        LoadA8(cpu, WramReadAt(wram, ITEM_ROW, cpu->x));
        Compare8(cpu, A8(cpu), WramReadAt(wram, ITEM_ROWS, cpu->x));
        if (cpu->negative)
            return Sound(cpu, 0, 0x8287a8u);
    }
    WramWriteAt(wram, ITEM_ROW, cpu->x, 0);
    return Sound(cpu, 0, 0x8287a8u);
}

/* $82:8720: moves the cursor of item X from the buttons just pressed. Carry
 * set (A unchanged) when nothing was pressed; else carry clear with A = 1 for
 * a move or a code of 2 to 9 for the action buttons. The directions that
 * play a button sound hand off at its call. M1. */
Lufia2ExecutionResult Lufia2MenuCursor(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);

    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x828720u);
    LoadA8(cpu, WramRead(wram, PRESSED_B));
    BitImmediate8(cpu, BUTTON_UP);
    if (!cpu->zero)
        return MoveUp(memory, cpu, wram);
    BitImmediate8(cpu, BUTTON_DOWN);
    if (!cpu->zero)
        return MoveDown(memory, cpu, wram);
    BitImmediate8(cpu, BUTTON_LEFT);
    if (!cpu->zero)
        return Sound(cpu, 0, 0x8287b5u);
    BitImmediate8(cpu, BUTTON_RIGHT);
    if (!cpu->zero)
        return Sound(cpu, 0, 0x8287e6u);
    LoadA8(cpu, WramRead(wram, PRESSED_A));
    BitImmediate8(cpu, 0x80u);
    if (!cpu->zero)
        return Action(memory, cpu, 0x8814u, 2u);
    LoadA8(cpu, WramRead(wram, PRESSED_B));
    BitImmediate8(cpu, 0x80u);
    if (!cpu->zero)
        return Sound(cpu, 1, 0x828823u);
    LoadA8(cpu, WramRead(wram, PRESSED_A));
    BitImmediate8(cpu, 0x40u);
    if (!cpu->zero)
        return Action(memory, cpu, 0x8838u, 4u);
    LoadA8(cpu, WramRead(wram, PRESSED_B));
    BitImmediate8(cpu, 0x40u);
    if (!cpu->zero)
        return Action(memory, cpu, 0x8847u, 5u);
    LoadA8(cpu, WramRead(wram, PRESSED_A));
    BitImmediate8(cpu, 0x20u);
    if (!cpu->zero)
        return Sound(cpu, 0, 0x828856u);
    BitImmediate8(cpu, 0x10u);
    if (!cpu->zero)
        return Sound(cpu, 0, 0x828868u);
    LoadA8(cpu, WramRead(wram, PRESSED_B));
    BitImmediate8(cpu, 0x10u);
    if (!cpu->zero)
        return Action(memory, cpu, 0x887du, 8u);
    LoadA8(cpu, WramRead(wram, PRESSED_B));
    BitImmediate8(cpu, 0x20u);
    if (!cpu->zero)
        return Action(memory, cpu, 0x888cu, 9u);
    cpu->carry = 1;
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

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x828b08u);
    Compare8(cpu, A8(cpu), 0u);
    if (cpu->zero) {
        LoadX16(cpu, WramRead16(wram, MENU_ITEM));
        for (int i = 0; i < (int)SLOT_SKIP; ++i)
            LoadX16(cpu, (uint16_t)(cpu->x - 1u));
        LoadA8(cpu, WramReadAt(wram, ITEM_FLAGS, cpu->x));
        And8(cpu, 0x01u);
        WramWriteAt(wram, ITEM_FLAGS, cpu->x, A8(cpu));
        SimulateJslFrame(memory, cpu, 0x82u, 0x8b1fu);
        (void)Lufia2MenuButtons(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        if (!cpu->carry) {
            if (Call(memory, cpu, 0x8b24u, Lufia2MenuCursor, &result))
                return result;
            if (!cpu->carry) {
                Compare8(cpu, A8(cpu), 1u);
                if (!cpu->zero)
                    return ExecutionReturned(0x828b4au);
                LoadA8(cpu, WramReadAt(wram, ITEM_FLAGS, cpu->x));
                AslA8(cpu);
                if (cpu->carry) {
                    LoadA8(cpu, 0x0au);
                    return ExecutionReturned(0x828b47u);
                }
                AslA8(cpu);
                if (cpu->carry) {
                    LoadA8(cpu, 0x0bu);
                    return ExecutionReturned(0x828b4au);
                }
                LoadA8(cpu, 1u);
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
