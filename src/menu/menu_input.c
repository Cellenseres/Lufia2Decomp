/* Grid menu cursor input and its once-per-frame polling loop. */

#include <stdbool.h>
#include <stddef.h>

#include "core/child_call.h"
#include "core/cpu_internal.h"
#include "core/plain_ops.h"
#include "core/wram_view.h"
#include "lufia2/menu.h"

enum {
    CURSOR_SLOT = 0x14a9u,
    PRESSED_A = 0x14abu,
    PRESSED_B = 0x14acu,
    LIST_INDEX = 0x14b3u,
    ITEM_FLAGS = 0x14c1u,
    ITEM_LIMIT = 0x14c7u,
    ITEM_COLUMN = 0x14d9u,
    ITEM_ROW = 0x14ebu,
    ITEM_COLUMNS = 0x14f1u,
    ITEM_ROWS = 0x14f7u,
    REDRAW_FLAGS = 0x74u,
    FIRST_CURSOR_SLOT = 5u,
    ITEM_LAST = 0x00ffu,                /* item tables end below $1610 */
    BUTTON_UP = 0x08u,
    BUTTON_DOWN = 0x04u,
    BUTTON_LEFT = 0x02u,
    BUTTON_RIGHT = 0x01u,
    EDGE_TOP = 0x80u,
    EDGE_BOTTOM = 0x40u,
    KEPT_FLAGS = 0x01u,
    WINDOW_DRAWN = 0x08u,
    MOVE_CODE = 1u,
    TOP_CODE = 0x0au,
    BOTTOM_CODE = 0x0bu,
    STACK_TOP = 0x1ffcu,
    CURSOR_STACK = 0x1f02u,             /* item lookup entered at $1F00+ */
    LOOP_STACK = 0x1f04u,
    SOUND_REQUEST = 0x80953bu,
    DRAW_STRING = 0x808878u,
    SPRITE_FRAME = 0x868b55u,
    CURSOR_ENTRY = 0x828720u,
    CURSOR_RETURNED = 0x82889du,
    CURSOR_NONE = 0x82889fu,
    LOOP_ENTRY = 0x828b08u,
    LOOP_POLL = 0x828b0cu,
    WINDOW_STRING_CALL = 0x829327u,
    WINDOW_RETURN = 0x82932fu,
    SPRITE_FRAME_CALL = 0x828b3cu
};

/* One native run with the service for its original children. */
typedef struct MenuCall {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2Wram wram;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult stop;
} MenuCall;

static void StartCall(MenuCall *call, const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    call->memory = memory;
    call->cpu = cpu;
    call->wram = WramViewOfCaller(memory, cpu);
    call->child = child;
    call->context = context;
    call->stop = ExecutionReturned(0);
}

/* PB82, M8/X16, DP0, binary, DB with low WRAM at $0000. */
static bool MenuCpuReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x82u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && cpu->direct_page == 0u && !cpu->decimal &&
        (cpu->data_bank < 0x40u || cpu->data_bank == 0x7eu ||
         (cpu->data_bank >= 0x80u && cpu->data_bank < 0xc0u));
}

/* JSL to an original child at `site`. */
static bool CallOriginal(MenuCall *call, uint32_t site, uint32_t target) {
    Lufia2CpuState *cpu = call->cpu;
    const uint16_t stack = cpu->stack;
    const uint8_t data_bank = cpu->data_bank;

    if (!CallChildWithFrame(call->memory, cpu, call->child, call->context,
            site, target, 3u, 0x82u)) {
        call->stop = ExecutionReturned(site);
        call->stop.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        cpu->resume_pc = site;
        return false;
    }
    /* Another CPU state finishes in the original code. */
    if (cpu->stack != stack || cpu->data_bank != data_bank ||
        !MenuCpuReady(cpu)) {
        call->stop = ExecutionHandoff(cpu, (site & 0xff0000u) |
            (uint16_t)(site + 4u));
        return false;
    }
    return true;
}

/* JSR to a native routine; a stop keeps its frame. */
static bool CallNative(MenuCall *call, uint16_t return_address,
    Lufia2ExecutionResult (*routine)(const Lufia2Memory *, Lufia2CpuState *)) {
    SimulateJsrFrame(call->memory, call->cpu, return_address);
    call->stop = routine(call->memory, call->cpu);
    if (call->stop.flow != LUFIA2_EXECUTION_RETURNED)
        return false;
    SimulateRtsFrame(call->memory, call->cpu);
    return true;
}

/* RTS past an original child; a changed return stays original. */
static bool ReturnTo(MenuCall *call, uint16_t return_address, uint32_t rts) {
    Lufia2CpuState *cpu = call->cpu;
    const uint16_t stack = cpu->stack;

    if (PullStackWord(call->memory, cpu) == return_address)
        return true;
    cpu->stack = stack;
    call->stop = ExecutionHandoff(cpu, rts);
    return false;
}

static uint8_t ItemByte(const MenuCall *call, uint32_t table) {
    return WramReadAt(call->wram, table, call->cpu->x);
}

static void LoadItem(MenuCall *call, uint32_t table) {
    LoadA8(call->cpu, ItemByte(call, table));
}

static void StoreItem(MenuCall *call, uint32_t table) {
    WramWriteAt(call->wram, table, call->cpu->x, A8(call->cpu));
}

static void ClearItem(MenuCall *call, uint32_t table) {
    WramWriteAt(call->wram, table, call->cpu->x, 0);
}

static void CompareItem(MenuCall *call, uint32_t table) {
    Compare8(call->cpu, A8(call->cpu), ItemByte(call, table));
}

/* INC or DEC of an item byte. */
static void StepItem(MenuCall *call, uint32_t table, int delta) {
    const uint8_t value = (uint8_t)(ItemByte(call, table) + delta);

    WramWriteAt(call->wram, table, call->cpu->x, value);
    SetNz8(call->cpu, value);
}

/* JSR $88A0, then CMP of $14B3 against the item limit. */
static bool IndexBelowLimit(
    MenuCall *call, uint16_t return_address, bool *below) {
    if (!CallNative(call, return_address, Lufia2MenuItemIndex))
        return false;
    LoadA8(call->cpu, WramRead(call->wram, LIST_INDEX));
    CompareItem(call, ITEM_LIMIT);
    *below = call->cpu->negative;
    return true;
}

/* $82:889C: the code in A, carry clear. */
static Lufia2ExecutionResult ActionDone(MenuCall *call, uint8_t code) {
    LoadA8(call->cpu, code);
    call->cpu->carry = false;
    return ExecutionReturned(CURSOR_RETURNED);
}

/* $82:8892: index and position of the moved item. */
static Lufia2ExecutionResult FinishMove(MenuCall *call) {
    SetAccumulatorWidth(call->cpu, 1);                         /* SEP #$20 */
    if (!CallNative(call, 0x8896u, Lufia2MenuItemIndex) ||
        !CallNative(call, 0x8899u, Lufia2MenuItemPosition))
        return call->stop;
    return ActionDone(call, MOVE_CODE);
}

/* LDA #sound, JSL $80:953B. */
static bool PlaySound(MenuCall *call, uint8_t sound, uint32_t site) {
    LoadA8(call->cpu, sound);
    return CallOriginal(call, site, SOUND_REQUEST);
}

static Lufia2ExecutionResult SoundThenFinish(MenuCall *call, uint32_t site) {
    if (!PlaySound(call, 0, site))
        return call->stop;
    return FinishMove(call);
}

/* $82:8727: row up; scrolling items mark the top edge. */
static Lufia2ExecutionResult MoveUp(MenuCall *call) {
    Lufia2CpuState *cpu = call->cpu;
    bool below = false;

    StepItem(call, ITEM_ROW, -1);
    LoadItem(call, ITEM_ROW);
    Compare8(cpu, A8(cpu), 0);
    if (!cpu->negative)
        return SoundThenFinish(call, 0x828760u);
    LoadItem(call, ITEM_FLAGS);
    if (!cpu->zero) {
        LoadItem(call, ITEM_FLAGS);
        Or8(cpu, EDGE_TOP);
        StoreItem(call, ITEM_FLAGS);
        ClearItem(call, ITEM_ROW);
        return FinishMove(call);
    }
    /* Wrap to the last row; first row past the limit. */
    LoadItem(call, ITEM_ROWS);
    DecrementA8(cpu);
    StoreItem(call, ITEM_ROW);
    LoadItem(call, ITEM_LIMIT);
    if (cpu->zero)
        return SoundThenFinish(call, 0x828760u);
    if (!IndexBelowLimit(call, 0x8752u, &below))
        return call->stop;
    if (below)
        return SoundThenFinish(call, 0x8287a8u);
    ClearItem(call, ITEM_ROW);
    return SoundThenFinish(call, 0x828760u);
}

/* $82:876B: row down; scrolling items mark the bottom edge. */
static Lufia2ExecutionResult MoveDown(MenuCall *call) {
    Lufia2CpuState *cpu = call->cpu;
    bool below = false;

    StepItem(call, ITEM_ROW, 1);
    LoadItem(call, ITEM_FLAGS);
    if (!cpu->zero) {
        LoadItem(call, ITEM_ROW);
        CompareItem(call, ITEM_ROWS);
        if (cpu->negative)
            return SoundThenFinish(call, 0x8287a8u);
        LoadItem(call, ITEM_FLAGS);
        Or8(cpu, EDGE_BOTTOM);
        StoreItem(call, ITEM_FLAGS);
        StepItem(call, ITEM_ROW, -1);
        return FinishMove(call);
    }
    /* Wrap to the first row at limit or row count. */
    LoadItem(call, ITEM_LIMIT);
    if (!cpu->zero) {
        if (!IndexBelowLimit(call, 0x8790u, &below))
            return call->stop;
    } else {
        LoadItem(call, ITEM_ROW);
        CompareItem(call, ITEM_ROWS);
        below = cpu->negative;
    }
    if (!below)
        ClearItem(call, ITEM_ROW);
    return SoundThenFinish(call, 0x8287a8u);
}

/* $82:87B3: sound, then column left with wrap. */
static Lufia2ExecutionResult MoveLeft(MenuCall *call) {
    Lufia2CpuState *cpu = call->cpu;
    bool below = false;

    if (!PlaySound(call, 0, 0x8287b5u))
        return call->stop;
    StepItem(call, ITEM_COLUMN, -1);
    LoadItem(call, ITEM_COLUMN);
    Compare8(cpu, A8(cpu), 0);
    if (cpu->negative) {
        LoadItem(call, ITEM_COLUMNS);
        DecrementA8(cpu);
        StoreItem(call, ITEM_COLUMN);
        LoadItem(call, ITEM_LIMIT);
        if (!cpu->zero) {
            if (!IndexBelowLimit(call, 0x87d1u, &below))
                return call->stop;
            if (!below)
                ClearItem(call, ITEM_COLUMN);
        }
    }
    return FinishMove(call);
}

/* $82:87E4: sound, then column right with wrap. */
static Lufia2ExecutionResult MoveRight(MenuCall *call) {
    Lufia2CpuState *cpu = call->cpu;
    bool below = true;

    if (!PlaySound(call, 0, 0x8287e6u))
        return call->stop;
    StepItem(call, ITEM_COLUMN, 1);
    LoadItem(call, ITEM_LIMIT);
    if (!cpu->zero && !IndexBelowLimit(call, 0x87f4u, &below))
        return call->stop;
    if (below) {
        LoadItem(call, ITEM_COLUMN);
        CompareItem(call, ITEM_COLUMNS);
        below = cpu->negative;
    }
    if (!below)
        ClearItem(call, ITEM_COLUMN);
    return FinishMove(call);
}

typedef enum {
    RULE_UP,
    RULE_DOWN,
    RULE_LEFT,
    RULE_RIGHT,
    RULE_ACTION
} ButtonRule;

/* One tested button; pad 0 reuses the byte in A. */
typedef struct ButtonEntry {
    uint16_t pad;
    uint8_t mask;
    ButtonRule rule;
    uint8_t code;
    uint8_t sound;
    uint32_t sound_call;                /* 0: no sound */
    uint16_t index_return;
} ButtonEntry;

/* In the original test order. */
static const ButtonEntry kButtons[] = {
    { PRESSED_B, BUTTON_UP, RULE_UP, 0, 0, 0, 0 },
    { 0, BUTTON_DOWN, RULE_DOWN, 0, 0, 0, 0 },
    { 0, BUTTON_LEFT, RULE_LEFT, 0, 0, 0, 0 },
    { 0, BUTTON_RIGHT, RULE_RIGHT, 0, 0, 0, 0 },
    { PRESSED_A, 0x80u, RULE_ACTION, 2, 0, 0, 0x8814u },
    { PRESSED_B, 0x80u, RULE_ACTION, 3, 1, 0x828823u, 0x8829u },
    { PRESSED_A, 0x40u, RULE_ACTION, 4, 0, 0, 0x8838u },
    { PRESSED_B, 0x40u, RULE_ACTION, 5, 0, 0, 0x8847u },
    { PRESSED_A, 0x20u, RULE_ACTION, 6, 0, 0x828856u, 0x885cu },
    { 0, 0x10u, RULE_ACTION, 7, 0, 0x828868u, 0x886eu },
    { PRESSED_B, 0x10u, RULE_ACTION, 8, 0, 0, 0x887du },
    { PRESSED_B, 0x20u, RULE_ACTION, 9, 0, 0, 0x888cu }
};

/* Optional sound, item index, then the code. */
static Lufia2ExecutionResult Action(
    MenuCall *call, const ButtonEntry *button) {
    if (button->sound_call &&
        !PlaySound(call, button->sound, button->sound_call))
        return call->stop;
    if (!CallNative(call, button->index_return, Lufia2MenuItemIndex))
        return call->stop;
    return ActionDone(call, button->code);
}

static Lufia2ExecutionResult MoveCursor(MenuCall *call) {
    Lufia2CpuState *cpu = call->cpu;
    size_t i;

    for (i = 0; i < sizeof(kButtons) / sizeof(kButtons[0]); ++i) {
        const ButtonEntry *button = &kButtons[i];

        if (button->pad)
            LoadA8(cpu, WramRead(call->wram, button->pad));
        BitImmediate8(cpu, button->mask);
        if (cpu->zero)
            continue;
        switch (button->rule) {
        case RULE_UP:
            return MoveUp(call);
        case RULE_DOWN:
            return MoveDown(call);
        case RULE_LEFT:
            return MoveLeft(call);
        case RULE_RIGHT:
            return MoveRight(call);
        default:
            return Action(call, button);
        }
    }
    cpu->carry = true;
    return ExecutionReturned(CURSOR_NONE);
}

/* $82:8720: cursor of item X from the new buttons. */
/* Returns carry set, or A = 1 or action code. */
Lufia2ExecutionResult Lufia2MenuCursor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    MenuCall call;

    if (!child || !MenuCpuReady(cpu) || cpu->stack < CURSOR_STACK ||
        cpu->stack > STACK_TOP || cpu->x > ITEM_LAST)
        return ExecutionHandoff(cpu, CURSOR_ENTRY);
    StartCall(&call, memory, cpu, child, context);
    return MoveCursor(&call);
}

/* JSL $82:8B4B from $82:8B1C. */
static bool ReadButtons(MenuCall *call) {
    SimulateJslFrame(call->memory, call->cpu, 0x82u, 0x8b1fu);
    call->stop = Lufia2MenuButtons(call->memory, call->cpu);
    if (call->stop.flow != LUFIA2_EXECUTION_RETURNED)
        return false;
    SimulateRtlFrame(call->memory, call->cpu);
    return true;
}

/* JSR $8720 from $82:8B22. */
static bool PollCursor(MenuCall *call) {
    Lufia2ExecutionResult moved;

    SimulateJsrFrame(call->memory, call->cpu, 0x8b24u);
    moved = Lufia2MenuCursor(call->memory, call->cpu, call->child,
        call->context);
    if (moved.flow != LUFIA2_EXECUTION_RETURNED) {
        call->stop = moved;
        return false;
    }
    return ReturnTo(call, 0x8b24u, moved.pc);
}

/* JSR $9313 from $82:8B36; $80:8878 stays original. */
static bool RefreshWindow(MenuCall *call) {
    Lufia2CpuState *cpu = call->cpu;

    SimulateJsrFrame(call->memory, cpu, 0x8b38u);
    call->stop = Lufia2MenuWindowRequest(call->memory, cpu);
    if (call->stop.flow == LUFIA2_EXECUTION_RETURNED) {
        SimulateRtsFrame(call->memory, cpu);
        return true;
    }
    if (call->stop.flow != LUFIA2_EXECUTION_BOUNDARY ||
        call->stop.pc != WINDOW_STRING_CALL ||
        !CallOriginal(call, WINDOW_STRING_CALL, DRAW_STRING))
        return false;
    LoadA8(cpu, WINDOW_DRAWN);
    TestBitsDirect(call->memory, cpu, REDRAW_FLAGS, 1);
    return ReturnTo(call, 0x8b38u, WINDOW_RETURN);
}

/* $82:8B27: a move reports its edge flag. */
static Lufia2ExecutionResult Confirm(MenuCall *call) {
    Lufia2CpuState *cpu = call->cpu;

    Compare8(cpu, A8(cpu), MOVE_CODE);
    if (!cpu->zero)
        return ExecutionReturned(0x828b4au);
    LoadItem(call, ITEM_FLAGS);
    AslA8(cpu);
    if (cpu->carry) {
        LoadA8(cpu, TOP_CODE);
        return ExecutionReturned(0x828b47u);
    }
    AslA8(cpu);
    if (cpu->carry) {
        LoadA8(cpu, BOTTOM_CODE);
        return ExecutionReturned(0x828b4au);
    }
    LoadA8(cpu, MOVE_CODE);
    return ExecutionReturned(0x828b44u);
}

/* $82:8B08: polls the cursor each frame until a button counts. */
/* A not 0 starts with the redraw; frames stay original. */
Lufia2ExecutionResult Lufia2MenuInputLoop(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    MenuCall call;
    bool redraw;

    if (!child || !MenuCpuReady(cpu) || cpu->stack < LOOP_STACK ||
        cpu->stack > STACK_TOP)
        return ExecutionHandoff(cpu, LOOP_ENTRY);
    if ((uint16_t)(WramRead16(WramViewOfCaller(memory, cpu), CURSOR_SLOT) -
            FIRST_CURSOR_SLOT) > ITEM_LAST)
        return ExecutionHandoff(cpu, LOOP_ENTRY);
    StartCall(&call, memory, cpu, child, context);
    Compare8(cpu, A8(cpu), 0);
    redraw = !cpu->zero;
    for (;;) {
        if (!redraw) {
            const uint16_t item = (uint16_t)(
                WramRead16(call.wram, CURSOR_SLOT) - FIRST_CURSOR_SLOT);

            if (item > ITEM_LAST)
                return ExecutionHandoff(cpu, LOOP_POLL);
            LoadX16(cpu, item);
            LoadItem(&call, ITEM_FLAGS);
            And8(cpu, KEPT_FLAGS);
            StoreItem(&call, ITEM_FLAGS);
            if (!ReadButtons(&call))
                return call.stop;
            if (!cpu->carry) {
                if (!PollCursor(&call))
                    return call.stop;
                if (!cpu->carry)
                    return Confirm(&call);
            }
        }
        redraw = false;
        if (!RefreshWindow(&call) ||
            !CallNative(&call, 0x8b3bu, Lufia2MenuCursorBlink) ||
            !CallOriginal(&call, SPRITE_FRAME_CALL, SPRITE_FRAME))
            return call.stop;
    }
}
