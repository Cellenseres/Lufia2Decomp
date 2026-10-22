/* Global DSP state: voice masks, master and echo setup, DSP reset. */

#include "spc/spc_driver_internal.h"

enum {
    ECHO_FILTER_PRESETS = 0x3FDD, /* 8 coefficients per preset */
    DSP_RESET_TABLE = 0x432B,     /* register, value pairs; $FF ends */
    VOICE_DEFAULTS = 0x4323,      /* registers 0-7 of every voice */
    DSP_RESET_TABLE_END = 0xFF,
    STARTUP_WAIT_TICKS = 15,
    STARTUP_TIMER_TARGET = 0x80,
    ECHO_BYTES_PER_DELAY_STEP = 0x800,
};

/* Effects own their voices; music masks only reach voices music owns. */
static uint8_t CombinedMask(uint8_t music, uint8_t effects, uint8_t music_owned) {
    return (uint8_t)((music & music_owned) | effects);
}

void SpcWritePitchModulation(Lufia2SpcDriver *driver) {
    const SpcVariables *vars = SpcVars(driver);

    DspWrite(driver, DSP_PITCH_MODULATION,
             CombinedMask(vars->music_pitch_mod_voices, vars->effect_pitch_mod_voices,
                          vars->music_owned_voices));
}

void SpcWriteEchoEnable(Lufia2SpcDriver *driver) {
    const SpcVariables *vars = SpcVars(driver);

    DspWrite(driver, DSP_ECHO_ENABLE,
             CombinedMask(vars->music_echo_voices, vars->effect_echo_voices,
                          vars->music_owned_voices));
}

void SpcWriteNoiseEnable(Lufia2SpcDriver *driver) {
    const SpcVariables *vars = SpcVars(driver);

    DspWrite(driver, DSP_NOISE_ENABLE,
             CombinedMask(vars->music_noise_voices, vars->effect_noise_voices,
                          vars->music_owned_voices));
}

void SpcWriteMasterVolume(Lufia2SpcDriver *driver) {
    const uint8_t volume = SpcSetup(driver)->master_volume;

    DspWrite(driver, DSP_MASTER_VOLUME_RIGHT, volume);
    DspWrite(driver, DSP_MASTER_VOLUME_LEFT, volume);
}

/* Presets past the table read whatever follows it, like the original. */
void SpcWriteEchoFilter(Lufia2SpcDriver *driver) {
    uint8_t index = (uint8_t)(SpcSetup(driver)->echo_filter << 3);

    for (uint8_t reg = DSP_FILTER_COEFFICIENT; reg != 0x8F; reg += 0x10)
        DspWrite(driver, reg,
                 SpcReadByte(driver, (uint16_t)(ECHO_FILTER_PRESETS + index++)));
}

/* Echo level relative to the master volume. */
void SpcSetEchoVolume(Lufia2SpcDriver *driver, uint8_t level) {
    SpcSettings *settings = SpcSetup(driver);
    const uint8_t scaled = (uint8_t)((level * settings->master_volume) >> 7);

    settings->echo_volume_right = scaled;
    settings->echo_volume_left = scaled;
}

/* Programs the echo delay and clears the echo buffer below $10000. */
void SpcSetupEcho(Lufia2SpcDriver *driver) {
    SpcSettings *settings = SpcSetup(driver);

    DspWrite(driver, DSP_ECHO_ENABLE, 0);
    DspWrite(driver, DSP_ECHO_VOLUME_RIGHT, 0);
    DspWrite(driver, DSP_ECHO_VOLUME_LEFT, 0);
    DspWrite(driver, DSP_FLAGS,
             (uint8_t)(settings->dsp_flags | DSP_FLAG_ECHO_WRITE_OFF));
    DspWrite(driver, DSP_ECHO_DELAY, settings->echo_delay);
    if (settings->echo_delay == 0) {
        DspWrite(driver, DSP_ECHO_START, 0);
        return;
    }

    const uint8_t pages = (uint8_t)(settings->echo_delay << 3);
    const uint8_t start_page = (uint8_t)(~pages + 1);
    DspWrite(driver, DSP_ECHO_START, start_page);

    uint8_t page = start_page;
    uint8_t remaining = pages;
    do {
        for (unsigned offset = 0; offset < 0x100; ++offset)
            SpcWriteByte(driver, (uint16_t)((page << 8) | offset), 0);
        ++page;
    } while (--remaining != 0);

    settings->dsp_flags &= DSP_FLAG_NOISE_CLOCK;
    DspWrite(driver, DSP_FLAGS, settings->dsp_flags);
}

/* First half of the DSP reset, up to its wait on the tempo timer. */
void SpcResetDsp(Lufia2SpcDriver *driver) {
    const Lufia2SpcHardware *hardware = driver->hardware;

    hardware->write_control(hardware->context, 0);
    DspWrite(driver, DSP_FLAGS, 0xE0);
    for (uint8_t index = 0;; index = (uint8_t)(index + 2)) {
        const uint8_t reg = SpcReadByte(driver, (uint16_t)(DSP_RESET_TABLE + index));
        if (reg == DSP_RESET_TABLE_END)
            break;
        DspWrite(driver, reg,
                 SpcReadByte(driver, (uint16_t)(DSP_RESET_TABLE + index + 1)));
    }
    DspWrite(driver, DSP_ECHO_START, 0);
    DspWrite(driver, DSP_ECHO_DELAY, 0);
    for (uint8_t voice = 0; voice < SPC_MUSIC_TRACKS; ++voice) {
        for (uint8_t reg = 0; reg < 8; ++reg)
            DspWrite(driver, (uint8_t)((voice << 4) | reg),
                     SpcReadByte(driver, (uint16_t)(VOICE_DEFAULTS + reg)));
    }
    hardware->write_timer_target(hardware->context, SPC_TIMER_TEMPO,
                                 STARTUP_TIMER_TARGET);
    hardware->write_control(hardware->context, 0x01);
    driver->counter = STARTUP_WAIT_TICKS;
}

/* Second half, after the timer has fired fifteen times. */
void SpcFinishDspReset(Lufia2SpcDriver *driver) {
    const Lufia2SpcHardware *hardware = driver->hardware;
    SpcSettings *settings = SpcSetup(driver);

    hardware->write_control(hardware->context, 0);
    DspWrite(driver, DSP_FLAGS, DSP_FLAG_ECHO_WRITE_OFF);
    settings->dsp_flags = DSP_FLAG_ECHO_WRITE_OFF;
    settings->master_volume = 0x70;
    SpcWriteMasterVolume(driver);
    SpcSetEchoVolume(driver, 0x54);
    settings->echo_feedback = 0x50;
    settings->echo_filter = 0;
    settings->echo_delay = 0;
    SpcWriteEchoFilter(driver);
}
