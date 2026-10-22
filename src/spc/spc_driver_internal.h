#ifndef LUFIA2_SPC_DRIVER_INTERNAL_H
#define LUFIA2_SPC_DRIVER_INTERNAL_H

#include "lufia2/spc_driver.h"
#include "spc/spc_ram.h"

/* S-DSP registers; voice registers are DSP_V* + 16 * voice. */
enum {
    DSP_VOLUME_LEFT = 0x00,
    DSP_VOLUME_RIGHT = 0x01,
    DSP_PITCH_LO = 0x02,
    DSP_PITCH_HI = 0x03,
    DSP_SOURCE = 0x04,
    DSP_ADSR1 = 0x05,
    DSP_ADSR2 = 0x06,
    DSP_MASTER_VOLUME_LEFT = 0x0C,
    DSP_ECHO_FEEDBACK = 0x0D,
    DSP_FILTER_COEFFICIENT = 0x0F,
    DSP_MASTER_VOLUME_RIGHT = 0x1C,
    DSP_ECHO_VOLUME_LEFT = 0x2C,
    DSP_PITCH_MODULATION = 0x2D,
    DSP_ECHO_VOLUME_RIGHT = 0x3C,
    DSP_NOISE_ENABLE = 0x3D,
    DSP_KEY_ON = 0x4C,
    DSP_ECHO_ENABLE = 0x4D,
    DSP_KEY_OFF = 0x5C,
    DSP_FLAGS = 0x6C,
    DSP_ECHO_START = 0x6D,
    DSP_ECHO_DELAY = 0x7D,
};

/* Bits of the DSP flag register. */
enum {
    DSP_FLAG_ECHO_WRITE_OFF = 0x20,
    DSP_FLAG_MUTE = 0x40,
    DSP_FLAG_NOISE_CLOCK = 0x1F,
};

/* CPU port numbers and the protocol bytes on them. */
enum {
    PORT_ARGUMENT = 0,
    PORT_ARGUMENT_HIGH = 1,
    PORT_COMMAND = 2,
    PORT_COMMAND_CHECK = 3,
    PROTOCOL_ACK_LOW = 0xCD,
    PROTOCOL_ACK_HIGH = 0xEF,
    PROTOCOL_END = 0xFF,
};

enum { SPC_TIMER_TEMPO = 0, SPC_TIMER_EFFECTS = 1 };

/* $3F11/$3F1F: MOV Y,#$40 then 64 DBNZ Y (6 cycles taken, 4 last). */
enum { SPC_KEY_SETTLE_CYCLES = 2 + 63 * 6 + 4 };

static inline SpcRam *SpcRamOf(const Lufia2SpcDriver *driver) {
    return (SpcRam *)driver->aram;
}

static inline SpcVariables *SpcVars(const Lufia2SpcDriver *driver) {
    return &SpcRamOf(driver)->layout.vars;
}

static inline SpcTracks *SpcTrackState(const Lufia2SpcDriver *driver) {
    return &SpcRamOf(driver)->layout.tracks;
}

static inline SpcSettings *SpcSetup(const Lufia2SpcDriver *driver) {
    return &SpcRamOf(driver)->layout.settings;
}

static inline uint8_t SpcReadByte(const Lufia2SpcDriver *driver, uint16_t address) {
    return driver->aram[address];
}

static inline void SpcWriteByte(Lufia2SpcDriver *driver, uint16_t address,
                                uint8_t value) {
    driver->aram[address] = value;
}

static inline void DspWrite(Lufia2SpcDriver *driver, uint8_t reg, uint8_t value) {
    driver->hardware->write_dsp(driver->hardware->context, reg, value);
}

static inline uint8_t DspRead(Lufia2SpcDriver *driver, uint8_t reg) {
    return driver->hardware->read_dsp(driver->hardware->context, reg);
}

static inline void DspDelay(Lufia2SpcDriver *driver, unsigned cycles) {
    driver->hardware->delay(driver->hardware->context, cycles);
}

static inline uint8_t PortRead(Lufia2SpcDriver *driver, unsigned port) {
    return driver->hardware->read_port(driver->hardware->context, port);
}

static inline void PortWrite(Lufia2SpcDriver *driver, unsigned port, uint8_t value) {
    driver->hardware->write_port(driver->hardware->context, port, value);
}

static inline uint8_t TimerRead(Lufia2SpcDriver *driver, unsigned timer) {
    return driver->hardware->read_timer(driver->hardware->context, timer);
}

/* Bit of a track's DSP voice, and its inverse. */
static inline uint8_t VoiceBit(int track) { return (uint8_t)(1u << (track & 7)); }

static inline uint8_t VoiceRegister(int track, uint8_t reg) {
    return (uint8_t)(((track & 7) << 4) | reg);
}

static inline bool IsEffectTrack(int track) { return track >= SPC_MUSIC_TRACKS; }

static inline uint16_t TrackPattern(const Lufia2SpcDriver *driver, int track) {
    const SpcTracks *t = SpcTrackState(driver);
    return (uint16_t)(t->pattern_lo[track] | (t->pattern_hi[track] << 8));
}

static inline void SetTrackPattern(Lufia2SpcDriver *driver, int track,
                                   uint16_t address) {
    SpcTracks *t = SpcTrackState(driver);
    t->pattern_lo[track] = (uint8_t)address;
    t->pattern_hi[track] = (uint8_t)(address >> 8);
}

/* SPC-700 DIV YA,X including its overflow behaviour. */
uint8_t SpcDivide(uint16_t dividend, uint8_t divisor, uint8_t *remainder);

/* spc_voice.c */
void SpcResetTrack(Lufia2SpcDriver *driver, int track);
void SpcSetInstrument(Lufia2SpcDriver *driver, int track, uint8_t instrument);
void SpcUpdateTrackVolume(Lufia2SpcDriver *driver, int track);
void SpcSetNotePitch(Lufia2SpcDriver *driver, int track);
void SpcQueueKeyOn(Lufia2SpcDriver *driver, int track);
void SpcQueueKeyOff(Lufia2SpcDriver *driver, int track);
void SpcFlushKeys(Lufia2SpcDriver *driver);
void SpcWriteVolumes(Lufia2SpcDriver *driver);
void SpcWritePitches(Lufia2SpcDriver *driver);
void SpcUpdateModulation(Lufia2SpcDriver *driver);

/* spc_mixer.c */
void SpcWritePitchModulation(Lufia2SpcDriver *driver);
void SpcWriteEchoEnable(Lufia2SpcDriver *driver);
void SpcWriteNoiseEnable(Lufia2SpcDriver *driver);
void SpcWriteMasterVolume(Lufia2SpcDriver *driver);
void SpcWriteEchoFilter(Lufia2SpcDriver *driver);
void SpcSetEchoVolume(Lufia2SpcDriver *driver, uint8_t level);
void SpcSetupEcho(Lufia2SpcDriver *driver);
void SpcResetDsp(Lufia2SpcDriver *driver);
void SpcFinishDspReset(Lufia2SpcDriver *driver);

/* spc_music.c */
void SpcStartSong(Lufia2SpcDriver *driver);
void SpcStopMusic(Lufia2SpcDriver *driver);
void SpcMusicTick(Lufia2SpcDriver *driver);
void SpcProcessTrackEvents(Lufia2SpcDriver *driver, int track);
void SpcEnterPattern(Lufia2SpcDriver *driver, int track, uint16_t order);

/* spc_effects.c */
void SpcEffectsTick(Lufia2SpcDriver *driver);
void SpcStartEffect(Lufia2SpcDriver *driver, uint8_t effect);
void SpcStopAllEffects(Lufia2SpcDriver *driver);
void SpcStopEffectGroup(Lufia2SpcDriver *driver, uint16_t group);
uint16_t SpcEffectGroupPointer(Lufia2SpcDriver *driver, uint8_t effect);

#endif
