#ifndef LUFIA2_SYSTEM_H
#define LUFIA2_SYSTEM_H

/* Random numbers, gold, sound queue and screen fade. */

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Optional music execution checkpoint; false preserves a child unwind.
 * Song at $80:942E before STA $54, fade at $80:9692 before PHP. */
typedef uint8_t (*Lufia2MusicCheckpoint)(
    void *context, Lufia2CpuState *cpu, uint32_t pc);

Lufia2ExecutionResult Lufia2GoldAdd(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2GoldSpend(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* Frame polls retain their exact continuation when the clock stalls. */
Lufia2ExecutionResult Lufia2FieldWaitForNextFrame(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2ItemRegionWaitForNextFrame(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2MenuWaitForNextFrame(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2WorldWaitForNextFrame(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2WorldWaitFrames(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

/* Original song loader/player and driver commands; any M/X, binary mode.
 * APU handshakes and sample uploads retain explicit child boundaries. */
Lufia2ExecutionResult Lufia2LoadSong(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, Lufia2MusicCheckpoint checkpoint,
    void *context);
Lufia2ExecutionResult Lufia2PlaySong(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);
Lufia2ExecutionResult Lufia2FadeOutMusic(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, Lufia2MusicCheckpoint checkpoint,
    void *context);
Lufia2ExecutionResult Lufia2SetMusicVolume(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

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

/* Resume between original NMI child calls; RTI stays external. */
Lufia2ExecutionResult Lufia2MainNmiResume(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry,
    Lufia2ExecutionCheckpoint checkpoint, void *context);


/* Original display/clock slice before the consumer checkpoint. */
Lufia2ExecutionResult Lufia2NmiDisplayAndClock(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
/* Restore the original interrupt frame after the width change. */
Lufia2ExecutionResult Lufia2NmiRestoreInterruptFrame(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* Native NMI entry; preserves the hardware interrupt frame. */
Lufia2ExecutionResult Lufia2NmiSaveInterruptFrame(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

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

/* $80:8703 requested OAM/palette DMA, then pad input/repeat. PB80, M8,
 * either X width, DP0, hardware DB, S1F00..1FFC. Restores caller P. The
 * host memory service must advance HVBJOY polling; LLE contexts use fallback. */
Lufia2ExecutionResult Lufia2NmiSpritesPaletteAndPads(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $80:87A7 scroll registers, queued DMA and tilemap uploads. Native mode,
 * DP=0, hardware DB, S=$1F04..$1FFC; any M/X. Restores P on normal RTS;
 * propagates the actual child return address if reverse DMA changes it. */
Lufia2ExecutionResult Lufia2NmiScrollAndUploads(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $80:87FC tilemap uploads. M8/X16, native mode, DP=0, hardware DB,
 * S=$1F02..$1FFC. Three request masks, channel 6 DMA, then RTS. */
Lufia2ExecutionResult Lufia2NmiTilemapUploads(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $00:057D RAM block move: A + 1 bytes from the source bank at X to the
 * destination bank at Y, banks taken from $057E/$057F. X16, any M/DP/DB;
 * S=$1F00..$1FFC, code in a low-RAM bank, prepared MVN/RTS bytes,
 * destination=$7E/$7F, Y >= $2000 and Y + A <= $FFFF.
 * Other entry states hand off untouched; RTS. */
Lufia2ExecutionResult Lufia2RamBlockMove(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WriteSoundDriverMode(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2SendSoundCommand(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SendImmediateSound(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2PlaySoundResource(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SendUncheckedSoundCommand(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2LoadSoundResource(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SoundDriverRequest05(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SoundDriverRead1E(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SoundDriverRead0F(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SoundDriverWrite0C(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SoundDriverWrite0D(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SoundDriverRead18(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SoundDriverWrite09(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SoundDriverWrite1A(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SoundDriverWrite1B(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SoundDriverRequest15(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2PrepareSongResource(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SoundDriverWrite0A(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SoundDriverWriteComplement10(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SoundDriverRead08(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WaitSoundDriverFlagsClear(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2InitializeSoundResourceTable(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WriteSoundTransferMarker(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2AdvanceSoundSourceBank(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2CheckSoundDriverSignature(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2SoundDriverWrite1F(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SoundDriverWrite20(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SoundDriverWrite21(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BeginQueuedSoundResource(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2UpdateSoundResourceQueue(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2UploadSoundResourceSlot(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SoundDriverRequest03(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2UploadSoundPayload(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WaitSoundDriverReply(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2SendSoundPayload(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SendSoundResourceHeader(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SendQueuedSoundChunk(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SceneUploadRequestedTilemaps(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2UploadTilemapBlock(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2StartListedDma(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2RefillRandomTable(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2SystemResolveEventFlagBit(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2SystemTestEventFlag(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SystemTestEventFlagLong(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SystemDivideVectorMagnitude(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2SystemCalculateVectorAngle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SystemDivide24ByByte(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* Original saved field blocks and scalar state. */
Lufia2ExecutionResult Lufia2RestoreSavedFieldState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2CaptureFieldSaveState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* Original alternating-byte save-record checksum; binary mode. */
Lufia2ExecutionResult Lufia2SaveRecordChecksum(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* Original party and capsule save records. */
Lufia2ExecutionResult Lufia2SaveUnpackCapsuleRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2SavePackCapsuleRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2SaveFlagPartyLevelMismatch(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2SaveUnpackPartyRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2SavePackStatBits(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* Original save record reconstruction and stat work. */
Lufia2ExecutionResult Lufia2SaveDerivePartyStatBonuses(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);
Lufia2ExecutionResult Lufia2SaveRestorePartyBaseStats(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);
Lufia2ExecutionResult Lufia2SavePackPartyRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);
Lufia2ExecutionResult Lufia2SavePackState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);
Lufia2ExecutionResult Lufia2SaveRestoreState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2InitializeDefaultRecords(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2PartyResetDefaultRecords(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2StartDefaultRecords(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SaveTestFileHighFlags(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SaveCheckGameFile(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2InitializeBootSound(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2ResetSoundResources(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2UploadBootSoundBlocks(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* Entry phase stops before the first acknowledgement. */
Lufia2ExecutionResult Lufia2BeginBootSoundTransfer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2SystemInitializeVideoMemory(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2InitializeRuntimeDefaults(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2InitializeStartupImage(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2FillWordBlock32(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2ClearGameFile(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2ValidateSaveFileChecksum(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2ValidateSaveFiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2ClearStartupWorkRam(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2StartFieldSystem(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* Reset takes E separately and always enters native mode. */
Lufia2ExecutionResult Lufia2ResetSystem(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint8_t entry_emulation);

Lufia2ExecutionResult Lufia2RunBootSequence(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SineAngle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2CosineAngle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FrameWaitRestoreRegisters(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

/* Original sound-handshake branches and bounded boot transfer phases. */
uint32_t Lufia2SoundPollBranchTarget(uint32_t entry, uint8_t zero, uint8_t overflow);
Lufia2ExecutionResult Lufia2SoundPollBranchStep(Lufia2CpuState *cpu, uint32_t entry);
Lufia2ExecutionResult Lufia2BootSoundTransferStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

/* Transfer phases stop before original polls and bank calls. */
Lufia2ExecutionResult Lufia2SoundUploadStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

/* Additional waits retain support for existing counter steps. */
Lufia2ExecutionResult Lufia2FrameWaitAdditionalStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

Lufia2ExecutionResult Lufia2WaitFramePreserveStatus(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2WaitFramePreserveAccumulator(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2WaitForNextFrame(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* One original frame-counter load, comparison or branch. */
Lufia2ExecutionResult Lufia2FrameWaitCounterStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

Lufia2ExecutionResult Lufia2TextWaitForNextFrame(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2AncientCaveWaitForNextFrame(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* One original frame-counter comparison or branch. */
Lufia2ExecutionResult Lufia2FrameWaitPollStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

/* One complete original poll iteration, including its branch. */
Lufia2ExecutionResult Lufia2SoundPollIteration(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

/* One original poll; branch remains pending. */
Lufia2ExecutionResult Lufia2SoundPollStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

/* Individual transfer boundaries retain original CPU state. */
Lufia2ExecutionResult Lufia2SoundUploadOpcodeStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);
Lufia2ExecutionResult Lufia2BootSoundOpcodeStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

#ifdef __cplusplus
}
#endif

#endif
