#include "core/cpu_ops.h"
#include "system/wram.h"
#include "text/text_internal.h"

enum {
    START_COMPONENT_FADE = 0xae42u,
    START_PACKED_COLOR_FADE = 0xaef2u,
    DP_COLOR_CHANNELS = 0x54u,
    DP_COLOR_INTENSITY = 0x55u,
    DP_RED = 0x56u,
    DP_GREEN = 0x58u,
    DP_BLUE = 0x5au,
    DP_CHANNEL_MASK = 0x5fu,
    COLOR_EFFECT_FLAGS = 0x0009a9u
};

static void ExpandColorChannels(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                uint16_t back) {
    SimulateJsrFrame(memory, cpu, back);
    OpLda(memory, cpu, OpDp(cpu, DP_COLOR_CHANNELS));
    OpSta(memory, cpu, OpDp(cpu, DP_CHANNEL_MASK));
    OpRepWidths(cpu, 0x20u);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_RED));
    OpSta(memory, cpu, OpDp(cpu, DP_GREEN));
    OpSta(memory, cpu, OpDp(cpu, DP_BLUE));
    OpLda(memory, cpu, OpDp(cpu, DP_COLOR_INTENSITY));
    OpAndValue(cpu, 0x1fu);
    for (unsigned channel = 0; channel < 3u; ++channel) {
        if (channel)
            for (unsigned bit = 0; bit < 5u; ++bit)
                OpAslA(cpu);
        OpLsrMem(memory, cpu, OpDp(cpu, DP_CHANNEL_MASK));
        if (cpu->carry)
            OpSta(memory, cpu, OpDp(cpu, (uint8_t)(DP_RED + 2u * channel)));
    }
    SimulateRtsFrame(memory, cpu);
}

static void StoreColorChannels(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                               uint16_t destination) {
    for (unsigned channel = 0; channel < 3u; ++channel) {
        OpLda(memory, cpu, OpDp(cpu, (uint8_t)(DP_RED + 2u * channel)));
        OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(destination + 2u * channel)));
    }
}

static void StartComponentFade(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_RECOVERY_PALETTE_LENGTH));
    Lufia2TextNextByte(memory, cpu, 0xae49u);
    OpSta(memory, cpu, OpDp(cpu, DP_COLOR_CHANNELS));
    Lufia2TextNextByte(memory, cpu, 0xae4eu);
    OpSta(memory, cpu, OpDp(cpu, DP_COLOR_INTENSITY));
    OpOraValue(cpu, 0u);
    if (cpu->negative) {
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
        OpIncA(cpu);
        OpSta(memory, cpu, OpDp(cpu, DP_COLOR_INTENSITY));
        OpLoadA(cpu, 0x81u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_RECOVERY_PALETTE_CONTROL));
        ExpandColorChannels(memory, cpu, 0xae61u);
        StoreColorChannels(memory, cpu, WRAM_FIELD_RECOVERY_PALETTE_STEPS);
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, 0x1fu);
        OpSta(memory, cpu, OpDp(cpu, DP_COLOR_INTENSITY));
        ExpandColorChannels(memory, cpu, 0xae79u);
        StoreColorChannels(memory, cpu, WRAM_FIELD_RECOVERY_PALETTE_LEVELS);
    } else {
        OpSta(memory, cpu, OpDp(cpu, DP_COLOR_INTENSITY));
        OpStz(memory, cpu, OpAbs(cpu, WRAM_FIELD_RECOVERY_PALETTE_CONTROL));
        ExpandColorChannels(memory, cpu, 0xae92u);
        StoreColorChannels(memory, cpu, WRAM_FIELD_RECOVERY_PALETTE_STEPS);
        for (unsigned channel = 0; channel < 3u; ++channel)
            OpStz(memory, cpu,
                  OpAbs(cpu,
                        (uint16_t)(WRAM_FIELD_RECOVERY_PALETTE_LEVELS + 2u * channel)));
    }
    OpSepWidths(cpu, 0x20u);
    Lufia2TextNextByte(memory, cpu, 0xaeafu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_RECOVERY_PALETTE_PERIOD));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_RECOVERY_PALETTE_DELAY));
}

static void ReadPackedColor(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                            uint16_t back, uint16_t destination) {
    SimulateJsrFrame(memory, cpu, back);
    Lufia2TextNextByte(memory, cpu, 0xaf35u);
    OpSta(memory, cpu, OpDp(cpu, DP_COLOR_CHANNELS));
    Lufia2TextNextByte(memory, cpu, 0xaf3au);
    OpSta(memory, cpu, OpDp(cpu, DP_COLOR_INTENSITY));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DP_COLOR_CHANNELS));
    OpAndValue(cpu, 0x1fu);
    OpSta(memory, cpu, OpDp(cpu, DP_RED));
    OpLda(memory, cpu, OpDp(cpu, DP_COLOR_CHANNELS));
    OpAndValue(cpu, 0x03e0u);
    OpSta(memory, cpu, OpDp(cpu, DP_GREEN));
    OpLda(memory, cpu, OpDp(cpu, DP_COLOR_CHANNELS));
    OpAndValue(cpu, 0x7c00u);
    SimulateRtsFrame(memory, cpu);
    OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(destination + 4u)));
    OpLda(memory, cpu, OpDp(cpu, DP_RED));
    OpSta(memory, cpu, OpAbs(cpu, destination));
    OpLda(memory, cpu, OpDp(cpu, DP_GREEN));
    OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(destination + 2u)));
}

static void StartPackedColorFade(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    ReadPackedColor(memory, cpu, 0xaef4u, WRAM_FIELD_RECOVERY_PALETTE_LEVELS);
    OpSepWidths(cpu, 0x20u);
    ReadPackedColor(memory, cpu, 0xaf06u, WRAM_FIELD_RECOVERY_PALETTE_STEPS);
    OpSepWidths(cpu, 0x20u);
    Lufia2TextNextByte(memory, cpu, 0xaf18u);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_RECOVERY_PALETTE_PERIOD));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_RECOVERY_PALETTE_DELAY));
    OpLoadA(cpu, 0x14u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_RECOVERY_PALETTE_LENGTH));
}

bool Lufia2TextExecuteColorCommand(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                   uint16_t handler) {
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal ||
        cpu->stack < 0x1f10u || cpu->stack > 0x1ffcu)
        return false;
    if (handler == START_COMPONENT_FADE)
        StartComponentFade(memory, cpu);
    else if (handler == START_PACKED_COLOR_FADE)
        StartPackedColorFade(memory, cpu);
    else
        return false;
    OpLda(memory, cpu, COLOR_EFFECT_FLAGS);
    OpOraValue(cpu, 2u);
    OpSta(memory, cpu, COLOR_EFFECT_FLAGS);
    return true;
}
