#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/menu.h"
#include "lufia2/menu_inventory_groups.h"
#include "system/wram.h"

enum {
    SORT_OUTPUT_OFFSET = 0x00u,
    SORT_SECOND_COUNT = 0x02u,
    SORT_COPY_COUNT = 0x04u,
    SORT_SOURCE_POINTER = 0x08u,
    SORT_SOURCE_BANK = 0x0au,
    SORT_MEMBER_SLOT = 0x22u,
    SORT_SPELL_POINTER = 0x2au,
    SORT_BUCKET_COUNT = 12u,
    SORT_INVENTORY_BYTES = 192u,
    SORT_SPELL_SLOTS = 36u,
    SORT_GROUP_ITEMS = WRAM_UNK_7E93C0,
    SORT_SECOND_SPELLS = WRAM_UNK_7E93C0 + 36u,
    SORT_EQUIPMENT_ITEMS = WRAM_UNK_7E93C0 + 0x308u,
    SORT_GROUP_LENGTHS = WRAM_UNK_7E93C0 + 0x918u,
    SORT_EQUIPMENT_LENGTH = WRAM_UNK_7E93C0 + 0x920u,
    SORT_GROUP_POINTERS = 0x8ee52eu,
    SORT_COUNT_POINTERS = 0x8ee546u,
    SORT_ITEM_MASKS = 0x8ee55eu,
    SORT_EQUIPMENT_MASKS = 0x8ee568u,
    SORT_MEMBER_LISTS = 0x8ed8bbu,
    SORT_MEMBER_SKIP = 0x04u,
    SORT_PREVIOUS_MEMBER = 9u,
    SORT_FIRST_SPELL_GROUP = 0x40u,
    EQUIPMENT_GROUP_COUNT = 6u,
    EQUIPMENT_LENGTH_COUNT = 7u,
    EQUIPMENT_SOURCE_OFFSET = SORT_SOURCE_POINTER
};

typedef struct MenuSortCall {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
} MenuSortCall;

static bool MenuSortReady(const Lufia2CpuState *cpu, Lufia2PushedChildCall child) {
    return child && cpu->program_bank == 0x82u && cpu->accumulator_is_8_bit &&
           !cpu->index_is_8_bit && !cpu->direct_page && !cpu->decimal &&
           cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static bool MenuSortChild(MenuSortCall *call, uint32_t site, uint32_t target,
                          uint8_t frame, uint8_t accumulator_width) {
    Lufia2CpuState *cpu = call->cpu;
    if (!CallChildWithFrame(call->memory, cpu, call->child, call->context, site, target,
                            frame, 0x82u)) {
        call->result =
            (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return false;
    }
    if (cpu->program_bank != 0x82u || cpu->accumulator_is_8_bit != accumulator_width ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal) {
        call->result = ExecutionHandoff(cpu, site + frame + 1u);
        return false;
    }
    return true;
}

static void GroupInventoryRecord(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                 uint32_t masks, uint32_t counts, uint32_t groups,
                                 bool inventory_offset) {
    OpAndValue(cpu, 0x00ffu);
    OpSta(memory, cpu, OpDp(cpu, SORT_OUTPUT_OFFSET));
    PushDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0u);
    for (;;) {
        OpLda(memory, cpu, OpLongX(cpu, masks));
        if (cpu->zero)
            break;
        OpBit(memory, cpu, OpDp(cpu, SORT_OUTPUT_OFFSET));
        if (!cpu->zero)
            break;
        OpInx(cpu);
        OpInx(cpu);
    }
    OpLda(memory, cpu, OpLongX(cpu, counts));
    OpTay(cpu);
    OpLda(memory, cpu, OpAbsY(cpu, 0u));
    OpSta(memory, cpu, OpDp(cpu, SORT_OUTPUT_OFFSET));
    OpIncA(cpu);
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, 0u));
    OpLda(memory, cpu, OpLongX(cpu, groups));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, SORT_OUTPUT_OFFSET));
    OpTay(cpu);
    OpLda(memory, cpu, inventory_offset
        ? OpDp(cpu, EQUIPMENT_SOURCE_OFFSET)
        : OpAbs(cpu, WRAM_ITEM_RECORD_ID));
    OpSta(memory, cpu, OpAbsY(cpu, 0u));
    PullDataBank(memory, cpu);
}

static void GatherInventoryGroups(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpStz(memory, cpu, OpDp(cpu, SORT_OUTPUT_OFFSET));
    OpLdx(cpu, 0u);
    do {
        OpPushX(memory, cpu);
        OpLda(memory, cpu, OpLongX(cpu, SORT_COUNT_POINTERS));
        OpSta(memory, cpu, OpDp(cpu, SORT_SOURCE_POINTER));
        OpLda(memory, cpu, DirectLongPointer(memory, cpu, SORT_SOURCE_POINTER));
        if (!cpu->zero) {
            OpLsrA(cpu);
            OpSta(memory, cpu, OpDp(cpu, SORT_COPY_COUNT));
            OpLda(memory, cpu, OpLongX(cpu, SORT_GROUP_POINTERS));
            OpSta(memory, cpu, OpDp(cpu, SORT_SOURCE_POINTER));
            OpLdy(cpu, 0u);
            do {
                OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, SORT_OUTPUT_OFFSET)));
                OpLda(memory, cpu,
                      DirectLongIndirectY(memory, cpu, SORT_SOURCE_POINTER));
                OpSta(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
                OpStepMem(memory, cpu, OpDp(cpu, SORT_OUTPUT_OFFSET), 1);
                OpStepMem(memory, cpu, OpDp(cpu, SORT_OUTPUT_OFFSET), 1);
                OpIny(cpu);
                OpIny(cpu);
                OpStepMem(memory, cpu, OpDp(cpu, SORT_COPY_COUNT), -1);
            } while (!cpu->zero);
        }
        OpPullX(memory, cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, SORT_BUCKET_COUNT * 2u);
    } while (!cpu->zero);
}

Lufia2ExecutionResult Lufia2MenuSortInventory(const Lufia2Memory *memory,
                                              Lufia2CpuState *cpu,
                                              Lufia2PushedChildCall child,
                                              void *context) {
    if (!MenuSortReady(cpu, child))
        return ExecutionHandoff(cpu, 0x82a9b5u);
    MenuSortCall call = {memory, cpu, child, context, {0}};
    OpLoadA(cpu, 5u);
    OpLdx(cpu, 7u);
    if (!MenuSortChild(&call, 0x82a9bau, 0x82895bu, 2u, 1u) ||
        !MenuSortChild(&call, 0x82a9bdu, 0x868b55u, 3u, 1u))
        return call.result;
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, SORT_SOURCE_BANK));
    OpRepWidths(cpu, 0x20u);
    PushDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpRepWidths(cpu, 0x20u);
    for (unsigned bucket = 0u; bucket < SORT_BUCKET_COUNT; ++bucket)
        OpStz(memory, cpu, OpAbs(cpu, (uint16_t)(SORT_GROUP_LENGTHS + bucket * 2u)));
    PullDataBank(memory, cpu);
    OpLdy(cpu, 0u);
    do {
        PushY(memory, cpu);
        OpLda(memory, cpu, OpAbsY(cpu, WRAM_INVENTORY_PACKED_ITEMS));
        if (!cpu->zero) {
            OpSta(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
            OpLoadA(cpu, 0u);
            OpSta(memory, cpu, OpAbsY(cpu, WRAM_INVENTORY_PACKED_ITEMS));
            if (!MenuSortChild(&call, 0x82aa07u, 0x81f194u, 3u, 0u))
                return call.result;
            GroupInventoryRecord(memory, cpu, SORT_ITEM_MASKS, SORT_COUNT_POINTERS,
                                 SORT_GROUP_POINTERS, false);
        }
        OpPullY(memory, cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, SORT_INVENTORY_BYTES);
    } while (!cpu->zero);
    OpLda(memory, cpu, SORT_EQUIPMENT_LENGTH);
    OpSta(memory, cpu, OpDp(cpu, SORT_COPY_COUNT));
    if (!cpu->zero) {
        OpLoadA(cpu, 0u);
        OpSta(memory, cpu, SORT_EQUIPMENT_LENGTH);
        OpLdx(cpu, 0u);
        do {
            OpPushX(memory, cpu);
            OpLda(memory, cpu, OpLongX(cpu, SORT_EQUIPMENT_ITEMS));
            OpSta(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
            if (!MenuSortChild(&call, 0x82aa6au, 0x81f1abu, 3u, 0u))
                return call.result;
            OpLda(memory, cpu, OpAbs(cpu, WRAM_RECORD_BUFFER + 0x14u));
            GroupInventoryRecord(memory, cpu, SORT_EQUIPMENT_MASKS,
                                 SORT_COUNT_POINTERS + 10u, SORT_GROUP_POINTERS + 10u, false);
            OpPullX(memory, cpu);
            OpInx(cpu);
            OpInx(cpu);
            OpCpx(cpu, OpReadX(memory, cpu, OpDp(cpu, SORT_COPY_COUNT)));
        } while (!cpu->zero);
    }
    GatherInventoryGroups(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    if (!MenuSortChild(&call, 0x82aaecu, 0x868b55u, 3u, 1u))
        return call.result;
    OpLoadA(cpu, 3u);
    OpLdx(cpu, 7u);
    OpLdy(cpu, 7u);
    if (!MenuSortChild(&call, 0x82aaf8u, 0x8289bcu, 2u, 1u) ||
        !MenuSortChild(&call, 0x82aafbu, 0x82ac84u, 2u, 1u))
        return call.result;
    return ExecutionReturned(0x82aafeu);
}

static void CopySpellGroup(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                           uint8_t count, uint32_t source) {
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, count)));
    if (cpu->zero)
        return;
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpLongX(cpu, source));
        OpSta(memory, cpu,
              AbsoluteIndexedAddress(cpu, Read16Direct(memory, cpu, SORT_SPELL_POINTER),
                                     cpu->y));
        OpIny(cpu);
        OpInx(cpu);
        OpCpx(cpu, OpReadX(memory, cpu, OpDp(cpu, count)));
    } while (!cpu->zero);
}

Lufia2ExecutionResult Lufia2MenuSortSpells(const Lufia2Memory *memory,
                                           Lufia2CpuState *cpu,
                                           Lufia2PushedChildCall child, void *context) {
    if (!MenuSortReady(cpu, child))
        return ExecutionHandoff(cpu, 0x82b3adu);
    MenuSortCall call = {memory, cpu, child, context, {0}};
    OpLoadA(cpu, 5u);
    OpLdx(cpu, 7u);
    if (!MenuSortChild(&call, 0x82b3b2u, 0x82895bu, 2u, 1u) ||
        !MenuSortChild(&call, 0x82b3b5u, 0x868b55u, 3u, 1u))
        return call.result;
    OpLdy(cpu, 0u);
    OpWriteX(memory, cpu, OpDp(cpu, SORT_OUTPUT_OFFSET), cpu->y);
    OpWriteX(memory, cpu, OpDp(cpu, SORT_SECOND_COUNT), cpu->y);
    do {
        PushY(memory, cpu);
        OpLda(memory, cpu,
              AbsoluteIndexedAddress(cpu, Read16Direct(memory, cpu, SORT_SPELL_POINTER),
                                     cpu->y));
        OpCmpValue(cpu, 0xffu);
        if (!cpu->zero) {
            OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_RECORD_ID));
            OpLoadA(cpu, 0xffu);
            OpSta(memory, cpu,
                  AbsoluteIndexedAddress(
                      cpu, Read16Direct(memory, cpu, SORT_SPELL_POINTER), cpu->y));
            if (!MenuSortChild(&call, 0x82b3ceu, 0x81f404u, 3u, 1u))
                return call.result;
            OpBitValue(cpu, SORT_FIRST_SPELL_GROUP);
            uint8_t count = cpu->zero ? SORT_SECOND_COUNT : SORT_OUTPUT_OFFSET;
            uint32_t source = cpu->zero ? SORT_SECOND_SPELLS : SORT_GROUP_ITEMS;
            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, count)));
            OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_RECORD_ID));
            OpSta(memory, cpu, OpLongX(cpu, source));
            OpStepMem(memory, cpu, OpDp(cpu, count), 1);
        }
        OpPullY(memory, cpu);
        OpIny(cpu);
        OpCpy(cpu, SORT_SPELL_SLOTS);
    } while (!cpu->zero);
    OpLdy(cpu, 0u);
    CopySpellGroup(memory, cpu, SORT_OUTPUT_OFFSET, SORT_GROUP_ITEMS);
    CopySpellGroup(memory, cpu, SORT_SECOND_COUNT, SORT_SECOND_SPELLS);
    OpLoadA(cpu, 3u);
    OpLdx(cpu, 7u);
    OpLdy(cpu, 7u);
    if (!MenuSortChild(&call, 0x82b426u, 0x8289bcu, 2u, 1u) ||
        !MenuSortChild(&call, 0x82b429u, 0x82ac84u, 2u, 1u))
        return call.result;
    return ExecutionReturned(0x82b42cu);
}

Lufia2ExecutionResult Lufia2MenuPrepareSpellMember(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    if (!MenuSortReady(cpu, child))
        return ExecutionHandoff(cpu, 0x82b5edu);
    MenuSortCall call = {memory, cpu, child, context, {0}};
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_MEMBER));
    OpSta(memory, cpu, OpDp(cpu, SORT_MEMBER_SLOT));
    if (!MenuSortChild(&call, 0x82b5f4u, 0x829971u, 2u, 0u))
        return call.result;
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, SORT_SPELL_POINTER)));
    OpLda(memory, cpu, OpAbsY(cpu, 0x13u));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_MEMBER_MP));
    OpLda(memory, cpu, OpDp(cpu, SORT_SPELL_POINTER));
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x96u);
    OpSta(memory, cpu, OpDp(cpu, SORT_SPELL_POINTER));
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82b609u);
}

static void SelectAdjacentSpellMember(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    do {
        OpLda(memory, cpu, OpDp(cpu, SORT_OUTPUT_OFFSET));
        OpCmpValue(cpu, SORT_PREVIOUS_MEMBER);
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_MEMBER));
            OpDecA(cpu);
            OpCmpValue(cpu, 0xffu);
            if (cpu->zero) {
                OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PARTY_MEMBER_COUNT));
                OpDecA(cpu);
            }
        } else {
            OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_MEMBER));
            OpIncA(cpu);
            OpCmp(memory, cpu, OpAbs(cpu, WRAM_MENU_PARTY_MEMBER_COUNT));
            if (cpu->zero)
                OpLoadA(cpu, 0u);
        }
        OpRepWidths(cpu, 0x20u);
        OpAndValue(cpu, 0x00ffu);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_MEMBER));
        OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_INDEX));
        OpAslA(cpu);
        OpTax(cpu);
        OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, WRAM_MENU_PARTY_CHARACTER_OFFSET)));
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbsY(cpu, 0x0fu));
        OpBitValue(cpu, SORT_MEMBER_SKIP);
    } while (!cpu->zero);
}

Lufia2ExecutionResult Lufia2MenuSelectAdjacentSpellMember(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu,
                                                          Lufia2PushedChildCall child,
                                                          void *context) {
    if (!MenuSortReady(cpu, child))
        return ExecutionHandoff(cpu, 0x82b60au);
    MenuSortCall call = {memory, cpu, child, context, {0}};
    OpSta(memory, cpu, OpDp(cpu, SORT_OUTPUT_OFFSET));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_MODE));
    OpCmpValue(cpu, 2u);
    if (!cpu->zero)
        return ExecutionReturned(0x82b67bu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PARTY_MEMBER_COUNT));
    OpCmpValue(cpu, 1u);
    if (cpu->zero)
        return ExecutionReturned(0x82b67bu);
    SelectAdjacentSpellMember(memory, cpu);
    OpLoadA(cpu, 0u);
    if (!MenuSortChild(&call, 0x82b654u, 0x80953bu, 3u, 1u))
        return call.result;
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x0082u);
    OpLdx(cpu, 0x1e0au);
    OpLdy(cpu, 3u);
    if (!MenuSortChild(&call, 0x82b663u, 0x8280cau, 2u, 1u))
        return call.result;
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_INDEX)));
    OpLda(memory, cpu, OpLongX(cpu, SORT_MEMBER_LISTS));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_LIST_ID));
    if (!MenuSortChild(&call, 0x82b672u, 0x82b5edu, 2u, 1u) ||
        !MenuSortChild(&call, 0x82b675u, 0x829984u, 2u, 1u) ||
        !MenuSortChild(&call, 0x82b678u, 0x82ac84u, 2u, 1u))
        return call.result;
    return ExecutionReturned(0x82b67bu);
}

static bool EquipmentGroupReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x82u && !cpu->index_is_8_bit &&
        !cpu->direct_page && !cpu->decimal &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

Lufia2ExecutionResult Lufia2MenuGroupEquipmentOffset(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EquipmentGroupReady(cpu) || cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x82bd9bu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_RECORD_BUFFER + 0x14u));
    GroupInventoryRecord(memory, cpu, SORT_EQUIPMENT_MASKS,
        SORT_COUNT_POINTERS + 10u, SORT_GROUP_POINTERS + 10u, true);
    return ExecutionReturned(0x82bddau);
}

Lufia2ExecutionResult Lufia2MenuFinishEquipmentGroups(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EquipmentGroupReady(cpu))
        return ExecutionHandoff(cpu, 0x82bddbu);
    PushDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpLongX(cpu, SORT_COUNT_POINTERS + 10u));
        OpTay(cpu);
        OpLda(memory, cpu, OpAbsY(cpu, 0u));
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpLongX(cpu, SORT_GROUP_POINTERS + 10u));
        OpTay(cpu);
        OpLoadA(cpu, 0xffffu);
        OpSta(memory, cpu, OpAbsY(cpu, 0u));
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, EQUIPMENT_GROUP_COUNT * 2u);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x82be03u);
}

static void ReadEquipmentInventoryEntry(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpPushX(memory, cpu);
    OpWriteX(memory, cpu, OpDp(cpu, EQUIPMENT_SOURCE_OFFSET), cpu->x);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
}

static void AdvanceEquipmentInventoryIndex(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpPullX(memory, cpu);
    OpInx(cpu);
    OpInx(cpu);
    OpCpx(cpu, SORT_INVENTORY_BYTES);
}

static bool EquipmentInventoryStepReady(const Lufia2CpuState *cpu) {
    const uint8_t bank = cpu->data_bank & 0x7fu;
    return EquipmentGroupReady(cpu) && !cpu->accumulator_is_8_bit &&
        cpu->stack >= 0x1f10u && (bank < 0x40u || cpu->data_bank == 0x7eu);
}

Lufia2ExecutionResult Lufia2MenuReadEquipmentInventoryEntry(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EquipmentInventoryStepReady(cpu) || cpu->x >= SORT_INVENTORY_BYTES)
        return ExecutionHandoff(cpu, 0x82bd70u);
    ReadEquipmentInventoryEntry(memory, cpu);
    return ExecutionHandoff(cpu, 0x82bd76u);
}

Lufia2ExecutionResult Lufia2MenuAdvanceEquipmentInventoryIndex(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EquipmentInventoryStepReady(cpu))
        return ExecutionHandoff(cpu, 0x82bd8eu);
    AdvanceEquipmentInventoryIndex(memory, cpu);
    return ExecutionHandoff(cpu, 0x82bd94u);
}

Lufia2ExecutionResult Lufia2MenuBuildEquipmentGroups(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->program_bank != 0x82u || cpu->direct_page ||
        cpu->decimal || cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x82bd48u);
    MenuSortCall call = {memory, cpu, child, context, {0}};
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    PushDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpRepWidths(cpu, 0x20u);
    for (unsigned group = 0u; group < EQUIPMENT_LENGTH_COUNT; ++group)
        OpStz(memory, cpu,
            OpAbs(cpu, (uint16_t)(SORT_GROUP_LENGTHS + 10u + group * 2u)));
    PullDataBank(memory, cpu);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_ITEM_QUANTITY));
    OpLdx(cpu, 0u);
    do {
        ReadEquipmentInventoryEntry(memory, cpu);
        if (!cpu->zero) {
            OpStepMem(memory, cpu, OpAbs(cpu, WRAM_MENU_ITEM_QUANTITY), 1);
            OpSta(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
            if (!MenuSortChild(&call, 0x82bd7eu, 0x81f194u, 3u, 0u))
                return call.result;
            OpAndValue(cpu, 2u);
            if (!cpu->zero) {
                if (!MenuSortChild(&call, 0x82bd87u, 0x81f1abu, 3u, 0u) ||
                    !MenuSortChild(&call, 0x82bd8bu, 0x82bd9bu, 2u, 0u))
                    return call.result;
            }
        }
        AdvanceEquipmentInventoryIndex(memory, cpu);
    } while (!cpu->zero);
    if (!MenuSortChild(&call, 0x82bd96u, 0x82bddbu, 2u, 0u))
        return call.result;
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x82bd9au);
}
