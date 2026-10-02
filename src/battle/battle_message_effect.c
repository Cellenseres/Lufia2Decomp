#include "battle/battle_internal.h"
#include "core/snes_registers.h"
#include "core/wram_view.h"

/* The message effect drives HDMA channel 4 (a two-register indirect transfer
 * to $2110) through the channel record at $1B03; its cleanup sets up channel
 * 3 (a direct transfer to $2101) through the record at $1AFE. The records
 * share one layout, and a request bit in $DA latches each channel. */
enum {
    HDMA_REQUESTS = 0xdau,
    TIMER_REQUESTS = 0xdbu,
    HDMA_CHANNEL_3_REQUEST = 0x08u,
    HDMA_CHANNEL_4_REQUEST = 0x10u,
    MESSAGE_TIMER_REQUEST = 0x04u,
    HDMA_RECORD_MODE = 0u,
    HDMA_RECORD_REGISTER = 1u,
    HDMA_RECORD_TABLE = 2u,
    HDMA_RECORD_TABLE_BANK = 4u,
    HDMA_CHANNEL_3_RECORD = 0x1afeu,
    HDMA_CHANNEL_4_RECORD = 0x1b03u,
    HDMA_INDIRECT_TWO_REGISTERS = 0x42u,
    MESSAGE_TABLE_BANK = 0x85u,
    MESSAGE_EFFECT_TABLE = 0xa0ceu,
    MESSAGE_CLEANUP_TABLE = 0xa081u,
    MESSAGE_PARAMETER = 0x1255u,
    MESSAGE_PARAMETER_SOURCE = 0x85a0deu,
    MESSAGE_START_VALUE = 0xdfu,
    MESSAGE_END_VALUE = 0xffffu,
    MESSAGE_TEXT_LEAD = 0x1268u,
    MESSAGE_EFFECT_STATE_0 = 0x1b27u,
    MESSAGE_EFFECT_STATE_1 = 0x1b28u,
    MESSAGE_EFFECT_STATE_2 = 0x1b29u,
};

static const uint16_t kMessagePositions[3] = {0x1258u, 0x125au, 0x125cu};

/* Sets the bits in a direct-page byte (TSB); only the zero flag follows the
 * old value. */
static void SetRequestBits(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                           uint8_t location, uint8_t bits) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const uint8_t old = WramRead(wram, location);

    cpu->zero = (old & bits) == 0;
    WramWrite(wram, location, (uint8_t)(old | bits));
}

static void ClearRequestBits(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                             uint8_t location, uint8_t bits) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const uint8_t old = WramRead(wram, location);

    cpu->zero = (old & bits) == 0;
    WramWrite(wram, location, (uint8_t)(old & ~bits));
}

/* $85:AADC: install the message display record and its original DMA bank. */
Lufia2ExecutionResult Lufia2BattleStartMessageEffect(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const Lufia2Wram io = WramViewLong(memory);
    unsigned i;

    for (i = 0; i < 3u; ++i)
        WramWrite16(wram, kMessagePositions[i], MESSAGE_START_VALUE);
    WramWrite(wram, MESSAGE_PARAMETER, Read8(memory, MESSAGE_PARAMETER_SOURCE));
    WramWrite(wram, HDMA_CHANNEL_4_RECORD + HDMA_RECORD_MODE,
              HDMA_INDIRECT_TWO_REGISTERS);
    WramWrite(wram, HDMA_CHANNEL_4_RECORD + HDMA_RECORD_REGISTER, 0x10u);
    WramWrite16(wram, HDMA_CHANNEL_4_RECORD + HDMA_RECORD_TABLE, MESSAGE_EFFECT_TABLE);
    WramWrite(wram, HDMA_CHANNEL_4_RECORD + HDMA_RECORD_TABLE_BANK, MESSAGE_TABLE_BANK);
    WramWrite(io, SNES_DASB(4), MESSAGE_TABLE_BANK);
    SetRequestBits(memory, cpu, HDMA_REQUESTS, HDMA_CHANNEL_4_REQUEST);
    WramWrite(wram, MESSAGE_EFFECT_STATE_2, 0x0fu);
    WramWrite(wram, MESSAGE_EFFECT_STATE_1, 1u);
    WramWrite(wram, MESSAGE_EFFECT_STATE_0, 0xffu);
    cpu->x = MESSAGE_EFFECT_TABLE;
    LoadA8(cpu, MESSAGE_TIMER_REQUEST);
    SetRequestBits(memory, cpu, TIMER_REQUESTS, MESSAGE_TIMER_REQUEST);
    return ExecutionReturned(0x85ab27u);
}

/* $85:AB28: zero keeps the message; a decrement to zero clears it. */
Lufia2ExecutionResult Lufia2BattleTickMessageEffect(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    unsigned i;

    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, WramRead16(wram, WRAM_BATTLE_WAIT_COUNTER));
    if (!cpu->zero) {
        if (WramStep16(wram, WRAM_BATTLE_WAIT_COUNTER, -1) == 0) {
            for (i = 0; i < 3u; ++i) {
                LoadA16(cpu, MESSAGE_END_VALUE);
                WramWrite16(wram, kMessagePositions[i], cpu->accumulator);
            }
            SetAccumulatorWidth(cpu, 1);
            LoadA8(cpu, MESSAGE_TIMER_REQUEST);
            ClearRequestBits(memory, cpu, TIMER_REQUESTS, MESSAGE_TIMER_REQUEST);
            WramWrite(wram, MESSAGE_TEXT_LEAD, 0);
            WramWrite(wram, MESSAGE_EFFECT_STATE_0, 0);
            return ExecutionReturned(0x85ab52u);
        }
    }
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 1u);
    WramWrite(wram, MESSAGE_EFFECT_STATE_1, A8(cpu));
    return ExecutionReturned(0x85ab5au);
}

/* $85:AB5B: install the cleanup record and update the original request bits. */
Lufia2ExecutionResult Lufia2BattleQueueMessageCleanup(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);

    /* The first byte is the direct page's low byte, which the original uses
     * as a zero. */
    TransferDirectToA(cpu);
    WramWrite(wram, HDMA_CHANNEL_3_RECORD + HDMA_RECORD_MODE, A8(cpu));
    WramWrite(wram, HDMA_CHANNEL_3_RECORD + HDMA_RECORD_REGISTER, 1u);
    WramWrite16(wram, HDMA_CHANNEL_3_RECORD + HDMA_RECORD_TABLE, MESSAGE_CLEANUP_TABLE);
    cpu->x = MESSAGE_CLEANUP_TABLE;
    WramWrite(wram, HDMA_CHANNEL_3_RECORD + HDMA_RECORD_TABLE_BANK, MESSAGE_TABLE_BANK);
    SetRequestBits(memory, cpu, HDMA_REQUESTS, HDMA_CHANNEL_3_REQUEST);
    LoadA8(cpu, MESSAGE_TIMER_REQUEST);
    ClearRequestBits(memory, cpu, TIMER_REQUESTS, MESSAGE_TIMER_REQUEST);
    return ExecutionReturned(0x85ab77u);
}
