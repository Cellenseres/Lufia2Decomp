#include "core/cpu_ops.h"
#include "lufia2/text.h"
#include "text/text_internal.h"

enum {
    WRITE_TILE_TEXT = 0xb128u,
    WRITE_DOUBLE_ROW_TEXT = 0xb1beu,
    TILE_TEXT_BUFFER = 0x7e3000u,
    ROM_TILE_GLYPHS = 0x80b13eu,
    ROM_DOUBLE_ROW_GLYPHS = 0x80b1d7u,
    DP_UPLOAD_REQUESTS = 0x74u,
    TILE_TEXT_BUDGET = 4096u
};

static uint32_t ClearTileText(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushY(memory, cpu);
    OpLdx(cpu, 0x3000u);
    OpLdy(cpu, 0x0800u);
    SimulateJsrFrame(memory, cpu, 0xb131u);
    const Lufia2ExecutionResult result = Lufia2TextClearUploadRows(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result.pc;
    SimulateRtsFrame(memory, cpu);
    OpPullY(memory, cpu);
    return 0u;
}

static void WriteTileCharacter(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                               bool double_row) {
    OpPushX(memory, cpu);
    ExchangeAccumulatorBytes(cpu);
    OpLoadA(cpu, 0u);
    ExchangeAccumulatorBytes(cpu);
    OpTax(cpu);
    OpLda(memory, cpu,
          OpLongX(cpu, double_row ? ROM_DOUBLE_ROW_GLYPHS : ROM_TILE_GLYPHS));
    OpPullX(memory, cpu);
    OpSta(memory, cpu, OpLongX(cpu, TILE_TEXT_BUFFER));
    if (double_row) {
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x10u);
        OpSta(memory, cpu, OpLongX(cpu, TILE_TEXT_BUFFER + 0x40u));
        OpLoadA(cpu, 0x28u);
        OpSta(memory, cpu, OpLongX(cpu, TILE_TEXT_BUFFER + 1u));
        OpSta(memory, cpu, OpLongX(cpu, TILE_TEXT_BUFFER + 0x41u));
        OpInx(cpu);
    } else {
        OpInx(cpu);
        OpLoadA(cpu, 0x28u);
        OpSta(memory, cpu, OpLongX(cpu, TILE_TEXT_BUFFER));
    }
    OpInx(cpu);
}

uint32_t Lufia2TextExecuteTileCommand(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                      uint16_t handler) {
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal ||
        cpu->stack < 0x1f10u || cpu->stack > 0x1ffcu ||
        (handler != WRITE_TILE_TEXT && handler != WRITE_DOUBLE_ROW_TEXT))
        return 0u;
    const bool double_row = handler == WRITE_DOUBLE_ROW_TEXT;
    if (!double_row) {
        const uint32_t handoff = ClearTileText(memory, cpu);
        if (handoff)
            return handoff;
    }
    Lufia2TextNextWord(memory, cpu, double_row ? 0xb1c0u : 0xb135u);
    OpTax(cpu);
    for (unsigned count = 0u; count < TILE_TEXT_BUDGET; ++count) {
        Lufia2TextNextByte(memory, cpu, double_row ? 0xb1c4u : 0xb139u);
        OpOraValue(cpu, 0u);
        if (cpu->zero) {
            OpLoadA(cpu, 8u);
            OpSta(memory, cpu, OpDp(cpu, DP_UPLOAD_REQUESTS));
            if (double_row) {
                SimulateJsrFrame(memory, cpu, 0xb1f3u);
                return 0x80bf0bu;
            }
            return LUFIA2_TEXT_COMMAND_NEXT;
        }
        WriteTileCharacter(memory, cpu, double_row);
    }
    return double_row ? 0x80b1c2u : 0x80b137u;
}
