#include "actor/actor_internal.h"
#include "core/child_call.h"
#include "core/wram_view.h"
#include "system/wram.h"

enum {
    PARTY_LAST_SLOT = 7u,
    PARTY_ACTOR_READY = 4u,
    PARTY_FIRST_READY_ID = 14u,
    PARTY_REBUILD_ACTOR = 0xfdu,
    PARTY_REBUILD_COMMAND = 0x83cae1u,
    PARTY_TRANSITION_COMMAND = 0x83cb71u,
    PARTY_COMMAND_COMMIT = 0x83c8c7u,
    PARTY_COMMAND_YIELD = 0x83c8d2u
};

typedef struct PartyActorSearch {
    uint16_t slot;
    uint8_t value;
    bool carry;
} PartyActorSearch;

static PartyActorSearch FindUnreadyPartyActor(Lufia2Wram wram, bool carry) {
    PartyActorSearch search = {PARTY_LAST_SLOT, 0u, carry};

    for (; search.slot != 0u; --search.slot) {
        search.value = WramReadAt(wram, WRAM_ACTOR_STATE, search.slot);
        if (!(search.value & PARTY_ACTOR_READY)) {
            search.value = WramReadAt(wram, WRAM_ACTOR_ID, search.slot);
            search.carry = search.value >= PARTY_FIRST_READY_ID;
            if (!search.carry)
                break;
        }
    }
    return search;
}

static bool PartyActorsReady(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const PartyActorSearch search =
        FindUnreadyPartyActor(WramViewOfCaller(memory, cpu), cpu->carry != 0);

    cpu->x = search.slot;
    LoadA8(cpu, search.value);
    cpu->carry = search.carry;
    if (search.slot != 0u)
        Compare8(cpu, search.value, PARTY_FIRST_READY_ID);
    else
        SetNz16(cpu, search.slot);
    return search.slot == 0u;
}

static void SelectRebuildActor(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint16_t slot = PARTY_LAST_SLOT;
    uint8_t actor;

    for (;;) {
        actor = WramReadAt(wram, WRAM_ACTOR_ID, slot);
        if (actor == PARTY_REBUILD_ACTOR || slot == 0u)
            break;
        --slot;
    }
    LoadA8(cpu, actor);
    Compare8(cpu, actor, PARTY_REBUILD_ACTOR);
    cpu->x = actor == PARTY_REBUILD_ACTOR ? slot : 0xffffu;
    if (actor != PARTY_REBUILD_ACTOR)
        SetNz16(cpu, cpu->x);
}

static bool PartySceneChild(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t site, uint32_t target,
    Lufia2ExecutionResult *result) {
    if (!CallChildWithFrame(memory, cpu, child, context, site, target, 3u, 0x83u)) {
        *result = ExecutionReturned(site);
        result->flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (cpu->program_bank != 0x83u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page) {
        *result = ExecutionHandoff(cpu, site + 4u);
        return false;
    }
    return true;
}

static void ClearPartyCommandState(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0xcb61u);
    LoadX16(cpu, 4u);
    LoadA8(cpu, 0xffu);
    do {
        Write8(memory, AbsoluteIndexedAddress(cpu, WRAM_FOLLOW_SLOTS,
            cpu->x), A8(cpu));
        cpu->x = (uint16_t)(cpu->x - 1u);
        SetNz16(cpu, cpu->x);
    } while (!cpu->negative);
    SimulateRtsFrame(memory, cpu);
}

Lufia2ExecutionResult Lufia2ActorPrimaryPartyCommand(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t command,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->program_bank != 0x83u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page ||
        cpu->stack < 0x1f20u || cpu->stack > 0x1ffcu ||
        (command != PARTY_REBUILD_COMMAND && command != PARTY_TRANSITION_COMMAND))
        return ExecutionHandoff(cpu, command);
    const bool rebuild = command == PARTY_REBUILD_COMMAND;
    Lufia2ExecutionResult result = ExecutionReturned(PARTY_COMMAND_YIELD);

    LoadX16(cpu, Read16Direct(memory, cpu, DP_ACTOR_SLOT));
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_ID, cpu->x);
    if (A8(cpu) != 0u) {
        LoadA8(cpu, 0xffu);
        StoreAAbsolute8(memory, cpu, WRAM_ACTOR_ID, cpu->x);
        if (!PartySceneChild(memory, cpu, child, context,
            rebuild ? 0x83caedu : 0x83cb7du, 0x83aaafu, &result))
            return result;
        return result;
    }
    if (rebuild) {
        LoadA8(cpu, Read8(memory, WRAM_UNK_7FE316));
        Or8(cpu, 0x80u);
        Write8(memory, WRAM_UNK_7FE316, A8(cpu));
    }
    if (!PartyActorsReady(memory, cpu))
        return ExecutionReturned(PARTY_COMMAND_COMMIT);
    if (!rebuild) {
        LoadA8(cpu, 0x88u);
        TestBitsAbsolute8(memory, cpu, WRAM_FIELD_FLAGS, 1u);
        LoadA8(cpu, 0x85u);
        if (!PartySceneChild(memory, cpu, child, context,
            0x83cb9fu, 0x83d350u, &result))
            return result;
        LoadX16(cpu, Read16Direct(memory, cpu, DP_ACTOR_SLOT));
        LoadA8(cpu, 0xffu);
        Write8(memory, LongIndexedAddress(WRAM_ACTOR_PRIMARY_TIMER, cpu->x), A8(cpu));
        return result;
    }
    LoadA8(cpu, Read8(memory, WRAM_UNK_7FE316));
    And8(cpu, 0x7fu);
    Write8(memory, WRAM_UNK_7FE316, A8(cpu));
    Write8(memory, AbsoluteIndexedAddress(cpu, WRAM_UNK_7E089D, 0u), 0u);
    StoreA8Absolute(memory, cpu, WRAM_UNK_7E085D, 0xffu);
    LoadAAbsolute8(memory, cpu, WRAM_UNK_7E091D, 0u);
    Write8(memory, WRAM_UNK_7FD0FE, A8(cpu));
    if (!PartySceneChild(memory, cpu, child, context,
        0x83cb2eu, 0x848204u, &result))
        return result;
    LoadA8(cpu, 0xf8u);
    Write8(memory, WRAM_ACTOR_DISPLAY_OFFSET_X, A8(cpu));
    LoadA8(cpu, 0xffu);
    Write8(memory, WRAM_ACTOR_DISPLAY_OFFSET_X + 1u, A8(cpu));
    SelectRebuildActor(memory, cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_ACTOR_SLOT)));
    PushAccumulator8(memory, cpu);
    Write16Direct(memory, cpu, DP_ACTOR_SLOT, cpu->x);
    if (!PartySceneChild(memory, cpu, child, context,
            0x83cb50u, 0x83ab4fu, &result) ||
        !PartySceneChild(memory, cpu, child, context,
            0x83cb54u, 0x83aaafu, &result))
        return result;
    LoadA8(cpu, Pull8(memory, cpu));
    StoreADirect8(memory, cpu, DP_ACTOR_SLOT);
    if (!PartySceneChild(memory, cpu, child, context,
        0x83cb5bu, 0x83ab4fu, &result))
        return result;
    ClearPartyCommandState(memory, cpu);
    return result;
}
