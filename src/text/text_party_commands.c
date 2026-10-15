#include "actor/actor_internal.h"
#include "core/cpu_ops.h"
#include "field/event_script_internal.h"
#include "lufia2/actor.h"
#include "lufia2/field.h"
#include "lufia2/item.h"
#include "lufia2/party.h"
#include "lufia2/system.h"
#include "system/wram.h"
#include "text/text_internal.h"

enum {
    BRANCH_ON_FLAG = 0xa29cu,
    REBUILD_PARTY_ACTORS = 0xb46bu,
    LEARN_SPELL = 0xa489u,
    REMOVE_ITEM_PAGE_ZERO = 0xa49du,
    REMOVE_ITEM_PAGE_ONE = 0xa4a6u,
    RESTORE_PARTY = 0xa4ddu,
    CLEAR_EQUIPMENT = 0xa50bu,
    RESET_MEMBER_MARKER = 0xa4f0u,
    INCREASE_MEMBER_STAT = 0xa54au,
    ADD_PARTY_MEMBER = 0xa5c8u,
    REMOVE_PARTY_MEMBER = 0xa5ffu,
    REVEAL_ACTOR = 0xa63au,
    TEST_ITEM_COUNT = 0xac28u,
    TEST_GOLD = 0xae03u,
    TEST_OBJECT_BIT = 0xae24u,
    SET_ACTOR_DISPLAY_OFFSET = 0xb078u,
    SET_OBJECT_LOCATION = 0xb2b1u,
    FIELD_PARTY_CAPACITY = 5u,
    DP_OPERAND = 0x54u,
    DP_STAT_VALUE = 0x4eu,
    DP_STAT_PERCENT = 0x50u,
    DP_STAT_PRODUCT = 0x52u,
    DP_STAT_SOURCE = 0x5du,
    DP_STAT_DESTINATION = 0x60u,
    TEXT_CONDITION = 0x079du,
    TEXT_GOLD_LOW = 0x0b55u,
    TEXT_GOLD_HIGH = 0x0b57u,
    ROM_CHARACTER_RECORDS = 0x859ebau,
    ROM_STAT_SOURCES = 0x80a596u,
    ROM_STAT_DESTINATIONS = 0x80a5a4u
};

typedef struct TextPartyService {
    const Lufia2Memory *memory;
    uint32_t handoff;
} TextPartyService;

static uint8_t RunPartyChild(void *context, Lufia2CpuState *cpu, uint32_t target,
                             uint32_t site, uint8_t frame) {
    TextPartyService *service = context;
    const Lufia2Memory *memory = service->memory;
    const uint8_t caller_bank = cpu->program_bank;
    cpu->program_bank = (uint8_t)(target >> 16);
    Lufia2ExecutionResult result;
    switch (target) {
    case 0x80834cu:
        result = Lufia2Multiply16By8(memory, cpu);
        break;
    case 0x81f4d5u:
        result = Lufia2PartyDerivedStats(memory, cpu);
        break;
    case 0x81f629u:
        result = Lufia2PartyClearStatuses(memory, cpu);
        break;
    case 0x81f5edu:
        result = Lufia2PartyRestore11(memory, cpu);
        break;
    case 0x81f60bu:
        result = Lufia2PartyRestore13(memory, cpu);
        break;
    case 0x82fa57u:
        result = Lufia2ItemRemoveRequested(memory, cpu);
        break;
    case 0x82fd15u:
        result = Lufia2PartyClearEquipment(memory, cpu);
        break;
    case 0x81f723u:
        result = Lufia2PartyAddMenuMember(memory, cpu);
        break;
    case 0x81f74fu:
        result = Lufia2PartyRemoveMenuMember(memory, cpu);
        break;
    case 0x848204u:
        result = Lufia2FieldRebuildPartyActors(memory, cpu, RunPartyChild, context);
        break;
    case 0x8482d5u:
        result = Lufia2ActorPreparePartyOffsets(memory, cpu);
        break;
    case 0x82fd3du:
        result = Lufia2PartyLearnSelectedSpell(memory, cpu, RunPartyChild, context);
        break;
    case 0x81f414u:
        result = Lufia2LoadSpellRecord(memory, cpu);
        break;
    case 0x82fb1fu:
        result = Lufia2ItemPossessionCount(memory, cpu);
        break;
    case 0x838ac9u:
        result = Lufia2FieldObjectBitTest(memory, cpu);
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
    uint16_t back = Pull8(memory, cpu);
    back |= (uint16_t)Pull8(memory, cpu) << 8;
    cpu->program_bank = frame == 3u ? Pull8(memory, cpu) : caller_bank;
    const uint32_t next = ((uint32_t)cpu->program_bank << 16) | (uint16_t)(back + 1u);
    if (next != site + frame + 1u) {
        service->handoff = next;
        return 0u;
    }
    return 1u;
}

static uint32_t CallPartyService(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                 uint32_t target, uint16_t back) {
    SimulateJslFrame(memory, cpu, 0x80u, back);
    TextPartyService service = {memory, 0u};
    RunPartyChild(&service, cpu, target, 0x800000u | (uint16_t)(back - 3u), 3u);
    return service.handoff;
}

static void ReadByteTableIndex(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                               uint16_t back) {
    SimulateJsrFrame(memory, cpu, back);
    Lufia2TextNextByte(memory, cpu, 0xa5b4u);
    ExchangeAccumulatorBytes(cpu);
    OpLoadA(cpu, 0u);
    ExchangeAccumulatorBytes(cpu);
    OpAslA(cpu);
    OpTax(cpu);
    SimulateRtsFrame(memory, cpu);
}

static void SetTextCondition(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                             uint16_t back, bool enabled) {
    SimulateJsrFrame(memory, cpu, back);
    OpLoadA(cpu, 0x80u);
    OpTestBits(memory, cpu, OpAbs(cpu, TEXT_CONDITION), enabled);
    SimulateRtsFrame(memory, cpu);
}

static uint32_t BranchOnFlag(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2TextNextByte(memory, cpu, 0xa29eu);
    Lufia2TextTestFlag(memory, cpu, 0xa2a1u);
    if (cpu->zero)
        Lufia2TextGoto(memory, cpu);
    else {
        Lufia2TextNextByte(memory, cpu, 0xa2a9u);
        Lufia2TextNextByte(memory, cpu, 0xa2acu);
    }
    return LUFIA2_TEXT_COMMAND_NEXT;
}

static uint32_t ResetMemberMarker(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    ReadByteTableIndex(memory, cpu, 0xa4f2u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, ROM_CHARACTER_RECORDS));
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, 0x000fu));
    OpBitValue(cpu, 4u);
    if (cpu->zero) {
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpAbsX(cpu, 0x00bcu));
    }
    return LUFIA2_TEXT_COMMAND_NEXT;
}

static uint32_t StatAddress(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                            uint8_t pointer) {
    return OpAbs(cpu, Read16Direct(memory, cpu, pointer));
}

static uint32_t IncreaseMemberStat(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    ReadByteTableIndex(memory, cpu, 0xa54cu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, ROM_CHARACTER_RECORDS));
    OpTax(cpu);
    OpWriteX(memory, cpu, OpDp(cpu, DP_STAT_SOURCE), cpu->x);
    OpWriteX(memory, cpu, OpDp(cpu, DP_STAT_DESTINATION), cpu->x);
    OpWriteX(memory, cpu, OpDp(cpu, DP_OPERAND), cpu->x);
    OpSepWidths(cpu, 0x20u);
    ReadByteTableIndex(memory, cpu, 0xa55eu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, ROM_STAT_SOURCES));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, DP_STAT_SOURCE));
    OpSta(memory, cpu, OpDp(cpu, DP_STAT_SOURCE));
    OpLda(memory, cpu, OpLongX(cpu, ROM_STAT_DESTINATIONS));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, DP_STAT_DESTINATION));
    OpSta(memory, cpu, OpDp(cpu, DP_STAT_DESTINATION));
    OpLda(memory, cpu, StatAddress(memory, cpu, DP_STAT_SOURCE));
    OpSta(memory, cpu, OpDp(cpu, DP_STAT_VALUE));
    OpSepWidths(cpu, 0x20u);
    Lufia2TextNextByte(memory, cpu, 0xa57bu);
    OpSta(memory, cpu, OpDp(cpu, DP_STAT_PERCENT));
    uint32_t handoff = CallPartyService(memory, cpu, 0x80834cu, 0xa581u);
    if (handoff)
        return handoff;
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DP_STAT_PRODUCT));
    cpu->carry = 0u;
    OpAdc(memory, cpu, StatAddress(memory, cpu, DP_STAT_DESTINATION));
    OpSta(memory, cpu, StatAddress(memory, cpu, DP_STAT_DESTINATION));
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_OPERAND)));
    handoff = CallPartyService(memory, cpu, 0x81f4d5u, 0xa592u);
    return handoff ? handoff : LUFIA2_TEXT_COMMAND_NEXT;
}

static uint32_t ChangePartyMembership(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                      bool adding) {
    Lufia2TextNextByte(memory, cpu, adding ? 0xa5cau : 0xa601u);
    PushY(memory, cpu);
    PushAccumulator8(memory, cpu);
    uint32_t handoff = CallPartyService(memory, cpu, adding ? 0x81f723u : 0x81f74fu,
                                        adding ? 0xa5d0u : 0xa607u);
    if (handoff)
        return handoff;
    LoadA8(cpu, Pull8(memory, cpu));
    OpSta(memory, cpu, OpDp(cpu, DP_OPERAND));
    OpLdx(cpu, adding ? FIELD_PARTY_CAPACITY - 1u : 0u);
    if (adding) {
        do {
            OpLda(memory, cpu, OpAbsX(cpu, WRAM_SCENE_PARTY_IDS));
            OpCmp(memory, cpu, OpDp(cpu, DP_OPERAND));
            if (cpu->zero) {
                OpPullY(memory, cpu);
                return LUFIA2_TEXT_COMMAND_NEXT;
            }
            OpDex(cpu);
        } while (!cpu->negative);
        OpLdx(cpu, 0u);
        for (;;) {
            OpLda(memory, cpu, OpAbsX(cpu, WRAM_SCENE_PARTY_IDS));
            if (cpu->negative)
                break;
            OpInx(cpu);
            OpCpx(cpu, FIELD_PARTY_CAPACITY);
            if (cpu->zero) {
                OpLdx(cpu, 0u);
                break;
            }
        }
        OpLda(memory, cpu, OpDp(cpu, DP_OPERAND));
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_SCENE_PARTY_IDS));
    } else {
        for (;;) {
            OpLda(memory, cpu, OpAbsX(cpu, WRAM_SCENE_PARTY_IDS));
            OpCmp(memory, cpu, OpDp(cpu, DP_OPERAND));
            if (cpu->zero)
                break;
            OpInx(cpu);
            OpCpx(cpu, FIELD_PARTY_CAPACITY);
            if (cpu->zero) {
                OpPullY(memory, cpu);
                return LUFIA2_TEXT_COMMAND_NEXT;
            }
        }
        OpLdy(cpu, cpu->x);
        for (;;) {
            OpInx(cpu);
            OpCpx(cpu, FIELD_PARTY_CAPACITY);
            if (cpu->zero)
                break;
            OpLda(memory, cpu, OpAbsX(cpu, WRAM_SCENE_PARTY_IDS));
            OpSta(memory, cpu, OpAbsY(cpu, WRAM_SCENE_PARTY_IDS));
            OpIny(cpu);
        }
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_SCENE_PARTY_IDS + 4u));
    }
    handoff = CallPartyService(memory, cpu, 0x848204u, adding ? 0xa5fau : 0xa635u);
    if (handoff)
        return handoff;
    OpPullY(memory, cpu);
    return LUFIA2_TEXT_COMMAND_NEXT;
}

static uint32_t RevealActor(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2TextNextByte(memory, cpu, 0xa63cu);
    OpSta(memory, cpu, OpDp(cpu, DP_OPERAND));
    Lufia2EventFindActorId(memory, cpu, 0xa642u);
    if (cpu->carry)
        return LUFIA2_TEXT_COMMAND_NEXT;
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
    OpAndValue(cpu, 0xfbu);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_UNK_7FE316));
    OpAndValue(cpu, 0x7fu);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7FE316));
    SimulateJslFrame(memory, cpu, 0x80u, 0xa65cu);
    cpu->program_bank = 0x83u;
    Lufia2ActorMarkMapOccupancy(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    cpu->program_bank = 0x80u;
    OpLda(memory, cpu, OpDp(cpu, DP_OPERAND));
    OpCmpValue(cpu, 0x10u);
    if (!cpu->carry)
        return LUFIA2_TEXT_COMMAND_NEXT;
    OpCmpValue(cpu, 0x50u);
    if (cpu->carry)
        return LUFIA2_TEXT_COMMAND_NEXT;
    LoadA16(cpu, cpu->direct_page);
    OpLda(memory, cpu, OpDp(cpu, DP_OPERAND));
    cpu->carry = 1u;
    OpSbcValue(cpu, 0x10u);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E081E));
    OpAndValue(cpu, 0x7fu);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E081E));
    return LUFIA2_TEXT_COMMAND_NEXT;
}

static uint32_t LearnSpell(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2TextNextByte(memory, cpu, 0xa48bu);
    PushAccumulator8(memory, cpu);
    Lufia2TextNextByte(memory, cpu, 0xa48fu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_RECORD_ID));
    LoadA8(cpu, Pull8(memory, cpu));
    PushY(memory, cpu);
    const uint32_t handoff = CallPartyService(memory, cpu, 0x82fd3du, 0xa498u);
    if (handoff)
        return handoff;
    OpPullY(memory, cpu);
    return LUFIA2_TEXT_COMMAND_NEXT;
}

static uint32_t TestItemCount(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SetTextCondition(memory, cpu, 0xac2au, false);
    OpRepWidths(cpu, 0x20u);
    PushY(memory, cpu);
    OpLoadA(cpu, 0x2bu);
    const uint32_t handoff = CallPartyService(memory, cpu, 0x82fb1fu, 0xac34u);
    if (handoff)
        return handoff;
    OpPullY(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpDp(cpu, DP_OPERAND));
    Lufia2TextNextByte(memory, cpu, 0xac3cu);
    OpDecA(cpu);
    OpCmp(memory, cpu, OpDp(cpu, DP_OPERAND));
    if (!cpu->carry)
        SetTextCondition(memory, cpu, 0xac44u, true);
    return LUFIA2_TEXT_COMMAND_NEXT;
}

static uint32_t RemoveItem(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                           bool second_page) {
    if (second_page) {
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, OpAbs(cpu, 0x09d0u));
    } else {
        OpStz(memory, cpu, OpAbs(cpu, 0x09d0u));
    }
    SimulateJsrFrame(memory, cpu, second_page ? 0xa4adu : 0xa4a2u);
    Lufia2TextNextByte(memory, cpu, 0xa4b3u);
    OpSta(memory, cpu, OpAbs(cpu, 0x09cfu));
    Lufia2TextNextByte(memory, cpu, 0xa4b9u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_ITEM_QUANTITY));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_ITEM_QUANTITY + 1u));
    PushY(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    const uint32_t handoff = CallPartyService(memory, cpu, 0x82fa57u, 0xa4c6u);
    if (handoff)
        return handoff;
    OpSepWidths(cpu, 0x20u);
    OpPullY(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    return LUFIA2_TEXT_COMMAND_NEXT;
}

static uint32_t RestoreParty(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    PushY(memory, cpu);
    uint32_t handoff = CallPartyService(memory, cpu, 0x81f629u, 0xa4e2u);
    if (!handoff)
        handoff = CallPartyService(memory, cpu, 0x81f5edu, 0xa4e6u);
    if (!handoff)
        handoff = CallPartyService(memory, cpu, 0x81f60bu, 0xa4eau);
    if (handoff)
        return handoff;
    OpPullY(memory, cpu);
    PullDataBank(memory, cpu);
    return LUFIA2_TEXT_COMMAND_NEXT;
}

static uint32_t ClearEquipment(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2TextNextByte(memory, cpu, 0xa50du);
    PushDataBank(memory, cpu);
    PushY(memory, cpu);
    OpRepWidths(cpu, 0x30u);
    const uint32_t handoff = CallPartyService(memory, cpu, 0x82fd15u, 0xa515u);
    if (handoff)
        return handoff;
    OpSepWidths(cpu, 0x20u);
    OpPullY(memory, cpu);
    PullDataBank(memory, cpu);
    return LUFIA2_TEXT_COMMAND_NEXT;
}

static uint32_t TestGold(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SetTextCondition(memory, cpu, 0xae05u, false);
    Lufia2TextNextWord(memory, cpu, 0xae08u);
    OpSta(memory, cpu, OpDp(cpu, DP_OPERAND));
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_OPERAND + 1u));
    OpLda(memory, cpu, OpAbs(cpu, TEXT_GOLD_HIGH));
    if (!cpu->zero) {
        SetTextCondition(memory, cpu, 0xae20u, true);
    } else {
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbs(cpu, TEXT_GOLD_LOW));
        OpCmp(memory, cpu, OpDp(cpu, DP_OPERAND));
        OpSepWidths(cpu, 0x20u);
        if (cpu->carry)
            SetTextCondition(memory, cpu, 0xae20u, true);
    }
    return LUFIA2_TEXT_COMMAND_NEXT;
}

static uint32_t TestObjectBit(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SetTextCondition(memory, cpu, 0xae26u, false);
    Lufia2TextNextByte(memory, cpu, 0xae29u);
    const uint32_t handoff = CallPartyService(memory, cpu, 0x838ac9u, 0xae2du);
    if (handoff)
        return handoff;
    if (!cpu->zero)
        SetTextCondition(memory, cpu, 0xae32u, true);
    return LUFIA2_TEXT_COMMAND_NEXT;
}

static uint32_t SetActorDisplayOffset(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2TextNextByte(memory, cpu, 0xb07au);
    Lufia2EventFindActorId(memory, cpu, 0xb07eu);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SLOT_WORD_OFFSET)));
    Lufia2TextNextByte(memory, cpu, 0xb083u);
    Lufia2TextWidenSignedOffset(memory, cpu, 0xb086u);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_DISPLAY_OFFSET_X));
    OpSepWidths(cpu, 0x20u);
    Lufia2TextNextByte(memory, cpu, 0xb08fu);
    Lufia2TextWidenSignedOffset(memory, cpu, 0xb092u);
    cpu->carry = 1u;
    OpSbcValue(cpu, 0x10u);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_DISPLAY_OFFSET_Y));
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_UNK_7FE216));
    OpIncA(cpu);
    OpBitValue(cpu, 1u);
    if (!cpu->zero) {
        OpRepWidths(cpu, 0x20u);
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SLOT_WORD_OFFSET)));
        OpLda(memory, cpu, OpLongX(cpu, WRAM_ACTOR_DISPLAY_OFFSET_Y));
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x10u);
        OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_DISPLAY_OFFSET_Y));
        OpSepWidths(cpu, 0x20u);
    }
    return LUFIA2_TEXT_COMMAND_NEXT;
}

static uint32_t SetObjectLocation(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2TextNextByte(memory, cpu, 0xb2b3u);
    ExchangeAccumulatorBytes(cpu);
    OpLoadA(cpu, 0u);
    ExchangeAccumulatorBytes(cpu);
    cpu->carry = 1u;
    OpSbcValue(cpu, 0x10u);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E085E));
    OpCmp(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID));
    if (cpu->zero) {
        OpTxa(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x10u);
        OpPushX(memory, cpu);
        Lufia2EventFindActorId(memory, cpu, 0xb2ccu);
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
        OpOraValue(cpu, 4u);
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
        SimulateJslFrame(memory, cpu, 0x80u, 0xb2dau);
        cpu->program_bank = 0x83u;
        Lufia2ActorClearMapOccupancy(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        cpu->program_bank = 0x80u;
        OpPullX(memory, cpu);
    }
    Lufia2TextNextByte(memory, cpu, 0xb2deu);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E085E));
    Lufia2TextNextByte(memory, cpu, 0xb2e4u);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E089E));
    return LUFIA2_TEXT_COMMAND_NEXT;
}

static uint32_t RebuildPartyActors(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2TextNextByte(memory, cpu, 0xb46du);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_WINDOW_MODE));
    PushAccumulator8(memory, cpu);
    OpAndValue(cpu, 0xfeu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_WINDOW_MODE));
    PushY(memory, cpu);
    const uint32_t handoff = CallPartyService(memory, cpu, 0x848204u, 0xb47bu);
    if (handoff)
        return handoff;
    OpPullY(memory, cpu);
    LoadA8(cpu, Pull8(memory, cpu));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_WINDOW_MODE));
    return LUFIA2_TEXT_COMMAND_NEXT;
}

uint32_t Lufia2TextExecutePartyCommand(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                       uint16_t handler) {
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal ||
        cpu->stack < 0x1f10u || cpu->stack > 0x1ffcu)
        return 0u;
    switch (handler) {
    case REMOVE_ITEM_PAGE_ZERO:
        return RemoveItem(memory, cpu, false);
    case REMOVE_ITEM_PAGE_ONE:
        return RemoveItem(memory, cpu, true);
    case RESTORE_PARTY:
        return RestoreParty(memory, cpu);
    case CLEAR_EQUIPMENT:
        return ClearEquipment(memory, cpu);
    case REBUILD_PARTY_ACTORS:
        return RebuildPartyActors(memory, cpu);
    case BRANCH_ON_FLAG:
        return BranchOnFlag(memory, cpu);
    case LEARN_SPELL:
        return LearnSpell(memory, cpu);
    case RESET_MEMBER_MARKER:
        return ResetMemberMarker(memory, cpu);
    case INCREASE_MEMBER_STAT:
        return IncreaseMemberStat(memory, cpu);
    case ADD_PARTY_MEMBER:
        return ChangePartyMembership(memory, cpu, true);
    case REMOVE_PARTY_MEMBER:
        return ChangePartyMembership(memory, cpu, false);
    case REVEAL_ACTOR:
        return RevealActor(memory, cpu);
    case TEST_ITEM_COUNT:
        return TestItemCount(memory, cpu);
    case TEST_GOLD:
        return TestGold(memory, cpu);
    case TEST_OBJECT_BIT:
        return TestObjectBit(memory, cpu);
    case SET_ACTOR_DISPLAY_OFFSET:
        return SetActorDisplayOffset(memory, cpu);
    case SET_OBJECT_LOCATION:
        return SetObjectLocation(memory, cpu);
    default:
        return 0u;
    }
}
