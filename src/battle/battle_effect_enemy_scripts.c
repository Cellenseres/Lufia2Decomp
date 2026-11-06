#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/battle.h"
#include "battle/effect_target_scripts.h"
#include "core/snes_registers.h"

enum {
    DP_EFFECT_STREAM = 0xc3u,
    DP_SCRIPT_ADDRESS = 0u,
    DP_REMAINING_ENEMIES = 2u,
    FIRST_ENEMY = 0x4f0bu,
    ENEMY_STRIDE = 11u,
    ENEMY_TARGET = 10u,
    TARGET_FLAGS = 15u,
    TARGET_SCRIPT_FLAG = 4u,
    ROM_TARGET_INDICES = 0x96ffecu,
    SCRIPT_ADDRESS = 3u,
    SCRIPT_FIRST_PARAMETER = 0x13u,
    SCRIPT_LAST_PARAMETER = 0x19u,
    SCRIPT_HORIZONTAL_SPEED = 0x1bu,
    SCRIPT_VERTICAL_SPEED = 0x1du,
    SCRIPT_HORIZONTAL_POSITION = 0x1fu,
    SCRIPT_VERTICAL_POSITION = 0x21u,
    SCRIPT_ADJUSTMENT = 0x23u,
    SCRIPT_TARGET = 0x25u
};

enum { DP_SAVED_PARAMETERS = 0x11u };

static Lufia2ExecutionResult InterruptedEnemyCall(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static void InitializeEnemyScript(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, bool saved_parameters) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DP_SCRIPT_ADDRESS));
    OpSta(memory, cpu, OpAbsX(cpu, SCRIPT_ADDRESS));
    for (uint16_t field = SCRIPT_FIRST_PARAMETER;
         field <= SCRIPT_LAST_PARAMETER; field += 2u) {
        OpLda(memory, cpu, saved_parameters ?
            OpDp(cpu, (uint8_t)(DP_SAVED_PARAMETERS + field - SCRIPT_FIRST_PARAMETER)) :
            OpAbsY(cpu, field));
        OpSta(memory, cpu, OpAbsX(cpu, field));
    }
    OpLda(memory, cpu, OpAbsY(cpu, 0u));
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpAbsX(cpu, SCRIPT_HORIZONTAL_POSITION));
    OpLda(memory, cpu, OpAbsY(cpu, 1u));
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpAbsX(cpu, SCRIPT_VERTICAL_POSITION));
    OpStz(memory, cpu, OpAbsX(cpu, SCRIPT_HORIZONTAL_SPEED));
    OpStz(memory, cpu, OpAbsX(cpu, SCRIPT_VERTICAL_SPEED));
    OpLda(memory, cpu, OpAbsY(cpu, 2u));
    OpSta(memory, cpu, OpAbsX(cpu, SCRIPT_ADJUSTMENT));
    OpLoadA(cpu, 0x0101u);
    OpSta(memory, cpu, OpAbsX(cpu, 0u));
    OpSta(memory, cpu, OpAbsX(cpu, 1u));
    OpLda(memory, cpu, OpAbsY(cpu, ENEMY_TARGET));
    OpDecA(cpu);
    OpSta(memory, cpu, OpAbsX(cpu, SCRIPT_TARGET));
}

Lufia2ExecutionResult BattleSpawnTargetScripts(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t entry) {
    const uint32_t slot_site = entry + 0x32u;
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, entry);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_EFFECT_STREAM));
    OpSta(memory, cpu, OpDp(cpu, DP_SCRIPT_ADDRESS));
    OpStepMem(memory, cpu, OpDp(cpu, DP_EFFECT_STREAM), 1);
    OpStepMem(memory, cpu, OpDp(cpu, DP_EFFECT_STREAM), 1);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_EFFECT_TARGET_COUNT & 0xffffu));
    OpAndValue(cpu, 0xffu);
    if (!cpu->zero) {
        OpSta(memory, cpu, OpDp(cpu, DP_REMAINING_ENEMIES));
        for (uint16_t field = SCRIPT_FIRST_PARAMETER;
             field <= SCRIPT_LAST_PARAMETER; field += 2u) {
            OpLda(memory, cpu, OpAbsY(cpu, field));
            OpSta(memory, cpu, OpDp(cpu,
                (uint8_t)(DP_SAVED_PARAMETERS + field - SCRIPT_FIRST_PARAMETER)));
        }
        PushY(memory, cpu);
        OpLdy(cpu, FIRST_ENEMY);
        do {
            OpSepWidths(cpu, 0x20u);
            const uint16_t caller_stack = cpu->stack;
            if (!CallChildWithFrame(memory, cpu, child, context,
                    slot_site, 0x818fabu, 2u, 0x81u))
                return InterruptedEnemyCall(slot_site);
            if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
                cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
                cpu->stack != caller_stack)
                return ExecutionHandoff(cpu,
                    (uint32_t)cpu->program_bank << 16 | (uint16_t)(slot_site + 3u));
            InitializeEnemyScript(memory, cpu, true);
            OpTya(cpu);
            cpu->carry = 0u;
            OpAdcValue(cpu, ENEMY_STRIDE);
            OpTay(cpu);
            OpStepMem(memory, cpu, OpAbs(cpu, WRAM_BATTLE_EFFECT_ACTIVE_COUNT), 1);
            OpStepMem(memory, cpu, OpDp(cpu, DP_REMAINING_ENEMIES), -1);
        } while (!cpu->zero);
        cpu->y = PullIndexValue(memory, cpu);
    }
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(entry + 0x8eu);
}

Lufia2ExecutionResult Lufia2BattleEffectSpawnListedTargetScripts(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return BattleSpawnTargetScripts(memory, cpu, child, context, 0x819fabu);
}

Lufia2ExecutionResult Lufia2BattleClearTargetRecords(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    enum { RECORD_COUNT = 12u, RECORD_DISABLED = 9u };
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x81c586u);
    PushDataBank(memory, cpu);
    OpPushX(memory, cpu);
    Push8(memory, cpu, A8(cpu));
    OpSetDataBank(memory, cpu, 0x97u);
    OpLdx(cpu, FIRST_ENEMY);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_WMADDL), cpu->x);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WMADDH));
    OpLdx(cpu, RECORD_COUNT * ENEMY_STRIDE);
    do {
        OpStz(memory, cpu, OpAbs(cpu, SNES_WMDATA));
        OpDex(cpu);
    } while (!cpu->zero);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpLoadA(cpu, 0xffu);
    for (unsigned record = 0u; record < RECORD_COUNT; ++record)
        OpSta(memory, cpu, OpAbs(cpu,
            (uint16_t)(FIRST_ENEMY + RECORD_DISABLED + record * ENEMY_STRIDE)));
    LoadA8(cpu, Pull8(memory, cpu));
    OpPullX(memory, cpu);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81c5ceu);
}

Lufia2ExecutionResult Lufia2BattleEffectSpawnEnemyScripts(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x81940eu);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_EFFECT_STREAM));
    OpSta(memory, cpu, OpDp(cpu, DP_SCRIPT_ADDRESS));
    OpStepMem(memory, cpu, OpDp(cpu, DP_EFFECT_STREAM), 1);
    OpStepMem(memory, cpu, OpDp(cpu, DP_EFFECT_STREAM), 1);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_EFFECT_TARGET_COUNT & 0xffffu));
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, DP_REMAINING_ENEMIES));
    PushY(memory, cpu);
    OpLdy(cpu, FIRST_ENEMY);
    do {
        uint32_t target_site = 0u;
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbsY(cpu, ENEMY_TARGET));
        if (cpu->negative) {
            OpDecA(cpu);
            OpRepWidths(cpu, 0x20u);
            OpAndValue(cpu, 0x7fu);
            OpTax(cpu);
            OpSepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpLongX(cpu, ROM_TARGET_INDICES));
            OpOraValue(cpu, 0x80u);
            target_site = 0x81943fu;
        } else {
            OpCmpValue(cpu, 5u);
            if (cpu->zero) {
                OpLoadA(cpu, 0x10u);
                target_site = 0x819456u;
            }
        }
        if (target_site) {
            PushY(memory, cpu);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    target_site, 0x81b2b5u, 3u, 0x81u))
                return InterruptedEnemyCall(target_site);
            cpu->y = PullIndexValue(memory, cpu);
            OpLda(memory, cpu, OpAbsX(cpu, TARGET_FLAGS));
            OpBitValue(cpu, TARGET_SCRIPT_FLAG);
            if (!cpu->zero) {
                if (!CallChildWithFrame(memory, cpu, child, context,
                        0x819466u, 0x818fabu, 2u, 0x81u))
                    return InterruptedEnemyCall(0x819466u);
                InitializeEnemyScript(memory, cpu, false);
                OpStepMem(memory, cpu, OpAbs(cpu, WRAM_BATTLE_EFFECT_ACTIVE_COUNT), 1);
            }
        }
        OpRepWidths(cpu, 0x20u);
        OpTya(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, ENEMY_STRIDE);
        OpTay(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, DP_REMAINING_ENEMIES), -1);
    } while (!cpu->zero);
    cpu->y = PullIndexValue(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x8194c9u);
}
