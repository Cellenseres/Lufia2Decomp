#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/board_game.h"
#include "system/wram.h"

enum {
    SESSION_CELLS = 0x70b8u,
    SESSION_CELL_LOCATIONS = 0x70eau,
    SESSION_CELL_GROUPS = 0x711cu,
    SESSION_DRAW_ORDER = 0x714eu,
    SESSION_SELECTED_GROUP = WRAM_BOARD_GAME_SELECTED_GROUP & 0xffffu,
    SESSION_DRAW_ORIGIN = WRAM_BOARD_GAME_DRAW_ORIGIN & 0xffffu,
    SESSION_DRAW_TIMER = WRAM_BOARD_GAME_DRAW_TIMER & 0xffffu,
    SESSION_DRAW_ANGLE = WRAM_BOARD_GAME_DRAW_ANGLE & 0xffffu,
    SESSION_DRAW_PHASE = WRAM_BOARD_GAME_DRAW_PHASE & 0xffffu,
    SESSION_FALL_STEP = WRAM_BOARD_GAME_DRAW_FALL_STEP & 0xffffu,
    SESSION_CONFIRMED = WRAM_BOARD_GAME_DRAW_CONFIRMED & 0xffffu,
    SESSION_RADIUS = WRAM_BOARD_GAME_DRAW_RADIUS & 0xffffu,
    SESSION_DRAW_OFFSET = WRAM_BOARD_GAME_DRAW_OFFSET & 0xffffu,
    SESSION_DRAWS_LEFT = WRAM_BOARD_GAME_DRAWS_REMAINING & 0xffffu,
    SESSION_SPECIAL_DRAWS = WRAM_BOARD_GAME_SPECIAL_DRAWS & 0xffffu,
    SESSION_COMPLETED = WRAM_BOARD_GAME_COMPLETED_LINES & 0xffffu,
    SESSION_NEAR = WRAM_BOARD_GAME_NEAR_LINES & 0xffffu,
    SESSION_OAM = WRAM_FIELD_OAM_LOW_BUFFER & 0xffffu,
    SESSION_REPLAY_CURSOR = SESSION_OAM + 127u * 4u,
    SESSION_WINDOW = WRAM_SPECIAL_MENU_WINDOW_PARAMETERS & 0xffffu,
    SESSION_BALANCE = WRAM_MENU_EXCHANGE_BALANCE & 0xffffu,
    SESSION_CALL_LIMIT = 65536u
};

typedef struct BoardSession {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
    unsigned calls;
} BoardSession;

static bool SessionReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x84u && !cpu->index_is_8_bit &&
           !cpu->direct_page && !cpu->decimal;
}

static bool SessionCall(BoardSession *session, uint32_t site, uint32_t target) {
    Lufia2CpuState *cpu = session->cpu;
    const bool byte_accumulator = cpu->accumulator_is_8_bit != 0u;
    const uint8_t frame = (target >> 16) == 0x84u ? 2u : 3u;
    if (session->calls++ == SESSION_CALL_LIMIT) {
        session->result = ExecutionHandoff(cpu, site);
        return false;
    }
    if (!CallChildWithFrame(session->memory, cpu, session->child,
                            session->context, site, target, frame, 0x84u)) {
        session->result = ExecutionReturned(site);
        session->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (!SessionReady(cpu) ||
        cpu->accumulator_is_8_bit != byte_accumulator) {
        session->result = ExecutionHandoff(cpu, site + frame + 1u);
        return false;
    }
    return true;
}

Lufia2ExecutionResult Lufia2BoardGameChooseReplay(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SessionReady(cpu))
        return ExecutionHandoff(cpu, 0x84caf1u);
    BoardSession session = {memory, cpu, child, context,
                            ExecutionReturned(0x84cb63u), 0u};
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, 0xc020u);
    OpWriteX(memory, cpu, OpAbs(cpu, SESSION_REPLAY_CURSOR), cpu->x);
    OpLdx(cpu, 0x3504u);
    OpWriteX(memory, cpu, OpAbs(cpu, SESSION_REPLAY_CURSOR + 2u), cpu->x);
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLoadA(cpu, 10u);
    OpSta(memory, cpu, OpDp(cpu, 2u));
    for (;;) {
        OpStepMem(memory, cpu, OpDp(cpu, 2u), -1);
        if (cpu->negative) {
            OpLoadA(cpu, 10u);
            OpSta(memory, cpu, OpDp(cpu, 2u));
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpAbs(cpu, SESSION_REPLAY_CURSOR + 2u));
            OpIncA(cpu);
            OpCmpValue(cpu, 0x3508u);
            if (cpu->zero)
                OpLoadA(cpu, 0x3504u);
            OpSta(memory, cpu, OpAbs(cpu, SESSION_REPLAY_CURSOR + 2u));
            OpSepWidths(cpu, 0x20u);
        }
        if (!SessionCall(&session, 0x84cb23u, 0x84cbcdu) ||
            !SessionCall(&session, 0x84cb26u, 0x849f1au))
            return session.result;
        OpCmpValue(cpu, 1u);
        if (cpu->zero) {
            OpLdx(cpu, 0xf000u);
            OpWriteX(memory, cpu, OpAbs(cpu, SESSION_REPLAY_CURSOR), cpu->x);
            OpLda(memory, cpu, OpDp(cpu, 0u));
            OpLoadA(cpu, cpu->zero ? 1u : 0u);
            break;
        }
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            OpLoadA(cpu, 0u);
            break;
        }
        ExchangeAccumulatorBytes(cpu);
        OpAndValue(cpu, 12u);
        if (cpu->zero)
            continue;
        OpLdx(cpu, 0xc020u);
        OpLda(memory, cpu, OpDp(cpu, 0u));
        OpIncA(cpu);
        OpAndValue(cpu, 1u);
        OpSta(memory, cpu, OpDp(cpu, 0u));
        if (!cpu->zero)
            OpLdx(cpu, 0xc048u);
        OpWriteX(memory, cpu, OpAbs(cpu, SESSION_REPLAY_CURSOR), cpu->x);
    }
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLdx(cpu, 0xf000u);
    OpWriteX(memory, cpu, OpAbs(cpu, SESSION_REPLAY_CURSOR), cpu->x);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return session.result;
}

Lufia2ExecutionResult Lufia2BoardGameContinueRound(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SessionReady(cpu))
        return ExecutionHandoff(cpu, 0x84cb64u);
    BoardSession session = {memory, cpu, child, context,
                            ExecutionReturned(0x84cbccu), 0u};
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 14u);
    OpWriteX(memory, cpu, OpAbs(cpu, SESSION_DRAWS_LEFT), cpu->x);
    if (!SessionCall(&session, 0x84cb6du, 0x84ccd3u) ||
        !SessionCall(&session, 0x84cb70u, 0x84d17au))
        return session.result;
    do {
        if (!SessionCall(&session, 0x84cb73u, 0x84cbcdu) ||
            !SessionCall(&session, 0x84cb76u, 0x84cda8u))
            return session.result;
        OpLda(memory, cpu, OpDp(cpu, 0x30u));
    } while (!cpu->zero);
    if (!SessionCall(&session, 0x84cb7du, 0x84a39bu) ||
        !SessionCall(&session, 0x84cb80u, 0x84a2b3u) ||
        !SessionCall(&session, 0x84cb83u, 0x84d17au) ||
        !SessionCall(&session, 0x84cb86u, 0x84cbcdu))
        return session.result;
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, SESSION_COMPLETED)));
    OpWriteX(memory, cpu, OpDp(cpu, 0x22u), cpu->x);
    if (!SessionCall(&session, 0x84cb8eu, 0x84cbe2u))
        return session.result;
    OpLda(memory, cpu, OpDp(cpu, 0x22u));
    OpCmp(memory, cpu, OpAbs(cpu, SESSION_COMPLETED));
    if (!cpu->negative) {
        OpStz(memory, cpu, OpAbs(cpu, SESSION_COMPLETED));
    } else {
        if (!SessionCall(&session, 0x84cb9du, 0x84a39bu))
            return session.result;
        OpLoadA(cpu, 5u);
        OpSta(memory, cpu, OpAbs(cpu, SESSION_WINDOW + 6u));
        if (!SessionCall(&session, 0x84cba6u, 0x84a2b3u))
            return session.result;
        OpLoadA(cpu, 0x28u);
        if (!SessionCall(&session, 0x84cbacu, 0x84a607u))
            return session.result;
        OpLda(memory, cpu, OpAbs(cpu, SESSION_COMPLETED));
        OpSta(memory, cpu, OpDp(cpu, 0u));
        OpLdy(cpu, 0xfabcu);
        if (!SessionCall(&session, 0x84cbb7u, 0x808878u))
            return session.result;
        OpLoadA(cpu, 8u);
        OpSta(memory, cpu, OpDp(cpu, 0x74u));
        do {
            if (!SessionCall(&session, 0x84cbc0u, 0x84cbcdu) ||
                !SessionCall(&session, 0x84cbc3u, 0x849f1au))
                return session.result;
            OpCmpValue(cpu, 0u);
        } while (cpu->zero);
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
    return session.result;
}

Lufia2ExecutionResult Lufia2BoardGameDrawNextCell(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SessionReady(cpu))
        return ExecutionHandoff(cpu, 0x84ccd3u);
    BoardSession session = {memory, cpu, child, context,
                            ExecutionReturned(0x84cd5eu), 0u};
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x84u);
    OpSta(memory, cpu, OpDp(cpu, 0x0au));
    OpLdx(cpu, 0xda69u);
    OpWriteX(memory, cpu, OpDp(cpu, 8u), cpu->x);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, SESSION_DRAWS_LEFT));
    if (!cpu->zero) {
        OpLoadA(cpu, 0x0200u);
        OpSta(memory, cpu, OpAbs(cpu, SESSION_DRAW_ORIGIN));
        OpLoadA(cpu, 14u);
        OpSta(memory, cpu, OpAbs(cpu, SESSION_DRAW_TIMER));
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, OpAbs(cpu, SESSION_DRAW_PHASE));
        OpLda(memory, cpu, OpAbs(cpu, SESSION_DRAWS_LEFT));
        cpu->carry = 0u;
        RolA16(cpu);
        OpTay(cpu);
        OpLda(memory, cpu, OpAbsY(cpu, SESSION_DRAW_ORDER));
        OpSta(memory, cpu, OpDp(cpu, 0u));
        OpTax(cpu);
        OpCmp(memory, cpu, OpAbs(cpu, SESSION_SPECIAL_DRAWS));
        bool special = cpu->zero != 0u;
        if (!special) {
            OpCmp(memory, cpu, OpAbs(cpu, SESSION_SPECIAL_DRAWS + 2u));
            special = cpu->zero != 0u;
        }
        OpLoadA(cpu, special ? 0x2800u : 0x2400u);
        OpSta(memory, cpu, OpDp(cpu, 0x2au));
        OpLoadA(cpu, special ? 0x1000u : 0x0c00u);
        OpSta(memory, cpu, OpDp(cpu, 0x1fu));
        OpLoadA(cpu, 0x90e8u);
        OpSta(memory, cpu, OpAbs(cpu, SESSION_OAM));
        OpTxa(cpu);
        cpu->carry = 0u;
        RolA16(cpu);
        OpTay(cpu);
        OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, 8u));
        OpOra(memory, cpu, OpDp(cpu, 0x2au));
        OpSta(memory, cpu, OpAbs(cpu, SESSION_OAM + 2u));
        OpStepMem(memory, cpu, OpDp(cpu, 0x30u), 1);
        OpStepMem(memory, cpu, OpAbs(cpu, SESSION_DRAWS_LEFT), -1);
        OpLda(memory, cpu, OpDp(cpu, 0u));
        cpu->carry = 0u;
        RolA16(cpu);
        OpTay(cpu);
        OpSta(memory, cpu, OpAbs(cpu, SESSION_DRAW_OFFSET));
        OpLda(memory, cpu, OpAbsY(cpu, SESSION_CELL_LOCATIONS));
        OpTay(cpu);
        OpLda(memory, cpu, OpAbsY(cpu, SESSION_CELLS));
        OpAndValue(cpu, 0xffu);
        OpOra(memory, cpu, OpDp(cpu, 0x1fu));
        OpSta(memory, cpu, OpAbsY(cpu, SESSION_CELLS));
        OpLda(memory, cpu, OpDp(cpu, 0x1fu));
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x0800u);
        OpSta(memory, cpu, OpDp(cpu, 0x1fu));
        OpLoadA(cpu, 0x14u);
        if (!SessionCall(&session, 0x84cd5au, 0x84a607u))
            return session.result;
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
    return session.result;
}

static void AddPackedDrawPosition(const Lufia2Memory *memory,
                                  Lufia2CpuState *cpu, uint32_t offset) {
    OpSepWidths(cpu, 0x20u);
    cpu->carry = 0u;
    OpAdc(memory, cpu, offset);
    ExchangeAccumulatorBytes(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, offset + 1u);
    ExchangeAccumulatorBytes(cpu);
    OpRepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, SESSION_OAM));
}

static bool ProjectDrawOrbit(BoardSession *session) {
    const Lufia2Memory *memory = session->memory;
    Lufia2CpuState *cpu = session->cpu;
    OpLda(memory, cpu, OpAbs(cpu, SESSION_DRAW_ANGLE));
    if (!SessionCall(session, 0x84cde8u, 0x808450u) ||
        !SessionCall(session, 0x84cdecu, 0x84ceb3u))
        return false;
    cpu->carry = 0u;
    RorA16(cpu);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x22u)));
    if (!cpu->zero) {
        LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffu));
        OpIncA(cpu);
    }
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpDp(cpu, 0x24u));
    OpLda(memory, cpu, OpAbs(cpu, SESSION_DRAW_ANGLE));
    if (!SessionCall(session, 0x84cdffu, 0x808486u) ||
        !SessionCall(session, 0x84ce03u, 0x84ceb3u))
        return false;
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x22u)));
    if (!cpu->zero) {
        LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffu));
        OpIncA(cpu);
    }
    OpOra(memory, cpu, OpDp(cpu, 0x24u));
    AddPackedDrawPosition(memory, cpu, OpAbs(cpu, SESSION_DRAW_ORIGIN));
    return true;
}

static void FinishDrawAnimation(const Lufia2Memory *memory,
                                 Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, SESSION_DRAW_PHASE));
    if (!cpu->negative)
        return;
    OpLda(memory, cpu, OpAbs(cpu, SESSION_DRAW_TIMER));
    if (!cpu->zero)
        return;
    OpLda(memory, cpu, OpAbs(cpu, SESSION_DRAW_OFFSET));
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, SESSION_CELL_LOCATIONS));
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, SESSION_CELLS));
    OpAndValue(cpu, 0xffu);
    OpOra(memory, cpu, OpDp(cpu, 0x1fu));
    OpSta(memory, cpu, OpAbsX(cpu, SESSION_CELLS));
    OpStepMem(memory, cpu, OpDp(cpu, 0x30u), -1);
    OpLoadA(cpu, 0xf000u);
    OpSta(memory, cpu, OpAbs(cpu, SESSION_OAM));
}

Lufia2ExecutionResult Lufia2BoardGameUpdateDrawAnimation(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SessionReady(cpu))
        return ExecutionHandoff(cpu, 0x84cda8u);
    BoardSession session = {memory, cpu, child, context,
                            ExecutionReturned(0x84ceb2u), 0u};
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, 0x30u));
    if (!cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, SESSION_DRAW_PHASE));
        bool move_sprite = false;
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, SESSION_DRAW_ORIGIN));
            OpSta(memory, cpu, OpDp(cpu, 0x24u));
            OpStepMem(memory, cpu, OpAbs(cpu, SESSION_DRAW_TIMER), -1);
            if (!cpu->negative) {
                move_sprite = true;
            } else {
                OpStepMem(memory, cpu, OpAbs(cpu, SESSION_DRAW_PHASE), -1);
                OpLoadA(cpu, 0u);
                OpSta(memory, cpu, OpAbs(cpu, SESSION_DRAW_ANGLE));
                OpSta(memory, cpu, OpAbs(cpu, SESSION_FALL_STEP));
                OpSta(memory, cpu, OpAbs(cpu, SESSION_CONFIRMED));
                OpLoadA(cpu, 40u);
                OpSta(memory, cpu, OpAbs(cpu, SESSION_RADIUS));
                OpLoadA(cpu, 346u);
                OpSta(memory, cpu, OpAbs(cpu, SESSION_DRAW_TIMER));
                OpLoadA(cpu, 0xaec0u);
                OpSta(memory, cpu, OpAbs(cpu, SESSION_DRAW_ORIGIN));
            }
        }
        if (!move_sprite) {
            if (!ProjectDrawOrbit(&session))
                return session.result;
            OpLda(memory, cpu, OpAbs(cpu, SESSION_FALL_STEP));
            cpu->carry = 0u;
            OpAdc(memory, cpu, OpAbs(cpu, SESSION_DRAW_ANGLE));
            OpIncA(cpu);
            OpIncA(cpu);
            OpCmpValue(cpu, 179u);
            if (!cpu->negative) {
                OpStepMem(memory, cpu, OpAbs(cpu, SESSION_FALL_STEP), 1);
                OpLda(memory, cpu, OpAbs(cpu, SESSION_RADIUS));
                cpu->carry = 1u;
                OpSbcValue(cpu, OpReadM(memory, cpu,
                                      OpAbs(cpu, SESSION_FALL_STEP)));
                OpSta(memory, cpu, OpAbs(cpu, SESSION_RADIUS));
                OpSepWidths(cpu, 0x20u);
                OpLda(memory, cpu, OpAbs(cpu, SESSION_FALL_STEP));
                cpu->carry = 0u;
                OpAdc(memory, cpu, OpAbs(cpu, SESSION_DRAW_ORIGIN + 1u));
                OpSta(memory, cpu, OpAbs(cpu, SESSION_DRAW_ORIGIN + 1u));
                OpRepWidths(cpu, 0x20u);
                OpLoadA(cpu, 0u);
            }
            OpSta(memory, cpu, OpAbs(cpu, SESSION_DRAW_ANGLE));
            OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, SESSION_CONFIRMED)));
            if (!cpu->zero) {
                OpCmpValue(cpu, 40u);
                if (!cpu->negative) {
                    OpCmpValue(cpu, 50u);
                    if (cpu->negative)
                        OpStz(memory, cpu, OpAbs(cpu, SESSION_DRAW_TIMER));
                }
            }
            OpStepMem(memory, cpu, OpAbs(cpu, SESSION_DRAW_TIMER), -1);
            if (cpu->negative) {
                OpStepMem(memory, cpu, OpAbs(cpu, SESSION_DRAW_PHASE), -1);
                OpLoadA(cpu, 0x0200u);
                OpSta(memory, cpu, OpAbs(cpu, SESSION_DRAW_ORIGIN));
                OpSta(memory, cpu, OpDp(cpu, 0x24u));
                OpLoadA(cpu, 16u);
                OpSta(memory, cpu, OpAbs(cpu, SESSION_DRAW_TIMER));
                move_sprite = true;
            }
        }
        if (move_sprite) {
            OpLda(memory, cpu, OpAbs(cpu, SESSION_OAM));
            AddPackedDrawPosition(memory, cpu, OpDp(cpu, 0x24u));
            FinishDrawAnimation(memory, cpu);
        }
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
    return session.result;
}

static bool InitializeBoardSession(BoardSession *session) {
    const Lufia2Memory *memory = session->memory;
    Lufia2CpuState *cpu = session->cpu;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, 0x0583u));
    OpRepWidths(cpu, 0x20u);
    PushDataBank(memory, cpu);
    for (unsigned palette = 0u; palette < 2u; ++palette) {
        OpLdx(cpu, 0xee20u);
        OpLdy(cpu, palette ? 0x0420u : 0x0320u);
        OpLoadA(cpu, 255u);
        OpMoveNext(memory, cpu, 0u, 0xa6u);
    }
    PullDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, 0x73u));
    if (!SessionCall(session, 0x84c84fu, 0x84a5a8u) ||
        !SessionCall(session, 0x84c852u, 0x84a4ddu) ||
        !SessionCall(session, 0x84c855u, 0x84a4fdu))
        return false;
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0x0298u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x54u), cpu->x);
    OpLoadA(cpu, 0x2000u);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLoadA(cpu, 0x2000u);
    OpSta(memory, cpu, OpDp(cpu, 2u));
    if (!SessionCall(session, 0x84c869u, 0x84a569u))
        return false;
    OpLdx(cpu, 0x0299u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x54u), cpu->x);
    OpLoadA(cpu, 0x4000u);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLoadA(cpu, 0x4000u);
    OpSta(memory, cpu, OpDp(cpu, 2u));
    if (!SessionCall(session, 0x84c87bu, 0x84a569u))
        return false;
    OpSepWidths(cpu, 0x20u);
    if (!SessionCall(session, 0x84c880u, 0x84d70au))
        return false;
    OpLoadA(cpu, 0x24u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    OpLoadA(cpu, 0x84u);
    OpSta(memory, cpu, OpDp(cpu, 0x5fu));
    OpLdx(cpu, 0x3682u);
    OpLdy(cpu, 0xf945u);
    if (!SessionCall(session, 0x84c892u, 0x808878u))
        return false;
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, 0x74u));
    OpSta(memory, cpu, OpDp(cpu, 0x72u));
    if (!SessionCall(session, 0x84c89cu, 0x84a5a8u))
        return false;
    OpLoadA(cpu, 0x28u);
    OpSta(memory, cpu, OpDp(cpu, 0x74u));
    OpLdx(cpu, 0xffffu);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x0596u), cpu->x);
    if (!SessionCall(session, 0x84c8a9u, 0x84a5b5u))
        return false;
    PushDataBank(memory, cpu);
    OpLoadA(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    if (!SessionCall(session, 0x84c8b1u, 0x84d765u))
        return false;
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x70b4u), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, SESSION_SELECTED_GROUP), cpu->x);
    OpLdx(cpu, 8u);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x70b6u), cpu->x);
    OpLdx(cpu, 0xffffu);
    OpWriteX(memory, cpu, OpAbs(cpu,
        WRAM_BOARD_GAME_FIRST_POSITION & 0xffffu), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu,
        WRAM_BOARD_GAME_SECOND_POSITION & 0xffffu), cpu->x);
    return true;
}

static bool StartBoardRound(BoardSession *session) {
    const Lufia2Memory *memory = session->memory;
    Lufia2CpuState *cpu = session->cpu;
    if (!SessionCall(session, 0x84c8ccu, 0x84d684u))
        return false;
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SESSION_WINDOW));
    OpLoadA(cpu, 19u);
    OpSta(memory, cpu, OpAbs(cpu, SESSION_WINDOW + 2u));
    OpLoadA(cpu, 12u);
    OpSta(memory, cpu, OpAbs(cpu, SESSION_WINDOW + 4u));
    OpLoadA(cpu, 7u);
    OpSta(memory, cpu, OpAbs(cpu, SESSION_WINDOW + 6u));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbs(cpu, 0x6010u));
    OpSta(memory, cpu, OpAbs(cpu, 0x6012u));
    OpSta(memory, cpu, OpAbs(cpu, SESSION_WINDOW + 8u));
    if (!SessionCall(session, 0x84c8f5u, 0x84a2b3u))
        return false;
    OpLdy(cpu, 0xfb29u);
    if (!SessionCall(session, 0x84c8fbu, 0x808878u))
        return false;
    OpSepWidths(cpu, 0x20u);
    if (!SessionCall(session, 0x84c901u, 0x84d892u) ||
        !SessionCall(session, 0x84c904u, 0x84cd5fu))
        return false;
    OpLoadA(cpu, 0x24u);
    if (!SessionCall(session, 0x84c909u, 0x84a607u))
        return false;
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpDp(cpu, 0x74u));
    do {
        if (!SessionCall(session, 0x84c910u, 0x84d860u) ||
            !SessionCall(session, 0x84c913u, 0x84d17au) ||
            !SessionCall(session, 0x84c916u, 0x84cbcdu) ||
            !SessionCall(session, 0x84c919u, 0x849f1au))
            return false;
        OpCmpValue(cpu, 1u);
    } while (!cpu->zero);
    OpLoadA(cpu, 2u);
    if (!SessionCall(session, 0x84c922u, 0x84a607u))
        return false;
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpSta(memory, cpu, OpDp(cpu, 2u));
    OpLdx(cpu, 5u);
    OpWriteX(memory, cpu, OpDp(cpu, 4u), cpu->x);
    do {
        OpLoadA(cpu, 0x25u);
        if (!SessionCall(session, 0x84c932u, 0x84a607u) ||
            !SessionCall(session, 0x84c935u, 0x84d860u) ||
            !SessionCall(session, 0x84c938u, 0x84d17au))
            return false;
        do {
            if (!SessionCall(session, 0x84c93bu, 0x84cbcdu))
                return false;
            OpStepMem(memory, cpu, OpDp(cpu, 2u), -1);
        } while (!cpu->zero);
        OpLda(memory, cpu, OpDp(cpu, 0u));
        cpu->carry = 0u;
        OpAdcValue(cpu, 10u);
        OpSta(memory, cpu, OpDp(cpu, 0u));
        OpSta(memory, cpu, OpDp(cpu, 2u));
        OpStepMem(memory, cpu, OpDp(cpu, 4u), -1);
    } while (!cpu->zero);
    return SessionCall(session, 0x84c94fu, 0x84d538u) &&
           SessionCall(session, 0x84c952u, 0x84cd5fu) &&
           SessionCall(session, 0x84c955u, 0x84cbcdu) &&
           SessionCall(session, 0x84c958u, 0x84d1efu);
}

static bool RunUngroupedDraw(BoardSession *session) {
    const Lufia2Memory *memory = session->memory;
    Lufia2CpuState *cpu = session->cpu;
    for (;;) {
        if (!SessionCall(session, 0x84c99au, 0x84cbcdu) ||
            !SessionCall(session, 0x84c99du, 0x84cda8u))
            return false;
        OpLda(memory, cpu, OpDp(cpu, 0x30u));
        if (cpu->zero)
            break;
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpDp(cpu, 0x4au));
        if (!SessionCall(session, 0x84c9a8u, 0x849f1au))
            return false;
        OpCmpValue(cpu, 1u);
        if (cpu->zero) {
            OpLdx(cpu, 1u);
            OpWriteX(memory, cpu, OpAbs(cpu, SESSION_CONFIRMED), cpu->x);
        }
    }
    return SessionCall(session, 0x84c9b7u, 0x84d17au);
}

static bool RunGroupedDraw(BoardSession *session) {
    const Lufia2Memory *memory = session->memory;
    Lufia2CpuState *cpu = session->cpu;
    OpSta(memory, cpu, OpAbs(cpu, SESSION_SELECTED_GROUP));
    if (!SessionCall(session, 0x84c9bfu, 0x84cee4u))
        return false;
    for (;;) {
        if (!SessionCall(session, 0x84c9c2u, 0x84cbcdu) ||
            !SessionCall(session, 0x84c9c5u, 0x84cda8u))
            return false;
        OpLda(memory, cpu, OpDp(cpu, 0x30u));
        if (cpu->zero)
            break;
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpDp(cpu, 0x4au));
        if (!SessionCall(session, 0x84c9d0u, 0x849f1au))
            return false;
        OpCmpValue(cpu, 1u);
        if (cpu->zero) {
            OpLdx(cpu, 1u);
            OpWriteX(memory, cpu, OpAbs(cpu, SESSION_CONFIRMED), cpu->x);
        }
        ExchangeAccumulatorBytes(cpu);
        OpAndValue(cpu, 15u);
        if (!cpu->zero && !SessionCall(session, 0x84c9e2u, 0x84d016u))
            return false;
    }
    return SessionCall(session, 0x84c9e7u, 0x84cf91u) &&
           SessionCall(session, 0x84c9eau, 0x84d17au) &&
           SessionCall(session, 0x84c9edu, 0x84cbcdu);
}

static bool RunBoardDraws(BoardSession *session) {
    const Lufia2Memory *memory = session->memory;
    Lufia2CpuState *cpu = session->cpu;
    for (;;) {
        if (!SessionCall(session, 0x84c95bu, 0x84cbcdu) ||
            !SessionCall(session, 0x84c95eu, 0x849f1au) ||
            !SessionCall(session, 0x84c961u, 0x84a39bu))
            return false;
        OpLoadA(cpu, 7u);
        OpSta(memory, cpu, OpAbs(cpu, SESSION_WINDOW + 6u));
        if (!SessionCall(session, 0x84c969u, 0x84a2b3u))
            return false;
        OpLdy(cpu, 0xfa98u);
        if (!SessionCall(session, 0x84c96fu, 0x808878u))
            return false;
        OpLoadA(cpu, 8u);
        OpSta(memory, cpu, OpDp(cpu, 0x74u));
        OpLda(memory, cpu, OpAbs(cpu, SESSION_DRAWS_LEFT));
        if (cpu->zero)
            return true;
        if (!SessionCall(session, 0x84c97fu, 0x84ccd3u) ||
            !SessionCall(session, 0x84c982u, 0x84d17au) ||
            !SessionCall(session, 0x84c985u, 0x84cd5fu))
            return false;
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbs(cpu, SESSION_DRAW_OFFSET));
        OpTax(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, SESSION_CELL_LOCATIONS));
        OpTax(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, SESSION_CELL_GROUPS));
        OpDecA(cpu);
        OpSepWidths(cpu, 0x20u);
        if (cpu->negative) {
            if (!RunUngroupedDraw(session))
                return false;
        } else if (!RunGroupedDraw(session)) {
            return false;
        }
    }
}

static bool WaitForBoardInput(BoardSession *session,
                              uint32_t frame_site, uint32_t input_site) {
    do {
        if (!SessionCall(session, frame_site, 0x84cbcdu) ||
            !SessionCall(session, input_site, 0x849f1au))
            return false;
        OpCmpValue(session->cpu, 0u);
    } while (session->cpu->zero);
    return true;
}

static bool ShowBoardLoss(BoardSession *session) {
    const Lufia2Memory *memory = session->memory;
    Lufia2CpuState *cpu = session->cpu;
    if (!SessionCall(session, 0x84ca67u, 0x84a39bu))
        return false;
    OpLoadA(cpu, 5u);
    OpSta(memory, cpu, OpAbs(cpu, SESSION_WINDOW + 6u));
    if (!SessionCall(session, 0x84ca6fu, 0x84a2b3u))
        return false;
    OpLoadA(cpu, 0x29u);
    if (!SessionCall(session, 0x84ca74u, 0x84a607u))
        return false;
    OpLdy(cpu, 0xfb0fu);
    if (!SessionCall(session, 0x84ca7au, 0x808878u))
        return false;
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpDp(cpu, 0x74u));
    return true;
}

static bool FinishBoardRound(BoardSession *session) {
    const Lufia2Memory *memory = session->memory;
    Lufia2CpuState *cpu = session->cpu;
    if (!SessionCall(session, 0x84c9f3u, 0x84cbcdu) ||
        !SessionCall(session, 0x84c9f6u, 0x84cbe2u))
        return false;
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, SESSION_COMPLETED)));
    bool lost = cpu->zero != 0u;
    if (!lost) {
        if (!SessionCall(session, 0x84c9feu, 0x84a39bu))
            return false;
        OpLoadA(cpu, 5u);
        OpSta(memory, cpu, OpAbs(cpu, SESSION_WINDOW + 6u));
        if (!SessionCall(session, 0x84ca06u, 0x84a2b3u))
            return false;
        OpLoadA(cpu, 0x28u);
        if (!SessionCall(session, 0x84ca0bu, 0x84a607u))
            return false;
        OpLda(memory, cpu, OpAbs(cpu, SESSION_COMPLETED));
        OpSta(memory, cpu, OpDp(cpu, 0u));
        OpLdy(cpu, 0xfabcu);
        if (!SessionCall(session, 0x84ca16u, 0x808878u))
            return false;
        OpLoadA(cpu, 8u);
        OpSta(memory, cpu, OpDp(cpu, 0x74u));
        if (!WaitForBoardInput(session, 0x84ca1eu, 0x84ca21u))
            return false;
        OpLoadA(cpu, 2u);
        if (!SessionCall(session, 0x84ca2au, 0x84a607u))
            return false;
        OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, SESSION_NEAR)));
        if (!cpu->zero) {
            if (!SessionCall(session, 0x84ca32u, 0x84a39bu))
                return false;
            OpLoadA(cpu, 7u);
            OpSta(memory, cpu, OpAbs(cpu, SESSION_WINDOW + 6u));
            if (!SessionCall(session, 0x84ca3au, 0x84a2b3u))
                return false;
            OpLdx(cpu, 0x3504u);
            OpLdy(cpu, 0xfaddu);
            if (!SessionCall(session, 0x84ca43u, 0x808878u))
                return false;
            OpLoadA(cpu, 8u);
            OpSta(memory, cpu, OpDp(cpu, 0x74u));
            if (!SessionCall(session, 0x84ca4bu, 0x84caf1u))
                return false;
            OpLda(memory, cpu, OpDp(cpu, 0u));
            if (!cpu->zero) {
                if (!SessionCall(session, 0x84ca52u, 0x84cb64u))
                    return false;
                OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, SESSION_COMPLETED)));
                lost = cpu->zero != 0u;
                if (!lost) {
                    OpRepWidths(cpu, 0x20u);
                    OpTxa(cpu);
                    OpLoadA(cpu, 5u);
                    OpSta(memory, cpu, OpAbs(cpu, 0x69e2u));
                    OpSepWidths(cpu, 0x20u);
                }
            }
        }
    }
    if (lost) {
        if (!ShowBoardLoss(session))
            return false;
    } else if (!SessionCall(session, 0x84ca8eu, 0x84d456u)) {
        return false;
    }
    return WaitForBoardInput(session, 0x84ca82u, 0x84ca85u);
}

static bool ChooseNextBoardRound(BoardSession *session, bool *play_again) {
    const Lufia2Memory *memory = session->memory;
    Lufia2CpuState *cpu = session->cpu;
    *play_again = false;
    if (!SessionCall(session, 0x84ca93u, 0x84a39bu))
        return false;
    OpLoadA(cpu, 7u);
    OpSta(memory, cpu, OpAbs(cpu, SESSION_WINDOW + 6u));
    if (!SessionCall(session, 0x84ca9bu, 0x84a2b3u))
        return false;
    OpLdx(cpu, 0x3504u);
    OpLdy(cpu, 0xfa55u);
    if (!SessionCall(session, 0x84caa4u, 0x808878u))
        return false;
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpDp(cpu, 0x74u));
    if (!SessionCall(session, 0x84caacu, 0x84caf1u))
        return false;
    OpLda(memory, cpu, OpDp(cpu, 0u));
    if (cpu->zero)
        return true;
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, SESSION_BALANCE));
    OpOra(memory, cpu, OpAbs(cpu, SESSION_BALANCE + 2u));
    OpSepWidths(cpu, 0x20u);
    if (!cpu->zero) {
        if (!SessionCall(session, 0x84cae9u, 0x84a39bu))
            return false;
        *play_again = true;
        return true;
    }
    if (!SessionCall(session, 0x84cabfu, 0x84a39bu))
        return false;
    OpLoadA(cpu, 7u);
    OpSta(memory, cpu, OpAbs(cpu, SESSION_WINDOW + 6u));
    if (!SessionCall(session, 0x84cac7u, 0x84a2b3u))
        return false;
    OpLdx(cpu, 0x3504u);
    OpLdy(cpu, 0xfa0au);
    if (!SessionCall(session, 0x84cad0u, 0x808878u))
        return false;
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpDp(cpu, 0x74u));
    if (!WaitForBoardInput(session, 0x84cad8u, 0x84cadbu))
        return false;
    OpLoadA(cpu, 2u);
    return SessionCall(session, 0x84cae4u, 0x84a607u);
}

Lufia2ExecutionResult Lufia2BoardGameRun(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SessionReady(cpu))
        return ExecutionHandoff(cpu, 0x84c826u);
    BoardSession session = {memory, cpu, child, context,
                            ExecutionReturned(0x84caf0u), 0u};
    if (!InitializeBoardSession(&session))
        return session.result;
    bool play_again;
    do {
        if (!StartBoardRound(&session) || !RunBoardDraws(&session) ||
            !FinishBoardRound(&session) ||
            !ChooseNextBoardRound(&session, &play_again))
            return session.result;
    } while (play_again);
    PullDataBank(memory, cpu);
    return session.result;
}
