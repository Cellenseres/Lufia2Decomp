#include "menu/card_table_services.h"
#include "lufia2/card_table.h"
#include "system/wram.h"

enum {
    TABLE_COPY_COUNT = 0u,
    TABLE_OAM_GROUPS = 0u,
    TABLE_OAM_PAIRS = 2u,
    TABLE_OPTION = 0x30u,
    TABLE_SPEEDS = WRAM_CARD_GAME_OPENING_CHOICES & 0xffffu,
    TABLE_SYMBOLS = WRAM_CARD_GAME_TILE_BUFFER & 0xffffu,
    TABLE_OAM_LOW = WRAM_FIELD_OAM_LOW_BUFFER & 0xffffu,
    TABLE_OAM_HIGH = WRAM_FIELD_OAM_HIGH_BUFFER & 0xffffu,
    TABLE_POSITIONS = (WRAM_POKER_OPPONENT_VALUES + 3u) & 0xffffu,
    TABLE_OFFSETS = (WRAM_POKER_ROUND_STAKE + 3u) & 0xffffu,
    TABLE_PAYOUT_STARTS = (WRAM_CARD_TABLE_PAYOUT_ACTIVE - 2u) & 0xffffu,
    TABLE_PAYOUT_ENDS = (WRAM_CARD_TABLE_PAYOUT_COPY - 2u) & 0xffffu,
    TABLE_RANDOM_MODE = WRAM_CARD_TABLE_RANDOM_SELECTION & 0xffffu,
    TABLE_RANDOM_PARAMETER = (WRAM_CARD_TABLE_RANDOM_SELECTION + 2u) & 0xffffu,
    TABLE_VARIANT = WRAM_CARD_TABLE_VARIANT_FLAGS & 0xffffu,
    TABLE_PENDING = WRAM_CARD_TABLE_PENDING_WORDS & 0xffffu,
    TABLE_REMAINING = WRAM_CARD_TABLE_PAYOUT_REMAINING & 0xffffu,
    TABLE_COUNTERS = WRAM_CARD_TABLE_STATE_COUNTERS & 0xffffu,
    TABLE_DELAY = (WRAM_BOARD_GAME_LINE_TOTALS + 6u) & 0xffffu,
    TABLE_REPEAT_DELAY = (WRAM_BOARD_GAME_LINE_TOTALS + 12u) & 0xffffu,
    TABLE_COUNTER_A = (WRAM_BOARD_GAME_LINE_TOTALS + 8u) & 0xffffu,
    TABLE_COUNTER_B = (WRAM_BOARD_GAME_LINE_TOTALS + 14u) & 0xffffu,
    TABLE_COUNTER_C = (WRAM_BOARD_GAME_LINE_TOTALS + 10u) & 0xffffu,
    TABLE_GROUP_STATE = (WRAM_BOARD_GAME_SAVED_GROUP_CELLS + 8u) & 0xffffu
};

static bool InitializeTableSpeeds(TableService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    static const uint32_t calls[] = {0x84ec55u, 0x84ec63u, 0x84ec71u};
    for (unsigned reel = 0u; reel < 3u; ++reel) {
        OpLoadA(cpu, 21u);
        if (!TableServiceCall(service, calls[reel], 0x808299u, false))
            return false;
        cpu->carry = 0u;
        OpAdcValue(cpu, 5u);
        OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(TABLE_SPEEDS + reel * 2u)));
    }
    return true;
}

static void InitializeTableSprites(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 11u);
    OpLdy(cpu, 0u);
    OpLoadA(cpu, 0xaaaau);
    do {
        OpSta(memory, cpu, OpAbsY(cpu, TABLE_OAM_HIGH));
        OpIny(cpu);
        OpIny(cpu);
        OpDex(cpu);
    } while (!cpu->zero);
    OpLdx(cpu, 0u);
    OpLdy(cpu, 3u);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, TABLE_POSITIONS));
        OpStz(memory, cpu, OpAbsX(cpu, TABLE_OFFSETS));
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
}

static void InitializeTableSymbols(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 0u);
    OpTxy(cpu);
    OpLoadA(cpu, 0x57u);
    OpSta(memory, cpu, OpDp(cpu, TABLE_COPY_COUNT));
    do {
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpLongX(cpu, 0x84f215u));
        OpRepWidths(cpu, 0x20u);
        OpAndValue(cpu, 0xffu);
        OpSta(memory, cpu, OpAbsY(cpu, TABLE_SYMBOLS));
        OpInx(cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, TABLE_COPY_COUNT), -1);
    } while (!cpu->zero);
    OpLoadA(cpu, 0x38u);
    OpSta(memory, cpu, OpDp(cpu, TABLE_COPY_COUNT));
    do {
        OpLda(memory, cpu, OpLongX(cpu, 0x84f215u));
        OpSta(memory, cpu, OpAbsY(cpu, TABLE_SYMBOLS));
        OpInx(cpu);
        OpInx(cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, TABLE_COPY_COUNT), -1);
    } while (!cpu->zero);
}

static void InitializeTableSpritePairs(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 0x0120u);
    OpLdy(cpu, 0x4fd8u);
    OpLoadA(cpu, 3u);
    OpSta(memory, cpu, OpDp(cpu, TABLE_OAM_GROUPS));
    do {
        OpLoadA(cpu, 2u);
        OpSta(memory, cpu, OpDp(cpu, TABLE_OAM_PAIRS));
        do {
            OpTya(cpu);
            OpSta(memory, cpu, OpAbsX(cpu, TABLE_OAM_LOW));
            OpInx(cpu);
            OpInx(cpu);
            OpLoadA(cpu, 0x0201u);
            OpSta(memory, cpu, OpAbsX(cpu, TABLE_OAM_LOW));
            OpInx(cpu);
            OpInx(cpu);
            OpTya(cpu);
            cpu->carry = 0u;
            OpAdcValue(cpu, 0x10u);
            OpSta(memory, cpu, OpAbsX(cpu, TABLE_OAM_LOW));
            OpInx(cpu);
            OpInx(cpu);
            OpLoadA(cpu, 0x0201u);
            OpSta(memory, cpu, OpAbsX(cpu, TABLE_OAM_LOW));
            OpInx(cpu);
            OpInx(cpu);
            OpStepMem(memory, cpu, OpDp(cpu, TABLE_OAM_PAIRS), -1);
        } while (!cpu->zero);
        OpTya(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x2000u);
        OpTay(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, TABLE_OAM_GROUPS), -1);
    } while (!cpu->zero);
    OpLdy(cpu, 3u);
    OpLdx(cpu, 0u);
    do {
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, OpAbsX(cpu, TABLE_PAYOUT_STARTS));
        OpIncA(cpu);
        OpIncA(cpu);
        OpSta(memory, cpu, OpAbsX(cpu, TABLE_PAYOUT_ENDS));
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
}

static bool ChooseTableRandomMode(TableService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    OpLda(memory, cpu, OpDp(cpu, TABLE_OPTION));
    OpAndValue(cpu, 0xffu);
    if (!cpu->zero) {
        OpLoadA(cpu, 5u);
        if (!TableServiceCall(service, 0x84ed42u, 0x808299u, false)) return false;
        OpCmpValue(cpu, 2u);
        if (cpu->negative) {
            OpCmpValue(cpu, 1u);
            if (cpu->zero) {
                OpLoadA(cpu, 0x0200u);
                OpSta(memory, cpu, OpAbs(cpu, TABLE_RANDOM_PARAMETER));
                OpLoadA(cpu, 1u);
            } else {
                OpLoadA(cpu, 0x0100u);
                OpSta(memory, cpu, OpAbs(cpu, TABLE_RANDOM_PARAMETER));
                OpLoadA(cpu, 2u);
            }
            OpSta(memory, cpu, OpAbs(cpu, TABLE_RANDOM_MODE));
            return true;
        }
    }
    OpStz(memory, cpu, OpAbs(cpu, TABLE_RANDOM_PARAMETER));
    OpStz(memory, cpu, OpAbs(cpu, TABLE_RANDOM_MODE));
    return true;
}

Lufia2ExecutionResult Lufia2CardTableInitialize(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !TableServiceReady(cpu, true) || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x84ec4fu);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84ed9du), true};
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x20u);
    if (!InitializeTableSpeeds(&service)) return service.result;
    InitializeTableSprites(memory, cpu);
    InitializeTableSymbols(memory, cpu);
    InitializeTableSpritePairs(memory, cpu);
    if (!TableServiceCall(&service, 0x84ed24u, 0x84ebdbu, false))
        return service.result;
    OpStz(memory, cpu, OpAbs(cpu, WRAM_CARD_TABLE_STATE_7178 & 0xffffu));
    OpLdx(cpu, 0u);
    OpLdy(cpu, 5u);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, TABLE_COUNTERS));
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    if (!ChooseTableRandomMode(&service)) return service.result;
    OpStz(memory, cpu, OpAbs(cpu, TABLE_VARIANT));
    OpStz(memory, cpu, OpAbs(cpu, TABLE_PENDING));
    OpStz(memory, cpu, OpAbs(cpu, TABLE_PENDING + 2u));
    OpStz(memory, cpu, OpAbs(cpu, TABLE_REMAINING));
    OpLoadA(cpu, 10u);
    OpSta(memory, cpu, OpAbs(cpu, TABLE_DELAY));
    OpSta(memory, cpu, OpAbs(cpu, TABLE_REPEAT_DELAY));
    OpStz(memory, cpu, OpAbs(cpu, TABLE_COUNTER_A));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_CARD_TABLE_STATE_71AC & 0xffffu));
    OpStz(memory, cpu, OpAbs(cpu, TABLE_GROUP_STATE));
    OpStz(memory, cpu, OpAbs(cpu, TABLE_COUNTER_B));
    OpStz(memory, cpu, OpAbs(cpu, TABLE_COUNTER_C));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_SPECIAL_MENU_SCENE_STATE & 0xffffu));
    UnpackStatus(cpu, Pull8(memory, cpu));
    return service.result;
}
