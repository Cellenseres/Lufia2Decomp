#ifndef LUFIA2_SPC_DRIVER_H
#define LUFIA2_SPC_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Native rebuild of the Lufia II SPC-700 sound driver (uploaded from
 * $90:C000, entry $0300).
 *
 * The driver runs as if the sound CPU were infinitely fast: the host calls
 * Lufia2SpcDriverRun after every change the original could react to (a CPU
 * port write, a timer tick) and the call returns once the driver waits for
 * the next one. ARAM keeps the original layout and contents, the DSP sees
 * the original register writes in the original order, and the CPU sees the
 * original port protocol.
 */

/* The sound hardware the driver programs. */
typedef struct Lufia2SpcHardware {
    void *context;
    /* CPU-to-sound ports, $F4-$F7. */
    uint8_t (*read_port)(void *context, unsigned port);
    /* Sound-to-CPU ports, $F4-$F7. */
    void (*write_port)(void *context, unsigned port, uint8_t value);
    uint8_t (*read_dsp)(void *context, uint8_t reg);
    void (*write_dsp)(void *context, uint8_t reg, uint8_t value);
    /* The original is busy this long before its next hardware access
     * (key-off settle, echo buffer clear); later accesses follow it. */
    void (*delay)(void *context, unsigned cycles);
    /* $F1: timer enables (bits 0-2, a 0-to-1 edge restarts the timer). */
    void (*write_control)(void *context, uint8_t value);
    void (*write_timer_target)(void *context, unsigned timer, uint8_t target);
    /* $FD-$FF: ticks since the last read (4 bits), cleared by the read. */
    uint8_t (*read_timer)(void *context, unsigned timer);
} Lufia2SpcHardware;

/* Where the driver waits; mirrors the polling loops of the original. */
typedef enum Lufia2SpcWait {
    LUFIA2_SPC_WAIT_STARTUP_TIMER,  /* $4290 */
    LUFIA2_SPC_WAIT_STARTUP_SIGNAL, /* $0372, $0377 */
    LUFIA2_SPC_WAIT_IDLE,           /* $0388 */
    LUFIA2_SPC_WAIT_ACKNOWLEDGE,    /* $05F4, $05F8 */
    LUFIA2_SPC_WAIT_HEADER_BYTE,    /* $0518 */
    LUFIA2_SPC_WAIT_HEADER_END,     /* $0511 */
    LUFIA2_SPC_WAIT_BLOCK_START,    /* $0563, $05BC */
    LUFIA2_SPC_WAIT_BLOCK_DATA,     /* $056B, $05C7 */
} Lufia2SpcWait;

enum { LUFIA2_SPC_MAX_STEPS = 6 };

typedef struct Lufia2SpcDriver {
    uint8_t *aram;
    const Lufia2SpcHardware *hardware;
    Lufia2SpcWait wait;
    /* Queued work that has to wait for the CPU, in order. */
    uint8_t steps[LUFIA2_SPC_MAX_STEPS];
    uint8_t step_count;
    /* Progress of the blocking step at the head of the queue. */
    uint8_t substep;
    uint8_t counter;
    uint16_t cursor;
    uint16_t effect_pointer;
    uint8_t effect_id;
    /* A command without handshake stays posted; it reruns on new data. */
    uint8_t repeat_command;
    uint16_t repeat_arguments;
} Lufia2SpcDriver;

/* Starts the driver at its entry point; ARAM holds the uploaded image. */
void Lufia2SpcDriverStart(Lufia2SpcDriver *driver, uint8_t *aram,
                          const Lufia2SpcHardware *hardware);

/* Runs the driver until it waits for the CPU or a timer. */
void Lufia2SpcDriverRun(Lufia2SpcDriver *driver);

#ifdef __cplusplus
}
#endif

#endif
