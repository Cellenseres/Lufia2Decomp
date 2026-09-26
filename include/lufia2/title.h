#ifndef LUFIA2_TITLE_H
#define LUFIA2_TITLE_H

/* Intro logos and title flow. */

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

/* $80:92A4 intro NMI, logo state machine. */
Lufia2ExecutionResult Lufia2IntroNmi(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:86ED title layer scroll and tile animation; M1X0. */
Lufia2ExecutionResult Lufia2TitleLayers(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:8996 title palette cycle; entry M1X0. */
Lufia2ExecutionResult Lufia2TitlePaletteCycle(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:838C the three title objects; entry M1X0. */
Lufia2ExecutionResult Lufia2TitleObjects(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:88BE title particle sprites; entry M1X0. */
Lufia2ExecutionResult Lufia2TitleParticleSprites(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:E746 title state jump; handlers stay LLE. */
Lufia2ExecutionResult Lufia2TitleStateDispatch(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
