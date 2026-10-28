/* Battle NMI uploads ($85:8DC5). */

#include <stddef.h>

#include "core/cpu_ops.h"
#include "core/child_call.h"
#include "lufia2/battle.h"
#include "lufia2/battle_nmi_timer_steps.h"
#include "lufia2/battle_timed_callbacks.h"
#include "primary_wave_internal.h"
#include "system/wram.h"

/* $85:8E98: sixteen queued VRAM DMA uploads on channel 6. */
static void BattleVramQueueBody(const Lufia2Memory *memory, Lufia2CpuState *cpu) {

    LoadX16(cpu, 0x005au);                                     /* 8E98 */
    do {
        LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1a8fu, cpu->x));
        if (!cpu->zero) {
            Write16Absolute(memory, cpu, SNES_DASL(6), cpu->y);
            LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1a91u, cpu->x));
            Write16Absolute(memory, cpu, SNES_A1TL(6), cpu->y);
            LoadA8(cpu, 0x01u);
            StoreAAbsolute8(memory, cpu, SNES_DMAP(6), 0);
            LoadA8(cpu, 0x7eu);
            StoreAAbsolute8(memory, cpu, SNES_A1B(6), 0);
            LoadA8(cpu, 0x18u);
            StoreAAbsolute8(memory, cpu, SNES_BBAD(6), 0);
            LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1a93u, cpu->x));
            Write16Absolute(memory, cpu, SNES_VMADDL, cpu->y);
            LoadA8(cpu, 0x40u);
            StoreAAbsolute8(memory, cpu, SNES_MDMAEN, 0);
            Write8(memory, AbsoluteIndexedAddress(cpu, 0x1a8fu, cpu->x), 0x00u);
            Write8(memory, AbsoluteIndexedAddress(cpu, 0x1a90u, cpu->x), 0x00u);
        }
        LoadX16(cpu, (uint16_t)(cpu->x - 6u));                 /* 8EC9 */
    } while (!cpu->negative);
}

static void BattleVramQueue(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0x8dccu);
    BattleVramQueueBody(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $85:8ED2: latch HDMA channels from $1AEF, enable $420C. */
static void BattleHdmaChannels(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0x8e24u);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0xdau)));    /* 8ED2 */
    Write8(memory, DirectAddress(cpu, 0xdau), 0x00u);
    {
        const uint32_t d9 = DirectAddress(cpu, 0xd9u);
        const uint8_t value = Read8(memory, d9);

        cpu->zero = (value & A8(cpu)) == 0;                    /* TSB $D9 */
        Write8(memory, d9, (uint8_t)(value | A8(cpu)));
    }
    Or8(cpu, Read8(memory, DirectAddress(cpu, 0xd8u)));
    Or8(cpu, 0x40u);
    And8(cpu, Read8(memory, DirectAddress(cpu, 0xd9u)));
    Write8(memory, DirectAddress(cpu, 0xd8u), A8(cpu));
    if (!cpu->zero) {
        LoadY16(cpu, SNES_DMAP(0));
        LoadX16(cpu, 0x1aefu);
        for (;;) {
            const uint32_t d8 = DirectAddress(cpu, 0xd8u);     /* 8EE8 */
            const uint8_t mask = Read8(memory, d8);

            cpu->carry = mask & 1u;
            Write8(memory, d8, (uint8_t)(mask >> 1));
            SetNz8(cpu, (uint8_t)(mask >> 1));
            if (cpu->carry) {
                LoadAAbsolute8(memory, cpu, 0x0000u, cpu->x);  /* 8EF2 */
                StoreAAbsolute8(memory, cpu, 0x0000u, cpu->y);
                SetAccumulatorWidth(cpu, 0);
                LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0001u, cpu->x));
                Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x0001u, cpu->y),
                    cpu->accumulator);
                LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0003u, cpu->x));
                Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x0003u, cpu->y),
                    cpu->accumulator);
            } else if (cpu->zero) {
                break;
            } else {
                SetAccumulatorWidth(cpu, 0);
            }
            LoadA16(cpu, cpu->x);                              /* 8F06 */
            cpu->carry = 0;
            Add16Value(cpu, 0x0005u);
            TransferAToX(cpu);
            LoadA16(cpu, cpu->y);
            Add16Value(cpu, 0x0010u);
            TransferAToY(cpu);
            SetAccumulatorWidth(cpu, 1);
        }
    }
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0xd9u)));    /* 8F15 */
    StoreAAbsolute8(memory, cpu, SNES_HDMAEN, 0);
    SimulateRtsFrame(memory, cpu);
}

/* Eight timers preserve their original child return frames. */
enum {
    BATTLE_TIMER_ACTIVE = WRAM_UNK_7E1B17,
    BATTLE_TIMER_COUNTDOWN = WRAM_BATTLE_RIPPLE_ROW_DELAY,
    BATTLE_TIMER_HANDLER = BATTLE_TIMER_COUNTDOWN + 1u,
    ROM_BATTLE_TIMER_HANDLERS = 0x9e05u,
    BATTLE_TIMER_STRIDE = 0x08u,
    BATTLE_TIMER_END = 0x40u
};

static bool BattleTimerExpired(const Lufia2Memory *memory,
                               Lufia2CpuState *cpu) {
    LoadAAbsolute8(memory, cpu, BATTLE_TIMER_ACTIVE, cpu->x);
    if (cpu->zero)
        return false;
    const uint32_t counter = AbsoluteIndexedAddress(cpu, BATTLE_TIMER_COUNTDOWN, cpu->x);
    const uint8_t left = (uint8_t)(Read8(memory, counter) - 1u);
    Write8(memory, counter, left);
    SetNz8(cpu, left);
    return cpu->zero;
}

static void BattleAdvanceTimer(Lufia2CpuState *cpu) {
    LoadA8(cpu, (uint8_t)cpu->x);
    cpu->carry = 0u;
    OpAdcValue(cpu, BATTLE_TIMER_STRIDE);
    TransferAToX(cpu);
    Compare8(cpu, (uint8_t)cpu->x, BATTLE_TIMER_END);
}

Lufia2ExecutionResult Lufia2BattleTimerStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry) {
    if ((entry != 0x858f1fu && entry != 0x858f3eu) ||
        cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit != 1u ||
        cpu->index_is_8_bit != 1u || cpu->x > 0xffu || cpu->y > 0xffu)
        return ExecutionHandoff(cpu, entry);
    if (entry == 0x858f1fu && BattleTimerExpired(memory, cpu))
        return ExecutionHandoff(cpu, 0x858f29u);
    BattleAdvanceTimer(cpu);
    return ExecutionHandoff(cpu, cpu->zero ? 0x858f47u : 0x858f1fu);
}

static Lufia2ExecutionResult BattleRunTimerSlots(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, bool resuming) {
    SetIndexWidth(cpu, 1u);
    if (resuming) {
        BattleAdvanceTimer(cpu);
        if (cpu->zero) {
            SetIndexWidth(cpu, 0u);
            return ExecutionReturned(0x858f49u);
        }
    } else {
        LoadX8(cpu, 0u);
    }
    unsigned visited = 0u;
    do {
        if (BattleTimerExpired(memory, cpu)) {
            LoadAAbsolute8(memory, cpu, BATTLE_TIMER_HANDLER, cpu->x);
            AslA8(cpu);
            TransferAToY(cpu);
            SetAccumulatorWidth(cpu, 0u);
            LoadA16(cpu, Read16AbsoluteIndexed(
                memory, cpu, ROM_BATTLE_TIMER_HANDLERS, cpu->y));
            Push8(memory, cpu, 0x8fu);
            Push8(memory, cpu, 0x3bu);
            PushAccumulator16(memory, cpu);
            SetAccumulatorWidth(cpu, 1u);
            SetIndexWidth(cpu, 0u);
            const uint8_t low = Pull8(memory, cpu);
            const uint16_t destination = (uint16_t)(low |
                ((uint16_t)Pull8(memory, cpu) << 8));
            const uint32_t target = ((uint32_t)cpu->program_bank << 16) |
                (uint16_t)(destination + 1u);
            if (child) {
                if (!child(context, cpu, target, 0x858f3bu, 2u)) {
                    const Lufia2ExecutionResult result = {
                        LUFIA2_EXECUTION_CHILD_UNWOUND, 0x858f3bu, 0u};
                    return result;
                }
                if (cpu->program_bank != 0x85u ||
                    cpu->accumulator_is_8_bit != 1u || cpu->index_is_8_bit > 1u)
                    return ExecutionHandoff(cpu,
                        ((uint32_t)cpu->program_bank << 16) | 0x8f3cu);
            } else {
                if (target != 0x85a8e7u)
                    return ExecutionHandoff(cpu, target);
                (void)Lufia2BattleFillPrimaryWave(memory, cpu);
                SimulateRtsFrame(memory, cpu);
            }
            SetIndexWidth(cpu, 1u);
        }
        BattleAdvanceTimer(cpu);
        if (!cpu->zero && ++visited == BATTLE_TIMER_END / BATTLE_TIMER_STRIDE)
            return ExecutionHandoff(cpu, 0x858f1fu);
    } while (!cpu->zero);
    SetIndexWidth(cpu, 0u);
    return ExecutionReturned(0x858f49u);
}

Lufia2ExecutionResult Lufia2BattleRunTimedCallbacks(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u ||
        cpu->accumulator_is_8_bit != 1u || cpu->index_is_8_bit > 1u)
        return ExecutionHandoff(cpu, 0x858f1bu);
    return BattleRunTimerSlots(memory, cpu, child, context, false);
}

Lufia2ExecutionResult Lufia2BattleResumeTimedCallbacks(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u ||
        cpu->accumulator_is_8_bit != 1u || cpu->index_is_8_bit > 1u)
        return ExecutionHandoff(cpu, 0x858f3cu);
    return BattleRunTimerSlots(memory, cpu, child, context, true);
}

static Lufia2ExecutionResult BattleRunTimers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    SimulateJsrFrame(memory, cpu, 0x8e78u);
    const Lufia2ExecutionResult result =
        BattleRunTimerSlots(memory, cpu, child, context, false);
    if (result.flow == LUFIA2_EXECUTION_RETURNED)
        SimulateRtsFrame(memory, cpu);
    return result;
}

enum {
    BATTLE_CALLBACK_SELECTORS = WRAM_BATTLE_NMI_CALLBACK_SELECTORS,
    BATTLE_CALLBACK_LIST_BYTES = 16u,
    ROM_BATTLE_CALLBACK_TARGETS = 0x9e37u
};

static Lufia2ExecutionResult BattleRunCallbackList(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    LoadY16(cpu, 0u);
    do {
        LoadAAbsolute8(memory, cpu, BATTLE_CALLBACK_SELECTORS, cpu->y);
        if (cpu->negative)
            break;
        SetAccumulatorWidth(cpu, 0u);
        OpAslA(cpu);
        OpAndValue(cpu, 0xffu);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1u);
        PushY(memory, cpu);
        if (!child)
            return ExecutionHandoff(cpu, 0x858e8bu);
        const uint32_t target = 0x850000u | Read16ProgramIndexed(
            memory, cpu, ROM_BATTLE_CALLBACK_TARGETS, cpu->x);
        if (!CallChildWithFrame(memory, cpu, child, context, 0x858e8bu,
                                target, 2u, 0x85u)) {
            const Lufia2ExecutionResult result = {
                LUFIA2_EXECUTION_CHILD_UNWOUND, cpu->resume_pc, 0u};
            return result;
        }
        if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit != 1u ||
            cpu->index_is_8_bit != 0u)
            return ExecutionHandoff(cpu,
                ((uint32_t)cpu->program_bank << 16) | 0x8e8eu);
        OpPullY(memory, cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, BATTLE_CALLBACK_LIST_BYTES);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x858e97u);
}

/* $85:8DC5: battle NMI work via the $00:0067 vector. */
Lufia2ExecutionResult Lufia2BattleNmiUploadsWithChildren(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    static const uint16_t scroll_regs[6] = {
        SNES_BG1HOFS, SNES_BG1VOFS, SNES_BG2HOFS, SNES_BG2VOFS, SNES_BG3HOFS, SNES_BG3VOFS};
    static const uint16_t window_regs[8] = {
        SNES_W12SEL, SNES_WOBJSEL, SNES_WH1, SNES_WH3, SNES_WOBJLOG, SNES_TS, SNES_TSW,
        SNES_CGADSUB};
    Lufia2ExecutionResult result;
    unsigned i;

    result.flow = LUFIA2_EXECUTION_RETURNED;
    result.pc = 0x858e97u;
    result.dispatches = 0;
    /* The NMI handler always enters with M=1. */
    if (!cpu->accumulator_is_8_bit) {
        result.flow = LUFIA2_EXECUTION_BOUNDARY;
        result.pc = cpu->resume_pc = 0x858dc5u;
        return result;
    }
    PushDataBank(memory, cpu);                                 /* 8DC5 */
    Push8(memory, cpu, 0x85u);
    PullDataBank(memory, cpu);
    SetIndexWidth(cpu, 0);
    BattleVramQueue(memory, cpu);
    LoadA8(cpu, 0xffu);                                        /* 8DCD */
    TestBitsAbsolute8(memory, cpu, 0x15b3u, 0);
    if (!cpu->zero) {
        for (i = 0; i < 12u; ++i) {
            LoadAAbsolute8(memory, cpu, (uint16_t)(0x0594u + i), 0);
            StoreAAbsolute8(memory, cpu, scroll_regs[i >> 1], 0);
        }
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x059au, 0));
        Write16Absolute(memory, cpu, 0x1256u, cpu->x);
    }
    BattleHdmaChannels(memory, cpu);                           /* 8E22 */
    for (i = 0; i < 8u; ++i) {
        LoadX16(cpu, Read16AbsoluteIndexed(
            memory, cpu, (uint16_t)(0x123cu + 2u * i), 0));
        Write16Absolute(memory, cpu, window_regs[i], cpu->x);
    }
    LoadAAbsolute8(memory, cpu, 0x1268u, 0);                   /* 8E55 */
    if (cpu->zero) {
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1249u, 0));
        Write16Absolute(memory, cpu, 0x1252u, cpu->x);
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1245u, 0));
        Write16Absolute(memory, cpu, 0x1250u, cpu->x);
    }
    LoadAAbsolute8(memory, cpu, 0x1254u, 0);                   /* 8E66 */
    StoreAAbsolute8(memory, cpu, 0x1255u, 0);
    LoadAAbsolute8(memory, cpu, WRAM_BRIGHTNESS, 0);
    StoreAAbsolute8(memory, cpu, SNES_INIDISP, 0);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0xdbu)));
    if (!cpu->zero) {
        result = BattleRunTimers(memory, cpu, child, context);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
    }
    return BattleRunCallbackList(memory, cpu, child, context);
}

Lufia2ExecutionResult Lufia2BattleNmiUploads(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return Lufia2BattleNmiUploadsWithChildren(memory, cpu, NULL, NULL);
}

Lufia2ExecutionResult Lufia2BattleUploadVramQueue(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x85u || cpu->direct_page || cpu->decimal ||
        cpu->index_is_8_bit || cpu->accumulator_is_8_bit != 1u ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x858e98u);
    BattleVramQueueBody(memory, cpu);
    return ExecutionReturned(0x858ed1u);
}
