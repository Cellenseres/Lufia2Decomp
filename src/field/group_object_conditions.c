#include "lufia2/field.h"
#include "item/menu_application_internal.h"
#include "system/wram.h"

enum {
    DP_GROUP_CONDITION_COUNT = 0x58u,
    DP_GROUP_MAP_INDEX = 0x5du,
    DP_GROUP_ID = 0x5fu,
    ROM_MAP_GROUPS = 0x94c998u,
    ROM_MAP_CONDITION_LISTS = 0x91b8b5u,
    MAP_GROUP_LIMIT = 0xf1u
};

static bool CountMapConditions(MenuUseCall *call) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    OpWriteX(memory, cpu, OpDp(cpu, DP_GROUP_MAP_INDEX), cpu->x);
    OpRepWidths(cpu, 0x20u);
    OpTxa(cpu);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, ROM_MAP_CONDITION_LISTS));
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    for (;;) {
        OpLda(memory, cpu, OpLongX(cpu, ROM_MAP_CONDITION_LISTS));
        OpCmpValue(cpu, 0xffu);
        if (cpu->zero)
            return true;
        OpPushX(memory, cpu);
        if (!MenuUseChild(call, 0x8eb827u, 0x8ec338u, 3u, 1u) ||
            !MenuUseChild(call, 0x8eb82bu, 0x8ec34fu, 3u, 1u))
            return false;
        OpBit(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E093B));
        if (cpu->zero)
            OpStepMem(memory, cpu, OpDp(cpu, DP_GROUP_CONDITION_COUNT), 1);
        OpPullX(memory, cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpInx(cpu);
    }
}

Lufia2ExecutionResult Lufia2FieldCountGroupObjectConditions(const Lufia2Memory *memory,
                                                            Lufia2CpuState *cpu,
                                                            Lufia2PushedChildCall child,
                                                            void *context) {
    if (!MenuUseReady(cpu, child, 0x8eu))
        return ExecutionHandoff(cpu, 0x8eb7e2u);
    MenuUseCall call = {memory, cpu, child, context, 0x8eu, {0}};
    PushDataBank(memory, cpu);
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpStz(memory, cpu, OpDp(cpu, DP_GROUP_CONDITION_COUNT));
    OpStz(memory, cpu, OpDp(cpu, DP_GROUP_CONDITION_COUNT + 1u));
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID));
    OpCmpValue(cpu, 2u);
    if (cpu->carry) {
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, ROM_MAP_GROUPS));
        OpSta(memory, cpu, OpDp(cpu, DP_GROUP_ID));
        OpLdx(cpu, 0u);
        OpWriteX(memory, cpu, OpDp(cpu, DP_GROUP_MAP_INDEX), cpu->x);
        bool exhausted = false;
        while (!exhausted) {
            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_GROUP_MAP_INDEX)));
            for (;;) {
                OpLda(memory, cpu, OpLongX(cpu, ROM_MAP_GROUPS));
                OpCmp(memory, cpu, OpDp(cpu, DP_GROUP_ID));
                if (cpu->zero)
                    break;
                OpInx(cpu);
                OpCpx(cpu, MAP_GROUP_LIMIT);
                if (cpu->carry) {
                    exhausted = true;
                    break;
                }
            }
            if (!exhausted) {
                if (!CountMapConditions(&call))
                    return call.result;
                OpStepMem(memory, cpu, OpDp(cpu, DP_GROUP_MAP_INDEX), 1);
            }
        }
    }
    OpLda(memory, cpu, OpDp(cpu, DP_GROUP_CONDITION_COUNT));
    UnpackStatus(cpu, Pull8(memory, cpu));
    OpPullX(memory, cpu);
    OpPullY(memory, cpu);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x8eb846u);
}
