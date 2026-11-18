/* NMI work of bank $80: sprite and palette DMA, pad reading, scroll registers
 * and VRAM uploads. */

#include "core/cpu_ops.h"
#include "core/plain_ops.h"
#include "core/snes_registers.h"
#include "core/wram_view.h"
#include "lufia2/nmi_control_steps.h"
#include "lufia2/nmi_dma_setup.h"
#include "lufia2/nmi_joypad_gate.h"
#include "lufia2/nmi_listed_dma_steps.h"
#include "lufia2/nmi_pad_steps.h"
#include "lufia2/nmi_tilemap_steps.h"
#include "lufia2/nmi_upload_checks.h"
#include "lufia2/nmi_upload_control_steps.h"
#include "lufia2/system.h"
#include "system/wram.h"

/* Direct-page request flags. Bit 7 is the one the NMI clears. */
enum {
    OAM_UPLOAD_FLAGS = 0x72u,
    PALETTE_UPLOAD_FLAGS = DP_NMI_UPLOAD_FLAGS,
    VRAM_UPLOAD_FLAGS = 0x74u,
    UPLOAD_PENDING_MASK = 0xc0u,
    UPLOAD_PENDING_BIT = 0x80u,
    /* Requests of the four-way DMA list: flag byte and its VRAM or CGRAM
     * address at the matching word. */
    DMA_LIST_FLAGS = 0x75u,
    DMA_LIST_ADDRESS = 0x79u,
    DMA_LIST_COUNT = 4u,
    SCROLL_REGISTER_COUNT = 8u,
    DMA_CHANNEL_MASK = 0x3fu,
    DMA_KIND_MASK = 0xc0u,
    DMA_KIND_CGRAM = 0x80u,
    HDMA_CHANNELS = 0x81u
};

/* Absolute work-RAM fields. */
enum {
    PAD_REPEAT_COUNTDOWN = 0x055au, /* per pad, 16 bits */
    PAD_LAST_BUTTONS = 0x055eu,
    PAD_REPEAT_MASK = 0x0562u,
    PAD_REPEAT_FIRST = 0x0010u,
    PAD_REPEAT_NEXT = 0x0004u
};

/* Source buffers and DMA channel of the upload. */
enum {
    UPLOAD_CHANNEL = 6u,
    OAM_BUFFER = 0x0100u,
    OAM_BUFFER_SIZE = 0x0220u,
    PALETTE_BUFFER = 0x0320u,
    PALETTE_BUFFER_SIZE = 0x0200u,
    UPLOAD_ENABLE = 1u << UPLOAD_CHANNEL,
    DMA_ONE_REGISTER = 0x00u,
    DMA_TWO_REGISTERS = 0x01u,
    OAM_DATA_PORT = 0x04u,
    VRAM_DATA_PORT = 0x18u,
    CGRAM_DATA_PORT = 0x22u,
    TILEMAP_BANK = 0x7eu,
    TILEMAP_SIZE = 0x0800u,
    JOYPAD_BUSY = 0x01u
};

static void ConfigureUploadDma(
    Lufia2Wram wram, uint8_t mode, uint8_t port, uint16_t source,
    uint8_t source_bank, uint16_t size) {
    WramWrite(wram, SNES_DMAP(UPLOAD_CHANNEL), mode);
    WramWrite16(wram, SNES_A1TL(UPLOAD_CHANNEL), source);
    WramWrite(wram, SNES_A1B(UPLOAD_CHANNEL), source_bank);
    WramWrite(wram, SNES_BBAD(UPLOAD_CHANNEL), port);
    WramWrite16(wram, SNES_DASL(UPLOAD_CHANNEL), size);
}

static void StartUploadDma(Lufia2Wram wram, uint8_t channels) {
    WramWrite(wram, SNES_MDMAEN, channels);
}

static uint8_t ReadJoypadStatus(Lufia2Wram wram) {
    return WramRead(wram, SNES_HVBJOY);
}

/* Clears bit 7 of a request byte (a read-modify-write). */
static void AcknowledgeUpload(Lufia2Wram wram, uint32_t flags) {
    WramWrite(wram, flags,
        (uint8_t)(WramRead(wram, flags) & ~UPLOAD_PENDING_BIT));
}

static void PrepareOamUpload(Lufia2Wram wram) {
    AcknowledgeUpload(wram, OAM_UPLOAD_FLAGS);
    WramWrite16(wram, SNES_OAMADDL, 0);
    ConfigureUploadDma(wram, DMA_ONE_REGISTER, OAM_DATA_PORT, OAM_BUFFER, 0,
        OAM_BUFFER_SIZE);
}

static void PreparePaletteUpload(Lufia2Wram wram) {
    AcknowledgeUpload(wram, PALETTE_UPLOAD_FLAGS);
    WramWrite(wram, SNES_CGADD, 0);
    ConfigureUploadDma(wram, DMA_ONE_REGISTER, CGRAM_DATA_PORT, PALETTE_BUFFER,
        0, PALETTE_BUFFER_SIZE);
}

Lufia2ExecutionResult Lufia2NmiPrepareUploadDma(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry) {
    if (cpu->program_bank != 0x80u || cpu->accumulator_is_8_bit != 1u ||
        cpu->index_is_8_bit != 0u || cpu->direct_page != 0u)
        return ExecutionHandoff(cpu, entry);
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    if (entry == 0x80870cu) {
        PrepareOamUpload(wram);
        LoadA8(cpu, OAM_DATA_PORT);
        LoadX16(cpu, OAM_BUFFER_SIZE);
        return ExecutionHandoff(cpu, 0x808731u);
    }
    if (entry == 0x80873cu) {
        PreparePaletteUpload(wram);
        LoadA8(cpu, CGRAM_DATA_PORT);
        LoadX16(cpu, PALETTE_BUFFER_SIZE);
        return ExecutionHandoff(cpu, 0x808760u);
    }
    return ExecutionHandoff(cpu, entry);
}

static uint16_t StepWordAt(
    Lufia2Wram wram, uint32_t location, uint16_t index, int delta) {
    const uint32_t low = WramAddress(wram, location, index);
    const uint16_t value = (uint16_t)(WramRead16At(wram, location, index) + delta);

    Write8(wram.memory, WramNextAddress(location, low), (uint8_t)(value >> 8));
    Write8(wram.memory, low, (uint8_t)value);
    return value;
}

static uint8_t ReadUploadRequest(Lufia2Wram wram, uint32_t flags) {
    return (uint8_t)(WramRead(wram, flags) & UPLOAD_PENDING_MASK);
}

typedef struct PadReadResult {
    uint16_t accumulator;
    bool carry;
} PadReadResult;

static PadReadResult ReadPad(Lufia2Wram wram, uint16_t index) {
    const uint16_t buttons = WramRead16At(wram, SNES_JOY1L, index);
    WramWrite16At(wram, DP_BUTTONS_HELD, index, buttons);
    const uint16_t previous = WramRead16At(wram, PAD_LAST_BUTTONS, index);
    PadReadResult result = {buttons, buttons >= previous};
    if (buttons == previous) {
        if (StepWordAt(wram, PAD_REPEAT_COUNTDOWN, index, -1) != 0)
            return result;
        const uint16_t held = WramRead16At(wram, PAD_LAST_BUTTONS, index);
        const uint16_t repeat_mask = WramRead16(wram, PAD_REPEAT_MASK);
        result.accumulator = (uint16_t)(held & repeat_mask);
        result.accumulator |= WramRead16At(wram, DP_BUTTONS_PRESSED, index);
        WramWrite16At(wram, DP_BUTTONS_PRESSED, index, result.accumulator);
        result.accumulator = PAD_REPEAT_NEXT;
    } else {
        WramWrite16At(wram, PAD_LAST_BUTTONS, index, buttons);
        result.accumulator =
            (uint16_t)(~buttons | WramRead16At(wram, DP_BUTTONS_PRESSED, index));
        WramWrite16At(wram, DP_BUTTONS_PRESSED, index, result.accumulator);
        result.accumulator = PAD_REPEAT_FIRST;
    }
    WramWrite16At(wram, PAD_REPEAT_COUNTDOWN, index, result.accumulator);
    return result;
}

static uint16_t ReadPads(Lufia2Wram wram) {
    (void)ReadPad(wram, 2u);
    return ReadPad(wram, 0u).accumulator;
}

Lufia2ExecutionResult Lufia2NmiReadPadStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x80u || cpu->accumulator_is_8_bit != 0u ||
        cpu->index_is_8_bit != 0u || cpu->direct_page != 0u ||
        (cpu->x != 0u && cpu->x != 2u) ||
        !(cpu->data_bank < 0x40u ||
          (cpu->data_bank >= 0x80u && cpu->data_bank < 0xc0u)))
        return ExecutionHandoff(cpu, 0x808770u);
    const PadReadResult result = ReadPad(WramViewOfCaller(memory, cpu), cpu->x);
    cpu->accumulator = result.accumulator;
    cpu->carry = result.carry;
    cpu->x = (uint16_t)(cpu->x - 2u);
    SetNz16(cpu, cpu->x);
    return ExecutionHandoff(cpu, cpu->x == 0xfffeu ? 0x8087a5u : 0x808770u);
}

Lufia2ExecutionResult Lufia2NmiControlStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry) {
    if (cpu->program_bank != entry >> 16 || cpu->accumulator_is_8_bit > 1u ||
        cpu->index_is_8_bit > 1u)
        return ExecutionHandoff(cpu, entry);
    switch (entry) {
    case 0x808703u:
        Push8(memory, cpu, PackStatus(cpu));
        return ExecutionHandoff(cpu, 0x808704u);
    case 0x808704u:
        SetIndexWidth(cpu, 0u);
        return ExecutionHandoff(cpu, 0x808706u);
    case 0x80876bu:
        SetAccumulatorWidth(cpu, 0u);
        /* REP clears the high index bytes when X stays narrow. */
        SetIndexWidth(cpu, cpu->index_is_8_bit);
        return ExecutionHandoff(cpu, 0x80876du);
    case 0x80876du:
        if (cpu->index_is_8_bit)
            return ExecutionHandoff(cpu, entry);
        LoadX16(cpu, 2u);
        return ExecutionHandoff(cpu, 0x808770u);
    case 0x8087a5u:
        UnpackStatus(cpu, Pull8(memory, cpu));
        return ExecutionHandoff(cpu, 0x8087a6u);
    case 0x8087a6u: {
        const uint16_t low = Pull8(memory, cpu);
        const uint16_t address = (uint16_t)(low | ((uint16_t)Pull8(memory, cpu) << 8));
        return ExecutionHandoff(cpu, ((uint32_t)cpu->program_bank << 16) |
            (uint16_t)(address + 1u));
    }
    case 0x808826u:
        SimulateJsrFrame(memory, cpu, 0x8828u);
        return ExecutionHandoff(cpu, 0x80882eu);
    default:
        return ExecutionHandoff(cpu, entry);
    }
}

Lufia2ExecutionResult Lufia2NmiCheckUploadRequest(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry) {
    if (cpu->program_bank != 0x80u || cpu->accumulator_is_8_bit != 1u ||
        cpu->index_is_8_bit != 0u || cpu->direct_page != 0u)
        return ExecutionHandoff(cpu, entry);
    uint32_t flags, skipped;
    if (entry == 0x808706u) {
        flags = OAM_UPLOAD_FLAGS;
        skipped = 0x808736u;
    } else if (entry == 0x808736u) {
        flags = PALETTE_UPLOAD_FLAGS;
        skipped = 0x808765u;
    } else {
        return ExecutionHandoff(cpu, entry);
    }
    const uint8_t request = ReadUploadRequest(WramViewOfCaller(memory, cpu), flags);
    LoadA8(cpu, request);
    return ExecutionHandoff(cpu, request ? entry + 6u : skipped);
}

/* Upload requested OAM/palette buffers, then read both pads after auto-read.
 * The memory service must advance the automatic-read timer while polling. */
Lufia2ExecutionResult Lufia2NmiSpritesPaletteAndPads(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;

    if (!cpu->accumulator_is_8_bit || cpu->program_bank != 0x80u ||
        cpu->direct_page != 0u || cpu->stack < 0x1f00u ||
        cpu->stack > 0x1ffcu ||
        !(cpu->data_bank < 0x40u ||
          (cpu->data_bank >= 0x80u && cpu->data_bank < 0xc0u)))
        return ExecutionHandoff(cpu, 0x808703u);
    wram = WramViewOfCaller(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    SetIndexWidth(cpu, 0);
    if (ReadUploadRequest(wram, OAM_UPLOAD_FLAGS)) {
        PrepareOamUpload(wram);
        StartUploadDma(wram, UPLOAD_ENABLE);
    }
    if (ReadUploadRequest(wram, PALETTE_UPLOAD_FLAGS)) {
        PreparePaletteUpload(wram);
        StartUploadDma(wram, UPLOAD_ENABLE);
    }
    while (ReadJoypadStatus(wram) & JOYPAD_BUSY)
        ;
    cpu->accumulator = ReadPads(wram);
    cpu->x = 0xfffeu;
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x8087a6u);
}

Lufia2ExecutionResult Lufia2StartListedDma(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x80884fu);
    PushAccumulator8(memory, cpu);
    OpAndValue(cpu, DMA_KIND_MASK);
    OpCmpValue(cpu, DMA_KIND_CGRAM);
    if (cpu->zero) {
        OpTxa(cpu);
        OpSta(memory, cpu, OpAbs(cpu, SNES_CGADD));
    } else {
        WramWrite16(WramViewOfCaller(memory, cpu), SNES_VMADDL, cpu->x);
    }
    LoadA8(cpu, Pull8(memory, cpu));
    OpAndValue(cpu, DMA_CHANNEL_MASK);
    OpSta(memory, cpu, OpAbs(cpu, SNES_MDMAEN));
    return ExecutionReturned(0x808865u);
}

/* A reverse DMA can change the return frame of the listed upload. */
static bool ReturnFromListedDma(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t expected_return, Lufia2ExecutionResult *result) {
    const uint16_t last_byte = PullStackWord(memory, cpu);

    if (last_byte == expected_return)
        return true;
    *result = ExecutionHandoff(cpu,
        ((uint32_t)cpu->program_bank << 16) | (uint16_t)(last_byte + 1u));
    return false;
}

Lufia2ExecutionResult Lufia2UploadTilemapBlock(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x80882eu);
    WramWrite16(WramViewOfCaller(memory, cpu), SNES_VMADDL, cpu->y);
    WramWrite16(WramViewOfCaller(memory, cpu), SNES_A1TL(UPLOAD_CHANNEL), cpu->x);
    OpLoadA(cpu, DMA_TWO_REGISTERS);
    OpSta(memory, cpu, OpAbs(cpu, SNES_DMAP(UPLOAD_CHANNEL)));
    OpLoadA(cpu, TILEMAP_BANK);
    OpSta(memory, cpu, OpAbs(cpu, SNES_A1B(UPLOAD_CHANNEL)));
    OpLoadA(cpu, VRAM_DATA_PORT);
    OpSta(memory, cpu, OpAbs(cpu, SNES_BBAD(UPLOAD_CHANNEL)));
    OpLdx(cpu, TILEMAP_SIZE);
    WramWrite16(WramViewOfCaller(memory, cpu), SNES_DASL(UPLOAD_CHANNEL), cpu->x);
    OpLoadA(cpu, UPLOAD_ENABLE);
    OpSta(memory, cpu, OpAbs(cpu, SNES_MDMAEN));
    return ExecutionReturned(0x80884eu);
}

typedef struct NmiTilemapBlock {
    uint8_t mask;
    uint16_t source;
    uint16_t vram;
    uint16_t return_address;
} NmiTilemapBlock;

static const NmiTilemapBlock tilemap_blocks[3] = {
    {0xc0u, 0x2000u, 0x0000u, 0x880au},
    {0x30u, 0x2800u, 0x0400u, 0x8819u},
    {0x0cu, 0x3000u, 0x0800u, 0x8828u},
};

static void SelectTilemapBlock(Lufia2CpuState *cpu, const NmiTilemapBlock *block) {
    OpLdx(cpu, block->source);
    OpLdy(cpu, block->vram);
}

Lufia2ExecutionResult Lufia2NmiPrepareTilemapBlock(Lufia2CpuState *cpu,
    uint32_t entry) {
    if (cpu->program_bank != 0x80u || cpu->accumulator_is_8_bit != 1u ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, entry);
    unsigned block;
    switch (entry) {
    case 0x808802u: block = 0u; break;
    case 0x808811u: block = 1u; break;
    case 0x808820u: block = 2u; break;
    default: return ExecutionHandoff(cpu, entry);
    }
    SelectTilemapBlock(cpu, &tilemap_blocks[block]);
    return ExecutionHandoff(cpu, entry + 6u);
}

/* $80:87FC: tilemap blocks requested in $74 (bit pairs), then clears the
 * requests. Leaves A = $AA and Z from the clear. */
static void UploadRequestedTilemaps(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram) {
    uint8_t requests;
    unsigned i;
    uint8_t current;

    requests = WramRead(wram, VRAM_UPLOAD_FLAGS);
    LoadA8(cpu, requests);
    for (i = 0; i < 3u; ++i) {
        if (!(requests & tilemap_blocks[i].mask))
            continue;
        SelectTilemapBlock(cpu, &tilemap_blocks[i]);
        SimulateJsrFrame(memory, cpu, tilemap_blocks[i].return_address);
        (void)Lufia2UploadTilemapBlock(memory, cpu);
        SimulateRtsFrame(memory, cpu);
        cpu->x = TILEMAP_SIZE;
        if (i + 1u < 3u) {
            requests = WramRead(wram, VRAM_UPLOAD_FLAGS);
            LoadA8(cpu, requests);
        }
    }
    LoadA8(cpu, 0xaau);
    current = WramRead(wram, VRAM_UPLOAD_FLAGS);
    cpu->zero = (current & 0xaau) == 0;
    WramWrite(wram, VRAM_UPLOAD_FLAGS, (uint8_t)(current & ~0xaau));
}

/* $80:87FC: the three tilemap blocks. Entry contract: M1X0 only. The code
 * has no PHP and no width change of its own and runs on the caller's
 * widths, so any other entry state is handed back to the original code, and
 * the exit state equals the entry state. */
Lufia2ExecutionResult Lufia2NmiTilemapUploads(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x8087fcu);
    UploadRequestedTilemaps(memory, cpu, WramViewOfCaller(memory, cpu));
    return ExecutionReturned(0x80882du);
}

static uint8_t WriteScrollRegisterPair(
    Lufia2Wram wram, uint16_t reg, uint16_t offset) {
    WramWriteAt(wram, 0, reg,
        WramReadAt(wram, WRAM_NMI_SCROLL_REGISTERS, offset));
    const uint8_t high = WramReadAt(wram, WRAM_NMI_SCROLL_REGISTERS + 1u, offset);
    WramWriteAt(wram, 0, reg, high);
    return high;
}

Lufia2ExecutionResult Lufia2NmiPrepareScrollWrites(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x80u || cpu->data_bank != 0x80u ||
        cpu->direct_page || cpu->decimal)
        return ExecutionHandoff(cpu, 0x8087a7u);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    LoadX16(cpu, SNES_BG1HOFS);
    LoadY16(cpu, 0u);
    return ExecutionHandoff(cpu, 0x8087b2u);
}

static void PrepareListedDmaDestination(Lufia2Wram wram, Lufia2CpuState *cpu,
                                        unsigned request) {
    WramWrite(wram, DMA_LIST_FLAGS + request, 0u);
    LoadX16(cpu, WramRead16(wram, DMA_LIST_ADDRESS + 2u * request));
}

Lufia2ExecutionResult Lufia2NmiPrepareListedDma(const Lufia2Memory *memory,
                                                Lufia2CpuState *cpu, uint32_t entry) {
    unsigned request;
    switch (entry) {
    case 0x8087c8u:
        request = 0u;
        break;
    case 0x8087d3u:
        request = 1u;
        break;
    case 0x8087deu:
        request = 2u;
        break;
    case 0x8087e9u:
        request = 3u;
        break;
    default:
        return ExecutionHandoff(cpu, entry);
    }
    if (cpu->program_bank != 0x80u || cpu->data_bank != 0x80u || cpu->direct_page ||
        cpu->decimal || cpu->accumulator_is_8_bit != 1u || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, entry);
    PrepareListedDmaDestination(WramViewOfCaller(memory, cpu), cpu, request);
    return ExecutionHandoff(cpu, entry + 4u);
}

Lufia2ExecutionResult Lufia2NmiSelectListedDma(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry) {
    unsigned request;
    switch (entry) {
    case 0x8087c4u: request = 0u; break;
    case 0x8087cfu: request = 1u; break;
    case 0x8087dau: request = 2u; break;
    case 0x8087e5u: request = 3u; break;
    default: return ExecutionHandoff(cpu, entry);
    }
    if (cpu->program_bank != 0x80u || cpu->data_bank != 0x80u ||
        cpu->direct_page || cpu->decimal ||
        !cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, entry);
    LoadA8(cpu, WramRead(WramViewOfCaller(memory, cpu), DMA_LIST_FLAGS + request));
    return ExecutionHandoff(cpu, entry + (cpu->zero ? 11u : 4u));
}

Lufia2ExecutionResult Lufia2NmiWriteScrollPair(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x80u || cpu->data_bank != 0x80u || cpu->direct_page ||
        cpu->decimal || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit ||
        cpu->x < SNES_BG1HOFS || cpu->x >= SNES_BG1HOFS + SCROLL_REGISTER_COUNT ||
        cpu->y != 2u * (cpu->x - SNES_BG1HOFS))
        return ExecutionHandoff(cpu, 0x8087b2u);
    LoadA8(cpu, WriteScrollRegisterPair(WramViewOfCaller(memory, cpu), cpu->x, cpu->y));
    LoadX16(cpu, (uint16_t)(cpu->x + 1u));
    LoadY16(cpu, (uint16_t)(cpu->y + 2u));
    Compare16(cpu, cpu->y, 2u * SCROLL_REGISTER_COUNT);
    return ExecutionHandoff(cpu, cpu->zero ? 0x8087c4u : 0x8087b2u);
}

Lufia2ExecutionResult Lufia2NmiStepTilemapRequests(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry) {
    if (cpu->program_bank != 0x80u || cpu->data_bank != 0x80u ||
        cpu->direct_page || cpu->decimal ||
        !cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, entry);
    Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint8_t mask;
    uint32_t skipped;
    switch (entry) {
    case 0x8087fcu:
        LoadA8(cpu, WramRead(wram, VRAM_UPLOAD_FLAGS));
        mask = 0xc0u;
        skipped = 0x80880du;
        break;
    case 0x80880du:
        mask = 0x30u;
        skipped = 0x80881cu;
        break;
    case 0x80881cu:
        mask = 0x0cu;
        skipped = 0x808829u;
        break;
    case 0x808829u: {
        LoadA8(cpu, 0xaau);
        const uint8_t requests = WramRead(wram, VRAM_UPLOAD_FLAGS);
        cpu->zero = (requests & A8(cpu)) == 0u;
        WramWrite(wram, VRAM_UPLOAD_FLAGS, (uint8_t)(requests & ~A8(cpu)));
        return ExecutionHandoff(cpu, 0x80882du);
    }
    default:
        return ExecutionHandoff(cpu, entry);
    }
    OpBitValue(cpu, mask);
    return ExecutionHandoff(cpu, cpu->zero ? skipped :
        entry + (entry == 0x8087fcu ? 6u : 4u));
}

Lufia2ExecutionResult Lufia2NmiFinishScrollAndReturn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x80u || cpu->data_bank != 0x80u ||
        cpu->direct_page || cpu->decimal ||
        !cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x8087f3u);
    Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    LoadA8(cpu, WramRead(wram, HDMA_CHANNELS));
    if (!cpu->zero)
        WramWrite(wram, SNES_HDMAEN, A8(cpu));
    UnpackStatus(cpu, Pull8(memory, cpu));
    uint16_t return_address = Pull8(memory, cpu);
    return_address |= (uint16_t)Pull8(memory, cpu) << 8;
    return ExecutionHandoff(cpu, 0x800000u | (uint16_t)(return_address + 1u));
}

/* $80:87A7: updates scroll registers, queued DMA, tilemaps and HDMA.
 * Saves and restores P; either accumulator and index width may enter. */
Lufia2ExecutionResult Lufia2NmiScrollAndUploads(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const uint16_t site_return[DMA_LIST_COUNT] = {
        0x87ceu, 0x87d9u, 0x87e4u, 0x87efu};
    Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    unsigned i;
    Lufia2ExecutionResult result;

    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    for (i = 0; i < SCROLL_REGISTER_COUNT; ++i)
        (void)WriteScrollRegisterPair(wram, (uint16_t)(SNES_BG1HOFS + i),
            (uint16_t)(2u * i));
    cpu->x = SNES_BG1HOFS + SCROLL_REGISTER_COUNT;
    cpu->y = 2u * SCROLL_REGISTER_COUNT;
    cpu->carry = true;
    for (i = 0; i < DMA_LIST_COUNT; ++i) {
        const uint8_t request = WramRead(wram, DMA_LIST_FLAGS + i);

        LoadA8(cpu, request);
        if (!request)
            continue;
        PrepareListedDmaDestination(wram, cpu, i);
        SimulateJsrFrame(memory, cpu, site_return[i]);
        (void)Lufia2StartListedDma(memory, cpu);
        if (!ReturnFromListedDma(memory, cpu, site_return[i], &result))
            return result;
    }
    SimulateJsrFrame(memory, cpu, 0x87f2u);
    UploadRequestedTilemaps(memory, cpu, wram);
    SimulateRtsFrame(memory, cpu);
    LoadA8(cpu, WramRead(wram, HDMA_CHANNELS));
    if (!cpu->zero)
        WramWrite(wram, SNES_HDMAEN, A8(cpu));
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x8087fbu);
}

Lufia2ExecutionResult Lufia2NmiUploadControlStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry) {
    if (cpu->program_bank != 0x80u || cpu->accumulator_is_8_bit != 1u ||
        cpu->index_is_8_bit > 1u)
        return ExecutionHandoff(cpu, entry);
    switch (entry) {
    case 0x808731u: case 0x808760u:
        LoadA8(cpu, UPLOAD_ENABLE);
        return ExecutionHandoff(cpu, entry + 2u);
    case 0x808733u: case 0x808762u:
        StartUploadDma(WramViewOfCaller(memory, cpu), (uint8_t)cpu->accumulator);
        return ExecutionHandoff(cpu, entry + 3u);
    default:
        return ExecutionHandoff(cpu, entry);
    }
}

Lufia2ExecutionResult Lufia2NmiPollJoypadReady(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x80u || cpu->accumulator_is_8_bit != 1u ||
        cpu->index_is_8_bit > 1u)
        return ExecutionHandoff(cpu, 0x808765u);
    LoadA8(cpu, ReadJoypadStatus(WramViewOfCaller(memory, cpu)));
    OpLsrA(cpu);
    return ExecutionHandoff(cpu, cpu->carry ? 0x808765u : 0x80876bu);
}
