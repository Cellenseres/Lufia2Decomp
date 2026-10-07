#include "actor/actor_internal.h"
#include "core/cpu_ops.h"
#include "lufia2/actor.h"
#include "lufia2/field.h"
#include "system/dp_scratch.h"
#include "system/wram.h"

enum { OBJECT_SLOT_COUNT = 32, ROM_SPRITE_SLOT_COUNTS = 0x83abf4u };

static bool ObjectReleaseContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x83u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && !cpu->direct_page &&
        cpu->stack >= 0x1f04u && cpu->stack <= 0x1ffcu;
}

static bool ObjectPositionMatches(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpLongX(cpu, WRAM_OBJECT_STATE));
    OpBitValue(cpu, 4u);
    if (!cpu->zero)
        return false;
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_OBJECT_FLAGS & 0xffffu));
    OpBitValue(cpu, 0x80u);
    if (cpu->zero)
        return false;
    OpLda(memory, cpu, OpAbsY(cpu, WRAM_OBJECT_FINE_X & 0xffffu));
    OpCmp(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    if (!cpu->zero)
        return false;
    OpLda(memory, cpu, OpAbsY(cpu, WRAM_OBJECT_FINE_Y & 0xffffu));
    OpCmp(memory, cpu, OpDp(cpu, DP_SCRATCH_C));
    return cpu->zero != 0;
}

static void ObjectWakePositionMatch(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_UNK_7FDFAE & 0xffffu));
    OpAndValue(cpu, 0xff00u);
    OpOraValue(cpu, 1u);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7FDFAE & 0xffffu));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_OBJECT_STATE));
    OpOraValue(cpu, 0x40u);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_STATE));
    cpu->carry = 1;
}

Lufia2ExecutionResult Lufia2ObjectWakeMatchingPosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ObjectReleaseContext(cpu))
        return ExecutionHandoff(cpu, 0x83ef6eu);
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    OpLoadA(cpu, 0x7fu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpRepWidths(cpu, 0x30u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SLOT_WORD_OFFSET)));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_OBJECT_FINE_X));
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_OBJECT_FINE_Y));
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_C));
    OpLdy(cpu, 0u);
    OpLdx(cpu, 0u);
    bool matched = false;
    do {
        if (ObjectPositionMatches(memory, cpu)) {
            ObjectWakePositionMatch(memory, cpu);
            matched = true;
            break;
        }
        OpInx(cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, OBJECT_SLOT_COUNT * 2u);
    } while (!cpu->zero);
    if (!matched)
        cpu->carry = 0;
    OpSepWidths(cpu, 0x20u);
    PullDataBank(memory, cpu);
    OpPullY(memory, cpu);
    return ExecutionReturned(0x83efd0u);
}

static bool ObjectReleaseAllocation(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SimulateJslFrame(memory, cpu, 0x83u, 0xf237u);
    Lufia2ExecutionResult result = Lufia2SpriteReleaseAllocation(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return false;
    uint8_t low = Pull8(memory, cpu);
    uint8_t high = Pull8(memory, cpu);
    uint8_t bank = Pull8(memory, cpu);
    uint16_t back = (uint16_t)(low | ((uint16_t)high << 8));
    cpu->program_bank = bank;
    if (back == 0xf237u && bank == 0x83u)
        return true;
    cpu->resume_pc = ((uint32_t)bank << 16) | (uint16_t)(back + 1u);
    return false;
}

Lufia2ExecutionResult Lufia2ObjectRemoveSlot(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ObjectReleaseContext(cpu))
        return ExecutionHandoff(cpu, 0x83f205u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLoadA(cpu, 4u);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_OBJECT_STATE));
    OpSepWidths(cpu, 0x30u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_OBJECT_ANIMATION_ID));
    OpCmpValue(cpu, 0xffu);
    if (!cpu->zero) {
        TransferDirectToA(cpu);
        OpLda(memory, cpu, OpLongX(cpu, WRAM_OBJECT_SPRITE_SIZE));
        OpCmpValue(cpu, 0xffu);
        if (!cpu->zero) {
            ExchangeAccumulatorBytes(cpu);
            OpLda(memory, cpu, OpLongX(cpu, WRAM_OBJECT_SPRITE_SLOT_A));
            OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
            OpLda(memory, cpu, OpLongX(cpu, WRAM_OBJECT_SPRITE_SLOT_B));
            OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_B));
            ExchangeAccumulatorBytes(cpu);
            OpTax(cpu);
            OpLda(memory, cpu, OpLongX(cpu, ROM_SPRITE_SLOT_COUNTS));
            if (!ObjectReleaseAllocation(memory, cpu))
                return ExecutionHandoff(cpu, cpu->resume_pc);
        }
    }
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_ANIMATION_ID));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_FRAME));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7FE3A6));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E1559));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_SPRITE_SLOT_A));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_SPRITE_SLOT_B));
    OpRepWidths(cpu, 0x10u);
    return ExecutionReturned(0x83f255u);
}
