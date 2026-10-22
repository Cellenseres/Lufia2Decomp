/* Driver start-up, main loop, CPU commands and the transfer protocol. */

#include "spc/spc_driver_internal.h"

#include <string.h>

/* CPU commands, as dispatched from the table at $03B8. */
enum {
    COMMAND_UPLOAD = 0x01,
    COMMAND_PLAY_SONG = 0x02,
    COMMAND_STOP_MUSIC = 0x03,
    COMMAND_PLAY_EFFECT = 0x04,
    COMMAND_STOP_EFFECTS = 0x05,
    COMMAND_FADE_OUT = 0x06,
    COMMAND_FADE_IN_SONG = 0x07,
    COMMAND_READ_PROGRESS = 0x08,
    COMMAND_MASTER_VOLUME = 0x09,
    COMMAND_FADE_TIME = 0x0A,
    COMMAND_MUSIC_VOLUME = 0x0B,
    COMMAND_EFFECT_VOLUME = 0x0C,
    COMMAND_OUTPUT_OPTIONS = 0x0D,
    COMMAND_RESET_DSP = 0x0E,
    COMMAND_READ_STATUS = 0x0F,
    COMMAND_MUSIC_VOICES = 0x10,
    COMMAND_SET_SAMPLE_END = 0x11,
    COMMAND_READ_SAMPLE_END = 0x12,
    COMMAND_SET_INSTRUMENT_SLOT = 0x13,
    COMMAND_UPLOAD_SAMPLE = 0x14,
    COMMAND_DOUBLE_TEMPO = 0x15,
    COMMAND_SET_UPLOAD_CURSOR = 0x16,
    COMMAND_UPLOAD_AT_CURSOR = 0x17,
    COMMAND_READ_OUTPUT_OPTIONS = 0x18,
    COMMAND_STOP_EFFECT = 0x19,
    COMMAND_MUSIC_TRACK_VOLUME = 0x1A,
    COMMAND_EFFECT_TRACK_VOLUME = 0x1B,
    COMMAND_POKE_AT_CURSOR = 0x1C,
    COMMAND_DEFINE_SAMPLE = 0x1D,
    COMMAND_READ_KEYED_VOICES = 0x1E,
    COMMAND_ECHO_FILTER = 0x1F,
    COMMAND_ECHO_FEEDBACK = 0x20,
    COMMAND_ECHO_VOLUME = 0x21,
    COMMAND_LIMIT = 0x22,
};

/* Work the driver queues while it waits for the CPU. */
enum {
    STEP_ACKNOWLEDGE = 1,
    STEP_RECEIVE_SAMPLE_HEADER,
    STEP_RECEIVE_AT_TRANSFER_CURSOR,
    STEP_RECEIVE_AT_UPLOAD_CURSOR,
    STEP_START_SONG,
    STEP_START_EFFECT,
    STEP_STOP_EFFECT_GROUP,
    STEP_ADVANCE_UPLOAD_CURSOR,
    STEP_RESET_DSP,
    STEP_FINISH_STARTUP,
};

enum {
    SAMPLE_HEADER_BYTES = 8,
    STARTUP_SIGNATURE_LOW = 0xAB,
    STARTUP_SIGNATURE_HIGH = 0xCD,
    STARTUP_REPLY_LOW = 0x67,
    STARTUP_REPLY_HIGH = 0x89,
    INVALID_COMMAND_TICKS = 256,
};

/* Substeps of a transfer. */
enum {
    ACK_WAIT_LOW,
    ACK_WAIT_HIGH,
    TRANSFER_WAIT_START,
    TRANSFER_WAIT_DATA,
    STARTUP_WAIT_LOW,
    STARTUP_WAIT_HIGH,
};

/* S-SMP DIV YA,X: quotients past 511 follow the chip's odd formula. */
uint8_t SpcDivide(uint16_t dividend, uint8_t divisor, uint8_t *remainder) {
    const uint8_t high = (uint8_t)(dividend >> 8);

    if (high < (divisor << 1)) {
        *remainder = (uint8_t)(dividend % divisor);
        return (uint8_t)(dividend / divisor);
    }
    const unsigned excess = dividend - ((unsigned)divisor << 9);
    *remainder = (uint8_t)(divisor + excess % (256u - divisor));
    return (uint8_t)(255u - excess / (256u - divisor));
}

static void QueueSteps(Lufia2SpcDriver *driver, const uint8_t *steps, unsigned count) {
    for (unsigned i = 0; i < count; ++i)
        driver->steps[driver->step_count++] = steps[i];
}

#define QUEUE(driver, ...)                                                             \
    do {                                                                               \
        const uint8_t queued_[] = {__VA_ARGS__};                                       \
        QueueSteps(driver, queued_, sizeof(queued_));                                  \
    } while (0)

static void FinishStep(Lufia2SpcDriver *driver) {
    memmove(driver->steps, driver->steps + 1, --driver->step_count);
    driver->substep = 0;
}

static uint16_t PortWord(Lufia2SpcDriver *driver) {
    return (uint16_t)(PortRead(driver, PORT_ARGUMENT) |
                      (PortRead(driver, PORT_ARGUMENT_HIGH) << 8));
}

/* $05EE: "done" handshake; the CPU answers by clearing both ports. */
static bool Acknowledge(Lufia2SpcDriver *driver) {
    switch (driver->substep) {
    case 0:
        PortWrite(driver, PORT_COMMAND, PROTOCOL_ACK_LOW);
        PortWrite(driver, PORT_COMMAND_CHECK, PROTOCOL_ACK_HIGH);
        driver->substep = ACK_WAIT_LOW + 1;
        /* fall through */
    case ACK_WAIT_LOW + 1:
        if (PortRead(driver, PORT_COMMAND) != 0) {
            driver->wait = LUFIA2_SPC_WAIT_ACKNOWLEDGE;
            return false;
        }
        driver->substep = ACK_WAIT_HIGH + 1;
        /* fall through */
    default:
        if (PortRead(driver, PORT_COMMAND_CHECK) != 0) {
            driver->wait = LUFIA2_SPC_WAIT_ACKNOWLEDGE;
            return false;
        }
        PortWrite(driver, PORT_COMMAND, 0);
        PortWrite(driver, PORT_COMMAND_CHECK, 0);
        return true;
    }
}

/* $04B8 after its handshake: length, loop offset and instrument record of
 * a sample, one byte per handshake, counted down from 8 on the CPU port. */
static bool ReceiveSampleHeader(Lufia2SpcDriver *driver) {
    SpcVariables *vars = SpcVars(driver);
    uint8_t *header = &vars->scratch[0];

    if (driver->substep == 0) {
        driver->counter = SAMPLE_HEADER_BYTES;
        driver->substep = 1;
    }
    while (driver->counter != 0 && driver->substep == 1) {
        if (PortRead(driver, PORT_COMMAND) != driver->counter) {
            driver->wait = LUFIA2_SPC_WAIT_HEADER_BYTE;
            return false;
        }
        const uint8_t value = PortRead(driver, PORT_ARGUMENT);
        const unsigned index = SAMPLE_HEADER_BYTES - driver->counter;
        PortWrite(driver, PORT_COMMAND, (uint8_t)(driver->counter | 0x80));
        if (index < 4) {
            header[index] = value;
        } else {
            /* The instrument record fills in as the bytes arrive. */
            SpcWriteByte(driver,
                         (uint16_t)(SPC_INSTRUMENTS_ADDRESS + vars->instrument_offset +
                                    index - 4),
                         value);
        }
        --driver->counter;
    }

    if (driver->substep == 1) {
        const uint16_t length = (uint16_t)(header[0] | (header[1] << 8));
        const uint16_t loop = (uint16_t)(header[2] | (header[3] << 8));
        const uint8_t slot = vars->instrument_offset;
        const uint16_t start = SpcWordValue(vars->sample_alloc);
        SpcDirectoryEntry *entry = &SpcRamOf(driver)->layout.directory[slot >> 2];

        SpcWordSet(&entry->start, start);
        SpcWordSet(&vars->sample_alloc, (uint16_t)(start + length));
        SpcWordSet(&entry->loop, (uint16_t)(start + loop));
        vars->instrument_offset = (uint8_t)(slot + 4);
        driver->cursor = start;
        driver->substep = 2;
    }
    if (PortRead(driver, PORT_COMMAND) != PROTOCOL_END) {
        driver->wait = LUFIA2_SPC_WAIT_HEADER_END;
        return false;
    }
    return true;
}

/* $05BA/$0563: two bytes per handshake until the CPU sends the end mark.
 * The cursor's high byte advances in ARAM when the index wraps. */
static bool ReceiveBlock(Lufia2SpcDriver *driver, SpcWord *cursor) {
    if (driver->substep == 0) {
        if (PortRead(driver, PORT_COMMAND) != PROTOCOL_END) {
            driver->wait = LUFIA2_SPC_WAIT_BLOCK_START;
            return false;
        }
        PortWrite(driver, PORT_COMMAND, PROTOCOL_END);
        driver->counter = 0;
        driver->substep = 1;
    }
    for (;;) {
        if (PortRead(driver, PORT_COMMAND_CHECK) == PROTOCOL_END)
            return true;
        const uint8_t index = driver->counter;
        if (PortRead(driver, PORT_COMMAND) != (index & 0x7F)) {
            driver->wait = LUFIA2_SPC_WAIT_BLOCK_DATA;
            return false;
        }
        SpcWriteByte(driver, (uint16_t)(SpcWordValue(*cursor) + index),
                     PortRead(driver, PORT_ARGUMENT));
        if (++driver->counter == 0)
            ++cursor->hi;
        SpcWriteByte(driver, (uint16_t)(SpcWordValue(*cursor) + driver->counter),
                     PortRead(driver, PORT_ARGUMENT_HIGH));
        if (++driver->counter == 0)
            ++cursor->hi;
        PortWrite(driver, PORT_COMMAND, (uint8_t)(index | 0x80));
    }
}

/* Destination of an $01 or $14 transfer, kept in the original scratch. */
static SpcWord *TransferCursor(Lufia2SpcDriver *driver) {
    return (SpcWord *)&SpcVars(driver)->scratch[0x0C];
}

static void SetSampleCursor(Lufia2SpcDriver *driver) {
    SpcWordSet(TransferCursor(driver), driver->cursor);
}

static bool RunStep(Lufia2SpcDriver *driver, uint8_t step) {
    SpcVariables *vars = SpcVars(driver);

    switch (step) {
    case STEP_ACKNOWLEDGE:
        return Acknowledge(driver);
    case STEP_RECEIVE_SAMPLE_HEADER:
        if (!ReceiveSampleHeader(driver))
            return false;
        SetSampleCursor(driver);
        return true;
    case STEP_RECEIVE_AT_TRANSFER_CURSOR:
        return ReceiveBlock(driver, TransferCursor(driver));
    case STEP_RECEIVE_AT_UPLOAD_CURSOR:
        return ReceiveBlock(driver, &vars->upload_cursor);
    case STEP_START_SONG:
        SpcStartSong(driver);
        return true;
    case STEP_START_EFFECT:
        SpcStartEffect(driver, driver->effect_id);
        return true;
    case STEP_STOP_EFFECT_GROUP:
        SpcStopEffectGroup(driver, driver->effect_pointer);
        return true;
    case STEP_ADVANCE_UPLOAD_CURSOR:
        SpcWordSet(&vars->upload_cursor,
                   (uint16_t)(SpcWordValue(vars->upload_cursor) + 2));
        return true;
    case STEP_RESET_DSP:
        if (driver->substep == 0) {
            SpcResetDsp(driver);
            driver->substep = 1;
        }
        while (driver->counter != 0) {
            if (TimerRead(driver, SPC_TIMER_TEMPO) == 0) {
                driver->wait = LUFIA2_SPC_WAIT_STARTUP_TIMER;
                return false;
            }
            --driver->counter;
        }
        SpcFinishDspReset(driver);
        return true;
    case STEP_FINISH_STARTUP:
        if (driver->substep == 0) {
            SpcSettings *settings = SpcSetup(driver);
            settings->music_volume = 0xFF;
            settings->effect_volume = 0xFF;
            settings->music_fade = 0xFF;
            settings->double_tempo = 0;
            vars->music_voice_enable = 0xFF;
            vars->music_owned_voices = 0xFF;
            settings->fade_step = 0x04;
            DspWrite(driver, DSP_KEY_OFF, 0xFF);
            DspDelay(driver, SPC_KEY_SETTLE_CYCLES);
            DspWrite(driver, DSP_KEY_OFF, 0);
            PortWrite(driver, PORT_ARGUMENT, STARTUP_SIGNATURE_LOW);
            PortWrite(driver, PORT_ARGUMENT_HIGH, STARTUP_SIGNATURE_HIGH);
            driver->substep = STARTUP_WAIT_LOW;
        }
        if (driver->substep == STARTUP_WAIT_LOW) {
            if (PortRead(driver, PORT_COMMAND) != STARTUP_REPLY_LOW) {
                driver->wait = LUFIA2_SPC_WAIT_STARTUP_SIGNAL;
                return false;
            }
            driver->substep = STARTUP_WAIT_HIGH;
        }
        if (PortRead(driver, PORT_COMMAND_CHECK) != STARTUP_REPLY_HIGH) {
            driver->wait = LUFIA2_SPC_WAIT_STARTUP_SIGNAL;
            return false;
        }
        driver->hardware->write_control(driver->hardware->context, 0x80);
        driver->hardware->write_timer_target(driver->hardware->context,
                                             SPC_TIMER_EFFECTS, 0x7D);
        driver->hardware->write_control(driver->hardware->context, 0x82);
        return true;
    }
    return true;
}

static void SetFadeLevel(Lufia2SpcDriver *driver, uint8_t level) {
    SpcSetup(driver)->music_fade = level;
}

static void RunCommand(Lufia2SpcDriver *driver, uint8_t command) {
    SpcVariables *vars = SpcVars(driver);
    SpcSettings *settings = SpcSetup(driver);
    uint8_t remainder;

    switch (command) {
    case COMMAND_UPLOAD:
        SpcWordSet(TransferCursor(driver), PortWord(driver));
        QUEUE(driver, STEP_ACKNOWLEDGE, STEP_RECEIVE_AT_TRANSFER_CURSOR,
              STEP_ACKNOWLEDGE);
        return;
    case COMMAND_PLAY_SONG:
        vars->status &= (uint8_t)~(SPC_STATUS_FADING_IN | SPC_STATUS_FADING_OUT);
        SetFadeLevel(driver, 0xFF);
        QUEUE(driver, STEP_ACKNOWLEDGE, STEP_START_SONG);
        return;
    case COMMAND_STOP_MUSIC:
        SpcStopMusic(driver);
        break;
    case COMMAND_PLAY_EFFECT:
        driver->effect_id = PortRead(driver, PORT_ARGUMENT);
        QUEUE(driver, STEP_ACKNOWLEDGE, STEP_START_EFFECT);
        return;
    case COMMAND_STOP_EFFECTS:
        SpcStopAllEffects(driver);
        break;
    case COMMAND_FADE_OUT:
        vars->status |= SPC_STATUS_FADING_OUT;
        vars->status &= (uint8_t)~SPC_STATUS_FADING_IN;
        break;
    case COMMAND_FADE_IN_SONG:
        vars->status |= SPC_STATUS_FADING_IN;
        vars->status &= (uint8_t)~SPC_STATUS_FADING_OUT;
        SetFadeLevel(driver, 0);
        QUEUE(driver, STEP_ACKNOWLEDGE, STEP_START_SONG);
        return;
    case COMMAND_READ_PROGRESS:
        PortWrite(driver, PORT_ARGUMENT, vars->patterns_played[0]);
        PortWrite(driver, PORT_ARGUMENT_HIGH, vars->keyed_voices);
        break;
    case COMMAND_MASTER_VOLUME:
        settings->master_volume = PortRead(driver, PORT_ARGUMENT);
        SpcWriteMasterVolume(driver);
        break;
    case COMMAND_FADE_TIME:
        settings->fade_step =
            SpcDivide(0x100, PortRead(driver, PORT_ARGUMENT), &remainder);
        break;
    case COMMAND_MUSIC_VOLUME:
        settings->music_volume = PortRead(driver, PORT_ARGUMENT);
        break;
    case COMMAND_EFFECT_VOLUME:
        settings->effect_volume = PortRead(driver, PORT_ARGUMENT);
        break;
    case COMMAND_OUTPUT_OPTIONS:
        vars->output_options = PortRead(driver, PORT_ARGUMENT);
        break;
    case COMMAND_RESET_DSP:
        QUEUE(driver, STEP_RESET_DSP, STEP_ACKNOWLEDGE);
        return;
    case COMMAND_READ_STATUS:
        PortWrite(driver, PORT_ARGUMENT, vars->status);
        break;
    case COMMAND_MUSIC_VOICES:
        vars->music_voice_enable = PortRead(driver, PORT_ARGUMENT);
        break;
    case COMMAND_SET_SAMPLE_END:
        SpcWordSet(&vars->sample_alloc, PortWord(driver));
        break;
    case COMMAND_READ_SAMPLE_END:
        PortWrite(driver, PORT_ARGUMENT, vars->sample_alloc.lo);
        PortWrite(driver, PORT_ARGUMENT_HIGH, vars->sample_alloc.hi);
        break;
    case COMMAND_SET_INSTRUMENT_SLOT:
        vars->instrument_offset = (uint8_t)(PortRead(driver, PORT_ARGUMENT) << 2);
        break;
    case COMMAND_UPLOAD_SAMPLE:
        QUEUE(driver, STEP_ACKNOWLEDGE, STEP_RECEIVE_SAMPLE_HEADER, STEP_ACKNOWLEDGE,
              STEP_RECEIVE_AT_TRANSFER_CURSOR, STEP_ACKNOWLEDGE);
        return;
    case COMMAND_DOUBLE_TEMPO:
        settings->double_tempo ^= 0xFF;
        break;
    case COMMAND_SET_UPLOAD_CURSOR:
        SpcWordSet(&vars->upload_cursor, PortWord(driver));
        vars->upload_started = 0xFF;
        break;
    case COMMAND_UPLOAD_AT_CURSOR:
        QUEUE(driver, STEP_ACKNOWLEDGE, STEP_RECEIVE_AT_UPLOAD_CURSOR,
              STEP_ACKNOWLEDGE);
        return;
    case COMMAND_READ_OUTPUT_OPTIONS:
        PortWrite(driver, PORT_ARGUMENT, vars->output_options);
        break;
    case COMMAND_STOP_EFFECT:
        driver->effect_pointer =
            SpcEffectGroupPointer(driver, PortRead(driver, PORT_ARGUMENT));
        QUEUE(driver, STEP_ACKNOWLEDGE, STEP_STOP_EFFECT_GROUP);
        return;
    case COMMAND_MUSIC_TRACK_VOLUME: {
        /* No handshake: the original repeats it while the port holds $1A. */
        const uint8_t track = PortRead(driver, PORT_ARGUMENT_HIGH);
        SpcWriteByte(driver, (uint16_t)(0x0723 + track),
                     PortRead(driver, PORT_ARGUMENT));
        SpcUpdateTrackVolume(driver, track);
        return;
    }
    case COMMAND_EFFECT_TRACK_VOLUME: {
        const uint8_t track = (uint8_t)(PortRead(driver, PORT_ARGUMENT_HIGH) + 8);
        SpcWriteByte(driver, (uint16_t)(0x0723 + track),
                     PortRead(driver, PORT_ARGUMENT));
        SpcUpdateTrackVolume(driver, track);
        break;
    }
    case COMMAND_POKE_AT_CURSOR: {
        const uint16_t cursor = SpcWordValue(vars->upload_cursor);
        SpcWriteByte(driver, cursor, PortRead(driver, PORT_ARGUMENT));
        SpcWriteByte(driver, (uint16_t)(cursor + 1),
                     PortRead(driver, PORT_ARGUMENT_HIGH));
        QUEUE(driver, STEP_ACKNOWLEDGE, STEP_ADVANCE_UPLOAD_CURSOR);
        return;
    }
    case COMMAND_DEFINE_SAMPLE:
        QUEUE(driver, STEP_ACKNOWLEDGE, STEP_RECEIVE_SAMPLE_HEADER, STEP_ACKNOWLEDGE);
        return;
    case COMMAND_READ_KEYED_VOICES:
        PortWrite(driver, PORT_ARGUMENT, vars->keyed_voices);
        break;
    case COMMAND_ECHO_FILTER:
        settings->echo_filter = PortRead(driver, PORT_ARGUMENT);
        SpcWriteEchoFilter(driver);
        break;
    case COMMAND_ECHO_FEEDBACK:
        settings->echo_feedback = PortRead(driver, PORT_ARGUMENT);
        DspWrite(driver, DSP_ECHO_FEEDBACK, settings->echo_feedback);
        break;
    case COMMAND_ECHO_VOLUME:
        SpcSetEchoVolume(driver, PortRead(driver, PORT_ARGUMENT));
        break;
    }
    QUEUE(driver, STEP_ACKNOWLEDGE);
}

/* One pass of the main loop at $0388; false when there was nothing to do. */
static bool RunMainLoop(Lufia2SpcDriver *driver) {
    SpcVariables *vars = SpcVars(driver);
    bool worked = false;

    if (vars->status & SPC_STATUS_MUSIC_PLAYING) {
        unsigned ticks = TimerRead(driver, SPC_TIMER_TEMPO);
        if (ticks != 0) {
            if (SpcSetup(driver)->double_tempo != 0)
                ticks = (uint8_t)(ticks << 1);
            while (ticks-- != 0)
                SpcMusicTick(driver);
            worked = true;
        }
    }
    if (TimerRead(driver, SPC_TIMER_EFFECTS) != 0) {
        SpcEffectsTick(driver);
        worked = true;
    }

    const uint8_t command = PortRead(driver, PORT_COMMAND);
    const bool posted = command != 0 && command == PortRead(driver, PORT_COMMAND_CHECK);
    if (!posted) {
        driver->repeat_command = 0;
        return worked;
    }
    /* An unacknowledged command reruns every pass; only a pass without
     * ticks and with the same data would change nothing. */
    const uint16_t arguments = PortWord(driver);
    if (!worked && command == driver->repeat_command &&
        arguments == driver->repeat_arguments)
        return false;
    if (command >= COMMAND_LIMIT) {
        /* An unknown command reruns the tempo loop with a stale count. */
        for (unsigned tick = 0; tick < INVALID_COMMAND_TICKS; ++tick)
            SpcMusicTick(driver);
    } else {
        RunCommand(driver, command);
    }
    if (command >= COMMAND_LIMIT || command == COMMAND_MUSIC_TRACK_VOLUME) {
        driver->repeat_command = command;
        driver->repeat_arguments = arguments;
    }
    return true;
}

void Lufia2SpcDriverRun(Lufia2SpcDriver *driver) {
    for (;;) {
        if (driver->step_count != 0) {
            if (!RunStep(driver, driver->steps[0]))
                return;
            FinishStep(driver);
            continue;
        }
        if (!RunMainLoop(driver)) {
            driver->wait = LUFIA2_SPC_WAIT_IDLE;
            return;
        }
    }
}

void Lufia2SpcDriverStart(Lufia2SpcDriver *driver, uint8_t *aram,
                          const Lufia2SpcHardware *hardware) {
    memset(driver, 0, sizeof(*driver));
    driver->aram = aram;
    driver->hardware = hardware;

    SpcVariables *vars = SpcVars(driver);
    memset((uint8_t *)vars + 1, 0, 0xEF);
    for (int track = SPC_TRACK_COUNT - 1; track >= 0; --track) {
        vars->track_flags[track] = 0;
        SpcResetTrack(driver, track);
    }
    memset(vars->effect_ids, 0, sizeof(vars->effect_ids));
    SpcSetup(driver)->effect_serial = 0;
    QUEUE(driver, STEP_RESET_DSP, STEP_FINISH_STARTUP, STEP_ACKNOWLEDGE);
}
