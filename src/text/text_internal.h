#ifndef LUFIA2_TEXT_TEXT_INTERNAL_H
#define LUFIA2_TEXT_TEXT_INTERNAL_H

/* Shared text engine internals. */

#include "lufia2/execution.h"

/* Text engine WRAM. */
#define TEXT_WINDOW_STATE 0x099cu /* bit 0: window buffer in use */
#define TEXT_GLYPH_ATTRIBUTE 0x09adu
#define TEXT_GLYPH 0x09afu             /* glyph or sub-script number */
#define TEXT_GLYPH_DESTINATION 0x09b1u /* $7E tilemap buffer address */
#define TEXT_GLYPH_UPLOAD 0x09b5u      /* VRAM upload source */
#define TEXT_SCRIPT_POINTER 0x09b7u
#define TEXT_SCRIPT_BANK 0x09b9u
#define TEXT_PRINT_DELAY 0x0b52u
#define TEXT_LINE_START 0x1250u
#define TEXT_CALLER_POINTER 0x1252u /* sub-script return */
#define TEXT_CALLER_BANK 0x1254u    /* zero: no caller */
#define TEXT_RETURN_POINTER 0x1257u
#define TEXT_RETURN_BANK 0x1259u /* zero: nothing queued */
#define TEXT_RETURN_COUNT 0x125au
#define TEXT_TYPING_SOUND 0x1260u /* zero: silent */
#define TEXT_CHOICE_INDEX 0x126au /* selected entry */
#define TEXT_CHOICE_TABLE 0x126bu /* word: pointer to the target table */
#define TEXT_CHOICE_TABLE_BANK 0x126du
#define TEXT_CHOICE_COUNT 0x126eu
#define TEXT_PROMPT_TIMER 0x1265u
#define TEXT_WAIT_ACTOR 0x1269u           /* negative: none */
#define TEXT_TYPING_SOUND_PHASE 0x7fd0c0u /* sound every other glyph */
#define TEXT_PRINT_COUNTDOWN 0x7fd0ffu
#define TEXT_GLYPH_STATE 0x09b3u /* cleared with the glyph buffer */
#define TEXT_GLYPH_BUFFER                                                              \
    0xd000u /* $7E: two-bit glyph tiles, one 16-byte tile per cell */
#define TEXT_GLYPH_BUFFER_BYTES 0x1000u
#define TEXT_WINDOW_TILEMAP 0x3000u   /* $7E: window tilemap rows, 32 tiles each */
#define TEXT_WINDOW_TILE_BASE 0x1255u /* added to the window border tile numbers */
#define TEXT_BG3_VOFS_SHADOW 0x059eu  /* BG3 vertical scroll as the NMI writes it */
#define TEXT_WINDOW_ROW_ORIGIN 0x7fd085u /* word: tilemap offset of the window */
#define TEXT_WINDOW_ROW_COUNT 0x7fd087u  /* rows queued so far */
#define TEXT_WINDOW_ROW_MARK 0x7fd088u
#define TEXT_WINDOW_ROW_WIDTH 0x7fd089u /* tiles per row */

/* Shared script fetch/pointer helpers include their original JSR frames. */
void Lufia2TextNextByte(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t return_address);
void Lufia2TextSetScriptPointer(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t return_address);
void Lufia2TextNextWord(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t return_address);
void Lufia2TextTestFlag(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t return_address);

/* $80:A074 opcode $14: M1/X0; finishes at 9D00 or the goto entry A3C6. */
uint32_t Lufia2TextEvaluateCondition(const Lufia2Memory *memory,
    Lufia2CpuState *cpu);
/* Internal M1/X0 children return at their original RTS instruction. */
Lufia2ExecutionResult Lufia2TextConditionRecord(const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2TextConditionItem(const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $84:8328: clear the window buffer $7E:3000-37FF and $099C bit 0. */
void Lufia2TextWindowClear(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address);

/* $80:9CB8 text step: known paths native, exact continuations for unknown children. */
Lufia2ExecutionResult Lufia2TextEngineStepBody(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2ExecutionResult result);

/* $80:9C80: text box waits for its timer or the A/X buttons. */
Lufia2ExecutionResult Lufia2TextPromptTick(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2ExecutionResult result);

#endif
