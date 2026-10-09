#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/system.h"
#include "system/wram.h"

enum {
    DP_DEFAULT_RECORD_MODIFIERS = 0xb5u,
    DP_DEFAULT_RECORD_ITEMS = 0xb8u,
    DEFAULT_RECORD_MODIFIERS = 0xf4d6u,
    DEFAULT_RECORD_ITEMS = 0xf44eu,
    DEFAULT_RECORD_BUFFER_BANK = 0x7fu,
    DEFAULT_RECORD_CLEAR_FIRST = 0x09f0u,
    DEFAULT_RECORD_CLEAR_SIZE = 0x07e8u,
    DEFAULT_CAPSULE_RECORD = 0x10dfu,
    DEFAULT_PARTY_RECORD_FIRST = 0x0badu,
    DEFAULT_PARTY_RECORD_SIZE = 0xbeu,
    DEFAULT_PARTY_RECORD_COUNT = 7u,
    DEFAULT_PARTY_SLOT_COUNT = 4u,
    DEFAULT_RECORD_DATA = 0xb2a1u,
    DEFAULT_RECORD_DATA_BANK = 0x85u,
    DEFAULT_RECORD_UNK_0B55 = 0x0b55u,
    WRAM_PORT_DATA = 0x2180u,
    WRAM_PORT_ADDRESS = 0x2181u,
    WRAM_PORT_ADDRESS_BANK = 0x2183u
};

static Lufia2ExecutionResult DefaultRecordChildUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static void SetDefaultRecordPointers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, DEFAULT_RECORD_ITEMS);
    OpWriteX(memory, cpu, OpDp(cpu, DP_DEFAULT_RECORD_ITEMS), cpu->x);
    OpLoadA(cpu, DEFAULT_RECORD_BUFFER_BANK);
    OpSta(memory, cpu, OpDp(cpu, DP_DEFAULT_RECORD_ITEMS + 2u));
    OpLdx(cpu, DEFAULT_RECORD_MODIFIERS);
    OpWriteX(memory, cpu, OpDp(cpu, DP_DEFAULT_RECORD_MODIFIERS), cpu->x);
    OpLoadA(cpu, DEFAULT_RECORD_BUFFER_BANK);
    OpSta(memory, cpu, OpDp(cpu, DP_DEFAULT_RECORD_MODIFIERS + 2u));
}

static void InitializeRecordBuffers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SetDefaultRecordPointers(memory, cpu);
    OpLdx(cpu, DEFAULT_RECORD_CLEAR_FIRST);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_PORT_ADDRESS), cpu->x);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_PORT_ADDRESS_BANK));
    OpLdx(cpu, DEFAULT_RECORD_CLEAR_SIZE);
    do {
        OpStz(memory, cpu, OpAbs(cpu, WRAM_PORT_DATA));
        OpDex(cpu);
    } while (!cpu->zero);
    OpLdx(cpu, DEFAULT_CAPSULE_RECORD);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_CHARACTER_RECORD), cpu->x);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E0A7F));
}

static void CopyDefaultPartyIds(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, DEFAULT_RECORD_DATA);
    OpLdy(cpu, WRAM_MENU_PARTY_MEMBER_COUNT);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        OpSta(memory, cpu, OpAbsY(cpu, 0u));
        OpInx(cpu);
        OpIny(cpu);
        OpCpy(cpu, WRAM_UNK_7E0A7F);
    } while (!cpu->zero);
}

static void CopyDefaultInventory(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, 0u));
    OpInx(cpu);
    OpInx(cpu);
    OpLdy(cpu, WRAM_GOLD);
    OpSta(memory, cpu, OpAbsY(cpu, 0u));
    OpLda(memory, cpu, OpAbsX(cpu, 0u));
    OpInx(cpu);
    OpInx(cpu);
    OpLdy(cpu, DEFAULT_RECORD_UNK_0B55);
    OpSta(memory, cpu, OpAbsY(cpu, 0u));
    OpLdy(cpu, WRAM_INVENTORY_PACKED_ITEMS);
    for (;;) {
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        if (cpu->zero)
            break;
        OpInx(cpu);
        OpInx(cpu);
        OpSta(memory, cpu, OpAbsY(cpu, 0u));
        OpIny(cpu);
        OpIny(cpu);
    }
    OpInx(cpu);
    OpInx(cpu);
}

Lufia2ExecutionResult Lufia2InitializeDefaultRecords(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x81ec56u);
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, DEFAULT_RECORD_DATA_BANK);
    InitializeRecordBuffers(memory, cpu);
    CopyDefaultPartyIds(memory, cpu);
    PushIndex(memory, cpu);
    OpLdx(cpu, 0u);
    OpTxy(cpu);
    OpRepWidths(cpu, 0x20u);
    do {
        OpLda(memory, cpu, OpAbsY(cpu, WRAM_MENU_PARTY_FIRST_ID));
        OpAndValue(cpu, 0xffu);
        OpCmpValue(cpu, 0xffu);
        if (!cpu->zero) {
            PushIndex(memory, cpu);
            OpTax(cpu);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x81ecb1u, 0x81f7bdu, 2u, cpu->program_bank))
                return DefaultRecordChildUnwound(0x81ecb1u);
            OpTxa(cpu);
            OpPullX(memory, cpu);
            OpSta(memory, cpu, OpAbsX(cpu, WRAM_MENU_PARTY_CHARACTER_OFFSET));
        }
        OpIny(cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpCpy(cpu, DEFAULT_PARTY_SLOT_COUNT);
    } while (!cpu->zero);
    OpPullX(memory, cpu);
    CopyDefaultInventory(memory, cpu);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_PARTY_STAT_MEMBER_INDEX));
    const uint32_t unpack_sites[] = {
        0x81ecf3u, 0x81ecf9u, 0x81ecffu, 0x81ed05u,
        0x81ed0bu, 0x81ed11u, 0x81ed17u};
    for (unsigned member = 0u; member < DEFAULT_PARTY_RECORD_COUNT; ++member) {
        OpLdy(cpu, (uint16_t)(DEFAULT_PARTY_RECORD_FIRST + member * DEFAULT_PARTY_RECORD_SIZE));
        if (!CallChildWithFrame(memory, cpu, child, context,
                unpack_sites[member], 0x81ed8eu, 2u, cpu->program_bank))
            return DefaultRecordChildUnwound(unpack_sites[member]);
    }
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E0A7F));
    PullDataBank(memory, cpu);
    OpLdx(cpu, 1u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID), cpu->x);
    const uint32_t service_sites[] = {0x81ed28u, 0x81ed2cu, 0x81ed30u};
    const uint32_t service_targets[] = {0x81f1c5u, 0x82994eu, 0x82c261u};
    for (unsigned service = 0u; service < 3u; ++service)
        if (!CallChildWithFrame(memory, cpu, child, context,
                service_sites[service], service_targets[service], 3u, cpu->program_bank))
            return DefaultRecordChildUnwound(service_sites[service]);
    return ExecutionReturned(0x81ed34u);
}

Lufia2ExecutionResult Lufia2PartyResetDefaultRecords(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x81ed35u);
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, DEFAULT_RECORD_DATA_BANK);
    SetDefaultRecordPointers(memory, cpu);
    OpLdx(cpu, 0xb395u);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_PARTY_STAT_MEMBER_INDEX));
    const uint32_t unpack_sites[] = {
        0x81ed55u, 0x81ed5bu, 0x81ed61u, 0x81ed67u,
        0x81ed6du, 0x81ed73u, 0x81ed79u};
    for (unsigned member = 0u; member < DEFAULT_PARTY_RECORD_COUNT; ++member) {
        OpLdy(cpu, (uint16_t)(DEFAULT_PARTY_RECORD_FIRST + member * DEFAULT_PARTY_RECORD_SIZE));
        if (!CallChildWithFrame(memory, cpu, child, context,
                unpack_sites[member], 0x81ee94u, 2u, cpu->program_bank))
            return DefaultRecordChildUnwound(unpack_sites[member]);
    }
    OpSepWidths(cpu, 0x20u);
    PullDataBank(memory, cpu);
    OpLdx(cpu, 1u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID), cpu->x);
    const uint32_t service_sites[] = {0x81ed85u, 0x81ed89u};
    const uint32_t service_targets[] = {0x81f1c5u, 0x82994eu};
    for (unsigned service = 0u; service < 2u; ++service)
        if (!CallChildWithFrame(memory, cpu, child, context,
                service_sites[service], service_targets[service], 3u, cpu->program_bank))
            return DefaultRecordChildUnwound(service_sites[service]);
    return ExecutionReturned(0x81ed8du);
}

Lufia2ExecutionResult Lufia2StartDefaultRecords(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x81ec35u);
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x81ec39u, 0x81ec41u, 2u, cpu->program_bank))
        return DefaultRecordChildUnwound(0x81ec39u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x81ec3cu, 0x81ec56u, 3u, cpu->program_bank))
        return DefaultRecordChildUnwound(0x81ec3cu);
    return ExecutionReturned(0x81ec40u);
}
