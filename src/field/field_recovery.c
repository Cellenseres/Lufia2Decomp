#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    REQUESTS = WRAM_FIELD_REQUESTS,
    OBJECT_KIND = WRAM_UNK_7E1559,
    OBJECT_UPDATE = WRAM_FIELD_RECOVERY_OBJECT_UPDATES,
    OBJECT_ZOOM_RATE = 0x15d9u,
    OBJECT_ZOOM_LIMIT = WRAM_FIELD_RECOVERY_ZOOM_LIMITS,
    OBJECT_SPIN_RATE = WRAM_FIELD_RECOVERY_SPIN_RATES,
    OBJECT_SECOND_SPIN_RATE = WRAM_FIELD_RECOVERY_SECOND_SPIN_RATES,
    PALETTE_LEVELS = WRAM_FIELD_RECOVERY_PALETTE_LEVELS,
    PALETTE_STEPS = WRAM_FIELD_RECOVERY_PALETTE_STEPS,
    PALETTE_DELAY = WRAM_FIELD_RECOVERY_PALETTE_DELAY,
    PALETTE_PERIOD = WRAM_FIELD_RECOVERY_PALETTE_PERIOD,
    PALETTE_LENGTH = WRAM_FIELD_RECOVERY_PALETTE_LENGTH,
    PALETTE_CONTROL = WRAM_FIELD_RECOVERY_PALETTE_CONTROL,
    EFFECT_CONTROL = WRAM_UNK_7E09A9
};

static uint8_t RecoveryReady(const Lufia2CpuState *cpu, uint8_t wide) {
    return cpu->program_bank == 0x83u && !cpu->index_is_8_bit &&
        cpu->accumulator_is_8_bit != wide && !cpu->decimal;
}

static Lufia2ExecutionResult RecoveryUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static uint8_t RecoveryChild(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t site,
    uint32_t target, uint8_t frame) {
    return CallChildWithFrame(memory, cpu, child, context,
        site, target, frame, 0x83u);
}

Lufia2ExecutionResult Lufia2FieldProcessRequests(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x83u || !cpu->accumulator_is_8_bit ||
        cpu->decimal || !child)
        return ExecutionHandoff(cpu, 0x838103u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09A8));
    OpBitValue(cpu, 8u);
    if (cpu->zero) {
        OpRepWidths(cpu, 0x10u);
        OpLda(memory, cpu, OpAbs(cpu, REQUESTS));
        OpBitValue(cpu, 2u);
        if (!cpu->zero) {
            if (!RecoveryChild(memory, cpu, child, context,
                    0x838113u, 0x83812eu, 2u))
                return RecoveryUnwound(0x838113u);
            OpLda(memory, cpu, OpAbs(cpu, REQUESTS));
        }
        OpBitValue(cpu, 4u);
        if (!cpu->zero) {
            if (!RecoveryChild(memory, cpu, child, context,
                    0x83811du, 0x838160u, 2u))
                return RecoveryUnwound(0x83811du);
            OpLda(memory, cpu, OpAbs(cpu, REQUESTS));
        }
        OpBitValue(cpu, 1u);
        if (!cpu->zero && !RecoveryChild(memory, cpu, child, context,
                0x838127u, 0x84836au, 3u))
            return RecoveryUnwound(0x838127u);
    }
    OpSepWidths(cpu, 0x10u);
    return ExecutionReturned(0x83812du);
}

Lufia2ExecutionResult Lufia2FieldFrameServices(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    static const struct { uint32_t site, target; uint8_t frame; } services[] = {
        {0x83819du, 0x809c72u, 3u},
        {0x8381a1u, 0x838682u, 2u},
        {0x8381a4u, 0x83bb93u, 3u},
        {0x8381a8u, 0x83e03eu, 2u},
        {0x8381abu, 0x8ebd77u, 3u},
        {0x8381afu, 0x83a21au, 3u},
        {0x8381b3u, 0x8381c6u, 2u},
        {0x8381b6u, 0x83aeb5u, 2u}
    };
    if (cpu->program_bank != 0x83u || cpu->decimal || !child)
        return ExecutionHandoff(cpu, 0x838192u);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x30u);
    if (!RecoveryChild(memory, cpu, child, context,
            0x838195u, 0x83900cu, 2u))
        return RecoveryUnwound(0x838195u);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_SOUND_COMMAND));
    for (unsigned service = 0; service < 8u; ++service) {
        if (!RecoveryChild(memory, cpu, child, context,
                services[service].site, services[service].target,
                services[service].frame))
            return RecoveryUnwound(services[service].site);
    }
    OpLda(memory, cpu, OpAbs(cpu, WRAM_SOUND_COMMAND));
    OpCmpValue(cpu, 0xffu);
    if (!cpu->zero && !RecoveryChild(memory, cpu, child, context,
            0x8381c0u, 0x80953bu, 3u))
        return RecoveryUnwound(0x8381c0u);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x8381c5u);
}

static Lufia2ExecutionResult PollRecovery(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t entry, uint8_t mask) {
    if (!RecoveryReady(cpu, 0u) || !child)
        return ExecutionHandoff(cpu, entry);
    OpLoadA(cpu, 1u);
    if (!RecoveryChild(memory, cpu, child, context,
            entry + 2u, 0x8382a0u, 2u))
        return RecoveryUnwound(entry + 2u);
    if (!cpu->carry) {
        OpLoadA(cpu, mask);
        OpTestBits(memory, cpu, OpAbs(cpu, REQUESTS), 0u);
    }
    return ExecutionReturned(entry + 12u);
}

Lufia2ExecutionResult Lufia2FieldPollHpRecovery(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return PollRecovery(memory, cpu, child, context, 0x83812eu, 2u);
}

Lufia2ExecutionResult Lufia2FieldPollMpRecovery(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return PollRecovery(memory, cpu, child, context, 0x838160u, 4u);
}

static Lufia2ExecutionResult StartRecovery(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t entry, uint8_t mp) {
    if (!RecoveryReady(cpu, 0u) || !child)
        return ExecutionHandoff(cpu, entry);
    OpLoadA(cpu, mp ? 4u : 2u);
    OpTestBits(memory, cpu, OpAbs(cpu, REQUESTS), 1u);
    if (!cpu->zero)
        return ExecutionReturned(entry + 36u);
    if (!RecoveryChild(memory, cpu, child, context,
            entry + 7u, 0x838327u, 2u))
        return RecoveryUnwound(entry + 7u);
    if (!RecoveryChild(memory, cpu, child, context,
            entry + 10u, mp ? 0x81f60bu : 0x81f5edu, 3u))
        return RecoveryUnwound(entry + 10u);
    OpLoadA(cpu, mp ? 12u : 16u);
    OpSta(memory, cpu, OpAbs(cpu, PALETTE_LEVELS));
    OpSta(memory, cpu, OpAbs(cpu, PALETTE_LEVELS + (mp ? 2u : 4u)));
    OpLoadA(cpu, 20u);
    OpSta(memory, cpu, OpAbs(cpu, PALETTE_LEVELS + (mp ? 4u : 2u)));
    if (!RecoveryChild(memory, cpu, child, context,
            entry + 27u, 0x8382b2u, 2u))
        return RecoveryUnwound(entry + 27u);
    OpLoadA(cpu, 0x2fu);
    if (!RecoveryChild(memory, cpu, child, context,
            entry + 32u, 0x848766u, 3u))
        return RecoveryUnwound(entry + 32u);
    return ExecutionReturned(entry + 36u);
}

Lufia2ExecutionResult Lufia2FieldStartHpRecovery(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return StartRecovery(memory, cpu, child, context, 0x83813bu, 0u);
}

Lufia2ExecutionResult Lufia2FieldStartMpRecovery(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return StartRecovery(memory, cpu, child, context, 0x83816du, 1u);
}

Lufia2ExecutionResult Lufia2FieldFindRecoveryObject(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    if (!RecoveryReady(cpu, 0u))
        return ExecutionHandoff(cpu, 0x8382a0u);
    OpLdx(cpu, 0u);
    do {
        OpCmp(memory, cpu, OpAbsX(cpu, OBJECT_KIND));
        if (cpu->zero) {
            cpu->carry = 1u;
            return ExecutionReturned(0x8382b1u);
        }
        OpInx(cpu);
        OpCpx(cpu, 31u);
    } while (!cpu->zero);
    cpu->carry = 0u;
    return ExecutionReturned(0x8382afu);
}

Lufia2ExecutionResult Lufia2FieldScalePaletteGreen(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)memory;
    (void)child;
    (void)context;
    if (!RecoveryReady(cpu, 1u))
        return ExecutionHandoff(cpu, 0x83831eu);
    ExchangeAccumulatorBytes(cpu);
    for (unsigned bit = 0u; bit < 3u; ++bit)
        OpLsrA(cpu);
    return ExecutionReturned(0x838322u);
}

Lufia2ExecutionResult Lufia2FieldScalePaletteBlue(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)memory;
    (void)child;
    (void)context;
    if (!RecoveryReady(cpu, 1u))
        return ExecutionHandoff(cpu, 0x838323u);
    ExchangeAccumulatorBytes(cpu);
    OpAslA(cpu);
    OpAslA(cpu);
    return ExecutionReturned(0x838326u);
}

Lufia2ExecutionResult Lufia2FieldPrepareRecoveryPalette(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    static const struct { uint16_t offset; uint32_t site, target; } channels[] = {
        {2u, 0x8382f0u, 0x83831eu},
        {8u, 0x8382f9u, 0x83831eu},
        {4u, 0x838302u, 0x838323u},
        {10u, 0x83830bu, 0x838323u}
    };
    if (!RecoveryReady(cpu, 0u) || !child)
        return ExecutionHandoff(cpu, 0x8382b2u);
    OpLda(memory, cpu, EFFECT_CONTROL);
    OpAndValue(cpu, 0xfbu);
    OpSta(memory, cpu, EFFECT_CONTROL);
    OpLoadA(cpu, 1u);
    for (unsigned channel = 0u; channel < 3u; ++channel)
        OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(PALETTE_STEPS + channel * 2u)));
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, PALETTE_CONTROL));
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpAbs(cpu, PALETTE_PERIOD));
    OpSta(memory, cpu, OpAbs(cpu, PALETTE_DELAY));
    OpLoadA(cpu, 20u);
    OpSta(memory, cpu, OpAbs(cpu, PALETTE_LENGTH));
    for (unsigned value = 0u; value < 6u; ++value)
        OpStz(memory, cpu, OpAbs(cpu, (uint16_t)(PALETTE_LEVELS + value * 2u + 1u)));
    OpRepWidths(cpu, 0x20u);
    for (unsigned channel = 0u; channel < 4u; ++channel) {
        const uint16_t address = (uint16_t)(PALETTE_LEVELS + channels[channel].offset);
        OpLda(memory, cpu, OpAbs(cpu, address));
        if (!RecoveryChild(memory, cpu, child, context,
                channels[channel].site, channels[channel].target, 2u))
            return RecoveryUnwound(channels[channel].site);
        OpSta(memory, cpu, OpAbs(cpu, address));
    }
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, EFFECT_CONTROL);
    OpOraValue(cpu, 2u);
    OpSta(memory, cpu, EFFECT_CONTROL);
    return ExecutionReturned(0x83831du);
}

Lufia2ExecutionResult Lufia2FieldLoadRecoveryGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    static const struct { uint8_t low, high, kind; uint32_t site; } resources[] = {
        {0x40u, 0x60u, 1u, 0x838335u},
        {0xe0u, 0x70u, 2u, 0x838346u}
    };
    if (!RecoveryReady(cpu, 0u) || !child)
        return ExecutionHandoff(cpu, 0x838327u);
    for (unsigned resource = 0u; resource < 2u; ++resource) {
        OpLoadA(cpu, resources[resource].low);
        OpSta(memory, cpu, OpDp(cpu, 8u));
        OpLoadA(cpu, resources[resource].high);
        OpSta(memory, cpu, OpDp(cpu, 9u));
        OpLoadA(cpu, 0xc1u);
        OpSta(memory, cpu, OpDp(cpu, 13u));
        OpLoadA(cpu, resources[resource].kind);
        if (!RecoveryChild(memory, cpu, child, context,
                resources[resource].site, 0x83834au, 2u))
            return RecoveryUnwound(resources[resource].site);
    }
    return ExecutionReturned(0x838349u);
}

Lufia2ExecutionResult Lufia2FieldConfigureRecoveryObjects(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!RecoveryReady(cpu, 0u) || !child)
        return ExecutionHandoff(cpu, 0x83834au);
    OpSta(memory, cpu, OpDp(cpu, 1u));
    OpLoadA(cpu, 0x7fu);
    OpSta(memory, cpu, OpDp(cpu, 6u));
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLoadA(cpu, 0x30u);
    OpSta(memory, cpu, OpDp(cpu, 10u));
    if (!RecoveryChild(memory, cpu, child, context,
            0x838358u, 0x83c729u, 3u))
        return RecoveryUnwound(0x838358u);
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, OBJECT_KIND));
        OpCmp(memory, cpu, OpDp(cpu, 1u));
        if (cpu->zero) {
            OpLoadA(cpu, 0xd1u);
            OpSta(memory, cpu, OpAbsX(cpu, OBJECT_ZOOM_RATE));
            OpLoadA(cpu, 0x84u);
            OpSta(memory, cpu, OpAbsX(cpu, OBJECT_ZOOM_LIMIT));
            OpLda(memory, cpu, OpDp(cpu, 13u));
            OpSta(memory, cpu, OpAbsX(cpu, OBJECT_SPIN_RATE));
            OpLoadA(cpu, 0x21u);
            OpSta(memory, cpu, OpAbsX(cpu, OBJECT_SECOND_SPIN_RATE));
            OpLoadA(cpu, 4u);
            OpSta(memory, cpu, OpAbsX(cpu, OBJECT_UPDATE));
        }
        OpInx(cpu);
        OpCpx(cpu, 32u);
    } while (!cpu->zero);
    return ExecutionReturned(0x838385u);
}
