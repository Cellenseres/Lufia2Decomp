#ifndef LUFIA2_SPC_RAM_H
#define LUFIA2_SPC_RAM_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Audio RAM layout of the Lufia II sound driver. The driver keeps all of its
 * persistent state in ARAM at the addresses the original SPC-700 program
 * uses, so the native driver and the original are interchangeable and host
 * code can read the same variables.
 *
 * Voices: 0-7 are the music tracks, 8-15 the sound-effect tracks. Effect
 * track n plays on DSP voice n - 8 and takes it over from music track n - 8.
 */

enum {
    SPC_TRACK_COUNT = 16,
    SPC_MUSIC_TRACKS = 8,
    SPC_LOOP_LEVELS = 2,
    SPC_DIRECTORY_ENTRIES = 64,
    SPC_INSTRUMENT_SLOTS = 64,
};

/* Bits of SpcVariables.track_flags. */
enum {
    SPC_TRACK_ACTIVE = 0x01,
    SPC_TRACK_MUTED_BY_EFFECT = 0x02,
    SPC_TRACK_INVERT_LEFT = 0x20,
    SPC_TRACK_INVERT_RIGHT = 0x40,
    SPC_TRACK_PERCUSSION = 0x80,
};

/* Bits of SpcVariables.status, reported to the CPU by command $0F. */
enum {
    SPC_STATUS_MUSIC_PLAYING = 0x01,
    SPC_STATUS_EFFECTS_PLAYING = 0x02,
    SPC_STATUS_FADING_OUT = 0x04,
    SPC_STATUS_FADING_IN = 0x08,
};

/* Bits of SpcVariables.echo_phase_flags. */
enum {
    SPC_ECHO_INVERT_RIGHT = 0x08,
    SPC_ECHO_INVERT_LEFT = 0x10,
};

/* Bit of SpcVariables.output_options. */
enum { SPC_OUTPUT_MONO = 0x01 };

typedef struct SpcWord {
    uint8_t lo;
    uint8_t hi;
} SpcWord;

static inline uint16_t SpcWordValue(SpcWord word) {
    return (uint16_t)(word.lo | (word.hi << 8));
}

static inline void SpcWordSet(SpcWord *word, uint16_t value) {
    word->lo = (uint8_t)value;
    word->hi = (uint8_t)(value >> 8);
}

/* Direct page $00-$EF. Bytes below $1D are scratch of single routines. */
typedef struct SpcVariables {
    uint8_t scratch[0x1D];
    uint8_t track_wait[SPC_TRACK_COUNT];            /* $1D ticks to next event */
    uint8_t track_gate[SPC_TRACK_COUNT];            /* $2D ticks to key-off */
    uint8_t track_flags[SPC_TRACK_COUNT];           /* $3D */
    uint8_t track_pitch_lo[SPC_TRACK_COUNT];        /* $4D note pitch */
    uint8_t track_pitch_hi[SPC_TRACK_COUNT];        /* $5D */
    uint8_t track_output_pitch_lo[SPC_TRACK_COUNT]; /* $6D tuned pitch */
    uint8_t track_output_pitch_hi[SPC_TRACK_COUNT]; /* $7D */
    uint8_t patterns_played[SPC_MUSIC_TRACKS];      /* $8D, track 0 read by $08 */
    uint8_t pending_key_on;                         /* $95 */
    uint8_t pending_key_off;                        /* $96 */
    uint8_t unused_97;
    uint8_t echo_phase_flags;   /* $98 */
    uint8_t output_options;     /* $99 */
    uint8_t music_voice_enable; /* $9A */
    uint8_t status;             /* $9B */
    SpcWord sample_alloc;       /* $9C next free sample byte */
    uint8_t instrument_offset;  /* $9E 4 * next instrument */
    uint8_t unused_9f;
    uint8_t keyed_voices;            /* $A0 */
    uint8_t music_noise_voices;      /* $A1 */
    uint8_t effect_noise_voices;     /* $A2 */
    uint8_t music_echo_voices;       /* $A3 */
    uint8_t effect_echo_voices;      /* $A4 */
    uint8_t music_pitch_mod_voices;  /* $A5 */
    uint8_t effect_pitch_mod_voices; /* $A6 */
    uint8_t music_owned_voices;      /* $A7 */
    SpcWord upload_cursor;           /* $A8 */
    uint8_t unused_aa[3];
    uint8_t upload_started;               /* $AD, written only */
    uint8_t effect_ids[SPC_MUSIC_TRACKS]; /* $AE id per DSP voice */
    uint8_t unused_b6;                    /* $B6, cleared only */
    uint8_t instrument_restore_voices;    /* $B7 */
    uint8_t music_owned_before;           /* $B8 */
    uint8_t echo_restore_delay;           /* $B9 */
    uint8_t unused_ba[0xF0 - 0xBA];
} SpcVariables;

/* Per-track state $0603-$08C2, sixteen entries per field. */
typedef struct SpcTracks {
    uint8_t order_lo[SPC_TRACK_COUNT];   /* $0603 order list cursor */
    uint8_t order_hi[SPC_TRACK_COUNT];   /* $0613 */
    uint8_t transpose[SPC_TRACK_COUNT];  /* $0623 */
    uint8_t pattern_lo[SPC_TRACK_COUNT]; /* $0633 event cursor */
    uint8_t pattern_hi[SPC_TRACK_COUNT]; /* $0643 */
    uint8_t loop_order_lo[SPC_LOOP_LEVELS][SPC_TRACK_COUNT];   /* $0653 */
    uint8_t loop_order_hi[SPC_LOOP_LEVELS][SPC_TRACK_COUNT];   /* $0673 */
    uint8_t loop_pattern_lo[SPC_LOOP_LEVELS][SPC_TRACK_COUNT]; /* $0693 */
    uint8_t loop_pattern_hi[SPC_LOOP_LEVELS][SPC_TRACK_COUNT]; /* $06B3 */
    uint8_t loop_count[SPC_LOOP_LEVELS][SPC_TRACK_COUNT];      /* $06D3 */
    uint8_t loop_transpose[SPC_LOOP_LEVELS][SPC_TRACK_COUNT];  /* $06F3 */
    uint8_t instrument[SPC_TRACK_COUNT];                       /* $0713 */
    uint8_t volume[SPC_TRACK_COUNT];                           /* $0723 */
    uint8_t pan[SPC_TRACK_COUNT];                              /* $0733 */
    uint8_t note[SPC_TRACK_COUNT];                             /* $0743 */
    uint8_t length[SPC_TRACK_COUNT];                           /* $0753 */
    uint8_t gate_time[SPC_TRACK_COUNT];                        /* $0763 */
    uint8_t velocity[SPC_TRACK_COUNT];                         /* $0773 */
    uint8_t previous_note[SPC_TRACK_COUNT];                    /* $0783 */
    uint8_t previous_length[SPC_TRACK_COUNT];                  /* $0793 */
    uint8_t previous_gate_time[SPC_TRACK_COUNT];               /* $07A3 */
    uint8_t previous_velocity[SPC_TRACK_COUNT];                /* $07B3 */
    uint8_t pitch_scale[SPC_TRACK_COUNT];      /* $07C3, $80 = unscaled */
    uint8_t envelope_setting[SPC_TRACK_COUNT]; /* $07D3, written only */
    uint8_t vibrato_phase[SPC_TRACK_COUNT];    /* $07E3 */
    uint8_t vibrato_speed[SPC_TRACK_COUNT];    /* $07F3 */
    uint8_t vibrato_carry[SPC_TRACK_COUNT];    /* $0803 */
    uint8_t vibrato_depth[SPC_TRACK_COUNT];    /* $0813 */
    uint8_t vibrato_value[SPC_TRACK_COUNT];    /* $0823 sign-magnitude */
    uint8_t tremolo_phase[SPC_TRACK_COUNT];    /* $0833 */
    uint8_t tremolo_speed[SPC_TRACK_COUNT];    /* $0843 */
    uint8_t tremolo_carry[SPC_TRACK_COUNT];    /* $0853 */
    uint8_t tremolo_depth[SPC_TRACK_COUNT];    /* $0863 */
    uint8_t tremolo_value[SPC_TRACK_COUNT];    /* $0873 sign-magnitude */
    uint8_t vibrato_amount[SPC_TRACK_COUNT];   /* $0883 */
    uint8_t tune_lo[SPC_TRACK_COUNT];          /* $0893 instrument tuning */
    uint8_t tune_hi[SPC_TRACK_COUNT];          /* $08A3 */
    uint8_t mixed_volume[SPC_TRACK_COUNT];     /* $08B3 */
} SpcTracks;

/* Global settings $08C3-$08D1. */
typedef struct SpcSettings {
    uint8_t master_volume; /* $08C3 */
    uint8_t unused_08c4;
    uint8_t echo_volume_right; /* $08C5 */
    uint8_t echo_volume_left;  /* $08C6 */
    uint8_t echo_filter;       /* $08C7 FIR preset */
    uint8_t echo_feedback;     /* $08C8 */
    uint8_t echo_delay;        /* $08C9 */
    uint8_t music_fade;        /* $08CA */
    uint8_t fade_step;         /* $08CB */
    uint8_t music_volume;      /* $08CC */
    uint8_t effect_volume;     /* $08CD */
    uint8_t dsp_flags;         /* $08CE FLG shadow */
    uint8_t unused_08cf;
    uint8_t double_tempo;  /* $08D0 */
    uint8_t effect_serial; /* $08D1 */
} SpcSettings;

typedef struct SpcInstrument {
    uint8_t adsr1;
    uint8_t adsr2;
    uint8_t tune_lo;
    uint8_t tune_hi;
} SpcInstrument;

typedef struct SpcDirectoryEntry {
    SpcWord start;
    SpcWord loop;
} SpcDirectoryEntry;

enum {
    SPC_DIRECTORY_ADDRESS = 0x0200,
    SPC_TRACKS_ADDRESS = 0x0603,
    SPC_SETTINGS_ADDRESS = 0x08C3,
    SPC_INSTRUMENTS_ADDRESS = 0x08D2,
    SPC_EFFECT_TABLE_ADDRESS = 0x09D2,
    SPC_SONG_ADDRESS = 0x4400,
};

typedef union SpcRam {
    uint8_t bytes[0x10000];
    struct {
        SpcVariables vars;
        uint8_t io_and_stack[SPC_DIRECTORY_ADDRESS - sizeof(SpcVariables)];
        SpcDirectoryEntry directory[SPC_DIRECTORY_ENTRIES];
        uint8_t unused_0300[SPC_TRACKS_ADDRESS - 0x0300];
        SpcTracks tracks;
        SpcSettings settings;
        SpcInstrument instruments[SPC_INSTRUMENT_SLOTS];
    } layout;
} SpcRam;

static_assert(offsetof(SpcVariables, track_wait) == 0x1D, "track_wait");
static_assert(offsetof(SpcVariables, patterns_played) == 0x8D, "patterns_played");
static_assert(offsetof(SpcVariables, status) == 0x9B, "status");
static_assert(offsetof(SpcVariables, effect_ids) == 0xAE, "effect_ids");
static_assert(offsetof(SpcVariables, echo_restore_delay) == 0xB9, "echo_delay");
static_assert(sizeof(SpcVariables) == 0xF0, "direct page");
static_assert(offsetof(SpcTracks, instrument) == 0x0713 - 0x0603, "instrument");
static_assert(offsetof(SpcTracks, mixed_volume) == 0x08B3 - 0x0603, "mixed");
static_assert(sizeof(SpcTracks) == SPC_SETTINGS_ADDRESS - SPC_TRACKS_ADDRESS, "tracks");
static_assert(sizeof(SpcSettings) == SPC_INSTRUMENTS_ADDRESS - SPC_SETTINGS_ADDRESS,
              "settings");
static_assert(offsetof(SpcRam, layout.tracks) == SPC_TRACKS_ADDRESS, "tracks at");
static_assert(offsetof(SpcRam, layout.instruments) == SPC_INSTRUMENTS_ADDRESS,
              "instruments at");

#endif
