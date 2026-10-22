/* Sound effects and the 64 Hz effect timer. */

#include "spc/spc_driver_internal.h"

/* An effect starts with its channel count, the DSP channels it takes and
 * one pattern offset per channel, all relative to the effect itself. */
enum {
    EFFECT_CHANNEL_COUNT = 0,
    EFFECT_CHANNELS = 1,
    EFFECT_FINISHED = 0x80, /* in effect_ids: channel has ended */
    EFFECT_ID_LIMIT = 0x10,
    ECHO_RESTORE_TICKS = 4,
};

static uint16_t EffectTableEntry(const Lufia2SpcDriver *driver, uint16_t offset) {
    const uint16_t entry = (uint16_t)(SPC_EFFECT_TABLE_ADDRESS + offset);
    const uint16_t relative =
        (uint16_t)(SpcReadByte(driver, entry) |
                   (SpcReadByte(driver, (uint16_t)(entry + 1)) << 8));
    return (uint16_t)(relative + SPC_EFFECT_TABLE_ADDRESS);
}

/* Effect stop requests index the table with an 8-bit offset. */
uint16_t SpcEffectGroupPointer(Lufia2SpcDriver *driver, uint8_t effect) {
    return EffectTableEntry(driver, (uint8_t)(effect << 1));
}

/* Gives an effect track's DSP voice back to music. */
static void ReleaseEffectTrack(Lufia2SpcDriver *driver, int track) {
    SpcVariables *vars = SpcVars(driver);
    const int voice = track - SPC_MUSIC_TRACKS;
    const uint8_t bit = VoiceBit(track);

    vars->effect_noise_voices &= (uint8_t)~bit;
    vars->effect_echo_voices &= (uint8_t)~bit;
    vars->effect_pitch_mod_voices &= (uint8_t)~bit;
    vars->music_owned_voices |= bit;
    if (vars->music_owned_voices == 0xFF)
        vars->status &= (uint8_t)~SPC_STATUS_EFFECTS_PLAYING;
    if (vars->track_flags[voice] & SPC_TRACK_ACTIVE)
        vars->instrument_restore_voices |= bit;
    vars->track_flags[voice] &= (uint8_t)~SPC_TRACK_MUTED_BY_EFFECT;
    vars->track_flags[track] = 0;
    vars->echo_restore_delay = ECHO_RESTORE_TICKS;
}

static void WriteMusicOwnedMasks(Lufia2SpcDriver *driver) {
    SpcWriteNoiseEnable(driver);
    SpcWritePitchModulation(driver);
}

/* Stops every effect that plays on one of the channels a group uses. */
void SpcStopEffectGroup(Lufia2SpcDriver *driver, uint16_t group) {
    SpcVariables *vars = SpcVars(driver);
    uint8_t remaining = SpcReadByte(driver, (uint16_t)(group + EFFECT_CHANNEL_COUNT));
    uint16_t channel_entry = (uint16_t)(group + EFFECT_CHANNELS);

    vars->music_owned_before = vars->music_owned_voices;
    do {
        const uint8_t channel = SpcReadByte(driver, channel_entry++);
        const uint8_t id = vars->effect_ids[channel & 7];
        if (id == 0)
            continue;
        for (int track = SPC_MUSIC_TRACKS; track < SPC_TRACK_COUNT; ++track) {
            uint8_t *track_id = &vars->effect_ids[track - SPC_MUSIC_TRACKS];
            if (!(vars->track_flags[track] & SPC_TRACK_ACTIVE) ||
                (*track_id & ~EFFECT_FINISHED) != id)
                continue;
            *track_id = 0;
            ReleaseEffectTrack(driver, track);
            SpcQueueKeyOff(driver, track);
        }
    } while (--remaining != 0);
    if (vars->music_owned_before != vars->music_owned_voices) {
        WriteMusicOwnedMasks(driver);
        SpcFlushKeys(driver);
    }
}

/* Smallest id from 0 to 15 that no channel is playing. */
static uint8_t FreeEffectId(const Lufia2SpcDriver *driver) {
    const SpcVariables *vars = SpcVars(driver);
    uint8_t id = 0;

    for (int channel = 0; channel < SPC_MUSIC_TRACKS;) {
        if (vars->effect_ids[channel] == id) {
            if (++id == EFFECT_ID_LIMIT)
                return id;
            channel = 0;
        } else {
            ++channel;
        }
    }
    return id;
}

/* Effect data names DSP channels 0-7; each takes over its music track. */
void SpcStartEffect(Lufia2SpcDriver *driver, uint8_t effect) {
    SpcVariables *vars = SpcVars(driver);
    SpcSettings *settings = SpcSetup(driver);
    const uint16_t group = EffectTableEntry(driver, (uint16_t)(effect << 1));
    uint8_t remaining = SpcReadByte(driver, (uint16_t)(group + EFFECT_CHANNEL_COUNT));
    uint16_t channel_entry = (uint16_t)(group + EFFECT_CHANNELS);
    uint16_t pattern_entry = (uint16_t)(channel_entry + remaining);

    settings->effect_serial = FreeEffectId(driver);
    SpcStopEffectGroup(driver, group);
    do {
        const int channel = SpcReadByte(driver, channel_entry++) & 7;
        const int track = channel + SPC_MUSIC_TRACKS;
        const uint16_t relative =
            (uint16_t)(SpcReadByte(driver, pattern_entry) |
                       (SpcReadByte(driver, (uint16_t)(pattern_entry + 1)) << 8));

        vars->track_flags[track] = SPC_TRACK_ACTIVE;
        vars->track_flags[channel] |= SPC_TRACK_MUTED_BY_EFFECT;
        if (vars->track_flags[channel] & SPC_TRACK_ACTIVE)
            SpcQueueKeyOff(driver, channel);
        vars->effect_ids[channel] = settings->effect_serial;
        SetTrackPattern(driver, track, (uint16_t)(group + relative));
        pattern_entry = (uint16_t)(pattern_entry + 2);
        vars->music_owned_voices &= (uint8_t)~VoiceBit(channel);
        vars->effect_echo_voices &= (uint8_t)~VoiceBit(channel);
        SpcResetTrack(driver, track);
        vars->track_wait[track] = 1;
        vars->track_gate[track] = 0;
    } while (--remaining != 0);
    SpcWriteEchoEnable(driver);
    SpcWriteNoiseEnable(driver);
    SpcWritePitchModulation(driver);
    vars->status |= SPC_STATUS_EFFECTS_PLAYING;
}

void SpcStopAllEffects(Lufia2SpcDriver *driver) {
    SpcVariables *vars = SpcVars(driver);

    for (int track = SPC_MUSIC_TRACKS; track < SPC_TRACK_COUNT; ++track) {
        if (!(vars->track_flags[track] & SPC_TRACK_ACTIVE))
            continue;
        ReleaseEffectTrack(driver, track);
        SpcQueueKeyOff(driver, track);
    }
    WriteMusicOwnedMasks(driver);
    vars->status &= (uint8_t)~SPC_STATUS_EFFECTS_PLAYING;
}

/* A finished channel frees its effect id once all its channels ended. */
static void FinishEffectChannel(Lufia2SpcDriver *driver, int track) {
    SpcVariables *vars = SpcVars(driver);
    uint8_t *own_id = &vars->effect_ids[track - SPC_MUSIC_TRACKS];
    const uint8_t id = *own_id;

    ReleaseEffectTrack(driver, track);
    *own_id = (uint8_t)(id | EFFECT_FINISHED);
    for (int channel = SPC_MUSIC_TRACKS - 1; channel >= 0; --channel) {
        if (!(vars->effect_ids[channel] & EFFECT_FINISHED) &&
            vars->effect_ids[channel] == id)
            return;
    }
    for (int channel = SPC_MUSIC_TRACKS - 1; channel >= 0; --channel) {
        const uint8_t other = vars->effect_ids[channel];
        if ((other & EFFECT_FINISHED) && (other & ~EFFECT_FINISHED) == id)
            vars->effect_ids[channel] = 0;
    }
}

static void EffectTracksTick(Lufia2SpcDriver *driver) {
    SpcVariables *vars = SpcVars(driver);

    if (!(vars->status & SPC_STATUS_EFFECTS_PLAYING))
        return;
    vars->music_owned_before = vars->music_owned_voices;
    for (int track = SPC_MUSIC_TRACKS; track < SPC_TRACK_COUNT; ++track) {
        if (vars->track_flags[track] == 0)
            continue;
        if (vars->track_gate[track] != 0 && --vars->track_gate[track] == 0)
            SpcQueueKeyOff(driver, track);
        if (--vars->track_wait[track] != 0)
            continue;
        SpcProcessTrackEvents(driver, track);
        if (!(vars->track_flags[track] & SPC_TRACK_ACTIVE))
            FinishEffectChannel(driver, track);
    }
    if (vars->music_owned_before != vars->music_owned_voices)
        WriteMusicOwnedMasks(driver);
}

/* Echo comes back a few ticks after an effect released its voice. */
static void EchoRestoreTick(Lufia2SpcDriver *driver) {
    SpcVariables *vars = SpcVars(driver);

    if (vars->echo_restore_delay != 0 && --vars->echo_restore_delay == 0)
        SpcWriteEchoEnable(driver);
}

static void FadeTick(Lufia2SpcDriver *driver) {
    SpcVariables *vars = SpcVars(driver);
    SpcSettings *settings = SpcSetup(driver);

    if (vars->status & SPC_STATUS_FADING_OUT) {
        if (settings->music_fade >= settings->fade_step) {
            settings->music_fade =
                (uint8_t)(settings->music_fade - settings->fade_step);
            return;
        }
        settings->music_fade = 0;
        vars->status &= (uint8_t)~SPC_STATUS_FADING_OUT;
        SpcStopMusic(driver);
    } else if (vars->status & SPC_STATUS_FADING_IN) {
        const unsigned level = (unsigned)settings->music_fade + settings->fade_step;
        if (level <= 0xFF) {
            settings->music_fade = (uint8_t)level;
            return;
        }
        settings->music_fade = 0xFF;
        vars->status &= (uint8_t)~SPC_STATUS_FADING_IN;
    }
}

void SpcEffectsTick(Lufia2SpcDriver *driver) {
    EchoRestoreTick(driver);
    EffectTracksTick(driver);
    FadeTick(driver);
    SpcUpdateModulation(driver);
    SpcWritePitches(driver);
    SpcWriteVolumes(driver);
    SpcFlushKeys(driver);
}
