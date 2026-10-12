#include "lufia2/field.h"
#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/joypad.h"
#include "system/wram.h"

enum {
    SELECTION_FRAME_POLL_LIMIT = 65536,
    SELECTION_STATE = WRAM_UNK_7E11D8,
    SELECTION_TRANSITION_LIMIT = 4096,
};

static uint8_t SelectionContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x84u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal;
}

static uint8_t SelectionChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target, uint8_t frame,
    Lufia2ExecutionResult *result) {
    if (!CallChildWithFrame(memory, cpu, child, context,
            site, target, frame, 0x84u)) {
        *result = (Lufia2ExecutionResult){
            LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return 0u;
    }
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit) {
        *result = ExecutionHandoff(cpu, site + frame + 1u);
        return 0u;
    }
    return 1u;
}

Lufia2ExecutionResult Lufia2FieldWaitSelectionFrame(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    if (cpu->program_bank != 0x84u)
        return ExecutionHandoff(cpu, 0x848363u);
    OpLda(memory, cpu, OpDp(cpu, DP_NMI_FRAME_COUNTER));
    for (uint32_t polls = 0u; polls < SELECTION_FRAME_POLL_LIMIT; ++polls) {
        OpCmp(memory, cpu, OpDp(cpu, DP_NMI_FRAME_COUNTER));
        if (!cpu->zero)
            return ExecutionReturned(0x848369u);
    }
    return ExecutionHandoff(cpu, 0x848365u);
}

Lufia2ExecutionResult Lufia2FieldTakeSelectionHighButtons(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    if (!SelectionContext(cpu))
        return ExecutionHandoff(cpu, 0x848758u);
    (void)TakeButtonPress8(memory, cpu, JOYPAD_HIGH_BYTE);
    return ExecutionReturned(0x84875eu);
}

Lufia2ExecutionResult Lufia2FieldTakeSelectionLowButtons(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    if (!SelectionContext(cpu))
        return ExecutionHandoff(cpu, 0x84875fu);
    (void)TakeButtonPress8(memory, cpu, JOYPAD_LOW_BYTE);
    return ExecutionReturned(0x848765u);
}

Lufia2ExecutionResult Lufia2FieldSetSelectionBlink(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    if (!SelectionContext(cpu))
        return ExecutionHandoff(cpu, 0x84868cu);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_ACTIVE_SLOT)));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_BLINK_COUNTER));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_OBJECT_FLAGS));
    OpOraValue(cpu, 0x10u);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_FLAGS));
    return ExecutionReturned(0x84869du);
}

Lufia2ExecutionResult Lufia2FieldResetSelectionBlink(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    if (!SelectionContext(cpu))
        return ExecutionHandoff(cpu, 0x84869eu);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_ACTIVE_SLOT)));
    TransferDirectToA(cpu);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_BLINK_COUNTER));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_OBJECT_FLAGS));
    OpAndValue(cpu, 0xefu);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_FLAGS));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_OBJECT_DRAW_FLAGS));
    OpAndValue(cpu, 0x7fu);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_DRAW_FLAGS));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_SELECTION_MODE));
    return ExecutionReturned(0x8486bfu);
}

Lufia2ExecutionResult Lufia2FieldReadSelectedItem(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    if (!SelectionContext(cpu))
        return ExecutionHandoff(cpu, 0x8486c0u);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_SELECTED_INDEX));
    if (!cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_COUNT));
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_SELECTED_INDEX)));
        OpAslA(cpu);
    }
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_MENU_LIST_COUNT + 1u));
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_MENU_LIST_COUNT));
    return ExecutionReturned(0x8486d6u);
}

Lufia2ExecutionResult Lufia2FieldSetSelectionMosaic(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!SelectionContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x848685u);
    OpSta(memory, cpu, OpAbs(cpu, 0x2106u));
    Lufia2ExecutionResult result;
    if (!SelectionChild(memory, cpu, child, context,
            0x848688u, 0x848363u, 2u, &result))
        return result;
    return ExecutionReturned(0x84868bu);
}

Lufia2ExecutionResult Lufia2FieldRotateSelectionMarker(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!SelectionContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x84863eu);
    OpSta(memory, cpu, OpDp(cpu, 0x5au));
    Lufia2ExecutionResult result;
    if (!SelectionChild(memory, cpu, child, context,
            0x848640u, 0x84869eu, 2u, &result))
        return result;
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_MARKER_SLOT)));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_MENU_ITEM_COLUMN));
    OpCmpValue(cpu, 0x30u);
    if (cpu->zero) {
        OpLoadA(cpu, 1u);
        ExchangeAccumulatorBytes(cpu);
        OpLoadA(cpu, 0xf8u);
    } else {
        TransferDirectToA(cpu);
        OpLoadA(cpu, 0x98u);
    }
    OpSta(memory, cpu, OpDp(cpu, 0x63u));
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_SELECTION_MODE));
    OpLoadA(cpu, 6u);
    OpSta(memory, cpu, OpDp(cpu, 0x58u));
    OpStz(memory, cpu, OpDp(cpu, 0x59u));
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpDp(cpu, 0x54u));
    if (!SelectionChild(memory, cpu, child, context,
            0x848667u, 0x83c6aau, 3u, &result))
        return result;
    OpLda(memory, cpu, OpDp(cpu, 0x63u));
    OpAslA(cpu);
    cpu->carry = 1u;
    if (cpu->decimal)
        return ExecutionHandoff(cpu, 0x84866fu);
    OpSbcValue(cpu, 0x40u);
    cpu->carry = 1u;
    RorA8(cpu);
    OpSta(memory, cpu, OpDp(cpu, 0x63u));
    OpStz(memory, cpu, OpDp(cpu, 0x54u));
    if (!SelectionChild(memory, cpu, child, context,
            0x848677u, 0x83c6aau, 3u, &result) ||
        !SelectionChild(memory, cpu, child, context,
            0x84867bu, 0x8486d7u, 2u, &result))
        return result;
    OpLoadA(cpu, 0u);
    if (!SelectionChild(memory, cpu, child, context,
            0x848680u, 0x848766u, 3u, &result))
        return result;
    return ExecutionReturned(0x848684u);
}

static void MeasureSelectionName(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_RECORD_BUFFER));
        if (cpu->zero)
            return;
        OpInx(cpu);
        OpCpx(cpu, 12u);
    } while (!cpu->carry);
}

Lufia2ExecutionResult Lufia2FieldDrawSelectedItemName(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!SelectionContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x8486d7u);
    OpLda(memory, cpu, (WRAM_FIELD_LAYER_SECTION_WORD + 4u));
    OpCmpValue(cpu, 0xffu);
    if (!cpu->zero)
        return ExecutionReturned(0x848757u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_SELECTION_MODE));
    Lufia2ExecutionResult result;
    if (!cpu->zero) {
        OpLoadA(cpu, 0x52u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_RECORD_BUFFER));
        OpLoadA(cpu, 0x65u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_RECORD_BUFFER + 1u));
        OpSta(memory, cpu, OpAbs(cpu, WRAM_RECORD_BUFFER + 3u));
        OpLoadA(cpu, 0x73u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_RECORD_BUFFER + 2u));
        OpLoadA(cpu, 0x74u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_RECORD_BUFFER + 4u));
        OpStz(memory, cpu, OpAbs(cpu, WRAM_RECORD_BUFFER + 5u));
        OpLdx(cpu, 0x34dau);
    } else {
        if (!SelectionChild(memory, cpu, child, context,
                0x848703u, 0x8486c0u, 2u, &result))
            return result;
        OpSta(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
        ExchangeAccumulatorBytes(cpu);
        OpSta(memory, cpu, OpAbs(cpu, (WRAM_ITEM_RECORD_ID + 1u)));
        if (!SelectionChild(memory, cpu, child, context,
                0x84870du, 0x81f1c5u, 3u, &result))
            return result;
        MeasureSelectionName(memory, cpu);
        OpWriteX(memory, cpu, OpDp(cpu, 0x54u), cpu->x);
        TransferDirectToA(cpu);
        OpLoadA(cpu, 11u);
        cpu->carry = 1u;
        if (cpu->decimal)
            return ExecutionHandoff(cpu, 0x848725u);
        OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0x54u)));
        LsrA8(cpu);
        OpAndValue(cpu, 0xfeu);
        OpRepWidths(cpu, 0x20u);
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x34d8u);
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
    }
    OpPushX(memory, cpu);
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    OpLdx(cpu, 0x3498u);
    OpLdy(cpu, 0x802bu);
    OpLoadA(cpu, 0x85u);
    OpSta(memory, cpu, OpDp(cpu, 0x5fu));
    if (!SelectionChild(memory, cpu, child, context,
            0x848743u, 0x808878u, 3u, &result))
        return result;
    OpPullX(memory, cpu);
    OpLdy(cpu, WRAM_RECORD_BUFFER);
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpDp(cpu, 0x5fu));
    if (!SelectionChild(memory, cpu, child, context,
            0x84874fu, 0x808878u, 3u, &result))
        return result;
    OpLoadA(cpu, 8u);
    OpTestBits(memory, cpu, OpDp(cpu, 0x74u), 1u);
    return ExecutionReturned(0x848757u);
}

enum SelectionPhase {
    SELECTION_CHILD_FAILED,
    SELECTION_CONTINUE,
    SELECTION_REPOSITION,
    SELECTION_CLOSE,
    SELECTION_RETURN
};

static uint8_t PositionSelectedItem(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    Lufia2ExecutionResult *result) {
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_ANGLE_STEP));
    OpSta(memory, cpu, OpAbs(cpu, 0x211bu));
    OpStz(memory, cpu, OpAbs(cpu, 0x211bu));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_SELECTED_INDEX));
    OpSta(memory, cpu, OpAbs(cpu, 0x211cu));
    OpLda(memory, cpu, OpAbs(cpu, 0x2134u));
    cpu->carry = 1u;
    RorA8(cpu);
    OpSta(memory, cpu, OpDp(cpu, 0x64u));
    OpLoadA(cpu, 0x30u);
    OpSta(memory, cpu, OpDp(cpu, 0x58u));
    OpStz(memory, cpu, OpDp(cpu, 0x59u));
    OpStz(memory, cpu, OpDp(cpu, 0x54u));
    if (!SelectionChild(memory, cpu, child, context,
            0x84841bu, 0x83c6aau, 3u, result) ||
        !SelectionChild(memory, cpu, child, context,
            0x84841fu, 0x8486d7u, 2u, result))
        return 0u;
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpAbs(cpu, SELECTION_STATE));
    return 1u;
}

static enum SelectionPhase InitializeSelectionView(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    Lufia2ExecutionResult *result) {
    OpLoadA(cpu, 0xb0u);
    if (!SelectionChild(memory, cpu, child, context,
            0x848376u, 0x84875fu, 2u, result))
        return SELECTION_CHILD_FAILED;
    uint8_t occupied_view = !cpu->zero;
    if (!occupied_view) {
        OpLoadA(cpu, 0x4fu);
        if (!SelectionChild(memory, cpu, child, context,
                0x84837du, 0x848758u, 2u, result))
            return SELECTION_CHILD_FAILED;
        occupied_view = !cpu->zero;
    }
    if (!occupied_view) {
        OpLoadA(cpu, 0xa0u);
        if (!SelectionChild(memory, cpu, child, context,
                0x848384u, 0x848758u, 2u, result))
            return SELECTION_CHILD_FAILED;
        if (!cpu->zero)
            return SELECTION_CLOSE;
        OpLdx(cpu, 0u);
        do {
            OpLda(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E1559));
            if (cpu->zero) {
                OpLda(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_CURSOR_SOURCE + 1u));
                OpOraValue(cpu, OpReadM(memory, cpu,
                    OpAbsX(cpu, WRAM_FIELD_RECOVERY_SPIN_RATES)));
                if (cpu->zero)
                    break;
            }
            OpInx(cpu);
            OpCpx(cpu, 32u);
            if (cpu->zero)
                return SELECTION_RETURN;
        } while (1);
        OpStz(memory, cpu, OpDp(cpu, 0x58u));
        OpStz(memory, cpu, OpDp(cpu, 0x59u));
        OpLoadA(cpu, 0x21u);
        OpSta(memory, cpu, OpDp(cpu, 0x5bu));
    } else {
        OpLoadA(cpu, 0x30u);
        OpSta(memory, cpu, OpDp(cpu, 0x56u));
        OpStz(memory, cpu, OpDp(cpu, 0x5au));
        OpStz(memory, cpu, OpDp(cpu, 0x57u));
        OpStz(memory, cpu, OpDp(cpu, 0x5bu));
        OpLoadA(cpu, 0x1bu);
        OpSta(memory, cpu, OpDp(cpu, 0x58u));
        OpStz(memory, cpu, OpDp(cpu, 0x59u));
        OpLoadA(cpu, 2u);
        OpSta(memory, cpu, OpDp(cpu, 0x54u));
        if (!SelectionChild(memory, cpu, child, context,
                0x8483c3u, 0x83c6aau, 3u, result))
            return SELECTION_CHILD_FAILED;
        OpLoadA(cpu, 0xf0u);
        OpSta(memory, cpu, OpDp(cpu, 0x56u));
    }
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_SELECTION_MODE));
    OpLoadA(cpu, 0x30u);
    OpSta(memory, cpu, OpDp(cpu, 0x55u));
    OpStz(memory, cpu, OpDp(cpu, 0x65u));
    OpLoadA(cpu, 3u);
    OpSta(memory, cpu, OpDp(cpu, 0x59u));
    OpStz(memory, cpu, OpDp(cpu, 0x54u));
    if (!SelectionChild(memory, cpu, child, context,
            0x8483dau, 0x83c6aau, 3u, result))
        return SELECTION_CHILD_FAILED;
    OpLoadA(cpu, 0x18u);
    OpTestBits(memory, cpu, OpDp(cpu, 0x58u), 1u);
    OpStz(memory, cpu, OpDp(cpu, 0x5bu));
    OpStz(memory, cpu, OpDp(cpu, 0x57u));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpDp(cpu, 0x54u));
    if (!SelectionChild(memory, cpu, child, context,
            0x8483eau, 0x83c6aau, 3u, result))
        return SELECTION_CHILD_FAILED;
    OpLoadA(cpu, 0x18u);
    OpSta(memory, cpu, OpDp(cpu, 0x58u));
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpDp(cpu, 0x54u));
    if (!SelectionChild(memory, cpu, child, context,
            0x8483f6u, 0x83c6aau, 3u, result))
        return SELECTION_CHILD_FAILED;
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_SELECTED_INDEX));
    return PositionSelectedItem(memory, cpu, child, context, result) ?
        SELECTION_CONTINUE : SELECTION_CHILD_FAILED;
}

static uint8_t BeginClosingSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    Lufia2ExecutionResult *result) {
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_RECOVERY_PALETTE_LENGTH));
    OpLoadA(cpu, 0x40u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_PALETTE_SCALE_PHASE));
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_PALETTE_SCALE_TARGET));
    OpLda(memory, cpu, 0x0009a9u);
    OpOraValue(cpu, 4u);
    OpSta(memory, cpu, 0x0009a9u);
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E1559));
        uint8_t close_slot = cpu->zero;
        if (!close_slot) {
            OpCmpValue(cpu, 2u);
            close_slot = cpu->zero;
            if (!close_slot) {
                OpCmpValue(cpu, 1u);
                close_slot = cpu->zero;
            }
        }
        if (close_slot) {
            OpLda(memory, cpu, OpAbsX(cpu, WRAM_FIELD_RECOVERY_OBJECT_UPDATES));
            OpOraValue(cpu, 4u);
            OpSta(memory, cpu, OpAbsX(cpu, WRAM_FIELD_RECOVERY_OBJECT_UPDATES));
            OpLoadA(cpu, 0x31u);
            OpSta(memory, cpu, OpAbsX(cpu, WRAM_FIELD_RECOVERY_SPIN_RATES));
            OpLda(memory, cpu, OpAbsX(cpu, WRAM_MENU_ITEM_COLUMN));
            OpAndValue(cpu, 0xfeu);
            OpSta(memory, cpu, OpAbsX(cpu, WRAM_MENU_ITEM_COLUMN));
            OpLoadA(cpu, 0xa0u);
            OpSta(memory, cpu, OpAbsX(cpu, WRAM_FIELD_ITEM_SECOND_VELOCITY));
            OpLoadA(cpu, 0x21u);
            OpSta(memory, cpu, OpAbsX(cpu, WRAM_FIELD_RECOVERY_SECOND_SPIN_RATES));
            OpStz(memory, cpu, OpAbsX(cpu, 0x16d9u));
            OpLda(memory, cpu, OpAbsX(cpu, 0x1579u));
            OpAndValue(cpu, 0xfeu);
            OpSta(memory, cpu, OpAbsX(cpu, 0x1579u));
            OpLoadA(cpu, 0x31u);
            OpSta(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_CURSOR_SOURCE + 1u));
            OpLoadA(cpu, 0xc0u);
            OpSta(memory, cpu, OpAbsX(cpu, WRAM_FIELD_RECOVERY_ZOOM_LIMITS));
        }
        OpInx(cpu);
        OpCpx(cpu, 32u);
    } while (!cpu->zero);
    OpLda(memory, cpu, (WRAM_FIELD_LAYER_SECTION_WORD + 4u));
    OpCmpValue(cpu, 0xffu);
    if (cpu->zero) {
        if (!SelectionChild(memory, cpu, child, context,
                0x8485dbu, 0x848328u, 3u, result))
            return 0u;
        OpLoadA(cpu, 8u);
        OpTestBits(memory, cpu, OpDp(cpu, 0x74u), 1u);
    }
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpAbs(cpu, SELECTION_STATE));
    return 1u;
}

static Lufia2ExecutionResult FinishClosingSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    OpBitValue(cpu, 8u);
    if (cpu->zero)
        return ExecutionReturned(0x84863du);
    OpLdx(cpu, 0u);
    uint8_t all_occupied = 0u;
    do {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E1559));
        if (cpu->zero)
            break;
        OpInx(cpu);
        OpCpx(cpu, 32u);
        all_occupied = cpu->zero;
    } while (!all_occupied);
    Lufia2ExecutionResult result;
    if (!all_occupied) {
        OpLoadA(cpu, 0xffu);
        if (!SelectionChild(memory, cpu, child, context,
                0x8485feu, 0x84875fu, 2u, &result))
            return result;
        if (cpu->zero) {
            OpLoadA(cpu, 0xffu);
            if (!SelectionChild(memory, cpu, child, context,
                    0x848605u, 0x848758u, 2u, &result))
                return result;
            if (cpu->zero)
                return ExecutionReturned(0x84863du);
        }
        OpLdx(cpu, 0u);
        for (unsigned slots = 0u; slots < SELECTION_TRANSITION_LIMIT; ++slots) {
            OpLda(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E1559));
            uint8_t remove = cpu->zero;
            if (!remove) {
                OpCmpValue(cpu, 2u);
                remove = cpu->zero;
                if (!remove) {
                    OpCmpValue(cpu, 1u);
                    remove = cpu->zero;
                }
            }
            if (remove) {
                OpWriteX(memory, cpu, OpDp(cpu, 0xa7u), cpu->x);
                if (!SelectionChild(memory, cpu, child, context,
                        0x84861eu, 0x83ab4fu, 3u, &result) ||
                    !SelectionChild(memory, cpu, child, context,
                        0x848622u, 0x83f205u, 3u, &result))
                    return result;
                OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 0xa7u)));
            }
            OpInx(cpu);
            OpCpx(cpu, 32u);
            if (cpu->zero)
                break;
            if (slots + 1u == SELECTION_TRANSITION_LIMIT)
                return ExecutionHandoff(cpu, 0x84860fu);
        }
    }
    OpLoadA(cpu, 8u);
    OpTestBits(memory, cpu, OpAbs(cpu, SELECTION_STATE), 0u);
    OpLoadA(cpu, 1u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_FIELD_REQUESTS), 0u);
    OpLoadA(cpu, 1u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09A9), 1u);
    return ExecutionReturned(0x84863du);
}

static enum SelectionPhase RepositionSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    Lufia2ExecutionResult *result) {
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_SELECTED_INDEX));
    OpLoadA(cpu, 0u);
    if (!SelectionChild(memory, cpu, child, context,
            0x8484d1u, 0x848766u, 3u, result))
        return SELECTION_CHILD_FAILED;
    OpStz(memory, cpu, OpDp(cpu, 0x58u));
    OpStz(memory, cpu, OpDp(cpu, 0x59u));
    return PositionSelectedItem(memory, cpu, child, context, result) ?
        SELECTION_REPOSITION : SELECTION_CHILD_FAILED;
}

static enum SelectionPhase ApplySelectionChoice(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    Lufia2ExecutionResult *result) {
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_SELECTION_MODE));
    if (cpu->zero) {
        OpLoadA(cpu, 2u);
        if (!SelectionChild(memory, cpu, child, context,
                0x84852du, 0x848766u, 3u, result) ||
            !SelectionChild(memory, cpu, child, context,
                0x848531u, 0x8486c0u, 2u, result))
            return SELECTION_CHILD_FAILED;
        OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_SELECTED_ITEM));
        ExchangeAccumulatorBytes(cpu);
        OpSta(memory, cpu, OpAbs(cpu, (WRAM_FIELD_SELECTED_ITEM + 1u)));
        OpLoadA(cpu, 0x3cu);
    } else {
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            OpLoadA(cpu, 0x80u);
            OpTestBits(memory, cpu, OpAbs(cpu, WRAM_FIELD_FLAGS), 1u);
            OpLoadA(cpu, 8u);
            OpTestBits(memory, cpu, OpAbs(cpu, WRAM_FIELD_RELOAD_FLAGS), 1u);
            OpLoadA(cpu, 1u);
            OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_HOLD_FRAMES));
        } else {
            OpLda(memory, cpu, OpAbs(cpu, WRAM_WINDOW_MODE));
            OpBitValue(cpu, 0x80u);
            if (!cpu->zero) {
                OpLoadA(cpu, 3u);
                if (!SelectionChild(memory, cpu, child, context,
                        0x8484eeu, 0x848766u, 3u, result))
                    return SELECTION_CHILD_FAILED;
                static const struct { uint8_t level; uint32_t site; } mosaic[] = {
                    {0x1fu, 0x8484f4u},
                    {0x2fu, 0x8484f9u},
                    {0x1fu, 0x8484feu}
                };
                for (unsigned i = 0u; i < sizeof mosaic / sizeof *mosaic; ++i) {
                    OpLoadA(cpu, mosaic[i].level);
                    if (!SelectionChild(memory, cpu, child, context,
                            mosaic[i].site, 0x848685u, 2u, result))
                        return SELECTION_CHILD_FAILED;
                }
                TransferDirectToA(cpu);
                if (!SelectionChild(memory, cpu, child, context,
                        0x848502u, 0x848685u, 2u, result))
                    return SELECTION_CHILD_FAILED;
            } else {
                OpLoadA(cpu, 2u);
                if (!SelectionChild(memory, cpu, child, context,
                        0x84850au, 0x848766u, 3u, result))
                    return SELECTION_CHILD_FAILED;
                OpLoadA(cpu, 0x41u);
                if (!SelectionChild(memory, cpu, child, context,
                        0x848510u, 0x84868cu, 2u, result))
                    return SELECTION_CHILD_FAILED;
                OpLoadA(cpu, 2u);
                OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_SELECTION_MODE));
            }
            return SELECTION_CONTINUE;
        }
    }
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_HOLD_FRAMES));
    OpLoadA(cpu, 0x11u);
    if (!SelectionChild(memory, cpu, child, context,
            0x848542u, 0x84868cu, 2u, result))
        return SELECTION_CHILD_FAILED;
    OpLoadA(cpu, 4u);
    OpSta(memory, cpu, OpAbs(cpu, SELECTION_STATE));
    return SELECTION_CONTINUE;
}

static enum SelectionPhase HandleSelectionInput(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    Lufia2ExecutionResult *result) {
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_SELECTION_MODE));
    if (cpu->zero) {
        OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_IDLE_SLOT)));
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_FIELD_RECOVERY_SECOND_SPIN_RATES));
        if (!cpu->zero)
            return SELECTION_RETURN;
        OpLoadA(cpu, 2u);
        if (!SelectionChild(memory, cpu, child, context,
                0x848440u, 0x848758u, 2u, result))
            return SELECTION_CHILD_FAILED;
        uint8_t previous = !cpu->zero;
        if (!previous) {
            OpLoadA(cpu, 0x20u);
            if (!SelectionChild(memory, cpu, child, context,
                    0x848447u, 0x84875fu, 2u, result))
                return SELECTION_CHILD_FAILED;
            previous = !cpu->zero;
        }
        if (previous) {
            OpLoadA(cpu, 0xc1u);
            OpSta(memory, cpu, OpDp(cpu, 0x5bu));
            OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_SELECTED_INDEX));
            OpDecA(cpu);
            if (cpu->negative) {
                OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_COUNT));
                OpDecA(cpu);
            }
            return RepositionSelection(memory, cpu, child, context, result);
        }
        OpLoadA(cpu, 1u);
        if (!SelectionChild(memory, cpu, child, context,
                0x84845eu, 0x848758u, 2u, result))
            return SELECTION_CHILD_FAILED;
        uint8_t next = !cpu->zero;
        if (!next) {
            OpLoadA(cpu, 0x10u);
            if (!SelectionChild(memory, cpu, child, context,
                    0x848465u, 0x84875fu, 2u, result))
                return SELECTION_CHILD_FAILED;
            next = !cpu->zero;
        }
        if (next) {
            OpLoadA(cpu, 0x41u);
            OpSta(memory, cpu, OpDp(cpu, 0x5bu));
            OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_SELECTED_INDEX));
            OpIncA(cpu);
            OpCmp(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_COUNT));
            if (cpu->zero)
                TransferDirectToA(cpu);
            return RepositionSelection(memory, cpu, child, context, result);
        }
    }
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_MARKER_SLOT)));
    if (!cpu->negative) {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_FIELD_RECOVERY_SPIN_RATES));
        if (cpu->zero) {
            OpLoadA(cpu, 4u);
            if (!SelectionChild(memory, cpu, child, context,
                    0x848486u, 0x848758u, 2u, result))
                return SELECTION_CHILD_FAILED;
            uint8_t rotate = !cpu->zero;
            if (rotate) {
                OpLoadA(cpu, 0xc1u);
            } else {
                OpLoadA(cpu, 8u);
                if (!SelectionChild(memory, cpu, child, context,
                        0x848491u, 0x848758u, 2u, result))
                    return SELECTION_CHILD_FAILED;
                rotate = !cpu->zero;
                if (rotate)
                    OpLoadA(cpu, 0x41u);
            }
            if (rotate) {
                if (!SelectionChild(memory, cpu, child, context,
                        0x848498u, 0x84863eu, 2u, result))
                    return SELECTION_CHILD_FAILED;
                return SELECTION_CONTINUE;
            }
        }
    }
    OpLoadA(cpu, 0x80u);
    if (!SelectionChild(memory, cpu, child, context,
            0x8484a0u, 0x848758u, 2u, result))
        return SELECTION_CHILD_FAILED;
    if (!cpu->zero) {
        OpLoadA(cpu, 1u);
        if (!SelectionChild(memory, cpu, child, context,
                0x8484b8u, 0x848766u, 3u, result))
            return SELECTION_CHILD_FAILED;
        OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_SELECTION_MODE));
        OpCmpValue(cpu, 2u);
        if (!cpu->zero)
            return SELECTION_CLOSE;
        if (!SelectionChild(memory, cpu, child, context,
                0x8484c3u, 0x84869eu, 2u, result))
            return SELECTION_CHILD_FAILED;
        return SELECTION_CONTINUE;
    }
    OpLoadA(cpu, 0x80u);
    if (!SelectionChild(memory, cpu, child, context,
            0x8484a7u, 0x84875fu, 2u, result))
        return SELECTION_CHILD_FAILED;
    if (cpu->zero) {
        OpLoadA(cpu, 0x60u);
        if (!SelectionChild(memory, cpu, child, context,
                0x8484aeu, 0x848758u, 2u, result))
            return SELECTION_CHILD_FAILED;
        if (cpu->zero)
            return SELECTION_CONTINUE;
    }
    return ApplySelectionChoice(memory, cpu, child, context, result);
}

Lufia2ExecutionResult Lufia2FieldUpdateItemSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!SelectionContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x84836au);
    OpLda(memory, cpu, OpAbs(cpu, SELECTION_STATE));
    OpBitValue(cpu, 1u);
    Lufia2ExecutionResult result;
    if (!cpu->zero) {
        const enum SelectionPhase phase = InitializeSelectionView(
            memory, cpu, child, context, &result);
        if (phase == SELECTION_CHILD_FAILED)
            return result;
        if (phase == SELECTION_RETURN)
            return ExecutionReturned(0x84863du);
        if (phase == SELECTION_CLOSE) {
            if (!BeginClosingSelection(memory, cpu, child, context, &result))
                return result;
            return FinishClosingSelection(memory, cpu, child, context);
        }
    }
    for (unsigned transitions = 0u;
            transitions < SELECTION_TRANSITION_LIMIT; ++transitions) {
        OpBitValue(cpu, 2u);
        if (!cpu->zero) {
            const enum SelectionPhase phase = HandleSelectionInput(
                memory, cpu, child, context, &result);
            if (phase == SELECTION_CHILD_FAILED)
                return result;
            if (phase == SELECTION_RETURN)
                return ExecutionReturned(0x84863du);
            if (phase == SELECTION_CLOSE) {
                if (!BeginClosingSelection(memory, cpu, child, context, &result))
                    return result;
                return FinishClosingSelection(memory, cpu, child, context);
            }
            if (phase == SELECTION_REPOSITION) {
                if (transitions + 1u == SELECTION_TRANSITION_LIMIT)
                    return ExecutionHandoff(cpu, 0x848427u);
                continue;
            }
            OpLda(memory, cpu, OpAbs(cpu, SELECTION_STATE));
        }
        OpBitValue(cpu, 4u);
        if (!cpu->zero) {
            OpLoadA(cpu, 0xffu);
            if (!SelectionChild(memory, cpu, child, context,
                    0x848556u, 0x848758u, 2u, &result))
                return result;
            uint8_t close = !cpu->zero;
            if (!close) {
                OpLoadA(cpu, 0xffu);
                if (!SelectionChild(memory, cpu, child, context,
                        0x84855du, 0x84875fu, 2u, &result))
                    return result;
                close = !cpu->zero;
            }
            if (!close) {
                OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_HOLD_FRAMES));
                OpDecA(cpu);
                OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_ITEM_HOLD_FRAMES));
                close = cpu->zero;
                if (!close)
                    OpLda(memory, cpu, OpAbs(cpu, SELECTION_STATE));
            }
            if (close && !BeginClosingSelection(
                    memory, cpu, child, context, &result))
                return result;
        }
        return FinishClosingSelection(memory, cpu, child, context);
    }
    return ExecutionHandoff(cpu, 0x848427u);
}
