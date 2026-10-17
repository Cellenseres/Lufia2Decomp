/* Displayed-cell rule of the field tile writers, read from bank $7F. */

#include "lufia2/field.h"
#include "system/wram.h"

enum {
    CELL_HIDDEN = 0x3000u,
    CELL_METATILE_MASK = 0x03ffu,
    METATILE_BYTES = 8u,
    BANK_LAST = 0xffffu,
    FIRST_ALTERNATE_LAYER = 2u, /* $80:F734 uses $7F:D040 from here */
};

static uint16_t ReadWord(const uint8_t *bank, uint16_t offset) {
    return (uint16_t)(bank[offset] | (bank[offset + 1u] << 8));
}

static uint16_t ReadBankWord(const uint8_t *bank, uint32_t address) {
    return ReadWord(bank, (uint16_t)(address & BANK_LAST));
}

void Lufia2FieldCellViewOpen(Lufia2FieldCellView *view, const uint8_t *bank_7f) {
    view->bank_7f = bank_7f;
    for (unsigned layer = 0; layer < LUFIA2_FIELD_CELL_LAYERS; ++layer) {
        view->cell_base[layer] =
            ReadBankWord(bank_7f, WRAM_FIELD_LAYER_CELL_BASE + 2u * layer);
        view->width[layer] = ReadBankWord(bank_7f, WRAM_FIELD_LAYER_WIDTH + 2u * layer);
        view->height[layer] =
            ReadBankWord(bank_7f, WRAM_FIELD_LAYER_HEIGHT + 2u * layer);
    }
    view->metatile_base = ReadBankWord(bank_7f, WRAM_FIELD_METATILE_BASE);
    view->alternate_metatile_base =
        ReadBankWord(bank_7f, WRAM_FIELD_ALTERNATE_METATILE_BASE);
}

/* A DB-relative read past $7F:FFFF would leave the bank. */
static uint8_t WordInBank(uint32_t offset) { return offset < BANK_LAST; }

uint8_t Lufia2FieldCellDisplayed(const Lufia2FieldCellView *view, unsigned layer,
                                 unsigned cell_x, unsigned cell_y,
                                 Lufia2FieldDisplayedCell *out) {
    if (!view->bank_7f || layer >= LUFIA2_FIELD_CELL_LAYERS)
        return 0;
    const uint16_t width = view->width[layer];
    const uint16_t height = view->height[layer];
    /* The streamers multiply by the low byte of the size. */
    if (width == 0 || height == 0 || width > 0xffu || height > 0xffu ||
        cell_x >= width || cell_y >= height)
        return 0;

    const uint8_t *bank = view->bank_7f;
    const uint16_t cell =
        (uint16_t)(view->cell_base[layer] + 2u * (cell_y * width + cell_x));
    if (!WordInBank(cell))
        return 0;
    out->word = ReadWord(bank, cell);
    out->hidden = (out->word & CELL_HIDDEN) == CELL_HIDDEN;
    out->source = out->hidden ? view->cell_base[0] : cell;
    if (!WordInBank(out->source))
        return 0;
    out->metatile = ReadWord(bank, out->source) & CELL_METATILE_MASK;

    const uint16_t base = layer >= FIRST_ALTERNATE_LAYER ? view->alternate_metatile_base
                                                         : view->metatile_base;
    const uint16_t tiles = (uint16_t)(base + out->metatile * METATILE_BYTES);
    if ((uint32_t)tiles + METATILE_BYTES - 1u > BANK_LAST)
        return 0;
    for (unsigned tile = 0; tile < 4u; ++tile)
        out->tiles[tile] = ReadWord(bank, (uint16_t)(tiles + 2u * tile));
    return 1;
}
