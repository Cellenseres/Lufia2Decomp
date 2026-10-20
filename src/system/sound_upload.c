#include "core/cpu_ops.h"
#include "lufia2/system.h"
#include "lufia2/sound_phase.h"
#include "lufia2/sound_source.h"
#include "lufia2/sound_progress.h"
#include "core/wram_view.h"
#include "system/wram.h"

enum {
    SOUND_UPLOAD_LENGTH = 0x56u,
    SOUND_UPLOAD_BASE = 0x5du,
    SOUND_UPLOAD_BANK = 0x5fu,
    SOUND_UPLOAD_LOW = 0x60u,
    SOUND_UPLOAD_HIGH = 0x61u,
    SOUND_UPLOAD_SEQUENCE = 0x62u,
    SOUND_UPLOAD_TABLE = 0x988000u,
    SOUND_UPLOAD_DATA_PORT = 0x002140u,
    SOUND_UPLOAD_HIGH_PORT = 0x002141u,
    SOUND_UPLOAD_REPLY_PORT = 0x002142u,
    SOUND_UPLOAD_MODE_PORT = 0x002143u,
    SOUND_UPLOAD_POLL_LIMIT = 65536u
};

Lufia2SoundHeaderProgress Lufia2SoundAdvanceHeader(
    const Lufia2Memory *memory, uint16_t direct_page,
    uint16_t source, uint16_t remaining) {
    Lufia2SoundHeaderProgress progress = {(uint16_t)(source + 1u), remaining};
    if (!progress.source) {
        const Lufia2Wram wram = {memory, 0u, direct_page};
        WramStep8(wram, SOUND_UPLOAD_BANK, 1);
        progress.source = 0x8000u;
    }
    progress.remaining = (uint16_t)(remaining - 1u);
    return progress;
}

Lufia2SoundPayloadProgress Lufia2SoundAdvancePayload(
    const Lufia2Memory *memory, uint16_t direct_page, uint16_t remaining) {
    const Lufia2Wram wram = {memory, 0u, direct_page};
    WramStep8(wram, SOUND_UPLOAD_SEQUENCE, 1);
    WramStep8(wram, SOUND_UPLOAD_SEQUENCE, 1);
    const Lufia2SoundPayloadProgress progress = {
        (uint16_t)(remaining - 2u), remaining >= 3u};
    return progress;
}

static void SoundUploadAdvanceHeader(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit && !cpu->index_is_8_bit) {
        const Lufia2SoundHeaderProgress progress = Lufia2SoundAdvanceHeader(
            memory, cpu->direct_page, cpu->y, cpu->x);
        cpu->y = progress.source;
        cpu->x = progress.remaining;
        SetNz16(cpu, cpu->x);
    } else {
        OpIny(cpu);
        if (cpu->zero) {
            OpStepMem(memory, cpu, OpDp(cpu, SOUND_UPLOAD_BANK), 1);
            OpLdy(cpu, 0x8000u);
        }
        OpDex(cpu);
    }
}

Lufia2ExecutionResult Lufia2SoundHeaderStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry) {
    if (cpu->program_bank != 0x80u || cpu->accumulator_is_8_bit > 1u ||
        cpu->index_is_8_bit > 1u ||
        (entry != 0x809910u &&
         (cpu->accumulator_is_8_bit != 1u || cpu->index_is_8_bit)))
        return ExecutionHandoff(cpu, entry);
    switch (entry) {
    case 0x8098ecu:
        OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, SOUND_UPLOAD_BASE));
        return ExecutionHandoff(cpu, entry + 2u);
    case 0x8098eeu:
        OpSta(memory, cpu, OpAbs(cpu, SOUND_UPLOAD_DATA_PORT));
        return ExecutionHandoff(cpu, entry + 3u);
    case 0x8098f1u:
        OpTxa(cpu);
        break;
    case 0x8098f2u:
    case 0x80990au:
        OpSta(memory, cpu, OpAbs(cpu, SOUND_UPLOAD_REPLY_PORT));
        return ExecutionHandoff(cpu, entry + 3u);
    case 0x8098f5u:
        OpOraValue(cpu, 0x80u);
        return ExecutionHandoff(cpu, entry + 2u);
    case 0x8098f7u:
        return Lufia2SoundPollStep(memory, cpu, entry);
    case 0x8098fau:
        return Lufia2SoundPollBranchStep(cpu, entry);
    case 0x8098fcu:
        OpIny(cpu);
        break;
    case 0x8098fdu:
        return ExecutionHandoff(cpu, cpu->zero ? 0x8098ffu : 0x809904u);
    case 0x8098ffu:
        OpStepMem(memory, cpu, OpDp(cpu, SOUND_UPLOAD_BANK), 1);
        return ExecutionHandoff(cpu, entry + 2u);
    case 0x809901u:
        OpLdy(cpu, Read16Pair(memory, entry + 1u, entry + 2u));
        return ExecutionHandoff(cpu, entry + 3u);
    case 0x809904u:
        OpDex(cpu);
        break;
    case 0x809905u:
        return ExecutionHandoff(cpu, cpu->zero ? 0x809907u : 0x8098ecu);
    case 0x809907u:
        OpPullX(memory, cpu);
        break;
    case 0x809908u:
        OpLoadA(cpu, 0xffu);
        return ExecutionHandoff(cpu, entry + 2u);
    case 0x80990du:
        SimulateJsrFrame(memory, cpu, 0x990fu);
        return ExecutionHandoff(cpu, 0x809a0au);
    case 0x809910u: {
        const uint16_t low = Pull8(memory, cpu);
        const uint16_t address = (uint16_t)(low | ((uint16_t)Pull8(memory, cpu) << 8));
        return ExecutionHandoff(cpu, 0x800000u | (uint16_t)(address + 1u));
    }
    default:
        return ExecutionHandoff(cpu, entry);
    }
    return ExecutionHandoff(cpu, entry + 1u);
}

static Lufia2ExecutionResult SoundUploadUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static bool SoundUploadMatches(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                               uint32_t port) {
    for (uint32_t polls = 0u; polls < SOUND_UPLOAD_POLL_LIMIT; ++polls) {
        OpCmp(memory, cpu, port);
        if (cpu->zero)
            return true;
    }
    return false;
}

static void SoundUploadCheckpoint(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                  uint32_t pc, bool stepped) {
    if (stepped)
        EmitExecutionCheckpoint(memory, cpu, pc);
}

static bool SoundUploadAdvance(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                               uint32_t site, bool save_bank, bool stepped) {
    SoundUploadCheckpoint(memory, cpu, site - 3u, stepped);
    OpIny(cpu);
    SoundUploadCheckpoint(memory, cpu, site - 2u, stepped);
    if (!cpu->zero)
        return true;
    if (stepped)
        return false;
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    Lufia2AdvanceSoundSourceBank(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    if (save_bank)
        OpSta(memory, cpu, OpAbs(cpu, WRAM_SOUND_UPLOAD_SOURCE_BANK));
    return true;
}

static bool SoundUploadReadByte(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                uint32_t site, uint8_t destination, bool save_bank,
                                bool stepped) {
    SoundUploadCheckpoint(memory, cpu, site - 8u, stepped);
    OpLda(memory, cpu, OpAbsY(cpu, 0u));
    SoundUploadCheckpoint(memory, cpu, site - 5u, stepped);
    OpSta(memory, cpu, OpDp(cpu, destination));
    return SoundUploadAdvance(memory, cpu, site, save_bank, stepped);
}

Lufia2SoundSourcePair Lufia2SoundReadSourcePair(
    const Lufia2Memory *memory, uint8_t bank, uint16_t direct_page, uint16_t source) {
    Lufia2SoundSourcePair pair = {source, 0u, false};
    if (source > 0xfffdu)
        return pair;
    const Lufia2Wram wram = {memory, bank, direct_page};
    for (uint16_t field = SOUND_UPLOAD_LOW; field <= SOUND_UPLOAD_HIGH; ++field) {
        pair.last_byte = Read8(memory, ((uint32_t)bank << 16) | pair.next);
        WramWrite(wram, field, pair.last_byte);
        ++pair.next;
    }
    pair.complete = true;
    return pair;
}

static void SoundUploadReadPair(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                uint32_t first_site, uint32_t second_site,
                                bool save_bank) {
    if (cpu->accumulator_is_8_bit && !cpu->index_is_8_bit) {
        const Lufia2SoundSourcePair pair = Lufia2SoundReadSourcePair(
            memory, cpu->data_bank, cpu->direct_page, cpu->y);
        if (pair.complete) {
            LoadA8(cpu, pair.last_byte);
            cpu->y = pair.next;
            SetNz16(cpu, cpu->y);
            return;
        }
    }
    (void)SoundUploadReadByte(memory, cpu, first_site, SOUND_UPLOAD_LOW, save_bank,
                              false);
    (void)SoundUploadReadByte(memory, cpu, second_site, SOUND_UPLOAD_HIGH, save_bank,
                              false);
}

static void SoundUploadSendPair(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                bool stepped) {
    SoundUploadCheckpoint(memory, cpu, 0x809969u, stepped);
    OpLda(memory, cpu, OpDp(cpu, SOUND_UPLOAD_LOW));
    SoundUploadCheckpoint(memory, cpu, 0x80996bu, stepped);
    OpSta(memory, cpu, SOUND_UPLOAD_DATA_PORT);
    SoundUploadCheckpoint(memory, cpu, 0x80996fu, stepped);
    OpLda(memory, cpu, OpDp(cpu, SOUND_UPLOAD_HIGH));
    SoundUploadCheckpoint(memory, cpu, 0x809971u, stepped);
    OpSta(memory, cpu, SOUND_UPLOAD_HIGH_PORT);
    SoundUploadCheckpoint(memory, cpu, 0x809975u, stepped);
    OpLda(memory, cpu, OpDp(cpu, SOUND_UPLOAD_SEQUENCE));
    SoundUploadCheckpoint(memory, cpu, 0x809977u, stepped);
    OpAndValue(cpu, 0x7fu);
    SoundUploadCheckpoint(memory, cpu, 0x809979u, stepped);
    OpSta(memory, cpu, SOUND_UPLOAD_REPLY_PORT);
}

static bool SoundUploadNextPair(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                bool stepped) {
    if (!stepped && cpu->accumulator_is_8_bit && !cpu->index_is_8_bit) {
        const Lufia2SoundPayloadProgress progress = Lufia2SoundAdvancePayload(
            memory, cpu->direct_page, cpu->x);
        cpu->x = progress.remaining;
        cpu->carry = progress.more;
        SetNz16(cpu, cpu->x);
        return progress.more;
    }
    SoundUploadCheckpoint(memory, cpu, 0x80999du, stepped);
    OpStepMem(memory, cpu, OpDp(cpu, SOUND_UPLOAD_SEQUENCE), 1);
    SoundUploadCheckpoint(memory, cpu, 0x80999fu, stepped);
    OpStepMem(memory, cpu, OpDp(cpu, SOUND_UPLOAD_SEQUENCE), 1);
    SoundUploadCheckpoint(memory, cpu, 0x8099a1u, stepped);
    OpCpx(cpu, 3u);
    SoundUploadCheckpoint(memory, cpu, 0x8099a4u, stepped);
    OpDex(cpu);
    SoundUploadCheckpoint(memory, cpu, 0x8099a5u, stepped);
    OpDex(cpu);
    SoundUploadCheckpoint(memory, cpu, 0x8099a6u, stepped);
    return cpu->carry != 0u;
}

Lufia2ExecutionResult Lufia2SoundUploadStep(const Lufia2Memory *memory,
                                            Lufia2CpuState *cpu, uint32_t entry) {
    if (cpu->program_bank != 0x80u || cpu->accumulator_is_8_bit != 1u ||
        cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, entry);
    switch (entry) {
    case 0x809969u:
        SoundUploadSendPair(memory, cpu, true);
        return ExecutionHandoff(cpu, 0x80997du);
    case 0x80999du:
        if (SoundUploadNextPair(memory, cpu, true))
            return ExecutionHandoff(cpu, 0x809969u);
        EmitExecutionCheckpoint(memory, cpu, 0x8099a8u);
        OpLoadA(cpu, 0xffu);
        EmitExecutionCheckpoint(memory, cpu, 0x8099aau);
        OpSta(memory, cpu, SOUND_UPLOAD_MODE_PORT);
        return ExecutionHandoff(cpu, 0x8099aeu);
    case 0x80997du:
        if (!SoundUploadReadByte(memory, cpu, 0x809985u, SOUND_UPLOAD_LOW, false, true))
            return ExecutionHandoff(cpu, 0x809985u);
        break;
    case 0x809988u:
        break;
    case 0x80998bu:
        EmitExecutionCheckpoint(memory, cpu, 0x80998bu);
        OpSta(memory, cpu, OpDp(cpu, SOUND_UPLOAD_HIGH));
        if (!SoundUploadAdvance(memory, cpu, 0x809990u, false, true))
            return ExecutionHandoff(cpu, 0x809990u);
        EmitExecutionCheckpoint(memory, cpu, 0x809993u);
        OpLda(memory, cpu, OpDp(cpu, SOUND_UPLOAD_SEQUENCE));
        EmitExecutionCheckpoint(memory, cpu, 0x809995u);
        OpOraValue(cpu, 0x80u);
        return ExecutionHandoff(cpu, 0x809997u);
    default:
        return ExecutionHandoff(cpu, entry);
    }
    if (!SoundUploadReadByte(memory, cpu, 0x809990u, SOUND_UPLOAD_HIGH, false, true))
        return ExecutionHandoff(cpu, 0x809990u);
    EmitExecutionCheckpoint(memory, cpu, 0x809993u);
    OpLda(memory, cpu, OpDp(cpu, SOUND_UPLOAD_SEQUENCE));
    EmitExecutionCheckpoint(memory, cpu, 0x809995u);
    OpOraValue(cpu, 0x80u);
    return ExecutionHandoff(cpu, 0x809997u);
}

static bool SoundUploadWait(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                            Lufia2PushedChildCall child, void *context, uint32_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    return child(context, cpu, 0x809a0au, site, 2u) != 0u;
}

Lufia2ExecutionResult Lufia2SendSoundPayload(const Lufia2Memory *memory,
                                             Lufia2CpuState *cpu,
                                             Lufia2PushedChildCall child,
                                             void *context) {
    if (!child || cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x809945u);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, SOUND_UPLOAD_REPLY_PORT);
    if (!SoundUploadMatches(memory, cpu, SOUND_UPLOAD_REPLY_PORT))
        return ExecutionHandoff(cpu, 0x80994bu);
    OpStz(memory, cpu, OpDp(cpu, SOUND_UPLOAD_SEQUENCE));
    SoundUploadReadPair(memory, cpu, 0x80995bu, 0x809966u, false);
    do {
        SoundUploadSendPair(memory, cpu, false);
        SoundUploadReadPair(memory, cpu, 0x809985u, 0x809990u, false);
        OpLda(memory, cpu, OpDp(cpu, SOUND_UPLOAD_SEQUENCE));
        OpOraValue(cpu, 0x80u);
        if (!SoundUploadMatches(memory, cpu, SOUND_UPLOAD_REPLY_PORT))
            return ExecutionHandoff(cpu, 0x809997u);
    } while (SoundUploadNextPair(memory, cpu, false));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, SOUND_UPLOAD_MODE_PORT);
    if (!SoundUploadWait(memory, cpu, child, context, 0x8099aeu))
        return SoundUploadUnwound(0x8099aeu);
    return ExecutionReturned(0x8099b1u);
}

Lufia2ExecutionResult Lufia2SendSoundResourceHeader(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu,
                                                    Lufia2PushedChildCall child,
                                                    void *context) {
    if (!child || cpu->program_bank != 0x80u || cpu->decimal)
        return ExecutionHandoff(cpu, 0x8098a5u);
    SetAccumulatorWidth(cpu, 1u);
    SetIndexWidth(cpu, 0u);
    ExchangeAccumulatorBytes(cpu);
    OpLoadA(cpu, 0u);
    ExchangeAccumulatorBytes(cpu);
    SetAccumulatorWidth(cpu, 0u);
    OpSta(memory, cpu, OpDp(cpu, SOUND_UPLOAD_LENGTH));
    OpAslA(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, SOUND_UPLOAD_LENGTH));
    OpTax(cpu);
    SetAccumulatorWidth(cpu, 1u);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpStz(memory, cpu, OpDp(cpu, SOUND_UPLOAD_BANK));
    SetAccumulatorWidth(cpu, 0u);
    OpStz(memory, cpu, OpDp(cpu, SOUND_UPLOAD_BASE));
    OpLda(memory, cpu, OpLongX(cpu, SOUND_UPLOAD_TABLE));
    OpAslA(cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpLsrA(cpu);
    OpOraValue(cpu, 0x8000u);
    OpTay(cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SetAccumulatorWidth(cpu, 1u);
    OpLda(memory, cpu, OpLongX(cpu, SOUND_UPLOAD_TABLE + 2u));
    RolA8(cpu);
    OpAdcValue(cpu, 0x98u);
    OpSta(memory, cpu, OpDp(cpu, SOUND_UPLOAD_BANK));
    PushY(memory, cpu);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, SOUND_UPLOAD_BASE));
    ExchangeAccumulatorBytes(cpu);
    OpIny(cpu);
    if (cpu->zero) {
        OpStepMem(memory, cpu, OpDp(cpu, SOUND_UPLOAD_BANK), 1);
        OpLdy(cpu, 0x8000u);
    }
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, SOUND_UPLOAD_BASE));
    ExchangeAccumulatorBytes(cpu);
    OpTax(cpu);
    OpPullY(memory, cpu);
    PushIndex(memory, cpu);
    OpLdx(cpu, 8u);
    do {
        OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, SOUND_UPLOAD_BASE));
        OpSta(memory, cpu, OpAbs(cpu, SOUND_UPLOAD_DATA_PORT));
        OpTxa(cpu);
        OpSta(memory, cpu, OpAbs(cpu, SOUND_UPLOAD_REPLY_PORT));
        OpOraValue(cpu, 0x80u);
        if (!SoundUploadMatches(memory, cpu, OpAbs(cpu, SOUND_UPLOAD_REPLY_PORT)))
            return ExecutionHandoff(cpu, 0x8098f7u);
        SoundUploadAdvanceHeader(memory, cpu);
    } while (!cpu->zero);
    OpPullX(memory, cpu);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, SOUND_UPLOAD_REPLY_PORT));
    if (!SoundUploadWait(memory, cpu, child, context, 0x80990du))
        return SoundUploadUnwound(0x80990du);
    return ExecutionReturned(0x809910u);
}

Lufia2ExecutionResult Lufia2SendQueuedSoundChunk(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu,
                                                 Lufia2PushedChildCall child,
                                                 void *context) {
    if (!child || cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x8097e5u);
    OpLdx(cpu, 0x100u);
    OpLda(memory, cpu, OpAbs(cpu, (WRAM_SOUND_UPLOAD_SOURCE_LENGTH + 1u)));
    if (cpu->zero) {
        OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_SOUND_UPLOAD_SOURCE_LENGTH)));
        OpLoadA(cpu, 0x80u);
        OpTestBits(memory, cpu, OpAbs(cpu, WRAM_SOUND_PARAMETER_INHIBIT), 0u);
    }
    OpLda(memory, cpu, OpAbs(cpu, WRAM_SOUND_UPLOAD_SOURCE_BANK));
    OpSta(memory, cpu, OpDp(cpu, SOUND_UPLOAD_BANK));
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_SOUND_UPLOAD_SOURCE_CURSOR)));
    OpLoadA(cpu, 0x17u);
    SimulateJsrFrame(memory, cpu, 0x9803u);
    Lufia2WriteSoundDriverMode(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    if (!SoundUploadWait(memory, cpu, child, context, 0x809804u))
        return SoundUploadUnwound(0x809804u);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, SOUND_UPLOAD_REPLY_PORT);
    bool ready = false;
    for (uint32_t polls = 0u; polls < SOUND_UPLOAD_POLL_LIMIT; ++polls) {
        OpLda(memory, cpu, SOUND_UPLOAD_REPLY_PORT);
        if (!cpu->zero) {
            ready = true;
            break;
        }
    }
    if (!ready)
        return ExecutionHandoff(cpu, 0x80980du);
    OpStz(memory, cpu, OpDp(cpu, SOUND_UPLOAD_SEQUENCE));
    SoundUploadReadPair(memory, cpu, 0x80981du, 0x80982bu, true);
    do {
        SoundUploadSendPair(memory, cpu, false);
        OpWriteX(memory, cpu, OpAbs(cpu, WRAM_SOUND_UPLOAD_SOURCE_CURSOR), cpu->y);
        SoundUploadReadPair(memory, cpu, 0x809850u, 0x80985eu, true);
        OpLda(memory, cpu, OpDp(cpu, SOUND_UPLOAD_SEQUENCE));
        OpOraValue(cpu, 0x80u);
        if (!SoundUploadMatches(memory, cpu, SOUND_UPLOAD_REPLY_PORT))
            return ExecutionHandoff(cpu, 0x809868u);
    } while (SoundUploadNextPair(memory, cpu, false));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, SOUND_UPLOAD_MODE_PORT);
    if (!SoundUploadWait(memory, cpu, child, context, 0x80987fu))
        return SoundUploadUnwound(0x80987fu);
    OpStepMem(memory, cpu, OpAbs(cpu, (WRAM_SOUND_UPLOAD_SOURCE_LENGTH + 1u)), -1);
    return ExecutionReturned(0x809885u);
}

Lufia2ExecutionResult Lufia2SoundUploadOpcodeStep(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu, uint32_t entry) {
    if (cpu->program_bank != 0x80u || cpu->accumulator_is_8_bit != 1u ||
        cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, entry);
    uint32_t next = entry + 2u;
    switch (entry) {
    case 0x809969u:
        OpLda(memory, cpu, OpDp(cpu, SOUND_UPLOAD_LOW));
        break;
    case 0x80996fu:
        OpLda(memory, cpu, OpDp(cpu, SOUND_UPLOAD_HIGH));
        break;
    case 0x809975u:
    case 0x809993u:
        OpLda(memory, cpu, OpDp(cpu, SOUND_UPLOAD_SEQUENCE));
        break;
    case 0x80996bu:
        OpSta(memory, cpu, SOUND_UPLOAD_DATA_PORT);
        next = entry + 4u;
        break;
    case 0x809971u:
        OpSta(memory, cpu, SOUND_UPLOAD_HIGH_PORT);
        next = entry + 4u;
        break;
    case 0x809979u:
        OpSta(memory, cpu, SOUND_UPLOAD_REPLY_PORT);
        next = entry + 4u;
        break;
    case 0x809977u:
        OpAndValue(cpu, 0x7fu);
        break;
    case 0x80997du:
    case 0x809988u:
        OpLda(memory, cpu, OpAbsY(cpu, 0u));
        next = entry + 3u;
        break;
    case 0x809980u:
        OpSta(memory, cpu, OpDp(cpu, SOUND_UPLOAD_LOW));
        break;
    case 0x80998bu:
        OpSta(memory, cpu, OpDp(cpu, SOUND_UPLOAD_HIGH));
        break;
    case 0x809982u:
    case 0x80998du:
        OpIny(cpu);
        next = entry + 1u;
        break;
    case 0x809983u:
        next = cpu->zero ? 0x809985u : 0x809988u;
        break;
    case 0x80998eu:
        next = cpu->zero ? 0x809990u : 0x809993u;
        break;
    case 0x809995u:
        OpOraValue(cpu, 0x80u);
        break;
    case 0x80999du:
    case 0x80999fu:
        OpStepMem(memory, cpu, OpDp(cpu, SOUND_UPLOAD_SEQUENCE), 1);
        break;
    case 0x8099a1u:
        OpCpx(cpu, 3u);
        next = entry + 3u;
        break;
    case 0x8099a4u:
    case 0x8099a5u:
        OpDex(cpu);
        next = entry + 1u;
        break;
    case 0x8099a6u:
        next = cpu->carry ? 0x809969u : 0x8099a8u;
        break;
    case 0x8099a8u:
        OpLoadA(cpu, 0xffu);
        break;
    case 0x8099aau:
        OpSta(memory, cpu, SOUND_UPLOAD_MODE_PORT);
        next = entry + 4u;
        break;
    default:
        return ExecutionHandoff(cpu, entry);
    }
    return ExecutionHandoff(cpu, next);
}
