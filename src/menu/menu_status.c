/* Status and equipment text dispatch. */

#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/wram_view.h"
#include "lufia2/menu.h"
#include "system/wram.h"

enum {
    STATUS_PARTY_SLOT = 0x22,
    STATUS_CHARACTER = 0x2a,
    STATUS_VALUE = 0x4e,
    STATUS_DIGITS = 0x51,
    STATUS_TABLE = 0x5d,
    STATUS_TEXT_BANK = 0x5f,
    STATUS_TARGET = 0x60,
    STATUS_TILE_CURSOR = 0xe7,
    STATUS_LOOP_LIMIT = 4096
};

static uint16_t StatusDispatchTarget(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0xa321u);
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0x00ffu);
    OpAslA(cpu);
    OpIncA(cpu);
    OpTay(cpu);
    PullAccumulator16(memory, cpu);
    OpSta(memory, cpu, OpDp(cpu, STATUS_TABLE));
    OpSepWidths(cpu, 0x20u);
    Push8(memory, cpu, cpu->program_bank);
    LoadA8(cpu, Pull8(memory, cpu));
    OpSta(memory, cpu, OpDp(cpu, STATUS_TEXT_BANK));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, STATUS_TABLE));
    OpSta(memory, cpu, OpDp(cpu, STATUS_TARGET));
    OpSepWidths(cpu, 0x20u);
    return Read16Bank(memory, 0u, STATUS_TARGET);
}

/* Handler entry points in the inline jump table at $82:A323, selected by the
 * accumulator on entry. */
enum {
    STATUS_MODE_PARTY_HEADER = 0xa32c,
    STATUS_MODE_PARTY_ROWS = 0xa345,
    STATUS_MODE_EQUIPMENT = 0xa373,
    STATUS_MODE_EQUIPMENT_REFRESH = 0xa3d6,
    STATUS_MODE_PARTY_DETAILS = 0xa403
};

enum {
    STATUS_TEXT_BANK_MENU = 0x8e,
    STATUS_DRAW_MODE_IDLE = 0x20,
    STATUS_EQUIPMENT_CURSOR = 0x1560,
    STATUS_EQUIPMENT_CHARACTER_LIST = 0x308a,
    STATUS_PARTY_HEADER_X = 0x35e0,
    STATUS_PERCENT_SCALE = 0x64,
    STATUS_CHARACTER_PERCENT = 0x00bc,
    STATUS_NO_DIGITS = 0xff,
    STATUS_DONE_FLAGS = 0x74,
    STATUS_WORK_FLAG = 0x08
};

/* Text blocks, as $80:8878 string pointers in the menu text bank. */
enum {
    STATUS_TEXT_PARTY_TITLE = 0xd1e9,
    STATUS_TEXT_PARTY_LABELS = 0xd4de,
    STATUS_TEXT_EQUIPMENT_A = 0xc37d,
    STATUS_TEXT_EQUIPMENT_B = 0xc4cc,
    STATUS_TEXT_EQUIPMENT_C = 0xd292,
    STATUS_TEXT_EQUIPMENT_D = 0xd591
};

/* One run of the screen: the callbacks it was given and the bound counter
 * that stops a party loop that never finishes. */
typedef struct {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2Wram wram;
    Lufia2PushedChildCall child;
    Lufia2ExecutionCheckpoint checkpoint;
    void *context;
    unsigned visits;
} StatusScreen;

static uint8_t StatusChild(StatusScreen *screen, uint32_t site, uint32_t target,
                           uint8_t frame) {
    return CallChildWithFrame(screen->memory, screen->cpu, screen->child,
                              screen->context, site, target, frame, 0x82u);
}

static Lufia2ExecutionResult StatusUnwound(uint32_t site) {
    Lufia2ExecutionResult result = {LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};

    return result;
}

#define STATUS_CALL(screen, site, callee, frame)                                       \
    do {                                                                               \
        if (!StatusChild(screen, site, callee, frame))                                 \
            return StatusUnwound(site);                                                \
    } while (0)

static void StatusSelectTextBank(StatusScreen *screen) {
    LoadA8(screen->cpu, STATUS_TEXT_BANK_MENU);
    WramWrite(screen->wram, STATUS_TEXT_BANK, STATUS_TEXT_BANK_MENU);
}

/* Draws one string of the menu text bank. */
#define STATUS_TEXT(screen, site, text)                                                \
    do {                                                                               \
        LoadY16((screen)->cpu, text);                                                  \
        STATUS_CALL(screen, site, 0x808878u, 3u);                                      \
    } while (0)

/* Puts the draw mode back to idle and the tile cursor at the equipment
 * panel. */
static void StatusEquipmentPanel(StatusScreen *screen) {
    LoadA8(screen->cpu, STATUS_DRAW_MODE_IDLE);
    WramWrite(screen->wram, WRAM_MENU_DRAW_MODE, STATUS_DRAW_MODE_IDLE);
    LoadX16(screen->cpu, STATUS_EQUIPMENT_CURSOR);
    WramWrite16(screen->wram, STATUS_TILE_CURSOR, STATUS_EQUIPMENT_CURSOR);
    StatusSelectTextBank(screen);
}

static Lufia2ExecutionResult StatusFinish(StatusScreen *screen) {
    OpLoadA(screen->cpu, STATUS_WORK_FLAG);
    OpTestBits(screen->memory, screen->cpu, OpDp(screen->cpu, STATUS_DONE_FLAGS), 1u);
    return ExecutionReturned(0x82a431u);
}

/* Loads the character of party slot `slot` (a word, as the original reads
 * it) and points X at that character's entry of the table at `table`. */
static void StatusSelectMember(StatusScreen *screen, uint16_t slot_word,
                               uint32_t table) {
    Lufia2CpuState *cpu = screen->cpu;

    LoadA16(cpu, slot_word);
    OpAslA(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, WramRead16At(screen->wram, WRAM_MENU_PARTY_CHARACTER_OFFSET, cpu->x));
    WramWrite16(screen->wram, STATUS_CHARACTER, cpu->accumulator);
    LoadA16(cpu, Read16Long(screen->memory, LongIndexedAddress(table, cpu->x)));
    TransferAToX(cpu);
}

/* Mode 1 (and the tail of mode 0): the party rows, one child draw per
 * member. */
static Lufia2ExecutionResult StatusDrawPartyRows(StatusScreen *screen) {
    Lufia2CpuState *cpu = screen->cpu;

    LoadX16(cpu, 0u);
    WramWrite16(screen->wram, STATUS_PARTY_SLOT, 0u);
    do {
        if (screen->visits++ == STATUS_LOOP_LIMIT)
            return ExecutionHandoff(cpu, 0x82a34au);
        OpRepWidths(cpu, 0x20u);
        StatusSelectMember(screen, WramRead16(screen->wram, STATUS_PARTY_SLOT),
                           0x82a36bu);
        OpSepWidths(cpu, 0x20u);
        STATUS_CALL(screen, 0x82a35cu, 0x8294c0u, 2u);
        OpStepMem(screen->memory, cpu, OpDp(cpu, STATUS_PARTY_SLOT), 1);
        OpLda(screen->memory, cpu, OpDp(cpu, STATUS_PARTY_SLOT));
        OpCmp(screen->memory, cpu, OpAbs(cpu, WRAM_MENU_PARTY_MEMBER_COUNT));
    } while (!cpu->zero);
    return StatusFinish(screen);
}

/* Mode 0: the title and label texts, then the party rows. */
static Lufia2ExecutionResult StatusDrawPartyHeader(StatusScreen *screen) {
    StatusSelectTextBank(screen);
    STATUS_TEXT(screen, 0x82a333u, STATUS_TEXT_PARTY_TITLE);
    StatusSelectTextBank(screen);
    LoadY16(screen->cpu, STATUS_TEXT_PARTY_LABELS);
    LoadX16(screen->cpu, STATUS_PARTY_HEADER_X);
    STATUS_CALL(screen, 0x82a341u, 0x808878u, 3u);
    return StatusDrawPartyRows(screen);
}

/* Mode 2: the equipment screen, with the character's percentage stat (scaled
 * by the hardware multiplier) printed as a number. */
static Lufia2ExecutionResult StatusDrawEquipment(StatusScreen *screen) {
    Lufia2CpuState *cpu = screen->cpu;
    const Lufia2Wram wram = screen->wram;
    uint16_t value;

    STATUS_CALL(screen, 0x82a373u, 0x82f9c9u, 2u);
    LoadX16(cpu, STATUS_EQUIPMENT_CHARACTER_LIST);
    STATUS_CALL(screen, 0x82a379u, 0x8294c0u, 2u);
    LoadX16(cpu, WramRead16(wram, STATUS_CHARACTER));
    OpLda(screen->memory, cpu, OpAbsX(cpu, STATUS_CHARACTER_PERCENT));
    WramWrite(wram, WRAM_SYSTEM_MULTIPLY_A, A8(cpu));
    WramWrite(wram, WRAM_SYSTEM_MULTIPLY_A + 1u, 0u);
    LoadY16(cpu, STATUS_PERCENT_SCALE);
    WramWrite16(wram, WRAM_SYSTEM_MULTIPLY_B, STATUS_PERCENT_SCALE);
    STATUS_CALL(screen, 0x82a38du, 0x828000u, 3u);
    LoadY16(cpu, WramRead16(wram, WRAM_SYSTEM_MULTIPLY_PRODUCT));
    value = cpu->y;
    WramWrite16(wram, STATUS_VALUE, value);
    LoadY16(cpu, STATUS_NO_DIGITS);
    WramWrite16(wram, STATUS_DIGITS, STATUS_NO_DIGITS);
    STATUS_CALL(screen, 0x82a39bu, 0x808378u, 3u);
    LoadA8(cpu, WramRead(wram, STATUS_VALUE));
    WramWrite(wram, 0u, A8(cpu));
    STATUS_CALL(screen, 0x82a3a3u, 0x82942fu, 2u);
    StatusEquipmentPanel(screen);
    if (screen->checkpoint)
        screen->checkpoint(screen->context, cpu, 0x82a3b4u);
    STATUS_TEXT(screen, 0x82a3b7u, STATUS_TEXT_EQUIPMENT_A);
    STATUS_TEXT(screen, 0x82a3beu, STATUS_TEXT_EQUIPMENT_B);
    STATUS_TEXT(screen, 0x82a3c5u, STATUS_TEXT_EQUIPMENT_C);
    StatusSelectTextBank(screen);
    STATUS_TEXT(screen, 0x82a3d0u, STATUS_TEXT_EQUIPMENT_D);
    return StatusFinish(screen);
}

/* Mode 3: redraws the equipment panel and its texts. */
static Lufia2ExecutionResult StatusRefreshEquipment(StatusScreen *screen) {
    STATUS_CALL(screen, 0x82a3d6u, 0x829971u, 2u);
    STATUS_CALL(screen, 0x82a3d9u, 0x82f9c9u, 2u);
    LoadX16(screen->cpu, STATUS_EQUIPMENT_CHARACTER_LIST);
    STATUS_CALL(screen, 0x82a3dfu, 0x8294c0u, 2u);
    StatusEquipmentPanel(screen);
    if (screen->checkpoint)
        screen->checkpoint(screen->context, screen->cpu, 0x82a3f0u);
    STATUS_TEXT(screen, 0x82a3f3u, STATUS_TEXT_EQUIPMENT_A);
    StatusSelectTextBank(screen);
    STATUS_TEXT(screen, 0x82a3feu, STATUS_TEXT_EQUIPMENT_D);
    return ExecutionReturned(0x82a402u);
}

/* Mode 4: the detail page of every party member in turn. */
static Lufia2ExecutionResult StatusDrawPartyDetails(StatusScreen *screen) {
    Lufia2CpuState *cpu = screen->cpu;

    LoadA8(cpu, 0u);
    do {
        if (screen->visits++ == STATUS_LOOP_LIMIT)
            return ExecutionHandoff(cpu, 0x82a405u);
        PushAccumulator8(screen->memory, cpu);
        OpRepWidths(cpu, 0x20u);
        OpAndValue(cpu, 0x00ffu);
        StatusSelectMember(screen, cpu->accumulator, 0x82a425u);
        OpSepWidths(cpu, 0x20u);
        STATUS_CALL(screen, 0x82a419u, 0x82950eu, 2u);
        LoadA8(cpu, Pull8(screen->memory, cpu));
        OpIncA(cpu);
        OpCmp(screen->memory, cpu, OpAbs(cpu, WRAM_MENU_PARTY_MEMBER_COUNT));
    } while (!cpu->zero);
    return StatusFinish(screen);
}

/* $82:A318: modes 0/1 party, 2 equipment, 3 equipment refresh, 4 details. */
Lufia2ExecutionResult Lufia2MenuDrawStatus(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, Lufia2ExecutionCheckpoint checkpoint,
    void *context) {
    StatusScreen screen;
    uint16_t target;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit || !child)
        return ExecutionHandoff(cpu, 0x82a318u);
    screen.memory = memory;
    screen.cpu = cpu;
    screen.wram = WramViewOfCaller(memory, cpu);
    screen.child = child;
    screen.checkpoint = checkpoint;
    screen.context = context;
    screen.visits = 0;
    PushAccumulator8(memory, cpu);
    OpLoadA(cpu, STATUS_DRAW_MODE_IDLE);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    LoadA8(cpu, Pull8(memory, cpu));

    target = StatusDispatchTarget(memory, cpu);
    switch (target) {
    case STATUS_MODE_PARTY_HEADER:
        return StatusDrawPartyHeader(&screen);
    case STATUS_MODE_PARTY_ROWS:
        return StatusDrawPartyRows(&screen);
    case STATUS_MODE_EQUIPMENT:
        return StatusDrawEquipment(&screen);
    case STATUS_MODE_EQUIPMENT_REFRESH:
        return StatusRefreshEquipment(&screen);
    case STATUS_MODE_PARTY_DETAILS:
        return StatusDrawPartyDetails(&screen);
    default:
        return ExecutionHandoff(cpu, 0x820000u | target);
    }
}
