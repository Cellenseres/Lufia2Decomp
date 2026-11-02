#include "battle/battle_internal.h"
#include "core/snes_registers.h"

enum {
    MESSAGE_RESOURCE_TABLE = 0x978000u,
    MESSAGE_LONG_DICTIONARY = 0x8eea00u,
    MESSAGE_SHORT_DICTIONARY = 0x9eddccu,
    MESSAGE_EXPANSION_BUFFER = WRAM_BATTLE_MESSAGE_RESOURCE_BUFFER & 0xffffu,
    MESSAGE_LINE_COLUMNS = 18u,
    MESSAGE_TOTAL_COLUMNS = 36u,
    MESSAGE_PADDING = 0x10u,
    MESSAGE_SYMBOL_LIMIT = 65536u,
    MESSAGE_PADDING_LIMIT = 256u
};

static void MessageCopyDictionary(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint32_t dictionary) {
    OpPushX(memory, cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, dictionary));
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    for (;;) {
        OpLda(memory, cpu, OpLongX(cpu, dictionary));
        if (cpu->zero)
            break;
        OpSta(memory, cpu, OpAbsY(cpu, MESSAGE_EXPANSION_BUFFER));
        OpInx(cpu);
        OpIny(cpu);
    }
    OpPullX(memory, cpu);
}

static void MessageReadDictionaryArgument(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint8_t command_base) {
    cpu->carry = 1u;
    OpSbcValue(cpu, command_base);
    OpSta(memory, cpu, SNES_WRMPYA);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, SNES_WRMPYB);
    OpInx(cpu);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpLongX(cpu, MESSAGE_RESOURCE_TABLE));
}

static void MessageExpandCommand(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (cpu->negative) {
        OpAslA(cpu);
        OpRepWidths(cpu, 0x20u);
        OpAndValue(cpu, 0xfeu);
        OpInx(cpu);
        MessageCopyDictionary(memory, cpu, MESSAGE_SHORT_DICTIONARY);
        return;
    }
    OpCmpValue(cpu, 0x20u);
    if (cpu->carry)
        return;
    OpCmpValue(cpu, 0x0au);
    if (cpu->zero)
        return;
    if (!cpu->carry) {
        MessageReadDictionaryArgument(memory, cpu, 5u);
        OpRepWidths(cpu, 0x20u);
        cpu->carry = 0u;
        OpAdc(memory, cpu, SNES_RDMPYL);
        OpAdcValue(cpu, 0x80u);
        OpAslA(cpu);
        OpInx(cpu);
        MessageCopyDictionary(memory, cpu, MESSAGE_SHORT_DICTIONARY);
    } else {
        MessageReadDictionaryArgument(memory, cpu, 0x10u);
        OpInx(cpu);
        OpRepWidths(cpu, 0x20u);
        cpu->carry = 0u;
        OpAdc(memory, cpu, SNES_RDMPYL);
        OpAslA(cpu);
        MessageCopyDictionary(memory, cpu, MESSAGE_LONG_DICTIONARY);
    }
}

static void MessageRestoreExpansionFrame(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x30u);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    PullAccumulator16(memory, cpu);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
}

Lufia2ExecutionResult Lufia2BattleExpandActionMessage(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x85u || cpu->direct_page)
        return ExecutionHandoff(cpu, 0x85c4f1u);
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    OpRepWidths(cpu, 0x30u);
    PushAccumulator16(memory, cpu);
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ACTION_MESSAGE_ID));
    OpCmpValue(cpu, 0x0300u);
    if (cpu->carry) {
        cpu->carry = 1u;
        OpSbcValue(cpu, 0xd0u);
    }
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, MESSAGE_RESOURCE_TABLE));
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpSetDataBank(memory, cpu, 0x7fu);
    OpLdy(cpu, 0u);
    for (;;) {
        OpLda(memory, cpu, OpLongX(cpu, MESSAGE_RESOURCE_TABLE));
        if (!cpu->zero) {
            const uint8_t command = (uint8_t)cpu->accumulator;
            MessageExpandCommand(memory, cpu);
            if (command >= 0x80u || (command < 0x20u && command != 0x0au))
                continue;
        }
        OpSta(memory, cpu, OpAbsY(cpu, MESSAGE_EXPANSION_BUFFER));
        OpIny(cpu);
        OpInx(cpu);
        OpCmpValue(cpu, 0u);
        if (cpu->zero)
            break;
    }
    MessageRestoreExpansionFrame(memory, cpu);
    return ExecutionReturned(0x85c5b4u);
}

static bool MessagePadColumns(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t columns, Lufia2ExecutionResult *result) {
    for (unsigned padding = 0u;; ++padding) {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_MESSAGE_WINDOW_COLUMNS));
        OpCmpValue(cpu, columns);
        if (cpu->zero)
            return true;
        if (padding == MESSAGE_PADDING_LIMIT) {
            *result = ExecutionHandoff(cpu,
                columns == MESSAGE_LINE_COLUMNS ? 0x85c5dcu : 0x85c5f0u);
            return false;
        }
        OpLoadA(cpu, MESSAGE_PADDING);
        OpSta(memory, cpu, OpLongX(cpu, WRAM_BATTLE_MESSAGE_RESOURCE_BUFFER));
        OpInx(cpu);
        OpStepMem(memory, cpu, OpAbs(cpu, WRAM_BATTLE_MESSAGE_WINDOW_COLUMNS), 1);
    }
}

/* $85:C5B5: format text; restore inherited expansion frame. */
Lufia2ExecutionResult Lufia2BattleFormatResourceMessage(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x85c5b5u);
    OpLdx(cpu, 0u);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_BATTLE_MESSAGE_WINDOW_COLUMNS));
    Lufia2ExecutionResult result;
    for (unsigned symbols = 0u; symbols < MESSAGE_SYMBOL_LIMIT; ++symbols) {
        OpLda(memory, cpu, OpAbsY(cpu, 0u));
        if (cpu->zero) {
            if (!MessagePadColumns(memory, cpu, MESSAGE_TOTAL_COLUMNS, &result))
                return result;
            TransferDirectToA(cpu);
            OpSta(memory, cpu, OpLongX(cpu, WRAM_BATTLE_MESSAGE_RESOURCE_BUFFER));
            MessageRestoreExpansionFrame(memory, cpu);
            return ExecutionReturned(0x85c60du);
        }
        OpCmpValue(cpu, 0x0au);
        if (cpu->zero) {
            if (!MessagePadColumns(memory, cpu, MESSAGE_LINE_COLUMNS, &result))
                return result;
            OpIny(cpu);
            continue;
        }
        OpCmpValue(cpu, 0x10u);
        if (!cpu->carry) {
            OpSta(memory, cpu, OpLongX(cpu, WRAM_BATTLE_MESSAGE_RESOURCE_BUFFER));
            OpIny(cpu);
            OpInx(cpu);
            OpLda(memory, cpu, OpAbsY(cpu, 0u));
        }
        OpSta(memory, cpu, OpLongX(cpu, WRAM_BATTLE_MESSAGE_RESOURCE_BUFFER));
        OpStepMem(memory, cpu, OpAbs(cpu, WRAM_BATTLE_MESSAGE_WINDOW_COLUMNS), 1);
        OpIny(cpu);
        OpInx(cpu);
    }
    return ExecutionHandoff(cpu, 0x85c5bbu);
}
