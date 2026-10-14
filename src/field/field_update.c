/* Field loop steps and event tick. */

#include "core/cpu_internal.h"
#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "core/joypad.h"
#include "field/field_internal.h"
#include "lufia2/field.h"
#include "system/wram.h"
#include "text/text_internal.h"

enum {
    DP_MENU_HDMA_CONTROL = 0x81u,
    DP_TEXT_UPLOAD_REQUEST = 0x74u,
    MENU_HELD_BUTTONS = ((JOY_HIGH_B | JOY_HIGH_START) << 8) | JOY_LOW_A,
    TEXT_WINDOW_FLAGS = 0x099cu
};

static uint8_t FieldRequestChild(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                 Lufia2PushedChildCall child, void *context,
                                 uint32_t site, uint32_t target, uint8_t frame_size,
                                 uint8_t index_width, Lufia2ExecutionResult *result) {
    const uint8_t bank = (uint8_t)(site >> 16);
    const uint8_t data_bank = cpu->data_bank;
    if (!CallChildWithFrame(memory, cpu, child, context, site, target, frame_size,
                            bank)) {
        *result = (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return 0u;
    }
    if (cpu->program_bank != bank || cpu->data_bank != data_bank ||
        !cpu->accumulator_is_8_bit || cpu->index_is_8_bit != index_width ||
        cpu->direct_page || cpu->decimal) {
        *result = ExecutionHandoff(cpu, site + frame_size + 1u);
        return 0u;
    }
    return 1u;
}

static uint8_t FieldMenuRequested(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLoadA(cpu, 0x40u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_FIELD_FLAGS), 0u);
    if (!cpu->zero)
        return 1u;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_WINDOW_MODE));
    OpBitValue(cpu, 2u);
    if (cpu->zero)
        return 0u;
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, MENU_HELD_BUTTONS);
    OpAndValue(cpu, Read16Direct(memory, cpu, DP_BUTTONS_HELD));
    if (cpu->zero)
        return 0u;
    OpTestBits(memory, cpu, OpDp(cpu, DP_BUTTONS_PRESSED), 0u);
    return !cpu->zero;
}

Lufia2ExecutionResult Lufia2FieldMenuRequestWithServices(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu,
                                                         Lufia2PushedChildCall child,
                                                         void *context) {
    if (cpu->program_bank != 0x83u || !cpu->accumulator_is_8_bit ||
        !cpu->index_is_8_bit || cpu->direct_page || cpu->decimal || !child)
        return ExecutionHandoff(cpu, 0x8383a0u);
    if (!FieldMenuRequested(memory, cpu)) {
        OpSepWidths(cpu, 0x20u);
        return ExecutionReturned(0x8383dfu);
    }
    OpSepWidths(cpu, 0x20u);
    Lufia2ExecutionResult result = {0};
    if (!FieldRequestChild(memory, cpu, child, context, 0x8383bdu, 0x83afeau, 3u, 1u,
                           &result))
        return result;
    OpStz(memory, cpu, OpDp(cpu, DP_MENU_HDMA_CONTROL));
    OpStz(memory, cpu, OpAbs(cpu, SNES_HDMAEN));
    if (!FieldRequestChild(memory, cpu, child, context, 0x8383c6u, 0x83900cu, 2u, 1u,
                           &result))
        return result;
    OpLoadA(cpu, 0x1fu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_TM));
    if (!FieldRequestChild(memory, cpu, child, context, 0x8383ceu, 0x809528u, 3u, 1u,
                           &result))
        return result;
    OpRepWidths(cpu, 0x30u);
    OpLoadA(cpu, 0x1fffu);
    cpu->stack = cpu->accumulator;
    OpSepWidths(cpu, 0x20u);
    return ExecutionHandoff(cpu, 0x83acefu);
}

static Lufia2ExecutionResult TickTextWindowCountdown(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu,
                                                     Lufia2PushedChildCall child,
                                                     void *context) {
    OpLda(memory, cpu, WRAM_FIELD_CONTROL_CHANGE_PENDING);
    OpCmpValue(cpu, 0xffu);
    if (!cpu->zero) {
        OpDecA(cpu);
        OpSta(memory, cpu, WRAM_FIELD_CONTROL_CHANGE_PENDING);
        if (cpu->zero) {
            OpLoadA(cpu, 0xffu);
            OpSta(memory, cpu, WRAM_FIELD_CONTROL_CHANGE_PENDING);
            OpLoadA(cpu, 0xa0u);
            OpTestBits(memory, cpu, OpAbs(cpu, TEXT_WINDOW_FLAGS), 0u);
            const uint8_t index_width = cpu->index_is_8_bit;
            Lufia2ExecutionResult result = {0};
            if (child) {
                if (!FieldRequestChild(memory, cpu, child, context, 0x80c234u,
                                       0x848328u, 3u, index_width, &result))
                    return result;
            } else {
                Lufia2TextWindowClear(memory, cpu, 0xc237u);
            }
            OpLoadA(cpu, 8u);
            OpSta(memory, cpu, OpDp(cpu, DP_TEXT_UPLOAD_REQUEST));
        }
    }
    return ExecutionReturned(0x80c23cu);
}

Lufia2ExecutionResult Lufia2FieldTextWindowTimer(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu,
                                                 Lufia2PushedChildCall child,
                                                 void *context) {
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit || cpu->direct_page ||
        cpu->decimal || !child)
        return ExecutionHandoff(cpu, 0x80c21au);
    return TickTextWindowCountdown(memory, cpu, child, context);
}

static Lufia2ExecutionResult FinishFieldTextTick(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result = ExecutionReturned(0x809cb7u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_TEXT_STATE));
    OpAndValue(cpu, 0x0au);
    if (!cpu->zero)
        return Lufia2TextPromptTick(memory, cpu, result);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_TEXT_STATE));
    return cpu->negative ? Lufia2TextEngineStepBody(memory, cpu, result) : result;
}

Lufia2ExecutionResult Lufia2FieldEventTickWithServices(const Lufia2Memory *memory,
                                                       Lufia2CpuState *cpu,
                                                       Lufia2PushedChildCall child,
                                                       void *context) {
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit || cpu->direct_page ||
        cpu->decimal || !child)
        return ExecutionHandoff(cpu, 0x809c72u);
    const uint8_t index_width = cpu->index_is_8_bit;
    Lufia2ExecutionResult result = {0};
    if (!FieldRequestChild(memory, cpu, child, context, 0x809c72u, 0x848000u, 3u,
                           index_width, &result) ||
        !FieldRequestChild(memory, cpu, child, context, 0x809c76u, 0x80c21au, 2u,
                           index_width, &result))
        return result;
    return FinishFieldTextTick(memory, cpu);
}

/* $83:80CD: field idle test; zero = no event running. */
void Lufia2FieldIdleBody(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const uint16_t gates[5] = {0x09a8u, WRAM_ACTOR_STATE, WRAM_FIELD_REQUESTS, WRAM_FIELD_FLAGS,
                                      WRAM_ANIMATION_MASK};
    static const uint8_t masks[5] = {0x08u, 0x88u, 0x07u, 0xa2u, 0x00u};
    unsigned i;

    for (i = 0; i < 5u; ++i) {
        LoadAAbsolute8(memory, cpu, gates[i], 0);              /* 80CD */
        if (masks[i])
            BitImmediate8(cpu, masks[i]);
        if (!cpu->zero)
            return;
    }
    LoadAAbsolute8(memory, cpu, WRAM_TEXT_STATE, 0);           /* 80EE */
    BitImmediate8(cpu, 0x80u);
    if (!cpu->zero)
        return;
    LoadX8(cpu, 0x00u);                                        /* 80F5 */
    do {
        LoadA8(cpu,
               Read8(memory, LongIndexedAddress(EVENT_ANIMATION_SLOT_STATE, cpu->x)));
        if (cpu->negative)
            return;
        LoadX8(cpu, (uint8_t)(cpu->x + 1u));                   /* 80FD */
        Compare8(cpu, (uint8_t)cpu->x, 0x08u);
    } while (!cpu->zero);
}

/* $83:80CD from the field loop's JSR (X8 only). */
Lufia2ExecutionResult Lufia2FieldIdleTest(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;

    result.flow = LUFIA2_EXECUTION_RETURNED;
    result.pc = 0x838102u;
    result.dispatches = 0;
    if (!cpu->index_is_8_bit) {
        result.flow = LUFIA2_EXECUTION_BOUNDARY;
        result.pc = cpu->resume_pc = 0x8380cdu;
        return result;
    }
    Lufia2FieldIdleBody(memory, cpu);
    return result;
}

/* $83:8682: tick the eight animation slots at $7F:D057. */
Lufia2ExecutionResult Lufia2FieldAnimationTicks(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const uint32_t slots = EVENT_ANIMATION_SLOT_STATE;
    Lufia2ExecutionResult result;

    result.flow = LUFIA2_EXECUTION_RETURNED;
    result.pc = 0x8386e9u;
    result.dispatches = 0;
    Push8(memory, cpu, PackStatus(cpu));                       /* 8682 */
    SetIndexWidth(cpu, 0);
    Write8(memory, AbsoluteIndexedAddress(cpu, WRAM_ANIMATION_MASK, 0), 0x00u);
    LoadA8(cpu, 0x01u);
    StoreAAbsolute8(memory, cpu, 0x17abu, 0);
    LoadX16(cpu, 0x0000u);
    do {
        LoadA8(cpu, Read8(memory, LongIndexedAddress(slots, cpu->x)));
        if (cpu->negative) {
            ExchangeAccumulatorBytes(cpu);                     /* 8696 */
            LoadAAbsolute8(memory, cpu, 0x17abu, 0);
            TestBitsAbsolute8(memory, cpu, WRAM_ANIMATION_MASK, 1);
            AslA8(cpu);
            StoreAAbsolute8(memory, cpu, 0x17abu, 0);
            ExchangeAccumulatorBytes(cpu);
            cpu->carry = 0;
            Adc8(cpu, 0x08u);
            Write8(memory, LongIndexedAddress(slots, cpu->x), A8(cpu));
            And8(cpu, 0x18u);
            Compare8(cpu, A8(cpu), 0x18u);
            if (cpu->zero) {
                LoadA8(cpu, Read8(memory, LongIndexedAddress(slots, cpu->x)));
                And8(cpu, 0xe7u);                              /* 86B3 */
                Write8(memory, LongIndexedAddress(slots, cpu->x), A8(cpu));
                PushIndex(memory, cpu);
                Write8(memory, 0x7fd049u, A8(cpu));
                LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
                BitImmediate8(cpu, 0x02u);
                if (!cpu->zero)
                    TransferDirectToA(cpu);                    /* 86C3 */
                Write8(memory, LongIndexedAddress(slots, cpu->x), A8(cpu));
                LoadA8(cpu, Read8(memory, LongIndexedAddress(
                                              EVENT_ANIMATION_SLOT_OBJECT, cpu->x)));
                Write8(memory, EVENT_OBJECT_OPERAND, A8(cpu));
                LoadA8(cpu, Read8(memory, EVENT_OBJECT_OPERAND));
                /* The slot handlers $83:8783/87CC stay LLE. */
                result.flow = LUFIA2_EXECUTION_BOUNDARY;
                result.pc = cpu->resume_pc =
                    cpu->negative ? 0x8386d6u : 0x8386dbu;
                return result;
            }
        }
        IncrementX16(cpu);                                     /* 86DF */
        Compare16(cpu, cpu->x, 0x0008u);
    } while (!cpu->zero);
    UnpackStatus(cpu, Pull8(memory, cpu));                     /* 86E8 */
    return result;
}

Lufia2ExecutionResult Lufia2FieldEventTick(const Lufia2Memory *memory,
                                           Lufia2CpuState *cpu) {
    SimulateJslFrame(memory, cpu, 0x80u, 0x9c75u);
    cpu->program_bank = 0x84u;
    const Lufia2ExecutionResult screen = Lufia2FieldScreenEffects(memory, cpu);
    if (screen.flow != LUFIA2_EXECUTION_RETURNED)
        return ExecutionHandoff(cpu, cpu->resume_pc);
    uint16_t screen_return = Pull8(memory, cpu);
    screen_return |= (uint16_t)Pull8(memory, cpu) << 8;
    cpu->program_bank = Pull8(memory, cpu);
    const uint32_t screen_landing =
        ((uint32_t)cpu->program_bank << 16) | (uint16_t)(screen_return + 1u);
    if (screen_landing != 0x809c76u)
        return ExecutionHandoff(cpu, screen_landing);
    SimulateJsrFrame(memory, cpu, 0x9c78u);
    (void)TickTextWindowCountdown(memory, cpu, NULL, NULL);
    SimulateRtsFrame(memory, cpu);
    return FinishFieldTextTick(memory, cpu);
}


/* $83:83A0: menu request; the menu itself runs on LLE. */
Lufia2ExecutionResult Lufia2FieldMenuRequest(const Lufia2Memory *memory,
                                             Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x8383a0u);
    const uint8_t requested = FieldMenuRequested(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    return requested ? ExecutionHandoff(cpu, 0x8383bdu) : ExecutionReturned(0x8383dfu);
}

/* $83:867B: A & pressed buttons; consume them from $4A. */
Lufia2ExecutionResult Lufia2FieldTakeButtons(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x83867bu);
    (void)TakeButtonPress8(memory, cpu, JOYPAD_LOW_BYTE); /* 867B */
    return ExecutionReturned(0x838681u);
}

/* $83:8103: $05B7 requests; any set request runs on LLE. */
Lufia2ExecutionResult Lufia2FieldStatusRequests(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x838103u);
    LoadAAbsolute8(memory, cpu, 0x09a8u, 0);                   /* 8103 */
    BitImmediate8(cpu, 0x08u);
    if (cpu->zero) {
        SetIndexWidth(cpu, 0);
        LoadAAbsolute8(memory, cpu, WRAM_FIELD_REQUESTS, 0);   /* 810C */
        BitImmediate8(cpu, 0x02u);
        if (!cpu->zero)
            return ExecutionHandoff(cpu, 0x838113u);
        BitImmediate8(cpu, 0x04u);                             /* 8119 */
        if (!cpu->zero)
            return ExecutionHandoff(cpu, 0x83811du);
        BitImmediate8(cpu, 0x01u);                             /* 8123 */
        if (!cpu->zero)
            return ExecutionHandoff(cpu, 0x838127u);
    }
    SetIndexWidth(cpu, 1);                                     /* 812B */
    return ExecutionReturned(0x83812du);
}
