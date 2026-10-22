/* Music sequencer: songs, order lists, patterns and pattern commands. */

#include "spc/spc_driver_internal.h"

/* Song header at $4400. */
enum {
    SONG_TEMPO = SPC_SONG_ADDRESS + 0x03,
    SONG_TRACK_MODES = SPC_SONG_ADDRESS + 0x10, /* $FF = unused track */
    SONG_ECHO_DELAY = SPC_SONG_ADDRESS + 0x19,
    SONG_ORDER_LISTS = SPC_SONG_ADDRESS + 0x20, /* page offset, low byte */
    SONG_PAGE = SPC_SONG_ADDRESS >> 8,
    SONG_TRACK_UNUSED = 0xFF,
    SONG_TRACK_PERCUSSION = 0x80,
};

/* Order lists: a byte >= $80 sets the transpose, $FF ends the track,
 * anything else starts a two-byte pattern address relative to the song. */
enum {
    ORDER_TRANSPOSE = 0x80,
    ORDER_END = 0xFF,
};

/* Pattern events. */
enum {
    EVENT_SHORT_NOTE = 0x80, /* $80-$EF: note with the previous timing */
    EVENT_COMMAND = 0xF0,
    NOTE_EVENT_SIZE = 4,
};

enum {
    CMD_VIBRATO = 0xF0,
    CMD_VOLUME = 0xF1,
    CMD_PAN = 0xF2,
    CMD_WAIT = 0xF3,
    CMD_TEMPO = 0xF4,
    CMD_PITCH_SCALE = 0xF6,
    CMD_INSTRUMENT = 0xF7,
    CMD_LOOP_START = 0xFB,
    CMD_LOOP_END = 0xFC,
    CMD_PATTERN_END = 0xFD,
    CMD_TRACK_OFF = 0xFE,
    CMD_EXTENDED = 0xFF,
};

/* Sub-commands of $FF. */
enum {
    EXT_ECHO_DELAY,
    EXT_ECHO_FEEDBACK,
    EXT_ECHO_FILTER,
    EXT_ECHO_ON,
    EXT_ECHO_OFF,
    EXT_NOP_5,
    EXT_NOP_6,
    EXT_NOP_7,
    EXT_ATTACK,
    EXT_DECAY,
    EXT_SUSTAIN_LEVEL,
    EXT_SUSTAIN_RATE,
    EXT_ENVELOPE,
    EXT_ECHO_VOLUME,
    EXT_ECHO_VOLUME_RIGHT,
    EXT_ECHO_VOLUME_LEFT,
    EXT_NOISE_CLOCK,
    EXT_NOISE_ON,
    EXT_NOISE_OFF,
    EXT_PITCH_MOD_ON,
    EXT_PITCH_MOD_OFF,
    EXT_TREMOLO_DEPTH,
    EXT_VIBRATO_DEPTH,
    EXT_TREMOLO_SPEED,
    EXT_VIBRATO_SPEED,
    EXT_INVERT_LEFT,
    EXT_INVERT_RIGHT,
    EXT_INVERT_NONE,
    EXT_ECHO_INVERT_LEFT,
    EXT_ECHO_INVERT_RIGHT,
    EXT_ECHO_INVERT_NONE,
    EXT_COUNT,
};

enum { LOOP_FIRST_PASS = 0xFE, LOOP_FOREVER = 0xFF };

/* Percussion: note -> pitch, pan and instrument, zero-terminated. */
enum {
    DRUM_NOTES = 0x3C10,
    DRUM_PITCH_LO = 0x3C1A,
    DRUM_PITCH_HI = 0x3C23,
    DRUM_PAN = 0x3C2C,
    DRUM_INSTRUMENT = 0x3C35,
};

/* $8D + track; effect tracks reach the bytes after the music counters. */
enum { PATTERNS_PLAYED_ADDRESS = 0x8D };

static uint16_t OrderCursor(const Lufia2SpcDriver *driver, int track) {
    const SpcTracks *t = SpcTrackState(driver);
    return (uint16_t)(t->order_lo[track] | (t->order_hi[track] << 8));
}

static void SetOrderCursor(Lufia2SpcDriver *driver, int track, uint16_t address) {
    SpcTracks *t = SpcTrackState(driver);
    t->order_lo[track] = (uint8_t)address;
    t->order_hi[track] = (uint8_t)(address >> 8);
}

/* Reads the order list from the cursor up to the next pattern entry. */
void SpcEnterPattern(Lufia2SpcDriver *driver, int track, uint16_t order) {
    SpcTracks *t = SpcTrackState(driver);

    for (;;) {
        const uint8_t entry = SpcReadByte(driver, order);
        if (!(entry & ORDER_TRANSPOSE)) {
            t->pattern_hi[track] = (uint8_t)(entry + SONG_PAGE);
            t->pattern_lo[track] = SpcReadByte(driver, (uint16_t)(order + 1));
            return;
        }
        t->transpose[track] = entry & 0x7F;
        order = (uint16_t)(OrderCursor(driver, track) + 1);
        SetOrderCursor(driver, track, order);
    }
}

void SpcStartSong(Lufia2SpcDriver *driver) {
    SpcVariables *vars = SpcVars(driver);
    SpcTracks *t = SpcTrackState(driver);
    SpcSettings *settings = SpcSetup(driver);
    const Lufia2SpcHardware *hardware = driver->hardware;

    vars->music_noise_voices = 0;
    vars->music_pitch_mod_voices = 0;
    vars->music_echo_voices = 0;
    for (int track = 0; track < SPC_MUSIC_TRACKS; ++track) {
        const uint8_t mode = SpcReadByte(driver, (uint16_t)(SONG_TRACK_MODES + track));
        vars->track_flags[track] &= SPC_TRACK_MUTED_BY_EFFECT;
        if (mode != SONG_TRACK_UNUSED)
            vars->track_flags[track] |=
                (uint8_t)((mode & SONG_TRACK_PERCUSSION) | SPC_TRACK_ACTIVE);
    }
    for (int track = 0; track < SPC_MUSIC_TRACKS; ++track) {
        const uint16_t entry = (uint16_t)(SONG_ORDER_LISTS + 2 * track);

        SpcResetTrack(driver, track);
        vars->track_wait[track] = 1;
        vars->patterns_played[track] = 0;
        t->order_lo[track] = SpcReadByte(driver, entry);
        t->order_hi[track] =
            (uint8_t)(SpcReadByte(driver, (uint16_t)(entry + 1)) + SONG_PAGE);
        SpcEnterPattern(driver, track, OrderCursor(driver, track));
        SpcSetInstrument(driver, track, 0);
    }
    SpcWritePitchModulation(driver);
    SpcWriteNoiseEnable(driver);
    settings->echo_delay = SpcReadByte(driver, SONG_ECHO_DELAY);
    settings->echo_feedback = 0x50;
    settings->echo_filter = 0;
    SpcWriteEchoFilter(driver);
    SpcSetupEcho(driver);
    SpcSetEchoVolume(driver, 0x54);
    hardware->write_control(hardware->context, 0);
    hardware->write_timer_target(hardware->context, SPC_TIMER_TEMPO,
                                 SpcReadByte(driver, SONG_TEMPO));
    hardware->write_timer_target(hardware->context, SPC_TIMER_EFFECTS, 0x7D);
    hardware->write_control(hardware->context, 0x03);
    vars->status |= SPC_STATUS_MUSIC_PLAYING;
}

void SpcStopMusic(Lufia2SpcDriver *driver) {
    SpcVariables *vars = SpcVars(driver);

    vars->status &= (uint8_t)~(SPC_STATUS_MUSIC_PLAYING | SPC_STATUS_FADING_OUT |
                               SPC_STATUS_FADING_IN);
    vars->music_noise_voices = 0;
    vars->music_pitch_mod_voices = 0;
    vars->music_echo_voices = 0;
    SpcWriteNoiseEnable(driver);
    SpcWritePitchModulation(driver);
    SpcWriteEchoEnable(driver);
    SpcSetEchoVolume(driver, 0);
    vars->music_voice_enable = 0xFF;
    for (int track = SPC_MUSIC_TRACKS - 1; track >= 0; --track) {
        const uint8_t flags = vars->track_flags[track];
        if ((flags & SPC_TRACK_ACTIVE) && !(flags & SPC_TRACK_MUTED_BY_EFFECT))
            SpcQueueKeyOff(driver, track);
        vars->track_flags[track] = flags & SPC_TRACK_MUTED_BY_EFFECT;
    }
    vars->pending_key_on = 0;
    SpcFlushKeys(driver);
    DspWrite(driver, DSP_FLAGS,
             (uint8_t)(SpcSetup(driver)->dsp_flags | DSP_FLAG_ECHO_WRITE_OFF));
}

void SpcMusicTick(Lufia2SpcDriver *driver) {
    SpcVariables *vars = SpcVars(driver);

    for (int track = 0; track < SPC_MUSIC_TRACKS; ++track) {
        if (!(vars->track_flags[track] & SPC_TRACK_ACTIVE))
            continue;
        if (vars->track_gate[track] != 0 && --vars->track_gate[track] == 0 &&
            !(vars->track_flags[track] & SPC_TRACK_MUTED_BY_EFFECT))
            SpcQueueKeyOff(driver, track);
        if (--vars->track_wait[track] != 0)
            continue;
        SpcProcessTrackEvents(driver, track);

        /* A voice an effect gave back gets its instrument again. */
        const uint8_t flags = vars->track_flags[track];
        if ((flags & SPC_TRACK_PERCUSSION) || (flags & SPC_TRACK_MUTED_BY_EFFECT))
            continue;
        if (vars->instrument_restore_voices & VoiceBit(track)) {
            vars->instrument_restore_voices &= (uint8_t)~VoiceBit(track);
            SpcSetInstrument(driver, track, SpcTrackState(driver)->instrument[track]);
        }
    }
    SpcWriteVolumes(driver);
    SpcWritePitches(driver);
    SpcFlushKeys(driver);
}

/* The note just played becomes the "previous" note. */
static void RememberNote(Lufia2SpcDriver *driver, int track) {
    SpcTracks *t = SpcTrackState(driver);

    t->previous_note[track] = t->note[track];
    t->previous_length[track] = t->length[track];
    t->previous_gate_time[track] = t->gate_time[track];
    t->previous_velocity[track] = t->velocity[track];
}

static void StartTrackNote(Lufia2SpcDriver *driver, int track) {
    SpcVariables *vars = SpcVars(driver);
    SpcTracks *t = SpcTrackState(driver);

    vars->track_wait[track] = t->length[track];
    vars->track_gate[track] = t->gate_time[track];
}

/* Drum notes set a fixed pitch, pan and instrument; others are ignored. */
static void PlayPercussion(Lufia2SpcDriver *driver, int track) {
    SpcVariables *vars = SpcVars(driver);
    SpcTracks *t = SpcTrackState(driver);

    SpcQueueKeyOn(driver, track);
    for (uint8_t drum = 0;; ++drum) {
        const uint8_t note = SpcReadByte(driver, (uint16_t)(DRUM_NOTES + drum));
        if (note == 0)
            return;
        if (note != t->note[track])
            continue;
        vars->track_pitch_lo[track] =
            SpcReadByte(driver, (uint16_t)(DRUM_PITCH_LO + drum));
        vars->track_pitch_hi[track] =
            SpcReadByte(driver, (uint16_t)(DRUM_PITCH_HI + drum));
        t->pan[track] = SpcReadByte(driver, (uint16_t)(DRUM_PAN + drum));
        SpcSetInstrument(driver, track,
                         SpcReadByte(driver, (uint16_t)(DRUM_INSTRUMENT + drum)));
        StartTrackNote(driver, track);
        return;
    }
}

/* A held note on the same pitch (a tie) does not key on again. */
static void SoundNote(Lufia2SpcDriver *driver, int track) {
    const SpcVariables *vars = SpcVars(driver);
    const SpcTracks *t = SpcTrackState(driver);
    const uint8_t flags = vars->track_flags[track];

    if (!(flags & SPC_TRACK_MUTED_BY_EFFECT) && t->gate_time[track] != 0) {
        if (flags & SPC_TRACK_PERCUSSION) {
            PlayPercussion(driver, track);
            return;
        }
        const bool tie =
            vars->track_gate[track] != 0 && t->previous_note[track] == t->note[track];
        if (!tie) {
            SpcSetNotePitch(driver, track);
            SpcQueueKeyOn(driver, track);
        }
    }
    StartTrackNote(driver, track);
}

static uint8_t PatternByte(const Lufia2SpcDriver *driver, uint16_t pattern,
                           int offset) {
    return SpcReadByte(driver, (uint16_t)(pattern + offset));
}

/* Moves the event cursor; true when the track must read on now. */
static bool AdvancePattern(Lufia2SpcDriver *driver, int track, uint16_t pattern,
                           int size) {
    SetTrackPattern(driver, track, (uint16_t)(pattern + size));
    return SpcVars(driver)->track_wait[track] == 0;
}

static void SetVoiceMask(uint8_t *music, uint8_t *effects, int track, bool on) {
    uint8_t *mask = IsEffectTrack(track) ? effects : music;
    *mask =
        on ? (uint8_t)(*mask | VoiceBit(track)) : (uint8_t)(*mask & ~VoiceBit(track));
}

/* Read-modify-write of one ADSR register of the track's voice. */
static void ChangeEnvelope(Lufia2SpcDriver *driver, int track, uint8_t reg,
                           uint8_t keep, uint8_t value) {
    const uint8_t dsp_reg = VoiceRegister(track, reg);
    DspWrite(driver, dsp_reg, (uint8_t)((DspRead(driver, dsp_reg) & keep) | value));
}

static uint8_t SwapNibbles(uint8_t value) {
    return (uint8_t)((value << 4) | (value >> 4));
}

static uint8_t ScaleByMaster(const Lufia2SpcDriver *driver, uint8_t level) {
    return (uint8_t)((level * SpcSetup(driver)->master_volume) >> 7);
}

/* $FF sub-commands; all of them read one parameter byte. */
static void RunExtendedCommand(Lufia2SpcDriver *driver, int track, uint8_t command,
                               uint8_t value) {
    SpcVariables *vars = SpcVars(driver);
    SpcTracks *t = SpcTrackState(driver);
    SpcSettings *settings = SpcSetup(driver);

    switch (command) {
    case EXT_ECHO_DELAY:
        settings->echo_delay = value;
        break;
    case EXT_ECHO_FEEDBACK:
        settings->echo_feedback = value;
        DspWrite(driver, DSP_ECHO_FEEDBACK, value);
        break;
    case EXT_ECHO_FILTER:
        settings->echo_filter = value;
        SpcWriteEchoFilter(driver);
        break;
    case EXT_ECHO_ON:
    case EXT_ECHO_OFF:
        SetVoiceMask(&vars->music_echo_voices, &vars->effect_echo_voices, track,
                     command == EXT_ECHO_ON);
        SpcWriteEchoEnable(driver);
        break;
    case EXT_NOP_5:
    case EXT_NOP_6:
    case EXT_NOP_7:
        break;
    case EXT_ATTACK:
        ChangeEnvelope(driver, track, DSP_ADSR1, 0xF0, value & 0x0F);
        break;
    case EXT_DECAY:
        ChangeEnvelope(driver, track, DSP_ADSR1, 0x0F,
                       (uint8_t)((SwapNibbles(value) & 0x70) | 0x80));
        break;
    case EXT_SUSTAIN_LEVEL:
        ChangeEnvelope(driver, track, DSP_ADSR2, 0x1F,
                       (uint8_t)((SwapNibbles(value) << 1) & 0xE0));
        break;
    case EXT_SUSTAIN_RATE:
        ChangeEnvelope(driver, track, DSP_ADSR2, 0xE0, value & 0x1F);
        break;
    case EXT_ENVELOPE:
        t->envelope_setting[track] = (uint8_t)((value & 0x1F) | 0x80);
        break;
    case EXT_ECHO_VOLUME:
        SpcSetEchoVolume(driver, value);
        break;
    case EXT_ECHO_VOLUME_RIGHT:
        settings->echo_volume_right = ScaleByMaster(driver, value);
        break;
    case EXT_ECHO_VOLUME_LEFT:
        settings->echo_volume_left = ScaleByMaster(driver, value);
        break;
    case EXT_NOISE_CLOCK:
        settings->dsp_flags = (uint8_t)((settings->dsp_flags & ~DSP_FLAG_NOISE_CLOCK) |
                                        (value & DSP_FLAG_NOISE_CLOCK));
        DspWrite(driver, DSP_FLAGS, settings->dsp_flags);
        break;
    case EXT_NOISE_ON:
    case EXT_NOISE_OFF:
        SetVoiceMask(&vars->music_noise_voices, &vars->effect_noise_voices, track,
                     command == EXT_NOISE_ON);
        SpcWriteNoiseEnable(driver);
        break;
    case EXT_PITCH_MOD_ON:
    case EXT_PITCH_MOD_OFF:
        SetVoiceMask(&vars->music_pitch_mod_voices, &vars->effect_pitch_mod_voices,
                     track, command == EXT_PITCH_MOD_ON);
        SpcWritePitchModulation(driver);
        break;
    case EXT_TREMOLO_DEPTH:
        t->tremolo_depth[track] = value;
        break;
    case EXT_VIBRATO_DEPTH:
        t->vibrato_depth[track] = value;
        break;
    case EXT_TREMOLO_SPEED:
        t->tremolo_speed[track] = value;
        break;
    case EXT_VIBRATO_SPEED:
        t->vibrato_speed[track] = value;
        break;
    case EXT_INVERT_LEFT:
        vars->track_flags[track] |= SPC_TRACK_INVERT_LEFT;
        break;
    case EXT_INVERT_RIGHT:
        vars->track_flags[track] |= SPC_TRACK_INVERT_RIGHT;
        break;
    case EXT_INVERT_NONE:
        vars->track_flags[track] &=
            (uint8_t)~(SPC_TRACK_INVERT_LEFT | SPC_TRACK_INVERT_RIGHT);
        break;
    case EXT_ECHO_INVERT_LEFT:
        vars->echo_phase_flags |= SPC_ECHO_INVERT_LEFT;
        break;
    case EXT_ECHO_INVERT_RIGHT:
        vars->echo_phase_flags |= SPC_ECHO_INVERT_RIGHT;
        break;
    case EXT_ECHO_INVERT_NONE:
        vars->echo_phase_flags &=
            (uint8_t)~(SPC_ECHO_INVERT_LEFT | SPC_ECHO_INVERT_RIGHT);
        break;
    }
}

/* $FB remembers where the loop body starts, in the first free level. */
static void StartLoop(Lufia2SpcDriver *driver, int track) {
    SpcTracks *t = SpcTrackState(driver);
    int level;

    if (t->loop_order_hi[0][track] == 0)
        level = 0;
    else if (t->loop_order_hi[1][track] == 0)
        level = 1;
    else
        return;
    const uint16_t body = (uint16_t)(TrackPattern(driver, track) + 1);
    t->loop_order_lo[level][track] = t->order_lo[track];
    t->loop_order_hi[level][track] = t->order_hi[track];
    t->loop_pattern_lo[level][track] = (uint8_t)body;
    t->loop_pattern_hi[level][track] = (uint8_t)(body >> 8);
    t->loop_count[level][track] = LOOP_FIRST_PASS;
    t->loop_transpose[level][track] = t->transpose[track];
}

/* $FC n: true when the track jumps back to the loop start. */
static bool EndLoop(Lufia2SpcDriver *driver, int track, uint8_t passes) {
    SpcTracks *t = SpcTrackState(driver);
    const uint8_t repeats = (uint8_t)(passes - 1);

    if (repeats == 0)
        return false;
    const int level = t->loop_order_hi[1][track] != 0 ? 1 : 0;
    const uint8_t count = t->loop_count[level][track];
    if (count == LOOP_FIRST_PASS) {
        t->loop_count[level][track] = repeats;
    } else if (count != LOOP_FOREVER) {
        t->loop_count[level][track] = (uint8_t)(count - 1);
        if (t->loop_count[level][track] == 0) {
            t->loop_order_hi[level][track] = 0;
            return false;
        }
    }
    t->order_lo[track] = t->loop_order_lo[level][track];
    t->order_hi[track] = t->loop_order_hi[level][track];
    t->pattern_lo[track] = t->loop_pattern_lo[level][track];
    t->pattern_hi[track] = t->loop_pattern_hi[level][track];
    t->transpose[track] = t->loop_transpose[level][track];
    return true;
}

/* $FD: true while the track has another pattern. */
static bool NextPattern(Lufia2SpcDriver *driver, int track) {
    SpcVariables *vars = SpcVars(driver);
    SpcTracks *t = SpcTrackState(driver);

    t->transpose[track] = 0;
    SpcWriteByte(
        driver, (uint16_t)(PATTERNS_PLAYED_ADDRESS + track),
        (uint8_t)(SpcReadByte(driver, (uint16_t)(PATTERNS_PLAYED_ADDRESS + track)) +
                  1));
    const uint16_t order = (uint16_t)(OrderCursor(driver, track) + 2);
    SetOrderCursor(driver, track, order);
    if (SpcReadByte(driver, order) != ORDER_END) {
        SpcEnterPattern(driver, track, order);
        return true;
    }

    vars->track_flags[track] &= (uint8_t)~SPC_TRACK_ACTIVE;
    uint8_t any_active = 0;
    for (int music = SPC_MUSIC_TRACKS - 1; music >= 0; --music)
        any_active |= vars->track_flags[music];
    if (!(any_active & SPC_TRACK_ACTIVE))
        vars->status &= (uint8_t)~SPC_STATUS_MUSIC_PLAYING;
    vars->track_flags[track] &= SPC_TRACK_MUTED_BY_EFFECT;
    if (vars->track_flags[track] == 0)
        SpcQueueKeyOff(driver, track);
    return false;
}

static void SilenceTrack(Lufia2SpcDriver *driver, int track) {
    SpcTracks *t = SpcTrackState(driver);

    SpcVars(driver)->track_flags[track] = 0;
    t->vibrato_amount[track] = 0;
    t->pitch_scale[track] = 0x80;
    t->vibrato_depth[track] = 0x20;
    t->tremolo_depth[track] = 0x10;
    SpcQueueKeyOff(driver, track);
}

/* Runs one pattern command; true when the track reads on immediately. */
static bool RunPatternCommand(Lufia2SpcDriver *driver, int track, uint16_t pattern,
                              uint8_t command) {
    SpcVariables *vars = SpcVars(driver);
    SpcTracks *t = SpcTrackState(driver);
    const Lufia2SpcHardware *hardware = driver->hardware;
    const uint8_t first = PatternByte(driver, pattern, 1);
    const uint8_t second = PatternByte(driver, pattern, 2);

    switch (command) {
    case CMD_VIBRATO:
        vars->track_wait[track] = first;
        if (t->vibrato_amount[track] == 0) {
            t->vibrato_phase[track] = 0;
            t->tremolo_phase[track] = 0;
        }
        t->vibrato_amount[track] = second;
        return AdvancePattern(driver, track, pattern, 3);
    case CMD_VOLUME:
        vars->track_wait[track] = first;
        t->volume[track] = second;
        SpcUpdateTrackVolume(driver, track);
        return AdvancePattern(driver, track, pattern, 3);
    case CMD_PAN:
        vars->track_wait[track] = first;
        t->pan[track] = second;
        return AdvancePattern(driver, track, pattern, 3);
    case CMD_WAIT:
        vars->track_wait[track] = first;
        return AdvancePattern(driver, track, pattern, 2);
    case CMD_TEMPO:
        vars->track_wait[track] = first;
        hardware->write_control(hardware->context, 0);
        hardware->write_timer_target(hardware->context, SPC_TIMER_TEMPO, second);
        hardware->write_timer_target(hardware->context, SPC_TIMER_EFFECTS, 0x7D);
        hardware->write_control(hardware->context, 0x03);
        return AdvancePattern(driver, track, pattern, 3);
    case CMD_PITCH_SCALE:
        vars->track_wait[track] = first;
        t->pitch_scale[track] = second;
        return AdvancePattern(driver, track, pattern, 3);
    case CMD_INSTRUMENT:
        vars->track_wait[track] = first;
        t->instrument[track] = second;
        if (!(vars->track_flags[track] & SPC_TRACK_MUTED_BY_EFFECT))
            SpcSetInstrument(driver, track, second);
        return AdvancePattern(driver, track, pattern, 3);
    case CMD_LOOP_START:
        StartLoop(driver, track);
        return AdvancePattern(driver, track, pattern, 1);
    case CMD_LOOP_END:
        if (EndLoop(driver, track, first))
            return true;
        return AdvancePattern(driver, track, pattern, 2);
    case CMD_PATTERN_END:
        return NextPattern(driver, track);
    case CMD_TRACK_OFF:
        SilenceTrack(driver, track);
        return false;
    case CMD_EXTENDED:
        if (first < EXT_COUNT)
            RunExtendedCommand(driver, track, first, second);
        return AdvancePattern(driver, track, pattern, 3);
    default:
        /* $F5 and $F8-$FA corrupt the original's stack; no song uses them. */
        return AdvancePattern(driver, track, pattern, 1);
    }
}

void SpcProcessTrackEvents(Lufia2SpcDriver *driver, int track) {
    SpcTracks *t = SpcTrackState(driver);

    for (;;) {
        const uint16_t pattern = TrackPattern(driver, track);
        const uint8_t event = PatternByte(driver, pattern, 0);

        if (event >= EVENT_COMMAND) {
            if (!RunPatternCommand(driver, track, pattern, event))
                return;
            continue;
        }

        RememberNote(driver, track);
        if (event < EVENT_SHORT_NOTE) {
            t->note[track] = (uint8_t)(event + t->transpose[track]);
            t->length[track] = PatternByte(driver, pattern, 1);
            t->gate_time[track] = PatternByte(driver, pattern, 2);
            t->velocity[track] = PatternByte(driver, pattern, 3);
            SpcUpdateTrackVolume(driver, track);
            SetTrackPattern(driver, track, (uint16_t)(pattern + NOTE_EVENT_SIZE));
        } else {
            t->note[track] = (uint8_t)((event & 0x7F) + t->transpose[track]);
            t->length[track] = t->previous_length[track];
            t->gate_time[track] = t->previous_gate_time[track];
            t->velocity[track] = t->previous_velocity[track];
            SpcUpdateTrackVolume(driver, track);
            SetTrackPattern(driver, track, (uint16_t)(pattern + 1));
        }
        SoundNote(driver, track);
        return;
    }
}
