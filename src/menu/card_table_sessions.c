#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "lufia2/card_table.h"
#include "system/wram.h"

enum {
    TABLE_RESOURCE = 0x54u,
    TABLE_DMA_TARGET = 0u,
    TABLE_DMA_LENGTH = 2u,
    TABLE_FORMAT_BANK = 0x5fu,
    TABLE_UPDATE_POINTER = 0x68u,
    TABLE_UPDATE_BANK = 0x6au,
    TABLE_PALETTE_FLAGS = 0x73u,
    TABLE_FRAME_FLAGS = 0x72u,
    TABLE_TEXT_DELAY = 0x74u,
    TABLE_INPUT = 0x30u,
    TABLE_VALUE = 0x0eu,
    TABLE_BALANCE = WRAM_MENU_EXCHANGE_BALANCE & 0xffffu,
    TABLE_HAND = (WRAM_POKER_ROUND_STATUS + 1u) & 0xffffu,
    TABLE_PREVIOUS_HAND = WRAM_CARD_TABLE_PREVIOUS_HAND_WORDS & 0xffffu,
    TABLE_STAKE = (WRAM_BOARD_GAME_LINE_TOTALS + 4u) & 0xffffu,
    TABLE_VARIANT = WRAM_CARD_TABLE_VARIANT_FLAGS & 0xffffu,
    TABLE_SELECTION = WRAM_CARD_TABLE_SELECTION_ACTIVE & 0xffffu,
    TABLE_REMAINING = WRAM_CARD_TABLE_PAYOUT_REMAINING & 0xffffu,
    TABLE_SESSION_MODE = WRAM_CARD_TABLE_SESSION_MODE & 0xffffu,
    TABLE_PAYOUT_INDEX = WRAM_CARD_TABLE_PAYOUT_INDEX & 0xffffu,
    TABLE_PAYOUT_STATE = WRAM_CARD_TABLE_PAYOUT_STATE & 0xffffu,
    TABLE_PAYOUT_VALUE = WRAM_CARD_TABLE_PAYOUT_VALUE & 0xffffu,
    TABLE_PAYOUT_COPY = WRAM_CARD_TABLE_PAYOUT_COPY & 0xffffu,
    TABLE_PAYOUT_ACTIVE = WRAM_CARD_TABLE_PAYOUT_ACTIVE & 0xffffu,
    TABLE_PAYOUT_STEP = WRAM_SPECIAL_MENU_PAYOUT_STEP & 0xffffu,
    TABLE_CALL_LIMIT = 4096u
};

typedef struct TableSession {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
    unsigned calls;
    bool alternate;
} TableSession;

static bool TableReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x84u && !cpu->direct_page && !cpu->decimal;
}

static bool TableCall(TableSession *table, uint16_t first_site,
                      uint16_t alternate_site, uint32_t target) {
    Lufia2CpuState *cpu = table->cpu;
    const uint32_t site = 0x840000u | (table->alternate ? alternate_site : first_site);
    const bool byte_mode = cpu->accumulator_is_8_bit != 0u;
    const uint8_t frame = target > 0xffffu ? 3u : 2u;
    if (table->calls++ == TABLE_CALL_LIMIT) {
        table->result = ExecutionHandoff(cpu, site);
        return false;
    }
    if (frame == 2u) target |= 0x840000u;
    if (!CallChildWithFrame(table->memory, cpu, table->child, table->context,
                            site, target, frame, 0x84u)) {
        table->result = ExecutionReturned(site);
        table->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    const bool normalizes = site == 0x84dad1u || site == 0x84f3c0u ||
        site == 0x84daf7u || site == 0x84f3e6u || site == 0x84db59u ||
        site == 0x84f44bu || site == 0x84dbfcu || site == 0x84dc12u ||
        site == 0x84dc1cu || site == 0x84dc31u;
    if (!TableReady(cpu) || cpu->index_is_8_bit ||
        (!normalizes && cpu->accumulator_is_8_bit != byte_mode)) {
        table->result = ExecutionHandoff(cpu, (uint32_t)cpu->program_bank << 16 |
            (uint16_t)(site + frame + 1u));
        return false;
    }
    return true;
}

static void LoadTablePalettes(TableSession *table) {
    const Lufia2Memory *memory = table->memory;
    Lufia2CpuState *cpu = table->cpu;
    PushDataBank(memory, cpu);
    for (unsigned palette = 0u; palette < 2u; ++palette) {
        const uint16_t source = table->alternate ? 0xf320u : 0xf020u;
        OpLdx(cpu, (uint16_t)(source - palette * 0x100u));
        OpLdy(cpu, (uint16_t)(0x0320u + palette * 0x100u));
        OpLoadA(cpu, 0x00ffu);
        OpMoveNext(memory, cpu, 0u, 0xa6u);
    }
    PullDataBank(memory, cpu);
}

static bool LoadTableGraphics(TableSession *table) {
    const Lufia2Memory *memory = table->memory;
    Lufia2CpuState *cpu = table->cpu;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, TABLE_PALETTE_FLAGS));
    if (!TableCall(table, 0xdad1u, 0xf3c0u, 0xa5a8u)) return false;
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, table->alternate ? 0x029du : 0x029bu);
    OpWriteX(memory, cpu, OpDp(cpu, TABLE_RESOURCE), cpu->x);
    OpLoadA(cpu, 0x8000u);
    OpSta(memory, cpu, OpDp(cpu, TABLE_DMA_TARGET));
    OpLoadA(cpu, 0x4000u);
    OpSta(memory, cpu, OpDp(cpu, TABLE_DMA_LENGTH));
    if (!TableCall(table, 0xdae5u, 0xf3d4u, 0xa569u)) return false;
    OpLdx(cpu, table->alternate ? 0x029cu : 0x029au);
    OpWriteX(memory, cpu, OpDp(cpu, TABLE_RESOURCE), cpu->x);
    OpLoadA(cpu, 0x2000u);
    OpSta(memory, cpu, OpDp(cpu, TABLE_DMA_TARGET));
    OpLoadA(cpu, 0x2000u);
    OpSta(memory, cpu, OpDp(cpu, TABLE_DMA_LENGTH));
    return TableCall(table, 0xdaf7u, 0xf3e6u, 0xa569u);
}

static void PrepareTableText(TableSession *table) {
    const Lufia2Memory *memory = table->memory;
    Lufia2CpuState *cpu = table->cpu;
    OpLoadA(cpu, table->alternate ? 0x30u : 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE & 0xffffu));
    OpLoadA(cpu, 0x84u);
    OpSta(memory, cpu, OpDp(cpu, TABLE_FORMAT_BANK));
}

static bool OpenTableSession(TableSession *table) {
    const Lufia2Memory *memory = table->memory;
    Lufia2CpuState *cpu = table->cpu;
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS & 0xffffu));
    OpLoadA(cpu, 0x44u);
    OpSta(memory, cpu, OpAbs(cpu, 0x210bu));
    OpLoadA(cpu, 0x11u);
    OpSta(memory, cpu, OpAbs(cpu, 0x210cu));
    OpRepWidths(cpu, 0x20u);
    LoadTablePalettes(table);
    if (!LoadTableGraphics(table)) return false;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, SNES_CGWSEL);
    OpLoadA(cpu, 0x90u);
    OpSta(memory, cpu, SNES_CGADSUB);
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, WRAM_CARD_TABLE_PHASE);
    OpLdx(cpu, 0xf12fu);
    OpWriteX(memory, cpu, OpDp(cpu, TABLE_UPDATE_POINTER), cpu->x);
    OpLoadA(cpu, 0x84u);
    OpSta(memory, cpu, OpDp(cpu, TABLE_UPDATE_BANK));
    if (!TableCall(table, 0xdb17u, 0xf406u, 0xa4fdu) ||
        !TableCall(table, 0xdb1au, 0xf409u, 0xa4ddu)) return false;
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7eu);
    if (!TableCall(table, 0xdb22u, 0xf411u, 0xec4fu)) return false;
    if (table->alternate) {
        if (!TableCall(table, 0u, 0xf414u, 0xf78fu) ||
            !TableCall(table, 0u, 0xf417u, 0xf7d3u)) return false;
    } else if (!TableCall(table, 0xdb25u, 0u, 0xedfau)) return false;
    if (!TableCall(table, 0xdb28u, 0xf41au, 0xea4bu) ||
        !TableCall(table, 0xdb2bu, 0xf41du, 0xf173u)) return false;
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, TABLE_FRAME_FLAGS));
    OpLoadA(cpu, 0xa0u);
    OpSta(memory, cpu, OpDp(cpu, TABLE_TEXT_DELAY));
    if (!TableCall(table, 0xdb36u, 0xf428u, 0xa5a8u)) return false;
    PrepareTableText(table);
    OpLdx(cpu, 0x3694u);
    OpLdy(cpu, 0xf945u);
    if (!TableCall(table, 0xdb48u, 0xf43au, 0x808878u)) return false;
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpDp(cpu, TABLE_TEXT_DELAY));
    if (!TableCall(table, 0xdb50u, 0xf442u, 0xa5a8u)) return false;
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, WRAM_CARD_TABLE_PHASE);
    if (!TableCall(table, 0xdb59u, 0xf44bu, 0xa5b5u)) return false;
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 7u);
    OpSta(memory, cpu, OpDp(cpu, TABLE_VALUE));
    return true;
}

static bool CloseTableSession(TableSession *table) {
    const Lufia2Memory *memory = table->memory;
    Lufia2CpuState *cpu = table->cpu;
    OpSepWidths(cpu, 0x20u);
    if (!TableCall(table, 0xdc36u, 0xf4a7u, 0xa5cau)) return false;
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, WRAM_CARD_TABLE_PHASE);
    if (!TableCall(table, 0xdc3fu, 0xf4b0u, 0xe9b1u)) return false;
    OpStz(memory, cpu, OpDp(cpu, TABLE_UPDATE_BANK));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, SNES_COLDATA);
    OpSta(memory, cpu, SNES_CGWSEL);
    OpSta(memory, cpu, SNES_CGADSUB);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return true;
}

static bool TableCanContinue(TableSession *table) {
    const Lufia2Memory *memory = table->memory;
    Lufia2CpuState *cpu = table->cpu;
    if (!table->alternate) {
        OpLda(memory, cpu, OpAbs(cpu, TABLE_REMAINING));
        if (!cpu->zero) return true;
    }
    OpLda(memory, cpu, OpAbs(cpu, TABLE_BALANCE + 2u));
    if (!cpu->zero) return true;
    OpLda(memory, cpu, OpAbs(cpu, TABLE_BALANCE));
    cpu->carry = 1u;
    if (table->alternate)
        OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, TABLE_STAKE)));
    else
        OpSbcValue(cpu, 10u);
    return cpu->carry != 0u;
}

static void RememberTableHand(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 0u);
    OpLdy(cpu, 3u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, TABLE_HAND));
        OpSta(memory, cpu, OpAbsX(cpu, TABLE_PREVIOUS_HAND));
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
}

static bool PlayTableHand(TableSession *table) {
    const Lufia2Memory *memory = table->memory;
    Lufia2CpuState *cpu = table->cpu;
    if (!TableCall(table, 0xdb90u, 0u, 0xdedcu) ||
        !TableCall(table, 0xdb93u, 0u, 0xe92au) ||
        !TableCall(table, 0xdb96u, 0u, 0xe30fu)) return false;
    OpLda(memory, cpu, OpAbs(cpu, TABLE_VARIANT));
    OpAndValue(cpu, 6u);
    bool draw_hand = cpu->zero != 0u;
    if (!draw_hand) {
        if (!TableCall(table, 0xdba1u, 0u, 0xdc87u)) return false;
        OpDecA(cpu);
        draw_hand = !cpu->zero;
    }
    if (draw_hand && !TableCall(table, 0xdbaau, 0u, 0xdeb9u)) return false;
    if (!TableCall(table, 0xdbadu, 0u, 0xe9b1u)) return false;
    RememberTableHand(memory, cpu);
    return TableCall(table, 0xdbc1u, 0u, 0xe829u) &&
           TableCall(table, 0xdbc4u, 0u, 0xe334u);
}

static bool PayTableAward(TableSession *table) {
    const Lufia2Memory *memory = table->memory;
    Lufia2CpuState *cpu = table->cpu;
    OpLda(memory, cpu, OpAbs(cpu, TABLE_REMAINING));
    if (!cpu->zero) {
        OpStepMem(memory, cpu, OpAbs(cpu, TABLE_REMAINING), -1);
        OpLoadA(cpu, 10u);
        OpSta(memory, cpu, OpAbs(cpu, TABLE_PAYOUT_STEP));
        do {
            OpLda(memory, cpu, OpAbs(cpu, TABLE_REMAINING));
            OpSta(memory, cpu, OpAbs(cpu, TABLE_PAYOUT_INDEX));
            OpLdx(cpu, 1u);
            OpWriteX(memory, cpu, OpAbs(cpu, TABLE_PAYOUT_STATE), cpu->x);
            OpLoadA(cpu, 0x22u);
            if (!TableCall(table, 0xdbe7u, 0u, 0xa607u) ||
                !TableCall(table, 0xdbeau, 0u, 0xeb36u)) return false;
            OpLda(memory, cpu, OpAbs(cpu, TABLE_PAYOUT_VALUE));
            OpSta(memory, cpu, OpAbs(cpu, TABLE_PAYOUT_COPY));
            OpLoadA(cpu, 1u);
            OpSta(memory, cpu, OpAbs(cpu, TABLE_PAYOUT_ACTIVE));
            if (!TableCall(table, 0xdbf9u, 0u, 0xebdbu) ||
                !TableCall(table, 0xdbfcu, 0u, 0xa421u)) return false;
            OpSepWidths(cpu, 0x20u);
            PrepareTableText(table);
            OpRepWidths(cpu, 0x20u);
            OpLdx(cpu, 0x3694u);
            OpLdy(cpu, 0xf945u);
            if (!TableCall(table, 0xdc12u, 0u, 0x808878u)) return false;
            OpSepWidths(cpu, 0x20u);
            OpLoadA(cpu, 0x88u);
            OpSta(memory, cpu, OpDp(cpu, TABLE_TEXT_DELAY));
            if (!TableCall(table, 0xdc1cu, 0u, 0xe9b1u)) return false;
            OpRepWidths(cpu, 0x20u);
            OpStepMem(memory, cpu, OpAbs(cpu, TABLE_REMAINING), -1);
        } while (!cpu->negative);
    }
    OpLda(memory, cpu, OpAbs(cpu, TABLE_VARIANT));
    OpAndValue(cpu, 6u);
    if (!cpu->zero) {
        OpLoadA(cpu, 0x22u);
        if (!TableCall(table, 0xdc31u, 0u, 0xa60cu)) return false;
    }
    return true;
}

static bool RunTableHands(TableSession *table) {
    const Lufia2Memory *memory = table->memory;
    Lufia2CpuState *cpu = table->cpu;
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, TABLE_SESSION_MODE));
    while (TableCanContinue(table)) {
        if (!TableCall(table, 0xdb7fu, 0u, 0xed9eu) ||
            !TableCall(table, 0xdb82u, 0u, 0xdc60u) ||
            !TableCall(table, 0xdb85u, 0u, 0xe194u)) return false;
        OpLda(memory, cpu, OpAbs(cpu, TABLE_SELECTION));
        if (cpu->zero) break;
        if (!PlayTableHand(table)) return false;
    }
    return PayTableAward(table);
}

static bool RunAlternateTableHands(TableSession *table) {
    const Lufia2Memory *memory = table->memory;
    Lufia2CpuState *cpu = table->cpu;
    OpLda(memory, cpu, OpDp(cpu, TABLE_INPUT));
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, TABLE_STAKE));
    OpStz(memory, cpu, OpAbs(cpu, TABLE_SESSION_MODE));
    while (TableCanContinue(table)) {
        if (!TableCall(table, 0u, 0xf471u, 0xed9eu) ||
            !TableCall(table, 0u, 0xf474u, 0xdc60u) ||
            !TableCall(table, 0u, 0xf477u, 0xf648u)) return false;
        OpLda(memory, cpu, OpAbs(cpu, TABLE_SELECTION));
        if (cpu->zero) break;
        if (!TableCall(table, 0u, 0xf482u, 0xe92au)) return false;
        OpLoadA(cpu, 10u);
        if (!TableCall(table, 0u, 0xf488u, 0x808299u)) return false;
        cpu->carry = 0u;
        OpAdcValue(cpu, 10u);
        OpLdy(cpu, cpu->accumulator);
        do {
            if (!TableCall(table, 0u, 0xf491u, 0xe9b1u)) return false;
            OpDey(cpu);
        } while (!cpu->zero);
        if (!TableCall(table, 0u, 0xf497u, 0xf4c6u) ||
            !TableCall(table, 0u, 0xf49au, 0xe9b1u) ||
            !TableCall(table, 0u, 0xf49du, 0xe829u) ||
            !TableCall(table, 0u, 0xf4a0u, 0xf53cu)) return false;
    }
    return true;
}

static Lufia2ExecutionResult RunTableSession(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, bool alternate) {
    const uint32_t entry = alternate ? 0x84f38au : 0x84da9bu;
    if (!child || !TableReady(cpu)) return ExecutionHandoff(cpu, entry);
    TableSession table = {memory, cpu, child, context,
        ExecutionReturned(alternate ? 0x84f4c5u : 0x84dc54u), 0u, alternate};
    if (!OpenTableSession(&table)) return table.result;
    if (alternate ? !RunAlternateTableHands(&table) : !RunTableHands(&table))
        return table.result;
    CloseTableSession(&table);
    return table.result;
}

Lufia2ExecutionResult Lufia2CardTableRunSession(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return RunTableSession(memory, cpu, child, context, false);
}

Lufia2ExecutionResult Lufia2CardTableRunAlternateSession(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return RunTableSession(memory, cpu, child, context, true);
}
