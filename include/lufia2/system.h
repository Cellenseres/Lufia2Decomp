#ifndef LUFIA2_SYSTEM_H
#define LUFIA2_SYSTEM_H

/* Random numbers, sound queue and screen fade. */

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Save/load entry events observe the file index in A before ROM work.
 * Both functions accept any M/X width in native binary mode. */
Lufia2ExecutionResult Lufia2LoadGameFile(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, Lufia2ExecutionCheckpoint checkpoint,
    void *context);
Lufia2ExecutionResult Lufia2SaveGameFile(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, Lufia2ExecutionCheckpoint checkpoint,
    void *context);

/* $80:914B reads/decrypts a whole slot; $80:9184 encrypts/writes it.
 * M1/X16, native binary mode. Header reads also occur inside game loads. */
Lufia2ExecutionResult Lufia2ReadGameFile(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, Lufia2ExecutionCheckpoint checkpoint,
    void *context);
Lufia2ExecutionResult Lufia2WriteGameFile(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

/* $80:90FC encrypted-slot word checksum, seed $6502; M1/X16. */
Lufia2ExecutionResult Lufia2SaveFileChecksum(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

/* $80:91D3 file index to SRAM slot offset in X; M1, either X width. */
Lufia2ExecutionResult Lufia2ResolveSaveFileAddress(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $80:82E7 seed the random table from DB:$0558; any M/X, binary mode.
 * Restores P and DP scratch byte; X/Y and A retain original outputs. */
Lufia2ExecutionResult Lufia2SeedRandom(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $80:8638 NMI body; exact reset/RTI boundaries retain the interrupt frame. */
Lufia2ExecutionResult Lufia2MainNmi(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, Lufia2ExecutionCheckpoint checkpoint,
    void *context);

/* $80:8E9D decompress resource $54 to $7E/$7F:[$60]; any width. */
Lufia2ExecutionResult Lufia2DecompressResource(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:8378 $4E /= $51, A = remainder; any width. */
Lufia2ExecutionResult Lufia2Divide16(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:8299: A = (A.low * next random byte) >> 8. */
void Lufia2RandomScale(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:82C7: A.low = next random byte, same table and index. */
void Lufia2RandomByte(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $84:8766: defer APU command A to $17AC. */
void Lufia2QueueDeferredSound(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:86C1 screen fade into the $0583 brightness. */
Lufia2ExecutionResult Lufia2ScreenFade(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:E808 X to digits $B4, $B3, $B2; M1X0. */
Lufia2ExecutionResult Lufia2DecimalDigits3(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:834C $51..$53 = $4E.word * $50.byte through two hardware products.
 * Any M/X, native binary mode, DP zero and DB mapping the SNES registers.
 * Restores entry widths and X; an A8 entry retains the computed A.high.
 */
Lufia2ExecutionResult Lufia2Multiply16By8(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
