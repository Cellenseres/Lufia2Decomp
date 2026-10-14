#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "lufia2/field.h"
#include "system/wram.h"

static uint8_t DiagnosticsControlChild(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                       Lufia2PushedChildCall child, void *context,
                                       uint32_t site, uint32_t target, uint8_t frame,
                                       uint8_t index_is_8_bit,
                                       Lufia2ExecutionResult *result) {
    if (!CallChildWithFrame(memory, cpu, child, context, site, target, frame, 0x85u)) {
        *result = (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return 0u;
    }
    if (cpu->program_bank != 0x85u || cpu->data_bank != 0x85u ||
        !cpu->accumulator_is_8_bit || cpu->index_is_8_bit != index_is_8_bit ||
        cpu->direct_page || cpu->decimal) {
        *result = ExecutionHandoff(cpu, site + frame + 1u);
        return 0u;
    }
    return 1u;
}

enum {
    DP_SAVE_MENU_MODE = 0x02u,
    DP_SAVE_MENU_FILE = 0x03u,
    DP_SAVE_MENU_FILE_FLAGS = 0x06u,
    DP_SAVE_MENU_TEXT_BANK = 0x5fu,
    DP_SAVE_MENU_UPLOAD = 0x74u,
    ROM_DIAGNOSTICS_CLEAR = 0x8584a9u,
    ROM_DIAGNOSTICS_SHOW_CURSOR = 0x858274u,
    ROM_DIAGNOSTICS_HIDE_CURSOR = 0x858267u,
    ROM_DIAGNOSTICS_WAIT_FRAME = 0x85851cu,
    ROM_DIAGNOSTICS_HIGH_BUTTONS = 0x85850eu,
    ROM_DIAGNOSTICS_LOW_BUTTONS = 0x858515u,
    ROM_SAVE_MENU_DRAW_STRING = 0x808878u,
    ROM_SAVE_MENU_TEST_FLAGS = 0x80905fu,
    ROM_SAVE_MENU_SAVE_FILE = 0x8090c9u,
    ROM_SAVE_MENU_LOAD_FILE = 0x809099u,
    ROM_SAVE_MENU_RESET_SCENE = 0x83adcau,
    SRAM_SAVE_MENU_FLAGS = 0x700000u,
    SAVE_MENU_SELECTION_PASSES = 1024u
};

static void ClearLoadedSaveTilemap(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    OpLoadA(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpRepWidths(cpu, 0x30u);
    OpLdx(cpu, 0x7fcu);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, 0x3000u));
        OpStz(memory, cpu, OpAbsX(cpu, 0x3002u));
        for (unsigned word = 0u; word < 4u; ++word)
            OpDex(cpu);
    } while (!cpu->negative);
    OpSepWidths(cpu, 0x20u);
    PullDataBank(memory, cpu);
}

Lufia2ExecutionResult Lufia2FieldDiagnosticsSaveMenu(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu,
                                                          Lufia2PushedChildCall child,
                                                          void *context) {
    if (cpu->program_bank != 0x85u || cpu->data_bank != 0x85u ||
        !cpu->accumulator_is_8_bit || cpu->direct_page || cpu->decimal || !child)
        return ExecutionHandoff(cpu, 0x858322u);
    Lufia2ExecutionResult result = {0};
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    if (!DiagnosticsControlChild(memory, cpu, child, context, 0x858326u,
                                 ROM_DIAGNOSTICS_CLEAR, 2u, 0u, &result))
        return result;
    OpLoadA(cpu, 0x85u);
    OpSta(memory, cpu, OpDp(cpu, DP_SAVE_MENU_TEXT_BANK));
    OpLdy(cpu, 0x8610u);
    if (!DiagnosticsControlChild(memory, cpu, child, context, 0x858330u,
                                 ROM_SAVE_MENU_DRAW_STRING, 3u, 0u, &result))
        return result;
    OpLda(memory, cpu, SRAM_SAVE_MENU_FLAGS);
    OpAndValue(cpu, 3u);
    OpSta(memory, cpu, OpDp(cpu, DP_SAVE_MENU_FILE));
    OpStz(memory, cpu, OpDp(cpu, DP_SAVE_MENU_MODE));
    OpStz(memory, cpu, OpDp(cpu, DP_SAVE_MENU_FILE_FLAGS));
    OpLdx(cpu, 3u);
    do {
        OpTxa(cpu);
        if (!DiagnosticsControlChild(memory, cpu, child, context, 0x858344u,
                                     ROM_SAVE_MENU_TEST_FLAGS, 3u, 0u, &result))
            return result;
        OpRolMem8(memory, cpu, OpDp(cpu, DP_SAVE_MENU_FILE_FLAGS));
        OpDex(cpu);
    } while (!cpu->negative);
    OpLda(memory, cpu, OpDp(cpu, DP_SAVE_MENU_FILE_FLAGS));
    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
    OpSta(memory, cpu, OpDp(cpu, DP_SAVE_MENU_FILE_FLAGS));
    OpLoadA(cpu, 0x85u);
    OpSta(memory, cpu, OpDp(cpu, DP_SAVE_MENU_TEXT_BANK));
    OpLdy(cpu, 0x867fu);
    if (!DiagnosticsControlChild(memory, cpu, child, context, 0x85835au,
                                 ROM_SAVE_MENU_DRAW_STRING, 3u, 0u, &result))
        return result;
    unsigned passes = 0u;
    for (;;) {
        if (!DiagnosticsControlChild(memory, cpu, child, context, 0x85835eu,
                                     ROM_DIAGNOSTICS_SHOW_CURSOR, 2u, 0u, &result))
            return result;
        OpLoadA(cpu, 8u);
        OpSta(memory, cpu, OpDp(cpu, DP_SAVE_MENU_UPLOAD));
        if (!DiagnosticsControlChild(memory, cpu, child, context, 0x858365u,
                                     ROM_DIAGNOSTICS_WAIT_FRAME, 2u, 0u, &result))
            return result;
        OpLoadA(cpu, 8u);
        if (!DiagnosticsControlChild(memory, cpu, child, context, 0x85836au,
                                     ROM_DIAGNOSTICS_HIGH_BUTTONS, 2u, 0u, &result))
            return result;
        if (!cpu->zero) {
            if (!DiagnosticsControlChild(memory, cpu, child, context, 0x85836fu,
                                         ROM_DIAGNOSTICS_HIDE_CURSOR, 2u, 0u, &result))
                return result;
            OpStepMem(memory, cpu, OpDp(cpu, DP_SAVE_MENU_FILE), -1);
            if (cpu->negative) {
                OpLoadA(cpu, 3u);
                OpSta(memory, cpu, OpDp(cpu, DP_SAVE_MENU_FILE));
            }
        } else {
            OpLoadA(cpu, 4u);
            if (!DiagnosticsControlChild(memory, cpu, child, context, 0x85837eu,
                                         ROM_DIAGNOSTICS_HIGH_BUTTONS, 2u, 0u, &result))
                return result;
            if (!cpu->zero) {
                if (!DiagnosticsControlChild(memory, cpu, child, context, 0x858383u,
                                             ROM_DIAGNOSTICS_HIDE_CURSOR, 2u, 0u,
                                             &result))
                    return result;
                OpLda(memory, cpu, OpDp(cpu, DP_SAVE_MENU_FILE));
                OpIncA(cpu);
                OpSta(memory, cpu, OpDp(cpu, DP_SAVE_MENU_FILE));
                OpCmpValue(cpu, 4u);
                if (cpu->carry)
                    OpStz(memory, cpu, OpDp(cpu, DP_SAVE_MENU_FILE));
            } else {
                OpLoadA(cpu, 3u);
                if (!DiagnosticsControlChild(memory, cpu, child, context, 0x858395u,
                                             ROM_DIAGNOSTICS_HIGH_BUTTONS, 2u, 0u,
                                             &result))
                    return result;
                if (!cpu->zero) {
                    if (!DiagnosticsControlChild(memory, cpu, child, context, 0x85839au,
                                                 ROM_DIAGNOSTICS_HIDE_CURSOR, 2u, 0u,
                                                 &result))
                        return result;
                    OpLda(memory, cpu, OpDp(cpu, DP_SAVE_MENU_MODE));
                    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 1u));
                    OpSta(memory, cpu, OpDp(cpu, DP_SAVE_MENU_MODE));
                } else {
                    OpLoadA(cpu, 0x90u);
                    if (!DiagnosticsControlChild(memory, cpu, child, context, 0x8583a7u,
                                                 ROM_DIAGNOSTICS_HIGH_BUTTONS, 2u, 0u,
                                                 &result))
                        return result;
                    if (!cpu->zero) {
                        cpu->carry = 1u;
                        return ExecutionReturned(0x8583adu);
                    }
                    OpLoadA(cpu, 0x80u);
                    if (!DiagnosticsControlChild(memory, cpu, child, context, 0x8583b0u,
                                                 ROM_DIAGNOSTICS_LOW_BUTTONS, 2u, 0u,
                                                 &result))
                        return result;
                    if (!cpu->zero)
                        break;
                }
            }
        }
        if (++passes == SAVE_MENU_SELECTION_PASSES)
            return ExecutionHandoff(cpu, 0x85835eu);
    }
    OpLda(memory, cpu, SRAM_SAVE_MENU_FLAGS);
    OpAndValue(cpu, 0xfcu);
    OpOra(memory, cpu, OpDp(cpu, DP_SAVE_MENU_FILE));
    OpSta(memory, cpu, SRAM_SAVE_MENU_FLAGS);
    OpLda(memory, cpu, OpDp(cpu, DP_SAVE_MENU_MODE));
    if (!cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, DP_SAVE_MENU_FILE));
        if (!DiagnosticsControlChild(memory, cpu, child, context, 0x8583cau,
                                     ROM_SAVE_MENU_SAVE_FILE, 3u, 0u, &result))
            return result;
    } else {
        OpLoadA(cpu, 0x80u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
        OpLda(memory, cpu, OpDp(cpu, DP_SAVE_MENU_FILE));
        PushAccumulator8(memory, cpu);
        if (!DiagnosticsControlChild(memory, cpu, child, context, 0x8583d8u,
                                     ROM_SAVE_MENU_RESET_SCENE, 3u, 0u, &result))
            return result;
        LoadA8(cpu, Pull8(memory, cpu));
        if (!DiagnosticsControlChild(memory, cpu, child, context, 0x8583ddu,
                                     ROM_SAVE_MENU_LOAD_FILE, 3u, 0u, &result))
            return result;
        if (!cpu->carry) {
            if (!DiagnosticsControlChild(memory, cpu, child, context, 0x8583e3u,
                                         ROM_DIAGNOSTICS_WAIT_FRAME, 2u, 0u, &result))
                return result;
            ClearLoadedSaveTilemap(memory, cpu);
            OpLoadA(cpu, 8u);
            OpSta(memory, cpu, OpDp(cpu, DP_SAVE_MENU_UPLOAD));
            OpRepWidths(cpu, 0x20u);
            OpLoadA(cpu, 0x1fffu);
            cpu->stack = cpu->accumulator;
            OpSepWidths(cpu, 0x20u);
            OpLoadA(cpu, 0x83u);
            PushAccumulator8(memory, cpu);
            PullDataBank(memory, cpu);
            cpu->program_bank = 0x83u;
            return ExecutionHandoff(cpu, 0x83ad23u);
        }
    }
    cpu->carry = 0u;
    return ExecutionReturned(0x858414u);
}

enum {
    DP_MESSAGE_TEXT_BANK = 0x5fu,
    DP_MESSAGE_UPLOAD = 0x74u,
    DP_MESSAGE_BUTTONS = 0x47u,
    DP_MESSAGE_PRESSED = 0x4bu,
    MESSAGE_POLL_BUDGET = 4096u,
    MESSAGE_PASS_BUDGET = 1024u
};

Lufia2ExecutionResult
Lufia2FieldDiagnosticsShowMessage(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                       Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit || cpu->direct_page ||
        cpu->decimal || !child)
        return ExecutionHandoff(cpu, 0x858415u);
    Lufia2ExecutionResult result = {0};
    PushAccumulator8(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpLoadA(cpu, 0x0fu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E0563));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
    PushAccumulator8(memory, cpu);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
    if (!DiagnosticsControlChild(memory, cpu, child, context, 0x85842eu, 0x85851cu, 2u,
                                 0u, &result) ||
        !DiagnosticsControlChild(memory, cpu, child, context, 0x858431u, 0x83b007u, 3u,
                                 0u, &result))
        return result;
    OpLoadA(cpu, 0x0fu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FADE_CONTROL));
    OpLoadA(cpu, 0xf3u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_CGADSUB));
    OpLoadA(cpu, 0xe7u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_COLDATA));
    OpLoadA(cpu, 0x0fu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_TM));
    if (!DiagnosticsControlChild(memory, cpu, child, context, 0x85844cu, 0x8584a9u, 2u,
                                 0u, &result))
        return result;
    OpStz(memory, cpu, OpAbs(cpu, WRAM_NMI_SCROLL_REGISTERS + 8u));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_NMI_SCROLL_REGISTERS + 9u));
    OpLoadA(cpu, 0x85u);
    OpSta(memory, cpu, OpDp(cpu, DP_MESSAGE_TEXT_BANK));
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    OpTxy(cpu);
    OpLdx(cpu, 0x3046u);
    if (!DiagnosticsControlChild(memory, cpu, child, context, 0x858462u, 0x808878u, 3u,
                                 0u, &result))
        return result;
    OpSepWidths(cpu, 0x10u);
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpDp(cpu, DP_MESSAGE_UPLOAD));
    if (!DiagnosticsControlChild(memory, cpu, child, context, 0x85846cu, 0x85851cu, 2u,
                                 1u, &result))
        return result;
    unsigned passes = 0u;
    for (;;) {
        OpLoadA(cpu, 0x10u);
        OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_MESSAGE_BUTTONS)));
        if (!cpu->zero) {
            unsigned polls = 0u;
            do {
                OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_MESSAGE_BUTTONS)));
                if (cpu->zero)
                    break;
                if (++polls == MESSAGE_POLL_BUDGET)
                    return ExecutionHandoff(cpu, 0x858475u);
            } while (1);
            if (!DiagnosticsControlChild(memory, cpu, child, context, 0x858479u,
                                         0x858045u, 3u, 1u, &result))
                return result;
            OpLoadA(cpu, 0x0fu);
            OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
            break;
        }
        OpLoadA(cpu, 0x80u);
        OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_MESSAGE_BUTTONS)));
        if (!cpu->zero) {
            OpTestBits(memory, cpu, OpDp(cpu, DP_MESSAGE_PRESSED), 0u);
            if (!cpu->zero) {
                if (!DiagnosticsControlChild(memory, cpu, child, context, 0x85848eu,
                                             0x8584a9u, 2u, 1u, &result))
                    return result;
                OpLoadA(cpu, 8u);
                OpSta(memory, cpu, OpDp(cpu, DP_MESSAGE_UPLOAD));
                if (!DiagnosticsControlChild(memory, cpu, child, context, 0x858495u,
                                             0x85851cu, 2u, 1u, &result))
                    return result;
                break;
            }
        }
        if (++passes == MESSAGE_PASS_BUDGET)
            return ExecutionHandoff(cpu, 0x85846fu);
    }
    if (!DiagnosticsControlChild(memory, cpu, child, context, 0x858498u, 0x80f2f3u, 3u,
                                 1u, &result))
        return result;
    LoadA8(cpu, Pull8(memory, cpu));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E0563));
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    LoadA8(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x8584a8u);
}

enum {
    DP_DIAGNOSTICS_PAGE = 0x00u,
    DP_DIAGNOSTICS_COLUMN = 0x02u,
    DP_DIAGNOSTICS_ROW = 0x03u,
    DP_DIAGNOSTICS_FIELD_VALUES = 0x0du,
    DP_DIAGNOSTICS_PPU1 = 0x22u,
    DP_DIAGNOSTICS_PPU2 = 0x23u,
    DP_DIAGNOSTICS_TEXT_BANK = 0x5fu,
    DP_DIAGNOSTICS_BUTTONS = 0x47u,
    DP_DIAGNOSTICS_UPLOAD = 0x74u,
    DIAGNOSTICS_SELECTION_BUDGET = 1024u,
    DIAGNOSTICS_RELEASE_BUDGET = 4096u,
    PPU_DIAGNOSTICS_STATUS1 = 0x213eu,
    PPU_DIAGNOSTICS_STATUS2 = 0x213fu
};

typedef enum DiagnosticsMenuAction {
    DIAGNOSTICS_MENU_HANDOFF,
    DIAGNOSTICS_MENU_REDRAW,
    DIAGNOSTICS_MENU_PPU,
    DIAGNOSTICS_MENU_FINISH,
    DIAGNOSTICS_MENU_RETURN
} DiagnosticsMenuAction;

static uint8_t MenuButton(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                          Lufia2PushedChildCall child, void *context, uint8_t mask,
                          uint32_t site, uint8_t high, Lufia2ExecutionResult *result) {
    OpLoadA(cpu, mask);
    return DiagnosticsControlChild(memory, cpu, child, context, site,
                                   high ? 0x85850eu : 0x858515u, 2u, 0u, result);
}

static uint8_t DrawDiagnosticsValues(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                     Lufia2PushedChildCall child, void *context,
                                     Lufia2ExecutionResult *result) {
    if (!DiagnosticsControlChild(memory, cpu, child, context, 0x85807du, 0x8584a9u, 2u,
                                 0u, result))
        return 0u;
    OpLdy(cpu, 0x85b0u);
    OpLda(memory, cpu, OpDp(cpu, DP_DIAGNOSTICS_FIELD_VALUES));
    if (!cpu->zero)
        OpLdy(cpu, 0x85cdu);
    OpLoadA(cpu, 0x85u);
    OpSta(memory, cpu, OpDp(cpu, DP_DIAGNOSTICS_TEXT_BANK));
    return DiagnosticsControlChild(memory, cpu, child, context, 0x85808eu, 0x808878u,
                                   3u, 0u, result) &&
           DiagnosticsControlChild(memory, cpu, child, context, 0x858092u, 0x85829du,
                                   2u, 0u, result) &&
           DiagnosticsControlChild(memory, cpu, child, context, 0x858095u, 0x858274u,
                                   2u, 0u, result);
}

static DiagnosticsMenuAction ReadDiagnosticsSelection(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu,
                                                      Lufia2PushedChildCall child,
                                                      void *context,
                                                      Lufia2ExecutionResult *result) {
    unsigned passes = 0u;
    for (;;) {
        if (!DiagnosticsControlChild(memory, cpu, child, context, 0x858098u, 0x85851cu,
                                     2u, 0u, result) ||
            !MenuButton(memory, cpu, child, context, 0x20u, 0x85809du, 0u, result))
            return DIAGNOSTICS_MENU_HANDOFF;
        if (!cpu->zero) {
            OpLda(memory, cpu, OpDp(cpu, DP_DIAGNOSTICS_PAGE));
            cpu->carry = 1u;
            OpSbcValue(cpu, 0x10u);
            OpSta(memory, cpu, OpDp(cpu, DP_DIAGNOSTICS_PAGE));
            return DIAGNOSTICS_MENU_REDRAW;
        }
        if (!MenuButton(memory, cpu, child, context, 0x40u, 0x8580adu, 1u, result))
            return DIAGNOSTICS_MENU_HANDOFF;
        if (!cpu->zero)
            return DIAGNOSTICS_MENU_PPU;
        if (!MenuButton(memory, cpu, child, context, 0x10u, 0x8580b7u, 0u, result))
            return DIAGNOSTICS_MENU_HANDOFF;
        if (!cpu->zero) {
            OpLda(memory, cpu, OpDp(cpu, DP_DIAGNOSTICS_PAGE));
            cpu->carry = 0u;
            OpAdcValue(cpu, 0x10u);
            OpSta(memory, cpu, OpDp(cpu, DP_DIAGNOSTICS_PAGE));
            return DIAGNOSTICS_MENU_REDRAW;
        }
        if (!MenuButton(memory, cpu, child, context, 3u, 0x8580c8u, 1u, result))
            return DIAGNOSTICS_MENU_HANDOFF;
        if (!cpu->zero) {
            if (!DiagnosticsControlChild(memory, cpu, child, context, 0x8580cdu,
                                         0x858267u, 2u, 0u, result))
                return DIAGNOSTICS_MENU_HANDOFF;
            OpLda(memory, cpu, OpDp(cpu, DP_DIAGNOSTICS_COLUMN));
            OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 1u));
            OpSta(memory, cpu, OpDp(cpu, DP_DIAGNOSTICS_COLUMN));
            return DIAGNOSTICS_MENU_REDRAW;
        }
        if (!MenuButton(memory, cpu, child, context, 8u, 0x8580dbu, 1u, result))
            return DIAGNOSTICS_MENU_HANDOFF;
        if (!cpu->zero) {
            if (!DiagnosticsControlChild(memory, cpu, child, context, 0x8580e0u,
                                         0x858267u, 2u, 0u, result))
                return DIAGNOSTICS_MENU_HANDOFF;
            OpLda(memory, cpu, OpDp(cpu, DP_DIAGNOSTICS_ROW));
            OpDecA(cpu);
            OpAndValue(cpu, 0x0fu);
            OpSta(memory, cpu, OpDp(cpu, DP_DIAGNOSTICS_ROW));
            return DIAGNOSTICS_MENU_REDRAW;
        }
        if (!MenuButton(memory, cpu, child, context, 4u, 0x8580efu, 1u, result))
            return DIAGNOSTICS_MENU_HANDOFF;
        if (!cpu->zero) {
            if (!DiagnosticsControlChild(memory, cpu, child, context, 0x8580f4u,
                                         0x858267u, 2u, 0u, result))
                return DIAGNOSTICS_MENU_HANDOFF;
            OpLda(memory, cpu, OpDp(cpu, DP_DIAGNOSTICS_ROW));
            OpIncA(cpu);
            OpAndValue(cpu, 0x0fu);
            OpSta(memory, cpu, OpDp(cpu, DP_DIAGNOSTICS_ROW));
            return DIAGNOSTICS_MENU_REDRAW;
        }
        if (!MenuButton(memory, cpu, child, context, 0x80u, 0x858103u, 0u, result))
            return DIAGNOSTICS_MENU_HANDOFF;
        if (!cpu->zero) {
            OpLoadA(cpu, 1u);
            if (!DiagnosticsControlChild(memory, cpu, child, context, 0x85810au,
                                         0x85821cu, 2u, 0u, result))
                return DIAGNOSTICS_MENU_HANDOFF;
            return DIAGNOSTICS_MENU_REDRAW;
        }
        if (!MenuButton(memory, cpu, child, context, 0x80u, 0x858112u, 1u, result))
            return DIAGNOSTICS_MENU_HANDOFF;
        if (!cpu->zero) {
            OpLoadA(cpu, 0xffu);
            if (!DiagnosticsControlChild(memory, cpu, child, context, 0x858119u,
                                         0x85821cu, 2u, 0u, result))
                return DIAGNOSTICS_MENU_HANDOFF;
            return DIAGNOSTICS_MENU_REDRAW;
        }
        if (!MenuButton(memory, cpu, child, context, 0x40u, 0x858121u, 0u, result))
            return DIAGNOSTICS_MENU_HANDOFF;
        if (!cpu->zero) {
            OpLda(memory, cpu, OpDp(cpu, DP_DIAGNOSTICS_FIELD_VALUES));
            OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
            OpSta(memory, cpu, OpDp(cpu, DP_DIAGNOSTICS_FIELD_VALUES));
            OpStz(memory, cpu, OpDp(cpu, DP_DIAGNOSTICS_COLUMN));
            OpStz(memory, cpu, OpDp(cpu, DP_DIAGNOSTICS_ROW));
            return DIAGNOSTICS_MENU_REDRAW;
        }
        if (!MenuButton(memory, cpu, child, context, 0x20u, 0x858135u, 1u, result))
            return DIAGNOSTICS_MENU_HANDOFF;
        if (!cpu->zero) {
            if (!DiagnosticsControlChild(memory, cpu, child, context, 0x85813au,
                                         0x858322u, 2u, 0u, result))
                return DIAGNOSTICS_MENU_HANDOFF;
            if (!cpu->carry)
                return DIAGNOSTICS_MENU_FINISH;
            OpStz(memory, cpu, OpDp(cpu, DP_DIAGNOSTICS_COLUMN));
            OpStz(memory, cpu, OpDp(cpu, DP_DIAGNOSTICS_ROW));
            return DIAGNOSTICS_MENU_REDRAW;
        }
        if (!MenuButton(memory, cpu, child, context, 0x10u, 0x858143u, 1u, result))
            return DIAGNOSTICS_MENU_HANDOFF;
        if (!cpu->zero)
            return DIAGNOSTICS_MENU_FINISH;
        if (++passes == DIAGNOSTICS_SELECTION_BUDGET) {
            *result = ExecutionHandoff(cpu, 0x858098u);
            return DIAGNOSTICS_MENU_HANDOFF;
        }
    }
}

static DiagnosticsMenuAction RunPpuDiagnostics(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu,
                                               Lufia2PushedChildCall child,
                                               void *context,
                                               Lufia2ExecutionResult *result) {
    if (!DiagnosticsControlChild(memory, cpu, child, context, 0x858178u, 0x8584a9u, 2u,
                                 0u, result))
        return DIAGNOSTICS_MENU_HANDOFF;
    OpLda(memory, cpu, PPU_DIAGNOSTICS_STATUS1);
    OpAndValue(cpu, 0x0fu);
    OpSta(memory, cpu, OpDp(cpu, DP_DIAGNOSTICS_PPU1));
    OpLda(memory, cpu, PPU_DIAGNOSTICS_STATUS2);
    OpAndValue(cpu, 0x0fu);
    OpSta(memory, cpu, OpDp(cpu, DP_DIAGNOSTICS_PPU2));
    OpLoadA(cpu, 0x85u);
    OpSta(memory, cpu, OpDp(cpu, DP_DIAGNOSTICS_TEXT_BANK));
    OpLdy(cpu, 0x87d7u);
    if (!DiagnosticsControlChild(memory, cpu, child, context, 0x858192u, 0x808878u, 3u,
                                 0u, result))
        return DIAGNOSTICS_MENU_HANDOFF;
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpDp(cpu, DP_DIAGNOSTICS_UPLOAD));
    unsigned passes = 0u;
    for (;;) {
        if (!MenuButton(memory, cpu, child, context, 0x80u, 0x85819cu, 0u, result))
            return DIAGNOSTICS_MENU_HANDOFF;
        if (!cpu->zero) {
            OpLoadA(cpu, 2u);
            OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09C9));
            OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID));
            OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_RETURN_MAP));
            OpStz(memory, cpu, OpAbs(cpu, WRAM_FIELD_DESTINATION_MAP));
            OpStz(memory, cpu, OpAbs(cpu, WRAM_FIELD_DESTINATION_MAP + 1u));
            OpLoadA(cpu, 0x88u);
            OpTestBits(memory, cpu, OpAbs(cpu, WRAM_FIELD_FLAGS), 1u);
            OpLoadA(cpu, 0x80u);
            OpTestBits(memory, cpu, OpAbs(cpu, WRAM_FIELD_RELOAD_FLAGS), 1u);
            return DIAGNOSTICS_MENU_FINISH;
        }
        if (!MenuButton(memory, cpu, child, context, 0x40u, 0x8581c1u, 1u, result))
            return DIAGNOSTICS_MENU_HANDOFF;
        if (!cpu->zero) {
            if (!DiagnosticsControlChild(memory, cpu, child, context, 0x8581c6u,
                                         0x80fbceu, 3u, 0u, result))
                return DIAGNOSTICS_MENU_HANDOFF;
            return DIAGNOSTICS_MENU_FINISH;
        }
        if (!MenuButton(memory, cpu, child, context, 0x10u, 0x8581cfu, 1u, result))
            return DIAGNOSTICS_MENU_HANDOFF;
        if (!cpu->zero)
            return DIAGNOSTICS_MENU_FINISH;
        if (!MenuButton(memory, cpu, child, context, 0x80u, 0x8581d6u, 1u, result))
            return DIAGNOSTICS_MENU_HANDOFF;
        if (!cpu->zero) {
            OpLoadA(cpu, 0xa0u);
            OpSta(memory, cpu, OpDp(cpu, DP_DIAGNOSTICS_UPLOAD));
            return DIAGNOSTICS_MENU_FINISH;
        }
        if (!MenuButton(memory, cpu, child, context, 0x40u, 0x8581e4u, 0u, result))
            return DIAGNOSTICS_MENU_HANDOFF;
        if (!cpu->zero) {
            OpLoadA(cpu, 0x20u);
            OpTestBits(memory, cpu, OpAbs(cpu, WRAM_FIELD_FLAGS), 1u);
            return DIAGNOSTICS_MENU_FINISH;
        }
        if (!MenuButton(memory, cpu, child, context, 0x10u, 0x8581f3u, 0u, result))
            return DIAGNOSTICS_MENU_HANDOFF;
        if (!cpu->zero) {
            OpLoadA(cpu, 1u);
            OpTestBits(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09A9), 0u);
            OpLoadA(cpu, 0x0fu);
            OpSta(memory, cpu, OpAbs(cpu, SNES_TM));
            OpLoadA(cpu, 0u);
            OpSta(memory, cpu, OpAbs(cpu, SNES_CGADSUB));
            OpLoadA(cpu, 0u);
            OpSta(memory, cpu, OpAbs(cpu, SNES_COLDATA));
            if (!DiagnosticsControlChild(memory, cpu, child, context, 0x85820cu,
                                         0x8584a9u, 2u, 0u, result))
                return DIAGNOSTICS_MENU_HANDOFF;
            OpLoadA(cpu, 8u);
            OpSta(memory, cpu, OpDp(cpu, DP_DIAGNOSTICS_UPLOAD));
            if (!DiagnosticsControlChild(memory, cpu, child, context, 0x858213u,
                                         0x85851cu, 2u, 0u, result))
                return DIAGNOSTICS_MENU_HANDOFF;
            return DIAGNOSTICS_MENU_RETURN;
        }
        if (++passes == DIAGNOSTICS_SELECTION_BUDGET) {
            *result = ExecutionHandoff(cpu, 0x85819au);
            return DIAGNOSTICS_MENU_HANDOFF;
        }
    }
}

static uint8_t FinishDiagnosticsDisplay(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                        Lufia2PushedChildCall child, void *context,
                                        Lufia2ExecutionResult *result) {
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_CGADSUB));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_COLDATA));
    OpLoadA(cpu, 0x1fu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_TM));
    OpLoadA(cpu, 1u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09A9), 1u);
    if (!DiagnosticsControlChild(memory, cpu, child, context, 0x85815fu, 0x8584a9u, 2u,
                                 0u, result))
        return 0u;
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpDp(cpu, DP_DIAGNOSTICS_UPLOAD));
    if (!DiagnosticsControlChild(memory, cpu, child, context, 0x858166u, 0x85851cu, 2u,
                                 0u, result))
        return 0u;
    OpLoadA(cpu, 0x10u);
    unsigned reads = 0u;
    do {
        OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_DIAGNOSTICS_BUTTONS)));
        if (cpu->zero)
            return 1u;
    } while (++reads != DIAGNOSTICS_RELEASE_BUDGET);
    *result = ExecutionHandoff(cpu, 0x85816bu);
    return 0u;
}

Lufia2ExecutionResult Lufia2FieldDiagnosticsRunMenu(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu,
                                                         Lufia2PushedChildCall child,
                                                         void *context) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
        !cpu->index_is_8_bit || cpu->direct_page || cpu->decimal || !child)
        return ExecutionHandoff(cpu, 0x858045u);
    Lufia2ExecutionResult result = {0};
    PushAccumulator8(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpLoadA(cpu, 0x0fu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E0563));
    OpStz(memory, cpu, OpDp(cpu, DP_DIAGNOSTICS_PAGE));
    OpStz(memory, cpu, OpDp(cpu, DP_DIAGNOSTICS_COLUMN));
    OpStz(memory, cpu, OpDp(cpu, DP_DIAGNOSTICS_ROW));
    OpStz(memory, cpu, OpDp(cpu, DP_DIAGNOSTICS_FIELD_VALUES));
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    OpLoadA(cpu, 0xf3u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_CGADSUB));
    OpLoadA(cpu, 0xe7u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_COLDATA));
    OpLoadA(cpu, 0x0fu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_TM));
    for (unsigned byte = 0u; byte < 4u; ++byte)
        OpStz(memory, cpu,
              OpAbs(cpu, (uint16_t)(WRAM_NMI_SCROLL_REGISTERS + 8u + byte)));
    unsigned redraws = 0u;
    for (;;) {
        if (!DrawDiagnosticsValues(memory, cpu, child, context, &result))
            return result;
        DiagnosticsMenuAction action =
            ReadDiagnosticsSelection(memory, cpu, child, context, &result);
        if (action == DIAGNOSTICS_MENU_REDRAW) {
            if (++redraws == DIAGNOSTICS_SELECTION_BUDGET)
                return ExecutionHandoff(cpu, 0x85807du);
            continue;
        }
        if (action == DIAGNOSTICS_MENU_PPU)
            action = RunPpuDiagnostics(memory, cpu, child, context, &result);
        if (action == DIAGNOSTICS_MENU_HANDOFF)
            return result;
        if (action == DIAGNOSTICS_MENU_FINISH &&
            !FinishDiagnosticsDisplay(memory, cpu, child, context, &result))
            return result;
        break;
    }
    OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E0563));
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    LoadA8(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x858177u);
}

enum { DP_DIAGNOSTICS_FRAME_COUNTER = 0x40u, DIAGNOSTICS_FRAME_READ_BUDGET = 4096u };

Lufia2ExecutionResult Lufia2FieldDiagnosticsWaitFrame(const Lufia2Memory *memory,
                                                           Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit || cpu->direct_page)
        return ExecutionHandoff(cpu, 0x85851cu);
    OpLda(memory, cpu, OpDp(cpu, DP_DIAGNOSTICS_FRAME_COUNTER));
    unsigned reads = 0u;
    do {
        OpCmp(memory, cpu, OpDp(cpu, DP_DIAGNOSTICS_FRAME_COUNTER));
        if (!cpu->zero)
            return ExecutionReturned(0x858522u);
    } while (++reads != DIAGNOSTICS_FRAME_READ_BUDGET);
    return ExecutionHandoff(cpu, 0x85851eu);
}
