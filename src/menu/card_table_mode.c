#include "menu/card_table_outcome.h"
#include "lufia2/card_table.h"

enum {
    MODE_SYMBOL = WRAM_CARD_TABLE_RANDOM_SELECTION & 0xffffu,
    MODE_FLAGS = (WRAM_CARD_TABLE_RANDOM_SELECTION + 2u) & 0xffffu,
    MODE_PENDING = WRAM_CARD_TABLE_PENDING_WORDS & 0xffffu,
    MODE_DEFERRED = (WRAM_CARD_TABLE_PENDING_WORDS + 2u) & 0xffffu,
    MODE_DEBUG = (WRAM_BOARD_GAME_SAVED_GROUP_CELLS + 8u) & 0xffffu,
    MODE_MULTIPLICAND = 0x6008u,
    MODE_MULTIPLIER = 0x600au,
    MODE_PRODUCT = 0x600cu,
    ROM_JACKPOT_ODDS = 0x84f2f0u,
    ROM_BONUS_ODDS = 0x84f2fcu
};

static bool RollMode(TableService *service, uint32_t site, uint16_t limit) {
    OpLoadA(service->cpu, limit);
    return TableServiceCall(service, site, 0x808299u, false);
}

static bool RollSelectionChance(TableService *service, uint32_t site) {
    OpLoadA(service->cpu, 4u);
    service->cpu->carry = 1u;
    OpSbcValue(service->cpu, OpReadM(service->memory, service->cpu,
        OpAbs(service->cpu, OUTCOME_SELECTION)));
    return TableServiceCall(service, site, 0x808299u, false);
}

static void SetModeBias(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t flags, uint16_t symbol) {
    OpLoadA(cpu, flags);
    OpSta(memory, cpu, OpAbs(cpu, MODE_FLAGS));
    OpLoadA(cpu, symbol);
    OpSta(memory, cpu, OpAbs(cpu, MODE_SYMBOL));
}

static void SetPendingMode(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t pending) {
    OpLoadA(cpu, pending);
    OpSta(memory, cpu, OpAbs(cpu, MODE_PENDING));
}

static bool ResolveDeferredMode(TableService *service, bool *forced) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    OpLda(memory, cpu, OpAbs(cpu, MODE_PENDING));
    if (!cpu->zero) return true;
    OpLda(memory, cpu, OpAbs(cpu, MODE_DEFERRED));
    if (cpu->zero) return true;
    OpStz(memory, cpu, OpAbs(cpu, MODE_DEFERRED));
    OpCmpValue(cpu, 1u);
    if (cpu->zero) {
        if (!RollMode(service, 0x84df2cu, 5u)) return false;
        OpCmpValue(cpu, 0u);
        if (!cpu->zero) return true;
        if (!RollMode(service, 0x84df38u, 7u)) return false;
        OpCmpValue(cpu, 0u);
        if (cpu->zero) {
            SetPendingMode(memory, cpu, 4u);
            SetModeBias(memory, cpu, 0x0200u, 1u);
            *forced = true;
            return true;
        }
        OpCmpValue(cpu, 3u);
        if (cpu->negative) {
            SetPendingMode(memory, cpu, 3u);
            return true;
        }
        OpCmpValue(cpu, 5u);
        SetPendingMode(memory, cpu, cpu->negative ? 2u : 1u);
        return true;
    }
    if (!RollMode(service, 0x84df50u, 6u)) return false;
    OpCmpValue(cpu, 0u);
    if (cpu->zero) {
        SetPendingMode(memory, cpu, 5u);
        return true;
    }
    OpCmpValue(cpu, 3u);
    if (cpu->negative) return true;
    if (!RollMode(service, 0x84df61u, 3u)) return false;
    OpCmpValue(cpu, 1u);
    if (cpu->negative) SetPendingMode(memory, cpu, 2u);
    else SetPendingMode(memory, cpu, cpu->zero ? 3u : 1u);
    return true;
}

static bool TryJackpotMode(TableService *service, bool *selected) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    OpLda(memory, cpu, OpAbs(cpu, MODE_PENDING));
    cpu->carry = 0u;
    RolA16(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, ROM_JACKPOT_ODDS));
    if (!TableServiceCall(service, 0x84dfa0u, 0x808299u, false)) return false;
    OpCmpValue(cpu, 0u);
    if (!cpu->zero) return true;
    if (!RollSelectionChance(service, 0x84dfb0u)) return false;
    OpCmpValue(cpu, 0u);
    if (!cpu->zero) return true;
    OpLda(memory, cpu, OpAbs(cpu, MODE_PENDING));
    OpCmpValue(cpu, 5u);
    if (cpu->zero) {
        if (!RollMode(service, 0x84dfc4u, 2u)) return false;
        OpCmpValue(cpu, 0u);
        if (!cpu->zero) return true;
        OpStz(memory, cpu, OpAbs(cpu, MODE_PENDING));
    }
    SetModeBias(memory, cpu, 0x0200u, 1u);
    OpLda(memory, cpu, OpAbs(cpu, MODE_PENDING));
    OpCmpValue(cpu, 1u);
    if (cpu->zero) {
        if (!RollMode(service, 0x84dfe7u, 3u)) return false;
        OpCmpValue(cpu, 0u);
        if (!cpu->zero) OpStz(memory, cpu, OpAbs(cpu, MODE_PENDING));
    } else {
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            if (!RollMode(service, 0x84dffdu, 5u)) return false;
            OpCmpValue(cpu, 2u);
            if (!cpu->negative) OpStz(memory, cpu, OpAbs(cpu, MODE_PENDING));
        } else {
            OpCmpValue(cpu, 4u);
            if (cpu->zero) {
                if (!RollMode(service, 0x84e013u, 5u)) return false;
                OpCmpValue(cpu, 2u);
                if (!cpu->negative) OpStz(memory, cpu, OpAbs(cpu, MODE_PENDING));
            }
        }
    }
    *selected = true;
    return true;
}

static bool TryBonusMode(TableService *service, bool *selected) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    OpLda(memory, cpu, OpAbs(cpu, MODE_PENDING));
    cpu->carry = 0u;
    RolA16(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, ROM_BONUS_ODDS));
    if (!TableServiceCall(service, 0x84e02cu, 0x808299u, false)) return false;
    OpCmpValue(cpu, 0u);
    if (!cpu->zero) return true;
    if (!RollSelectionChance(service, 0x84e03cu)) return false;
    OpCmpValue(cpu, 0u);
    if (!cpu->zero) return true;
    OpLda(memory, cpu, OpAbs(cpu, MODE_PENDING));
    OpCmpValue(cpu, 5u);
    if (cpu->zero) {
        if (!RollMode(service, 0x84e050u, 2u)) return false;
        OpCmpValue(cpu, 0u);
        if (!cpu->zero) return true;
        OpStz(memory, cpu, OpAbs(cpu, MODE_PENDING));
    }
    SetModeBias(memory, cpu, 0x0100u, 2u);
    OpLda(memory, cpu, OpAbs(cpu, MODE_PENDING));
    OpCmpValue(cpu, 3u);
    if (cpu->zero) {
        if (!RollMode(service, 0x84e073u, 3u)) return false;
        OpCmpValue(cpu, 0u);
        if (!cpu->zero) OpStz(memory, cpu, OpAbs(cpu, MODE_PENDING));
    } else {
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            if (!RollMode(service, 0x84e089u, 10u)) return false;
            OpCmpValue(cpu, 0u);
            if (cpu->zero) OpStz(memory, cpu, OpAbs(cpu, MODE_PENDING));
        }
    }
    *selected = true;
    return true;
}

static bool BeginCounterMode(TableService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    if (!RollMode(service, 0x84e0bfu, 4u)) return false;
    OpTax(cpu);
    OpLoadA(cpu, 10u);
    OpCpx(cpu, 0u);
    if (cpu->zero) {
        cpu->carry = 0u;
        OpAdcValue(cpu, 20u);
    }
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_COUNTDOWN));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_VARIANT));
    return true;
}

static bool ChooseCommonSymbol(TableService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    if (!RollMode(service, 0x84e124u, 2u)) return false;
    cpu->carry = 0u;
    OpAdcValue(cpu, 3u);
    OpSta(memory, cpu, OpAbs(cpu, MODE_SYMBOL));
    if (!RollMode(service, 0x84e132u, 50u)) return false;
    OpCmpValue(cpu, 4u);
    if (cpu->zero) {
        OpLoadA(cpu, 7u);
        OpSta(memory, cpu, OpAbs(cpu, MODE_SYMBOL));
        return true;
    }
    if (!RollMode(service, 0x84e146u, 50u)) return false;
    OpCmpValue(cpu, 4u);
    if (cpu->zero) {
        if (!RollMode(service, 0x84e152u, 2u)) return false;
        cpu->carry = 0u;
        OpAdcValue(cpu, 5u);
        OpSta(memory, cpu, OpAbs(cpu, MODE_SYMBOL));
    }
    return true;
}

static bool ChooseGeneralMode(TableService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    OpLda(memory, cpu, OpAbs(cpu, OUTCOME_VARIANT));
    OpAndValue(cpu, 1u);
    bool counter_mode = !cpu->zero;
    if (!counter_mode) {
        if (!RollMode(service, 0x84e0a3u, 150u)) return false;
        OpCmpValue(cpu, 4u);
        if (cpu->zero) {
            if (!RollSelectionChance(service, 0x84e0b3u)) return false;
            OpCmpValue(cpu, 0u);
            if (cpu->zero) {
                if (!BeginCounterMode(service)) return false;
                counter_mode = true;
            }
        }
        if (!counter_mode) {
            if (!RollMode(service, 0x84e0e7u, 8u)) return false;
            OpCmpValue(cpu, 4u);
            if (!cpu->zero) return true;
            if (!RollSelectionChance(service, 0x84e0f7u)) return false;
            OpCmpValue(cpu, 0u);
            if (!cpu->zero) return true;
            OpLoadA(cpu, 1u);
            OpOraValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, MODE_FLAGS)));
            OpSta(memory, cpu, OpAbs(cpu, MODE_FLAGS));
            if (!RollMode(service, 0x84e10cu, 4u)) return false;
            OpCmpValue(cpu, 2u);
            if (cpu->zero) {
                if (!RollMode(service, 0x84e118u, 250u)) return false;
                OpCmpValue(cpu, 100u);
                if (cpu->zero) {
                    if (!BeginCounterMode(service)) return false;
                    counter_mode = true;
                }
            }
        }
    }
    if (counter_mode) {
        OpLoadA(cpu, 2u);
        OpOraValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, MODE_FLAGS)));
        OpSta(memory, cpu, OpAbs(cpu, MODE_FLAGS));
    }
    return ChooseCommonSymbol(service);
}

static bool DrawRoundModeDebug(TableService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    OpLda(memory, cpu, OpAbs(cpu, MODE_DEBUG));
    if (cpu->zero) return true;
    OpLda(memory, cpu, OpAbs(cpu, MODE_PENDING));
    OpSta(memory, cpu, OpAbs(cpu, MODE_MULTIPLICAND));
    OpLoadA(cpu, 10u);
    OpSta(memory, cpu, OpAbs(cpu, MODE_MULTIPLIER));
    if (!TableServiceCall(service, 0x84e16eu, 0x84a5dfu, false)) return false;
    OpLda(memory, cpu, OpAbs(cpu, MODE_PRODUCT));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, MODE_SYMBOL));
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_NUMBER));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_NUMBER_SLOT));
    if (!TableServiceCall(service, 0x84e181u, 0x84eb36u, false)) return false;
    OpLda(memory, cpu, OpAbs(cpu, OUTCOME_CHANGED_DIGITS));
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_FIRST_MOVEMENT));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, OUTCOME_FIRST_PENDING));
    return TableServiceCall(service, 0x84e190u, 0x84ebdbu, false);
}

Lufia2ExecutionResult Lufia2CardTableChooseRoundMode(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !OutcomeWordEntry(cpu)) return ExecutionHandoff(cpu, 0x84dedcu);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84e193u), false};
    OpLda(memory, cpu, OpAbs(cpu, OUTCOME_VARIANT));
    OpAndValue(cpu, 6u);
    if (!cpu->zero) return service.result;
    OpStz(memory, cpu, OpAbs(cpu, MODE_SYMBOL));
    OpLda(memory, cpu, OpAbs(cpu, MODE_FLAGS));
    OpAndValue(cpu, 0xff00u);
    OpSta(memory, cpu, OpAbs(cpu, MODE_FLAGS));
    OpCmpValue(cpu, 0x0100u);
    if (cpu->zero) {
        OpLoadA(cpu, 2u);
        OpSta(memory, cpu, OpAbs(cpu, MODE_SYMBOL));
    } else {
        OpCmpValue(cpu, 0x0200u);
        if (cpu->zero) {
            OpLoadA(cpu, 1u);
            OpSta(memory, cpu, OpAbs(cpu, MODE_SYMBOL));
        } else {
            OpLda(memory, cpu, OpAbs(cpu, OUTCOME_VARIANT));
            bool selected = false;
            if (cpu->zero) {
                if (!ResolveDeferredMode(&service, &selected)) return service.result;
                if (!selected && !TryJackpotMode(&service, &selected)) return service.result;
                if (!selected && !TryBonusMode(&service, &selected)) return service.result;
            }
            if (!selected && !ChooseGeneralMode(&service)) return service.result;
        }
    }
    DrawRoundModeDebug(&service);
    return service.result;
}
