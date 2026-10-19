#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "lufia2/system.h"
#include "system/wram.h"

enum {
    ROM_BOOT_CONTROLLER = 0x808035u,
    ROM_BOOT_CLEAR_RAM = 0x8080d3u,
    ROM_BOOT_RUNTIME_DEFAULTS = 0x8080e1u,
    ROM_BOOT_VIDEO_MEMORY = 0x808117u,
    ROM_BOOT_FIELD_HANDOFF = 0x808281u,
    ROM_BOOT_RANDOM_SEED = 0x8082e7u,
    ROM_BOOT_VALIDATE_FILES = 0x809037u,
    ROM_BOOT_STARTUP_IMAGE = 0x8091dfu,
    ROM_BOOT_INITIALIZE_SOUND = 0x809383u,
    ROM_BOOT_RESET_SOUND = 0x8093d6u,
    ROM_BOOT_SOUND_REQUEST_03 = 0x809528u,
    ROM_BOOT_SOUND_REQUEST_05 = 0x8095d2u,
    ROM_BOOT_SOUND_SIGNATURE = 0x8099b2u,
    ROM_BOOT_TITLE_DISPATCH = 0x82e746u,
    ROM_BOOT_FIELD_SESSION = 0x83acb7u,
    ROM_BOOT_RESET_OWNER = 0x808000u,
    ROM_BOOT_NMI_WAIT = 0x808084u,

    BOOT_LAST_CLEAR_WORD = 0x1dfeu,
    BOOT_STACK_TOP = 0x1fffu,
    BOOT_FORCED_BLANK = 0x8fu,
    DP_BOOT_UNK_6F = 0x6fu,
    BOOT_WAIT_READ_BUDGET = 4096u,
    DP_BOOT_NMI_BANK = 0x6au,
    DP_BOOT_SAVE_FLAGS = 0x54u,
    DP_BOOT_BUTTONS = 0x46u,
    DP_BOOT_TITLE_STATE = 0x30u,
    BOOT_TITLE_BUTTONS = 0x3000u,
    BOOT_NMI_AND_JOYPAD = 0x81u,
    BOOT_TITLE_STATE = 6u,
    BOOT_RANDOM_SEED = 0x96u,
    BOOT_MEMORY_SELECT = 0x420du,
    SRAM_BOOT_FLAGS = 0x700000u,
    SRAM_BOOT_MIRROR = 0x702000u
};

static void DisableBootVideo(const Lufia2Memory *memory, uint8_t bank,
                             uint16_t old_page) {
    const uint32_t bank_base = (uint32_t)bank << 16;

    Write8(memory, bank_base | SNES_INIDISP, BOOT_FORCED_BLANK);
    Write8(memory, (uint16_t)(old_page + DP_BOOT_NMI_BANK), 0u);
    Write8(memory, (uint16_t)(old_page + DP_BOOT_UNK_6F), 0u);
    Write8(memory, bank_base | SNES_NMITIMEN, 0u);
    Write8(memory, bank_base | SNES_MDMAEN, 0u);
    Write8(memory, bank_base | SNES_HDMAEN, 0u);
}

static void ClearBootWorkBytes(const Lufia2Memory *memory) {
    Write8(memory, WRAM_UNK_7E0520, 0u);
    Write8(memory, DP_BOOT_NMI_BANK, 0u);
    Write8(memory, DP_BOOT_UNK_6F, 0u);
}

Lufia2ExecutionResult Lufia2ResetSystem(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint8_t entry_emulation) {
    const uint32_t entry = cpu->program_bank == 0u ? 0x008000u : ROM_BOOT_RESET_OWNER;
    if ((cpu->program_bank != 0x80u && cpu->program_bank != 0u) || entry_emulation > 1u ||
        (entry_emulation && (!cpu->accumulator_is_8_bit || !cpu->index_is_8_bit ||
            cpu->x > 0xffu || cpu->y > 0xffu || (cpu->stack & 0xff00u) != 0x100u)))
        return ExecutionHandoff(cpu, entry);
    cpu->irq_disable = 1u;
    cpu->decimal = 0u;
    cpu->carry = entry_emulation;
    SetAccumulatorWidth(cpu, 1u);
    SetIndexWidth(cpu, 1u);
    LoadA8(cpu, 0u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    DisableBootVideo(memory, cpu->data_bank, cpu->direct_page);
    cpu->stack = BOOT_STACK_TOP;
    cpu->direct_page = 0u;
    LoadA16(cpu, 0u);
    ClearBootWorkBytes(memory);
    cpu->program_bank = 0x80u;
    return ExecutionHandoff(cpu, ROM_BOOT_CONTROLLER);
}

static uint8_t BootChild(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                         Lufia2PushedChildCall child, void *context, uint32_t site,
                         uint32_t target, uint8_t frame, uint8_t index_is_8_bit,
                         Lufia2ExecutionResult *result) {
    if (!CallChildWithFrame(memory, cpu, child, context, site, target, frame, 0x80u)) {
        *result = (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return 0u;
    }
    if (cpu->program_bank != 0x80u || cpu->data_bank != 0x80u ||
        !cpu->accumulator_is_8_bit || cpu->index_is_8_bit != index_is_8_bit ||
        cpu->direct_page || cpu->decimal) {
        *result = ExecutionHandoff(cpu, site + frame + 1u);
        return 0u;
    }
    return 1u;
}

Lufia2ExecutionResult Lufia2ClearStartupWorkRam(const Lufia2Memory *memory,
                                                Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        !cpu->index_is_8_bit || cpu->direct_page)
        return ExecutionHandoff(cpu, ROM_BOOT_CLEAR_RAM);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    OpLdx(cpu, BOOT_LAST_CLEAR_WORD);
    do {
        OpStz(memory, cpu, DirectIndexedAddress(cpu, 0u, cpu->x));
        OpDex(cpu);
        OpDex(cpu);
    } while (!cpu->zero);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x8080e0u);
}

Lufia2ExecutionResult Lufia2StartFieldSystem(const Lufia2Memory *memory,
                                             Lufia2CpuState *cpu) {
    (void)memory;
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        !cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, ROM_BOOT_FIELD_HANDOFF);
    cpu->program_bank = 0x83u;
    return ExecutionHandoff(cpu, ROM_BOOT_FIELD_SESSION);
}

static uint8_t BootSaveMemoryResponds(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, SRAM_BOOT_FLAGS);
    OpSta(memory, cpu, OpDp(cpu, DP_BOOT_SAVE_FLAGS));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, SRAM_BOOT_FLAGS);
    OpCmp(memory, cpu, SRAM_BOOT_MIRROR);
    if (!cpu->zero)
        return 0u;
    OpLda(memory, cpu, OpDp(cpu, DP_BOOT_SAVE_FLAGS));
    OpSta(memory, cpu, SRAM_BOOT_FLAGS);
    OpCmp(memory, cpu, SRAM_BOOT_MIRROR);
    return cpu->zero;
}

Lufia2ExecutionResult Lufia2RunBootSequence(const Lufia2Memory *memory,
                                            Lufia2CpuState *cpu,
                                            Lufia2PushedChildCall child,
                                            void *context) {
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        !cpu->index_is_8_bit || cpu->direct_page || cpu->decimal || !child)
        return ExecutionHandoff(cpu, ROM_BOOT_CONTROLLER);
    Lufia2ExecutionResult result = {0};
    OpSepWidths(cpu, 0x30u);
    OpLoadA(cpu, 0x80u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, BOOT_MEMORY_SELECT));
    if (!BootChild(memory, cpu, child, context, 0x808040u, ROM_BOOT_VIDEO_MEMORY, 2u,
                   1u, &result) ||
        !BootChild(memory, cpu, child, context, 0x808043u, ROM_BOOT_RUNTIME_DEFAULTS,
                   2u, 1u, &result) ||
        !BootChild(memory, cpu, child, context, 0x808046u, ROM_BOOT_VALIDATE_FILES, 3u,
                   1u, &result) ||
        !BootChild(memory, cpu, child, context, 0x80804au, ROM_BOOT_SOUND_SIGNATURE, 3u,
                   1u, &result))
        return result;
    const uint8_t sound_signature_matches = !cpu->carry;
    if (sound_signature_matches) {
        if (!BootChild(memory, cpu, child, context, 0x808050u, ROM_BOOT_CLEAR_RAM, 2u,
                       1u, &result) ||
            !BootChild(memory, cpu, child, context, 0x808053u,
                       ROM_BOOT_RUNTIME_DEFAULTS, 2u, 1u, &result))
            return result;
        OpLoadA(cpu, BOOT_NMI_AND_JOYPAD);
        OpSta(memory, cpu, OpAbs(cpu, SNES_NMITIMEN));
        if (!BootChild(memory, cpu, child, context, 0x80805bu, ROM_BOOT_STARTUP_IMAGE,
                       2u, 1u, &result) ||
            !BootChild(memory, cpu, child, context, 0x80805eu,
                       ROM_BOOT_INITIALIZE_SOUND, 3u, 1u, &result))
            return result;
        OpLoadA(cpu, BOOT_RANDOM_SEED);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_RANDOM_SEED_WORK));
        if (!BootChild(memory, cpu, child, context, 0x808067u, ROM_BOOT_RANDOM_SEED, 3u,
                       1u, &result))
            return result;
    } else {
        if (!BootChild(memory, cpu, child, context, 0x80806du,
                       ROM_BOOT_RUNTIME_DEFAULTS, 2u, 1u, &result))
            return result;
        OpLoadA(cpu, BOOT_NMI_AND_JOYPAD);
        OpSta(memory, cpu, OpAbs(cpu, SNES_NMITIMEN));
        if (!BootChild(memory, cpu, child, context, 0x808075u, ROM_BOOT_STARTUP_IMAGE,
                       2u, 1u, &result) ||
            !BootChild(memory, cpu, child, context, 0x808078u,
                       ROM_BOOT_SOUND_REQUEST_03, 3u, 1u, &result) ||
            !BootChild(memory, cpu, child, context, 0x80807cu,
                       ROM_BOOT_SOUND_REQUEST_05, 3u, 1u, &result) ||
            !BootChild(memory, cpu, child, context, 0x808080u, ROM_BOOT_RESET_SOUND, 3u,
                       1u, &result))
            return result;
    }
    unsigned reads = 0u;
    do {
        OpLda(memory, cpu, OpDp(cpu, DP_BOOT_NMI_BANK));
        if (cpu->zero)
            break;
        if (++reads == BOOT_WAIT_READ_BUDGET)
            return ExecutionHandoff(cpu, ROM_BOOT_NMI_WAIT);
    } while (1);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, WRAM_UNK_7E0520);
    if (!BootSaveMemoryResponds(memory, cpu))
        return ExecutionHandoff(cpu, ROM_BOOT_RESET_OWNER);
    OpLda(memory, cpu, OpDp(cpu, DP_BOOT_SAVE_FLAGS));
    OpSta(memory, cpu, SRAM_BOOT_FLAGS);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E057C));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DP_BOOT_BUTTONS));
    OpCmpValue(cpu, BOOT_TITLE_BUTTONS);
    OpSepWidths(cpu, 0x20u);
    const uint8_t title_state_requested = cpu->zero;
    if (title_state_requested) {
        OpRepWidths(cpu, 0x10u);
        OpLoadA(cpu, BOOT_TITLE_STATE);
        OpSta(memory, cpu, OpDp(cpu, DP_BOOT_TITLE_STATE));
        if (!BootChild(memory, cpu, child, context, 0x8080c9u, ROM_BOOT_TITLE_DISPATCH,
                       3u, 0u, &result))
            return result;
        OpSepWidths(cpu, 0x30u);
    }
    return ExecutionHandoff(cpu, ROM_BOOT_FIELD_HANDOFF);
}
