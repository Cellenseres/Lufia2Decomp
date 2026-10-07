#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    DP_CONTACT_DIRECTION = 0x54u,
    DP_CONTACT_WIDTH = 0x55u,
    DP_CONTACT_HEIGHT = 0x56u,
    DP_CONTACT_ACTOR = 0x65u,
    DP_CONTACT_X = 0x8fu,
    DP_CONTACT_Y = 0x91u,
    DP_CONTACT_BUTTONS = 0x47u,
    ACTOR_FIRST_CONTACT_SLOT = 8u,
    ACTOR_CONTACT_SLOT_LIMIT = 40u,
    ROM_CONTACT_FACING = 0x83b9feu,
    ROM_PLAYER_CONTACT_FACING = 0x83b9f6u,
    WRAM_CONTACT_CLASS = WRAM_FIELD_CONTACT_CLASS,
    WRAM_CONTACT_MODE = WRAM_FIELD_BATTLE_SOURCE,
    WRAM_CONTACT_RESOURCE = WRAM_FIELD_CONTACT_RESOURCE
};

static Lufia2ExecutionResult ContactUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static uint8_t ContactActorAvailable(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
    OpBitValue(cpu, 4u);
    if (!cpu->zero)
        return 0u;
    OpBitValue(cpu, 0x80u);
    if (!cpu->zero)
        return 0u;
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_FLAGS));
    OpBitValue(cpu, 0x14u);
    if (!cpu->zero)
        return 0u;
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_ID));
    OpCmpValue(cpu, 0xfdu);
    return !cpu->zero;
}

static uint8_t ContactActorOverlaps(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpLongX(cpu, WRAM_UNK_7FE216));
    OpAndValue(cpu, 2u);
    OpLsrA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_CONTACT_WIDTH));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_Y));
    OpCmp(memory, cpu, OpDp(cpu, DP_CONTACT_Y));
    if (!cpu->zero) {
        OpDecA(cpu);
        OpDecA(cpu);
        OpCmp(memory, cpu, OpDp(cpu, DP_CONTACT_Y));
        if (cpu->carry)
            return 0u;
        OpIncA(cpu);
        OpIncA(cpu);
        OpIncA(cpu);
        OpCmp(memory, cpu, OpDp(cpu, DP_CONTACT_Y));
        if (!cpu->carry)
            return 0u;
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_X));
        OpCmp(memory, cpu, OpDp(cpu, DP_CONTACT_X));
        if (cpu->zero)
            return 1u;
        cpu->carry = 0u;
        OpAdcValue(cpu, Read8(memory, DirectAddress(cpu, DP_CONTACT_WIDTH)));
        OpCmp(memory, cpu, OpDp(cpu, DP_CONTACT_X));
        return cpu->zero;
    }
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_X));
    OpDecA(cpu);
    OpDecA(cpu);
    OpCmp(memory, cpu, OpDp(cpu, DP_CONTACT_X));
    if (cpu->carry)
        return 0u;
    OpIncA(cpu);
    OpIncA(cpu);
    OpIncA(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, Read8(memory, DirectAddress(cpu, DP_CONTACT_WIDTH)));
    OpCmp(memory, cpu, OpDp(cpu, DP_CONTACT_X));
    return cpu->carry;
}

static uint8_t ContactProbeDirection(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_Y));
    OpCmp(memory, cpu, OpDp(cpu, DP_CONTACT_Y));
    if (!cpu->zero) {
        LoadX16(cpu, 0u);
        if (!cpu->carry)
            LoadX16(cpu, 4u);
        return 1u;
    }
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_X));
    OpCmp(memory, cpu, OpDp(cpu, DP_CONTACT_X));
    if (cpu->zero)
        return 0u;
    LoadX16(cpu, 6u);
    if (!cpu->carry)
        LoadX16(cpu, 2u);
    return 1u;
}

static Lufia2ExecutionResult PublishActorContact(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    OpLdx(cpu, Read16Direct(memory, cpu, DP_CONTACT_ACTOR));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_Y));
    OpCmp(memory, cpu, OpAbs(cpu, WRAM_ACTOR_TILE_Y));
    if (!cpu->zero) {
        OpLoadA(cpu, 0u);
        if (!cpu->carry)
            OpLoadA(cpu, 4u);
    } else {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_X));
        OpCmp(memory, cpu, OpAbs(cpu, WRAM_ACTOR_TILE_X));
        OpLoadA(cpu, 6u);
        if (!cpu->carry)
            OpLoadA(cpu, 2u);
    }
    OpSta(memory, cpu, OpDp(cpu, DP_CONTACT_DIRECTION));
    OpSepWidths(cpu, 0x10u);
    cpu->carry = 1u;
    OpSbcValue(cpu, Read8(memory,
        AbsoluteIndexedAddress(cpu, WRAM_ACTOR_FACING, cpu->x)));
    cpu->carry = 0u;
    OpAdcValue(cpu, 4u);
    OpAndValue(cpu, 6u);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, ROM_CONTACT_FACING));
    OpSta(memory, cpu, OpDp(cpu, DP_CONTACT_WIDTH));
    OpLdx(cpu, Read8(memory, DirectAddress(cpu, DP_CONTACT_ACTOR)));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E1291));
    OpBitValue(cpu, 0x10u);
    if (cpu->zero) {
        OpLoadA(cpu, 0xffu);
    } else {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_FACING));
        cpu->carry = 1u;
        OpSbcValue(cpu, Read8(memory, DirectAddress(cpu, DP_CONTACT_DIRECTION)));
        OpAndValue(cpu, 6u);
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, ROM_PLAYER_CONTACT_FACING));
        cpu->carry = 0u;
        OpAdcValue(cpu, Read8(memory, DirectAddress(cpu, DP_CONTACT_WIDTH)));
    }
    OpSta(memory, cpu, WRAM_CONTACT_CLASS);
    OpRepWidths(cpu, 0x10u);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, WRAM_CONTACT_MODE);
    OpLda(memory, cpu, OpDp(cpu, DP_CONTACT_ACTOR));
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_ID));
    cpu->carry = 1u;
    OpSbcValue(cpu, 0x50u);
    OpSta(memory, cpu, WRAM_CONTACT_RESOURCE);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x83b9f0u, 0x8383ebu, 3u, 0x83u))
        return ContactUnwound(0x83b9f0u);
    cpu->carry = 1u;
    return ExecutionReturned(0x83b9f5u);
}

Lufia2ExecutionResult Lufia2FieldProbeActorContact(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x83u || !cpu->accumulator_is_8_bit ||
        cpu->decimal || !child)
        return ExecutionHandoff(cpu, 0x83b8bfu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E057C));
    if (!cpu->zero) {
        OpLoadA(cpu, 0x80u);
        OpAndValue(cpu, Read8(memory, DirectAddress(cpu, DP_CONTACT_BUTTONS)));
        if (!cpu->zero)
            return ExecutionReturned(0x83b8cau);
    }
    OpRepWidths(cpu, 0x10u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_TILE_X));
    OpSta(memory, cpu, OpDp(cpu, DP_CONTACT_X));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_TILE_Y));
    OpSta(memory, cpu, OpDp(cpu, DP_CONTACT_Y));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x83b8d7u, 0x83f988u, 2u, 0x83u))
        return ContactUnwound(0x83b8d7u);
    OpSta(memory, cpu, OpDp(cpu, DP_CONTACT_HEIGHT));
    LoadX16(cpu, ACTOR_FIRST_CONTACT_SLOT);
    do {
        if (ContactActorAvailable(memory, cpu) && ContactActorOverlaps(memory, cpu)) {
            Write16Direct(memory, cpu, DP_CONTACT_ACTOR, cpu->x);
            uint8_t blocked = 0u;
            if (ContactProbeDirection(memory, cpu)) {
                if (!CallChildWithFrame(memory, cpu, child, context,
                        0x83b964u, 0x83d927u, 2u, 0x83u))
                    return ContactUnwound(0x83b964u);
                blocked = !cpu->zero;
                if (blocked)
                    OpLdx(cpu, Read16Direct(memory, cpu, DP_CONTACT_ACTOR));
            }
            if (!blocked) {
                OpLdx(cpu, Read16Direct(memory, cpu, DP_CONTACT_ACTOR));
                OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_FLAGS));
                OpBitValue(cpu, 1u);
                if (cpu->zero) {
                    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_ID));
                    OpSta(memory, cpu, OpDp(cpu, DP_CONTACT_DIRECTION));
                    if (!CallChildWithFrame(memory, cpu, child, context,
                            0x83b97bu, 0x80bfe7u, 3u, 0x83u))
                        return ContactUnwound(0x83b97bu);
                    OpLdx(cpu, OpRead16(memory, OpAbs(cpu, WRAM_UNK_7E09B7)));
                    OpCpx(cpu, 0xffffu);
                    if (!cpu->zero) {
                        cpu->carry = 1u;
                        return ExecutionReturned(0x83b988u);
                    }
                }
                return PublishActorContact(memory, cpu, child, context);
            }
        }
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, ACTOR_CONTACT_SLOT_LIMIT);
    } while (!cpu->zero);
    cpu->carry = 0u;
    return ExecutionReturned(0x83b941u);
}
