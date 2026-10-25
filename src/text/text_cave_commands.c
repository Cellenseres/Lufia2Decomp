#include <stddef.h>

#include "core/cpu_ops.h"
#include "core/child_call.h"
#include "core/snes_registers.h"
#include "lufia2/ancient_cave.h"
#include "system/wram.h"
#include "text/text_internal.h"
#include "text/text_child_service.h"

enum {
    TRANSITION_CAVE = 0xabd0u,
    RUN_SCENE_OPERATION = 0xba39u,
    ROM_SCENE_OPERATIONS = 0xba55u,
    DP_HDMA_CHANNELS = 0x81u,
    DP_CAVE_RETURN_STATE = 0xaeu,
    SCENE_EFFECT_VALUE = WRAM_UNK_7E1271,
    SCENE_EFFECT_FLAGS = WRAM_SCREEN_EFFECTS,
    SCENE_EFFECT_STEP_TIMER = WRAM_FIELD_FIXED_COLOR_STEP_TIMER
};

typedef struct CaveCommandService {
    const Lufia2Memory *memory;
    uint32_t handoff;
    Lufia2PushedChildCall child;
    void *context;
    bool unwound;
} CaveCommandService;

static uint8_t RunCaveCommandChild(void *context, Lufia2CpuState *cpu, uint32_t target,
                                   uint32_t site, uint8_t frame) {
    CaveCommandService *service = context;
    const uint8_t caller_bank = cpu->program_bank;
    cpu->program_bank = (uint8_t)(target >> 16);
    Lufia2ExecutionResult result;
    switch (target) {
    case 0x84878au:
        result =
            Lufia2AncientCaveEnter(service->memory, cpu, RunCaveCommandChild, context);
        break;
    case 0x848888u:
        result = Lufia2AncientCaveResetParty(service->memory, cpu, RunCaveCommandChild,
                                             context);
        break;
    case 0x84890bu:
        result =
            Lufia2AncientCaveExit(service->memory, cpu, RunCaveCommandChild, context);
        break;
    default:
        service->handoff = target;
        return 0u;
    }
    if (result.flow != LUFIA2_EXECUTION_RETURNED) {
        if (!service->handoff)
            service->handoff = result.pc;
        return 0u;
    }
    uint16_t back = Pull8(service->memory, cpu);
    back |= (uint16_t)Pull8(service->memory, cpu) << 8;
    cpu->program_bank = frame == 3u ? Pull8(service->memory, cpu) : caller_bank;
    const uint32_t next = ((uint32_t)cpu->program_bank << 16) | (uint16_t)(back + 1u);
    if (next != site + frame + 1u) {
        service->handoff = next;
        return 0u;
    }
    return 1u;
}

static uint32_t CallCaveChild(CaveCommandService *service, Lufia2CpuState *cpu,
                              uint32_t target, uint32_t site, uint8_t frame) {
    service->handoff = 0u;
    if (service->child) {
        if (!CallChildWithFrame(service->memory, cpu, service->child, service->context,
                                site, target, frame, 0x80u)) {
            service->unwound = true;
            service->handoff = site;
        }
    } else {
        if (frame == 3u)
            SimulateJslFrame(service->memory, cpu, 0x80u, (uint16_t)(site + 3u));
        else
            SimulateJsrFrame(service->memory, cpu, (uint16_t)(site + 2u));
        RunCaveCommandChild(service, cpu, target, site, frame);
    }
    return service->handoff;
}

static uint32_t TransitionCave(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                               CaveCommandService *service) {
    Lufia2TextNextByte(memory, cpu, 0xabd2u);
    OpCmpValue(cpu, 1u);
    const bool leaving = cpu->zero;
    PushDataBank(memory, cpu);
    PushY(memory, cpu);
    uint32_t handoff = CallCaveChild(service, cpu, leaving ? 0x84890bu : 0x84878au,
                                     leaving ? 0x80abe4u : 0x80abd9u, 3u);
    if (handoff || !service->child)
        return handoff ? handoff : leaving ? 0x80abe8u : 0x80abddu;
    if (leaving) {
        OpLda(memory, cpu, OpDp(cpu, DP_CAVE_RETURN_STATE));
        if (cpu->negative) {
            handoff = CallCaveChild(service, cpu, 0x80a350u, 0x80abecu, 2u);
            if (handoff)
                return handoff;
            if (!cpu->accumulator_is_8_bit)
                return 0x80abefu;
            OpLoadA(cpu, 0x80u);
            OpSta(memory, cpu, OpAbs(cpu, WRAM_TEXT_STATE));
        }
    }
    OpPullY(memory, cpu);
    PullDataBank(memory, cpu);
    return LUFIA2_TEXT_COMMAND_NEXT;
}

Lufia2ExecutionResult Lufia2TextRunCaveTransition(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu,
                                                  Lufia2PushedChildCall child,
                                                  void *context) {
    if (!child || cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal ||
        cpu->stack < 0x1f20u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x80abd0u);
    CaveCommandService service = {memory, 0u, child, context, false};
    const uint32_t flow = TransitionCave(memory, cpu, &service);
    if (service.unwound) {
        Lufia2ExecutionResult result = ExecutionReturned(service.handoff);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    if (flow == LUFIA2_TEXT_COMMAND_NEXT)
        return ExecutionReturned(flow);
    return ExecutionHandoff(cpu, flow);
}

static uint32_t StartSceneChild(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                uint32_t target, uint16_t back, uint8_t frame) {
    if (frame == 3u)
        SimulateJslFrame(memory, cpu, 0x80u, back);
    else
        SimulateJsrFrame(memory, cpu, back);
    cpu->program_bank = (uint8_t)(target >> 16);
    return target;
}

static void PrepareSceneEffect(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLoadA(cpu, 6u);
    OpSta(memory, cpu, SCENE_EFFECT_STEP_TIMER);
    OpStz(memory, cpu, OpAbs(cpu, SCENE_EFFECT_VALUE));
}

static void FinishSceneEffect(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpOraValue(cpu, 0u);
    if (cpu->zero) {
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpAbs(cpu, SCENE_EFFECT_VALUE));
        OpLoadA(cpu, 0x10u);
    } else {
        OpLoadA(cpu, 0xe0u);
        OpSta(memory, cpu, OpAbs(cpu, SCENE_EFFECT_VALUE));
        OpLoadA(cpu, 0x20u);
    }
    OpTestBits(memory, cpu, OpAbs(cpu, SCENE_EFFECT_FLAGS), true);
}

static void SetSceneEffect(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PrepareSceneEffect(memory, cpu);
    Lufia2TextNextByte(memory, cpu, 0xbb7bu);
    FinishSceneEffect(memory, cpu);
}

static uint16_t PrepareSceneOperation(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    ExchangeAccumulatorBytes(cpu);
    OpLoadA(cpu, 0u);
    ExchangeAccumulatorBytes(cpu);
    OpAslA(cpu);
    OpTax(cpu);
    const uint16_t target =
        Read16Bank(memory, 0x80u, (uint16_t)(ROM_SCENE_OPERATIONS + cpu->x));
    SimulateJsrFrame(memory, cpu, 0xba44u);
    return target;
}

static uint16_t ReadSceneOperation(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2TextNextByte(memory, cpu, 0xba3bu);
    return PrepareSceneOperation(memory, cpu);
}

static uint32_t ContinueSceneOperation(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_FLAGS));
    OpBitValue(cpu, 0x40u);
    if (cpu->zero)
        return LUFIA2_TEXT_COMMAND_NEXT;
    OpStz(memory, cpu, OpAbs(cpu, WRAM_TEXT_STATE));
    return LUFIA2_TEXT_COMMAND_YIELD;
}

static uint32_t FinishSceneOperation(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    uint16_t back = Pull8(memory, cpu);
    back |= (uint16_t)Pull8(memory, cpu) << 8;
    if (back != 0xba44u)
        return 0x800000u | (uint16_t)(back + 1u);
    return ContinueSceneOperation(memory, cpu);
}

static Lufia2ExecutionResult SceneOperationResult(Lufia2CpuState *cpu,
                                                  uint32_t next) {
    if (next == LUFIA2_TEXT_COMMAND_NEXT || next == LUFIA2_TEXT_COMMAND_YIELD)
        return ExecutionReturned(next);
    return ExecutionHandoff(cpu, next);
}

static uint32_t StartSceneOperation(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                    uint16_t target) {
    switch (target) {
    case 0xbb70u:
        SetSceneEffect(memory, cpu);
        return FinishSceneOperation(memory, cpu);
    case 0xbb94u:
        PushY(memory, cpu);
        PushDataBank(memory, cpu);
        return StartSceneChild(memory, cpu, 0x83afeau, 0xbb99u, 3u);
    case 0xbaf1u:
        Lufia2TextNextByte(memory, cpu, 0xbaf3u);
        OpOraValue(cpu, 0u);
        if (!cpu->zero)
            return StartSceneChild(memory, cpu, 0x80bf0bu, 0xbafau, 2u);
        OpLoadA(cpu, 0x30u);
        OpTestBits(memory, cpu, OpDp(cpu, DP_HDMA_CHANNELS), false);
        OpLoadA(cpu, 0x80u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
        OpStz(memory, cpu, OpAbs(cpu, SNES_CGADSUB));
        OpStz(memory, cpu, OpAbs(cpu, SNES_CGWSEL));
        return StartSceneChild(memory, cpu, 0x80bf0bu, 0xbb5du, 2u);
    case 0xbaedu:
        return 0x808000u;
    case 0xba69u:
        Lufia2TextNextByte(memory, cpu, 0xba6bu);
        PushDataBank(memory, cpu);
        PushY(memory, cpu);
        PushAccumulator8(memory, cpu);
        return StartSceneChild(memory, cpu, 0x83afeau, 0xba72u, 3u);
    case 0xbac9u:
        Lufia2TextNextByte(memory, cpu, 0xbacbu);
        PushY(memory, cpu);
        PushDataBank(memory, cpu);
        return StartSceneChild(memory, cpu, 0x8eb2eeu, 0xbad1u, 3u);
    case 0xba97u:
        Lufia2TextNextByte(memory, cpu, 0xba99u);
        return StartSceneChild(memory, cpu, 0x8482e7u, 0xba9du, 3u);
    case 0xbaa2u:
        Lufia2TextNextByte(memory, cpu, 0xbaa4u);
        OpLoadA(cpu, 0x80u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
        return StartSceneChild(memory, cpu, 0x80bf0bu, 0xbaacu, 2u);
    default:
        return 0x800000u | target;
    }
}

Lufia2ExecutionResult Lufia2TextRunSceneOperation(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu,
                                                  Lufia2PushedChildCall child,
                                                  void *context) {
    if (!child || cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal ||
        cpu->stack < 0x1f20u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x80ba39u);
    if (!CallChildWithFrame(memory, cpu, child, context, 0x80ba39u,
                            0x80c0b7u, 2u, 0x80u))
        return (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND, 0x80ba39u, 0u};
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal)
        return ExecutionHandoff(cpu, (uint32_t)cpu->program_bank << 16 | 0xba3cu);
    const uint16_t target = PrepareSceneOperation(memory, cpu);
    if (target == 0xbb70u) {
        TextChildService service = {memory, cpu, child, context, ExecutionReturned(target)};
        PrepareSceneEffect(memory, cpu);
        if (!TextCallChild(&service, 0x80bb79u, 0x80c0b7u, 2u))
            return service.result;
        FinishSceneEffect(memory, cpu);
        return SceneOperationResult(cpu, FinishSceneOperation(memory, cpu));
    }
    if (target == 0xba69u || target == 0xba97u || target == 0xbaa2u ||
        target == 0xbac9u || target == 0xbaf1u || target == 0xbb94u) {
        const Lufia2ExecutionResult result =
            Lufia2TextSceneDisplayCallback(memory, cpu, target, child, context);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
        return SceneOperationResult(cpu, FinishSceneOperation(memory, cpu));
    }
    if (target == 0xbaedu || target == 0xba67u)
        return SceneOperationResult(cpu, StartSceneOperation(memory, cpu, target));
    if (!child(context, cpu, 0x800000u | target, 0x80ba42u, 2u)) {
        Lufia2ExecutionResult result = ExecutionReturned(0x80ba42u);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal)
        return ExecutionHandoff(cpu, (uint32_t)cpu->program_bank << 16 | 0xba45u);
    return SceneOperationResult(cpu, ContinueSceneOperation(memory, cpu));
}

uint32_t Lufia2TextExecuteCaveCommand(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                      uint16_t handler) {
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal ||
        cpu->stack < 0x1f20u || cpu->stack > 0x1ffcu)
        return 0u;
    switch (handler) {
    case TRANSITION_CAVE: {
        CaveCommandService service = {memory, 0u, NULL, NULL, false};
        return TransitionCave(memory, cpu, &service);
    }
    case RUN_SCENE_OPERATION:
        return StartSceneOperation(memory, cpu, ReadSceneOperation(memory, cpu));
    default:
        return 0u;
    }
}
