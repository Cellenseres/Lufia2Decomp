/* Per-track sound state and its DSP voice output. */

#include "spc/spc_driver_internal.h"

enum {
    NOTE_PITCH_LO_TABLE = 0x4362, /* 12 semitones, octave 8 */
    NOTE_PITCH_HI_TABLE = 0x436E,
    SINE_TABLE = 0x437A, /* 128 entries, half a period */
    SEMITONES = 12,
    TOP_OCTAVE = 8,
};

/* Raw track fields for indexes the CPU may pass out of range. */
enum {
    TRACK_VOLUME_ADDRESS = 0x0723,
    TRACK_VELOCITY_ADDRESS = 0x0773,
    TRACK_MIXED_VOLUME_ADDRESS = 0x08B3,
};

void SpcResetTrack(Lufia2SpcDriver *driver, int track) {
    SpcTracks *t = SpcTrackState(driver);

    t->transpose[track] = 0;
    t->length[track] = 0;
    t->gate_time[track] = 0;
    t->velocity[track] = 0;
    t->previous_length[track] = 0;
    t->previous_gate_time[track] = 0;
    t->previous_velocity[track] = 0;
    t->tremolo_carry[track] = 0;
    t->vibrato_carry[track] = 0;
    t->vibrato_amount[track] = 0;
    t->loop_order_hi[0][track] = 0;
    t->loop_order_hi[1][track] = 0;
    t->loop_count[0][track] = 0;
    t->loop_count[1][track] = 0;
    SpcVars(driver)->track_gate[track] = 0;
    t->envelope_setting[track] = 0x87;
    t->volume[track] = 0x70;
    t->pitch_scale[track] = 0x80;
    t->tremolo_speed[track] = 0x20;
    t->vibrato_speed[track] = 0x20;
    t->vibrato_value[track] = 0;
    t->tremolo_value[track] = 0;
    t->vibrato_phase[track] = 0;
    t->tremolo_phase[track] = 0;
    t->tremolo_depth[track] = 0x10;
    t->vibrato_depth[track] = 0x20;
}

/* The instrument number doubles as the DSP source number. */
void SpcSetInstrument(Lufia2SpcDriver *driver, int track, uint8_t instrument) {
    const SpcInstrument *record =
        &SpcRamOf(driver)->layout.instruments[instrument & (SPC_INSTRUMENT_SLOTS - 1)];
    SpcTracks *t = SpcTrackState(driver);

    DspWrite(driver, VoiceRegister(track, DSP_SOURCE), instrument);
    DspWrite(driver, VoiceRegister(track, DSP_ADSR1), record->adsr1);
    DspWrite(driver, VoiceRegister(track, DSP_ADSR2), record->adsr2);
    t->tune_lo[track] = record->tune_lo;
    t->tune_hi[track] = record->tune_hi;
}

/* Product of velocity, track volume and the music or effect group volume. */
void SpcUpdateTrackVolume(Lufia2SpcDriver *driver, int track) {
    const SpcSettings *settings = SpcSetup(driver);
    const uint8_t index = (uint8_t)track;
    const uint8_t group =
        index >= SPC_MUSIC_TRACKS ? settings->effect_volume : settings->music_volume;
    const uint8_t velocity =
        (uint8_t)((SpcReadByte(driver, TRACK_VELOCITY_ADDRESS + index) << 1) | 1);
    const uint8_t volume =
        (uint8_t)((SpcReadByte(driver, TRACK_VOLUME_ADDRESS + index) << 1) | 1);
    const uint8_t scaled = (uint8_t)((velocity * group) >> 8);

    SpcWriteByte(driver, TRACK_MIXED_VOLUME_ADDRESS + index,
                 (uint8_t)((volume * scaled) >> 8));
}

/* Base pitch of the track's note, multiplied by the instrument tuning. */
void SpcSetNotePitch(Lufia2SpcDriver *driver, int track) {
    SpcVariables *vars = SpcVars(driver);
    SpcTracks *t = SpcTrackState(driver);
    const uint8_t note = t->note[track];
    const uint8_t octave = note / SEMITONES;
    const uint8_t semitone = note % SEMITONES;
    const uint8_t table_hi = SpcReadByte(driver, NOTE_PITCH_HI_TABLE + semitone);
    uint16_t pitch = (uint16_t)(SpcReadByte(driver, NOTE_PITCH_LO_TABLE + semitone) |
                                (table_hi << 8));

    /* Halve once per octave below 8; higher octaves wrap the 8-bit counter. */
    if (table_hi != 0) {
        uint8_t step = octave;
        do {
            pitch >>= 1;
            ++step;
        } while (step != TOP_OCTAVE && pitch != 0);
    }

    /* The original combines the partial products in this order. */
    const uint8_t pitch_lo = (uint8_t)pitch;
    const uint8_t pitch_hi = (uint8_t)(pitch >> 8);
    const uint8_t tune_lo = t->tune_lo[track];
    const uint8_t tune_hi = t->tune_hi[track];
    const uint16_t high_part = (uint16_t)(tune_hi * pitch_hi);
    const uint8_t middle_carry = (uint8_t)((tune_hi * pitch_lo) >> 8);
    const uint16_t sum = (uint16_t)(tune_lo * pitch_lo + high_part);
    const uint16_t low_shift =
        (uint16_t)(((uint8_t)(tune_lo * pitch_hi) << 8) | middle_carry);
    const uint16_t tuned = (uint16_t)(low_shift + sum);

    vars->track_pitch_lo[track] = (uint8_t)tuned;
    vars->track_pitch_hi[track] = (uint8_t)(tuned >> 8);
}

/* Music voices only key on while the CPU has them enabled. */
void SpcQueueKeyOn(Lufia2SpcDriver *driver, int track) {
    SpcVariables *vars = SpcVars(driver);
    uint8_t key_on = (uint8_t)(vars->pending_key_on | VoiceBit(track));
    uint8_t keyed = (uint8_t)(vars->keyed_voices | VoiceBit(track));

    if (!IsEffectTrack(track)) {
        key_on &= vars->music_voice_enable;
        keyed &= vars->music_voice_enable;
    }
    vars->pending_key_on = key_on;
    vars->keyed_voices = keyed;
}

void SpcQueueKeyOff(Lufia2SpcDriver *driver, int track) {
    SpcVariables *vars = SpcVars(driver);

    vars->pending_key_off |= VoiceBit(track);
    vars->keyed_voices &= (uint8_t)~VoiceBit(track);
}

/* Releases first, then keys the new notes on after a short settle. */
void SpcFlushKeys(Lufia2SpcDriver *driver) {
    SpcVariables *vars = SpcVars(driver);

    if (vars->pending_key_off != 0) {
        DspWrite(driver, DSP_KEY_OFF, vars->pending_key_off);
        DspDelay(driver, SPC_KEY_SETTLE_CYCLES);
    }
    if (vars->pending_key_on != 0) {
        DspWrite(driver, DSP_KEY_OFF, 0);
        DspDelay(driver, SPC_KEY_SETTLE_CYCLES);
        DspWrite(driver, DSP_KEY_ON, vars->pending_key_on);
    }
    vars->pending_key_on = 0;
    vars->pending_key_off = 0;
}

static bool TrackSounds(const Lufia2SpcDriver *driver, int track) {
    const uint8_t flags = SpcVars(driver)->track_flags[track];

    return (flags & SPC_TRACK_ACTIVE) && !(flags & SPC_TRACK_MUTED_BY_EFFECT);
}

static uint8_t Negate(uint8_t value) { return (uint8_t)(~value + 1); }

/* Tremolo raises or lowers the level by a sign-magnitude amount. */
static uint8_t ApplyTremolo(const Lufia2SpcDriver *driver, int track, uint8_t level) {
    const SpcTracks *t = SpcTrackState(driver);
    const uint8_t tremolo = t->tremolo_value[track];

    if (tremolo == 0 || t->vibrato_amount[track] == 0)
        return level;
    const uint8_t depth =
        (uint8_t)(((uint8_t)(t->vibrato_amount[track] << 1) * level) >> 8);
    const uint8_t offset = (uint8_t)(((uint8_t)(tremolo << 1) * depth) >> 8);

    if (tremolo & 0x80)
        return (uint8_t)(Negate(offset) + level);
    const uint8_t raised = (uint8_t)(offset + level);
    return (raised & 0x80) ? 0x7F : raised;
}

/* Left and right level for the track's pan position. */
static void PanLevels(const Lufia2SpcDriver *driver, int track, uint8_t level,
                      uint8_t *left, uint8_t *right) {
    const SpcVariables *vars = SpcVars(driver);
    const uint8_t pan = SpcTrackState(driver)->pan[track];

    if ((vars->output_options & SPC_OUTPUT_MONO) || pan == 0x40) {
        *left = (uint8_t)(level >> 1);
        *right = (uint8_t)(level >> 1);
        return;
    }
    const uint8_t borrow = (pan & 0x80) ? 0 : 1;
    const uint8_t scaled = (uint8_t)(((uint8_t)(pan << 1) * level) >> 8);

    *right = scaled;
    *left = (uint8_t)~(uint8_t)(scaled - level - borrow);
}

void SpcWriteVolumes(Lufia2SpcDriver *driver) {
    const SpcVariables *vars = SpcVars(driver);
    const SpcTracks *t = SpcTrackState(driver);
    const SpcSettings *settings = SpcSetup(driver);
    const bool mono = (vars->output_options & SPC_OUTPUT_MONO) != 0;

    for (int track = 0; track < SPC_TRACK_COUNT; ++track) {
        if (!TrackSounds(driver, track))
            continue;
        uint8_t level = t->mixed_volume[track];
        if (!IsEffectTrack(track))
            level = (uint8_t)((level * settings->music_fade) >> 8);
        level = ApplyTremolo(driver, track, level);

        uint8_t left, right;
        PanLevels(driver, track, level, &left, &right);
        if (!mono) {
            if (vars->track_flags[track] & SPC_TRACK_INVERT_LEFT)
                left = Negate(left);
            if (vars->track_flags[track] & SPC_TRACK_INVERT_RIGHT)
                right = Negate(right);
        }
        DspWrite(driver, VoiceRegister(track, DSP_VOLUME_LEFT), left);
        DspWrite(driver, VoiceRegister(track, DSP_VOLUME_RIGHT), right);
    }

    uint8_t echo_right =
        (uint8_t)((settings->echo_volume_right * settings->music_fade) >> 8);
    if (!mono && (vars->echo_phase_flags & SPC_ECHO_INVERT_RIGHT))
        echo_right = Negate(echo_right);
    DspWrite(driver, DSP_ECHO_VOLUME_RIGHT, echo_right);

    uint8_t echo_left =
        (uint8_t)((settings->echo_volume_left * settings->music_fade) >> 8);
    if (!mono && (vars->echo_phase_flags & SPC_ECHO_INVERT_LEFT))
        echo_left = Negate(echo_left);
    DspWrite(driver, DSP_ECHO_VOLUME_LEFT, echo_left);
}

/* Note pitch times the track's pitch scale ($80 = 1.0), low nibble cleared. */
static uint16_t ScaledPitch(const Lufia2SpcDriver *driver, int track) {
    const SpcVariables *vars = SpcVars(driver);
    const uint8_t scale = SpcTrackState(driver)->pitch_scale[track];
    const uint8_t lo = vars->track_pitch_lo[track];
    const uint8_t hi = vars->track_pitch_hi[track];

    if (scale == 0x80)
        return (uint16_t)((hi << 8) | (lo & 0xF0));
    if (scale == 0) {
        const uint16_t doubled = (uint16_t)(((hi << 8) | lo) << 1);
        return (uint16_t)(doubled & 0xFFF0);
    }
    const uint16_t low_product = (uint16_t)(scale * lo);
    const uint16_t high_product = (uint16_t)(scale * hi);
    const unsigned sum = (unsigned)(uint8_t)high_product + (low_product >> 8);
    const uint8_t product_lo = (uint8_t)sum;
    const uint8_t product_hi = (uint8_t)((high_product >> 8) + (sum >> 8));
    const uint16_t doubled = (uint16_t)(((product_hi << 8) | product_lo) << 1);
    return (uint16_t)(doubled & 0xFFF0);
}

/* Vibrato bends the pitch up or down by a sign-magnitude amount. */
static uint16_t ApplyVibrato(const Lufia2SpcDriver *driver, int track, uint16_t pitch) {
    const SpcTracks *t = SpcTrackState(driver);
    const uint8_t amount = t->vibrato_amount[track];

    if (amount == 0)
        return pitch;
    const uint8_t vibrato = t->vibrato_value[track];
    const uint8_t depth = (uint8_t)(((uint8_t)(vibrato << 1) * amount) >> 8);
    const uint8_t low_part = (uint8_t)((depth * (pitch & 0xFF)) >> 8);
    const uint16_t delta = (uint16_t)(depth * (pitch >> 8) + low_part);

    return (vibrato & 0x80) ? (uint16_t)(pitch - delta) : (uint16_t)(pitch + delta);
}

void SpcWritePitches(Lufia2SpcDriver *driver) {
    SpcVariables *vars = SpcVars(driver);

    for (int track = 0; track < SPC_TRACK_COUNT; ++track) {
        if (!TrackSounds(driver, track))
            continue;
        const uint16_t pitch = ApplyVibrato(driver, track, ScaledPitch(driver, track));
        vars->track_output_pitch_lo[track] = (uint8_t)pitch;
        vars->track_output_pitch_hi[track] = (uint8_t)(pitch >> 8);
    }

    /* An effect that took a voice over supplies its pitch. */
    for (int voice = 0; voice < SPC_MUSIC_TRACKS; ++voice) {
        int track = voice;
        if (vars->track_flags[voice] & SPC_TRACK_MUTED_BY_EFFECT)
            track = voice + SPC_MUSIC_TRACKS;
        else if (!(vars->track_flags[voice] & SPC_TRACK_ACTIVE))
            continue;
        DspWrite(driver, VoiceRegister(voice, DSP_PITCH_LO),
                 vars->track_output_pitch_lo[track]);
        DspWrite(driver, VoiceRegister(voice, DSP_PITCH_HI),
                 vars->track_output_pitch_hi[track]);
    }
}

/* One oscillator step; the result is a sign-magnitude sine value. */
static uint8_t StepOscillator(const Lufia2SpcDriver *driver, uint8_t *phase,
                              uint8_t *carry, uint8_t speed, uint8_t depth) {
    const uint8_t sum = (uint8_t)(*carry + speed);

    *carry = sum & 1;
    *phase = (uint8_t)(*phase + (sum >> 1));
    const uint8_t sine = SpcReadByte(driver, SINE_TABLE + (*phase & 0x7F));
    const uint8_t magnitude = (uint8_t)((sine * depth) >> 8);
    return (*phase & 0x80) ? (uint8_t)(magnitude | 0x80) : magnitude;
}

void SpcUpdateModulation(Lufia2SpcDriver *driver) {
    SpcTracks *t = SpcTrackState(driver);

    for (int track = SPC_TRACK_COUNT - 1; track >= 0; --track) {
        if (t->vibrato_amount[track] == 0)
            continue;
        t->vibrato_value[track] =
            StepOscillator(driver, &t->vibrato_phase[track], &t->vibrato_carry[track],
                           t->vibrato_speed[track], t->vibrato_depth[track]);
        if (t->tremolo_depth[track] == 0)
            continue;
        t->tremolo_value[track] =
            StepOscillator(driver, &t->tremolo_phase[track], &t->tremolo_carry[track],
                           t->tremolo_speed[track], t->tremolo_depth[track]);
    }
}
