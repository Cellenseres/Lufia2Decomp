/* NMI work of bank $80: sprite and palette DMA, pad reading, scroll registers
 * and VRAM uploads. */

#include "core/cpu_internal.h"
#include "core/plain_ops.h"
#include "core/snes_registers.h"
#include "core/wram_view.h"
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

static void RunUploadDma(
    Lufia2Wram wram, uint8_t mode, uint8_t port, uint16_t source,
    uint8_t source_bank, uint16_t size) {
    WramWrite(wram, SNES_DMAP(UPLOAD_CHANNEL), mode);
    WramWrite16(wram, SNES_A1TL(UPLOAD_CHANNEL), source);
    WramWrite(wram, SNES_A1B(UPLOAD_CHANNEL), source_bank);
    WramWrite(wram, SNES_BBAD(UPLOAD_CHANNEL), port);
    WramWrite16(wram, SNES_DASL(UPLOAD_CHANNEL), size);
    WramWrite(wram, SNES_MDMAEN, UPLOAD_ENABLE);
}

/* Clears bit 7 of a request byte (a read-modify-write). */
static void AcknowledgeUpload(Lufia2Wram wram, uint32_t flags) {
    WramWrite(wram, flags,
        (uint8_t)(WramRead(wram, flags) & ~UPLOAD_PENDING_BIT));
}

static uint16_t StepWordAt(
    Lufia2Wram wram, uint32_t location, uint16_t index, int delta) {
    const uint32_t low = WramAddress(wram, location, index);
    const uint16_t value = (uint16_t)(WramRead16At(wram, location, index) + delta);

    Write8(wram.memory, WramNextAddress(location, low), (uint8_t)(value >> 8));
    Write8(wram.memory, low, (uint8_t)value);
    return value;
}

/* Pad 2 then pad 1: held buttons into the DP words, plus presses (and
 * releases, as the inverted mask) with a key repeat. Returns the last A. */
static uint16_t ReadPads(Lufia2Wram wram) {
    uint16_t a = 0;
    int pad;

    for (pad = 2; pad >= 0; pad -= 2) {
        const uint16_t index = (uint16_t)pad;
        const uint16_t buttons = WramRead16At(wram, SNES_JOY1L, index);

        WramWrite16At(wram, DP_BUTTONS_HELD, index, buttons);
        a = buttons;
        if (buttons == WramRead16At(wram, PAD_LAST_BUTTONS, index)) {
            if (StepWordAt(wram, PAD_REPEAT_COUNTDOWN, index, -1) != 0)
                continue;
            const uint16_t previous = WramRead16At(wram, PAD_LAST_BUTTONS, index);
            const uint16_t repeat_mask = WramRead16(wram, PAD_REPEAT_MASK);

            a = (uint16_t)(previous & repeat_mask);
            a |= WramRead16At(wram, DP_BUTTONS_PRESSED, index);
            WramWrite16At(wram, DP_BUTTONS_PRESSED, index, a);
            a = PAD_REPEAT_NEXT;
            WramWrite16At(wram, PAD_REPEAT_COUNTDOWN, index, a);
        } else {
            WramWrite16At(wram, PAD_LAST_BUTTONS, index, buttons);
            a = (uint16_t)(~buttons | WramRead16At(wram, DP_BUTTONS_PRESSED, index));
            WramWrite16At(wram, DP_BUTTONS_PRESSED, index, a);
            a = PAD_REPEAT_FIRST;
            WramWrite16At(wram, PAD_REPEAT_COUNTDOWN, index, a);
        }
    }
    return a;
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
    if (WramRead(wram, OAM_UPLOAD_FLAGS) & UPLOAD_PENDING_MASK) {
        AcknowledgeUpload(wram, OAM_UPLOAD_FLAGS);
        WramWrite16(wram, SNES_OAMADDL, 0);
        RunUploadDma(wram, DMA_ONE_REGISTER, OAM_DATA_PORT, OAM_BUFFER, 0,
            OAM_BUFFER_SIZE);
    }
    if (WramRead(wram, PALETTE_UPLOAD_FLAGS) & UPLOAD_PENDING_MASK) {
        AcknowledgeUpload(wram, PALETTE_UPLOAD_FLAGS);
        WramWrite(wram, SNES_CGADD, 0);
        RunUploadDma(wram, DMA_ONE_REGISTER, CGRAM_DATA_PORT, PALETTE_BUFFER,
            0, PALETTE_BUFFER_SIZE);
    }
    while (WramRead(wram, SNES_HVBJOY) & JOYPAD_BUSY)
        ;
    cpu->accumulator = ReadPads(wram);
    cpu->x = 0xfffeu;
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x8087a6u);
}

/* $80:884F: starts the DMA channels in the low six bits of A, after setting
 * the CGRAM address (kind $80) or the VRAM address from X. */
static void StartListedDma(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram,
    uint8_t request) {
    Push8(memory, cpu, request);
    cpu->carry = (request & DMA_KIND_MASK) >= DMA_KIND_CGRAM;
    if ((request & DMA_KIND_MASK) == DMA_KIND_CGRAM)
        WramWrite(wram, SNES_CGADD, (uint8_t)cpu->x);
    else
        WramWrite16(wram, SNES_VMADDL, cpu->x);
    LoadA8(cpu, (uint8_t)(Pull8(memory, cpu) & DMA_CHANNEL_MASK));
    WramWrite(wram, SNES_MDMAEN, A8(cpu));
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

/* $80:882E: one 2 KiB tilemap block from $7E:X to VRAM word Y. */
static void UploadTilemapBlock(
    Lufia2Wram wram, uint16_t source, uint16_t vram_address) {
    WramWrite16(wram, SNES_VMADDL, vram_address);
    WramWrite16(wram, SNES_A1TL(UPLOAD_CHANNEL), source);
    WramWrite(wram, SNES_DMAP(UPLOAD_CHANNEL), DMA_TWO_REGISTERS);
    WramWrite(wram, SNES_A1B(UPLOAD_CHANNEL), TILEMAP_BANK);
    WramWrite(wram, SNES_BBAD(UPLOAD_CHANNEL), VRAM_DATA_PORT);
    WramWrite16(wram, SNES_DASL(UPLOAD_CHANNEL), TILEMAP_SIZE);
    WramWrite(wram, SNES_MDMAEN, UPLOAD_ENABLE);
}

/* $80:87FC: tilemap blocks requested in $74 (bit pairs), then clears the
 * requests. Leaves A = $AA and Z from the clear. */
static void UploadRequestedTilemaps(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram) {
    static const struct {
        uint8_t mask;
        uint16_t source;
        uint16_t vram;
        uint16_t return_address;
    } blocks[3] = {
        {0xc0u, 0x2000u, 0x0000u, 0x880au},
        {0x30u, 0x2800u, 0x0400u, 0x8819u},
        {0x0cu, 0x3000u, 0x0800u, 0x8828u},
    };
    uint8_t requests;
    unsigned i;
    uint8_t current;

    requests = WramRead(wram, VRAM_UPLOAD_FLAGS);
    LoadA8(cpu, requests);
    for (i = 0; i < 3u; ++i) {
        if (!(requests & blocks[i].mask))
            continue;
        cpu->x = blocks[i].source;
        cpu->y = blocks[i].vram;
        SimulateJsrFrame(memory, cpu, blocks[i].return_address);
        UploadTilemapBlock(wram, blocks[i].source, blocks[i].vram);
        SimulateRtsFrame(memory, cpu);
        cpu->x = TILEMAP_SIZE;
        requests = WramRead(wram, VRAM_UPLOAD_FLAGS);
        LoadA8(cpu, requests);
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
    for (i = 0; i < SCROLL_REGISTER_COUNT; ++i) {
        /* Each scroll register takes its low byte, then its high byte. */
        WramWriteAt(wram, 0, (uint16_t)(SNES_BG1HOFS + i),
            WramReadAt(wram, WRAM_NMI_SCROLL_REGISTERS, (uint16_t)(2u * i)));
        WramWriteAt(wram, 0, (uint16_t)(SNES_BG1HOFS + i),
            WramReadAt(wram, WRAM_NMI_SCROLL_REGISTERS + 1u, (uint16_t)(2u * i)));
    }
    cpu->x = SNES_BG1HOFS + SCROLL_REGISTER_COUNT;
    cpu->y = 2u * SCROLL_REGISTER_COUNT;
    cpu->carry = true;
    for (i = 0; i < DMA_LIST_COUNT; ++i) {
        const uint8_t request = WramRead(wram, DMA_LIST_FLAGS + i);

        LoadA8(cpu, request);
        if (!request)
            continue;
        WramWrite(wram, DMA_LIST_FLAGS + i, 0);
        LoadX16(cpu, WramRead16(wram, DMA_LIST_ADDRESS + 2u * i));
        SimulateJsrFrame(memory, cpu, site_return[i]);
        StartListedDma(memory, cpu, wram, request);
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
